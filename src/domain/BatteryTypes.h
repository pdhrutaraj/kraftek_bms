#pragma once
#include <QString>
#include <QDateTime>

// Telemetry structures for the specified 48 V lead-acid cranking bank.
// Values from SimulatedDataSource are illustrative only.
struct BatteryReading {
    int id = 1;                  // B1..B4, each nominally 12 V / 180 Ah
    double voltageV = 12.60;
    double temperatureC = 25.0;  // external NTC at battery terminal lug
    double socPct = 92.0;
    double sohPct = 98.0;
    bool sensorValid = true;
    QDateTime timestamp = QDateTime::currentDateTime();
};

struct BankStatus {
    QString mode = QStringLiteral("48 V Series");
    double voltageV = 50.40;
    // Demo convention: positive = charging, negative = discharge/cranking.
    double currentA = 35.0;
    double socPct = 92.0;
    double sohPct = 98.0;
    bool dataFresh = true;
    bool charging = true;
    QString systemState = QStringLiteral("NORMAL");
};
