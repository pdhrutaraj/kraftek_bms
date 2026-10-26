# Verification Plan — Draft

This plan is a requirements checklist, not evidence of passing tests. Test high-current behavior on an approved, protected test setup with qualified personnel and appropriate fixtures.

| ID | Verification | Acceptance evidence to define before test |
|---|---|---|
| V-01 | Four block voltage channels and independent bank voltage | Calibrated source comparison across voltage, temperature and common-mode envelope |
| V-02 | 0.01 V display resolution | GUI retains hundredths; separately demonstrate measurement accuracy against approved tolerance |
| V-03 | NTC channels | Temperature chamber/resistance simulation, open/short fault injection, mounting response characterization |
| V-04 | Charge current 10–40 A | Calibrated current-source or safe power test; direction, accuracy and thermal behavior |
| V-05 | Cranking profile 600–700 A for 3–5 s and bank sag to 36 V | Representative instrumented test demonstrates no nuisance trip within the approved crank envelope |
| V-06 | SCP criterion >900 A for >50 ms | Hardware-level timing/threshold test with safe injection method; verify coordination with interrupt hardware and fuse clearing behavior |
| V-07 | OVP/UVP/OTP/OCP/time-capacity backup | Threshold, hysteresis, debounce, fault latch, reset and charger/load path behavior per approved requirements |
| V-08 | Communications loss / malformed frames | Watchdog timeout, CRC/error handling, stale command rejection; local protection unaffected |
| V-09 | Remote command interlocks | Unauthorized or unsafe requests rejected; local fault state cannot be overridden remotely |
| V-10 | SQLite outage / storage full / power loss | Protection and acquisition remain functional; recoverable/integrity-checked event history behavior |
| V-11 | AI diagnostics | Documented data provenance, validation set, uncertainty and false-positive/negative metrics; no direct safety trip authority |
| V-12 | Synthetic AI demo | Verify all five manual scenarios and automatic cycling; confirm scenario labels and AI outputs persist in SQLite; check migration from a pre-existing database; verify the score is labeled as a demo score and cannot control protection. |
| V-13 | GUI human factors | Alarm priority, stale-data indication, units, sign convention, operator workflow and accessibility review |
| V-14 | Environmental/mechanical | Vibration, temperature, ingress, connector retention and EMC tests to be specified for deployment environment |

## Required engineering decisions before detailed design freeze

1. UVP meaning: under-voltage protection usually inhibits discharge/load to prevent deep discharge; it does not normally disconnect charging. Approve separate resting-voltage, cranking-sag and charge-inhibit rules.
2. Exact OVP/UVP thresholds for each 12 V lead-acid block and bank, by battery type and operating mode.
3. OTP trip/reset values and whether the stated 45–50 °C range is a warning or a hard cut-off.
4. Sensor response time and hardware-latch timing relative to the >900 A / >50 ms SCP requirement.
5. Power-path device ratings and interruption topology for charging and discharge/cranking.
6. Communication-loss default state and safe restart/reset procedure.
