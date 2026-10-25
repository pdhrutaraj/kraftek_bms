#pragma once
#include "../domain/BatteryTypes.h"
#include <QStringList>
#include <QList>

struct DiagnosticResult {
    QString status = QStringLiteral("MONITORING");
    QString summary = QStringLiteral("Collecting telemetry baseline");
    QStringList findings;
    double anomalyScore = 0.0;
};

// Advisory statistical anomaly detector; not a trained or safety-certified model.
class DiagnosticEngine final {
public:
    DiagnosticResult evaluate(const QList<BatteryReading> &readings, const BankStatus &bank);
private:
    QList<double> recentBankVoltages_;
    QList<double> recentCurrents_;
    int sampleCount_ = 0;
};
