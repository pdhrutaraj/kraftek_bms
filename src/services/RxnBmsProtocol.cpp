#include "RxnBmsProtocol.h"

#include <QElapsedTimer>
#include <QSerialPort>
#include <utility>

namespace {

class QtSerialRxnTransport final : public RxnBmsTransport {
public:
    bool open(const QString &name, QString *error) override {
        serial_.setPortName(name);
        serial_.setBaudRate(QSerialPort::Baud9600);
        serial_.setDataBits(QSerialPort::Data8);
        serial_.setParity(QSerialPort::NoParity);
        serial_.setStopBits(QSerialPort::OneStop);
        serial_.setFlowControl(QSerialPort::NoFlowControl);

        if (!serial_.open(QIODevice::ReadWrite)) {
            if (error) {
                *error = serial_.errorString();
            }
            return false;
        }

        serial_.clear(QSerialPort::AllDirections);
        return true;
    }

    void close() override {
        if (serial_.isOpen()) {
            serial_.close();
        }
    }

    bool isOpen() const override {
        return serial_.isOpen();
    }

    QString portName() const override {
        return serial_.portName();
    }

    void clearInput() override {
        serial_.clear(QSerialPort::Input);
    }

    qint64 write(const QByteArray &data) override {
        return serial_.write(data);
    }

    bool waitForBytesWritten(int timeoutMs) override {
        return serial_.waitForBytesWritten(timeoutMs);
    }

    qint64 bytesAvailable() const override {
        return serial_.bytesAvailable();
    }

    bool waitForReadyRead(int timeoutMs) override {
        return serial_.waitForReadyRead(timeoutMs);
    }

    QByteArray read(qint64 maxSize) override {
        return serial_.read(maxSize);
    }

    QString errorString() const override {
        return serial_.errorString();
    }

private:
    QSerialPort serial_;
};

std::unique_ptr<RxnBmsTransport> makeDefaultTransport() {
    return std::unique_ptr<RxnBmsTransport>(new QtSerialRxnTransport());
}

} // namespace

RxnBmsProtocol::RxnBmsProtocol(QObject *parent)
    : RxnBmsProtocol(makeDefaultTransport()) {
    // Retained for source compatibility with existing callers.
    // QSerialPort is owned by the transport, not parented to this argument.
    Q_UNUSED(parent);
}

RxnBmsProtocol::RxnBmsProtocol(
    std::unique_ptr<RxnBmsTransport> transport)
    : transport_(std::move(transport)) {
}

RxnBmsProtocol::~RxnBmsProtocol() {
    close();
}

bool RxnBmsProtocol::open(const QString &name, QString *error) {
    if (!transport_) {
        if (error) {
            *error = QStringLiteral("RXN transport is not available");
        }
        return false;
    }

    close();
    return transport_->open(name, error);
}

void RxnBmsProtocol::close() {
    if (transport_) {
        transport_->close();
    }
}

bool RxnBmsProtocol::isOpen() const {
    return transport_ && transport_->isOpen();
}

QString RxnBmsProtocol::portName() const {
    return transport_ ? transport_->portName() : QString();
}

QByteArray RxnBmsProtocol::makeReadCommand(Bank bank, quint16 address) {
    // B0: RW=0 and BS[2:0] in bits 6..4. B1:B2 address MSB first.
    const quint8 command =
        static_cast<quint8>((static_cast<quint8>(bank) & 0x07u) << 4u);

    QByteArray packet(8, char(0));
    packet[0] = static_cast<char>(command);
    packet[1] = static_cast<char>((address >> 8) & 0xff);
    packet[2] = static_cast<char>(address & 0xff);

    // B3:B4 are ignored dummy bytes for reads; B5:B6 are don't-care bytes.
    packet[7] = static_cast<char>(ProtocolChecksumByte);

    return packet;
}

bool RxnBmsProtocol::readExact(
    int count,
    QByteArray *out,
    QString *error) {

    out->clear();

    if (!transport_) {
        if (error) {
            *error = QStringLiteral("RXN transport is not available");
        }
        return false;
    }

    QElapsedTimer timer;
    timer.start();

    while (out->size() < count && timer.elapsed() < timeoutMs_) {
        if (transport_->bytesAvailable() == 0) {
            const int remaining =
                qMax(1, timeoutMs_ - static_cast<int>(timer.elapsed()));

            if (!transport_->waitForReadyRead(remaining)) {
                continue;
            }
        }

        *out += transport_->read(count - out->size());
    }

    if (out->size() != count) {
        if (error) {
            *error = QStringLiteral(
                "Response timeout/incomplete frame: received %1 of %2 "
                "bytes within %3 ms")
                .arg(out->size())
                .arg(count)
                .arg(timeoutMs_);
        }
        return false;
    }

    return true;
}

RxnBmsProtocol::Result RxnBmsProtocol::transactRead(
    Bank bank,
    quint16 address,
    int dataBytes) {

    Result result;

    if (!isOpen()) {
        result.error = QStringLiteral("Serial port is not open");
        return result;
    }

    if (dataBytes < 0 || dataBytes > 2048) {
        result.error =
            QStringLiteral("Invalid expected response data length");
        return result;
    }

    transport_->clearInput();

    const QByteArray request = makeReadCommand(bank, address);

    if (transport_->write(request) != request.size()
        || !transport_->waitForBytesWritten(timeoutMs_)) {

        result.error =
            QStringLiteral("Failed to transmit complete 8-byte command: %1")
                .arg(transport_->errorString());

        return result;
    }

    QByteArray frame;

    // Existing framing assumption: ACK + data + checksum.
    if (!readExact(dataBytes + 2, &frame, &result.error)) {
        return result;
    }

    const quint8 ackValue = static_cast<quint8>(frame.at(0));
    result.ack = static_cast<Ack>(ackValue);

    if (ackValue != static_cast<quint8>(Ack::Success)) {
        result.error =
            QStringLiteral("RXN BMS NACK %1: %2")
                .arg(ackValue)
                .arg(ackDescription(ackValue));

        return result;
    }

    const quint8 receivedChecksum =
        static_cast<quint8>(frame.at(frame.size() - 1));

    if (receivedChecksum != ProtocolChecksumByte) {
        result.ack = Ack::ChecksumError;

        result.error =
            QStringLiteral(
                "Checksum byte mismatch (received %1; protocol document "
                "prints 85). Confirm vendor checksum definition.")
                .arg(receivedChecksum);

        return result;
    }

    result.data = frame.mid(1, dataBytes);
    result.ok = true;

    return result;
}

RxnBmsProtocol::Result RxnBmsProtocol::readRegister(
    Bank bank,
    quint16 address,
    int bytes) {

    return transactRead(bank, address, bytes);
}

RxnBmsProtocol::Result RxnBmsProtocol::readRealTimeRegister(
    quint16 address,
    int bytes) {

    return transactRead(Bank::RealTime, address, bytes);
}

QVector<RxnBmsProtocol::Result>
RxnBmsProtocol::readRealTimeRegisters(
    const QVector<quint16> &addresses) {

    QVector<Result> results;
    results.reserve(addresses.size());

    for (quint16 address : addresses) {
        results.push_back(readRealTimeRegister(address, 2));
    }

    return results;
}

QString RxnBmsProtocol::ackDescription(quint8 value) {
    switch (static_cast<Ack>(value)) {
    case Ack::Success:
        return QStringLiteral("SUCCESS");

    case Ack::BankError:
        return QStringLiteral("BANK_ERR - invalid bank");

    case Ack::AddressError:
        return QStringLiteral("ADD_ERR - invalid register address");

    case Ack::DataError:
        return QStringLiteral("DATA_ERR - invalid data");

    case Ack::AccessError:
        return QStringLiteral("ACCESS_ERR - register not accessible/read-only");

    case Ack::ChecksumError:
        return QStringLiteral("CHECKSUM_ERR - invalid checksum");
    }

    return QStringLiteral("Unknown acknowledgement");
}
