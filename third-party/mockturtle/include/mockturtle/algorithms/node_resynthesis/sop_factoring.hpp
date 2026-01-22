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
  \file sop_factoring.hpp
  \brief Resynthesis with SOP factoring

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/isop.hpp>
#include <kitty/operators.hpp>

#include "../../utils/sop_utils.hpp"
#include "../../utils/stopwatch.hpp"

namespace mockturtle
{

/*! \brief Parameters for sop_factoring function. */
struct sop_factoring_params
{
  /*! \brief Select divisors with a quick algorithm. */
  bool use_quick_factoring{ true };

  /*! \brief Factoring is also tried for the negated TT. */
  bool try_both_polarities{ true };

  /*! \brief Factoring considers input and output inverters as additional cost. */
  bool consider_inverter_cost{ false };
};

/*! \brief Resynthesis function based on SOP factoring.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``. The method converts a
 * given truth table in an ISOP, then factors the ISOP, and
 * returns the factored form.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      aig_network aig = ...;

      sop_factoring<aig_network> resyn;
      refactoring( aig, resyn );
   \endverbatim
 *
 */
template<class Ntk>
class sop_factoring
{
public:
  using signal = typename Ntk::signal;
  using sop_t = std::vector<uint64_t>;

public:
  explicit sop_factoring( sop_factoring_params const& ps = {} )
      : _ps( ps ) {}

public:
  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& dest, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    assert( function.num_vars() <= 31 );

    if ( kitty::is_const0( function ) )
    {
      /* constant 0 */
      fn( dest.get_constant( false ) );
      return;
    }
    else if ( kitty::is_const0( ~function ) )
    {
      /* constant 1 */
      fn( dest.get_constant( true ) );
      return;
    }

    /* derive ISOP */
    bool negated;
    auto cubes = get_isop( function, negated );

    /* create literal form of SOP */
    sop_t sop = cubes_to_sop( cubes, function.num_vars() );

    /* derive the factored form */
    signal f = gen_factor_rec( dest, { begin, end }, sop, 2 * function.num_vars() );

    fn( negated ? !f : f );
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& dest, kitty::dynamic_truth_table const& function, kitty::dynamic_truth_table const& dc, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    assert( function.num_vars() <= 31 );

    if ( kitty::is_const0( function & ( ~dc ) ) )
    {
      /* constant 0 */
      fn( dest.get_constant( false ) );
      return;
    }
    else if ( kitty::is_const0( ~( function | dc ) ) )
    {
      /* constant 1 */
      fn( dest.get_constant( true ) );
      return;
    }

    /* derive ISOP */
    bool negated;
    auto cubes = get_isop_dc( function, dc, negated );

    /* create literal form of SOP */
    sop_t sop = cubes_to_sop( cubes, function.num_vars() );

    /* derive the factored form */
    signal f = gen_factor_rec( dest, { begin, end }, sop, 2 * function.num_vars() );

    fn( negated ? !f : f );
  }

private:
  std::vector<kitty::cube> get_isop( kitty::dynamic_truth_table const& function, bool& negated ) const
  {
    std::vector<kitty::cube> cubes = kitty::isop( function );

    if ( _ps.try_both_polarities )
    {
      std::vector<kitty::cube> n_cubes = kitty::isop( ~function );

      if ( _ps.consider_inverter_cost )
      {
        uint32_t n_lit = 0;
        uint32_t lit = 0;
        kitty::cube n_term;
        kitty::cube term;
        for ( auto const& c : n_cubes )
        {
          n_term._mask |= c._mask & ( ~c )._bits;
          n_lit += c.num_literals();
        }
        for ( auto const& c : cubes )
        {
          term._mask |= c._mask & ( ~c )._bits;
          lit += c.num_literals();
        }

        /* positive cost: cubes + input negations + output negation */
        uint32_t positive_cost = cubes.size() + term.num_literals() + 1;
        /* negative cost: cubes + input negations */
        uint32_t negative_cost = n_cubes.size() + n_term.num_literals();

        if ( negative_cost < positive_cost )
        {
          negated = true;
          return n_cubes;
        }
      }
      else
      {
        if ( n_cubes.size() < cubes.size() )
        {
          negated = true;
          return n_cubes;
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
            negated = true;
            return n_cubes;
          }
        }
      }
    }

    negated = false;
    return cubes;
  }

  std::vector<kitty::cube> get_isop_dc( kitty::dynamic_truth_table const& function, kitty::dynamic_truth_table const& dc, bool& negated ) const
  {
    std::vector<kitty::cube> cubes;
    kitty::detail::isop_rec( function & ~dc, function | dc, function.num_vars(), cubes );

    if ( _ps.try_both_polarities )
    {
      std::vector<kitty::cube> n_cubes;
      kitty::detail::isop_rec( ~function & ~dc, ~function | dc, function.num_vars(), n_cubes );

      if ( _ps.consider_inverter_cost )
      {
        uint32_t n_lit = 0;
        uint32_t lit = 0;
        kitty::cube n_term;
        kitty::cube term;
        for ( auto const& c : n_cubes )
        {
          n_term._mask |= c._mask & ( ~c )._bits;
          n_lit += c.num_literals();
        }
        for ( auto const& c : cubes )
        {
          term._mask |= c._mask & ( ~c )._bits;
          lit += c.num_literals();
        }

        /* positive cost: cubes + input negations + output negation */
        uint32_t positive_cost = cubes.size() + term.num_literals() + 1;
        /* negative cost: cubes + input negations */
        uint32_t negative_cost = n_cubes.size() + n_term.num_literals();

        if ( negative_cost < positive_cost )
        {
          negated = true;
          return n_cubes;
        }
      }
      else
      {
        if ( n_cubes.size() < cubes.size() )
        {
          negated = true;
          return n_cubes;
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
            negated = true;
            return n_cubes;
          }
        }
      }
    }

    negated = false;
    return cubes;
  }

#pragma region SOP factoring
  signal gen_factor_rec( Ntk& ntk, std::vector<signal> const& children, sop_t& sop, uint32_t const num_lit ) const
  {
    sop_t divisor, quotient, reminder;

    assert( sop.size() );

    /* compute the divisor */
    bool success = _ps.use_quick_factoring ? sop_quick_divisor( sop, divisor, num_lit ) : sop_good_divisor( sop, divisor, num_lit );
    if ( !success )
    {
      /* generate trivial sop circuit */
      return gen_andor_circuit_rec( ntk, children, sop.begin(), sop.end(), num_lit );
    }

    /* divide the SOP by the divisor */
    sop_divide( sop, divisor, quotient, reminder );

    assert( quotient.size() > 0 );

    if ( quotient.size() == 1 )
    {
      return lit_factor_rec( ntk, children, sop, quotient[0], num_lit );
    }
    sop_make_cube_free( quotient );

    /* divide the SOP by the quotient */
    sop_divide( sop, quotient, divisor, reminder );

    if ( sop_is_cube_free( divisor ) )
    {
      signal div_s = gen_factor_rec( ntk, children, divisor, num_lit );
      signal quot_s = gen_factor_rec( ntk, children, quotient, num_lit );

      /* build (D)*(Q) + R */
      signal dq_and = ntk.create_and( div_s, quot_s );

      if ( reminder.size() )
      {
        signal rem_s = gen_factor_rec( ntk, children, reminder, num_lit );
        return ntk.create_or( dq_and, rem_s );
      }

      return dq_and;
    }

    /* get the common cube */
    uint64_t cube = UINT64_MAX;
    for ( auto const& c : divisor )
    {
      cube &= c;
    }

    return lit_factor_rec( ntk, children, sop, cube, num_lit );
  }

  signal lit_factor_rec( Ntk& ntk, std::vector<signal> const& children, sop_t const& sop, uint64_t const c_sop, uint32_t const num_lit ) const
  {
    sop_t divisor, quotient, reminder;

    /* extract the best literal */
    detail::sop_best_literal( sop, divisor, c_sop, num_lit );

    /* divide SOP by the literal */
    sop_divide_by_cube( sop, divisor, quotient, reminder );

    /* create the divisor: cube */
    signal div_s = gen_and_circuit_rec( ntk, children, divisor[0], 0, num_lit );

    /* factor the quotient */
    signal quot_s = gen_factor_rec( ntk, children, quotient, num_lit );

    /* build l*Q + R */
    signal dq_and = ntk.create_and( div_s, quot_s );

    /* factor the reminder */
    if ( reminder.size() != 0 )
    {
      signal rem_s = gen_factor_rec( ntk, children, reminder, num_lit );
      return ntk.create_or( dq_and, rem_s );
    }

    return dq_and;
  }
#pragma endregion

#pragma region Circuit generation from SOP
  signal gen_and_circuit_rec( Ntk& ntk, std::vector<signal> const& children, uint64_t const cube, uint32_t const begin, uint32_t const end ) const
  {
    /* count set literals */
    uint32_t num_lit = 0;
    uint32_t lit = begin;
    uint32_t i;
    for ( i = begin; i < end; ++i )
    {
      if ( detail::cube_has_lit( cube, i ) )
      {
        ++num_lit;
        lit = i;
      }
    }

    assert( num_lit > 0 );

    if ( num_lit == 1 )
    {
      /* return the coprresponding signal with the correct polarity */
      if ( lit % 2 == 1 )
        return children[lit / 2];
      else
        return !children[lit / 2];
    }

    /* find splitting point */
    uint32_t count_lit = 0;
    for ( i = begin; i < end; ++i )
    {
      if ( detail::cube_has_lit( cube, i ) )
      {
        if ( count_lit >= num_lit / 2 )
          break;

        ++count_lit;
      }
    }

    signal tree1 = gen_and_circuit_rec( ntk, children, cube, begin, i );
    signal tree2 = gen_and_circuit_rec( ntk, children, cube, i, end );

    return ntk.create_and( tree1, tree2 );
  }

  signal gen_andor_circuit_rec( Ntk& ntk, std::vector<signal> const& children, sop_t::const_iterator const& begin, sop_t::const_iterator const& end, uint32_t const num_lit ) const
  {
    auto num_prod = std::distance( begin, end );

    assert( num_prod > 0 );

    if ( num_prod == 1 )
      return gen_and_circuit_rec( ntk, children, *begin, 0, num_lit );

    /* create or tree */
    signal tree1 = gen_andor_circuit_rec( ntk, children, begin, begin + num_prod / 2, num_lit );
    signal tree2 = gen_andor_circuit_rec( ntk, children, begin + num_prod / 2, end, num_lit );

    return ntk.create_or( tree1, tree2 );
  }
#pragma endregion

private:
  sop_factoring_params const& _ps;

  mutable stopwatch<>::duration time_factoring{};
};

} // namespace mockturtle
