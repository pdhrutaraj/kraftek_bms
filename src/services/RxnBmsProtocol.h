#pragma once

#include <QByteArray>
#include <QSerialPort>
#include <QString>
#include <QVector>

// RXN Electric BMS Communication Protocol v2 (April 2024).
// This driver intentionally exposes raw register words and read-only transactions.
class RxnBmsProtocol final {
public:
    enum class Bank : quint8 {
        Configuration = 0b001,
        RealTime       = 0b010,
        Control        = 0b011,
        Historical     = 0b100
    };

    enum class Ack : quint8 {
        Success = 1,
        BankError = 30,
        AddressError = 31,
        DataError = 32,
        AccessError = 33,
        ChecksumError = 34
    };

    struct Result {
        bool ok = false;
        Ack ack = Ack::Success;
        QByteArray data;       // Raw register bytes, MSB first; scaling is not applied.
        QString error;
    };

    explicit RxnBmsProtocol(QObject *parent = nullptr);
    ~RxnBmsProtocol();

    bool open(const QString &portName, QString *error = nullptr);
    void close();
    bool isOpen() const;
    QString portName() const;

    // Safe API: only read commands are exposed. One register word = two response data bytes.
    Result readRegister(Bank bank, quint16 address, int responseDataBytes = 2);
    Result readRealTimeRegister(quint16 address, int responseDataBytes = 2);
    QVector<Result> readRealTimeRegisters(const QVector<quint16> &addresses);

    static QByteArray makeReadCommand(Bank bank, quint16 address);
    static QString ackDescription(quint8 value);

    // Register addresses explicitly listed in protocol v2. Values are raw uint16 words.
    static constexpr quint16 RegSsrStatus = 0;
    static constexpr quint16 RegBatteryVoltage = 15;
    static constexpr quint16 RegBatterySoc = 18;
    static constexpr quint16 RegBatteryCurrent = 19;
    static constexpr quint16 RegBatteryCurrentDirection = 20;
    static constexpr quint16 RegBmsTemperature = 70;
    static constexpr quint16 RegBatteryTemperature = 72;
    static constexpr quint16 RegCellVoltageArray = 101;
    static constexpr quint16 RegCellResistanceArray = 301;
    static constexpr quint16 RegCellSocArray = 501;

private:
    Result transactRead(Bank bank, quint16 address, int responseDataBytes);
    bool readExact(int count, QByteArray *out, QString *error);
    QSerialPort serial_;
    int timeoutMs_ = 50;
    static constexpr quint8 ProtocolChecksumByte = 85; // Document prints "85"; interpretation requires vendor confirmation.
};
