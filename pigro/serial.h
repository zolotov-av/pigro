#ifndef SERIAL_H
#define SERIAL_H

#include <string>
#include <unistd.h>
#include <sys/poll.h>
#include <iostream>

namespace pigro
{

    class exception
    {
    private:
        std::string m_message;
    public:
        exception(const std::string &msg): m_message(msg) { }

        std::string message() const { return m_message; }
    };

    std::string errno_message(int err);

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
            const auto err = errno;
            std::cerr << "warning: " << errno_message(err) << std::endl;
        }

        return ret;
    }

    class serial
    {
    private:

        /**
         * Файловый дескриптор (виртуального) последовательного порта
         */
        int serial_fd = 0;

    public:

        serial(const char *path = "/dev/ttyUSB0");
        ~serial();

        ssize_t read(void *buf, size_t len)
        {
            return check( ::read(serial_fd, buf, len) );
        }

        ssize_t write(const void *buf, size_t len)
        {
            return check( ::write(serial_fd, buf, len) );
        }

        bool wait()
        {
            pollfd p{};
            p.fd = serial_fd;
            p.events = POLLIN | POLLPRI | POLLERR;

            return check( poll(&p, 1, 200) ) == 1;

        }

        void wait_sync()
        {
            if ( wait() ) return;

            // timeout
            throw pigro::exception("serial timeout");
        }

        uint8_t read_sync()
        {
            wait_sync();
            uint8_t data;
            read(&data, sizeof(data));
            return data;
        }

    };

}


#endif // SERIAL_H
