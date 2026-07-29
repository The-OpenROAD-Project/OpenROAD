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
  \file bit_operations.hpp
  \brief Implements bit manipulation on truth tables

  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <numeric>

#include "detail/mscfix.hpp"
#include "partial_truth_table.hpp"
#include "quaternary_truth_table.hpp"
#include "static_truth_table.hpp"
#include "ternary_truth_table.hpp"

namespace kitty
{

/*! \brief Sets bit at index to true

  \param tt Truth table
  \param index Bit index
*/
template<typename TT>
void set_bit( TT& tt, uint64_t index )
{
  tt._bits[index >> 6] |= uint64_t( 1 ) << ( index & 0x3f );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void set_bit( static_truth_table<NumVars, true>& tt, uint64_t index )
{
  tt._bits |= uint64_t( 1 ) << index;
}

template<typename TT>
void set_bit( ternary_truth_table<TT>& tt, uint64_t index, bool value = true )
{
  set_bit( tt._care, index );
  if ( value )
    set_bit( tt._bits, index );
  else
    clear_bit( tt._bits, index );
}

template<typename TT>
void set_bit( quaternary_truth_table<TT>& tt, uint64_t index, bool value = true )
{
  if ( value )
  {
    set_bit( tt._onset, index );
    clear_bit( tt._offset, index );
  }
  else
  {
    clear_bit( tt._onset, index );
    set_bit( tt._offset, index );
  }
}

/*! \endcond */

/*! \brief Gets bit at index

  \param tt Truth table
  \param index Bit index

  \return 1 if bit is set, otherwise 0
*/
template<typename TT>
auto get_bit( const TT& tt, uint64_t index )
{
  return ( tt._bits[index >> 6] >> ( index & 0x3f ) ) & 0x1;
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
auto get_bit( const static_truth_table<NumVars, true>& tt, uint64_t index )
{
  return ( tt._bits >> index ) & 0x1;
}

/*! \brief Gets bit at index

\param tt Ternary truth table
\param index Bit index

\return 1 if bit is set, 0 if it is reset or if it is a don't care
*/
template<typename TT>
auto get_bit( const ternary_truth_table<TT>& tt, uint64_t index )
{
  return get_bit( tt._bits, index );
}

/*! \brief Gets bit at index

\param tt Ternary truth table
\param index Bit index

\return 1 if bit is set, 0 if it is reset, if it is a don't care, or if it is a don't know
*/
template<typename TT>
auto get_bit( const quaternary_truth_table<TT>& tt, uint64_t index )
{
  return get_bit( tt._onset, index ) && !get_bit( tt._offset, index );
}

/*! \endcond */

/*! \brief Clears bit at index (sets bit at index to false)

  \param tt Truth table
  \param index Bit index
*/
template<typename TT>
void clear_bit( TT& tt, uint64_t index )
{
  tt._bits[index >> 6] &= ~( uint64_t( 1 ) << ( index & 0x3f ) );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void clear_bit( static_truth_table<NumVars, true>& tt, uint64_t index )
{
  tt._bits &= ~( uint64_t( 1 ) << index );
}
/*! \endcond */

/*! \brief Flips bit at index

  \param tt Truth table
  \param index Bit index
*/
template<typename TT>
void flip_bit( TT& tt, uint64_t index )
{
  tt._bits[index >> 6] ^= uint64_t( 1 ) << ( index & 0x3f );
}

template<typename TT>
void flip_bit( ternary_truth_table<TT>& tt, uint64_t index )
{
  if ( !get_bit( tt._care, index ) )
    return;
  flip_bit( tt._bits, index );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void flip_bit( static_truth_table<NumVars, true>& tt, uint64_t index )
{
  tt._bits ^= uint64_t( 1 ) << index;
}

/*! \brief Checks if a bit in a ternary truth table is a don't care

  \param tt Ternary truth table
  \param index Bit index
*/
template<typename TT>
bool is_dont_care( const ternary_truth_table<TT>& tt, uint64_t index )
{
  return !get_bit( tt._care, index );
}

/*! \brief Checks if a bit in a quaternary truth table is a don't care

  \param tt Quaternary truth table
  \param index Bit index
*/
template<typename TT>
bool is_dont_care( const quaternary_truth_table<TT>& tt, uint64_t index )
{
  return get_bit( tt._onset, index ) && get_bit( tt._offset, index );
}

/*! \cond PRIVATE */
template<typename TT>
bool is_dont_care( const TT& tt, uint64_t index )
{
  (void)tt;
  (void)index;
  return false;
}

/*! \brief Sets a bit in a ternary truth table as a don't care

  \param tt Ternary truth table
  \param index Bit index
*/
template<typename TT>
void set_dont_care( ternary_truth_table<TT>& tt, uint64_t index )
{
  clear_bit( tt._care, index );
  clear_bit( tt._bits, index );
}

/*! \brief Sets a bit in a quaternary truth table as a don't care

  \param tt Ternary truth table
  \param index Bit index
*/
template<typename TT>
void set_dont_care( quaternary_truth_table<TT>& tt, uint64_t index )
{
  set_bit( tt._onset, index );
  set_bit( tt._offset, index );
}

/*! \brief Checks if a bit in a ternary truth table is a don't know

  \param tt Ternary truth table
  \param index Bit index
*/
template<typename TT>
bool is_dont_know( const ternary_truth_table<TT>& tt, uint64_t index )
{
  return !get_bit( tt._care, index );
}

/*! \brief Checks if a bit in a quaternary truth table is a don't care

  \param tt Ternary truth table
  \param index Bit index
*/
template<typename TT>
bool is_dont_know( const quaternary_truth_table<TT>& tt, uint64_t index )
{
  return !get_bit( tt._onset, index ) && !get_bit( tt._offset, index );
}

/*! \cond PRIVATE */
template<typename TT>
bool is_dont_know( const TT& tt, uint64_t index )
{
  (void)tt;
  (void)index;
  return false;
}

template<typename TT>
void set_dont_know( quaternary_truth_table<TT>& tt, uint64_t index )
{
  clear_bit( tt._onset, index );
  clear_bit( tt._offset, index );
}

/*! Returns a block of bits vector.
 */
template<typename TT>
uint64_t get_block( const TT& tt, uint64_t block_index )
{
  assert( block_index < tt.num_blocks() );
  return tt._bits[block_index];
}

template<uint32_t NumVars>
uint64_t get_block( const static_truth_table<NumVars, true>& tt, uint64_t block_index )
{
  assert( block_index == 0 );
  (void)block_index;
  return tt._bits;
}

/*! \brief Copies bit at index

  Copy the bit from `tt_from` at index `index_from` to `tt_to` at index `index_to`.

  \param tt_from Truth table to copy from
  \param index_from Bit index to copy from
  \param tt_to Truth table to write to
  \param index_to Bit index to write to
*/
template<typename TTfrom, typename TTto>
void copy_bit( const TTfrom& tt_from, uint64_t index_from, TTto& tt_to, uint64_t index_to )
{
  if ( get_bit( tt_from, index_from ) )
  {
    set_bit( tt_to, index_to );
  }
  else
  {
    clear_bit( tt_to, index_to );
  }
}

/*! \brief Clears all bits

  \param tt Truth table
*/
template<typename TT>
void clear( TT& tt )
{
  std::fill( std::begin( tt._bits ), std::end( tt._bits ), 0 );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
void clear( static_truth_table<NumVars, true>& tt )
{
  tt._bits = 0;
}
/*! \endcond */

/*! \brief Count ones in truth table

  \param tt Truth table
*/
template<typename TT>
inline uint64_t count_ones( const TT& tt )
{
  return std::accumulate( tt.cbegin(), tt.cend(), uint64_t( 0 ),
                          []( auto accu, auto word ) {
                            return accu + __builtin_popcountll( word );
                          } );
}

/*! \cond PRIVATE */
template<uint32_t NumVars>
inline uint64_t count_ones( const static_truth_table<NumVars, true>& tt )
{
  return __builtin_popcountll( tt._bits );
}

template<typename TT>
inline uint64_t count_ones( const ternary_truth_table<TT>& tt )
{
  return count_ones( tt._bits & tt._care );
}
/*! \endcond */

/*! \brief Count zeros in truth table

  \param tt Truth table
*/
template<typename TT>
inline uint64_t count_zeros( const TT& tt )
{
  return count_ones( ~tt );
}

/*! \cond PRIVATE */
inline int64_t find_first_bit_in_word( uint64_t word )
{
  int64_t n = 0;
  if ( word == 0 )
  {
    return -1;
  }

  if ( ( word & UINT64_C( 0x00000000FFFFFFFF ) ) == 0 )
  {
    n += 32;
    word >>= 32;
  }
  if ( ( word & UINT64_C( 0x000000000000FFFF ) ) == 0 )
  {
    n += 16;
    word >>= 16;
  }
  if ( ( word & UINT64_C( 0x00000000000000FF ) ) == 0 )
  {
    n += 8;
    word >>= 8;
  }
  if ( ( word & UINT64_C( 0x000000000000000F ) ) == 0 )
  {
    n += 4;
    word >>= 4;
  }
  if ( ( word & UINT64_C( 0x0000000000000003 ) ) == 0 )
  {
    n += 2;
    word >>= 2;
  }
  if ( ( word & UINT64_C( 0x0000000000000001 ) ) == 0 )
  {
    n++;
  }

  return n;
}

inline int64_t find_last_bit_in_word( uint64_t word )
{
  int64_t n = 0;
  if ( word == 0 )
  {
    return -1;
  }

  if ( ( word & UINT64_C( 0xFFFFFFFF00000000 ) ) == 0 )
  {
    n += 32;
    word <<= 32;
  }
  if ( ( word & UINT64_C( 0xFFFF000000000000 ) ) == 0 )
  {
    n += 16;
    word <<= 16;
  }
  if ( ( word & UINT64_C( 0xFF00000000000000 ) ) == 0 )
  {
    n += 8;
    word <<= 8;
  }
  if ( ( word & UINT64_C( 0xF000000000000000 ) ) == 0 )
  {
    n += 4;
    word <<= 4;
  }
  if ( ( word & UINT64_C( 0xC000000000000000 ) ) == 0 )
  {
    n += 2;
    word <<= 2;
  }
  if ( ( word & UINT64_C( 0x8000000000000000 ) ) == 0 )
  {
    n++;
  }

  return 63 - n;
}
/*! \endcond */

/*! \brief Finds least-significant one-bit

  Returns -1, if truth table is constant 0.

  \param tt Truth table
  \param start Bit to start from (default is 0)
*/
template<typename TT>
int64_t find_first_one_bit( const TT& tt, int64_t start = 0 )
{
  uint64_t mask = ~( ( UINT64_C( 1 ) << uint64_t( start % 64 ) ) - 1u );
  auto it = tt.cbegin() + ( start >> 6 );
  if ( it != tt.cend() && ( *it & mask ) != 0 )
  {
    return 64 * std::distance( tt.cbegin(), it ) + find_first_bit_in_word( *it & mask );
  }
  else if ( it == tt.cend() )
  {
    return -1;
  }

  it = std::find_if( it + 1, tt.cend(), []( auto word ) { return word != 0; } );

  if ( it == tt.cend() )
  {
    return -1;
  }

  return 64 * std::distance( tt.cbegin(), it ) + find_first_bit_in_word( *it );
}

/*! \brief Finds most-significant one-bit

  Returns -1, if truth table is constant 0.

  \param tt Truth table
*/
template<typename TT>
int64_t find_last_one_bit( const TT& tt )
{
  const auto it = std::find_if( tt.crbegin(), tt.crend(), []( auto word ) { return word != 0; } );

  if ( it == tt.crend() )
  {
    return -1;
  }

  return 64 * ( std::distance( it, tt.crend() ) - 1 ) + find_last_bit_in_word( *it );
}

/*! \brief Finds least-significant bit difference

  Returns -1, if truth tables are the same

  \param first First truth table
  \param second Second truth table
*/
template<typename TT>
int64_t find_first_bit_difference( const TT& first, const TT& second )
{
  if constexpr ( std::is_same<TT, partial_truth_table>::value )
  {
    assert( first.num_bits() == second.num_bits() );
  }
  else
  {
    assert( first.num_vars() == second.num_vars() );
  }

  auto it = first.cbegin();
  auto it2 = second.cbegin();
  auto w = 0;

  while ( it != first.cend() )
  {
    if ( *it ^ *it2 )
    {
      return 64 * w + find_first_bit_in_word( *it ^ *it2 );
    }
    ++it;
    ++it2;
    ++w;
  }
  return -1;
}

/*! \brief Finds most-significant bit difference

  Returns -1, if truth tables are the same

  \param first First truth table
  \param second Second truth table
*/
template<typename TT>
int64_t find_last_bit_difference( const TT& first, const TT& second )
{
  if constexpr ( std::is_same<TT, partial_truth_table>::value )
  {
    assert( first.num_bits() == second.num_bits() );
  }
  else
  {
    assert( first.num_vars() == second.num_vars() );
  }

  auto it = first.crbegin();
  auto it2 = second.crbegin();
  auto w = first.num_blocks() - 1;

  while ( it != first.crend() )
  {
    if ( *it ^ *it2 )
    {
      return 64 * w + find_last_bit_in_word( *it ^ *it2 );
    }
    ++it;
    ++it2;
    --w;
  }
  return -1;
}

} // namespace kitty