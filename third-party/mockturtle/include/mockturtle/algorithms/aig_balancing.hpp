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
  \file aig_balancing.hpp
  \brief Balances the AIG to reduce the depth

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <vector>
#include <random>
#include <algorithm>

#include "cleanup.hpp"
#include "../networks/aig.hpp"
#include "../traits.hpp"
#include "../views/depth_view.hpp"
#include "../views/fanout_view.hpp"

namespace mockturtle
{

struct aig_balancing_params
{
  /*! \brief Minimizes the number of levels. */
  bool minimize_levels{ true };

  /*! \brief Use fast version, it may not find some area optimizations. */
  bool fast_mode{ true };
};

namespace detail
{

template<class Ntk>
class aig_balance_impl
{
public:
  static constexpr size_t storage_init_size = 30;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using storage_t = std::vector<std::vector<signal>>;

public:
  aig_balance_impl( Ntk& ntk, aig_balancing_params const& ps )
      : ntk( ntk ), ps( ps ), storage( storage_init_size )
  {
  }

  void run()
  {
    ntk.clear_values();

    for ( auto i = 0; i < storage_init_size; ++i )
      storage[i].reserve( 10 );

    /* balance every CO */
    ntk.foreach_co( [&]( auto const& f ) {
      balance_rec( ntk.get_node( f ), 0 );
    } );
  }

private:
  signal balance_rec( node const& n, uint32_t level )
  {
    if ( ntk.is_ci( n ) )
      return ntk.make_signal( n );

    /* node has been replaced in a previous recursion */
    if ( ntk.is_dead( n ) || ntk.value( n ) > 0 )
    {
      return ntk.make_signal( find_substituted_node( n ) );
    }

    if ( level >= storage.size() )
    {
      storage.emplace_back( std::vector<signal>() );
      storage.back().reserve( 10 );
    }

    /* collect leaves of the AND tree */
    collect_leaves( n, storage[level] );

    if ( storage[level].size() == 0 )
    {
      ntk.substitute_node( n, ntk.get_constant( false ) );
      return ntk.get_constant( false );
    }

    /* recur over the leaves */
    for ( auto& f : storage[level] )
    {
      signal new_signal = balance_rec( ntk.get_node( f ), level + 1 );
      f = new_signal ^ ntk.is_complemented( f );
    }

    assert( storage[level].size() > 1 );

    /* sort by decreasing level */
    std::stable_sort( storage[level].begin(), storage[level].end(), [this]( auto const& a, auto const& b ) {
      return ntk.level( ntk.get_node( a ) ) > ntk.level( ntk.get_node( b ) );
    } );

    /* mark TFI cone of n */
    ntk.incr_trav_id();
    mark_tfi( ntk.make_signal( n ), true );

    /* generate the AND tree */
    while ( storage[level].size() > 1 )
    {
      /* explore multiple possibilities to find logic sharing */
      if ( ps.fast_mode )
      {
        if ( ps.minimize_levels )
          pick_nodes_fast( storage[level], find_left_most_at_level( storage[level] ) );
        else
          pick_nodes_area_fast( storage[level] );
      }
      else
      {
        if ( ps.minimize_levels )
          pick_nodes( storage[level], find_left_most_at_level( storage[level] ) );
        else
          pick_nodes_area( storage[level] );
      }

      /* pop the two selected nodes to create the new AND gate */
      signal child1 = storage[level].back();
      storage[level].pop_back();
      signal child2 = storage[level].back();
      storage[level].pop_back();
      signal new_sig = ntk.create_and( child1, child2 );

      /* update level for AND node */
      update_level( ntk.get_node( new_sig ) );

      /* insert the new node back */
      insert_node_sorted( storage[level], new_sig );
    }

    signal root = storage[level][0];

    /* replace if new */
    if ( n != ntk.get_node( root ) )
    {
      ntk.substitute_node_no_restrash( n, root );
    }

    /* remember the substitution and the new node as already balanced */
    ntk.set_value( n, ntk.node_to_index( ntk.get_node( root ) ) );
    ntk.set_value( ntk.get_node( root ), ntk.node_to_index( ntk.get_node( root ) ) );

    /* clean leaves storage */
    storage[level].clear();

    return root;
  }

  void collect_leaves( node const& n, std::vector<signal>& leaves )
  {
    ntk.incr_trav_id();

    int ret = collect_leaves_rec( ntk.make_signal( n ), leaves, true );

    /* check for constant false */
    if ( ret < 0 )
    {
      leaves.clear();
    }
  }

  int collect_leaves_rec( signal const& f, std::vector<signal>& leaves, bool is_root )
  {
    node n = ntk.get_node( f );

    /* check if already visited */
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      for ( signal const& s : leaves )
      {
        if ( ntk.get_node( s ) != n )
          continue;

        if ( s == f )
          return 1;   /* same polarity: duplicate */
        else
          return -1;  /* opposite polarity: const0 */
      }

      return 0;
    }

    /* set as leaf if signal is complemented or is a CI or has a multiple fanout */
    if ( !is_root && ( ntk.is_complemented( f ) || ntk.is_ci( n ) || ntk.fanout_size( n ) > 1 ) )
    {
      leaves.push_back( f );
      ntk.set_visited( n, ntk.trav_id() );
      return 0;
    }

    int ret = 0;
    ntk.foreach_fanin( n, [&]( auto const& child ) {
      ret |= collect_leaves_rec( child, leaves, false );
    } );

    return ret;
  }

  size_t find_left_most_at_level( std::vector<signal> const& leaves )
  {
    size_t pointer = leaves.size() - 1;
    uint32_t current_level = ntk.level( ntk.get_node( leaves[leaves.size() - 2] ) );

    while ( pointer > 0 )
    {
      if ( ntk.level( ntk.get_node( leaves[pointer - 1] ) ) > current_level )
        break;

      --pointer;
    }

    assert( ntk.level( ntk.get_node( leaves[pointer] ) ) == current_level );
    return pointer;
  }

  inline void pick_nodes( std::vector<signal>& leaves, size_t left_most )
  {
    size_t right_most = leaves.size() - 2;

    if ( ntk.level( ntk.get_node( leaves[leaves.size() - 1] ) ) == ntk.level( ntk.get_node( leaves[leaves.size() - 2] ) ) )
      right_most = left_most;

    for ( size_t right_pointer = leaves.size() - 1; right_pointer > right_most; --right_pointer )
    {
      assert( left_most < right_pointer );

      size_t left_pointer = right_pointer;
      while ( left_pointer-- > left_most )
      {
        /* select if node exists */
        std::optional<signal> pnode = ntk.has_and( leaves[right_pointer], leaves[left_pointer] );
        if ( pnode.has_value() )
        {
          /* already present in TFI */
          if ( ntk.visited( ntk.get_node( *pnode ) ) == ntk.trav_id() )
          {
            continue;
          }

          if ( leaves[right_pointer] != leaves[leaves.size() - 1] )
            std::swap( leaves[right_pointer], leaves[leaves.size() - 1] );
          if ( leaves[left_pointer] != leaves[leaves.size() - 2] )
            std::swap( leaves[left_pointer], leaves[leaves.size() - 2] );
          break;
        }
      }
    }
  }

  inline void pick_nodes_fast( std::vector<signal>& leaves, size_t left_most )
  {
    size_t left_pointer = leaves.size() - 1;
    while ( left_pointer-- > left_most )
    {
      /* select if node exists */
      std::optional<signal> pnode = ntk.has_and( leaves.back(), leaves[left_pointer] );
      if ( pnode.has_value() )
      {
        /* already present in TFI */
        if ( ntk.visited( ntk.get_node( *pnode ) ) == ntk.trav_id() )
        {
          continue;
        }

        if ( leaves[left_pointer] != leaves[leaves.size() - 2] )
          std::swap( leaves[left_pointer], leaves[leaves.size() - 2] );
        break;
      }
    }
  }

  inline void pick_nodes_area( std::vector<signal>& leaves )
  {
    for ( size_t right_pointer = leaves.size() - 1; right_pointer > 0; --right_pointer )
    {
      size_t left_pointer = right_pointer;
      while ( left_pointer-- > 0 )
      {
        /* select if node exists */
        std::optional<signal> pnode = ntk.has_and( leaves[right_pointer], leaves[left_pointer] );
        if ( pnode.has_value() )
        {
          /* already present in TFI */
          if ( ntk.visited( ntk.get_node( *pnode ) ) == ntk.trav_id() )
          {
            continue;
          }

          if ( leaves[right_pointer] != leaves[leaves.size() - 1] )
            std::swap( leaves[right_pointer], leaves[leaves.size() - 1] );
          if ( leaves[left_pointer] != leaves[leaves.size() - 2] )
            std::swap( leaves[left_pointer], leaves[leaves.size() - 2] );
          break;
        }
      }
    }
  }

  inline void pick_nodes_area_fast( std::vector<signal>& leaves )
  {
    size_t left_pointer = leaves.size() - 1;
    while ( left_pointer-- > 0 )
    {
      /* select if node exists */
      std::optional<signal> pnode = ntk.has_and( leaves.back(), leaves[left_pointer] );
      if ( pnode.has_value() )
      {
        /* already present in TFI */
        if ( ntk.visited( ntk.get_node( *pnode ) ) == ntk.trav_id() )
        {
          continue;
        }

        if ( leaves[left_pointer] != leaves[leaves.size() - 2] )
          std::swap( leaves[left_pointer], leaves[leaves.size() - 2] );
        break;
      }
    }
  }

  void insert_node_sorted( std::vector<signal>& leaves, signal const& f )
  {
    node n = ntk.get_node( f );

    /* check uniqueness */
    for ( auto const& s : leaves )
    {
      if ( s == f )
        return;
    }

    leaves.push_back( f );
    for ( size_t i = leaves.size() - 1; i > 0; --i )
    {
      auto& s2 = leaves[i - 1];

      if ( ntk.level( ntk.get_node( s2 ) ) < ntk.level( n ) )
      {
        std::swap( s2, leaves[i] );
      }
      else
      {
        break;
      }
    }
  }

  void update_level( node const& n )
  {
    uint32_t l = 0;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      l = std::max( l, ntk.level( ntk.get_node( f ) ) );
    } );

    ntk.set_level( n, l + 1 );
  }

  node find_substituted_node( node n )
  {
    while ( ntk.is_dead( n ) )
      n = ntk.index_to_node( ntk.value( n ) );
    
    return n;
  }

  void mark_tfi( signal const& f, bool is_root )
  {
    node n = ntk.get_node( f );

    /* check if already visited */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;

    ntk.set_visited( n, ntk.trav_id() );

    /* set as leaf if signal is complemented or is a CI or has a multiple fanout */
    if ( !is_root && ( ntk.is_complemented( f ) || ntk.is_ci( n ) || ntk.fanout_size( n ) > 1 ) )
    {
      return;
    }

    ntk.foreach_fanin( n, [&]( auto const& child ) {
      mark_tfi( child, false );
    } );
  }

private:
  Ntk& ntk;
  aig_balancing_params const& ps;

  storage_t storage;
};

} /* namespace detail */

/*! \brief AIG balancing.
 *
 * This method balance the AIG to reduce the
 * depth. Level minimization can be turned off.
 * In this case, balancing tries to reconstruct
 * AND trees such that logic sharing is maximized.
 *
 * **Required network functions:**
 * - `get_node`
 * - `node_to_index`
 * - `get_constant`
 * - `create_pi`
 * - `create_po`
 * - `create_not`
 * - `is_complemented`
 * - `foreach_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `clone_node`
 * - `is_pi`
 * - `is_constant`
 * - `has_and`
 */
template<class Ntk>
void aig_balance( Ntk& ntk, aig_balancing_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_clone_node_v<Ntk>, "Ntk does not implement the clone_node method" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_has_and_v<Ntk>, "Ntk does not implement the has_and method" );

  fanout_view<Ntk> f_ntk{ ntk };
  depth_view<fanout_view<Ntk>> d_ntk{ f_ntk };

  detail::aig_balance_impl p( d_ntk, ps );
  p.run();

  ntk = cleanup_dangling( ntk );
}

} /* namespace mockturtle */