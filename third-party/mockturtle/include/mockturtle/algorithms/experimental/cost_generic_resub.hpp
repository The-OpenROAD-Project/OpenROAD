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
  \file cost_generic_resub.hpp
  \brief generic widnowing algorithm with customized cost function

  \author Hanyu Wang
*/

#pragma once

#include "../../networks/aig.hpp"
#include "../../networks/xag.hpp"
#include "../../traits.hpp"
#include "../../utils/index_list/index_list.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/cost_view.hpp"
#include "../../views/depth_view.hpp"
#include "../../views/fanout_view.hpp"
#include "../../views/topo_view.hpp"
#include "../detail/resub_utils.hpp"
#include "../dont_cares.hpp"
#include "../reconv_cut.hpp"
#include "../simulation.hpp"
#include "boolean_optimization.hpp"
#include "cost_resyn.hpp"
#include <kitty/kitty.hpp>

#include <functional>
#include <optional>
#include <vector>

namespace mockturtle::experimental
{

/*! \brief Parameters for cost.
 */
struct costfn_windowing_params
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

  /*! \brief Use don't cares for optimization. */
  bool use_dont_cares{ false };

  /*! \brief Window size for don't cares calculation. */
  uint32_t window_size{ 12u };

  /*! \brief Whether to normalize the truth tables.
   *
   * For some enumerative resynthesis engines, if the truth tables
   * are normalized, some cases can be eliminated and thus improves
   * efficiency. When this option is turned off, be sure to use an
   * implementation of resynthesis that does not make this assumption;
   * otherwise, quality degradation may be observed.
   *
   * Normalization is typically only useful for enumerative methods
   * and for smaller solutions (i.e. when `max_inserts` < 2). Turning
   * on normalization may result in larger runtime overhead when there
   * are many divisors or when the truth tables are long.
   */
  bool normalize{ false };
};

struct costfn_windowing_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Accumulated runtime for cut computation. */
  stopwatch<>::duration time_cuts{ 0 };

  /*! \brief Accumulated runtime for mffc computation. */
  stopwatch<>::duration time_mffc{ 0 };

  /*! \brief Accumulated runtime for divisor collection. */
  stopwatch<>::duration time_divs{ 0 };

  /*! \brief Accumulated runtime for simulation. */
  stopwatch<>::duration time_sim{ 0 };

  /*! \brief Accumulated runtime for don't care computation. */
  stopwatch<>::duration time_dont_care{ 0 };

  /*! \brief Total number of leaves. */
  uint64_t num_leaves{ 0u };

  /*! \brief Total number of divisors. */
  uint64_t num_divisors{ 0u };

  /*! \brief Number of constructed windows. */
  uint32_t num_windows{ 0u };

  /*! \brief Total number of MFFC nodes. */
  uint64_t sum_mffc_size{ 0u };

  void report() const
  {
    // clang-format off
    fmt::print( "[i] costfn_windowing report\n" );
    fmt::print( "    tot. #leaves = {:5d}, tot. #divs = {:5d}, sum  |MFFC| = {:5d}\n", num_leaves, num_divisors, sum_mffc_size );
    fmt::print( "    avg. #leaves = {:>5.2f}, avg. #divs = {:>5.2f}, avg. |MFFC| = {:>5.2f}\n", float( num_leaves ) / float( num_windows ), float( num_divisors ) / float( num_windows ), float( sum_mffc_size ) / float( num_windows ) );
    fmt::print( "    ===== Runtime Breakdown =====\n" );
    fmt::print( "    Total       : {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "      Cut       : {:>5.2f} secs\n", to_seconds( time_cuts ) );
    fmt::print( "      MFFC      : {:>5.2f} secs\n", to_seconds( time_mffc ) );
    fmt::print( "      Divs      : {:>5.2f} secs\n", to_seconds( time_divs ) );
    fmt::print( "      Simulation: {:>5.2f} secs\n", to_seconds( time_sim ) );
    fmt::print( "      Dont cares: {:>5.2f} secs\n", to_seconds( time_dont_care ) );
    // clang-format on
  }
};

namespace detail
{
template<class Ntk, class TT>
struct cost_aware_problem
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  signal root;
  std::vector<signal> divs;
  std::vector<uint32_t> div_ids;    /* positions of divisor truth tables in `tts` */
  std::vector<node> div_id_to_node; /* maps IDs in `div_ids` to the corresponding node */
  std::vector<TT> tts;
  TT care;
  uint32_t mffc_size;
  uint32_t max_cost{ std::numeric_limits<uint32_t>::max() };
};

template<class Ntk, class TT = kitty::dynamic_truth_table>
class costfn_windowing
{
public:
  using problem_t = cost_aware_problem<Ntk, TT>;
  using params_t = costfn_windowing_params;
  using stats_t = costfn_windowing_stats;

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit costfn_windowing( Ntk& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk ), ps( ps ), st( st ), cps( { ps.max_pis } ), mffc_mgr( ntk ),
        divs_mgr( ntk, divisor_collector_params( { ps.max_divisors, ps.max_divisors, ps.skip_fanout_limit_for_divisors } ) ),
        sim( ntk, win.tts, ps.max_pis )
  {
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
    static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
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

    /* compute a cut and collect supported nodes */
    std::vector<node> leaves = call_with_stopwatch( st.time_cuts, [&]() {
      return reconvergence_driven_cut<Ntk, false, has_level_v<Ntk>>( ntk, { n }, cps ).first;
    } );
    std::vector<node> supported;
    call_with_stopwatch( st.time_divs, [&]() {
      divs_mgr.collect_supported_nodes( n, leaves, supported );
    } );

    /* simulate */
    call_with_stopwatch( st.time_sim, [&]() {
      sim.simulate( leaves, supported );
    } );

    /* mark MFFC nodes and collect divisors */
    ++mffc_marker;
    win.mffc_size = call_with_stopwatch( st.time_mffc, [&]() {
      return mffc_mgr.call_on_mffc_and_count( n, leaves, [&]( node const& n ) {
        ntk.set_value( n, mffc_marker );
      } );
    } );
    call_with_stopwatch( st.time_divs, [&]() {
      collect_divisors( leaves, supported );
    } );

    /* normalize */
    call_with_stopwatch( st.time_sim, [&]() {
      if ( ps.normalize )
      {
        win.root = normalize_truth_tables() ? !ntk.make_signal( n ) : ntk.make_signal( n );
      }
      else
      {
        win.root = ntk.make_signal( n );
      }
    } );

    /* compute don't cares */
    call_with_stopwatch( st.time_dont_care, [&]() {
      if ( ps.use_dont_cares )
      {
        win.care = ~satisfiability_dont_cares( ntk, leaves, ps.window_size );
      }
      else
      {
        win.care = ~kitty::create<TT>( ps.max_pis );
      }
    } );

    /* compute cost */
    win.max_cost = ntk.get_cost( n, win.divs );

    st.num_windows++;
    st.num_leaves += leaves.size();
    st.num_divisors += win.divs.size();
    st.sum_mffc_size += win.mffc_size;

    return win;
  }

  template<typename res_t>
  uint32_t gain( problem_t const& prob, res_t const& res ) const
  {
    static_assert( is_index_list_v<res_t>, "res_t is not an index_list (windowing engine and resynthesis engine do not match)" );
    return 1; /* cannot predict the final cost */
  }

  template<typename res_t>
  bool update_ntk( problem_t const& prob, res_t const& res )
  {
    static_assert( is_index_list_v<res_t>, "res_t is not an index_list (windowing engine and resynthesis engine do not match)" );
    assert( res.num_pos() == 1 );
    insert( ntk, std::begin( prob.divs ), std::end( prob.divs ), res, [&]( signal const& g ) {
      ntk.substitute_node( ntk.get_node( prob.root ), ntk.is_complemented( prob.root ) ? !g : g );
    } );
    return true; /* continue optimization */
  }

  template<typename res_t>
  bool report( problem_t const& prob, res_t const& res )
  {
    static_assert( is_index_list_v<res_t>, "res_t is not an index_list (windowing engine and resynthesis engine do not match)" );
    assert( res.num_pos() == 1 );
    fmt::print( "[i] found solution {} for root signal {}{}\n", to_index_list_string( res ), ntk.is_complemented( prob.root ) ? "!" : "", ntk.get_node( prob.root ) );
    return true;
  }

private:
  void collect_divisors( std::vector<node> const& leaves, std::vector<node> const& supported )
  {
    win.divs.clear();
    win.div_ids.clear();

    uint32_t i{ 1 };
    for ( auto const& l : leaves )
    {
      win.div_ids.emplace_back( i++ );
      win.divs.emplace_back( ntk.make_signal( l ) );
    }

    i = ps.max_pis + 1;
    for ( auto const& n : supported )
    {
      if ( ntk.value( n ) != mffc_marker ) /* not in MFFC, not root */
      {
        win.div_ids.emplace_back( i );
        win.divs.emplace_back( ntk.make_signal( n ) );
      }
      ++i;
    }
    assert( i == win.tts.size() );
  }

  bool normalize_truth_tables()
  {
    assert( win.divs.size() == win.div_ids.size() );
    for ( auto i = 0u; i < win.divs.size(); ++i )
    {
      if ( kitty::get_bit( win.tts.at( win.div_ids.at( i ) ), 0 ) )
      {
        win.tts.at( win.div_ids.at( i ) ) = ~win.tts.at( win.div_ids.at( i ) );
        win.divs.at( i ) = !win.divs.at( i );
      }
    }

    if ( kitty::get_bit( win.tts.back(), 0 ) )
    {
      win.tts.back() = ~win.tts.back();
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  Ntk& ntk;
  problem_t win;
  params_t const& ps;
  stats_t& st;
  reconvergence_driven_cut_parameters const cps;
  typename mockturtle::detail::node_mffc_inside<Ntk> mffc_mgr; // TODO: namespaces can be removed when we move out of experimental::
  divisor_collector<Ntk> divs_mgr;
  window_simulator<Ntk, TT> sim;
  uint32_t mffc_marker{ 0u };
  std::shared_ptr<typename network_events<Ntk>::modified_event_type> lazy_update_event;
}; /* costfn_windowing */

template<class Ntk, class TT, class ResynEngine>
class costfn_resynthesis
{
public:
  using problem_t = cost_aware_problem<Ntk, TT>;
  using res_t = typename ResynEngine::index_list_t;
  using params_t = typename ResynEngine::params;
  using stats_t = typename ResynEngine::stats;

  explicit costfn_resynthesis( Ntk const& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk ), engine( ntk, ps, st )
  {
    static_assert( has_cost_v<Ntk>, "Ntk does not implement the get_cost method" );
  }

  void init()
  {
  }

  std::optional<res_t> operator()( problem_t& prob )
  {
    return engine( prob.tts.back(), prob.care, prob.divs, std::begin( prob.div_ids ), std::end( prob.div_ids ), prob.tts, prob.max_cost );
  }

private:
  Ntk const& ntk;
  typename ResynEngine::stats rst;
  ResynEngine engine;
}; /* costfn_resynthesis */

} /* namespace detail */

using cost_generic_resub_params = boolean_optimization_params<costfn_windowing_params, cost_resyn_params>;
using cost_generic_resub_stats = boolean_optimization_stats<costfn_windowing_stats, cost_resyn_stats>;

/*! \brief Cost-generic resubstitution algorithm.
 *
 * This algorithm creates a reconvergence-driven window for each node in the
 * network, collects divisors, and builds the resynthesis problem. A search core
 * then collects all the resubstitution candidates with the same functionality as
 * the target. The candidate with the lowest cost will then replace the MFFC
 * of the window.
 *
 * \param ntk Network
 * \param cost_fn Customized cost function
 * \param ps Optimization params
 * \param pst Optimization statistics
 */
template<class Ntk, class CostFn>
void cost_generic_resub( Ntk& ntk, CostFn cost_fn, cost_generic_resub_params const& ps, cost_generic_resub_stats* pst = nullptr )
{
  fanout_view fntk( ntk );
  cost_view viewed( fntk, cost_fn );
  using Viewed = decltype( viewed );
  using TT = typename kitty::dynamic_truth_table;
  using windowing_t = typename detail::costfn_windowing<Viewed, TT>;
  using engine_t = cost_resyn<Viewed, TT>;
  using resyn_t = typename detail::costfn_resynthesis<Viewed, TT, engine_t>;
  using opt_t = typename detail::boolean_optimization_impl<Viewed, windowing_t, resyn_t>;

  cost_generic_resub_stats st;
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

} // namespace mockturtle::experimental