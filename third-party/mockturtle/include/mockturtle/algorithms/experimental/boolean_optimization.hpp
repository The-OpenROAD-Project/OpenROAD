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
  \file boolean_optimization.hpp
  \brief A general logic optimization framework using Boolean methods

  \author Hanyu Wang
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../traits.hpp"
#include "../../utils/progress_bar.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../utils/null_utils.hpp"
#include "../../views/topo_view.hpp"

#include <optional>

namespace mockturtle::experimental
{

template<class WinParams = null_params, class ResynParams = null_params>
struct boolean_optimization_params
{
  /*! \brief Show progress. */
  bool progress{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Whether to use new nodes as pivots. */
  bool optimize_new_nodes{ false };

  /*! \brief Whether to run in dry-run mode (call `report` instead of `update_ntk`). */
  bool dry_run{ false };

  /*! \brief Whether to print verbosely in dry-run mode. Ignored if `dry_run` is `false`. */
  bool dry_run_verbose{ true };

  /*! \brief Parameter object for the windowing engine. */
  WinParams wps;

  /*! \brief Parameter object for the resynthesis engine. */
  ResynParams rps;
};

template<class WinStats = null_stats, class ResynStats = null_stats>
struct boolean_optimization_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Accumulated runtime of structural analysis and simulation. */
  stopwatch<>::duration time_windowing{ 0 };

  /*! \brief Accumulated runtime of resynthesis. */
  stopwatch<>::duration time_resynthesis{ 0 };

  /*! \brief Accumulated runtime of updating network. */
  stopwatch<>::duration time_update{ 0 };

  /*! \brief Total number of gain. */
  uint32_t estimated_gain{ 0 };

  /*! \brief Initial network size (before resubstitution). */
  uint32_t initial_size{ 0 };

  /*! \brief Number of constructed resynthesis problems. */
  uint32_t num_problems{ 0u };

  /*! \brief Number of solutions found. */
  uint32_t num_solutions{ 0u };

  /*! \brief Statistics object for the windowing engine. */
  WinStats wst;

  /*! \brief Statistics object for the resynthesis engine. */
  ResynStats rst;

  void report() const
  {
    // clang-format off
    fmt::print( "[i] Boolean optimization top-level report\n" );
    fmt::print( "Estimated gain: {:8d} ({:.2f}%)\n", estimated_gain, ( 100.0 * estimated_gain ) / initial_size );
    fmt::print( "#problems = {}, #solutions = {} ({:.2f}%)\n", num_problems, num_solutions, float( num_solutions ) / float( num_problems ) );
    fmt::print( "======== Runtime Breakdown ========\n" );
    fmt::print( "Total         : {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "  Windowing   : {:>5.2f} secs\n", to_seconds( time_windowing ) );
    fmt::print( "  Resynthesis : {:>5.2f} secs\n", to_seconds( time_resynthesis ) );
    fmt::print( "  Update ntk  : {:>5.2f} secs\n", to_seconds( time_update ) );
    fmt::print( "========= Windowing Stats =========\n" );
    wst.report();
    fmt::print( "======== Resynthesis Stats ========\n" );
    rst.report();
    fmt::print( "===================================\n\n" );
    // clang-format on
  }
};

namespace detail
{

/*! \brief Logic optimization using Boolean methods.
 *
 * \tparam Ntk Network type.
 * \tparam Windowing Implementation of a windowing algorithm that creates
 * a resynthesis problem to be solved.
 * \tparam ResynSolver Implementation of a resynthesis algorithm that
 * solves the resynthesis problem created by `Windowing`.
 */
template<class Ntk, class Windowing, class ResynSolver>
class boolean_optimization_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  using problem_t = typename Windowing::problem_t;
  using res_t = typename ResynSolver::res_t;
  using params_t = boolean_optimization_params<typename Windowing::params_t, typename ResynSolver::params_t>;
  using stats_t = boolean_optimization_stats<typename Windowing::stats_t, typename ResynSolver::stats_t>;

  explicit boolean_optimization_impl( Ntk& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk ), ps( ps ), st( st ), windowing( ntk, ps.wps, st.wst ), resyn( ntk, ps.rps, st.rst )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
    static_assert( std::is_same_v<problem_t, typename ResynSolver::problem_t>, "Types of resynthesis problem of Windowing and ResynSolver do not match" );
  }

  ~boolean_optimization_impl()
  {}

  void run()
  {
    stopwatch t( st.time_total );
    progress_bar pbar{ ntk.size(), "B-opt |{0}| node = {1:>4}   cand = {2:>4}   est. gain = {3:>5}", ps.progress };

    /* initialize */
    call_with_stopwatch( st.time_windowing, [&]() {
      windowing.init();
    } );
    call_with_stopwatch( st.time_resynthesis, [&]() {
      resyn.init();
    } );

    st.initial_size = ntk.num_gates();
    topo_view<Ntk>{ ntk }.foreach_gate( [&]( auto const n, auto i ) { // TODO: maybe problematic
      if ( !ps.optimize_new_nodes && i >= st.initial_size )
      {
        return false; /* terminate */
      }
      pbar( i, i, candidates, st.estimated_gain );

      /* construct a resynthesis problem; usually by creating a window around the root node */
      auto prob = call_with_stopwatch( st.time_windowing, [&]() {
        return windowing( n );
      } );
      if ( !prob )
      {
        return true; /* next */
      }
      ++st.num_problems;

      /* solve the resynthesis problem; usually by finding a (re)substitution */
      auto res = call_with_stopwatch( st.time_resynthesis, [&]() {
        return resyn( *prob );
      } );
      if ( !res )
      {
        return true; /* next */
      }
      ++st.num_solutions;

      /* update progress bar */
      candidates++;
      st.estimated_gain += windowing.gain( *prob, *res );

      /* update network or report choice */
      bool cont = true;
      if ( !ps.dry_run )
      {
        cont = call_with_stopwatch( st.time_update, [&]() {
          return windowing.update_ntk( *prob, *res );
        } );
      }
      else if ( ps.dry_run_verbose )
      {
        cont = windowing.report( *prob, *res );
      }

      return cont;
    } );
  }

private:
  Ntk& ntk;

  params_t const& ps;
  stats_t& st;

  Windowing windowing;
  ResynSolver resyn;

  /* temporary statistics for progress bar */
  uint32_t candidates{ 0 };
}; /* boolean_optimization_impl */

template<class Ntk>
struct null_problem
{
  using node = typename Ntk::node;
  node pivot;
};

/*! \brief A windowing implementation that creates windows of only the pivot node.
 *
 * This class is an example to demonstrate the interfaces required by
 * the `Windowing` template argument of class `boolean_optimization_impl`.
 * It is designed to be used together with `null_resynthesis`.
 */
template<class Ntk>
class null_windowing
{
public:
  using problem_t = null_problem<Ntk>;
  using res_t = typename Ntk::signal;
  using params_t = null_params;
  using stats_t = null_stats;
  using node = typename Ntk::node;

  explicit null_windowing( Ntk& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk )
  {
    (void)ps;
    (void)st;
  }

  void init()
  {}

  std::optional<problem_t> operator()( node const& n )
  {
    return problem_t{ n };
  }

  uint32_t gain( problem_t const& prob, res_t const& res ) const
  {
    (void)prob;
    (void)res;
    return 0u;
  }

  bool update_ntk( problem_t const& prob, res_t const& res )
  {
    ntk.substitute_node( prob.pivot, res );
    return true;
  }

  bool report( problem_t const& prob, res_t const& res )
  {
    fmt::print( "[i] substitute node {} with signal {}{}\n", prob.pivot, ntk.is_complemented( res ) ? "!" : "", ntk.get_node( res ) );
    return true;
  }

private:
  Ntk& ntk;
};

/*! \brief A resynthesis implementation that returns the pivot node itself.
 *
 * This class is an example to demonstrate the interfaces required by
 * the `ResynSolver` template argument of class `boolean_optimization_impl`.
 * It is designed to be used together with `null_windowing`.
 */
template<class Ntk>
class null_resynthesis
{
public:
  using problem_t = null_problem<Ntk>;
  using res_t = typename Ntk::signal;
  using params_t = null_params;
  using stats_t = null_stats;

  explicit null_resynthesis( Ntk const& ntk, params_t const& ps, stats_t& st )
      : ntk( ntk )
  {
    (void)ps;
    (void)st;
  }

  void init()
  {}

  std::optional<res_t> operator()( problem_t& prob )
  {
    return ntk.make_signal( prob.pivot );
  }

private:
  Ntk const& ntk;
};

} /* namespace detail */

template<class Ntk, typename params_t = boolean_optimization_params<null_params, null_params>, typename stats_t = boolean_optimization_stats<null_stats, null_stats>>
void null_optimization( Ntk& ntk, params_t const& ps = {}, stats_t* pst = nullptr )
{
  stats_t st;

  using windowing_t = typename detail::null_windowing<Ntk>;
  using resyn_t = typename detail::null_resynthesis<Ntk>;
  using opt_t = typename detail::boolean_optimization_impl<Ntk, windowing_t, resyn_t>;

  opt_t p( ntk, ps, st );
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