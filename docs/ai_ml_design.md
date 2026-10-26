# AI/ML diagnostics prototype and synthetic demo

## Implemented demo behavior

`SimulatedDataSource` supports manual selection and optional automatic cycling of five synthetic scenarios: Normal, Voltage sag, Block imbalance, Temperature rise, and Sensor spike. Automatic cycling advances every 12 telemetry samples. All values are generated locally; no hardware bus or control output is used.

`DiagnosticEngine` combines transparent statistical/engineering-review heuristics with a small nearest-centroid classifier whose class centroids are fitted at startup from a deterministic generated labelled synthetic feature set. It is a lightweight trained demo classifier, not a model fitted to field data and not a validated estimator. Its ML score is a synthetic distance-from-normal score, not a calibrated probability.

The AI Demo page exposes scenario controls, the predicted class, score, and explanation. Each telemetry snapshot stores a scenario label; `ai_diagnostics` stores the associated advisory result, model version, statistical score, and synthetic model score. Startup migrates existing telemetry tables by adding `scenario_label` when absent.

## Safety and limitations

- Simulation only. No CAN/RS-485, sensor, charger, contactor, or protection output is connected.
- All anomaly results are advisory. They must never issue, inhibit, or suppress a protection trip.
- The thresholds and synthetic centroids are demonstration values only; they are not battery-specific validated limits.
- The score is not a probability of failure. Synthetic class matching does not establish real-world predictive performance.
- SoC/SoH remain illustrative inputs, not validated estimates.

## Production ML path

For a genuine trained model, create a reproducible synthetic dataset generator with explicit labels, split data into training/validation/test sets by scenario and operating profile, fit a lightweight model offline, serialize versioned model parameters, and report precision/recall and false-positive/false-negative rates. Then test on independent measured data before any field use. Keep deterministic hardware protection local to the BMS slave, independent of the HMI, database, communications, and AI inference.
