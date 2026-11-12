#include <QtTest>
#include <QQueue>

#include "services/RxnBmsProtocol.h"

#include <memory>

class FakeRxnBmsTransport final : public RxnBmsTransport {
public:
    bool open(const QString &name, QString *error) override {
        Q_UNUSED(error);
        portName_ = name;
        open_ = true;
        return true;
    }

    void close() override {
        open_ = false;
    }

    bool isOpen() const override {
        return open_;
    }

    QString portName() const override {
        return portName_;
    }

    void clearInput() override {
        ++clearInputCalls;
        available_.clear();
        pendingChunks_.clear();
    }

    qint64 write(const QByteArray &data) override {
        const qint64 accepted =
            partialWrite && !data.isEmpty()
                ? data.size() - 1
                : data.size();

        writtenData += data.left(static_cast<int>(accepted));

        if (!responseScripts_.isEmpty()) {
            pendingChunks_ = responseScripts_.dequeue();
        }

        queuedWriteBytes_ = accepted;
        return accepted;
    }

    qint64 bytesToWrite() const override {
        return queuedWriteBytes_;
    }

    bool waitForBytesWritten(int timeoutMs) override {
        Q_UNUSED(timeoutMs);
        ++writeWaitCalls;

        if (writeWaitResult) {
            if (bytesDrainedPerWait < 0) {
                queuedWriteBytes_ = 0;
            } else {
                queuedWriteBytes_ =
                    qMax<qint64>(0, queuedWriteBytes_ - bytesDrainedPerWait);
            }
        }

        return writeWaitResult;
    }

    qint64 bytesAvailable() const override {
        return available_.size();
    }

    bool waitForReadyRead(int timeoutMs) override {
        Q_UNUSED(timeoutMs);
        ++waitForReadyReadCalls;

        if (readWaitError) {
            return false;
        }

        if (available_.isEmpty() && !pendingChunks_.isEmpty()) {
            available_ = pendingChunks_.dequeue();
        }

        return !available_.isEmpty();
    }

    bool hasReadError() const override {
        return readWaitError;
    }

    QByteArray read(qint64 maxSize) override {
        ++readCalls;

        if (readReturnsEmptyWhenDataAvailable && !available_.isEmpty()) {
            return {};
        }

        const QByteArray chunk =
            available_.left(static_cast<int>(maxSize));

        available_.remove(0, chunk.size());
        return chunk;
    }

    QString errorString() const override {
        return QStringLiteral("fake transport error");
    }

    // Each request consumes one response script.
    // A script can contain one complete frame or multiple fragments.
    void enqueueResponse(const QList<QByteArray> &chunks) {
        QQueue<QByteArray> script;

        for (const QByteArray &chunk : chunks) {
            script.enqueue(chunk);
        }

        responseScripts_.enqueue(script);
    }

    void setResponse(const QByteArray &response) {
        enqueueResponse({response});
    }

    void setStaleInput(const QByteArray &data) {
        available_ = data;
    }

    QByteArray writtenData;
    bool partialWrite = false;
    bool writeWaitResult = true;
    qint64 bytesDrainedPerWait = -1;
    int clearInputCalls = 0;
    int writeWaitCalls = 0;
    int waitForReadyReadCalls = 0;
    int readCalls = 0;
    bool readReturnsEmptyWhenDataAvailable = false;
    bool readWaitError = false;

private:
    bool open_ = false;
    QString portName_;

    QByteArray available_;
    QQueue<QByteArray> pendingChunks_;
    QQueue<QQueue<QByteArray>> responseScripts_;
    qint64 queuedWriteBytes_ = 0;
};


class RxnBmsProtocolTest : public QObject {
    Q_OBJECT

private slots:
    void testReadCommandEncoding();
    void testSuccessfulReadResponse();
    void testErrorAcknowledgements();
    void testChecksumMismatch();
    void testIncompleteResponseTimeout();
    void testNoResponseTimeout();
    void testImmediateReadTransportError();
    void testReadMakesNoProgress();
    void testZeroLengthReadResponse();
    void testPartialCommandWriteFails();
    void testCommandWriteWaitFails();
    void testCommandWriteQueueDrainsInParts();
    void testInvalidBankRejectedBeforeTransmission();
    void testReadWhenPortIsClosed();
    void testFragmentedResponse();
    void testMultipleRegisterReads();
    void testStaleInputIsCleared();
    void testNackFrameDoesNotPoisonNextRead();
};


void RxnBmsProtocolTest::testReadCommandEncoding() {
    QCOMPARE(
        RxnBmsProtocol::makeReadCommand(
            RxnBmsProtocol::Bank::RealTime, 0x1234),
        QByteArray::fromHex("2012340000000055"));

    QCOMPARE(
        RxnBmsProtocol::makeReadCommand(
            RxnBmsProtocol::Bank::Configuration, 0x0001),
        QByteArray::fromHex("1000010000000055"));

    QCOMPARE(
        RxnBmsProtocol::makeReadCommand(
            RxnBmsProtocol::Bank::Control, 0xABCD),
        QByteArray::fromHex("30abcd0000000055"));

    QCOMPARE(
        RxnBmsProtocol::makeReadCommand(
            RxnBmsProtocol::Bank::Historical, 0x0000),
        QByteArray::fromHex("4000000000000055"));
}


void RxnBmsProtocolTest::testSuccessfulReadResponse() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    transport->setResponse(QByteArray::fromHex("01123455"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.data, QByteArray::fromHex("1234"));
    QCOMPARE(static_cast<int>(result.ack),
             static_cast<int>(RxnBmsProtocol::Ack::Success));

    QCOMPARE(transport->writtenData,
             QByteArray::fromHex("20000f0000000055"));
    QCOMPARE(transport->clearInputCalls, 1);
}


void RxnBmsProtocolTest::testErrorAcknowledgements() {
    const QList<int> errorAcks = {30, 31, 32, 33, 34};

    for (int ack : errorAcks) {
        auto fake = std::make_unique<FakeRxnBmsTransport>();

        // X3 unsuccessful read response is eight bytes:
        // ACK, command, address MSB/LSB, data MSB/LSB,
        // don't-care byte, checksum.
        QByteArray response;
        response.append(static_cast<char>(ack));
        response.append(char(0x20)); // RTD read command
        response.append(char(0x00)); // Address MSB
        response.append(char(0x0f)); // Address LSB: register 15
        response.append(char(0x00));
        response.append(char(0x00));
        response.append(char(0x00));
        response.append(char(85));   // Provisional response checksum

        fake->setResponse(response);

        RxnBmsProtocol protocol(std::move(fake));
        QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

        const auto result = protocol.readRealTimeRegister(15, 2);

        QVERIFY(!result.ok);
        QCOMPARE(static_cast<int>(result.ack), ack);
        QVERIFY(result.error.contains(QString::number(ack)));
    }
}


void RxnBmsProtocolTest::testChecksumMismatch() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    fake->setResponse(QByteArray::fromHex("01123456"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);
    QCOMPARE(static_cast<int>(result.ack),
             static_cast<int>(RxnBmsProtocol::Ack::ChecksumError));

    QVERIFY(result.error.contains(
        QStringLiteral("Checksum byte mismatch")));
}


void RxnBmsProtocolTest::testReadMakesNoProgress() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    // The fake reports an available ACK byte but returns an empty read.
    transport->setResponse(QByteArray::fromHex("01"));
    transport->readReturnsEmptyWhenDataAvailable = true;

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains(
        QStringLiteral("Transport read made no progress")));
    QVERIFY(result.error.contains(
        QStringLiteral("fake transport error")));

    QCOMPARE(transport->readCalls, 1);
    QCOMPARE(transport->waitForReadyReadCalls, 1);
    QVERIFY(result.rawResponse.isEmpty());
}


void RxnBmsProtocolTest::testZeroLengthReadResponse() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    // Zero data bytes: success ACK followed by the provisional checksum.
    transport->setResponse(QByteArray::fromHex("0155"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 0);

    QVERIFY2(result.ok, qPrintable(result.error));
    QVERIFY(result.data.isEmpty());
    QCOMPARE(result.rawResponse, QByteArray::fromHex("0155"));
    QCOMPARE(static_cast<int>(result.ack),
             static_cast<int>(RxnBmsProtocol::Ack::Success));
}


void RxnBmsProtocolTest::testIncompleteResponseTimeout() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    fake->setResponse(QByteArray::fromHex("0112"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains(
        QStringLiteral("timeout/incomplete frame")));

    QVERIFY(result.error.contains(
        QStringLiteral("received 1 of 3 bytes")));
}


void RxnBmsProtocolTest::testNoResponseTimeout() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();

    // No response is queued.
    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);

    QVERIFY(result.error.contains(
        QStringLiteral("timeout/incomplete frame")));

    QVERIFY(result.error.contains(
        QStringLiteral("received 0 of 1 bytes")));
}


void RxnBmsProtocolTest::testImmediateReadTransportError() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    transport->readWaitError = true;

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains(
        QStringLiteral("Transport read error")));
    QVERIFY(result.error.contains(
        QStringLiteral("fake transport error")));

    // A genuine transport error must not be retried until timeout.
    QCOMPARE(transport->waitForReadyReadCalls, 1);
    QCOMPARE(transport->readCalls, 0);
    QVERIFY(result.rawResponse.isEmpty());
}


void RxnBmsProtocolTest::testPartialCommandWriteFails() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    transport->partialWrite = true;

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);

    QVERIFY(result.error.contains(
        QStringLiteral("Failed to transmit")));

    // Fake transport accepted only seven of the eight bytes.
    QCOMPARE(transport->writtenData.size(), 7);
}


void RxnBmsProtocolTest::testCommandWriteWaitFails() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    transport->setResponse(QByteArray::fromHex("01123455"));
    transport->writeWaitResult = false;

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains(
        QStringLiteral("Command transmission timeout/error")));

    QCOMPARE(transport->writeWaitCalls, 1);
    QCOMPARE(transport->waitForReadyReadCalls, 0);
    QVERIFY(result.rawResponse.isEmpty());
}


void RxnBmsProtocolTest::testCommandWriteQueueDrainsInParts() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    transport->setResponse(QByteArray::fromHex("01123455"));
    transport->bytesDrainedPerWait = 2;

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.data, QByteArray::fromHex("1234"));

    // Eight queued bytes drained two bytes per successful wait.
    QCOMPARE(transport->writeWaitCalls, 4);
    QCOMPARE(transport->waitForReadyReadCalls, 1);
}


void RxnBmsProtocolTest::testInvalidBankRejectedBeforeTransmission() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto invalidBank =
        static_cast<RxnBmsProtocol::Bank>(0x05);

    const auto result = protocol.readRegister(invalidBank, 15, 2);

    QVERIFY(!result.ok);
    QCOMPARE(result.error, QStringLiteral("Invalid RXN register bank"));

    QVERIFY(transport->writtenData.isEmpty());
    QCOMPARE(transport->clearInputCalls, 0);
    QCOMPARE(transport->writeWaitCalls, 0);
    QCOMPARE(transport->waitForReadyReadCalls, 0);
}


void RxnBmsProtocolTest::testReadWhenPortIsClosed() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    RxnBmsProtocol protocol(std::move(fake));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY(!result.ok);

    QCOMPARE(result.error,
             QStringLiteral("Serial port is not open"));
}


void RxnBmsProtocolTest::testFragmentedResponse() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();

    // One response delivered as three separate fragments.
    fake->enqueueResponse({
        QByteArray::fromHex("01"),
        QByteArray::fromHex("12"),
        QByteArray::fromHex("3455")
    });

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.data, QByteArray::fromHex("1234"));
}


void RxnBmsProtocolTest::testMultipleRegisterReads() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    // First register: ACK + 0x1234 + checksum.
    transport->setResponse(QByteArray::fromHex("01123455"));

    // Second register: ACK + 0x5678 + checksum.
    transport->setResponse(QByteArray::fromHex("01567855"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const QVector<quint16> addresses = {15, 18};

    const auto results =
        protocol.readRealTimeRegisters(addresses);

    QCOMPARE(results.size(), 2);

    QVERIFY2(results.at(0).ok,
             qPrintable(results.at(0).error));

    QVERIFY2(results.at(1).ok,
             qPrintable(results.at(1).error));

    QCOMPARE(results.at(0).data, QByteArray::fromHex("1234"));
    QCOMPARE(results.at(1).data, QByteArray::fromHex("5678"));

    // Confirm both requests were sent in the requested order.
    QCOMPARE(
        transport->writtenData,
        QByteArray::fromHex(
            "20000f0000000055"
            "2000120000000055"));

    QCOMPARE(transport->clearInputCalls, 2);
}


void RxnBmsProtocolTest::testStaleInputIsCleared() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    // Simulate old bytes already present before a new request.
    transport->setStaleInput(QByteArray::fromHex("deadbeef"));

    // The actual response to the new request.
    transport->setResponse(QByteArray::fromHex("01abcd55"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto result = protocol.readRealTimeRegister(15, 2);

    QVERIFY2(result.ok, qPrintable(result.error));
    QCOMPARE(result.data, QByteArray::fromHex("abcd"));

    QCOMPARE(transport->clearInputCalls, 1);
}



void RxnBmsProtocolTest::testNackFrameDoesNotPoisonNextRead() {
    auto fake = std::make_unique<FakeRxnBmsTransport>();
    FakeRxnBmsTransport *transport = fake.get();

    // First transaction: complete eight-byte NACK response.
    transport->setResponse(QByteArray::fromHex(
        "1e20000f00000055"));

    // Second transaction: successful two-byte register response.
    transport->setResponse(QByteArray::fromHex(
        "01567855"));

    RxnBmsProtocol protocol(std::move(fake));
    QVERIFY(protocol.open(QStringLiteral("fake-rxn")));

    const auto first = protocol.readRealTimeRegister(15, 2);
    QVERIFY(!first.ok);
    QCOMPARE(static_cast<int>(first.ack),
             static_cast<int>(RxnBmsProtocol::Ack::BankError));
    QCOMPARE(first.rawResponse.size(), 8);

    const auto second = protocol.readRealTimeRegister(18, 2);
    QVERIFY2(second.ok, qPrintable(second.error));
    QCOMPARE(second.data, QByteArray::fromHex("5678"));
    QCOMPARE(second.rawResponse.size(), 4);

    QCOMPARE(transport->clearInputCalls, 2);
}



QTEST_GUILESS_MAIN(RxnBmsProtocolTest)

#include "tst_rxnbmsprotocol.moc"
