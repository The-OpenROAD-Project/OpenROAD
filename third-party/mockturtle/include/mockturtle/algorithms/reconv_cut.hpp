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
  \file reconv_cut.hpp
  \brief Implements reconvergence-driven cuts (based on ABC's
  implementation in `abcReconv.c` by Alan Mishchenko).

  \author Heinz Riener
*/

#pragma once

#include "../traits.hpp"

#include <cassert>
#include <iostream>
#include <optional>

namespace mockturtle
{

/*! \brief Parameters for reconvergence-driven cut computation
 *
 * The data structure `reconvergence_driven_cut_parameters` holds configurable parameters
 * with default arguments for `reconvergence_driven_cut_impl*`.
 */
struct reconvergence_driven_cut_parameters
{
  /* Maximum number of leaves */
  uint64_t max_leaves{ 8u };

  /* Skip nodes with many fanouts */
  uint64_t max_fanouts{ 100000u };

  /* Initially reserve memory for a fixed number of nodes */
  uint64_t reserve_memory_for_nodes{ 300u };
};

/*! \brief Statistics for reconvergence-driven cut computation
 *
 * The data structure `reconvergence_driven_cut_statistics` holds data
 * collected when running a reconvergence-driven cut computation
 * algorithm.
 */
struct reconvergence_driven_cut_statistics
{
  /* Total number of calls */
  uint64_t num_calls{ 0 };

  /* Total number of leaves */
  uint64_t num_leaves{ 0 };

  /* Total number of nodes */
  uint64_t num_nodes{ 0 };
};

/*! \cond PRIVATE */
namespace detail
{

template<typename Ntk, bool compute_nodes = false, bool sort_equal_cost_by_level = true>
class reconvergence_driven_cut_impl
{
public:
  using parameters_type = reconvergence_driven_cut_parameters;
  using statistics_type = reconvergence_driven_cut_statistics;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit reconvergence_driven_cut_impl( Ntk const& ntk, reconvergence_driven_cut_parameters const& ps, reconvergence_driven_cut_statistics& st )
      : ntk( ntk ), ps( ps ), st( st )
  {
    leaves.reserve( ps.max_leaves );
    if constexpr ( compute_nodes )
    {
      nodes.reserve( ps.reserve_memory_for_nodes );
    }
  }

  std::pair<std::vector<node>, std::vector<node>> run( std::vector<node> const& pivots )
  {
    assert( pivots.size() > 0u );

    /* prepare for traversal and clean internal state */
    ntk.incr_trav_id();
    nodes.clear();
    leaves.clear();

    /* collect and mark all pivots */
    for ( const auto& pivot : pivots )
    {
      if constexpr ( compute_nodes )
      {
        nodes.emplace_back( pivot );
      }
      ntk.set_visited( pivot, ntk.trav_id() );
    }

    leaves = pivots;

    if ( leaves.size() > ps.max_leaves )
    {
      /* special case: cut already overflows at the current node because the cut size limit is very low */
      leaves.clear();
      nodes.clear();
      return { leaves, nodes };
    }

    /* compute the cut */
    while ( construct_cut() )
      ;
    assert( leaves.size() <= ps.max_leaves );

    /* update statistics */
    ++st.num_calls;
    st.num_leaves += leaves.size();
    st.num_nodes += nodes.size();

    return { leaves, nodes };
  }

private:
  bool construct_cut()
  {
    uint64_t best_cost{ std::numeric_limits<uint64_t>::max() };
    std::optional<node> best_fanin;
    uint64_t best_position;

    /* evaluate fanins of the cut */
    uint64_t position{ 0 };
    for ( const auto& l : leaves )
    {
      uint64_t const current_cost{ cost( l ) };
      if constexpr ( sort_equal_cost_by_level )
      {
        if ( best_cost > current_cost ||
             ( best_cost == current_cost && best_fanin && ntk.level( l ) > ntk.level( *best_fanin ) ) )
        {
          best_cost = current_cost;
          best_fanin = std::make_optional( l );
          best_position = position;
        }
      }
      else
      {
        if ( best_cost > current_cost )
        {
          best_cost = current_cost;
          best_fanin = std::make_optional( l );
          best_position = position;
        }
      }

      if ( best_cost == 0u )
      {
        break;
      }

      ++position;
    }

    if ( !best_fanin )
    {
      return false;
    }

    if ( leaves.size() - 1 + best_cost > ps.max_leaves )
    {
      return false;
    }

    /* remove the best node from the array */
    leaves.erase( std::begin( leaves ) + best_position );

    /* add the fanins of best to leaves and nodes */
    ntk.foreach_fanin( *best_fanin, [&]( signal const& fi ) {
      node const& n = ntk.get_node( fi );
      if ( n != 0 && ( ntk.visited( n ) != ntk.trav_id() ) )
      {
        ntk.set_visited( n, ntk.trav_id() );
        if constexpr ( compute_nodes )
        {
          nodes.emplace_back( n );
        }
        leaves.emplace_back( n );
      }
    } );

    assert( leaves.size() <= ps.max_leaves );
    return true;
  }

  uint64_t cost( node const& n ) const
  {
    /* make sure the node is in the construction zone */
    assert( ntk.visited( n ) == ntk.trav_id() );

    /* cannot expand over a constant or CI node */
    if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
    {
      return std::numeric_limits<uint64_t>::max();
    }

    /* count the number of leaves that we haven't visited */
    uint64_t cost{ 0 };
    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      cost += ntk.visited( ntk.get_node( fi ) ) != ntk.trav_id();
    } );

    /* always accept if the number of leaves does not increase */
    if ( cost < ntk.fanin_size( n ) )
    {
      return cost;
    }

    /* skip nodes with many fanouts */
    if ( ntk.fanout_size( n ) > ps.max_fanouts )
    {
      return std::numeric_limits<uint64_t>::max();
    }

    /* return the number of nodes that will be on the leaves if this node is removed */
    return cost;
  }

private:
  Ntk const& ntk;
  reconvergence_driven_cut_parameters ps;
  reconvergence_driven_cut_statistics& st;

  std::vector<node> leaves;
  std::vector<node> nodes;
}; /* reconvergence_drive_cut_impl */

template<typename Ntk, bool compute_nodes = false, bool sort_equal_cost_by_level = false>
class reconvergence_driven_cut_impl2
{
public:
  using parameters_type = reconvergence_driven_cut_parameters;
  using statistics_type = reconvergence_driven_cut_statistics;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit reconvergence_driven_cut_impl2( Ntk const& ntk, reconvergence_driven_cut_parameters const& ps, reconvergence_driven_cut_statistics& st )
      : ntk( ntk ), ps( ps ), st( st )
  {
  }

  std::pair<std::vector<node>, std::vector<node>> run( std::vector<node> const& pivots )
  {
    assert( pivots.size() > 0u );

    /* prepare for traversal and clean internal state */
    ntk.incr_trav_id();
    nodes.clear();
    leaves.clear();
    assert( nodes.empty() );

    for ( const auto& pivot : pivots )
    {
      ntk.set_visited( pivot, ntk.trav_id() );
    }

    while ( construct_cut() )
      ;
    assert( leaves.size() <= ps.max_leaves );

    /* update statistics */
    ++st.num_calls;
    st.num_leaves += leaves.size();
    st.num_nodes += nodes.size();

    return { leaves, nodes };
  }

  bool construct_cut()
  {
    assert( leaves.size() <= ps.max_leaves && "cut-size overflow" );
    std::stable_sort( std::begin( leaves ), std::end( leaves ),
               [this]( node const& a, node const& b ) {
                 return cost( a ) < cost( b );
               } );

    /* find the first non-pi node to extend the cut (because the vector is sorted, this non-pi is cost-minimal) */
    auto const it = std::find_if( std::begin( leaves ), std::end( leaves ),
                                  [&]( node const& n ) {
                                    return !ntk.is_ci( n );
                                  } );
    if ( std::end( leaves ) == it )
    {
      /* if all nodes are pis, then the cut cannot be extended */
      return false;
    }

    /* the cost is identical to the number of nodes added to `leaves` if *it is used to expand leaves */
    int64_t const c = cost( *it );
    if ( leaves.size() + c > ps.max_leaves )
    {
      /* if the expansion exceeds the cut_size, then the cut cannot be extended */
      return false;
    }

    /* otherwise expand the cut with the children of *it and mark *it visited */
    node const n = *it;
    leaves.erase( it );
    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      node const& child = ntk.get_node( fi );
      if ( !ntk.is_constant( child ) && std::find( std::begin( leaves ), std::end( leaves ), child ) == std::end( leaves ) && ntk.visited( child ) != ntk.trav_id() )
      {
        leaves.emplace_back( child );
        ntk.set_visited( child, ntk.trav_id() );
      }
    } );

    assert( leaves.size() <= ps.max_leaves );
    return true;
  }

  /* counts the number of non-constant leaves */
  int64_t cost( node const& n ) const
  {
    int32_t current_cost = -1;
    ntk.foreach_fanin( n, [&]( signal const& s ) {
      auto const& child = ntk.get_node( s );
      if ( !ntk.is_constant( child ) )
      {
        ++current_cost;
      }
    } );
    return current_cost;
  }

private:
  Ntk const& ntk;
  reconvergence_driven_cut_parameters ps;
  reconvergence_driven_cut_statistics& st;

  std::vector<node> leaves;
  std::vector<node> nodes;
}; /* reconvergence_drive_cut_impl2 */

template<typename Ntk, typename Impl>
std::pair<std::vector<node<Ntk>>, std::vector<node<Ntk>>> reconvergence_driven_cut( Ntk const& ntk, std::vector<node<Ntk>> const& pivots, reconvergence_driven_cut_parameters const& ps, reconvergence_driven_cut_statistics& st )
{
  return Impl( ntk, ps, st ).run( pivots );
}

} // namespace detail
/*! \endcond */

/*! \brief Reconvergence-driven cut towards inputs.
 *
 * This class implements a generation algorithm for
 * reconvergence-driven cuts.  The cut grows towards the primary
 * inputs starting from a set of pivot nodes.
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `get_node`
 * - `visited`
 * - `has_visited`
 * - `foreach_fanin`
 *
 */
template<typename Ntk, bool compute_nodes = false, bool sort_equal_cost_by_level = true>
std::pair<std::vector<node<Ntk>>, std::vector<node<Ntk>>> reconvergence_driven_cut( Ntk const& ntk, std::vector<node<Ntk>> const& pivots, reconvergence_driven_cut_parameters const& ps = {}, reconvergence_driven_cut_statistics* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the has_visited method" );
  static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  if constexpr ( sort_equal_cost_by_level )
  {
    static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  }

  using Impl = detail::reconvergence_driven_cut_impl<Ntk, compute_nodes, sort_equal_cost_by_level>;

  reconvergence_driven_cut_statistics st;
  auto const result = detail::reconvergence_driven_cut<Ntk, Impl>( ntk, pivots, ps, st );
  if ( pst )
  {
    *pst = st;
  }
  return result;
}

/*! \brief Reconvergence-driven cut towards inputs.
 *
 * This class implements a generation algorithm for
 * reconvergence-driven cuts.  The cut grows towards the primary
 * inputs starting from a single pivot node.
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `get_node`
 * - `visited`
 * - `has_visited`
 * - `foreach_fanin`
 *
 */
template<typename Ntk, bool compute_nodes = false, bool sort_equal_cost_by_level = true>
std::pair<std::vector<node<Ntk>>, std::vector<node<Ntk>>> reconvergence_driven_cut( Ntk const& ntk, node<Ntk> const& pivot, reconvergence_driven_cut_parameters const& ps = {}, reconvergence_driven_cut_statistics* pst = nullptr )
{
  return reconvergence_driven_cut<Ntk, compute_nodes, sort_equal_cost_by_level>( ntk, std::vector<node<Ntk>>{ pivot }, ps, pst );
}

/*! \brief Reconvergence-driven cut towards inputs.
 *
 * This class implements a generation algorithm for
 * reconvergence-driven cuts.  The cut grows towards the primary
 * inputs starting from a single pivot signal.
 *
 * **Required network functions:**
 * - `is_constant`
 * - `is_pi`
 * - `get_node`
 * - `visited`
 * - `has_visited`
 * - `foreach_fanin`
 *
 */
template<typename Ntk, bool compute_nodes = false, bool sort_equal_cost_by_level = true>
std::pair<std::vector<node<Ntk>>, std::vector<node<Ntk>>> reconvergence_driven_cut( Ntk const& ntk, signal<Ntk> const& pivot, reconvergence_driven_cut_parameters const& ps = {}, reconvergence_driven_cut_statistics* pst = nullptr )
{
  return reconvergence_driven_cut<Ntk, compute_nodes, sort_equal_cost_by_level>( ntk, std::vector<node<Ntk>>{ ntk.get_node( pivot ) }, ps, pst );
}

} // namespace mockturtle