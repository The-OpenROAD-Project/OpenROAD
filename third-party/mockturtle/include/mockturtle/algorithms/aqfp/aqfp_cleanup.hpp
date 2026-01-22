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
  \file aqfp_cleanup.hpp
  \brief Buffered network cleanup and types conversion

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <vector>

#include "../../networks/buffered.hpp"
#include "../../utils/node_map.hpp"
#include "../../views/topo_view.hpp"

namespace mockturtle
{

/*! \brief Buffered cleanup dangling.
 *
 * This function implements `cleanup_dangling` for buffered networks.
 *
 * \param ntk buffered network type
 */
template<class Ntk>
Ntk cleanup_dangling_buffered( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( is_buffered_network_type_v<Ntk>, "Ntk is not a buffered network type" );
  static_assert( has_is_buf_v<Ntk>, "Ntk does not implement the is_buf method" );
  static_assert( has_create_buf_v<Ntk>, "Ntk does not implement the create_buf method" );

  using signal = typename Ntk::signal;

  Ntk res;
  node_map<signal, Ntk> old2new( ntk );

  old2new[ntk.get_constant( false )] = res.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    old2new[ntk.get_constant( true )] = res.get_constant( true );
  }

  ntk.foreach_pi( [&]( auto const& n ) {
    old2new[n] = res.create_pi();
  } );

  topo_view topo{ ntk };
  topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
      return;

    std::vector<signal> children;

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      children.push_back( old2new[f] ^ ntk.is_complemented( f ) );
    } );

    signal f;
    if ( ntk.is_buf( n ) )
    {
      f = res.create_buf( children[0] );
    }
    else
    {
      f = res.clone_node( ntk, n, children );
    }

    old2new[n] = f;
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_complemented( f ) )
      res.create_po( res.create_not( old2new[f] ) );
    else
      res.create_po( old2new[f] );
  } );

  return res;
}

/*! \brief Converts a `buffered_mig_network` to a `buffered_aqfp_network`.
 *
 * This function converts a `buffered_mig_network` to a `buffered_aqfp_network`.
 *
 * \param ntk `buffered_mig_network`
 */
buffered_aqfp_network convert_buffered_mig_to_aqfp( buffered_mig_network const& ntk )
{
  using signal = typename buffered_aqfp_network::signal;

  buffered_aqfp_network res;
  node_map<signal, buffered_mig_network> old2new( ntk );

  old2new[ntk.get_constant( false )] = res.get_constant( false );

  ntk.foreach_pi( [&]( auto const& n ) {
    old2new[n] = res.create_pi();
  } );

  topo_view topo{ ntk };
  topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
      return;

    std::vector<signal> children;

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      children.push_back( old2new[f] ^ ntk.is_complemented( f ) );
    } );

    signal f;
    if ( ntk.is_buf( n ) )
    {
      f = res.create_buf( children[0] );
    }
    else
    {
      f = res.create_maj( children[0], children[1], children[2] );
    }

    old2new[n] = f;
  } );

  ntk.foreach_po( [&]( auto const& f ) {
    if ( ntk.is_complemented( f ) )
      res.create_po( res.create_not( old2new[f] ) );
    else
      res.create_po( old2new[f] );
  } );

  return res;
}

} // namespace mockturtle