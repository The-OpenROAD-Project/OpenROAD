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
  \file majority.hpp
  \brief Generate majority-n networks

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <array>

#include "../traits.hpp"

namespace mockturtle
{

namespace detail
{

template<class Ntk>
signal<Ntk> fake_majority9( Ntk& ntk, std::array<signal<Ntk>, 9> const& xs )
{
  return ntk.create_maj(
      ntk.create_maj( xs[0], xs[1], xs[2] ),
      ntk.create_maj( xs[3], xs[4], xs[5] ),
      ntk.create_maj( xs[6], xs[7], xs[8] ) );
}

template<class Ntk>
signal<Ntk> general_associativity( Ntk& ntk, signal<Ntk> const& y, std::vector<signal<Ntk>> const& xs )
{
  assert( xs.size() >= 2u );

  return std::accumulate( xs.rbegin() + 1, xs.rend(), xs.back(),
                          [&]( auto const& f, auto const& x ) { return ntk.create_maj( x, y, f ); } );
}

} // namespace detail

/*! \brief Implements Majority-5 using 4 MAJ operations.
 *
 * All majority operations require no inverters and are leafy.
 */
template<class Ntk>
signal<Ntk> majority5( Ntk& ntk, std::array<signal<Ntk>, 5> const& xs )
{
  const auto lhs = ntk.create_maj( xs[0], xs[1], xs[2] );
  const auto rhs = detail::general_associativity( ntk, xs[3], { xs[0], xs[1], xs[2] } );
  return ntk.create_maj( lhs, xs[4], rhs );
}

/*! \brief Implements Majority-7 using 7 MAJ operations.
 *
 * All majority operations require no inverters and are leafy.
 */
template<class Ntk>
signal<Ntk> majority7( Ntk& ntk, std::array<signal<Ntk>, 7> const& xs )
{
  const auto side = [&]( std::array<signal<Ntk>, 6> const& ws ) {
    const auto l1 = ntk.create_maj( ws[0], ws[1], ws[2] );
    return detail::general_associativity( ntk, l1, { ws[3], ws[4], ws[5] } );
  };

  return ntk.create_maj(
      side( { xs[1], xs[2], xs[3], xs[4], xs[5], xs[6] } ),
      xs[0],
      side( { xs[4], xs[5], xs[6], xs[1], xs[2], xs[3] } ) );
}

/*! \brief Implements Majority-9 using 13 MAJ operations.
 *
 * All majority operations require no inverters.
 */
template<class Ntk>
signal<Ntk> majority9_13( Ntk& ntk, std::array<signal<Ntk>, 9> const& xs )
{
  const auto side = [&]( std::array<signal<Ntk>, 9> const& ws ) {
    const auto l1 = ntk.create_maj( ws[3], ws[4], ws[5] );
    const auto l2 = detail::general_associativity( ntk, l1, { ws[0], ws[1], ws[2] } );
    return detail::general_associativity( ntk, l2, { ws[6], ws[7], ws[8] } );
  };

  return ntk.create_maj(
      side( xs ),
      detail::fake_majority9( ntk, xs ),
      side( { xs[0], xs[1], xs[2], xs[6], xs[7], xs[8], xs[3], xs[4], xs[5] } ) );
}

/*! \brief Implements Majority-9 using 12 MAJ operations.
 *
 * This construction requires one inverter.
 */
template<class Ntk>
signal<Ntk> majority9_12( Ntk& ntk, std::array<signal<Ntk>, 9> const& xs )
{
  const auto side = [&]( std::array<signal<Ntk>, 9> const& ws ) {
    const auto bottom = ntk.create_maj( ntk.create_not( ws[0] ), ws[1], ws[2] );
    const auto l1 = ntk.create_maj( ws[3], ws[4], ws[5] );
    const auto l2 = ntk.create_maj( ws[0], l1, bottom );
    return detail::general_associativity( ntk, l2, { ws[6], ws[7], ws[8] } );
  };

  return ntk.create_maj(
      side( xs ),
      detail::fake_majority9( ntk, xs ),
      side( { xs[0], xs[1], xs[2], xs[6], xs[7], xs[8], xs[3], xs[4], xs[5] } ) );
}

} // namespace mockturtle