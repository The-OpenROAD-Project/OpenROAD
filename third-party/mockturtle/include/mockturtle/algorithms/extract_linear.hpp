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
  \file extract_linear.hpp
  \brief Extract linear subcircuit in XAGs

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

#include "../networks/xag.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

namespace mockturtle
{

/*! \brief Extract linear circuit from XAG
 *
 * Creates a new XAG that only contains the XOR gates of the original XAG.  For
 * each AND gate, the new XAG will contain one additional PI (for the AND
 * output) and two additional POs (for the AND inputs) in the same order as the
 * AND gates are traversed in topological order.
 *
 * Besides the new XAG, this function returns a vector of the size of all
 * original AND gates with pointers to the signals referring to the AND's fanin
 * and fanout (in that order).
 */
inline std::pair<xag_network, std::vector<std::array<xag_network::signal, 3>>>
extract_linear_circuit( xag_network const& xag )
{
  xag_network dest;
  std::vector<std::array<xag_network::signal, 3>> and_tuples;
  node_map<xag_network::signal, xag_network> old_to_new( xag );

  old_to_new[xag.get_constant( false )] = dest.get_constant( false );
  xag.foreach_pi( [&]( auto const& n ) {
    old_to_new[n] = dest.create_pi();
  } );

  topo_view topo{ xag };
  topo.foreach_node( [&]( auto const& n ) {
    if ( xag.is_constant( n ) || xag.is_pi( n ) )
      return;

    if ( xag.is_and( n ) )
    {
      std::array<xag_network::signal, 3> signal_tuple;
      xag.foreach_fanin( n, [&]( auto const& f, auto i ) {
        signal_tuple[i] = old_to_new[f] ^ xag.is_complemented( f );
      } );
      const auto and_pi = dest.create_pi();
      old_to_new[n] = and_pi;
      signal_tuple[2] = and_pi;
      and_tuples.push_back( signal_tuple );
    }
    else /* if ( xag.is_xor( n ) ) */
    {
      std::array<xag_network::signal, 2> children{};
      xag.foreach_fanin( n, [&]( auto const& f, auto i ) {
        children[i] = old_to_new[f] ^ xag.is_complemented( f );
      } );
      old_to_new[n] = dest.create_xor( children[0], children[1] );
    }
  } );

  xag.foreach_po( [&]( auto const& f ) {
    dest.create_po( old_to_new[f] ^ xag.is_complemented( f ) );
  } );
  for ( auto const& [a, b, _] : and_tuples )
  {
    (void)_;
    dest.create_po( a );
    dest.create_po( b );
  }

  return { dest, and_tuples };
}

namespace detail
{

struct merge_linear_circuit_impl
{
public:
  merge_linear_circuit_impl( xag_network const& xag, uint32_t num_and_gates )
      : xag( xag ),
        num_and_gates( num_and_gates ),
        old_to_new( xag ),
        and_pi( xag )
  {
  }

  xag_network run()
  {
    old_to_new[xag.get_constant( false )] = dest.get_constant( false );

    orig_pis = xag.num_pis() - num_and_gates;
    orig_pos = xag.num_pos() - 2 * num_and_gates;

    xag.foreach_pi( [&]( auto const& n, auto i ) {
      if ( i == orig_pis )
        return false;

      old_to_new[n] = dest.create_pi();
      return true;
    } );

    for ( auto i = 0u; i < num_and_gates; ++i )
    {
      create_and( i );
    }

    xag.foreach_po( [&]( auto const& f, auto i ) {
      if ( i == orig_pos )
        return false;

      dest.create_po( run_rec( xag.get_node( f ) ) ^ xag.is_complemented( f ) );
      return true;
    } );

    return dest;
  }

private:
  xag_network::signal create_and( uint32_t index )
  {
    if ( old_to_new.has( xag.pi_at( orig_pis + index ) ) )
    {
      return old_to_new[xag.pi_at( orig_pis + index )];
    }

    const auto f1 = xag.po_at( orig_pos + 2u * index );
    const auto f2 = xag.po_at( orig_pos + 2u * index + 1u );
    const auto c1 = run_rec( xag.get_node( f1 ) ) ^ xag.is_complemented( f1 );
    const auto c2 = run_rec( xag.get_node( f2 ) ) ^ xag.is_complemented( f2 );
    return old_to_new[xag.pi_at( orig_pis + index )] = dest.create_and( c1, c2 );
  }

  xag_network::signal run_rec( xag_network::node const& n )
  {
    if ( old_to_new.has( n ) )
    {
      return old_to_new[n];
    }

    assert( xag.is_xor( n ) );
    std::array<xag_network::signal, 2> children{};
    xag.foreach_fanin( n, [&]( auto const& cf, auto i ) {
      children[i] = run_rec( xag.get_node( cf ) ) ^ xag.is_complemented( cf );
    } );
    return old_to_new[n] = dest.create_xor( children[0], children[1] );
  }

private:
  xag_network dest;
  xag_network const& xag;
  uint32_t num_and_gates;
  uint32_t orig_pis, orig_pos;
  unordered_node_map<xag_network::signal, xag_network> old_to_new;
  unordered_node_map<uint32_t, xag_network> and_pi;
};

} // namespace detail

/*! \brief Re-insert AND gates in linear circuit
 *
 * Given an extracted linear circuit from `extract_linear_circuit` and the
 * number of original AND gates, this function re-inserts the AND gates,
 * assuming that they are represented as PI and PO pairs at the end of the
 * original PIs and POs.
 */
inline xag_network merge_linear_circuit( xag_network const& xag, uint32_t num_and_gates )
{
  return detail::merge_linear_circuit_impl( xag, num_and_gates ).run();
}

} /* namespace mockturtle */
