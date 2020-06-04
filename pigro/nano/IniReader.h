#ifndef INIREADER_H
#define INIREADER_H

#include <nano/string.h>
#include <nano/TextReader.h>

#include <map>

namespace nano
{


    template <int linesize = 512>
    class IniReader
    {
    private:

        using options = std::map<std::string, std::string>;
        using sections = std::map<std::string, options>;

        sections data;

    public:

        IniReader() = default;
        IniReader(const IniReader &) = delete;
        IniReader(IniReader &&) = default;
        ~IniReader() = default;

        void open(const char *path)
        {
            TextReader<linesize> reader(path);

            while ( !reader.eof() )
            {
                std::string line = reader.readLine();

                std::string_view sv = nano::trim(line);
                if ( sv.empty() ) continue;
                if ( sv[0] == '#' ) continue;

                if ( sv[0] == '[' )
                {

                }
            }



            /*
            char buf[4096];
            char section[4096] = "_";

            while ( fgets(buf, sizeof(buf), f) )
            {
              trim(buf);
              if ( *buf == '[' )
              {
                strncpy(section, buf+1, strlen(buf)-2);
                trim(section);
              }
              char *p = strchr(buf, '=');
              if ( p )
              {
                *p = 0;
                if ( ! ini_set(ini, section, trim(buf), trim(p + 1)) )
                {
                  fclose(f);
                  ini_close(ini);
                  return 0;
                }
              }
            }

            fclose(f);
            */

        }

    };

}

#endif // INIREADER_H
