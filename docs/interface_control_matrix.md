# Interface and Control Matrix (Draft for Review)

| Signal / function | Source → destination | Draft behavior | Required review / verification |
|---|---|---|---|
| B1–B4 block voltage | BMS slave → HMI | Four channels, target 0.01 V display resolution | Accuracy, common-mode range, isolation, sampling, calibration, open-wire detection |
| Bank voltage | BMS slave → HMI | Independent total-bank measurement | Compare against sum of block readings; define tolerance and fault response |
| Bank current | Shunt or Hall → BMS slave → HMI | Charge/discharge sign convention must be fixed; capture 600–700 A start transient | Range, bandwidth, saturation, offset, transient recovery, calibration |
| Four NTC temperatures | NTCs → BMS slave → HMI | Terminal-lug temperatures; OTP target 45–50 °C pending approval | Trip/reset thresholds, hysteresis, sensor open/short detection, thermal response |
| Charging inhibit/cut-off | BMS slave → charger/interrupt hardware | Local OVP/OTP/OCP/time-capacity logic may request interruption | Actual interruption device, DC rating, failure mode and feedback confirmation |
| Discharge/cranking enable | BMS slave / external command | Local interlocks dominate remote requests | No-trip crank envelope, restart rules, fault latch and recovery behavior |
| SCP | Current sensing → local hardware latch | Draft criterion >900 A for >50 ms | Coordinate with sensor response, battery fault current, fuses, wiring and switching device |
| Communications | BMS slave ↔ external HMI | CAN or RS-485 Modbus RTU; select one for implementation | Message map, baud/bitrate, timeouts, CRC/error handling, node identity, security |
| Remote command bits | HMI → BMS slave | Request only; reject unsafe commands locally | Authentication/authorization, stale command handling, state feedback, audit trail |
| Event log | BMS telemetry → HMI SQLite | Store measurements, cranking events, alarms, configuration changes | Timestamp source, retention, power-loss integrity, export and schema migration |
| AI/ML diagnostics | HMI data pipeline | Advisory SoC/SoH, sag trend, anomaly detection | Dataset validation, confidence/uncertainty, false-alarm evaluation; never sole trip source |
| Communication loss | Link watchdog → local BMS | Local protection remains active; reject stale remote commands | Define output state and recovery policy for each command and hardware path |

## Draft state semantics

- `NORMAL`: no currently reported fault; not proof of system safety.
- `CHARGING`: charging current is present or charger state is confirmed (define which source is authoritative).
- `CRANKING`: a start event has been detected; current/voltage event capture is active.
- `WARNING`: advisory threshold crossed without a trip.
- `FAULT_LATCHED`: local protection has latched a fault; reset only under an approved recovery policy.
- `COMM_LOSS`: telemetry/command link timed out; local protection remains independent.

The HMI prototype currently uses only simulator-generated `NORMAL` status. These states are proposed, not implemented hardware behavior.
