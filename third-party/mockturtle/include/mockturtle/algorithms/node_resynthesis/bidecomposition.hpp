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
  \file bidecomposition.hpp
  \brief Resynthesis with bi_decomposition

  \author Eleonora Testa
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>

#include "../../algorithms/bi_decomposition.hpp"

namespace mockturtle
{

/*! \brief Resynthesis function based on bi-decomposition
 *
 * This resynthesis function can be passed to  ``refactoring``.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const xag_network xag = ...;
      bidecomposition_resynthesis<xag_network> resyn;
      const auto xag = refactoring( xag, resyn );
   \endverbatim
 */

template<class Ntk>
class bidecomposition_resynthesis
{
public:
  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, kitty::dynamic_truth_table const& dc, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    fn( bi_decomposition<Ntk>( ntk, function, ~dc, { begin, end } ) );
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    operator()( ntk, function, function.construct(), begin, end, fn );
  }
};
} /* namespace mockturtle */