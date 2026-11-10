#pragma once

#include "../domain/BatteryTypes.h"

#include <QList>
#include <QString>

enum class AlarmCode {
    BatteryUndervoltage,
    BatteryOvertemperature,
    BatterySensorFault,
    BankDataStale,
    InvalidBankMeasurement
};

enum class AlarmSeverity {
    Warning,
    Critical
};

struct AlarmRecord {
    AlarmCode code = AlarmCode::BatterySensorFault;
    AlarmSeverity severity = AlarmSeverity::Warning;
    int batteryId = -1;
    QString message;
};

struct AlarmEngineConfig {
    double undervoltageThresholdV = 11.5;
    double overtemperatureThresholdC = 45.0;
};

class AlarmEngine final {
public:
    explicit AlarmEngine(
        const AlarmEngineConfig &config = AlarmEngineConfig());

    QList<AlarmRecord> evaluate(
        const QList<BatteryReading> &readings,
        const BankStatus &bank) const;

private:
    AlarmEngineConfig config_;
};
