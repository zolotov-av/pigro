#include "PigroWindow.h"

#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>

void PigroWindow::setButtonsEnabled(bool value)
{
    ui.pbInfo->setEnabled(value);
    ui.pbCheckFirmware->setEnabled(value);
    ui.pbExportFirmware->setEnabled(value);
    ui.pbChipErase->setEnabled(value);
    ui.pbWriteFirmware->setEnabled(value);
    ui.pbWriteFuse->setEnabled(value);
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
        ui.lePigroIniPath->setText(path);
        QSettings().setValue("pigro.ini", path);
        link->setProjectPath(path);
    }
}

void PigroWindow::pigroStarted()
{
    ui.leState->setText(tr("running"));
}

void PigroWindow::pigroStopped()
{
    ui.leState->setText(tr("stopped"));
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
    ui.leDevicePath->setText(dev);

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->readFirmware();
}

void PigroWindow::checkFirmware()
{
    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execCheckFirmware();
}

void PigroWindow::chipErase()
{
    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execChipErase();
}

void PigroWindow::writeFirmware()
{
    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execWriteFirmware();
}

void PigroWindow::writeFuse()
{
    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

    setButtonsEnabled(false);
    link->setTTY(dev);
    link->setProjectPath(ui.lePigroIniPath->text());
    link->execWriteFuse();
}

void PigroWindow::showInfo()
{
    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

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

void PigroWindow::endProgress()
{
    ui.pbInfo->setEnabled(true);
    ui.pbCheckFirmware->setEnabled(true);
    ui.pbExportFirmware->setEnabled(true);
    ui.pbChipErase->setEnabled(true);
    ui.pbWriteFirmware->setEnabled(true);
    ui.pbWriteFuse->setEnabled(true);
}

PigroWindow::PigroWindow(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);

    ui.leState->setText(tr("init"));

    {
        QSettings settings;
        ui.lePigroIniPath->setText(settings.value("pigro.ini").toString());
        ui.leReadFilePath->setText(settings.value("ReadFilePath").toString());
    }

    connect(link, &PigroApp::started, this, &PigroWindow::pigroStarted);
    connect(link, &PigroApp::stopped, this, &PigroWindow::pigroStopped);
    connect(link, &PigroApp::sessionStarted, this, &PigroWindow::sessionStarted);
    connect(link, &PigroApp::beginProgress, this, &PigroWindow::beginProgress);
    connect(link, &PigroApp::reportProgress, this, &PigroWindow::reportProgress);
    connect(link, &PigroApp::reportMessage, this, &PigroWindow::reportMessage);
    connect(link, &PigroApp::endProgress, this, &PigroWindow::endProgress);
    connect(link, &PigroApp::chipInfo, this, &PigroWindow::chipInfo);
    connect(link, &PigroApp::dataReady, this, &PigroWindow::dataReady);

    connect(ui.actionOpenProject, &QAction::triggered, this, &PigroWindow::actionOpenProject);

    connect(ui.pbRefreshTty, &QPushButton::clicked, this, &PigroWindow::refreshTty);
    connect(ui.pbOpenPigroIni, &QPushButton::clicked, this, &PigroWindow::actionOpenProject);
    connect(ui.pbOpenExportFile, &QPushButton::clicked, this, &PigroWindow::openExportFile);
    connect(ui.pbExportFirmware, &QPushButton::clicked, this, &PigroWindow::readFirmware);
    connect(ui.pbCheckFirmware, &QPushButton::clicked, this, &PigroWindow::checkFirmware);
    connect(ui.pbChipErase, &QPushButton::clicked, this, &PigroWindow::chipErase);
    connect(ui.pbWriteFirmware, &QPushButton::clicked, this, &PigroWindow::writeFirmware);
    connect(ui.pbWriteFuse, &QPushButton::clicked, this, &PigroWindow::writeFuse);
    connect(ui.pbInfo, &QPushButton::clicked, this, &PigroWindow::showInfo);
    connect(ui.pbCancel, &QPushButton::clicked, link, &PigroApp::cancel);

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
