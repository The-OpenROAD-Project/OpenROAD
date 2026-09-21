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
  \file quaternary_truth_table.hpp
  \brief Implements quaternary_truth_table

  \author Siang-Yun Lee
  \author Gianluca Radi
*/

#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

#include "detail/constants.hpp"
#include "traits.hpp"

namespace kitty
{

template<class TT>
struct quaternary_truth_table
{
  /*! \brief Standard constructor.

    Initialize the truth table using the constructor of the inner
    truth table type.

    \param n Number of variables or number of bits (when `TT = partial_truth_table`)
  */
  explicit quaternary_truth_table( uint32_t n )
      : _onset( n ), _offset( n )
  {
  }

  /*! \brief Empty constructor.

    Creates an empty truth table by calling the empty constructor
    of the inner truth table type.
   */
  quaternary_truth_table() : _onset(), _offset() {}

  /*! \brief Construct from onset and offset.

    Meanings of the values of (offset, onset) at each bit position:
    00 is a don't-know (x) or not involved in the cube,
    01 is a positive literal (1), 10 is a negative literal (0), and
    11 is a don't-care (-), meaning that both 0 and 1 are accepted.

    \param onset Onset truth table.
    \param offset Offset truth table.
  */
  quaternary_truth_table( TT const& onset, TT const& offset )
      : _onset( onset ), _offset( offset )
  {
  }

  /*! \brief Construct from a binary truth table.

    Initialize the truth table as equivalent to a binary truth table.
    (All bits are cared and known, being either `0` or `1`, without any
    `-` or `x`.)

    \param binary Binary truth table.
  */
  quaternary_truth_table( TT const& binary )
      : _onset( binary ), _offset( ~binary )
  {
  }

  /*! Constructs a new quaternary_truth_table instance of the same size. */
  inline quaternary_truth_table<TT> construct() const
  {
    if constexpr ( std::is_same_v<TT, partial_truth_table> )
      return quaternary_truth_table<TT>( _onset.num_bits() );
    else
      return quaternary_truth_table<TT>( _onset.num_vars() );
  }

  /*! Returns number of variables.
   */
  template<typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
  auto num_vars() const noexcept { return _onset.num_vars(); }

  /*! Returns number of blocks.
   */
  inline auto num_blocks() const noexcept { return _onset.num_blocks(); }

  /*! Returns number of bits.
   */
  inline auto num_bits() const noexcept { return _onset.num_bits(); }

  /*! \brief Begin iterator to onset.
   */
  inline auto begin_onset() noexcept { return _onset.begin(); }

  /*! \brief End iterator to onset.
   */
  inline auto end_onset() noexcept { return _onset.end(); }

  /*! \brief Begin iterator to onset.
   */
  inline auto begin_onset() const noexcept { return _onset.begin(); }

  /*! \brief End iterator to onset.
   */
  inline auto end_onset() const noexcept { return _onset.end(); }

  /*! \brief Reverse begin iterator to onset.
   */
  inline auto rbegin_onset() noexcept { return _onset.rbegin(); }

  /*! \brief Reverse end iterator to onset.
   */
  inline auto rend_onset() noexcept { return _onset.rend(); }

  /*! \brief Constant begin iterator to onset.
   */
  inline auto cbegin_onset() const noexcept { return _onset.cbegin(); }

  /*! \brief Constant end iterator to onset.
   */
  inline auto cend_onset() const noexcept { return _onset.cend(); }

  /*! \brief Constant reverse begin iterator to onset.
   */
  inline auto crbegin_onset() const noexcept { return _onset.crbegin(); }

  /*! \brief Constant teverse end iterator to onset.
   */
  inline auto crend_onset() const noexcept { return _onset.crend(); }

  /*! \brief Begin iterator to offset.
   */
  inline auto begin_offset() noexcept { return _offset.begin(); }

  /*! \brief End iterator to offset.
   */
  inline auto end_offset() noexcept { return _offset.end(); }

  /*! \brief Begin iterator to offset.
   */
  inline auto begin_offset() const noexcept { return _offset.begin(); }

  /*! \brief End iterator to offset.
   */
  inline auto end_offset() const noexcept { return _offset.end(); }

  /*! \brief Reverse begin iterator to offset.
   */
  inline auto rbegin_offset() noexcept { return _offset.rbegin(); }

  /*! \brief Reverse end iterator to offset.
   */
  inline auto rend_offset() noexcept { return _offset.rend(); }

  /*! \brief Constant begin iterator to offset.
   */
  inline auto cbegin_offset() const noexcept { return _offset.cbegin(); }

  /*! \brief Constant end iterator to offset.
   */
  inline auto cend_offset() const noexcept { return _offset.cend(); }

  /*! \brief Constant reverse begin iterator to offset.
   */
  inline auto crbegin_offset() const noexcept { return _offset.crbegin(); }

  /*! \brief Constant teverse end iterator to offset.
   */
  inline auto crend_offset() const noexcept { return _offset.crend(); }

  /*! \brief Assign other truth table.

    This replaces the current truth table with another truth table.
    The other truth table must also be quaternary, and the inner type of
    the other truth table must be assignable to the inner type of this
    truth table.

    \param other Other truth table
  */
  template<class TT2>
  quaternary_truth_table<TT>& operator=( const quaternary_truth_table<TT2>& other )
  {
    _onset = other._onset;
    _offset = other._offset;

    return *this;
  }

  /*! \brief Assigned by a binary truth table.

    Replaces with a truth table equivalent to a binary truth table.
    (All bits are cared and known, being either `0` or `1`, without any
    `-` or `x`.)

    \param other Binary truth table.
  */
  template<class TT2>
  quaternary_truth_table<TT>& operator=( TT const& other )
  {
    _onset = other;
    _offset = ~other;

    return *this;
  }

  /*! \brief Masks valid truth table bits.

    This operation makes sure to zero out all unused bits.
  */
  inline void mask_bits() noexcept
  {
    _onset.mask_bits();
    _offset.mask_bits();
  }

  /*! \cond PRIVATE */
public: /* fields */
  TT _onset;
  TT _offset;
  /*! \endcond */
};

template<class TT>
struct is_truth_table<kitty::quaternary_truth_table<TT>> : is_truth_table<TT>
{
};

template<class TT>
struct is_complete_truth_table<kitty::quaternary_truth_table<TT>> : is_complete_truth_table<TT>
{
};

template<class TT>
struct is_completely_specified_truth_table<kitty::quaternary_truth_table<TT>> : std::false_type
{
};

} // namespace kitty
