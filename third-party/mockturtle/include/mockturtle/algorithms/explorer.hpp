/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2023  EPFL
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
  \file explorer.hpp
  \brief Implements the design space explorer engine

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "lut_mapper.hpp"
#include "collapse_mapped.hpp"
#include "klut_to_graph.hpp"
#include "cut_rewriting.hpp"
#include "refactoring.hpp"
#include "mig_algebraic_rewriting.hpp"
#include "mapper.hpp"
#include "rewrite.hpp"
#include "node_resynthesis/mig_npn.hpp"
#include "node_resynthesis/sop_factoring.hpp"
#include "resubstitution.hpp"
#include "aig_resub.hpp"
#include "mig_resub.hpp"
#include "sim_resub.hpp"
#include "cleanup.hpp"
#include "balancing.hpp"
#include "balancing/sop_balancing.hpp"
#include "aig_balancing.hpp"
#include "miter.hpp"
#include "equivalence_checking.hpp"
#include "aqfp/buffer_insertion.hpp"
#include "../networks/klut.hpp"
#include "../networks/mig.hpp"
#include "../views/mapping_view.hpp"
#include "../io/write_verilog.hpp"
#include "../io/write_aiger.hpp"
#include "../io/verilog_reader.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/abc.hpp"

#include <random>

#define explorer_debug 0

namespace mockturtle
{

struct explorer_params
{
  /*! \brief Number of iterations to run with different random seed, restarting from the original
   * network (including the first iteration). */
  uint32_t num_restarts{1u};

  /*! \brief Initial random seed used to generate random seeds randomly. */
  uint32_t random_seed{0u};

  /*! \brief Maximum number of steps in each iteration. */
  uint32_t max_steps{100000u};

  /*! \brief Maximum number of steps without improvement in each iteration. */
  uint32_t max_steps_no_impr{1000000u};

  /*! \brief Number of compressing scripts to run per step. */
  uint32_t compressing_scripts_per_step{3u};

  /*! \brief Timeout per iteration in seconds. */
  uint32_t timeout{30u};

  /*! \brief Be verbose. */
  bool verbose{false};

  /*! \brief Be very verbose. */
  bool very_verbose{false};
};

struct explorer_stats
{
  stopwatch<>::duration time_total{0};
  stopwatch<>::duration time_evaluate{0};
};

template<class Ntk>
using script_t = std::function<void( Ntk&, uint32_t, uint32_t )>;

template<class Ntk>
using cost_fn_t = std::function<uint32_t( Ntk const& )>;

template<class Ntk>
std::function<uint32_t( Ntk const& )> size_cost_fn = []( Ntk const& ntk ){ return ntk.num_gates(); };

template<class Ntk>
class explorer
{
public:
  using RandEngine = std::default_random_engine;

  explorer( explorer_params const& ps, explorer_stats& st, cost_fn_t<Ntk> const& cost_fn = size_cost_fn<Ntk> )
      : _ps( ps ), _st( st ), cost( cost_fn )
  {
  }

  void add_decompressing_script( script_t<Ntk> const& algo, float weight = 1.0 )
  {
    decompressing_scripts.emplace_back( std::make_pair( algo, total_weights_dec ) );
    total_weights_dec += weight;
  }

  void add_compressing_script( script_t<Ntk> const& algo, float weight = 1.0 )
  {
    compressing_scripts.emplace_back( std::make_pair( algo, total_weights_com ) );
    total_weights_com += weight;
  }

  Ntk run( Ntk const& ntk )
  {
    stopwatch t( _st.time_total );
    if ( decompressing_scripts.size() == 0 )
    {
      std::cerr << "[e] No decompressing script provided.\n";
      return ntk;
    }
    if ( compressing_scripts.size() == 0 )
    {
      std::cerr << "[e] No compressing script provided.\n";
      return ntk;
    }

    RandEngine rnd( _ps.random_seed );
    auto init_cost = call_with_stopwatch( _st.time_evaluate, [&](){ return cost( ntk ); } );
    Ntk best = ntk.clone();
    auto best_cost = init_cost;
    for ( auto i = 0u; i < _ps.num_restarts; ++i )
    {
      Ntk current = ntk.clone();
      auto new_cost = run_one_iteration( current, rnd(), init_cost );
      if ( new_cost < best_cost )
      {
        best = current.clone();
        best_cost = new_cost;
      }
      if ( _ps.verbose )
        fmt::print( "[i] best cost in restart {}: {}, overall best cost: {}\n", i, new_cost, best_cost );
    }
    return best;
  }

private:
  uint32_t run_one_iteration( Ntk& ntk, uint32_t seed, uint32_t init_cost )
  {
    if ( _ps.verbose )
    {
      fmt::print( "\n[i] new restart using seed {}, original cost = {}\n", seed, init_cost );
    }
 
    stopwatch<>::duration elapsed_time{0};
    RandEngine rnd( seed );
    Ntk best = ntk.clone();
    auto best_cost = init_cost;
    uint32_t last_update{0u};
    for ( auto i = 0u; i < _ps.max_steps; ++i )
    {
    #if explorer_debug
      Ntk backup = ntk.clone();
    #endif

      {
        stopwatch t( elapsed_time );
        decompress( ntk, rnd, i );
        compress( ntk, rnd, i );
      }
      auto new_cost = call_with_stopwatch( _st.time_evaluate, [&](){ return cost( ntk ); } );
      if ( _ps.very_verbose )
        fmt::print( "[i] after step {}, cost = {}\n", i, new_cost );

    #if explorer_debug
      if ( !*equivalence_checking( *miter<Ntk>( ntk, best ) ) )
      {
        write_verilog( backup, "debug.v" );
        write_verilog( ntk, "wrong.v" );
        fmt::print( "NEQ at step {}!\n", i );
        break;
      }
    #endif

      if ( new_cost < best_cost )
      {
        best = ntk.clone();
        best_cost = new_cost;
        last_update = i;
        if ( _ps.verbose )
        {
          fmt::print( "[i] updated new best at step {}: {}\n", i, best_cost );
        }
      }
      if ( i - last_update >= _ps.max_steps_no_impr )
      {
        if ( _ps.verbose )
          fmt::print( "[i] break restart at step {} after {} steps without improvement (elapsed time: {} secs)\n", i, _ps.max_steps_no_impr, to_seconds( elapsed_time ) );
        break;
      }
      if ( to_seconds( elapsed_time ) >= _ps.timeout )
      {
        if ( _ps.verbose )
          fmt::print( "[i] break restart at step {} after timeout of {} secs\n", i, to_seconds( elapsed_time ) );
        break;
      }
    }
    std::cout << std::flush;
    ntk = best;
    return best_cost;
  }

  void decompress( Ntk& ntk, RandEngine& rnd, uint32_t i )
  {
    std::uniform_real_distribution<> dis( 0.0, total_weights_dec );
    float r = dis( rnd );
    for ( auto it = decompressing_scripts.rbegin(); it != decompressing_scripts.rend(); ++it )
    {
      if ( r >= it->second )
      {
        it->first( ntk, i, rnd() );
        break;
      }
    }
  }

  void compress( Ntk& ntk, RandEngine& rnd, uint32_t i )
  {
    std::uniform_real_distribution<> dis( 0.0, total_weights_com );
    for ( auto j = 0u; j < _ps.compressing_scripts_per_step; ++j )
    {
      float r = dis( rnd );
      for ( auto it = compressing_scripts.rbegin(); it != compressing_scripts.rend(); ++it )
      {
        if ( r >= it->second )
        {
          it->first( ntk, i, rnd() );
          break;
        }
      }
    }
  }

private:
  const explorer_params _ps;
  explorer_stats& _st;

  std::vector<std::pair<script_t<Ntk>, float>> decompressing_scripts;
  float total_weights_dec{0.0};
  std::vector<std::pair<script_t<Ntk>, float>> compressing_scripts;
  float total_weights_com{0.0};

  cost_fn_t<Ntk> cost;
};

mig_network explore_mig( mig_network const& ntk, explorer_params const ps = {} )
{
  using Ntk = mig_network;

  explorer_stats st;
  explorer<Ntk> expl( ps, st );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with k-LUT mapping using random value {}, k = {}\n", rand, 2 + (rand % 5) );
    lut_map_params mps;
    mps.cut_enumeration_ps.cut_size = 3 + (rand & 0x3); //3 + (i % 4);
    mapping_view<Ntk> mapped{ _ntk };
    lut_map( mapped, mps );
    const auto klut = *collapse_mapped_network<klut_network>( mapped );
    
    if ( (rand >> 2) & 0x1 )
    {
      _ntk = convert_klut_to_graph<Ntk>( klut );
    }
    else
    {
      sop_factoring<Ntk> resyn;
      _ntk = node_resynthesis<Ntk>( klut, resyn );
    }
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with break-MAJ using random value {}\n", rand );
    std::mt19937 g( rand );
    _ntk.foreach_gate( [&]( auto n ){
      bool is_maj = true;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        if ( _ntk.is_constant( _ntk.get_node( fi ) ) )
          is_maj = false;
        return;
      });
      if ( !is_maj )
        return;
      std::vector<typename Ntk::signal> fanins;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        fanins.emplace_back( fi );
      });

      std::shuffle( fanins.begin(), fanins.end(), g );
      _ntk.substitute_node( n, _ntk.create_or( _ntk.create_and( fanins[0], fanins[1] ), _ntk.create_and( fanins[2], !_ntk.create_and( !fanins[0], !fanins[1] ) ) ) );
    });
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    mig_npn_resynthesis resyn{ true };
    exact_library<mig_network> exact_lib( resyn );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; /* high-effort remap */
    _ntk = map( _ntk, exact_lib, mps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    resubstitution_params rps;
    rps.max_inserts = rand & 0x7;
    rps.max_pis = (rand >> 3) & 0x1 ? 6 : 8;
    depth_view depth_mig{ _ntk };
    fanout_view fanout_mig{ depth_mig };
    mig_resubstitution2( fanout_mig, rps );
    _ntk = cleanup_dangling( _ntk );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    sop_rebalancing<mig_network> balance_fn;
    balancing_params bps;
    bps.cut_enumeration_ps.cut_size = 6u;
    _ntk = balancing( _ntk, {balance_fn}, bps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    depth_view depth_mig{ _ntk };
    mig_algebraic_depth_rewriting( depth_mig );
    _ntk = cleanup_dangling( _ntk );
  } );

  return expl.run( ntk );
}

#ifdef ENABLE_ABC
mig_network deepsyn_mig_v1( mig_network const& ntk, explorer_params const ps = {} )
{
  using Ntk = mig_network;

  explorer_stats st;
  explorer<Ntk> expl( ps, st );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with &if using random value {}\n", rand );
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );

    std::string script = fmt::format(
      "&dch{}; &if -a -K {}; &mfs -e -W 20 -L 20; &st",
      (rand & 0x1) ? " -f" : "",
      2 + (i % 5));
    aig = call_abc_script( aig, script );

    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with k-LUT mapping using random value {}, k = {}\n", rand, 2 + (rand % 5) );
    lut_map_params mps;
    mps.cut_enumeration_ps.cut_size = 3 + (rand & 0x3); //3 + (i % 4);
    klut_network klut = lut_map( _ntk, mps );
    
    if ( (rand >> 2) & 0x1 )
    {
      _ntk = convert_klut_to_graph<Ntk>( klut );
    }
    else
    {
      sop_factoring<Ntk> resyn;
      _ntk = node_resynthesis<Ntk>( klut, resyn );
    }
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with break-MAJ using random value {}\n", rand );
    std::mt19937 g( rand );
    _ntk.foreach_gate( [&]( auto n ){
      bool is_maj = true;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        if ( _ntk.is_constant( _ntk.get_node( fi ) ) )
          is_maj = false;
        return;
      });
      if ( !is_maj )
        return;
      std::vector<typename Ntk::signal> fanins;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        fanins.emplace_back( fi );
      });

      std::shuffle( fanins.begin(), fanins.end(), g );
      _ntk.substitute_node( n, _ntk.create_or( _ntk.create_and( fanins[0], fanins[1] ), _ntk.create_and( fanins[2], !_ntk.create_and( !fanins[0], !fanins[1] ) ) ) );
    });
    _ntk = cleanup_dangling( _ntk );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "compressing with resyn2rs using random value {}\n", rand );
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );
    //std::string script = (rand & 0x1) ? "; &c2rs" : "; &dc2";
    std::string script = "&put; resyn2rs; &get";

    aig = call_abc_script( aig, script );
    
    mig_npn_resynthesis resyn{ true };
    exact_library_params eps;
    eps.np_classification = false;
    eps.compute_dc_classes = rand & 0x2;
    exact_library<mig_network> exact_lib( resyn, eps );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; // high-effort remap
    mps.use_dont_cares = rand & 0x2;
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "compressing with remapping using random value {}\n", rand );
    mig_npn_resynthesis resyn{ true };
    exact_library_params eps;
    eps.np_classification = false;
    eps.compute_dc_classes = rand & 0x2;
    exact_library<mig_network> exact_lib( resyn, eps );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; // high-effort remap
    mps.use_dont_cares = rand & 0x2;
    _ntk = map( _ntk, exact_lib, mps );
  } );
  
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "compressing with rewriting using random value {}\n", rand );
    mig_npn_resynthesis resyn{ true };
    exact_library_params eps;
    eps.np_classification = false;
    eps.compute_dc_classes = rand & 0x1;
    exact_library<mig_network> exact_lib( resyn, eps );
    rewrite_params rps;
    rps.use_dont_cares = rand & 0x1;
    rewrite( _ntk, exact_lib, rps );
    _ntk = cleanup_dangling( _ntk );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "compressing with resub using random value {}\n", rand );
    resubstitution_params rps;
    rps.max_inserts = (rand >> 1) & 0x7;
    rps.max_pis = (rand >> 4) & 0x3 ? 6 : 8;
    depth_view depth_mig{ _ntk };
    fanout_view fanout_mig{ depth_mig };
    mig_resubstitution2( fanout_mig, rps );
    _ntk = cleanup_dangling( _ntk );
  } );

  return expl.run( ntk );
}

mig_network deepsyn_mig_v2( mig_network const& ntk, explorer_params const ps = {} )
{
  using Ntk = mig_network;

  explorer_stats st;
  explorer<Ntk> expl( ps, st );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with &if using random value {}\n", rand );
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );

    std::string script = fmt::format(
      "&dch{}; &if -a -K {}; &mfs -e -W 20 -L 20; &st",
      (rand & 0x1) ? " -f" : "",
      2 + (i % 5));
    aig = call_abc_script( aig, script );

    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with k-LUT mapping using random value {}, k = {}\n", rand, 2 + (rand % 5) );
    lut_map_params mps;
    mps.cut_enumeration_ps.cut_size = 3 + (rand & 0x3); //3 + (i % 4);
    klut_network klut = lut_map( _ntk, mps );
    
    if ( (rand >> 2) & 0x1 )
    {
      _ntk = convert_klut_to_graph<Ntk>( klut );
    }
    else
    {
      sop_factoring<Ntk> resyn;
      _ntk = node_resynthesis<Ntk>( klut, resyn );
    }
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with break-MAJ using random value {}\n", rand );
    std::mt19937 g( rand );
    _ntk.foreach_gate( [&]( auto n ){
      bool is_maj = true;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        if ( _ntk.is_constant( _ntk.get_node( fi ) ) )
          is_maj = false;
        return;
      });
      if ( !is_maj )
        return;
      std::vector<typename Ntk::signal> fanins;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        fanins.emplace_back( fi );
      });

      std::shuffle( fanins.begin(), fanins.end(), g );
      _ntk.substitute_node( n, _ntk.create_or( _ntk.create_and( fanins[0], fanins[1] ), _ntk.create_and( fanins[2], !_ntk.create_and( !fanins[0], !fanins[1] ) ) ) );
    });
    _ntk = cleanup_dangling( _ntk );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "compressing with resyn2rs using random value {}\n", rand );
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );
    //std::string script = (rand & 0x1) ? "; &c2rs" : "; &dc2";
    std::string script = "&put; resyn2rs; &get";

    aig = call_abc_script( aig, script );
    
    mig_npn_resynthesis resyn{ true };
    exact_library_params eps;
    eps.np_classification = false;
    eps.compute_dc_classes = rand & 0x2;
    exact_library<mig_network> exact_lib( resyn, eps );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; // high-effort remap
    mps.use_dont_cares = rand & 0x2;
    _ntk = map( aig, exact_lib, mps );
  } );
  
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "compressing with Ale flow using random value {}\n", rand );
    //_ntk = cleanup_dangling( _ntk );

    mig_npn_resynthesis resyn{ true };
    exact_library_params eps;
    eps.np_classification = false;
    eps.compute_dc_classes = true;
    exact_library<mig_network> exact_lib( resyn, eps );
    
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = true; // high-effort remap
    
    rewrite_params rps;

    resubstitution_params rsps;
    rsps.max_inserts = 20;
    rsps.max_pis = 8;
    
    mps.use_dont_cares = rand & 0x8;
    _ntk = map( _ntk, exact_lib, mps );
    mps.use_dont_cares = rand & 0xf;
    _ntk = map( _ntk, exact_lib, mps );
    mps.use_dont_cares = rand & 0x10;
    _ntk = map( _ntk, exact_lib, mps );

    rps.use_dont_cares = rand & 0x1;
    rewrite( _ntk, exact_lib, rps );
    _ntk = cleanup_dangling( _ntk );
    rps.use_dont_cares = rand & 0x2;
    rewrite( _ntk, exact_lib, rps );
    _ntk = cleanup_dangling( _ntk );
    rps.use_dont_cares = rand & 0x4;
    rewrite( _ntk, exact_lib, rps );
    _ntk = cleanup_dangling( _ntk );

    depth_view depth_mig{ _ntk };
    fanout_view fanout_mig{ depth_mig };
    mig_resubstitution2( fanout_mig, rsps );
    _ntk = cleanup_dangling( _ntk );
  } );

  return expl.run( ntk );
}

mig_network deepsyn_mig_depth( mig_network const& ntk, explorer_params const ps = {} )
{
  using Ntk = mig_network;

  cost_fn_t<Ntk> depth_cost = []( Ntk const& _ntk ){
    depth_view d{ _ntk };
    return d.depth();
  };

  explorer_stats st;
  explorer<Ntk> expl( ps, st, depth_cost );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );

    std::string script = fmt::format(
      "&dch{} -m; &if -K {}; &mfs -e -W 20 {}",
      (rand & 0x1) ? " -f" : "",
      2 + (i % 5),
      ((rand >> 2) & 0x1) ? "; &fx; &st" : "");
    aig = call_abc_script( aig, script );

    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = false;
    mps.required_time = std::numeric_limits<double>::max();
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with break-MAJ using random value {}\n", rand );
    std::mt19937 g( rand );
    _ntk.foreach_gate( [&]( auto n ){
      bool is_maj = true;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        if ( _ntk.is_constant( _ntk.get_node( fi ) ) )
          is_maj = false;
        return;
      });
      if ( !is_maj )
        return;
      std::vector<typename Ntk::signal> fanins;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        fanins.emplace_back( fi );
      });

      std::shuffle( fanins.begin(), fanins.end(), g );
      _ntk.substitute_node( n, _ntk.create_or( _ntk.create_and( fanins[0], fanins[1] ), _ntk.create_and( fanins[2], !_ntk.create_and( !fanins[0], !fanins[1] ) ) ) );
    });
    _ntk = cleanup_dangling( _ntk );
  }, 0.3 );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );
    std::string script = "&put; resyn2rs; &get";
    aig = call_abc_script( aig, script );

    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = false;
    mps.required_time = std::numeric_limits<double>::max();
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    mig_npn_resynthesis resyn{ true };
    exact_library<mig_network> exact_lib( resyn );
    map_params mps;
    mps.skip_delay_round = false;
    //mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; /* high-effort remap */
    _ntk = map( _ntk, exact_lib, mps );
  }, 0.5 );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    resubstitution_params rps;
    rps.max_inserts = rand & 0x7;
    rps.max_pis = (rand >> 3) & 0x1 ? 6 : 8;
    depth_view depth_mig{ _ntk };
    fanout_view fanout_mig{ depth_mig };
    mig_resubstitution2( fanout_mig, rps );
    _ntk = cleanup_dangling( _ntk );
  }, 0.5 );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    sop_rebalancing<mig_network> balance_fn;
    balancing_params bps;
    bps.cut_enumeration_ps.cut_size = ( rand & 0x1 ) ? 8 : 10;
    _ntk = balancing( _ntk, {balance_fn}, bps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    depth_view depth_mig{ _ntk };
    mig_algebraic_depth_rewriting( depth_mig );
    _ntk = cleanup_dangling( _ntk );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    sop_factoring<Ntk> resyn;
    refactoring( _ntk, resyn );
    _ntk = cleanup_dangling( _ntk );
  }, 0.5 );

  return expl.run( ntk );
}

aig_network deepsyn_aig( aig_network const& ntk, explorer_params const ps = {} )
{
  using Ntk = aig_network;

  explorer_stats st;
  explorer<Ntk> expl( ps, st );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    std::string script = fmt::format(
      "&dch{}; &if -a -K {}; &mfs -e -W 20 -L 20{}",
      (rand & 0x1) ? " -f" : "",
      2 + (i % 5),
      ((rand >> 2) & 0x1) ? "; &fx; &st" : "");
    _ntk = call_abc_script( _ntk, script );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    std::string script = (rand & 0x1) ? "; &c2rs" : "; &dc2";
    _ntk = call_abc_script( _ntk, script );
  } );

  return expl.run( ntk );
}

mig_network deepsyn_aqfp( mig_network const& ntk, explorer_params const ps = {}, explorer_stats * pst = nullptr )
{
  using Ntk = mig_network;

  cost_fn_t<Ntk> aqfp_cost = []( Ntk const& _ntk ){
    buffer_insertion_params bps;
    bps.assume.balance_cios = true;
    bps.assume.splitter_capacity = 4;
    bps.assume.ci_phases = {0};
    bps.assume.num_phases = 1;
    bps.scheduling = buffer_insertion_params::better_depth;
    bps.optimization_effort = buffer_insertion_params::one_pass;
    buffer_insertion buf_inst( _ntk, bps );
    auto numbufs = buf_inst.dry_run();

    //return (_ntk.num_gates() * 6 + numbufs * 2);
    return (_ntk.num_gates() * 6 + numbufs * 2) * buf_inst.depth();
  };

  explorer_stats st;
  explorer<Ntk> expl( ps, st, aqfp_cost );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );

    std::string script = fmt::format(
      "&dch{}; &if -a -K {}; &mfs -e -W 20 -L 20{}",
      (rand & 0x1) ? " -f" : "",
      2 + (i % 5),
      ((rand >> 2) & 0x1) ? "; &fx; &st" : "");
    aig = call_abc_script( aig, script );

    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );
    std::string script = (rand & 0x1) ? "; &c2rs" : "; &dc2";
    aig = call_abc_script( aig, script );

    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    _ntk = map( aig, exact_lib, mps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    mig_npn_resynthesis resyn2{ true };
    exact_library<mig_network> exact_lib( resyn2 );
    map_params mps;
    mps.skip_delay_round = true;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; /* high-effort remap */
    _ntk = map( _ntk, exact_lib, mps );
  } );

  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    resubstitution_params rps;
    rps.max_inserts = rand & 0x3;
    depth_view depth_mig{ _ntk };
    fanout_view fanout_mig{ depth_mig };
    mig_resubstitution2( fanout_mig, rps );
    _ntk = cleanup_dangling( _ntk );
  } );

  // balancing
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    sop_rebalancing<mig_network> balance_fn;
    balancing_params bps;
    bps.cut_enumeration_ps.cut_size = 6u;
    _ntk = balancing( _ntk, {balance_fn}, bps );
  } );

  // algebraic depth optimization
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    depth_view depth_mig{ _ntk };
    mig_algebraic_depth_rewriting( depth_mig );
    _ntk = cleanup_dangling( _ntk );
  } );

  auto res = expl.run( ntk );
  if ( pst )
    *pst = st;
  return res;
}
#endif

void compress2rs_aig( aig_network& aig )
{
  xag_npn_resynthesis<aig_network> resyn;
  cut_rewriting_params cps;
  cps.cut_enumeration_ps.cut_size = 4;
  resubstitution_params rps;
  //sop_rebalancing<aig_network> balance_fn;
  //balancing_params bps;
  //bps.cut_enumeration_ps.cut_size = 6u;
  aig_balancing_params abps;
  refactoring_params fps;
  sop_factoring<aig_network> resyn2;

  /*abps.minimize_levels = true;*/ aig_balance( aig, abps ); // "b -l"
  rps.max_pis = 6; rps.max_inserts = 1; /*rps.preserve_depth = true;*/ aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 6 -l"
  /*cps.preserve_depth = true;*/ aig = cut_rewriting( aig, resyn, cps ); // "rw -l"
  rps.max_pis = 6; rps.max_inserts = 2; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 6 -N 2 -l"
  /*fps.preserve_depth = true;*/ refactoring( aig, resyn2, fps ); aig = cleanup_dangling( aig ); // "rf -l"
  rps.max_pis = 8; rps.max_inserts = 1; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 8 -l"
  aig_balance( aig, abps ); // "b -l"
  rps.max_pis = 8; rps.max_inserts = 2; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 8 -N 2 -l"
  aig = cut_rewriting( aig, resyn, cps ); // "rw -l"
  rps.max_pis = 10; rps.max_inserts = 1; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 10 -l"
  cps.allow_zero_gain = true; aig = cut_rewriting( aig, resyn, cps ); // "rwz -l"
  rps.max_pis = 10; rps.max_inserts = 2; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 10 -N 2 -l"
  aig_balance( aig, abps ); // "b -l"
  rps.max_pis = 12; rps.max_inserts = 1; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 12 -l"
  fps.allow_zero_gain = true; refactoring( aig, resyn2, fps ); aig = cleanup_dangling( aig ); // "rfz -l"
  rps.max_pis = 12; rps.max_inserts = 2; aig_resubstitution( aig, rps ); aig = cleanup_dangling( aig ); // "rs -K 12 -N 2 -l"
  aig = cut_rewriting( aig, resyn, cps ); // "rwz -l"
  aig_balance( aig, abps ); // "b -l"
}

mig_network explore_aqfp( mig_network const& ntk, explorer_params const ps = {} )
{
  using Ntk = mig_network;

  cost_fn_t<Ntk> aqfp_cost = []( Ntk const& _ntk ){
    buffer_insertion_params bps;
    bps.assume.balance_cios = true;
    bps.assume.splitter_capacity = 4;
    bps.assume.ci_phases = {0};
    bps.scheduling = buffer_insertion_params::better_depth;
    bps.optimization_effort = buffer_insertion_params::none;
    buffer_insertion buf_inst( _ntk, bps );

    return _ntk.num_gates() * 6 + buf_inst.dry_run() * 2;
  };

  explorer_stats st;
  explorer<Ntk> expl( ps, st, aqfp_cost );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with k-LUT mapping using random value {}, k = {}\n", rand, 2 + (rand % 5) );
    lut_map_params mps;
    mps.cut_enumeration_ps.cut_size = 3 + (i & 0x3);
    mapping_view<Ntk> mapped{ _ntk };
    lut_map( mapped, mps );
    const auto klut = *collapse_mapped_network<klut_network>( mapped );
    
    if ( (rand >> 2) & 0x1 )
    {
      _ntk = convert_klut_to_graph<Ntk>( klut );
    }
    else
    {
      sop_factoring<Ntk> resyn;
      _ntk = node_resynthesis<Ntk>( klut, resyn );
    }
  } );

  expl.add_decompressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    //fmt::print( "decompressing with break-MAJ using random value {}\n", rand );
    std::mt19937 g( rand );
    _ntk.foreach_gate( [&]( auto n ){
      bool is_maj = true;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        if ( _ntk.is_constant( _ntk.get_node( fi ) ) )
          is_maj = false;
        return;
      });
      if ( !is_maj )
        return;
      std::vector<typename Ntk::signal> fanins;
      _ntk.foreach_fanin( n, [&]( auto fi ){
        fanins.emplace_back( fi );
      });

      std::shuffle( fanins.begin(), fanins.end(), g );
      _ntk.substitute_node( n, _ntk.create_or( _ntk.create_and( fanins[0], fanins[1] ), _ntk.create_and( fanins[2], !_ntk.create_and( !fanins[0], !fanins[1] ) ) ) );
    });
  }, 0.3 );

  // high-effort AIG optimization + (50% chance high-effort) mapping
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    aig_network aig = cleanup_dangling<mig_network, aig_network>( _ntk );
    compress2rs_aig( aig );

    mig_npn_resynthesis resyn3{ true };
    exact_library<mig_network> exact_lib( resyn3 );
    map_params mps;
    mps.skip_delay_round = false;
    mps.required_time = std::numeric_limits<double>::max();
    mps.area_flow_rounds = 1;
    mps.enable_logic_sharing = rand & 0x1; /* high-effort remap */
    _ntk = map( aig, exact_lib, mps );
  } );

  // high-effort MIG resub x1
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    resubstitution_params rps;
    rps.max_inserts = rand & 0x7;
    rps.max_pis = (rand >> 3) & 0x1 ? 6 : 8;
    depth_view depth_mig{ _ntk };
    fanout_view fanout_mig{ depth_mig };
    mig_resubstitution2( fanout_mig, rps );
    _ntk = cleanup_dangling( _ntk );
  }, 0.5 );

  // balancing
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    sop_rebalancing<mig_network> balance_fn;
    balancing_params bps;
    bps.cut_enumeration_ps.cut_size = 6u;
    _ntk = balancing( _ntk, {balance_fn}, bps );
  }, 0.5 );

  // algebraic depth optimization
  expl.add_compressing_script( []( Ntk& _ntk, uint32_t i, uint32_t rand ){
    depth_view depth_mig{ _ntk };
    mig_algebraic_depth_rewriting( depth_mig );
    _ntk = cleanup_dangling( _ntk );
  }, 0.5 );

  return expl.run( ntk );
}

} // namespace mockturtle
