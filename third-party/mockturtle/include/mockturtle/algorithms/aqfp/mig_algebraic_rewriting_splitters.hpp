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
  \file mig_algebraic_rewriting_splitters.hpp
  \brief MIG algebraric rewriting with fanout size limitation

  \author Mathias Soeken
  \author Eleonora Testa
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../utils/stopwatch.hpp"
#include "../../views/fanout_view.hpp"
#include "../../views/topo_view.hpp"
#include "../mig_algebraic_rewriting.hpp"

#include <iostream>
#include <optional>

namespace mockturtle
{

namespace detail
{

template<class Ntk>
class mig_algebraic_depth_rewriting_splitter_impl
{
public:
  mig_algebraic_depth_rewriting_splitter_impl( Ntk& ntk, mig_algebraic_depth_rewriting_params const& ps, mig_algebraic_depth_rewriting_stats& st )
      : ntk( ntk ), ps( ps ), st( st )
  {
  }

  void run()
  {
    stopwatch t( st.time_total );

    switch ( ps.strategy )
    {
    case mig_algebraic_depth_rewriting_params::dfs:
      run_dfs();
      break;
    case mig_algebraic_depth_rewriting_params::selective:
      run_selective();
      break;
    case mig_algebraic_depth_rewriting_params::aggressive:
      run_aggressive();
      break;
    }
  }

private:
  void run_dfs()
  {
    ntk.foreach_po( [this]( auto po ) {
      const auto driver = ntk.get_node( po );
      if ( ntk.level( driver ) < ntk.depth() )
        return;
      topo_view topo{ ntk, po };
      topo.foreach_node( [this]( auto n ) {
        mark_critical_paths();
        reduce_depth( n );
        return true;
      } );
    } );
  }

  void run_selective()
  {
    uint32_t counter{ 0 };
    while ( true )
    {
      mark_critical_paths();

      topo_view topo{ ntk };
      topo.foreach_node( [this, &counter]( auto n ) {
        if ( ntk.fanout_size( n ) == 0 || ntk.value( n ) == 0 )
          return;
        if ( reduce_depth( n ) )
        {
          mark_critical_paths();
        }
        else
        {
          ++counter;
        }
      } );

      if ( counter > ntk.size() )
        break;
    }
  }

  void run_aggressive()
  {
    uint32_t counter{ 0 }, init_size{ ntk.size() };
    while ( true )
    {
      topo_view topo{ ntk };
      topo.foreach_node( [this, &counter]( auto n ) {
        if ( ntk.fanout_size( n ) == 0 )
          return;

        if ( !reduce_depth( n ) )
        {
          ++counter;
        }
      } );

      if ( ntk.size() > ps.overhead * init_size )
        break;
      if ( counter > ntk.size() )
        break;
    }
  }

private:
  bool reduce_depth( node<Ntk> const& n )
  {

    if ( !ntk.is_maj( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_maj( ntk.get_node( ocs[2] ) ) )
      return false;

    if ( ntk.fanout_size( ntk.get_node( ocs[2] ) ) != 1 )
      return false;

    /* depth of last child must be (significantly) higher than depth of second child */
    if ( ntk.level( ntk.get_node( ocs[2] ) ) <= ntk.level( ntk.get_node( ocs[1] ) ) + 1 )
      return false;

    /* child must have single fanout, if no area overhead is allowed */
    if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( ocs[2] ) ) != 1 )
      return false;

    /* get children of last child */
    auto ocs2 = ordered_children( ntk.get_node( ocs[2] ) );

    /* depth of last grand-child must be higher than depth of second grand-child */
    if ( ntk.level( ntk.get_node( ocs2[2] ) ) == ntk.level( ntk.get_node( ocs2[1] ) ) )
      return false;

    /* propagate inverter if necessary */
    if ( ntk.is_complemented( ocs[2] ) )
    {
      ocs2[0] = !ocs2[0];
      ocs2[1] = !ocs2[1];
      ocs2[2] = !ocs2[2];
    }

    auto index = 0;

    if ( auto cand = associativity_candidate( ocs[0], ocs[1], ocs2[0], ocs2[1], ocs2[2] ); cand )
    {
      const auto& [x, y, z, u, assoc] = *cand;

      signal<Ntk> third;
      if ( assoc )
      {
        third = u;
      }
      else
        third = x;

      auto h = ntk.create_maj( x, y, u );
      auto opt = ntk.create_maj( z, third, h );

      if ( ntk.fanout_size( ntk.get_node( opt ) ) < 1 ) // opt does not exists
      {
        if ( ( ntk.fanout_size( ntk.get_node( h ) ) == 5 ) || ( ntk.fanout_size( ntk.get_node( h ) ) >= 17 ) || ( ntk.fanout_size( ntk.get_node( h ) ) == 2 ) ) // it would mean the depth is actually increased
        {
          ntk.take_out_node( ntk.get_node( opt ) );
          return true;
        }
      }

      ntk.substitute_node( n, opt );
      ntk.update_levels();
      return true;
    }

    /* distributivity */
    if ( ps.allow_area_increase )
    {
      auto s1 = ntk.create_maj( ocs[0], ocs[1], ocs2[0] );

      auto s2 = ntk.create_maj( ocs[0], ocs[1], ocs2[1] );
      if ( ntk.fanout_size( ntk.get_node( s2 ) ) < 1 )
      {
        index = ntk.node_to_index( ntk.get_node( ocs[0] ) );
        if ( ( !ntk.is_pi( ntk.get_node( ocs[0] ) ) ) && ( index != 0 ) ) //&& ( ntk.is_on_critical_path( ntk.get_node( ocs[0] ) ) ) )
        {
          if ( ( ntk.fanout_size( ntk.get_node( ocs[0] ) ) == 5 ) || ( ntk.fanout_size( ntk.get_node( ocs[0] ) ) >= 17 ) || ( ntk.fanout_size( ntk.get_node( ocs[0] ) ) == 2 ) ) // it would mean the depth is actually increased
          {
            if ( ntk.fanout_size( ntk.get_node( s1 ) ) < 1 )
              ntk.take_out_node( ntk.get_node( s1 ) );
            ntk.take_out_node( ntk.get_node( s2 ) );
            return true;
          }
        }

        index = ntk.node_to_index( ntk.get_node( ocs[1] ) );
        if ( ( !ntk.is_pi( ntk.get_node( ocs[1] ) ) ) && ( index != 0 ) )
        {
          if ( ( ntk.fanout_size( ntk.get_node( ocs[1] ) ) == 5 ) || ( ntk.fanout_size( ntk.get_node( ocs[1] ) ) >= 17 ) || ( ntk.fanout_size( ntk.get_node( ocs[1] ) ) == 2 ) ) // it would mean the depth is actually increased
          {
            if ( ntk.fanout_size( ntk.get_node( s1 ) ) < 1 )
              ntk.take_out_node( ntk.get_node( s1 ) );
            ntk.take_out_node( ntk.get_node( s2 ) );
            return true;
          }
        }
      }

      auto opt = ntk.create_maj( ocs2[2], s1, s2 );

      if ( ( ntk.fanout_size( ntk.get_node( s1 ) ) == 5 ) || ( ntk.fanout_size( ntk.get_node( s1 ) ) >= 17 ) || ( ntk.fanout_size( ntk.get_node( s1 ) ) == 2 ) ) // it would mean the depth is actually increased
      {
        if ( ntk.fanout_size( ntk.get_node( s2 ) ) == 1 )
          ntk.take_out_node( ntk.get_node( s2 ) );
        if ( ntk.fanout_size( ntk.get_node( opt ) ) < 1 )
          ntk.take_out_node( ntk.get_node( opt ) );
        return true;
      }
      if ( ( ntk.fanout_size( ntk.get_node( s2 ) ) == 5 ) || ( ntk.fanout_size( ntk.get_node( s2 ) ) >= 17 ) || ( ntk.fanout_size( ntk.get_node( s2 ) ) == 2 ) ) // it would mean the depth is actually increased
      {
        if ( ntk.fanout_size( ntk.get_node( s1 ) ) == 1 )
          ntk.take_out_node( ntk.get_node( s1 ) );
        if ( ntk.fanout_size( ntk.get_node( opt ) ) < 1 )
          ntk.take_out_node( ntk.get_node( opt ) );
        return true;
      }
      if ( ( ntk.fanout_size( ntk.get_node( opt ) ) == 4 ) || ( ntk.fanout_size( ntk.get_node( opt ) ) >= 16 ) || ( ntk.fanout_size( ntk.get_node( opt ) ) == 1 ) ) // it would mean the depth is actually increased
      {
        if ( ntk.fanout_size( ntk.get_node( s1 ) ) == 1 )
          ntk.take_out_node( ntk.get_node( s1 ) );
        if ( ntk.fanout_size( ntk.get_node( s2 ) ) == 1 )
          ntk.take_out_node( ntk.get_node( s2 ) );
        return true;
      }
      ntk.substitute_node( n, opt );
      ntk.update_levels();
    }
    return true;
  }

  using candidate_t = std::tuple<signal<Ntk>, signal<Ntk>, signal<Ntk>, signal<Ntk>, bool>;
  std::optional<candidate_t> associativity_candidate( signal<Ntk> const& v, signal<Ntk> const& w, signal<Ntk> const& x, signal<Ntk> const& y, signal<Ntk> const& z ) const
  {
    if ( v.index == x.index )
    {
      return candidate_t{ w, y, z, v, v.complement == x.complement };
    }
    if ( v.index == y.index )
    {
      return candidate_t{ w, x, z, v, v.complement == y.complement };
    }
    if ( w.index == x.index )
    {
      return candidate_t{ v, y, z, w, w.complement == x.complement };
    }
    if ( w.index == y.index )
    {
      return candidate_t{ v, x, z, w, w.complement == y.complement };
    }

    return std::nullopt;
  }

  std::array<signal<Ntk>, 3> ordered_children( node<Ntk> const& n ) const
  {
    std::array<signal<Ntk>, 3> children;
    ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) { children[i] = f; } );
    std::stable_sort( children.begin(), children.end(), [this]( auto const& c1, auto const& c2 ) {
      return ntk.level( ntk.get_node( c1 ) ) < ntk.level( ntk.get_node( c2 ) );
    } );
    return children;
  }

  void mark_critical_path( node<Ntk> const& n )
  {
    if ( ntk.is_pi( n ) || ntk.is_constant( n ) || ntk.value( n ) )
      return;

    const auto level = ntk.level( n );
    ntk.set_value( n, 1 );
    ntk.foreach_fanin( n, [this, level]( auto const& f ) {
      if ( ntk.level( ntk.get_node( f ) ) == level - 1 )
      {
        mark_critical_path( ntk.get_node( f ) );
      }
    } );
  }

  void mark_critical_paths()
  {
    ntk.clear_values();
    ntk.foreach_po( [this]( auto const& f ) {
      if ( ntk.level( ntk.get_node( f ) ) == ntk.depth() )
      {
        mark_critical_path( ntk.get_node( f ) );
      }
    } );
  }

private:
  Ntk& ntk;
  mig_algebraic_depth_rewriting_params const& ps;
  mig_algebraic_depth_rewriting_stats& st;
}; // namespace detail

} // namespace detail

/*! \brief Majority algebraic depth rewriting.
 *
 * This algorithm tries to rewrite a network with majority gates for depth
 * optimization using the associativity and distributivity rule in
 * majority-of-3 logic.  It can be applied to networks other than MIGs, but
 * only considers pairs of nodes which both implement the majority-of-3
 * function.
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_maj`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_maj`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

  .. note::

      The implementation of this algorithm was heavily inspired by an
      implementation from Luca Amarù.
   \endverbatim
 */
template<class Ntk>
void mig_algebraic_depth_rewriting_splitters( Ntk& ntk, mig_algebraic_depth_rewriting_params const& ps = {}, mig_algebraic_depth_rewriting_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  mig_algebraic_depth_rewriting_stats st;
  detail::mig_algebraic_depth_rewriting_splitter_impl<Ntk> p( ntk, ps, st );
  p.run();

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */