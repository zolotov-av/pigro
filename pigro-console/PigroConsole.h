#ifndef PIGROCONSOLE_H
#define PIGROCONSOLE_H

#include <QObject>
#include <pigro/Pigro.h>

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

    Pigro *pigro { new Pigro(this) };

    PigroAction m_action;

    /**
     * Запус команды
     */
    void execute(PigroAction action);

private slots:

    void sessionStarted(int major, int minor);
    void sessionStopped();
    void reportMessage(const QString &message);

public:

    explicit PigroConsole(QObject *parent = nullptr);
    PigroConsole(const PigroConsole &) = delete;
    PigroConsole(PigroConsole &&) = delete;

    ~PigroConsole();

    PigroConsole& operator = (const PigroConsole &) = delete;
    PigroConsole& operator = (PigroConsole &&) = delete;

    bool verbose() const { return pigro->verbose(); }

    void setVerbose(bool value)
    {
        pigro->setVerbose(value);
    }

    int exec(PigroAction action);

};

#endif // PIGROCONSOLE_H
