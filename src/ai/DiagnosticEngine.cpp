#include "DiagnosticEngine.h"
#include <QtMath>
#include <algorithm>

static double mean(const QList<double> &v) { if (v.isEmpty()) return 0.0; double s=0; for(double x:v)s+=x; return s/v.size(); }
static double sd(const QList<double> &v) { if(v.size()<4)return 0.0; const double m=mean(v); double s=0; for(double x:v)s+=(x-m)*(x-m); return qSqrt(s/(v.size()-1)); }
static void pushBounded(QList<double> &v,double x,int max=120){v.push_back(x);while(v.size()>max)v.pop_front();}

DiagnosticEngine::DiagnosticEngine() { trainSyntheticModel(); }

QList<double> DiagnosticEngine::features(const QList<BatteryReading> &readings, const BankStatus &bank) const {
    double minV=1000.0, maxV=-1000.0, maxT=-1000.0; int invalid=0;
    for (const auto &r : readings) { minV=qMin(minV,r.voltageV); maxV=qMax(maxV,r.voltageV); maxT=qMax(maxT,r.temperatureC); if(!r.sensorValid) ++invalid; }
    if (readings.isEmpty()) { minV=0; maxV=0; maxT=0; }
    return {qMax(0.0,maxV-minV), qMax(0.0,maxT-30.0), double(invalid),
            qMax(0.0,45.0-bank.voltageV), qMax(0.0,qAbs(bank.currentA)-200.0)/500.0};
}

void DiagnosticEngine::trainSyntheticModel() {
    // Generate a deterministic labelled synthetic training set, then fit class centroids.
    // Feature order: block spread V, temperature excess C, invalid sensors, bank sag V, high-current factor.
    struct Prototype { QString label; QList<double> center; };
    const QList<Prototype> prototypes = {
        {QStringLiteral("NORMAL"), {0.06, 0.0, 0.0, 0.0, 0.0}},
        {QStringLiteral("BLOCK_IMBALANCE"), {0.50, 0.0, 0.0, 0.0, 0.0}},
        {QStringLiteral("TEMPERATURE_RISE"), {0.06, 18.0, 0.0, 0.0, 0.0}},
        {QStringLiteral("SENSOR_ANOMALY"), {0.90, 0.0, 1.0, 0.0, 0.0}},
        {QStringLiteral("VOLTAGE_SAG"), {0.08, 0.0, 0.0, 6.8, 0.84}}
    };
    const double jitter[] = {0.025, 0.8, 0.08, 0.25, 0.04};
    for (const auto &prototype : prototypes) {
        QList<double> sums(5, 0.0);
        const int sampleCount = 100;
        for (int n = 0; n < sampleCount; ++n) {
            for (int f = 0; f < 5; ++f) {
                // Deterministic bounded perturbation makes repeatable labelled examples without RNG dependencies.
                const double wave = qSin((n + 1) * (f + 2) * 0.173) * 0.65
                                  + qCos((n + 3) * (f + 1) * 0.071) * 0.35;
                sums[f] += qMax(0.0, prototype.center.at(f) + jitter[f] * wave);
            }
        }
        for (double &value : sums) value /= sampleCount;
        centroids_.push_back({prototype.label, sums});
    }
}

QString DiagnosticEngine::classify(const QList<double> &x, double *score) const {
    QString best = QStringLiteral("NORMAL"); double bestDistance = 1e100; double normalDistance = 1e100;
    for (const auto &c : centroids_) {
        double d = 0.0;
        for (int i=0; i<x.size() && i<c.values.size(); ++i) {
            // Scale dimensions so the voltage, temperature, sensor and current features are comparable.
            const double scale[] = {0.5, 15.0, 1.0, 7.0, 1.0};
            const double z = (x.at(i)-c.values.at(i))/scale[i]; d += z*z;
        }
        if (c.label == QStringLiteral("NORMAL")) normalDistance = d;
        if (d < bestDistance) { bestDistance = d; best = c.label; }
    }
    // This is a synthetic anomaly score based on distance from the normal centroid, not a probability.
    if (score) *score = qBound(0.0, 100.0 * (1.0 - qExp(-normalDistance / 2.0)), 100.0);
    return best;
}

DiagnosticResult DiagnosticEngine::evaluate(const QList<BatteryReading> &readings, const BankStatus &bank) {
    DiagnosticResult out; ++sampleCount_;
    const auto x = features(readings, bank);
    out.mlClass = classify(x, &out.mlScore);
    if (!readings.isEmpty()) {
        double minV=1000,maxV=-1000,maxT=-1000; int invalid=0;
        for(const auto &r:readings){minV=qMin(minV,r.voltageV);maxV=qMax(maxV,r.voltageV);maxT=qMax(maxT,r.temperatureC);if(!r.sensorValid)++invalid;}
        const double spread=maxV-minV;
        if(invalid) out.findings << QStringLiteral("Synthetic sensor anomaly: %1 sensor(s) marked invalid").arg(invalid);
        if(spread>0.25) out.findings << QStringLiteral("Block-voltage imbalance: %1 V spread across B1–B4").arg(spread,0,'f',2);
        if(maxT>=45.0) out.findings << QStringLiteral("Synthetic high terminal temperature (%1 °C); review approved thresholds").arg(maxT,0,'f',1);
        if(bank.voltageV<40.0 && qAbs(bank.currentA)>200.0) out.findings << QStringLiteral("Synthetic high-current voltage-sag pattern; not a trip command");
    }
    if(recentBankVoltages_.size()>=10){double s=sd(recentBankVoltages_);if(s>0.05 && qAbs(bank.voltageV-mean(recentBankVoltages_))>3.0*s)out.findings<<QStringLiteral("Bank voltage deviates from recent statistical baseline");}
    if(recentCurrents_.size()>=10){double s=sd(recentCurrents_);if(s>1.0 && qAbs(bank.currentA-mean(recentCurrents_))>3.0*s)out.findings<<QStringLiteral("Bank current deviates from recent statistical baseline");}
    const double statisticalScore = qMin(100.0, out.findings.size()*25.0);
    const bool mlAnomaly = out.mlClass != QStringLiteral("NORMAL");
    out.anomalyScore = qMax(statisticalScore, mlAnomaly ? qMax(35.0, out.mlScore) : 0.0);
    if(!out.findings.isEmpty() || mlAnomaly){
        out.status=QStringLiteral("REVIEW");
        out.summary = !out.findings.isEmpty() ? out.findings.first() : QStringLiteral("Synthetic ML classifier pattern: %1").arg(out.mlClass);
    } else if(sampleCount_<10){out.status=QStringLiteral("BASELINING");out.summary=QStringLiteral("Building a rolling telemetry baseline (%1/10 samples)").arg(sampleCount_);}
    else {out.status=QStringLiteral("NORMAL");out.summary=QStringLiteral("No configured statistical or synthetic-model anomaly detected");}
    pushBounded(recentBankVoltages_,bank.voltageV); pushBounded(recentCurrents_,bank.currentA);
    return out;
}
