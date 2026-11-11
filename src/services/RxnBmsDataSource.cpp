
#include "RxnBmsDataSource.h"

QList<BatteryReading> RxnBmsDataSource::readBatteries()
{
    // No verified RXN protocol implementation is connected yet.
    // Return no readings rather than fabricated battery measurements.
    return {};
}

BankStatus RxnBmsDataSource::readBankStatus()
{
    BankStatus status;

    // Explicitly overwrite the nominal defaults in BatteryTypes.h.
    status.mode = QStringLiteral("48 V Series");
    status.voltageV = 0.0;
    status.currentA = 0.0;
    status.socPct = 0.0;
    status.sohPct = 0.0;
    status.dataFresh = false;
    status.charging = false;
    status.systemState = QStringLiteral("RXN BMS NOT CONNECTED");

    return status;
}
