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
  \file arithmetic.hpp
  \brief Generate arithmetic logic networks

  \author Heinz Riener
  \author Jovan Blanuša
  \author Mathias Soeken
*/

#pragma once

#include <list>
#include <utility>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include "../networks/aig.hpp"
#include "../traits.hpp"
#include "control.hpp"

namespace mockturtle
{

/*! \brief Inserts a full adder into a network.
 *
 * Inserts a full adder for three inputs (two 1-bit operands and one carry)
 * into the network and returns a pair of sum and carry bit.
 *
 * By default creates a seven 2-input gate network composed of AND, NOR, and OR
 * gates.  If network has `create_node` function, creates two 3-input gate
 * network.  If the network has ternary `create_maj` and `create_xor3`
 * functions, it will use them (except for AIGs).
 *
 * \param ntk Network
 * \param a First input operand
 * \param b Second input operand
 * \param c Carry
 * \return Pair of sum (`first`) and carry (`second`)
 */
template<typename Ntk>
inline std::pair<signal<Ntk>, signal<Ntk>> full_adder( Ntk& ntk, const signal<Ntk>& a, const signal<Ntk>& b, const signal<Ntk>& c )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  /* specialization for LUT-ish networks */
  if constexpr ( has_create_node_v<Ntk> )
  {
    kitty::dynamic_truth_table tt_maj( 3u ), tt_xor( 3u );
    kitty::create_from_hex_string( tt_maj, "e8" );
    kitty::create_from_hex_string( tt_xor, "96" );

    const auto sum = ntk.create_node( { a, b, c }, tt_xor );
    const auto carry = ntk.create_node( { a, b, c }, tt_maj );

    return { sum, carry };
  }
  /* use MAJ and XOR3 if available by network, unless network is AIG */
  else if constexpr ( !std::is_same_v<typename Ntk::base_type, aig_network> && has_create_maj_v<Ntk> && has_create_xor3_v<Ntk> )
  {
    const auto carry = ntk.create_maj( a, b, c );
    const auto sum = ntk.create_xor3( a, b, c );
    return { sum, carry };
  }
  else
  {
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
    static_assert( has_create_nor_v<Ntk>, "Ntk does not implement the create_nor method" );
    static_assert( has_create_or_v<Ntk>, "Ntk does not implement the create_or method" );

    const auto w1 = ntk.create_and( a, b );
    const auto w2 = ntk.create_nor( a, b );
    const auto w3 = ntk.create_nor( w1, w2 );
    const auto w4 = ntk.create_and( c, w3 );
    const auto w5 = ntk.create_nor( c, w3 );
    const auto sum = ntk.create_nor( w4, w5 );
    const auto carry = ntk.create_or( w1, w4 );

    return { sum, carry };
  }
}

/*! \brief Inserts a half adder into a network.
 *
 * Inserts a half adder for two inputs (two 1-bit operands)
 * into the network and returns a pair of sum and carry bit.
 *
 * It creates three 2-input gate network composed of AND and NOR gates.
 *
 * \param ntk Network
 * \param a First input operand
 * \param b Second input operand
 * \return Pair of sum (`first`) and carry (`second`)
 */
template<typename Ntk>
inline std::pair<signal<Ntk>, signal<Ntk>> half_adder( Ntk& ntk, const signal<Ntk>& a, const signal<Ntk>& b )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  /* specialization for LUT-ish networks */
  if constexpr ( has_create_node_v<Ntk> )
  {
    kitty::dynamic_truth_table tt_and( 2u ), tt_xor( 2u );
    kitty::create_from_hex_string( tt_and, "8" );
    kitty::create_from_hex_string( tt_xor, "6" );

    const auto sum = ntk.create_node( { a, b }, tt_xor );
    const auto carry = ntk.create_node( { a, b }, tt_and );

    return { sum, carry };
  }
  else
  {
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
    static_assert( has_create_nor_v<Ntk>, "Ntk does not implement the create_nor method" );

    const auto carry = ntk.create_and( a, b );
    const auto w2 = ntk.create_nor( a, b );
    const auto sum = ntk.create_nor( carry, w2 );

    return { sum, carry };
  }
}

/*! \brief Creates carry ripple adder structure.
 *
 * Creates a carry ripple structure composed of full adders.  The vectors `a`
 * and `b` must have the same size.  The resulting sum bits are eventually
 * stored in `a` and the carry bit will be overridden to store the output carry
 * bit.
 *
 * \param a First input operand, will also have the output after the call
 * \param b Second input operand
 * \param carry Carry bit, will also have the output carry after the call
 */
template<typename Ntk>
inline void carry_ripple_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, signal<Ntk>& carry )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  assert( a.size() == b.size() );

  auto pa = a.begin();
  for ( auto pb = b.begin(); pa != a.end(); ++pa, ++pb )
  {
    std::tie( *pa, carry ) = full_adder( ntk, *pa, *pb, carry );
  }
}

/*! \brief Creates carry ripple subtractor structure.
 *
 * Creates a carry ripple structure composed of full adders.  The vectors `a`
 * and `b` must have the same size.  The resulting sum bits are eventually
 * stored in `a` and the carry bit will be overridden to store the output carry
 * bit.  The inputs in `b` are inverted to realize subtraction with full
 * adders.  The carry bit must be passed in inverted state to the subtractor.
 *
 * \param a First input operand, will also have the output after the call
 * \param b Second input operand
 * \param carry Carry bit, will also have the output carry after the call
 */
template<typename Ntk>
inline void carry_ripple_subtractor_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, const std::vector<signal<Ntk>>& b, signal<Ntk>& carry )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );

  assert( a.size() == b.size() );

  auto pa = a.begin();
  for ( auto pb = b.begin(); pa != a.end(); ++pa, ++pb )
  {
    std::tie( *pa, carry ) = full_adder( ntk, *pa, ntk.create_not( *pb ), carry );
  }
}

/*! \brief Creates a classical multiplier using full adders.
 *
 * The vectors `a` and `b` must not have the same size.  The function creates
 * the multiplier in `ntk` and returns output signals, whose size is the summed
 * sizes of `a` and `b`.
 *
 * \param ntk Network
 * \param a First input operand
 * \param b Second input operand
 */
template<typename Ntk>
inline std::vector<signal<Ntk>> carry_ripple_multiplier( Ntk& ntk, std::vector<signal<Ntk>> const& a, std::vector<signal<Ntk>> const& b )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  auto res = constant_word( ntk, 0, static_cast<uint32_t>( a.size() + b.size() ) );
  auto tmp = constant_word( ntk, 0, static_cast<uint32_t>( a.size() * 2 ) );

  for ( auto j = 0u; j < b.size(); ++j )
  {
    for ( auto i = 0u; i < a.size(); ++i )
    {
      std::tie( i ? tmp[a.size() + i - 1] : res[j], tmp[i] ) = full_adder( ntk, ntk.create_and( a[i], b[j] ), tmp[a.size() + i], tmp[i] );
    }
  }

  auto carry = tmp.back() = ntk.get_constant( false );
  for ( auto i = 0u; i < a.size(); ++i )
  {
    std::tie( res[b.size() + i], carry ) = full_adder( ntk, tmp[i], tmp[a.size() + i], carry );
  }

  return res;
}

// CLA implementation based on Alan Mishchenko's implementation in
// https://github.com/berkeley-abc/abc/blob/master/src/base/wlc/wlcBlast.c
namespace detail
{

template<typename Ntk>
inline std::pair<signal<Ntk>, signal<Ntk>> carry_lookahead_adder_inplace_rec( Ntk& ntk,
                                                                              typename std::vector<signal<Ntk>>::iterator genBegin,
                                                                              typename std::vector<signal<Ntk>>::iterator genEnd,
                                                                              typename std::vector<signal<Ntk>>::iterator proBegin,
                                                                              typename std::vector<signal<Ntk>>::iterator carBegin )
{
  auto const term_case = [&]( signal<Ntk> const& gen0, signal<Ntk> const& gen1, signal<Ntk> const& pro0, signal<Ntk> const& pro1, signal<Ntk> const& car ) -> std::tuple<signal<Ntk>, signal<Ntk>, signal<Ntk>> {
    auto tmp = ntk.create_and( gen0, pro1 );
    auto rPro = ntk.create_and( pro0, pro1 );
    auto rGen = ntk.create_or( ntk.create_or( gen1, tmp ), ntk.create_and( rPro, car ) );
    auto rCar = ntk.create_or( gen0, ntk.create_and( pro0, car ) );

    return { rGen, rPro, rCar };
  };

  auto m = std::distance( genBegin, genEnd );

  if ( m == 2 )
  {
    const auto [gen, pro, car] = term_case( *genBegin, *( genBegin + 1 ), *proBegin, *( proBegin + 1 ), *carBegin );
    *( carBegin + 1 ) = car;
    return { gen, pro };
  }
  else
  {
    m >>= 1;
    const auto [gen0, pro0] = carry_lookahead_adder_inplace_rec( ntk, genBegin, genBegin + m, proBegin, carBegin );
    *( carBegin + m ) = gen0;
    const auto [gen1, pro1] = carry_lookahead_adder_inplace_rec( ntk, genBegin + m, genEnd, proBegin + m, carBegin + m );

    const auto [gen, pro, car] = term_case( gen0, gen1, pro0, pro1, *carBegin );
    *( carBegin + m ) = car;
    return { gen, pro };
  }
}

template<typename Ntk>
inline void carry_lookahead_adder_inplace_pow2( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, signal<Ntk>& carry )
{
  if ( a.size() == 1u )
  {
    a[0] = full_adder( ntk, a[0], b[0], carry ).first;
    return;
  }

  std::vector<signal<Ntk>> gen( a.size() ), pro( a.size() ), car( a.size() + 1 );
  car[0] = carry;
  std::transform( a.begin(), a.end(), b.begin(), gen.begin(), [&]( auto const& f, auto const& g ) { return ntk.create_and( f, g ); } );
  std::transform( a.begin(), a.end(), b.begin(), pro.begin(), [&]( auto const& f, auto const& g ) { return ntk.create_xor( f, g ); } );

  carry_lookahead_adder_inplace_rec( ntk, gen.begin(), gen.end(), pro.begin(), car.begin() );
  std::transform( pro.begin(), pro.end(), car.begin(), a.begin(), [&]( auto const& f, auto const& g ) { return ntk.create_xor( f, g ); } );
}

} // namespace detail

/*! \brief Creates carry lookahead adder structure.
 *
 * Creates a carry lookahead structure composed of full adders.  The vectors `a`
 * and `b` must have the same size.  The resulting sum bits are eventually
 * stored in `a` and the carry bit will be overridden to store the output carry
 * bit.
 *
 * \param a First input operand, will also have the output after the call
 * \param b Second input operand
 * \param carry Carry bit, will also have the output carry after the call
 */
template<typename Ntk>
inline void carry_lookahead_adder_inplace( Ntk& ntk, std::vector<signal<Ntk>>& a, std::vector<signal<Ntk>> const& b, signal<Ntk>& carry )
{
  /* extend bitsize to next power of two */
  const auto log2 = static_cast<uint32_t>( std::ceil( std::log2( static_cast<double>( a.size() + 1 ) ) ) );

  std::vector<signal<Ntk>> a_ext( a.begin(), a.end() );
  a_ext.resize( static_cast<uint64_t>( 1 ) << log2, ntk.get_constant( false ) );
  std::vector<signal<Ntk>> b_ext( b.begin(), b.end() );
  b_ext.resize( static_cast<uint64_t>( 1 ) << log2, ntk.get_constant( false ) );

  detail::carry_lookahead_adder_inplace_pow2( ntk, a_ext, b_ext, carry );

  std::copy_n( a_ext.begin(), a.size(), a.begin() );
  carry = a_ext[a.size()];
}

/*! \brief Creates a sideways sum adder using half and full adders
 *
 * The function creates the adder in `ntk` and returns output signals,
 * whose size is floor(log2(a.size()))+1.
 *
 * \param ntk Network
 * \param a Input operand
 */
template<typename Ntk>
inline std::vector<signal<Ntk>> sideways_sum_adder( Ntk& ntk, std::vector<signal<Ntk>> const& a )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );

  int n = static_cast<int>( a.size() );

  int out_n = 1; // floor(log2(n) + 1)
  int tmpn = n;
  while ( tmpn >>= 1 )
    out_n++;

  std::list<signal<Ntk>> sum_bits, carry_bits;

  auto* first_level = &sum_bits;
  auto* second_level = &carry_bits;

  auto res = constant_word( ntk, 0, out_n );
  int output_ind = 0;

  for ( int i = 0; i < n; i++ )
    first_level->push_back( a[i] );

  while ( 1 )
  {
    while ( !first_level->empty() )
    {
      if ( first_level->size() == 1 )
      {
        auto in_sig = first_level->front();
        first_level->pop_front();
        res[output_ind++] = in_sig;
      }
      else if ( first_level->size() == 2 )
      {
        auto in_sig1 = first_level->front();
        first_level->pop_front();
        auto in_sig2 = first_level->front();
        first_level->pop_front();
        signal<Ntk> tmp_sum;
        signal<Ntk> tmp_carry;
        std::tie( tmp_sum, tmp_carry ) = half_adder( ntk, in_sig1, in_sig2 );
        first_level->push_back( tmp_sum );
        second_level->push_back( tmp_carry );
      }
      else // first_level->size() >=3
      {
        auto in_sig1 = first_level->front();
        first_level->pop_front();
        auto in_sig2 = first_level->front();
        first_level->pop_front();
        auto in_sig3 = first_level->front();
        first_level->pop_front();
        signal<Ntk> tmp_sum;
        signal<Ntk> tmp_carry;
        std::tie( tmp_sum, tmp_carry ) = full_adder( ntk, in_sig1, in_sig2, in_sig3 );
        first_level->push_back( tmp_sum );
        second_level->push_back( tmp_carry );
      }
    }

    if ( second_level->empty() )
      break;

    // swapping buffers
    auto tmp = first_level;
    first_level = second_level;
    second_level = tmp;
  }

  return res;
}

} // namespace mockturtle
