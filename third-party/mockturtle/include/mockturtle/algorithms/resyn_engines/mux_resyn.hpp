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
  \file mux_resyn.hpp
  \brief Implements resynthesis methods for MuxIGs.

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../utils/index_list/index_list.hpp"
#include "../../utils/null_utils.hpp"

#include <fmt/format.h>
#include <kitty/kitty.hpp>

#include <optional>
#include <unordered_map>
#include <vector>

namespace mockturtle
{

/*! \brief Logic resynthesis engine for MuxIGs with top-down decomposition.
 *
 */
template<class TT>
class mux_resyn
{
public:
  using stats = null_stats;
  using index_list_t = muxig_index_list;
  using truth_table_t = TT;

public:
  explicit mux_resyn( stats& st )
      : st( st )
  {
  }

  template<class iterator_type, class truth_table_storage_type>
  std::optional<index_list_t> operator()( TT const& target, TT const& care, iterator_type begin, iterator_type end, truth_table_storage_type const& tts, uint32_t max_size = std::numeric_limits<uint32_t>::max() )
  {
    divisors.clear();
    normalized.clear();

    normalized.emplace_back( ~target ); // 0 XNOR target = ~target
    normalized.emplace_back( target );  // 1 XNOR target = target

    while ( begin != end )
    {
      auto const& tt = tts[*begin];
      assert( tt.num_bits() == target.num_bits() );
      normalized.emplace_back( tt ^ normalized[0] ); // tt XNOR target = tt XOR ~target
      normalized.emplace_back( tt ^ target );        // ~tt XNOR target = tt XOR target
      divisors.emplace_back( tt );
      ++begin;
    }

    num_bits = kitty::count_ones( care );
    remaining_size = max_size;
    max_depth = std::min( 10u, max_size );
    index_list.clear();
    index_list.add_inputs( divisors.size() );

    auto res = compute_function( care, 0 );
    if ( res )
    {
      index_list.add_output( *res );
      return index_list;
    }
    else
    {
      return std::nullopt;
    }
  }

private:
  std::optional<index_list_t::element_type> compute_function( TT const& care, uint32_t depth )
  {
    if ( depth > max_depth )
      return std::nullopt;

    uint32_t chosen_t{0}, chosen_s{0}, chosen_e{0}, score1{0}, score2{0}, max_score{0}, min_score1{num_bits}, min_score2{num_bits};
    for ( auto t = 0u; t < normalized.size(); ++t )
    {
      score1 = kitty::count_ones( normalized.at( t ) & care );
      if ( score1 > max_score )
      {
        max_score = score1;
        chosen_t = t;
        if ( score1 == num_bits )
        {
          /* 0-resub */
          return t;
        }
      }
    }

    if ( remaining_size == 0 )
      return std::nullopt;

    TT uncovered = ~normalized.at( chosen_t ) & care;
    for ( auto s = 0u; s < divisors.size(); ++s )
    {
      TT& tt_s = divisors.at( s );
      TT tt_not_s = ~divisors.at( s );

      // try positive s
      score1 = kitty::count_ones( uncovered & tt_s );
      if ( score1 < min_score1 )
      {
        min_score1 = score1;
        chosen_s = s * 2;
        min_score2 = num_bits;
      }
      if ( score1 == min_score1 )
      {
        score2 = kitty::count_ones( tt_not_s & care );
        if ( score2 < min_score2 )
        {
          min_score2 = score2;
          chosen_s = s * 2;
        }
      }

      // try negative s
      score1 = kitty::count_ones( uncovered & tt_not_s );
      if ( score1 < min_score1 )
      {
        min_score1 = score1;
        chosen_s = s * 2 + 1;
        min_score2 = num_bits;
      }
      if ( score1 == min_score1 )
      {
        score2 = kitty::count_ones( tt_s & care );
        if ( score2 < min_score2 )
        {
          min_score2 = score2;
          chosen_s = s * 2 + 1;
        }
      }
    }

    TT tt_chosen_s = chosen_s % 2 ? ~divisors.at( chosen_s / 2 ) : divisors.at( chosen_s / 2 );
    if ( min_score2 != 0 )
    {
      TT to_cover = care & ~tt_chosen_s;
      max_score = 0;
      for ( auto e = 0u; e < normalized.size(); ++e )
      {
        score1 = kitty::count_ones( normalized.at( e ) & to_cover );
        if ( score1 > max_score )
        {
          max_score = score1;
          chosen_e = e;
          if ( max_score == min_score2 ) // best e-child
            break;
        }
      }
    }

    if ( min_score1 != 0 ) /* expand on t-child */
    {
      TT t_care = care & tt_chosen_s;
      auto res = compute_function( t_care, depth + 1 );
      if ( res && remaining_size > 0 )
        chosen_t = *res;
      else
        return std::nullopt;
    }

    if ( max_score != min_score2 ) /* expand on e-child */
    {
      TT e_care = care & ~tt_chosen_s;
      auto res = compute_function( e_care, depth + 1 );
      if ( res && remaining_size > 0 )
        chosen_e = *res;
      else
        return std::nullopt;
    }

    assert( remaining_size >= 1 );
    --remaining_size;
    return index_list.add_mux( chosen_s + 2, chosen_t, chosen_e );
  }

private:
  uint32_t num_bits;
  uint32_t remaining_size;
  uint32_t max_depth;

  std::vector<TT> divisors;
  std::vector<TT> normalized;

  muxig_index_list index_list;

  stats& st;
}; /* mux_resyn */

} /* namespace mockturtle */