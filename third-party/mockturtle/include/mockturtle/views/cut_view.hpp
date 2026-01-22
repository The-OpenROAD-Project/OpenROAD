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
  \file cut_view.hpp
  \brief Implements an isolated view on a single cut in a network

  \author Bruno Schmitt
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <parallel_hashmap/phmap.h>

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "immutable_view.hpp"

namespace mockturtle
{

/*! \brief Implements an isolated view on a single cut in a network.
 *
 * This view can create a network from a single cut in a largest network.  This
 * cut has a single output `root` and set of `leaves`.  The view reimplements
 * the methods `size`, `num_pis`, `num_pos`, `foreach_pi`, `foreach_po`,
 * `foreach_node`, `foreach_gate`, `is_pi`, `node_to_index`, and
 * `index_to_node`.
 *
 * This view assumes that all nodes' visited flags are set 0 before creating
 * the view.  The view guarantees that all the nodes in the view will have a 0
 * visited flag after the construction.
 *
 * **Required network functions:**
 * - `set_visited`
 * - `visited`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `make_signal`
 */
template<typename Ntk>
class cut_view : public immutable_view<Ntk>
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  static constexpr bool is_topologically_sorted = true;

public:
  explicit cut_view( Ntk const& ntk, std::vector<node> const& leaves, signal const& root )
      : immutable_view<Ntk>( ntk ), _root( root )
  {
    construct( leaves );
  }

  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  explicit cut_view( Ntk const& ntk, std::vector<signal> const& leaves, signal const& root )
      : immutable_view<Ntk>( ntk ), _root( root )
  {
    construct( leaves );
  }

private:
  template<typename LeaveType>
  void construct( std::vector<LeaveType> const& leaves )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_trav_id_v<Ntk>, "Ntk does not implement the trav_id method" );
    static_assert( std::is_same_v<LeaveType, node> || std::is_same_v<LeaveType, signal>, "leaves must be vector of either node or signal" );

    this->incr_trav_id();

    /* constants */
    add_constants();

    /* primary inputs */
    for ( auto const& leaf : leaves )
    {
      if constexpr ( std::is_same_v<LeaveType, node> )
      {
        add_leaf( leaf );
      }
      else
      {
        add_leaf( this->get_node( leaf ) );
      }
    }

    traverse( this->get_node( _root ) );

    /* restore visited */
    // for ( auto const& n : _nodes )
    //{
    //   this->set_visited( n, 0 );
    // }
  }

public:
  inline auto size() const { return _nodes.size(); }
  inline auto num_pis() const { return _num_leaves; }
  inline auto num_pos() const { return 1; }
  inline auto num_gates() const { return _nodes.size() - _num_leaves - _num_constants; }

  inline auto node_to_index( const node& n ) const { return _node_to_index.at( n ); }
  inline auto index_to_node( uint32_t index ) const { return _nodes[index]; }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    std::vector<signal> roots{ { _root } };
    detail::foreach_element( roots.begin(), roots.end(), fn );
  }

  inline bool is_pi( node const& pi ) const
  {
    const auto beg = _nodes.begin() + _num_constants;
    return std::find( beg, beg + _num_leaves, pi ) != beg + _num_leaves;
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _nodes.begin() + _num_constants, _nodes.begin() + _num_constants + _num_leaves, fn );
  }

  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    detail::foreach_element( _nodes.begin(), _nodes.end(), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    detail::foreach_element( _nodes.begin() + _num_constants + _num_leaves, _nodes.end(), fn );
  }

private:
  inline void add_constants()
  {
    add_node( this->get_node( this->get_constant( false ) ) );
    this->set_visited( this->get_node( this->get_constant( false ) ), this->trav_id() );
    if ( this->get_node( this->get_constant( true ) ) != this->get_node( this->get_constant( false ) ) )
    {
      add_node( this->get_node( this->get_constant( true ) ) );
      this->set_visited( this->get_node( this->get_constant( true ) ), this->trav_id() );
      ++_num_constants;
    }
  }

  inline void add_leaf( node const& leaf )
  {
    if ( this->visited( leaf ) == this->trav_id() )
      return;

    add_node( leaf );
    this->set_visited( leaf, this->trav_id() );
    ++_num_leaves;
  }

  inline void add_node( node const& n )
  {
    _node_to_index[n] = static_cast<uint32_t>( _nodes.size() );
    _nodes.push_back( n );
  }

  void traverse( node const& n )
  {
    if ( this->visited( n ) == this->trav_id() )
      return;

    this->foreach_fanin( n, [&]( const auto& f ) {
      traverse( this->get_node( f ) );
    } );

    add_node( n );
    this->set_visited( n, this->trav_id() );
  }

public:
  unsigned _num_constants{ 1 };
  unsigned _num_leaves{ 0 };
  std::vector<node> _nodes;
  phmap::flat_hash_map<node, uint32_t> _node_to_index;
  signal _root;
};

template<class T>
cut_view( T const&, std::vector<node<T>> const&, signal<T> const& ) -> cut_view<T>;

template<class T, typename = std::enable_if_t<!std::is_same_v<typename T::signal, typename T::node>>>
cut_view( T const&, std::vector<signal<T>> const&, signal<T> const& ) -> cut_view<T>;

} /* namespace mockturtle */
