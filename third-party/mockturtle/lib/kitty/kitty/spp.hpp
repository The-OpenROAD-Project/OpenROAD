/* kitty: C++ truth table library
 * Copyright (C) 2017-2025  EPFL
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
  \file spp.hpp
  \brief Implements methods to compute SPP representations

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "cube.hpp"
#include "traits.hpp"

namespace kitty
{

/*! \brief Merges products in an ESOP into pseudo products

  Given, e.g., the two cubes `abc` and `abd`, this algorithm will combine them
  into a single cube `ab(c+d)`.  The term `(c+d)` is stored as a literal in the
  first argument of the result pair, starting free indexes after `num_vars`.
  Which literals are used is stored in the second argument of the result pair,
  as bitpattern over the original inputs.

  \param esop ESOP form
  \param num_vars Number of variables in ESOP form
*/
inline std::pair<std::vector<cube>, std::vector<uint64_t>> simple_spp( const std::vector<cube>& esop, uint32_t num_vars )
{
  auto next_free = num_vars;
  std::vector<uint64_t> sums;
  auto e = esop.size();
  auto copy = esop;
  const auto var_mask = ( 1u << num_vars ) - 1;
  for ( auto i = 0u; i < e; ++i )
  {
    auto& c = copy[i];
    if ( ( c._mask & 0b1111 ) != c._mask )
    {
      continue;
    }

    const auto it = std::find_if( copy.begin() + i, copy.begin() + e,
                                  [&]( auto const& c2 )
                                  {
                                    bool cnd1 = ( c2._mask & var_mask ) == c2._mask;
                                    const auto same_mask = c._mask & c2._mask;
                                    bool cnd2 = ( c._bits & same_mask ) == ( c2._bits & same_mask );
                                    bool cnd3 = __builtin_popcount( c._mask & ~c2._mask ) == 1;
                                    bool cnd4 = __builtin_popcount( ~c._mask & c2._mask ) == 1;

                                    return cnd1 && cnd2 && cnd3 && cnd4;
                                  } );
    if ( it != copy.begin() + e )
    {
      auto to_delete = c._mask ^ it->_mask;
      c._mask &= ~to_delete;
      c.add_literal( next_free++, __builtin_popcount( ( c._bits | it->_bits ) & to_delete ) % 2 == 0 );
      c._bits &= ~to_delete;
      sums.push_back( to_delete );
      --e;
      std::swap( *it, copy[e] );
    }
  }

  copy.resize( e );
  return { copy, sums };
}

/*! \brief Creates truth table from SPP

  This method is helpful to check which truth table is computed by an SPP form.
*/
template<typename TT>
void create_from_spp( TT& tt, const std::vector<cube>& cubes, const std::vector<uint64_t>& sums )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  clear( tt );

  for ( auto cube : cubes )
  {
    auto product = ~tt.construct(); /* const1 of same size */

    auto bits = cube._bits;
    auto mask = cube._mask;

    for ( auto i = 0u; i < tt.num_vars(); ++i )
    {
      if ( mask & 1 )
      {
        auto var = tt.construct();
        create_nth_var( var, i, !( bits & 1 ) );
        product &= var;
      }
      bits >>= 1;
      mask >>= 1;
    }

    for ( auto i = 0u; i < sums.size(); ++i )
    {
      if ( mask & 1 )
      {
        auto ssum = tt.construct();
        for ( auto j = 0u; j < tt.num_vars(); ++j )
        {
          if ( ( sums[i] >> j ) & 1 )
          {
            auto var = tt.construct();
            create_nth_var( var, j );
            ssum ^= var;
          }
        }
        if ( !( bits & 1 ) )
        {
          ssum = ~ssum;
        }
        product &= ssum;
      }
      bits >>= 1;
      mask >>= 1;
    }

    tt ^= product;
  }
}

} // namespace kitty