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

        return accepted;
    }

    bool waitForBytesWritten(int timeoutMs) override {
        Q_UNUSED(timeoutMs);
        return writeWaitResult;
    }

    qint64 bytesAvailable() const override {
        return available_.size();
    }

    bool waitForReadyRead(int timeoutMs) override {
        Q_UNUSED(timeoutMs);

        if (available_.isEmpty() && !pendingChunks_.isEmpty()) {
            available_ = pendingChunks_.dequeue();
        }

        return !available_.isEmpty();
    }

    QByteArray read(qint64 maxSize) override {
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
    int clearInputCalls = 0;

private:
    bool open_ = false;
    QString portName_;

    QByteArray available_;
    QQueue<QByteArray> pendingChunks_;
    QQueue<QQueue<QByteArray>> responseScripts_;
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
    void testPartialCommandWriteFails();
    void testReadWhenPortIsClosed();
    void testFragmentedResponse();
    void testMultipleRegisterReads();
    void testStaleInputIsCleared();
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

        QByteArray response;
        response.append(static_cast<char>(ack));
        response.append(char(0));
        response.append(char(0));
        response.append(char(85));

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
        QStringLiteral("received 2 of 4 bytes")));
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
        QStringLiteral("received 0 of 4 bytes")));
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


QTEST_GUILESS_MAIN(RxnBmsProtocolTest)

#include "tst_rxnbmsprotocol.moc"
