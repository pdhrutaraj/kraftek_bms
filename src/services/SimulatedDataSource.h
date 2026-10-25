#pragma once
#include "../interfaces/IBatteryDataSource.h"

class SimulatedDataSource final : public IBatteryDataSource {
public:
    QList<BatteryReading> readBatteries() override;
    BankStatus readBankStatus() override;
private:
    int tick_ = 0;
};
