#include "PigroWindow.h"

#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QSerialPortInfo>

void PigroWindow::refreshTty()
{
    ui.cbTty->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for(const auto &port : ports)
    {
        ui.cbTty->addItem(QStringLiteral("%1 (%2)").arg(port.portName(), port.systemLocation()), port.systemLocation());
    }

}

void PigroWindow::openPigroIni()
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
    }
}

void PigroWindow::openReadFile()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    //dialog.setNameFilter(tr("pigro.ini (pigro.ini);;INI (*.ini);;All files (*.*)"));
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

    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) )
    {
        QMessageBox::information(this, tr("Read Firmware error"), tr("Cannot open file: %1").arg(file.fileName()));
        return;
    }

    //const FirmwareData orig = FirmwareData::LoadFromFile("/home/ilya/projects/pigro-temp/pigro_avrxx/demo/pigro_avr.hex");

    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

    if ( link->open(dev) )
    {
        link->checkProtoVersion();
        ui.leProtoVersion->setText(link->protoVersion());
        link->loadConfig(ui.lePigroIniPath->text());
        const auto firmware = link->readFirmware();
        link->close();

        QTextStream ts(&file);
        firmware.saveToTextStream(ts);
    }

}

void PigroWindow::showInfo()
{
    const QString dev = ui.cbTty->currentData().toString();
    ui.leDevicePath->setText(dev);

    if ( link->open(dev) )
    {
        link->checkProtoVersion();
        ui.leProtoVersion->setText(link->protoVersion());
        link->loadConfig(ui.lePigroIniPath->text());
        ui.leChipModel->setText(link->getChipInfo());
        link->close();
    }
}

void PigroWindow::beginProgress(int min, int max)
{
    this->setEnabled(false);
    ui.progressBar->setMinimum(min);
    ui.progressBar->setMaximum(max);
    ui.progressBar->setValue(min);
}

void PigroWindow::reportProgress(int value)
{
    ui.progressBar->setValue(value);
}

void PigroWindow::endProgress()
{
    this->setEnabled(true);
}

PigroWindow::PigroWindow(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);

    {
        QSettings settings;
        ui.lePigroIniPath->setText(settings.value("pigro.ini").toString());
        ui.leReadFilePath->setText(settings.value("ReadFilePath").toString());
    }

    connect(link, &PigroApp::beginProgress1, this, &PigroWindow::beginProgress);
    connect(link, &PigroApp::reportProgress1, this, &PigroWindow::reportProgress);
    connect(link, &PigroApp::endProgress1, this, &PigroWindow::endProgress);

    connect(ui.pbRefreshTty, &QPushButton::clicked, this, &PigroWindow::refreshTty);
    connect(ui.pbOpenPigroIni, &QPushButton::clicked, this, &PigroWindow::openPigroIni);
    connect(ui.pbOpenReadFile, &QPushButton::clicked, this, &PigroWindow::openReadFile);
    connect(ui.pbReadFirmware, &QPushButton::clicked, this, &PigroWindow::readFirmware);
    connect(ui.pbInfo, &QPushButton::clicked, this, &PigroWindow::showInfo);

    refreshTty();
}

PigroWindow::~PigroWindow()
{
}
