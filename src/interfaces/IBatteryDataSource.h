
#pragma once

#include "../domain/BatteryTypes.h"
#include <QList>
#include <QString>

class IBatteryDataSource {
public:
    virtual ~IBatteryDataSource() = default;

    virtual QList<BatteryReading> readBatteries() = 0;
    virtual BankStatus readBankStatus() = 0;

    virtual QString sourceId() const = 0;

    virtual bool supportsDemoScenarios() const {
        return false;
    }

    virtual void setDemoScenario(const QString &) {}
    virtual void setAutomaticCycle(bool) {}

    virtual QString demoScenario() const {
        return QStringLiteral("N/A");
    }
};
