#include "DashboardController.h"
#include <QDebug>

DashboardController::DashboardController(BatteryModel *model, std::unique_ptr<IBatteryDataSource> source, QObject *parent)
    : QObject(parent), model_(model), source_(std::move(source)) {}

void DashboardController::refresh() {
    const auto readings = source_->readBatteries();
    model_->setReadings(readings);
    const BankStatus s = source_->readBankStatus();
    qint64 snapshotId = -1;
    const bool stored = database_.saveSnapshot(readings, s, QStringLiteral("SIMULATED"), &snapshotId);
    emit databaseStatusChanged(database_.isReady() && stored
        ? QStringLiteral("CONNECTED · SQLite snapshot #%1 · %2").arg(snapshotId).arg(database_.databasePath())
        : QStringLiteral("DATABASE UNAVAILABLE · %1").arg(database_.lastError()));
    const DiagnosticResult d = diagnostics_.evaluate(readings, s);
    emit diagnosticsChanged(d.status, d.summary, d.anomalyScore);
    const QString diagnosticKey = d.status + QStringLiteral("|") + d.summary;
    if (!d.findings.isEmpty() && diagnosticKey != lastLoggedDiagnostic_) {
        database_.logEvent(QStringLiteral("WARNING"), QStringLiteral("ML_ADVISORY"), d.summary, QStringLiteral("DiagnosticEngine"));
        lastLoggedDiagnostic_ = diagnosticKey;
    } else if (d.findings.isEmpty()) { lastLoggedDiagnostic_.clear(); }
    emit bankMetricsChanged(s.voltageV, s.currentA, s.socPct, s.sohPct,
                            s.systemState, s.dataFresh, s.mode);
}

void DashboardController::setDisplayMode(const QString &mode) {
    Q_UNUSED(mode);
    // The product topology is fixed: four 12 V / 180 Ah lead-acid batteries in series.
    refresh();
}
