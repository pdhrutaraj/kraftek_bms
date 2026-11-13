qt_add_executable(tst_rxnbmsprotocol
    tst_rxnbmsprotocol.cpp
    ../src/services/RxnBmsProtocol.h
    ../src/services/RxnBmsProtocol.cpp
    ../src/services/RxnBmsTransport.h
)

target_include_directories(tst_rxnbmsprotocol PRIVATE
    ../src
)

target_link_libraries(tst_rxnbmsprotocol PRIVATE
    Qt6::Core
    Qt6::SerialPort
    Qt6::Test
)

add_test(NAME tst_rxnbmsprotocol COMMAND tst_rxnbmsprotocol)

# Battery health calculator unit tests (header-only implementation).
qt_add_executable(tst_batteryhealthcalculator
    tst_batteryhealthcalculator.cpp
    ../src/services/BatteryHealthCalculator.h
    ../src/domain/BatteryTypes.h
)

target_include_directories(tst_batteryhealthcalculator PRIVATE
    ../src
)

target_link_libraries(tst_batteryhealthcalculator PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(
    NAME tst_batteryhealthcalculator
    COMMAND tst_batteryhealthcalculator
)

# Battery table model unit tests.
qt_add_executable(tst_batterymodel
    tst_batterymodel.cpp
    ../src/model/BatteryModel.h
    ../src/model/BatteryModel.cpp
    ../src/domain/BatteryTypes.h
)

target_include_directories(tst_batterymodel PRIVATE
    ../src
)

target_link_libraries(tst_batterymodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(
    NAME tst_batterymodel
    COMMAND tst_batterymodel
)

# Deterministic alarm engine unit tests.
qt_add_executable(tst_alarmengine
    tst_alarmengine.cpp
    ../src/services/AlarmEngine.h
    ../src/services/AlarmEngine.cpp
    ../src/domain/BatteryTypes.h
)

target_include_directories(tst_alarmengine PRIVATE
    ../src
)

target_link_libraries(tst_alarmengine PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(
    NAME tst_alarmengine
    COMMAND tst_alarmengine
)

# SQLite telemetry database unit tests.
qt_add_executable(tst_telemetrydatabase
    tst_telemetrydatabase.cpp
    ../src/services/TelemetryDatabase.h
    ../src/services/TelemetryDatabase.cpp
    ../src/domain/BatteryTypes.h
)

target_include_directories(tst_telemetrydatabase PRIVATE
    ../src
)

target_link_libraries(tst_telemetrydatabase PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Test
)

add_test(
    NAME tst_telemetrydatabase
    COMMAND tst_telemetrydatabase
)

# Diagnostic engine input-validation and synthetic advisory tests.
qt_add_executable(tst_diagnosticengine
    tst_diagnosticengine.cpp
    ../src/ai/DiagnosticEngine.h
    ../src/ai/DiagnosticEngine.cpp
    ../src/domain/BatteryTypes.h
)

target_include_directories(tst_diagnosticengine PRIVATE
    ../src
)

target_link_libraries(tst_diagnosticengine PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(
    NAME tst_diagnosticengine
    COMMAND tst_diagnosticengine
)
