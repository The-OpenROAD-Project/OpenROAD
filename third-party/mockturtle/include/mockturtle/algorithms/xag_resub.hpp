/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2024  EPFL
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
  \file xag_resub.hpp
  \brief XAG-specific resubstitution rules

  \author Alessandro Tempia Calvino
*/

#pragma once

#include "../networks/xag.hpp"
#include "../utils/index_list/index_list.hpp"
#include "../utils/truth_table_utils.hpp"
#include "resubstitution.hpp"
#include "resyn_engines/xag_resyn.hpp"

#include <kitty/kitty.hpp>

namespace mockturtle
{

struct xag_resyn_resub_stats
{
  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_compute_function{ 0 };

  /*! \brief Number of found solutions. */
  uint32_t num_success{ 0 };

  /*! \brief Number of times that no solution can be found. */
  uint32_t num_fail{ 0 };

  void report() const
  {
    fmt::print( "[i]     <ResubFn: xag_resyn_functor>\n" );
    fmt::print( "[i]         #solution = {:6d}\n", num_success );
    fmt::print( "[i]         #invoke   = {:6d}\n", num_success + num_fail );
    fmt::print( "[i]         engine time: {:>5.2f} secs\n", to_seconds( time_compute_function ) );
  }
}; /* xag_resyn_resub_stats */

/*! \brief Interfacing resubstitution functor with XAG resynthesis engines for `window_based_resub_engine`.
 */
template<typename Ntk, typename Simulator, typename TTcare, typename ResynEngine = xag_resyn_decompose<typename Simulator::truthtable_t, xag_resyn_static_params_for_win_resub<Ntk>>>
struct xag_resyn_functor
{
public:
  using node = xag_network::node;
  using signal = xag_network::signal;
  using stats = xag_resyn_resub_stats;
  using TT = typename ResynEngine::truth_table_t;

  static_assert( std::is_same_v<TT, typename Simulator::truthtable_t>, "truth table type of the simulator does not match" );

public:
  explicit xag_resyn_functor( Ntk& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, stats& st )
      : ntk( ntk ), sim( sim ), tts( ntk ), divs( divs ), st( st )
  {
    assert( divs.size() == num_divs );
    (void)num_divs;
    div_signals.reserve( divs.size() );
  }

  std::optional<signal> operator()( node const& root, TTcare care, uint32_t required, uint32_t max_inserts, uint32_t potential_gain, uint32_t& real_gain )
  {
    (void)required;
    TT target = sim.get_tt( sim.get_phase( root ) ? !ntk.make_signal( root ) : ntk.make_signal( root ) );
    TT care_transformed = target.construct();
    care_transformed = care;

    typename ResynEngine::stats st_eng;
    ResynEngine engine( st_eng );
    for ( auto const& d : divs )
    {
      div_signals.emplace_back( sim.get_phase( d ) ? !ntk.make_signal( d ) : ntk.make_signal( d ) );
      tts[d] = sim.get_tt( ntk.make_signal( d ) );
    }

    auto const res = call_with_stopwatch( st.time_compute_function, [&]() {
      return engine( target, care_transformed, std::begin( divs ), std::end( divs ), tts, std::min( potential_gain - 1, max_inserts ) );
    } );
    if ( res )
    {
      ++st.num_success;
      signal ret;
      real_gain = potential_gain - ( *res ).num_gates();
      insert( ntk, div_signals.begin(), div_signals.end(), *res, [&]( signal const& s ) { ret = s; } );
      return ret;
    }
    else
    {
      ++st.num_fail;
      return std::nullopt;
    }
  }

private:
  Ntk& ntk;
  Simulator const& sim;
  unordered_node_map<TT, Ntk> tts;
  std::vector<node> const& divs;
  std::vector<signal> div_signals;
  stats& st;
}; /* xag_resyn_functor */

/*! \brief XAG-specific resubstitution algorithm.
 *
 * This algorithms iterates over each node, creates a
 * reconvergence-driven cut, and attempts to re-express the node's
 * function using existing nodes from the cut.  Node which are no
 * longer used (including nodes in their transitive fanins) can then
 * be removed.  The objective is to reduce the size of the network as
 * much as possible while maintaining the global input-output
 * functionality.
 *
 * **Required network functions:**
 *
 * - `clear_values`
 * - `fanout_size`
 * - `foreach_fanin`
 * - `foreach_fanout`
 * - `foreach_gate`
 * - `foreach_node`
 * - `get_constant`
 * - `get_node`
 * - `is_complemented`
 * - `is_pi`
 * - `level`
 * - `make_signal`
 * - `set_value`
 * - `set_visited`
 * - `size`
 * - `substitute_node`
 * - `value`
 * - `visited`
 *
 * \param ntk A network type derived from xag_network
 * \param ps Resubstitution parameters
 * \param pst Resubstitution statistics
 */
template<class Ntk>
void xag_resubstitution( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Network type is not xag_network" );

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
  static_assert( has_size_v<Ntk>, "Ntk does not implement the has_size method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the has substitute_node method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the has_value method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the has_visited method" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );

  using truthtable_t = kitty::dynamic_truth_table;
  using truthtable_dc_t = kitty::dynamic_truth_table;
  using functor_t = xag_resyn_functor<Ntk, detail::window_simulator<Ntk, truthtable_t>, truthtable_dc_t>;

  using resub_impl_t = detail::resubstitution_impl<Ntk, detail::window_based_resub_engine<Ntk, truthtable_t, truthtable_dc_t, functor_t>>;

  resubstitution_stats st;
  typename resub_impl_t::engine_st_t engine_st;
  typename resub_impl_t::collector_st_t collector_st;

  resub_impl_t p( ntk, ps, st, engine_st, collector_st );
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

} /* namespace mockturtle */
