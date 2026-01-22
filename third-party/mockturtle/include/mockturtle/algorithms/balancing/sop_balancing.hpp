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
  \file sop_balancing.hpp
  \brief SOP-based balancing engine for `balancing` algorithm

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <queue>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/isop.hpp>
#include <kitty/operations.hpp>

#include "../../traits.hpp"
#include "../../utils/stopwatch.hpp"
#include "../balancing.hpp"
#include "utils.hpp"

namespace mockturtle
{

/*! \brief SOP rebalancing function
 *
 * This class can be used together with the generic `balancing` function.  It
 * converts each cut function into an SOP and then performs weight-oriented
 * tree balancing on the AND terms and the outer OR function.
 */
template<class Ntk>
struct sop_rebalancing
{
  void operator()( Ntk& dest, kitty::dynamic_truth_table const& function, std::vector<arrival_time_pair<Ntk>> const& inputs, uint32_t best_level, uint32_t best_cost, rebalancing_function_callback_t<Ntk> const& callback ) const
  {
    bool inverted = false;
    auto [and_terms, num_and_gates] = create_function( dest, function, inputs, inverted );
    const auto num_gates = num_and_gates + ( and_terms.empty() ? 0u : static_cast<uint32_t>( and_terms.size() ) - 1u );
    arrival_time_pair<Ntk> cand = balanced_tree( dest, and_terms, false );

    if ( cand.level < best_level || ( cand.level == best_level && num_gates < best_cost ) )
    {
      cand.f = cand.f ^ inverted;
      callback( cand, num_gates );
    }
  }

private:
  std::pair<arrival_time_queue<Ntk>, uint32_t> create_function( Ntk& dest, kitty::dynamic_truth_table const& func, std::vector<arrival_time_pair<Ntk>> const& arrival_times, bool& inverted ) const
  {
    const auto sop = create_sop_form( func, inverted );

    stopwatch<> t_tree( time_tree_balancing );
    arrival_time_queue<Ntk> and_terms;
    uint32_t num_and_gates{};
    for ( auto const& cube : sop )
    {
      arrival_time_queue<Ntk> product_queue;
      for ( auto i = 0u; i < func.num_vars(); ++i )
      {
        if ( cube.get_mask( i ) )
        {
          const auto [f, l] = arrival_times[i];
          product_queue.push( { cube.get_bit( i ) ? f : dest.create_not( f ), l } );
        }
      }
      if ( !product_queue.empty() )
      {
        num_and_gates += static_cast<uint32_t>( product_queue.size() ) - 1u;
      }
      and_terms.push( balanced_tree( dest, product_queue ) );
    }
    return { and_terms, num_and_gates };
  }

  arrival_time_pair<Ntk> balanced_tree( Ntk& dest, arrival_time_queue<Ntk>& queue, bool _and = true ) const
  {
    if ( queue.empty() )
    {
      return { dest.get_constant( true ), 0u };
    }

    while ( queue.size() > 1u )
    {
      auto [s1, l1] = queue.top();
      queue.pop();
      auto [s2, l2] = queue.top();
      queue.pop();
      const auto s = _and ? dest.create_and( s1, s2 ) : dest.create_or( s1, s2 );
      const auto l = std::max( l1, l2 ) + 1;
      queue.push( { s, l } );
    }
    return queue.top();
  }

  std::vector<kitty::cube> create_sop_form( kitty::dynamic_truth_table const& func, bool& inverted ) const
  {
    stopwatch<> t( time_sop );
    inverted = false;

    if ( auto it = sop_hash_.find( func ); it != sop_hash_.end() )
    {
      sop_cache_hits++;
      return it->second;
    }

    if ( both_phases_ )
    {
      if ( auto it = sop_hash_.find( ~func ); it != sop_hash_.end() )
      {
        inverted = true;
        sop_cache_hits++;
        return it->second;
      }
    }

    sop_cache_misses++;
    std::vector<kitty::cube> sop = kitty::isop( func );

    if ( both_phases_ )
    {
      std::vector<kitty::cube> n_sop = kitty::isop( ~func );

      if ( n_sop.size() < sop.size() )
      {
        inverted = true;
        return sop_hash_[~func] = n_sop;
      }
      else if ( n_sop.size() == sop.size() )
      {
        /* compute literal cost */
        uint32_t lit = 0, n_lit = 0;
        for ( auto const& c : sop )
        {
          lit += c.num_literals();
        }
        for ( auto const& c : n_sop )
        {
          n_lit += c.num_literals();
        }

        if ( n_lit < lit )
        {
          inverted = true;
          return sop_hash_[~func] = n_sop;
        }
      }
    }

    return sop_hash_[func] = sop;
  }

private:
  mutable std::unordered_map<kitty::dynamic_truth_table, std::vector<kitty::cube>, kitty::hash<kitty::dynamic_truth_table>> sop_hash_;

public:
  bool both_phases_{ false };

public:
  mutable uint32_t sop_cache_hits{};
  mutable uint32_t sop_cache_misses{};

  mutable stopwatch<>::duration time_sop{};
  mutable stopwatch<>::duration time_tree_balancing{};
};

} // namespace mockturtle