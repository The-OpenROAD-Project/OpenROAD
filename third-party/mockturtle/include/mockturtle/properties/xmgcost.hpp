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
  \file xmgcost.hpp
  \brief Cost functions for xmg-based networks

  \author Heinz Riener
  \author Shubham Rai
*/

#pragma once

#include "../traits.hpp"

#include <fmt/format.h>

namespace mockturtle
{

struct xmg_gate_stats
{
  /*! \brief Total number of XOR3 gates (structurally). */
  uint32_t total_xor3{ 0 };

  /*! \brief Number of XOR3 (functionally). */
  uint32_t xor3{ 0 };

  /*! \brief Number of XOR2 (functionally). */
  uint32_t xor2{ 0 };

  /*! \brief Total number of MAJ gates (structurally). */
  uint32_t total_maj{ 0 };

  /*! \brief Number of MAJ gates. */
  uint32_t maj{ 0 };

  /*! \brief Number of AND/OR gates. */
  uint32_t and_or{ 0 };

  void report() const
  {
    fmt::print( "XOR3: {} = {} XOR3 + {} XOR2 / MAJ: {} = {} MAJ3 + {} AND/OR\n",
                total_xor3, xor2, xor3, total_maj, maj, and_or );
  }
};

/*! \brief Profile gates
 *
 * Counts the numbers of MAJ and XOR nodes in an XMG.
 *
 * \param ntk Network
 * \param stats Statistics
 */
template<class Ntk>
void xmg_profile_gates( Ntk const& ntk, xmg_gate_stats& stats )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_is_xor3_v<Ntk>, "Ntk does not implement the is_xor3 method" );

  ntk.foreach_gate( [&]( auto const& node ) {
    bool has_const_fanin = false;

    /* Check if all of the fanin nodes are not constant */
    ntk.foreach_fanin( node, [&]( auto const& f ) {
      if ( ntk.is_constant( ntk.get_node( f ) ) )
      {
        has_const_fanin = true;
        return false;
      }
      return true;
    } );

    if ( ntk.is_maj( node ) )
    {
      if ( has_const_fanin )
      {
        ++stats.and_or;
      }
      else
      {
        ++stats.maj;
      }
    }
    else if ( ntk.is_xor3( node ) )
    {
      if ( has_const_fanin )
      {
        ++stats.xor2;
      }
      else
      {
        ++stats.xor3;
      }
    }
  } );

  stats.total_xor3 = stats.xor2 + stats.xor3;
  stats.total_maj = stats.and_or + stats.maj;
}

} // namespace mockturtle