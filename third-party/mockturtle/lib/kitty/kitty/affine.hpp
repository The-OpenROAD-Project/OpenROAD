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
  \file affine.hpp
  \brief Implements affine canonization algorithms

  \author Mathias Soeken
*/

#pragma once

#include "dynamic_truth_table.hpp"
#include "operators.hpp"
#include "static_truth_table.hpp"
#include "spectral.hpp"
#include "traits.hpp"

/*! \cond PRIVATE */
#include "detail/linear_constants.hpp"
/*! \endcond */

namespace kitty
{

/*! \cond PRIVATE */
namespace detail
{
/* all delta-swap operations have been optimized to work with integer masks omega */
template<typename TT>
inline void delta_swap_inplace_opt( TT& tt, uint64_t delta, uint64_t omega )
{
  assert( tt.num_vars() <= 6 );
  const uint64_t y = ( tt._bits[0] ^ ( tt._bits[0] >> delta ) ) & omega;
  tt._bits[0] = tt._bits[0] ^ y ^ ( y << delta );
}

template<uint32_t NumVars>
inline void delta_swap_inplace_opt( static_truth_table<NumVars, true>& tt, uint64_t delta, uint64_t omega )
{
  assert( NumVars <= 6 );
  const uint64_t y = ( tt._bits ^ ( tt._bits >> delta ) ) & omega;
  tt._bits = tt._bits ^ y ^ ( y << delta );
}

template<typename TT>
void permute_with_masks_inplace_opt( TT& tt, uint64_t const* masks )
{
  for ( auto k = 0u; k < tt.num_vars(); ++k )
  {
    delta_swap_inplace_opt( tt, uint64_t( 1 ) << k, masks[k] );
  }

  for ( int k = tt.num_vars() - 2, i = tt.num_vars(); k >= 0; --k, ++i )
  {
    delta_swap_inplace_opt( tt, uint64_t( 1 ) << k, masks[i] );
  }
}

template<typename TT>
TT permute_with_masks_opt( const TT& tt, uint64_t const* masks )
{
  auto copy = tt;
  permute_with_masks_inplace_opt( copy, masks );
  return copy;
}

template<typename Fn>
inline void for_each_permutation_mask( unsigned num_vars, Fn&& fn )
{
  assert( num_vars >= 2 && num_vars <= 4 );

  const auto offset = 2 * num_vars - 1;

  const auto s = detail::masks_start[num_vars - 2u];
  const auto e = detail::masks_end[num_vars - 2u];

  for ( auto i = s; i < e; i += offset )
  {
    fn( &detail::linear_masks[i] );
  }
}
} /* namespace detail */
/*! \endcond */

/*! \brief Applies exact linear classification

  This algorithms applies all linear input transformations to a function and
  returns the function with the smallest integer representation as
  representative of the equivalence class.

  \param tt Truth table
*/
template<typename TT, typename Callback = decltype( detail::exact_spectral_canonization_null_callback )>
TT exact_linear_canonization( const TT& tt, Callback&& fn = detail::exact_spectral_canonization_null_callback )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  detail::miller_spectral_canonization_impl<TT> impl( tt, false, false, false );
  return impl.run( fn ).first;
}

/*! \cond PRIVATE */
template<typename TT>
TT exact_linear_canonization_old( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  auto min = tt;

  detail::for_each_permutation_mask( tt.num_vars(), [&min, &tt]( const auto* mask )
                                     { min = std::min( min, detail::permute_with_masks_opt( tt, mask ) ); } );

  return min;
}
/*! \endcond */

/*! \brief Applies exact linear classification and output negation

  This algorithms applies all linear input transformations to a function and its
  complement and returns the function with the smallest integer representation
  as representative of the equivalence class.

  \param tt Truth table
*/
template<typename TT>
TT exact_linear_output_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  return std::min( exact_linear_canonization_old( tt ), exact_linear_canonization_old( ~tt ) );
}

/*! \brief Applies exact affine classification

  This algorithms applies all linear input transformations and input
  complementations to a function and returns the function with the smallest
  integer representation as representative of the equivalence class.

  \param tt Truth table
*/
template<typename TT, typename Callback = decltype( detail::exact_spectral_canonization_null_callback )>
TT exact_affine_canonization( const TT& tt, Callback&& fn = detail::exact_spectral_canonization_null_callback )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  detail::miller_spectral_canonization_impl<TT> impl( tt, true, false, false );
  return impl.run( fn ).first;
}

/*! \cond PRIVATE */
template<typename TT>
TT exact_affine_canonization_old( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  assert( num_vars >= 2 && num_vars <= 4 );

  auto copy = tt;

  const auto& flips = detail::flips[num_vars - 2u];

  auto min = exact_linear_canonization_old( copy );

  for ( int j = static_cast<int>( flips.size() ) - 1; j >= 0; --j )
  {
    const auto pos = flips[j];
    flip_inplace( copy, pos );
    min = std::min( min, exact_linear_canonization_old( copy ) );
  }

  return min;
}
/*! \endcond */

/*! \brief Applies exact affine classification and output negation

  This algorithms applies all linear input transformations and input
  complementations to a function and its complement and returns the function
  with the smallest integer representation as representative of the equivalence
  class.

  \param tt Truth table
*/
template<typename TT>
TT exact_affine_output_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  return std::min( exact_affine_canonization_old( tt ), exact_affine_canonization_old( ~tt ) );
}

} /* namespace kitty */