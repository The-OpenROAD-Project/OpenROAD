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
  \file davio.hpp
  \brief Use Davio decomposition for resynthesis

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <optional>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>

#include "../../traits.hpp"
#include "../decomposition.hpp"

namespace mockturtle
{

/*! \brief Resynthesis function based on Davio decomposition.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  The given truth table will be
 * resynthized based on Shanon decomposition.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const klut_network klut = ...;

      positive_davio_resynthesis<xag_network> resyn;
      auto xag = node_resynthesis<xag_network>( klut, resyn );
   \endverbatim
 *
 */
template<class Ntk, class ResynFn = null_resynthesis<Ntk>>
class positive_davio_resynthesis
{
public:
  positive_davio_resynthesis( std::optional<uint32_t> const& threshold = {}, ResynFn* resyn = nullptr )
      : threshold_( threshold ),
        resyn_( resyn ) {}

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    if ( threshold_ )
    {
      std::vector<uint32_t> vars( function.num_vars() - std::min<uint32_t>( *threshold_, function.num_vars() ) );
      std::iota( vars.begin(), vars.end(), 0u );
      const auto f = positive_davio_decomposition<Ntk, ResynFn>( ntk, function, vars, std::vector<signal<Ntk>>( begin, end ), *resyn_ );
      fn( f );
    }
    else
    {
      std::vector<uint32_t> vars( function.num_vars() );
      std::iota( vars.begin(), vars.end(), 0u );
      const auto f = positive_davio_decomposition( ntk, function, vars, std::vector<signal<Ntk>>( begin, end ) );
      fn( f );
    }
  }

private:
  std::optional<uint32_t> threshold_;
  ResynFn* resyn_;
};

/*! \brief Resynthesis function based on Davio decomposition.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  The given truth table will be
 * resynthized based on Shanon decomposition.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const klut_network klut = ...;

      negative_davio_resynthesis<xag_network> resyn;
      auto xag = node_resynthesis<xag_network>( klut, resyn );
   \endverbatim
 *
 */
template<class Ntk, class ResynFn = null_resynthesis<Ntk>>
class negative_davio_resynthesis
{
public:
  negative_davio_resynthesis( std::optional<uint32_t> const& threshold = {}, ResynFn* resyn = nullptr )
      : threshold_( threshold ),
        resyn_( resyn ) {}

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    if ( threshold_ )
    {
      std::vector<uint32_t> vars( function.num_vars() - std::min<uint32_t>( *threshold_, function.num_vars() ) );
      std::iota( vars.begin(), vars.end(), 0u );
      const auto f = negative_davio_decomposition<Ntk, ResynFn>( ntk, function, vars, std::vector<signal<Ntk>>( begin, end ), *resyn_ );
      fn( f );
    }
    else
    {
      std::vector<uint32_t> vars( function.num_vars() );
      std::iota( vars.begin(), vars.end(), 0u );
      const auto f = negative_davio_decomposition( ntk, function, vars, std::vector<signal<Ntk>>( begin, end ) );
      fn( f );
    }
  }

private:
  std::optional<uint32_t> threshold_;
  ResynFn* resyn_;
};

} /* namespace mockturtle */