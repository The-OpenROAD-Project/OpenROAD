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
  \file aqfp_db.hpp
  \brief AQFP DAG database

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <map>
#include <unordered_map>
#include <vector>

#include <kitty/kitty.hpp>

#include "detail/dag.hpp"
#include "detail/dag_cost.hpp"
#include "detail/npn_cache.hpp"

namespace mockturtle
{

template<typename T>
T bitwise_majority( const T& a, const T& b, const T& c )
{
  return ( a & b ) | ( c & ( a | b ) );
}

template<typename T>
T bitwise_majority( const T& a, const T& b, const T& c, const T& d, const T& e )
{
  return ( a & b & c ) | ( a & b & d ) | ( a & b & e ) | ( a & c & d ) | ( a & c & e ) |
         ( a & d & e ) | ( b & c & d ) | ( b & c & e ) | ( b & d & e ) | ( c & d & e );
}

/*! \brief Returns the level of input with index `input_idx` from level configuration `lvl_cfg`. */
inline uint32_t level_of_input( uint64_t lvl_cfg, uint32_t input_idx )
{
  return ( lvl_cfg >> ( 8u * input_idx ) ) & 0xff;
}

/*! \brief Returns the vector representation of the level configuration `lvl_cfg`. */
inline std::vector<uint32_t> lvl_cfg_to_vec( uint64_t lvl_cfg, uint32_t num_leaves )
{
  std::vector<uint32_t> res( num_leaves );
  for ( auto i = 0u; i < num_leaves; i++ )
  {
    res[i] = level_of_input( lvl_cfg, i );
  }
  return res;
}

/*! \brief  Returns the level configuration for levels represented by `levels`. */
inline uint64_t lvl_cfg_from_vec( std::vector<uint32_t> levels )
{
  uint64_t res = 0u;
  for ( auto i = 0u; i < levels.size(); i++ )
  {
    res |= ( levels[i] << ( 8u * i ) );
  }
  return res;
}

/*! \brief A class to represent an AQFP exact synthesis database. */
template<typename Ntk = aqfp_dag<>>
class aqfp_db
{

public:
  struct replacement
  {
    double cost;
    Ntk ntk;
    std::vector<uint32_t> input_levels; // input levels
    std::vector<uint32_t> input_perm;   // input permutation so that ntk compute the npn-class
  };

  aqfp_db(
      const std::unordered_map<uint32_t, double>& gate_costs = { { 3u, 6.0 }, { 5u, 10.0 } },
      const std::unordered_map<uint32_t, double>& splitters = { { 1u, 2.0 }, { 4u, 2.0 } } )
      : gate_costs( gate_costs ), splitters( splitters ), db( get_default_db() ), cc( gate_costs, splitters )
  {
  }

  aqfp_db( const std::unordered_map<uint64_t, std::map<uint64_t, replacement>>& db,
           const std::unordered_map<uint32_t, double>& gate_costs = { { 3u, 6.0 }, { 5u, 10.0 } },
           const std::unordered_map<uint32_t, double>& splitters = { { 1u, 2.0 }, { 4u, 2.0 } } )
      : gate_costs( gate_costs ), splitters( splitters ), db( db ), cc( gate_costs, splitters )
  {
  }

  using gate_info = std::vector<uint32_t>;                                               // fanin list with lsb denoting the inversion
  using mig_structure = std::tuple<std::vector<gate_info>, std::vector<uint32_t>, bool>; // (gates, levels, output inverted flag);

  template<typename ComparisonFn>
  mig_structure get_best_replacement( uint64_t f, std::vector<uint32_t> _levels, std::vector<bool> _is_const, ComparisonFn&& comparison_fn )
  {
    /* find the npn class for the function */
    auto tmp = npndb( f );
    auto& npntt = std::get<0>( tmp );
    auto& npnperm = std::get<2>( tmp );

    if ( db[npntt].empty() )
    {
      assert( false );
      return { {}, {}, false };
    }

    /* map input levels */
    std::vector<uint32_t> levels( _levels.size() );
    std::vector<bool> is_const( _levels.size() );
    for ( auto i = 0u; i < levels.size(); i++ )
    {
      levels[i] = _levels[npnperm[i]];
      is_const[i] = _is_const[npnperm[i]];
    }

    double best_cost = std::numeric_limits<double>::infinity();
    uint32_t best_lev = std::numeric_limits<uint32_t>::max();
    replacement best = db[npntt].begin()->second;
    uint32_t best_ind = 0;

    uint32_t temp_ind = 0;
    for ( auto it = db[npntt].begin(); it != db[npntt].end(); it++ )
    {
      const auto& lvl_cfg = it->first;
      const auto& r = it->second;

      uint32_t max_lev = 0u;
      for ( auto i = 0u; i < levels.size(); i++ )
      {
        auto temp = levels[i] + level_of_input( lvl_cfg, i );
        max_lev = std::max( max_lev, temp );
      }
      uint32_t buffer_count = 0u;
      for ( auto i = 0u; i < levels.size(); i++ )
      {
        if ( !is_const[i] )
        {
          auto temp = levels[i] + level_of_input( lvl_cfg, i );
          buffer_count += ( max_lev - temp );
        }
      }

      double cost = buffer_count * splitters.at( 1u ) + r.cost;

      if ( comparison_fn( { cost, max_lev }, { best_cost, best_lev } ) )
      {
        best_cost = cost;
        best_lev = max_lev;
        best = r;
        best_ind = temp_ind;
      }
      temp_ind++;
    }

    usage_stats[{ npntt, best_ind }]++;
    return compute_replacement_structure( best, f );
  }

  /*! \brief Load database from input stream `is`. */
  void load_db( std::istream& is, uint32_t version = 1u )
  {
    load_db( is, db, version );
  }

  /*! \brief Load database from input stream `is`. */
  template<typename T>
  static void load_db( std::istream& is, T& db, uint32_t version = 1u )
  {
    std::string line;

    std::getline( is, line );
    uint32_t num_func = std::stoul( line );

    for ( auto func = 0u; func < num_func; func++ )
    {
      uint32_t N = 4;
      if ( version > 1u )
      {
        std::getline( is, line );
        N = std::stoul( line );
      }

      std::getline( is, line );
      uint64_t npn = std::stoul( line, 0, 16 );

      std::getline( is, line );
      uint32_t num_entries = std::stoul( line );

      for ( auto j = 0u; j < num_entries; j++ )
      {
        std::getline( is, line );
        uint64_t lvl_cfg = std::stoul( line, 0, 16 );
        auto levels = lvl_cfg_to_vec( lvl_cfg, N );

        std::getline( is, line );
        double cost = std::stod( line );

        std::getline( is, line );
        Ntk ntk( line );

        std::vector<uint32_t> perm( N );
        for ( int i = 0; i < N; i++ )
        {
          uint32_t t;
          is >> t;
          perm[i] = t;
        }
        std::getline( is, line ); // ignore the current line end

        lvl_cfg = lvl_cfg_from_vec( levels );

        if ( !db[npn].count( lvl_cfg ) || db[npn][lvl_cfg].cost > cost )
        {
          db[npn][lvl_cfg] = { cost, ntk, levels, perm };
        }
      }
    }
  }

  void print_usage_state( std::ostream& os )
  {
    os << "printing stats\n";
    for ( auto& x : usage_stats )
    {
      os << fmt::format( "{}, {}, {}\n", x.first.first, x.first.second, x.second );
    }
    os << "printing stats done\n";
  }

  template<typename Fn>
  void for_each_db_entry( Fn&& func )
  {
    for ( const auto& [npn_class, entries_for_npn_class] : db )
    {
      for ( const auto& [depth_config, replacement] : entries_for_npn_class )
      {
        func( npn_class, compute_replacement_structure( replacement, npn_class ), replacement.cost );
      }
    }
  }

private:
  std::unordered_map<uint32_t, double> gate_costs;
  std::unordered_map<uint32_t, double> splitters;
  std::unordered_map<uint64_t, std::map<uint64_t, replacement>> db;
  std::map<std::pair<uint64_t, uint32_t>, uint32_t> usage_stats;
  dag_aqfp_cost_and_depths<Ntk> cc;
  npn_cache npndb;

  std::pair<bool, std::vector<uint32_t>> inverter_config_for_func( const std::vector<uint64_t>& input_tt, const Ntk& net, uint64_t func )
  {
    uint32_t num_inputs = net.input_slots.size();
    if ( net.zero_input != 0 )
    {
      num_inputs--;
    }

    std::vector<uint64_t> tt( net.nodes.size(), input_tt[0] );
    auto input_ind = 1u;

    auto tmp_input_slots = net.input_slots;
    std::stable_sort( tmp_input_slots.begin(), tmp_input_slots.end() );
    assert( tmp_input_slots == net.input_slots );

    for ( auto i : net.input_slots )
    {
      if ( i != (int)net.zero_input )
      {
        tt[i] = input_tt[input_ind++];
      }
    }

    auto shift = 0u;
    for ( auto i = 0u; i < net.num_gates(); i++ )
    {
      shift += ( net.nodes[i].size() - 1 );
    }

    const auto n_gates = net.num_gates();
    std::vector<uint32_t> res( n_gates );

    for ( auto inv_config_itr = 0ul; inv_config_itr < ( 1ul << shift ); inv_config_itr++ )
    {
      auto inv_config = inv_config_itr;

      for ( auto i = n_gates; i > 0; i-- )
      {
        const auto& n = net.nodes[i - 1];

        const auto n_fanin = n.size();
        const auto shift = n_fanin - 1;
        const auto mask = ( 1 << shift ) - 1;
        const auto ith_gate_config = ( inv_config & mask );

        res[i - 1] = ith_gate_config;

        // only consider half the inverter configurations, the other half is covered by output inversion
        if ( n_fanin == 3u )
        {
          tt[i - 1] = bitwise_majority(
              ( ith_gate_config & 1 ) ? ~tt[n[0]] : tt[n[0]],
              ( ith_gate_config & 2 ) ? ~tt[n[1]] : tt[n[1]],
              tt[n[2]] );
        }
        else
        {
          tt[i - 1] = bitwise_majority(
              ( ith_gate_config & 1 ) ? ~tt[n[0]] : tt[n[0]],
              ( ith_gate_config & 2 ) ? ~tt[n[1]] : tt[n[1]],
              ( ith_gate_config & 4 ) ? ~tt[n[2]] : tt[n[2]],
              ( ith_gate_config & 8 ) ? ~tt[n[3]] : tt[n[3]],
              tt[n[4]] );
        }
        inv_config >>= shift;
      }

      if ( ( func & 0xffff ) == ( tt[0] & 0xffff ) )
      {
        return { false, res };
      }
      if ( ( ~func & 0xffff ) == ( tt[0] & 0xffff ) )
      {
        return { true, res };
      }
    }

    assert( false );
    return {};
  }

  mig_structure compute_replacement_structure( const replacement& rep, uint64_t func )
  {
    std::vector<uint32_t> levs( 4u );
    for ( auto i = 0u; i < 4u; i++ )
    {
      levs[i] = rep.input_levels[rep.input_perm[i]];
    }

    auto [new_cost, gate_levels] = cc( rep.ntk, levs );
    (void)new_cost;

    auto [npntt, npn_inv, npnperm] = npndb( func );

    std::vector<uint32_t> ind = { 0u, 1u, 2u, 3u };

    std::vector<uint32_t> ind_func_from_npn = {
        ind[npnperm[0]],
        ind[npnperm[1]],
        ind[npnperm[2]],
        ind[npnperm[3]] };

    std::vector<uint32_t> ind_func_from_dag = {
        ind_func_from_npn[rep.input_perm[0]],
        ind_func_from_npn[rep.input_perm[1]],
        ind_func_from_npn[rep.input_perm[2]],
        ind_func_from_npn[rep.input_perm[3]] };

    auto input_perm = ind_func_from_dag;

    std::vector<uint64_t> input_tt = {
        0xaaaaUL,
        0xccccUL,
        0xf0f0UL,
        0xff00UL };

    std::vector<uint64_t> input_perm_tt = {
        0x0000UL,
        input_tt[input_perm[0]],
        input_tt[input_perm[1]],
        input_tt[input_perm[2]],
        input_tt[input_perm[3]] };

    auto [output_inv, inverter_config] = inverter_config_for_func( input_perm_tt, rep.ntk, func );

    std::vector<gate_info> gates{ {}, {}, {}, {}, {} };
    std::vector<uint32_t> depths{ 0u, 0u, 0u, 0u, 0u };

    std::map<uint32_t, uint32_t> sigmap;

    std::vector<uint32_t> inputs;
    auto i = 0u;
    for ( auto x : rep.ntk.input_slots )
    {
      if ( x == rep.ntk.zero_input )
      {
        sigmap[x] = 0u;
      }
      else
      {
        sigmap[x] = input_perm[i++] + 1;
      }
      depths[sigmap[x]] = gate_levels[x];
    }

    for ( auto i = rep.ntk.num_gates(); i > 0; i-- )
    {
      auto j = i - 1;
      sigmap[j] = rep.ntk.num_gates() + 4u - j;

      gates.push_back( {} );
      assert( gates.size() == sigmap[j] + 1 );
      depths.push_back( gate_levels[j] );

      auto type = inverter_config[j];
      auto& node = rep.ntk.nodes[j];
      for ( auto k = 0u; k < node.size(); k++ )
      {
        auto new_fanin_id = sigmap[node[k]];
        auto new_fanin_inv = ( type & ( 1u << k ) ) > 0;
        gates[sigmap[j]].push_back( ( new_fanin_id << 1 ) | ( new_fanin_inv ? 1u : 0u ) );
      }
    }

    return { gates, depths, output_inv };
  }

  static std::unordered_map<uint64_t, std::map<uint64_t, replacement>> get_default_db()
  {
    return {};
  }
};

} // namespace mockturtle