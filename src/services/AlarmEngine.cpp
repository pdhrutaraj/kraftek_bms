#include "AlarmEngine.h"

#include <QtMath>

AlarmEngine::AlarmEngine(const AlarmEngineConfig &config)
    : config_(config)
{
}

QList<AlarmRecord> AlarmEngine::evaluate(
    const QList<BatteryReading> &readings,
    const BankStatus &bank) const
{
    QList<AlarmRecord> alarms;

    // Evaluate each individual battery.
    for (const BatteryReading &r : readings) {

        // Reject invalid or non-finite sensor measurements.
        if (!r.sensorValid ||
            !qIsFinite(r.voltageV) ||
            !qIsFinite(r.temperatureC)) {

            alarms.append({
                AlarmCode::BatterySensorFault,
                AlarmSeverity::Critical,
                r.id,
                QStringLiteral(
                    "Battery %1 sensor fault or invalid measurement")
                    .arg(r.id)
            });

            continue;
        }

        // Individual battery undervoltage.
        if (r.voltageV < config_.undervoltageThresholdV) {

            alarms.append({
                AlarmCode::BatteryUndervoltage,
                AlarmSeverity::Critical,
                r.id,
                QStringLiteral(
                    "Battery %1 undervoltage: %2 V")
                    .arg(r.id)
                    .arg(r.voltageV, 0, 'f', 2)
            });
        }

        // Individual battery overtemperature.
        if (r.temperatureC > config_.overtemperatureThresholdC) {

            alarms.append({
                AlarmCode::BatteryOvertemperature,
                AlarmSeverity::Critical,
                r.id,
                QStringLiteral(
                    "Battery %1 overtemperature: %2 °C")
                    .arg(r.id)
                    .arg(r.temperatureC, 0, 'f', 1)
            });
        }
    }

    // Bank data freshness.
    if (!bank.dataFresh) {

        alarms.append({
            AlarmCode::BankDataStale,
            AlarmSeverity::Warning,
            -1,
            QStringLiteral("Battery bank telemetry is stale")
        });
    }

    // Validate bank measurements.
    if (!qIsFinite(bank.voltageV) ||
        !qIsFinite(bank.currentA)) {

        alarms.append({
            AlarmCode::InvalidBankMeasurement,
            AlarmSeverity::Critical,
            -1,
            QStringLiteral("Invalid battery bank voltage or current measurement")
        });
    }

    return alarms;
}
