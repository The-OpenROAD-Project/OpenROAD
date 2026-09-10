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
  \file npn.hpp
  \brief Implements NPN canonization algorithms

  \author Alessandro Tempia Calvino
  \author Mathias Soeken
*/

#pragma once

#include <numeric>

#include "detail/constants.hpp"
#include "operators.hpp"
#include "traits.hpp"

namespace kitty
{

/*! \cond PRIVATE */

namespace detail
{
template<typename TT>
void exact_npn_canonization_null_callback( const TT& tt )
{
  (void)tt;
}
} /* namespace detail */
/*! \endcond */

/*! \brief Exact P canonization

  Given a truth table, this function finds the lexicographically smallest truth
  table in its P class, called P representative. Two functions are in the
  same P class, if one can obtain one from the other by input permutation.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole P class.

  The function returns a NPN configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the P representative
  - input negations and output negation, which is 0 in this case
  - input permutation to apply

  \param tt The truth table
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return NPN configuration
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, uint32_t, std::vector<uint8_t>> exact_p_canonization( const TT& tt, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    return std::make_tuple( tt, 0u, std::vector<uint8_t>{} );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    return std::make_tuple( tt, 0u, std::vector<uint8_t>{ 0 } );
  }

  assert( num_vars >= 2 && num_vars <= 7 );

  auto t1 = tt;
  auto tmin = t1;

  fn( t1 );

  const auto& swaps = detail::swaps[num_vars - 2u];

  int best_swap = -1;

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );

    fn( t1 );

    if ( t1 < tmin )
    {
      best_swap = static_cast<int>( i );
      tmin = t1;
    }
  }

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  for ( auto i = 0; i <= best_swap; ++i )
  {
    const auto pos = swaps[i];
    std::swap( perm[pos], perm[pos + 1] );
  }

  return std::make_tuple( tmin, 0u, perm );
}

/*! \brief Exact NPN canonization

  Given a truth table, this function finds the lexicographically smallest truth
  table in its NPN class, called NPN representative. Two functions are in the
  same NPN class, if one can obtain one from the other by input negation, input
  permutation, and output negation.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole NPN class.

  The function returns a NPN configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the NPN representative
  - input negations and output negation, output negation is stored as bit *n*,
    where *n* is the number of variables in `tt`
  - input permutation to apply

  \param tt The truth table (with at most 6 variables)
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return NPN configuration
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, uint32_t, std::vector<uint8_t>> exact_npn_canonization( const TT& tt, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    const auto bit = get_bit( tt, 0 );
    return std::make_tuple( unary_not_if( tt, bit ), static_cast<uint32_t>( bit ), std::vector<uint8_t>{} );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    const auto bit1 = get_bit( tt, 1 );
    return std::make_tuple( unary_not_if( tt, bit1 ), static_cast<uint32_t>( bit1 << 1 ), std::vector<uint8_t>{ 0 } );
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt, t2 = ~tt;
  auto tmin = std::min( t1, t2 );
  auto invo = tmin == t2;

  fn( t1 );
  fn( t2 );

  const auto& swaps = detail::swaps[num_vars - 2u];
  const auto& flips = detail::flips[num_vars - 2u];

  int best_swap = -1;
  int best_flip = -1;

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );
    swap_adjacent_inplace( t2, pos );

    fn( t1 );
    fn( t2 );

    if ( t1 < tmin || t2 < tmin )
    {
      best_swap = static_cast<int>( i );
      tmin = std::min( t1, t2 );
      invo = tmin == t2;
    }
  }

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    swap_adjacent_inplace( t1, 0 );
    flip_inplace( t1, pos );
    swap_adjacent_inplace( t2, 0 );
    flip_inplace( t2, pos );

    fn( t1 );
    fn( t2 );

    if ( t1 < tmin || t2 < tmin )
    {
      best_swap = -1;
      best_flip = static_cast<int>( j );
      tmin = std::min( t1, t2 );
      invo = tmin == t2;
    }

    for ( std::size_t i = 0; i < swaps.size(); ++i )
    {
      const auto pos = swaps[i];
      swap_adjacent_inplace( t1, pos );
      swap_adjacent_inplace( t2, pos );

      fn( t1 );
      fn( t2 );

      if ( t1 < tmin || t2 < tmin )
      {
        best_swap = static_cast<int>( i );
        best_flip = static_cast<int>( j );
        tmin = std::min( t1, t2 );
        invo = tmin == t2;
      }
    }
  }

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  for ( auto i = 0; i <= best_swap; ++i )
  {
    const auto pos = swaps[i];
    std::swap( perm[pos], perm[pos + 1] );
  }

  uint32_t phase = uint32_t( invo ) << num_vars;
  for ( auto i = 0; i <= best_flip; ++i )
  {
    phase ^= 1 << flips[i];
  }

  return std::make_tuple( tmin, phase, perm );
}

/*! \brief Exact N canonization

  Given a truth table, this function finds the lexicographically smallest truth
  table in its N class, called N representative. Two functions are in the
  same N class, if one can obtain one from the other by input negations.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole N class.

  The function returns a N configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the N representative
  - input negations that lead to the representative

  \param tt The truth table
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return N configurations
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, uint32_t> exact_n_canonization( const TT& tt, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    return std::make_tuple( tt, 0 );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    return std::make_tuple( tt, 0 );
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;
  auto tmin = t1;

  fn( t1 );

  const auto& flips = detail::flips[num_vars - 2u];
  int best_flip = -1;

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    flip_inplace( t1, pos );

    fn( t1 );

    if ( t1 < tmin )
    {
      best_flip = static_cast<int>( j );
      tmin = t1;
    }
  }

  uint32_t phase = 0;
  for ( auto i = 0; i <= best_flip; ++i )
  {
    phase ^= 1 << flips[i];
  }

  return std::make_tuple( tmin, phase );
}

/*! \brief Exact N canonization given a support size

  Given a truth table, this function finds the lexicographically smallest truth
  table in its N class, called N representative. Two functions are in the
  same N class, if one can obtain one from the other by input negations.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole N class.

  The function returns a N configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the N representative
  - input negations that lead to the representative

  \param tt The truth table
  \param support_size Support size used for the canonization
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return N configurations
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, uint32_t> exact_n_canonization_support( const TT& tt, uint32_t support_size, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  assert( support_size <= tt.num_vars() );

  const auto num_vars = support_size;

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    return std::make_tuple( tt, 0 );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    return std::make_tuple( tt, 0 );
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;
  auto tmin = t1;

  fn( t1 );

  const auto& flips = detail::flips[num_vars - 2u];
  int best_flip = -1;

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    flip_inplace( t1, pos );

    fn( t1 );

    if ( t1 < tmin )
    {
      best_flip = static_cast<int>( j );
      tmin = t1;
    }
  }

  uint32_t phase = 0;
  for ( auto i = 0; i <= best_flip; ++i )
  {
    phase ^= 1 << flips[i];
  }

  return std::make_tuple( tmin, phase );
}

/*! \brief Flip-swap NPN heuristic

  This algorithm will iteratively try to reduce the numeric value of the truth
  table by first inverting each input, then inverting the output, and then
  swapping each pair of inputs.  Every improvement is accepted, the algorithm
  stops, if no more improvement can be achieved.

  The function returns a NPN configuration which contains the
  necessary transformations to obtain the representative.  It is a
  tuple of

  - the NPN representative
  - input negations and output negation, output negation is stored as
    bit *n*, where *n* is the number of variables in `tt`
  - input permutation to apply

  \param tt Truth table
  \return NPN configuration
*/
template<typename TT>
std::tuple<TT, uint32_t, std::vector<uint8_t>> flip_swap_npn_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* initialize permutation and phase */
  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  uint32_t phase{ 0u };

  auto npn = tt;
  auto improvement = true;

  while ( improvement )
  {
    improvement = false;

    /* input inversion */
    for ( auto i = 0u; i < num_vars; ++i )
    {
      const auto flipped = flip( npn, i );
      if ( flipped < npn )
      {
        npn = flipped;
        phase ^= 1 << perm[i];
        improvement = true;
      }
    }

    /* output inversion */
    const auto flipped = ~npn;
    if ( flipped < npn )
    {
      npn = flipped;
      phase ^= 1 << num_vars;
      improvement = true;
    }

    /* permute inputs */
    for ( auto d = 1u; d < num_vars - 1; ++d )
    {
      for ( auto i = 0u; i < num_vars - d; ++i )
      {
        auto j = i + d;

        const auto permuted = swap( npn, i, j );
        if ( permuted < npn )
        {
          npn = permuted;
          std::swap( perm[i], perm[j] );

          improvement = true;
        }
      }
    }
  }

  return std::make_tuple( npn, phase, perm );
}

/*! \cond PRIVATE */
namespace detail
{

template<typename TT>
void sifting_npn_canonization_loop( TT& npn, uint32_t& phase, std::vector<uint8_t>& perm )
{
  auto improvement = true;
  auto forward = true;

  const auto n = npn.num_vars();

  while ( improvement )
  {
    improvement = false;

    for ( int i = forward ? 0 : n - 2; forward ? i < static_cast<int>( n - 1 ) : i >= 0; forward ? ++i : --i )
    {
      auto local_improvement = false;
      for ( auto k = 1u; k < 8u; ++k )
      {
        if ( k % 4u == 0u )
        {
          const auto next_t = swap( npn, i, i + 1 );
          if ( next_t < npn )
          {
            npn = next_t;
            std::swap( perm[i], perm[i + 1] );
            local_improvement = true;
          }
        }
        else if ( k % 2u == 0u )
        {
          const auto next_t = flip( npn, i + 1 );
          if ( next_t < npn )
          {
            npn = next_t;
            phase ^= 1 << perm[i + 1];
            local_improvement = true;
          }
        }
        else
        {
          const auto next_t = flip( npn, i );
          if ( next_t < npn )
          {
            npn = next_t;
            phase ^= 1 << perm[i];
            local_improvement = true;
          }
        }
      }

      if ( local_improvement )
      {
        improvement = true;
      }
    }

    forward = !forward;
  }
}
template<typename TT>
void sifting_p_canonization_loop( TT& p, uint32_t& phase, std::vector<uint8_t>& perm )
{
  (void)phase;
  auto improvement = true;
  auto forward = true;

  const auto n = p.num_vars();

  while ( improvement )
  {
    improvement = false;

    for ( int i = forward ? 0 : n - 2; forward ? i < static_cast<int>( n - 1 ) : i >= 0; forward ? ++i : --i )
    {
      auto local_improvement = false;

      const auto next_t = swap( p, i, i + 1 );
      if ( next_t < p )
      {
        p = next_t;
        std::swap( perm[i], perm[i + 1] );
        local_improvement = true;
      }

      if ( local_improvement )
      {
        improvement = true;
      }
    }
    forward = !forward;
  }
}
} /* namespace detail */
/*! \endcond */

/*! \brief Sifting NPN heuristic

  The algorithm will always consider two adjacent variables and try all possible
  transformations on these two.  It will try once in forward direction and once
  in backward direction.  It will try for the regular function and inverted
  function.

  The function returns a NPN configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the NPN representative
  - input negations and output negation, output negation is stored as bit *n*,
    where *n* is the number of variables in `tt`
  - input permutation to apply

  \param tt Truth table
  \return NPN configuration
*/
template<typename TT>
std::tuple<TT, uint32_t, std::vector<uint8_t>> sifting_npn_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* initialize permutation and phase */
  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );
  uint32_t phase{ 0u };

  if ( num_vars < 2 )
  {
    return std::make_tuple( tt, phase, perm );
  }

  auto npn = tt;

  detail::sifting_npn_canonization_loop( npn, phase, perm );

  const auto best_perm = perm;
  const auto best_phase = phase;
  const auto best_npn = npn;

  npn = ~tt;
  phase = 1 << num_vars;
  std::iota( perm.begin(), perm.end(), 0u );

  detail::sifting_npn_canonization_loop( npn, phase, perm );

  if ( best_npn < npn )
  {
    perm = best_perm;
    phase = best_phase;
    npn = best_npn;
  }

  return std::make_tuple( npn, phase, perm );
}

/*! \brief Sifting P heuristic

  The algorithm will always consider two adjacent variables and try all possible
  transformations on these two.  It will try once in forward direction and once
  in backward direction.  It will try for the regular function.

  The function returns a P configuration which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the P representative
  - input negations and output negation, which is 0 in this case
  - input permutation to apply

  \param tt Truth table
  \return NPN configuration
*/
template<typename TT>
std::tuple<TT, uint32_t, std::vector<uint8_t>> sifting_p_canonization( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* initialize permutation and phase */
  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );
  uint32_t phase{ 0u };

  if ( num_vars < 2u )
  {
    return std::make_tuple( tt, phase, perm );
  }

  auto npn = tt;

  detail::sifting_p_canonization_loop( npn, phase, perm );

  return std::make_tuple( npn, phase, perm );
}

/*! \brief Exact NPN enumeration

  Given a truth table, this function enumerates all the functions in its
  NPN class. Two functions are in the same NP class, if one can be obtained
  from the other by input negation, input permutation, and output negation.

  The function takes a callback as second parameter which is called for
  every enumerated function. The callback should take as parameters:
  - NPN-enumerated truth table
  - input and output negations
  - input permutation to apply

  \param tt Truth table
  \param fn Callback for each enumerated truth table in the NP class
*/
template<typename TT, typename Callback>
void exact_npn_enumeration( const TT& tt, Callback&& fn )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    fn( tt, 0u, std::vector<uint8_t>{} );
    fn( ~tt, 1u, std::vector<uint8_t>{} );
    return;
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    fn( tt, 0u, std::vector<uint8_t>{ 0 } );
    fn( ~tt, 2u, std::vector<uint8_t>{ 0 } );
    return;
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  uint32_t phase = 0;

  fn( t1, phase, perm );

  const auto& swaps = detail::swaps[num_vars - 2u];
  const auto& flips = detail::flips[num_vars - 2u];

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );

    std::swap( perm[pos], perm[pos + 1] );

    fn( t1, phase, perm );
    fn( ~t1, phase | ( 1u << num_vars ), perm );
  }

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    swap_adjacent_inplace( t1, 0 );
    flip_inplace( t1, pos );

    std::swap( perm[0], perm[1] );
    phase ^= 1 << perm[pos];

    fn( t1, phase, perm );
    fn( ~t1, phase | ( 1u << num_vars ), perm );

    for ( std::size_t i = 0; i < swaps.size(); ++i )
    {
      const auto pos = swaps[i];
      swap_adjacent_inplace( t1, pos );

      std::swap( perm[pos], perm[pos + 1] );

      fn( t1, phase, perm );
      fn( ~t1, phase | ( 1u << num_vars ), perm );
    }
  }
}

/*! \brief Exact NP enumeration

  Given a truth table, this function enumerates all the functions in its
  NP class. Two functions are in the same NP class, if one can be obtained
  from the other by input negation and input permutation.

  The function takes a callback as second parameter which is called for
  every enumerated function. The callback should take as parameters:
  - NP-enumerated truth table
  - input negations
  - input permutation to apply

  \param tt Truth table
  \param fn Callback for each enumerated truth table in the NP class
*/
template<typename TT, typename Callback>
void exact_np_enumeration( const TT& tt, Callback&& fn )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    fn( tt, 0u, std::vector<uint8_t>{} );
    return;
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    fn( tt, 0u, std::vector<uint8_t>{ 0 } );
    return;
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  uint32_t phase = 0;

  fn( t1, phase, perm );

  const auto& swaps = detail::swaps[num_vars - 2u];
  const auto& flips = detail::flips[num_vars - 2u];

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );

    std::swap( perm[pos], perm[pos + 1] );

    fn( t1, phase, perm );
  }

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    swap_adjacent_inplace( t1, 0 );
    flip_inplace( t1, pos );

    std::swap( perm[0], perm[1] );
    phase ^= 1 << perm[pos];

    fn( t1, phase, perm );

    for ( std::size_t i = 0; i < swaps.size(); ++i )
    {
      const auto pos = swaps[i];
      swap_adjacent_inplace( t1, pos );

      std::swap( perm[pos], perm[pos + 1] );

      fn( t1, phase, perm );
    }
  }
}

/*! \brief Exact multi NP enumeration

  Given multiple truth tables, this function enumerates all the functions in their
  NP class. Two functions are in the same NP class, if one can be obtained
  from the other by input negation and input permutation.

  The function takes a callback as second parameter which is called for
  every enumerated function. The callback should take as parameters:
  - NP-enumerated truth tables
  - input negations
  - input permutation to apply

  \param tts Truth tables
  \param fn Callback for each enumerated truth table in the NP class
*/
template<typename TT, typename Callback>
void exact_multi_np_enumeration( const std::vector<TT>& tts, Callback&& fn )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  assert( tts.size() > 0 );

  const auto num_vars = tts[0].num_vars();

  for ( auto i = 0; i < tts.size(); ++i )
    assert( tts[i].num_vars() == num_vars );

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    fn( tts, 0u, std::vector<uint8_t>{} );
    return;
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    fn( tts, 0u, std::vector<uint8_t>{ 0 } );
    return;
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tts;

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  uint32_t phase = 0;

  fn( t1, phase, perm );

  const auto& swaps = detail::swaps[num_vars - 2u];
  const auto& flips = detail::flips[num_vars - 2u];

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];

    for ( auto& tt : t1 )
      swap_adjacent_inplace( tt, pos );

    std::swap( perm[pos], perm[pos + 1] );

    fn( t1, phase, perm );
  }

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];

    for ( auto& tt : t1 )
    {
      swap_adjacent_inplace( tt, 0 );
      flip_inplace( tt, pos );
    }

    std::swap( perm[0], perm[1] );
    phase ^= 1 << perm[pos];

    fn( t1, phase, perm );

    for ( std::size_t i = 0; i < swaps.size(); ++i )
    {
      const auto pos = swaps[i];

      for ( auto& tt : t1 )
        swap_adjacent_inplace( tt, pos );

      std::swap( perm[pos], perm[pos + 1] );

      fn( t1, phase, perm );
    }
  }
}

/*! \brief Exact P enumeration

  Given a truth table, this function enumerates all the functions in its
  P class. Two functions are in the same P class, if one can be obtained
  from the other by input permutation.

  The function takes a callback as second parameter which is called for
  every enumerated function. The callback should take as parameters:
  - P-enumerated truth table
  - input permutation to apply

  \param tt Truth table
  \param fn Callback for each enumerated truth table in the P class
*/
template<typename TT, typename Callback>
void exact_p_enumeration( const TT& tt, Callback&& fn )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    fn( tt, std::vector<uint8_t>{} );
    return;
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    fn( tt, std::vector<uint8_t>{ 0 } );
    return;
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  fn( t1, perm );

  const auto& swaps = detail::swaps[num_vars - 2u];

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];
    swap_adjacent_inplace( t1, pos );

    std::swap( perm[pos], perm[pos + 1] );

    fn( t1, perm );
  }
}

/*! \brief Exact multi P enumeration

  Given multiple truth tables, this function enumerates all the functions in their
  P class. Two functions are in the same P class, if one can be obtained
  from the other by input permutation.

  The function takes a callback as second parameter which is called for
  every enumerated function. The callback should take as parameters:
  - P-enumerated truth tables
  - input permutation to apply

  \param tt Truth tables
  \param fn Callback for each enumerated truth table in the P class
*/
template<typename TT, typename Callback>
void exact_multi_p_enumeration( const std::vector<TT>& tts, Callback&& fn )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  assert( tts.size() > 0 );

  const auto num_vars = tts[0].num_vars();

  for ( auto i = 0; i < tts.size(); ++i )
    assert( tts[i].num_vars == num_vars );

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    fn( tts, std::vector<uint8_t>{} );
    return;
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    fn( tts, std::vector<uint8_t>{ 0 } );
    return;
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tts;

  std::vector<uint8_t> perm( num_vars );
  std::iota( perm.begin(), perm.end(), 0u );

  fn( t1, perm );

  const auto& swaps = detail::swaps[num_vars - 2u];

  for ( std::size_t i = 0; i < swaps.size(); ++i )
  {
    const auto pos = swaps[i];

    for ( auto& tt : t1 )
      swap_adjacent_inplace( tt, pos );

    std::swap( perm[pos], perm[pos + 1] );

    fn( t1, perm );
  }
}

/*! \brief Exact N enumeration

  Given a truth table, this function enumerates all the functions in its
  N class. Two functions are in the same N class, if one can be obtained
  from the other by input negation.

  The function takes a callback as second parameter which is called for
  every enumerated function. The callback should take as parameters:
  - N-enumerated truth table
  - input negation to apply

  \param tt Truth table
  \param fn Callback for each enumerated truth table in the N class
*/
template<typename TT, typename Callback>
void exact_n_enumeration( const TT& tt, Callback&& fn )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    fn( tt, 0 );
    return;
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    fn( tt, 0 );
    return;
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;
  fn( t1, 0 );

  const auto& flips = detail::flips[num_vars - 2u];
  uint32_t phase = 0;

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    flip_inplace( t1, pos );

    phase ^= 1 << pos;

    fn( t1, phase );
  }
}

/*! \brief Exact N canonization complete

  Given a truth table, this function finds the lexicographically smallest truth
  table in its N class, called N representative. Two functions are in the
  same N class, if one can obtain one from the other by input negations.

  The function can accept a callback as second parameter which is called for
  every visited function when trying out all combinations.  This allows to
  exhaustively visit the whole N class.

  The function returns all the N configurations which contains the necessary
  transformations to obtain the representative.  It is a tuple of

  - the N representative
  - a vector of all input negations that lead to the representative

  \param tt The truth table
  \param fn Callback for each visited truth table in the class (default does nothing)
  \return N configurations
*/
template<typename TT, typename Callback = decltype( detail::exact_npn_canonization_null_callback<TT> )>
std::tuple<TT, std::vector<uint32_t>> exact_n_canonization_complete( const TT& tt, Callback&& fn = detail::exact_npn_canonization_null_callback<TT> )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto num_vars = tt.num_vars();

  /* Special case for n = 0 */
  if ( num_vars == 0 )
  {
    return std::make_tuple( tt, std::vector<uint32_t>{ 0 } );
  }

  /* Special case for n = 1 */
  if ( num_vars == 1 )
  {
    return std::make_tuple( tt, std::vector<uint32_t>{ 0 } );
  }

  assert( num_vars >= 2 && num_vars <= 6 );

  auto t1 = tt;
  auto tmin = t1;

  fn( t1 );

  const auto& flips = detail::flips[num_vars - 2u];

  std::vector<int> best_flip{ -1 };

  for ( std::size_t j = 0; j < flips.size(); ++j )
  {
    const auto pos = flips[j];
    flip_inplace( t1, pos );

    fn( t1 );

    if ( t1 < tmin )
    {
      best_flip.erase( best_flip.begin() + 1, best_flip.end() );
      best_flip[0] = static_cast<int>( j );
      tmin = t1;
    }
    else if ( t1 == tmin )
    {
      best_flip.push_back( static_cast<int>( j ) );
    }
  }

  std::vector<uint32_t> phases( best_flip.size() );
  uint32_t phase = 0;
  int cnt = 0;
  for ( auto i = 0u; i < best_flip.size(); ++i )
  {
    auto flip = best_flip[i];
    for ( ; cnt <= flip; ++cnt )
    {
      phase ^= 1 << flips[cnt];
    }
    phases[i] = phase;
  }

  return std::make_tuple( tmin, phases );
}

/*! \brief Obtain truth table from NPN configuration

  Given an NPN configuration, which contains a representative
  function, input/output negations, and input permutations this
  function computes the original truth table.

  \param config NPN configuration
*/
template<typename TT>
TT create_from_npn_config( const std::tuple<TT, uint32_t, std::vector<uint8_t>>& config )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto& from = std::get<0>( config );
  const auto& phase = std::get<1>( config );
  auto perm = std::get<2>( config );
  const auto num_vars = from.num_vars();

  /* is output complemented? */
  auto res = ( ( phase >> num_vars ) & 1 ) ? ~from : from;

  /* input permutations */
  for ( auto i = 0u; i < num_vars; ++i )
  {
    if ( perm[i] == i )
    {
      continue;
    }

    int k = i;
    while ( perm[k] != i )
    {
      ++k;
    }

    swap_inplace( res, i, k );
    std::swap( perm[i], perm[k] );
  }

  /* input complementations */
  for ( auto i = 0u; i < num_vars; ++i )
  {
    if ( ( phase >> i ) & 1 )
    {
      flip_inplace( res, i );
    }
  }

  return res;
}

/*! \brief Obtain truth table applying a NPN configuration

  Given an NPN configuration composed of input/output negations,
  and input permutations this function applies the transformation
  to the input truth table. This function can be used to obtain
  the NPN representative function given the NPN transformation.
  This function is the inverse of `create_from_npn_config`.

  \param from truth table
  \param phase input/output negations to apply
  \param perm input permutations to apply
*/
template<typename TT>
TT apply_npn_transformation( TT const& from, uint32_t phase, std::vector<uint8_t> const& perm )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  /* transpose the permutation vector */
  std::vector<uint8_t> perm_transposed( perm.size() );
  for ( auto i = 0; i < perm.size(); ++i )
    perm_transposed[perm[i]] = i;

  const auto num_vars = from.num_vars();

  /* is output complemented? */
  auto res = ( ( phase >> num_vars ) & 1 ) ? ~from : from;

  /* input complementations */
  for ( auto i = 0u; i < num_vars; ++i )
  {
    if ( ( phase >> i ) & 1 )
    {
      flip_inplace( res, i );
    }
  }

  /* input permutations */
  for ( auto i = 0u; i < num_vars; ++i )
  {
    if ( perm_transposed[i] == i )
    {
      continue;
    }

    int k = i;
    while ( perm_transposed[k] != i )
    {
      ++k;
    }

    swap_inplace( res, i, k );
    std::swap( perm_transposed[i], perm_transposed[k] );
  }

  return res;
}

} /* namespace kitty */