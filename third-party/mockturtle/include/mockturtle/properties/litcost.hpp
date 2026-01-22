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
  \file litcost.hpp
  \brief Cost function based on the factored literal cost

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <cstdint>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/isop.hpp>

#include "../utils/sop_utils.hpp"

namespace mockturtle
{

namespace detail
{
uint32_t count_literals_rec( std::vector<uint64_t>&, uint32_t const );

uint32_t count_term_literals( uint64_t const term, uint32_t const num_lit )
{
  uint32_t lit = 0;

  for ( auto i = 0u; i < num_lit; ++i )
  {
    if ( cube_has_lit( term, i ) )
      ++lit;
  }

  return lit;
}

uint32_t lit_factor_count_rec( std::vector<uint64_t> const& sop, uint64_t const c_sop, uint32_t const num_lit )
{
  using sop_t = std::vector<uint64_t>;

  sop_t divisor, quotient, reminder;

  /* extract the best literal */
  sop_best_literal( sop, divisor, c_sop, num_lit );

  /* divide SOP by the literal */
  sop_divide_by_cube( sop, divisor, quotient, reminder );

  /* count literals in the divisor: cube */
  uint32_t div_lit = count_term_literals( divisor[0], num_lit );

  /* factor the quotient */
  uint32_t quot_lit = count_literals_rec( quotient, num_lit );

  /* factor the reminder */
  if ( reminder.size() != 0 )
  {
    return div_lit + quot_lit + count_literals_rec( reminder, num_lit );
  }

  return div_lit + quot_lit;
}

uint32_t count_literals_rec( std::vector<uint64_t>& sop, uint32_t const num_lit )
{
  using sop_t = std::vector<uint64_t>;

  assert( sop.size() );

  sop_t divisor, quotient, reminder;

  /* compute the divisor */
  if ( !sop_quick_divisor( sop, divisor, num_lit ) )
  {
    /* count_literals of the current SOP */
    return sop_count_literals( sop );
  }

  /* divide the SOP by the divisor */
  sop_divide( sop, divisor, quotient, reminder );

  assert( quotient.size() > 0 );

  if ( quotient.size() == 1 )
  {
    return lit_factor_count_rec( sop, quotient[0], num_lit );
  }

  sop_make_cube_free( quotient );

  /* divide the SOP by the quotient */
  sop_divide( sop, quotient, divisor, reminder );

  if ( sop_is_cube_free( divisor ) )
  {
    uint32_t div_lit = count_literals_rec( divisor, num_lit );
    uint32_t quot_lit = count_literals_rec( quotient, num_lit );

    if ( reminder.size() )
    {
      return div_lit + quot_lit + count_literals_rec( reminder, num_lit );
    }

    return div_lit + quot_lit;
  }

  /* get the common cube */
  uint64_t cube = UINT64_MAX;
  for ( auto const& c : divisor )
  {
    cube &= c;
  }

  return lit_factor_count_rec( sop, cube, num_lit );
}

} // namespace detail

/*! \brief Counts number of literals of the factored form of a SOP.
 *
 * This method computes the factored form of the SOP and
 * returns its number of factored form literals.
 *
 * \param sop Sum-of-products
 * \param num_vars Number of variables
 */
uint32_t factored_literal_cost( std::vector<kitty::cube> const& sop, uint32_t num_vars )
{
  /* trivial cases: constant 0 or 1 */
  if ( sop.size() == 0 || sop.size() == 1 && sop[0]._mask == 0 )
    return 0;

  using sop_t = std::vector<uint64_t>;
  sop_t lit_sop = cubes_to_sop( sop, num_vars );

  return detail::count_literals_rec( lit_sop, num_vars * 2 );
}

/*! \brief Counts number of literals of the factored form of a SOP.
 *
 * This method computes the factored form of a completely specified
 * function given as a truth table and returns its number of
 * factored form literals.
 *
 * \param tt function as truth table
 * \param try_both_polarities factoring is also tried for the negated TT
 */
uint32_t factored_literal_cost( kitty::dynamic_truth_table const& tt, bool try_both_polarities = false )
{
  if ( kitty::is_const0( tt ) || kitty::is_const0( ~tt ) )
  {
    /* constant */
    return 0;
  }

  std::vector<kitty::cube> cubes = kitty::isop( tt );

  if ( try_both_polarities )
  {
    std::vector<kitty::cube> n_cubes = kitty::isop( ~tt );

    if ( n_cubes.size() < cubes.size() )
    {
      cubes = n_cubes;
    }
    else if ( n_cubes.size() == cubes.size() )
    {
      uint32_t n_lit = 0;
      uint32_t lit = 0;
      for ( auto const& c : n_cubes )
      {
        n_lit += c.num_literals();
      }
      for ( auto const& c : cubes )
      {
        lit += c.num_literals();
      }

      if ( n_lit < lit )
      {
        cubes = n_cubes;
      }
    }
  }

  return factored_literal_cost( cubes, tt.num_vars() );
}

/*! \brief Counts number of literals of the factored form of a SOP.
 *
 * This method computes the factored form of an incompletely specified
 * function given as a truth table and its don't care set and returns
 * its number of factored form literals.
 *
 * \param tt function as truth table
 * \param dc don't care set
 * \param try_both_polarities factoring is also tried for the negated TT
 */
uint32_t factored_literal_cost( kitty::dynamic_truth_table const& tt, kitty::dynamic_truth_table const& dc, bool try_both_polarities = false )
{
  if ( kitty::is_const0( tt & ( ~dc ) ) || kitty::is_const0( ~( tt | dc ) ) )
  {
    /* constant */
    return 0;
  }

  std::vector<kitty::cube> cubes;
  kitty::detail::isop_rec( tt & ~dc, tt | dc, tt.num_vars(), cubes );

  if ( try_both_polarities )
  {
    std::vector<kitty::cube> n_cubes;
    kitty::detail::isop_rec( ~tt & ~dc, ~tt | dc, tt.num_vars(), n_cubes );

    if ( n_cubes.size() < cubes.size() )
    {
      cubes = n_cubes;
    }
    else if ( n_cubes.size() == cubes.size() )
    {
      uint32_t n_lit = 0;
      uint32_t lit = 0;
      for ( auto const& c : n_cubes )
      {
        n_lit += c.num_literals();
      }
      for ( auto const& c : cubes )
      {
        lit += c.num_literals();
      }

      if ( n_lit < lit )
      {
        cubes = n_cubes;
      }
    }
  }

  return factored_literal_cost( cubes, tt.num_vars() );
}

} // namespace mockturtle
