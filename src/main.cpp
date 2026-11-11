
#include "model/BatteryModel.h"
#include "controller/DashboardController.h"
#include "view/MainWindow.h"
#include "services/SimulatedDataSource.h"
#include "services/RxnBmsDataSource.h"

#include <QApplication>
#include <QFile>
#include <QCommandLineParser>
#include <QDebug>
#include <memory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("Kraftek BMS Console"));
    app.setOrganizationName(QStringLiteral("Kraftek"));
    app.setStyle(QStringLiteral("Fusion"));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Kraftek Battery Monitoring System"));

    parser.addHelpOption();

    QCommandLineOption sourceOption(
        QStringList() << QStringLiteral("s")
                      << QStringLiteral("source"),
        QStringLiteral("Select data source: simulator or rxn."),
        QStringLiteral("source"),
        QStringLiteral("simulator"));

    parser.addOption(sourceOption);
    parser.process(app);

    const QString selectedSource =
        parser.value(sourceOption).trimmed().toLower();

    std::unique_ptr<IBatteryDataSource> source;

    if (selectedSource == QStringLiteral("simulator")) {
        source = std::make_unique<SimulatedDataSource>();
        qInfo() << "Data source: SIMULATOR";
    }
    else if (selectedSource == QStringLiteral("rxn")) {
        source = std::make_unique<RxnBmsDataSource>();
        qInfo() << "Data source: RXN_BMS (placeholder, no serial communication)";
    }
    else {
        qCritical() << "Invalid data source:" << selectedSource;
        qCritical() << "Use --source simulator or --source rxn";
        return 2;
    }

    QFile theme(QStringLiteral(":/styles/bms.qss"));

    if (theme.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    }

    BatteryModel model;

    DashboardController controller(
        &model,
        std::move(source));

    MainWindow window(&model, &controller);
    window.showFullScreen();

    return app.exec();
}

/*
#include "model/BatteryModel.h"
#include "controller/DashboardController.h"
#include "view/MainWindow.h"
#include "services/SimulatedDataSource.h"
#include <QApplication>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QMessageBox>
#include <memory>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Kraftek BMS Console"));
    app.setOrganizationName(QStringLiteral("Kraftek"));
    app.setStyle(QStringLiteral("Fusion"));
    QFile theme(QStringLiteral(":/styles/bms.qss"));
    if (theme.open(QIODevice::ReadOnly | QIODevice::Text)) app.setStyleSheet(QString::fromUtf8(theme.readAll()));
    BatteryModel model;
    auto source = std::make_unique<SimulatedDataSource>();
    DashboardController controller(&model, std::move(source));
    MainWindow window(&model, &controller);
    window.showFullScreen();
    return app.exec();
}
*/