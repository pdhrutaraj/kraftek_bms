#pragma once
#include "../domain/BatteryTypes.h"
#include <QList>
#include <QtMath>
#include <limits>

struct BatteryHealthSummary {
    bool available = false;
    double bankSohPct = 0.0;
    int weakestBatteryId = -1;
    double weakestSohPct = 0.0;
    double weakestVoltageV = 0.0;
    double weakestTemperatureC = 0.0;
    int validBatteryCount = 0;
    int invalidBatteryCount = 0;
};

class BatteryHealthCalculator final {
public:
    static BatteryHealthSummary calculate(const QList<BatteryReading> &readings) {
        BatteryHealthSummary out;
        double minimum = std::numeric_limits<double>::max();
        for (const BatteryReading &r : readings) {
            const bool valid = r.sensorValid && qIsFinite(r.sohPct) && r.sohPct >= 0.0 && r.sohPct <= 100.0
                && qIsFinite(r.voltageV) && qIsFinite(r.temperatureC);
            if (!valid) { ++out.invalidBatteryCount; continue; }
            ++out.validBatteryCount;
            if (r.sohPct < minimum) {
                minimum = r.sohPct;
                out.weakestBatteryId = r.id;
                out.weakestSohPct = r.sohPct;
                out.weakestVoltageV = r.voltageV;
                out.weakestTemperatureC = r.temperatureC;
            }
        }
        if (out.validBatteryCount > 0) {
            out.available = true;
            out.bankSohPct = minimum;
        }
        return out;
    }
};
