#include "serial.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <cstring>

namespace pigro
{

    std::string errno_message(int err)
    {
        char buf[1024];
        char *message = strerror_r(err, buf, sizeof(buf));
        if ( message != nullptr )
        {
            return std::string(message);
        }

        int len = snprintf(buf, sizeof(buf), "unknown error (errno=%d)", err);
        return std::string(buf, len);
    }

    serial::serial(const char *path): serial_fd(open(path, O_RDWR | O_NOCTTY | O_NDELAY))
    {
        if (serial_fd == -1)
        {
            throw exception("pigro::serial() Unable to open /dev/ttyUSB0");
        }

        struct termios options;

        // Получение текущих опций для порта...
        check( tcgetattr(serial_fd, &options) );

        speed_t speed = B9600;

        cfsetispeed(&options, speed);
        cfsetospeed(&options, speed);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;

        options.c_cflag &= ~CRTSCTS;
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

        options.c_iflag = 0;
        //options.c_iflag &= ~(IXON | IXOFF | IXANY);
        //options.c_iflag &= ~(ICRNL | INLCR | IGNCR);

        options.c_oflag = 0;
        //options.c_oflag &= ~OPOST;
        //options.c_oflag &= ~(ICRNL | INLCR | IGNCR);

        // Установка новых опций для порта...
        check( tcsetattr(serial_fd, TCSANOW, &options) );

        // ???
        check( fcntl(serial_fd, F_SETFL, 0) );
    }

    serial::~serial()
    {
        warn( ::close(serial_fd) );
    }

}

