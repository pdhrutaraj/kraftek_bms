# Responsive UI and historical trends update

## Included
- Responsive initial sizing and compact sidebar labels. The Overview uses four metric cards and four battery cards in one row at TX2 NX desktop widths, with compact spacing and no vertical scrollbar; historical plots remain on the dedicated Trends page. Other pages retain scrolling.
- The Overview omits the large historical preview and explanatory panels to preserve single-screen operation.
- Bank SOH is the minimum valid individual battery SOH. Individual and bank SOH values have progress bars.
- Charging/discharging/idle indication based on the existing simulator convention: positive bank current is charging, negative current is discharge.
- Historical Trends page with editable start/end date-time range, Bank/B1-B4 selection, and plots for voltage, SoC, SoH, current, battery temperature, derived SoC drain rate, and indicative internal resistance.
- SQLite history query joining telemetry snapshots to block readings.

## Important measurement limits
- The project still uses synthetic simulator telemetry; no live RS-485/CAN interface or protection output is added.
- The four blocks are in series, so the same string current is used for all individual-battery trends. No per-block current sensor is modeled.
- Drain rate is computed from SoC change per elapsed hour; it is not a coulomb-counter measurement.
- Internal resistance is only an indicative ΔV/ΔI estimate and appears only when current changes by at least 2 A between recorded samples. It is not a validated pulse/DCIR test.
- Historical plots require saved telemetry in the selected time range. The SQLite database starts recording when the application runs; older periods without records remain empty.

## Build validation
This source was statically reviewed in the packaging environment. CMake configuration could not run to compilation because Qt6 development packages (`Qt6Config.cmake`) are not installed in that environment. Build and visually verify on the project's x86 Qt6 development machine and TX2 NX Qt6 target before deployment.
