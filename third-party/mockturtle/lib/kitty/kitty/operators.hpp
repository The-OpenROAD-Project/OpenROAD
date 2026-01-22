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
  \file operators.hpp
  \brief Implements operator shortcuts to operations

  \author Mathias Soeken
*/

#pragma once

#include "dynamic_truth_table.hpp"
#include "operations.hpp"
#include "static_truth_table.hpp"
#include "partial_truth_table.hpp"
#include "ternary_truth_table.hpp"
#include "quaternary_truth_table.hpp"

namespace kitty
{

/*! \brief Operator for unary_not */
inline dynamic_truth_table operator~( const dynamic_truth_table& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for unary_not */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator~( const static_truth_table<NumVars>& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for unary_not */
inline partial_truth_table operator~( const partial_truth_table& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for unary_not */
template<typename TT>
inline ternary_truth_table<TT> operator~( const ternary_truth_table<TT>& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for unary_not */
template<typename TT>
inline quaternary_truth_table<TT> operator~( const quaternary_truth_table<TT>& tt )
{
  return unary_not( tt );
}

/*! \brief Operator for binary_and */
inline dynamic_truth_table operator&( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator&( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and */
inline partial_truth_table operator&( const partial_truth_table& first, const partial_truth_table& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and */
template<typename TT>
inline ternary_truth_table<TT> operator&( const ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and */
template<typename TT>
inline quaternary_truth_table<TT> operator&( const quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  return binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
inline void operator&=( dynamic_truth_table& first, const dynamic_truth_table& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
template<uint32_t NumVars>
inline void operator&=( static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
inline void operator&=( partial_truth_table& first, const partial_truth_table& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
template<typename TT>
inline void operator&=( ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_and and assign */
template<typename TT>
inline void operator&=( quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  first = binary_and( first, second );
}

/*! \brief Operator for binary_or */
inline dynamic_truth_table operator|( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator|( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or */
inline partial_truth_table operator|( const partial_truth_table& first, const partial_truth_table& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or */
template<typename TT>
inline ternary_truth_table<TT> operator|( const ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or */
template<typename TT>
inline quaternary_truth_table<TT> operator|( const quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  return binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
inline void operator|=( dynamic_truth_table& first, const dynamic_truth_table& second )
{
  first = binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
template<uint32_t NumVars>
inline void operator|=( static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  first = binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
inline void operator|=( partial_truth_table& first, const partial_truth_table& second )
{
  first = binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
template<typename TT>
inline void operator|=( ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  first = binary_or( first, second );
}

/*! \brief Operator for binary_or and assign */
template<typename TT>
inline void operator|=( quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  first = binary_or( first, second );
}

/*! \brief Operator for binary_xor */
inline dynamic_truth_table operator^( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return binary_xor( first, second );
}

/*! \brief Operator for binary_xor */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator^( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return binary_xor( first, second );
}

/*! \brief Operator for binary_xor */
inline partial_truth_table operator^( const partial_truth_table& first, const partial_truth_table& second )
{
  return binary_xor( first, second );
}

/*! \brief Operator for binary_xor */
template<typename TT>
inline ternary_truth_table<TT> operator^( const ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  return binary_xor( first, second );
}

/*! \brief Operator for binary_xor */
template<typename TT>
inline quaternary_truth_table<TT> operator^( const quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  return binary_xor( first, second );
}

/*! \brief Operator for binary_xor and assign */
inline void operator^=( dynamic_truth_table& first, const dynamic_truth_table& second )
{
  first = binary_xor( first, second );
}

/*! \brief Operator for binary_xor and assign */
template<uint32_t NumVars>
inline void operator^=( static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  first = binary_xor( first, second );
}

/*! \brief Operator for binary_xor and assign */
inline void operator^=( partial_truth_table& first, const partial_truth_table& second )
{
  first = binary_xor( first, second );
}

/*! \brief Operator for binary_xor and assign */
template<typename TT>
inline void operator^=( ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  first = binary_xor( first, second );
}

/*! \brief Operator for binary_xor and assign */
template<typename TT>
inline void operator^=( quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  first = binary_xor( first, second );
}

/*! \brief Operator for equal */
inline bool operator==( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return equal( first, second );
}

/*! \brief Operator for equal */
template<uint32_t NumVars>
inline bool operator==( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return equal( first, second );
}

/*! \brief Operator for equal */
inline bool operator==( const partial_truth_table& first, const partial_truth_table& second )
{
  return equal( first, second );
}

/*! \brief Operator for equal */
template<typename TT>
inline bool operator==( const ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  return equal( first, second );
}

/*! \brief Operator for equal */
template<typename TT>
inline bool operator==( const quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  return equal( first, second );
}

/*! \brief Operator for not equals (!equal) */
inline bool operator!=( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return !equal( first, second );
}

/*! \brief Operator for not equals (!equal) */
template<uint32_t NumVars>
inline bool operator!=( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return !equal( first, second );
}

/*! \brief Operator for not equal */
inline bool operator!=( const partial_truth_table& first, const partial_truth_table& second )
{
  return !equal( first, second );
}

/*! \brief Operator for not equals (!equal) */
template<typename TT>
inline bool operator!=( const ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  return !equal( first, second );
}

/*! \brief Operator for not equals (!equal) */
template<typename TT>
inline bool operator!=( const quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  return !equal( first, second );
}

/*! \brief Operator for less_than */
inline bool operator<( const dynamic_truth_table& first, const dynamic_truth_table& second )
{
  return less_than( first, second );
}

/*! \brief Operator for less_than */
template<uint32_t NumVars>
inline bool operator<( const static_truth_table<NumVars>& first, const static_truth_table<NumVars>& second )
{
  return less_than( first, second );
}

/*! \brief Operator for less_than */
inline bool operator<( const partial_truth_table& first, const partial_truth_table& second )
{
  return less_than( first, second );
}

/*! \brief Operator for less_than */
template<typename TT>
inline bool operator<( const ternary_truth_table<TT>& first, const ternary_truth_table<TT>& second )
{
  return less_than( first, second );
}

/*! \brief Operator for less_than */
template<typename TT>
inline bool operator<( const quaternary_truth_table<TT>& first, const quaternary_truth_table<TT>& second )
{
  return less_than( first, second );
}

/*! \brief Operator for left_shift */
inline dynamic_truth_table operator<<( const dynamic_truth_table& tt, uint64_t shift )
{
  return shift_left( tt, shift );
}

/*! \brief Operator for left_shift */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator<<( const static_truth_table<NumVars>& tt, uint64_t shift )
{
  return shift_left( tt, shift );
}

/*! \brief Operator for left_shift */
inline partial_truth_table operator<<( const partial_truth_table& tt, uint64_t shift )
{
  return shift_left( tt, shift );
}

/*! \brief Operator for left_shift */
template<typename TT>
inline ternary_truth_table<TT> operator<<( const ternary_truth_table<TT>& tt, uint64_t shift )
{
  return shift_left( tt, shift );
}

/*! \brief Operator for left_shift */
template<typename TT>
inline quaternary_truth_table<TT> operator<<( const quaternary_truth_table<TT>& tt, uint64_t shift )
{
  return shift_left( tt, shift );
}

/*! \brief Operator for left_shift_inplace */
inline void operator<<=( dynamic_truth_table& tt, uint64_t shift )
{
  shift_left_inplace( tt, shift );
}

/*! \brief Operator for left_shift_inplace */
template<uint32_t NumVars>
inline void operator<<=( static_truth_table<NumVars>& tt, uint64_t shift )
{
  shift_left_inplace( tt, shift );
}

/*! \brief Operator for left_shift_inplace */
inline void operator<<=( partial_truth_table& tt, uint64_t shift )
{
  shift_left_inplace( tt, shift );
}

/*! \brief Operator for left_shift_inplace */
template<typename TT>
inline void operator<<=( ternary_truth_table<TT>& tt, uint64_t shift )
{
  shift_left_inplace( tt, shift );
}

/*! \brief Operator for left_shift_inplace */
template<typename TT>
inline void operator<<=( quaternary_truth_table<TT>& tt, uint64_t shift )
{
  shift_left_inplace( tt, shift );
}

/*! \brief Operator for right_shift */
inline dynamic_truth_table operator>>( const dynamic_truth_table& tt, uint64_t shift )
{
  return shift_right( tt, shift );
}

/*! \brief Operator for right_shift */
template<uint32_t NumVars>
inline static_truth_table<NumVars> operator>>( const static_truth_table<NumVars>& tt, uint64_t shift )
{
  return shift_right( tt, shift );
}

/*! \brief Operator for right_shift */
inline partial_truth_table operator>>( const partial_truth_table& tt, uint64_t shift )
{
  return shift_right( tt, shift );
}

/*! \brief Operator for right_shift */
template<typename TT>
inline ternary_truth_table<TT> operator>>( const ternary_truth_table<TT>& tt, uint64_t shift )
{
  return shift_right( tt, shift );
}

/*! \brief Operator for right_shift */
template<typename TT>
inline quaternary_truth_table<TT> operator>>( const quaternary_truth_table<TT>& tt, uint64_t shift )
{
  return shift_right( tt, shift );
}

/*! \brief Operator for right_shift_inplace */
inline void operator>>=( dynamic_truth_table& tt, uint64_t shift )
{
  shift_right_inplace( tt, shift );
}

/*! \brief Operator for right_shift_inplace */
template<uint32_t NumVars>
inline void operator>>=( static_truth_table<NumVars>& tt, uint64_t shift )
{
  shift_right_inplace( tt, shift );
}

/*! \brief Operator for right_shift_inplace */
inline void operator>>=( partial_truth_table& tt, uint64_t shift )
{
  shift_right_inplace( tt, shift );
}

/*! \brief Operator for right_shift_inplace */
template<typename TT>
inline void operator>>=( ternary_truth_table<TT>& tt, uint64_t shift )
{
  shift_right_inplace( tt, shift );
}

/*! \brief Operator for right_shift_inplace */
template<typename TT>
inline void operator>>=( quaternary_truth_table<TT>& tt, uint64_t shift )
{
  shift_right_inplace( tt, shift );
}

} // namespace kitty
