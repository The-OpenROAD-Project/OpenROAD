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
  \file cost_view.hpp
  \brief Implements various cost estimation methods for a network

  \author Hanyu Wang
*/

#pragma once

#include "../networks/events.hpp"
#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../utils/recursive_cost_functions.hpp"
#include "immutable_view.hpp"

#include <cstdint>
#include <vector>

namespace mockturtle
{

/*! \brief Implements `get_cost` methods for networks.
 *
 * This view computes the cost of the entire network, a subnetwork, and
 * also fanin cone of a single node. It maintains the context of each
 * node, which is the aggregated variables that affect the cost a node.
 *
 * **Required network functions:**
 * - `size`
 * - `get_node`
 * - `visited`
 * - `set_visited`
 * - `foreach_fanin`
 * - `foreach_po`
 *
 * Example
 *
   \verbatim embed:rst

   .. code-block:: c++

      // create network somehow
      xag_network xag = ...;

      // create a cost view on the network, for example size cost
      auto viewed = cost_view( xag, xag_size_cost_function<xag_network>() );

      // print size
      std::cout << "size: " << viewed.get_cost() << "\n";
   \endverbatim
 */
template<class Ntk, class RecCostFn>
class cost_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using context_t = typename RecCostFn::context_t;

  explicit cost_view( RecCostFn const& cost_fn = {} )
      : Ntk(),
        _cost_fn( cost_fn ),
        context( *this )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );

    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
  }

  explicit cost_view( Ntk const& ntk, RecCostFn const& cost_fn = {} )
      : Ntk( ntk ),
        _cost_fn( cost_fn ),
        context( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );

    update_cost();
    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
  }

  explicit cost_view( cost_view<Ntk, RecCostFn> const& other )
      : Ntk( other ),
        _cost_fn( other._cost_fn ),
        context( other.context )
  {
    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
  }

  cost_view<Ntk, RecCostFn>& operator=( cost_view<Ntk, RecCostFn> const& other )
  {
    /* delete the event of this network */
    Ntk::events().release_add_event( add_event );

    /* update the base class */
    this->_storage = other._storage;
    this->_events = other._events;

    /* copy */
    context = other.context;
    _cost_fn = other._cost_fn;

    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );

    return *this;
  }

  ~cost_view()
  {
    Ntk::events().release_add_event( add_event );
  }

  /*! \brief Returns the context of node n */
  context_t get_context( node const& n ) const
  {
    return context[n];
  }

  /*! \brief Assigns the context of node n */
  void set_context( node const& n, context_t cost_val )
  {
    context[n] = cost_val;
    this->set_visited( n, this->trav_id() );
  }

  /*! \brief Returns the cost of the entire network */
  uint32_t get_cost() const
  {
    return _cost;
  }

  /*! \brief Returns the cost of node n's fanin cone */
  uint32_t get_cost( node const& n )
  {
    uint32_t _c = 0u;
    this->incr_trav_id();
    compute_cost( n, _c );
    return _c;
  }

  /*! \brief Returns the cost between node n and divs */
  uint32_t get_cost( node const& n, std::vector<signal> const& divs )
  {
    uint32_t _c = 0u;
    this->incr_trav_id();
    for ( signal s : divs )
    {
      this->set_visited( this->get_node( s ), this->trav_id() );
    }
    compute_cost( n, _c );
    return _c;
  }

  /*! \brief Updates the context and cost of the entire network */
  void update_cost()
  {
    context.reset( context_t{} );
    this->incr_trav_id();
    compute_cost();
  }

  void on_add( node const& n )
  {
    context.resize();

    std::vector<context_t> fanin_costs;
    this->foreach_fanin( n, [&]( auto const& f ) {
      fanin_costs.emplace_back( context[this->get_node( f )] );
    } );
    context[n] = _cost_fn( *this, n, fanin_costs );
    _cost_fn( *this, n, _cost, context[n] );
  }

  /*! \brief Creates a PI with context assigned */
  signal create_pi( context_t pi_cotext )
  {
    signal s = Ntk::create_pi();
    context.resize();
    set_context( this->get_node( s ), pi_cotext );
    return s;
  }

  signal create_pi()
  {
    signal s = Ntk::create_pi();
    context.resize();
    return s;
  }

  void create_po( signal const& f )
  {
    Ntk::create_po( f );
  }

private:
  context_t compute_cost( node const& n, uint32_t& _c )
  {
    context_t _context{};
    if ( this->visited( n ) == this->trav_id() )
    {
      _context = context[n]; // do not update context
      _cost_fn( *this, n, _c, _context );
      return _context;
    }
    if ( this->is_constant( n ) )
    {
      _context = context[n] = context_t{};
    }
    else if ( this->is_pi( n ) )
    {
      _context = context[n] = _cost_fn( *this, n );
    }
    else
    {
      std::vector<context_t> fanin_costs;
      this->foreach_fanin( n, [&]( auto const& f ) {
        fanin_costs.emplace_back( compute_cost( this->get_node( f ), _c ) );
      } );
      _context = context[n] = _cost_fn( *this, n, fanin_costs );
    }
    _cost_fn( *this, n, _c, _context );
    this->set_visited( n, this->trav_id() );
    return _context;
  }
  void compute_cost()
  {
    _cost = 0u; /* must define the zero initialization */
    this->foreach_po( [&]( auto const& f ) {
      compute_cost( this->get_node( f ), _cost );
    } );
  }

  node_map<context_t, Ntk> context;
  uint32_t _cost;
  RecCostFn _cost_fn;

  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;
};

template<class T>
cost_view( T const& ) -> cost_view<T, typename T::RecCostFn>;

template<class T, class RecCostFn>
cost_view( T const&, RecCostFn const& ) -> cost_view<T, RecCostFn>;

} // namespace mockturtle