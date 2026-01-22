/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2023  EPFL
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
  \file rewrite_cut.hpp
  \brief Cut enumeration for rewriting

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../cut_enumeration.hpp"

namespace mockturtle
{

/*! \brief Cut implementation for rewrite */
struct cut_enumeration_rewrite_cut
{
  uint32_t cost;
};

template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_rewrite_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_rewrite_cut> const& c2 )
{
  if ( c1->data.cost < c2->data.cost )
    return true;
  if ( c1->data.cost > c2->data.cost )
    return false;
  return c1.size() < c2.size();
}

template<>
struct cut_enumeration_update_cut<cut_enumeration_rewrite_cut>
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    uint32_t value = 0;

    for ( auto leaf : cut )
    {
      value += ( ntk.fanout_size( ntk.index_to_node( leaf ) ) == 1 ) ? 1u : 0u;
    }

    cut->data.cost = value;
  }
};

template<int MaxLeaves>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enumeration_rewrite_cut>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, C = " << std::setw( 3 ) << c->data.cost;
  return os;
}

} // namespace mockturtle