#pragma once
#include "../interfaces/IBatteryDataSource.h"

class SimulatedDataSource final : public IBatteryDataSource {
public:
    QList<BatteryReading> readBatteries() override;
    BankStatus readBankStatus() override;
    void setDemoScenario(const QString &scenario) override;
    void setAutomaticCycle(bool enabled) override { automaticCycle_ = enabled; }
    QString demoScenario() const override { return scenario_; }
private:
    int tick_ = 0;
    QString scenario_ = QStringLiteral("Normal");
    bool automaticCycle_ = false;
};
