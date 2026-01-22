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
  \file collapse_mapped.hpp
  \brief Collapses mapped network into k-LUT network

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <iterator>
#include <optional>
#include <unordered_map>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../utils/window_utils.hpp"
#include "../views/color_view.hpp"
#include "../views/fanout_view.hpp"
#include "../views/topo_view.hpp"
#include "../views/window_view.hpp"
#include "simulation.hpp"

#include <kitty/dynamic_truth_table.hpp>

namespace mockturtle
{

namespace detail
{

template<class NtkDest, class NtkSource>
class collapse_mapped_network_impl
{
public:
  collapse_mapped_network_impl( NtkSource const& ntk )
      : ntk( ntk )
  {
  }

  void run( NtkDest& dest )
  {
    node_map<signal<NtkDest>, NtkSource> node_to_signal( ntk );

    /* special map for output drivers to perform some optimizations */
    enum class driver_type
    {
      none,
      pos,
      neg,
      mixed
    };
    node_map<driver_type, NtkSource> node_driver_type( ntk, driver_type::none );

    /* opposites are filled for nodes with mixed driver types, since they have
       two nodes in the network. */
    std::unordered_map<node<NtkSource>, signal<NtkDest>> opposites;

    /* initial driver types */
    ntk.foreach_po( [&]( auto const& f ) {
      switch ( node_driver_type[f] )
      {
      case driver_type::none:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::pos;
        break;
      case driver_type::pos:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::mixed : driver_type::pos;
        break;
      case driver_type::neg:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::mixed;
        break;
      case driver_type::mixed:
      default:
        break;
      }
    } );

    /* it could be that internal nodes also point to an output driver node */
    ntk.foreach_node( [&]( auto const n ) {
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) || !ntk.is_cell_root( n ) )
        return;

      ntk.foreach_cell_fanin( n, [&]( auto fanin ) {
        if ( node_driver_type[fanin] == driver_type::neg )
        {
          node_driver_type[fanin] = driver_type::mixed;
        }
      } );
    } );

    /* constants */
    auto add_constant_to_map = [&]( bool value ) {
      const auto n = ntk.get_node( ntk.get_constant( value ) );
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = dest.get_constant( value );
        break;

      case driver_type::neg:
        node_to_signal[n] = dest.get_constant( !value );
        break;

      case driver_type::mixed:
        node_to_signal[n] = dest.get_constant( value );
        opposites[n] = dest.get_constant( !value );
        break;
      }
    };

    add_constant_to_map( false );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    {
      add_constant_to_map( true );
    }

    /* primary inputs */
    ntk.foreach_pi( [&]( auto n ) {
      signal<NtkDest> dest_signal;
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest_signal;
        break;

      case driver_type::neg:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest.create_not( dest_signal );
        break;

      case driver_type::mixed:
        dest_signal = dest.create_pi();
        node_to_signal[n] = dest_signal;
        opposites[n] = dest.create_not( node_to_signal[n] );
        break;
      }

      if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
      {
        if ( ntk.has_name( ntk.make_signal( n ) ) )
          dest.set_name( dest_signal, ntk.get_name( ntk.make_signal( n ) ) );
      }
    } );

    /* ROs & RO names & register information */
    if constexpr ( has_foreach_ro_v<NtkSource> && has_create_ro_v<NtkDest> )
    {
      std::vector<signal<NtkDest>> cis;
      ntk.foreach_ro( [&]( auto const& n, auto i ) {
        cis.push_back( dest.create_ro() );
        if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
        {
          auto const s = ntk.make_signal( n );
          if ( ntk.has_name( s ) )
          {
            dest.set_name( cis.back(), ntk.get_name( s ) );
          }
          if ( ntk.has_name( !s ) )
          {
            dest.set_name( !cis.back(), ntk.get_name( !s ) );
          }
        }
        dest.set_register( i, ntk.register_at( i ) );
      } );

      auto it = std::begin( cis );
      if constexpr ( has_foreach_ro_v<NtkSource> )
      {
        ntk.foreach_ro( [&]( auto n ) {
          node_to_signal[n] = *it++;
        } );
      }
    }
    
    fanout_view<NtkSource> fanout_ntk{ ntk };
    fanout_ntk.clear_visited();
    color_view<fanout_view<NtkSource>> color_ntk{ fanout_ntk };

    /* nodes */
    topo_view topo{ ntk };
    topo.foreach_node( [&]( auto n ) {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) || !ntk.is_cell_root( n ) )
        return;

      std::vector<signal<NtkDest>> children;
      ntk.foreach_cell_fanin( n, [&]( auto fanin ) {
        children.push_back( node_to_signal[fanin] );
      } );

      kitty::dynamic_truth_table tt;

      if constexpr ( has_cell_function_v<NtkSource> )
      {
        tt = ntk.cell_function( n );
      }
      else
      {
        /* compute function constructing a window */
        std::vector<node<NtkSource>> roots{ n };
        std::vector<node<NtkSource>> leaves;

        ntk.foreach_cell_fanin( n, [&]( auto fanin ) {
          leaves.push_back( fanin );
        } );

        std::vector<node<NtkSource>> gates{ collect_nodes( color_ntk, leaves, roots ) };
        window_view window_ntk{ color_ntk, leaves, roots, gates };

        using Ntk = mockturtle::window_view<mockturtle::color_view<mockturtle::fanout_view<NtkSource>>>;
        default_simulator<kitty::dynamic_truth_table> sim( window_ntk.num_pis() );
        unordered_node_map<kitty::dynamic_truth_table, Ntk> node_to_value( window_ntk );

        simulate_nodes( window_ntk, node_to_value, sim );

        tt = node_to_value[n];
      }

      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = dest.create_node( children, tt );
        break;

      case driver_type::neg:
        node_to_signal[n] = dest.create_node( children, ~tt );
        break;

      case driver_type::mixed:
        node_to_signal[n] = dest.create_node( children, tt );
        opposites[n] = dest.create_node( children, ~tt );
        break;
      }
    } );

    /* outputs */
    ntk.foreach_po( [&]( auto const& f, auto index ) {
      (void)index;

      if ( ntk.is_complemented( f ) && node_driver_type[f] == driver_type::mixed )
      {
        dest.create_po( opposites[ntk.get_node( f )] );
      }
      else
      {
        dest.create_po( node_to_signal[f] );
      }

      if constexpr ( has_has_output_name_v<NtkSource> && has_get_output_name_v<NtkSource> && has_set_output_name_v<NtkDest> )
      {
        if ( ntk.has_output_name( index ) )
        {
          dest.set_output_name( index, ntk.get_output_name( index ) );
        }
      }
    } );

    /* RIs */
    if constexpr ( has_foreach_ri_v<NtkSource> && has_create_ri_v<NtkDest> )
    {
      ntk.foreach_ri( [&]( auto const& f ) {
        dest.create_ri( ntk.is_complemented( f ) ? dest.create_not( node_to_signal[f] ) : node_to_signal[f] );
      } );
    }

    /* CO names */
    if constexpr ( has_has_output_name_v<NtkSource> && has_get_output_name_v<NtkSource> && has_set_output_name_v<NtkDest> )
    {
      ntk.foreach_co( [&]( auto co, auto index ) {
        (void)co;
        if ( ntk.has_output_name( index ) )
        {
          dest.set_output_name( index, ntk.get_output_name( index ) );
        }
      } );
    }
  }

private:
  NtkSource const& ntk;
};

} /* namespace detail */

/*! \brief Collapse mapped network into k-LUT network.
 *
 * Collapses a mapped network into a k-LUT network.  In the mapped network each
 * cell is represented in terms of a collection of nodes from the subject graph.
 * This method creates a new network in which each cell is represented by a
 * single node.
 *
 * This function performs some optimizations with respect to possible output
 * complementations in the subject graph:
 *
 * - If an output driver is only used in positive form, nothing changes
 * - If an output driver is only used in complemented form, the cell function
 *   of the node is negated.
 * - If an output driver is used in both forms, two nodes will be created for
 *   the mapped node.
 *
 * **Required network functions for parameter ntk (type NtkSource):**
 * - `has_mapping`
 * - `get_constant`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_node`
 * - `foreach_cell_fanin`
 * - `is_constant`
 * - `is_pi`
 * - `is_cell_root`
 * - `cell_function`
 * - `is_complemented`
 *
 * **Required network functions for return value (type NtkDest):**
 * - `get_constant`
 * - `create_pi`
 * - `create_node`
 * - `create_not`
 */
template<class NtkDest, class NtkSource>
std::optional<NtkDest> collapse_mapped_network( NtkSource const& ntk )
{
  static_assert( is_network_type_v<NtkSource>, "NtkSource is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_has_mapping_v<NtkSource>, "NtkSource does not implement the has_mapping method" );
  static_assert( has_num_gates_v<NtkSource>, "NtkSource does not implement the num_gates method" );
  static_assert( has_get_constant_v<NtkSource>, "NtkSource does not implement the get_constant method" );
  static_assert( has_get_node_v<NtkSource>, "NtkSource does not implement the get_node method" );
  static_assert( has_foreach_pi_v<NtkSource>, "NtkSource does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<NtkSource>, "NtkSource does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<NtkSource>, "NtkSource does not implement the foreach_node method" );
  static_assert( has_foreach_cell_fanin_v<NtkSource>, "NtkSource does not implement the foreach_cell_fanin method" );
  static_assert( has_is_constant_v<NtkSource>, "NtkSource does not implement the is_constant method" );
  static_assert( has_is_pi_v<NtkSource>, "NtkSource does not implement the is_pi method" );
  static_assert( has_is_cell_root_v<NtkSource>, "NtkSource does not implement the is_cell_root method" );
  static_assert( has_is_complemented_v<NtkSource>, "NtkSource does not implement the is_complemented method" );

  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_node_v<NtkDest>, "NtkDest does not implement the create_node method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );

  if ( !ntk.has_mapping() && ntk.num_gates() > 0 )
  {
    return std::nullopt;
  }
  else
  {
    detail::collapse_mapped_network_impl<NtkDest, NtkSource> p( ntk );
    NtkDest dest;
    p.run( dest );
    return dest;
  }
}

/*! \brief Collapse mapped network into k-LUT network.
 *
 * Collapses a mapped network into a k-LUT network.  In the mapped network each
 * cell is represented in terms of a collection of nodes from the subject graph.
 * This method creates a new network in which each cell is represented by a
 * single node.
 *
 * This function performs some optimizations with respect to possible output
 * complementations in the subject graph:
 *
 * - If an output driver is only used in positive form, nothing changes
 * - If an output driver is only used in complemented form, the cell function
 *   of the node is negated.
 * - If an output driver is used in both forms, two nodes will be created for
 *   the mapped node.
 *
 * **Required network functions for parameter ntk (type NtkSource):**
 * - `has_mapping`
 * - `get_constant`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_node`
 * - `foreach_cell_fanin`
 * - `is_constant`
 * - `is_pi`
 * - `is_cell_root`
 * - `cell_function`
 * - `is_complemented`
 *
 * **Required network functions for return value (type NtkDest):**
 * - `get_constant`
 * - `create_pi`
 * - `create_node`
 * - `create_not`
 */
template<class NtkDest, class NtkSource>
bool collapse_mapped_network( NtkDest& dest, NtkSource const& ntk )
{
  static_assert( is_network_type_v<NtkSource>, "NtkSource is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_has_mapping_v<NtkSource>, "NtkSource does not implement the has_mapping method" );
  static_assert( has_num_gates_v<NtkSource>, "NtkSource does not implement the num_gates method" );
  static_assert( has_get_constant_v<NtkSource>, "NtkSource does not implement the get_constant method" );
  static_assert( has_get_node_v<NtkSource>, "NtkSource does not implement the get_node method" );
  static_assert( has_foreach_pi_v<NtkSource>, "NtkSource does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<NtkSource>, "NtkSource does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<NtkSource>, "NtkSource does not implement the foreach_node method" );
  static_assert( has_foreach_cell_fanin_v<NtkSource>, "NtkSource does not implement the foreach_cell_fanin method" );
  static_assert( has_is_constant_v<NtkSource>, "NtkSource does not implement the is_constant method" );
  static_assert( has_is_pi_v<NtkSource>, "NtkSource does not implement the is_pi method" );
  static_assert( has_is_cell_root_v<NtkSource>, "NtkSource does not implement the is_cell_root method" );
  static_assert( has_is_complemented_v<NtkSource>, "NtkSource does not implement the is_complemented method" );

  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_node_v<NtkDest>, "NtkDest does not implement the create_node method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );

  if ( !ntk.has_mapping() && ntk.num_gates() > 0 )
  {
    return false;
  }
  else
  {
    detail::collapse_mapped_network_impl<NtkDest, NtkSource> p( ntk );
    p.run( dest );
    return true;
  }
}

} /* namespace mockturtle */