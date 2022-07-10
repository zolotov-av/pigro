#include "ProjectModel.h"

ProjectModel::ProjectModel(QObject *parent) : QStandardItemModel(parent)
{
}

ProjectModel::~ProjectModel()
{

}

void ProjectModel::setFirmwareInfo(const FirmwareInfo &info)
{
    closeProject();

    m_firmware_info = info;

    m_project = new QStandardItem(QIcon(":/pigro-gui/icons/icons8-matherboard.png"), m_firmware_info.title());
    m_chip = new QStandardItem(QIcon(":/pigro-gui/icons/icons8-cpu.png"), m_firmware_info.device);
    m_hex = new QStandardItem(QIcon(":/pigro-gui/icons/icons8-file.svg"), m_firmware_info.hexFileName);

    appendRow(m_project);
    m_project->appendRow(m_chip);
    m_project->appendRow(m_hex);

    m_project->setEditable(false);
    m_chip->setEditable(false);
    m_hex->setEditable(false);
}

void ProjectModel::openProject(const QString &path)
{
    setFirmwareInfo(FirmwareInfo{path});
}

void ProjectModel::closeProject()
{
    this->removeRow(0);
    m_project = nullptr;
    m_chip = nullptr;
    m_hex = nullptr;
}
