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
  \file direct.hpp
  \brief Resynthesis by trying to directly add gates

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>

#include "../../algorithms/akers_synthesis.hpp"
#include "../../networks/mig.hpp"

namespace mockturtle
{

struct direct_resynthesis_params
{
  bool warn_on_unsupported{ false };
};

/*! \brief Resynthesis function that creates a gate for each node.
 *
 * If it detects a function that can be constructed as a gate, it does so.
 * Otherwise, it does not create a gate.  In that case, a warning can be
 * printed, if configured in the parameter struct.
 *
 * The function works with all 0-, 1-, and 2-input node functions and with
 * some 3-input node functions, e.g., 3-input majority for MIGs and XMGs, or
 * 3-input XOR for XMGs.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const klut_network klut = ...;
      direct_resynthesis resyn;
      const auto mig = node_resynthesis<mig_network>( klut, resyn );
   \endverbatim
 */
template<class Ntk>
class direct_resynthesis
{
public:
  direct_resynthesis( direct_resynthesis_params const& ps = {} )
      : ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
    static_assert( has_create_or_v<Ntk>, "Ntk does not implement the create_or method" );
    static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    (void)end;
    switch ( function.num_vars() )
    {
    case 0u:
      synthesize0( ntk, function, fn );
      break;

    case 1u:
      synthesize1( ntk, function, *begin, fn );
      break;

    case 2u:
      synthesize2( ntk, function, *begin, *( begin + 1 ), fn );
      break;

    case 3u:
      synthesize3( ntk, function, *begin, *( begin + 1 ), *( begin + 2 ), fn );
      break;
    }
  }

private:
  template<typename Fn>
  void synthesize0( Ntk& ntk, kitty::dynamic_truth_table const& function, Fn&& fn ) const
  {
    fn( ntk.get_constant( kitty::is_const0( function ) ) );
  }

  template<typename Fn>
  void synthesize1( Ntk& ntk, kitty::dynamic_truth_table const& function, signal<Ntk> const& f, Fn&& fn ) const
  {
    switch ( *function.begin() )
    {
    case 0b00:
      fn( ntk.get_constant( false ) );
      break;
    case 0b01:
      fn( ntk.create_not( f ) );
      break;
    case 0b10:
      fn( f );
      break;
    case 0b11:
      fn( ntk.get_constant( true ) );
      break;
    }
  }

  template<typename Fn>
  void synthesize2( Ntk& ntk, kitty::dynamic_truth_table const& function, signal<Ntk> const& f, signal<Ntk> const& g, Fn&& fn ) const
  {
    switch ( *function.begin() )
    {
    case 0b0000:
      fn( ntk.get_constant( false ) );
      break;
    case 0b0001: /* NOR */
      fn( ntk.create_not( ntk.create_or( f, g ) ) );
      break;
    case 0b0010: /* AND(f, !g) */
      fn( ntk.create_and( f, ntk.create_not( g ) ) );
      break;
    case 0b0011: /* !g */
      fn( ntk.create_not( g ) );
      break;
    case 0b0100: /* AND(!f, g) */
      fn( ntk.create_and( ntk.create_not( f ), g ) );
      break;
    case 0b0101: /* !f */
      fn( ntk.create_not( f ) );
      break;
    case 0b0110: /* XOR */
      fn( ntk.create_xor( f, g ) );
      break;
    case 0b0111: /* NAND */
      fn( ntk.create_not( ntk.create_and( f, g ) ) );
      break;
    case 0b1000: /* AND */
      fn( ntk.create_and( f, g ) );
      break;
    case 0b1001: /* XNOR */
      fn( ntk.create_not( ntk.create_xor( f, g ) ) );
      break;
    case 0b1010: /* f */
      fn( f );
      break;
    case 0b1011: /* OR(f, !g) */
      fn( ntk.create_or( f, ntk.create_not( g ) ) );
      break;
    case 0b1100: /* g */
      fn( g );
      break;
    case 0b1101: /* OR(!f, g) */
      fn( ntk.create_or( ntk.create_not( f ), g ) );
      break;
    case 0b1110: /* OR(f, g) */
      fn( ntk.create_or( f, g ) );
      break;
    case 0b1111:
      fn( ntk.get_constant( true ) );
      break;
    }
  }

  template<typename Fn>
  void synthesize3( Ntk& ntk, kitty::dynamic_truth_table const& function, signal<Ntk> const& f, signal<Ntk> const& g, signal<Ntk> const& h, Fn&& fn ) const
  {
    // TODO? all contained extended 1-input and 2-input functions
    // TODO create_ite

    const auto word = *function.begin();
    switch ( word )
    {
    case 0x00:
      fn( ntk.get_constant( false ) );
      break;
    case 0xff:
      fn( ntk.get_constant( true ) );
      break;
    case 0xe8: /* <fgh> */
    case 0xd4: /* <!fgh> */
    case 0xb2: /* <f!gh> */
    case 0x8e: /* <fg!h> */
    case 0x71: /* <!f!gh> */
    case 0x4d: /* <!fg!h> */
    case 0x2b: /* <f!g!h> */
    case 0x17: /* <!f!g!h> */
      if constexpr ( has_create_maj_v<Ntk> )
      {
        const auto _f = ( ( word == 0xd4 ) || ( word == 0x71 ) || ( word == 0x4d ) || ( word == 0x17 ) ) ? ntk.create_not( f ) : f;
        const auto _g = ( ( word == 0xb2 ) || ( word == 0x71 ) || ( word == 0x2b ) || ( word == 0x17 ) ) ? ntk.create_not( g ) : g;
        const auto _h = ( ( word == 0x8e ) || ( word == 0x4d ) || ( word == 0x2b ) || ( word == 0x17 ) ) ? ntk.create_not( h ) : h;
        fn( ntk.create_maj( _f, _g, _h ) );
      }
      else
      {
        if ( ps.warn_on_unsupported )
        {
          std::cout << "[w] function " << kitty::to_hex( function ) << " cannot be synthesized as gate\n";
        }
      }
      break;
    case 0x96: /* [abc] */
    case 0x69: /* ![abc] */
      if constexpr ( has_create_xor3_v<Ntk> )
      {
        const auto o = ntk.create_xor3( f, g, h );
        fn( word == 0x69 ? ntk.create_not( o ) : o );
      }
      else
      {
        if ( ps.warn_on_unsupported )
        {
          std::cout << "[w] function " << kitty::to_hex( function ) << " cannot be synthesized as gate\n";
        }
      }
      break;
    default:
      std::cout << "[w] failed to synthesize function " << kitty::to_hex( function ) << "\n";
    }
  }

private:
  direct_resynthesis_params ps;
};

} /* namespace mockturtle */