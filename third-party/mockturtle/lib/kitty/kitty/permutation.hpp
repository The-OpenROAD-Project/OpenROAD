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
  \file permutation.hpp
  \brief Efficient permutation of truth tables

  \author Mathias Soeken
*/

#pragma once

#include <numeric>
#include <vector>

#include "bit_operations.hpp"
#include "operators.hpp"
#include "print.hpp"

namespace kitty
{

namespace detail
{
template<typename TT>
std::pair<TT, TT> compute_permutation_masks_pair( const TT& tt, std::vector<uint32_t>& left, std::vector<uint32_t>& right, unsigned step )
{
  const auto n = tt.num_vars();
  const auto loops = ( 1 << ( n - 1 - step ) );
  const auto diff = ( 1 << step );

  const auto offset = 1 << ( n - 1 );

  struct node_t
  {
    std::vector<uint32_t>::iterator a, b; /* numbers in left or right */
    unsigned lf{}, rf{};                  /* left field right field */
    bool visited = false;
  };

  std::vector<node_t> nodes( tt.num_bits() );

  /* compute graph */
  auto idx1 = 0u, idx2 = 0u;
  auto it1 = std::begin( left );
  for ( auto l1 = 0; l1 < loops; ++l1 )
  {
    for ( auto c1 = 0; c1 < diff; ++c1 )
    {
      auto it2 = std::begin( right );
      idx2 = offset;

      nodes[idx1].a = it1;
      nodes[idx1].b = it1 + diff;

      for ( auto l2 = 0; l2 < loops; ++l2 )
      {
        for ( auto c2 = 0; c2 < diff; ++c2 )
        {
          if ( idx1 == 0u )
          {
            nodes[idx2].a = it2;
            nodes[idx2].b = it2 + diff;
          }

          /* pair elements */
          auto& n1 = nodes[idx1];
          auto& n2 = nodes[idx2];

          /* connect graph */
          if ( *n1.a == *n2.a )
          {
            n1.lf = idx2;
            n2.lf = idx1;
          }
          else if ( *n1.a == *n2.b )
          {
            n1.lf = idx2;
            n2.rf = idx1;
          }
          if ( *n1.b == *n2.a )
          {
            n1.rf = idx2;
            n2.lf = idx1;
          }
          else if ( *n1.b == *n2.b )
          {
            n1.rf = idx2;
            n2.rf = idx1;
          }

          ++it2;
          ++idx2;
        }
        it2 += diff;
      }
      ++it1;
      ++idx1;
    }
    it1 += diff;
  }

  /* traverse graph and compute masks */
  auto mask_left = tt.construct();
  auto mask_right = tt.construct();

  while ( true )
  {
    auto idx = 0;

    while ( idx < offset && nodes[idx].visited )
    {
      ++idx;
    }

    if ( idx == offset )
    {
      break;
    }

    auto left_side = true;
    auto nr = *nodes[idx].a;
    auto start = idx;

    do
    {
      auto& n = nodes[idx];

      auto match = *n.a == nr;

      nr = match ? *n.b : *n.a;
      idx = match ? n.rf : n.lf;
      n.visited = true;

      if ( left_side != match )
      {
        std::swap( *n.a, *n.b );

        if ( left_side )
        {
          set_bit( mask_left, std::distance( left.begin(), n.a ) );
        }
        else
        {
          set_bit( mask_right, std::distance( right.begin(), n.a ) );
        }
      }

      left_side = !left_side;

    } while ( idx != start );
  }

  return std::make_pair( mask_left, mask_right );
}
} /* namespace detail */

/*! \brief Applies delta-swap operation

  The delta-swap operation swaps all position pairs \f$(i, i+\delta)\f$, for
  which \f$\omega\f$ is set 1 at position \f$i\f$.

  See also Eq. 7.1.3-(69) in The Art of Computer Programming.

  \param tt Truth table
  \param delta Index distance delta
  \param omega Enable mask
*/
template<typename TT>
inline void delta_swap_inplace( TT& tt, uint64_t delta, const TT& omega )
{
  const auto y = ( tt ^ ( tt >> delta ) ) & omega;
  tt = tt ^ y ^ ( y << delta );
}

/*! \brief Applies delta-swap operation

  Out-of-place variant for `delta_swap_inplace`.
*/
template<typename TT>
TT delta_swap( const TT& tt, uint64_t delta, const TT& omega )
{
  auto copy = tt;
  delta_swap_inplace( copy, delta, omega );
  return copy;
}

/*! \brief Permutes a truth table using a sequence of delta-swaps

  Masks is an array containing the \f$\omega\f$ masks.  The \f$\delta\f$ values
  are chosen as increasing and decreasing powers of 2, as described in Eq.
  7.1.3-(71) of The Art of Computer Programming.

  \param tt Truth table
  \param masks Array of omega-masks
*/
template<typename TT>
void permute_with_masks_inplace( TT& tt, const std::vector<TT>& masks )
{
  for ( auto k = 0u; k < tt.num_vars(); ++k )
  {
    delta_swap_inplace( tt, uint64_t( 1 ) << k, masks[k] );
  }

  for ( int k = tt.num_vars() - 2, i = tt.num_vars(); k >= 0; --k, ++i )
  {
    delta_swap_inplace( tt, uint64_t( 1 ) << k, masks[i] );
  }
}

/*! \brief Permutes a truth table using a sequence of delta-swaps

  Out-of-place variant of `permute_with_masks_inplace`.
*/
template<typename TT>
TT permute_with_masks( const TT& tt, const std::vector<TT>& masks )
{
  auto copy = tt;
  permute_with_masks_inplace( copy, masks );
  return copy;
}

/*! \brief Computes permutation bitmasks

  These bitmasks can be used with the `permute_with_masks` algorithm.  The
  algorithm to compute these masks is described in The Art of Computer
  Programming, Section 7.1.3 'Bit permutation in general'.

  The input truth table can be arbitrary but is used to determine the type and
  size of of the returned permutation masks.

  \param tt Base truth table to derive types and size
  \param permutation Permutation
*/
template<typename TT>
std::vector<TT> compute_permutation_masks( const TT& tt, const std::vector<uint32_t>& permutation )
{
  std::vector<TT> masks;

  std::vector<uint32_t> left( permutation.size() ), right = permutation;
  std::iota( left.begin(), left.end(), 0u );

  for ( auto i = 0u; i < tt.num_vars() - 1u; ++i )
  {
    const auto pair = detail::compute_permutation_masks_pair( tt, left, right, i );

    masks.insert( masks.begin() + i, pair.second );
    masks.insert( masks.begin() + i, pair.first );
  }

  auto mask = tt.construct();
  for ( uint64_t i = 0u; i < ( static_cast<uint64_t>( tt.num_bits() ) >> 1 ); ++i )
  {
    if ( left[i] != right[i] )
    {
      set_bit( mask, i );
    }
  }
  masks.insert( masks.begin() + tt.num_vars() - 1, mask );

  return masks;
}

} /* namespace kitty */