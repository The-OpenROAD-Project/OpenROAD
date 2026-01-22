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
  \file dag_cost.hpp
  \brief Cost computing functions for AQFP DAG structures

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <limits>
#include <unordered_map>
#include <vector>

#include "../../../properties/aqfpcost.hpp"

namespace mockturtle
{

/*! \brief Cost function for dag structures that compute the gate cost. */
template<typename Ntk>
class dag_gate_cost
{
public:
  dag_gate_cost( const std::unordered_map<uint32_t, double>& gate_costs ) : gate_costs( gate_costs ) {}

  double operator()( const Ntk& net )
  {
    double res = 0.0;

    for ( const auto& node : net.nodes )
    {
      if ( !node.empty() )
        res += gate_costs.at( node.size() );
    }

    return res;
  }

private:
  std::unordered_map<uint32_t, double> gate_costs;
};

/*! \brief Cost function for dag structures that compute the gate and path balancing cost. */
template<typename Ntk>
class dag_aqfp_cost
{
public:
  static constexpr double IMPOSSIBLE = std::numeric_limits<double>::infinity();

  using depth_config_t = uint64_t;

  dag_aqfp_cost( const std::unordered_map<uint32_t, double>& gate_costs, const std::unordered_map<uint32_t, double>& splitters )
      : simp_cc( gate_costs ), fanout_cc( splitters ) {}

  /*! \brief Compute cost assuming all primary inputs are at the same depth. */
  double operator()( const Ntk& orig_net )
  {
    net = orig_net;
    fanout = std::vector<std::vector<typename Ntk::node_type>>( net.nodes.size() );
    minlev = std::vector<uint32_t>( net.nodes.size() );
    maxlev = std::vector<uint32_t>( net.nodes.size() );
    curlev = std::vector<uint32_t>( net.nodes.size() );

    compute_fanouts( net, fanout );
    compute_min_levels( net, fanout, minlev );

    // perform depth bounded search
    double cost = IMPOSSIBLE;

    auto lastlev = *( std::max_element( minlev.begin(), minlev.end() ) );
    while ( true )
    {
      compute_max_levels( net, fanout, maxlev, lastlev );

      // fix levels for the root and the inputs
      maxlev[0] = 0;
      for ( auto f : net.input_slots )
      {
        if ( f != net.zero_input )
        {
          minlev[f] = lastlev;
        }
      }

      // check different level configurations for the other gates and compute the cost for buffers and splitters
      cost = compute_best_cost( 1u, 0.0 );

      if ( cost < IMPOSSIBLE )
      {
        break;
      }

      lastlev++;
    }

    // add the gate costs
    cost += simp_cc( net );

    return cost;
  }

protected:
  dag_gate_cost<Ntk> simp_cc;
  fanout_net_cost fanout_cc;

  Ntk net;
  std::vector<std::vector<typename Ntk::node_type>> fanout;
  std::vector<uint32_t> minlev;
  std::vector<uint32_t> maxlev;
  std::vector<uint32_t> curlev;

  template<typename FanOutT>
  void compute_fanouts( const Ntk& net, FanOutT& fanout )
  {
    for ( auto i = 0u; i < net.nodes.size(); i++ )
    {
      for ( auto&& f : net.nodes[i] )
      {
        if ( net.zero_input != f )
        {
          fanout[f].push_back( i );
        }
      }
    }
  }

  template<typename FanOutT, typename MinLevT>
  void compute_min_levels( const Ntk& net, const FanOutT& fanout, MinLevT& minlev )
  {
    for ( auto i = 0u; i < net.nodes.size(); i++ )
    {
      if ( fanout[i].size() == 0u )
      {
        minlev[i] = 0u;
      }
      else
      {
        auto critical_fo = *( std::max_element( fanout[i].begin(), fanout[i].end(),
                                                [&]( auto x, auto y ) { return ( minlev[x] < minlev[y] ); } ) );
        minlev[i] = 1 + minlev[critical_fo];
        if ( fanout[i].size() > 1 )
        {
          minlev[i]++;
        }
      }
    }
  }

  template<typename FanOutT, typename MaxLevT>
  void compute_max_levels( const Ntk& net, const FanOutT& fanout, MaxLevT& maxlev, uint32_t lastlev )
  {
    for ( auto& f : net.input_slots )
    {
      if ( f != net.zero_input )
      {
        maxlev[f] = lastlev;
      }
      else
      {
        maxlev[f] = std::numeric_limits<uint32_t>::max();
      }
    }

    for ( auto i = net.num_gates(); i > 0u; i-- )
    {
      maxlev[i - 1] = std::numeric_limits<uint32_t>::max();
      for ( auto f : net.nodes[i - 1] )
      {
        auto t = maxlev[f] - 1;
        if ( fanout[f].size() > 1 )
        {
          t--;
        }
        if ( t < maxlev[i - 1] )
        {
          maxlev[i - 1] = t;
        }
      }
    }
  }

  double cost_for_node_if_in_level( uint32_t lev, std::vector<typename Ntk::node_type> fanouts )
  {
    std::vector<uint32_t> rellev;
    for ( auto fo : fanouts )
    {
      rellev.push_back( lev - curlev[fo] );
    }
    std::stable_sort( rellev.begin(), rellev.end() );

    return fanout_cc( rellev );
  }

  double compute_best_cost( uint32_t current_gid, double cost_so_far )
  {
    if ( net.num_gates() == current_gid )
    {
      for ( auto f : net.input_slots )
      {
        if ( ( f == net.zero_input ) || fanout[f].empty() )
        {
          continue;
        }

        cost_so_far += cost_for_node_if_in_level( maxlev[f], fanout[f] );

        if ( cost_so_far >= IMPOSSIBLE )
        {
          return IMPOSSIBLE;
        }
      }

      return cost_so_far;
    }

    auto result = IMPOSSIBLE;
    for ( auto lev = minlev[current_gid]; lev <= maxlev[current_gid]; lev++ )
    {
      auto cost = cost_for_node_if_in_level( lev, fanout[current_gid] );
      if ( cost >= IMPOSSIBLE )
      {
        continue;
      }
      curlev[current_gid] = lev;
      auto temp = compute_best_cost( current_gid + 1, cost_so_far + cost );
      if ( temp < result )
      {
        result = temp;
      }
    }

    return result;
  }
};

/*! \brief Compute cost together with the depth assignment to nodes for a given input depth configuration. */
template<typename Ntk>
class dag_aqfp_cost_and_depths : public dag_aqfp_cost<Ntk>
{
public:
  static constexpr double IMPOSSIBLE = std::numeric_limits<double>::infinity();

  using dag_aqfp_cost<Ntk>::simp_cc;
  using dag_aqfp_cost<Ntk>::fanout_cc;
  using dag_aqfp_cost<Ntk>::net;
  using dag_aqfp_cost<Ntk>::fanout;
  using dag_aqfp_cost<Ntk>::minlev;
  using dag_aqfp_cost<Ntk>::maxlev;
  using dag_aqfp_cost<Ntk>::curlev;
  using dag_aqfp_cost<Ntk>::compute_fanouts;
  using dag_aqfp_cost<Ntk>::compute_min_levels;
  using dag_aqfp_cost<Ntk>::compute_max_levels;
  using dag_aqfp_cost<Ntk>::cost_for_node_if_in_level;

  dag_aqfp_cost_and_depths( const std::unordered_map<uint32_t, double>& gate_costs, const std::unordered_map<uint32_t, double>& splitters )
      : dag_aqfp_cost<Ntk>( gate_costs, splitters ) {}

  std::pair<double, std::vector<uint32_t>> operator()( const Ntk& orig_net, const std::vector<uint32_t>& input_depths )
  {
    net = orig_net;
    fanout = std::vector<std::vector<typename Ntk::node_type>>( net.nodes.size() );
    minlev = std::vector<uint32_t>( net.nodes.size() );
    maxlev = std::vector<uint32_t>( net.nodes.size() );
    curlev = std::vector<uint32_t>( net.nodes.size() );

    compute_fanouts( net, fanout );
    compute_min_levels( net, fanout, minlev );
    compute_max_levels( net, fanout, maxlev, input_depths );

    uint32_t ind = 0u;

    // fix levels for the root and the inputs
    maxlev[0] = 0;
    ind = 0u;
    for ( auto f : net.input_slots )
    {
      if ( f != net.zero_input )
      {
        minlev[f] = input_depths[ind++];
      }
    }

    // check different level configurations for the other gates and compute the cost for buffers and splitters
    auto [cost, levels] = compute_best_cost_and_levels( 1u, 0.0 );

    // add the gate costs
    cost += simp_cc( net );

    return { cost, levels };
  }

private:
  template<typename FanOutT, typename MaxLevT>
  void compute_max_levels( const Ntk& net, const FanOutT& fanout, MaxLevT& maxlev, std::vector<uint32_t> input_depths )
  {
    uint32_t ind = 0u;
    for ( auto& f : net.input_slots )
    {
      if ( f != net.zero_input )
      {
        maxlev[f] = input_depths[ind++];
      }
      else
      {
        maxlev[f] = std::numeric_limits<uint32_t>::max();
      }
    }

    for ( auto i = net.num_gates(); i > 0u; i-- )
    {
      maxlev[i - 1] = std::numeric_limits<uint32_t>::max();
      for ( auto f : net.nodes[i - 1] )
      {
        auto t = maxlev[f] - 1;
        if ( fanout[f].size() > 1 )
        {
          t--;
        }
        if ( t < maxlev[i - 1] )
        {
          maxlev[i - 1] = t;
        }
      }
    }
  }

  std::tuple<double, std::vector<uint32_t>> compute_best_cost_and_levels( uint32_t current_gid, double cost_so_far )
  {
    if ( net.num_gates() == current_gid )
    {
      for ( auto f : net.input_slots )
      {
        if ( ( f == net.zero_input ) || fanout[f].empty() )
        {
          continue;
        }

        curlev[f] = maxlev[f];
        cost_so_far += cost_for_node_if_in_level( maxlev[f], fanout[f] );

        if ( cost_so_far >= IMPOSSIBLE )
        {
          return { IMPOSSIBLE, {} };
        }
      }

      return { cost_so_far, curlev };
    }

    double res_cost = IMPOSSIBLE;
    std::vector<uint32_t> res_lev = {};
    for ( auto lev = minlev[current_gid]; lev <= maxlev[current_gid]; lev++ )
    {
      auto cost = cost_for_node_if_in_level( lev, fanout[current_gid] );
      if ( cost >= IMPOSSIBLE )
      {
        continue;
      }
      curlev[current_gid] = lev;
      auto [temp_cost, temp_lev] = compute_best_cost_and_levels( current_gid + 1, cost_so_far + cost );
      if ( temp_cost < res_cost )
      {
        res_cost = temp_cost;
        res_lev = temp_lev;
      }
    }

    return { res_cost, res_lev };
  }
};

/*! \brief Compute costs for different input depth configurations. */
template<typename Ntk>
class dag_aqfp_cost_all_configs : public dag_aqfp_cost<Ntk>
{
public:
  static constexpr double IMPOSSIBLE = std::numeric_limits<double>::infinity();

  using depth_config_t = uint64_t; // each byte encodes a depth of a primary input

  using dag_aqfp_cost<Ntk>::simp_cc;
  using dag_aqfp_cost<Ntk>::fanout_cc;
  using dag_aqfp_cost<Ntk>::net;
  using dag_aqfp_cost<Ntk>::fanout;
  using dag_aqfp_cost<Ntk>::minlev;
  using dag_aqfp_cost<Ntk>::maxlev;
  using dag_aqfp_cost<Ntk>::curlev;
  using dag_aqfp_cost<Ntk>::compute_fanouts;
  using dag_aqfp_cost<Ntk>::compute_min_levels;
  using dag_aqfp_cost<Ntk>::compute_max_levels;
  using dag_aqfp_cost<Ntk>::cost_for_node_if_in_level;

  dag_aqfp_cost_all_configs( const std::unordered_map<uint32_t, double>& gate_costs, const std::unordered_map<uint32_t, double>& splitters )
      : dag_aqfp_cost<Ntk>( gate_costs, splitters ) {}

  std::unordered_map<depth_config_t, double> operator()( const Ntk& orig_net )
  {
    std::unordered_map<depth_config_t, double> config_cost;
    net = orig_net;
    fanout = std::vector<std::vector<typename Ntk::node_type>>( net.nodes.size() );
    minlev = std::vector<uint32_t>( net.nodes.size() );
    maxlev = std::vector<uint32_t>( net.nodes.size() );
    curlev = std::vector<uint32_t>( net.nodes.size() );

    compute_fanouts( net, fanout );
    compute_min_levels( net, fanout, minlev );

    auto lastlev = *( std::max_element( minlev.begin(), minlev.end() ) );
    while ( true )
    {
      compute_max_levels( net, fanout, maxlev, lastlev );

      // fix levels for the root and the inputs
      maxlev[0] = 0;

      // check different level configurations for the other gates and compute the cost for buffers and splitters
      compute_best_costs_for_all_configs( 1u, 0.0, config_cost );

      if ( config_cost.size() > 0 )
      {
        break;
      }

      lastlev++;
    }

    double cost_for_gates = simp_cc( net );

    for ( auto it = config_cost.begin(); it != config_cost.end(); it++ )
    {
      it->second += cost_for_gates;
    }

    return config_cost;
  }

private:
  void compute_best_costs_for_all_configs( uint32_t current_gid, double cost_so_far, std::unordered_map<depth_config_t, double>& config_cost )
  {
    if ( net.nodes.size() == current_gid )
    {
      depth_config_t depth_config = 0u;
      uint32_t ind = 0;
      for ( auto f : net.input_slots )
      {
        if ( f == net.zero_input )
        {
          continue;
        }

        depth_config |= ( curlev[f] << ( 8 * ind ) );
        ind++;
      }

      if ( !config_cost.count( depth_config ) )
      {
        config_cost[depth_config] = IMPOSSIBLE;
      }

      config_cost[depth_config] = std::min( config_cost[depth_config], cost_so_far );
      return;
    }

    if ( net.zero_input == (typename Ntk::node_type)current_gid )
    {
      compute_best_costs_for_all_configs( current_gid + 1, cost_so_far, config_cost );
      return;
    }

    for ( auto lev = minlev[current_gid]; lev <= maxlev[current_gid]; lev++ )
    {
      auto cost = cost_for_node_if_in_level( lev, fanout[current_gid] );
      if ( cost >= IMPOSSIBLE )
      {
        continue;
      }
      curlev[current_gid] = lev;
      compute_best_costs_for_all_configs( current_gid + 1, cost_so_far + cost, config_cost );
    }
  }
};

} // namespace mockturtle