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
  \file xmg_optimization.hpp
  \brief Rewriting MAJ to XNORs.

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <string>

#include "../networks/xmg.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"
#include "cleanup.hpp"
#include "dont_cares.hpp"

namespace mockturtle
{

/*! \brief Optimizes some MAJ gates using satisfiability don't cares
 *
 * The function is based on `xag_dont_cares_optimization` in `xag_optimization.hpp`.
 *
 * If a MAJ gate is satisfiability don't care for assignments 000 and 111, it can be
 * replaced by an XNOR gate.
 */
inline xmg_network xmg_dont_cares_optimization( xmg_network const& xmg )
{
  node_map<xmg_network::signal, xmg_network> old_to_new( xmg );

  xmg_network dest;
  old_to_new[xmg.get_constant( false )] = dest.get_constant( false );

  xmg.foreach_pi( [&]( auto const& n ) {
    old_to_new[n] = dest.create_pi();
  } );

  satisfiability_dont_cares_checker<xmg_network> checker( xmg );

  topo_view<xmg_network>{ xmg }.foreach_node( [&]( auto const& n ) {
    if ( xmg.is_constant( n ) || xmg.is_pi( n ) )
      return;

    std::array<xmg_network::signal, 3> fanin;
    xmg.foreach_fanin( n, [&]( auto const& f, auto i ) {
      fanin[i] = old_to_new[f] ^ xmg.is_complemented( f );
    } );

    if ( xmg.is_maj( n ) )
    {
      if ( checker.is_dont_care( n, { false, false, false } ) && checker.is_dont_care( n, { true, true, true } ) )
      {
        old_to_new[n] = dest.create_xor3( !fanin[0], fanin[1], fanin[2] );
      }
      else
      {
        old_to_new[n] = dest.create_maj( fanin[0], fanin[1], fanin[2] );
      }
    }
    else /* is XOR */
    {
      old_to_new[n] = dest.create_xor3( fanin[0], fanin[1], fanin[2] );
    }
  } );

  xmg.foreach_po( [&]( auto const& f ) {
    dest.create_po( old_to_new[f] ^ xmg.is_complemented( f ) );
  } );

  return dest;
}

} /* namespace mockturtle */