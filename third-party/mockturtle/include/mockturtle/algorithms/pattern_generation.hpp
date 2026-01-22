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
  \file pattern_generation.hpp
  \brief Expressive Simulation Pattern Generation

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/aig.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "circuit_validator.hpp"
#include "dont_cares.hpp"
#include "simulation.hpp"
#include <bill/sat/interface/abc_bsat2.hpp>
#include <bill/sat/interface/z3.hpp>
#include <kitty/partial_truth_table.hpp>
#include <random>

namespace mockturtle
{

struct pattern_generation_params
{
  /*! \brief Number of patterns each node should have for both values.
   *
   * When this parameter is set to greater than 1, and if the network has more
   * than 2048 PIs, the `BUFFER_SIZE` in `lib/bill/sat/interface/abc_bsat2.hpp`
   * has to be increased to at least `ntk.num_pis()`.
   */
  uint32_t num_stuck_at{ 1 };

  /*! \brief Whether to consider observability, and how many levels. 0 = no. -1 = Consider TFO until PO. */
  int32_t odc_levels{ 0 };

  /*! \brief Show progress. */
  bool progress{ false };

  /*! \brief Be verbose. Note that it will take more time to do extra ODC computation if this is turned on. */
  bool verbose{ false };

  /*! \brief Random seed. */
  std::default_random_engine::result_type random_seed{ 1 };

  /*! \brief Conflict limit of the SAT solver. */
  uint32_t conflict_limit{ 1000 };

  /*! \brief Maximum number of clauses of the SAT solver. (incremental CNF construction) */
  uint32_t max_clauses{ 1000 };
};

struct pattern_generation_stats
{
  /*! \brief Total time. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Time for simulation. */
  stopwatch<>::duration time_sim{ 0 };

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_sat{ 0 };

  /*! \brief Time for ODC computation */
  stopwatch<>::duration time_odc{ 0 };

  /*! \brief Number of constant nodes. */
  uint32_t num_constant{ 0 };

  /*! \brief Number of generated patterns. */
  uint32_t num_generated_patterns{ 0 };

  /*! \brief Number of stuck-at patterns that is re-generated because the original one was unobservable. */
  uint32_t unobservable_type1{ 0 };

  /*! \brief Number of additional patterns generated because the node was unobservable with one value. */
  uint32_t unobservable_type2{ 0 };

  /*! \brief Number of unobservable nodes (node for which an observable pattern can not be found). */
  uint32_t unobservable_node{ 0 };
};

namespace detail
{

template<class Ntk, class Simulator, bool use_odc = false, bool substitute_const = false>
class patgen_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using TT = incomplete_node_map<kitty::partial_truth_table, Ntk>;

  explicit patgen_impl( Ntk& ntk, Simulator& sim, pattern_generation_params const& ps, validator_params& vps, pattern_generation_stats& st )
      : ntk( ntk ), ps( ps ), st( st ), vps( vps ), validator( ntk, vps ),
        tts( ntk ), sim( sim )
  {
  }

  void run()
  {
    stopwatch t( st.time_total );

    if constexpr ( has_EXCDC_interface_v<Ntk> )
    {
      sim.remove_CDC_patterns( ntk );
    }

    call_with_stopwatch( st.time_sim, [&]() {
      simulate_nodes<Ntk>( ntk, tts, sim, true );
    } );

    if ( ps.num_stuck_at > 0 )
    {
      stuck_at_check();
      if constexpr ( std::is_same_v<Simulator, bit_packed_simulator> )
      {
        sim.pack_bits();
        call_with_stopwatch( st.time_sim, [&]() {
          tts.reset();
          simulate_nodes<Ntk>( ntk, tts, sim, true );
        } );
      }
      if constexpr ( substitute_const )
      {
        for ( auto n : const_nodes )
        {
          if ( !ntk.is_dead( ntk.get_node( n ) ) )
          {
            ntk.substitute_node( ntk.get_node( n ), ntk.get_constant( ntk.is_complemented( n ) ) );
          }
        }
      }
    }

    if constexpr ( use_odc )
    {
      observability_check();
      if constexpr ( std::is_same_v<Simulator, bit_packed_simulator> )
      {
        sim.pack_bits();
        call_with_stopwatch( st.time_sim, [&]() {
          tts.reset();
          simulate_nodes<Ntk>( ntk, tts, sim, true );
        } );
      }
    }

    if constexpr ( std::is_same_v<Simulator, bit_packed_simulator> )
    {
      sim.randomize_dont_care_bits( ps.random_seed );
      if constexpr ( has_EXCDC_interface_v<Ntk> )
      {
        sim.remove_CDC_patterns( ntk );
      }
    }
  }

private:
  void stuck_at_check()
  {
    progress_bar pbar{ ntk.size(), "patgen-sa |{0}| node = {1:>4} #pat = {2:>4}", ps.progress };

    kitty::partial_truth_table zero = sim.compute_constant( false );

    ntk.foreach_gate( [&]( auto const& n, auto i ) {
      pbar( i, i, sim.num_bits() );

      if ( tts[n].num_bits() != sim.num_bits() )
      {
        call_with_stopwatch( st.time_sim, [&]() {
          simulate_node<Ntk>( ntk, n, tts, sim );
        } );
      }
      assert( zero.num_bits() == sim.num_bits() );

      if ( ( tts[n] == zero ) || ( tts[n] == ~zero ) )
      {
        bool value = ( tts[n] == zero ); /* wanted value of n */

        const auto res = call_with_stopwatch( st.time_sat, [&]() {
          validator.set_odc_levels( 0 );
          return validator.validate( n, !value );
        } );
        if ( !res )
        {
          return true; /* timeout, next node */
        }
        else if ( !( *res ) ) /* SAT, pattern found */
        {
          if constexpr ( use_odc )
          {
            /* check if the found pattern is observable */
            bool observable = call_with_stopwatch( st.time_odc, [&]() {
              return pattern_is_observable( ntk, n, validator.cex, ps.odc_levels );
            } );
            if ( !observable )
            {
              if ( ps.verbose )
              {
                std::cout << "\t[i] generated pattern is not observable (type 1). node: " << n << ", with value " << value << "\n";
              }

              const auto res2 = call_with_stopwatch( st.time_sat, [&]() {
                validator.set_odc_levels( ps.odc_levels );
                return validator.validate( n, !value );
              } );
              if ( res2 )
              {
                if ( !( *res2 ) )
                {
                  ++st.unobservable_type1;
                  if ( ps.verbose )
                  {
                    assert( pattern_is_observable( ntk, n, validator.cex, ps.odc_levels ) );
                    std::cout << "\t\t[i] unobservable pattern resolved.\n";
                  }
                }
                else
                {
                  ++st.unobservable_node;
                  if ( ps.verbose )
                  {
                    std::cout << "\t\t[i] unobservable node " << n << "\n";
                  }
                }
              }
            }
          }

          new_pattern( validator.cex, n );

          if ( ps.num_stuck_at > 1 )
          {
            auto generated = call_with_stopwatch( st.time_sat, [&]() {
              validator.set_odc_levels( ps.odc_levels );
              return validator.generate_pattern( n, value, { validator.cex }, ps.num_stuck_at - 1 );
            } );
            for ( auto& pattern : generated )
            {
              new_pattern( pattern, n );
            }
          }

          zero = sim.compute_constant( false );
        }
        else /* UNSAT, constant node */
        {
          ++st.num_constant;
          const_nodes.emplace_back( value ? ntk.make_signal( n ) : !ntk.make_signal( n ) );
          return true; /* next gate */
        }
      }
      else if ( ps.num_stuck_at > 1 )
      {
        auto const& tt = tts[n];
        if ( kitty::count_ones( tt ) < ps.num_stuck_at )
        {
          generate_more_patterns( n, tt, true, zero );
        }
        else if ( kitty::count_zeros( tt ) < ps.num_stuck_at )
        {
          generate_more_patterns( n, tt, false, zero );
        }
      }
      return true; /* next gate */
    } );
  }

  void observability_check()
  {
    progress_bar pbar{ ntk.size(), "patgen-obs |{0}| node = {1:>4} #pat = {2:>4}", ps.progress };

    kitty::partial_truth_table zero = sim.compute_constant( false );

    ntk.foreach_gate( [&]( auto const& n, auto i ) {
      pbar( i, i, sim.num_bits() );

      for ( auto& f : const_nodes )
      {
        if ( ntk.get_node( f ) == n )
        {
          return true; /* skip constant nodes */
        }
      }

      if ( tts[n].num_bits() != sim.num_bits() )
      {
        call_with_stopwatch( st.time_sim, [&]() {
          simulate_node<Ntk>( ntk, n, tts, sim );
        } );
      }
      assert( zero.num_bits() == sim.num_bits() );

      /* compute ODC */
      auto odc = call_with_stopwatch( st.time_odc, [&]() {
        return observability_dont_cares<Ntk>( ntk, n, sim, tts, ps.odc_levels );
      } );

      /* check if under non-ODCs n is always the same value */
      if ( ( tts[n] & ~odc ) == zero )
      {
        if ( ps.verbose )
        {
          std::cout << "\t[i] under all observable patterns, node " << n << " is always 0 (type 2).\n";
        }

        const auto res = call_with_stopwatch( st.time_sat, [&]() {
          validator.set_odc_levels( ps.odc_levels );
          return validator.validate( n, false );
        } );
        if ( res )
        {
          if ( !( *res ) )
          {
            new_pattern( validator.cex, n );
            ++st.unobservable_type2;

            if ( ps.verbose )
            {
              auto odc2 = call_with_stopwatch( st.time_odc, [&]() { return observability_dont_cares<Ntk>( ntk, n, sim, tts, ps.odc_levels ); } );
              assert( ( tts[n] & ~odc2 ) != sim.compute_constant( false ) );
              std::cout << "\t\t[i] added generated pattern to resolve unobservability.\n";
            }

            zero = sim.compute_constant( false );
          }
          else
          {
            ++st.unobservable_node;
            if ( ps.verbose )
            {
              std::cout << "\t\t[i] unobservable node " << n << "\n";
            }
          }
        }
      }
      else if ( ( tts[n] | odc ) == ~zero )
      {
        if ( ps.verbose )
        {
          std::cout << "\t[i] under all observable patterns, node " << n << " is always 1 (type 2).\n";
        }

        const auto res = call_with_stopwatch( st.time_sat, [&]() {
          validator.set_odc_levels( ps.odc_levels );
          return validator.validate( n, true );
        } );
        if ( res )
        {
          if ( !( *res ) )
          {
            new_pattern( validator.cex, n );
            ++st.unobservable_type2;

            if ( ps.verbose )
            {
              auto odc2 = call_with_stopwatch( st.time_odc, [&]() { return observability_dont_cares<Ntk>( ntk, n, sim, tts, ps.odc_levels ); } );
              assert( ( tts[n] | odc2 ) != sim.compute_constant( true ) );
              std::cout << "\t\t[i] added generated pattern to resolve unobservability.\n";
            }

            zero = sim.compute_constant( false );
          }
          else
          {
            ++st.unobservable_node;
            if ( ps.verbose )
            {
              std::cout << "\t\t[i] unobservable node " << n << "\n";
            }
          }
        }
      }

      return true; /* next gate */
    } );
  }

private:
  void new_pattern( std::vector<bool> const& pattern, node const& n )
  {
    if constexpr ( std::is_same_v<Simulator, bit_packed_simulator> )
    {
      sim.add_pattern( pattern, compute_support( n ) );
    }
    else
    {
      (void)n;
      sim.add_pattern( pattern );
    }

    if constexpr ( has_EXCDC_interface_v<Ntk> )
    {
      assert( !ntk.pattern_is_EXCDC( pattern ) );
    }
    ++st.num_generated_patterns;

    /* re-simulate */
    if ( sim.num_bits() % 64 == 0 )
    {
      call_with_stopwatch( st.time_sim, [&]() {
        simulate_nodes<Ntk>( ntk, tts, sim, false );
      } );
    }
  }

  void generate_more_patterns( node const& n, kitty::partial_truth_table const& tt, bool value, kitty::partial_truth_table& zero )
  {
    /* collect the `value` patterns */
    std::vector<std::vector<bool>> patterns;
    for ( auto i = 0u; i < tt.num_bits(); ++i )
    {
      if ( kitty::get_bit( tt, i ) == value )
      {
        patterns.emplace_back();
        ntk.foreach_pi( [&]( auto const& pi ) {
          patterns.back().emplace_back( kitty::get_bit( tts[pi], i ) );
        } );
      }
    }

    auto generated = call_with_stopwatch( st.time_sat, [&]() {
      validator.set_odc_levels( ps.odc_levels );
      return validator.generate_pattern( n, value, patterns, ps.num_stuck_at - patterns.size() );
    } );
    for ( auto& pattern : generated )
    {
      new_pattern( pattern, n );
    }
    zero = sim.compute_constant( false );
  }

  std::vector<bool> compute_support( node const& n )
  {
    ntk.incr_trav_id();
    if constexpr ( use_odc )
    {
      if ( ps.odc_levels != 0 )
      {
        std::vector<node> leaves;
        mark_fanout_leaves_rec( n, 1, leaves );
        ntk.foreach_po( [&]( auto const& f ) {
          if ( ntk.visited( ntk.get_node( f ) ) == ntk.trav_id() )
          {
            leaves.emplace_back( ntk.get_node( f ) );
          }
        } );

        ntk.incr_trav_id();
        for ( auto& l : leaves )
        {
          mark_support_rec( l );
        }
      }
    }
    mark_support_rec( n );

    std::vector<bool> care( ntk.num_pis(), false );
    ntk.foreach_pi( [&]( auto const& f, uint32_t i ) {
      if ( ntk.visited( f ) == ntk.trav_id() )
      {
        care[i] = true;
      }
    } );
    return care;
  }

  void mark_support_rec( node const& n )
  {
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      return;
    }
    ntk.set_visited( n, ntk.trav_id() );

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( ntk.visited( ntk.get_node( f ) ) == ntk.trav_id() )
      {
        return true;
      }
      mark_support_rec( ntk.get_node( f ) );
      return true;
    } );
  }

  void mark_fanout_leaves_rec( node const& n, int32_t level, std::vector<node>& leaves )
  {
    ntk.foreach_fanout( n, [&]( auto const& fo ) {
      if ( ntk.visited( fo ) == ntk.trav_id() )
      {
        return true;
      }
      ntk.set_visited( fo, ntk.trav_id() );

      if ( level == ps.odc_levels )
      {
        leaves.emplace_back( fo );
        return true;
      }

      mark_fanout_leaves_rec( fo, level + 1, leaves );
      return true;
    } );
  }

private:
  Ntk& ntk;

  pattern_generation_params const& ps;
  pattern_generation_stats& st;

  validator_params& vps;
  circuit_validator<Ntk, bill::solvers::bsat2, true, true, use_odc> validator;

  TT tts;
  std::vector<signal> const_nodes;

  Simulator& sim;
};

} /* namespace detail */

/*! \brief Expressive simulation pattern generation.
 *
 * This function implements two simulation pattern generation methods:
 * stuck-at value checking and observability checking. Please refer to
 * [1] for details of the algorithm and its purpose.
 *
 * [1] Simulation-Guided Boolean Resubstitution. IWLS 2020 (arXiv:2007.02579).
 *
 * \param sim Reference of a `partial_simulator` or `bit_packed_simulator`
 * object where the generated patterns will be stored.
 * It can be empty (`Simulator( ntk.num_pis(), 0 )`)
 * or already containing some patterns generated from previous runs
 * (`Simulator( filename )`) or randomly generated
 * (`Simulator( ntk.num_pis(), num_random_patterns )`). The generated
 * patterns can then be written out with `write_patterns`
 * or directly be used by passing the simulator to another algorithm.
 */
template<bool substitute_const = false, class Ntk, class Simulator>
void pattern_generation( Ntk& ntk, Simulator& sim, pattern_generation_params const& ps = {}, pattern_generation_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
  static_assert( std::is_same_v<Simulator, partial_simulator> || std::is_same_v<Simulator, bit_packed_simulator>, "Simulator should be either partial_simulator or bit_packed_simulator" );

  pattern_generation_stats st;
  validator_params vps;
  vps.conflict_limit = ps.conflict_limit;
  vps.max_clauses = ps.max_clauses;
  vps.random_seed = ps.random_seed;

  if ( ps.odc_levels != 0 )
  {
    using fanout_view_t = fanout_view<Ntk>;
    fanout_view_t fanout_view{ ntk };

    detail::patgen_impl<fanout_view_t, Simulator, /*use_odc*/ true, substitute_const> p( fanout_view, sim, ps, vps, st );
    p.run();
  }
  else
  {
    detail::patgen_impl<Ntk, Simulator, /*use_odc*/ false, substitute_const> p( ntk, sim, ps, vps, st );
    p.run();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */