#include "TelemetryDatabase.h"
#include <QDir>
#include <QStandardPaths>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

TelemetryDatabase::TelemetryDatabase(QObject *parent) : QObject(parent) {
    connectionName_ = QStringLiteral("kraftek_bms_sqlite_%1").arg(static_cast<qulonglong>(reinterpret_cast<quintptr>(this)));
    QString root = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (root.isEmpty()) root = QDir::currentPath() + QStringLiteral("/data");
    QDir().mkpath(root);
    path_ = root + QStringLiteral("/kraftek_bms.sqlite3");
    db_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db_.setDatabaseName(path_);
    ready_ = initialize();
}

TelemetryDatabase::~TelemetryDatabase() {
    if (db_.isValid()) { db_.close(); db_ = QSqlDatabase(); QSqlDatabase::removeDatabase(connectionName_); }
}

bool TelemetryDatabase::initialize() {
    if (!db_.open()) { lastError_ = db_.lastError().text(); return false; }
    QSqlQuery q(db_);
    const QStringList statements = {
        QStringLiteral("PRAGMA foreign_keys=ON"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS telemetry (id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp_utc TEXT NOT NULL, bank_voltage_v REAL NOT NULL, bank_current_a REAL NOT NULL, soc_pct REAL, soh_pct REAL, mode TEXT NOT NULL, system_state TEXT NOT NULL, data_quality TEXT NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS block_readings (id INTEGER PRIMARY KEY AUTOINCREMENT, telemetry_id INTEGER NOT NULL REFERENCES telemetry(id) ON DELETE CASCADE, block_id INTEGER NOT NULL CHECK(block_id BETWEEN 1 AND 4), voltage_v REAL NOT NULL, temperature_c REAL, soc_pct REAL, soh_pct REAL, sensor_valid INTEGER NOT NULL)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS events (id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp_utc TEXT NOT NULL, severity TEXT NOT NULL, event_code TEXT NOT NULL, message TEXT NOT NULL, source TEXT NOT NULL, acknowledged INTEGER NOT NULL DEFAULT 0)"),
        QStringLiteral("CREATE TABLE IF NOT EXISTS configuration_audit (id INTEGER PRIMARY KEY AUTOINCREMENT, timestamp_utc TEXT NOT NULL, operator_id TEXT, parameter_key TEXT NOT NULL, old_value TEXT, new_value TEXT, result TEXT NOT NULL, reason TEXT)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_telemetry_time ON telemetry(timestamp_utc)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_events_time ON events(timestamp_utc)")
    };
    for (const auto &sql : statements) if (!q.exec(sql)) { lastError_ = q.lastError().text(); return false; }
    return true;
}

bool TelemetryDatabase::saveSnapshot(const QList<BatteryReading> &readings, const BankStatus &bank,
                                     const QString &quality, qint64 *snapshotId) {
    if (!ready_) return false;
    if (!db_.transaction()) { lastError_ = db_.lastError().text(); return false; }
    QSqlQuery q(db_);
    q.prepare(QStringLiteral("INSERT INTO telemetry(timestamp_utc,bank_voltage_v,bank_current_a,soc_pct,soh_pct,mode,system_state,data_quality) VALUES(?,?,?,?,?,?,?,?)"));
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)); q.addBindValue(bank.voltageV); q.addBindValue(bank.currentA);
    q.addBindValue(bank.socPct); q.addBindValue(bank.sohPct); q.addBindValue(bank.mode); q.addBindValue(bank.systemState); q.addBindValue(quality);
    if (!q.exec()) { lastError_ = q.lastError().text(); db_.rollback(); return false; }
    const qint64 id = q.lastInsertId().toLongLong();
    QSqlQuery b(db_); b.prepare(QStringLiteral("INSERT INTO block_readings(telemetry_id,block_id,voltage_v,temperature_c,soc_pct,soh_pct,sensor_valid) VALUES(?,?,?,?,?,?,?)"));
    for (const auto &r : readings) {
        b.bindValue(0, id); b.bindValue(1, r.id); b.bindValue(2, r.voltageV); b.bindValue(3, r.temperatureC);
        b.bindValue(4, r.socPct); b.bindValue(5, r.sohPct); b.bindValue(6, r.sensorValid ? 1 : 0);
        if (!b.exec()) { lastError_ = b.lastError().text(); db_.rollback(); return false; }
    }
    if (!db_.commit()) { lastError_ = db_.lastError().text(); db_.rollback(); return false; }
    if (snapshotId) *snapshotId = id;
    return true;
}

bool TelemetryDatabase::logEvent(const QString &severity, const QString &code, const QString &message, const QString &source) {
    if (!ready_) return false;
    QSqlQuery q(db_); q.prepare(QStringLiteral("INSERT INTO events(timestamp_utc,severity,event_code,message,source) VALUES(?,?,?,?,?)"));
    q.addBindValue(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)); q.addBindValue(severity); q.addBindValue(code); q.addBindValue(message); q.addBindValue(source);
    if (!q.exec()) { lastError_ = q.lastError().text(); return false; }
    return true;
}
