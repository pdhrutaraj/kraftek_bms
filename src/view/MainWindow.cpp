#include "MainWindow.h"
#include "../model/BatteryModel.h"
#include "../controller/DashboardController.h"
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QDateTime>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QPixmap>
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
#include <QSignalBlocker>
#include <QLayout>
#include <QDateTimeEdit>
#include <QDateTime>
#include <QScreen>
#include <QGuiApplication>
#include <QResizeEvent>
#include <QSerialPortInfo>
#include <QMetaObject>
#include <QByteArray>
#include <QChar>
#include <QSerialPort>
#include <QThread>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QElapsedTimer>
#include <QStringList>
#include <QScrollArea>
#include <QSizePolicy>
#include <QPainterPath>
#include <QDateTime>
#include <QApplication>
#include <QMessageBox>
#include <QProcess>
#include <QProcessEnvironment>

class TrendWidget final : public QWidget {
public:
    explicit TrendWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setMinimumHeight(180);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
    void setData(const QVector<QPointF> &points, const QString &title, const QString &unit,
                 const QString &startLabel = {}, const QString &endLabel = {}) {
        points_ = points; title_ = title; unit_ = unit;
        startLabel_ = startLabel; endLabel_ = endLabel; update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this); p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#111b25"));
        const QRectF area = rect().adjusted(52, 28, -18, -34);
        p.setPen(QColor("#9db0bf"));
        p.drawText(QRectF(12, 4, width()-24, 20), Qt::AlignLeft|Qt::AlignVCenter,
                   title_.isEmpty() ? QStringLiteral("Historical trend") : title_);
        if (points_.isEmpty()) {
            p.setPen(QColor("#8ea1b1"));
            p.drawText(area, Qt::AlignCenter, QStringLiteral("No historical samples in selected time range"));
            return;
        }
        double minX=points_.first().x(), maxX=points_.last().x();
        double minY=points_.first().y(), maxY=minY;
        for (const QPointF &pt : points_) { minY=qMin(minY,pt.y()); maxY=qMax(maxY,pt.y()); }
        if (maxX <= minX) maxX=minX+1.0;
        if (qFuzzyCompare(minY, maxY)) { minY-=1.0; maxY+=1.0; }
        const double pad=(maxY-minY)*0.10; minY-=pad; maxY+=pad;
        p.setPen(QPen(QColor("#2b3d4b"), 1, Qt::DashLine));
        for (int i=0;i<5;++i) {
            const qreal y=area.top()+area.height()*i/4.0;
            p.drawLine(QPointF(area.left(),y), QPointF(area.right(),y));
            const double value=maxY-(maxY-minY)*i/4.0;
            p.setPen(QColor("#91a5b5"));
            p.drawText(QRectF(2,y-8,44,16),Qt::AlignRight|Qt::AlignVCenter,QString::number(value,'f',1));
            p.setPen(QPen(QColor("#2b3d4b"),1,Qt::DashLine));
        }
        QPainterPath path;
        for (int i=0;i<points_.size();++i) {
            const QPointF &pt=points_.at(i);
            const qreal x=area.left()+(pt.x()-minX)/(maxX-minX)*area.width();
            const qreal y=area.bottom()-(pt.y()-minY)/(maxY-minY)*area.height();
            if (i==0) path.moveTo(x,y); else path.lineTo(x,y);
        }
        p.setPen(QPen(QColor("#45d6bd"),2.4)); p.drawPath(path);
        p.setPen(QColor("#8ea1b1"));
        p.drawText(QRectF(area.left(),area.bottom()+5,area.width(),18),Qt::AlignLeft|Qt::AlignVCenter,startLabel_);
        p.drawText(QRectF(area.left(),area.bottom()+5,area.width(),18),Qt::AlignRight|Qt::AlignVCenter,endLabel_);
        p.drawText(QRectF(4,area.top(),18,area.height()),Qt::AlignCenter,unit_);
    }
private:
    QVector<QPointF> points_;
    QString title_, unit_, startLabel_, endLabel_;
};


class SocSparkline final : public QWidget {
public:
    explicit SocSparkline(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedHeight(38);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    void setValues(const QVector<double> &values) { values_ = values; update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), Qt::transparent);
        const QRectF plot = rect().adjusted(2, 3, -2, -4);
        painter.setPen(QPen(QColor("#2b3d4b"), 1, Qt::DashLine));
        painter.drawLine(plot.left(), plot.center().y(), plot.right(), plot.center().y());
        if (values_.size() < 2) {
            painter.setPen(QColor("#8499aa"));
            painter.drawText(plot, Qt::AlignCenter, QStringLiteral("SOC trend"));
            return;
        }
        QPainterPath path;
        for (int i = 0; i < values_.size(); ++i) {
            const qreal x = plot.left() + (plot.width() * i / qMax(1, values_.size() - 1));
            const qreal y = plot.bottom() - (qBound(0.0, values_.at(i), 100.0) / 100.0) * plot.height();
            if (i == 0) path.moveTo(x, y); else path.lineTo(x, y);
        }
        painter.setPen(QPen(QColor("#e4bd63"), 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
    }
private:
    QVector<double> values_;
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
    setMinimumSize(640, 420);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QRect available = screen->availableGeometry();
        resize(qMin(1440, available.width()), qMin(900, available.height()));
    } else { resize(1100, 720); }
    auto *root = new QWidget(this); root->setObjectName("appRoot");
    auto *outer = new QVBoxLayout(root); outer->setContentsMargins(0,0,0,0); outer->setSpacing(0);

    auto *top = new QFrame; top->setObjectName("topBar"); top->setFixedHeight(76);
    auto *topLayout = new QHBoxLayout(top); topLayout->setContentsMargins(24,10,24,10); topLayout->setSpacing(18);
    auto *brand = new QVBoxLayout; brand->setSpacing(2);
    auto *brandLogo = new QLabel;
    brandLogo->setObjectName("brandLogo");
    brandLogo->setFixedSize(170, 42);
    brandLogo->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    const QPixmap logo(QStringLiteral(":/images/kraftek_logo.png"));
    if (!logo.isNull()) {
        brandLogo->setPixmap(logo.scaled(
            brandLogo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        brandLogo->setText(QStringLiteral("Kraftek"));
        brandLogo->setObjectName("brandName");
    }

    auto *brandSub = label("POWER SYSTEMS  /  BATTERY MONITORING", "brandSub");
    brandSubLabel_ = brandSub;
    brand->addWidget(brandLogo);
    brand->addWidget(brandSub);
    auto *separator = new QFrame; separator->setObjectName("topSeparator"); separator->setFixedWidth(1);
    auto *screenName = new QVBoxLayout; screenName->setSpacing(3);
    screenName->addWidget(label("Battery System Console", "screenTitle"));
    auto *screenSubtitle = label("48 V CRANKING BANK  •  OPERATOR DISPLAY", "screenSubtitle"); screenSubtitleLabel_ = screenSubtitle; screenName->addWidget(screenSubtitle);
    topLayout->addLayout(brand); topLayout->addWidget(separator); topLayout->addLayout(screenName); topLayout->addStretch();
    telemetryBoxWidget_ = new QWidget; auto *telemetryBox = new QVBoxLayout(telemetryBoxWidget_); telemetryBox->setContentsMargins(0,0,0,0); telemetryBox->setSpacing(3);
    telemetryBox->addWidget(label("TELEMETRY LINK", "microLabel"));
    const bool isSimulator = controller_ && controller_->sourceId() == QStringLiteral("SIMULATOR");
    telemetryValue_ = label(isSimulator ? "●  SIMULATOR ACTIVE" : "●  RXN BMS NOT CONNECTED",
                            isSimulator ? "linkPill" : "linkPillBad");
    telemetryBox->addWidget(telemetryValue_);
    topLayout->addWidget(telemetryBoxWidget_);
    clockBoxWidget_ = new QWidget; auto *clockBox = new QVBoxLayout(clockBoxWidget_); clockBox->setContentsMargins(0,0,0,0); clockBox->setSpacing(3);
    clockBox->addWidget(label("LOCAL SYSTEM TIME", "microLabel"));
    clockValue_ = label("--:--:--", "clockValue"); clockBox->addWidget(clockValue_);
    topLayout->addWidget(clockBoxWidget_);
    //
    //topLayout->addWidget(clockBoxWidget_);

// TX2 shutdown button
auto *shutdownButton = new QPushButton(
    QStringLiteral("SHUT DOWN BMS"), top);

shutdownButton->setObjectName("shutdownButton");
shutdownButton->setMinimumWidth(130);
shutdownButton->setMinimumHeight(40);
shutdownButton->setToolTip(
    QStringLiteral("Safely shut down the Kraftek application and Jetson TX2 NX"));

shutdownButton->setStyleSheet(
    "QPushButton {"
    " background-color: #a8323e;"
    " color: white;"
    " border: none;"
    " border-radius: 6px;"
    " padding: 8px 12px;"
    " font-weight: bold;"
    "}"
    "QPushButton:hover { background-color: #c94350; }"
    "QPushButton:pressed { background-color: #76232b; }"
);

topLayout->addWidget(shutdownButton);

connect(shutdownButton, &QPushButton::clicked,
        this, [this]() {

    const auto answer = QMessageBox::warning(
        this,
        QStringLiteral("Confirm TX2 Shutdown"),
        QStringLiteral(
            "Are you sure you want to shut down "
            "the Kraftek BMS application and Jetson TX2 NX?\n\n"
            "Administrator authentication will be required."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (answer != QMessageBox::Yes)
        return;

    auto *process = new QProcess(this);

    QProcessEnvironment env =
        QProcessEnvironment::systemEnvironment();

    env.insert(
        QStringLiteral("SUDO_ASKPASS"),
        QStringLiteral(
            "/home/user/kraftek_bms/scripts/shutdown_askpass.sh")
    );

    process->setProcessEnvironment(env);

    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process](int exitCode,
                            QProcess::ExitStatus exitStatus) {

        const QString errorText =
            QString::fromLocal8Bit(
                process->readAllStandardError()).trimmed();

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QMessageBox::information(
                this,
                QStringLiteral("Shutdown Accepted"),
                QStringLiteral(
                    "The operating system accepted the shutdown request.\n"
                    "The Jetson TX2 NX is powering off.")
            );
        } else {
            QMessageBox::warning(
                this,
                QStringLiteral("Shutdown Cancelled or Failed"),
                QStringLiteral(
                    "The TX2 has not been shut down.\n\n"
                    "%1")
                    .arg(errorText.isEmpty()
                         ? QStringLiteral("Authentication was cancelled, "
                                           "or the shutdown command failed "
                                           "(exit code %1).").arg(exitCode)
                         : errorText)
            );
        }

        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred,
            this, [this, process](QProcess::ProcessError error) {

        if (error == QProcess::FailedToStart) {
            QMessageBox::critical(
                this,
                QStringLiteral("Shutdown Error"),
                QStringLiteral(
                    "Could not start sudo.\n"
                    "Check that /usr/bin/sudo is available.")
            );

            process->deleteLater();
        }
    });

    process->start(
        QStringLiteral("/usr/bin/sudo"),
        QStringList{
            QStringLiteral("-k"),
            QStringLiteral("-A"),
            QStringLiteral("/bin/systemctl"),
            QStringLiteral("poweroff")
        }
    );
});

/*
connect(shutdownButton, &QPushButton::clicked,
        this, [this]() {

    const auto answer = QMessageBox::warning(
        this,
        QStringLiteral("Confirm TX2 Shutdown"),
        QStringLiteral(
            "Are you sure you want to shut down the Kraftek BMS "
            "application and Jetson TX2 NX?\n\n"
            "The system will power off."),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (answer != QMessageBox::Yes)
        return;

    auto *process = new QProcess(this);

    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this, process](int exitCode,
                            QProcess::ExitStatus exitStatus) {

        const QString errorText =
            QString::fromLocal8Bit(
                process->readAllStandardError()).trimmed();

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            QApplication::quit();
        } else {
            QMessageBox::critical(
                this,
                QStringLiteral("Shutdown Failed"),
                QStringLiteral(
                    "The operating system rejected the shutdown request.\n\n%1")
                    .arg(errorText.isEmpty()
                         ? QStringLiteral("systemctl returned exit code %1")
                               .arg(exitCode)
                         : errorText)
            );
        }

        process->deleteLater();
    });

    connect(process, &QProcess::errorOccurred,
            this, [this, process](QProcess::ProcessError error) {

        if (error == QProcess::FailedToStart) {
            QMessageBox::critical(
                this,
                QStringLiteral("Shutdown Error"),
                QStringLiteral(
                    "Could not start systemctl. "
                    "Check that systemctl is installed and accessible.")
            );

            process->deleteLater();
        }
    });

    process->start(
        QStringLiteral("systemctl"),
        QStringList{QStringLiteral("poweroff")}
    );
});
*/
//outer->addWidget(top);
    //
    outer->addWidget(top);

    auto *body = new QWidget; auto *bodyLayout = new QHBoxLayout(body); bodyLayout->setContentsMargins(0,0,0,0); bodyLayout->setSpacing(0);
    auto *sidebar = new QFrame; sidebar_ = sidebar; sidebar->setObjectName("sidebar"); sidebar->setMinimumWidth(68); sidebar->setMaximumWidth(218); sidebar->setFixedWidth(218);
    auto *side = new QVBoxLayout(sidebar); side->setContentsMargins(14,22,14,18); side->setSpacing(8);
    sideSectionLabel_ = label("WORKSPACE", "sideSection"); side->addWidget(sideSectionLabel_);
    const QString navNames[] = {"OVERVIEW", "BATTERY DETAILS", "ALARMS & EVENTS", "SYSTEM STATUS", "HISTORICAL TRENDS", "AI ADVISORY", "RS-485 DIAGNOSTICS"};
    const QString navShort[] = {"HOME", "BATTERIES", "EVENTS", "SYSTEM", "TRENDS", "AI", "RS-485"};
    for (int i = 0; i < 7; ++i) {
        auto *btn = new QPushButton(navNames[i]); btn->setObjectName(i == 0 ? "navButtonActive" : "navButton");
        btn->setCheckable(true); btn->setProperty("pageIndex", i); btn->setProperty("fullText", navNames[i]); btn->setProperty("shortText", navShort[i]); navButtons_.append(btn); if (i == 0) btn->setChecked(true);
        side->addWidget(btn); connect(btn, &QPushButton::clicked, this, [this, i, sidebar, btn]() {
            showPage(i);
            for (auto *b : sidebar->findChildren<QPushButton*>()) { b->setChecked(b == btn); b->setObjectName(b == btn ? "navButtonActive" : "navButton"); b->style()->unpolish(b); b->style()->polish(b); }
        });
    }
    side->addSpacing(18); bankConfigLabel_ = label("BANK CONFIGURATION", "sideSection"); side->addWidget(bankConfigLabel_);
    auto *bankConfig = label("48 V DC\n4 × 12 V / 180 Ah\nLEAD-ACID SERIES", "sideNote"); bankConfig->setWordWrap(true); side->addWidget(bankConfig);
    auto *modeNote = label("External display/HMI. BMS slave has no physical screen.", "sideNote"); modeNoteLabel_ = modeNote; modeNote->setWordWrap(true); side->addWidget(modeNote);
    side->addStretch();
    auto *classification = label(
        isSimulator ? "PROTOTYPE  •  DEMO DATA" : "RXN BMS  •  NOT CONNECTED",
        "classification");
    classificationLabel_ = classification;
    classification->setAlignment(Qt::AlignCenter);
    side->addWidget(classification);
    bodyLayout->addWidget(sidebar);

    pages_ = new QStackedWidget; pages_->setObjectName("pageStack");
    auto addScrollablePage = [this](QWidget *page, bool verticalScroll = true) {
        page->setObjectName("contentPage");
        page->setAttribute(Qt::WA_StyledBackground, true);
        auto *scroll = new QScrollArea;
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setVerticalScrollBarPolicy(verticalScroll ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        scroll->setWidget(page);
        pages_->addWidget(scroll);
    };
    addScrollablePage(makeDashboardPage(), false);
    addScrollablePage(makeBatteryPage(), false);
    addScrollablePage(makeAlarmsPage());
    addScrollablePage(makeSystemPage(), false);
    addScrollablePage(makeTrendPage(), false);
    addScrollablePage(makeAiDemoPage());
    addScrollablePage(makeRs485DiagnosticsPage());
    bodyLayout->addWidget(pages_, 1); outer->addWidget(body, 1);

    auto *footer = new QFrame; footer->setObjectName("footerBar"); footer->setFixedHeight(32);
    auto *foot = new QHBoxLayout(footer); foot->setContentsMargins(20,4,20,4);
    foot->addWidget(label("KRAFTEK BMS  •  SOFTWARE BUILD 0.5", "footerText")); foot->addStretch();
    foot->addWidget(label("PROTOTYPE INTERFACE — NOT FOR OPERATIONAL CONTROL", "footerWarning"));
    outer->addWidget(footer);
    setCentralWidget(root);

    
    connect(controller_, &DashboardController::bankMetricsChanged, this, &MainWindow::updateMetrics);
    connect(controller_, &DashboardController::batteryHealthChanged, this,
        [this](bool available, double bankSoh, int weakestId, double weakestSoh,
               double weakestVoltage, double weakestTemperature, int validCount, int invalidCount) {
            if (bankSohBar_) { bankSohBar_->setVisible(available); bankSohBar_->setValue(qRound(bankSoh)); }
            if (weakestBatteryValue_) {
                if (!available) weakestBatteryValue_->setText(QStringLiteral("Bank SOH unavailable — no valid battery health readings."));
                else weakestBatteryValue_->setText(QStringLiteral("Weakest: B%1 · %2% SOH · %3 V · %4 °C\nValid batteries: %5 · invalid: %6")
                    .arg(weakestId).arg(weakestSoh,0,'f',1).arg(weakestVoltage,0,'f',2)
                    .arg(weakestTemperature,0,'f',1).arg(validCount).arg(invalidCount));
            }
            for (int i=0; i<4; ++i) {
                if (!batterySoh_[i] || !batterySohBars_[i]) continue;
                const auto rs = model_->readings();
                if (i >= rs.size() || !rs.at(i).sensorValid || !qIsFinite(rs.at(i).sohPct) || rs.at(i).sohPct < 0 || rs.at(i).sohPct > 100) {
                    batterySoh_[i]->setText(QStringLiteral("N/A")); batterySohBars_[i]->setValue(0); batterySohBars_[i]->setEnabled(false);
                } else {
                    batterySoh_[i]->setText(QStringLiteral("%1%").arg(rs.at(i).sohPct,0,'f',1));
                    batterySohBars_[i]->setValue(qRound(rs.at(i).sohPct)); batterySohBars_[i]->setEnabled(true);
                }
            }
        });
    connect(controller_, &DashboardController::databaseStatusChanged, this, [this](const QString &s) { if (databaseStatusValue_) databaseStatusValue_->setText(s); });
    connect(controller_, &DashboardController::diagnosticsChanged, this, [this](const QString &status, const QString &summary, double score) { if (diagnosticStatusValue_) diagnosticStatusValue_->setText(QStringLiteral("%1 · score %2/100 — %3").arg(status).arg(score,0,'f',0).arg(summary)); });
    connect(controller_, &DashboardController::aiDemoChanged, this, &MainWindow::updateAiDemo);
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() { clockValue_->setText(QDateTime::currentDateTime().toString("dd MMM yyyy  •  HH:mm:ss")); controller_->refresh(); });
    timer->start(1000); clockValue_->setText(QDateTime::currentDateTime().toString("dd MMM yyyy  •  HH:mm:ss"));
    updateResponsiveLayout();
    controller_->refresh();
    loadEventHistory();
}

QWidget *MainWindow::makeMetricCard(const QString &eyebrow, QLabel **value, const QString &unit, const QString &accent) {
    auto *card = panel("metricCard");
    card->setProperty("accent", accent);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 8, 10, 8);
    layout->setSpacing(3);

    auto *title = label(eyebrow, "metricEyebrow");
    title->setAlignment(Qt::AlignCenter);
    title->setWordWrap(true);
    layout->addWidget(title);

    auto *valueRow = new QHBoxLayout;
    valueRow->setSpacing(4);
    valueRow->addStretch();
    *value = label("--", "metricValue");
    (*value)->setAlignment(Qt::AlignCenter);
    valueRow->addWidget(*value);
    auto *unitLabel = label(unit, "metricUnit");
    unitLabel->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
    valueRow->addWidget(unitLabel);
    valueRow->addStretch();
    layout->addLayout(valueRow);
    layout->addStretch();
    return card;
}

QWidget *MainWindow::makeBatteryCard(int index, QLabel **voltage, QLabel **temperature,
                                     QLabel **stateOfCharge, QLabel **stateOfHealth,
                                     QLabel **status, QProgressBar **socBar,
                                     QProgressBar **sohBar) {
    auto *card = panel("batteryCard");
    card->setMinimumWidth(0);
    auto *l = new QVBoxLayout(card); l->setContentsMargins(9,8,9,8); l->setSpacing(4);
    auto *head = new QHBoxLayout;
    auto *name = label(QStringLiteral("BATTERY %1").arg(index + 1), "batteryName");
    name->setWordWrap(true); head->addWidget(name,1);
    *status = label("● DATA UNAVAILABLE", "statusUnavailable");
    head->addWidget(*status); l->addLayout(head);
    auto *voltageRow = new QHBoxLayout;
    *voltage = label("12.58", "batteryVoltage"); (*voltage)->setMinimumWidth(0);
    voltageRow->addWidget(*voltage); voltageRow->addWidget(label("V DC", "metricUnit"),0,Qt::AlignBottom); l->addLayout(voltageRow);
    l->addWidget(label("TERMINAL VOLTAGE", "microLabel"));
    auto *sep = new QFrame; sep->setObjectName("thinSeparator"); sep->setFixedHeight(1); l->addWidget(sep);
    auto *stats = new QGridLayout; stats->setHorizontalSpacing(8); stats->setVerticalSpacing(4);
    auto *tempTitle=label("TEMP", "microLabel"); auto *socTitle=label("SOC", "microLabel"); auto *sohTitle=label("SOH", "microLabel");
    stats->addWidget(tempTitle,0,0); stats->addWidget(socTitle,0,1); stats->addWidget(sohTitle,0,2);
    *temperature=label("27.0 °C","batteryStatValue"); *stateOfCharge=label("92%","batteryStatValue"); *stateOfHealth=label("97%","batteryStatValue");
    stats->addWidget(*temperature,1,0); stats->addWidget(*stateOfCharge,1,1); stats->addWidget(*stateOfHealth,1,2);
    l->addLayout(stats);
    *socBar = new QProgressBar; (*socBar)->setObjectName("socBar"); (*socBar)->setRange(0,100); (*socBar)->setValue(92); (*socBar)->setTextVisible(false); (*socBar)->setFixedHeight(6); l->addWidget(*socBar);
    *sohBar = new QProgressBar; (*sohBar)->setObjectName("sohBar"); (*sohBar)->setRange(0,100); (*sohBar)->setValue(97); (*sohBar)->setTextVisible(false); (*sohBar)->setFixedHeight(6); l->addWidget(*sohBar);
    return card;
}

QWidget *MainWindow::makeDashboardPage() {
    auto *page = new QWidget; auto *root = new QVBoxLayout(page); root->setContentsMargins(10,8,10,8); root->setSpacing(6); root->setSizeConstraint(QLayout::SetNoConstraint);
    auto *heading = new QHBoxLayout; auto *titles = new QVBoxLayout; titles->setSpacing(4);
    titles->addWidget(label("Operational overview", "pageHeading"));
    auto *description=label("Live bank telemetry, individual battery condition and charging state", "pageDescription"); description->setWordWrap(true); titles->addWidget(description);
    heading->addLayout(titles,1); systemStateValue_=label("NORMAL","statePill"); heading->addWidget(systemStateValue_,0,Qt::AlignVCenter); root->addLayout(heading);
    metricsGrid_ = new QGridLayout; metricsGrid_->setSpacing(10);
    metricCards_.clear();
    metricCards_ << makeMetricCard("BANK VOLTAGE",&bankVoltageValue_,"V DC","teal")
                 << makeMetricCard("BANK CURRENT",&currentValue_,"A","blue");
    auto *socCard = makeMetricCard("STATE OF CHARGE", &socValue_, "%", "gold");
    auto *socLayout = qobject_cast<QVBoxLayout *>(socCard->layout());
    bankSocGraph_ = new SocSparkline(socCard);
    if (socLayout) socLayout->addWidget(bankSocGraph_);
    metricCards_ << socCard;

    // Use the same base metric-card factory as voltage/current/SOC for consistent alignment.
    auto *sohCard = makeMetricCard("STATE OF HEALTH", &sohValue_, "%", "purple");
    auto *sohLayout = qobject_cast<QVBoxLayout *>(sohCard->layout());
    bankSohBar_ = new QProgressBar;
    bankSohBar_->setObjectName("sohBar");
    bankSohBar_->setRange(0, 100);
    bankSohBar_->setValue(0);
    bankSohBar_->setTextVisible(false);
    bankSohBar_->setFixedHeight(7);
    if (sohLayout) sohLayout->addWidget(bankSohBar_);
    metricCards_ << sohCard;
    for (int i=0;i<metricCards_.size();++i) metricsGrid_->addWidget(metricCards_.at(i),0,i);
    root->addLayout(metricsGrid_);
    chargeStatusValue_=label("CHARGE STATUS: --", "statePill"); root->addWidget(chargeStatusValue_);
    auto *healthPanel=panel(); auto *healthLayout=new QVBoxLayout(healthPanel); healthLayout->setContentsMargins(12,10,12,10);
    weakestBatteryValue_=label("Waiting for valid battery health data","pageDescription"); weakestBatteryValue_->setWordWrap(true); healthLayout->addWidget(weakestBatteryValue_); root->addWidget(healthPanel);
    auto *batteryHeading = new QHBoxLayout; batteryHeading->addWidget(label("BATTERY STRING", "sectionHeading")); batteryHeading->addStretch(); auto *meta=label("4 × 12 V / 180 Ah","sectionMeta"); meta->setWordWrap(true); batteryHeading->addWidget(meta); root->addLayout(batteryHeading);
    batteryGrid_=new QGridLayout; batteryGrid_->setSpacing(10); batteryCards_.clear();
    for (int i=0;i<4;++i) {
        QWidget *card=makeBatteryCard(i,&batteryVoltage_[i],&batteryTemperature_[i],&batterySoc_[i],&batterySoh_[i],&batteryStatus_[i],&batterySocBars_[i],&batterySohBars_[i]);
        batteryCards_.append(card);
    }
    root->addLayout(batteryGrid_);
    // Keep the overview focused on essential live values so it fits one screen.
    // Full historical plots and explanatory panels are available on dedicated pages.
    return page;
}

QWidget *MainWindow::makeBatteryPage() {
    auto *page = new QWidget; auto *l = new QVBoxLayout(page); l->setContentsMargins(24,22,24,22); l->setSpacing(16);
    l->addWidget(label("Battery details", "pageHeading")); l->addWidget(label("B1–B4 block voltage, terminal-lug temperature, SoC/SoH estimates and sensor validity", "pageDescription"));
    auto *table = new QTableView; table->setObjectName("dataTable"); table->setModel(model_); table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); table->verticalHeader()->hide(); table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setAlternatingRowColors(true); table->setMinimumHeight(210); l->addWidget(table);
    auto *note = panel();
    auto *nl = new QVBoxLayout(note);
    nl->setContentsMargins(18,16,18,16);
    nl->addWidget(label("MEASUREMENT NOTE", "panelHeading"));
    const bool isSimulator = controller_ && controller_->sourceId() == QStringLiteral("SIMULATOR");
    auto *txt = label(
        isSimulator
            ? "Values on this screen are generated by the simulator. Accuracy, calibration status, isolation ratings and sensor-fault behavior must be verified against the selected hardware before operational use."
            : "RXN BMS telemetry is not currently connected. Measurements remain unavailable until a validated device connection and protocol are configured. Hardware accuracy, calibration, isolation ratings and sensor-fault behavior must be verified before operational use.",
        "pageDescription");
    txt->setWordWrap(true);
    nl->addWidget(txt);
    l->addWidget(note);
    l->addStretch();
    return page;
}

QWidget *MainWindow::makeAlarmsPage()
{
    auto *page = new QWidget;
    auto *l = new QVBoxLayout(page);
    l->setContentsMargins(24, 22, 24, 22);
    l->setSpacing(16);

    l->addWidget(label("Alarms & events", "pageHeading"));
    l->addWidget(label(
        "Recorded software events retrieved from the local SQLite database.",
        "pageDescription"));

    // These records are not live hardware alarms and must not be
    // interpreted as proof of physical battery-bank safety.
    auto *notice = panel();
    auto *n = new QVBoxLayout(notice);
    n->setContentsMargins(18, 16, 18, 16);
    n->addWidget(label("NO LIVE ALARM INPUT", "alarmHeadline"));

    auto *txt = label(
        "Protection thresholds in this screen are specification references only; "
        "this prototype does not receive hardware alarms. The items below are "
        "software status notices, not evidence that the physical battery bank is safe.",
        "pageDescription");
    txt->setWordWrap(true);
    n->addWidget(txt);
    l->addWidget(notice);

    auto *filters = panel();
    auto *filterLayout = new QGridLayout(filters);
    filterLayout->setContentsMargins(16, 12, 16, 12);
    filterLayout->setHorizontalSpacing(10);
    filterLayout->setVerticalSpacing(8);

    eventStart_ = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    eventStart_->setCalendarPopup(true);
    eventStart_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    eventEnd_ = new QDateTimeEdit(QDateTime::currentDateTime());
    eventEnd_->setCalendarPopup(true);
    eventEnd_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");

    eventSeverity_ = new QComboBox;
    eventSeverity_->addItem("All severities", QString());
    eventSeverity_->addItem("INFO", QStringLiteral("INFO"));
    eventSeverity_->addItem("WARNING", QStringLiteral("WARNING"));
    eventSeverity_->addItem("CRITICAL", QStringLiteral("CRITICAL"));

    auto *refreshButton = new QPushButton("Refresh events");

    filterLayout->addWidget(label("FROM", "metricEyebrow"), 0, 0);
    filterLayout->addWidget(label("TO", "metricEyebrow"), 0, 1);
    filterLayout->addWidget(label("SEVERITY", "metricEyebrow"), 0, 2);
    filterLayout->addWidget(eventStart_, 1, 0);
    filterLayout->addWidget(eventEnd_, 1, 1);
    filterLayout->addWidget(eventSeverity_, 1, 2);
    filterLayout->addWidget(refreshButton, 1, 3);

    l->addWidget(filters);

    eventSummary_ = label("No event history loaded.", "pageDescription");
    l->addWidget(eventSummary_);

    eventTable_ = new QTableWidget(0, 7);
    eventTable_->setObjectName("eventTable");
    eventTable_->setHorizontalHeaderLabels({
        "TIME (LOCAL)", "SEVERITY", "EVENT CODE", "MESSAGE",
        "SOURCE", "ACKNOWLEDGED", "ID"
    });
    eventTable_->verticalHeader()->hide();
    eventTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    eventTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    eventTable_->setWordWrap(true);
    eventTable_->setAlternatingRowColors(true);
    eventTable_->setSortingEnabled(false);
    eventTable_->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    eventTable_->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::Stretch);
    eventTable_->setColumnHidden(6, true);

    l->addWidget(eventTable_, 1);

    connect(refreshButton, &QPushButton::clicked,
            this, &MainWindow::loadEventHistory);
    connect(eventSeverity_, &QComboBox::currentTextChanged,
            this, [this](const QString &) { loadEventHistory(); });

    loadEventHistory();

    return page;
}

void MainWindow::loadEventHistory()
{
    if (!controller_ || !eventStart_ || !eventEnd_ ||
        !eventSeverity_ || !eventTable_ || !eventSummary_) {
        return;
    }

    const QDateTime startLocal = eventStart_->dateTime();
    const QDateTime endLocal = eventEnd_->dateTime();

    if (!startLocal.isValid() || !endLocal.isValid() ||
        startLocal > endLocal) {
        eventTable_->setRowCount(0);
        eventSummary_->setText(
            "Invalid time range: the start must not be later than the end.");
        return;
    }

    const QString severity = eventSeverity_->currentData().toString();

    const QList<HistoricalEvent> events = controller_->queryEvents(
        startLocal.toUTC(),
        endLocal.toUTC(),
        severity,
        500);

    eventTable_->setRowCount(0);

    for (const HistoricalEvent &event : events) {
        const int row = eventTable_->rowCount();
        eventTable_->insertRow(row);

        const QString localTime = event.timestampUtc.isValid()
            ? event.timestampUtc.toLocalTime().toString("yyyy-MM-dd HH:mm:ss")
            : QStringLiteral("Invalid timestamp");

        const QStringList values = {
            localTime,
            event.severity,
            event.eventCode,
            event.message,
            event.source,
            event.acknowledged ? QStringLiteral("YES")
                               : QStringLiteral("NO"),
            QString::number(event.id)
        };

        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            eventTable_->setItem(row, column, item);
        }
    }

    eventTable_->resizeRowsToContents();

    eventSummary_->setText(
        QStringLiteral("%1 event(s) found · maximum 500 displayed")
            .arg(events.size()));
}

QWidget *MainWindow::makeAiDemoPage() {
    auto *page = new QWidget; auto *l = new QVBoxLayout(page); l->setContentsMargins(18,12,18,12); l->setSpacing(8); l->setSizeConstraint(QLayout::SetMinimumSize);
    l->addWidget(label("AI battery health Advisory", "pageHeading"));
    const bool isSimulator = controller_ && controller_->sourceId() == QStringLiteral("SIMULATOR");
    auto *description = label(
        isSimulator
            ? "Synthetic telemetry · statistical rules + lightweight synthetic-trained centroid classifier"
            : "RXN BMS diagnostic status. No validated RXN field-data diagnostic model is currently configured.",
        "pageDescription");
    description->setWordWrap(true);
    l->addWidget(description);
    auto *warning = panel(); auto *wl = new QVBoxLayout(warning); wl->setContentsMargins(12,8,12,8);
    auto *warningText = label(
        isSimulator
            ? "SIMULATION ONLY — AI outputs are advisory and are not connected to BMS protection or physical controls."
            : "DIAGNOSTICS NOT VALIDATED — No RXN field-data model is configured. Outputs are not protection signals and do not control physical hardware.",
        "footerWarning");
    warningText->setWordWrap(true);
    wl->addWidget(warningText);
    l->addWidget(warning);
    auto *controls = panel(); auto *cl = new QGridLayout(controls); cl->setContentsMargins(12,8,12,8); cl->setHorizontalSpacing(12); cl->setVerticalSpacing(5);
    cl->addWidget(label("SELECT SYNTHETIC SCENARIO", "metricEyebrow"),0,0);
    scenarioCombo_ = new QComboBox; scenarioCombo_->addItems({"Normal", "Voltage sag", "Block imbalance", "Temperature rise", "Sensor spike"}); cl->addWidget(scenarioCombo_,1,0);
    auto *apply = new QPushButton("Apply scenario now"); cl->addWidget(apply,1,1);
    autoCycleCheck_ = new QCheckBox("Automatically cycle scenarios every 12 samples"); cl->addWidget(autoCycleCheck_,2,0,1,2);
    connect(apply, &QPushButton::clicked, this, [this]() { controller_->setDemoScenario(scenarioCombo_->currentText()); });
    connect(scenarioCombo_, &QComboBox::currentTextChanged, this, [this](const QString &scenario) { if (!autoCycleCheck_->isChecked()) controller_->setDemoScenario(scenario); });
    connect(autoCycleCheck_, &QCheckBox::toggled, this, [this](bool enabled) { controller_->setAutomaticCycle(enabled); if (!enabled) controller_->setDemoScenario(scenarioCombo_->currentText()); });
    controls->setVisible(isSimulator);
    l->addWidget(controls);
    auto *grid = new QGridLayout; grid->setSpacing(8);
    auto addValue = [grid](int row, int col, const QString &title, QLabel **target) {
        auto *card = panel("metricCard"); auto *v = new QVBoxLayout(card); v->setContentsMargins(12,8,12,8); v->addWidget(label(title,"metricEyebrow")); *target = label("--","metricValue"); (*target)->setWordWrap(true); v->addWidget(*target); grid->addWidget(card,row,col);
    };
    addValue(0,0,"ACTIVE SCENARIO",&aiScenarioValue_);
    addValue(0,1,"ML CLASSIFICATION",&aiClassValue_);
    addValue(1,0,"ML DISTANCE SCORE",&aiScoreValue_);
    addValue(1,1,"STATISTICAL ANOMALY SCORE",&aiAnomalyScoreValue_);
    l->addLayout(grid);
    auto *result = panel(); auto *rl = new QVBoxLayout(result); rl->setContentsMargins(12,8,12,8); rl->addWidget(label("ADVISORY EXPLANATION", "panelHeading"));
    aiSummaryValue_ = label(
        isSimulator ? "Waiting for synthetic telemetry…" : "Waiting for RXN BMS diagnostic status…",
        "pageDescription");
    aiSummaryValue_->setWordWrap(true);
    rl->addWidget(aiSummaryValue_);

    aiSensorValidityValue_ = label("Sensor flags: N/A", "pageDescription");
    aiSensorValidityValue_->setWordWrap(true);
    rl->addWidget(aiSensorValidityValue_);

    rl->addWidget(label("INDIVIDUAL FINDINGS", "panelHeading"));
    aiFindingsValue_ = label("No diagnostic findings available.", "pageDescription");
    aiFindingsValue_->setWordWrap(true);
    rl->addWidget(aiFindingsValue_);

    l->addWidget(result);
    return page;
}

void MainWindow::updateAiDemo(
    const QString &scenario, const QString &mlClass,
    double mlScore, double anomalyScore,
    int validSensorCount, int invalidSensorCount,
    const QStringList &findings, const QString &summary) {

    const bool isSimulator =
        controller_ && controller_->sourceId() == QStringLiteral("SIMULATOR");
    const bool analysisAvailable =
        isSimulator && mlClass != QStringLiteral("NOT RUN") &&
        scenario != QStringLiteral("N/A");

    if (aiScenarioValue_)
        aiScenarioValue_->setText(isSimulator ? scenario : QStringLiteral("N/A"));

    if (aiClassValue_)
        aiClassValue_->setText(mlClass);

    if (aiScoreValue_) {
        aiScoreValue_->setText(
            analysisAvailable
                ? QStringLiteral("%1 / 100").arg(mlScore, 0, 'f', 1)
                : QStringLiteral("N/A"));
    }

    if (aiAnomalyScoreValue_) {
        aiAnomalyScoreValue_->setText(
            analysisAvailable
                ? QStringLiteral("%1 / 100").arg(anomalyScore, 0, 'f', 1)
                : QStringLiteral("N/A"));
    }

    if (aiSensorValidityValue_) {
        if (analysisAvailable && validSensorCount >= 0 &&
            invalidSensorCount >= 0) {
            aiSensorValidityValue_->setText(
                QStringLiteral("Sensor flags: %1 marked valid · %2 marked invalid")
                    .arg(validSensorCount)
                    .arg(invalidSensorCount));
        } else {
            aiSensorValidityValue_->setText(QStringLiteral("Sensor flags: N/A"));
        }
    }

    if (aiFindingsValue_) {
        if (!analysisAvailable) {
            aiFindingsValue_->setText(
                QStringLiteral("N/A"));
        } else if (findings.isEmpty()) {
            aiFindingsValue_->setText(
                QStringLiteral("No individual findings reported."));
        } else {
            aiFindingsValue_->setText(
                QStringLiteral("• ") + findings.join(QStringLiteral("\n• ")));
        }
    }

    if (aiSummaryValue_) {
        const QString displaySummary = analysisAvailable
            ? summary + QStringLiteral(
                "\n\nML distance score is not a probability of failure. "
                "All diagnostic outputs are advisory and are not protection signals.")
            : summary;
        aiSummaryValue_->setText(displaySummary);
    }

    if (isSimulator && scenarioCombo_ &&
        scenarioCombo_->currentText() != scenario) {
        const QSignalBlocker blocker(scenarioCombo_);
        scenarioCombo_->setCurrentText(scenario);
    }
}

QWidget *MainWindow::makeTrendPage() {
    auto *page=new QWidget; auto *root=new QVBoxLayout(page); root->setContentsMargins(16,16,16,16); root->setSpacing(12);
    root->addWidget(label("Historical trends", "pageHeading"));
    auto *desc=label("Select a time band and plot bank or individual-battery history from SQLite. Historical records retain their originating data source.","pageDescription"); desc->setWordWrap(true); root->addWidget(desc);
    auto *controls=panel(); auto *grid=new QGridLayout(controls); grid->setContentsMargins(12,12,12,12); grid->setHorizontalSpacing(8); grid->setVerticalSpacing(8);
    grid->addWidget(label("START TIME","metricEyebrow"),0,0); grid->addWidget(label("END TIME","metricEyebrow"),0,1);
    historyStart_=new QDateTimeEdit(QDateTime::currentDateTime().addSecs(-3600)); historyStart_->setCalendarPopup(true); historyStart_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    historyEnd_=new QDateTimeEdit(QDateTime::currentDateTime()); historyEnd_->setCalendarPopup(true); historyEnd_->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    grid->addWidget(historyStart_,1,0); grid->addWidget(historyEnd_,1,1);
    grid->addWidget(label("SERIES","metricEyebrow"),2,0); grid->addWidget(label("MEASURE","metricEyebrow"),2,1);
    historyScope_=new QComboBox; historyScope_->addItems({"Bank", "Battery 1", "Battery 2", "Battery 3", "Battery 4"}); grid->addWidget(historyScope_,3,0);
    historyMetric_=new QComboBox; historyMetric_->addItems({"Voltage", "State of Charge", "State of Health", "Current", "Temperature", "Drain Rate", "Internal Resistance"}); grid->addWidget(historyMetric_,3,1);
    auto *load=new QPushButton("Load selected history"); grid->addWidget(load,4,0,1,2); connect(load,&QPushButton::clicked,this,&MainWindow::loadHistory);
    root->addWidget(controls);
    historyChart_=new TrendWidget; historyChart_->setMinimumHeight(300); root->addWidget(historyChart_,1);
    historySummary_=label("Choose a start and end time, series and measure, then load history.","pageDescription"); historySummary_->setWordWrap(true); root->addWidget(historySummary_);
    auto *note=label("Drain rate is derived from the change in SoC over elapsed time. Internal resistance is only an indicative ΔV/ΔI estimate from historical samples; it is N/A when current change is too small. No dedicated per-battery current sensor or validated resistance test is represented by this estimate.","pageDescription"); note->setWordWrap(true); root->addWidget(note);
    connect(historyScope_, &QComboBox::currentIndexChanged, this, [this]() { if (historyEnd_ && historyStart_ && historyEnd_->dateTime() < historyStart_->dateTime()) historyEnd_->setDateTime(historyStart_->dateTime()); });
    return page;
}

void MainWindow::loadHistory() {
    if (!historyStart_ || !historyEnd_ || !historyChart_ || !controller_) return;
    const QDateTime start=historyStart_->dateTime().toUTC();
    const QDateTime end=historyEnd_->dateTime().toUTC();
    if (end < start) { historySummary_->setText("End time must be after start time."); return; }
    const int batteryId=historyScope_->currentIndex();
    const auto records=controller_->queryHistory(start,end,batteryId);
    QVector<QPointF> points;
    QString metric=historyMetric_->currentText(); QString unit; double lastDrain=0.0; double lastResistance=0.0; bool drainValid=false; bool resistanceValid=false;
    for (int i=0;i<records.size();++i) {
        const auto &r=records.at(i); double y=0.0; bool valid=true;
        if (metric=="Voltage") { y=batteryId==0?r.bankVoltageV:r.batteryVoltageV; unit="V"; }
        else if (metric=="State of Charge") { y=batteryId==0?r.bankSocPct:r.batterySocPct; unit="%"; }
        else if (metric=="State of Health") { y=batteryId==0?r.bankSohPct:r.batterySohPct; unit="%"; }
        else if (metric=="Current") { y=r.bankCurrentA; unit="A"; }
        else if (metric=="Temperature") { if (batteryId==0) { valid=false; } else { y=r.batteryTemperatureC; } unit="°C"; }
        else if (metric=="Drain Rate") {
            unit="% SoC/h"; valid=false;
            if (i>0) { const auto &prev=records.at(i-1); const qint64 ms=prev.timestampUtc.msecsTo(r.timestampUtc); const double prevSoc=batteryId==0?prev.bankSocPct:prev.batterySocPct; const double soc=batteryId==0?r.bankSocPct:r.batterySocPct;
                if (ms>0) { y=(prevSoc-soc)*3600000.0/ms; valid=true; lastDrain=y; drainValid=true; } }
        } else if (metric=="Internal Resistance") {
            unit="mΩ (estimate)"; valid=false;
            if (i>0) { const auto &prev=records.at(i-1); const double di=qAbs(r.bankCurrentA-prev.bankCurrentA); const double dv=qAbs((batteryId==0?r.bankVoltageV:r.batteryVoltageV)-(batteryId==0?prev.bankVoltageV:prev.batteryVoltageV));
                if (di>=2.0) { y=(dv/di)*1000.0; valid=true; lastResistance=y; resistanceValid=true; } }
        }
        if (batteryId > 0 && !r.sensorValid && metric != "Current") valid=false;
        if (valid && qIsFinite(y)) points.append(QPointF(r.timestampUtc.toMSecsSinceEpoch(),y));
    }
    const QString scope=batteryId==0?QStringLiteral("Bank"):QStringLiteral("Battery %1").arg(batteryId);
    historyChart_->setData(points,scope+QStringLiteral(" · ")+metric,unit,start.toLocalTime().toString("MM-dd HH:mm"),end.toLocalTime().toString("MM-dd HH:mm"));
    QString summary=QStringLiteral("%1 sample(s) in selected range · %2 · %3").arg(points.size()).arg(scope,metric);
    if (metric=="Drain Rate") summary+=drainValid?QStringLiteral(" · latest derived drain rate %1 %2").arg(lastDrain,0,'f',3).arg(unit):QStringLiteral(" · insufficient SoC change/history to estimate drain rate");
    if (metric=="Internal Resistance") summary+=resistanceValid?QStringLiteral(" · latest ΔV/ΔI estimate %1 %2").arg(lastResistance,0,'f',2).arg(unit):QStringLiteral(" · insufficient current variation for ΔV/ΔI estimate");
    if (metric=="Temperature" && batteryId==0) summary+=QStringLiteral(" · select an individual battery for temperature");
    historySummary_->setText(summary);
}

QWidget *MainWindow::makeSystemPage() {
    const bool isSimulator = controller_ && controller_->sourceId() == QStringLiteral("SIMULATOR");
    auto *page = new QWidget;
    auto *l = new QVBoxLayout(page);
    l->setContentsMargins(24,22,24,22);
    l->setSpacing(16);
    l->addWidget(label("System status", "pageHeading"));
    l->addWidget(label("Application build and integration readiness", "pageDescription"));

    auto *grid = new QGridLayout;
    grid->setSpacing(12);
    const QStringList names = {
        "DATA SOURCE", "SQLITE3 DATABASE", "CAN / RS-485 INTERFACE",
        "PROTECTION CONTROLLER", "AI / ML DIAGNOSTICS", "PHYSICAL CONTACTORS"
    };
    const QStringList states = {
        isSimulator ? "SIMULATOR ACTIVE" : "RXN BMS NOT CONNECTED",
        "INITIALIZING DATABASE…", "NOT INTEGRATED", "NOT VALIDATED",
        "INITIALIZING DIAGNOSTICS…", "NOT CONNECTED"
    };

    for (int i = 0; i < names.size(); ++i) {
        auto *c = panel();
        auto *cl = new QVBoxLayout(c);
        cl->setContentsMargins(18,16,18,16);
        cl->addWidget(label(names[i], "metricEyebrow"));
        auto *v = label(states[i], "systemStateValue");
        v->setWordWrap(true);
        if (i == 1) databaseStatusValue_ = v;
        if (i == 4) diagnosticStatusValue_ = v;
        cl->addWidget(v);
        grid->addWidget(c, i/2, i%2);
    }
    l->addLayout(grid);

    auto *warning = panel();
    auto *wl = new QVBoxLayout(warning);
    wl->setContentsMargins(18,16,18,16);
    wl->addWidget(label("SAFETY / RELEASE GATE", "panelHeading"));

    const QString warningText = isSimulator
        ? "SQLite records HMI telemetry and diagnostic events. AI/ML diagnostics combine statistical heuristics with a lightweight classifier fitted only to synthetic demo samples; this is not validated against field data or safety-certified. Outputs are advisory only. Protection remains deterministic and local to the BMS hardware. This interface is a prototype visualization only; do not use it for operational battery control until sensing, protection, communications, fail-safe behavior and acceptance tests are independently verified."
        : "SQLite records HMI telemetry and diagnostic events. RXN BMS telemetry is not connected, and no validated RXN field-data AI/ML diagnostic model is configured. Outputs are advisory only and are not protection signals. Protection must remain deterministic and local to the BMS hardware. This interface is a prototype visualization only; do not use it for operational battery control until sensing, protection, communications, fail-safe behavior and acceptance tests are independently verified.";

    auto *text = label(warningText, "pageDescription");
    text->setWordWrap(true);
    wl->addWidget(text);
    l->addWidget(warning);
    l->addStretch();
    return page;
}


MainWindow::~MainWindow()
{
    shuttingDown_ = true;

    // Stop and join the receive-only listener.
    stopRs485Listening(false);

    // Wait for any register-test worker.
    if (rs485Worker_ && rs485Worker_->isRunning()) {
        rs485Worker_->wait();
    }
}

void MainWindow::appendRs485Log(const QString &message)
{
    if (!rs485Output_)
        return;

    rs485Output_->appendPlainText(
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")
        + "  " + message);
    rs485Output_->verticalScrollBar()->setValue(
        rs485Output_->verticalScrollBar()->maximum());
}

void MainWindow::reportUnverifiedRs485Parameter(const QString &parameter)
{
    appendRs485Log(
        QStringLiteral("%1: NOT SENT — RXN register address, data format "
                       "and scaling are not verified.")
            .arg(parameter));
}


void MainWindow::updateRs485ListenerControls()
{
    const bool testWorkerRunning =
        rs485Worker_ && rs485Worker_->isRunning();

    if (rs485StartButton_) {
        rs485StartButton_->setEnabled(
            !shuttingDown_ &&
            !rs485Listening_ &&
            !rs485Busy_ &&
            !testWorkerRunning);
    }

    if (rs485StopButton_) {
        rs485StopButton_->setEnabled(
            !shuttingDown_ && rs485Listening_);
    }

    if (rs485PortCombo_) {
        rs485PortCombo_->setEnabled(
            !shuttingDown_ &&
            !rs485Listening_ &&
            !rs485Busy_ &&
            !testWorkerRunning);
    }
}


void MainWindow::startRs485Listening()
{
    if (shuttingDown_ || rs485Busy_ || rs485Listening_)
        return;

    if (rs485Worker_ && rs485Worker_->isRunning()) {
        appendRs485Log(
            "ERROR: A register test is still using the serial port.");
        return;
    }

    if (!rs485PortCombo_ ||
        rs485PortCombo_->currentData().toString().isEmpty()) {
        appendRs485Log(
            "ERROR: Select an available serial port first.");
        return;
    }

    const QString portName =
        rs485PortCombo_->currentData().toString();

    const quint64 generation = ++rs485ListenerGeneration_;

    rs485ListenerStopRequested_.store(false);
    rs485Listening_ = true;

    appendRs485Log(
        QStringLiteral(
            "Starting receive-only listener: %1, 115200 8N1.")
            .arg(portName));

    updateRs485ListenerControls();

    QThread *worker = QThread::create(
        [this, portName, generation]() {

            QSerialPort serial;

            serial.setPortName(portName);
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setDataBits(QSerialPort::Data8);
            serial.setParity(QSerialPort::NoParity);
            serial.setStopBits(QSerialPort::OneStop);
            serial.setFlowControl(QSerialPort::NoFlowControl);

            QString openError;

            if (!serial.open(QIODevice::ReadOnly)) {

                openError =
                    QStringLiteral("Cannot open %1: %2")
                        .arg(portName, serial.errorString());

            } else {

                QMetaObject::invokeMethod(
                    this,
                    [this, portName, generation]() {

                        if (shuttingDown_ ||
                            generation != rs485ListenerGeneration_) {
                            return;
                        }

                        if (rs485StatusValue_) {
                            rs485StatusValue_->setText(
                                "LISTENING — 115200 8N1");
                        }

                        appendRs485Log(
                            QStringLiteral(
                                "Listening on %1. Receive-only mode; "
                                "no commands transmitted.")
                                .arg(portName));
                    },
                    Qt::QueuedConnection);
            }

            while (serial.isOpen() &&
                   !rs485ListenerStopRequested_.load()) {

                if (serial.waitForReadyRead(100)) {

                    QByteArray received = serial.readAll();

                    if (received.isEmpty())
                        continue;

                    received += serial.readAll();

                    const int byteCount = received.size();

                    const QString hex =
                        QString::fromLatin1(
                            received.toHex(' ').toUpper());

                    QString ascii;
                    ascii.reserve(byteCount);

                    for (char ch : received) {

                        const quint8 byte =
                            static_cast<quint8>(
                                static_cast<unsigned char>(ch));

                        if (byte >= 32 && byte <= 126) {
                            ascii.append(
                                QChar(static_cast<ushort>(byte)));
                        } else {
                            ascii.append(QChar('.'));
                        }
                    }

                    QMetaObject::invokeMethod(
                        this,
                        [this, hex, ascii, byteCount, generation]() {

                            if (shuttingDown_ ||
                                generation != rs485ListenerGeneration_) {
                                return;
                            }

                            appendRs485Log(
                                QStringLiteral(
                                    "RX %1 byte(s) | HEX: %2 | ASCII: %3")
                                    .arg(byteCount)
                                    .arg(hex, ascii));
                        },
                        Qt::QueuedConnection);

                } else {

                    const QSerialPort::SerialPortError error =
                        serial.error();

                    if (error != QSerialPort::NoError &&
                        error != QSerialPort::TimeoutError) {

                        const QString errorMessage =
                            serial.errorString();

                        QMetaObject::invokeMethod(
                            this,
                            [this, errorMessage, generation]() {

                                if (shuttingDown_ ||
                                    generation != rs485ListenerGeneration_) {
                                    return;
                                }

                                appendRs485Log(
                                    "Listener serial error: " +
                                    errorMessage);
                            },
                            Qt::QueuedConnection);

                        break;
                    }
                }
            }

            if (serial.isOpen())
                serial.close();

            QMetaObject::invokeMethod(
                this,
                [this, openError, generation]() {

                    if (shuttingDown_ ||
                        generation != rs485ListenerGeneration_) {
                        return;
                    }

                    rs485Listening_ = false;

                    if (!openError.isEmpty()) {

                        appendRs485Log(
                            "ERROR: " + openError);

                        if (rs485StatusValue_) {
                            rs485StatusValue_->setText(
                                "LISTENER ERROR");
                        }

                    } else {

                        if (rs485StatusValue_) {
                            rs485StatusValue_->setText(
                                "Listener stopped.");
                        }
                    }

                    updateRs485ListenerControls();
                },
                Qt::QueuedConnection);
        });

    rs485ListenerWorker_ = worker;

    connect(
        worker,
        &QThread::finished,
        worker,
        &QObject::deleteLater);

    worker->start();
}


void MainWindow::stopRs485Listening(bool logMessage)
{
    const bool wasListening = rs485Listening_;

    // Invalidate callbacks queued by the previous listener.
    ++rs485ListenerGeneration_;

    rs485ListenerStopRequested_.store(true);

    if (rs485ListenerWorker_) {

        QThread *worker = rs485ListenerWorker_.data();

        if (worker->isRunning())
            worker->wait();

        rs485ListenerWorker_ = nullptr;
    }

    rs485Listening_ = false;

    if (logMessage && wasListening) {
        appendRs485Log("RS-485 listener stopped.");
    }

    if (!shuttingDown_) {

        if (rs485StatusValue_)
            rs485StatusValue_->setText("Listener stopped.");

        updateRs485ListenerControls();
    }
}


QWidget *MainWindow::makeRs485DiagnosticsPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(9);
    layout->setSizeConstraint(QLayout::SetMinimumSize);

    auto *heading = label("RS-485 DIAGNOSTICS", "pageHeading");
    heading->setStyleSheet("font-size: 20px;");
    layout->addWidget(heading);

    auto *description = label(
        "RXN X3 communication and register-read test bench. Diagnostic "
        "results are independent of production dashboard telemetry.",
        "pageDescription");
    description->setWordWrap(true);
    layout->addWidget(description);

    // Serial interface selection.
    auto *portCard = panel();
    auto *portLayout = new QGridLayout(portCard);
    portLayout->addWidget(label("SERIAL PORT"), 0, 0);

    rs485PortCombo_ = new QComboBox;
    rs485PortCombo_->setMinimumWidth(150);
    portLayout->addWidget(rs485PortCombo_, 0, 1);

    auto *refreshButton = new QPushButton("REFRESH PORTS");
    portLayout->addWidget(refreshButton, 0, 2);

    rs485StartButton_ = new QPushButton("START LISTENING");
    rs485StartButton_->setMinimumHeight(34);

    rs485StopButton_ = new QPushButton("STOP LISTENING");
    rs485StopButton_->setMinimumHeight(34);
    rs485StopButton_->setEnabled(false);

    portLayout->addWidget(rs485StartButton_, 1, 0);
    portLayout->addWidget(rs485StopButton_, 1, 1, 1, 2);

    connect(
        rs485StartButton_,
        &QPushButton::clicked,
        this,
        &MainWindow::startRs485Listening);

    connect(
        rs485StopButton_,
        &QPushButton::clicked,
        this,
        [this]() {
            stopRs485Listening();
        });

    rs485StatusValue_ = label("Not scanned", "metricValue");
    rs485SettingsValue_ = label(
        "Observed X3 cell-voltage test: 115200, 8N1. "
        "Manual protocol implementation remains separate at 9600, 8N1.",
        "pageDescription");
    rs485SettingsValue_->setWordWrap(true);

    portLayout->addWidget(label("INTERFACE STATUS"), 2, 0);
    portLayout->addWidget(rs485StatusValue_, 2, 1, 1, 2);
    portLayout->addWidget(rs485SettingsValue_, 3, 0, 1, 3);

    layout->addWidget(portCard);

    connect(refreshButton, &QPushButton::clicked,
            this, &MainWindow::refreshRs485Diagnostics);

    // Parameter test panel.
    auto *testCard = panel();
    auto *testLayout = new QGridLayout(testCard);
    testLayout->setContentsMargins(12, 10, 12, 10);
    testLayout->setHorizontalSpacing(8);
    testLayout->setVerticalSpacing(7);

    auto *testHeading = label("RXN X3 REGISTER READ TESTS", "panelHeading");
    testLayout->addWidget(testHeading, 0, 0, 1, 5);

    auto addParameterRow = [this, testLayout](
        int row, const QString &parameter, bool cellParameter,
        bool verified) {

        auto *name = label(parameter, "pageDescription");
        testLayout->addWidget(name, row, 0);

        const int count = cellParameter ? 4 : 1;

        for (int i = 0; i < count; ++i) {
            const QString buttonText = cellParameter
                ? QStringLiteral("B%1").arg(i + 1)
                : QStringLiteral("READ");

            auto *button = new QPushButton(buttonText);
            button->setMinimumHeight(30);
            button->setObjectName("rs485TestButton");
            button->setProperty("diagnosticTest", true);

            testLayout->addWidget(button, row, i + 1);

            if (verified && parameter == QStringLiteral("CELL VOLTAGE")) {
                connect(button, &QPushButton::clicked, this,
                        [this, i]() { runX3CellVoltageTest(i); });
            } else {
                const QString testName = cellParameter
                    ? QStringLiteral("%1 B%2").arg(parameter).arg(i + 1)
                    : parameter;

                connect(button, &QPushButton::clicked, this,
                        [this, testName]() {
                            reportUnverifiedRs485Parameter(testName);
                        });
            }
        }
    };

    addParameterRow(1, "CELL VOLTAGE", true, true);
    // Battery-temperature register tests. Register-to-physical-sensor
    // mapping is not yet confirmed, so buttons display register numbers.
    auto *temperatureName = label("BATTERY TEMPERATURE (REGISTERS 72–75)", "pageDescription");
    testLayout->addWidget(temperatureName, 2, 0);

    for (int i = 0; i < 4; ++i) {
        const int registerAddress = 72 + i;
        auto *button = new QPushButton(QStringLiteral("R%1").arg(registerAddress));
        button->setMinimumHeight(30);
        button->setObjectName("rs485TestButton");
        button->setProperty("diagnosticTest", true);
        testLayout->addWidget(button, 2, i + 1);

        connect(button, &QPushButton::clicked, this,
                [this, registerAddress]() {
                    runX3TemperatureTest(registerAddress);
                });
    }
    addParameterRow(3, "CELL SOC", true, false);
    addParameterRow(4, "CELL SOH", true, false);

    auto *readAll = new QPushButton("READ ALL CELL VOLTAGES");
    readAll->setMinimumHeight(32);
    readAll->setObjectName("rs485TestButton");
    readAll->setProperty("diagnosticTest", true);
    testLayout->addWidget(readAll, 5, 0, 1, 5);

    connect(readAll, &QPushButton::clicked,
            this, [this]() { runX3CellVoltageTest(-1); });

    auto *readAllTemperatures = new QPushButton("READ ALL BATTERY TEMPERATURE REGISTERS (72–100)");
    readAllTemperatures->setMinimumHeight(32);
    readAllTemperatures->setObjectName("rs485TestButton");
    readAllTemperatures->setProperty("diagnosticTest", true);
    testLayout->addWidget(readAllTemperatures, 6, 0, 1, 5);

    connect(readAllTemperatures, &QPushButton::clicked,
            this, [this]() { runX3TemperatureTest(-1); });

    layout->addWidget(testCard);

    // Bank parameters.
    auto *bankCard = panel();
    auto *bankLayout = new QGridLayout(bankCard);
    bankLayout->addWidget(label("BANK PARAMETER TESTS", "panelHeading"), 0, 0, 1, 3);

    const QStringList bankParameters = {
        "BANK VOLTAGE",
        "BANK CURRENT",
        "BANK SOC",
        "BANK SOH",
        "BMS TEMPERATURE"
    };

    int bankRow = 1;
    for (const QString &parameter : bankParameters) {
        auto *name = label(parameter, "pageDescription");
        auto *button = new QPushButton("READ");
        button->setMinimumHeight(28);
        button->setObjectName("rs485TestButton");
        button->setProperty("diagnosticTest", true);

        bankLayout->addWidget(name, bankRow, 0);
        bankLayout->addWidget(button, bankRow, 1, 1, 2);

        if (parameter == QStringLiteral("BMS TEMPERATURE")) {
            connect(button, &QPushButton::clicked, this,
                    [this]() { runX3TemperatureTest(70); });
        } else {
            connect(button, &QPushButton::clicked, this,
                    [this, parameter]() {
                        reportUnverifiedRs485Parameter(parameter);
                    });
        }

        ++bankRow;
    }

    layout->addWidget(bankCard);

    // Communication output.
    auto *outputCard = panel();
    auto *outputLayout = new QVBoxLayout(outputCard);
    outputLayout->addWidget(label("COMMUNICATION OUTPUT", "panelHeading"));

    rs485Output_ = new QPlainTextEdit;
    rs485Output_->setReadOnly(true);
    rs485Output_->setMaximumBlockCount(500);
    rs485Output_->setFixedHeight(180);
    
    rs485Output_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rs485Output_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    rs485Output_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rs485Output_->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    rs485Output_->setPlaceholderText(
        "Diagnostic test results, transmitted frames and received frames "
        "will appear here.");
    outputLayout->addWidget(rs485Output_);

    layout->addWidget(outputCard);
    layout->addStretch();

    refreshRs485Diagnostics();
    appendRs485Log("Diagnostic page ready. No serial communication started.");

    return page;
}

void MainWindow::refreshRs485Diagnostics()
{
    if (rs485Listening_ || rs485Busy_) {
        appendRs485Log(
            "Stop listening or finish the current test before refreshing ports.");
        return;
    }

    const QString previousPort =
        rs485PortCombo_ ? rs485PortCombo_->currentData().toString() : QString();

    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (rs485PortCombo_) {
        rs485PortCombo_->clear();

        for (const QSerialPortInfo &info : ports) {
            rs485PortCombo_->addItem(
                QStringLiteral("%1 (%2)")
                    .arg(info.portName(), info.systemLocation()),
                info.systemLocation());
        }

        const int previousIndex = rs485PortCombo_->findData(previousPort);
        if (previousIndex >= 0)
            rs485PortCombo_->setCurrentIndex(previousIndex);
    }

    if (ports.isEmpty()) {
        if (rs485StatusValue_)
            rs485StatusValue_->setText("No serial interfaces detected.");

        appendRs485Log("No serial ports detected.");
        return;
    }

    QStringList descriptions;

    for (const QSerialPortInfo &info : ports) {
        descriptions.append(
            QStringLiteral("%1 (%2)")
                .arg(info.portName(), info.systemLocation()));
    }

    if (rs485StatusValue_) {
        rs485StatusValue_->setText(
            QStringLiteral("%1 serial interface(s) detected.")
                .arg(ports.size()));
    }

    appendRs485Log("Available serial ports: " + descriptions.join(", "));
    updateRs485ListenerControls();
}

void MainWindow::runX3TemperatureTest(int registerAddress)
{
    if (rs485Busy_) {
        appendRs485Log("A diagnostic transaction is already running.");
        return;
    }

    // Register 70 is the BMS temperature. Battery-temperature registers
    // are 72 through 100. Register 71 is deliberately excluded.
    const bool readAllBatteryTemperatures = (registerAddress == -1);
    if (!readAllBatteryTemperatures &&
        registerAddress != 70 &&
        (registerAddress < 72 || registerAddress > 100)) {
        appendRs485Log("ERROR: Temperature register must be 70 or 72–100.");
        return;
    }

    if (!rs485PortCombo_ || rs485PortCombo_->currentData().toString().isEmpty()) {
        appendRs485Log("ERROR: Select an available serial port first.");
        return;
    }

    const QString portName = rs485PortCombo_->currentData().toString();

    const bool resumeListening = rs485Listening_;

    if (resumeListening) {
        appendRs485Log(
            "Stopping listener before starting register test.");
        stopRs485Listening(false);
    }

    rs485Busy_ = true;
    updateRs485ListenerControls();
    const auto buttons = findChildren<QPushButton *>("rs485TestButton");
    for (QPushButton *button : buttons)
        button->setEnabled(false);

    const QString testDescription = readAllBatteryTemperatures
        ? QStringLiteral("battery-temperature registers 72–75")
        : QStringLiteral("temperature register %1").arg(registerAddress);

    appendRs485Log(
        QStringLiteral("Starting RXN X3 %1 test on %2 at 115200 8N1. "
                       "Conversion: temperature °C = raw − 128.")
            .arg(testDescription, portName));

    // Keep all blocking serial operations inside this worker thread.
    rs485Worker_ = QThread::create([this, portName, registerAddress,
                                    readAllBatteryTemperatures, resumeListening]() {
        QSerialPort serial;
        serial.setPortName(portName);
        serial.setBaudRate(QSerialPort::Baud115200);
        serial.setDataBits(QSerialPort::Data8);
        serial.setParity(QSerialPort::NoParity);
        serial.setStopBits(QSerialPort::OneStop);
        serial.setFlowControl(QSerialPort::NoFlowControl);

        QStringList results;
        QString error;

        if (!serial.open(QIODevice::ReadWrite)) {
            error = "Could not open serial port: " + serial.errorString();
        } else {
            const int firstRegister =
                readAllBatteryTemperatures ? 72 : registerAddress;
            const int lastRegister =
                readAllBatteryTemperatures ? 75 : registerAddress;

            for (int address = firstRegister; address <= lastRegister; ++address) {
                // Empirically observed RXN X3 diagnostic request:
                // [0x30, register, 0, 0, 0, 0, 0, 0x55]
                // This is intentionally separate from the manual-based
                // 9600 8N1 protocol implementation.
                QByteArray request;
                request.append(static_cast<char>(48));
                request.append(static_cast<char>(address));
                request.append(char(0));
                request.append(char(0));
                request.append(char(0));
                request.append(char(0));
                request.append(char(0));
                request.append(static_cast<char>(85));

                serial.clear(QSerialPort::Input);

                const qint64 written = serial.write(request);
                if (written != request.size() ||
                    !serial.waitForBytesWritten(300)) {
                    error = QStringLiteral("Register %1 TX failed: %2")
                                .arg(address)
                                .arg(serial.errorString());
                    break;
                }

                QByteArray response;
                QElapsedTimer timer;
                timer.start();

                while (response.size() < 4 && timer.elapsed() < 350) {
                    if (serial.bytesAvailable() == 0)
                        serial.waitForReadyRead(50);

                    response += serial.read(4 - response.size());
                }

                QStringList txBytesDecimal;
                for (char byte : request) {
                    txBytesDecimal.append(
                        QString::number(static_cast<quint8>(byte)));
                }
                const QString txHex = txBytesDecimal.join(' ');
                const QString rxHex =
                    QString::fromLatin1(response.toHex(' ').toUpper());

                if (response.size() != 4) {
                    results.append(
                        QStringLiteral("R%1 | TX: %2 | RX: %3 | TIMEOUT: received %4/4 bytes")
                            .arg(address)
                            .arg(txHex, rxHex)
                            .arg(response.size()));
                    continue;
                }

                const quint8 ack =
                    static_cast<quint8>(response.at(0));

                if (ack != 1) {
                    results.append(
                        QStringLiteral("R%1 | TX: %2 | RX: %3 | ACK: %4 (not success)")
                            .arg(address)
                            .arg(txHex, rxHex)
                            .arg(ack));
                    continue;
                }

                const quint8 raw =
                    static_cast<quint8>(response.at(1));

                if (raw == 0) {
                    results.append(
                        QStringLiteral(
                            "R%1 | TX: %2 | RX: %3 | ACK: 1 SUCCESS | "
                            "Raw: 0 | Temperature: UNAVAILABLE | "
                            "trailing byte: 0x%4 (unverified)")
                            .arg(address)
                            .arg(txHex, rxHex)
                            .arg(static_cast<quint8>(response.at(3)),
                                 2, 16, QLatin1Char('0')).toUpper());

                    continue;
                }

                const int temperatureC =
                    static_cast<int>(raw) - 128;

                results.append(
                    QStringLiteral("R%1 | TX: %2 | RX: %3 | ACK: 1 SUCCESS | "
                                   "Raw: %4 | Temperature: %5 °C | "
                                   "trailing byte: 0x%6 (unverified)")
                        .arg(address)
                        .arg(txHex, rxHex)
                        .arg(raw)
                        .arg(temperatureC)
                        .arg(static_cast<quint8>(response.at(3)),
                             2, 16, QLatin1Char('0')).toUpper());
            }

            serial.close();
        }

        QMetaObject::invokeMethod(
            this,
            [this, results, error, resumeListening]() {
                rs485Busy_ = false;

                const auto buttons =
                    findChildren<QPushButton *>("rs485TestButton");
                for (QPushButton *button : buttons)
                    button->setEnabled(true);

                if (!error.isEmpty())
                    appendRs485Log("ERROR: " + error);

                for (const QString &result : results)
                    appendRs485Log(result);

                if (results.isEmpty() && error.isEmpty())
                    appendRs485Log("No temperature diagnostic response received.");

                updateRs485ListenerControls();

                if (resumeListening && !shuttingDown_) {
                    appendRs485Log(
                        "Register test completed. Resuming RS-485 listener.");
                    startRs485Listening();
                }
            },
            Qt::QueuedConnection);
    });

    connect(rs485Worker_, &QThread::finished,
            rs485Worker_, &QObject::deleteLater);
    rs485Worker_->start();
}

void MainWindow::runX3CellVoltageTest(int cellNumber)
{
    if (rs485Busy_) {
        appendRs485Log("A diagnostic transaction is already running.");
        return;
    }

    if (!rs485PortCombo_ || rs485PortCombo_->currentData().toString().isEmpty()) {
        appendRs485Log("ERROR: Select an available serial port first.");
        return;
    }

    const QString portName = rs485PortCombo_->currentData().toString();

    const bool resumeListening = rs485Listening_;

    if (resumeListening) {
        appendRs485Log(
            "Stopping listener before starting register test.");
        stopRs485Listening(false);
    }

    rs485Busy_ = true;
    updateRs485ListenerControls();

    const auto buttons = findChildren<QPushButton *>("rs485TestButton");
    for (QPushButton *button : buttons)
        button->setEnabled(false);

    appendRs485Log(
        QStringLiteral("Starting RXN X3 cell-voltage test on %1 at 115200 8N1.")
            .arg(portName));

    // The QSerialPort is created, opened, used and destroyed in this worker.
    // The GUI thread never performs blocking serial I/O.
    rs485Worker_ = QThread::create([this, portName, cellNumber, resumeListening]() {
        QSerialPort serial;
        serial.setPortName(portName);
        serial.setBaudRate(QSerialPort::Baud115200);
        serial.setDataBits(QSerialPort::Data8);
        serial.setParity(QSerialPort::NoParity);
        serial.setStopBits(QSerialPort::OneStop);
        serial.setFlowControl(QSerialPort::NoFlowControl);

        QStringList results;
        QString error;

        if (!serial.open(QIODevice::ReadWrite)) {
            error = "Could not open serial port: " + serial.errorString();
        } else {
            const int firstCell = cellNumber < 0 ? 0 : cellNumber;
            const int lastCell = cellNumber < 0 ? 3 : cellNumber;

            for (int cell = firstCell; cell <= lastCell; ++cell) {
                // Empirically verified RXN X3 request:
                // [0x30, address, 0, 0, 0, 0, 0, 0x55]
                const quint8 address = static_cast<quint8>(101 + cell);

                QByteArray request;
                request.append(static_cast<char>(48));
                request.append(static_cast<char>(address));
                request.append(char(0));
                request.append(char(0));
                request.append(char(0));
                request.append(char(0));
                request.append(char(0));
                request.append(static_cast<char>(85));

                serial.clear(QSerialPort::Input);

                const qint64 written = serial.write(request);
                if (written != request.size()
                    || !serial.waitForBytesWritten(300)) {
                    error = QStringLiteral(
                        "B%1 TX failed: %2")
                        .arg(cell + 1)
                        .arg(serial.errorString());
                    break;
                }

                QByteArray response;
                QElapsedTimer timer;
                timer.start();

                while (response.size() < 4 && timer.elapsed() < 350) {
                    if (serial.bytesAvailable() == 0)
                        serial.waitForReadyRead(50);

                    response += serial.read(4 - response.size());
                }

                QStringList txBytesDecimal;
                for (char byte : request) {
                    txBytesDecimal.append(
                        QString::number(static_cast<quint8>(byte)));
                }
                const QString txHex = txBytesDecimal.join(' ');
                const QString rxHex = QString::fromLatin1(response.toHex(' ').toUpper());

                if (response.size() != 4) {
                    results.append(
                        QStringLiteral("B%1 | TX: %2 | RX: %3 | TIMEOUT: received %4/4 bytes")
                            .arg(cell + 1)
                            .arg(txHex, rxHex)
                            .arg(response.size()));
                    continue;
                }

                const quint8 ack = static_cast<quint8>(response.at(0));

                if (ack != 1) {
                    results.append(
                        QStringLiteral("B%1 | TX: %2 | RX: %3 | ACK: %4 (not success)")
                            .arg(cell + 1)
                            .arg(txHex, rxHex)
                            .arg(ack));
                    continue;
                }

                const quint16 raw =
                    static_cast<quint8>(response.at(1))
                    | (static_cast<quint16>(
                           static_cast<quint8>(response.at(2))) << 8);

                const double voltage = raw / 1000.0;

                results.append(
                    QStringLiteral(
                        "B%1 | TX: %2 | RX: %3 | ACK: 1 SUCCESS | "
                        "Raw: %4 | Voltage: %5 V | trailing byte: 0x%6 (unverified)")
                        .arg(cell + 1)
                        .arg(txHex, rxHex)
                        .arg(raw)
                        .arg(voltage, 0, 'f', 3)
                        .arg(static_cast<quint8>(response.at(3)), 2, 16,
                             QLatin1Char('0')).toUpper());
            }

            serial.close();
        }

        QMetaObject::invokeMethod(
            this,
            [this, results, error, resumeListening]() {
                rs485Busy_ = false;

                const auto buttons =
                    findChildren<QPushButton *>("rs485TestButton");

                for (QPushButton *button : buttons)
                    button->setEnabled(true);

                if (!error.isEmpty())
                    appendRs485Log("ERROR: " + error);

                for (const QString &result : results)
                    appendRs485Log(result);

                if (results.isEmpty() && error.isEmpty())
                    appendRs485Log("No diagnostic response received.");

                updateRs485ListenerControls();

                if (resumeListening && !shuttingDown_) {
                    appendRs485Log(
                        "Register test completed. Resuming RS-485 listener.");
                    startRs485Listening();
                }
            },
            Qt::QueuedConnection);
    });

    connect(rs485Worker_, &QThread::finished,
            rs485Worker_, &QObject::deleteLater);

    rs485Worker_->start();
}

void MainWindow::showPage(int page) { if (pages_ && page >= 0 && page < pages_->count()) pages_->setCurrentIndex(page); }

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateResponsiveLayout();
}

void MainWindow::updateResponsiveLayout() {
    const bool compact=width()<1050;
    if (sidebar_) {
        sidebar_->setFixedWidth(compact?76:218);
        if (sideSectionLabel_) sideSectionLabel_->setVisible(!compact);
        if (bankConfigLabel_) bankConfigLabel_->setVisible(!compact);
        if (modeNoteLabel_) modeNoteLabel_->setVisible(!compact);
        if (classificationLabel_) classificationLabel_->setVisible(!compact);
        for (QPushButton *button : navButtons_) {
            button->setText(compact?button->property("shortText").toString():button->property("fullText").toString());
            button->setToolTip(button->property("fullText").toString());
            button->setStyleSheet(compact?QStringLiteral("text-align:center; padding:10px 2px;"):QString());
        }
    }
    if (brandSubLabel_) brandSubLabel_->setVisible(width()>=1050);
    if (screenSubtitleLabel_) screenSubtitleLabel_->setVisible(width()>=1050);
    if (telemetryBoxWidget_) telemetryBoxWidget_->setVisible(width()>=900);
    if (clockBoxWidget_) clockBoxWidget_->setVisible(width()>=900);
    if (sidebar_) for (QLabel *note : sidebar_->findChildren<QLabel *>("sideNote")) note->setVisible(!compact);
    const int available=width()-(compact?92:240)-36;
    const int metricColumns=available>=850?4:(available>=520?2:1);
    if (metricsGrid_) {
        while (QLayoutItem *item=metricsGrid_->takeAt(0)) delete item;
        for (int i=0;i<metricCards_.size();++i) metricsGrid_->addWidget(metricCards_.at(i),i/metricColumns,i%metricColumns);
        for (int c=0;c<metricColumns;++c) metricsGrid_->setColumnStretch(c,1);
    }
    const int batteryColumns=available>=850?4:(available>=520?2:1);
    if (batteryGrid_) {
        while (QLayoutItem *item=batteryGrid_->takeAt(0)) delete item;
        for (int i=0;i<batteryCards_.size();++i) batteryGrid_->addWidget(batteryCards_.at(i),i/batteryColumns,i%batteryColumns);
        for (int c=0;c<batteryColumns;++c) batteryGrid_->setColumnStretch(c,1);
    }
    if (lowerGrid_) {
        const int cols=available>=850?3:1;
        while (QLayoutItem *item=lowerGrid_->takeAt(0)) delete item;
        if (cols==3 && lowerCards_.size()>=2) {
            lowerGrid_->addWidget(lowerCards_.at(0),0,0,1,2);
            lowerGrid_->addWidget(lowerCards_.at(1),0,2,1,1);
        } else {
            for (int i=0;i<lowerCards_.size();++i) lowerGrid_->addWidget(lowerCards_.at(i),i,0);
        }
        for (int c=0;c<cols;++c) lowerGrid_->setColumnStretch(c,1);
    }
}

void MainWindow::updateMetrics(double voltageV, double currentA, double socPct, double sohPct,
                               const QString &systemState, bool dataFresh, const QString &mode) {
    const bool isSimulator = controller_ && controller_->sourceId() == QStringLiteral("SIMULATOR");

    if (systemStateValue_)
        systemStateValue_->setText(systemState);

    if (telemetryValue_) {
        if (dataFresh) {
            telemetryValue_->setText(isSimulator
                ? QStringLiteral("●  SIMULATOR ACTIVE")
                : QStringLiteral("●  RXN BMS DATA ACTIVE"));
            telemetryValue_->setObjectName("linkPill");
        } else {
            telemetryValue_->setText(isSimulator
                ? QStringLiteral("●  DATA STALE")
                : QStringLiteral("●  RXN BMS NOT CONNECTED"));
            telemetryValue_->setObjectName("linkPillBad");
        }
        telemetryValue_->style()->unpolish(telemetryValue_);
        telemetryValue_->style()->polish(telemetryValue_);
    }

    if (dataFresh) {
        if (bankVoltageValue_) bankVoltageValue_->setText(QString::number(voltageV,'f',2));
        if (currentValue_) currentValue_->setText(QString::number(currentA,'f',1));
        if (socValue_) socValue_->setText(QString::number(qRound(socPct)));
        if (sohValue_) sohValue_->setText(QString::number(qRound(sohPct)));
        if (bankSohBar_) {
            bankSohBar_->setValue(qBound(0,qRound(sohPct),100));
            bankSohBar_->setEnabled(true);
        }
        bankSocHistory_.append(qBound(0.0, socPct, 100.0));
        while (bankSocHistory_.size() > 60) bankSocHistory_.removeFirst();
        if (bankSocGraph_) bankSocGraph_->setValues(bankSocHistory_);

        if (chargeStatusValue_) {
            const QString state = currentA > 1.0 ? QStringLiteral("CHARGING")
                : currentA < -1.0 ? QStringLiteral("DISCHARGING")
                : QStringLiteral("IDLE / FLOAT");
            chargeStatusValue_->setText(
                QStringLiteral("%1 · %2 A").arg(state).arg(currentA,0,'f',1));
        }
    } else {
        if (bankVoltageValue_) bankVoltageValue_->setText(QStringLiteral("N/A"));
        if (currentValue_) currentValue_->setText(QStringLiteral("N/A"));
        if (socValue_) socValue_->setText(QStringLiteral("N/A"));
        if (sohValue_) sohValue_->setText(QStringLiteral("N/A"));
        if (bankSohBar_) {
            bankSohBar_->setValue(0);
            bankSohBar_->setEnabled(false);
        }
        if (chargeStatusValue_)
            chargeStatusValue_->setText(QStringLiteral("CHARGE STATUS: UNAVAILABLE"));
    }

    const auto readings = model_->readings();
    for (int i = 0; i < 4; ++i) {
        const bool hasReading = i < readings.size();
        const bool valid = dataFresh && hasReading
            && readings.at(i).sensorValid;

        if (batteryStatus_[i]) {
            QString statusText;
            QString statusStyle;
            if (!dataFresh || !hasReading) {
                statusText = QStringLiteral("● DATA UNAVAILABLE");
                statusStyle = QStringLiteral("statusUnavailable");
            } else if (!readings.at(i).sensorValid) {
                statusText = QStringLiteral("● SENSOR FAULT");
                statusStyle = QStringLiteral("statusFault");
            } else {
                statusText = QStringLiteral("● ONLINE");
                statusStyle = QStringLiteral("statusOnline");
            }
            batteryStatus_[i]->setText(statusText);
            batteryStatus_[i]->setObjectName(statusStyle);
            batteryStatus_[i]->style()->unpolish(batteryStatus_[i]);
            batteryStatus_[i]->style()->polish(batteryStatus_[i]);
        }

        if (!valid) {
            if (batteryVoltage_[i]) batteryVoltage_[i]->setText(QStringLiteral("N/A"));
            if (batteryTemperature_[i]) batteryTemperature_[i]->setText(QStringLiteral("N/A"));
            if (batterySoc_[i]) batterySoc_[i]->setText(QStringLiteral("N/A"));
            if (batterySocBars_[i]) {
                batterySocBars_[i]->setValue(0);
                batterySocBars_[i]->setEnabled(false);
            }
            if (batterySoh_[i]) batterySoh_[i]->setText(QStringLiteral("N/A"));
            if (batterySohBars_[i]) {
                batterySohBars_[i]->setValue(0);
                batterySohBars_[i]->setEnabled(false);
            }
            continue;
        }

        const auto &r = readings.at(i);
        if (batteryVoltage_[i]) batteryVoltage_[i]->setText(QString::number(r.voltageV,'f',2));
        if (batteryTemperature_[i]) batteryTemperature_[i]->setText(QString::number(r.temperatureC,'f',1) + " °C");
        if (batterySoc_[i]) batterySoc_[i]->setText(QString::number(qRound(r.socPct)) + "%");
        if (batterySocBars_[i]) {
            batterySocBars_[i]->setValue(qBound(0,qRound(r.socPct),100));
            batterySocBars_[i]->setEnabled(true);
        }
        if (batterySoh_[i]) batterySoh_[i]->setText(
            qIsFinite(r.sohPct) && r.sohPct >= 0.0 && r.sohPct <= 100.0
                ? QString::number(r.sohPct,'f',1) + "%" : QStringLiteral("N/A"));
        if (batterySohBars_[i]) {
            const bool sohValid = qIsFinite(r.sohPct) && r.sohPct >= 0.0 && r.sohPct <= 100.0;
            batterySohBars_[i]->setValue(sohValid ? qBound(0,qRound(r.sohPct),100) : 0);
            batterySohBars_[i]->setEnabled(sohValid);
        }
    }

    Q_UNUSED(mode);
}
