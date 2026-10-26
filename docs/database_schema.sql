-- Proposed SQLite schema for the external HMI/main processor only.
-- Database availability must never be a prerequisite for local BMS protection.
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS telemetry (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_utc TEXT NOT NULL,
    bank_voltage_v REAL NOT NULL,
    bank_current_a REAL NOT NULL,
    soc_pct REAL,
    soh_pct REAL,
    mode TEXT NOT NULL,
    system_state TEXT NOT NULL,
    data_quality TEXT NOT NULL DEFAULT 'SIMULATED',
    scenario_label TEXT NOT NULL DEFAULT 'Normal'
);

CREATE TABLE IF NOT EXISTS block_readings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    telemetry_id INTEGER NOT NULL REFERENCES telemetry(id) ON DELETE CASCADE,
    block_id INTEGER NOT NULL CHECK (block_id BETWEEN 1 AND 4),
    voltage_v REAL NOT NULL,
    temperature_c REAL,
    soc_pct REAL,
    soh_pct REAL,
    sensor_valid INTEGER NOT NULL CHECK (sensor_valid IN (0,1))
);

CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_utc TEXT NOT NULL,
    severity TEXT NOT NULL,
    event_code TEXT NOT NULL,
    message TEXT NOT NULL,
    source TEXT NOT NULL,
    acknowledged INTEGER NOT NULL DEFAULT 0 CHECK (acknowledged IN (0,1))
);

CREATE TABLE IF NOT EXISTS configuration_audit (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp_utc TEXT NOT NULL,
    operator_id TEXT,
    parameter_key TEXT NOT NULL,
    old_value TEXT,
    new_value TEXT,
    result TEXT NOT NULL,
    reason TEXT
);

CREATE INDEX IF NOT EXISTS idx_telemetry_time ON telemetry(timestamp_utc);
CREATE INDEX IF NOT EXISTS idx_events_time ON events(timestamp_utc);
CREATE INDEX IF NOT EXISTS idx_events_code ON events(event_code);


-- Synthetic AI demo output; advisory-only, tied to the telemetry snapshot.
CREATE TABLE IF NOT EXISTS ai_diagnostics (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    telemetry_id INTEGER NOT NULL REFERENCES telemetry(id) ON DELETE CASCADE,
    timestamp_utc TEXT NOT NULL,
    scenario_label TEXT NOT NULL,
    status TEXT NOT NULL,
    summary TEXT NOT NULL,
    ml_class TEXT NOT NULL,
    statistical_score REAL NOT NULL,
    ml_score REAL NOT NULL,
    model_version TEXT NOT NULL DEFAULT 'synthetic-centroid-demo-v1'
);
