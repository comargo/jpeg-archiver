#include "task.h"

#include <QFile>
#include <QByteArray>
#include <jpeg-recompress.h>
#include <jpeglib.h>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDir>

Task::Task(std::shared_ptr<Config> config, const QString inputFile, const QString outputFile)
    : m_config(config)
    , m_inputFile(inputFile)
    , m_outputFile(outputFile)
{
}

bool Task::process()
{
    if(QFile::exists(m_outputFile) && !m_config->overwrite()) {
        m_error = QCoreApplication::translate("Task", "File already exists");
        return false;
    }
    // Steps to do
    // 1) Read file
    QFile inputJpeg{m_inputFile};
    if(!inputJpeg.open(QFile::ReadOnly)) { // TODO: Log the error
        m_error = inputJpeg.errorString();
        return false;
    }
    QByteArray inputBuffer = inputJpeg.readAll();
    QByteArray outputBuffer = inputBuffer;
    unsigned char *output_buf = nullptr;
    size_t output_buf_size = 0;
    int quality = jpeg_recompress(inputBuffer.data(), static_cast<size_t>(inputBuffer.size()), *m_config, &output_buf, &output_buf_size);
    if(output_buf_size == 0) {
        m_error = QCoreApplication::translate("Task","File already processed");
        return false;
    }
    outputBuffer = QByteArray{reinterpret_cast<char*>(output_buf), static_cast<int>(output_buf_size)};
    ::free(output_buf);
    if(!QFileInfo(m_outputFile).dir().exists()) {
        QFileInfo(m_outputFile).dir().mkpath(".");
    }
    QFile outputJpeg(m_outputFile);
    if(!outputJpeg.open(QFile::WriteOnly)) {
        m_error = outputJpeg.errorString();
        return false;
    }
    outputJpeg.write(outputBuffer);
    m_inSize = inputBuffer.size();
    m_outSize = outputBuffer.size();
    m_quality = quality;
    return true;
}

QString Task::inputFile() const
{
    return m_inputFile;
}

QString Task::outputFile() const
{
    return m_outputFile;
}

QString Task::error() const
{
    return m_error;
}

int Task::inSize() const
{
    return m_inSize;
}

int Task::outSize() const
{
    return m_outSize;
}

int Task::quality() const
{
    return m_quality;
}
