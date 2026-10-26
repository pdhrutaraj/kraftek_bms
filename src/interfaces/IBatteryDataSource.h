#pragma once
#include "../domain/BatteryTypes.h"
#include <QList>

class IBatteryDataSource {
public:
    virtual ~IBatteryDataSource() = default;
    virtual QList<BatteryReading> readBatteries() = 0;
    virtual BankStatus readBankStatus() = 0;
    virtual void setDemoScenario(const QString &) {}
    virtual void setAutomaticCycle(bool) {}
    virtual QString demoScenario() const { return QStringLiteral("Normal"); }
};
