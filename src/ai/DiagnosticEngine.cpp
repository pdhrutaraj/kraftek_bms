#include "DiagnosticEngine.h"
#include <QtMath>
#include <algorithm>

static double mean(const QList<double> &v) { if (v.isEmpty()) return 0.0; double s=0; for(double x:v)s+=x; return s/v.size(); }
static double sd(const QList<double> &v) { if(v.size()<4)return 0.0; const double m=mean(v); double s=0; for(double x:v)s+=(x-m)*(x-m); return qSqrt(s/(v.size()-1)); }
static void pushBounded(QList<double> &v,double x,int max=120){v.push_back(x);while(v.size()>max)v.pop_front();}
DiagnosticResult DiagnosticEngine::evaluate(const QList<BatteryReading> &readings, const BankStatus &bank) {
    DiagnosticResult out; ++sampleCount_;
    if (!readings.isEmpty()) {
        double minV=1000,maxV=-1000,maxT=-1000; int invalid=0;
        for(const auto &r:readings){minV=qMin(minV,r.voltageV);maxV=qMax(maxV,r.voltageV);maxT=qMax(maxT,r.temperatureC);if(!r.sensorValid)++invalid;}
        const double spread=maxV-minV;
        if(invalid) out.findings << QStringLiteral("%1 battery voltage/temperature sensor(s) report invalid status").arg(invalid);
        if(spread>0.25) out.findings << QStringLiteral("Block-voltage imbalance: %1 V spread across B1–B4").arg(spread,0,'f',2);
        if(maxT>=45.0) out.findings << QStringLiteral("High terminal temperature observed (%1 °C); verify approved OTP thresholds").arg(maxT,0,'f',1);
        if(bank.voltageV<40.0 && qAbs(bank.currentA)>200.0) out.findings << QStringLiteral("High-current voltage-sag pattern; correlate with crank event and reference limits");
    }
    if(recentBankVoltages_.size()>=10){double s=sd(recentBankVoltages_);if(s>0.05 && qAbs(bank.voltageV-mean(recentBankVoltages_))>3.0*s)out.findings<<QStringLiteral("Bank voltage deviates from recent statistical baseline");}
    if(recentCurrents_.size()>=10){double s=sd(recentCurrents_);if(s>1.0 && qAbs(bank.currentA-mean(recentCurrents_))>3.0*s)out.findings<<QStringLiteral("Bank current deviates from recent statistical baseline");}
    out.anomalyScore=qMin(100.0, out.findings.size()*25.0);
    if(!out.findings.isEmpty()){out.status=QStringLiteral("REVIEW");out.summary=out.findings.first();}
    else if(sampleCount_<10){out.status=QStringLiteral("BASELINING");out.summary=QStringLiteral("Building a rolling telemetry baseline (%1/10 samples)").arg(sampleCount_);}
    else {out.status=QStringLiteral("NORMAL");out.summary=QStringLiteral("No configured statistical anomaly detected");}
    pushBounded(recentBankVoltages_,bank.voltageV); pushBounded(recentCurrents_,bank.currentA);
    return out;
}
