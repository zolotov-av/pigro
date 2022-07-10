#ifndef PIGRO_PROJECT_MODEL_H
#define PIGRO_PROJECT_MODEL_H

#include <QStandardItemModel>
#include <FirmwareInfo.h>

class ProjectModel: public QStandardItemModel
{
    Q_OBJECT

private:

    QStandardItem *m_project {nullptr};
    QStandardItem *m_chip {nullptr};
    QStandardItem *m_hex {nullptr};

    FirmwareInfo m_firmware_info;

public:

    explicit ProjectModel(QObject *parent = nullptr);
    ProjectModel(const ProjectModel &) = delete;
    ProjectModel(ProjectModel &&) = delete;

    ~ProjectModel();

    ProjectModel& operator = (const ProjectModel &) = delete;
    ProjectModel& operator = (ProjectModel &&) = delete;

    bool isEmpty() const { return m_project == nullptr; }

    void setFirmwareInfo(const FirmwareInfo &info);

    void openProject(const QString &path);
    void closeProject();


signals:

};

#endif // PIGRO_PROJECT_MODEL_H
