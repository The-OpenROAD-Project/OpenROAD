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
  \file gates_to_nodes.hpp
  \brief Convert gate-based network into node-based network

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "simulation.hpp"

namespace mockturtle
{

/*! \brief Translates a gate-based network into a node-based network.
 *
 * A node will be created in the node-based network for every gate based on the
 * gate function.  Possible complemented fanins are merged into the node
 * function.
 *
 * **Required network functions for parameter ntk (type NtkSource):**
 * - `foreach_pi`
 * - `foreach_gate`
 * - `foreach_fanin`
 * - `get_constant`
 * - `get_node`
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `node_function`
 *
 * **Required network functions for return value (type NtkDest):**
 * - `create_pi`
 * - `create_po`
 * - `create_node`
 * - `create_not`
 * - `get_constant`
 *
 * \param ntk Network
 */
template<class NtkDest, class NtkSource>
NtkDest gates_to_nodes( NtkSource const& ntk )
{
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_node_v<NtkDest>, "NtkDest does not implement the create_node method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );

  static_assert( is_network_type_v<NtkSource>, "NtkSource is not a network type" );
  static_assert( has_foreach_pi_v<NtkSource>, "NtkSource does not implement the foreach_pi method" );
  static_assert( has_foreach_gate_v<NtkSource>, "NtkSource does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<NtkSource>, "NtkSource does not implement the foreach_fanin method" );
  static_assert( has_get_constant_v<NtkSource>, "NtkSource does not implement the get_constant method" );
  static_assert( has_get_node_v<NtkSource>, "NtkSource does not implement the get_node method" );
  static_assert( has_is_constant_v<NtkSource>, "NtkSource does not implement the is_constant method" );
  static_assert( has_is_pi_v<NtkSource>, "NtkSource does not implement the is_pi method" );
  static_assert( has_is_complemented_v<NtkSource>, "NtkSource does not implement the is_complemented method" );
  static_assert( has_node_function_v<NtkSource>, "NtkSource does not implement the node_function method" );

  NtkDest dest;
  node_map<signal<NtkDest>, NtkSource> node_to_signal( ntk );

  ntk.foreach_pi( [&]( auto const& n ) {
    node_to_signal[n] = dest.create_pi();
  } );

  node_to_signal[ntk.get_constant( false )] = dest.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    node_to_signal[ntk.get_constant( true )] = dest.get_constant( true );
  }

  ntk.foreach_gate( [&]( auto const& n ) {
    std::vector<signal<NtkDest>> children;
    auto func = ntk.node_function( n );
    ntk.foreach_fanin( n, [&]( auto const& c, auto i ) {
      if ( ntk.is_complemented( c ) )
      {
        kitty::flip_inplace( func, i );
      }
      children.push_back( node_to_signal[c] );
    } );

    node_to_signal[n] = dest.create_node( children, func );
  } );

  /* outputs */
  ntk.foreach_po( [&]( auto const& s ) {
    dest.create_po( ntk.is_complemented( s ) ? dest.create_not( node_to_signal[s] ) : node_to_signal[s] );
  } );

  return dest;
}

/*! \brief Creates a new network with a single node per output.
 *
 * This method can be applied to networks with a small number of primary inputs,
 * to collapse all the logic of an output into a single node.  The returning
 * network must support arbitrary node functions, e.g., `klut_network`.
 */
template<class NtkDest, class NtkSrc>
NtkDest single_node_network( NtkSrc const& src )
{
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_node_v<NtkDest>, "NtkDest does not implement the create_node method" );
  static_assert( is_network_type_v<NtkSrc>, "NtkSrc is not a network type" );
  static_assert( has_num_pis_v<NtkSrc>, "NtkSrc does not implement the num_pis method" );

  NtkDest ntk;
  std::vector<signal<NtkDest>> pis( src.num_pis() );
  std::generate( pis.begin(), pis.end(), [&]() { return ntk.create_pi(); } );

  default_simulator<kitty::dynamic_truth_table> sim( src.num_pis() );
  const auto tts = simulate<kitty::dynamic_truth_table>( src, sim );

  for ( auto tt : tts )
  {
    const auto support = kitty::min_base_inplace( tt );
    const auto small_tt = kitty::shrink_to( tt, static_cast<unsigned int>( support.size() ) );
    std::vector<signal<NtkDest>> children( support.size() );
    for ( auto i = 0u; i < support.size(); ++i )
    {
      children[i] = pis[support[i]];
    }
    ntk.create_po( ntk.create_node( children, small_tt ) );
  }

  return ntk;
}

} /* namespace mockturtle */