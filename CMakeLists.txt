cmake_minimum_required(VERSION 3.21)
project(KraftekBmsQt6Mvc VERSION 0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Sql)

qt_add_executable(kraftek_bms
    src/main.cpp
    src/domain/BatteryTypes.h
    src/interfaces/IBatteryDataSource.h
    src/services/SimulatedDataSource.h
    src/services/SimulatedDataSource.cpp
    src/services/TelemetryDatabase.h
    src/services/TelemetryDatabase.cpp
    src/ai/DiagnosticEngine.h
    src/ai/DiagnosticEngine.cpp
    src/model/BatteryModel.h
    src/model/BatteryModel.cpp
    src/controller/DashboardController.h
    src/controller/DashboardController.cpp
    src/view/MainWindow.h
    src/view/MainWindow.cpp
    resources/resources.qrc
)

target_link_libraries(kraftek_bms PRIVATE Qt6::Core Qt6::Widgets Qt6::Sql)
target_include_directories(kraftek_bms PRIVATE src)

install(TARGETS kraftek_bms RUNTIME DESTINATION bin)
