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
  \file debugging_utils.hpp
  \brief Network debugging utilities

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../algorithms/simulation.hpp"
#include "../traits.hpp"
#include "../views/topo_view.hpp"

#include <kitty/kitty.hpp>

#include <algorithm>
#include <vector>

namespace mockturtle
{

/*! \brief Prints information of all nodes in a network
 *
 * This utility function prints the following information for all
 * nodes in the network:
 * - ID
 * - Fanin signals, if any
 * - Level, if `level` is provided for the network type
 * - Whether the node is dead
 * - Reference count (fanout size)
 * - Visited marker
 * - Custom value data
 *
 * It also prints the outputs of the network.
 */
template<typename Ntk>
inline void print( Ntk const& ntk )
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  for ( uint32_t n = 0; n < ntk.size(); ++n )
  {
    std::cout << n;

    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
    {
      std::cout << std::endl;
      continue;
    }

    std::cout << " = ";

    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      std::cout << ( ntk.is_complemented( fi ) ? "~" : "" ) << ntk.get_node( fi ) << " ";
    } );
    std::cout << " ;";
    if constexpr ( has_level_v<Ntk> )
    {
      std::cout << " [level = " << int32_t( ntk.level( n ) ) << "]";
    }
    std::cout << " [dead = " << ntk.is_dead( n ) << "]";
    std::cout << " [ref = " << ntk.fanout_size( n ) << "]";
    std::cout << " [visited = " << ntk.visited( n ) << "]";
    std::cout << " [value = " << ntk.value( n ) << "]";
    std::cout << std::endl;
  }

  ntk.foreach_co( [&]( signal const& s ) {
    std::cout << "o " << ( ntk.is_complemented( s ) ? "~" : "" ) << ntk.get_node( s ) << std::endl;
  } );
}

/*! \brief Counts dead nodes in a network
 *
 * This utility function counts how many nodes in the network are
 * said to be dead (i.e., `is_dead` returns true).
 */
template<typename Ntk>
inline uint64_t count_dead_nodes( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead function" );

  uint64_t counter{ 0 };
  for ( uint64_t n = 0; n < ntk.size(); ++n )
  {
    if ( ntk.is_dead( n ) )
    {
      ++counter;
    }
  }
  return counter;
}

/*! \brief Counts dangling roots in a network
 *
 * This utility function counts how many nodes in the network have
 * a fanout size of zero. Note that it does not skip the nodes which
 * are marked as dead, which are normally skipped when using `foreach`
 * functions.
 */
template<typename Ntk>
inline uint64_t count_dangling_roots( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size function" );

  uint64_t counter{ 0 };
  for ( uint64_t n = 0; n < ntk.size(); ++n )
  {
    if ( ntk.fanout_size( n ) == 0 )
    {
      ++counter;
    }
  }
  return counter;
}

namespace detail
{

template<typename Ntk>
void count_reachable_dead_nodes_recur( Ntk const& ntk, typename Ntk::node const& n, std::vector<typename Ntk::node>& nodes )
{
  using signal = typename Ntk::signal;

  if ( ntk.current_color() == ntk.color( n ) )
  {
    return;
  }

  if ( ntk.is_dead( n ) )
  {
    if ( std::find( std::begin( nodes ), std::end( nodes ), n ) == std::end( nodes ) )
    {
      nodes.push_back( n );
    }
  }

  ntk.paint( n );
  ntk.foreach_fanin( n, [&]( signal const& fi ) {
    count_reachable_dead_nodes_recur( ntk, ntk.get_node( fi ), nodes );
  } );
}

} /* namespace detail */

/*! \brief Counts reachable dead nodes in a network
 *
 * This utility function counts how many nodes in the network are
 * said to be dead (i.e., `is_dead` returns true) and are reachable
 * from an output.
 *
 * This function requires the `paint` by the network (provided by
 * wrapping with `color_view`).
 */
template<typename Ntk>
inline uint64_t count_reachable_dead_nodes( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_color_v<Ntk>, "Ntk does not implement the color function" );
  static_assert( has_current_color_v<Ntk>, "Ntk does not implement the current_color function" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co function" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin function" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node function" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead function" );
  static_assert( has_new_color_v<Ntk>, "Ntk does not implement the new_color function" );
  static_assert( has_paint_v<Ntk>, "Ntk does not implement the paint function" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.new_color();

  std::vector<node> dead_nodes;
  ntk.foreach_co( [&]( signal const& po ) {
    detail::count_reachable_dead_nodes_recur( ntk, ntk.get_node( po ), dead_nodes );
  } );

  return dead_nodes.size();
}

namespace detail
{

template<typename Ntk>
void count_reachable_dead_nodes_from_node_recur( Ntk const& ntk, typename Ntk::node const& n, std::vector<typename Ntk::node>& nodes )
{
  using node = typename Ntk::node;

  if ( ntk.current_color() == ntk.color( n ) )
  {
    return;
  }

  if ( ntk.is_dead( n ) )
  {
    if ( std::find( std::begin( nodes ), std::end( nodes ), n ) == std::end( nodes ) )
    {
      nodes.push_back( n );
    }
  }

  ntk.paint( n );
  ntk.foreach_fanin( n, [&]( auto const& f ) {
    count_reachable_dead_nodes_from_node_recur( ntk, ntk.get_node( f ), nodes );
  } );
}

} /* namespace detail */

/*! \brief Counts dead nodes that are reachable from a given node
 *
 * This utility function counts how many nodes in the network are
 * said to be dead (i.e., `is_dead` returns true) and are reachable
 * from a given node (i.e., in the transitive fanin cone of this node).
 *
 * This function requires `paint` of the network (provided by
 * wrapping with `color_view`).
 */
template<typename Ntk>
inline uint64_t count_reachable_dead_nodes_from_node( Ntk const& ntk, typename Ntk::node const& n )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_color_v<Ntk>, "Ntk does not implement the color function" );
  static_assert( has_current_color_v<Ntk>, "Ntk does not implement the current_color function" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin function" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node function" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead function" );
  static_assert( has_new_color_v<Ntk>, "Ntk does not implement the new_color function" );
  static_assert( has_paint_v<Ntk>, "Ntk does not implement the paint function" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.new_color();

  std::vector<node> dead_nodes;
  detail::count_reachable_dead_nodes_from_node_recur( ntk, n, dead_nodes );

  return dead_nodes.size();
}

/*! \brief Counts nodes with dead fanin(s) in a network
 *
 * This utility function counts how many (not-dead) nodes in the
 * network have at least one fanin being dead.
 */
template<typename Ntk>
uint64_t count_nodes_with_dead_fanins( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin function" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node function" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead function" );
  static_assert( has_new_color_v<Ntk>, "Ntk does not implement the new_color function" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  uint64_t counter{ 0u };
  ntk.foreach_node( [&]( node const& n ) {
    ntk.foreach_fanin( n, [&]( signal const& s ) {
      if ( ntk.is_dead( ntk.get_node( s ) ) )
      {
        counter++;
        return false;
      }
      return true;
    } );
  } );

  return counter;
}

namespace detail
{

template<typename Ntk>
bool network_is_acyclic_recur( Ntk const& ntk, typename Ntk::node const& n )
{
  using signal = typename Ntk::signal;

  if ( ntk.color( n ) == ntk.current_color() )
  {
    return true;
  }

  if ( ntk.color( n ) == ntk.current_color() - 1 )
  {
    /* cycle detected at node n */
    return false;
  }

  ntk.paint( n, ntk.current_color() - 1 );

  bool result{ true };
  ntk.foreach_fanin( n, [&]( signal const& fi ) {
    if ( !network_is_acyclic_recur( ntk, ntk.get_node( fi ) ) )
    {
      result = false;
      return false;
    }
    return true;
  } );
  ntk.paint( n, ntk.current_color() );

  return result;
}

} /* namespace detail */

/*! \brief Check if a network is acyclic
 *
 * This utility function checks if the network is acyclic, i.e., there
 * is no path from a node to itself.
 *
 * This function requires `paint` of the network (provided by
 * wrapping with `color_view`).
 */
template<typename Ntk>
bool network_is_acyclic( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_ci_v<Ntk>, "Ntk does not implement the foreach_ci function" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co function" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin function" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node function" );
  static_assert( has_color_v<Ntk>, "Ntk does not implement the color function" );
  static_assert( has_current_color_v<Ntk>, "Ntk does not implement the current_color function" );
  static_assert( has_paint_v<Ntk>, "Ntk does not implement the paint function" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.new_color();
  ntk.new_color();

  ntk.paint( ntk.get_node( ntk.get_constant( false ) ) );
  ntk.foreach_ci( [&]( node const& n ) {
    ntk.paint( n );
  } );

  bool result{ true };
  ntk.foreach_co( [&]( signal const& o ) {
    if ( !detail::network_is_acyclic_recur( ntk, ntk.get_node( o ) ) )
    {
      result = false;
      return false;
    }
    return true;
  } );

  return result;
}

/*! \brief Check the level information of a network
 *
 * This utility function checks if the levels of each node in the
 * network and the depth of the network are correct.
 *
 * This function requires `level` of the network (provided by
 * wrapping with `depth_view`).
 */
template<typename Ntk>
bool check_network_levels( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size function" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant function" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci function" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node function" );
  static_assert( has_is_dead_v<Ntk>, "Ntk does not implement the is_dead function" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin function" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level function" );
  static_assert( has_depth_v<Ntk>, "Ntk does not implement the depth function" );

  using signal = typename Ntk::signal;

  uint32_t max = 0;
  for ( uint32_t i = 0u; i < ntk.size(); ++i )
  {
    if ( ntk.is_constant( i ) || ntk.is_ci( i ) || ntk.is_dead( i ) )
    {
      continue;
    }

    uint32_t max_fanin_level = 0;
    ntk.foreach_fanin( i, [&]( signal fi ) {
      if ( ntk.level( ntk.get_node( fi ) ) > max_fanin_level )
      {
        max_fanin_level = ntk.level( ntk.get_node( fi ) );
      }
    } );

    /* the node's level has not been correctly computed */
    if ( ntk.level( i ) != max_fanin_level + 1 )
    {
      return false;
    }

    if ( ntk.level( i ) > max )
    {
      max = ntk.level( i );
    }
  }

  /* the network's depth has not been correctly computed */
  if ( ntk.depth() != max )
  {
    return false;
  }

  return true;
}

/*! \brief Check the fanout information of a network
 *
 * This utility function checks if the fanouts of each node in the
 * network are correct.
 *
 * This function requires `foreach_fanout` of the network (provided by
 * wrapping with `fanout_view`).
 */
template<typename Ntk>
bool check_fanouts( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size function" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node function" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin function" );
  static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout function" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co function" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size function" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  for ( auto i = 0u; i < ntk.size(); ++i )
  {
    uint32_t fanout_counter{ 0 };

    bool fanout_error = false;
    ntk.foreach_fanout( i, [&]( node fo ) {
      ++fanout_counter;

      /* check the fanins of the fanout  */
      bool found = false;
      ntk.foreach_fanin( fo, [&]( signal fi ) {
        if ( ntk.get_node( fi ) == i )
        {
          found = true;
          return false;
        }
        return true;
      } );

      /* if errors have been detected, then terminate */
      if ( !found )
      {
        fanout_error = true;
        return false;
      }

      return true;
    } );

    /* report error */
    if ( fanout_error )
    {
      return false;
    }

    /* update the fanout counter by considering outputs */
    ntk.foreach_co( [&]( signal f ) {
      if ( ntk.get_node( f ) == i )
      {
        ++fanout_counter;
      }
    } );

    /* report error fanout_size does not match with the counter */
    if ( fanout_counter != ntk.fanout_size( i ) )
    {
      return false;
    }
  }

  return true;
}

/*! \brief Check functional equivalence between a window in a network
 * and a stand-alone window.
 *
 * This utility function checks if a window in a network, defined by
 * a set of inputs, a set of gates (internal nodes), and a set of
 * outputs, is functionally equivalent to an extracted window represented
 * as a stand-alone network.
 *
 */
template<typename Ntk, typename NtkWin>
bool check_window_equivalence( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs, std::vector<typename Ntk::signal> const& outputs, std::vector<typename Ntk::node> const& gates, NtkWin const& win_opt )
{
  NtkWin win;
  clone_subnetwork( ntk, inputs, outputs, gates, win );
  topo_view topo_win{ win_opt };
  assert( win.num_pis() == win_opt.num_pis() );
  assert( win.num_pos() == win_opt.num_pos() );

  default_simulator<kitty::dynamic_truth_table> sim( inputs.size() );
  auto const tts1 = simulate<kitty::dynamic_truth_table, NtkWin>( win, sim );
  auto const tts2 = simulate<kitty::dynamic_truth_table, topo_view<NtkWin>>( topo_win, sim );
  for ( auto i = 0u; i < tts1.size(); ++i )
  {
    if ( tts1[i] != tts2[i] )
    {
      return false;
    }
  }
  return true;
}

} /* namespace mockturtle */