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
  \file legacy.hpp
  \brief Some older not used routines

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <vector>

#include <fmt/format.h>

#include "../traits.hpp"
#include "arithmetic.hpp"
#include "control.hpp"

namespace mockturtle
{

namespace legacy
{

namespace detail
{

template<class IntType = int64_t>
inline std::pair<IntType, IntType> compute_montgomery_parameters( IntType c, IntType k = 0 )
{
  if ( k == 0 )
  {
    k = 1 << ( static_cast<IntType>( std::ceil( std::log2( c ) ) ) + 1 );
  }

  // egcd
  IntType y = k % c;
  IntType x = c;
  IntType a{ 0 }, b{ 1 };

  while ( y )
  {
    std::tie( a, b ) = std::pair<IntType, IntType>{ b, a - ( x / y ) * b };
    std::tie( x, y ) = std::pair<IntType, IntType>{ y, x % y };
  }

  const IntType ki = ( a > 0 ) ? ( a % c ) : ( c + ( a % c ) % c );
  const IntType factor = ( k * ki - 1 ) / c;

  return { k, factor };
}

template<class Ntk>
std::vector<signal<Ntk>> to_montgomery_form( Ntk& ntk, std::vector<signal<Ntk>> const& t, int32_t mod, uint32_t rbits, int64_t np )
{
  /* bit-width of original mod */
  uint32_t nbits = t.size() - rbits;

  std::vector<signal<Ntk>> t_rpart( t.begin(), t.begin() + rbits );
  auto m = carry_ripple_multiplier( ntk, t_rpart, constant_word( ntk, np, rbits ) );
  assert( m.size() == 2 * rbits );
  m.resize( rbits );
  assert( m.size() == rbits );

  m = carry_ripple_multiplier( ntk, m, constant_word( ntk, mod, nbits ) );
  assert( m.size() == t.size() );

  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, m, t, carry );

  m.erase( m.begin(), m.begin() + rbits );
  assert( m.size() == nbits );

  std::vector<signal<Ntk>> sum( m.begin(), m.end() );
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, sum, constant_word( ntk, mod, nbits ), carry_inv );

  mux_inplace( ntk, !carry, m, sum );
  return m;
}

} /* namespace detail */

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod (2^k -
 * c)\f$.  The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t c )
{
  /* c must be smaller than 2^k */
  assert( c < ( UINT64_C( 1 ) << a.size() ) );

  /* refer to simpler case */
  if ( c == 0 )
  {
    modular_adder_inplace( ntk, a, b );
    return;
  }

  const auto word = constant_word( ntk, c, static_cast<uint32_t>( a.size() ) );
  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a, word, carry );

  carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a, b, carry );

  std::vector<signal<Ntk>> sum( a.begin(), a.end() );
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a, word, carry_inv );

  mux_inplace( ntk, !carry, a, sum );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod (2^k -
 * c)\f$.  The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t c )
{
  /* c must be smaller than 2^k */
  assert( c < ( UINT64_C( 1 ) << a.size() ) );

  /* refer to simpler case */
  if ( c == 0 )
  {
    modular_subtractor_inplace( ntk, a, b );
    return;
  }

  auto carry = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a, b, carry );

  const auto word = constant_word( ntk, c, static_cast<uint32_t>( a.size() ) );
  std::vector<signal<Ntk>> sum( a.begin(), a.end() );
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, sum, word, carry_inv );

  mux_inplace( ntk, carry, a, sum );
}

/*! \brief Creates modular multiplication based on Montgomery multiplication
 *
 * Given two inputs words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(ab) \bmod (2^k - c)\f$.
 * The first input word `a` is overridden and stores the output signals.
 *
 * The implementation is based on Montgomery multiplication and includes the
 * encoding and decoding in and from the Montgomery number representation.
 * Correct functionality is only ensured if both `a` and `b` are smaller than
 * \f$2^k - c\f$.
 */
template<class Ntk>
inline void modular_multiplication_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t c )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  const auto n = ( 1 << a.size() ) - c;
  const auto nbits = static_cast<uint64_t>( std::ceil( std::log2( n ) ) );

  auto [r, np] = detail::compute_montgomery_parameters<int64_t>( n );
  const auto rbits = static_cast<uint64_t>( std::log2( r ) );

  const auto f2 = constant_word( ntk, ( r * r ) % n, rbits );

  const auto ma = detail::to_montgomery_form( ntk, carry_ripple_multiplier( ntk, a, f2 ), n, rbits, np );
  const auto mb = detail::to_montgomery_form( ntk, carry_ripple_multiplier( ntk, b, f2 ), n, rbits, np );

  assert( ma.size() == nbits );
  assert( mb.size() == nbits );

  a = detail::to_montgomery_form( ntk, zero_extend( ntk, carry_ripple_multiplier( ntk, ma, mb ), nbits + rbits ), n, rbits, np );
  a = detail::to_montgomery_form( ntk, zero_extend( ntk, a, nbits + rbits ), n, rbits, np );
}

} // namespace legacy

} // namespace mockturtle
