# RXN BMS Hardware Data Qualification Specification

Status: DRAFT — NOT HARDWARE QUALIFIED

## 1. Purpose

Define the evidence required before telemetry from a physical RXN BMS
can be considered verified hardware data.

This document defines qualification requirements. It does not claim
that hardware communication, measurements, or validation tests have
passed.

The existing dataset exporter must remain fail-closed until a separately
reviewed implementation explicitly approves a hardware source.

## 2. Current implementation status

| Item | Current status |
|---|---|
| RXN protocol document | Available: Communication Protocol v2, April 2024 |
| RS-485 transport implementation | Implemented in RxnBmsProtocol |
| Serial configuration | 9600 baud, 8N1, as documented |
| Command generation | Read commands only |
| Response parsing | Raw response bytes and ACK parsing |
| Checksum interpretation | Vendor confirmation required |
| Response data length | Caller must supply expected length |
| Engineering-unit conversion | Not verified |
| RXN data source integration | Placeholder; no serial communication |
| Physical hardware measurements | Not verified |
| Field-training eligibility | Not approved |

The existence of a protocol driver does not prove that it has communicated
successfully with a physical RXN BMS.

## 3. Qualification stages

### Q1 — Physical interface and bus safety

Required evidence:

- RXN hardware model and firmware identification.
- USB-to-RS485 adapter model and electrical specifications.
- A/B polarity and reference-ground requirements confirmed.
- Isolation and protection arrangements documented.
- Existing termination and biasing documented.
- Bus topology and existing master ownership confirmed.
- No second master is connected during normal operation.

Acceptance: approved wiring and bus arrangement documented before
connecting the Jetson to an energized operational bus.

### Q2 — Protocol confirmation

Obtain vendor confirmation for:

- Command checksum interpretation, including the documented value 85.
- Response checksum algorithm and validation procedure.
- Read response length rules.
- ACK values and their meanings.
- Register address and bank definitions.
- Multi-register and array response ordering.
- Timeout and retry behavior.
- Any protocol or firmware-version dependencies.

Acceptance: protocol uncertainties are resolved against vendor documentation
or written vendor clarification. Unconfirmed fields must not be guessed.

### Q3 — Raw communication verification

Required evidence:

- Captured raw request and response bytes.
- Timestamp for every captured transaction.
- Register address and expected response length.
- ACK and checksum validation result.
- Timeout, malformed-frame and disconnect test results.
- Comparison with vendor tooling or an independently verified reference.

Start with documented read-only RTD registers.

Do not send configuration, relay, battery-switch, alarm-clear or protection
write commands as part of qualification.

Acceptance: repeatable, correctly framed and validated read responses.

### Q4 — Engineering-unit validation

For every proposed measurement, document:

- Register address and data type.
- Signedness and byte/word ordering.
- Scaling factor and engineering unit.
- Valid range and reserved values.
- Array length and element-to-battery mapping.
- Invalid, unavailable and stale-data behavior.

Required measurement checks include:

- Four individual 12 V block voltages.
- Independent 48 V bank voltage.
- Charge/discharge current and direction.
- Available battery and BMS temperatures.
- Other measurements only where their register definitions are confirmed.

Acceptance: decoded values agree with independent calibrated instruments
within tolerances approved for the measurement.

Do not infer measurement accuracy from display resolution.

### Q5 — Data integrity and fault handling

Verify:

- Unique and correctly mapped block identifiers.
- Valid timestamps and documented clock source.
- Missing, duplicate and out-of-order responses.
- Stale-data detection.
- Invalid sensor and out-of-range values.
- Serial disconnect and reconnection.
- Partial or malformed responses.
- No fabricated measurements on communication failure.

Acceptance: invalid or unavailable measurements remain explicitly invalid;
they are never silently replaced with plausible values.

### Q6 — Hardware qualification record

Record the following evidence:

- Hardware model, serial identifier and firmware version.
- Adapter and wiring configuration.
- Protocol document revision and vendor clarifications.
- Register decoding specification revision.
- Test equipment identification and calibration status.
- Test date, test procedure and captured evidence references.
- Expected and measured values, tolerances and results.
- Unresolved issues and deviations.
- Reviewer decision and approval date.

Qualification status must be explicit:

- NOT_TESTED
- IN_PROGRESS
- FAILED
- QUALIFIED

Only a reviewed and documented QUALIFIED record may proceed to
field-training eligibility review.

## 4. Field-training approval is a separate decision

Hardware qualification alone does not automatically approve a dataset
for AI/ML training.

A separate dataset review must consider:

- Source identity and provenance consistency.
- Simulator and mixed-source contamination.
- Timestamp continuity and data completeness.
- Sensor validity and measurement uncertainty.
- Duplicate and missing records.
- Operating conditions and test coverage.
- Independent ground-truth labels where supervised learning is intended.
- Dataset version, manifest and review record.

AI diagnostic predictions must not be treated as verified ground-truth
fault labels.

## 5. Fail-closed requirements

Until qualification and separate field-training approval are implemented
and reviewed:

- SIMULATOR remains ineligible for field training.
- RXN_BMS remains ineligible.
- Unknown, blank and mixed sources remain ineligible.
- A data_quality value of VALID does not establish hardware provenance.
- Archive export remains available.
- Field-training export must continue to reject unapproved data.

No change to the current exporter approval policy is authorized by
this specification.

## 6. Safety boundary

Hardware qualification and AI dataset preparation must not alter the
BMS slave's deterministic protection behavior.

Voltage, current, temperature, over-current and other safety protections
must remain local to the BMS slave and independent of the Jetson GUI,
AI/ML diagnostics, SQLite database and communications link.

No qualification test may bypass protection interlocks or disturb an
operational RS-485 bus without an approved test procedure.

## 7. Qualification decision

Current decision: NOT QUALIFIED.

Reason: physical RXN serial communication, vendor-confirmed protocol
details, engineering-unit decoding and independent measurement validation
have not yet been demonstrated.

Next action: resolve the outstanding protocol questions with RXN and
prepare an approved read-only hardware validation procedure.
