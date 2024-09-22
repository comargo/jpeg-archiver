#include "maindialog.h"
#include "processlogmodel.h"
#include "ui_maindialog.h"

#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QThread>
#include <QMessageBox>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentMap>

static const QString LAST_OPEN_DIRECTORY(QStringLiteral("LastOpenDirectory"));
static const QString LAST_SAVE_DIRECTORY(QStringLiteral("LastSaveDirectory"));

MainDialog::MainDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MainDialog)
{
    ui->setupUi(this);
    ui->qualityComboBox->addItem(tr("Low"), QVariant::fromValue(JR_QUALITY_LOW));
    ui->qualityComboBox->addItem(tr("Medium"), QVariant::fromValue(JR_QUALITY_MEDIUM));
    ui->qualityComboBox->addItem(tr("High"), QVariant::fromValue(JR_QUALITY_HIGH));
    ui->qualityComboBox->addItem(tr("Very high"), QVariant::fromValue(JR_QUALITY_VERYHIGH));

    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(1);
    ui->progressBar->setDisabled(true);

    connect(ui->btnInputFileBrowse, &QPushButton::clicked, this, &MainDialog::onOpenFileBrowse);
    connect(ui->btnInputDirBrowse, &QPushButton::clicked, this, [this]{onDirBrowse(ui->inputLineEdit, tr("Select input directory"), &m_lastOpenDir);});
    connect(ui->btnOutputFileBrowse, &QPushButton::clicked, this, &MainDialog::onSaveFileBrowse);
    connect(ui->btnOutputDirBrowse, &QPushButton::clicked, this, [this]{onDirBrowse(ui->outputLineEdit, tr("Select output directory"), &m_lastSaveDir);});
    connect(ui->btnExtraOptions, &QPushButton::clicked, this, &MainDialog::onExtraOptions);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &MainDialog::processFiles);

    QSettings settings;
    auto standardPicturesLocations = QStandardPaths::standardLocations(QStandardPaths::StandardLocation::PicturesLocation);
    if(!standardPicturesLocations.empty()) {
        m_lastOpenDir = standardPicturesLocations.first();
    }
    m_lastOpenDir = settings.value(LAST_OPEN_DIRECTORY, m_lastOpenDir).toString();

    m_lastSaveDir = QDir(QStandardPaths::writableLocation(QStandardPaths::StandardLocation::PicturesLocation)).filePath("Compressed");
    m_lastSaveDir = settings.value(LAST_SAVE_DIRECTORY, m_lastSaveDir).toString();

    fromConfig();

    m_logModel = new ProcessLogModel(this);
    ui->logView->setModel(m_logModel);

    m_futureWatcher = new QFutureWatcher<void>(this);
    connect(m_futureWatcher, &QFutureWatcherBase::finished, this, &MainDialog::onProcessFinished);
    connect(m_futureWatcher, &QFutureWatcherBase::progressRangeChanged, ui->progressBar, &QProgressBar::setRange);
    connect(m_futureWatcher, &QFutureWatcherBase::progressValueChanged, ui->progressBar, &QProgressBar::setValue);
    // connect(m_thread, &ProcessControllerThread::skipped, m_logModel, &ProcessLogModel::appendError);
    // connect(m_thread, &ProcessControllerThread::processed, m_logModel, &ProcessLogModel::appendSuccess);
}

MainDialog::~MainDialog()
{
    QSettings settings;
    settings.setValue(LAST_OPEN_DIRECTORY, m_lastOpenDir);
    settings.setValue(LAST_SAVE_DIRECTORY, m_lastSaveDir);
    delete ui;
}

void MainDialog::done(int r)
{
    if(!m_futureWatcher->isRunning())
        return QDialog::done(r);

    auto doInterrupt = QMessageBox::warning(this, QString(), tr("Do you really want to interrupt?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if(doInterrupt == QMessageBox::Yes) {
        connect(m_futureWatcher, &QFutureWatcherBase::finished, this, &QDialog::reject);
        ui->progressBar->setFormat("Finishing...");
        m_futureWatcher->cancel();
        if(!m_futureWatcher->isRunning())
            return QDialog::done(r);
    }
}

void MainDialog::fromConfig()
{
    ui->qualityComboBox->setCurrentIndex(ui->qualityComboBox->findData(QVariant::fromValue(m_config.quality())));
    ui->stripMetadataCheckBox->setChecked(m_config.strip());
}

void MainDialog::toConfig()
{
    m_config.setQuality(ui->qualityComboBox->currentData().value<jpeg_recompress_quality_t>());
    m_config.setStrip(ui->stripMetadataCheckBox->checkState() == Qt::CheckState::Checked);
}

void MainDialog::enableControls(bool enable)
{
    std::function<QWidgetList(QLayoutItem* layoutItem)> getWidgetsFromLayoutItem = [&getWidgetsFromLayoutItem](QLayoutItem* layoutItem)
    {
        if(layoutItem == nullptr)
            return QWidgetList{};
        if(layoutItem->widget())
            return QWidgetList{layoutItem->widget()};
        if(layoutItem->layout()) {
            QWidgetList widgets{};
            for(int i=0; i<layoutItem->layout()->count(); ++i) {
                widgets.append(getWidgetsFromLayoutItem(layoutItem->layout()->itemAt(i)));
            }
            return widgets;
        }
        return QWidgetList{};
    };

    QWidgetList widgets{};
    widgets.append(getWidgetsFromLayoutItem(ui->formLayout));
    widgets.append(ui->btnExtraOptions);
    widgets.append(ui->buttonBox->button(QDialogButtonBox::Ok));

    std::for_each(widgets.begin(), widgets.end(), std::bind(&QWidget::setEnabled, std::placeholders::_1, enable));
    ui->progressBar->setDisabled(enable);
}

QList<MainDialog::ProcessEntry> MainDialog::scanDirs(const QString &relativePath) const
{
    if(relativePath.isEmpty()) {
        // Check if file name provided filename
        QFileInfo inputFileInfo(QDir::fromNativeSeparators(ui->inputLineEdit->text()));
        if(inputFileInfo.isFile()) {
            auto outPath = QDir::fromNativeSeparators(ui->inputLineEdit->text());
            if(QFileInfo(outPath).isDir()) {
                outPath = QDir(outPath).absoluteFilePath(inputFileInfo.fileName());
            }
            return {{inputFileInfo.absoluteFilePath(), outPath}};
        }
    }
    QDir workInDir{QDir(QDir::fromNativeSeparators(ui->inputLineEdit->text())).absoluteFilePath(relativePath)};
    QDir workOutDir{QDir(QDir::fromNativeSeparators(ui->outputLineEdit->text())).absoluteFilePath(relativePath)};
    QList<MainDialog::ProcessEntry> output;
    auto pathTransformer =
        [&workInDir,
         &workOutDir](const QString &path) -> MainDialog::ProcessEntry {
        return {workInDir.absoluteFilePath(path),
                workOutDir.absoluteFilePath(path)};
    };

    auto pathList = workInDir.entryList({"*.jpg", "*.jpeg", "*.JPG"},
                                        QDir::Files | QDir::Readable);
    std::transform(pathList.cbegin(), pathList.cend(),
                   std::back_inserter(output), pathTransformer);
    auto dirList =
        workInDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for(const auto &subdir : dirList) {
        output.append(scanDirs(QDir(relativePath).filePath(subdir)));
    }

    return output;
}

bool MainDialog::processFile(const QString &inPath, const QString &outPath,
                             int &quality, int &inSize, int &outSize,
                             QString &errorString) const {
    try {
    if (QFile::exists(outPath) && !m_config.overwrite()) {
            errorString = tr("File already exists");
        return false;
    }

    // Steps to do
    // 1) Read file
    QFile inputJpeg{inPath};
    if (!inputJpeg.open(QFile::ReadOnly)) { // TODO: Log the error
        errorString = inputJpeg.errorString();
        return false;
    }
    QByteArray inputBuffer = inputJpeg.readAll();
    unsigned char *output_buf = nullptr;
    size_t output_buf_size = 0;
    quality = jpeg_recompress((unsigned char *)inputBuffer.data(),
                              static_cast<size_t>(inputBuffer.size()), m_config,
                              &output_buf, &output_buf_size);
    if (output_buf_size == 0) {
        errorString = tr("File already processed");
        return false;
    }
    if (!QFileInfo(outPath).dir().exists()) {
        QFileInfo(outPath).dir().mkpath(".");
    }
    QFile outputJpeg(outPath);
    if (!outputJpeg.open(QFile::WriteOnly)) {
        errorString = outputJpeg.errorString();
        return false;
    }
    outputJpeg.write(reinterpret_cast<const char *>(output_buf),
                     static_cast<qint64>(output_buf_size));
    inSize = inputBuffer.size();
    outSize = static_cast<int>(output_buf_size);
    ::free(output_buf);

    return true;
    } catch (const std::exception &ex) {
        errorString = ex.what();
        return false;
    }
}

void MainDialog::onOpenFileBrowse()
{
    QString file = QFileDialog::getOpenFileName(this, QString(), m_lastOpenDir, "JPEG files (*.jpg)");
    if(file.isNull())
        return;
    QFileInfo fileInfo{file};
    m_lastOpenDir = fileInfo.dir().absolutePath();
    ui->inputLineEdit->setText(QDir::toNativeSeparators(file));
}

void MainDialog::onSaveFileBrowse()
{
    QString file = QFileDialog::getSaveFileName(this, QString(), m_lastSaveDir, "JPEG files (*.jpg)");
    if(file.isEmpty())
        return;
    m_lastSaveDir = QFileInfo(file).dir().absolutePath();
    ui->outputLineEdit->setText(QDir::toNativeSeparators(file));
}

void MainDialog::onDirBrowse(QLineEdit *lineEdit, const QString &caption, QString *pDir)
{
    QString selectedDir = QFileDialog::getExistingDirectory(this, caption, *pDir);
    if(selectedDir.isNull())
        return;
    lineEdit->setText(QDir::toNativeSeparators(selectedDir));
    *pDir = selectedDir;
}

void MainDialog::onExtraOptions()
{
    toConfig();
//    ExtraOptionsDialog dlg(this);
//    dlg.setConfig(m_config);
//    if(dlg.exec() != QDialog::Accepted)
//        return;
//    m_config = dlg.config();
    fromConfig();
}

void MainDialog::processFiles()
{
    if(m_futureWatcher->isRunning()) {
        Q_ASSERT(!m_futureWatcher->isRunning());
        return;
    }
    toConfig();
    enableControls(false);
    auto files = scanDirs({});
    m_logModel->clear();
    m_futureWatcher->setFuture(QtConcurrent::map(files, [this](const MainDialog::ProcessEntry &entry) {
        int quality = 0, inSize = 0, outSize = 0;
        QString errorString;
        if(processFile(entry.inPath, entry.outPath, quality, inSize, outSize,
                        errorString)) {
            m_logModel->appendSuccess(entry.inPath, quality, inSize, outSize);
        }
        else {
            m_logModel->appendError(entry.inPath, errorString);
        }
    }));
    while(m_futureWatcher->isRunning()) {
        qApp->processEvents();
    }
}

void MainDialog::onProcessFinished()
{
    enableControls(true);
}
