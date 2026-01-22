/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2022  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file dump_resyn.hpp
  \brief Dumps out resynthesis problems.

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../utils/null_utils.hpp"

#include <fmt/format.h>
#include <kitty/kitty.hpp>
#include <optional>
#include <string>
#include <fstream>
#include <iostream>

namespace mockturtle
{

template<class TT, class IndexList>
class resyn_dumper
{
public:
  using stats = null_stats;
  using index_list_t = IndexList;
  using truth_table_t = TT;

  explicit resyn_dumper( stats& st )
  {
    (void)st;
  }

  ~resyn_dumper()
  {
    std::cout << "avg. size = " << float( total_size ) / float( num_calls ) << "\n";
  }

  void reset_filename( std::string const& new_prefix )
  {
    filename_prefix = new_prefix;
    id_counter = 0;
  }

  template<class iterator_type, class truth_table_storage_type>
  std::optional<index_list_t> operator()( TT const& target, TT const& care, iterator_type begin, iterator_type end, truth_table_storage_type const& tts, uint32_t max_size = std::numeric_limits<uint32_t>::max(), uint32_t max_level = std::numeric_limits<uint32_t>::max() )
  {
    (void)max_level;

    std::string const filename = fmt::format( "{}{:0>{}}.resyn", filename_prefix, id_counter++, id_width );
    std::ofstream os( filename.c_str(), std::ofstream::out );

    /* header */
    const uint32_t I = 0;
    const uint32_t N = std::distance( begin, end );
    const uint32_t T = 1;
    const uint32_t L = target.num_bits();
    os << fmt::format( "resyn {} {} {} {}\n", I, N, T, L );

    /* simulation signatures */
    while ( begin != end )
    {
      auto const& tt = tts[*begin];
      assert( tt.num_bits() == target.num_bits() );
      kitty::print_binary( tt, os );
      os << "\n";
      ++begin;
    }

    /* target offset */
    kitty::print_binary( ~target & care, os );
    os << "\n";
    /* target onset */
    kitty::print_binary( target & care, os );
    os << "\n";

    /* comment */
    os << fmt::format( "c\nmax size = {}\n", max_size );

    total_size += max_size;
    num_calls++;
    os.close();
    return std::nullopt;
  }

private:
  std::string filename_prefix = "resyn";
  uint32_t id_counter{0};
  const uint32_t id_width{3}; // width = 4 means max id = 9999
  uint32_t total_size{0};
  uint32_t num_calls{0};
};

} // namespace mockturtle
