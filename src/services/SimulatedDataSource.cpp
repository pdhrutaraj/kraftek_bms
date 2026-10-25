#include "SimulatedDataSource.h"
#include <QtMath>

QList<BatteryReading> SimulatedDataSource::readBatteries() {
    ++tick_;
    QList<BatteryReading> out;
    for (int i = 0; i < 4; ++i) {
        BatteryReading r;
        r.id = i + 1;
        r.voltageV = 12.60 + 0.025 * qSin((tick_ + i * 4) / 8.0);
        r.temperatureC = 28.0 + 1.1 * qSin((tick_ + i * 3) / 15.0);
        r.socPct = 91.0 + 1.3 * qSin((tick_ + i * 2) / 20.0);
        r.sohPct = 97.0 + 0.3 * qSin((tick_ + i) / 30.0);
        r.sensorValid = true;
        r.timestamp = QDateTime::currentDateTime();
        out.push_back(r);
    }
    return out;
}

BankStatus SimulatedDataSource::readBankStatus() {
    BankStatus s;
    s.mode = QStringLiteral("48 V Series");
    s.voltageV = 50.40;
    s.currentA = 35.0;
    s.socPct = 91.0;
    s.sohPct = 97.0;
    s.dataFresh = true;
    s.charging = true;
    s.systemState = QStringLiteral("NORMAL");
    return s;
}
