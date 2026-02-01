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
  \file sim_resub.hpp
  \brief Simulation-guided resubstitution

  \author Hanyu Wang
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../io/write_patterns.hpp"
#include "../../networks/aig.hpp"
#include "../../networks/xag.hpp"
#include "../../traits.hpp"
#include "../../utils/index_list/index_list.hpp"
#include "../../views/depth_view.hpp"
#include "../../views/fanout_view.hpp"
#include "../circuit_validator.hpp"
#include "../detail/resub_utils.hpp"
#include "../dont_cares.hpp"
#include "../pattern_generation.hpp"
#include "../resyn_engines/aig_enumerative.hpp"
#include "../resyn_engines/mig_enumerative.hpp"
#include "../resyn_engines/mig_resyn.hpp"
#include "../resyn_engines/xag_resyn.hpp"
#include "../simulation.hpp"
#include <kitty/kitty.hpp>

#include <functional>
#include <optional>
#include <vector>

namespace mockturtle::experimental
{

struct breadth_first_windowing_params
{
  /*! \brief Maximum number of divisors to consider. */
  uint32_t max_divisors{ 50 };

  /*! \brief Maximum number of TFI nodes to collect. */
  uint32_t max_tfi{ uint32_t( max_divisors * 0.5 ) };

  /*! \brief Maximum number of nodes added by resubstitution. */
  uint32_t max_inserts{ std::numeric_limits<uint32_t>::max() };

  /*! \brief Maximum fanout of a node to be considered as root. */
  uint32_t skip_fanout_limit_for_roots{ 1000 };

  /*! \brief Maximum fanout of a node to be considered as divisor. */
  uint32_t skip_fanout_limit_for_divisors{ 100 };
};

struct simulation_guided_resynthesis_params
{
  /*! \brief Whether to use pre-generated patterns stored in a file.
   * If not, by default, 1024 random pattern + 1x stuck-at patterns will be generated.
   */
  std::optional<std::string> pattern_filename{};

  /*! \brief Whether to save the appended patterns (with CEXs) into file. */
  std::optional<std::string> save_patterns{};

  /*! \brief Maximum number of clauses of the SAT solver. */
  uint32_t max_clauses{ 1000 };

  /*! \brief Conflict limit for the SAT solver. */
  uint32_t conflict_limit{ 1000 };

  /*! \brief Random seed for the SAT solver (influences the randomness of counter-examples). */
  uint32_t random_seed{ 1 };

  /*! \brief Maximum number of trials to call the resub functor. */
  uint32_t max_trials{ 100 };

  /*! \brief Whether to utilize ODC, and how many levels. 0 = no. -1 = Consider TFO until PO. */
  int32_t odc_levels{ 0 };
};

struct breadth_first_windowing_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Accumulated runtime for mffc computation. */
  stopwatch<>::duration time_mffc{ 0 };

  /*! \brief Accumulated runtime for divisor collection. */
  stopwatch<>::duration time_divs{ 0 };

  /*! \brief Total number of divisors. */
  uint64_t num_divisors{ 0u };

  /*! \brief Number of constructed windows. */
  uint32_t num_windows{ 0u };

  /*! \brief Total number of MFFC nodes. */
  uint64_t sum_mffc_size{ 0u };

  void report() const
  {
    // clang-format off
    fmt::print( "[i] breadth_first_windowing report\n" );
    fmt::print( "    tot. #divs = {:5d}, sum  |MFFC| = {:5d}\n", num_divisors, sum_mffc_size );
    fmt::print( "    avg. #divs = {:>5.2f}, avg. |MFFC| = {:>5.2f}\n", float( num_divisors ) / float( num_windows ), float( sum_mffc_size ) / float( num_windows ) );
    fmt::print( "    ===== Runtime Breakdown =====\n" );
    fmt::print( "    Total : {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "      MFFC: {:>5.2f} secs\n", to_seconds( time_mffc ) );
    fmt::print( "      Divs: {:>5.2f} secs\n", to_seconds( time_divs ) );
    // clang-format on
  }
};

struct simulation_guided_resynthesis_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Time for pattern generation. */
  stopwatch<>::duration time_patgen{ 0 };

  /*! \brief Time for simulation. */
  stopwatch<>::duration time_sim{ 0 };

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_sat{ 0 };

  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_resyn{ 0 };

  /*! \brief Time for computing ODCs. */
  stopwatch<>::duration time_odc{ 0 };

  /*! \brief Time for saving patterns. */
  stopwatch<>::duration time_patsave{ 0 };

  /*! \brief Number of calls to the resynthesis engine. */
  uint32_t num_calls{ 0 };

  /*! \brief Number of solutions found by the resynthesis engine. */
  uint32_t num_sols{ 0 };

  /*! \brief Number of patterns used. */
  uint32_t num_pats{ 0 };

  /*! \brief Number of valid solutions. */
  uint32_t num_valid{ 0 };

  /*! \brief Number of counter-examples. */
  uint32_t num_cex{ 0 };

  /*! \brief Number of SAT solver timeout. */
  uint32_t num_timeout{ 0 };

  void report() const
  {
    // clang-format off
    fmt::print( "[i] simulation_guided_resynthesis report\n" );
    fmt::print( "    initial #pats = {} + #CEXs = {} --> {}\n", num_pats, num_cex, num_pats + num_cex );
    fmt::print( "    #resyn calls = {}, #solutions = {} ({:.2f}%)\n", num_calls, num_sols, float( num_sols ) / float( num_calls ) * 100 );
    fmt::print( "    #valid = {} ({:.2f}%), #CEXs = {} ({:.2f}%), #TOs = {} ({:.2f}%)\n", num_valid, float( num_valid ) / float( num_sols ) * 100, num_cex, float( num_cex ) / float( num_sols ) * 100, num_timeout, float( num_timeout ) / float( num_sols ) * 100 );
    fmt::print( "    ===== Runtime Breakdown =====\n" );
    fmt::print( "    Total         : {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "      Pattern gen.: {:>5.2f} secs [Called in init() -- should be subtracted]\n", to_seconds( time_patgen ) );
    fmt::print( "      Simulation  : {:>5.2f} secs\n", to_seconds( time_sim ) );
    fmt::print( "      SAT         : {:>5.2f} secs\n", to_seconds( time_sat ) );
    fmt::print( "      Resynthesis : {:>5.2f} secs\n", to_seconds( time_resyn ) );
    fmt::print( "      ODC comp.   : {:>5.2f} secs\n", to_seconds( time_odc ) );
    fmt::print( "    Save patterns : {:>5.2f} secs [Called in destructor -- not included in total runtime]\n", to_seconds( time_patsave ) );
    // clang-format on
  }
};

namespace detail
{

template<class Ntk>
struct breadth_first_window
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  node root;
  std::vector<node> divs;
  uint32_t mffc_size;
  uint32_t max_size{ std::numeric_limits<uint32_t>::max() };
  // uint32_t max_level{std::numeric_limits<uint32_t>::max()};
};

template<class Ntk>
class breadth_first_windowing
{
public:
  using problem_t = breadth_first_window<Ntk>;
  using params_t = breadth_first_windowing_params;
  using stats_t = breadth_first_windowing_stats;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit breadth_first_windowing( Ntk& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk ), ps( ps ), st( st ), mffc_mgr( ntk ),
        divs_mgr( ntk, divisor_collector_params( { ps.max_tfi, ps.max_divisors, ps.skip_fanout_limit_for_divisors } ) )
  {
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  }

  void init()
  {
  }

  std::optional<std::reference_wrapper<problem_t>> operator()( node const& n )
  {
    stopwatch t( st.time_total );
    if ( ntk.fanout_size( n ) > ps.skip_fanout_limit_for_roots )
    {
      return std::nullopt; /* skip nodes with too many fanouts */
    }

    /* collect TFI nodes with BFS and supported "wing" nodes */
    win.root = n;
    win.divs.clear();
    call_with_stopwatch( st.time_divs, [&]() {
      divs_mgr.collect_tfi_and_wings( n, win.divs );
    } );

    /* compute and mark MFFC nodes */
    ++mffc_marker;
    win.mffc_size = call_with_stopwatch( st.time_mffc, [&]() {
      return mffc_mgr.call_on_mffc_and_count( n, {}, [&]( node const& n ) {
        ntk.set_value( n, mffc_marker );
      } );
    } );

    /* exclude MFFC node in divs */
    uint32_t counter{ 0 };
    for ( int32_t i = 0; i < win.divs.size(); ++i )
    {
      if ( ntk.value( win.divs.at( i ) ) == mffc_marker )
      {
        ++counter;
        win.divs[i] = win.divs.back();
        win.divs.pop_back();
        --i;
      }
    }

    /* all MFFC nodes should be in TFI (thus collected in divs) */
    assert( counter == win.mffc_size );
    win.max_size = std::min( win.mffc_size - 1, ps.max_inserts );

    st.num_windows++;
    st.num_divisors += win.divs.size();
    st.sum_mffc_size += win.mffc_size;

    return win;
  }

  template<typename res_t>
  uint32_t gain( problem_t const& prob, res_t const& res ) const
  {
    static_assert( is_index_list_v<res_t>, "res_t is not an index_list (windowing engine and resynthesis engine do not match)" );
    return prob.mffc_size - res.num_gates();
  }

  template<typename res_t>
  bool update_ntk( problem_t const& prob, res_t const& res )
  {
    static_assert( is_index_list_v<res_t>, "res_t is not an index_list (windowing engine and resynthesis engine do not match)" );
    assert( res.num_pos() == 1 );
    insert<false>( ntk, prob.divs.begin(), prob.divs.end(), res, [&]( signal const& g ) {
      ntk.substitute_node( prob.root, g );
    } );
    return true; /* continue optimization */
  }

  template<typename res_t>
  bool report( problem_t const& prob, res_t const& res )
  {
    static_assert( is_index_list_v<res_t>, "res_t is not an index_list (windowing engine and resynthesis engine do not match)" );
    assert( res.num_pos() == 1 );
    fmt::print( "[i] found solution {} for root node {}\n", to_index_list_string( res ), prob.root );
    return true;
  }

private:
private:
  Ntk& ntk;
  problem_t win;
  params_t const& ps;
  stats_t& st;
  typename mockturtle::detail::node_mffc_inside<Ntk> mffc_mgr; // TODO: namespaces can be removed when we move out of experimental::
  uint32_t mffc_marker{ 0u };
  divisor_collector<Ntk> divs_mgr;
}; /* breadth_first_windowing */

template<class Ntk, class ResynEngine, bool UseODC = false, bill::solvers Solver = bill::solvers::bsat2>
class simulation_guided_resynthesis
{
public:
  using problem_t = breadth_first_window<Ntk>;
  using res_t = typename ResynEngine::index_list_t;
  using params_t = simulation_guided_resynthesis_params;
  using stats_t = simulation_guided_resynthesis_stats;

  using node = typename Ntk::node;
  using TT = kitty::partial_truth_table;
  using validator_t = circuit_validator<Ntk, Solver, /*use_pushpop*/ false, /*randomize*/ true, /*use_odc*/ UseODC>;

  explicit simulation_guided_resynthesis( Ntk const& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk ), ps( ps ), st( st ), engine( rst ),
        validator( ntk, { ps.max_clauses, ps.odc_levels, ps.conflict_limit, ps.random_seed } ), tts( ntk )
  {}

  ~simulation_guided_resynthesis()
  {
    if ( ps.save_patterns )
    {
      call_with_stopwatch( st.time_patsave, [&]() {
        write_patterns( sim, *ps.save_patterns );
      } );
    }

    if ( add_event )
    {
      ntk.events().release_add_event( add_event );
    }
  }

  void init()
  {
    add_event = ntk.events().register_add_event( [&]( const auto& n ) {
      tts.resize();
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_node<Ntk>( ntk, n, tts, sim );
      } );
    } );

    /* prepare simulation patterns */
    call_with_stopwatch( st.time_patgen, [&]() {
      if ( ps.pattern_filename )
      {
        sim = partial_simulator( *ps.pattern_filename );
      }
      else
      {
        sim = partial_simulator( ntk.num_pis(), 1024 );
        pattern_generation( ntk, sim );
      }
    } );
    st.num_pats = sim.num_bits();

    /* first simulation: the whole circuit; from 0 bits. */
    call_with_stopwatch( st.time_sim, [&]() {
      simulate_nodes<Ntk>( ntk, tts, sim, true );
    } );
  }

  std::optional<res_t> operator()( problem_t& prob )
  {
    for ( auto j = 0u; j < ps.max_trials; ++j )
    {
      check_tts( prob.root );
      for ( auto const& d : prob.divs )
      {
        check_tts( d );
      }

      TT const care = call_with_stopwatch( st.time_odc, [&]() {
        return ( ps.odc_levels == 0 ) ? sim.compute_constant( true ) : ~observability_dont_cares( ntk, prob.root, sim, tts, ps.odc_levels );
      } );

      const auto res = call_with_stopwatch( st.time_resyn, [&]() {
        ++st.num_calls;
        return engine( tts[prob.root], care, std::begin( prob.divs ), std::end( prob.divs ), tts, prob.max_size );
      } );

      if ( res )
      {
        ++st.num_sols;
        auto const& id_list = *res;
        auto valid = call_with_stopwatch( st.time_sat, [&]() {
          return validator.validate( prob.root, prob.divs, id_list );
        } );
        if ( valid )
        {
          if ( *valid )
          {
            ++st.num_valid;
            if constexpr ( UseODC )
            {
              /* restart the solver -- clear constructed CNF */
              call_with_stopwatch( st.time_sat, [&]() {
                validator.update();
              } );
            }
            return id_list;
          }
          else
          {
            ++st.num_cex;
            call_with_stopwatch( st.time_sim, [&]() {
              sim.add_pattern( validator.cex );
            } );

            /* re-simulate the whole circuit (for the last block) when a block is full */
            if ( sim.num_bits() % 64 == 0 )
            {
              call_with_stopwatch( st.time_sim, [&]() {
                simulate_nodes<Ntk>( ntk, tts, sim, false );
              } );
            }
            continue;
          }
        }
        else /* timeout */
        {
          ++st.num_timeout;
          return std::nullopt;
        }
      }
      else /* functor can not find any potential resubstitution */
      {
        return std::nullopt;
      }
    } /* limit on number of trials exceeded */
    return std::nullopt;
  }

private:
  void check_tts( node const& n )
  {
    if ( tts[n].num_bits() != sim.num_bits() )
    {
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_node<Ntk>( ntk, n, tts, sim );
      } );
    }
  }

private:
  Ntk const& ntk;
  params_t const& ps;
  stats_t& st;
  typename ResynEngine::stats rst;
  ResynEngine engine;
  partial_simulator sim;
  validator_t validator;
  incomplete_node_map<TT, Ntk> tts;

  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;
}; /* simulation_guided_resynthesis */

} /* namespace detail */

using sim_resub_params = boolean_optimization_params<breadth_first_windowing_params, simulation_guided_resynthesis_params>;
using sim_resub_stats = boolean_optimization_stats<breadth_first_windowing_stats, simulation_guided_resynthesis_stats>;

template<class Ntk>
void simulation_xag_heuristic_resub( Ntk& ntk, sim_resub_params const& ps = {}, sim_resub_stats* pst = nullptr )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Ntk::base_type is not xag_network" );

  using ViewedNtk = depth_view<fanout_view<Ntk>>;
  fanout_view<Ntk> fntk( ntk );
  ViewedNtk viewed( fntk );

  using windowing_t = typename detail::breadth_first_windowing<ViewedNtk>;
  using engine_t = xag_resyn_decompose<kitty::partial_truth_table, xag_resyn_static_params_for_sim_resub<ViewedNtk>>;
  using resyn_t = typename detail::simulation_guided_resynthesis<ViewedNtk, engine_t>;
  using opt_t = typename detail::boolean_optimization_impl<ViewedNtk, windowing_t, resyn_t>;

  sim_resub_stats st;
  opt_t p( viewed, ps, st );
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

template<class Ntk>
void simulation_aig_heuristic_resub( Ntk& ntk, sim_resub_params const& ps = {}, sim_resub_stats* pst = nullptr )
{
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network>, "Ntk::base_type is not aig_network" );

  using ViewedNtk = depth_view<fanout_view<Ntk>>;
  fanout_view<Ntk> fntk( ntk );
  ViewedNtk viewed( fntk );

  using windowing_t = typename detail::breadth_first_windowing<ViewedNtk>;
  using engine_t = xag_resyn_decompose<kitty::partial_truth_table, aig_resyn_static_params_for_sim_resub<ViewedNtk>>;
  using resyn_t = typename detail::simulation_guided_resynthesis<ViewedNtk, engine_t>;
  using opt_t = typename detail::boolean_optimization_impl<ViewedNtk, windowing_t, resyn_t>;

  sim_resub_stats st;
  opt_t p( viewed, ps, st );
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

} /* namespace mockturtle::experimental */