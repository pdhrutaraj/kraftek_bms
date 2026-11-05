#include <QtTest>

#include "services/RxnBmsProtocol.h"

class RxnBmsProtocolTest : public QObject {
    Q_OBJECT

private slots:
    void testFrameworkIsAvailable() {
        QVERIFY(true);
    }
};

QTEST_GUILESS_MAIN(RxnBmsProtocolTest)

#include "tst_rxnbmsprotocol.moc"
