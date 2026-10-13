/* kitty: C++ truth table library
 * Copyright (C) 2017-2025  EPFL
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
  \file algorithm.hpp
  \brief Implements several generic algorithms on truth tables

  \author Mathias Soeken
*/

#pragma once

#include "bit_operations.hpp"
#include "detail/constants.hpp"
#include "static_truth_table.hpp"
#include "partial_truth_table.hpp"

#include <algorithm>
#include <cassert>

namespace kitty
{

/*! \brief Perform bitwise unary operation on truth table

  \param tt Truth table
  \param op Unary operation that takes as input a word (`uint64_t`) and returns a word

  \return new constructed truth table of same type and dimensions
 */
template<typename TT, typename Fn>
auto unary_operation( const TT& tt, Fn&& op )
{
  auto result = tt.construct();
  std::transform( tt.cbegin(), tt.cend(), result.begin(), op );
  result.mask_bits();
  return result;
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
auto unary_operation( const static_truth_table<NumVars, true>& tt, Fn&& op )
{
  auto result = tt.construct();
  result._bits = op( tt._bits );
  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \cond PRIVATE */
/*!
    \param num_blocks_offset Number of blocks that don't need to be computed
    (the first `num_blocks_offset` blocks of `result` will remain the same before computation)
 */
template<typename Fn>
void unary_operation( partial_truth_table& result, const partial_truth_table& tt, Fn&& op, int const& num_blocks_offset = 0 )
{
  result.resize( tt.num_bits() );
  std::transform( tt.cbegin() + num_blocks_offset, tt.cend(), result.begin() + num_blocks_offset, op );
}
/*! \endcond */

/*! \brief Perform bitwise binary operation on two truth tables

  The dimensions of `first` and `second` must match.  This is ensured
  at compile-time for static truth tables, but at run-time for dynamic
  truth tables.

  \param first First truth table
  \param second Second truth table
  \param op Binary operation that takes as input two words (`uint64_t`) and returns a word

  \return new constructed truth table of same type and dimensions
 */
template<typename TT, typename Fn>
auto binary_operation( const TT& first, const TT& second, Fn&& op )
{
  assert( first.num_vars() == second.num_vars() );

  auto result = first.construct();
  std::transform( first.cbegin(), first.cend(), second.cbegin(), result.begin(), op );
  result.mask_bits();
  return result;
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
auto binary_operation( const static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second, Fn&& op )
{
  auto result = first.construct();
  result._bits = op( first._bits, second._bits );
  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \cond PRIVATE */
template<typename Fn>
auto binary_operation( const partial_truth_table& first, const partial_truth_table& second, Fn&& op )
{
  assert( first.num_bits() == second.num_bits() );

  auto result = first.construct();
  std::transform( first.cbegin(), first.cend(), second.cbegin(), result.begin(), op );
  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \cond PRIVATE */
/*!
    \param num_blocks_offset Number of blocks that don't need to be computed
    (the first `num_blocks_offset` blocks of `result` will remain the same before computation)
 */
template<typename Fn>
void binary_operation( partial_truth_table& result, const partial_truth_table& first, const partial_truth_table& second, Fn&& op, int const& num_blocks_offset = 0 )
{
  assert( first.num_bits() == second.num_bits() );

  result.resize( first.num_bits() );
  std::transform( first.cbegin() + num_blocks_offset, first.cend(), second.cbegin() + num_blocks_offset, result.begin() + num_blocks_offset, op );
}
/*! \endcond */

/*! \brief Perform bitwise ternary operation on three truth tables

  The dimensions of `first`, `second`, and `third` must match.  This
  is ensured at compile-time for static truth tables, but at run-time
  for dynamic truth tables.

  \param first First truth table
  \param second Second truth table
  \param third Third truth table
  \param op Ternary operation that takes as input three words (`uint64_t`) and returns a word

  \return new constructed truth table of same type and dimensions
 */
template<typename TT, typename Fn>
auto ternary_operation( const TT& first, const TT& second, const TT& third, Fn&& op )
{
  assert( first.num_vars() == second.num_vars() && second.num_vars() == third.num_vars() );

  auto result = first.construct();
  auto it1 = first.cbegin();
  const auto it1_e = first.cend();
  auto it2 = second.cbegin();
  auto it3 = third.cbegin();
  auto it = result.begin();

  while ( it1 != it1_e )
  {
    *it++ = op( *it1++, *it2++, *it3++ );
  }

  result.mask_bits();
  return result;
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
auto ternary_operation( const static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second, const static_truth_table<NumVars, true>& third, Fn&& op )
{
  auto result = first.construct();
  result._bits = op( first._bits, second._bits, third._bits );
  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \cond PRIVATE */
template<typename Fn>
auto ternary_operation( const partial_truth_table& first, const partial_truth_table& second, const partial_truth_table& third, Fn&& op )
{
  assert( first.num_bits() == second.num_bits() && second.num_bits() == third.num_bits() );

  auto result = first.construct();
  auto it1 = first.cbegin();
  const auto it1_e = first.cend();
  auto it2 = second.cbegin();
  auto it3 = third.cbegin();
  auto it = result.begin();

  while ( it1 != it1_e )
  {
    *it++ = op( *it1++, *it2++, *it3++ );
  }

  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \brief Perform bitwise quaternary operation on four truth tables

  The dimensions of all truth tables must match.  This
  is ensured at compile-time for static truth tables, but at run-time
  for dynamic truth tables.

  \param first First truth table
  \param second Second truth table
  \param third Third truth table
  \param fourth Fourth truth table
  \param op Quaternary operation that takes as input four words (`uint64_t`) and returns a word

  \return new constructed truth table of same type and dimensions
 */
template<typename TT, typename Fn>
auto quaternary_operation( const TT& first, const TT& second, const TT& third, const TT& fourth, Fn&& op )
{
  assert( first.num_vars() == second.num_vars() && second.num_vars() == third.num_vars() && third.num_vars() == fourth.num_vars() );

  auto result = first.construct();
  auto it1 = first.cbegin();
  const auto it1_e = first.cend();
  auto it2 = second.cbegin();
  auto it3 = third.cbegin();
  auto it4 = fourth.cbegin();
  auto it = result.begin();

  while ( it1 != it1_e )
  {
    *it++ = op( *it1++, *it2++, *it3++, *it4++ );
  }

  result.mask_bits();
  return result;
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
auto quaternary_operation( const static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second, const static_truth_table<NumVars, true>& third, const static_truth_table<NumVars, true>& fourth, Fn&& op )
{
  auto result = first.construct();
  result._bits = op( first._bits, second._bits, third._bits, fourth._bits );
  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \cond PRIVATE */
template<typename Fn>
auto quaternary_operation( const partial_truth_table& first, const partial_truth_table& second, const partial_truth_table& third, const partial_truth_table& fourth, Fn&& op )
{
  assert( first.num_bits() == second.num_bits() && second.num_bits() == third.num_bits() && third.num_bits() == fourth.num_bits() );

  auto result = first.construct();
  auto it1 = first.cbegin();
  const auto it1_e = first.cend();
  auto it2 = second.cbegin();
  auto it3 = third.cbegin();
  auto it4 = fourth.cbegin();
  auto it = result.begin();

  while ( it1 != it1_e )
  {
    *it++ = op( *it1++, *it2++, *it3++, *it4++ );
  }

  result.mask_bits();
  return result;
}
/*! \endcond */

/*! \brief Computes a predicate based on two truth tables

  The dimensions of `first` and `second` must match.  This is ensured
  at compile-time for static truth tables, but at run-time for dynamic
  truth tables.

  \param first First truth table
  \param second Second truth table
  \param op Binary operation that takes as input two words (`uint64_t`) and returns a Boolean

  \return true or false based on the predicate
 */
template<typename TT, typename Fn>
bool binary_predicate( const TT& first, const TT& second, Fn&& op )
{
  assert( first.num_vars() == second.num_vars() );

  return std::equal( first.begin(), first.end(), second.begin(), op );
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
bool binary_predicate( const static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second, Fn&& op )
{
  return op( first._bits, second._bits );
}
/*! \endcond */

/*! \cond PRIVATE */
template<typename Fn>
bool binary_predicate( const partial_truth_table& first, const partial_truth_table& second, Fn&& op )
{
  assert( first.num_bits() == second.num_bits() );

  return std::equal( first.begin(), first.end(), second.begin(), op );
}
/*! \endcond */

/*! \brief Computes a predicate based on three truth tables

  The dimensions of `first`, `second` and `third` must match.  This is ensured
  at compile-time for static truth tables, but at run-time for dynamic
  truth tables.

  \param first First truth table
  \param second Second truth table
  \param third Third truth table
  \param op Ternary operation that takes as input three words (`uint64_t`) and returns a Boolean

  \return true or false based on the predicate
 */
template<typename TT, typename Fn>
bool ternary_predicate( const TT& first, const TT& second, const TT& third, Fn&& op )
{
  assert( first.num_blocks() == second.num_blocks() && first.num_blocks() == third.num_blocks() );

  for ( auto i = 0u; i < first.num_blocks(); ++i )
  {
    if ( !op( first._bits[i], second._bits[i], third._bits[i] ) )
    {
      return false;
    }
  }
  return true;
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
bool ternary_predicate( const static_truth_table<NumVars, true>& first, const static_truth_table<NumVars, true>& second, const static_truth_table<NumVars, true>& third, Fn&& op )
{
  return op( first._bits, second._bits, third._bits );
}
/*! \endcond */

/*! \brief Assign computed values to bits

  The functor `op` computes bits which are assigned to the bits of the
  truth table.

  \param tt Truth table
  \param op Unary operation that takes no input and returns a word (`uint64_t`)
*/
template<typename TT, typename Fn>
void assign_operation( TT& tt, Fn&& op )
{
  std::generate( tt.begin(), tt.end(), op );
  tt.mask_bits();
}

/*! \cond PRIVATE */
template<uint32_t NumVars, typename Fn>
void assign_operation( static_truth_table<NumVars, true>& tt, Fn&& op )
{
  tt._bits = op();
  tt.mask_bits();
}
/*! \endcond */

/*! \brief Iterates through each block of a truth table

 The functor `op` is called for every block of the truth table.

 \param tt Truth table
 \param op Unary operation that takes as input a word (`uint64_t`) and returns void
*/
template<typename TT, typename Fn>
void for_each_block( const TT& tt, Fn&& op )
{
  std::for_each( tt.cbegin(), tt.cend(), op );
}

/*! \brief Iterates through each block of a truth table in reverse
    order

 The functor `op` is called for every block of the truth table in
 reverse order.

 \param tt Truth table
 \param op Unary operation that takes as input a word (`uint64_t`) and returns void
*/
template<typename TT, typename Fn>
void for_each_block_reversed( const TT& tt, Fn&& op )
{
  std::for_each( tt.crbegin(), tt.crend(), op );
}

/*! \cond PRIVATE */
template<typename TT, typename Fn>
void for_each_one_bit_naive( const TT& tt, Fn&& op )
{
  for ( uint64_t bit = 0u; bit < tt.num_bits(); ++bit )
  {
    if ( get_bit( tt, bit ) )
    {
      op( bit );
    }
  }
}
/*! \endcond */

/*! \cond PRIVATE */
template<typename TT, typename Fn>
void for_each_one_bit_jump( const TT& tt, Fn&& op )
{
  uint64_t offset = 0, low_bit, value;

  for ( auto block : tt._bits )
  {
    while ( block )
    {
      low_bit = value = block - ( block & ( block - 1 ) );

      value |= value >> 1;
      value |= value >> 2;
      value |= value >> 4;
      value |= value >> 8;
      value |= value >> 16;
      value |= value >> 32;
      op( offset + detail::de_bruijn64[( static_cast<uint64_t>( ( value - ( value >> 1 ) ) * UINT64_C( 0x07EDD5E59A4E28C2 ) ) ) >> 58] );

      block ^= low_bit;
    }
    offset += 64;
  }
}

template<uint32_t NumVars, typename Fn>
void for_each_one_bit_jump( const static_truth_table<NumVars, true>& tt, Fn&& op )
{
  uint64_t block = tt._bits;

  while ( block )
  {
    uint64_t low_bit = block - ( block & ( block - 1 ) );
    uint64_t value = low_bit;

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    op( detail::de_bruijn64[( static_cast<uint64_t>( ( value - ( value >> 1 ) ) * UINT64_C( 0x07EDD5E59A4E28C2 ) ) ) >> 58] );

    block ^= low_bit;
  }
}
/*! \endcond */

/*! \brief Iterates through each 1-bit in the truth table

  The functor `op` is called for every bit position of the truth table
  for which the bit is assigned 1.

  \param tt Truth table
  \param op Unary operation that takes as input a word (`uint64_t`) and returns void
*/
template<typename TT, typename Fn>
inline void for_each_one_bit( const TT& tt, Fn&& op )
{
  for_each_one_bit_naive( tt, op );
}
} /* namespace kitty */