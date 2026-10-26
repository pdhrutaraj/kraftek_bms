# KRAFTEK BMS synthetic AI demo implementation notes

## Included
- Five synthetic scenarios: Normal, Voltage sag, Block imbalance, Temperature rise, Sensor spike.
- Manual scenario selection and automatic cycling every 12 simulator samples.
- Statistical/engineering heuristic checks plus a lightweight nearest-centroid classifier fitted at startup from deterministic generated, labelled synthetic examples.
- New AI Demo navigation page with scenario, predicted class, synthetic anomaly score, and explanation.
- SQLite telemetry scenario labels and linked `ai_diagnostics` rows. Existing telemetry DBs are migrated by adding `scenario_label` if missing.
- Updated AI design, schema, verification plan, README, and changelog.

## Important interpretation
The centroid classifier is trained only on generated synthetic feature vectors. It is not trained on measured battery data and has no demonstrated real-world predictive performance. Its score is a demonstration score, not a calibrated failure probability. AI output is advisory only; no hardware control or protection action is implemented.

## Build and test on each target
On the HP ProBook:
```bash
cd ~/ai_based_battery_monitor/kraftek_bms
JOBS=2 ./build_x86_64.sh
./build_x86/kraftek_bms
```

On the Jetson TX2 NX:
```bash
cd ~/kraftek_bms
JOBS=2 ./build_tx2nx.sh
./build_tx2nx/kraftek_bms
```

In the AI DEMO page, test each scenario manually, then enable automatic cycling. Check the System Status database message and inspect the SQLite `telemetry.scenario_label` and `ai_diagnostics` tables. Back up any existing database before first launch of this build.

The source archive was inspected and edited in an environment without Qt 6 installed, so a local CMake build could not be completed here. Build verification must be performed on the two machines with their configured Qt installations.
