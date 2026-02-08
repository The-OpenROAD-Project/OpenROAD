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
  \file name_utils.hpp
  \brief Utility functions to restore network names after optimization.

  \author Marcel Walter
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "node_map.hpp"

namespace mockturtle
{

/*! \brief Restores the network name that might have been given to network's former incarnation.
 *
 * \param ntk_src The source logic network, which potentially has a name
 * \param ntk_dest The destination logic network, whose name is to be restored
 */
template<typename NtkSrc, typename NtkDest>
void restore_network_name( const NtkSrc& ntk_src, NtkDest& ntk_dest ) noexcept
{
  static_assert( is_network_type_v<NtkSrc>, "NtkSrc is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  if constexpr ( has_get_network_name_v<NtkSrc> && has_set_network_name_v<NtkDest> )
  {
    ntk_dest.set_network_name( ntk_src.get_network_name() );
  }
}

/*! \brief Restores all names that might have been given to a network's former incarnation.
 *
 * **Required network functions for the NtkSrc:**
 * - `foreach_node`
 * - `foreach_fanin`
 * - `foreach_po`
 * - `get_node`
 *
 * \param ntk_src The source logic network, which potentially has named signals
 * \param ntk_dest The destination logic network, whose names are to be restored
 * \param old2new Mapping of nodes from ntk_src to signals of ntk_dest
 */
template<typename NtkSrc, typename NtkDest>
void restore_names( const NtkSrc& ntk_src, NtkDest& ntk_dest, node_map<signal<NtkDest>, NtkSrc>& old2new ) noexcept
{
  restore_network_name( ntk_src, ntk_dest );

  if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
  {
    static_assert( has_foreach_node_v<NtkSrc>, "NtkSrc does not implement the foreach_node function" );
    static_assert( has_foreach_fanin_v<NtkSrc>, "NtkSrc does not implement the foreach_fanin function" );
    static_assert( has_foreach_po_v<NtkSrc>, "NtkSrc does not implement the foreach_po function" );
    static_assert( has_get_node_v<NtkSrc>, "NtkSrc does not implement the get_node function" );

    const auto restore_signal_name = [&ntk_src, &ntk_dest, &old2new]( const auto& f ) {
      if ( ntk_src.has_name( f ) )
      {
        const auto name = ntk_src.get_name( f );

        ntk_dest.set_name( old2new[ntk_src.get_node( f )], name );
      }
    };

    const auto restore_output_name = [&ntk_src, &ntk_dest]( [[maybe_unused]] const auto& po, const auto i ) {
      if ( ntk_src.has_output_name( i ) )
      {
        const auto name = ntk_src.get_output_name( i );

        ntk_dest.set_output_name( i, name );
      }
    };

    ntk_src.foreach_node( [&ntk_src, &restore_signal_name]( const auto& n ) { ntk_src.foreach_fanin( n, restore_signal_name ); } );

    ntk_src.foreach_po( restore_output_name );
  }
}

/*! \brief Restore PI and PO names, matching by order.
 *
 * **Required network functions for NtkSrc:**
 * - `foreach_pi`
 * - `foreach_po`
 * - `num_pis`
 * - `num_pos`
 * - `has_name`
 * - `get_name`
 * - `make_signal`
 * - `has_output_name`
 * - `get_output_name`
 *
 * **Required network functions for NtkDest:**
 * - `foreach_pi`
 * - `num_pis`
 * - `num_pos`
 * - `set_name`
 * - `make_signal`
 * - `set_output_name`
 *
 * \param ntk_src The source logic network, which potentially has named signals
 * \param ntk_dest The destination logic network, whose names are to be restored
 */
template<typename NtkSrc, typename NtkDest>
void restore_pio_names_by_order( const NtkSrc& ntk_src, NtkDest& ntk_dest )
{
  static_assert( is_network_type_v<NtkSrc>, "NtkSrc is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );
  static_assert( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc>, "NtkSrc does not implement the has_name and/or get_name functions" );
  static_assert( has_has_output_name_v<NtkSrc> && has_get_output_name_v<NtkSrc>, "NtkSrc does not implement the has_output_name and/or get_output_name functions" );
  static_assert( has_set_name_v<NtkDest> && has_set_output_name_v<NtkDest>, "NtkDest does not implement the set_name and/or set_output_name functions" );

  assert( ntk_src.num_pis() == ntk_dest.num_pis() );
  assert( ntk_src.num_pos() == ntk_dest.num_pos() );

  std::vector<std::string> pi_names( ntk_src.num_pis(), "" );
  ntk_src.foreach_pi( [&]( auto const& n, auto i ) {
    if ( ntk_src.has_name( ntk_src.make_signal( n ) ) )
      pi_names[i] = ntk_src.get_name( ntk_src.make_signal( n ) );
  } );
  ntk_dest.foreach_pi( [&]( auto const& n, auto i ) {
    if ( pi_names[i] != "" )
      ntk_dest.set_name( ntk_dest.make_signal( n ), pi_names[i] );
  } );

  ntk_src.foreach_po( [&]( auto const& f, auto i ) {
    if ( ntk_src.has_output_name( i ) )
      ntk_dest.set_output_name( i, ntk_src.get_output_name( i ) );
  } );
}

} // namespace mockturtle