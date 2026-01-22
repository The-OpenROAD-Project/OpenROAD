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
  \file sorting.hpp
  \brief Generate sorting networks

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <numeric>

namespace mockturtle
{

/*! \brief Generates sorting network based on bubble sort.
 *
 * The functor is called for every comparator in the network.  The arguments
 * to the functor are two integers that define on which lines the comparator
 * acts.
 *
 * \param n Number of elements to sort
 * \param compare_fn Functor
 */
template<class Fn>
void bubble_sorting_network( uint32_t n, Fn&& compare_fn )
{
  if ( n <= 1 )
  {
    return;
  }
  for ( auto c = n - 1; c >= 1; --c )
  {
    for ( auto j = 0u; j < c; ++j )
    {
      compare_fn( j, j + 1 );
    }
  }
}

/*! \brief Generates sorting network based on insertion sort.
 *
 * The functor is called for every comparator in the network.  The arguments
 * to the functor are two integers that define on which lines the comparator
 * acts.
 *
 * \param n Number of elements to sort
 * \param compare_fn Functor
 */
template<class Fn>
void insertion_sorting_network( uint32_t n, Fn&& compare_fn )
{
  if ( n <= 1 )
  {
    return;
  }
  for ( auto c = 1u; c < n; ++c )
  {
    for ( int j = c - 1; j >= 0; --j )
    {
      compare_fn( j, j + 1 );
    }
  }
}

namespace detail
{

template<class Fn>
void batcher_merge( std::vector<uint32_t> const& list, Fn&& compare_fn )
{
  if ( list.size() == 2u )
  {
    compare_fn( list[0], list[1] );
    return;
  }

  std::vector<uint32_t> even, odd;
  for ( auto i = 0u; i < list.size(); i += 2 )
  {
    even.push_back( list[i] );
    odd.push_back( list[i + 1] );
  }

  batcher_merge( even, compare_fn );
  batcher_merge( odd, compare_fn );

  for ( auto i = 1u; i < list.size() - 2; i += 2 )
  {
    compare_fn( list[i], list[i + 1] );
  }
}

template<class Fn>
void batcher_sort( uint32_t begin, uint32_t end, Fn&& compare_fn )
{
  const auto size = end - begin;
  if ( size == 2u )
  {
    compare_fn( begin, begin + 1 );
    return;
  }

  batcher_sort( begin, begin + size / 2, compare_fn );
  batcher_sort( begin + size / 2, end, compare_fn );

  std::vector<uint32_t> list( size );
  std::iota( list.begin(), list.end(), begin );
  batcher_merge( list, compare_fn );
}

} // namespace detail

template<class Fn>
void batcher_sorting_network( uint32_t n, Fn&& compare_fn )
{
  if ( n < 2 )
    return;
  detail::batcher_sort( 0u, n, compare_fn );
}

} // namespace mockturtle