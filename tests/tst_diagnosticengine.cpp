#include <QtTest>
#include <limits>

#include "ai/DiagnosticEngine.h"

class DiagnosticEngineTest : public QObject {
    Q_OBJECT

private:
    static BatteryReading battery(
        int id, double voltage = 12.60,
        double temperature = 25.0, bool sensorValid = true) {

        BatteryReading r;
        r.id = id;
        r.voltageV = voltage;
        r.temperatureC = temperature;
        r.sensorValid = sensorValid;
        return r;
    }

    static QList<BatteryReading> normalReadings() {
        return {
            battery(1),
            battery(2),
            battery(3),
            battery(4)
        };
    }

    static BankStatus normalBank() {
        BankStatus bank;
        bank.dataFresh = true;
        bank.voltageV = 50.4;
        bank.currentA = 35.0;
        return bank;
    }

private slots:
    void staleBankIsUnavailable();
    void rejectedInputDoesNotAdvanceBaseline();
    void nonFiniteBankVoltageIsUnavailable();
    void nonFiniteBankCurrentIsUnavailable();
    void emptyInputIsInsufficient();
    void missingBatteryIsInsufficient();
    void duplicateBatteryIdIsInsufficient();
    void fewerThanTwoValidSensorsIsInsufficient();
    void validSensorWithNanVoltageIsInsufficient();
    void invalidSensorMeasurementsAreExcluded();
    void blockImbalanceIsReviewed();
    void highTemperatureIsReviewed();
    void highCurrentVoltageSagIsReviewed();
    void normalInputBuildsBaseline();
};

void DiagnosticEngineTest::staleBankIsUnavailable() {
    DiagnosticEngine engine;
    BankStatus bank = normalBank();
    bank.dataFresh = false;

    const auto result = engine.evaluate(normalReadings(), bank);

    QCOMPARE(result.status, QStringLiteral("UNAVAILABLE"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
    QCOMPARE(result.anomalyScore, 0.0);
    QCOMPARE(result.mlScore, 0.0);
}

void DiagnosticEngineTest::rejectedInputDoesNotAdvanceBaseline() {
    DiagnosticEngine engine;

    BankStatus stale = normalBank();
    stale.dataFresh = false;

    const auto rejected = engine.evaluate(normalReadings(), stale);
    QCOMPARE(rejected.status, QStringLiteral("UNAVAILABLE"));

    const auto accepted = engine.evaluate(normalReadings(), normalBank());
    QCOMPARE(accepted.status, QStringLiteral("BASELINING"));
    QVERIFY(accepted.summary.contains(QStringLiteral("(1/10 samples)")));
}

void DiagnosticEngineTest::nonFiniteBankVoltageIsUnavailable() {
    DiagnosticEngine engine;
    BankStatus bank = normalBank();
    bank.voltageV = std::numeric_limits<double>::quiet_NaN();

    const auto result = engine.evaluate(normalReadings(), bank);

    QCOMPARE(result.status, QStringLiteral("UNAVAILABLE"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
}

void DiagnosticEngineTest::nonFiniteBankCurrentIsUnavailable() {
    DiagnosticEngine engine;
    BankStatus bank = normalBank();
    bank.currentA = std::numeric_limits<double>::infinity();

    const auto result = engine.evaluate(normalReadings(), bank);

    QCOMPARE(result.status, QStringLiteral("UNAVAILABLE"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
}

void DiagnosticEngineTest::emptyInputIsInsufficient() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({}, normalBank());

    QCOMPARE(result.status, QStringLiteral("INSUFFICIENT_DATA"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
    QCOMPARE(result.anomalyScore, 0.0);
}

void DiagnosticEngineTest::missingBatteryIsInsufficient() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1), battery(2), battery(4)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("INSUFFICIENT_DATA"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
}

void DiagnosticEngineTest::duplicateBatteryIdIsInsufficient() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1), battery(2), battery(2), battery(4)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("INSUFFICIENT_DATA"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
}

void DiagnosticEngineTest::fewerThanTwoValidSensorsIsInsufficient() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1, 12.6, 25.0, false),
        battery(2, 12.6, 25.0, false),
        battery(3, 12.6, 25.0, false),
        battery(4)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("INSUFFICIENT_DATA"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
}

void DiagnosticEngineTest::validSensorWithNanVoltageIsInsufficient() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1, std::numeric_limits<double>::quiet_NaN()),
        battery(2),
        battery(3),
        battery(4)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("INSUFFICIENT_DATA"));
    QCOMPARE(result.mlClass, QStringLiteral("NOT RUN"));
}

void DiagnosticEngineTest::invalidSensorMeasurementsAreExcluded() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1, std::numeric_limits<double>::quiet_NaN(),
                std::numeric_limits<double>::infinity(), false),
        battery(2),
        battery(3),
        battery(4)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("REVIEW"));
    QCOMPARE(result.mlClass, QStringLiteral("SENSOR_ANOMALY"));
    QVERIFY(result.anomalyScore > 0.0);

    QVERIFY(result.summary.contains(QStringLiteral("invalid")));
    for (const QString &finding : result.findings) {
        QVERIFY(!finding.contains(QStringLiteral("Block-voltage imbalance")));
        QVERIFY(!finding.contains(QStringLiteral("High terminal temperature")));
    }
}

void DiagnosticEngineTest::blockImbalanceIsReviewed() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1, 12.60),
        battery(2, 12.60),
        battery(3, 12.95),
        battery(4, 12.60)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("REVIEW"));
    QVERIFY(result.summary.contains(QStringLiteral("Block-voltage imbalance")));
}

void DiagnosticEngineTest::highTemperatureIsReviewed() {
    DiagnosticEngine engine;

    const auto result = engine.evaluate({
        battery(1, 12.60, 48.0),
        battery(2),
        battery(3),
        battery(4)
    }, normalBank());

    QCOMPARE(result.status, QStringLiteral("REVIEW"));
    QVERIFY(result.summary.contains(QStringLiteral("High terminal temperature")));
}

void DiagnosticEngineTest::highCurrentVoltageSagIsReviewed() {
    DiagnosticEngine engine;
    BankStatus bank = normalBank();
    bank.voltageV = 38.2;
    bank.currentA = -620.0;

    const auto result = engine.evaluate({
        battery(1, 9.55),
        battery(2, 9.55),
        battery(3, 9.55),
        battery(4, 9.55)
    }, bank);

    QCOMPARE(result.status, QStringLiteral("REVIEW"));
    QVERIFY(result.summary.contains(QStringLiteral("voltage-sag")));
}

void DiagnosticEngineTest::normalInputBuildsBaseline() {
    DiagnosticEngine engine;
    const auto readings = normalReadings();
    const auto bank = normalBank();

    for (int i = 0; i < 9; ++i) {
        const auto result = engine.evaluate(readings, bank);
        QCOMPARE(result.status, QStringLiteral("BASELINING"));
        QCOMPARE(result.mlClass, QStringLiteral("NORMAL"));
    }

    const auto tenth = engine.evaluate(readings, bank);
    QCOMPARE(tenth.status, QStringLiteral("NORMAL"));
    QCOMPARE(tenth.mlClass, QStringLiteral("NORMAL"));
}

QTEST_GUILESS_MAIN(DiagnosticEngineTest)

#include "tst_diagnosticengine.moc"
