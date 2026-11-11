#include "DashboardController.h"

#include <cmath>

DashboardController::DashboardController(
    BatteryModel *model,
    std::unique_ptr<IBatteryDataSource> source,
    QObject *parent)
    : QObject(parent),
      model_(model),
      source_(std::move(source)),
      database_()
{
}

QString DashboardController::sourceId() const
{
    return source_ ? source_->sourceId() : QStringLiteral("NO_SOURCE");
}

void DashboardController::refresh()
{
    if (!source_ || !model_)
        return;

    const QString source = sourceId();
    const bool isSimulator = source_->supportsDemoScenarios();

    const auto readings = source_->readBatteries();
    BankStatus s = source_->readBankStatus();

    bool bankFinite =
        std::isfinite(s.voltageV) &&
        std::isfinite(s.currentA) &&
        std::isfinite(s.socPct) &&
        std::isfinite(s.sohPct);

    bool readingsFinite = !readings.isEmpty();

    for (const auto &r : readings) {
        if (!std::isfinite(r.voltageV) ||
            !std::isfinite(r.temperatureC) ||
            !std::isfinite(r.socPct) ||
            !std::isfinite(r.sohPct)) {
            readingsFinite = false;
            break;
        }
    }

    const bool dataAvailable = s.dataFresh && bankFinite && readingsFinite;
    s.dataFresh = dataAvailable;

    model_->setReadings(readings);

    if (!dataAvailable) {
        const QString state = source + QStringLiteral("|NO_DATA");

        if (lastAcquisitionState_ != state) {
            database_.logEvent(
                QStringLiteral("WARNING"),
                QStringLiteral("TELEMETRY_UNAVAILABLE"),
                source + QStringLiteral(": waiting for valid battery telemetry"),
                source);
            lastAcquisitionState_ = state;
        }

        emit batteryHealthChanged(false, 0.0, 0, 0.0, 0.0, 0.0, 0, 0);
        emit diagnosticsChanged(
            QStringLiteral("UNAVAILABLE"),
            QStringLiteral("Diagnostics are unavailable because valid telemetry has not been received."),
            0.0);
        emit aiDemoChanged(
            QStringLiteral("N/A"),
            QStringLiteral("N/A"),
            0.0,
            QStringLiteral("No valid telemetry. No AI analysis was performed."));

        emit databaseStatusChanged(
            database_.isReady()
                ? source + QStringLiteral(" · WAITING FOR TELEMETRY · ") + database_.databasePath()
                : QStringLiteral("DATABASE ISSUE · ") + database_.lastError());

        emit bankMetricsChanged(
            0.0, 0.0, 0.0, 0.0,
            s.systemState, false, s.mode);
        return;
    }

    if (!lastAcquisitionState_.isEmpty() &&
        lastAcquisitionState_ != source + QStringLiteral("|AVAILABLE")) {
        database_.logEvent(
            QStringLiteral("INFO"),
            QStringLiteral("TELEMETRY_RESTORED"),
            source + QStringLiteral(": valid telemetry is available again"),
            source);
    }
    lastAcquisitionState_ = source + QStringLiteral("|AVAILABLE");

    const BatteryHealthSummary health =
        BatteryHealthCalculator::calculate(readings);

    if (health.available)
        s.sohPct = health.bankSohPct;

    emit batteryHealthChanged(
        health.available, health.bankSohPct,
        health.weakestBatteryId, health.weakestSohPct,
        health.weakestVoltageV, health.weakestTemperatureC,
        health.validBatteryCount, health.invalidBatteryCount);

    bool degraded = false;
    for (const auto &r : readings) {
        if (!r.sensorValid) {
            degraded = true;
            break;
        }
    }

    const QString quality =
        degraded ? QStringLiteral("DEGRADED") : QStringLiteral("VALID");

    const QString scenario =
        isSimulator ? source_->demoScenario() : QStringLiteral("N/A");

    qint64 snapshotId = -1;
    const bool stored = database_.saveSnapshot(
        readings, s, source, quality, scenario, &snapshotId);

    bool aiStored = true;
    DiagnosticResult d;

    if (isSimulator) {
        d = diagnostics_.evaluate(readings, s);

        if (stored) {
            aiStored = database_.saveDiagnostic(
                snapshotId, scenario, d.status, d.summary,
                d.mlClass, d.anomalyScore, d.mlScore);
        } else {
            aiStored = false;
        }

        emit diagnosticsChanged(d.status, d.summary, d.anomalyScore);
        emit aiDemoChanged(scenario, d.mlClass, d.mlScore, d.summary);

        const QString diagnosticKey =
            scenario + QStringLiteral("|") + d.status +
            QStringLiteral("|") + d.summary;

        if ((!d.findings.isEmpty() ||
             d.mlClass != QStringLiteral("NORMAL")) &&
            diagnosticKey != lastLoggedDiagnostic_) {
            database_.logEvent(
                QStringLiteral("INFO"),
                QStringLiteral("SYNTHETIC_AI_ADVISORY"),
                d.summary,
                QStringLiteral("SIMULATOR/") + d.mlClass);
            lastLoggedDiagnostic_ = diagnosticKey;
        } else if (d.findings.isEmpty() &&
                   d.mlClass == QStringLiteral("NORMAL")) {
            lastLoggedDiagnostic_.clear();
        }
    } else {
        emit diagnosticsChanged(
            QStringLiteral("NOT RUN"),
            QStringLiteral("The synthetic demo diagnostic model is not applied to this data source."),
            0.0);
        emit aiDemoChanged(
            QStringLiteral("N/A"),
            QStringLiteral("NOT RUN"),
            0.0,
            QStringLiteral("No validated RXN field-data diagnostic model is configured."));
    }

    if (!stored) {
        emit databaseStatusChanged(
            QStringLiteral("DATABASE WRITE ISSUE · ") + database_.lastError());
    } else if (isSimulator && !aiStored) {
        emit databaseStatusChanged(
            QStringLiteral("TELEMETRY SAVED · AI RESULT NOT SAVED · snapshot #%1")
                .arg(snapshotId));
    } else if (isSimulator) {
        emit databaseStatusChanged(
            QStringLiteral("SIMULATOR · snapshot #%1 · AI demo result saved · %2")
                .arg(snapshotId).arg(database_.databasePath()));
    } else {
        emit databaseStatusChanged(
            source + QStringLiteral(" · telemetry snapshot #%1 · %2")
                .arg(snapshotId).arg(database_.databasePath()));
    }

    emit bankMetricsChanged(
        s.voltageV, s.currentA, s.socPct,
        health.available ? health.bankSohPct : s.sohPct,
        s.systemState, true, s.mode);
}

void DashboardController::setDisplayMode(const QString &mode)
{
    Q_UNUSED(mode);
    refresh();
}

void DashboardController::setDemoScenario(const QString &scenario)
{
    if (source_ && source_->supportsDemoScenarios()) {
        source_->setDemoScenario(scenario);
        refresh();
    }
}

void DashboardController::setAutomaticCycle(bool enabled)
{
    automaticCycle_ = enabled;

    if (source_ && source_->supportsDemoScenarios())
        source_->setAutomaticCycle(enabled);
}

QList<HistoricalReading> DashboardController::queryHistory(
    const QDateTime &startUtc,
    const QDateTime &endUtc,
    int batteryId)
{
    return database_.queryHistory(startUtc, endUtc, batteryId);
}
