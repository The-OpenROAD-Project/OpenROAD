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
  \file modular_arithmetic.hpp
  \brief Generate modular arithmetic logic networks

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

namespace detail
{

inline void invert_modulus( std::vector<bool>& m )
{
  m.flip();

  for ( auto i = 0u; i < m.size(); ++i )
  {
    m[i] = !m[i];
    if ( m[i] )
      break;
  }
}

inline void increment_inplace( std::vector<bool>& word )
{
  auto it = word.begin();

  while ( it != word.end() )
  {
    if ( ( *it++ = !*it ) )
    {
      return;
    }
  }
}

inline std::vector<bool> increment( std::vector<bool> const& word )
{
  auto copy = word;
  increment_inplace( copy );
  return copy;
}

inline void decrement_inplace( std::vector<bool>& word )
{
  auto it = word.begin();

  while ( it != word.end() )
  {
    if ( !( *it++ = !*it ) )
    {
      return;
    }
  }
}

inline std::vector<bool> decrement( std::vector<bool> const& word )
{
  auto copy = word;
  decrement_inplace( copy );
  return copy;
}

} /* namespace detail */

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod 2^k\f$.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b )
{
  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a, b, carry );
}

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod m\f$.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  // bit-size for corrected addition
  const uint32_t bitsize = static_cast<uint32_t>( m.size() );
  assert( bitsize <= a.size() );

  // corrected registers
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );
  std::vector<signal<Ntk>> b_trim( b.begin(), b.begin() + bitsize );

  // 1. Compute (a + b) on bitsize bits
  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, a_trim, b_trim, carry ); /* a_trim <- a + b */

  // store result in sum (and extend it to bitsize + 1 bits)
  auto sum = a_trim; /* sum <- a + b */
  sum.emplace_back( ntk.get_constant( false ) );

  // 2. Compute (a + b) - m (m is represented as word) on (bitsize + 1) bits
  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );
  auto carry_inv = ntk.get_constant( true );
  a_trim.emplace_back( carry );
  carry_ripple_subtractor_inplace( ntk, a_trim, word, carry_inv ); /* a_trim <- a + b - c */

  // if overflow occurred in step 2, return result from step 2, otherwise, result from step 1.
  mux_inplace( ntk, carry_inv, a_trim, sum );

  // copy corrected register back into input register
  std::copy_n( a_trim.begin(), bitsize, a.begin() );
}

/*! \brief Creates modular adder
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a + b) \bmod m\f$.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  // simpler case
  if ( m == ( UINT64_C( 1 ) << a.size() ) )
  {
    modular_adder_inplace( ntk, a, b );
    return;
  }

  // bit-size for corrected addition
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_adder_inplace( ntk, a, b, mvec );
}

template<class Ntk>
inline std::vector<signal<Ntk>> modular_adder( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  auto w = a;
  modular_adder_inplace( ntk, w, b, m );
  return w;
}

template<class Ntk>
inline void modular_adder_hiasat_inplace( Ntk& ntk, std::vector<signal<Ntk>>& x, std::vector<signal<Ntk>> const& y, std::vector<bool> const& m )
{
  assert( m.size() <= x.size() );
  assert( x.size() == y.size() );

  const uint32_t bitsize = static_cast<uint32_t>( m.size() );

  // corrected registers
  std::vector<signal<Ntk>> x_trim( x.begin(), x.begin() + bitsize );
  std::vector<signal<Ntk>> y_trim( y.begin(), y.begin() + bitsize );

  // compute Z-vector from m-vector (Z = 2^bitsize - m)
  auto z = m;
  detail::invert_modulus( z );

  /* SAC unit */
  std::vector<signal<Ntk>> A( bitsize ), B( bitsize + 1 ), a( bitsize ), b( bitsize + 1 );

  B[0] = b[0] = ntk.get_constant( false );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    A[i] = ntk.create_xor( x_trim[i], y_trim[i] );
    B[i + 1] = ntk.create_and( x_trim[i], y_trim[i] );
    a[i] = z[i] ? ntk.create_xnor( x_trim[i], y_trim[i] ) : A[i];
    b[i + 1] = z[i] ? ntk.create_or( x_trim[i], y_trim[i] ) : B[i + 1];
  }

  /* CPG unit */
  std::vector<signal<Ntk>> G( bitsize ), P( bitsize + 1 ), g( bitsize ), p( bitsize + 1 );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    G[i] = ntk.create_and( A[i], B[i] );
    P[i] = ntk.create_xor( A[i], B[i] );
    g[i] = ntk.create_and( a[i], b[i] );
    p[i] = ntk.create_xor( a[i], b[i] );
  }
  P[bitsize] = B[bitsize];
  p[bitsize] = b[bitsize];

  /* CLA for C_out */
  std::vector<signal<Ntk>> C( bitsize );
  C[0] = p[bitsize];
  for ( auto i = 1u; i < bitsize; ++i )
  {
    std::vector<signal<Ntk>> cube;
    cube.push_back( g[i] );
    for ( auto j = i + 1u; j < bitsize; ++j )
    {
      cube.push_back( p[j] );
    }
    C[i] = ntk.create_nary_and( cube );
  }
  const auto Cout = ntk.create_nary_or( C );
  // ntk.create_po( Cout );

  /* MUX store result in p and g */
  p.pop_back();
  P.pop_back();
  mux_inplace( ntk, Cout, g, G );
  mux_inplace( ntk, Cout, p, P );

  /* CLAS */
  C[0] = ntk.get_constant( false );
  for ( auto i = 1u; i < bitsize; ++i )
  {
    C[i] = ntk.create_or( g[i - 1], ntk.create_and( p[i - 1], C[i - 1] ) );
  }

  for ( auto i = 0u; i < bitsize; ++i )
  {
    x[i] = ntk.create_xor( p[i], C[i] );
  }
}

template<class Ntk>
inline void modular_adder_hiasat_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  // simpler case
  if ( m == ( UINT64_C( 1 ) << a.size() ) )
  {
    modular_adder_inplace( ntk, a, b );
    return;
  }

  // bit-size for corrected addition
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_adder_hiasat_inplace( ntk, a, b, mvec );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod 2^k\f$.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b )
{
  auto carry = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a, b, carry );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod m\f$.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  // bit-size for corrected addition
  const uint32_t bitsize = static_cast<uint32_t>( m.size() );
  assert( bitsize <= a.size() );

  // corrected registers
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );
  std::vector<signal<Ntk>> b_trim( b.begin(), b.begin() + bitsize );

  // 1. Compute (a - b) on bitsize bits
  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, a_trim, b_trim, carry_inv ); /* a_trim <- a - b */

  // store result in sum (and extend it to bitsize + 1 bits)
  auto sum = a_trim; /* sum <- a - b */

  sum.emplace_back( ntk.get_constant( false ) );

  // 2. Compute (a - b) + m (m is represented as word) on (bitsize + 1) bits
  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );
  auto carry = ntk.get_constant( false );
  a_trim.emplace_back( ntk.create_not( carry_inv ) );
  carry_ripple_adder_inplace( ntk, a_trim, word, carry ); /* a_trim <- (a - b) + c */

  // if overflow occurred in step 2, return result from step 2, otherwise, result from step 1.
  mux_inplace( ntk, carry, a_trim, sum );

  // copy corrected register back into input register
  std::copy_n( a_trim.begin(), bitsize, a.begin() );
}

/*! \brief Creates modular subtractor
 *
 * Given two input words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a - b) \bmod m\f$.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  // simpler case
  if ( m == ( UINT64_C( 1 ) << a.size() ) )
  {
    modular_subtractor_inplace( ntk, a, b );
    return;
  }

  // bit-size for corrected subtraction
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_subtractor_inplace( ntk, a, b, mvec );
}

template<class Ntk>
inline std::vector<signal<Ntk>> modular_subtractor( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  auto w = a;
  modular_subtractor_inplace( ntk, w, b, m );
  return w;
}

/*! \brief Creates modular doubling (multiplication by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(2 * a) \bmod m\f$.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_doubling_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<bool> const& m )
{
  assert( a.size() >= m.size() );
  const auto bitsize = m.size();
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );

  std::vector<signal<Ntk>> shifted( bitsize + 1u, ntk.get_constant( false ) );
  std::copy( a_trim.begin(), a_trim.end(), shifted.begin() + 1u );
  std::copy_n( shifted.begin(), bitsize, a_trim.begin() );

  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );

  auto carry_inv = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, shifted, word, carry_inv );

  mux_inplace( ntk, ntk.create_not( carry_inv ), a_trim, std::vector<signal<Ntk>>( shifted.begin(), shifted.begin() + bitsize ) );
  std::copy( a_trim.begin(), a_trim.end(), a.begin() );
}

/*! \brief Creates modular doubling (multiplication by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(2 * a) \bmod m\f$.
 * The input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_doubling_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, uint64_t m )
{
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_doubling_inplace( ntk, a, mvec );
}

/*! \brief Creates modular halving (corrected division by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a / 2) \bmod m\f$.  The
 * modulus must be odd, and the function is evaluated to \f$a / 2\f$, if `a` is
 * even and to \f$(a + m) / 2\f$, if `a` is odd.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_halving_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<bool> const& m )
{
  assert( a.size() >= m.size() );
  assert( m.size() > 0u );
  assert( m[0] );
  const auto bitsize = m.size();
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );

  std::vector<signal<Ntk>> extended( bitsize + 1u, ntk.get_constant( false ) ), a_extended( bitsize + 1u, ntk.get_constant( false ) );
  std::copy( a_trim.begin(), a_trim.end(), extended.begin() );
  std::copy( a_trim.begin(), a_trim.end(), a_extended.begin() );

  std::vector<signal<Ntk>> word( bitsize + 1, ntk.get_constant( false ) );
  std::transform( m.begin(), m.end(), word.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );

  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, extended, word, carry );

  mux_inplace( ntk, a_trim[0], extended, a_extended );

  std::copy_n( extended.begin() + 1, bitsize, a.begin() );
}

/*! \brief Creates modular halving (corrected division by 2)
 *
 * Given one input word \f$a\f$ of size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(a / 2) \bmod m\f$.  The
 * modulus must be odd, and the function is evaluated to \f$a / 2\f$, if `a` is
 * even and to \f$(a + m) / 2\f$, if `a` is odd.
 * The input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_halving_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, uint64_t m )
{
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_halving_inplace( ntk, a, mvec );
}

/*! \brief Creates modular multiplication
 *
 * Given two inputs words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(ab) \bmod c\f$.
 * The modulus `m` is passed as a vector of Booleans to support large bitsizes.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_multiplication_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  assert( a.size() >= m.size() );
  assert( a.size() == b.size() );

  const auto bitsize = m.size();
  std::vector<signal<Ntk>> a_trim( a.begin(), a.begin() + bitsize );
  std::vector<signal<Ntk>> b_trim( b.begin(), b.begin() + bitsize );

  std::vector<signal<Ntk>> accu( bitsize );
  auto itA = a_trim.rbegin();
  std::transform( b_trim.begin(), b_trim.end(), accu.begin(), [&]( auto const& f ) { return ntk.create_and( *itA, f ); } );

  while ( ++itA != a_trim.rend() )
  {
    modular_doubling_inplace( ntk, accu, m );
    std::vector<signal<Ntk>> summand( bitsize );
    std::transform( b_trim.begin(), b_trim.end(), summand.begin(), [&]( auto const& f ) { return ntk.create_and( *itA, f ); } );
    modular_adder_inplace( ntk, accu, summand, m );
  }

  std::copy( accu.begin(), accu.end(), a.begin() );
}

/*! \brief Creates modular multiplier
 *
 * Given two inputs words of the same size *k*, this function creates a circuit
 * that computes *k* output signals that represent \f$(ab) \bmod c\f$.
 * The first input word `a` is overridden and stores the output signals.
 */
template<class Ntk>
inline void modular_multiplication_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, uint64_t m )
{
  const auto bitsize = static_cast<uint32_t>( std::ceil( std::log2( m ) ) );
  std::vector<bool> mvec( bitsize );
  for ( auto i = 0u; i < bitsize; ++i )
  {
    mvec[i] = static_cast<bool>( ( m >> i ) & 1 );
  }

  modular_multiplication_inplace( ntk, a, b, mvec );
}

template<class Ntk>
inline std::vector<signal<Ntk>> modular_multiplication( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& m )
{
  auto w = a;
  modular_multiplication_inplace( ntk, w, b, m );
  return w;
}

template<typename Ntk>
inline std::vector<signal<Ntk>> modular_constant_multiplier_one_bits( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<bool> const& constant )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  std::vector<signal<Ntk>> sum( a.size(), ntk.get_constant( false ) );

  auto it = std::find( constant.begin(), constant.end(), true );
  if ( it != constant.end() )
  {
    auto shift = std::distance( constant.begin(), it );
    std::copy_n( a.begin(), a.size() - shift, sum.begin() + shift );
    it = std::find( it + 1, constant.end(), true );

    while ( it != constant.end() )
    {
      shift = std::distance( constant.begin(), it );
      std::vector<signal<Ntk>> summand( a.size(), ntk.get_constant( false ) );
      std::copy_n( a.begin(), a.size() - shift, summand.begin() + shift );
      auto carry = ntk.get_constant( false );
      carry_ripple_adder_inplace( ntk, sum, summand, carry );
      it = std::find( it + 1, constant.end(), true );
    }
  }

  return sum;
}

template<class Ntk>
inline std::vector<signal<Ntk>> modular_constant_multiplier_csd( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<bool> const& constant )
{
  // constant == 0
  if ( std::find( constant.begin(), constant.end(), true ) == constant.end() )
  {
    return std::vector<signal<Ntk>>( a.size(), ntk.get_constant( false ) );
  }
  // constant == 1
  else if ( constant.front() && std::find( constant.begin() + 1, constant.end(), true ) == constant.end() )
  {
    return a;
  }
  // constant % 2 == 0
  else if ( !constant.front() )
  {
    // constant / 2
    std::vector<bool> new_constant( a.size(), false );
    std::copy( constant.begin() + 1, constant.end(), new_constant.begin() );
    auto res = modular_constant_multiplier( ntk, a, new_constant );
    res.insert( res.begin(), ntk.get_constant( false ) );
    res.pop_back();
    return res;
  }
  // constant % 4 == 1u
  else if ( !constant[1] )
  {
    auto res = modular_constant_multiplier( ntk, a, detail::decrement( constant ) );
    modular_adder_inplace( ntk, res, a );
    return res;
  }
  else /* ( constant % 4 == 3u ) */
  {
    auto res = modular_constant_multiplier( ntk, a, detail::increment( constant ) );
    modular_subtractor_inplace( ntk, res, a );
    return res;
  }
}

/*! \brief Creates modular constant-multiplier
 *
 * Given an input word of size *k* and a constant with the same bit-width,
 * this function creates a circuit that computes \f$(a\cdot\mathrm{constant}) \bmod 2^k\f$.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> modular_constant_multiplier( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<bool> const& constant )
{
  return modular_constant_multiplier_csd( ntk, a, constant );
}

/*! \brief Creates vector of Booleans from hex string
 *
 * This function can be used to create moduli for very large numbers that cannot
 * be represented using any of the integer built-in data types.  If the vector
 * `res` is too small for the value `hex` most-significant digits will be
 * ignored.
 */
inline void bool_vector_from_hex( std::vector<bool>& res, std::string_view hex, bool shrink_to_fit = true )
{
  auto itR = res.begin();
  auto itS = hex.rbegin();

  while ( itR != res.end() && itS != hex.rend() )
  {
    uint32_t number{ 0 };
    if ( *itS >= '0' && *itS <= '9' )
    {
      number = *itS - '0';
    }
    else if ( *itS >= 'a' && *itS <= 'f' )
    {
      number = *itS - 'a' + 10;
    }
    else if ( *itS >= 'A' && *itS <= 'F' )
    {
      number = *itS - 'A' + 10;
    }
    else
    {
      assert( false && "invalid hex number" );
    }

    for ( auto i = 0u; i < 4u; ++i )
    {
      *itR++ = ( number >> i ) & 1;
      if ( itR == res.end() )
      {
        break;
      }
    }

    ++itS;
  }

  if ( shrink_to_fit )
  {
    auto find_last = []( std::vector<bool>::const_iterator itFirst,
                         std::vector<bool>::const_iterator itLast,
                         bool value ) -> std::vector<bool>::const_iterator {
      auto cur = itLast;
      while ( itFirst != itLast )
      {
        if ( *itFirst == value )
        {
          cur = itFirst;
        }
        ++itFirst;
      }
      return cur;
    };

    const auto itLast = find_last( res.begin(), res.end(), true );
    if ( itLast == res.end() )
    {
      res.clear();
    }
    else
    {
      res.erase( find_last( res.begin(), res.end(), true ) + 1u, res.end() );
    }
  }
  else
  {
    /* in case the hex string was short, fill remaining values with false */
    std::fill( itR, res.end(), false );
  }
}

inline void bool_vector_from_dec( std::vector<bool>& res, uint64_t value )
{
  auto it = res.begin();
  while ( value && it != res.end() )
  {
    *it++ = value % 2;
    value >>= 1;
  }
}

inline uint64_t bool_vector_to_long( std::vector<bool> const& vec )
{
  return std::accumulate( vec.begin(), vec.end(), std::make_pair( 0u, 0ul ),
                          []( auto accu, auto bit ) {
                            return std::make_pair( accu.first + 1u, accu.second + ( bit ? 1ul << accu.first : 0ul ) );
                          } )
      .second;
}

/*! \brief Creates a multiplier assuming Montgomery numbers as inputs.
 *
 * This modular multiplication assumes the two inputs *a* and *b* to be
 * Montgomery numbers representing \f$a \cdot 2^k \bmod N\f$, where \f$N\f$ is
 * the modulus as bit-string, and \f$k\f$ is the bit-width of *a* and *b*.  It
 * returns a signal of length *b*.  The last paramaeter *NN* must be computed
 * such that \f$R \cdot 2^k = N \cdot NN\f$ using the extended GCD.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> montgomery_multiplication( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<signal<Ntk>> const& b, std::vector<bool> const& N, std::vector<bool> const& NN )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  assert( a.size() == b.size() );

  const auto logR = a.size();

  std::vector<signal<Ntk>> Nbits( logR, ntk.get_constant( false ) );
  std::transform( N.begin(), N.end(), Nbits.begin(), [&]( auto b ) { return ntk.get_constant( b ); } );

  /* multiply a and b and truncate to least-significant logR bits */
  auto mult1 = carry_ripple_multiplier( ntk, a, b );
  std::vector<signal<Ntk>> mult1_truncated( mult1.begin(), mult1.begin() + logR );

  /* compute (((a * b) % R) * NN) % R */
  auto mult2 = modular_constant_multiplier( ntk, mult1_truncated, NN );

  mult2.resize( 2 * logR, ntk.get_constant( false ) );
  auto Ncopy = N;
  Ncopy.resize( 2 * logR, false );
  auto summand = modular_constant_multiplier( ntk, mult2, Ncopy );

  assert( mult1.size() == 2 * logR );
  assert( summand.size() == 2 * logR );

  auto carry = ntk.get_constant( false );
  carry_ripple_adder_inplace( ntk, mult1, summand, carry );
  mult1.erase( mult1.begin(), mult1.begin() + logR );

  auto tcopy = mult1;
  carry = ntk.get_constant( true );
  carry_ripple_subtractor_inplace( ntk, tcopy, Nbits, carry );
  mux_inplace( ntk, carry, tcopy, mult1 );

  return tcopy;
}

/*! \brief Creates a multiplier assuming Montgomery numbers as inputs.
 *
 * This modular multiplication assumes the two inputs *a* and *b* to be
 * Montgomery numbers representing \f$a \cdot 2^k \bmod N\f$, where \f$N\f$ is
 * the modulus, and \f$k\f$ is the bit-width of *a* and *b*.  It returns a
 * signal of length *b*.
 */
template<class Ntk>
inline std::vector<signal<Ntk>> montgomery_multiplication( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<signal<Ntk>> const& b, uint64_t N )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  const auto logR = a.size();
  const auto R = 1 << logR;

  // egcd
  int64_t s = 0, old_s = 1, r = R, old_r = N;
  while ( r )
  {
    const auto q = old_r / r;
    std::tie( old_r, r ) = std::pair{ r, old_r - q * r };
    std::tie( old_s, s ) = std::pair{ s, old_s - q * s };
  }
  const auto NN = std::abs( old_s );

  std::vector<bool> Nvec( logR ), NNvec( logR );
  for ( auto i = 0u; i < logR; ++i )
  {
    Nvec[i] = static_cast<bool>( ( N >> i ) & 1 );
    NNvec[i] = static_cast<bool>( ( NN >> i ) & 1 );
  }

  // std::cout << fmt::format( "[i] R = {}, NN = {}, N = {}\n", R, NN, N );

  return montgomery_multiplication( ntk, a, b, Nvec, NNvec );
}

} // namespace mockturtle
