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
  \file window_view.hpp
  \brief Implements an isolated view on a window in a network

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "../utils/window_utils.hpp"
#include "immutable_view.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace mockturtle
{

/*! \brief Implements an isolated view on a window in a network.
 *
 * This view creates a network from a window in a large network.  The
 * window is specified by three parameters:
 *   1.) `inputs` are the common support of all window nodes, they do
 *       not overlap with `gates` (i.e., the intersection of `inputs` and
 *       `gates` is the empty set).
 *   2.) `gates` are the nodes in the window, supported by the
 *       `inputs` (i.e., `gates` are in the transitive fanout of the
 *       `inputs`,
 *   3.) `outputs` are signals (regular or complemented nodes)
 *        pointing to nodes in `gates` or `inputs`.  Not all fanouts
 *        of an output node are already part of the window.
 *
 * The second parameter `gates` has to be passed and is not
 * automatically computed (for example in contrast to `cut_view`),
 * because there are different strategies to construct a window from a
 * support set.  The outputs could be automatically computed.
 *
 * The window_view implements three new API methods:
 *   1.) `belongs_to`: takes a node (or a signal) and returns true if and
 *       only if the corresponding node belongs to the window
 *   2.) `foreach_internal_fanout`: takes a node and invokes a predicate
         on all fanout nodes of the node that belong to the window
 *   3.) `foreach_external_fanout`: takes a node and invokes a predicate
         on all fanouts of the node that do not belong to the window
 */
template<typename Ntk>
class window_view : public immutable_view<Ntk>
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  explicit window_view( Ntk const& ntk, std::vector<node> const& inputs, std::vector<signal> const& outputs, std::vector<node> const& gates )
      : immutable_view<Ntk>( ntk ), _inputs( inputs ), _outputs( outputs )
  {
    construct( inputs, gates );
  }

  explicit window_view( Ntk const& ntk, std::vector<node> const& inputs, std::vector<node> const& outputs, std::vector<node> const& gates )
      : immutable_view<Ntk>( ntk ), _inputs( inputs )
  {
    construct( inputs, gates );

    /* convert output nodes to signals */
    std::transform( std::begin( outputs ), std::end( outputs ), std::back_inserter( _outputs ),
                    [this]( node const& n ) {
                      return this->make_signal( n );
                    } );
  }

#pragma region Window
  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  inline bool belongs_to( signal const& s ) const
  {
    return std::find( std::begin( _nodes ), std::end( _nodes ), get_node( s ) ) != std::end( _nodes );
  }

  inline bool belongs_to( node const& n ) const
  {
    return std::find( std::begin( _nodes ), std::end( _nodes ), n ) != std::end( _nodes );
  }
#pragma endregion

#pragma region Structural properties
  inline uint32_t size() const
  {
    return static_cast<uint32_t>( _nodes.size() );
  }

  inline uint32_t num_cis() const
  {
    return num_pis();
  }

  inline uint32_t num_cos() const
  {
    return num_pos();
  }

  inline uint32_t num_pis() const
  {
    return static_cast<uint32_t>( _inputs.size() );
  }

  inline uint32_t num_pos() const
  {
    return static_cast<uint32_t>( _outputs.size() );
  }

  inline uint32_t num_registers() const
  {
    return 0u;
  }

  inline uint32_t num_gates() const
  {
    return static_cast<uint32_t>( _nodes.size() - _inputs.size() - 1u );
  }

  inline uint32_t fanout_size( node const& n ) const = delete;

  inline uint32_t node_to_index( node const& n ) const
  {
    return _node_to_index.at( n );
  }

  inline node index_to_node( uint32_t index ) const
  {
    return _nodes[index];
  }

  inline bool is_pi( node const& n ) const
  {
    return std::find( std::begin( _inputs ), std::end( _inputs ), n ) != std::end( _inputs );
  }

  inline bool is_ci( node const& n ) const
  {
    return is_pi( n );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _outputs.size() );
    return *( std::begin( _outputs ) + index );
  }

  signal co_at( uint32_t index ) const
  {
    return po_at( index );
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( std::begin( _inputs ), std::end( _inputs ), fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( std::begin( _outputs ), std::end( _outputs ), fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    foreach_pi( fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    foreach_po( fn );
  }

  template<typename Fn>
  void foreach_ro( Fn&& fn ) const
  {
    (void)fn;
  }

  template<typename Fn>
  void foreach_ri( Fn&& fn ) const
  {
    (void)fn;
  }

  template<typename Fn>
  void foreach_register( Fn&& fn ) const
  {
    (void)fn;
  }

  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    detail::foreach_element( std::begin( _nodes ), std::end( _nodes ), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    detail::foreach_element( std::begin( _nodes ) + 1u + _inputs.size(), std::end( _nodes ), fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    /* constants and inputs do not have fanins */
    if ( this->is_constant( n ) ||
         std::find( std::begin( _inputs ), std::end( _inputs ), n ) != std::end( _inputs ) )
    {
      return;
    }

    /* if it's not a window input, the node has to be a window node */
    assert( std::find( std::begin( _nodes ) + 1 + _inputs.size(), std::end( _nodes ), n ) != std::end( _nodes ) );
    immutable_view<Ntk>::foreach_fanin( n, fn );
  }

  template<typename Fn>
  void foreach_internal_fanout( node const& n, Fn&& fn ) const
  {
    this->foreach_fanout( n, [&]( node const& fo ) {
      if ( tbelongs_to( fo ) )
      {
        fn( fo );
      }
    } );
  }

  template<typename Fn>
  void foreach_external_fanout( node const& n, Fn&& fn ) const
  {
    this->foreach_fanout( n, [&]( node const& fo ) {
      if ( !belongs_to( fo ) )
      {
        fn( fo );
      }
    } );
  }
#pragma endregion

protected:
  void construct( std::vector<node> const& inputs, std::vector<node> const& gates )
  {
    /* copy constant to nodes */
    _nodes.emplace_back( this->get_node( this->get_constant( false ) ) );

    /* copy inputs to nodes */
    std::copy( std::begin( inputs ), std::end( inputs ), std::back_inserter( _nodes ) );

    /* copy gates to nodes */
    std::copy( std::begin( gates ), std::end( gates ), std::back_inserter( _nodes ) );

    /* create a mapping from node id (index in the original network) to window index */
    for ( uint32_t index = 0; index < _nodes.size(); ++index )
    {
      _node_to_index[_nodes.at( index )] = index;
    }
  }

protected:
  std::vector<node> _inputs;
  std::vector<signal> _outputs;
  std::vector<node> _nodes;
  std::unordered_map<node, uint32_t> _node_to_index;
}; /* window_view */

} /* namespace mockturtle */