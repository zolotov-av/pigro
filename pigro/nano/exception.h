#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <QString>
#include <string>
#include <exception>
#include <iostream>

namespace nano
{

    std::string errno_message(int err);

    class exception: public virtual std::exception
    {
    private:

        std::string m_message;

    public:

        explicit exception(const char *msg): m_message(msg) { }
        explicit exception(const std::string &msg): m_message(msg) { }
        explicit exception(const QString &msg): m_message(msg.toStdString()) { }

        std::string message() const { return m_message; }

        const char * what() const noexcept override;

    };

    template <typename T>
    T check(T ret)
    {
        if ( ret == -1 )
        {
            throw exception( errno_message(errno) );
        }

        return ret;
    }

    template <typename T>
    T warn(T ret)
    {
        if ( ret == -1 )
        {
            auto err = errno;
            std::cerr << "warning: " << errno_message(err) << std::endl;
        }

        return ret;
    }


}

#endif // EXCEPTION_H
