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
  \file functional_reduction.hpp
  \brief Functional reduction for any network type

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/fanout_view.hpp"

#include <bill/sat/interface/abc_bsat2.hpp>
#include <kitty/partial_truth_table.hpp>

#include "../io/write_patterns.hpp"
#include "circuit_validator.hpp"
#include "simulation.hpp"

namespace mockturtle
{

struct functional_reduction_params
{
  /*! \brief Show progress. */
  bool progress{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Maximum number of iterations to run. 0 = repeat until no further improvement can be found. */
  uint32_t max_iterations{ 10 };

  /*! \brief Whether to use pre-generated patterns stored in a file.
   * If not, by default, 256 random patterns will be used.
   */
  std::optional<std::string> pattern_filename{};

  /*! \brief Whether to save the appended patterns (with CEXs) into file. */
  std::optional<std::string> save_patterns{};

  /*! \brief Maximum number of nodes in the transitive fanin cone (and their fanouts) to be compared to. */
  uint32_t max_TFI_nodes{ 1000 };

  /*! \brief Maximum fanout count of a node in the transitive fanin cone to explore its fanouts. */
  uint32_t skip_fanout_limit{ 100 };

  /*! \brief Conflict limit for the SAT solver. */
  uint32_t conflict_limit{ 100 };

  /*! \brief Maximum number of clauses of the SAT solver. (incremental CNF construction) */
  uint32_t max_clauses{ 1000 };

  /*! \brief Initial number of (random) simulation patterns. */
  uint32_t num_patterns{ 256 };

  /*! \brief Maximum number of simulation patterns. Discards all patterns and re-seeds with random patterns when exceeded. */
  uint32_t max_patterns{ 1024 };
};

struct functional_reduction_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Time for simulation. */
  stopwatch<>::duration time_sim{ 0 };

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_sat{ 0 };

  /*! \brief Number of accepted constant nodes. */
  uint32_t num_const_accepts{ 0 };

  /*! \brief Number of accepted functionally equivalent nodes. */
  uint32_t num_equ_accepts{ 0 };

  /*! \brief Number of counter-examples (SAT calls). */
  uint32_t num_cex{ 0 };

  /*! \brief Number of successful node reductions (UNSAT calls). */
  uint32_t num_reduction{ 0 };

  /*! \brief Number of SAT solver timeout. */
  uint32_t num_timeout{ 0 };

  void report() const
  {
    // clang-format off
    std::cout <<              "[i] Functional Reduction\n";
    std::cout <<              "[i] ========  Stats  ========\n";
    std::cout << fmt::format( "[i] #constant = {:8d}\n", num_const_accepts );
    std::cout << fmt::format( "[i] #FE pairs = {:8d}\n", num_equ_accepts );
    std::cout << fmt::format( "[i] #SAT      = {:8d}\n", num_cex );
    std::cout << fmt::format( "[i] #UNSAT    = {:8d}\n", num_reduction );
    std::cout << fmt::format( "[i] #TIMEOUT  = {:8d}\n", num_timeout );
    std::cout <<              "[i] ======== Runtime ========\n";
    std::cout << fmt::format( "[i] total        : {:>5.2f} secs\n", to_seconds( time_total ) );
    std::cout << fmt::format( "[i]   simulation : {:>5.2f} secs\n", to_seconds( time_sim ) );
    std::cout << fmt::format( "[i]   SAT solving: {:>5.2f} secs\n", to_seconds( time_sat ) );
    std::cout <<              "[i] =========================\n\n";
    // clang-format on
  }
};

namespace detail
{
template<typename Ntk, typename validator_t = circuit_validator<Ntk, bill::solvers::bsat2>>
class functional_reduction_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using TT = unordered_node_map<kitty::partial_truth_table, Ntk>;

  explicit functional_reduction_impl( Ntk& ntk, functional_reduction_params const& ps, validator_params const& vps, functional_reduction_stats& st )
      : ntk( ntk ), ps( ps ), st( st ), tts( ntk ),
        sim( ps.pattern_filename ? partial_simulator( *ps.pattern_filename ) : partial_simulator( ntk.num_pis(), ps.num_patterns, std::rand() ) ), validator( ntk, vps )
  {
    static_assert( !validator_t::use_odc_, "`circuit_validator::use_odc` flag should be turned off." );
  }

  ~functional_reduction_impl()
  {
    if ( ps.save_patterns )
    {
      write_patterns( sim, *ps.save_patterns );
    }
  }

  void run()
  {
    stopwatch t( st.time_total );

    /* first simulation: the whole circuit; from 0 bits. */
    call_with_stopwatch( st.time_sim, [&]() {
      simulate_nodes<Ntk>( ntk, tts, sim, true );
    } );

    /* remove constant nodes. */
    substitute_constants();

    /* substitute functional equivalent nodes. */
    auto size_before = ntk.size();
    substitute_equivalent_nodes();
    uint32_t iterations{0};
    while ( ps.max_iterations && iterations++ <= ps.max_iterations && ntk.size() != size_before )
    {
      size_before = ntk.size();
      substitute_equivalent_nodes();
    }
  }

private:
  void substitute_constants()
  {
    progress_bar pbar{ ntk.size(), "FR-const |{0}| node = {1:>4}   cand = {2:>4}", ps.progress };

    auto zero = sim.compute_constant( false );
    auto one = sim.compute_constant( true );
    ntk.foreach_gate( [&]( auto const& n, auto i ) {
      pbar( i, i, candidates );

      check_tts( n );
      bool const_value;
      if ( tts[n] == zero )
      {
        const_value = false;
      }
      else if ( tts[n] == one )
      {
        const_value = true;
      }
      else /* not constant */
      {
        return true; /* next */
      }

      /* update progress bar */
      candidates++;

      const auto res = call_with_stopwatch( st.time_sat, [&]() {
        return validator.validate( n, const_value );
      } );
      if ( !res ) /* timeout */
      {
        ++st.num_timeout;
        return true;
      }
      else if ( !( *res ) ) /* SAT, cex found */
      {
        found_cex();
        zero = sim.compute_constant( false );
        one = sim.compute_constant( true );
      }
      else /* UNSAT, constant verified */
      {
        ++st.num_reduction;
        ++st.num_const_accepts;
        /* update network */
        ntk.substitute_node( n, ntk.get_constant( const_value ) );
      }
      return true;
    } );
  }

  void substitute_equivalent_nodes()
  {
    progress_bar pbar{ ntk.size(), "FR-equ |{0}| node = {1:>4}   cand = {2:>4}", ps.progress };
    ntk.foreach_gate( [&]( auto const& root, auto i ) {
      pbar( i, i, candidates );

      check_tts( root );
      auto tt = tts[root];
      auto ntt = ~tts[root];
      std::vector<node> tfi;
      bool keep_trying = true;
      foreach_transitive_fanin( root, [&]( auto const& n ) {
        tfi.emplace_back( n );
        if ( tfi.size() > ps.max_TFI_nodes )
        {
          return false;
        }

        keep_trying = try_node( tt, ntt, root, n );
        return keep_trying;
      } );

      if ( keep_trying ) /* didn't find a substitution in TFI cone, explore fanouts. */
      {
        for ( auto j = 0u; j < tfi.size() && tfi.size() <= ps.max_TFI_nodes && keep_trying; ++j )
        {
          auto& n = tfi.at( j );
          if ( ntk.fanout_size( n ) > ps.skip_fanout_limit )
          {
            continue;
          }

          /* if the fanout has all fanins in the set, add it */
          ntk.foreach_fanout( n, [&]( node const& p ) {
            if ( ntk.visited( p ) == ntk.trav_id() )
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
            {
              return true; /* next fanout */
            }

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

            tfi.emplace_back( p );
            ntk.set_visited( p, ntk.trav_id() );

            check_tts( p );
            keep_trying = try_node( tt, ntt, root, p );
            return keep_trying;
          } );
        }
      }

      return true; /* next */
    } );
  }

  bool try_node( kitty::partial_truth_table& tt, kitty::partial_truth_table& ntt, node const& root, node const& n )
  {
    signal g;
    if ( tt == tts[n] )
    {
      g = ntk.make_signal( n );
    }
    else if ( ntt == tts[n] )
    {
      g = !ntk.make_signal( n );
    }
    else /* not equivalent */
    {
      return true; /* try next transitive fanin node */
    }

    /* update progress bar */
    candidates++;

    const auto res = call_with_stopwatch( st.time_sat, [&]() {
      return validator.validate( root, g );
    } );
    if ( !res ) /* timeout */
    {
      ++st.num_timeout;
      return true; /* try next transitive fanin node */
    }
    else if ( !( *res ) ) /* SAT, cex found */
    {
      found_cex();
      check_tts( root );
      tt = tts[root];
      ntt = ~tts[root];
      return true; /* try next transitive fanin node */
    }
    else /* UNSAT, equivalent node verified */
    {
      ++st.num_reduction;
      ++st.num_equ_accepts;
      /* update network */
      ntk.substitute_node( root, g );
      return false; /* break `foreach_transitive_fanin` */
    }
  }

  void found_cex()
  {
    ++st.num_cex;
    sim.add_pattern( validator.cex );

    if ( sim.num_bits() > ps.max_patterns )
    {
      reseed_patterns();
      return;
    }

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

  void reseed_patterns()
  {
    sim = partial_simulator( ntk.num_pis(), ps.num_patterns, std::rand() );
    tts.reset();
    call_with_stopwatch( st.time_sim, [&]() {
      simulate_nodes<Ntk>( ntk, tts, sim, true );
    } );
  }

  template<typename Fn>
  void foreach_transitive_fanin( node const& n, Fn&& fn )
  {
    ntk.incr_trav_id();
    ntk.set_visited( n, ntk.trav_id() );

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      return foreach_transitive_fanin_rec( ntk.get_node( f ), fn );
    } );
  }

  template<typename Fn>
  bool foreach_transitive_fanin_rec( node const& n, Fn&& fn )
  {
    ntk.set_visited( n, ntk.trav_id() );
    if ( !fn( n ) )
    {
      return false;
    }
    bool continue_loop = true;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.visited( ntk.get_node( f ) ) == ntk.trav_id() )
      {
        return true;
      } /* skip visited node, continue looping. */

      continue_loop = foreach_transitive_fanin_rec( ntk.get_node( f ), fn );
      return continue_loop; /* break `foreach_fanin` loop immediately when receiving `false`. */
    } );
    return continue_loop; /* return `false` only if `false` has ever been received from recursive calls. */
  }

private:
  Ntk& ntk;
  functional_reduction_params const& ps;
  functional_reduction_stats& st;

  TT tts;
  partial_simulator sim;
  validator_t validator;

  uint32_t candidates{ 0 };
}; /* functional_reduction_impl */

} /* namespace detail */

/*! \brief Functional reduction.
 *
 * Removes constant nodes and substitute functionally equivalent nodes.
 */
template<class Ntk>
void functional_reduction( Ntk& ntk, functional_reduction_params const& ps = {}, functional_reduction_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
  static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );

  validator_params vps;
  vps.max_clauses = ps.max_clauses;
  vps.conflict_limit = ps.conflict_limit;

  using fanout_view_t = fanout_view<Ntk>;
  fanout_view_t fanout_view{ ntk };

  functional_reduction_stats st;
  detail::functional_reduction_impl p( fanout_view, ps, vps, st );
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

} /* namespace mockturtle */