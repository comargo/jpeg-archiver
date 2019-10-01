#include "processlogmodel.h"

#include <QIcon>
#include <QMutex>

enum class Sections : int{
    File = 0,
    Quality = 1,
    InSize = 2,
    OutSize = 3,
    Ratio = 4,
    LastColumn,
};

class ProcessLogModelPrivate {
    ProcessLogModel* q_ptr;
    Q_DECLARE_PUBLIC(ProcessLogModel)

    struct ModelData {
        QString fileName;
        int quality;
        int insize;
        int outsize;
        bool status;
        QString failureReason;
    };

    QList<ModelData> model;

    QVariant dataDisplay(const QModelIndex &index) const;
    QVariant sizeToString(int size) const;

    void append(const ModelData &modelData);

    QMutex mtxModel;
};


ProcessLogModel::ProcessLogModel(QObject *parent)
    : QAbstractTableModel(parent)
    , d_ptr(new ProcessLogModelPrivate{})
{
    d_ptr->q_ptr = this;
}

ProcessLogModel::~ProcessLogModel()
{
}

QVariant ProcessLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(role != Qt::DisplayRole)
        return QVariant{};
    if(Qt::Orientation::Horizontal == orientation) {
        switch (Sections(section)) {
        case Sections::File:
            return tr("File name");
        case Sections::Quality:
            return tr("Quality");
        case Sections::InSize:
            return tr("Original size");
        case Sections::OutSize:
            return tr("New file size");
        case Sections::Ratio:
            return tr("Compression ratio");
        default:
            return QVariant{};
        }
    }
    else {
        return section;
    }
}

int ProcessLogModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const ProcessLogModel);
    if (parent.isValid())
        return 0;
    return d->model.size();
}

int ProcessLogModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(Sections::LastColumn);
}

QVariant ProcessLogModel::data(const QModelIndex &index, int role) const
{
    Q_D(const ProcessLogModel);
    if (!index.isValid())
        return QVariant();

    if(Qt::DisplayRole == role)
        return d->dataDisplay(index);

    if(index.column() == 0) {
        auto &modelData = d->model.at(index.row());
        if(modelData.status) {
            if(Qt::DecorationRole == role) {
                if(modelData.quality != 0) {
                    return QIcon(":/icons/tick-circle.png");
                }
                else {
                    return QIcon(":/icons/exclamation-circle.png");
                }
            }
        }
        else {
            if(Qt::DecorationRole == role) {
                return QIcon(":/icons/cross-circle.png");
            }
            if(Qt::ToolTipRole == role) {
                return modelData.failureReason;
            }
        }
    }
    return QVariant();
}

void ProcessLogModel::appendSuccess(QString filename, int quality, int inSize, int outSize)
{
    Q_D(ProcessLogModel);
    d->append({filename, quality, inSize, outSize, true, QString()});
}

void ProcessLogModel::appendError(QString filename, QString reason)
{
    Q_D(ProcessLogModel);
    d->append({filename, 0, 0, 0, false, reason});
}

void ProcessLogModel::clear()
{
    Q_D(ProcessLogModel);
    beginResetModel();
    d->model.clear();
    endResetModel();
}

QVariant ProcessLogModelPrivate::sizeToString(int size) const
{
    double fSize = size;
    QString unit = "byte";
    if(size > 1024*1024) {
        fSize = size/(1024.0*1024.0);
        unit = "MB";
    }
    if(size > 1024) {
        fSize = size/1024.0;
        unit = "KB";
    }
    return QString("%1 %2").arg(fSize, 0, 'f', 2).arg(unit);
}

void ProcessLogModelPrivate::append(const ProcessLogModelPrivate::ModelData &modelData)
{
    Q_Q(ProcessLogModel);
    QMutexLocker locker(&mtxModel);
    q->beginInsertRows(QModelIndex(), model.size(), model.size());
    model.push_back(modelData);
    q->endInsertRows();
}

QVariant ProcessLogModelPrivate::dataDisplay(const QModelIndex &index) const
{
    auto &data = model.at(index.row());
    switch (Sections(index.column())) {
    case Sections::File:
        return data.fileName;
    case Sections::Quality:
        if(data.quality)
            return QString::number(data.quality);
        else
            return QVariant{};
    case Sections::InSize:
        if(data.insize)
            return sizeToString(data.insize);
        else
            return QVariant{};
    case Sections::OutSize:
        if(data.insize)
            return sizeToString(data.outsize);
        else
            return QVariant{};
    case Sections::Ratio:
        if(data.insize != 0 && data.outsize != 0)
            return QString::number(100*data.outsize/data.insize)+"%";
        else
            return QVariant{};
    default:
        return QVariant{};

    }
}
