
#include <QtTest>
#include <limits>

#include "services/AlarmEngine.h"

class AlarmEngineTest : public QObject {
    Q_OBJECT

private:
    static BatteryReading battery(
        int id,
        double voltage,
        double temperature,
        bool sensorValid = true)
    {
        BatteryReading r;
        r.id = id;
        r.voltageV = voltage;
        r.temperatureC = temperature;
        r.sensorValid = sensorValid;
        return r;
    }

private slots:
    void normalBankHasNoAlarms();
    void undervoltageDetected();
    void undervoltageBoundaryIsAllowed();
    void overtemperatureDetected();
    void overtemperatureBoundaryIsAllowed();
    void invalidSensorDetected();
    void multipleBatteryAlarmsDetected();
    void staleBankDataDetected();
    void invalidBankMeasurementDetected();
    void alarmsClearAfterRecovery();

    // Additional boundary and measurement tests.
    void nonFiniteBatteryVoltageDetected();
    void nonFiniteBatteryTemperatureDetected();
    void sameBatteryCanHaveMultipleAlarms();
    void customThresholdsAreApplied();
    void nonFiniteBankCurrentDetected();
};

void AlarmEngineTest::normalBankHasNoAlarms()
{
    AlarmEngine engine;

    const QList<BatteryReading> readings = {
        battery(1, 12.6, 25.0),
        battery(2, 12.6, 26.0),
        battery(3, 12.5, 27.0),
        battery(4, 12.6, 25.0)
    };

    BankStatus bank;

    const auto alarms = engine.evaluate(readings, bank);

    QVERIFY(alarms.isEmpty());
}

void AlarmEngineTest::undervoltageDetected()
{
    AlarmEngine engine;

    const auto alarms = engine.evaluate(
        {battery(2, 11.4, 25.0)},
        BankStatus{});

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code, AlarmCode::BatteryUndervoltage);
    QCOMPARE(alarms.first().severity, AlarmSeverity::Critical);
    QCOMPARE(alarms.first().batteryId, 2);
}

void AlarmEngineTest::undervoltageBoundaryIsAllowed()
{
    AlarmEngine engine;

    const auto alarms = engine.evaluate(
        {battery(1, 11.5, 25.0)},
        BankStatus{});

    QVERIFY(alarms.isEmpty());
}

void AlarmEngineTest::overtemperatureDetected()
{
    AlarmEngine engine;

    const auto alarms = engine.evaluate(
        {battery(3, 12.6, 45.1)},
        BankStatus{});

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code,
             AlarmCode::BatteryOvertemperature);
    QCOMPARE(alarms.first().batteryId, 3);
}

void AlarmEngineTest::overtemperatureBoundaryIsAllowed()
{
    AlarmEngine engine;

    const auto alarms = engine.evaluate(
        {battery(1, 12.6, 45.0)},
        BankStatus{});

    QVERIFY(alarms.isEmpty());
}

void AlarmEngineTest::invalidSensorDetected()
{
    AlarmEngine engine;

    const auto alarms = engine.evaluate(
        {battery(2, 12.6, 25.0, false)},
        BankStatus{});

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code, AlarmCode::BatterySensorFault);
    QCOMPARE(alarms.first().batteryId, 2);
}

void AlarmEngineTest::multipleBatteryAlarmsDetected()
{
    AlarmEngine engine;

    const QList<BatteryReading> readings = {
        battery(1, 11.2, 25.0),
        battery(2, 12.6, 46.0),
        battery(3, 11.0, 47.0),
        battery(4, 12.6, 25.0)
    };

    const auto alarms = engine.evaluate(readings, BankStatus{});

    QCOMPARE(alarms.size(), 4);
}

void AlarmEngineTest::staleBankDataDetected()
{
    AlarmEngine engine;

    BankStatus bank;
    bank.dataFresh = false;

    const auto alarms = engine.evaluate({}, bank);

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code, AlarmCode::BankDataStale);
    QCOMPARE(alarms.first().severity, AlarmSeverity::Warning);
}

void AlarmEngineTest::invalidBankMeasurementDetected()
{
    AlarmEngine engine;

    BankStatus bank;
    bank.voltageV = std::numeric_limits<double>::quiet_NaN();

    const auto alarms = engine.evaluate({}, bank);

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code,
             AlarmCode::InvalidBankMeasurement);
}

void AlarmEngineTest::alarmsClearAfterRecovery()
{
    AlarmEngine engine;

    const auto fault = engine.evaluate(
        {battery(1, 11.0, 25.0)},
        BankStatus{});

    QCOMPARE(fault.size(), 1);

    const auto recovered = engine.evaluate(
        {battery(1, 12.6, 25.0)},
        BankStatus{});

    QVERIFY(recovered.isEmpty());
}

void AlarmEngineTest::nonFiniteBatteryVoltageDetected()
{
    AlarmEngine engine;

    const double nan = std::numeric_limits<double>::quiet_NaN();

    const auto alarms = engine.evaluate(
        {battery(1, nan, 25.0)},
        BankStatus{});

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code, AlarmCode::BatterySensorFault);
    QCOMPARE(alarms.first().batteryId, 1);
}

void AlarmEngineTest::nonFiniteBatteryTemperatureDetected()
{
    AlarmEngine engine;

    const double infinity = std::numeric_limits<double>::infinity();

    const auto alarms = engine.evaluate(
        {battery(2, 12.6, infinity)},
        BankStatus{});

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code, AlarmCode::BatterySensorFault);
    QCOMPARE(alarms.first().batteryId, 2);
}

void AlarmEngineTest::sameBatteryCanHaveMultipleAlarms()
{
    AlarmEngine engine;

    const auto alarms = engine.evaluate(
        {battery(3, 11.0, 46.0)},
        BankStatus{});

    QCOMPARE(alarms.size(), 2);

    QCOMPARE(alarms.at(0).code, AlarmCode::BatteryUndervoltage);
    QCOMPARE(alarms.at(0).batteryId, 3);

    QCOMPARE(alarms.at(1).code, AlarmCode::BatteryOvertemperature);
    QCOMPARE(alarms.at(1).batteryId, 3);
}

void AlarmEngineTest::customThresholdsAreApplied()
{
    AlarmEngineConfig config;
    config.undervoltageThresholdV = 12.0;
    config.overtemperatureThresholdC = 40.0;

    AlarmEngine engine(config);

    const auto alarms = engine.evaluate(
        {battery(1, 11.9, 40.1)},
        BankStatus{});

    QCOMPARE(alarms.size(), 2);
    QCOMPARE(alarms.at(0).code, AlarmCode::BatteryUndervoltage);
    QCOMPARE(alarms.at(1).code, AlarmCode::BatteryOvertemperature);
}

void AlarmEngineTest::nonFiniteBankCurrentDetected()
{
    AlarmEngine engine;

    BankStatus bank;
    bank.currentA = std::numeric_limits<double>::infinity();

    const auto alarms = engine.evaluate({}, bank);

    QCOMPARE(alarms.size(), 1);
    QCOMPARE(alarms.first().code,
             AlarmCode::InvalidBankMeasurement);
}

QTEST_GUILESS_MAIN(AlarmEngineTest)

#include "tst_alarmengine.moc"

