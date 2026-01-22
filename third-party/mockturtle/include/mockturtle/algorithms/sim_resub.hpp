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
  \brief Simulation-Guided Resubstitution

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../io/write_patterns.hpp"
#include "../networks/aig.hpp"
#include "../networks/xag.hpp"
#include "../networks/mig.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "circuit_validator.hpp"
#include "pattern_generation.hpp"
#include "resubstitution.hpp"
#include "resyn_engines/xag_resyn.hpp"
#include "resyn_engines/mig_resyn.hpp"
#include "simulation.hpp"

#include <bill/bill.hpp>
#include <fmt/format.h>
#include <kitty/kitty.hpp>

#include <algorithm>
#include <variant>

namespace mockturtle
{

namespace detail
{

template<typename ResynSt>
struct sim_resub_stats
{
  /*! \brief Time for pattern generation. */
  stopwatch<>::duration time_patgen{ 0 };

  /*! \brief Time for saving patterns. */
  stopwatch<>::duration time_patsave{ 0 };

  /*! \brief Time for simulation. */
  stopwatch<>::duration time_sim{ 0 };

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_sat{ 0 };
  stopwatch<>::duration time_sat_restart{ 0 };

  /*! \brief Time for computing ODCs. */
  stopwatch<>::duration time_odc{ 0 };

  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_resyn{ 0 };

  /*! \brief Time for translating from index lists to network signals. */
  stopwatch<>::duration time_interface{ 0 };

  /*! \brief Number of patterns used. */
  uint32_t num_pats{ 0 };

  /*! \brief Number of counter-examples. */
  uint32_t num_cex{ 0 };

  /*! \brief Number of successful resubstitutions. */
  uint32_t num_resub{ 0 };

  /*! \brief Number of SAT solver timeout. */
  uint32_t num_timeout{ 0 };

  /*! \brief Number of calls to the resynthesis engine. */
  uint32_t num_resyn{ 0 };

  ResynSt resyn_st;

  void report() const
  {
    fmt::print( "[i] <ResubEngine: simulation_based_resub_engine>\n" );
    fmt::print( "[i]     ========  Stats  ========\n" );
    fmt::print( "[i]     #pat        = {:6d}\n", num_pats );
    fmt::print( "[i]     #resyn call = {:6d}\n", num_resyn );
    fmt::print( "[i]     #valid      = {:6d}\n", num_resub );
    fmt::print( "[i]     #CEX        = {:6d}\n", num_cex );
    fmt::print( "[i]     #timeout    = {:6d}\n", num_timeout );
    fmt::print( "[i]     ======== Runtime ========\n" );
    fmt::print( "[i]     generate pattern: {:>5.2f} secs [excluded]\n", to_seconds( time_patgen ) );
    fmt::print( "[i]     save pattern    : {:>5.2f} secs [excluded]\n", to_seconds( time_patsave ) );
    fmt::print( "[i]     simulation      : {:>5.2f} secs\n", to_seconds( time_sim ) );
    fmt::print( "[i]     SAT solve       : {:>5.2f} secs\n", to_seconds( time_sat ) );
    fmt::print( "[i]     SAT restart     : {:>5.2f} secs\n", to_seconds( time_sat_restart ) );
    fmt::print( "[i]     compute ODCs    : {:>5.2f} secs\n", to_seconds( time_odc ) );
    fmt::print( "[i]     interfacing     : {:>5.2f} secs\n", to_seconds( time_interface ) );
    fmt::print( "[i]     compute function: {:>5.2f} secs\n", to_seconds( time_resyn ) );
    fmt::print( "[i]     ======== Details ========\n" );
    resyn_st.report();
    fmt::print( "[i]     =========================\n\n" );
  }
};

/*! \brief Simulation-based resubstitution engine.
 *
 * This engine simulates the entire network using partial truth tables and calls a
 * resynthesis engine (template parameter `ResynEngine`) to find potential resubstitutions.
 * If a resubstitution candidate is found, it then formally verifies it with SAT solving.
 * If the validation fails, a counter-example will be added to the simulation patterns,
 * and resynthesis will be invoked again with updated truth tables, looping until it returns
 * `std::nullopt`. This engine only requires the divisor collector to prepare `divs`.
 *
 * Please refer to the following paper for further details.
 *
 * [1] A Simulation-Guided Paradigm for Logic Synthesis and Verification. TCAD, 2022.
 *
 * Required interface of `ResynEngine`:
 * - A public `operator()`: `std::optional<index_list_t> operator()`
 * `( TT const& target, TT const& care, iterator_type begin, iterator_type end,
 * truth_table_storage_type const& tts, uint32_t max_size )`
 *
 * All classes implemented in `algorithms/resyn_engines/` are compatible.
 *
 * \tparam validator_t Specialization of `circuit_validator`.
 * \tparam ResynEngine A resynthesis solver to compute the resubstitution candidate.
 * \tparam MffcRes Typename of `potential_gain`.
 */
template<class Ntk, typename validator_t = circuit_validator<Ntk, bill::solvers::bsat2, false, true, false>, class ResynEngine = xag_resyn_decompose<kitty::partial_truth_table, xag_resyn_static_params_for_sim_resub<Ntk>>, typename MffcRes = uint32_t>
class simulation_based_resub_engine
{
public:
  static constexpr bool require_leaves_and_mffc = false;
  using stats = sim_resub_stats<typename ResynEngine::stats>;
  using mffc_result_t = MffcRes;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using TT = kitty::partial_truth_table;

  explicit simulation_based_resub_engine( Ntk& ntk, resubstitution_params const& ps, stats& st )
      : ntk( ntk ), ps( ps ), st( st ), tts( ntk ), validator( ntk, { ps.max_clauses, ps.odc_levels, ps.conflict_limit, ps.random_seed } ), engine( st.resyn_st )
  {
    if constexpr ( !validator_t::use_odc_ )
    {
      assert( ps.odc_levels == 0 && "to consider ODCs, circuit_validator::use_odc (the last template parameter) has to be turned on" );
    }

    add_event = ntk.events().register_add_event( [&]( const auto& n ) {
      tts.resize();
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_node<Ntk>( ntk, n, tts, sim );
      } );
    } );
  }

  ~simulation_based_resub_engine()
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

      if constexpr ( has_EXCDC_interface_v<Ntk> )
      {
        sim.remove_CDC_patterns( ntk );
      }
    } );
    st.num_pats = sim.num_bits();
    assert( sim.num_bits() > 0 );

    /* first simulation: the whole circuit; from 0 bits. */
    call_with_stopwatch( st.time_sim, [&]() {
      simulate_nodes<Ntk>( ntk, tts, sim, true );
    } );
  }

  void update()
  {
    if constexpr ( validator_t::use_odc_ || has_EXODC_interface_v<Ntk> )
    {
      call_with_stopwatch( st.time_sat_restart, [&]() {
        validator.update();
      } );
      tts.reset();
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_nodes<Ntk>( ntk, tts, sim, true );
      } );
    }
  }

  std::optional<signal> run( node const& n, std::vector<node> const& divs, mffc_result_t potential_gain, uint32_t& last_gain )
  {
    for ( auto j = 0u; j < ps.max_trials; ++j )
    {
      check_tts( n );
      for ( auto const& d : divs )
      {
        check_tts( d );
      }

      TT const care = call_with_stopwatch( st.time_odc, [&]() {
        return ( ps.odc_levels == 0 ) ? sim.compute_constant( true ) : ~observability_dont_cares( ntk, n, sim, tts, ps.odc_levels );
      } );

      const auto res = call_with_stopwatch( st.time_resyn, [&]() {
        ++st.num_resyn;
        return engine( tts[n], care, std::begin( divs ), std::end( divs ), tts, std::min( potential_gain - 1, ps.max_inserts ) );
      } );

      if ( res )
      {
        auto const& id_list = *res;
        assert( id_list.num_pos() == 1u );
        last_gain = potential_gain - id_list.num_gates();
        auto valid = call_with_stopwatch( st.time_sat, [&]() {
          return validator.validate( n, divs, id_list );
        } );
        if ( valid )
        {
          if ( *valid )
          {
            ++st.num_resub;
            signal out_sig;
            call_with_stopwatch( st.time_interface, [&]() {
              std::vector<signal> divs_sig( divs.size() );
              std::transform( divs.begin(), divs.end(), divs_sig.begin(), [&]( const node n ) {
                return ntk.make_signal( n );
              } );
              insert( ntk, divs_sig.begin(), divs_sig.end(), id_list, [&]( signal const& s ) {
                out_sig = s;
              } );
            } );
            return out_sig;
          }
          else
          {
            found_cex();
            continue;
          }
        }
        else /* timeout */
        {
          return std::nullopt;
        }
      }
      else /* functor can not find any potential resubstitution */
      {
        return std::nullopt;
      }
    }
    return std::nullopt;
  }

  void found_cex()
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
  }

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
  Ntk& ntk;
  resubstitution_params const& ps;
  stats& st;

  incomplete_node_map<TT, Ntk> tts;
  partial_simulator sim;

  validator_t validator;
  ResynEngine engine;

  /* events */
  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;
}; /* simulation_based_resub_engine */

template<class Ntk, typename resub_impl_t>
void sim_resubstitution_run( Ntk& ntk, resubstitution_params const& ps, resubstitution_stats* pst )
{
  resubstitution_stats st;
  typename resub_impl_t::engine_st_t engine_st;
  typename resub_impl_t::collector_st_t collector_st;

  resub_impl_t p( ntk, ps, st, engine_st, collector_st );
  p.run();
  st.time_resub -= engine_st.time_patgen;
  st.time_total -= engine_st.time_patgen + engine_st.time_patsave;

  if ( ps.verbose )
  {
    st.report();
    collector_st.report();
    engine_st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace detail */

template<class Ntk>
void sim_resubstitution( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network> 
                 || std::is_same_v<typename Ntk::base_type, xag_network>
                 || std::is_same_v<typename Ntk::base_type, mig_network>, "Currently only supports AIG, XAG, and MIG" );

  using resub_view_t = fanout_view<depth_view<Ntk>>;
  depth_view<Ntk> depth_view{ ntk };
  resub_view_t resub_view{ depth_view };

  if constexpr ( std::is_same_v<typename Ntk::base_type, aig_network> )
  {
    using resyn_engine_t = xag_resyn_decompose<kitty::partial_truth_table, aig_resyn_static_params_for_sim_resub<resub_view_t>>;

    if ( ps.odc_levels != 0 )
    {
      using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, true>;
      using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t, resyn_engine_t>>;
      detail::sim_resubstitution_run<resub_view_t, resub_impl_t>( resub_view, ps, pst );
    }
    else
    {
      using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, false>;
      using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t, resyn_engine_t>>;
      detail::sim_resubstitution_run<resub_view_t, resub_impl_t>( resub_view, ps, pst );
    }
  }
  else if constexpr ( std::is_same_v<typename Ntk::base_type, xag_network> )
  {
    using resyn_engine_t = xag_resyn_decompose<kitty::partial_truth_table, xag_resyn_static_params_for_sim_resub<resub_view_t>>;

    if ( ps.odc_levels != 0 )
    {
      using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, true>;
      using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t, resyn_engine_t>>;
      detail::sim_resubstitution_run<resub_view_t, resub_impl_t>( resub_view, ps, pst );
    }
    else
    {
      using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, false>;
      using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t, resyn_engine_t>>;
      detail::sim_resubstitution_run<resub_view_t, resub_impl_t>( resub_view, ps, pst );
    }
  }
  else if constexpr ( std::is_same_v<typename Ntk::base_type, mig_network> )
  {
    using resyn_engine_t = mig_resyn_topdown<kitty::partial_truth_table, mig_resyn_static_params>;

    if ( ps.odc_levels != 0 )
    {
      using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, true>;
      using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t, resyn_engine_t>>;
      detail::sim_resubstitution_run<resub_view_t, resub_impl_t>( resub_view, ps, pst );
    }
    else
    {
      using validator_t = circuit_validator<resub_view_t, bill::solvers::bsat2, false, true, false>;
      using resub_impl_t = typename detail::resubstitution_impl<resub_view_t, typename detail::simulation_based_resub_engine<resub_view_t, validator_t, resyn_engine_t>>;
      detail::sim_resubstitution_run<resub_view_t, resub_impl_t>( resub_view, ps, pst );
    }
  }
}

} /* namespace mockturtle */
