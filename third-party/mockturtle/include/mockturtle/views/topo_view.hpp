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
  \file topo_view.hpp
  \brief Reimplements foreach_node to guarantee topological order

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Mathias Soeken
  \author Max Austin
*/

#pragma once

#include <cassert>
#include <optional>
#include <vector>

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "immutable_view.hpp"

namespace mockturtle
{

/*! \brief Ensures topological order for of all nodes reachable from the outputs.
 *
 * Overrides the interface methods `foreach_node`, `foreach_gate`,
 * `size`, `num_gates`.
 *
 * This class computes *on construction* a topological order of the nodes which
 * are reachable from the outputs.  Constant nodes and primary inputs will also
 * be considered even if they are not reachable from the outputs.  Further,
 * constant nodes and primary inputs will be visited first before any gate node
 * is visited.  Constant nodes precede primary inputs, and primary inputs are
 * visited in the same order in which they were created.
 *
 * Since the topological order is computed only once when creating an instance,
 * this view disables changes to the network interface.  Also, since only
 * reachable nodes are traversed, not all network nodes may be called in
 * `foreach_node` and `foreach_gate`.
 *
 * **Required network functions:**
 * - `get_constant`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `incr_trav_id`
 * - `set_visited`
 * - `trav_id`
 * - `visited`
 *
 * Example
 *
   \verbatim embed:rst

   .. code-block:: c++

      // create network somehow; aig may not be in topological order
      aig_network aig = ...;

      // create a topological view on the network
      topo_view aig_topo{aig};

      // call algorithm that requires topological order
      cut_enumeration( aig_topo );
   \endverbatim
 */
template<class Ntk, bool sorted = is_topologically_sorted_v<Ntk>>
class topo_view
{
};

template<typename Ntk>
class topo_view<Ntk, false> : public immutable_view<Ntk>
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  static constexpr bool is_topologically_sorted = true;

  /*! \brief Default constructor.
   *
   * Constructs topological view on another network.
   */
  topo_view( Ntk const& ntk ) : immutable_view<Ntk>( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_trav_id_v<Ntk>, "Ntk does not implement the trav_id method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );

    update_topo();
  }

  /*! \brief Default constructor.
   *
   * Constructs topological view, but only for the transitive fan-in starting
   * from a given start signal.
   */
  topo_view( Ntk const& ntk, typename Ntk::signal const& start_signal )
      : immutable_view<Ntk>( ntk ),
        start_signal( start_signal )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_trav_id_v<Ntk>, "Ntk does not implement the trav_id method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );

    update_topo();
  }

  /*! \brief Reimplementation of `size`. */
  auto size() const
  {
    return static_cast<uint32_t>( topo_order.size() );
  }

  /*! \brief Reimplementation of `num_gates`. */
  auto num_gates() const
  {
    uint32_t const offset = 1u + this->num_pis() + ( this->get_node( this->get_constant( true ) ) != this->get_node( this->get_constant( false ) ) );
    return static_cast<uint32_t>( topo_order.size() - offset );
  }

  /*! \brief Reimplementation of `node_to_index`. */
  uint32_t node_to_index( node const& n ) const
  {
    return std::distance( std::begin( topo_order ), std::find( std::begin( topo_order ), std::end( topo_order ), n ) );
  }

  /*! \brief Reimplementation of `index_to_node`. */
  node index_to_node( uint32_t index ) const
  {
    return topo_order.at( index );
  }

  /*! \brief Reimplementation of `foreach_node`. */
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    detail::foreach_element( topo_order.begin(),
                             topo_order.end(),
                             fn );
  }

  /*! \brief Implementation of `foreach_node` in reverse topological order. */
  template<typename Fn>
  void foreach_node_reverse( Fn&& fn ) const
  {
    detail::foreach_element( topo_order.rbegin(),
                             topo_order.rend(),
                             fn );
  }

  /*! \brief Reimplementation of `foreach_gate`. */
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    uint32_t const offset = 1u + this->num_pis() + ( this->get_node( this->get_constant( true ) ) != this->get_node( this->get_constant( false ) ) );
    detail::foreach_element( topo_order.begin() + offset,
                             topo_order.end(),
                             fn );
  }

  /*! \brief Implementation of `foreach_gate` in reverse topological order. */
  template<typename Fn>
  void foreach_gate_reverse( Fn&& fn ) const
  {
    uint32_t const offset = 1u + this->num_pis() + ( this->get_node( this->get_constant( true ) ) != this->get_node( this->get_constant( false ) ) );
    detail::foreach_element( topo_order.rbegin(),
                             topo_order.rend() - offset,
                             fn );
  }

  /*! \brief Reimplementation of `foreach_po`.
   *
   * If `start_signal` is provided in constructor, only this is returned as
   * primary output, otherwise reverts to original `foreach_po` implementation.
   */
  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    if ( start_signal )
    {
      std::vector<signal> signals( 1, *start_signal );
      detail::foreach_element( signals.begin(), signals.end(), fn );
    }
    else
    {
      Ntk::foreach_po( fn );
    }
  }

  uint32_t num_pos() const
  {
    return start_signal ? 1 : Ntk::num_pos();
  }

  void update_topo()
  {
    this->incr_trav_id();
    this->incr_trav_id();
    topo_order.reserve( this->size() );

    /* constants and PIs */
    const auto c0 = this->get_node( this->get_constant( false ) );
    topo_order.push_back( c0 );
    this->set_visited( c0, this->trav_id() );

    if ( const auto c1 = this->get_node( this->get_constant( true ) ); this->visited( c1 ) != this->trav_id() )
    {
      topo_order.push_back( c1 );
      this->set_visited( c1, this->trav_id() );
    }

    this->foreach_ci( [this]( auto n ) {
      if ( this->visited( n ) != this->trav_id() )
      {
        topo_order.push_back( n );
        this->set_visited( n, this->trav_id() );
      }
    } );

    if ( start_signal )
    {
      if ( this->visited( this->get_node( *start_signal ) ) == this->trav_id() )
        return;
      create_topo_rec( this->get_node( *start_signal ) );
    }
    else
    {
      Ntk::foreach_co( [this]( auto f ) {
        /* node was already visited */
        if ( this->visited( this->get_node( f ) ) == this->trav_id() )
          return;

        create_topo_rec( this->get_node( f ) );
      } );
    }
  }

private:
  void create_topo_rec( node const& n )
  {
    /* is permanently marked? */
    if ( this->visited( n ) == this->trav_id() )
      return;

    /* ensure that the node is not temporarily marked */
    assert( this->visited( n ) != this->trav_id() - 1 );

    /* mark node temporarily */
    this->set_visited( n, this->trav_id() - 1 );

    /* mark children */
    this->foreach_fanin( n, [this]( signal const& f ) {
      create_topo_rec( this->get_node( f ) );
    } );

    /* mark node n permanently */
    this->set_visited( n, this->trav_id() );

    /* visit node */
    topo_order.push_back( n );
  }

private:
  std::vector<node> topo_order;
  std::optional<signal> start_signal;
};

template<typename Ntk>
class topo_view<Ntk, true> : public Ntk
{
public:
  topo_view( Ntk const& ntk ) : Ntk( ntk )
  {
  }
};

template<class T>
topo_view( T const& ) -> topo_view<T>;

template<class T>
topo_view( T const&, typename T::signal const& ) -> topo_view<T>;

} // namespace mockturtle