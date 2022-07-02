#include "PigroWindow.h"

#include <QSettings>
#include <QFileDialog>
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
    dialog.setNameFilter(tr("pigro.ini (pigro.ini);;INI (*.ini);;All files (*.*)"));
    if ( dialog.exec() )
    {
        const auto fileNames = dialog.selectedFiles();
        const QString path = fileNames.at(0);
        ui.lePigroIniPath->setText(path);
        QSettings().setValue("pigro.ini", path);
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

PigroWindow::PigroWindow(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);

    ui.lePigroIniPath->setText(QSettings().value("pigro.ini").toString());

    connect(ui.pbRefreshTty, &QPushButton::clicked, this, &PigroWindow::refreshTty);
    connect(ui.pbOpenPigroIni, &QPushButton::clicked, this, &PigroWindow::openPigroIni);
    connect(ui.pbInfo, &QPushButton::clicked, this, &PigroWindow::showInfo);

    refreshTty();
}

PigroWindow::~PigroWindow()
{
}
