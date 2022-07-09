#ifndef DEVICEINFO_H
#define DEVICEINFO_H

#include <array>
#include <optional>
#include <nano/config.h>
#include <nano/ini.h>
#include <nano/math.h>
#include <QDir>

struct DeviceInfo
{

    /**
     * Загрузить описание чипа из файла
     */
    static std::optional<nano::options> LoadFromFile(const QString &n, const QString &path);

    static inline std::optional<nano::options> LoadByName(const std::string &name)
    {
        return LoadByName(QString::fromStdString(name));
    }

    /**
     * Найти описание чипа по имени
     */
    static std::optional<nano::options> LoadByName(const QString &name);

};


#endif // DEVICEINFO_H
