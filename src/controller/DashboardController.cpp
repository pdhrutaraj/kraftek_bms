#include "DashboardController.h"
#include <QDebug>

DashboardController::DashboardController(BatteryModel *model, std::unique_ptr<IBatteryDataSource> source, QObject *parent)
    : QObject(parent), model_(model), source_(std::move(source)), database_() {}

void DashboardController::refresh() {
    const auto readings = source_->readBatteries();
    model_->setReadings(readings);
    const BankStatus s = source_->readBankStatus();
    const QString scenario = source_->demoScenario();
    const DiagnosticResult d = diagnostics_.evaluate(readings, s);
    qint64 snapshotId = -1;
    const bool stored = database_.saveSnapshot(readings, s, QStringLiteral("SYNTHETIC_DEMO"), scenario, &snapshotId);
    bool aiStored = false;
    if (stored) aiStored = database_.saveDiagnostic(snapshotId, scenario, d.status, d.summary, d.mlClass, d.anomalyScore, d.mlScore);
    emit databaseStatusChanged(database_.isReady() && stored && aiStored
        ? QStringLiteral("CONNECTED · snapshot #%1 · AI result saved · %2").arg(snapshotId).arg(database_.databasePath())
        : QStringLiteral("DATABASE ISSUE · %1").arg(database_.lastError()));
    emit diagnosticsChanged(d.status, d.summary, d.anomalyScore);
    emit aiDemoChanged(scenario, d.mlClass, d.mlScore, d.summary);
    const QString diagnosticKey = scenario + QStringLiteral("|") + d.status + QStringLiteral("|") + d.summary;
    if ((!d.findings.isEmpty() || d.mlClass != QStringLiteral("NORMAL")) && diagnosticKey != lastLoggedDiagnostic_) {
        database_.logEvent(QStringLiteral("INFO"), QStringLiteral("SYNTHETIC_AI_ADVISORY"), d.summary, QStringLiteral("SyntheticDemo").append(QStringLiteral("/")).append(d.mlClass));
        lastLoggedDiagnostic_ = diagnosticKey;
    } else if (d.findings.isEmpty() && d.mlClass == QStringLiteral("NORMAL")) { lastLoggedDiagnostic_.clear(); }
    emit bankMetricsChanged(s.voltageV, s.currentA, s.socPct, s.sohPct,
                            s.systemState, s.dataFresh, s.mode);
}

void DashboardController::setDisplayMode(const QString &mode) { Q_UNUSED(mode); refresh(); }
void DashboardController::setDemoScenario(const QString &scenario) { source_->setDemoScenario(scenario); refresh(); }
void DashboardController::setAutomaticCycle(bool enabled) { source_->setAutomaticCycle(enabled); }
