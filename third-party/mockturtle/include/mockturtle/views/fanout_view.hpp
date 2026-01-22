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
  \file fanout_view.hpp
  \brief Implements fanout for a network

  \author Alessandro Tempia Calvino
  \author Hanyu Wang
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include "../networks/detail/foreach.hpp"
#include "../networks/events.hpp"
#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "immutable_view.hpp"

#include <cstdint>
#include <stack>
#include <vector>

namespace mockturtle
{

struct fanout_view_params
{
  bool update_on_add{ true };
  bool update_on_modified{ true };
  bool update_on_delete{ true };
};

/*! \brief Implements `foreach_fanout` methods for networks.
 *
 * This view computes the fanout of each node of the network.
 * It implements the network interface method `foreach_fanout`.  The
 * fanout are computed at construction and can be recomputed by
 * calling the `update_fanout` method.
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_fanin`
 *
 */
template<typename Ntk, bool has_fanout_interface = has_foreach_fanout_v<Ntk>>
class fanout_view
{
};

template<typename Ntk>
class fanout_view<Ntk, true> : public Ntk
{
public:
  fanout_view( Ntk const& ntk, fanout_view_params const& ps = {} ) : Ntk( ntk )
  {
    (void)ps;
  }
};

template<typename Ntk>
class fanout_view<Ntk, false> : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit fanout_view( fanout_view_params const& ps = {} )
      : Ntk(), _fanout( *this ), _ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_fanout();

    register_events();
  }

  explicit fanout_view( Ntk const& ntk, fanout_view_params const& ps = {} )
      : Ntk( ntk ), _fanout( ntk ), _ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );

    update_fanout();

    register_events();
  }

  /*! \brief Copy constructor. */
  fanout_view( fanout_view<Ntk, false> const& other )
      : Ntk( other ), _fanout( other._fanout ), _ps( other._ps )
  {
    register_events();
  }

  fanout_view<Ntk, false>& operator=( fanout_view<Ntk, false> const& other )
  {
    release_events();

    /* update the base class */
    this->_storage = other._storage;
    this->_events = other._events;

    /* copy */
    _ps = other._ps;
    _fanout = other._fanout;

    register_events();

    return *this;
  }

  ~fanout_view()
  {
    release_events();
  }

  template<typename Fn>
  void foreach_fanout( node const& n, Fn&& fn ) const
  {
    assert( n < this->size() );
    detail::foreach_element( _fanout[n].begin(), _fanout[n].end(), fn );
  }

  void update_fanout()
  {
    compute_fanout();
  }

  std::vector<node> fanout( node const& n ) const /* deprecated */
  {
    return _fanout[n];
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    if ( Ntk::get_node( new_signal ) == old_node && !Ntk::is_complemented( new_signal ) )
      return;

    if ( Ntk::is_dead( Ntk::get_node( new_signal ) ) )
    {
      Ntk::revive_node( Ntk::get_node( new_signal ) );
    }

    std::unordered_map<node, signal> old_to_new;
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _curr] = to_substitute.top();
      to_substitute.pop();

      signal _new = _curr;
      /* find the real new node */
      if ( Ntk::is_dead( Ntk::get_node( _new ) ) )
      {
        auto it = old_to_new.find( Ntk::get_node( _new ) );
        while ( it != old_to_new.end() )
        {
          _new = Ntk::is_complemented( _new ) ? Ntk::create_not( it->second ) : it->second;
          it = old_to_new.find( Ntk::get_node( _new ) );
        }
      }
      /* revive */
      if ( Ntk::is_dead( Ntk::get_node( _new ) ) )
      {
        Ntk::revive_node( Ntk::get_node( _new ) );
      }

      if ( Ntk::get_node( _new ) == _old && !Ntk::is_complemented( _new ) )
        continue;

      const auto parents = _fanout[_old];
      for ( auto n : parents )
      {
        if ( const auto repl = Ntk::replace_in_node( n, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      Ntk::replace_in_outputs( _old, _new );

      /* reset fan-in of old node */
      if ( _old != Ntk::get_node( _new ) ) /* substitute a node using itself*/
      {
        old_to_new.insert( { _old, _new } );
        Ntk::take_out_node( _old );
      }
    }
  }

  void substitute_node_no_restrash( node const& old_node, signal const& new_signal )
  {
    if ( Ntk::get_node( new_signal ) == old_node && !Ntk::is_complemented( new_signal ) )
      return;

    if ( Ntk::is_dead( Ntk::get_node( new_signal ) ) )
    {
      Ntk::revive_node( Ntk::get_node( new_signal ) );
    }

    const auto parents = _fanout[old_node];
    for ( auto n : parents )
    {
      Ntk::replace_in_node_no_restrash( n, old_node, new_signal );
    }

    /* check outputs */
    Ntk::replace_in_outputs( old_node, new_signal );

    /* recursively reset old node */
    if ( old_node != new_signal.index )
    {
      Ntk::take_out_node( old_node );
    }
  }

private:
  void register_events()
  {
    if ( _ps.update_on_add )
    {
      add_event = Ntk::events().register_add_event( [this]( auto const& n ) {
        _fanout.resize();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].push_back( n );
        } );
      } );
    }

    if ( _ps.update_on_modified )
    {
      modified_event = Ntk::events().register_modified_event( [this]( auto const& n, auto const& previous ) {
        (void)previous;
        for ( auto const& f : previous )
        {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        }
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].push_back( n );
        } );
      } );
    }

    if ( _ps.update_on_delete )
    {
      delete_event = Ntk::events().register_delete_event( [this]( auto const& n ) {
        _fanout[n].clear();
        Ntk::foreach_fanin( n, [&, this]( auto const& f ) {
          _fanout[f].erase( std::remove( _fanout[f].begin(), _fanout[f].end(), n ), _fanout[f].end() );
        } );
      } );
    }
  }

  void release_events()
  {
    if ( add_event )
    {
      Ntk::events().release_add_event( add_event );
    }

    if ( modified_event )
    {
      Ntk::events().release_modified_event( modified_event );
    }

    if ( delete_event )
    {
      Ntk::events().release_delete_event( delete_event );
    }
  }

  void compute_fanout()
  {
    _fanout.reset();

    /* Compute fanout also for buffers in buffered networks */
    if constexpr ( is_buffered_network_type_v<Ntk> )
    {
      this->foreach_node( [&]( auto const& n ) {
        if ( this->is_pi( n ) || this->is_constant( n ) )
          return true;
        this->foreach_fanin( n, [&]( auto const& c ) {
          auto& fanout = _fanout[c];
          if ( std::find( fanout.begin(), fanout.end(), n ) == fanout.end() )
          {
            fanout.push_back( n );
          }
        } );
        return true;
      } );
    }
    else
    {
      this->foreach_gate( [&]( auto const& n ) {
        this->foreach_fanin( n, [&]( auto const& c ) {
          auto& fanout = _fanout[c];
          if ( std::find( fanout.begin(), fanout.end(), n ) == fanout.end() )
          {
            fanout.push_back( n );
          }
        } );
      } );
    }
  }

  node_map<std::vector<node>, Ntk> _fanout;
  fanout_view_params _ps;

  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;
  std::shared_ptr<typename network_events<Ntk>::modified_event_type> modified_event;
  std::shared_ptr<typename network_events<Ntk>::delete_event_type> delete_event;
};

template<class T>
fanout_view( T const&, fanout_view_params const& ps = {} ) -> fanout_view<T>;

} // namespace mockturtle