#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

// Byte-transport abstraction for the RXN protocol.
// Production uses QSerialPort; tests can inject a fake transport.
class RxnBmsTransport {
public:
    virtual ~RxnBmsTransport() = default;

    virtual bool open(const QString &portName, QString *error) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual QString portName() const = 0;

    virtual void clearInput() = 0;
    virtual qint64 write(const QByteArray &data) = 0;
    virtual bool waitForBytesWritten(int timeoutMs) = 0;
    virtual qint64 bytesAvailable() const = 0;
    virtual bool waitForReadyRead(int timeoutMs) = 0;
    virtual QByteArray read(qint64 maxSize) = 0;

    virtual QString errorString() const = 0;
};
