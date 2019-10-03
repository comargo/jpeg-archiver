#include "processcontrollerthread.h"

#include <QDir>
#include <QFileInfo>

#include <memory>
#include <mutex>
#include <list>
#include <condition_variable>
#include <thread>
#include <future>
#include <functional>

#include "task.h"

const unsigned int MAX_THREAD_NUM = UINT_MAX;

class ProcessControllerThreadPrivate {
    ProcessControllerThread* q_ptr;
    std::shared_ptr<Config> config;
    QString input;
    QString output;
    std::list<Task> taskList;
    std::mutex mtxTaskList;
    std::condition_variable cvTaskListChanged;
    std::atomic_bool listFull{false};
    std::atomic_uint numberOfFiles{0};
    std::atomic_uint processed{0};
    std::atomic_bool abort{false};

    void addTask(const Task& task);
    void scanDir(const QString &relativePath);
    void process();
\
    Q_DECLARE_PUBLIC(ProcessControllerThread)

};

ProcessControllerThread::ProcessControllerThread(QObject *parent)
    : QThread(parent)
    , d_ptr(new ProcessControllerThreadPrivate)
{
    Q_D(ProcessControllerThread);
    d->q_ptr = this;

}

ProcessControllerThread::~ProcessControllerThread()
{

}

Config ProcessControllerThread::config() const
{
    Q_D(const ProcessControllerThread);
    return *d->config;
}

void ProcessControllerThread::setConfig(const Config &config)
{
    Q_D(ProcessControllerThread);
    d->config.reset(new Config(config));
}

QString ProcessControllerThread::input() const
{
    Q_D(const ProcessControllerThread);
    return d->input;
}

void ProcessControllerThread::setInput(const QString &input)
{
    Q_D(ProcessControllerThread);
    d->input = input;
}

QString ProcessControllerThread::output() const
{
    Q_D(const ProcessControllerThread);
    return d->output;
}

void ProcessControllerThread::setOutput(const QString &output)
{
    Q_D(ProcessControllerThread);
    d->output = output;
}

int ProcessControllerThread::numberOfFiles() const
{
    Q_D(const ProcessControllerThread);
    return static_cast<int>(d->numberOfFiles);
}

void ProcessControllerThread::abort()
{
    Q_D(ProcessControllerThread);
    d->abort = true;
    d->cvTaskListChanged.notify_all();
}

void ProcessControllerThread::run()
{
    Q_D(ProcessControllerThread);
    d->taskList.clear();
    d->listFull = false;
    d->numberOfFiles = 0;
    d->processed = 0;
    d->abort = false;

    std::list<std::thread> processThreads;
    uint nThreads = std::min(std::thread::hardware_concurrency(), MAX_THREAD_NUM);
    for(uint i=0; i<nThreads; ++i) {
        processThreads.emplace_back(&ProcessControllerThreadPrivate::process, d);
    }

    QFileInfo inputInfo(d->input);
    if(inputInfo.isFile()) {
        QString outputPath;
        QFileInfo outputInfo(d->output);
        if(outputInfo.isDir()) {
            outputPath = QDir(outputInfo.absoluteFilePath()).filePath(inputInfo.fileName());
        }
        else {
            outputPath = outputInfo.absoluteFilePath();
        }
        d->addTask(Task{d->config, inputInfo.canonicalFilePath(), outputPath});
    }
    else {
        d->scanDir("");
    }

    emit numberOfFilesChanged(static_cast<int>(d->numberOfFiles));
    d->listFull = true;
    d->cvTaskListChanged.notify_all();

    std::for_each(processThreads.begin(), processThreads.end(), std::mem_fn(&std::thread::join));
}

void ProcessControllerThreadPrivate::addTask(const Task &task)
{
    if(!QFileInfo::exists(task.inputFile()))
        return;
    std::lock_guard<std::mutex> lock{mtxTaskList};
    taskList.push_back(task);
    numberOfFiles++;
    cvTaskListChanged.notify_one();
}

void ProcessControllerThreadPrivate::scanDir(const QString &relativePath)
{
    if(abort)
        return;
    QDir workDir{QDir(input).absoluteFilePath(relativePath)};

    if(workDir.canonicalPath() == QDir(output).canonicalPath())
        return;

    auto fileList = workDir.entryList({"*.jpg", "*.jpeg", "*.JPG"}, QDir::Files|QDir::Readable, QDir::Name);
    auto dirList = workDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot, QDir::Name);
    std::list<std::future<void>> futures;
    std::transform(dirList.begin(), dirList.end(), std::back_inserter(futures), [this, relativePath](const QString &path) -> std::future<void>{
        return std::async(std::launch::async, &ProcessControllerThreadPrivate::scanDir, this, QDir(relativePath).filePath(path));
    });

    QDir saveDir{QDir(output).absoluteFilePath(relativePath)};
    std::for_each(fileList.begin(), fileList.end(), [this, workDir, saveDir](const QString &path) {
        addTask(Task{config, workDir.absoluteFilePath(path), saveDir.absoluteFilePath(path)});
    });
    Q_Q(ProcessControllerThread);
    emit q->numberOfFilesChanged(static_cast<int>(taskList.size()));

    std::for_each(futures.begin(), futures.end(), std::mem_fn(&std::future<void>::wait));

}

void ProcessControllerThreadPrivate::process()
{
    Q_Q(ProcessControllerThread);
    do {
    std::unique_lock<std::mutex> lock(mtxTaskList);
        cvTaskListChanged.wait(lock, [this](){
            return listFull || !taskList.empty() || abort;
        });
        while(!taskList.empty() && !abort) {
            Task task = taskList.front();
            taskList.pop_front();
            // release queue
            lock.unlock();

            if(task.process()) {
                emit q->processed(task.inputFile(), task.quality(), task.inSize(), task.outSize());
            }
            else {
                emit q->skipped(task.inputFile(), task.error());
            }
            Q_Q(ProcessControllerThread);
            emit q->currentProgress(static_cast<int>(++processed));

            // lock queue to check if it empty and take another task
            lock.lock();
        }
    } while(!listFull && !abort);
    return;
}



