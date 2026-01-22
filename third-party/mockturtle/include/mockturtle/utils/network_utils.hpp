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
  \file network_utils.hpp
  \brief Utility functions to insert a network into another network.

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "node_map.hpp"

#include <vector>

namespace mockturtle
{

namespace detail
{

template<typename NtkSrc, typename NtkDest>
auto clone_node_topologically( NtkSrc const& ntk, NtkDest& subntk, unordered_node_map<typename NtkDest::signal, NtkSrc>& node_to_signal, typename NtkSrc::node n )
{
  if ( node_to_signal.has( n ) )
  {
    return node_to_signal[n];
  }

  std::vector<typename NtkDest::signal> children;
  ntk.foreach_fanin( n, [&]( auto const& fi ) {
    auto s = clone_node_topologically( ntk, subntk, node_to_signal, ntk.get_node( fi ) );
    children.emplace_back( ntk.is_complemented( fi ) ? !s : s );
  } );

  if constexpr ( has_is_and_v<NtkSrc> )
  {
    static_assert( has_create_and_v<NtkDest> && "NtkDest does not implement the create_and method" );
    if ( ntk.is_and( n ) )
    {
      assert( children.size() == 2u );
      return node_to_signal[n] = subntk.create_and( children[0], children[1] );
    }
  }
  if constexpr ( has_is_xor_v<NtkSrc> )
  {
    static_assert( has_create_xor_v<NtkDest> && "NtkDest does not implement the create_xor method" );
    if ( ntk.is_xor( n ) )
    {
      assert( children.size() == 2u );
      return node_to_signal[n] = subntk.create_xor( children[0], children[1] );
    }
  }
  if constexpr ( has_is_maj_v<NtkSrc> )
  {
    static_assert( has_create_maj_v<NtkDest> && "NtkDest does not implement the create_maj method" );
    if ( ntk.is_maj( n ) )
    {
      assert( children.size() == 3u );
      return node_to_signal[n] = subntk.create_maj( children[0], children[1], children[2] );
    }
  }
  if constexpr ( has_is_xor3_v<NtkSrc> )
  {
    static_assert( has_create_xor3_v<NtkDest> && "NtkDest does not implement the create_xor3 method" );
    if ( ntk.is_xor3( n ) )
    {
      assert( children.size() == 3u );
      return node_to_signal[n] = subntk.create_xor3( children[0], children[1], children[2] );
    }
  }

  assert( false && "[e] unsupported node type" );
  return subntk.get_constant( false );
}

} // namespace detail

/*! \brief Constructs a (sub-)network from a window of another network.
 *
 * The window is specified by three parameters:
 *   1.) `inputs` are the common support of all window nodes, they do
 *       not overlap with `gates` (i.e., the intersection of `inputs` and
 *       `gates` is the empty set).
 *   2.) `gates` are the nodes in the window, supported by the
 *       `inputs` (i.e., `gates` are in the transitive fanout of the
 *       `inputs`).
 *   3.) `outputs` are signals (regular or complemented nodes)
 *        pointing to nodes in `gates` or `inputs`.  Not all fanouts
 *        of an output node are already part of the window.
 *
 * **Required network functions for the source Ntk:**
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_complemented`
 *
 * **Required network functions for the cloned SubNtk:**
 * - `create_pi`
 * - `create_po`
 * - `create_not`
 * - `get_constant`
 *
 * \param ntk A logic network
 * \param subntk An empty network to be constructed
 */
template<typename Ntk, typename SubNtk>
void clone_subnetwork( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs, std::vector<typename Ntk::signal> const& outputs, std::vector<typename Ntk::node> const& gates, SubNtk& subntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );

  static_assert( is_network_type_v<SubNtk>, "SubNtk is not a network type" );
  static_assert( has_create_pi_v<SubNtk>, "SubNtk does not implement the create_pi method" );
  static_assert( has_create_po_v<SubNtk>, "SubNtk does not implement the create_po method" );
  static_assert( has_create_not_v<SubNtk>, "SubNtk does not implement the create_not method" );
  static_assert( has_get_constant_v<SubNtk>, "SubNtk does not implement the get_constant method" );

  /* map from nodes in ntk to signals in subntk */
  unordered_node_map<typename SubNtk::signal, Ntk> node_to_signal( ntk );

  /* constant */
  node_to_signal[ntk.get_node( ntk.get_constant( false ) )] = subntk.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    node_to_signal[ntk.get_node( ntk.get_constant( true ) )] = subntk.get_constant( true );
  }

  /* inputs */
  for ( auto const& i : inputs )
  {
    node_to_signal[i] = subntk.create_pi();
  }

  /* create gates topologically */
  for ( auto const& g : gates )
  {
    detail::clone_node_topologically( ntk, subntk, node_to_signal, g );
  }

  /* outputs */
  for ( auto const& o : outputs )
  {
    subntk.create_po( ntk.is_complemented( o ) ? subntk.create_not( node_to_signal[ntk.get_node( o )] ) : node_to_signal[ntk.get_node( o )] );
  }
}

/*! \brief Inserts a network into another network
 *
 * **Required network functions for the host Ntk:**
 * - `get_constant`
 * - `create_not`
 *
 * **Required network functions for the subnetwork SubNtk:**
 * - `num_pis`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_gate`
 * - `foreach_fanin`
 * - `get_node`
 * - `is_complemented`
 *
 * \param ntk The host logic network
 * \param begin Begin iterator of signal inputs in the host network
 * \param end End iterator of signal inputs in the host network
 * \param subntk The sub-network
 * \param fn Callback function
 */
template<typename Ntk, typename SubNtk, typename BeginIter, typename EndIter, typename Fn>
void insert_ntk( Ntk& ntk, BeginIter begin, EndIter end, SubNtk const& subntk, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );

  static_assert( is_network_type_v<SubNtk>, "SubNtk is not a network type" );
  static_assert( has_num_pis_v<SubNtk>, "SubNtk does not implement the num_pis method" );
  static_assert( has_foreach_pi_v<SubNtk>, "SubNtk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<SubNtk>, "SubNtk does not implement the foreach_po method" );
  static_assert( has_foreach_gate_v<SubNtk>, "SubNtk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<SubNtk>, "SubNtk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<SubNtk>, "SubNtk does not implement the get_node method" );
  static_assert( has_is_complemented_v<SubNtk>, "SubNtk does not implement the is_complemented method" );

  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal<Ntk>>, "BeginIter value_type must be Ntk signal type" );
  static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal<Ntk>>, "EndIter value_type must be Ntk signal type" );

  assert( uint64_t( std::distance( begin, end ) ) == subntk.num_pis() );

  /* map from nodes in subntk to signals in ntk */
  unordered_node_map<typename Ntk::signal, SubNtk> node_to_signal( subntk );

  /* inputs */
  auto it = begin;
  subntk.foreach_pi( [&]( auto const& n ) {
    node_to_signal[n] = *( it++ );
  } );

  /* create gates topologically */
  subntk.foreach_gate( [&]( auto const& n ) {
    detail::clone_node_topologically( subntk, ntk, node_to_signal, n );
  } );

  /* outputs */
  subntk.foreach_po( [&]( auto const& f ) {
    fn( subntk.is_complemented( f ) ? ntk.create_not( node_to_signal[subntk.get_node( f )] ) : node_to_signal[subntk.get_node( f )] );
  } );
}

} // namespace mockturtle