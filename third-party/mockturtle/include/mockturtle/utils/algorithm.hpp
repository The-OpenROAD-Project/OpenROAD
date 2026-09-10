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
  \file algorithm.hpp
  \brief STL-like algorithm extensions

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Marcel Walter
  \author Mathias Soeken
*/

#pragma once

#include <iterator>

namespace mockturtle
{

template<class Iterator, class T, class BinaryOperation>
T tree_reduce( Iterator first, Iterator last, T const& init, BinaryOperation&& op )
{
  const auto len = std::distance( first, last );

  switch ( len )
  {
  case 0u:
    return init;
  case 1u:
    return *first;
  case 2u:
    return op( *first, *( first + 1 ) );
  default:
  {
    const auto m = len / 2;
    return op( tree_reduce( first, first + m, init, op ), tree_reduce( first + m, last, init, op ) );
  }
  break;
  }
}

template<class Iterator, class T, class TernaryOperation>
T ternary_tree_reduce( Iterator first, Iterator last, T const& init, TernaryOperation&& op )
{
  const auto len = std::distance( first, last );

  switch ( len )
  {
  case 0u:
    return init;
  case 1u:
    return *first;
  case 2u:
    return op( init, *first, *( first + 1 ) );
  case 3u:
    return op( *first, *( first + 1 ), *( first + 2 ) );
  default:
  {
    const auto m1 = len / 3;
    const auto m2 = ( len - m1 ) / 2;
    return op( ternary_tree_reduce( first, first + m1, init, op ),
               ternary_tree_reduce( first + m1, first + m1 + m2, init, op ),
               ternary_tree_reduce( first + m1 + m2, last, init, op ) );
  }
  break;
  }
}

template<class Iterator, class UnaryOperation, class T>
Iterator max_element_unary( Iterator first, Iterator last, UnaryOperation&& fn, T const& init )
{
  auto best = last;
  auto max = init;
  for ( ; first != last; ++first )
  {
    if ( const auto v = fn( *first ) > max )
    {
      max = v;
      best = first;
    }
  }
  return best;
}

template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto range( T begin, T end )
{
  struct iterator
  {
    using value_type = T;

    value_type curr_;
    bool operator!=( iterator const& other ) const { return curr_ != other.curr_; }
    iterator& operator++()
    {
      ++curr_;
      return *this;
    }
    iterator operator++( int )
    {
      auto copy = *this;
      ++( *this );
      return copy;
    }
    value_type operator*() const { return curr_; }
  };
  struct iterable_wrapper
  {
    T begin_;
    T end_;
    auto begin() { return iterator{ begin_ }; }
    auto end() { return iterator{ end_ }; }
  };
  return iterable_wrapper{ begin, end };
}

template<class T, typename = std::enable_if_t<std::is_integral_v<T>>>
constexpr auto range( T end )
{
  return range<T>( {}, end );
}

/*! \brief Performs the set union of two sorted sets.
 *
 * Compared to std::set_union, limits the copy to `limit`.
 * Moreover, it returns the number of elements copied if the
 * union operation is successful. Else, it returns -1.
 *
 */
template<class InputIterator1, class InputIterator2, class OutputIterator>
int32_t set_union_safe( InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2, OutputIterator result, uint32_t limit )
{
  /* special case: sets are at the limit */
  if ( std::distance( first1, last1 ) == limit && std::distance( first2, last2 ) == limit )
  {
    while ( first1 != last1 )
    {
      if ( *first1 != *first2 )
        return -1;

      *result = *first1;
      ++first1;
      ++first2;
      ++result;
    }

    return static_cast<int32_t>( limit );
  }

  uint32_t size = 0;
  while ( size < limit )
  {
    if ( first1 == last1 )
    {
      size += std::distance( first2, last2 );
      if ( size <= limit )
      {
        std::copy( first2, last2, result );
        return static_cast<int32_t>( size );
      }
      else
      {
        return -1;
      }
    }
    else if ( first2 == last2 )
    {
      size += std::distance( first1, last1 );
      if ( size <= limit )
      {
        std::copy( first1, last1, result );
        return static_cast<int32_t>( size );
      }
      else
      {
        return -1;
      }
    }

    if ( *first1 < *first2 )
    {
      *result = *first1;
      ++first1;
    }
    else if ( *first2 < *first1 )
    {
      *result = *first2;
      ++first2;
    }
    else
    {
      *result = *first1;
      ++first1;
      ++first2;
    }

    ++result;
    ++size;
  }

  if ( std::distance( first1, last1 ) + std::distance( first2, last2 ) > 0 )
    return -1;

  return static_cast<int32_t>( size );
}

} /* namespace mockturtle */