# System Architecture — 48 V Lead-Acid Cranking BMS

## Functional split

1. **Displayless BMS slave:** local acquisition of B1–B4 voltage, bank voltage, shunt/Hall current and four NTC temperatures; local coulomb counting; deterministic fault handling; buzzer; validated CAN or RS-485 Modbus RTU telemetry; accepted remote request bits only after local interlocks.
2. **External main processor/HMI:** Qt 6 Widgets MVC, operator display, alarm/event presentation, SQLite history, configuration workflow, trend analysis and AI/ML diagnostics.
3. **High-current power path:** separately engineered charging interruption and any discharge/cranking interruption. The actual switching elements, fuses, busbars, conductors, and fault-current ratings require electrical design and verification.

## Sensor notes

- Four 12 V block-voltage channels are required, plus a separate bank-voltage channel.
- Target display resolution is 0.01 V; resolution is not an accuracy specification. Define accuracy over temperature, common-mode range, isolation, filtering, calibration and diagnostic coverage.
- For a 1000 A / 75 mV shunt, 1000 A corresponds to 75 mV and 700 A corresponds to 52.5 mV. Use a suitably protected differential front end and verify transient recovery. A Hall sensor is an alternative only after bandwidth, offset, temperature drift, isolation and peak-current performance are established.
- Four external NTC probes attach at battery terminal lugs. Define probe curve/tolerance, mounting, open/short detection, response time and trip/reset hysteresis.

## Protection boundary

Protection decisions must not depend on the HMI, AI model, network, or database. The GUI cannot override a locally latched fault or directly guarantee a contactor/charger interruption. Remote commands are requests subject to local safety state and interlocks.
