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
  \file aqfpcost.hpp
  \brief Cost functions for AQFP networks

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <vector>

#include "../utils/hash_functions.hpp"
#include "../views/fanout_view.hpp"

#include "../algorithms/aqfp/aqfp_assumptions.hpp"

namespace mockturtle
{

/*! \brief Cost function for computing the best splitter and buffer cost for a fanout net with given relative levels. */
class fanout_net_cost
{
public:
  static constexpr double IMPOSSIBLE = std::numeric_limits<double>::infinity();

  fanout_net_cost( const std::unordered_map<uint32_t, double>& splitters )
      : buffer_cost( splitters.at( 1u ) ), splitters( remove_buffer( splitters ) )
  {
  }

  double operator()( const std::vector<uint32_t>& config )
  {
    return cost_for_config( config, false );
  }

  double operator()( const std::vector<uint32_t>& config, bool ignore_initial_buffers )
  {
    return cost_for_config( config, ignore_initial_buffers );
  }

private:
  using cache_key_t = std::tuple<bool, std::vector<uint32_t>>;

  double buffer_cost;
  std::unordered_map<uint32_t, double> splitters;
  std::unordered_map<cache_key_t, double, hash<cache_key_t>> cache;

  static std::unordered_map<uint32_t, double> remove_buffer( std::unordered_map<uint32_t, double> splitters )
  {
    splitters.erase( 1u );
    return splitters;
  }

  double cost_for_config( const std::vector<uint32_t> config, bool ignore_initial_buffers )
  {
    if ( config.size() == 1 )
    {
      if ( config[0] >= 1 )
      {
        return ignore_initial_buffers ? 0.0 : ( config[0] - 1 ) * buffer_cost;
      }
      else
      {
        return IMPOSSIBLE;
      }
    }

    std::tuple key = { ignore_initial_buffers, config };
    if ( cache.count( key ) )
    {
      return cache[key];
    }

    auto result = IMPOSSIBLE;

    for ( const auto& s : splitters )
    {
      for ( auto size = 2u; size <= std::min( s.first, uint32_t( config.size() ) ); size++ )
      {
        auto sp_lev = config[config.size() - size] - 1;
        if ( sp_lev == 0 )
        {
          continue;
        }

        auto temp = s.second;

        for ( auto i = config.size() - size; i < config.size(); i++ )
        {
          temp += ( config[i] - config[config.size() - size] ) * buffer_cost;
        }

        std::vector<uint32_t> new_config( config.begin(), config.begin() + ( config.size() - size ) );
        new_config.push_back( sp_lev );
        std::stable_sort( new_config.begin(), new_config.end() );

        temp += cost_for_config( new_config, ignore_initial_buffers );

        if ( temp < result )
        {
          result = temp;
        }
      }
    }

    return ( cache[key] = result );
  }
};

/*! \brief Cost function for computing the cost of a path-balanced AQFP network with a given assignment of node levels.
 *
 * Assumes no path balancing or splitters are needed for primary inputs or register outputs.
 */
struct aqfp_network_cost
{
  static constexpr double IMPOSSIBLE = std::numeric_limits<double>::infinity();

  aqfp_network_cost( const aqfp_assumptions& assume, const std::unordered_map<uint32_t, double>& gate_costs, const std::unordered_map<uint32_t, double>& splitters )
      : assume( assume ), gate_costs( gate_costs ), fanout_cc( splitters ) {}

  template<typename Ntk, typename LevelMap, typename PoLevelMap>
  double operator()( const Ntk& ntk, const LevelMap& level_of_node, const PoLevelMap& po_level_of_node )
  {
    /*  Collect fanouts of each node.
        Cannot use the fanout_view as duplicate fanis are not accounted with the correct multiplicity. */
    std::unordered_map<node<Ntk>, std::vector<node<Ntk>>> fanouts;
    ntk.foreach_gate( [&]( auto n ) { ntk.foreach_fanin( n, [&]( auto fi ) { fanouts[ntk.get_node( fi )].push_back( n ); } ); } );

    auto gate_cost = 0.0;
    auto fanout_net_cost = 0.0;

    std::vector<node<Ntk>> nodes;
    if ( assume.branch_pis )
    {
      ntk.foreach_ci( [&]( auto n ) { nodes.push_back( n ); } );
    }
    ntk.foreach_gate( [&]( auto n ) { nodes.push_back( n ); } );

    if ( po_level_of_node.size() == 0 )
    {
      std::cout << "[w] - the map po_level_of_node is empty!\n";
    }

    size_t critical_po_level = std::max_element( po_level_of_node.begin(), po_level_of_node.end(), []( auto n1, auto n2 ) { return n1.second < n2.second; } )
                                   ->second;
    for ( auto n : nodes )
    {
      if ( !ntk.is_ci( n ) ) // n must be a gate
      {
        gate_cost += gate_costs.at( ntk.fanin_size( n ) );
      }

      if ( ntk.fanout_size( n ) == 0 )
      {
        std::cout << fmt::format( "[w] - dangling node {}\n", n );
        continue;
      }

      std::vector<uint32_t> rellev;

      for ( auto fo : fanouts[n] )
      {
        assert( level_of_node.at( fo ) > level_of_node.at( n ) );
        rellev.push_back( level_of_node.at( fo ) - level_of_node.at( n ) );
      }

      uint32_t pos = 0u;
      while ( rellev.size() < ntk.fanout_size( n ) )
      {
        pos++;
        if ( assume.balance_pos )
        {
          rellev.push_back( critical_po_level + 1 - level_of_node.at( n ) );
        }
        else
        {
          rellev.push_back( po_level_of_node.at( n ) + 1 - level_of_node.at( n ) );
        }
      }

      if ( rellev.size() > 1u || ( rellev.size() == 1u && rellev[0] > 0 ) )
      {
        std::stable_sort( rellev.begin(), rellev.end() );
        auto net_cost = fanout_cc( rellev, ntk.is_ci( n ) && !assume.balance_pis );
        if ( net_cost == std::numeric_limits<double>::infinity() )
        {
          std::cerr << fmt::format( "[e] impossible to synthesize fanout net of node {} for relative levels [{}]\n", n, fmt::join( rellev, " " ) );
          std::abort();
        }
        fanout_net_cost += net_cost;
      }
      else
      {
        std::cerr << fmt::format( "[e] invalid level assignment for node {} with levels [{}]\n", n, fmt::join( rellev, " " ) );
        std::abort();
      }
    }

    return gate_cost + fanout_net_cost;
  }

private:
  aqfp_assumptions assume;
  std::unordered_map<uint32_t, double> gate_costs;
  fanout_net_cost fanout_cc;
};

} // namespace mockturtle