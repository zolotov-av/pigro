#ifndef PIGRO_LOG_H
#define PIGRO_LOG_H

#include <QString>
#include <QThread>
#include <cstdio>

class trace
{
public:

    static QString getThreadName()
    {
        return QThread::currentThread()->objectName();
    }

    static void setThreadName(const QString &name)
    {
        QThread::currentThread()->setObjectName(name);
    }

    static void log(const char *message);

    static void log(const std::string &message)
    {
        log(message.c_str());
    }

    static void log(const QString &message)
    {
        log(message.toStdString());
    }

};

#endif // PIGRO_LOG_H
