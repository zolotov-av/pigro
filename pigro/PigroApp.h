#ifndef PIGROAPP_H
#define PIGROAPP_H

#include "Pigro.h"
#include <QThread>
#include <mutex>

class PigroApp final: public QObject
{
    Q_OBJECT

private:

    std::mutex m_mutex { };

    QThread *m_thread { new QThread(this) };
    Pigro *m_private { nullptr };

    QString m_tty { QStringLiteral("/dev/ttyUSB0") };
    QString m_project_path { QStringLiteral("pigro.ini") };

private slots:

    void threadStarted();
    void threadFinished();

public:

    FirmwareData getFirmwareData();

    explicit PigroApp(QObject *parent = nullptr);
    PigroApp(const PigroApp &) = delete;
    PigroApp(PigroApp &&) = delete;

    ~PigroApp();

    PigroApp& operator = (const PigroApp &) = delete;
    PigroApp& operator = (PigroApp &&) = delete;

    void setTTY(const QString &name)
    {
        m_tty = name;
    }

    void setProjectPath(const QString &path)
    {
        m_project_path = path;
    }

    /**
     * Прочитать прошивку зашитую в устройство
     */
    void readFirmware()
    {
        const std::lock_guard lock(m_mutex);
        if ( m_private )
        {
            QMetaObject::invokeMethod(m_private, "readFirmware", Q_ARG(QString, m_tty), Q_ARG(QString, m_project_path));
        }
    }

    void execChipInfo()
    {
        QMetaObject::invokeMethod(m_private, "isp_chip_info", Q_ARG(QString, m_tty), Q_ARG(QString, m_project_path));
    }

    void execCheckFirmware()
    {
        QMetaObject::invokeMethod(m_private, "isp_check_firmware", Q_ARG(QString, m_tty), Q_ARG(QString, m_project_path));
    }

    void execChipErase()
    {
        QMetaObject::invokeMethod(m_private, "isp_chip_erase", Q_ARG(QString, m_tty), Q_ARG(QString, m_project_path));
    }

    void execWriteFirmware()
    {
        QMetaObject::invokeMethod(m_private, "isp_write_firmware", Q_ARG(QString, m_tty), Q_ARG(QString, m_project_path));
    }

    void execWriteFuse()
    {
        QMetaObject::invokeMethod(m_private, "isp_write_fuse", Q_ARG(QString, m_tty), Q_ARG(QString, m_project_path));
    }

    void start()
    {
        m_thread->start();
    }

    void stop()
    {
        m_thread->quit();
    }

    void wait()
    {
        m_thread->wait();
    }

public slots:

    void cancel();

signals:

    void started();
    void stopped();

    void sessionStarted(int major, int minor);
    void sessionStopped();

    void beginProgress(int min, int max);
    void reportProgress(int value);
    void reportMessage(const QString &message);
    void reportResult(const QString &result);
    void reportException(const QString &message);
    void endProgress();

    void chipInfo(const QString &info);
    void dataReady();

};

#endif // PIGROAPP_H
