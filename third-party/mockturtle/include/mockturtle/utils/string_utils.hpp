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
  \file string_utils.hpp
  \brief String utils

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <numeric>
#include <string>
#include <type_traits>

namespace mockturtle
{

template<class Iterator, class MapFn, class JoinFn>
std::invoke_result_t<MapFn, typename Iterator::value_type> map_and_join( Iterator begin, Iterator end, MapFn&& map_fn, JoinFn&& join_fn )
{
  if constexpr ( std::is_same_v<std::decay_t<JoinFn>, std::string> )
  {
    return std::accumulate( begin + 1, end, map_fn( *begin ),
                            [&]( auto const& a, auto const& v ) {
                              return a + join_fn + map_fn( v );
                            } );
  }
  else
  {
    return std::accumulate( begin + 1, end, map_fn( *begin ),
                            [&]( auto const& a, auto const& v ) {
                              return join_fn( a, map_fn( v ) );
                            } );
  }
}

} // namespace mockturtle