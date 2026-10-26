#include "SimulatedDataSource.h"
#include <QtMath>
#include <QStringList>

static const QStringList scenarios = {QStringLiteral("Normal"), QStringLiteral("Voltage sag"),
    QStringLiteral("Block imbalance"), QStringLiteral("Temperature rise"), QStringLiteral("Sensor spike")};

void SimulatedDataSource::setDemoScenario(const QString &scenario) {
    if (scenarios.contains(scenario)) scenario_ = scenario;
}

QList<BatteryReading> SimulatedDataSource::readBatteries() {
    ++tick_;
    if (automaticCycle_ && tick_ % 12 == 0) {
        const int next = (scenarios.indexOf(scenario_) + 1) % scenarios.size();
        scenario_ = scenarios.at(next);
    }
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
        if (scenario_ == QStringLiteral("Block imbalance") && i == 2) r.voltageV -= 0.48;
        if (scenario_ == QStringLiteral("Temperature rise") && i == 1) r.temperatureC = 48.0 + 0.4 * qSin(tick_ / 3.0);
        if (scenario_ == QStringLiteral("Sensor spike") && i == 0) { r.sensorValid = false; r.voltageV += 0.9; }
        if (scenario_ == QStringLiteral("Voltage sag")) r.voltageV -= 3.05 + 0.05 * qSin(tick_ / 2.0);
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
    if (scenario_ == QStringLiteral("Voltage sag")) { s.voltageV = 38.2; s.currentA = -620.0; s.charging = false; s.systemState = QStringLiteral("SIMULATED CRANK SAG"); }
    return s;
}
