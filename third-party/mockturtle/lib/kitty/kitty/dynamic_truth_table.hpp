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
  \file dynamic_truth_table.hpp
  \brief Implements dynamic_truth_table

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

#include "detail/constants.hpp"
#include "traits.hpp"

namespace kitty
{

/*! Truth table in which number of variables is known at runtime.
 */
struct dynamic_truth_table
{
  /*! Standard constructor.

    The number of variables provided to the truth table can be
    computed at runtime.  However, once the truth table is constructed
    its number of variables cannot change anymore.

    The constructor computes the number of blocks and resizes the
    vector accordingly.

    \param num_vars Number of variables
  */
  explicit dynamic_truth_table( uint32_t num_vars )
      : _bits( ( num_vars <= 6 ) ? 1u : ( 1u << ( num_vars - 6 ) ) ),
        _num_vars( num_vars )
  {
  }

  /*! Empty constructor.

    Creates an empty truth table. It has 0 variables, but no bits, i.e., it is
    different from a truth table for the constant function.  This constructor is
    only used for convenience, if algorithms require the existence of default
    constructable classes.
   */
  dynamic_truth_table() : _num_vars( 0 ) {}

  /*! Constructs a new dynamic truth table instance with the same number of variables. */
  inline dynamic_truth_table construct() const
  {
    return dynamic_truth_table( _num_vars );
  }

  /*! Returns number of variables.
   */
  inline auto num_vars() const noexcept { return _num_vars; }

  /*! Returns number of blocks.
   */
  inline auto num_blocks() const noexcept { return _bits.size(); }

  /*! Returns number of bits.
   */
  inline auto num_bits() const noexcept { return uint64_t( 1 ) << _num_vars; }

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
    table type has to be complete.  The vector of bits is resized accordingly.

    \param other Other truth table
  */
  template<class TT, typename = std::enable_if_t<is_truth_table<TT>::value && is_complete_truth_table<TT>::value>>
  dynamic_truth_table& operator=( const TT& other )
  {
    _bits.resize( other.num_blocks() );
    std::copy( other.begin(), other.end(), begin() );
    _num_vars = other.num_vars();

    if ( _num_vars < 6 )
    {
      mask_bits();
    }

    return *this;
  }

  /*! Masks the number of valid truth table bits.

    If the truth table has less than 6 variables, it may not use all
    the bits.  This operation makes sure to zero out all non-valid
    bits.
  */
  inline void mask_bits() noexcept
  {
    if ( _num_vars < 6 )
    {
      _bits[0u] &= detail::masks[_num_vars];
    }
  }

  /*! \cond PRIVATE */
public: /* fields */
  std::vector<uint64_t> _bits;
  uint32_t _num_vars;
  /*! \endcond */
};

template<>
struct is_truth_table<kitty::dynamic_truth_table> : std::true_type
{
};

template<>
struct is_complete_truth_table<kitty::dynamic_truth_table> : std::true_type
{
};

template<>
struct is_completely_specified_truth_table<kitty::dynamic_truth_table> : std::true_type
{
};

} // namespace kitty