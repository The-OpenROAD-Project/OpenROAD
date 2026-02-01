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
  \file dag_gen.hpp
  \brief AQFP DAG generation

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <iostream>
#include <limits>
#include <mutex>
#include <queue>
#include <set>
#include <stack>
#include <thread>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>

#include "dag.hpp"
#include "dag_util.hpp"
#include "partial_dag.hpp"

namespace mockturtle
{

using part = std::multiset<int>;
using partition = std::multiset<part>;
using partition_set = std::set<partition>;

struct dag_generator_params
{

  uint32_t max_gates;      // max number of gates allowed
  uint32_t max_levels;     // max number of gate levels
  uint32_t max_num_in;     // max number of primary input slots (including the constant)
  uint32_t max_num_fanout; // max number of fanouts per gate
  uint32_t max_width;      // max number of gates in any given level

  std::vector<uint32_t> allowed_num_fanins;                  // the types of allowed majority gates
  std::unordered_map<uint32_t, uint32_t> max_gates_of_fanin; // max number of gates allowed for each type

  uint32_t verbose;

  dag_generator_params() : max_gates( std::numeric_limits<uint32_t>::max() ),
                           max_levels( std::numeric_limits<uint32_t>::max() ),
                           max_num_in( std::numeric_limits<uint32_t>::max() ),
                           max_num_fanout( std::numeric_limits<uint32_t>::max() ),
                           max_width( std::numeric_limits<uint32_t>::max() ),
                           allowed_num_fanins( { 3u } ),
                           max_gates_of_fanin( { { 3u, std::numeric_limits<uint32_t>::max() } } ),
                           verbose( 0u ) {}
};

/*! \brief Generate all DAGs derived from a given partial DAG. */
template<typename NodeT = int>
class dags_from_partial_dag
{
  using PartialNtk = aqfp_partial_dag<NodeT>;
  using Ntk = aqfp_dag<NodeT>;

public:
  dags_from_partial_dag( uint32_t max_num_in, uint32_t max_num_fanout ) : max_num_in( max_num_in ), max_num_fanout( max_num_fanout ) {}

  std::vector<Ntk> operator()( const PartialNtk& net )
  {
    std::vector<NodeT> leaves = net.last_layer_leaves;
    leaves.insert( leaves.end(), net.other_leaves.begin(), net.other_leaves.end() );
    std::stable_sort( leaves.begin(), leaves.end() );

    auto max_counts = net.max_equal_fanins();

    auto partitions = partition_gen( leaves, max_counts, max_num_in + 1, max_num_fanout );

    std::vector<Ntk> result;
    for ( auto p : partitions )
    {
      auto new_net = get_dag_for_partition( net, p );

      if ( net.input_slots.size() <= max_num_in )
      {
        result.push_back( new_net );
      }

      int i = 0;
      for ( auto it = p.begin(); it != p.end(); )
      {
        auto temp_net = new_net;
        temp_net.zero_input = new_net.input_slots[i];
        result.push_back( temp_net );
        auto c = p.count( *it );
        i += c;
        std::advance( it, c );
      }
    }

    return result;
  }

private:
  uint32_t max_num_in;
  uint32_t max_num_fanout;
  detail::partition_generator<NodeT> partition_gen;

  /**
   * \brief Compute the DAG obtained from partial DAG `orig` by combining slots
   * according to partitions `p`.
   */
  Ntk get_dag_for_partition( const PartialNtk& orig, const partition& p )
  {
    auto net = orig.copy_without_leaves();
    std::for_each( p.begin(), p.end(), [&net]( const auto& q ) { net.add_leaf_node( q ); } );
    return std::move( net );
  }
};

#if !__clang__ || __clang_major__ > 10

/*! \brief Generate all DAGs satisfying the parameters. */
template<typename NodeT = int>
class dag_generator
{
  using PartialNtk = aqfp_partial_dag<NodeT>;

public:
  dag_generator( const dag_generator_params& params, uint32_t num_threads = 1u ) : params( params ), num_threads( num_threads ) {}

  template<typename Fn>
  void for_each_dag( Fn&& callback )
  {
    if ( params.verbose > 0u )
    {
      std::cerr << fmt::format( "Generating partial dags.\n" );
    }

    generate_all_partial_dags();

    if ( params.verbose > 0u )
    {
      std::cerr << fmt::format( "Generated {} partial dags.\n", partial_dags.size() );
      std::cerr << fmt::format( "Generating dags in {} threads...\n", num_threads );
    }

    std::vector<std::thread> threads;
    std::mutex mu;
    for ( auto i = 0u; i < num_threads; i++ )
    {
      threads.emplace_back(
          [&]( auto id ) {
            dags_from_partial_dag<NodeT> dag_from_pdag( params.max_num_in, params.max_num_fanout );
            while ( true )
            {
              mu.lock();
              if ( partial_dags.empty() )
              {
                mu.unlock();
                return;
              }
              auto p = partial_dags.front();
              partial_dags.pop();
              mu.unlock();
              auto dags = dag_from_pdag( p );
              for ( const auto& dag : dags )
              {
                callback( dag, id );
              }
            }
          },
          i );
    }

    for ( auto i = 0u; i < num_threads; i++ )
    {
      threads[i].join();
    }
  }

private:
  dag_generator_params params;
  uint32_t num_threads;

  std::queue<PartialNtk> partial_dags;

  detail::partition_generator<NodeT> partition_gen;
  detail::partition_extender<NodeT> partition_ext;
  detail::sublist_generator<NodeT> sublist_gen;

  void generate_all_partial_dags()
  {
    std::stack<PartialNtk> stk;

    for ( auto&& fin : params.allowed_num_fanins )
    {
      if ( params.max_gates_of_fanin.at( fin ) > 0 )
      {
        auto root = PartialNtk::get_root( fin );
        stk.push( root );
      }
    }

    while ( !stk.empty() )
    {
      auto res = stk.top();
      stk.pop();

      partial_dags.push( res );

      if ( params.max_levels > res.num_levels )
      {
        auto ext = get_layer_extension( res );
        for ( const auto& e : ext )
        {
          stk.push( e );
        }
      }
    }
  }

  /*! \brief Extend the current aqfp logical network by one more level. */
  std::vector<PartialNtk> get_layer_extension( const PartialNtk& net )
  {
    std::vector<PartialNtk> result;

    auto max_counts = net.max_equal_fanins();

    auto last_options = sublist_gen( net.last_layer_leaves );
    auto other_options = sublist_gen( net.other_leaves );

    auto last_counts = detail::get_frequencies( net.last_layer_leaves );
    auto other_counts = detail::get_frequencies( net.other_leaves );

    auto net_without_leaves = net.copy_without_leaves();

    /* Consider all different ways of choosing a non-empty subset of last layer slots */
    for ( auto&& last : last_options )
    {
      if ( last.empty() )
        continue;

      /* Remaining slots in the last layer */
      auto last_counts_cpy = last_counts;
      for ( auto&& e : last )
      {
        last_counts_cpy[e]--;
      }

      /* Consider all different ways of choosing a subset of other layer slots */
      for ( auto&& other : other_options )
      {

        /* Remaining slots in the other layers */
        auto other_counts_cpy = other_counts;
        for ( auto&& e : other )
        {
          other_counts_cpy[e]--;
        }

        /* Compute the new set of other leaves for all resulting partial DAGs */
        std::vector<int> other_leaves_new;
        for ( auto it = last_counts_cpy.begin(); it != last_counts_cpy.end(); it++ )
        {
          for ( auto i = 0u; i < it->second; i++ )
          {
            other_leaves_new.push_back( it->first );
          }
        }
        for ( auto it = other_counts_cpy.begin(); it != other_counts_cpy.end(); it++ )
        {
          for ( auto i = 0u; i < it->second; i++ )
          {
            other_leaves_new.push_back( it->first );
          }
        }

        if ( params.max_gates == 0 || params.max_gates > net_without_leaves.num_gates() )
        {
          auto max_gates = params.max_gates > 0u ? params.max_gates - net_without_leaves.num_gates() : 0u;
          auto last_layers_partitions = partition_gen( last, max_counts, max_gates, params.max_num_fanout );

          for ( auto p : last_layers_partitions )
          {
            auto extensions = partition_ext( other, p, max_counts, params.max_num_fanout );
            for ( auto q : extensions )
            {
              auto temp = get_next_partial_dags( net_without_leaves, q, other_leaves_new );
              for ( auto&& r : temp )
              {
                r.num_levels++;
                result.push_back( r );
              }
            }
          }
        }
      }
    }

    return result;
  }

  /*! \brief Compute the partial DAGs obtained by combining the slots of `orig` as indicated by
   *  partitioning 'p'.
   */
  std::vector<PartialNtk> get_next_partial_dags( const PartialNtk& orig, const partition& p, const std::vector<int>& other_leaves )
  {
    auto max_allowed_of_fanin = params.max_gates_of_fanin;
    for ( auto it = orig.num_gates_of_fanin.begin(); it != orig.num_gates_of_fanin.end(); it++ )
    {
      assert( max_allowed_of_fanin[it->first] >= it->second );
      max_allowed_of_fanin[it->first] -= it->second;
    }

    std::vector<part> q( p.begin(), p.end() );

    /* For each part in partition 'p', consider gates of different number of fanins to connect. */
    auto res = add_node_recur( orig, q, 0, max_allowed_of_fanin );

    for ( auto&& net : res )
    {
      net.other_leaves = other_leaves;
    }

    return res;
  }

  /*! \brief Recursively consider gates with different number of fanins for different parts in partition `p`. */
  std::vector<PartialNtk> add_node_recur( const PartialNtk& orig, const std::vector<part>& p, uint32_t ind, std::unordered_map<uint32_t, uint32_t>& max_allowed_of_fanin )
  {
    if ( ind == p.size() )
    {
      return { orig };
    }

    std::vector<PartialNtk> res;

    /* Decide what fanin gate to use for part in partition 'p' at index 'ind'. */
    for ( auto&& fin : params.allowed_num_fanins )
    {
      if ( max_allowed_of_fanin[fin] == 0 )
      {
        continue;
      }
      max_allowed_of_fanin[fin]--;

      auto temp = add_node_recur( orig, p, ind + 1, max_allowed_of_fanin );

      for ( const auto& t : temp )
      {
        auto net = t.copy_with_last_layer_leaves();
        net.add_internal_node( fin, p[ind], true );
        res.push_back( net );
      }

      max_allowed_of_fanin[fin]++;
    }

    return res;
  }
};

#endif

} // namespace mockturtle