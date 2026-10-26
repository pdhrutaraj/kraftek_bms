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
    window.show();
    return app.exec();
}
