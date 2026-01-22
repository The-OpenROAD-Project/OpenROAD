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
  \file cnf.hpp
  \brief Implements methods to compute conjunctive normal forms (CNF)

  \author Mathias Soeken
*/

#pragma once

#include <vector>

#include "cube.hpp"
#include "isop.hpp"
#include "operators.hpp"
#include "traits.hpp"

namespace kitty
{

/*! \brief Create CNF of the characteristic function

  Creates a CNF representation for the characteritic function of the input
  function, also known as Tseytin transformation.  To obtain small CNF,
  an ISOP is computed.

  \param tt Truth table
*/
template<typename TT, typename = std::enable_if_t<is_complete_truth_table<TT>::value>>
std::vector<cube> cnf_characteristic( const TT& tt )
{
  std::vector<cube> cubes;
  detail::isop_rec( tt, tt, tt.num_vars(), cubes );

  for ( auto& cube : cubes )
  {
    cube._bits = ~cube._bits & cube._mask;
    cube.add_literal( tt.num_vars(), true );
  }

  const auto end = cubes.size();

  detail::isop_rec( ~tt, ~tt, tt.num_vars(), cubes );

  for ( auto i = end; i < cubes.size(); ++i )
  {
    auto& cube = cubes[i];
    cube._bits = ~cube._bits & cube._mask;
    cube.add_literal( tt.num_vars(), false );
  }

  return cubes;
}

} // namespace kitty