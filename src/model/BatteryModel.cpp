#include "BatteryModel.h"
#include <QBrush>
#include <QColor>
#include <QtMath>

BatteryModel::BatteryModel(QObject *parent) : QAbstractTableModel(parent) {}
int BatteryModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : readings_.size(); }
int BatteryModel::columnCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : ColumnCount; }

QVariant BatteryModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= readings_.size()) return {};
    const auto &r = readings_.at(index.row());
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case BatteryColumn: return QStringLiteral("BATTERY %1").arg(r.id);
        case VoltageColumn: return QStringLiteral("%1 V").arg(r.voltageV, 0, 'f', 2);
        case TemperatureColumn: return QStringLiteral("%1 °C").arg(r.temperatureC, 0, 'f', 1);
        case SocColumn: return QStringLiteral("%1%").arg(qRound(r.socPct));
        case SohColumn: return QStringLiteral("%1%").arg(qRound(r.sohPct));
        case StateColumn: return r.sensorValid ? QStringLiteral("ONLINE") : QStringLiteral("SENSOR FAULT");
        default: return {};
        }
    }
    if (role == Qt::TextAlignmentRole && index.column() != BatteryColumn) return int(Qt::AlignCenter);
    if (role == Qt::ForegroundRole && index.column() == StateColumn)
        return QBrush(r.sensorValid ? QColor(QStringLiteral("#4adeb4")) : QColor(QStringLiteral("#ff6577")));
    return {};
}

QVariant BatteryModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return {};
    if (orientation == Qt::Vertical) return section + 1;
    switch (section) {
    case BatteryColumn: return QStringLiteral("BATTERY ID");
    case VoltageColumn: return QStringLiteral("VOLTAGE");
    case TemperatureColumn: return QStringLiteral("TEMPERATURE");
    case SocColumn: return QStringLiteral("STATE OF CHARGE");
    case SohColumn: return QStringLiteral("STATE OF HEALTH");
    case StateColumn: return QStringLiteral("SENSOR STATUS");
    default: return {};
    }
}

void BatteryModel::setReadings(const QList<BatteryReading> &readings) {
    beginResetModel(); readings_ = readings; endResetModel();
}
