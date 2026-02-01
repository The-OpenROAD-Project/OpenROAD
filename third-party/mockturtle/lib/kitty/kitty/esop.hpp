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
  \file esop.hpp
  \brief Implements methods to compute exclusive sum-of-products (ESOP) representations

  \author Mathias Soeken
  \author Winston Haaswijk
*/

#pragma once

// #ifndef _MSC_VER
// #warning "DEPRECATED: the functions in this file are marked as deprecated.  Most recent implementation can be found in https://github.com/hriener/easy/ in the file src/esop/constructors.hpp"
// #endif

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "algorithm.hpp"
#include "constructors.hpp"
#include "cube.hpp"
#include "hash.hpp"
#include "operations.hpp"
#include "operators.hpp"
#include "traits.hpp"

namespace kitty
{

/*! \cond PRIVATE */
namespace detail
{

enum class pkrm_decomposition
{
  positive_davio,
  negative_davio,
  shannon
};

template<typename TT>
using expansion_cache = std::unordered_map<TT, std::pair<uint32_t, pkrm_decomposition>, hash<TT>>;

template<typename TT>
uint32_t find_pkrm_expansions( const TT& tt, expansion_cache<TT>& cache, uint8_t var_index )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return 0;
  }
  if ( is_const0( ~tt ) )
  {
    return 1;
  }

  /* already computed */
  const auto it = cache.find( tt );
  if ( it != cache.end() )
  {
    return it->second.first;
  }

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  const auto ex0 = find_pkrm_expansions( tt0, cache, var_index + 1 );
  const auto ex1 = find_pkrm_expansions( tt1, cache, var_index + 1 );
  const auto ex2 = find_pkrm_expansions( tt0 ^ tt1, cache, var_index + 1 );

  const auto ex_max = std::max( std::max( ex0, ex1 ), ex2 );

  uint32_t cost{};
  pkrm_decomposition decomp;

  if ( ex_max == ex0 )
  {
    cost = ex1 + ex2;
    decomp = pkrm_decomposition::positive_davio;
  }
  else if ( ex_max == ex1 )
  {
    cost = ex0 + ex2;
    decomp = pkrm_decomposition::negative_davio;
  }
  else
  {
    cost = ex0 + ex1;
    decomp = pkrm_decomposition::shannon;
  }
  cache.insert( { tt, { cost, decomp } } );
  return cost;
}

inline void add_to_cubes( std::unordered_set<cube, hash<cube>>& pkrm, const cube& c, bool distance_one_merging = true )
{
  /* first check whether cube is already contained; if so, delete it */
  const auto it = pkrm.find( c );
  if ( it != pkrm.end() )
  {
    pkrm.erase( it );
    return;
  }

  /* otherwise, check if there is a distance-1 cube; if so, merge it */
  if ( distance_one_merging )
  {
    for ( auto it = pkrm.begin(); it != pkrm.end(); ++it )
    {
      if ( c.distance( *it ) == 1 )
      {
        auto new_cube = c.merge( *it );
        pkrm.erase( it );
        add_to_cubes( pkrm, new_cube );
        return;
      }
    }
  }

  /* otherwise, just add the cube */
  pkrm.insert( c );
}

inline cube with_literal( const cube& c, uint8_t var_index, bool polarity )
{
  auto copy = c;
  copy.add_literal( var_index, polarity );
  return copy;
}

template<typename TT>
void optimum_pkrm_rec( std::unordered_set<cube, hash<cube>>& pkrm, const TT& tt, const expansion_cache<TT>& cache, uint8_t var_index, const cube& c )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return;
  }
  if ( is_const0( ~tt ) )
  {
    add_to_cubes( pkrm, c );
    return;
  }

  const auto& p = cache.at( tt );

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  switch ( p.second )
  {
  case pkrm_decomposition::positive_davio:
    optimum_pkrm_rec( pkrm, tt0, cache, var_index + 1, c );
    optimum_pkrm_rec( pkrm, tt0 ^ tt1, cache, var_index + 1, with_literal( c, var_index, true ) );
    break;
  case pkrm_decomposition::negative_davio:
    optimum_pkrm_rec( pkrm, tt1, cache, var_index + 1, c );
    optimum_pkrm_rec( pkrm, tt0 ^ tt1, cache, var_index + 1, with_literal( c, var_index, false ) );
    break;
  case pkrm_decomposition::shannon:
    optimum_pkrm_rec( pkrm, tt0, cache, var_index + 1, with_literal( c, var_index, false ) );
    optimum_pkrm_rec( pkrm, tt1, cache, var_index + 1, with_literal( c, var_index, true ) );
    break;
  }
}

template<typename TT>
void esop_from_pprm_rec( std::unordered_set<cube, hash<cube>>& cubes, const TT& tt, uint8_t var_index, const cube& c )
{
  /* terminal cases */
  if ( is_const0( tt ) )
  {
    return;
  }
  if ( is_const0( ~tt ) )
  {
    /* add to cubes, but do not apply distance-1 merging */
    add_to_cubes( cubes, c, false );
    return;
  }

  const auto tt0 = cofactor0( tt, var_index );
  const auto tt1 = cofactor1( tt, var_index );

  esop_from_pprm_rec( cubes, tt0, var_index + 1, c );
  esop_from_pprm_rec( cubes, tt0 ^ tt1, var_index + 1, with_literal( c, var_index, true ) );
}

static constexpr uint64_t ANF1[] = { 0, 3, 2, 1 };
static constexpr uint64_t ANF2[] = { 0, 15, 10, 5, 12, 3, 6, 9, 8, 7, 2, 13, 4, 11, 14, 1 };
static constexpr uint64_t ANF3[] = { 0, 255, 170, 85, 204, 51, 102, 153, 136, 119, 34, 221, 68, 187, 238, 17, 240, 15, 90, 165, 60, 195, 150, 105, 120, 135, 210, 45, 180, 75, 30, 225, 160, 95, 10, 245, 108, 147, 198, 57, 40, 215, 130, 125, 228, 27, 78, 177, 80, 175, 250, 5, 156, 99, 54, 201, 216, 39, 114, 141, 20, 235, 190, 65, 192, 63, 106, 149, 12, 243, 166, 89, 72, 183, 226, 29, 132, 123, 46, 209, 48, 207, 154, 101, 252, 3, 86, 169, 184, 71, 18, 237, 116, 139, 222, 33, 96, 159, 202, 53, 172, 83, 6, 249, 232, 23, 66, 189, 36, 219, 142, 113, 144, 111, 58, 197, 92, 163, 246, 9, 24, 231, 178, 77, 212, 43, 126, 129, 128, 127, 42, 213, 76, 179, 230, 25, 8, 247, 162, 93, 196, 59, 110, 145, 112, 143, 218, 37, 188, 67, 22, 233, 248, 7, 82, 173, 52, 203, 158, 97, 32, 223, 138, 117, 236, 19, 70, 185, 168, 87, 2, 253, 100, 155, 206, 49, 208, 47, 122, 133, 28, 227, 182, 73, 88, 167, 242, 13, 148, 107, 62, 193, 64, 191, 234, 21, 140, 115, 38, 217, 200, 55, 98, 157, 4, 251, 174, 81, 176, 79, 26, 229, 124, 131, 214, 41, 56, 199, 146, 109, 244, 11, 94, 161, 224, 31, 74, 181, 44, 211, 134, 121, 104, 151, 194, 61, 164, 91, 14, 241, 16, 239, 186, 69, 220, 35, 118, 137, 152, 103, 50, 205, 84, 171, 254, 1 };

template<class TT>
TT algebraic_normal_form( const TT& func )
{
  switch ( func.num_vars() )
  {
  case 0:
    return func;
  case 1:
  {
    auto r = func.construct();
    kitty::create_from_words( r, &ANF1[*func.begin()], &ANF1[*func.begin()] + 1 );
    return r;
  }
  case 2:
  {
    auto r = func.construct();
    kitty::create_from_words( r, &ANF2[*func.begin()], &ANF2[*func.begin()] + 1 );
    return r;
  }
  case 3:
  {
    auto r = func.construct();
    kitty::create_from_words( r, &ANF3[*func.begin()], &ANF3[*func.begin()] + 1 );
    return r;
  }
  case 4:
    return unary_operation( func, []( uint64_t word )
                            {
      const auto b0 = ANF3[word & 0x000000FF];
      const auto b1 = ANF3[( word & 0x0000FF00 ) >> 8];

      return b0 | ( ( b0 ^ b1 ) << 8 ); } );
  case 5:
    return unary_operation( func, []( uint64_t word )
                            {
      const auto b0 = ANF3[word & 0xFF];
      const auto b1 = ANF3[( word >> 010 ) & 0xFF];
      const auto b2 = ANF3[( word >> 020 ) & 0xFF];
      const auto b3 = ANF3[( word >> 030 ) & 0xFF];

      return b0 | ( ( b0 ^ b1 ) << 010 ) | ( ( b0 ^ b2 ) << 020 ) | ( ( b0 ^ b1 ^ b2 ^ b3 ) << 030 ); } );
  default:
    auto r = unary_operation( func, []( uint64_t word )
                              {
      const auto b0 = ANF3[word & 0xFF];
      const auto b1 = ANF3[( word >> 010 ) & 0xFF];
      const auto b2 = ANF3[( word >> 020 ) & 0xFF];
      const auto b3 = ANF3[( word >> 030 ) & 0xFF];
      const auto b4 = ANF3[( word >> 040 ) & 0xFF];
      const auto b5 = ANF3[( word >> 050 ) & 0xFF];
      const auto b6 = ANF3[( word >> 060 ) & 0xFF];
      const auto b7 = ANF3[( word >> 070 ) & 0xFF];

      return b0 |
             ( ( b0 ^ b1 ) << 010 ) |
             ( ( b0 ^ b2 ) << 020 ) |
             ( ( b0 ^ b1 ^ b2 ^ b3 ) << 030 ) |
             ( ( b0 ^ b4 ) << 040 ) |
             ( ( b0 ^ b1 ^ b4 ^ b5 ) << 050 ) |
             ( ( b0 ^ b2 ^ b4 ^ b6 ) << 060 ) |
             ( ( b0 ^ b1 ^ b2 ^ b3 ^ b4 ^ b5 ^ b6 ^ b7 ) << 070 ); } );

    for ( auto i = 1u; i < (uint32_t)func.num_blocks(); i = i << 1u )
    {
      for ( auto j = 0u; j < (uint32_t)func.num_blocks(); j += i << 1u )
      {
        for ( auto k = j; k < j + i; ++k )
        {
          *( r.begin() + k + i ) ^= *( r.begin() + k );
        }
      }
    }

    return r;
  }
}
} // namespace detail
/*! \endcond */

/*! \brief Computes ESOP representation using optimum PKRM

  This algorithm first computes an ESOP using the algorithm described
  in [R. Drechsler, IEEE Trans. C 48(9), 1999, 987–990].

  The algorithm applies post-optimization to merge distance-1 cubes.

  \param tt Truth table
*/
template<typename TT>
inline std::vector<cube> esop_from_optimum_pkrm( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  std::unordered_set<cube, hash<cube>> cubes;
  detail::expansion_cache<TT> cache;

  detail::find_pkrm_expansions( tt, cache, 0 );
  detail::optimum_pkrm_rec( cubes, tt, cache, 0, cube() );

  return std::vector<cube>( cubes.begin(), cubes.end() );
}

template<typename TT>
inline std::vector<cube> esop_from_pprm_slow( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  std::unordered_set<cube, hash<cube>> cubes;
  detail::esop_from_pprm_rec( cubes, tt, 0, cube() );

  return std::vector<cube>( cubes.begin(), cubes.end() );
}

/*! \brief Computes PPRM representation for a function

  This algorithm applies recursively the positive Davio decomposition which
  eventually leads into the PPRM representation of a function.

  \param tt Truth table
*/
template<class TT>
std::vector<cube> esop_from_pprm( const TT& func )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  std::vector<cube> cubes;
  for_each_one_bit( detail::algebraic_normal_form( func ), [&]( auto bit )
                    { cubes.emplace_back( (uint32_t)bit, (uint32_t)bit ); } );
  return cubes;
}

} // namespace kitty