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
  \file muxig.hpp
  \brief  Mux-inverter graph logic network implementation

  \author Dewmini Marakkalage
  */

#pragma once

#include "tig.hpp"

namespace mockturtle
{

template<>
struct compute_function<three_input_function::mux>
{
  template<typename T>
  inline std::enable_if_t<kitty::is_truth_table<T>::value, T> operator()( T a, T b, T c )
  {
    return ternary_operation( a, b, c, []( auto a, auto b, auto c ) { return ( a & b ) | ( ~a & c ); } );
  }

  template<typename T>
  inline std::enable_if_t<std::is_integral<T>::value, T> operator()( T a, T b, T c )
  {
    return ( a & b ) | ( ~a & c );
  }
};

using muxig_signal = tig_network<three_input_function::mux>::signal;
using muxig_network = tig_network<three_input_function::mux>;

template<>
inline muxig_network::normalization_result muxig_network::normalized_fanins_for_and( muxig_signal a, muxig_signal b )
{
  if ( a.index > b.index )
  {
    std::swap( a, b );
  }

  if ( a.index == 0 )
  {
    return { false, { a.complement ? b : get_constant( false ) } };
  }

  if ( a.index == b.index )
  {
    return { false, { ( a.complement == b.complement ) ? a : get_constant( false ) } };
  }

  return { false, { !a, get_constant( false ), b } };
}

template<>
inline muxig_network::normalization_result muxig_network::normalized_fanins_for_xor( muxig_signal a, muxig_signal b )
{
  if ( a.index > b.index )
  {
    std::swap( a, b );
  }

  if ( a.index == 0 )
  {
    return { false, { a.complement ? !b : b } };
  }

  if ( a.index == b.index )
  {
    return { false, { get_constant( a.complement != b.complement ) } };
  }

  if ( b.complement )
  {
    // !(!a, !b, b) or (a, !b, b)
    if ( a.complement )
    {
      return { true, { !a, !b, b } };
    }
    return { false, { a, !b, b } };
  }

  // (!a, b, !b) or !(a, b, !b)
  if ( a.complement )
  {
    return { false, { !a, b, !b } };
  }
  return { true, { a, b, !b } };
}

template<>
inline muxig_network::normalization_result muxig_network::normalized_fanins( muxig_signal a, muxig_signal b, muxig_signal c )
{
  /* mux(a b c) = a b + a' c */
  /* always make sure that b is never inverted */
  /* always make sure that b.index <= c.index */
  /* if possible, make sure a is not inverted */
  /* if possible, make sure that a.index < b.index */
  /* else, if possible, make sure that a.index < c.index */

  if ( a.index == 0 )
  { // (1 b c) = b, (0 b c) = c,
    return { false, { a.complement ? b : c } };
  }

  if ( b.index > c.index )
  {
    std::swap( b, c );
    a = !a;
  }

  if ( b.index == 0 )
  {
    if ( b.complement )
    {
      // (a 1 c) = a + a'c = a + c = (a'c')'
      return complement( normalized_fanins_for_and( !a, !c ) );
    }
    // (a 0 c) = a'c
    return normalized_fanins_for_and( !a, c );
  }

  if ( a.index == b.index )
  {
    if ( a.complement == b.complement )
    {
      // (a a c) = aa + a'c = a + c = (a'c')'
      return complement( normalized_fanins_for_and( !a, !c ) );
    }
    // (a a' c) = aa' + a'c = a'c
    return normalized_fanins_for_and( !a, c );
  }

  if ( a.index == c.index )
  {
    if ( a.complement == c.complement )
    {
      // (a b a) = ab + a'a = ab
      return normalized_fanins_for_and( a, b );
    }
    // (a b a') == ab + a'a' = ab + a' = a' + b = (ab')'
    return complement( normalized_fanins_for_and( a, !b ) );
  }

  if ( b.index == c.index )
  {
    if ( b.complement == c.complement )
    {
      // (a, b, b) = a b + a' b
      return { false, { b } };
    }
    // (a, b, b') = a b + a' b' = a xnor b = (a xor b)'
    return complement( normalized_fanins_for_xor( a, b ) );
  }

  // (a b c) = a b + a' c
  if ( b.complement )
  {
    return { true, { a, !b, !c } };
  }
  return { false, { a, b, c } };
}

template<>
inline muxig_signal muxig_network::create_and( muxig_signal const& a, muxig_signal const& b )
{
  return create_gate( a, b, get_constant( false ) );
}

template<>
inline muxig_signal muxig_network::create_xor( muxig_signal const& a, muxig_signal const& b )
{
  return create_gate( a, !b, b );
}

template<>
inline muxig_signal muxig_network::create_maj( signal const& a, signal const& b, signal const& c )
{
  auto sel = !create_xor( a, b );
  return create_gate( sel, a, c );
}

template<>
inline muxig_signal muxig_network::create_ite( signal a, signal b, signal c )
{
  return create_gate( a, b, c );
}

template<>
inline bool muxig_network::is_ite( node const& n ) const
{
  return true;
}

template<>
inline bool muxig_network::is_mux( node const& n ) const
{
  return true;
}

template<>
inline kitty::dynamic_truth_table muxig_network::node_function( const node& n ) const
{
  (void)n;
  kitty::dynamic_truth_table tt( 3 );
  tt._bits[0] = 0xd8;
  return tt;
}

} // namespace mockturtle
