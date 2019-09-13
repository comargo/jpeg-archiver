#ifndef MAINDIALOG_H
#define MAINDIALOG_H

#include <QDialog>
#include <QLineEdit>

#include "config.h"

namespace Ui {
class MainDialog;
}

class ProcessControllerThread;

class MainDialog : public QDialog
{
    Q_OBJECT

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

protected slots:
    void onOpenFileBrowse();
    void onSaveFileBrowse();
    void onDirBrowse(QLineEdit* lineEdit, const QString &caption, QString *pDir);
    void onExtraOptions();
    void processFiles();
    void onProcessFinished();

private:
    Ui::MainDialog *ui;
    Config m_config;
    QString m_lastOpenDir;
    QString m_lastSaveDir;
    ProcessControllerThread* m_thread;

};

#endif // MAINDIALOG_H
