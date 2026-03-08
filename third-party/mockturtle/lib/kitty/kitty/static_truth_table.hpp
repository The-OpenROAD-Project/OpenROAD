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
  \file static_truth_table.hpp
  \brief Implements static_truth_table

  \author Mathias Soeken
*/

#pragma once

#include <array>
#include <cstdint>

#include "detail/constants.hpp"
#include "traits.hpp"

namespace kitty
{

/*! Truth table in which number of variables is known at compile time.

  We dispatch on the Boolean template parameter to distinguish between
  a small truth table (up to 6 variables) and a large truth table
  (more than 6 variables).  A small truth table fits into a single
  block and therefore dedicated optimizations are possible.
 */
template<uint32_t NumVars, bool = ( NumVars <= 6 )>
struct static_truth_table;

/*! Truth table (for up to 6 variables) in which number of variables is known at compile time.
 */
template<uint32_t NumVars>
struct static_truth_table<NumVars, true>
{
  /*! \cond PRIVATE */
  enum
  {
    NumBits = uint64_t( 1 ) << NumVars
  };
  /*! \endcond */

  /*! Constructs a new static truth table instance with the same number of variables. */
  inline static_truth_table<NumVars> construct() const
  {
    return static_truth_table<NumVars>();
  }

  /*! Returns number of variables.
   */
  inline auto num_vars() const noexcept { return NumVars; }

  /*! Returns number of blocks.
   */
  inline auto num_blocks() const noexcept { return 1u; }

  /*! Returns number of bits.
   */
  inline auto num_bits() const noexcept { return NumBits; }

  /*! \brief Begin iterator to bits.
   */
  inline auto begin() noexcept { return &_bits; }

  /*! \brief End iterator to bits.
   */
  inline auto end() noexcept { return ( &_bits ) + 1; }

  /*! \brief Begin iterator to bits.
   */
  inline auto begin() const noexcept { return &_bits; }

  /*! \brief End iterator to bits.
   */
  inline auto end() const noexcept { return ( &_bits ) + 1; }

  /*! \brief Reverse begin iterator to bits.
   */
  inline auto rbegin() noexcept { return &_bits; }

  /*! \brief Reverse end iterator to bits.
   */
  inline auto rend() noexcept { return ( &_bits ) + 1; }

  /*! \brief Constant begin iterator to bits.
   */
  inline auto cbegin() const noexcept { return &_bits; }

  /*! \brief Constant end iterator to bits.
   */
  inline auto cend() const noexcept { return ( &_bits ) + 1; }

  /*! \brief Constant reverse begin iterator to bits.
   */
  inline auto crbegin() const noexcept { return &_bits; }

  /*! \brief Constant everse end iterator to bits.
   */
  inline auto crend() const noexcept { return ( &_bits ) + 1; }

  /*! \brief Assign other truth table if number of variables match.

    This replaces the current truth table with another truth table, if `other`
    has the same number of variables.  Otherwise, the truth table is not
    changed.

    \param other Other truth table
  */
  template<class TT, typename = std::enable_if_t<is_truth_table<TT>::value && is_complete_truth_table<TT>::value>>
  static_truth_table<NumVars>& operator=( const TT& other )
  {
    if ( other.num_vars() == num_vars() )
    {
      std::copy( other.begin(), other.end(), begin() );
    }

    return *this;
  }

  /*! Masks the number of valid truth table bits.

    If the truth table has less than 6 variables, it may not use all
    the bits.  This operation makes sure to zero out all non-valid
    bits.
  */
  inline void mask_bits() noexcept { _bits &= detail::masks[NumVars]; }

  /*! \cond PRIVATE */
public: /* fields */
  uint64_t _bits = 0;
  /*! \endcond */
};

/*! Truth table (more than 6 variables) in which number of variables is known at compile time.
 */
template<uint32_t NumVars>
struct static_truth_table<NumVars, false>
{
  /*! \cond PRIVATE */
  enum
  {
    NumBlocks = ( NumVars <= 6 ) ? 1u : ( 1u << ( NumVars - 6 ) )
  };

  enum
  {
    NumBits = uint64_t( 1 ) << NumVars
  };
  /*! \endcond */

  /*! Standard constructor.

    The number of variables provided to the truth table must be known
    at runtime.  The number of blocks will be computed as a compile
    time constant.
  */
  static_truth_table()
  {
    _bits.fill( 0 );
  }

  /*! Constructs a new static truth table instance with the same number of variables. */
  inline static_truth_table<NumVars> construct() const
  {
    return static_truth_table<NumVars>();
  }

  /*! Returns number of variables.
   */
  inline auto num_vars() const noexcept { return NumVars; }

  /*! Returns number of blocks.
   */
  inline auto num_blocks() const noexcept { return NumBlocks; }

  /*! Returns number of bits.
   */
  inline auto num_bits() const noexcept { return NumBits; }

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

  /*! \brief Assign other truth table if number of variables match.

    This replaces the current truth table with another truth table, if `other`
    has the same number of variables.  Otherwise, the truth table is not
    changed.

    \param other Other truth table
  */
  template<class TT, typename = std::enable_if_t<is_truth_table<TT>::value>>
  static_truth_table<NumVars>& operator=( const TT& other )
  {
    if ( other.num_bits() == num_bits() )
    {
      std::copy( other.begin(), other.end(), begin() );
    }

    return *this;
  }

  /*! Masks the number of valid truth table bits.

    We know that we will have at least 7 variables in this data
    structure.
  */
  inline void mask_bits() noexcept {}

  /*! \cond PRIVATE */
public: /* fields */
  std::array<uint64_t, NumBlocks> _bits;
  /*! \endcond */
};

template<uint32_t NumVars>
struct is_truth_table<kitty::static_truth_table<NumVars>> : std::true_type
{
};

template<uint32_t NumVars>
struct is_complete_truth_table<kitty::static_truth_table<NumVars>> : std::true_type
{
};

template<uint32_t NumVars>
struct is_completely_specified_truth_table<kitty::static_truth_table<NumVars>> : std::true_type
{
};

} // namespace kitty