
#include <QtTest>
#include <limits>

#include "services/BatteryHealthCalculator.h"

class BatteryHealthCalculatorTest : public QObject {
    Q_OBJECT

private:
    static BatteryReading battery(int id, double soh,
                                  double voltage = 12.6,
                                  double temperature = 25.0,
                                  bool sensorValid = true) {
        BatteryReading r;
        r.id = id;
        r.sohPct = soh;
        r.voltageV = voltage;
        r.temperatureC = temperature;
        r.sensorValid = sensorValid;
        return r;
    }

private slots:
    void emptyInput();
    void fourBatterySeriesBank();
    void weakestBatteryIsSelected();
    void invalidSensorIsExcluded();
    void invalidSohBoundariesAreRejected();
    void nonFiniteMeasurementsAreRejected();
    void sohBoundaryZeroAndHundredAreValid();
    void equalSohKeepsFirstBattery();
};

void BatteryHealthCalculatorTest::emptyInput() {
    const auto result = BatteryHealthCalculator::calculate({});

    QVERIFY(!result.available);
    QCOMPARE(result.validBatteryCount, 0);
    QCOMPARE(result.invalidBatteryCount, 0);
    QCOMPARE(result.weakestBatteryId, -1);
}

void BatteryHealthCalculatorTest::fourBatterySeriesBank() {
    const QList<BatteryReading> readings = {
        battery(1, 98.0),
        battery(2, 95.0),
        battery(3, 92.0),
        battery(4, 90.0)
    };

    const auto result = BatteryHealthCalculator::calculate(readings);

    QVERIFY(result.available);
    QCOMPARE(result.validBatteryCount, 4);
    QCOMPARE(result.invalidBatteryCount, 0);
    QCOMPARE(result.bankSohPct, 90.0);
    QCOMPARE(result.weakestBatteryId, 4);
}

void BatteryHealthCalculatorTest::weakestBatteryIsSelected() {
    const QList<BatteryReading> readings = {
        battery(1, 95.0, 12.60, 24.0),
        battery(2, 72.5, 12.30, 31.0),
        battery(3, 88.0, 12.55, 26.0),
        battery(4, 91.0, 12.58, 25.0)
    };

    const auto result = BatteryHealthCalculator::calculate(readings);

    QVERIFY(result.available);
    QCOMPARE(result.weakestBatteryId, 2);
    QCOMPARE(result.bankSohPct, 72.5);
    QCOMPARE(result.weakestSohPct, 72.5);
    QCOMPARE(result.weakestVoltageV, 12.30);
    QCOMPARE(result.weakestTemperatureC, 31.0);
}

void BatteryHealthCalculatorTest::invalidSensorIsExcluded() {
    const QList<BatteryReading> readings = {
        battery(1, 95.0),
        battery(2, 10.0, 12.0, 25.0, false),
        battery(3, 85.0)
    };

    const auto result = BatteryHealthCalculator::calculate(readings);

    QVERIFY(result.available);
    QCOMPARE(result.validBatteryCount, 2);
    QCOMPARE(result.invalidBatteryCount, 1);
    QCOMPARE(result.bankSohPct, 85.0);
    QCOMPARE(result.weakestBatteryId, 3);
}

void BatteryHealthCalculatorTest::invalidSohBoundariesAreRejected() {
    const QList<BatteryReading> readings = {
        battery(1, -0.01),
        battery(2, 100.01),
        battery(3, 80.0)
    };

    const auto result = BatteryHealthCalculator::calculate(readings);

    QVERIFY(result.available);
    QCOMPARE(result.validBatteryCount, 1);
    QCOMPARE(result.invalidBatteryCount, 2);
    QCOMPARE(result.bankSohPct, 80.0);
    QCOMPARE(result.weakestBatteryId, 3);
}

void BatteryHealthCalculatorTest::nonFiniteMeasurementsAreRejected() {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();

    const QList<BatteryReading> readings = {
        battery(1, nan),
        battery(2, 90.0, infinity, 25.0),
        battery(3, 85.0, 12.5, nan),
        battery(4, 80.0)
    };

    const auto result = BatteryHealthCalculator::calculate(readings);

    QVERIFY(result.available);
    QCOMPARE(result.validBatteryCount, 1);
    QCOMPARE(result.invalidBatteryCount, 3);
    QCOMPARE(result.bankSohPct, 80.0);
    QCOMPARE(result.weakestBatteryId, 4);
}

void BatteryHealthCalculatorTest::sohBoundaryZeroAndHundredAreValid() {
    const auto zero = BatteryHealthCalculator::calculate({
        battery(1, 0.0)
    });

    QVERIFY(zero.available);
    QCOMPARE(zero.validBatteryCount, 1);
    QCOMPARE(zero.bankSohPct, 0.0);

    const auto hundred = BatteryHealthCalculator::calculate({
        battery(1, 100.0)
    });

    QVERIFY(hundred.available);
    QCOMPARE(hundred.validBatteryCount, 1);
    QCOMPARE(hundred.bankSohPct, 100.0);
}

void BatteryHealthCalculatorTest::equalSohKeepsFirstBattery() {
    const auto result = BatteryHealthCalculator::calculate({
        battery(2, 80.0),
        battery(4, 80.0)
    });

    QVERIFY(result.available);
    QCOMPARE(result.bankSohPct, 80.0);
    QCOMPARE(result.weakestBatteryId, 2);
}

QTEST_GUILESS_MAIN(BatteryHealthCalculatorTest)

#include "tst_batteryhealthcalculator.moc"

