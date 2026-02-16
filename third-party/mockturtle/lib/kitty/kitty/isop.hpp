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
  \file isop.hpp
  \brief Implements methods to compute irredundant sum-of-products (ISOP) representations

  \author Mathias Soeken
*/

#pragma once

#include "cube.hpp"
#include "operations.hpp"
#include "operators.hpp"
#include "traits.hpp"

namespace kitty
{

/*! \cond PRIVATE */
namespace detail
{
template<typename TT>
TT isop_rec( const TT& tt, const TT& dc, uint8_t var_index, std::vector<cube>& cubes )
{
  assert( var_index <= tt.num_vars() );
  assert( is_const0( tt & ~dc ) );

  if ( is_const0( tt ) )
  {
    return tt;
  }

  if ( is_const0( ~dc ) )
  {
    cubes.emplace_back(); /* add empty cube */
    return dc;
  }

  assert( var_index > 0 );

  int var = var_index - 1;
  for ( ; var >= 0; --var )
  {
    if ( has_var( tt, var ) || has_var( dc, var ) )
    {
      break;
    }
  }

  assert( var >= 0 );

  /* co-factor */
  const auto tt0 = cofactor0( tt, var );
  const auto tt1 = cofactor1( tt, var );
  const auto dc0 = cofactor0( dc, var );
  const auto dc1 = cofactor1( dc, var );

  const auto beg0 = cubes.size();
  const auto res0 = isop_rec( tt0 & ~dc1, dc0, var, cubes );
  const auto end0 = cubes.size();
  const auto res1 = isop_rec( tt1 & ~dc0, dc1, var, cubes );
  const auto end1 = cubes.size();
  auto res2 = isop_rec( ( tt0 & ~res0 ) | ( tt1 & ~res1 ), dc0 & dc1, var, cubes );

  auto var0 = tt.construct();
  create_nth_var( var0, var, true );
  auto var1 = tt.construct();
  create_nth_var( var1, var );
  res2 |= ( res0 & var0 ) | ( res1 & var1 );

  for ( auto c = beg0; c < end0; ++c )
  {
    cubes[c].add_literal( var, false );
  }
  for ( auto c = end0; c < end1; ++c )
  {
    cubes[c].add_literal( var, true );
  }

  assert( is_const0( tt & ~res2 ) );
  assert( is_const0( res2 & ~dc ) );

  return res2;
}
} /* namespace detail */
/* \endcond */

/*! \brief Computes ISOP representation

  Computes the irredundant sum-of-products representation using the
  Minato-Morreale algorithm [S. Minato, IEEE Trans. CAD 15(4), 1996,
  377-384].

  \param tt Truth table
*/
template<typename TT>
inline std::vector<cube> isop( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  std::vector<cube> cubes;
  detail::isop_rec( tt, tt, tt.num_vars(), cubes );
  return cubes;
}

} /* namespace kitty */