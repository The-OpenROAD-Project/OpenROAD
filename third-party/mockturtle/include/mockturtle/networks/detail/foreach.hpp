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
  \file foreach.hpp
  \brief For each functor utilities

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <type_traits>

namespace mockturtle::detail
{

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_with_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType, uint32_t>;

template<class Fn, class ElementType, class ReturnType>
inline constexpr bool is_callable_without_index_v = std::is_invocable_r_v<ReturnType, Fn, ElementType>;

template<class Iterator, class ElementType = typename Iterator::value_type, class Fn>
Iterator foreach_element( Iterator begin, Iterator end, Fn&& fn, uint32_t counter_offset = 0 )
{
  static_assert( is_callable_with_index_v<Fn, ElementType, void> ||
                 is_callable_without_index_v<Fn, ElementType, void> ||
                 is_callable_with_index_v<Fn, ElementType, bool> ||
                 is_callable_without_index_v<Fn, ElementType, bool> );

  if constexpr ( is_callable_without_index_v<Fn, ElementType, bool> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !fn( *begin++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, bool> )
  {
    uint32_t index{ counter_offset };
    while ( begin != end )
    {
      if ( !fn( *begin++, index++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_without_index_v<Fn, ElementType, void> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      fn( *begin++ );
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, void> )
  {
    uint32_t index{ counter_offset };
    while ( begin != end )
    {
      fn( *begin++, index++ );
    }
    return begin;
  }
}

template<class Iterator, class ElementType = typename Iterator::value_type, class Pred, class Fn>
Iterator foreach_element_if( Iterator begin, Iterator end, Pred&& pred, Fn&& fn, uint32_t counter_offset = 0 )
{
  static_assert( is_callable_with_index_v<Fn, ElementType, void> ||
                 is_callable_without_index_v<Fn, ElementType, void> ||
                 is_callable_with_index_v<Fn, ElementType, bool> ||
                 is_callable_without_index_v<Fn, ElementType, bool> );

  if constexpr ( is_callable_without_index_v<Fn, ElementType, bool> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      if ( !fn( *begin++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, bool> )
  {
    uint32_t index{ counter_offset };
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      if ( !fn( *begin++, index++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_without_index_v<Fn, ElementType, void> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      fn( *begin++ );
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, void> )
  {
    uint32_t index{ counter_offset };
    while ( begin != end )
    {
      if ( !pred( *begin ) )
      {
        ++begin;
        continue;
      }
      fn( *begin++, index++ );
    }
    return begin;
  }
}

template<class Iterator, class ElementType, class Transform, class Fn>
Iterator foreach_element_transform( Iterator begin, Iterator end, Transform&& transform, Fn&& fn, uint32_t counter_offset = 0 )
{
  static_assert( is_callable_with_index_v<Fn, ElementType, void> ||
                 is_callable_without_index_v<Fn, ElementType, void> ||
                 is_callable_with_index_v<Fn, ElementType, bool> ||
                 is_callable_without_index_v<Fn, ElementType, bool> );

  if constexpr ( is_callable_without_index_v<Fn, ElementType, bool> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      if ( !fn( transform( *begin++ ) ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, bool> )
  {
    uint32_t index{ counter_offset };
    while ( begin != end )
    {
      if ( !fn( transform( *begin++ ), index++ ) )
      {
        return begin;
      }
    }
    return begin;
  }
  else if constexpr ( is_callable_without_index_v<Fn, ElementType, void> )
  {
    (void)counter_offset;
    while ( begin != end )
    {
      fn( transform( *begin++ ) );
    }
    return begin;
  }
  else if constexpr ( is_callable_with_index_v<Fn, ElementType, void> )
  {
    uint32_t index{ counter_offset };
    while ( begin != end )
    {
      fn( transform( *begin++ ), index++ );
    }
    return begin;
  }
}

} // namespace mockturtle::detail