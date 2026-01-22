/* kitty: C++ truth table library
 * Copyright (C) 2017-2025  EPFL
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
  \file enumeration.hpp
  \brief Enumeration routines

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <stack>
#include <vector>

#include "operators.hpp"

namespace kitty
{

/*! \brief Enumerate all representatives using 1-neighborhood search.

  This function is based on Algorithm 4.2.1 in the PhD thesis
  "Analysis of Affine Equivalent Boolean Functions for Cryptography"
  by J.E. Fuller (Queensland University of Technology)

  \param functions Vector must be initialized with single seed function, will
                   contain all enumerated functions after call
  \param canonization_fn A canonization function, which takes as input a truth
                         table and returns a truth table
*/
template<class TT, class CanonizationFn>
void fuller_neighborhood_enumeration( std::vector<TT>& functions, CanonizationFn&& canonization_fn )
{
  /* there must be one seed truth table given */
  assert( functions.size() == 1u );

  /* classify seed function */
  functions.front() = canonization_fn( functions.front() );

  /* get number of bits from seed truth table */
  const auto num_bits = static_cast<uint64_t>( functions.front().num_bits() );
  std::vector<TT> neighborhood( num_bits );
  uint32_t num{ 1 };
  std::stack<TT> stack;
  stack.push( functions.front() );

  while ( !stack.empty() )
  {
    const auto g = stack.top();
    stack.pop();

    /* Finding connecting classes */
    for ( auto j = 0u; j < num_bits; ++j )
    {
      neighborhood[j] = g;
      flip_bit( neighborhood[j], j );
      neighborhood[j] = canonization_fn( neighborhood[j] );
    }

    for ( auto j = 0u; j < num_bits; ++j )
    {
      bool flag{ false };
      for ( auto i = 0u; i < num; ++i )
      {
        if ( neighborhood[j] == functions[i] )
        {
          flag = true;
          break;
        }
      }
      if ( !flag )
      {
        ++num;
        functions.push_back( neighborhood[j] );
        stack.push( neighborhood[j] );
      }
    }
  }
}

} /* namespace kitty */