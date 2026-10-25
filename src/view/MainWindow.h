#pragma once
#include <QMainWindow>
class QLabel;
class QTableView;
class QProgressBar;
class QStackedWidget;
class BatteryModel;
class DashboardController;
class TrendWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(BatteryModel *model, DashboardController *controller, QWidget *parent = nullptr);
private slots:
    void updateMetrics(double voltageV, double currentA, double socPct, double sohPct,
                       const QString &systemState, bool dataFresh, const QString &mode);
    void showPage(int page);
private:
    QWidget *makeMetricCard(const QString &eyebrow, QLabel **value, const QString &unit, const QString &accent);
    QWidget *makeBatteryCard(int index, QLabel **voltage, QLabel **temperature, QLabel **stateOfCharge, QProgressBar **socBar);
    QWidget *makeDashboardPage();
    QWidget *makeBatteryPage();
    QWidget *makeAlarmsPage();
    QWidget *makeSystemPage();
    BatteryModel *model_;
    DashboardController *controller_;
    QLabel *bankVoltageValue_ = nullptr;
    QLabel *currentValue_ = nullptr;
    QLabel *socValue_ = nullptr;
    QLabel *sohValue_ = nullptr;
    QLabel *systemStateValue_ = nullptr;
    QLabel *telemetryValue_ = nullptr;
    QLabel *clockValue_ = nullptr;
    QLabel *databaseStatusValue_ = nullptr;
    QLabel *diagnosticStatusValue_ = nullptr;
    QLabel *batteryVoltage_[4] = {nullptr, nullptr, nullptr, nullptr};
    QLabel *batteryTemperature_[4] = {nullptr, nullptr, nullptr, nullptr};
    QLabel *batterySoc_[4] = {nullptr, nullptr, nullptr, nullptr};
    QProgressBar *batterySocBars_[4] = {nullptr, nullptr, nullptr, nullptr};
    QStackedWidget *pages_ = nullptr;
    TrendWidget *trendWidget_ = nullptr;
};
