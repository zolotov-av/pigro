#include "PigroWindow.h"
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

PigroWindow::PigroWindow(QWidget *parent): QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.pbRefreshTty, &QPushButton::clicked, this, &PigroWindow::refreshTty);

    refreshTty();
}

PigroWindow::~PigroWindow()
{
}
