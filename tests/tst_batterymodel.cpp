
#include <QtTest>

#include "model/BatteryModel.h"

class BatteryModelTest : public QObject {
    Q_OBJECT

private slots:
    void emptyModel();
    void modelDimensions();
    void displaysBatteryReadings();
    void displaysSensorFault();
    void headersAreCorrect();
    void invalidIndexesReturnEmptyValues();
    void setReadingsResetsModel();
    void childIndexesHaveNoRowsOrColumns();
};

void BatteryModelTest::emptyModel() {
    BatteryModel model;

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), int(BatteryModel::ColumnCount));
}

void BatteryModelTest::modelDimensions() {
    BatteryModel model;

    QList<BatteryReading> readings(4);
    for (int i = 0; i < readings.size(); ++i) {
        readings[i].id = i + 1;
    }

    model.setReadings(readings);

    QCOMPARE(model.rowCount(), 4);
    QCOMPARE(model.columnCount(), int(BatteryModel::ColumnCount));
}

void BatteryModelTest::displaysBatteryReadings() {
    BatteryModel model;

    BatteryReading r;
    r.id = 2;
    r.voltageV = 12.56;
    r.temperatureC = 27.4;
    r.socPct = 91.6;
    r.sohPct = 97.2;
    r.sensorValid = true;

    model.setReadings({r});

    QCOMPARE(model.data(model.index(0, BatteryModel::BatteryColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("BATTERY 2"));

    QCOMPARE(model.data(model.index(0, BatteryModel::VoltageColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("12.56 V"));

    QCOMPARE(model.data(model.index(0, BatteryModel::TemperatureColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("27.4 °C"));

    QCOMPARE(model.data(model.index(0, BatteryModel::SocColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("92%"));

    QCOMPARE(model.data(model.index(0, BatteryModel::SohColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("97%"));

    QCOMPARE(model.data(model.index(0, BatteryModel::StateColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("ONLINE"));
}

void BatteryModelTest::displaysSensorFault() {
    BatteryModel model;

    BatteryReading r;
    r.id = 3;
    r.sensorValid = false;

    model.setReadings({r});

    QCOMPARE(model.data(model.index(0, BatteryModel::StateColumn),
                        Qt::DisplayRole).toString(),
             QStringLiteral("SENSOR FAULT"));
}

void BatteryModelTest::headersAreCorrect() {
    BatteryModel model;

    QCOMPARE(model.headerData(BatteryModel::BatteryColumn,
                              Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("BATTERY ID"));

    QCOMPARE(model.headerData(BatteryModel::VoltageColumn,
                              Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("VOLTAGE"));

    QCOMPARE(model.headerData(BatteryModel::TemperatureColumn,
                              Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("TEMPERATURE"));

    QCOMPARE(model.headerData(BatteryModel::SocColumn,
                              Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("STATE OF CHARGE"));

    QCOMPARE(model.headerData(BatteryModel::SohColumn,
                              Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("STATE OF HEALTH"));

    QCOMPARE(model.headerData(BatteryModel::StateColumn,
                              Qt::Horizontal, Qt::DisplayRole).toString(),
             QStringLiteral("SENSOR STATUS"));

    QCOMPARE(model.headerData(0, Qt::Vertical, Qt::DisplayRole).toInt(), 1);
}

void BatteryModelTest::invalidIndexesReturnEmptyValues() {
    BatteryModel model;

    BatteryReading r;
    model.setReadings({r});

    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(4, 0), Qt::DisplayRole).isValid());
    QVERIFY(!model.data(model.index(0, BatteryModel::ColumnCount),
                        Qt::DisplayRole).isValid());
}

void BatteryModelTest::setReadingsResetsModel() {
    BatteryModel model;

    QSignalSpy spy(&model, &QAbstractItemModel::modelReset);
    QVERIFY(spy.isValid());

    BatteryReading r;
    model.setReadings({r});

    QCOMPARE(spy.count(), 1);
    QCOMPARE(model.rowCount(), 1);

    model.setReadings({});

    QCOMPARE(spy.count(), 2);
    QCOMPARE(model.rowCount(), 0);
}

void BatteryModelTest::childIndexesHaveNoRowsOrColumns() {
    BatteryModel model;

    BatteryReading r;
    model.setReadings({r});

    const QModelIndex parent = model.index(0, 0);

    QCOMPARE(model.rowCount(parent), 0);
    QCOMPARE(model.columnCount(parent), 0);
}

QTEST_GUILESS_MAIN(BatteryModelTest)

#include "tst_batterymodel.moc"

