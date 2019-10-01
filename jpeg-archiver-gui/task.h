#ifndef TASK_H
#define TASK_H

#include <QString>

#include <memory>

#include "config.h"

class Task {
public:
    Task(std::shared_ptr<Config> config, const QString inputFile, const QString outputFile);

    bool process();

    QString inputFile() const;
    QString outputFile() const;

    QString error() const;
    int inSize() const;
    int outSize() const;
    int quality() const;

private:
    std::shared_ptr<Config> m_config;
    QString m_inputFile;
    QString m_outputFile;
    QString m_error;
    int m_inSize = 0;
    int m_outSize = 0;
    int m_quality = 0;
};

#endif // TASK_H
