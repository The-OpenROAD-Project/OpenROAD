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
  \file partial_dag.hpp
  \brief AQFP partial DAG data structure

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <sstream>
#include <vector>

#include <fmt/format.h>

#include "dag.hpp"

namespace mockturtle
{

/*! \brief A class for constructing DAG structures in an incremental manner. */
template<typename NodeT = int>
struct aqfp_partial_dag : public aqfp_dag<NodeT>
{
  using node_type = NodeT;

  using aqfp_dag<node_type>::nodes;
  using aqfp_dag<node_type>::input_slots;
  using aqfp_dag<node_type>::zero_input;
  using aqfp_dag<node_type>::num_gates;

  uint32_t num_levels = 0u; // current number of levels

  std::unordered_map<uint32_t, uint32_t> num_gates_of_fanin;
  std::vector<uint32_t> node_num_fanin;     // number of fanins of each node
  std::vector<node_type> last_layer_leaves; // remaining fanin slots in the last layer
  std::vector<node_type> other_leaves;      // remaining fanin slots of the other layers

  aqfp_partial_dag(
      const std::vector<std::vector<NodeT>>& nodes = {},
      const std::vector<NodeT>& input_slots = {},
      node_type zero_input = {},
      uint32_t num_levels = 0u,
      const std::unordered_map<uint32_t, uint32_t>& num_gates_of_fanin = {},
      const std::vector<uint32_t>& node_num_fanin = {},
      const std::vector<node_type>& last_layer_leaves = {},
      const std::vector<node_type>& other_leaves = {} )
      : aqfp_dag<node_type>( nodes, input_slots, zero_input ),
        num_levels( num_levels ),
        num_gates_of_fanin( num_gates_of_fanin ),
        node_num_fanin( node_num_fanin ),
        last_layer_leaves( last_layer_leaves ),
        other_leaves( other_leaves ) {}

  aqfp_partial_dag( const std::string& str )
  {
    decode_from_string( str );
  }

  /*! \brief Decode a string representation of a DAG into a DAG.
   *
   * Format: ng ni zi k0 g0f0 g0f1 .. g0fk0 k1 g1f0 g1f1 .. g1fk1 ....
   * ng := num gates, ni := num inputs, zi := zero input, ki := num fanin of i-th gate, gifj = j-th fanin of i-th gate
   */
  void decode_from_string( const std::string& str )
  {
    num_levels = 0;

    std::istringstream iss( str );
    auto ng = 0u;
    auto ni = 0u;
    auto zi = 0u;

    iss >> ng >> ni >> zi;
    zero_input = zi;

    std::vector<uint32_t> level( ng + ni, 0u );
    level[0u] = 1u;
    for ( auto i = 0u; i < ng; i++ )
    {
      auto nf = 0u;
      iss >> nf;

      num_gates_of_fanin[nf]++;
      node_num_fanin.push_back( nf );
      nodes.push_back( {} );

      for ( auto j = 0u; j < nf; j++ )
      {
        auto t = 0u;
        iss >> t;
        nodes[i].push_back( t );
        level[t] = std::max( level[t], level[i] + 1 );
      }
    }

    num_levels = *std::max_element( level.begin(), level.end() );

    for ( auto i = 0u; i < ni; i++ )
    {
      nodes.push_back( {} );
      node_num_fanin.push_back( 0 );
      input_slots.push_back( nodes.size() - 1 );
    }
  }

  /*! \brief Returns a map with maximum number of equal fanins that a gate may have without that gate being redundant.
   *
   * A 3-input majority gate may not have any equal fanins as it would simplify otherwise.
   * A 5-input majoirty gate may have up to 2 equal fanins but if it had more, then it would simplify.
   */
  std::vector<uint32_t> max_equal_fanins() const
  {
    std::vector<uint32_t> res( num_gates() );
    for ( auto i = 0u; i < num_gates(); i++ )
    {
      res[i] = node_num_fanin[i] / 2;
    }
    return res;
  }

  /*! \brief Add "fanin" as a fanin of "node". */
  void add_fanin( NodeT node, NodeT fanin )
  {
    nodes[node].push_back( fanin );
  }

  /*! \brief Adds a new node with "num_fanin" fanins that is connected to "fanouts".
   *
   * Optionally, it can be specified as a last layer node if at least one of its fanouts previously belonged to
   * the last layer.
   */
  uint32_t add_internal_node( uint32_t num_fanin = 3u, const std::multiset<NodeT>& fanouts = {}, bool is_in_last_layer = true )
  {
    uint32_t node_id = nodes.size();
    nodes.push_back( {} );

    assert( node_id == node_num_fanin.size() );

    num_gates_of_fanin[num_fanin]++;
    node_num_fanin.push_back( num_fanin );

    for ( auto&& fo : fanouts )
    {
      add_fanin( fo, node_id );
    }

    if ( is_in_last_layer )
    {
      for ( auto slot = 0u; slot < num_fanin; slot++ )
      {
        last_layer_leaves.push_back( node_id );
      }
    }

    return node_id;
  }

  /*! \brief Adds a new input node connected to "fanouts". */
  uint32_t add_leaf_node( const std::multiset<NodeT>& fanouts = {} )
  {
    auto input_slot = add_internal_node( 0u, fanouts, false );
    input_slots.push_back( input_slot );
    return input_slot;
  }

  /*! \brief Make a copy  with empty last_layer_leaves and other_leaves. */
  aqfp_partial_dag copy_without_leaves() const
  {
    aqfp_partial_dag res{
        nodes,
        input_slots,
        zero_input,

        num_levels,

        num_gates_of_fanin,
        node_num_fanin,
        {},
        {},
    };

    return res;
  }

  /*! \brief Make a copy  with empty other_leaves. */
  aqfp_partial_dag copy_with_last_layer_leaves() const
  {
    aqfp_partial_dag res{
        nodes,
        input_slots,
        zero_input,

        num_levels,

        num_gates_of_fanin,
        node_num_fanin,
        last_layer_leaves,
        {},
    };

    return res;
  }

  /*! \brief Create a aqfp_partial_dag with a single gate of a given number of fanins. */
  static aqfp_partial_dag get_root( uint32_t num_fanin )
  {
    aqfp_partial_dag net;

    net.num_levels = 1u;

    std::multiset<NodeT> fanouts = {};
    net.add_internal_node( num_fanin, fanouts, true );

    return net;
  }
};

} // namespace mockturtle