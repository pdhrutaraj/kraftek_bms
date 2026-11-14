#pragma once
#include "../model/BatteryModel.h"
#include "../interfaces/IBatteryDataSource.h"
#include "../services/TelemetryDatabase.h"
#include "../services/BatteryHealthCalculator.h"
#include "../ai/DiagnosticEngine.h"
#include <QObject>
#include <QStringList>
#include <memory>

class DashboardController final : public QObject {
    Q_OBJECT
public:
    DashboardController(BatteryModel *model,
                       std::unique_ptr<IBatteryDataSource> source,
                       QObject *parent = nullptr);

    void refresh();
    QString sourceId() const;
    QList<HistoricalReading> queryHistory(const QDateTime &startUtc,
                                          const QDateTime &endUtc,
                                          int batteryId = 0);
    QList<HistoricalEvent> queryEvents(const QDateTime &startUtc,
                                       const QDateTime &endUtc,
                                       const QString &severity = QString(),
                                       int limit = 100);

public slots:
    void setDisplayMode(const QString &mode);
    void setDemoScenario(const QString &scenario);
    void setAutomaticCycle(bool enabled);

signals:
    void bankMetricsChanged(double voltageV, double currentA, double socPct, double sohPct,
                            const QString &systemState, bool dataFresh, const QString &mode);
    void displayModeChanged(const QString &mode);
    void batteryHealthChanged(bool available, double bankSohPct, int weakestBatteryId,
                              double weakestSohPct, double weakestVoltageV,
                              double weakestTemperatureC, int validBatteryCount,
                              int invalidBatteryCount);
    void databaseStatusChanged(const QString &status);
    void diagnosticsChanged(const QString &status, const QString &summary, double anomalyScore);
    void aiDemoChanged(const QString &scenario, const QString &mlClass,
                       double mlScore, double anomalyScore,
                       int validSensorCount, int invalidSensorCount,
                       const QStringList &findings, const QString &summary);

private:
    BatteryModel *model_;
    std::unique_ptr<IBatteryDataSource> source_;
    TelemetryDatabase database_;
    DiagnosticEngine diagnostics_;
    QString lastLoggedDiagnostic_;
    QString lastAcquisitionState_;
    bool automaticCycle_ = false;
};
