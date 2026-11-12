#pragma once

#include <QObject>
#include "RxnBmsTransport.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QString>
#include <QVector>

#include <memory>

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
        QByteArray data;
        QByteArray rawResponse; // Complete or partial RXN response for diagnostics.
        QString error;
    };
    // Normal production constructor: uses QSerialPort transport.
    explicit RxnBmsProtocol(QObject *parent = nullptr);

    // Injection constructor: intended for isolated tests.
    explicit RxnBmsProtocol(std::unique_ptr<RxnBmsTransport> transport);

    ~RxnBmsProtocol();

    bool open(const QString &portName, QString *error = nullptr);
    void close();
    bool isOpen() const;
    QString portName() const;

    Result readRegister(Bank bank, quint16 address, int responseDataBytes = 2);
    Result readRealTimeRegister(quint16 address, int responseDataBytes = 2);
    QVector<Result> readRealTimeRegisters(const QVector<quint16> &addresses);

    static QByteArray makeReadCommand(Bank bank, quint16 address);
    static QString ackDescription(quint8 value);

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
    bool readExact(
        int count,
        QByteArray *out,
        QString *error,
        QElapsedTimer *timer);

    std::unique_ptr<RxnBmsTransport> transport_;

    int timeoutMs_ = 50;

    // Current implementation assumption; vendor confirmation is pending.
    static constexpr quint8 ProtocolChecksumByte = 85;
};
