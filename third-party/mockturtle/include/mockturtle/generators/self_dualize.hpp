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
  \file self_dualize.hpp
  \brief Self-dualize a logic network

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#include "../algorithms/reconv_cut.hpp"
#include "../networks/aig.hpp"
#include "../views/cut_view.hpp"
#include "../views/topo_view.hpp"

#include <algorithm>
#include <unordered_map>

namespace mockturtle
{

/*! \brief Generates a self-dual AIG
 *
 * Generates the self-dualization of a multi-output logic network N.
 * The algorithm iterates over the output functions f0, ..., fm of N
 * and computes self-dualized function gi for 0 <= i <= m defined by
 * the formula
 *
 * gi(x0, x1, ..., xn)
 *   = (x0 * fi(x1, ..., xn)) + (!x0 * !fi(!x1, ..., !xn)).
 *
 */
inline aig_network self_dualize_aig( aig_network const& src_aig )
{
  using node = node<aig_network>;
  using signal = signal<aig_network>;

  aig_network dest_aig;
  std::unordered_map<node, signal> node_to_signal_one;
  std::unordered_map<node, signal> node_to_signal_two;

  /* copy inputs */
  node_to_signal_one[0] = dest_aig.get_constant( false );
  node_to_signal_two[0] = dest_aig.get_constant( false );
  src_aig.foreach_pi( [&]( const auto& n ) {
    auto const pi = dest_aig.create_pi();
    node_to_signal_one[n] = pi;
    node_to_signal_two[n] = !pi;
  } );

  reconvergence_driven_cut_parameters ps;
  ps.max_leaves = 99999999u;
  reconvergence_driven_cut_statistics st;
  detail::reconvergence_driven_cut_impl<aig_network, false, false> cut_generator( src_aig, ps, st );

  src_aig.foreach_po( [&]( const auto& f ) {
    auto leaves = cut_generator.run( { src_aig.get_node( f ) } ).first;
    std::stable_sort( std::begin( leaves ), std::end( leaves ) );

    /* check if all leaves are pis */
    for ( const auto& l : leaves )
    {
      (void)l;
      assert( src_aig.is_pi( l ) );
    }

    cut_view<aig_network> view( src_aig, leaves, f );
    topo_view<decltype( view )> topo_view( view );

    /* create cone once */
    topo_view.foreach_gate( [&]( const auto& g ) {
      std::vector<signal> new_fanins;
      topo_view.foreach_fanin( g, [&]( const auto& fi ) {
        auto const n = topo_view.get_node( fi );
        new_fanins.emplace_back( topo_view.is_complemented( fi ) ? !node_to_signal_one[n] : node_to_signal_one[n] );
      } );

      assert( new_fanins.size() == 2u );
      node_to_signal_one[g] = dest_aig.create_and( new_fanins[0u], new_fanins[1u] );
    } );

    /* create cone once */
    topo_view.foreach_gate( [&]( const auto& g ) {
      std::vector<signal> new_fanins;
      topo_view.foreach_fanin( g, [&]( const auto& fi ) {
        auto const n = topo_view.get_node( fi );
        new_fanins.emplace_back( topo_view.is_complemented( fi ) ? !node_to_signal_two[n] : node_to_signal_two[n] );
      } );

      assert( new_fanins.size() == 2u );
      node_to_signal_two[g] = dest_aig.create_and( new_fanins[0u], new_fanins[1u] );
    } );

    auto const output_signal_one = topo_view.is_complemented( f ) ? !node_to_signal_one[topo_view.get_node( f )] : node_to_signal_one[topo_view.get_node( f )];
    auto const output_signal_two = topo_view.is_complemented( f ) ? !node_to_signal_two[topo_view.get_node( f )] : node_to_signal_two[topo_view.get_node( f )];

    auto const new_pi = dest_aig.create_pi();
    auto const output = dest_aig.create_or( dest_aig.create_and( new_pi, output_signal_one ), dest_aig.create_and( !new_pi, !output_signal_two ) );
    dest_aig.create_po( output );
  } );

  return dest_aig;
}

} /* namespace mockturtle */