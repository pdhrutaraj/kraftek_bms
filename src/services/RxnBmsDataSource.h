
#pragma once

#include "../interfaces/IBatteryDataSource.h"

class RxnBmsDataSource final : public IBatteryDataSource {
public:
    QList<BatteryReading> readBatteries() override;
    BankStatus readBankStatus() override;

    QString sourceId() const override {
        return QStringLiteral("RXN_BMS");
    }

    bool supportsDemoScenarios() const override {
        return false;
    }

    QString demoScenario() const override {
        return QStringLiteral("N/A");
    }
};
