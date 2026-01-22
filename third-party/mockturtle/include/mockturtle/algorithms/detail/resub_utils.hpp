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
  \file resub_utils.hpp
  \brief Utility classes for the resubstitution framework

  class `node_mffc_inside`, class `window_simulator` (originally `simulator`),
  and class `default_resub_functor` moved from resubstitution.hpp

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <algorithm>
#include <iostream>
#include <optional>
#include <vector>

#include <kitty/constructors.hpp>

namespace mockturtle::experimental::detail
{

struct divisor_collector_params
{
  /*! \brief Maximum number of nodes to be collected in the transitive fanin cone. */
  uint32_t max_num_tfi{ std::numeric_limits<uint32_t>::max() };

  /*! \brief Maximum number of nodes to collect (in total). */
  uint32_t max_num_collect{ std::numeric_limits<uint32_t>::max() };

  /*! \brief Maximum fanout size when considering a node in the "wings". */
  uint32_t max_fanouts{ std::numeric_limits<uint32_t>::max() };

  /*! \brief Maximum level when considering a node in the "wings".
   *
   * The network should be wrapped with `depth_view` for this parameter
   * to be in effect.
   */
  uint32_t max_level{ std::numeric_limits<uint32_t>::max() };
};

/*! \brief Implements helper functions for collecting divisors/supported nodes. */
template<typename Ntk>
class divisor_collector
{
public:
  using node = typename Ntk::node;

  divisor_collector( Ntk const& ntk, divisor_collector_params ps = {} )
      : ntk( ntk ), ps( ps )
  {
    static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method (please wrap with fanout_view)" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_trav_id_v<Ntk>, "Ntk does not implement the trav_id method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    assert( ps.max_num_collect >= ps.max_num_tfi );
  }

  void set_max_level( uint32_t max_level )
  {
    ps.max_level = max_level;
  }

  /*! \brief Collect nodes in the transitive fanin cone of root until leaves in a topological order.
   *
   * `root` itself is included and will be the last element.
   * Constant node(s) is not collected.
   * Leaves are not collected.
   *
   * \param root The root node
   * \param leaves The leaf nodes forming a cut supporting the root node
   * \param tfi The container where TFI nodes are collected
   */
  void collect_tfi( node const& root, std::vector<node> const& leaves, std::vector<node>& tfi )
  {
    ntk.incr_trav_id();
    for ( const auto& l : leaves )
    {
      ntk.set_visited( l, ntk.trav_id() );
    }
    collect_tfi_rec( root, tfi );
  }

  /*! \brief Collect nodes in the TFI (BFS) and wings.
   *
   * Constant node(s) is not collected.
   * Any node in the TFO of root is not collected.
   * Collects TFI nodes with breadth-first search until PIs (unbounded)
   * or until the limit on `max_num_tfi` exceeds, and then collects "wings"
   * nodes until the limit on `max_num_collect` exceeds.
   * Collected nodes are in a topological order, with `root` itself being the last element.
   *
   * \param root The root node
   * \param collected The container where TFI and "wings" nodes are collected
   */
  void collect_tfi_and_wings( node const& root, std::vector<node>& collected )
  {
    collect_tfi_bfs( root, collected );
    std::reverse( collected.begin(), collected.end() ); /* now in topo order */
    collected.pop_back();                               /* remove `root` */

    /* note: we cannot use range-based loop here because we push to the vector in the loop */
    for ( auto i = 0u; i < collected.size(); ++i )
    {
      if ( !collect_wings( root, collected.at( i ), collected ) )
      {
        break;
      }
    }

    collected.emplace_back( root );
    return;
  }

  /*! \brief Collect all nodes that are supported by the leaves until the root (TFI + wings).
   *
   * Constant node(s) is not collected.
   * Leaves are not collected.
   * Any node in the TFO of root is not collected.
   * Collected nodes are in a topological order, with `root` itself being the last element.
   *
   * \param root The root node
   * \param leaves The leaf nodes forming a cut supporting the root node
   * \param supported The container where supported nodes are collected
   */
  void collect_supported_nodes( node const& root, std::vector<node> const& leaves, std::vector<node>& supported )
  {
    /* collect TFI nodes until (excluding) leaves */
    collect_tfi( root, leaves, supported );
    supported.pop_back(); /* remove `root` */

    if ( supported.size() > ps.max_num_tfi )
    {
      return;
    }

    /* collect "wings" */
    for ( auto const& l : leaves )
    {
      if ( !collect_wings( root, l, supported ) )
      {
        supported.emplace_back( root );
        return;
      }
    }

    /* note: we cannot use range-based loop here because we push to the vector in the loop */
    for ( auto i = 0u; i < supported.size(); ++i )
    {
      if ( !collect_wings( root, supported.at( i ), supported ) )
      {
        break;
      }
    }

    supported.emplace_back( root );
    return;
  }

private:
  void collect_tfi_rec( node const& n, std::vector<node>& tfi )
  {
    /* collect until leaves and skip visited nodes */
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      return;
    }
    ntk.set_visited( n, ntk.trav_id() );

    /* collect in topological order -- lower nodes first */
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      collect_tfi_rec( ntk.get_node( f ), tfi );
    } );

    if ( !ntk.is_constant( n ) )
    {
      tfi.emplace_back( n );
    }
  }

  /*! \brief Collect nodes in the transitive fanin cone of root with breadth-first search.
   *
   * `root` itself is included and will be the first element.
   * Constant node(s) is not collected.
   * Collects until PIs (unbounded) or until the limit on `max_num_tfi` exceeds.
   * Collected nodes are NOT in a topological order.
   *
   * \param root The root node
   * \param tfi The container where TFI nodes are collected
   */
  void collect_tfi_bfs( node const& root, std::vector<node>& tfi )
  {
    ntk.incr_trav_id();
    assert( tfi.size() == 0 );
    tfi.reserve( ps.max_num_tfi );
    tfi.emplace_back( root );
    ntk.set_visited( root, ntk.trav_id() );
    ntk.set_visited( ntk.get_node( ntk.get_constant( false ) ), ntk.trav_id() ); /* don't collect constant node */
    uint32_t i{ 0 };
    while ( i < tfi.size() && tfi.size() < ps.max_num_tfi )
    {
      node const& n = tfi.at( i++ );
      ntk.foreach_fanin( n, [&]( const auto& f ) {
        node const& ni = ntk.get_node( f );
        if ( ntk.visited( ni ) != ntk.trav_id() )
        {
          tfi.emplace_back( ni );
          ntk.set_visited( ni, ntk.trav_id() );
        }
      } );
    }
  }

  /*\return Whether to continue collecting */
  bool collect_wings( node const& root, node const& n, std::vector<node>& supported )
  {
    if ( ntk.fanout_size( n ) > ps.max_fanouts )
    {
      return true;
    }

    /* if the fanout has all fanins in the set, add it */
    ntk.foreach_fanout( n, [&]( node const& p ) {
      if ( ntk.visited( p ) == ntk.trav_id() )
      {
        return true; /* next fanout */
      }

      if constexpr ( has_level_v<Ntk> )
      {
        if ( ntk.level( p ) > ps.max_level )
        {
          return true; /* next fanout */
        }
      }

      bool all_fanins_visited = true;
      ntk.foreach_fanin( p, [&]( const auto& g ) {
        if ( ntk.visited( ntk.get_node( g ) ) != ntk.trav_id() )
        {
          all_fanins_visited = false;
          return false; /* terminate fanin-loop */
        }
        return true; /* next fanin */
      } );

      if ( !all_fanins_visited )
      {
        return true; /* next fanout */
      }

      bool has_root_as_child = false;
      ntk.foreach_fanin( p, [&]( const auto& g ) {
        if ( ntk.get_node( g ) == root )
        {
          has_root_as_child = true;
          return false; /* terminate fanin-loop */
        }
        return true; /* next fanin */
      } );

      if ( has_root_as_child )
      {
        return true; /* next fanout */
      }

      supported.emplace_back( p );
      ntk.set_visited( p, ntk.trav_id() );

      /* quit fanout-loop if there are too many nodes collected */
      return supported.size() < ps.max_num_collect;
    } );
    return supported.size() < ps.max_num_collect;
  }

private:
  Ntk const& ntk;
  divisor_collector_params ps;
}; /* divisor_collector */

template<typename Ntk, typename TT>
class window_simulator
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit window_simulator( Ntk const& ntk, std::vector<TT>& tts, uint32_t num_pis )
      : ntk( ntk ), tts( tts ), node_to_id( ntk ), num_pis( num_pis )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_fanin_size_v<Ntk>, "Ntk does not implement the fanin_size method" );
    static_assert( has_compute_v<Ntk, TT>, "Ntk does not implement the compute method for TT" );
    assert( ntk.get_node( ntk.get_constant( false ) ) == ntk.get_node( ntk.get_constant( true ) ) && "network types whose constant nodes are different are not supported" );

    tts.resize( num_pis + 1 );
    auto tt = kitty::create<TT>( num_pis );
    tts[0] = tt;
    node_to_id[ntk.get_constant( false )] = 0;

    for ( auto i = 0u; i < num_pis; ++i )
    {
      kitty::create_nth_var( tt, i );
      tts[i + 1] = tt;
    }
  }

  /*! \brief Simulates a window in a network.
   *
   * Every node in `nodes` must have all of its fanins either in `leaves`, or in
   * `nodes` and precedes it (i.e., supported and in a topological order).
   *
   * After simulation, `tts` contains:
   * - `tts[0]` is the constant-zero truth table
   * - `tts[1]` to `tts[num_pis]` are the projection functions (primary inputs)
   * - `tts[num_pis + 1 + i]` is the truth table for the node `nodes[i]`
   */
  std::vector<TT>& simulate( std::vector<node> const& leaves, std::vector<node> const& nodes )
  {
    node_to_id.resize();
    assert( leaves.size() <= num_pis );
    for ( auto i = 0u; i < leaves.size(); ++i )
    {
      node_to_id[leaves[i]] = i + 1;
    }

    tts.resize( num_pis + 1 );
    for ( auto const& n : nodes )
    {
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
        assert( node_to_id.has( f ) && node_to_id[f] < tts.size() && "some node in `nodes` has a fanin outside of `nodes` and `leaves`, or `nodes` is not in a topological order" );
        fanin_values[i] = tts[node_to_id[f]];
      } );
      node_to_id[n] = tts.size();
      tts.emplace_back( ntk.compute( n, std::begin( fanin_values ), std::begin( fanin_values ) + ntk.fanin_size( n ) ) );
    }

    assert( tts.size() == num_pis + 1 + nodes.size() );
    return tts;
  }

private:
  Ntk const& ntk;
  std::vector<TT>& tts;
  incomplete_node_map<uint32_t, Ntk> node_to_id;
  std::array<TT, Ntk::max_fanin_size> fanin_values;
  uint32_t num_pis;
}; /* window_simulator */

} /* namespace mockturtle::experimental::detail */

namespace mockturtle::detail
{

/* based on abcRefs.c */
template<typename Ntk>
class node_mffc_inside
{
public:
  using node = typename Ntk::node;

public:
  explicit node_mffc_inside( Ntk const& ntk )
      : ntk( ntk )
  {
    static_assert( has_incr_fanout_size_v<Ntk>, "Ntk does not implement the incr_fanout_size method" );
    static_assert( has_decr_fanout_size_v<Ntk>, "Ntk does not implement the decr_fanout_size method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_trav_id_v<Ntk>, "Ntk does not implement the trav_id method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  }

  template<typename Fn>
  int32_t call_on_mffc_and_count( node const& n, std::vector<node> const& leaves, Fn&& fn )
  {
    /* increment the fanout counters for the leaves */
    for ( const auto& l : leaves )
      ntk.incr_fanout_size( l );

    /* dereference the node */
    auto count1 = node_deref_rec( n );

    /* call `fn` on MFFC nodes */
    ntk.incr_trav_id();
    node_mffc_cone_rec( n, true, fn );

    /* reference it back */
    auto count2 = node_ref_rec( n );
    (void)count2;
    assert( count1 == count2 );

    for ( const auto& l : leaves )
      ntk.decr_fanout_size( l );

    return count1;
  }

  int32_t run( node const& n, std::vector<node> const& leaves, std::vector<node>& inside )
  {
    inside.clear();
    return call_on_mffc_and_count( n, leaves, [&inside]( node const& m ) { inside.emplace_back( m ); } );
  }

private:
  /* ! \brief Dereference the node's MFFC */
  int32_t node_deref_rec( node const& n )
  {
    if ( ntk.is_pi( n ) )
      return 0;

    int32_t counter = 1;
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk.get_node( f );

      ntk.decr_fanout_size( p );
      if ( ntk.fanout_size( p ) == 0 )
      {
        counter += node_deref_rec( p );
      }
    } );

    return counter;
  }

  /* ! \brief Reference the node's MFFC */
  int32_t node_ref_rec( node const& n )
  {
    if ( ntk.is_pi( n ) )
      return 0;

    int32_t counter = 1;
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk.get_node( f );

      auto v = ntk.fanout_size( p );
      ntk.incr_fanout_size( p );
      if ( v == 0 )
      {
        counter += node_ref_rec( p );
      }
    } );

    return counter;
  }

  template<typename Fn>
  void node_mffc_cone_rec( node const& n, bool top_most, Fn&& fn )
  {
    /* skip visited nodes */
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      return;
    }
    ntk.set_visited( n, ntk.trav_id() );

    if ( !top_most && ( ntk.is_pi( n ) || ntk.fanout_size( n ) > 0 ) )
    {
      return;
    }

    /* recurse on children */
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      node_mffc_cone_rec( ntk.get_node( f ), false, fn );
    } );

    /* collect the internal nodes */
    fn( n );
  }

private:
  Ntk const& ntk;
}; /* node_mffc_inside */

template<typename Ntk, typename TT>
class window_simulator
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using truthtable_t = TT;

  explicit window_simulator( Ntk const& ntk, uint32_t num_divisors, uint32_t max_pis )
      : ntk( ntk ), num_divisors( num_divisors ), tts( num_divisors + 1 ), node_to_index( ntk.size(), 0u ), phase( ntk.size(), false )
  {
    auto tt = kitty::create<truthtable_t>( max_pis );
    tts[0] = tt;

    for ( auto i = 0u; i < tt.num_vars(); ++i )
    {
      kitty::create_nth_var( tt, i );
      tts[i + 1] = tt;
    }
  }

  void resize()
  {
    if ( ntk.size() > node_to_index.size() )
    {
      node_to_index.resize( ntk.size(), 0u );
    }
    if ( ntk.size() > phase.size() )
    {
      phase.resize( ntk.size(), false );
    }
  }

  void assign( node const& n, uint32_t index )
  {
    assert( n < node_to_index.size() );
    assert( index < num_divisors + 1 );
    node_to_index[n] = index;
  }

  truthtable_t get_tt( signal const& s ) const
  {
    auto const tt = tts.at( node_to_index.at( ntk.get_node( s ) ) );
    return ntk.is_complemented( s ) ? ~tt : tt;
  }

  void set_tt( uint32_t index, truthtable_t const& tt )
  {
    tts[index] = tt;
  }

  void normalize( std::vector<node> const& nodes )
  {
    for ( const auto& n : nodes )
    {
      assert( n < phase.size() );
      assert( n < node_to_index.size() );

      if ( n == 0 )
      {
        return;
      }

      auto& tt = tts[node_to_index.at( n )];
      if ( kitty::get_bit( tt, 0 ) )
      {
        tt = ~tt;
        phase[n] = true;
      }
      else
      {
        phase[n] = false;
      }
    }
  }

  bool get_phase( node const& n ) const
  {
    assert( n < phase.size() );
    return phase.at( n );
  }

private:
  Ntk const& ntk;
  uint32_t num_divisors;

  std::vector<truthtable_t> tts;
  std::vector<uint32_t> node_to_index;
  std::vector<bool> phase;
}; /* window_simulator */

struct default_resub_functor_stats
{
  /*! \brief Accumulated runtime for const-resub */
  stopwatch<>::duration time_resubC{ 0 };

  /*! \brief Accumulated runtime for zero-resub */
  stopwatch<>::duration time_resub0{ 0 };

  /*! \brief Number of accepted constant resubsitutions */
  uint32_t num_const_accepts{ 0 };

  /*! \brief Number of accepted zero resubsitutions */
  uint32_t num_div0_accepts{ 0 };

  void report() const
  {
    std::cout << "[i] kernel: default_resub_functor\n";
    std::cout << fmt::format( "[i]     constant-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_const_accepts, to_seconds( time_resubC ) );
    std::cout << fmt::format( "[i]            0-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div0_accepts, to_seconds( time_resub0 ) );
    std::cout << fmt::format( "[i]            total   {:6d}\n",
                              ( num_const_accepts + num_div0_accepts ) );
  }
};

/*! \brief A window-based resub functor which is basically doing functional reduction (fraig). */
template<typename Ntk, typename Simulator, typename TT>
class default_resub_functor
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using stats = default_resub_functor_stats;

  explicit default_resub_functor( Ntk const& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, default_resub_functor_stats& st )
      : ntk( ntk ), sim( sim ), divs( divs ), num_divs( num_divs ), st( st )
  {
  }

  std::optional<signal> operator()( node const& root, TT care, uint32_t required, uint32_t max_inserts, uint32_t num_mffc, uint32_t& last_gain ) const
  {
    /* The default resubstitution functor does not insert any gates
       and consequently does not use the argument `max_inserts`. Other
       functors, however, make use of this argument. */
    (void)care;
    (void)max_inserts;
    assert( kitty::is_const0( ~care ) );

    /* consider constants */
    auto g = call_with_stopwatch( st.time_resubC, [&]() {
      return resub_const( root, required );
    } );
    if ( g )
    {
      ++st.num_const_accepts;
      last_gain = num_mffc;
      return g; /* accepted resub */
    }

    /* consider equal nodes */
    g = call_with_stopwatch( st.time_resub0, [&]() {
      return resub_div0( root, required );
    } );
    if ( g )
    {
      ++st.num_div0_accepts;
      last_gain = num_mffc;
      return g; /* accepted resub */
    }

    return std::nullopt;
  }

private:
  std::optional<signal> resub_const( node const& root, uint32_t required ) const
  {
    (void)required;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    if ( tt == sim.get_tt( ntk.get_constant( false ) ) )
    {
      return sim.get_phase( root ) ? ntk.get_constant( true ) : ntk.get_constant( false );
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div0( node const& root, uint32_t required ) const
  {
    (void)required;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    for ( const auto& d : divs )
    {
      if ( root == d )
      {
        break;
      }

      if ( tt != sim.get_tt( ntk.make_signal( d ) ) )
      {
        continue; /* next */
      }

      return ( sim.get_phase( d ) ^ sim.get_phase( root ) ) ? !ntk.make_signal( d ) : ntk.make_signal( d );
    }

    return std::nullopt;
  }

private:
  Ntk const& ntk;
  Simulator const& sim;
  std::vector<node> const& divs;
  uint32_t num_divs;
  stats& st;
}; /* default_resub_functor */

template<typename Ntk, typename node>
void update_node_level_once( Ntk& ntk, node const& n, bool first_node )
{
  uint32_t curr_level = ntk.level( n );

  uint32_t max_level = 0;
  ntk.foreach_fanin( n, [&]( const auto& f ) {
    auto const p = ntk.get_node( f );
    auto const fanin_level = ntk.level( p );
    if ( fanin_level > max_level )
    {
      max_level = fanin_level;
    }
  } );
  ++max_level;

  if ( curr_level != max_level )
  {
    ntk.set_level( n, max_level );

    /* update only one more level */
    if ( first_node )
    {
      ntk.foreach_fanout( n, [&]( const auto& p ) {
        update_node_level_once( ntk, p, false );
      } );
    }
  }
}

/*! \brief Register an `on_modified` event that lazily updates node levels.
 *
 * This is a trick learnt from ABC's implementation and is used in
 * enumeration-based resubstitution algorithms. It only updates the level of
 * the modified node and its fanout nodes. The update is not propagated to
 * the fanouts' fanouts, thus being fast but inaccurate.
 *
 * This method can be called in the constructor of an algorithm's implementation
 * class. Note that its return value should be stored and
 * `release_lazy_level_update_events` should then be called in the destructor
 * of the class.
 */
template<class Ntk, typename event_t = std::shared_ptr<typename network_events<Ntk>::modified_event_type>>
event_t register_lazy_level_update_events( Ntk& ntk )
{
  static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not have fanout interface." );
  using node = typename Ntk::node;

  auto const update_level_of_existing_node = [&]( node const& n, const auto& old_children ) {
    (void)old_children;
    ntk.resize_levels();
    update_node_level_once( ntk, n, true );
  };

  return ntk.events().register_modified_event( update_level_of_existing_node );
}

template<class Ntk, typename event_t = std::shared_ptr<typename network_events<Ntk>::modified_event_type>>
void release_lazy_level_update_events( Ntk& ntk, event_t& event )
{
  ntk.events().release_modified_event( event );
}

} /* namespace mockturtle::detail */