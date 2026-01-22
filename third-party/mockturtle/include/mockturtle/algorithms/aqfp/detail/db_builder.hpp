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
  \file db_builder.hpp
  \brief Builder class for AQFP DAG database

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include <kitty/kitty.hpp>

#include "../aqfp_db.hpp"
#include "dag.hpp"
#include "dag_cost.hpp"
#include "npn_cache.hpp"

namespace mockturtle
{

/*! \brief A class to help the generation of AQFP database in an incremental manner. */
template<typename Ntk = aqfp_dag<>>
class aqfp_db_builder
{
  using db_type = aqfp_db<Ntk>;
  using replacement = typename db_type::replacement;

public:
  aqfp_db_builder(
      const std::unordered_map<uint32_t, double>& gate_costs = { { 3u, 6.0 }, { 5u, 10.0 } },
      const std::unordered_map<uint32_t, double>& splitters = { { 1u, 2.0 }, { 4u, 2.0 } } )
      : gate_costs( gate_costs ), splitters( splitters ), cc( gate_costs, splitters ) {}

  db_type build()
  {
    return db_type( db, gate_costs, splitters );
  }

  /*! \brief Update the database with a rusult for network `net`. */
  void update( const Ntk& ntk, const std::unordered_map<uint64_t, double>& cost_config, uint32_t N = 4u )
  {
    std::vector<uint64_t> input_tt = { 0x0000UL, 0xaaaaUL, 0xccccUL, 0xf0f0UL, 0xff00UL };
    auto fs = all_functions_from_dag( input_tt, ntk );

    std::unordered_set<uint64_t> npn;
    for ( auto f : fs )
    {
      auto tmp = npndb( f );

      auto& npntt = std::get<0>( tmp );
      auto& npnperm = std::get<2>( tmp );

      if ( npn.count( npntt ) ) // already processed
      {
        continue;
      }
      npn.insert( npntt );

      std::vector<uint32_t> revperm( N );
      for ( uint32_t i = 0; i < N; i++ )
      {
        revperm[npnperm[i]] = i;
      }

      for ( auto it = cost_config.begin(); it != cost_config.end(); it++ )
      {
        auto& lvl_cfg = it->first;
        auto& cost = it->second;
        std::vector<uint32_t> new_levels = { level_of_input( lvl_cfg, npnperm[0] ),
                                             level_of_input( lvl_cfg, npnperm[1] ),
                                             level_of_input( lvl_cfg, npnperm[2] ),
                                             level_of_input( lvl_cfg, npnperm[3] ) };
        assert( new_levels.size() == N );
        auto new_lvl_cfg = lvl_cfg_from_vec( new_levels );

        if ( !db[npntt].count( new_lvl_cfg ) || db[npntt][new_lvl_cfg].cost > cost )
        {
          db[npntt][new_lvl_cfg] = { cost, ntk, new_levels, revperm };
        }
      }
    }
  }

  /*! Filter database configurations that are "covered" by other configurations. */
  void remove_redundant( bool verbose = false )
  {
    for ( auto i = db.begin(); i != db.end(); i++ )
    {
      auto& npn = i->first;
      auto& configs = i->second;

      std::map<uint64_t, replacement> good_configs;
      for ( auto it = configs.begin(); it != configs.end(); it++ )
      {
        bool ok = true;

        uint32_t N = it->second.input_levels.size();

        for ( auto jt = configs.begin(); jt != configs.end(); jt++ )
        {
          if ( jt->first == it->first )
          {
            continue;
          }

          /* check whether there exist input-wise smaller level config */
          bool jt_smaller_to_it = true;
          for ( auto j = 0; j < N; j++ )
          {
            if ( level_of_input( jt->first, j ) > level_of_input( it->first, j ) )
            {
              jt_smaller_to_it = false;
              break;
            }
          }
          if ( jt_smaller_to_it )
          {
            double extra_buff_cost = 0;
            for ( auto j = 0; j < N; j++ )
            {
              extra_buff_cost += splitters.at( 1u ) * ( level_of_input( it->first, j ) - level_of_input( jt->first, j ) );
            }
            if ( extra_buff_cost + jt->second.cost <= it->second.cost )
            {
              if ( verbose )
              {
                std::cerr << fmt::format( "[aqfp_db] configuration {:04x} already covered by {:04x} [{} {} {}].\n",
                                          it->first, jt->first, it->second.cost, jt->second.cost, extra_buff_cost );
              }
              ok = false;
              break;
            }
          }
        }
        if ( ok )
        {
          good_configs[it->first] = it->second;
        }
      }
      db[npn] = good_configs;
    }
  }

  /*! \brief Save database to the output stream `os`. */
  void save_db_to_file( std::ostream& os, bool hardcode = false )
  {
    if ( !hardcode )
    {
      /* output the database in plain-text encoding */

      /* number of functions */
      os << fmt::format( "{}\n", db.size() );
      for ( auto i = db.begin(); i != db.end(); i++ )
      {
        /* npn class */
        auto& k = i->first;

        auto& m = i->second;
        os << fmt::format( "{:04x}\n", k );

        /* number of entries for the npn class */
        os << fmt::format( "{}\n", m.size() );
        for ( auto it = m.begin(); it != m.end(); it++ )
        {
          auto& lvl_cfg = it->first;
          auto& r = it->second;

          os << fmt::format( "{:08x}\n", lvl_cfg );
          os << fmt::format( "{}\n", r.cost );
          os << fmt::format( "{}\n", r.ntk.encode_as_string() );
          os << fmt::format( "{}\n", fmt::join( r.input_perm, " " ) );
        }
      }

      return;
    }

    /* output the database encoded as an initializer list */

    os << "{\n";
    for ( auto i = db.begin(); i != db.end(); i++ )
    {
      os << fmt::format( "\t{{ 0x{:04x}, {{\n", i->first );

      for ( auto j = i->second.begin(); j != i->second.end(); j++ )
      {
        os << fmt::format( "\t\t\t\t{{ 0x{:08x}, ", j->first );
        os << fmt::format( "{{ {}, {{ \"{}\" }}, {{ {} }}, {{ {} }} }} }},\n",
                           j->second.cost,
                           j->second.ntk.encode_as_string(),
                           fmt::join( j->second.input_levels, ", " ),
                           fmt::join( j->second.input_perm, ", " ) );
      }

      os << "\t\t}\n\t},\n";
    }
    os << "}\n";
  }

  /*! \brief Load database from input stream `is`. */
  void load_db( std::istream& is )
  {
    aqfp_db<Ntk>::load_db( is, db );
  }

private:
  std::unordered_map<uint32_t, double> gate_costs;
  std::unordered_map<uint32_t, double> splitters;
  std::unordered_map<uint64_t, std::map<uint64_t, replacement>> db;
  dag_aqfp_cost_and_depths<Ntk> cc;
  npn_cache npndb;

  /*! \brief Compute all functions synthesizable from `net` if input slots are assigned the truthtables in `input_tt`. */
  std::unordered_set<uint64_t> all_functions_from_dag( const std::vector<uint64_t>& input_tt, const Ntk& net )
  {
    // static_assert( N == 4u, "Template parameter N must be equal to 4 in the current implementation" );

    uint32_t num_inputs = net.input_slots.size();
    if ( net.zero_input != 0 )
    {
      num_inputs--;
    }

    std::unordered_set<uint64_t> res;

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

      res.insert( tt[0] & 0xffff );
    }

    return res;
  }
};

} // namespace mockturtle