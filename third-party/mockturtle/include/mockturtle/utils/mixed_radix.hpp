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
  \file mixed_radix.hpp
  \brief Mixed radix loop

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <type_traits>
#include <vector>

namespace mockturtle
{

/*! \brief Mixed radix enumeration.
 *
 * The iterator pair `begin` and `end` represent a list of radixes, which are
 * used to enumerate all combinations of indexes.
 *
 * For example if the radixes are \f$2, 3, 3\f$, then the callable `fn` is
 * called on the index lists \f$0, 0, 0\f$, \f$0, 0, 1\f$, \f$0, 0, 2\f$,
 * \f$\dots\f$, \f$1, 2, 1\f$, \f$1, 2, 2\f$.
 *
 * The callable `fn` expects two parameters which are an iterator pair of the
 * indexes.  If it returns a `bool`, the iteration is stopped, if the return
 * value is `false`.
 *
 * \param begin Begin iterator of radixes
 * \param end End iterator of radixes
 * \param fn Callable
 */
template<typename Iterator, typename Fn>
void foreach_mixed_radix_tuple( Iterator begin, Iterator end, Fn&& fn )
{
  constexpr auto is_bool_f = std::is_invocable_r_v<bool, Fn, std::vector<uint32_t>::iterator, std::vector<uint32_t>::iterator>;
  constexpr auto is_void_f = std::is_invocable_r_v<void, Fn, std::vector<uint32_t>::iterator, std::vector<uint32_t>::iterator>;

  static_assert( is_bool_f || is_void_f );

  std::vector<uint32_t> positions( std::distance( begin, end ), 0u );

  while ( true )
  {
    if constexpr ( is_bool_f )
    {
      if ( !fn( positions.begin(), positions.end() ) )
      {
        return;
      }
    }
    else
    {
      fn( positions.begin(), positions.end() );
    }

    auto itm = end - 1;
    auto itp = positions.end() - 1;
    auto ret = false;
    while ( !ret && *itp == ( *itm - 1 ) )
    {
      *itp = 0;
      if ( itp != positions.begin() )
      {
        --itp;
      }

      if ( itm == begin )
      {
        ret = true;
      }
      else
      {
        --itm;
      }
    }

    if ( ret )
    {
      break;
    }

    ( *itp )++;
  }
}

} // namespace mockturtle
