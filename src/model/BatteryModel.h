#pragma once
#include "../domain/BatteryTypes.h"
#include <QAbstractTableModel>
#include <QList>

class BatteryModel final : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { BatteryColumn, VoltageColumn, TemperatureColumn, SocColumn, SohColumn, StateColumn, ColumnCount };
    explicit BatteryModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void setReadings(const QList<BatteryReading> &readings);
    const QList<BatteryReading> &readings() const { return readings_; }
private:
    QList<BatteryReading> readings_;
};
