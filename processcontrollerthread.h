#ifndef QPROCESSCONTROLLERTHREAD_H
#define QPROCESSCONTROLLERTHREAD_H

#include <QThread>

#include <QString>
#include "config.h"

class ProcessControllerThreadPrivate;
class ProcessControllerThread : public QThread
{
    Q_OBJECT
public:
    ProcessControllerThread(QObject *parent = Q_NULLPTR);
    ~ProcessControllerThread() override;

    // QThread interface
    Config config() const;
    void setConfig(const Config &config);

    QString input() const;
    void setInput(const QString &input);

    QString output() const;
    void setOutput(const QString &output);

    int numberOfFiles() const;

public slots:
    void abort();

protected:
    void run() override;

signals:
    void numberOfFilesChanged(int num);
    void currentProgress(int num);

private:
    Q_DECLARE_PRIVATE(ProcessControllerThread)
    QScopedPointer<ProcessControllerThreadPrivate> d_ptr;
};

#endif // QPROCESSCONTROLLERTHREAD_H
