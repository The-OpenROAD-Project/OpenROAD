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
  \file npn_cache.hpp
  \brief Cached NPN class computation

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <tuple>
#include <vector>

#include <kitty/kitty.hpp>

namespace mockturtle
{

/*! \brief Cache for mapping an N-input truthtable to the corresponding NPN class and the associated NPN transformation. */
class npn_cache
{
  using npn_info = std::tuple<uint64_t, uint32_t, std::vector<uint8_t>>;

public:
  npn_cache() : arr( 1ul << ( 1ul << 4u ) ), has( 1ul << ( 1ul << 4u ), false )
  {
  }

  npn_info operator()( uint64_t tt, uint32_t num_inputs = 4u )
  {
    if ( num_inputs == 4u )
    {
      if ( has[tt] )
      {
        return arr[tt];
      }

      kitty::dynamic_truth_table dtt( num_inputs );
      dtt._bits[0] = tt;
      auto tmp = kitty::exact_npn_canonization( dtt );

      has[tt] = true;
      return ( arr[tt] = { std::get<0>( tmp )._bits[0] & 0xffff, std::get<1>( tmp ), std::get<2>( tmp ) } );
    }

    if ( cache.count( tt ) )
    {
      return cache[tt];
    }

    kitty::dynamic_truth_table dtt( num_inputs );
    dtt._bits[0] = tt;
    auto tmp = kitty::exact_npn_canonization( dtt );

    return ( arr[tt] = { std::get<0>( tmp )._bits[0] & 0xffff, std::get<1>( tmp ), std::get<2>( tmp ) } );
  }

private:
  std::vector<npn_info> arr;
  std::vector<bool> has;

  std::unordered_map<uint64_t, npn_info> cache;
};

} // namespace mockturtle