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
  \file exact_mc_synthesis.hpp
  \brief SAT-based XAG synthesis based on MC

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <bill/sat/interface/common.hpp>
#include <bill/sat/interface/glucose.hpp>
#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operations.hpp>
#include <kitty/properties.hpp>

#include "../algorithms/simulation.hpp"
#include "../generators/sorting.hpp"
#include "../io/write_verilog.hpp"
#include "../networks/xag.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/cnf_view.hpp"
#include "cnf.hpp"

namespace mockturtle
{

struct exact_mc_synthesis_params
{
  /* \brief Minimum number of AND gates. */
  uint32_t min_and_gates{ 0u };

  /*! \brief Use CEGAR based solving strategy. */
  bool use_cegar{ false };

  /*! \brief Use subset symmetry breaking. */
  bool break_subset_symmetries{ true };

  /*! \brief Use multi-level subset symmetry breaking. */
  bool break_multi_level_subset_symmetries{ true };

  /*! \brief Use symmetric variables. */
  bool break_symmetric_variables{ true };

  /*! \brief User-specified variable symmetries. */
  std::vector<std::pair<uint32_t, uint32_t>> custom_symmetric_variables;

  /*! \brief Ensure to use all gates and essential variables. */
  bool ensure_to_use_gates{ true };

  /*! \brief Heuristic XOR bound (based on sorter network). */
  std::optional<uint32_t> heuristic_xor_bound{};

  /*! \brief Updates XOR bound heuristic after each found solution. */
  bool auto_update_xor_bound{ false };

  /*! \brief Conflict limit for the SAT solver. */
  uint32_t conflict_limit{ 0u };

  /*! \brief Use conflict limit only when searching for multiple solutions
   *
   * The conflict limit will be ignored for the first call.
   */
  bool ignore_conflict_limit_for_first_solution{ false };

  /*! \brief Show progress (in CEGAR). */
  bool progress{ false };

  /*! \brief Write DIMACS file, everytime solve is called. */
  std::optional<std::string> write_dimacs{};

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Be very verbose */
  bool very_verbose{ false };
};

struct exact_mc_synthesis_stats
{
  /*! \brief Total time. */
  stopwatch<>::duration time_total{};

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_solving{};

  /*! \brief Total number of variables. */
  uint32_t num_vars{};

  /*! \brief Total number of clauses. */
  uint32_t num_clauses{};

  /*! \brief Prints report. */
  void report() const
  {
    fmt::print( "[i] total time    = {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i] solving time  = {:>5.2f} secs\n", to_seconds( time_solving ) );
    fmt::print( "[i] total vars    = {}\n", num_vars );
    fmt::print( "[i] total clauses = {}\n", num_clauses );
  }
};

namespace detail
{

template<class Ntk, bill::solvers Solver>
struct exact_mc_synthesis_impl
{
  using problem_network_t = cnf_view<xag_network, false, Solver>;

  exact_mc_synthesis_impl( kitty::dynamic_truth_table const& func, uint32_t num_solutions, exact_mc_synthesis_params const& ps, exact_mc_synthesis_stats& st )
      : num_vars_( func.num_vars() ),
        func_( kitty::get_bit( func, 0 ) ? ~func : func ),
        invert_( kitty::get_bit( func, 0 ) ),
        heuristic_xor_bound_( ps.heuristic_xor_bound ),
        num_solutions_( num_solutions ),
        ps_( ps ),
        st_( st )
  {
  }

  std::vector<Ntk> run()
  {
    stopwatch<> t( st_.time_total );

    std::vector<Ntk> ntks;
    const auto degree = kitty::polynomial_degree( func_ );
    uint32_t num_ands = std::max( ps_.min_and_gates, degree == 0u ? degree : degree - 1u );

    while ( true )
    {
      if ( ps_.verbose )
      {
        fmt::print( "try with {} AND gates\n", num_ands );
      }

      cnf_view_params cvps;
      cvps.write_dimacs = ps_.write_dimacs;
      problem_network_t pntk( cvps );
      reset( pntk );

      for ( auto i = 0u; i < num_ands; ++i )
      {
        add_gate( pntk );
      }
      add_output( pntk );
      if ( ps_.heuristic_xor_bound || ps_.auto_update_xor_bound )
      {
        add_xor_counter( pntk );
      }

      // TODO use LUT mapping before CNF generation
      if ( const auto sol = ps_.use_cegar ? solve_with_cegar( pntk ) : solve_direct( pntk ); sol )
      {
        ntks.push_back( *sol );
        if ( ps_.very_verbose )
        {
          debug_solution( pntk );
        }
        while ( ntks.size() < num_solutions_ )
        {
          block( pntk );
          if ( const auto result = solve( pntk, false ); result && *result )
          {
            ntks.push_back( extract_network( pntk ) );
            if ( ps_.very_verbose )
            {
              debug_solution( pntk );
              fmt::print( "[i] found {} solutions so far\n", ntks.size() );
            }
          }
          else
          {
            break;
          }
        }
        return ntks;
      }
      ++num_ands;
    }
  }

private:
  std::optional<Ntk> solve_direct( problem_network_t& pntk )
  {
    prune_search_space( pntk );

    for ( auto b = 1u; b < func_.num_bits(); ++b )
    {
      constrain_assignment( pntk, b );
    }

    st_.num_vars += pntk.num_vars();
    st_.num_clauses += pntk.num_clauses();
    if ( const auto result = solve( pntk, true ); result && *result )
    {
      return extract_network( pntk );
    }
    else
    {
      return std::nullopt;
    }
  }

  std::optional<Ntk> solve_with_cegar( problem_network_t& pntk )
  {
    prune_search_space( pntk );

    uint32_t num_ands = static_cast<uint32_t>( ltfi_vars_.size() ) / 2, bctr = 0u;
    progress_bar pbar( static_cast<uint32_t>( func_.num_bits() ), "exact_mc_synthesis |{}| ANDs = {}   asserted bits = {}   SAT solving time = {:.2f} secs", ps_.progress );
    while ( true )
    {
      pbar( bctr, num_ands, bctr, to_seconds( st_.time_solving ) );
      if ( const auto result = solve( pntk, true ); result && *result )
      {
        const auto sol = extract_network( pntk );
        default_simulator<kitty::dynamic_truth_table> sim( num_vars_ );
        const auto simulated = simulate<kitty::dynamic_truth_table>( sol, sim )[0u];
        if ( const auto bit = kitty::find_first_bit_difference( func_, simulated ); bit == -1 )
        {
          st_.num_vars += pntk.num_vars();
          st_.num_clauses += pntk.num_clauses();
          return sol;
        }
        else
        {
          constrain_assignment( pntk, static_cast<uint32_t>( bit ) );
          bctr++;
        }
      }
      else
      {
        st_.num_vars += pntk.num_vars();
        st_.num_clauses += pntk.num_clauses();
        return std::nullopt;
      }
    }
  }

  std::optional<bool> solve( problem_network_t& pntk, bool first )
  {
    stopwatch<> t_sat( st_.time_solving );
    bill::result::clause_type assumptions;
    pntk.foreach_po( [&]( auto const& f ) {
      assumptions.push_back( pntk.lit( f ) );
    } );
    if ( heuristic_xor_bound_ )
    {
      if ( int32_t pos = static_cast<int32_t>( xor_counter_.size() ) - *heuristic_xor_bound_ - 1; pos >= 0 )
      {
        assumptions.push_back( pntk.lit( !xor_counter_[pos] ) );
      }
    }
    const auto res = pntk.solve( assumptions, ps_.ignore_conflict_limit_for_first_solution && first ? 0u : ps_.conflict_limit );

    if ( ps_.auto_update_xor_bound && res && *res )
    {
      heuristic_xor_bound_ = count_xors( pntk ) - 1u;
    }

    return res;
  }

private:
  Ntk extract_network( problem_network_t& pntk )
  {
    Ntk xag;
    std::vector<signal<Ntk>> nodes( num_vars_ );
    std::generate( nodes.begin(), nodes.end(), [&]() { return xag.create_pi(); } );

    const auto extract_ltfi = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars ) -> signal<Ntk> {
      std::vector<signal<Ntk>> ltfi;
      for ( auto j = 0u; j < ltfi_vars.size(); ++j )
      {
        if ( pntk.model_value( ltfi_vars[j] ) )
        {
          ltfi.push_back( nodes[j] );
        }
      }
      return xag.create_nary_xor( ltfi );
    };

    for ( auto i = 0u; i < ltfi_vars_.size() / 2; ++i )
    {
      nodes.push_back( xag.create_and( extract_ltfi( ltfi_vars_[2 * i] ), extract_ltfi( ltfi_vars_[2 * i + 1] ) ) );
    }

    const auto c = extract_ltfi( ltfi_vars_.back() );
    xag.create_po( invert_ ? xag.create_not( c ) : c );

    return xag;
  }

  void block( problem_network_t& pntk )
  {
    std::vector<signal<problem_network_t>> blocked_lits;
    for ( auto const& ltfi : ltfi_vars_ )
    {
      for ( auto const& l : ltfi )
      {
        blocked_lits.push_back( l ^ pntk.model_value( l ) );
      }
    }
    pntk.add_clause( blocked_lits );
  }

  void reset( problem_network_t const& pntk )
  {
    // TODO: can we make this more iterative?
    ltfi_vars_.clear();
    truth_vars_.clear();
    truth_vars_.resize( func_.num_bits() );

    /* pre-assign truth_vars_ with primary inputs */
    for ( auto i = 0u; i < num_vars_; ++i )
    {
      const auto var_tt = kitty::nth_var<kitty::dynamic_truth_table>( num_vars_, i );
      for ( auto b = 0u; b < func_.num_bits(); ++b )
      {
        truth_vars_[b].push_back( pntk.get_constant( kitty::get_bit( var_tt, b ) ) );
      }
    }
  }

  void add_gate( problem_network_t& pntk )
  {
    uint32_t gate_index = static_cast<uint32_t>( ltfi_vars_.size() ) / 2;

    // add select variables
    for ( auto j = 0u; j < 2u; ++j )
    {
      ltfi_vars_.push_back( std::vector<signal<problem_network_t>>( num_vars_ + gate_index ) );
      std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
    }
  }

  void add_output( problem_network_t& pntk )
  {
    ltfi_vars_.push_back( std::vector<signal<problem_network_t>>( num_vars_ + ltfi_vars_.size() / 2 ) );
    std::generate( ltfi_vars_.back().begin(), ltfi_vars_.back().end(), [&]() { return pntk.create_pi(); } );
  }

  void constrain_assignment( problem_network_t& pntk, uint32_t bit )
  {
    const auto create_xor_clause = [&]( std::vector<signal<problem_network_t>> const& ltfi_vars ) -> signal<problem_network_t> {
      std::vector<signal<problem_network_t>> ltfi( ltfi_vars.size() );
      for ( auto j = 0u; j < ltfi.size(); ++j )
      {
        ltfi[j] = pntk.create_and( ltfi_vars[j], truth_vars_[bit][j] );
      }
      return pntk.create_nary_xor( ltfi );
    };

    for ( auto i = 0u; i < ltfi_vars_.size() / 2; ++i )
    {
      truth_vars_[bit].push_back( pntk.create_and( create_xor_clause( ltfi_vars_[2 * i] ), create_xor_clause( ltfi_vars_[2 * i + 1] ) ) );
    }

    const auto po_signal = create_xor_clause( ltfi_vars_.back() );
    pntk.create_po( kitty::get_bit( func_, bit ) ? po_signal : pntk.create_not( po_signal ) );
  }

  void prune_search_space( problem_network_t& pntk )
  {
    // At least one element in LTFI
    for ( auto const& ltfi : ltfi_vars_ )
    {
      pntk.add_clause( ltfi );
    }

    // linear TFIs are no subset of each other
    if ( ps_.break_subset_symmetries )
    {
      for ( auto i = 0u; i < ltfi_vars_.size() / 2u; ++i )
      {
        auto const& ltfi1 = ltfi_vars_[2 * i];
        auto const& ltfi2 = ltfi_vars_[2 * i + 1];

        std::vector<signal<problem_network_t>> ands( ltfi1.size() );
        std::vector<signal<problem_network_t>> ands2( ltfi1.size() );
        for ( auto j = 0u; j < ltfi1.size(); ++j )
        {
          ands[j] = pntk.create_and( ltfi1[j], pntk.create_not( ltfi2[j] ) );
          ands2[j] = pntk.create_and( ltfi2[j], pntk.create_not( ltfi1[j] ) );
        }
        pntk.add_clause( ands );
        pntk.add_clause( ands2 );
      }
    }

    // left linear TFI is lexicographically smaller than right one
    for ( auto i = 0u; i < ltfi_vars_.size() / 2u; ++i )
    {
      auto const& ltfi2 = ltfi_vars_[2 * i];
      auto const& ltfi1 = ltfi_vars_[2 * i + 1];

      auto n = ltfi1.size();
      std::vector<signal<problem_network_t>> as( n - 1u );
      std::generate( as.begin(), as.end(), [&]() { return pntk.create_pi(); } );

      pntk.add_clause( !ltfi1[0], ltfi2[0] );
      pntk.add_clause( !ltfi1[0], as[0] );
      pntk.add_clause( ltfi2[0], as[0] );

      for ( auto k = 1u; k < n - 1; ++k )
      {
        pntk.add_clause( !ltfi1[k], ltfi2[k], !as[k - 1] );
        pntk.add_clause( !ltfi1[k], as[k], !as[k - 1] );
        pntk.add_clause( ltfi2[k], as[k], !as[k - 1] );
      }
      pntk.add_clause( !ltfi1.back(), !as.back() );
      pntk.add_clause( ltfi2.back(), !as.back() );
    }

    // break on multi-level subset relation
    if ( ps_.break_multi_level_subset_symmetries )
    {
      for ( auto ii = 0u; ii < ltfi_vars_.size(); ++ii )
      {
        const auto& ltfi = ltfi_vars_[ii];
        for ( auto i = 0u; i < ii / 2u; ++i )
        {
          const auto n = ltfi_vars_[2 * i].size();
          std::vector<signal<problem_network_t>> ands_left, ands_right;
          ands_left.push_back( ltfi[num_vars_ + i] );
          for ( auto k = 0u; k < n; ++k )
          {
            ands_left.push_back( pntk.create_or( !ltfi[k], ltfi_vars_[2 * i][k] ) );
            ands_left.push_back( pntk.create_or( !ltfi[k], ltfi_vars_[2 * i + 1][k] ) );
            ands_right.push_back( pntk.create_xnor( ltfi[k], pntk.create_and( ltfi_vars_[2 * i][k], ltfi_vars_[2 * i + 1][k] ) ) );
          }
          pntk.create_po( pntk.create_or( !pntk.create_nary_and( ands_left ), pntk.create_nary_and( ands_right ) ) );
        }
      }
    }

    // break on symmetric variables
    if ( ps_.break_symmetric_variables )
    {
      const auto break_symmetric_vars = [&]( auto j, auto jj ) {
        if ( ps_.very_verbose )
        {
          fmt::print( "[i] symmetry breaking based on symmetric variables {} and {}\n", j, jj );
        }
        for ( auto ii = 0u; ii < ltfi_vars_.size(); ++ii )
        {
          std::vector<signal<problem_network_t>> clause;
          clause.push_back( !ltfi_vars_[ii][jj] );
          for ( auto i = 0u; i <= ii; ++i )
          {
            clause.push_back( ltfi_vars_[i][j] );
          }
          pntk.add_clause( clause );
        }
      };

      for ( auto jj = 1u; jj < num_vars_; ++jj )
      {
        for ( auto j = 0u; j < jj; ++j )
        {
          if ( kitty::is_symmetric_in( func_, j, jj ) )
          {
            break_symmetric_vars( j, jj );
          }
        }
      }

      for ( const auto& [j, jj] : ps_.custom_symmetric_variables )
      {
        break_symmetric_vars( j, jj );
      }
    }

    // ensure to use essential variables and gates
    if ( ps_.ensure_to_use_gates )
    {
      const auto num_ands = ltfi_vars_.size() / 2;
      for ( auto j = 0u; j < num_vars_ + num_ands; ++j )
      {
        if ( j < num_vars_ && !kitty::has_var( func_, j ) )
        {
          continue;
        }

        std::vector<signal<problem_network_t>> clause;
        for ( auto const& ltfi : ltfi_vars_ )
        {
          if ( j < ltfi.size() )
          {
            clause.push_back( ltfi[j] );
          }
        }
        pntk.add_clause( clause );
      }
    }
  }

  void add_xor_counter( problem_network_t& pntk )
  {
    xor_counter_.clear();
    for ( auto const& ltfi : ltfi_vars_ )
    {
      std::copy( ltfi.begin(), ltfi.end(), std::back_inserter( xor_counter_ ) );
    }

    insertion_sorting_network( static_cast<uint32_t>( xor_counter_.size() ), [&]( auto a, auto b ) {
      auto const aa = pntk.create_and( xor_counter_[a], xor_counter_[b] );
      auto const bb = pntk.create_or( xor_counter_[a], xor_counter_[b] );
      xor_counter_[a] = aa;
      xor_counter_[b] = bb;
    } );
  }

private:
  uint32_t count_xors( problem_network_t& pntk ) const
  {
    uint32_t ctr{};
    for ( auto const& ltfi : ltfi_vars_ )
    {
      for ( auto const& l : ltfi )
      {
        ctr += pntk.model_value( l ) ? 1u : 0u;
      }
    }
    return ctr;
  }

  void debug_solution( problem_network_t& pntk ) const
  {
    const auto num_ands = ltfi_vars_.size() / 2u;
    const auto print_ltfi = [&]( std::vector<signal<problem_network_t>> const& ltfi ) {
      for ( auto const& f : ltfi )
      {
        fmt::print( "{} ", (uint32_t)pntk.model_value( f ) );
      }
      if ( auto padding = 2u * ( num_ands + num_vars_ - ltfi.size() ); padding > 0 )
      {
        fmt::print( "{}", std::string( padding, ' ' ) );
      }
    };

    for ( auto i = 0u; i < ltfi_vars_.size() / 2u; ++i )
    {
      fmt::print( "{:>2} = ", i + 1 );
      print_ltfi( ltfi_vars_[2 * i] );
      fmt::print( "   " );
      print_ltfi( ltfi_vars_[2 * i + 1] );
      fmt::print( "\n" );
    }
    fmt::print( " f = " );
    print_ltfi( ltfi_vars_.back() );
    fmt::print( "\n  XORs = {}\n\n", count_xors( pntk ) );
  }

private:
  uint32_t num_vars_;
  std::vector<std::vector<signal<problem_network_t>>> ltfi_vars_;
  std::vector<std::vector<signal<problem_network_t>>> truth_vars_;
  std::vector<signal<problem_network_t>> xor_counter_;
  kitty::dynamic_truth_table func_;
  bool invert_{ false };
  std::optional<uint32_t> heuristic_xor_bound_;
  uint32_t num_solutions_;
  exact_mc_synthesis_params const& ps_;
  exact_mc_synthesis_stats& st_;
};

} // namespace detail

template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::glucose_41>
Ntk exact_mc_synthesis( kitty::dynamic_truth_table const& func, exact_mc_synthesis_params const& ps = {}, exact_mc_synthesis_stats* pst = nullptr )
{
  exact_mc_synthesis_stats st;
  const auto xag = detail::exact_mc_synthesis_impl<Ntk, Solver>{ func, 1u, ps, st }.run().front();

  if ( ps.verbose )
  {
    st.report();
  }
  if ( pst )
  {
    *pst = st;
  }

  return xag;
}

template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::glucose_41>
std::vector<Ntk> exact_mc_synthesis_multiple( kitty::dynamic_truth_table const& func, uint32_t num_solutions, exact_mc_synthesis_params const& ps = {}, exact_mc_synthesis_stats* pst = nullptr )
{
  exact_mc_synthesis_stats st;
  const auto xags = detail::exact_mc_synthesis_impl<Ntk, Solver>{ func, num_solutions, ps, st }.run();

  if ( ps.verbose )
  {
    st.report();
  }
  if ( pst )
  {
    *pst = st;
  }

  return xags;
}

} /* namespace mockturtle */