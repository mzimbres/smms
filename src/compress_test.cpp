/* Copyright (c) 2018-2021 Marcelo Zimbres Silva (mzimbres at gmail dot com)
 *
 * This file is part of smms.
 *
 * smms is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * smms is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with smms.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <cstring>
#include <iterator>

#include <zlib.h>

#ifndef ZLIB_CONST
#define ZLIB_CONST
#endif

namespace gzip {

// These live in gzip.hpp because it doesnt need to use deps.
// Otherwise, they would need to live in impl files if these methods used
// zlib structures or functions like inflate/deflate)
inline bool is_compressed(const char* data, std::size_t size)
{
    return size > 2
        && (( // zlib
                   static_cast<uint8_t>(data[0]) == 0x78 &&
                   (static_cast<uint8_t>(data[1]) == 0x9C ||
                    static_cast<uint8_t>(data[1]) == 0x01 ||
                    static_cast<uint8_t>(data[1]) == 0xDA ||
                    static_cast<uint8_t>(data[1]) == 0x5E))
        || // gzip
               (static_cast<uint8_t>(data[0]) == 0x1F && static_cast<uint8_t>(data[1]) == 0x8B));
}

class Compressor
{
    std::size_t max_;
    int level_;

  public:
    Compressor(
      int level = Z_DEFAULT_COMPRESSION,
      std::size_t max_bytes = 2000000000) // by default refuse operation if uncompressed data is > 2GB
    : max_(max_bytes),
    level_(level)
    { }

    template <class InputType>
    void compress(InputType& output,
                  char const* data,
                  std::size_t size) const
    {

#ifdef DEBUG
        // Verify if size input will fit into unsigned int, type used for zlib's avail_in
        if (size > std::numeric_limits<unsigned int>::max())
        {
            throw std::runtime_error("size arg is too large to fit into unsigned int type");
        }
#endif
        if (size > max_)
        {
            throw std::runtime_error("size may use more memory than intended when decompressing");
        }

        z_stream deflate_s;
        deflate_s.zalloc = Z_NULL;
        deflate_s.zfree = Z_NULL;
        deflate_s.opaque = Z_NULL;
        deflate_s.avail_in = 0;
        deflate_s.next_in = Z_NULL;

        // The windowBits parameter is the base two logarithm of the window
        // size (the size of the history buffer).  It should be in the range
        // 8..15 for this version of the library.  Larger values of this
        // parameter result in better compression at the expense of memory
        // usage.  This range of values also changes the decoding type:
        //  -8 to -15 for raw deflate
        //  8 to 15 for zlib
        // (8 to 15) + 16 for gzip
        // (8 to 15) + 32 to automatically detect gzip/zlib header (decompression/inflate only)
        constexpr int window_bits = 15 + 16; // gzip with windowbits of 15

        constexpr int mem_level = 8;
        // The memory requirements for deflate are (in bytes):
        // (1 << (window_bits+2)) +  (1 << (mem_level+9))
        // with a default value of 8 for mem_level and our window_bits of 15
        // this is 128Kb

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (deflateInit2(&deflate_s, level_, Z_DEFLATED, window_bits, mem_level, Z_DEFAULT_STRATEGY) != Z_OK)
        {
            throw std::runtime_error("deflate init failed");
        }
#pragma GCC diagnostic pop

        auto* p = const_cast<char*>(data);
        deflate_s.next_in = reinterpret_cast<z_const Bytef*>(p);
        deflate_s.avail_in = static_cast<unsigned int>(size);

        std::size_t size_compressed = 0;
        do
        {
            size_t increase = size / 2 + 1024;
            if (output.size() < (size_compressed + increase))
            {
                output.resize(size_compressed + increase);
            }

            // There is no way we see that "increase" would not fit in an
            // unsigned int, hence we use static cast here to avoid
            // -Wshorten-64-to-32 error
            deflate_s.avail_out = static_cast<unsigned int>(increase);
            deflate_s.next_out = reinterpret_cast<Bytef*>((&output[0] + size_compressed));

            // From http://www.zlib.net/zlib_how.html
            // "deflate() has a return value that can indicate errors, yet we
            // do not check it here.  Why not? Well, it turns out that
            // deflate() can do no wrong here." Basically only possible error
            // is from deflateInit not working properly
            deflate(&deflate_s, Z_FINISH);
            size_compressed += (increase - deflate_s.avail_out);
        } while (deflate_s.avail_out == 0);

        deflateEnd(&deflate_s);
        output.resize(size_compressed);
    }
};

inline
std::string compress(
  char const* data,
  std::size_t size,
  int level = Z_DEFAULT_COMPRESSION)
{
    Compressor comp(level);
    std::string output;
    comp.compress(output, data, size);
    return output;
}

} // namespace gzip

auto readfile(std::string const& file)
{
   using iter_type = std::istreambuf_iterator<char>;

   if (std::empty(file))
      return std::string {iter_type {std::cin}, {}};

   std::ifstream ifs(file);
   return std::string {iter_type {ifs}, {}};
}

int main(int argc, char* argv[])
{
  auto const content = readfile(argv[1]);
  auto const compressed = gzip::compress(content.data(), std::size(content));
  std::cout.write(compressed.data(), std::size(compressed));
}

