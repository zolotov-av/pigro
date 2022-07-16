#include "FirmwareData.h"

uint32_t FirmwareData::getDataSize() const
{
    uint32_t size { 0 };
    for(const auto &it : *this)
    {
        size += it.second.page_size();
    }

    return size;
}

std::vector<uint8_t> FirmwareData::getDataDump() const
{
    std::vector<uint8_t> data (getDataSize());
    uint32_t addr = 0;
    for(const auto &it : *this)
    {
        const auto page_size = it.second.page_size();
        memcpy(data.data() + addr, it.second.data.data(), page_size);
        addr += page_size;
    }

    return data;
}

void FirmwareData::saveToTextStream(QTextStream &ts) const
{
    const auto data = getDataDump();
    const unsigned lineCount = data.size() / 16;

    for(unsigned iLine = 0; iLine < lineCount; iLine++)
    {
        const uint32_t line_addr = iLine * 16;
        char buf[100];
        char *rest = buf + sprintf(buf, ":10%04X00", line_addr);
        uint8_t cs = 0 - 0x10 - uint8_t(line_addr) - uint8_t(line_addr >> 8);
        for(unsigned iByte = 0; iByte < 16; iByte++)
        {
            const uint8_t byte = data[line_addr + iByte];
            cs -= byte;
            rest += sprintf(rest, "%02X", byte);
        }
        sprintf(rest, "%02X\n", cs);
        ts << buf;
    }

    const unsigned restCount = data.size() - lineCount * 16;
    if ( restCount > 0 )
    {
        const uint32_t line_addr = lineCount * 16;
        char buf[100];
        char *rest = buf + sprintf(buf, ":%02X%04X00", restCount, line_addr);
        uint8_t cs = 0 - 0x10 - uint8_t(restCount) - uint8_t(line_addr) - uint8_t(line_addr >> 8);
        for(unsigned iByte = 0; iByte < restCount; iByte++)
        {
            const uint8_t byte = data[line_addr + iByte];
            cs -= byte;
            rest += sprintf(rest, "%02X", byte);
        }
        sprintf(rest, "%02X\n", cs);
        ts << buf;
    }

    ts << ":00000001FF\n";

}
