# Changelog

## 0.6 — Synthetic-data AI demo
- Added manual and automatic cycling across five synthetic battery scenarios.
- Added a lightweight nearest-centroid classifier fitted at startup from deterministic labelled synthetic samples, combined with statistical diagnostic heuristics.
- Added an AI Demo page with scenario controls, predicted class, advisory score and explanation.
- Added scenario labels and per-snapshot AI diagnostic persistence in SQLite, including a backward-compatible telemetry-table migration.
- Explicitly labelled all outputs simulation-only; no hardware controls or protection actions added.

## 0.5 — SQLite + diagnostics integration
- Added Qt SQL / SQLite3 runtime database initialization and transactional storage of bank telemetry and B1–B4 readings.
- Added event persistence for advisory diagnostic findings.
- Added rolling-baseline statistical anomaly diagnostics and sensor/temperature/voltage-spread/cranking-sag review heuristics.
- Added live database and diagnostic status to System Status page.
- Kept telemetry simulator active; live CAN/RS-485 and physical protection remain unintegrated.
- Clarified that diagnostics are advisory heuristics, not a trained or safety-certified ML model.

## 0.4
- Professional dark industrial Qt6 MVC HMI prototype for 48 V / 4 × 12 V 180 Ah lead-acid cranking bank.
