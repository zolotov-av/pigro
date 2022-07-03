#ifndef PIGROCONSOLE_H
#define PIGROCONSOLE_H

#include <QObject>
#include <pigro/PigroApp.h>

enum PigroAction {
    AT_ACT_INFO,
    AT_ACT_STAT,
    AT_ACT_CHECK,
    AT_ACT_WRITE,
    AT_ACT_ERASE,
    AT_ACT_READ_FUSE,
    AT_ACT_WRITE_FUSE,
    AT_ACT_TEST
};

class PigroConsole final: public QObject
{
    Q_OBJECT

private:

    PigroApp *link { new PigroApp(this) };

    QString m_tty { QStringLiteral("/dev/ttyUSB0")};
    QString m_pigro_ini { QStringLiteral("pigro.ini") };

    PigroAction m_action;

    bool m_verbose { false };

    /**
     * Запус команды
     */
    int execute(PigroAction action);

private slots:

    void pigroStarted();
    void pigroStopped();
    void sessionStarted(int major, int minor);
    void reportMessage(const QString &message);

public:

    explicit PigroConsole(QObject *parent = nullptr);
    PigroConsole(const PigroConsole &) = delete;
    PigroConsole(PigroConsole &&) = delete;

    ~PigroConsole();

    PigroConsole& operator = (const PigroConsole &) = delete;
    PigroConsole& operator = (PigroConsole &&) = delete;

    bool verbose() const { return m_verbose; }

    void setVerbose(bool value)
    {
        m_verbose = value;
    }

    void setTTY(const QString &dev)
    {
        m_tty = dev;
    }

    void setPigroIniPath(const QString &path)
    {
        m_pigro_ini = path;
    }

    int exec(PigroAction action);

};

#endif // PIGROCONSOLE_H
