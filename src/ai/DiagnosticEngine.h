#pragma once
#include "../domain/BatteryTypes.h"
#include <QStringList>
#include <QList>

struct DiagnosticResult {
    QString status = QStringLiteral("MONITORING");
    QString summary = QStringLiteral("Collecting telemetry baseline");
    QString mlClass = QStringLiteral("NORMAL");
    QStringList findings;
    double anomalyScore = 0.0;
    double mlScore = 0.0;
};

// Hybrid synthetic-demo detector: transparent rules plus a tiny nearest-centroid
// classifier trained deterministically on generated feature vectors at startup.
// Advisory only; not safety-certified and not a physical protection signal.
class DiagnosticEngine final {
public:
    DiagnosticEngine();
    DiagnosticResult evaluate(const QList<BatteryReading> &readings, const BankStatus &bank);
private:
    struct Centroid { QString label; QList<double> values; };
    QList<double> features(const QList<BatteryReading> &readings, const BankStatus &bank) const;
    void trainSyntheticModel();
    QString classify(const QList<double> &x, double *score) const;
    QList<Centroid> centroids_;
    QList<double> recentBankVoltages_;
    QList<double> recentCurrents_;
    int sampleCount_ = 0;
};
