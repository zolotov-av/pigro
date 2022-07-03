#ifndef PIGROPRIVATE_H
#define PIGROPRIVATE_H

#include <QObject>

class PigroPrivate: public QObject
{
    Q_OBJECT

public:

    explicit PigroPrivate(QObject *parent = nullptr);
    PigroPrivate(const PigroPrivate &) = delete;
    PigroPrivate(PigroPrivate &&) = delete;

    ~PigroPrivate();

    PigroPrivate& operator = (const PigroPrivate &) = delete;
    PigroPrivate& operator = (PigroPrivate &&) = delete;

};

#endif // PIGROPRIVATE_H
