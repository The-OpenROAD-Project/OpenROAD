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
  \file control.hpp
  \brief Generate control logic networks

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Creates a word from a constant
 *
 * Creates a vector of `bitwidth` constants that represent the positive number
 * `value`.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> constant_word( Ntk& ntk, uint64_t value, uint32_t bitwidth )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  std::vector<signal<Ntk>> word( bitwidth );
  for ( auto i = 0u; i < bitwidth; ++i )
  {
    bool bit = false;
    if ( i < 64 )
    {
      bit = static_cast<bool>( ( value >> i ) & 1 );
    }
    word[i] = ntk.get_constant( bit );
  }
  return word;
}

/*! \brief Extends a word by leading zeros
 *
 * Adds leading zeros as most-significant bits to word `a`.  The size of `a`
 * must be smaller or equal to `bitwidth`, which is the width of the resulting
 * word.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> zero_extend( Ntk& ntk, std::vector<signal<Ntk>> const& a, uint32_t bitwidth )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  assert( bitwidth >= a.size() );

  auto ret{ a };
  for ( auto i = a.size(); i < bitwidth; ++i )
  {
    ret.emplace_back( ntk.get_constant( false ) );
  }
  return ret;
}

/*! \brief Creates a 2k-k MUX (array of k 2-1 MUXes).
 *
 * This creates *k* MUXes using `cond` as condition signal and `t` for the then
 * signals and `e` for the else signals.  The method works in-place and writes
 * the outputs of the networ into `t`.
 */
template<class Ntk>
inline void mux_inplace( Ntk& ntk, signal<Ntk> const& cond, std::vector<signal<Ntk>>& t, std::vector<signal<Ntk>> const& e )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite method" );

  std::transform( t.begin(), t.end(), e.begin(), t.begin(), [&]( auto const& a, auto const& b ) { return ntk.create_ite( cond, a, b ); } );
}

template<class Ntk>
inline std::vector<signal<Ntk>> mux( Ntk& ntk, signal<Ntk> const& cond, std::vector<signal<Ntk>> const& t, std::vector<signal<Ntk>> const& e )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite method" );

  std::vector<signal<Ntk>> ret;
  std::transform( t.begin(), t.end(), e.begin(), std::back_inserter( ret ), [&]( auto const& a, auto const& b ) { return ntk.create_ite( cond, a, b ); } );
  return ret;
}

/*! \brief Creates k-to-2^k binary decoder
 *
 * Given k signals `xs`, this function creates 2^k signals of which exactly one
 * input is 1, for each of the 2^k input assignments to `xs`.
 */
template<class Ntk>
std::vector<signal<Ntk>> binary_decoder( Ntk& ntk, std::vector<signal<Ntk>> const& xs )
{
  if ( xs.empty() )
  {
    return {};
  }

  if ( xs.size() == 1u )
  {
    return { ntk.create_not( xs[0] ), xs[0] };
  }

  // recursion
  const auto m = ( xs.size() + 1 ) / 2;

  const auto d1 = binary_decoder( ntk, std::vector<signal<Ntk>>( xs.begin(), xs.begin() + m ) );
  const auto d2 = binary_decoder( ntk, std::vector<signal<Ntk>>( xs.begin() + m, xs.end() ) );

  std::vector<signal<Ntk>> d( 1 << xs.size() );
  auto it = d.begin();

  for ( auto const& s2 : d2 )
  {
    for ( auto const& s1 : d1 )
    {
      *it++ = ntk.create_and( s1, s2 );
    }
  }

  return d;
}

/*! \brief Creates 2^k MUX
 *
 * Given k select signals `sel` and 2^k data signals `data`, this function
 * creates a logic network that outputs `data[i]` when `i` is the encoded
 * assignment of `sel`.
 *
 * This is an iterative construction based on MUX gates.  A more efficient
 * method may be provided by the Klein-Paterson variant
 * `binary_mux_klein_paterson`.
 */
template<class Ntk>
signal<Ntk> binary_mux( Ntk& ntk, std::vector<signal<Ntk>> const& sel, std::vector<signal<Ntk>> data )
{
  for ( auto i = 0u; i < sel.size(); ++i )
  {
    for ( auto j = 0u; j < ( 1u << ( sel.size() - i - 1u ) ); ++j )
    {
      data[j] = ntk.create_ite( sel[i], data[2 * j + 1], data[2 * j] );
    }
  }

  return data[0u];
}

/*! \brief Creates 2^k MUX
 *
 * Given k select signals `sel` and 2^k data signals `data`, this function
 * creates a logic network that outputs `data[i]` when `i` is the encoded
 * assignment of `sel`.
 *
 * This Klein-Paterson variant uses fewer gates than the direct method
 * `binary_mux` (see Klein, & Paterson. (1980). Asymptotically Optimal Circuit
 * for a Storage Access Function. IEEE Transactions on Computers, C-29(8),
 * 737–738. doi:10.1109/tc.1980.1675657 )
 */
template<class Ntk>
signal<Ntk> binary_mux_klein_paterson( Ntk& ntk, std::vector<signal<Ntk>> const& sel, std::vector<signal<Ntk>> const& data )
{
  if ( sel.size() == 1u )
  {
    return ntk.create_ite( sel[0u], data[1u], data[0u] );
  }

  // recursion
  const auto s = sel.size() / 2u;
  const auto r = sel.size() - s;

  const auto ds = binary_decoder( ntk, std::vector<signal<Ntk>>( sel.begin(), sel.begin() + s ) );
  std::vector<signal<Ntk>> s_data( 1u << r );
  for ( auto j = 0u; j < s_data.size(); ++j )
  {
    std::vector<signal<Ntk>> and_terms( 1u << s );
    std::transform( ds.begin(), ds.end(),
                    data.begin() + ( j * ( 1u << s ) ),
                    and_terms.begin(),
                    [&]( auto const& f1, auto const& f2 ) { return ntk.create_and( f1, f2 ); } );
    s_data[j] = ntk.create_nary_or( and_terms );
  }

  return binary_mux_klein_paterson( ntk, std::vector<signal<Ntk>>( sel.begin() + s, sel.end() ), s_data );
}

} // namespace mockturtle