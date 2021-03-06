#ifndef INTELHEX_H
#define INTELHEX_H

#include <QString>
#include <map>
#include <list>
#include <array>
#include <vector>
#include <string_view>

/**
 * The Intel HEX file format
 */
class IntelHEX
{
public:

    struct row_t
    {
        uint8_t bytes[256 + 5];

        uint8_t length() const { return bytes[0]; }
        uint16_t addr() const { return bytes[1] * 0x100 + bytes[2]; }
        uint8_t type() const { return bytes[3]; }
        uint8_t checksum() const { return bytes[length()+4]; }
        const uint8_t* data() const { return &bytes[4]; }
    };

    std::list<row_t> rows;

    IntelHEX() = default;
    IntelHEX(const IntelHEX &) = delete;
    IntelHEX(IntelHEX &&) = default;

    explicit IntelHEX(const char *path)
    {
        open(QString::fromLocal8Bit(path));
    }

    explicit IntelHEX(const std::string &path)
    {
        open(QString::fromStdString(path));
    }

    explicit IntelHEX(const QString &path)
    {
        open(path);
    }

    void open(const QString &path);

};

#endif // INTELHEX_H
