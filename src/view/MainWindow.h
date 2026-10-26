#pragma once
#include <QMainWindow>
class QLabel;
class QTableView;
class QProgressBar;
class QStackedWidget;
class BatteryModel;
class DashboardController;
class TrendWidget;
class QComboBox;
class QCheckBox;
class QPushButton;

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
    QWidget *makeAiDemoPage();
    void updateAiDemo(const QString &scenario, const QString &mlClass, double mlScore, const QString &summary);
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
    QLabel *aiScenarioValue_ = nullptr;
    QLabel *aiClassValue_ = nullptr;
    QLabel *aiScoreValue_ = nullptr;
    QLabel *aiSummaryValue_ = nullptr;
    QComboBox *scenarioCombo_ = nullptr;
    QCheckBox *autoCycleCheck_ = nullptr;
};
