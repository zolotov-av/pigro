#ifndef PIGROLINK_H
#define PIGROLINK_H

#include <cstdint>
#include <string>
#include <nano/config.h>

#include "DeviceInfo.h"

constexpr auto PACKET_MAXLEN = 6;

struct packet_t
{
    unsigned char cmd;
    unsigned char len;
    unsigned char data[PACKET_MAXLEN];
};

class PigroLink
{
public:

    virtual bool verbose() const = 0;

    virtual std::string get_option(const std::string &name, const std::string &default_value = {}) = 0;

    virtual const nano::options& chip_info() const = 0;

    /**
     * Отправить пакет данных
     */
    virtual bool send_packet(const packet_t *pkt) = 0;

    /**
     * Прочитать пакет данных
     */
    virtual void recv_packet(packet_t *pkt) = 0;



};

#endif // PIGROLINK_H