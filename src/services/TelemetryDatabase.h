#pragma once
#include "../domain/BatteryTypes.h"
#include <QObject>
#include <QSqlDatabase>
#include <QString>
#include <QDateTime>
#include <QList>

struct HistoricalReading {
    QDateTime timestampUtc;
    QString dataSource;
    double bankVoltageV = 0.0;
    double bankCurrentA = 0.0;
    double bankSocPct = 0.0;
    double bankSohPct = 0.0;
    int batteryId = 0; // 0 means bank-level record
    double batteryVoltageV = 0.0;
    double batteryTemperatureC = 0.0;
    double batterySocPct = 0.0;
    double batterySohPct = 0.0;
    bool sensorValid = false;
};

struct HistoricalEvent {
    qint64 id = 0;
    QDateTime timestampUtc;
    QString severity;
    QString eventCode;
    QString message;
    QString source;
    bool acknowledged = false;
};

class TelemetryDatabase final : public QObject {
    Q_OBJECT
public:
    explicit TelemetryDatabase(QObject *parent = nullptr,
                               const QString &databasePath = QString());
    ~TelemetryDatabase() override;
    bool isReady() const { return ready_; }
    QString databasePath() const { return path_; }
    QString lastError() const { return lastError_; }
    bool saveSnapshot(const QList<BatteryReading> &readings, const BankStatus &bank,
                      const QString &dataSource, const QString &quality,
                      const QString &scenario, qint64 *snapshotId = nullptr);
    QList<HistoricalReading> queryHistory(const QDateTime &startUtc, const QDateTime &endUtc, int batteryId = 0);
    bool saveDiagnostic(qint64 snapshotId, const QString &scenario, const QString &status,
                        const QString &summary, const QString &mlClass, double statisticalScore, double mlScore);
    bool logEvent(const QString &severity, const QString &code, const QString &message,
                  const QString &source = QStringLiteral("HMI"));
    QList<HistoricalEvent> queryEvents(
        const QDateTime &startUtc,
        const QDateTime &endUtc,
        const QString &severity = QString(),
        int limit = 100);
private:
    bool initialize();
    QSqlDatabase db_;
    QString connectionName_;
    QString path_;
    QString lastError_;
    bool ready_ = false;
};
