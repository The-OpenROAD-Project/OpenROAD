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
  \file satlut_mapping.hpp
  \brief SAT LUT mapping

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cmath>

#include "../generators/sorting.hpp"
#include "../utils/include/percy.hpp"
#include "../utils/node_map.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/topo_view.hpp"
#include "cell_window.hpp"
#include "cut_enumeration.hpp"
#include "cut_enumeration/mf_cut.hpp"

#include <fmt/format.h>

namespace mockturtle
{

/*! \brief Parameters for satlut_mapping.
 *
 * The data structure `satlut_mapping_params` holds configurable parameters with
 * default arguments for `satlut_mapping`.
 */
struct satlut_mapping_params
{
  satlut_mapping_params()
  {
    cut_enumeration_ps.cut_size = 6;
    cut_enumeration_ps.cut_limit = 8;
  }

  /*! \brief Parameters for cut enumeration
   *
   * The default cut size is 6, the default cut limit is 8.
   */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Conflict limit for SAT solver.
   *
   * The default limit is 0, which means the number of conflicts is not used
   * as a resource limit.
   */
  uint32_t conflict_limit{ 0u };

  /*! \brief Show progress. */
  bool progress{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Be very verbose. */
  bool very_verbose{ false };
};

/*! \brief Statistics for satlut_mapping.
 *
 * The data structure `satlut_mapping_stats` provides data collected by running
 * `satlut_mapping`.
 */
struct satlut_mapping_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Total runtime. */
  stopwatch<>::duration time_sat{ 0 };

  /*! \brief Number of SAT variables. */
  uint64_t num_vars{ 0u };

  /*! \brief Number of SAT clauses. */
  uint64_t num_clauses{ 0u };

  void report()
  {
    std::cout << fmt::format( "[i] total time              = {:>7.2f} secs\n", to_seconds( time_total ) )
              << fmt::format( "[i] SAT solving time        = {:>7.2f} secs\n", to_seconds( time_sat ) )
              << fmt::format( "[i] number of SAT variables = {}\n", num_vars )
              << fmt::format( "[i] number of SAT clauses   = {}\n", num_clauses );
  }
};

namespace detail
{

template<class Solver>
std::vector<int> cardinality_network( Solver& solver, std::vector<int> const& vars, int& next_var )
{
  int lits[3];

  auto logn = static_cast<uint32_t>( ceil( log2( vars.size() ) ) );
  auto current = vars;

  if ( current.size() != static_cast<uint64_t>( 1u ) << logn )
  {
    current.resize( static_cast<uint64_t>( 1u ) << logn, next_var );
    lits[0] = pabc::Abc_Var2Lit( next_var++, 1 );
    solver.add_clause( lits, lits + 1 );
  }

  batcher_sorting_network( static_cast<uint32_t>( current.size() ), [&]( auto a, auto b ) {
    auto va = current[a];
    auto vb = current[b];
    auto va_next = next_var++;
    auto vb_next = next_var++;

    // AND(a, b) a + !c , b + !c , !a + !b + c
    lits[0] = pabc::Abc_Var2Lit( va, 0 );
    lits[1] = pabc::Abc_Var2Lit( va_next, 1 );
    solver.add_clause( lits, lits + 2 );
    lits[0] = pabc::Abc_Var2Lit( vb, 0 );
    lits[1] = pabc::Abc_Var2Lit( va_next, 1 );
    solver.add_clause( lits, lits + 2 );
    lits[0] = pabc::Abc_Var2Lit( va, 1 );
    lits[1] = pabc::Abc_Var2Lit( vb, 1 );
    lits[2] = pabc::Abc_Var2Lit( va_next, 0 );
    solver.add_clause( lits, lits + 3 );

    // OR(a, b) !a + c , !b + c , a + b + !c
    lits[0] = pabc::Abc_Var2Lit( va, 1 );
    lits[1] = pabc::Abc_Var2Lit( vb_next, 0 );
    solver.add_clause( lits, lits + 2 );
    lits[0] = pabc::Abc_Var2Lit( vb, 1 );
    lits[1] = pabc::Abc_Var2Lit( vb_next, 0 );
    solver.add_clause( lits, lits + 2 );
    lits[0] = pabc::Abc_Var2Lit( va, 0 );
    lits[1] = pabc::Abc_Var2Lit( vb, 0 );
    lits[2] = pabc::Abc_Var2Lit( vb_next, 1 );
    solver.add_clause( lits, lits + 3 );

    current[a] = va_next;
    current[b] = vb_next;
  } );

  for ( auto i = 0u; i < current.size() - 1; ++i )
  {
    lits[0] = pabc::Abc_Var2Lit( current[i], 1 );
    lits[1] = pabc::Abc_Var2Lit( current[i + 1], 0 );
    solver.add_clause( lits, lits + 2 );
  }

  return current;
}

template<class Ntk, bool StoreFunction, typename CutData>
class satlut_mapping_impl
{
public:
  using network_cuts_t = network_cuts<Ntk, StoreFunction, CutData>;
  using cut_t = typename network_cuts_t::cut_t;

public:
  satlut_mapping_impl( Ntk& ntk, satlut_mapping_params const& ps, satlut_mapping_stats& st )
      : ntk( ntk ),
        ps( ps ),
        st( st ),
        cuts( cut_enumeration<Ntk, StoreFunction, CutData>( ntk, ps.cut_enumeration_ps ) )
  {
  }

  void run()
  {
    stopwatch t( st.time_total );

    std::vector<int> card_inp;
    node_map<int, Ntk> gate_var( ntk );
    node_map<std::vector<int>, Ntk> cut_vars( ntk );
    auto next_var = 0;

    percy::bsat_wrapper solver;

    /* initialize gate vars */
    ntk.foreach_gate( [&]( auto n ) {
      card_inp.push_back( next_var );
      gate_var[n] = next_var++;
    } );

    const auto card_out = cardinality_network( solver, card_inp, next_var );

    /* create clauses */
    int cut_lits[2];
    ntk.foreach_gate( [&]( auto n ) {
      std::vector<int> gate_is_mapped;
      gate_is_mapped.push_back( pabc::Abc_Var2Lit( gate_var[n], 1 ) );

      for ( auto const& cut : cuts.cuts( ntk.node_to_index( n ) ) )
      {
        if ( cut->size() == 1 )
        {
          break; /* we assume that trivial cuts are in the end of the set */
        }
        gate_is_mapped.push_back( pabc::Abc_Var2Lit( next_var, 0 ) );
        cut_lits[0] = pabc::Abc_Var2Lit( next_var, 1 );
        cut_vars[n].push_back( next_var++ );
        for ( auto leaf : *cut )
        {
          if ( ntk.is_pi( ntk.index_to_node( leaf ) ) )
            continue;
          cut_lits[1] = pabc::Abc_Var2Lit( gate_var[ntk.index_to_node( leaf )], 0 );
          solver.add_clause( cut_lits, cut_lits + 2 );
        }
      }

      solver.add_clause( &gate_is_mapped[0], &gate_is_mapped[0] + gate_is_mapped.size() );
    } );

    /* outputs must be mapped */
    ntk.foreach_po( [&]( auto f ) {
      auto lit = pabc::Abc_Var2Lit( gate_var[f], 0 );
      solver.add_clause( &lit, &lit + 1 );
    } );

    st.num_vars = solver.nr_vars();
    st.num_clauses = solver.nr_clauses();

    auto best_size = ntk.has_mapping() ? ntk.num_cells() + 1 : card_inp.size();

    progress_bar pbar{ "satlut iteration = {0}   try size = {1}", ps.progress };
    auto iteration = 0u;
    while ( true )
    {
      pbar( ++iteration, best_size );
      if ( best_size > card_out.size() )
      {
        std::cout << fmt::format( "[e] best_size = {}   card_inp.size() = {}   card_out.size() = {}   ntk.num_cells = {}   ntk.has_mapping = {}\n",
                                  best_size, card_inp.size(), card_out.size(), ntk.num_cells(), ntk.has_mapping() );
        assert( false );
      }
      auto assump = pabc::Abc_Var2Lit( card_out[card_out.size() - best_size], 1 );

      const auto result = call_with_stopwatch( st.time_sat, [&]() { return solver.solve( &assump, &assump + 1, ps.conflict_limit ); } );
      if ( result == percy::success )
      {
        ntk.clear_mapping();
        ntk.foreach_gate( [&]( auto n ) {
          if ( solver.var_value( gate_var[n] ) )
          {
            for ( auto i = 0u; i < cut_vars[n].size(); ++i )
            {
              if ( solver.var_value( cut_vars[n][i] ) )
              {
                const auto index = ntk.node_to_index( n );
                std::vector<node<Ntk>> nodes;
                for ( auto const& l : cuts.cuts( index )[i] )
                {
                  nodes.push_back( ntk.index_to_node( l ) );
                }
                ntk.add_to_mapping( n, nodes.begin(), nodes.end() );

                if constexpr ( StoreFunction )
                {
                  ntk.set_cell_function( n, cuts.truth_table( cuts.cuts( index )[i] ) );
                }
                break;
              }
            }
          }
        } );

        if ( ntk.num_cells() == ntk.num_pos() )
        {
          /* no further improvement possible */
          break;
        }

        best_size = ntk.num_cells();
      }
      else
      {
        break;
      }
    }
  }

private:
  Ntk& ntk;
  satlut_mapping_params const& ps;
  satlut_mapping_stats& st;
  network_cuts_t cuts;
};

} // namespace detail

/*! \brief SAT-LUT mapping.
 *
 * This algorithm implements the SAT-based area-oriented LUT mapping algorithm
 * presented in [B. Schmitt, A. Mishchenko, and R.K. Brayton, *ASP-DAC* **23**
 * (2018), 586-591].
 *
 * The interface is similar to the one in `lut_mapping`.
 *
 * This algorithm applies SAT-LUT mapping to the whole networking and therefore
 * may show poor performance for larger networks.  There exists a method with
 * the same name that takes as input a window size to apply SAT-LUT mapping to
 * windows.
 *
 * **Required network functions:**
 * - `is_pi`
 * - `index_to_node`
 * - `node_to_index`
 * - `foreach_gate`
 * - `foreach_po`
 * - `num_gates`
 * - `num_cells`
 * - `has_mapping`
 * - `clear_mapping`
 * - `add_to_mapping`
 * - `set_cell_function` if `StoreFunction` is true
 *
 * \param ntk Logic network to be mapped
 * \param ps Parameters
 * \param st Statistics
 */
template<class Ntk, bool StoreFunction = false, typename CutData = cut_enumeration_mf_cut>
void satlut_mapping( Ntk& ntk, satlut_mapping_params const& ps = {}, satlut_mapping_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_cells_v<Ntk>, "Ntk does not implement the num_cells method" );
  static_assert( has_has_mapping_v<Ntk>, "Ntk does not implement the has_mapping method" );
  static_assert( has_clear_mapping_v<Ntk>, "Ntk does not implement the clear_mapping method" );
  static_assert( has_add_to_mapping_v<Ntk>, "Ntk does not implement the add_to_mapping method" );
  static_assert( !StoreFunction || has_set_cell_function_v<Ntk>, "Ntk does not implement the set_cell_function method" );

  satlut_mapping_stats st;
  detail::satlut_mapping_impl<Ntk, StoreFunction, CutData> p( ntk, ps, st );
  p.run();
  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

/*! \brief SAT-LUT mapping (windowed).
 *
 * This algorithm applies SAT-LUT mapping to windows of a given size (e.g., 32,
 * 64, 128) and can therefore better deal with larger networks.  It has
 * otherwise the same interface as `satlut_mapping`.
 *
 * The initial network must already contain a mapping, e.g., found with
 * `lut_mapping`.
 *
 * **Required network functions:**
 * - `is_pi`
 * - `index_to_node`
 * - `node_to_index`
 * - `foreach_gate`
 * - `foreach_po`
 * - `num_gates`
 * - `num_cells`
 * - `has_mapping`
 * - `clear_mapping`
 * - `add_to_mapping`
 * - `is_cell_root`
 * - `set_cell_function` if `StoreFunction` is true
 *
 * \param ntk Logic network to be mapped
 * \param window_size Maximum number of gates in a window
 * \param ps Parameters
 * \param st Statistics
 */
template<class Ntk, bool StoreFunction = false, typename CutData = cut_enumeration_mf_cut>
void satlut_mapping( Ntk& ntk, uint32_t window_size, satlut_mapping_params ps = {}, satlut_mapping_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_cells_v<Ntk>, "Ntk does not implement the num_cells method" );
  static_assert( has_has_mapping_v<Ntk>, "Ntk does not implement the has_mapping method" );
  static_assert( has_clear_mapping_v<Ntk>, "Ntk does not implement the clear_mapping method" );
  static_assert( has_add_to_mapping_v<Ntk>, "Ntk does not implement the add_to_mapping method" );
  static_assert( has_is_cell_root_v<Ntk>, "Ntk does not implement the is_cell_root method" );
  static_assert( !StoreFunction || has_set_cell_function_v<Ntk>, "Ntk does not implement the set_cell_function method" );

  if ( !ntk.has_mapping() )
  {
    return;
  }

  satlut_mapping_stats st;
  stopwatch<>::duration time_total{};
  cell_window window( ntk, window_size );
  progress_bar pbar{ ntk.size(), "satlut (windowed) |{0}| node = {1:>4} / " + std::to_string( ntk.size() ), ps.progress };
  ps.progress = false; /* do not show inner progress */
  ntk.foreach_gate( [&]( auto n, int index ) {
    stopwatch<> t( time_total );
    pbar( index, ntk.node_to_index( n ) );
    if ( ntk.is_cell_root( n ) )
    {
      if ( !window.compute_window_for( n ) ) /* window has been visited before */
      {
        return true;
      }

      if ( ps.verbose )
      {
        std::cout << fmt::format( "[i] cell {:>5}   size = {:>4}   nodes = {:>2}   gates = {:>3}   pis = {:>3}   pos = {:>3}\n",
                                  n,
                                  window.size(),
                                  window.num_cells(),
                                  window.num_gates(),
                                  window.num_pis(),
                                  window.num_pos() );
      }
      if ( window.num_cells() == window.num_pos() || window.num_pos() == 0 )
      {
        return true;
      }
      topo_view window_topo{ window };
      detail::satlut_mapping_impl<decltype( window_topo ), StoreFunction, CutData> p( window_topo, ps, st );
      p.run();
      return true;
    }

    return true;
  } );

  st.time_total = time_total;

  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} // namespace mockturtle