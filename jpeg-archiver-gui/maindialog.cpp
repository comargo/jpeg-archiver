#include "maindialog.h"
#include "processcontrollerthread.h"
#include "processlogmodel.h"
#include "ui_maindialog.h"

#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QThread>
#include <QMessageBox>

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

    m_thread = new ProcessControllerThread(this);
    connect(m_thread, &QThread::finished, this, &MainDialog::onProcessFinished);
    connect(m_thread, &ProcessControllerThread::numberOfFilesChanged, ui->progressBar, &QProgressBar::setMaximum);
    connect(m_thread, &ProcessControllerThread::currentProgress, ui->progressBar, &QProgressBar::setValue);
    connect(m_thread, &ProcessControllerThread::skipped, m_logModel, &ProcessLogModel::appendError);
    connect(m_thread, &ProcessControllerThread::processed, m_logModel, &ProcessLogModel::appendSuccess);
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
    if(!m_thread->isRunning())
        return QDialog::done(r);

    auto doInterrupt = QMessageBox::warning(this, QString(), tr("Do you really want to interrupt?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
    if(doInterrupt == QMessageBox::Yes) {
        connect(m_thread, &QThread::finished, this, &QDialog::reject);
        ui->progressBar->setFormat("Finishing...");
        m_thread->abort();
        if(!m_thread->isRunning())
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
    if(m_thread->isRunning()) {
        Q_ASSERT(!m_thread->isRunning());
        return;
    }
    toConfig();
    enableControls(false);

    m_thread->setConfig(m_config);
    m_thread->setInput(QDir::fromNativeSeparators(ui->inputLineEdit->text()));
    m_thread->setOutput(QDir::fromNativeSeparators(ui->outputLineEdit->text()));
    m_logModel->clear();

    m_thread->start();
}

void MainDialog::onProcessFinished()
{
    enableControls(true);
}
