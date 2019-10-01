#ifndef PROCESSLOGMODEL_H
#define PROCESSLOGMODEL_H

#include <QAbstractTableModel>

class ProcessLogModelPrivate;
class ProcessLogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ProcessLogModel(QObject *parent = nullptr);
    ~ProcessLogModel() override;

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

public:
    void appendSuccess(QString filename, int quality, int inSize, int outSize);
    void appendError(QString filename, QString reason);
    void clear();



private:
    QScopedPointer<ProcessLogModelPrivate> d_ptr;
    Q_DECLARE_PRIVATE(ProcessLogModel)
};

#endif // PROCESSLOGMODEL_H
