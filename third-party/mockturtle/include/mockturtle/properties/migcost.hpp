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
  \file migcost.hpp
  \brief Cost functions for majority-based technologies

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <unordered_set>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Counts number of inverters.
 *
 * This number counts all nodes that need to be inverted.  Multiple signals
 * with complements to the same node are counted once.
 *
 * \param ntk Network
 */
template<class Ntk>
uint32_t num_inverters( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );

  std::unordered_set<node<Ntk>> inverted_nodes;

  ntk.foreach_gate( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        inverted_nodes.insert( ntk.get_node( f ) );
      }
    } );
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_complemented( f ) )
    {
      inverted_nodes.insert( ntk.get_node( f ) );
    }
  } );

  return static_cast<uint32_t>( inverted_nodes.size() );
}

/*! \brief Counts fanins which are primary inputs.
 *
 * \param ntk Network
 */
template<class Ntk>
uint32_t num_dangling_inputs( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );

  uint32_t costs{ 0u };

  ntk.foreach_gate( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.is_pi( ntk.get_node( f ) ) )
      {
        costs++;
      }
    } );
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_pi( ntk.get_node( f ) ) )
    {
      costs++;
    }
  } );

  return costs;
}

} // namespace mockturtle