#ifndef TEXTSTREAM_H
#define TEXTSTREAM_H

#include <string>
#include <array>
#include <fstream>
#include <iostream>
#include "exception.h"

namespace nano
{

    template <int linesize = 1024>
    class TextReader
    {
    private:

        unsigned int lineno = 0;
        std::string path;
        std::fstream fstream;

    public:

        TextReader(const std::string &a_path): path(a_path), fstream(path, std::ios_base::in)
        {
            if ( fstream.fail() ) throw exception("fail to open requested file: " + a_path);
        }

        bool eof()
        {
            return fstream.eof();
        }

        std::string readLine()
        {
            lineno++;
            std::array<char, linesize> line;
            fstream.getline(line.data(), line.size());
            if ( fstream.fail() ) throw exception("line #" + std::to_string(lineno) + " too long: " + path);
            return std::string(line.data());
        }

    };

}

#endif // TEXTSTREAM_H
