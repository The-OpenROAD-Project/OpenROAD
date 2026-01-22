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
  \file resubstitution.hpp
  \brief Generic resubstitution framework

  \author Eleonora Testa
  \author Heinz Riener
  \author Mathias Soeken
  \author Shubham Rai
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/depth_view.hpp"
#include "../views/fanout_view.hpp"

#include "detail/resub_utils.hpp"
#include "dont_cares.hpp"
#include "reconv_cut.hpp"

#include <vector>

namespace mockturtle
{

/*! \brief Parameters for resubstitution.
 *
 * The data structure `resubstitution_params` holds configurable parameters with
 * default arguments for `resubstitution`.
 */
struct resubstitution_params
{
  /*! \brief Maximum number of PIs of reconvergence-driven cuts. */
  uint32_t max_pis{ 8 };

  /*! \brief Maximum number of divisors to consider. */
  uint32_t max_divisors{ 150 };

  /*! \brief Maximum number of nodes added by resubstitution. */
  uint32_t max_inserts{ 2 };

  /*! \brief Maximum fanout of a node to be considered as root. */
  uint32_t skip_fanout_limit_for_roots{ 1000 };

  /*! \brief Maximum fanout of a node to be considered as divisor. */
  uint32_t skip_fanout_limit_for_divisors{ 100 };

  /*! \brief Show progress. */
  bool progress{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };

  /****** window-based resub engine ******/

  /*! \brief Use don't cares for optimization. Only used by window-based resub engine. */
  bool use_dont_cares{ false };

  /*! \brief Window size for don't cares calculation. Only used by window-based resub engine. */
  uint32_t window_size{ 12u };

  /*! \brief Whether to prevent from increasing depth. Currently only used by window-based resub engine. */
  bool preserve_depth{ false };

  /****** simulation-based resub engine ******/

  /*! \brief Whether to use pre-generated patterns stored in a file.
   * If not, by default, 1024 random pattern + 1x stuck-at patterns will be generated. Only used by simulation-based resub engine.
   */
  std::optional<std::string> pattern_filename{};

  /*! \brief Whether to save the appended patterns (with CEXs) into file. Only used by simulation-based resub engine. */
  std::optional<std::string> save_patterns{};

  /*! \brief Maximum number of clauses of the SAT solver. Only used by simulation-based resub engine. */
  uint32_t max_clauses{ 1000 };

  /*! \brief Conflict limit for the SAT solver. Only used by simulation-based resub engine. */
  uint32_t conflict_limit{ 1000 };

  /*! \brief Random seed for the SAT solver (influences the randomness of counter-examples). Only used by simulation-based resub engine. */
  uint32_t random_seed{ 1 };

  /*! \brief Whether to utilize ODC, and how many levels. 0 = no. -1 = Consider TFO until PO. Only used by simulation-based resub engine. */
  int32_t odc_levels{ 0 };

  /*! \brief Maximum number of trials to call the resub functor. Only used by simulation-based resub engine. */
  uint32_t max_trials{ 100 };

  /* k-resub engine specific */
  /*! \brief Maximum number of divisors to consider in k-resub engine. Only used by `abc_resub_functor` with simulation-based resub engine. */
  uint32_t max_divisors_k{ 50 };
};

/*! \brief Statistics for resubstitution.
 *
 * The data structure `resubstitution_stats` provides data collected by running
 * `resubstitution`.
 */
struct resubstitution_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Accumulated runtime of the divisor collector. */
  stopwatch<>::duration time_divs{ 0 };

  /*! \brief Accumulated runtime of the resub engine. */
  stopwatch<>::duration time_resub{ 0 };

  /*! \brief Accumulated runtime of the callback function. */
  stopwatch<>::duration time_callback{ 0 };

  /*! \brief Total number of divisors. */
  uint64_t num_total_divisors{ 0 };

  /*! \brief Total number of gain. */
  uint64_t estimated_gain{ 0 };

  /*! \brief Initial network size (before resubstitution). */
  uint64_t initial_size{ 0 };

  void report() const
  {
    // clang-format off
    fmt::print( "[i] <Top level>\n" );
    fmt::print( "[i]     ========  Stats  ========\n" );
    fmt::print( "[i]     #divisors = {:8d}\n", num_total_divisors );
    fmt::print( "[i]     est. gain = {:8d} ({:>5.2f}%)\n", estimated_gain, ( 100.0 * estimated_gain ) / initial_size );
    fmt::print( "[i]     ======== Runtime ========\n" );
    fmt::print( "[i]     total         : {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i]       DivCollector: {:>5.2f} secs\n", to_seconds( time_divs ) );
    fmt::print( "[i]       ResubEngine : {:>5.2f} secs\n", to_seconds( time_resub ) );
    fmt::print( "[i]       callback    : {:>5.2f} secs\n", to_seconds( time_callback ) );
    fmt::print( "[i]     =========================\n\n" );
    // clang-format on
  }
};

namespace detail
{

template<typename Ntk>
bool substitute_fn( Ntk& ntk, typename Ntk::node const& n, typename Ntk::signal const& g )
{
  ntk.substitute_node( n, g );
  return true;
}

template<typename Ntk>
bool report_fn( Ntk& ntk, typename Ntk::node const& n, typename Ntk::signal const& g )
{
  fmt::print( "[i] Substitute node {} with signal {}{}\n", n, ntk.is_complemented( g ) ? "!" : "", ntk.get_node( g ) );
  return false;
}

struct default_collector_stats
{
  /*! \brief Total number of leaves. */
  uint64_t num_total_leaves{ 0 };

  /*! \brief Accumulated runtime for cut computation. */
  stopwatch<>::duration time_cuts{ 0 };

  /*! \brief Accumulated runtime for mffc computation. */
  stopwatch<>::duration time_mffc{ 0 };

  /*! \brief Accumulated runtime for divisor computation. */
  stopwatch<>::duration time_divs{ 0 };

  void report() const
  {
    // clang-format off
    fmt::print( "[i] <DivCollector: default_divisor_collector>\n" );
    fmt::print( "[i]     #leaves = {:6d}\n", num_total_leaves );
    fmt::print( "[i]     ======== Runtime ========\n" );
    fmt::print( "[i]     reconv. cut : {:>5.2f} secs\n", to_seconds( time_cuts ) );
    fmt::print( "[i]     MFFC        : {:>5.2f} secs\n", to_seconds( time_mffc ) );
    fmt::print( "[i]     divs collect: {:>5.2f} secs\n", to_seconds( time_divs ) );
    fmt::print( "[i]     =========================\n\n" );
    // clang-format on
  }
};

/*! \brief Prepare the three public data members `leaves`, `divs` and `mffc`
 * to be ready for usage.
 *
 * `leaves`: sufficient support for all divisors
 * `divs`: divisor nodes that can be used for resubstitution
 * `mffc`: MFFC nodes which are needed to do simulation from
 * `leaves`, through `divs` and `mffc` until the root node,
 * but should be excluded from resubstitution.
 * The last element of `mffc` is always the root node.
 *
 * `divs` and `mffc` are in topological order.
 *
 * \param MffcMgr Manager class to compute the potential gain if a
 * resubstitution exists (number of MFFC nodes when the cost function is circuit size).
 * \param MffcRes Typename of the return value of `MffcMgr`.
 * \param cut_comp Manager class to compute reconvergence-driven cuts.
 */
template<class Ntk, class MffcMgr = node_mffc_inside<Ntk>, typename MffcRes = uint32_t, typename cut_comp = detail::reconvergence_driven_cut_impl<Ntk>>
class default_divisor_collector
{
public:
  using stats = default_collector_stats;
  using mffc_result_t = MffcRes;
  using node = typename Ntk::node;

  using cut_comp_parameters_type = typename cut_comp::parameters_type;
  using cut_comp_statistics_type = typename cut_comp::statistics_type;

public:
  explicit default_divisor_collector( Ntk const& ntk, resubstitution_params const& ps, stats& st )
      : ntk( ntk ), ps( ps ), st( st ), cuts( ntk, cut_comp_parameters_type{ ps.max_pis }, cuts_st )
  {
  }

  bool run( node const& n, mffc_result_t& potential_gain )
  {
    /* skip nodes with many fanouts */
    if ( ntk.fanout_size( n ) > ps.skip_fanout_limit_for_roots )
    {
      return false;
    }

    /* compute a reconvergence-driven cut */
    leaves = call_with_stopwatch( st.time_cuts, [&]() {
      return cuts.run( { n } ).first;
    } );
    st.num_total_leaves += leaves.size();

    /* collect the MFFC */
    MffcMgr mffc_mgr( ntk );
    potential_gain = call_with_stopwatch( st.time_mffc, [&]() {
      return mffc_mgr.run( n, leaves, mffc );
    } );

    /* collect the divisor nodes in the cut */
    bool div_comp_success = call_with_stopwatch( st.time_divs, [&]() {
      return collect_divisors( n );
    } );

    if ( !div_comp_success )
    {
      return false;
    }

    return true;
  }

private:
  void collect_divisors_rec( node const& n )
  {
    /* skip visited nodes */
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      return;
    }
    ntk.set_visited( n, ntk.trav_id() );

    ntk.foreach_fanin( n, [&]( const auto& f ) {
      collect_divisors_rec( ntk.get_node( f ) );
    } );

    /* collect the internal nodes */
    if ( ntk.value( n ) == 0 && n != 0 ) /* ntk.fanout_size( n ) */
    {
      divs.emplace_back( n );
    }
  }

  bool collect_divisors( node const& root )
  {
    auto max_depth = std::numeric_limits<uint32_t>::max();
    if ( ps.preserve_depth )
    {
      max_depth = ntk.level( root );
    }
    /* add the leaves of the cuts to the divisors */
    divs.clear();

    ntk.incr_trav_id();
    for ( const auto& l : leaves )
    {
      divs.emplace_back( l );
      ntk.set_visited( l, ntk.trav_id() );
    }

    /* mark nodes in the MFFC */
    for ( const auto& t : mffc )
    {
      ntk.set_value( t, 1 );
    }

    /* collect the cone (without MFFC) */
    collect_divisors_rec( root );

    /* unmark the current MFFC */
    for ( const auto& t : mffc )
    {
      ntk.set_value( t, 0 );
    }

    /* check if the number of divisors is not exceeded */
    if ( divs.size() + mffc.size() - leaves.size() > ps.max_divisors - ps.max_pis )
    {
      return false;
    }
    uint32_t limit = ps.max_divisors - ps.max_pis - mffc.size() + leaves.size();

    /* explore the fanouts, which are not in the MFFC */
    bool quit = false;
    for ( auto i = 0u; i < divs.size(); ++i )
    {
      auto const d = divs.at( i );

      if ( ntk.fanout_size( d ) > ps.skip_fanout_limit_for_divisors )
      {
        continue;
      }
      if ( divs.size() >= limit )
      {
        break;
      }

      /* if the fanout has all fanins in the set, add it */
      ntk.foreach_fanout( d, [&]( node const& p ) {
        if ( ntk.visited( p ) == ntk.trav_id() || ntk.level( p ) > max_depth )
        {
          return true; /* next fanout */
        }

        bool all_fanins_visited = true;
        ntk.foreach_fanin( p, [&]( const auto& g ) {
          if ( ntk.visited( ntk.get_node( g ) ) != ntk.trav_id() )
          {
            all_fanins_visited = false;
            return false; /* terminate fanin-loop */
          }
          return true; /* next fanin */
        } );

        if ( !all_fanins_visited )
          return true; /* next fanout */

        bool has_root_as_child = false;
        ntk.foreach_fanin( p, [&]( const auto& g ) {
          if ( ntk.get_node( g ) == root )
          {
            has_root_as_child = true;
            return false; /* terminate fanin-loop */
          }
          return true; /* next fanin */
        } );

        if ( has_root_as_child )
        {
          return true; /* next fanout */
        }

        divs.emplace_back( p );
        ntk.set_visited( p, ntk.trav_id() );

        /* quit computing divisors if there are too many of them */
        if ( divs.size() >= limit )
        {
          quit = true;
          return false; /* terminate fanout-loop */
        }

        return true; /* next fanout */
      } );

      if ( quit )
      {
        break;
      }
    }

    /* note: different from the previous version, now we do not add MFFC nodes into divs */
    assert( root == mffc.at( mffc.size() - 1u ) );
    /* note: this assertion makes sure window_simulator does not go out of bounds */
    assert( divs.size() + mffc.size() - leaves.size() <= ps.max_divisors - ps.max_pis );

    return true;
  }

private:
  Ntk const& ntk;
  resubstitution_params ps;
  stats& st;

  cut_comp cuts;
  cut_comp_statistics_type cuts_st;

public:
  std::vector<node> leaves;
  std::vector<node> divs;
  std::vector<node> mffc;
};

template<typename ResubFnSt>
struct window_resub_stats
{
  /*! \brief Number of successful resubstitutions. */
  uint32_t num_resub{ 0 };

  /*! \brief Time for simulation. */
  stopwatch<>::duration time_sim{ 0 };

  /*! \brief Time for don't-care computation. */
  stopwatch<>::duration time_dont_care{ 0 };

  /*! \brief Time of the resub functor. */
  stopwatch<>::duration time_compute_function{ 0 };

  ResubFnSt functor_st;

  void report() const
  {
    // clang-format off
    fmt::print( "[i] <ResubEngine: window_based_resub_engine>\n" );
    fmt::print( "[i]     #resub = {:6d}\n", num_resub );
    fmt::print( "[i]     ======== Runtime ========\n" );
    fmt::print( "[i]     simulation: {:>5.2f} secs\n", to_seconds( time_sim ) );
    fmt::print( "[i]     don't care: {:>5.2f} secs\n", to_seconds( time_dont_care ) );
    fmt::print( "[i]     functor   : {:>5.2f} secs\n", to_seconds( time_compute_function ) );
    fmt::print( "[i]     ======== Details ========\n" );
    functor_st.report();
    fmt::print( "[i]     =========================\n\n" );
    // clang-format on
  }
};

/*! \brief Window-based resubstitution engine.
 *
 * This engine computes the complete truth tables of nodes within a window
 * with the leaves as inputs. It does not verify the resubstitution candidates
 * given by the resubstitution functor. This engine requires the divisor
 * collector to prepare three data members: `leaves`, `divs` and `mffc`.
 *
 * Required interfaces of the resubstitution functor:
 * - Constructor: `resub_fn( Ntk const& ntk, Simulator const& sim,`
 * `std::vector<node> const& divs, uint32_t num_divs, ResubFnSt& st )`
 * - A public `operator()`: `std::optional<signal> operator()`
 * `( node const& root, TTdc care, uint32_t required, uint32_t max_inserts,`
 * `MffcRes potential_gain, uint32_t& last_gain ) const`
 *
 * Compatible resubstitution functors implemented:
 * - `default_resub_functor`
 * - `aig_resub_functor`
 * - `mig_resub_functor`
 * - `xmg_resub_functor`
 * - `xag_resub_functor`
 * - `mig_resyn_functor`
 *
 * \param TTsim Truth table type for simulation.
 * \param TTdc Truth table type for don't-care computation.
 * \param ResubFn Resubstitution functor to compute the resubstitution.
 * \param MffcRes Typename of `potential_gain` needed by the resubstitution functor.
 */
template<class Ntk, class TTsim, class TTdc = kitty::dynamic_truth_table, class ResubFn = default_resub_functor<Ntk, window_simulator<Ntk, TTsim>, TTdc>, typename MffcRes = uint32_t>
class window_based_resub_engine
{
public:
  static constexpr bool require_leaves_and_mffc = true;
  using stats = window_resub_stats<typename ResubFn::stats>;
  using mffc_result_t = MffcRes;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit window_based_resub_engine( Ntk& ntk, resubstitution_params const& ps, stats& st )
      : ntk( ntk ), ps( ps ), st( st ), sim( ntk, ps.max_divisors, ps.max_pis )
  {
  }

  void init() {}
  void update() {}

  std::optional<signal> run( node const& n, std::vector<node> const& leaves, std::vector<node> const& divs, std::vector<node> const& mffc, mffc_result_t potential_gain, uint32_t& last_gain )
  {
    /* simulate the collected divisors */
    call_with_stopwatch( st.time_sim, [&]() {
      simulate( leaves, divs, mffc );
    } );

    auto care = kitty::create<TTdc>( ps.max_pis );
    call_with_stopwatch( st.time_dont_care, [&]() {
      if ( ps.use_dont_cares )
      {
        care = ~satisfiability_dont_cares( ntk, leaves, ps.window_size );
      }
      else
      {
        care = ~care;
      }
    } );

    ResubFn resub_fn( ntk, sim, divs, divs.size(), st.functor_st );
    auto res = call_with_stopwatch( st.time_compute_function, [&]() {
      auto max_depth = std::numeric_limits<uint32_t>::max();
      if ( ps.preserve_depth )
      {
        max_depth = ntk.level( n );
      }
      return resub_fn( n, care, max_depth, ps.max_inserts, potential_gain, last_gain );
    } );
    if ( res )
    {
      ++st.num_resub;
    }
    return res;
  }

private:
  void simulate( std::vector<node> const& leaves, std::vector<node> const& divs, std::vector<node> const& mffc )
  {
    sim.resize();
    for ( auto i = 0u; i < divs.size() + mffc.size(); ++i )
    {
      const auto d = i < divs.size() ? divs.at( i ) : mffc.at( i - divs.size() );

      /* skip constant 0 */
      if ( d == 0 )
        continue;

      /* assign leaves to variables */
      if ( i < leaves.size() )
      {
        sim.assign( d, i + 1 );
        continue;
      }

      /* compute truth tables of inner nodes */
      sim.assign( d, i - uint32_t( leaves.size() ) + ps.max_pis + 1 );
      std::vector<TTsim> tts;
      ntk.foreach_fanin( d, [&]( const auto& s ) {
        tts.emplace_back( sim.get_tt( ntk.make_signal( ntk.get_node( s ) ) ) ); /* ignore sign */
      } );

      auto const tt = ntk.compute( d, tts.begin(), tts.end() );
      sim.set_tt( i - uint32_t( leaves.size() ) + ps.max_pis + 1, tt );
    }

    /* normalize truth tables */
    sim.normalize( divs );
    sim.normalize( mffc );
  }

private:
  Ntk& ntk;
  resubstitution_params const& ps;
  stats& st;

  window_simulator<Ntk, TTsim> sim;
}; /* window_based_resub_engine */

/*! \brief The top-level resubstitution framework.
 *
 * \param ResubEngine The engine that computes the resubtitution for a given root
 * node and divisors. One can choose from `window_based_resub_engine` which
 * does complete simulation within small windows, or `simulation_based_resub_engine`
 * which does partial simulation on the whole circuit.
 *
 * \param DivCollector Collects divisors near a given root node, and compute
 * the potential gain (MFFC size or its variants).
 * Currently only `default_divisor_collector` is implemented, but
 * a frontier-based approach may be integrated in the future.
 * When using `window_based_resub_engine`, the `DivCollector` should prepare
 * three public data members: `leaves`, `divs`, and `mffc` (see documentation
 * of `default_divisor_collector` for details). When using `simulation_based_resub_engine`,
 * only `divs` is needed.
 */
template<class Ntk, class ResubEngine = window_based_resub_engine<Ntk, kitty::dynamic_truth_table>, class DivCollector = default_divisor_collector<Ntk>>
class resubstitution_impl
{
public:
  using engine_st_t = typename ResubEngine::stats;
  using collector_st_t = typename DivCollector::stats;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using resub_callback_t = std::function<bool( Ntk&, node const&, signal const& )>;
  using mffc_result_t = typename ResubEngine::mffc_result_t;

  /*! \brief Constructor of the top-level resubstitution framework.
   *
   * \param ntk The network to be optimized.
   * \param ps Resubstitution parameters.
   * \param st Top-level resubstitution statistics.
   * \param engine_st Statistics of the resubstitution engine.
   * \param collector_st Statistics of the divisor collector.
   * \param callback Callback function when a resubstitution is found.
   */
  explicit resubstitution_impl( Ntk& ntk, resubstitution_params const& ps, resubstitution_stats& st, engine_st_t& engine_st, collector_st_t& collector_st )
      : ntk( ntk ), ps( ps ), st( st ), engine_st( engine_st ), collector_st( collector_st )
  {
    static_assert( std::is_same_v<typename ResubEngine::mffc_result_t, typename DivCollector::mffc_result_t>, "MFFC result type of the engine and the collector are different" );

    st.initial_size = ntk.num_gates();

    register_events();
  }

  ~resubstitution_impl()
  {
    ntk.events().release_add_event( add_event );
    ntk.events().release_modified_event( modified_event );
    ntk.events().release_delete_event( delete_event );
  }

  void run( resub_callback_t const& callback = substitute_fn<Ntk> )
  {
    stopwatch t( st.time_total );

    /* start the managers */
    DivCollector collector( ntk, ps, collector_st );
    ResubEngine resub_engine( ntk, ps, engine_st );
    call_with_stopwatch( st.time_resub, [&]() {
      resub_engine.init();
    } );

    progress_bar pbar{ ntk.size(), "resub |{0}| node = {1:>4}   cand = {2:>4}   est. gain = {3:>5}", ps.progress };

    auto const size = ntk.num_gates();
    ntk.foreach_gate( [&]( auto const& n, auto i ) {
      if ( i >= size )
      {
        return false; /* terminate */
      }

      pbar( i, i, candidates, st.estimated_gain );

      /* compute cut, collect divisors, compute MFFC */
      mffc_result_t potential_gain;
      const auto collector_success = call_with_stopwatch( st.time_divs, [&]() {
        return collector.run( n, potential_gain );
      } );
      if ( !collector_success )
      {
        return true; /* next */
      }

      /* update statistics */
      last_gain = 0;
      st.num_total_divisors += collector.divs.size();

      /* try to find a resubstitution with the divisors */
      auto g = call_with_stopwatch( st.time_resub, [&]() {
        if constexpr ( ResubEngine::require_leaves_and_mffc ) /* window-based */
        {
          return resub_engine.run( n, collector.leaves, collector.divs, collector.mffc, potential_gain, last_gain );
        }
        else /* simulation-based */
        {
          return resub_engine.run( n, collector.divs, potential_gain, last_gain );
        }
      } );
      if ( !g )
      {
        return true; /* next */
      }

      /* update progress bar */
      candidates++;
      st.estimated_gain += last_gain;

      /* update network */
      bool updated = call_with_stopwatch( st.time_callback, [&]() {
        return callback( ntk, n, *g );
      } );
      if ( updated )
      {
        resub_engine.update();
      }

      return true; /* next */
    } );
  }

private:
  void register_events()
  {
    auto const update_level_of_new_node = [&]( const auto& n ) {
      ntk.resize_levels();
      update_node_level( n );
    };

    auto const update_level_of_existing_node = [&]( node const& n, const auto& old_children ) {
      (void)old_children;
      ntk.resize_levels();
      update_node_level( n );
    };

    auto const update_level_of_deleted_node = [&]( const auto& n ) {
      ntk.set_level( n, -1 );
    };

    add_event = ntk.events().register_add_event( update_level_of_new_node );
    modified_event = ntk.events().register_modified_event( update_level_of_existing_node );
    delete_event = ntk.events().register_delete_event( update_level_of_deleted_node );
  }

  /* maybe should move to depth_view */
  void update_node_level( node const& n, bool top_most = true )
  {
    uint32_t curr_level = ntk.level( n );

    uint32_t max_level = 0;
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const p = ntk.get_node( f );
      auto const fanin_level = ntk.level( p );
      if ( fanin_level > max_level )
      {
        max_level = fanin_level;
      }
    } );
    ++max_level;

    if ( curr_level != max_level )
    {
      ntk.set_level( n, max_level );

      /* update only one more level */
      if ( top_most )
      {
        ntk.foreach_fanout( n, [&]( const auto& p ) {
          update_node_level( p, false );
        } );
      }
    }
  }

private:
  Ntk& ntk;

  resubstitution_params const& ps;
  resubstitution_stats& st;
  engine_st_t& engine_st;
  collector_st_t& collector_st;

  /* temporary statistics for progress bar */
  uint32_t candidates{ 0 };
  uint32_t last_gain{ 0 };

  /* events */
  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;
  std::shared_ptr<typename network_events<Ntk>::modified_event_type> modified_event;
  std::shared_ptr<typename network_events<Ntk>::delete_event_type> delete_event;
};

} /* namespace detail */

/*! \brief Window-based Boolean resubstitution with default resub functor (only div0). */
template<class Ntk>
void default_resubstitution( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );

  using resub_view_t = fanout_view<depth_view<Ntk>>;
  depth_view<Ntk> depth_view{ ntk };
  resub_view_t resub_view{ depth_view };

  if ( ps.max_pis == 8 )
  {
    using truthtable_t = kitty::static_truth_table<8>;
    using truthtable_dc_t = kitty::dynamic_truth_table;
    using resub_impl_t = detail::resubstitution_impl<resub_view_t, typename detail::window_based_resub_engine<resub_view_t, truthtable_t, truthtable_dc_t>>;

    resubstitution_stats st;
    typename resub_impl_t::engine_st_t engine_st;
    typename resub_impl_t::collector_st_t collector_st;

    resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
    p.run();

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
  else
  {
    using resub_impl_t = detail::resubstitution_impl<resub_view_t>;

    resubstitution_stats st;
    typename resub_impl_t::engine_st_t engine_st;
    typename resub_impl_t::collector_st_t collector_st;

    resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
    p.run();

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
}

} /* namespace mockturtle */
