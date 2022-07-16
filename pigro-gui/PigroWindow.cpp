#include "PigroWindow.h"

#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>

#include <QDebug>

void PigroWindow::setButtonsEnabled(bool value)
{
    qDebug() << QStringLiteral("PigroWindow::setButtonsEnabled(%1): thread: %2").arg((value ? "true" : "false"), QThread::currentThread()->objectName());

    ui.actionInfo->setEnabled(value);
    ui.actionCheck->setEnabled(value);
    ui.actionWrite->setEnabled(value);
    ui.actionWriteFuse->setEnabled(value);
    ui.actionErase->setEnabled(value);
    ui.actionExport->setEnabled(value);

    ui.actionCancel->setEnabled(!value);
}

void PigroWindow::openProject(const QString &path)
{
    firmwareInfo.loadFromFile(path);
    m_project->setFirmwareInfo(firmwareInfo);

    if ( firmwareInfo.m_chip_info.have("name") )
    {
        const QString chipName = QString::fromStdString(firmwareInfo.m_chip_info.value("name"));
        ui.leChipName->setText(QStringLiteral("%1 (%2)").arg(chipName, firmwareInfo.device_type));
    }
    else
    {
        ui.leChipName->setText(QStringLiteral("%1 (%2)").arg(firmwareInfo.device, firmwareInfo.device_type));

    }
    ui.leHexFile->setText(firmwareInfo.hexFileName);

    ui.lePigroIniPath->setText(path);
    link->setProjectPath(path);

    ui.projectView->expand(m_project->index(0, 0));
}

void PigroWindow::actionOpenProject()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("pigro.ini (pigro.ini);;INI (*.ini);;All files (*.*)"));
    if ( dialog.exec() )
    {
        const auto fileNames = dialog.selectedFiles();
        const QString path = fileNames.at(0);
        try
        {
            openProject(path);

            QSettings().setValue("pigro.ini", path);
        }
        catch (const std::exception &e)
        {
            QMessageBox::information(this, tr("Error"), tr("Fail to open project file: %1").arg(e.what()));
        }
    }
}

void PigroWindow::actionCloseProject()
{
    m_project->closeProject();
}

void PigroWindow::pigroStarted()
{
    //ui.leState->setText(tr("running"));
}

void PigroWindow::pigroStopped()
{
    //ui.leState->setText(tr("stopped"));
}

void PigroWindow::sessionStarted(int major, int minor)
{
    ui.leProtoVersion->setText(QStringLiteral("%1.%2").arg(major).arg(minor));
}

void PigroWindow::refreshTty()
{
    ui.cbTty->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for(const auto &port : ports)
    {
        ui.cbTty->addItem(QStringLiteral("%1 (%2)").arg(port.portName(), port.systemLocation()), port.systemLocation());
    }
}

void PigroWindow::openExportFile()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Intel HEX (*.hex);;All files (*.*)"));
    if ( dialog.exec() )
    {
        const auto fileNames = dialog.selectedFiles();
        const QString path = fileNames.at(0);
        ui.leReadFilePath->setText(path);
        QSettings().setValue("ReadFilePath", path);
    }
}

void PigroWindow::readFirmware()
{
    QFile file(ui.leReadFilePath->text());
    if ( file.exists() )
    {
        if ( QMessageBox::question(this, tr("Read Firmware"), tr("File exists, override?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes )
        {
            return;
        }
    }

    const QString dev = ui.cbTty->currentData().toString();

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->readFirmware();
}

void PigroWindow::checkFirmware()
{
    const QString dev = ui.cbTty->currentData().toString();

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execCheckFirmware();
}

void PigroWindow::chipErase()
{
    const QString dev = ui.cbTty->currentData().toString();

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execChipErase();
}

void PigroWindow::writeFirmware()
{
    const QString dev = ui.cbTty->currentData().toString();

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execWriteFirmware();
}

void PigroWindow::writeFuse()
{
    const QString dev = ui.cbTty->currentData().toString();

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execWriteFuse();
}

void PigroWindow::showInfo()
{
    const QString dev = ui.cbTty->currentData().toString();

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execChipInfo();
}

void PigroWindow::beginProgress(int min, int max)
{
    setButtonsEnabled(false);

    ui.progressBar->setMinimum(min);
    ui.progressBar->setMaximum(max);
    ui.progressBar->setValue(min);
}

void PigroWindow::reportProgress(int value)
{
    ui.progressBar->setValue(value);
}

void PigroWindow::reportMessage(const QString &message)
{
    QTextCursor prev_cursor = ui.pteMessages->textCursor();
    ui.pteMessages->moveCursor(QTextCursor::End);
    ui.pteMessages->insertPlainText(message + "\n");
    ui.pteMessages->ensureCursorVisible();
    ui.pteMessages->setTextCursor(prev_cursor);
}

void PigroWindow::reportException(const QString &message)
{
    reportMessage(QStringLiteral("[ FAIL ] ").append(message));
    endProgress();
}

void PigroWindow::endProgress()
{
    setButtonsEnabled(true);
}

PigroWindow::PigroWindow(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);
    ui.projectView->setModel(m_project);

    {
        QSettings settings;
        try
        {
            openProject(settings.value("pigro.ini").toString());
        }
        catch (...)
        {
            // ignore
        }
        ui.leReadFilePath->setText(settings.value("ReadFilePath").toString());
    }

    connect(link, &PigroApp::started, this, &PigroWindow::pigroStarted);
    connect(link, &PigroApp::stopped, this, &PigroWindow::pigroStopped);
    connect(link, &PigroApp::sessionStarted, this, &PigroWindow::sessionStarted);
    connect(link, &PigroApp::beginProgress, this, &PigroWindow::beginProgress, Qt::QueuedConnection);
    connect(link, &PigroApp::reportProgress, this, &PigroWindow::reportProgress);
    connect(link, &PigroApp::reportMessage, this, &PigroWindow::reportMessage);
    connect(link, &PigroApp::reportException, this, &PigroWindow::reportException);
    connect(link, &PigroApp::endProgress, this, &PigroWindow::endProgress, Qt::QueuedConnection);
    connect(link, &PigroApp::chipInfo, this, &PigroWindow::chipInfo);
    connect(link, &PigroApp::dataReady, this, &PigroWindow::dataReady);

    connect(ui.actionOpenProject, &QAction::triggered, this, &PigroWindow::actionOpenProject);
    connect(ui.actionCloseProject, &QAction::triggered, this, &PigroWindow::actionCloseProject);

    connect(ui.pbRefreshTty, &QPushButton::clicked, this, &PigroWindow::refreshTty);
    connect(ui.pbOpenExportFile, &QPushButton::clicked, this, &PigroWindow::openExportFile);
    connect(ui.pbCancel, &QPushButton::clicked, link, &PigroApp::cancel);

    connect(ui.actionInfo, &QAction::triggered, this, &PigroWindow::showInfo);
    connect(ui.actionCheck, &QAction::triggered, this, &PigroWindow::checkFirmware);
    connect(ui.actionWrite, &QAction::triggered, this, &PigroWindow::writeFirmware);
    connect(ui.actionWriteFuse, &QAction::triggered, this, &PigroWindow::writeFuse);
    connect(ui.actionErase, &QAction::triggered, this, &PigroWindow::chipErase);
    connect(ui.actionExport, &QAction::triggered, this, &PigroWindow::readFirmware);
    connect(ui.actionCancel, &QAction::triggered, link, &PigroApp::cancel);

    link->start();

    refreshTty();
}

PigroWindow::~PigroWindow()
{
    link->stop();
    link->wait();
}

void PigroWindow::chipInfo(const QString &info)
{
    ui.leChipModel->setText(info);
}

void PigroWindow::dataReady()
{
    const auto firmware = link->getFirmwareData();

    QFile file(ui.leReadFilePath->text());
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        QMessageBox::information(this, tr("Read Firmware error"), tr("Cannot open file: %1").arg(file.fileName()));
        return;
    }

    QTextStream ts(&file);
    firmware.saveToTextStream(ts);
}
