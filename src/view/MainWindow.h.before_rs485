#pragma once
#include <QMainWindow>
#include <QList>
#include <QVector>
#include <QPointF>
#include <QDateTime>

class QLabel;
class QTableView;
class QProgressBar;
class QStackedWidget;
class QGridLayout;
class QWidget;
class QResizeEvent;
class BatteryModel;
class DashboardController;
class TrendWidget;
class SocSparkline;
class QComboBox;
class QCheckBox;
class QPushButton;
class QDateTimeEdit;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(BatteryModel *model, DashboardController *controller, QWidget *parent = nullptr);
protected:
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void updateMetrics(double voltageV, double currentA, double socPct, double sohPct,
                       const QString &systemState, bool dataFresh, const QString &mode);
    void showPage(int page);
private:
    QWidget *makeMetricCard(const QString &eyebrow, QLabel **value, const QString &unit, const QString &accent);
    QWidget *makeBatteryCard(int index, QLabel **voltage, QLabel **temperature, QLabel **stateOfCharge,
                             QLabel **stateOfHealth, QProgressBar **socBar, QProgressBar **sohBar);
    QWidget *makeDashboardPage();
    QWidget *makeBatteryPage();
    QWidget *makeAlarmsPage();
    QWidget *makeSystemPage();
    QWidget *makeAiDemoPage();
    QWidget *makeTrendPage();
    void updateAiDemo(const QString &scenario, const QString &mlClass, double mlScore, const QString &summary);
    void updateResponsiveLayout();
    void loadHistory();
    BatteryModel *model_;
    DashboardController *controller_;
    QWidget *sidebar_ = nullptr;
    QWidget *telemetryBoxWidget_ = nullptr;
    QWidget *clockBoxWidget_ = nullptr;
    QWidget *sideSectionLabel_ = nullptr;
    QWidget *bankConfigLabel_ = nullptr;
    QWidget *modeNoteLabel_ = nullptr;
    QWidget *classificationLabel_ = nullptr;
    QWidget *brandSubLabel_ = nullptr;
    QWidget *screenSubtitleLabel_ = nullptr;
    QList<QPushButton *> navButtons_;
    QLabel *bankVoltageValue_ = nullptr;
    QLabel *currentValue_ = nullptr;
    QLabel *socValue_ = nullptr;
    QLabel *sohValue_ = nullptr;
    QLabel *systemStateValue_ = nullptr;
    QLabel *telemetryValue_ = nullptr;
    QLabel *clockValue_ = nullptr;
    QLabel *databaseStatusValue_ = nullptr;
    QLabel *diagnosticStatusValue_ = nullptr;
    QLabel *chargeStatusValue_ = nullptr;
    QLabel *weakestBatteryValue_ = nullptr;
    QLabel *batteryVoltage_[4] = {nullptr, nullptr, nullptr, nullptr};
    QLabel *batteryTemperature_[4] = {nullptr, nullptr, nullptr, nullptr};
    QLabel *batterySoc_[4] = {nullptr, nullptr, nullptr, nullptr};
    QLabel *batterySoh_[4] = {nullptr, nullptr, nullptr, nullptr};
    QProgressBar *batterySocBars_[4] = {nullptr, nullptr, nullptr, nullptr};
    QProgressBar *batterySohBars_[4] = {nullptr, nullptr, nullptr, nullptr};
    QProgressBar *bankSohBar_ = nullptr;
    SocSparkline *bankSocGraph_ = nullptr;
    QVector<double> bankSocHistory_;
    QStackedWidget *pages_ = nullptr;
    TrendWidget *trendWidget_ = nullptr;
    TrendWidget *historyChart_ = nullptr;
    QGridLayout *metricsGrid_ = nullptr;
    QGridLayout *batteryGrid_ = nullptr;
    QGridLayout *lowerGrid_ = nullptr;
    QList<QWidget *> lowerCards_;
    QList<QWidget *> metricCards_;
    QList<QWidget *> batteryCards_;
    QLabel *aiScenarioValue_ = nullptr;
    QLabel *aiClassValue_ = nullptr;
    QLabel *aiScoreValue_ = nullptr;
    QLabel *aiSummaryValue_ = nullptr;
    QComboBox *scenarioCombo_ = nullptr;
    QCheckBox *autoCycleCheck_ = nullptr;
    QDateTimeEdit *historyStart_ = nullptr;
    QDateTimeEdit *historyEnd_ = nullptr;
    QComboBox *historyScope_ = nullptr;
    QComboBox *historyMetric_ = nullptr;
    QLabel *historySummary_ = nullptr;
};
