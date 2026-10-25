#pragma once
#include "../domain/BatteryTypes.h"
#include <QObject>
#include <QSqlDatabase>
#include <QString>

class TelemetryDatabase final : public QObject {
    Q_OBJECT
public:
    explicit TelemetryDatabase(QObject *parent = nullptr);
    ~TelemetryDatabase() override;
    bool isReady() const { return ready_; }
    QString databasePath() const { return path_; }
    QString lastError() const { return lastError_; }
    bool saveSnapshot(const QList<BatteryReading> &readings, const BankStatus &bank,
                      const QString &quality, qint64 *snapshotId = nullptr);
    bool logEvent(const QString &severity, const QString &code, const QString &message,
                  const QString &source = QStringLiteral("HMI"));
private:
    bool initialize();
    QSqlDatabase db_;
    QString connectionName_;
    QString path_;
    QString lastError_;
    bool ready_ = false;
};
