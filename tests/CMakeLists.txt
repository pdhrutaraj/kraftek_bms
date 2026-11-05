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
