#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include "config.h"

class ProcessLogModel;


class QLineEdit;
template <class T> class QFutureWatcher;
namespace Ui {
class MainDialog;
}

class ProcessControllerThread;

class MainDialog : public QDialog
{
    Q_OBJECT
public:
    struct ProcessEntry
    {
        QString inPath;
        QString outPath;
    };

public:
    explicit MainDialog(QWidget *parent = nullptr);
    ~MainDialog();

    // QDialog interface
public slots:
    void done(int) override;

protected:
    void fromConfig();
    void toConfig();

    void enableControls(bool enable);

    QList<ProcessEntry> scanDirs(const QString &relativePath) const;
    bool processFile(const QString &inPath, const QString &outPath, int &quality, int &inSize, int &outSize, QString &errorString) const;

protected slots:
    void onOpenFileBrowse();
    void onSaveFileBrowse();
    void onDirBrowse(QLineEdit *lineEdit, const QString &caption, QString *pDir);
    void onExtraOptions();
    void processFiles();
    void onProcessFinished();

private:
    Ui::MainDialog *ui;
    Config m_config;
    QString m_lastOpenDir;
    QString m_lastSaveDir;
    ProcessLogModel *m_logModel;
    QFutureWatcher<void> *m_futureWatcher;
};

#endif // MAINDIALOG_H
