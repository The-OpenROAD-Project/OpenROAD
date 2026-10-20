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
  \file partial_truth_table.hpp
  \brief Implements partial_truth_table

  \author Siang-Yun Lee
*/

#pragma once

#include "detail/constants.hpp"
#include "traits.hpp"

#include <cstdint>
#include <type_traits>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>

namespace kitty
{

/*! Truth table with resizable, arbitrary number of bits
 */
struct partial_truth_table
{
  /*! \brief Standard constructor.
    \param num_bits Number of bits in use initially
  */
  explicit partial_truth_table( uint32_t num_bits )
      : _bits( num_bits ? ( ( ( num_bits - 1 ) >> 6 ) + 1 ) : 0 ),
        _num_bits( num_bits )
  {
  }

  /*! \brief Empty constructor.

    Creates an empty truth table. It has no bit in use. This constructor is
    only used for convenience, if algorithms require the existence of default
    constructable classes.
   */
  partial_truth_table() : _num_bits( 0 ) {}

  /*! \brief Constructs a new partial truth table instance with the same number of bits and blocks. */
  inline partial_truth_table construct() const
  {
    return partial_truth_table( _num_bits );
  }

  /*! \brief Returns number of (allocated) blocks.
   */
  inline auto num_blocks() const noexcept { return _bits.size(); }

  /*! \brief Returns number of (used) bits.
   */
  inline auto num_bits() const noexcept { return _num_bits; }

  /*! \brief Begin iterator to bits.
   */
  inline auto begin() noexcept { return _bits.begin(); }

  /*! \brief End iterator to bits.
   */
  inline auto end() noexcept { return _bits.end(); }

  /*! \brief Begin iterator to bits.
   */
  inline auto begin() const noexcept { return _bits.begin(); }

  /*! \brief End iterator to bits.
   */
  inline auto end() const noexcept { return _bits.end(); }

  /*! \brief Reverse begin iterator to bits.
   */
  inline auto rbegin() noexcept { return _bits.rbegin(); }

  /*! \brief Reverse end iterator to bits.
   */
  inline auto rend() noexcept { return _bits.rend(); }

  /*! \brief Constant begin iterator to bits.
   */
  inline auto cbegin() const noexcept { return _bits.cbegin(); }

  /*! \brief Constant end iterator to bits.
   */
  inline auto cend() const noexcept { return _bits.cend(); }

  /*! \brief Constant reverse begin iterator to bits.
   */
  inline auto crbegin() const noexcept { return _bits.crbegin(); }

  /*! \brief Constant teverse end iterator to bits.
   */
  inline auto crend() const noexcept { return _bits.crend(); }

  /*! \brief Assign other truth table.

    This replaces the current truth table with another truth table.  The truth
    table type is arbitrary.  The vector of bits is resized accordingly.

    \param other Other truth table
  */
  template<class TT, typename = std::enable_if_t<is_truth_table<TT>::value>>
  partial_truth_table& operator=( const TT& other )
  {
    _bits.resize( other.num_blocks() );
    std::copy( other.begin(), other.end(), begin() );
    _num_bits = 1 << other.num_vars();

    return *this;
  }

  /*! \brief Masks the number of valid truth table bits.

    If not all the bits in the last block are used up,
    we block out the remaining bits (fill with zero).
    Bits are used from LSB.
  */
  inline void mask_bits() noexcept
  {
    if ( _num_bits & 0x3f )
    {
      _bits.back() &= 0xFFFFFFFFFFFFFFFF >> ( 64 - ( _num_bits & 0x3f ) );
    }
  }

  /*! \brief Resize the truth table to have the given number of bits.

    If the desired length is larger than the current length, zeros will
    be filled up in the end of the truth table. If the desired length
    is shorter than the current length, extra bits in the end of the
    truth table will be discarded.

    \param num_bits Desired length (number of bits)
  */
  inline void resize( int num_bits ) noexcept
  {
    _num_bits = num_bits;

    unsigned needed_blocks = num_bits ? ( ( ( num_bits - 1 ) >> 6 ) + 1 ) : 0;
    _bits.resize( needed_blocks, 0u );

    mask_bits();
  }

  /*! \brief Add a bit to the end of the truth table.
    \param bit Bit to be added
  */
  inline void add_bit( bool bit ) noexcept
  {
    resize( _num_bits + 1 );
    if ( bit )
    {
      _bits.back() |= (uint64_t)1 << ( ( _num_bits & 0x3f ) - 1 );
    }
  }

  /*! \brief Add several bits to the end of the truth table.
    \param bits Bits to be added
  */
  inline void add_bits( std::vector<bool>& bits ) noexcept
  {
    for ( unsigned i = 0; i < bits.size(); ++i )
    {
      add_bit( bits.at( i ) );
    }
  }

  /*! \brief Add several bits to the end of the truth table.
    \param bits Bits to be added
    \param num_bits Number of bits in `bits` to be added (counted from LSB)
  */
  inline void add_bits( uint64_t bits, int num_bits = 64 ) noexcept
  {
    assert( num_bits <= 64 );

    if ( ( _num_bits & 0x3f ) + num_bits <= 64 ) /* no need for a new block */
    {
      if ( _bits.size() == 0u )
      {
        _bits.emplace_back( 0u );
      }
      _bits.back() |= bits << ( _num_bits & 0x3f );
    }
    else
    {
      auto first_half_len = 64 - ( _num_bits & 0x3f );
      _bits.back() |= bits << ( _num_bits & 0x3f );
      _bits.emplace_back( 0u );
      _bits.back() |= ( bits & ( 0xFFFFFFFFFFFFFFFF >> ( 64 - num_bits ) ) ) >> first_half_len;
    }
    _num_bits += num_bits;
  }

  /*! \brief Overwrite the value at position `from` to position `to`.
    \param from Position of the bit to be copied
    \param to Position of the bit to be overwritten
  */
  inline void copy_bit( uint32_t from, uint32_t to )
  {
    if ( ( _bits[from >> 6] >> ( from & 0x3f ) ) & 0x1 )
    {
      /* set_bit( to ) */
      _bits[to >> 6] |= uint64_t( 1 ) << ( to & 0x3f );
    }
    else
    {
      /* clear_bit( to ) */
      _bits[to >> 6] &= ~( uint64_t( 1 ) << ( to & 0x3f ) );
    }
  }

  /*! \brief Erase the bit at the given position by swapping with the
             last bit and shrinking the truth table.

    This method is faster than `erase_bit` but do not keep the order
    of the bits.

    \param position Position of the bit to be erased
  */
  inline void erase_bit_swap( uint32_t position )
  {
    assert( position < _num_bits );
    /* move the last bit (`_num_bits - 1`) to `position` */
    copy_bit( _num_bits - 1, position );
    resize( _num_bits - 1 );
  }

  /*! \brief Erase the bit at the given position and shift all the following bits.

    This method keeps the order of the bits but is slower and
    do not keep the position of the bits.

    \param position Position of the bit to be erased
  */
  inline void erase_bit_shift( uint32_t position )
  {
    assert( position < _num_bits );
    uint64_t mask = 0xFFFFFFFFFFFFFFFF << ( position & 0x3f );
    _bits[position >> 6] = ( _bits[position >> 6] & ~mask ) | ( _bits[position >> 6] >> 1 & mask );
    uint32_t hole = position | 0x3f;
    while ( hole < _num_bits - 1 )
    {
      copy_bit( hole + 1, hole );
      hole += 64;
      _bits[hole >> 6] >>= 1;
    }
    resize( _num_bits - 1 );
  }

  /*! \cond PRIVATE */
public: /* fields */
  std::vector<uint64_t> _bits;
  uint32_t _num_bits;
  /*! \endcond */
};

template<>
struct is_truth_table<kitty::partial_truth_table> : std::true_type
{
};

template<>
struct is_complete_truth_table<kitty::partial_truth_table> : std::false_type
{
};

template<>
struct is_completely_specified_truth_table<kitty::partial_truth_table> : std::true_type
{
};

} // namespace kitty