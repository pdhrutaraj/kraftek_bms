# RXN BMS Communication Protocol v2 – implementation notes

Source: `RXN BMS - Communication Protocol v2.pdf`, April 2024, Version 2.

## Implemented transport

`src/services/RxnBmsProtocol.{h,cpp}` implements the master-side RS-485/UART serial transport:

- 9600 baud, 8 data bits, no parity, one stop bit, no hardware/software flow control.
- Half-duplex RS-485 physical link (direction control is expected to be handled by the USB-RS485 adapter/driver).
- 8-byte master command frame.
- Command byte: bit 7 = 0 for read; bits 6..4 select bank (`001` CFG, `010` RTD, `011` Control, `100` Historical).
- 16-bit register address is transmitted MSB first.
- Read response is parsed as ACK + requested data bytes + checksum byte.
- 50 ms response timeout, per the protocol document.
- ACK values 1 and 30..34 are decoded.

Only read APIs are exposed. There is deliberately no write method and no control-bank write support.

## Register addresses transcribed from the document

| RTD register | Name | Document type/access |
|---:|---|---|
| 0 | SSR_STATUS | uint16, read |
| 15 | VBAT | uint16, read |
| 18 | BAT_SOC | Type/access not clearly rendered in summary |
| 19 | IBAT | uint16, read |
| 20 | IBAT_DIR | Values 0/1/2 = standby/charge/discharge |
| 70 | BMS_TEMP | uint16, read |
| 72 | BAT_TEMP | uint16, read |
| 101 | VCELL array | uint16 array |
| 301 | CELL_RES array | uint16 array, marked disabled |
| 501 | CELL_SOC array | uint16 array |

The driver returns raw bytes in MSB-first order. It does not convert units, signedness, scaling, or array indexing into engineering values.

## Explicit protocol gaps / vendor confirmation required

1. The command and response tables label the checksum field but do not define an algorithm. The command table prints `85`; this implementation currently places decimal 85 (`0x55`) in the command checksum byte and checks for the same response trailer. Confirm this interpretation with RXN before connecting to a live BMS. If 85 is a checksum algorithm identifier or example rather than a literal, this must be changed.
2. The document does not clearly state how the master specifies the response length for a read. `readRegister()` therefore requires the caller to supply the expected data-byte count. The convenience `readRealTimeRegister()` defaults to one 16-bit word (2 bytes).
3. Several register engineering-unit scales, signedness, and array lengths are absent or unclear. No guessed conversions are applied.
4. Broadcast IDs and CAN packet parsing are not implemented in this RS-485 driver.
5. The document gives 470 ohm termination for RS-485. Confirm the physical bus topology and whether termination/biasing already exists before adding resistors; do not add duplicate termination blindly.

## Safety / application integration

- The application continues to start with `SimulatedDataSource`; no live hardware is opened automatically.
- The protocol class is a standalone service intended to be injected through a future `IBatteryDataSource` implementation after register scaling and framing are confirmed.
- No BMS configuration, relay, battery-switch, alarm, or protection writes are sent.
- Do not use the `readWrite` open mode as permission to issue writes; Qt's serial port is opened for byte transport, but this class only builds read commands.

## Build dependency

Qt 6 SerialPort is now required by CMake (`Qt6::SerialPort`). Install the matching Qt SerialPort development package for the host/target Qt version before building.

## Hardware validation sequence

1. Keep the current BMS operating normally; do not attach or detach RS-485 wiring while energized unless the vendor procedure permits it.
2. Verify adapter A/B polarity, common reference requirements, isolation, bus termination and existing master ownership with RXN/vendor documentation.
3. Ensure there is only one active master. The RXN protocol says the master initiates every transaction; do not connect this host in parallel with an existing master.
4. Confirm checksum and read-response length with RXN before enabling polling.
5. Start with one documented read-only RTD register, compare raw response bytes with vendor tooling, and only then add register decoding.
