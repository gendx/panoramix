/*
    Panoramix - 3D view of your surroundings.
    Copyright (C) 2017  Guillaume Endignoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt
*/

#include "gzip.hpp"

#include <zlib.h>
#include <cassert>
#include <sstream>

std::string Gzip::decompress(const std::string& input)
{
    int ret;
    z_stream strm;
    static constexpr int CHUNK_SIZE = 8192;
    unsigned char out[CHUNK_SIZE];

    // allocate inflate state
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = input.size();
    strm.next_in = (Bytef*)input.c_str();

    std::ostringstream oss;

    ret = inflateInit2(&strm, 15 + 32);
    if (ret != Z_OK)
        return std::string();

    // decompress until deflate stream ends or end of file
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = out;

        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);  // state not clobbered
        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR; // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&strm);
            return std::string();
        }

        oss.write((char*)out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return oss.str();
}
