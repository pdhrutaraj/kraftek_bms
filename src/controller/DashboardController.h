#pragma once
#include "../model/BatteryModel.h"
#include "../interfaces/IBatteryDataSource.h"
#include "../services/TelemetryDatabase.h"
#include "../ai/DiagnosticEngine.h"
#include <QObject>
#include <memory>

class DashboardController final : public QObject {
    Q_OBJECT
public:
    DashboardController(BatteryModel *model, std::unique_ptr<IBatteryDataSource> source, QObject *parent = nullptr);
    void refresh();
public slots:
    void setDisplayMode(const QString &mode);
    void setDemoScenario(const QString &scenario);
    void setAutomaticCycle(bool enabled);
signals:
    void bankMetricsChanged(double voltageV, double currentA, double socPct, double sohPct,
                            const QString &systemState, bool dataFresh, const QString &mode);
    void displayModeChanged(const QString &mode);
    void databaseStatusChanged(const QString &status);
    void diagnosticsChanged(const QString &status, const QString &summary, double anomalyScore);
    void aiDemoChanged(const QString &scenario, const QString &mlClass, double mlScore, const QString &summary);
private:
    BatteryModel *model_;
    std::unique_ptr<IBatteryDataSource> source_;
    TelemetryDatabase database_;
    DiagnosticEngine diagnostics_;
    QString lastLoggedDiagnostic_;
};
