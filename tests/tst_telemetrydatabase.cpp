#include <QtTest>
#include <QTemporaryDir>
#include <QFileInfo>

#include "services/TelemetryDatabase.h"

class TelemetryDatabaseTest : public QObject {
    Q_OBJECT

private:
    static QList<BatteryReading> sampleReadings()
    {
        QList<BatteryReading> readings;

        for (int i = 1; i <= 4; ++i) {
            BatteryReading r;
            r.id = i;
            r.voltageV = 12.60 + (i * 0.01);
            r.temperatureC = 25.0 + i;
            r.socPct = 90.0 + i;
            r.sohPct = 97.0 + i;
            r.sensorValid = true;
            readings.append(r);
        }

        return readings;
    }

    static BankStatus sampleBank()
    {
        BankStatus bank;
        bank.voltageV = 50.50;
        bank.currentA = 25.0;
        bank.socPct = 92.0;
        bank.sohPct = 98.0;
        bank.mode = QStringLiteral("48 V Series");
        bank.systemState = QStringLiteral("NORMAL");
        return bank;
    }

private slots:
    void databaseInitializes();
    void savesAndReadsBankSnapshot();
    void readsIndividualBatteryHistory();
    void savesSnapshotWithoutBlockReadings();
    void failedBlockInsertRollsBackSnapshot();
    void savesDiagnosticForExistingSnapshot();
    void rejectsDiagnosticForUnknownSnapshot();
    void logsEvent();
    void rejectsInvalidBatteryHistoryId();
    void rejectsZeroDiagnosticSnapshotId();
};

void TelemetryDatabaseTest::databaseInitializes()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString path = tempDir.filePath("initialization_test.sqlite3");
    TelemetryDatabase database(nullptr, path);

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));
    QCOMPARE(database.databasePath(), path);
    QVERIFY(QFileInfo::exists(path));
}

void TelemetryDatabaseTest::savesAndReadsBankSnapshot()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("bank_snapshot.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    qint64 snapshotId = -1;

    QVERIFY2(database.saveSnapshot(
                 sampleReadings(),
                 sampleBank(),
                 QStringLiteral("SIMULATOR"),
                 QStringLiteral("VALID"),
                 QStringLiteral("Database test"),
                 &snapshotId),
             qPrintable(database.lastError()));

    QVERIFY(snapshotId > 0);

    const QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-10);
    const QDateTime end = QDateTime::currentDateTimeUtc().addSecs(10);

    const QList<HistoricalReading> history =
        database.queryHistory(start, end, 0);

    QCOMPARE(history.size(), 1);

    const HistoricalReading &row = history.first();

    QCOMPARE(row.dataSource, QStringLiteral("SIMULATOR"));
    QCOMPARE(row.bankVoltageV, 50.50);
    QCOMPARE(row.bankCurrentA, 25.0);
    QCOMPARE(row.bankSocPct, 92.0);
    QCOMPARE(row.bankSohPct, 98.0);
    QCOMPARE(row.batteryId, 0);
}

void TelemetryDatabaseTest::readsIndividualBatteryHistory()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("battery_history.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    QVERIFY2(database.saveSnapshot(
                 sampleReadings(),
                 sampleBank(),
                 QStringLiteral("SIMULATOR"),
                 QStringLiteral("VALID"),
                 QStringLiteral("Battery history test")),
             qPrintable(database.lastError()));

    const QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-10);
    const QDateTime end = QDateTime::currentDateTimeUtc().addSecs(10);

    const QList<HistoricalReading> history =
        database.queryHistory(start, end, 2);

    QCOMPARE(history.size(), 1);

    const HistoricalReading &row = history.first();

    QCOMPARE(row.batteryId, 2);
    QCOMPARE(row.batteryVoltageV, 12.62);
    QCOMPARE(row.batteryTemperatureC, 27.0);
    QCOMPARE(row.batterySocPct, 92.0);
    QCOMPARE(row.batterySohPct, 99.0);
    QVERIFY(row.sensorValid);
}

void TelemetryDatabaseTest::savesSnapshotWithoutBlockReadings()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("empty_blocks.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    qint64 snapshotId = -1;

    QVERIFY2(database.saveSnapshot(
                 {},
                 sampleBank(),
                 QStringLiteral("SIMULATOR"),
                 QStringLiteral("VALID"),
                 QStringLiteral("Bank-only test"),
                 &snapshotId),
             qPrintable(database.lastError()));

    QVERIFY(snapshotId > 0);

    const QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-10);
    const QDateTime end = QDateTime::currentDateTimeUtc().addSecs(10);

    const QList<HistoricalReading> history =
        database.queryHistory(start, end, 0);

    QCOMPARE(history.size(), 1);
    QCOMPARE(history.first().batteryId, 0);
    QCOMPARE(history.first().bankVoltageV, 50.50);
}

void TelemetryDatabaseTest::failedBlockInsertRollsBackSnapshot()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("rollback_test.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    QList<BatteryReading> readings = sampleReadings();

    // The schema only permits block IDs 1 through 4.
    BatteryReading invalid;
    invalid.id = 5;
    readings.append(invalid);

    qint64 snapshotId = -1;

    QVERIFY(!database.saveSnapshot(
        readings,
        sampleBank(),
        QStringLiteral("SIMULATOR"),
        QStringLiteral("VALID"),
        QStringLiteral("Rollback test"),
        &snapshotId));

    // The failed transaction must not leave a partial bank record.
    const QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-10);
    const QDateTime end = QDateTime::currentDateTimeUtc().addSecs(10);

    const QList<HistoricalReading> history =
        database.queryHistory(start, end, 0);

    QVERIFY(history.isEmpty());
}

void TelemetryDatabaseTest::savesDiagnosticForExistingSnapshot()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("diagnostic_test.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    qint64 snapshotId = -1;

    QVERIFY(database.saveSnapshot(
        sampleReadings(),
        sampleBank(),
        QStringLiteral("SIMULATOR"),
        QStringLiteral("VALID"),
        QStringLiteral("Diagnostic test"),
        &snapshotId));

    QVERIFY(snapshotId > 0);

    QVERIFY2(database.saveDiagnostic(
                 snapshotId,
                 QStringLiteral("Diagnostic test"),
                 QStringLiteral("NORMAL"),
                 QStringLiteral("No anomaly detected"),
                 QStringLiteral("NORMAL"),
                 0.05,
                 0.02),
             qPrintable(database.lastError()));
}

void TelemetryDatabaseTest::rejectsDiagnosticForUnknownSnapshot()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("invalid_diagnostic.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    QVERIFY(!database.saveDiagnostic(
        999999,
        QStringLiteral("Unknown snapshot"),
        QStringLiteral("NORMAL"),
        QStringLiteral("Test"),
        QStringLiteral("NORMAL"),
        0.05,
        0.02));
}

void TelemetryDatabaseTest::logsEvent()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("event_test.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    QVERIFY2(database.logEvent(
                 QStringLiteral("WARNING"),
                 QStringLiteral("TEST_EVENT"),
                 QStringLiteral("Database event logging test"),
                 QStringLiteral("UNIT_TEST")),
             qPrintable(database.lastError()));
}

void TelemetryDatabaseTest::rejectsInvalidBatteryHistoryId()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("invalid_battery_id.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    const QDateTime start = QDateTime::currentDateTimeUtc().addSecs(-10);
    const QDateTime end = QDateTime::currentDateTimeUtc().addSecs(10);

    const QList<HistoricalReading> history =
        database.queryHistory(start, end, 5);

    QVERIFY(history.isEmpty());
    QVERIFY(!database.lastError().isEmpty());
}

void TelemetryDatabaseTest::rejectsZeroDiagnosticSnapshotId()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    TelemetryDatabase database(
        nullptr, tempDir.filePath("zero_snapshot_id.sqlite3"));

    QVERIFY2(database.isReady(), qPrintable(database.lastError()));

    QVERIFY(!database.saveDiagnostic(
        0,
        QStringLiteral("Invalid snapshot"),
        QStringLiteral("NORMAL"),
        QStringLiteral("Test"),
        QStringLiteral("NORMAL"),
        0.05,
        0.02));

    QVERIFY(!database.lastError().isEmpty());
}

QTEST_GUILESS_MAIN(TelemetryDatabaseTest)

#include "tst_telemetrydatabase.moc"
