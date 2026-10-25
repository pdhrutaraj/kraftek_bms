#include "MainWindow.h"
#include "../model/BatteryModel.h"
#include "../controller/DashboardController.h"
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPainterPath>
#include <QTableWidget>
#include <QtMath>
#include <QStyle>
#include <QWidget>
#include <QScrollArea>

class TrendWidget final : public QWidget {
public:
    explicit TrendWidget(QWidget *parent = nullptr) : QWidget(parent) { setMinimumHeight(150); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        const QRectF area = rect().adjusted(14, 12, -14, -20);
        p.setPen(QPen(QColor("#273746"), 1, Qt::DashLine));
        for (int i = 0; i < 4; ++i) { const qreal y = area.top() + area.height() * i / 3.0; p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y)); }
        const QColor colors[] = {QColor("#45d6bd"), QColor("#63a8ff"), QColor("#e5b85c")};
        for (int line = 0; line < 3; ++line) {
            QPainterPath path;
            for (int i = 0; i < 50; ++i) {
                const qreal x = area.left() + area.width() * i / 49.0;
                const qreal wave = 0.48 + 0.12 * qSin((i + line * 7) * 0.22) + 0.035 * qSin(i * 0.63 + line);
                const qreal y = area.bottom() - wave * area.height();
                if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
            }
            p.setPen(QPen(colors[line], 2.2)); p.drawPath(path);
        }
    }
};

static QLabel *label(const QString &text, const QString &objectName = {}) {
    auto *l = new QLabel(text); if (!objectName.isEmpty()) l->setObjectName(objectName); return l;
}
static QFrame *panel(const QString &objectName = QStringLiteral("panel")) {
    auto *f = new QFrame; f->setObjectName(objectName); f->setFrameShape(QFrame::NoFrame); return f;
}

MainWindow::MainWindow(BatteryModel *model, DashboardController *controller, QWidget *parent)
    : QMainWindow(parent), model_(model), controller_(controller) {
    setWindowTitle(QStringLiteral("Kraftek| Integrated Battery Monitoring System"));
    setMinimumSize(1100, 720); resize(1440, 900);
    auto *root = new QWidget(this); root->setObjectName("appRoot");
    auto *outer = new QVBoxLayout(root); outer->setContentsMargins(0,0,0,0); outer->setSpacing(0);

    auto *top = new QFrame; top->setObjectName("topBar"); top->setFixedHeight(76);
    auto *topLayout = new QHBoxLayout(top); topLayout->setContentsMargins(24,10,24,10); topLayout->setSpacing(18);
    auto *brand = new QVBoxLayout; brand->setSpacing(2);
    auto *brandName = label("Kraftek", "brandName");
    auto *brandSub = label("POWER SYSTEMS  /  BATTERY MONITORING", "brandSub");
    brand->addWidget(brandName); brand->addWidget(brandSub);
    auto *separator = new QFrame; separator->setObjectName("topSeparator"); separator->setFixedWidth(1);
    auto *screenName = new QVBoxLayout; screenName->setSpacing(3);
    screenName->addWidget(label("Battery System Console", "screenTitle"));
    screenName->addWidget(label("48 V CRANKING BANK  •  OPERATOR DISPLAY", "screenSubtitle"));
    topLayout->addLayout(brand); topLayout->addWidget(separator); topLayout->addLayout(screenName); topLayout->addStretch();
    auto *telemetryBox = new QVBoxLayout; telemetryBox->setSpacing(3);
    telemetryBox->addWidget(label("TELEMETRY LINK", "microLabel"));
    telemetryValue_ = label("●  SIMULATION ACTIVE", "linkPill"); telemetryBox->addWidget(telemetryValue_);
    topLayout->addLayout(telemetryBox);
    auto *clockBox = new QVBoxLayout; clockBox->setSpacing(3);
    clockBox->addWidget(label("LOCAL SYSTEM TIME", "microLabel"));
    clockValue_ = label("--:--:--", "clockValue"); clockBox->addWidget(clockValue_);
    topLayout->addLayout(clockBox);
    outer->addWidget(top);

    auto *body = new QWidget; auto *bodyLayout = new QHBoxLayout(body); bodyLayout->setContentsMargins(0,0,0,0); bodyLayout->setSpacing(0);
    auto *sidebar = new QFrame; sidebar->setObjectName("sidebar"); sidebar->setFixedWidth(218);
    auto *side = new QVBoxLayout(sidebar); side->setContentsMargins(14,22,14,18); side->setSpacing(8);
    side->addWidget(label("WORKSPACE", "sideSection"));
    const QString navNames[] = {"OVERVIEW", "BATTERY DETAILS", "ALARMS & EVENTS", "SYSTEM STATUS"};
    for (int i = 0; i < 4; ++i) {
        auto *btn = new QPushButton(navNames[i]); btn->setObjectName(i == 0 ? "navButtonActive" : "navButton");
        btn->setCheckable(true); btn->setProperty("pageIndex", i); if (i == 0) btn->setChecked(true);
        side->addWidget(btn); connect(btn, &QPushButton::clicked, this, [this, i, sidebar, btn]() {
            showPage(i);
            for (auto *b : sidebar->findChildren<QPushButton*>()) { b->setChecked(b == btn); b->setObjectName(b == btn ? "navButtonActive" : "navButton"); b->style()->unpolish(b); b->style()->polish(b); }
        });
    }
    side->addSpacing(18); side->addWidget(label("BANK CONFIGURATION", "sideSection"));
    auto *bankConfig = label("48 V DC\n4 × 12 V / 180 Ah\nLEAD-ACID SERIES", "sideNote"); bankConfig->setWordWrap(true); side->addWidget(bankConfig);
    auto *modeNote = label("External display/HMI. BMS slave has no physical screen.", "sideNote"); modeNote->setWordWrap(true); side->addWidget(modeNote);
    side->addStretch();
    auto *classification = label("PROTOTYPE  •  DEMO DATA", "classification"); classification->setAlignment(Qt::AlignCenter); side->addWidget(classification);
    bodyLayout->addWidget(sidebar);

    pages_ = new QStackedWidget; pages_->setObjectName("pageStack");
    pages_->addWidget(makeDashboardPage()); pages_->addWidget(makeBatteryPage()); pages_->addWidget(makeAlarmsPage()); pages_->addWidget(makeSystemPage());
    bodyLayout->addWidget(pages_, 1); outer->addWidget(body, 1);

    auto *footer = new QFrame; footer->setObjectName("footerBar"); footer->setFixedHeight(32);
    auto *foot = new QHBoxLayout(footer); foot->setContentsMargins(20,4,20,4);
    foot->addWidget(label("KRAFTEK BMS  •  SOFTWARE BUILD 0.5", "footerText")); foot->addStretch();
    foot->addWidget(label("SIMULATED TELEMETRY — NOT FOR OPERATIONAL CONTROL", "footerWarning")); outer->addWidget(footer);
    setCentralWidget(root);

    
    connect(controller_, &DashboardController::bankMetricsChanged, this, &MainWindow::updateMetrics);
    connect(controller_, &DashboardController::databaseStatusChanged, this, [this](const QString &s) { if (databaseStatusValue_) databaseStatusValue_->setText(s); });
    connect(controller_, &DashboardController::diagnosticsChanged, this, [this](const QString &status, const QString &summary, double score) { if (diagnosticStatusValue_) diagnosticStatusValue_->setText(QStringLiteral("%1 · score %2/100 — %3").arg(status).arg(score,0,'f',0).arg(summary)); });
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { clockValue_->setText(QDateTime::currentDateTime().toString("dd MMM yyyy  •  HH:mm:ss")); controller_->refresh(); });
    timer->start(1000); clockValue_->setText(QDateTime::currentDateTime().toString("dd MMM yyyy  •  HH:mm:ss"));
    controller_->refresh();
}

QWidget *MainWindow::makeMetricCard(const QString &eyebrow, QLabel **value, const QString &unit, const QString &accent) {
    auto *card = panel("metricCard"); card->setProperty("accent", accent);
    auto *l = new QVBoxLayout(card); l->setContentsMargins(18,16,18,16); l->setSpacing(6);
    l->addWidget(label(eyebrow, "metricEyebrow"));
    auto *row = new QHBoxLayout; *value = label("--", "metricValue"); row->addWidget(*value); row->addWidget(label(unit, "metricUnit"), 0, Qt::AlignBottom); row->addStretch(); l->addLayout(row);
    l->addStretch(); return card;
}

QWidget *MainWindow::makeBatteryCard(int index, QLabel **voltage, QLabel **temperature, QLabel **stateOfCharge, QProgressBar **socBar) {
    auto *card = panel("batteryCard"); auto *l = new QVBoxLayout(card); l->setContentsMargins(18,16,18,16); l->setSpacing(10);
    auto *head = new QHBoxLayout; head->addWidget(label(QStringLiteral("BATTERY %1").arg(index + 1), "batteryName")); head->addStretch(); head->addWidget(label("● ONLINE", "statusOnline")); l->addLayout(head);
    auto *voltageRow = new QHBoxLayout; *voltage = label("12.58", "batteryVoltage"); voltageRow->addWidget(*voltage); voltageRow->addWidget(label("V DC", "metricUnit"), 0, Qt::AlignBottom); voltageRow->addStretch(); l->addLayout(voltageRow);
    l->addWidget(label("TERMINAL VOLTAGE", "microLabel"));
    auto *sep = new QFrame; sep->setObjectName("thinSeparator"); sep->setFixedHeight(1); l->addWidget(sep);
    auto *stats = new QGridLayout; stats->setHorizontalSpacing(10); stats->setVerticalSpacing(5);
    stats->addWidget(label("TEMPERATURE", "microLabel"),0,0); stats->addWidget(label("STATE OF CHARGE", "microLabel"),0,1);
    *temperature = label("27.0 °C", "batteryStatValue"); *stateOfCharge = label("92%", "batteryStatValue");
    stats->addWidget(*temperature,1,0); stats->addWidget(*stateOfCharge,1,1); l->addLayout(stats);
    *socBar = new QProgressBar; (*socBar)->setObjectName("socBar"); (*socBar)->setRange(0,100); (*socBar)->setValue(92); (*socBar)->setTextVisible(false); (*socBar)->setFixedHeight(6); l->addWidget(*socBar);
    return card;
}

QWidget *MainWindow::makeDashboardPage() {
    auto *page = new QWidget; auto *root = new QVBoxLayout(page); root->setContentsMargins(24,22,24,18); root->setSpacing(16);
    auto *heading = new QHBoxLayout; auto *titles = new QVBoxLayout; titles->setSpacing(4);
    titles->addWidget(label("Operational overview", "pageHeading")); titles->addWidget(label("Live battery-bank telemetry and condition summary", "pageDescription"));
    heading->addLayout(titles); heading->addStretch(); systemStateValue_ = label("NORMAL", "statePill"); heading->addWidget(systemStateValue_,0,Qt::AlignVCenter); root->addLayout(heading);
    auto *metrics = new QGridLayout; metrics->setSpacing(12);
    metrics->addWidget(makeMetricCard("BANK VOLTAGE", &bankVoltageValue_, "V DC", "teal"),0,0);
    metrics->addWidget(makeMetricCard("BANK CURRENT", &currentValue_, "A", "blue"),0,1);
    metrics->addWidget(makeMetricCard("STATE OF CHARGE", &socValue_, "%", "gold"),0,2);
    metrics->addWidget(makeMetricCard("STATE OF HEALTH", &sohValue_, "%", "violet"),0,3);
    root->addLayout(metrics);
    auto *batteryHeading = new QHBoxLayout; batteryHeading->addWidget(label("BATTERY STRING", "sectionHeading")); batteryHeading->addStretch(); batteryHeading->addWidget(label("4 UNITS  /  12 V • 180 Ah EACH", "sectionMeta")); root->addLayout(batteryHeading);
    auto *batteries = new QGridLayout; batteries->setSpacing(12);
    for (int i=0;i<4;++i) batteries->addWidget(makeBatteryCard(i,&batteryVoltage_[i],&batteryTemperature_[i],&batterySoc_[i],&batterySocBars_[i]),0,i);
    root->addLayout(batteries);
    auto *lower = new QGridLayout; lower->setSpacing(12);
    auto *trendPanel = panel(); auto *trendLayout = new QVBoxLayout(trendPanel); trendLayout->setContentsMargins(18,15,18,12); trendLayout->setSpacing(8);
    auto *trendHead = new QHBoxLayout; trendHead->addWidget(label("TELEMETRY TREND", "panelHeading")); trendHead->addStretch(); trendHead->addWidget(label("ILLUSTRATIVE SIMULATION", "sectionMeta")); trendLayout->addLayout(trendHead);
    trendWidget_ = new TrendWidget; trendLayout->addWidget(trendWidget_); lower->addWidget(trendPanel,0,0,1,2);
    auto *alarmPanel = panel(); auto *alarmLayout = new QVBoxLayout(alarmPanel); alarmLayout->setContentsMargins(18,15,18,12); alarmLayout->setSpacing(10);
    alarmLayout->addWidget(label("SYSTEM EVENTS", "panelHeading"));
    auto *event1 = new QHBoxLayout; event1->addWidget(label("●", "eventDotGreen")); auto *ev1 = new QVBoxLayout; ev1->addWidget(label("Telemetry simulator connected", "eventTitle")); ev1->addWidget(label("No hardware link is active", "eventMeta")); event1->addLayout(ev1,1); alarmLayout->addLayout(event1);
    auto *event2 = new QHBoxLayout; event2->addWidget(label("●", "eventDotAmber")); auto *ev2 = new QVBoxLayout; ev2->addWidget(label("Operational controls inhibited", "eventTitle")); ev2->addWidget(label("Prototype / simulation mode", "eventMeta")); event2->addLayout(ev2,1); alarmLayout->addLayout(event2);
    alarmLayout->addStretch(); lower->addWidget(alarmPanel,0,2,1,1); root->addLayout(lower,1);
    return page;
}

QWidget *MainWindow::makeBatteryPage() {
    auto *page = new QWidget; auto *l = new QVBoxLayout(page); l->setContentsMargins(24,22,24,22); l->setSpacing(16);
    l->addWidget(label("Battery details", "pageHeading")); l->addWidget(label("B1–B4 block voltage, terminal-lug temperature, SoC/SoH estimates and sensor validity", "pageDescription"));
    auto *table = new QTableView; table->setObjectName("dataTable"); table->setModel(model_); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->verticalHeader()->hide(); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setAlternatingRowColors(true); table->setMinimumHeight(210); l->addWidget(table);
    auto *note = panel(); auto *nl = new QVBoxLayout(note); nl->setContentsMargins(18,16,18,16); nl->addWidget(label("MEASUREMENT NOTE", "panelHeading")); auto *txt = label("Values on this screen are generated by the simulator. Accuracy, calibration status, isolation ratings and sensor-fault behavior must be verified against the selected hardware before operational use.", "pageDescription"); txt->setWordWrap(true); nl->addWidget(txt); l->addWidget(note); l->addStretch(); return page;
}

QWidget *MainWindow::makeAlarmsPage() {
    auto *page = new QWidget; auto *l = new QVBoxLayout(page); l->setContentsMargins(24,22,24,22); l->setSpacing(16);
    l->addWidget(label("Alarms & events", "pageHeading")); l->addWidget(label("Alarm presentation and event history prototype", "pageDescription"));
    auto *notice = panel(); auto *n = new QVBoxLayout(notice); n->setContentsMargins(18,16,18,16); n->addWidget(label("NO LIVE ALARM INPUT", "alarmHeadline")); auto *txt=label("Protection thresholds in this screen are specification references only; this prototype does not receive hardware alarms. The items below are software status notices, not evidence that the physical battery bank is safe.","pageDescription"); txt->setWordWrap(true); n->addWidget(txt); l->addWidget(notice);
    auto *table = new QTableWidget(3,3); table->setObjectName("eventTable"); table->setHorizontalHeaderLabels({"TIME", "CLASS", "MESSAGE"}); table->verticalHeader()->hide(); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setSelectionBehavior(QAbstractItemView::SelectRows);
    const QStringList classes={"INFO","INHIBIT","INFO"}; const QStringList messages={"Simulator started","Physical control outputs are not connected","No live hardware telemetry received"};
    for(int r=0;r<3;++r){table->setItem(r,0,new QTableWidgetItem(QDateTime::currentDateTime().toString("HH:mm:ss")));table->setItem(r,1,new QTableWidgetItem(classes[r]));table->setItem(r,2,new QTableWidgetItem(messages[r]));} l->addWidget(table,1); return page;
}

QWidget *MainWindow::makeSystemPage() {
    auto *page = new QWidget; auto *l = new QVBoxLayout(page); l->setContentsMargins(24,22,24,22); l->setSpacing(16);
    l->addWidget(label("System status", "pageHeading")); l->addWidget(label("Application build and integration readiness", "pageDescription"));
    auto *grid = new QGridLayout; grid->setSpacing(12);
    const QStringList names={"DATA SOURCE","SQLITE3 DATABASE","CAN / RS-485 INTERFACE","PROTECTION CONTROLLER","AI / ML DIAGNOSTICS","PHYSICAL CONTACTORS"};
    const QStringList states={"SIMULATOR ACTIVE","INITIALIZING DATABASE…","NOT INTEGRATED","NOT VALIDATED","INITIALIZING DIAGNOSTICS…","NOT CONNECTED"};
    for(int i=0;i<names.size();++i){auto *c=panel();auto *cl=new QVBoxLayout(c);cl->setContentsMargins(18,16,18,16);cl->addWidget(label(names[i],"metricEyebrow"));auto *v=label(states[i],"systemStateValue");v->setWordWrap(true);if(i==1)databaseStatusValue_=v;if(i==4)diagnosticStatusValue_=v;cl->addWidget(v);grid->addWidget(c,i/2,i%2);} l->addLayout(grid);
    auto *warning=panel();auto *wl=new QVBoxLayout(warning);wl->setContentsMargins(18,16,18,16);wl->addWidget(label("SAFETY / RELEASE GATE", "panelHeading"));auto *text=label("SQLite records HMI telemetry and diagnostic events. AI/ML diagnostics are advisory statistical heuristics only—not a trained, validated, or safety-certified model. Protection remains deterministic and local to the BMS hardware. This interface is a prototype visualization only; do not use it for operational battery control until sensing, protection, communications, fail-safe behavior and acceptance tests are independently verified.","pageDescription");text->setWordWrap(true);wl->addWidget(text);l->addWidget(warning);l->addStretch();return page;
}

void MainWindow::showPage(int page) { if (pages_ && page >= 0 && page < pages_->count()) pages_->setCurrentIndex(page); }

void MainWindow::updateMetrics(double voltageV, double currentA, double socPct, double sohPct,
                               const QString &systemState, bool dataFresh, const QString &mode) {
    bankVoltageValue_->setText(QString::number(voltageV, 'f', 2));
    currentValue_->setText(QString::number(currentA, 'f', 1));
    socValue_->setText(QString::number(qRound(socPct)));
    sohValue_->setText(QString::number(qRound(sohPct)));
    systemStateValue_->setText(systemState);
    if (!dataFresh) { telemetryValue_->setText("●  DATA STALE"); telemetryValue_->setObjectName("linkPillBad"); }
    else telemetryValue_->setText("●  SIMULATION ACTIVE");
    Q_UNUSED(mode);
    const auto readings = model_->readings();
    for (int i=0;i<readings.size() && i<4;++i) {
        const auto &r=readings.at(i);
        if (batteryVoltage_[i]) batteryVoltage_[i]->setText(QString::number(r.voltageV,'f',2));
        if (batteryTemperature_[i]) batteryTemperature_[i]->setText(QString::number(r.temperatureC,'f',1)+" °C");
        if (batterySoc_[i]) batterySoc_[i]->setText(QString::number(qRound(r.socPct))+"%");
        if (batterySocBars_[i]) batterySocBars_[i]->setValue(qRound(r.socPct));
    }
}
