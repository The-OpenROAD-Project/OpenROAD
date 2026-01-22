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
  \file miter.hpp
  \brief Generate miter from two networks

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <optional>

#include "../traits.hpp"
#include "cleanup.hpp"

namespace mockturtle
{

/*! \brief Creates a combinational miter from two networks.
 *
 * This method combines two networks that have the same number of primary
 * inputs and the same number of primary outputs into a miter.  The miter
 * has the same number of inputs and one primary output.  This output is the
 * OR of XORs of all primary output pairs.  In other words, the miter outputs
 * 1 for all input assignments in which the two input networks differ.
 *
 * All networks may have different types.  The method returns an optional, which
 * is `nullopt`, whenever the two input networks don't match in their number of
 * primary inputs and primary outputs.
 */
template<class NtkDest, class NtkSource1, class NtkSource2>
std::optional<NtkDest> miter( NtkSource1 const& ntk1, NtkSource2 const& ntk2 )
{
  static_assert( is_network_type_v<NtkSource1>, "NtkSource1 is not a network type" );
  static_assert( is_network_type_v<NtkSource2>, "NtkSource2 is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_num_pis_v<NtkSource1>, "NtkSource1 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource1>, "NtkSource1 does not implement the num_pos method" );
  static_assert( has_num_pis_v<NtkSource2>, "NtkSource2 does not implement the num_pis method" );
  static_assert( has_num_pos_v<NtkSource2>, "NtkSource2 does not implement the num_pos method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_xor_v<NtkDest>, "NtkDest does not implement the create_xor method" );
  static_assert( has_create_nary_or_v<NtkDest>, "NtkDest does not implement the create_nary_or method" );

  /* both networks must have same number of inputs and outputs */
  if ( ( ntk1.num_pis() != ntk2.num_pis() ) || ( ntk1.num_pos() != ntk2.num_pos() ) )
  {
    return std::nullopt;
  }

  /* create primary inputs */
  NtkDest dest;
  std::vector<signal<NtkDest>> pis;
  for ( auto i = 0u; i < ntk1.num_pis(); ++i )
  {
    pis.push_back( dest.create_pi() );
  }

  /* copy networks */
  const auto pos1 = cleanup_dangling( ntk1, dest, pis.begin(), pis.end() );
  const auto pos2 = cleanup_dangling( ntk2, dest, pis.begin(), pis.end() );

  if constexpr ( has_EXODC_interface_v<NtkSource1> )
  {
    ntk1.build_oe_miter( dest, pos1, pos2 );
    return dest;
  }
  if constexpr ( has_EXODC_interface_v<NtkSource2> )
  {
    ntk2.build_oe_miter( dest, pos1, pos2 );
    return dest;
  }

  /* create XOR of output pairs */
  std::vector<signal<NtkDest>> xor_outputs;
  std::transform( pos1.begin(), pos1.end(), pos2.begin(), std::back_inserter( xor_outputs ),
                  [&]( auto const& o1, auto const& o2 ) { return dest.create_xor( o1, o2 ); } );

  /* create big OR of XOR gates */
  dest.create_po( dest.create_nary_or( xor_outputs ) );

  return dest;
}

} // namespace mockturtle