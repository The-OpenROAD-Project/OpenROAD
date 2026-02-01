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
  \file gia_cut.hpp
  \brief Cut enumeration as in giaCut.c

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>

#include "../cut_enumeration.hpp"

namespace mockturtle
{

/*! \brief Cut implementation based on ABC's giaCut.c

  See <a href="https://github.com/berkeley-abc/abc/blob/master/src/aig/gia/giaCut.c">giaCut.c</a> in ABC's repository.
*/
struct cut_enumeration_gia_cut
{
  uint32_t num_tree_leaves;
};

template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_gia_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_gia_cut> const& c2 )
{
  if ( c1->data.num_tree_leaves < c2->data.num_tree_leaves )
  {
    return true;
  }
  if ( c1->data.num_tree_leaves > c2->data.num_tree_leaves )
  {
    return false;
  }
  return c1.size() < c2.size();
}

template<>
struct cut_enumeration_update_cut<cut_enumeration_gia_cut>
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    (void)n;
    (void)cuts;
    cut->data.num_tree_leaves = std::count_if( cut.begin(), cut.end(),
                                               [&ntk]( auto index ) {
                                                 return ntk.fanout_size( index ) == 1;
                                               } );
  }
};

} // namespace mockturtle