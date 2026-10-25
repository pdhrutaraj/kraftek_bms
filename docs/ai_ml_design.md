# AI/ML diagnostics prototype

## Implemented in this revision

`src/ai/DiagnosticEngine.*` implements a small, explainable, online statistical anomaly detector. It tracks a bounded rolling baseline for bank voltage and current and flags three-standard-deviation deviations after enough samples. It also applies transparent engineering-review heuristics for invalid sensor flags, block voltage spread above 0.25 V, terminal temperature at/above 45 °C, and high-current voltage-sag patterns (bank voltage below 40 V while absolute current exceeds 200 A). It emits a status, summary, and coarse advisory score.

## What this is—and is not

- This is a **diagnostic prototype**, not a trained supervised ML model and not a validated estimator.
- Thresholds are placeholders for engineering review and must be approved using the selected battery chemistry, sensor accuracy, temperature range, and cranking waveforms.
- The 36 V crank-sag behavior is deliberately treated as a diagnostic correlation, not an automatic trip.
- No AI output may issue or suppress a protection trip. Local protection hardware/firmware must independently implement validated OVP, UVP, OTP, OCP, and SCP behavior.
- No trustworthy SoC/SoH estimate can be produced by this detector alone. Production SoC should use calibrated current integration (coulomb counting), with validated rest-voltage/temperature correction and reset/recovery handling. SoH needs an approved test method and battery-specific validation.
- For production ML, collect labeled field/cranking datasets, split by battery/system and operating conditions, establish false-positive/false-negative acceptance criteria, version models, test out-of-distribution behavior, and retain a deterministic fallback.

## Suggested next stage

Build a recorded-data replay harness, label crank-start / charge / rest / fault windows, and compare a transparent baseline against a trained anomaly model offline before shipping any model to the HMI. Keep model inference on the external processor and protections on the displayless BMS slave.
