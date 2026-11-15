#include <QtTest>
#include <QApplication>
#include <QLabel>
#include <QProgressBar>
#include <QStandardPaths>
#include <QTimer>

#include <algorithm>
#include <memory>

#include "../src/model/BatteryModel.h"
#include "../src/controller/DashboardController.h"
#include "../src/view/MainWindow.h"
#include "../src/interfaces/IBatteryDataSource.h"

class FakeBatteryDataSource final : public IBatteryDataSource
{
public:
    FakeBatteryDataSource()
    {
        bank_.voltageV = 50.40;
        bank_.currentA = 12.0;
        bank_.socPct = 75.0;
        bank_.sohPct = 94.0;
        bank_.dataFresh = true;
        bank_.systemState = QStringLiteral("NORMAL");

        for (int i = 0; i < 4; ++i) {
            BatteryReading r;
            r.id = i + 1;
            r.voltageV = 12.50 + i * 0.01;
            r.temperatureC = 25.0 + i;
            r.socPct = 75.0 + i;
            r.sohPct = 94.0 - i;
            r.sensorValid = true;
            readings_.append(r);
        }
    }

    QList<BatteryReading> readBatteries() override { return readings_; }
    BankStatus readBankStatus() override { return bank_; }
    QString sourceId() const override { return QStringLiteral("DASHBOARD_TEST"); }

    QList<BatteryReading> &readings() { return readings_; }
    BankStatus &bank() { return bank_; }

private:
    QList<BatteryReading> readings_;
    BankStatus bank_;
};

class DashboardRegressionTest final : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<BatteryModel> model_;
    std::unique_ptr<DashboardController> controller_;
    std::unique_ptr<MainWindow> window_;
    FakeBatteryDataSource *source_ = nullptr;

    QList<QWidget *> cards() const
    {
        auto result = window_->findChildren<QWidget *>(
            QStringLiteral("batteryCard"));

        std::sort(result.begin(), result.end(),
                  [](QWidget *a, QWidget *b) {
            auto *la = a->findChild<QLabel *>("batteryName");
            auto *lb = b->findChild<QLabel *>("batteryName");
            return la && lb && la->text() < lb->text();
        });
        return result;
    }

    QLabel *status(QWidget *card) const
    {
        for (QLabel *label : card->findChildren<QLabel *>()) {
            if (label->objectName().startsWith(QStringLiteral("status")))
                return label;
        }
        return nullptr;
    }

    void verifyUnavailable(QWidget *card, const QString &expected)
    {
        auto *s = status(card);
        auto *voltage = card->findChild<QLabel *>("batteryVoltage");
        auto values = card->findChildren<QLabel *>("batteryStatValue");
        auto *soc = card->findChild<QProgressBar *>("socBar");
        auto *soh = card->findChild<QProgressBar *>("sohBar");

        QVERIFY(s);
        QVERIFY(voltage);
        QCOMPARE(values.size(), 3);
        QVERIFY(soc);
        QVERIFY(soh);

        QCOMPARE(s->text(), expected);
        QCOMPARE(voltage->text(), QStringLiteral("N/A"));
        for (auto *value : values)
            QCOMPARE(value->text(), QStringLiteral("N/A"));

        QVERIFY(!soc->isEnabled());
        QVERIFY(!soh->isEnabled());
        QCOMPARE(soc->value(), 0);
        QCOMPARE(soh->value(), 0);
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("KraftekTest");
        QCoreApplication::setApplicationName("DashboardRegression");
        QStandardPaths::setTestModeEnabled(true);
    }

    void init()
    {
        model_ = std::make_unique<BatteryModel>();

        auto fake = std::make_unique<FakeBatteryDataSource>();
        source_ = fake.get();

        controller_ = std::make_unique<DashboardController>(
            model_.get(), std::move(fake));

        window_ = std::make_unique<MainWindow>(
            model_.get(), controller_.get());

        // Disable the window's periodic refresh timer for deterministic tests.
        for (auto *timer : window_->findChildren<QTimer *>())
            timer->stop();

        QCOMPARE(cards().size(), 4);
    }

    void cleanup()
    {
        window_.reset();
        controller_.reset();
        model_.reset();
        source_ = nullptr;
    }

    void allFourBatteriesOnline()
    {
        const auto batteryCards = cards();
        QCOMPARE(batteryCards.size(), 4);

        for (int i = 0; i < 4; ++i) {
            auto *card = batteryCards.at(i);
            auto *s = status(card);
            auto *voltage = card->findChild<QLabel *>("batteryVoltage");
            auto values = card->findChildren<QLabel *>("batteryStatValue");
            auto *soc = card->findChild<QProgressBar *>("socBar");
            auto *soh = card->findChild<QProgressBar *>("sohBar");

            QVERIFY(s);
            QVERIFY(voltage);
            QCOMPARE(values.size(), 3);
            QVERIFY(soc);
            QVERIFY(soh);

            QCOMPARE(s->text(), QStringLiteral("● ONLINE"));
            QCOMPARE(voltage->text(),
                     QString::number(12.50 + i * 0.01, 'f', 2));
            QCOMPARE(values.at(0)->text(),
                     QString::number(25.0 + i, 'f', 1) + QStringLiteral(" °C"));
            QCOMPARE(values.at(1)->text(),
                     QString::number(75 + i) + QStringLiteral("%"));
            QCOMPARE(values.at(2)->text(),
                     QString::number(94.0 - i, 'f', 1) + QStringLiteral("%"));

            QVERIFY(soc->isEnabled());
            QVERIFY(soh->isEnabled());
            QCOMPARE(soc->value(), 75 + i);
            QCOMPARE(soh->value(), 94 - i);
        }
    }

    void oneInvalidSensorDoesNotHideOtherBatteries()
    {
        QVERIFY(source_);
        source_->readings()[1].sensorValid = false;
        controller_->refresh();

        const auto batteryCards = cards();
        QCOMPARE(batteryCards.size(), 4);

        verifyUnavailable(batteryCards.at(1),
                          QStringLiteral("● SENSOR FAULT"));

        for (int i : {0, 2, 3}) {
            auto *s = status(batteryCards.at(i));
            QVERIFY(s);
            QCOMPARE(s->text(), QStringLiteral("● ONLINE"));

            auto *voltage = batteryCards.at(i)->findChild<QLabel *>(
                "batteryVoltage");
            QVERIFY(voltage);
            QVERIFY(voltage->text() != QStringLiteral("N/A"));
        }
    }

    void staleAndMissingTelemetryAreUnavailable()
    {
        QVERIFY(source_);

        source_->bank().dataFresh = false;
        controller_->refresh();

        auto batteryCards = cards();
        QCOMPARE(batteryCards.size(), 4);
        for (auto *card : batteryCards)
            verifyUnavailable(card, QStringLiteral("● DATA UNAVAILABLE"));

        source_->bank().dataFresh = true;
        source_->readings().clear();
        controller_->refresh();

        batteryCards = cards();
        QCOMPARE(batteryCards.size(), 4);
        for (auto *card : batteryCards)
            verifyUnavailable(card, QStringLiteral("● DATA UNAVAILABLE"));
    }

    void validReadingRestoresBatteryAfterFault()
    {
        QVERIFY(source_);

        source_->readings()[0].sensorValid = false;
        controller_->refresh();

        auto batteryCards = cards();
        verifyUnavailable(batteryCards.at(0),
                          QStringLiteral("● SENSOR FAULT"));

        source_->readings()[0].sensorValid = true;
        controller_->refresh();

        batteryCards = cards();
        auto *s = status(batteryCards.at(0));
        auto *voltage = batteryCards.at(0)->findChild<QLabel *>(
            "batteryVoltage");
        auto *soc = batteryCards.at(0)->findChild<QProgressBar *>("socBar");
        auto *soh = batteryCards.at(0)->findChild<QProgressBar *>("sohBar");

        QVERIFY(s);
        QVERIFY(voltage);
        QVERIFY(soc);
        QVERIFY(soh);

        QCOMPARE(s->text(), QStringLiteral("● ONLINE"));
        QCOMPARE(voltage->text(), QStringLiteral("12.50"));
        QVERIFY(soc->isEnabled());
        QVERIFY(soh->isEnabled());
        QCOMPARE(soc->value(), 75);
        QCOMPARE(soh->value(), 94);
    }
};

QTEST_MAIN(DashboardRegressionTest)
#include "tst_dashboard.moc"
