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
  \file recursive_cost_functions.hpp
  \brief Various recursive cost functions for (optimization) algorithms

  \author Hanyu Wang
*/

#pragma once

#include <cstdint>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief (Recursive) customizable cost function
 *
 * To define a new cost function, you need to first specify how each node
 * contributes to the total cost via the *contribution function*. Each node
 * is evaluated individually and independently.
 *
 * If additional (global) information is required to decide a node's contribution,
 * you may specify them as *context*. The content stored in the context can be
 * arbitrarily defined (`context_t`), but the derivation must be recursive. In
 * other words, the context of a node is derived using *context propagation function*
 * which takes only the context of fanins as input.
 *
 * Examples of recursive cost functions can be found at:
 * `mockturtle/utils/recursive_cost_functions.hpp`
 */
template<class Ntk>
struct recursive_cost_functions
{
  using base_type = recursive_cost_functions;
  using context_t = uint32_t;
  /*! \brief Context propagation function
   *
   * Return the context of a node given fanin contexts.
   */
  virtual context_t operator()( Ntk const& ntk, node<Ntk> const& n, std::vector<context_t> const& fanin_contexts = {} ) const = 0;

  /*! \brief Contribution function
   *
   * Update the total cost using node n and its context.
   */
  virtual void operator()( Ntk const& ntk, node<Ntk> const& n, uint32_t& total_cost, context_t const context ) const = 0;
};

template<class Ntk>
struct xag_depth_cost_function : recursive_cost_functions<Ntk>
{
public:
  using context_t = uint32_t;
  context_t operator()( Ntk const& ntk, node<Ntk> const& n, std::vector<context_t> const& fanin_contexts = {} ) const
  {
    uint32_t _cost = ntk.is_pi( n ) ? 0 : *std::max_element( std::begin( fanin_contexts ), std::end( fanin_contexts ) ) + 1;
    return _cost;
  }
  void operator()( Ntk const& ntk, node<Ntk> const& n, uint32_t& total_cost, context_t const context ) const
  {
    total_cost = std::max( total_cost, context );
  }
};

template<class Ntk>
struct t_xag_depth_cost_function : recursive_cost_functions<Ntk>
{
public:
  using context_t = uint32_t;
  context_t operator()( Ntk const& ntk, node<Ntk> const& n, std::vector<context_t> const& fanin_contexts = {} ) const
  {
    uint32_t _cost = ntk.is_pi( n ) ? 0 : *std::max_element( std::begin( fanin_contexts ), std::end( fanin_contexts ) ) + ntk.is_and( n );
    return _cost;
  }
  void operator()( Ntk const& ntk, node<Ntk> const& n, uint32_t& total_cost, context_t const context ) const
  {
    total_cost = std::max( total_cost, context );
  }
};

template<class Ntk>
struct xag_size_cost_function : recursive_cost_functions<Ntk>
{
public:
  using context_t = uint32_t;
  context_t operator()( Ntk const& ntk, node<Ntk> const& n, std::vector<uint32_t> const& fanin_contexts = {} ) const
  {
    return 0;
  }
  void operator()( Ntk const& ntk, node<Ntk> const& n, uint32_t& total_cost, context_t const context ) const
  {
    total_cost += ( !ntk.is_pi( n ) && ntk.visited( n ) != ntk.trav_id() ) ? 1 : 0;
  }
};

} /* namespace mockturtle */
