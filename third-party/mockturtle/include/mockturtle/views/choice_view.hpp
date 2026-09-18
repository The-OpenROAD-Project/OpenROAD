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
  \file choice_view.hpp
  \brief Implement choices in network

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <cassert>
#include <optional>
#include <utility>
#include <vector>

#include "../networks/detail/foreach.hpp"
#include "../networks/events.hpp"
#include "../traits.hpp"

namespace mockturtle
{

struct choice_view_params
{
  bool add_choices_on_substitute{ true };
  bool update_on_add{ true };
};

/*! \brief Implements choices in network
 *
 * Overrides the interface methods `substitute_node`, `incr_fanout_size`,
 * `decr_fanout_size`, `fanout_size`.
 *
 * This class manages equivalent nodes keeping them saved as alternatives in
 * the network. Each node belongs to an equivalence class in which a node
 * is the class representative, by default the one with the lowest index.
 * Equivalence classes are saved as linked lists. The `_choice_repr` vector
 * associates each node to the next one in the linked list (the one closer
 * to the representative). The representative is the tail and "points" at itself.
 * The `_choice_phase` vector is used to save the polarity of each node in the
 * class with respect to the representative. The representative uses its field
 * to point to the head of the list.
 *
 * This view is not compatible with `fanout_view`.
 *
 * **Required network functions:**
 * - `get_node`
 * - `size`
 * - `node_to_index`
 * - `index_to_node`
 * - `is_complemented`
 * - `make_signal`
 */
template<typename Ntk, bool has_choice_interface = has_foreach_choice_v<Ntk>>
class choice_view
{
};

template<typename Ntk>
class choice_view<Ntk, true> : public Ntk
{
public:
  choice_view( Ntk const& ntk, choice_view_params const& ps = {} ) : Ntk( ntk )
  {
    (void)ps;
  }
};

template<typename Ntk>
class choice_view<Ntk, false> : public Ntk
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  choice_view( choice_view_params const& ps = {} )
      : Ntk(), _ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
    static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );

    init_choice_classes();

    if ( _ps.update_on_add )
    {
      _add_event = Ntk::events().register_add_event( [this]( auto const& n ) {
        on_add( n );
      } );
    }
  }

  choice_view( Ntk const& ntk, choice_view_params const& ps = {} )
      : Ntk( ntk ), _ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
    static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );

    init_choice_classes();

    if ( _ps.update_on_add )
    {
      _add_event = Ntk::events().register_add_event( [this]( auto const& n ) {
        on_add( n );
      } );
    }
  }

  choice_view<Ntk, false>& operator=( choice_view<Ntk, false> const& choice_ntk )
  {
    Ntk::operator=( choice_ntk );
    if ( this != &choice_ntk )
    {
      this->_choice_repr = choice_ntk._choice_repr;
      this->_choice_phase = choice_ntk._choice_phase;
      this->_ps = choice_ntk._ps;
    }
    if ( _ps.update_on_add )
    {
      _add_event = Ntk::events().register_add_event( [this]( auto const& n ) {
        on_add( n );
      } );
    }
    return *this;
  }

  ~choice_view()
  {
    if ( _ps.update_on_add )
    {
      Ntk::events().release_add_event( _add_event );
    }
  }

  template<typename T = Ntk>
  std::enable_if_t<!std::is_same_v<typename T::node, typename T::signal>, void> add_choice( node const& n1, node const& n2 )
  {
    add_choice( n1, Ntk::make_signal( n2 ) );
  }

  void add_choice( node const& n1, signal const& s2 )
  {
    auto const n2 = Ntk::get_node( s2 );
    auto const id1 = Ntk::node_to_index( n1 );
    auto const id2 = Ntk::node_to_index( n2 );

    if ( id1 == id2 )
    {
      /* same node */
      return;
    }

    auto rep1 = get_choice_representative( n1 );
    auto rep2 = get_choice_representative( n2 );

    auto idrep1 = Ntk::node_to_index( rep1 );
    auto idrep2 = Ntk::node_to_index( rep2 );

    if ( idrep1 == idrep2 )
    {
      /* already in the same equivalence class */
      return;
    }

    /* set the representative as the node with lowest index */
    if ( idrep1 > idrep2 )
    {
      std::swap( rep1, rep2 );
      std::swap( idrep1, idrep2 );
    }

    /* merge the eq lists */
    bool inv = false;
    if ( ( ( _choice_repr->at( id1 ) != n1 && Ntk::is_complemented( _choice_phase->at( id1 ) ) ) != Ntk::is_complemented( s2 ) ) !=
         ( _choice_repr->at( id2 ) != n2 && Ntk::is_complemented( _choice_phase->at( id2 ) ) ) )
    {
      /* before merging, complement nodes accordingly to the new representative phase, if needed */
      invert_phases_in_class( rep2 );
      inv = true;
    }
    _choice_repr->at( idrep2 ) = Ntk::get_node( _choice_phase->at( idrep1 ) );
    _choice_phase->at( idrep1 ) = _choice_phase->at( idrep2 );
    /* store the right phase */
    _choice_phase->at( idrep2 ) = Ntk::make_signal( rep2 ) ^ inv;
  }

  /* Set sig as an equivalence representative of n without including n in the choice list */
  void set_representative( node const& n, signal const& sig )
  {
    /* TODO: TEST */
    auto nsig = Ntk::get_node( sig );
    auto repr = get_choice_representative( nsig );
    bool c = false;

    if ( repr != nsig )
    {
      c = Ntk::is_complemented( _choice_phase->at( Ntk::node_to_index( nsig ) ) );
    }
    _choice_repr->at( Ntk::node_to_index( n ) ) = repr;
    _choice_phase->at( Ntk::node_to_index( n ) ) = Ntk::make_signal( n ) ^ ( Ntk::is_complemented( sig ) ^ c );
  }

  void update_choice_representative( node const& n )
  {
    assert( Ntk::node_to_index( n ) < Ntk::size() );

    if ( is_choice_representative( n ) )
      return;

    bool inv = Ntk::is_complemented( _choice_phase->at( Ntk::node_to_index( n ) ) );
    auto repr = get_choice_representative( n );
    _choice_phase->at( Ntk::node_to_index( n ) ) = Ntk::make_signal( _choice_repr->at( Ntk::node_to_index( n ) ) );
    _choice_repr->at( Ntk::node_to_index( n ) ) = n;
    _choice_repr->at( Ntk::node_to_index( repr ) ) = Ntk::get_node( _choice_phase->at( Ntk::node_to_index( repr ) ) );
    _choice_phase->at( Ntk::node_to_index( repr ) ) = Ntk::make_signal( repr );
    if ( inv )
    {
      invert_phases_in_class( n );
    }
  }

  /* returns the new class representative */
  std::optional<signal> remove_choice( node const& n )
  {
    assert( Ntk::node_to_index( n ) < Ntk::size() );

    auto next = Ntk::node_to_index( _choice_repr->at( Ntk::node_to_index( n ) ) );
    auto repr = Ntk::node_to_index( get_choice_representative( next ) );
    auto tail = Ntk::node_to_index( Ntk::get_node( _choice_phase->at( Ntk::node_to_index( repr ) ) ) );

    /* if n is a representative, recompute the representative of the new class */
    if ( repr == Ntk::node_to_index( n ) && tail != Ntk::node_to_index( n ) )
    {
      auto new_repr = tail;
      node pred = tail;
      foreach_choice( tail, [&]( auto const& g ) {
        if ( Ntk::node_to_index( g ) != repr && Ntk::node_to_index( g ) < new_repr )
        {
          new_repr = Ntk::node_to_index( g );
        }
        if ( Ntk::node_to_index( g ) != repr && Ntk::node_to_index( _choice_repr->at( g ) ) == repr )
        {
          pred = Ntk::node_to_index( g );
        }
        return true;
      } );
      auto const new_repr_signal = _choice_phase->at( new_repr );
      bool polarity = Ntk::is_complemented( new_repr_signal );
      if ( new_repr == pred )
      {
        _choice_phase->at( new_repr ) = _choice_phase->at( repr );
      }
      else
      {
        _choice_phase->at( new_repr ) = Ntk::make_signal( _choice_repr->at( new_repr ) );
        _choice_repr->at( pred ) = Ntk::index_to_node( tail );
      }
      _choice_repr->at( new_repr ) = Ntk::index_to_node( new_repr );
      if ( polarity )
      {
        invert_phases_in_class( new_repr );
      }
      return new_repr_signal;
    }
    else if ( tail == n )
    {
      _choice_phase->at( Ntk::node_to_index( repr ) ) = Ntk::make_signal( next );
    }
    else
    {
      while ( Ntk::node_to_index( _choice_repr->at( tail ) ) != Ntk::node_to_index( n ) )
      {
        tail = Ntk::node_to_index( _choice_repr->at( tail ) );
      }
      _choice_repr->at( tail ) = Ntk::index_to_node( next );
    }

    _choice_repr->at( Ntk::node_to_index( n ) ) = n;
    return std::nullopt;
  }

  void clear_choices()
  {
    for ( auto i = 0u; i < Ntk::size(); i++ )
    {
      _choice_repr->at( i ) = Ntk::index_to_node( i );
      _choice_phase->at( i ) = Ntk::make_signal( Ntk::index_to_node( i ) );
    }
  }

  bool delete_choice_from_network( node const& n )
  {
    if ( Ntk::is_dead( n ) || !is_choice( n ) )
    {
      /* node is already dead or not dangling */
      return false;
    }

    Ntk::take_out_node( n );
    remove_choice( n );
    return true;
  }

  node get_choice_representative( node const& n ) const
  {
    assert( Ntk::node_to_index( n ) < Ntk::size() );

    auto rep = _choice_repr->at( Ntk::node_to_index( n ) );
    while ( Ntk::node_to_index( rep ) != Ntk::node_to_index( _choice_repr->at( Ntk::node_to_index( rep ) ) ) )
    {
      rep = _choice_repr->at( Ntk::node_to_index( rep ) );
    }
    return rep;
  }

  bool is_choice_representative( node const& n ) const
  {
    assert( Ntk::node_to_index( n ) < Ntk::size() );
    return _choice_repr->at( Ntk::node_to_index( n ) ) == n;
  }

  signal get_choice_representative_signal( node const& n ) const
  {
    auto repr = Ntk::make_signal( get_choice_representative( n ) );

    if ( Ntk::get_node( repr ) == n )
    {
      return repr;
    }

    return repr ^ Ntk::is_complemented( _choice_phase->at( Ntk::node_to_index( n ) ) );
  }

  template<typename T = Ntk>
  std::enable_if_t<!std::is_same_v<typename T::node, typename T::signal>, typename T::signal> get_choice_representative_signal( signal const& sig ) const
  {
    auto n = Ntk::get_node( sig );
    auto repr = get_choice_representative( n );

    if ( repr == n )
    {
      return sig;
    }

    bool c = Ntk::is_complemented( _choice_phase->at( Ntk::node_to_index( n ) ) ) != Ntk::is_complemented( sig );
    return Ntk::make_signal( repr ) ^ c;
  }

  uint32_t count_choices( node const& n ) const
  {
    assert( Ntk::node_to_index( n ) < Ntk::size() );
    uint32_t size = 1u;
    auto p = n;
    while ( Ntk::node_to_index( p ) != Ntk::node_to_index( _choice_repr->at( Ntk::node_to_index( p ) ) ) )
    {
      p = _choice_repr->at( Ntk::node_to_index( p ) );
      size++;
    }
    p = Ntk::get_node( _choice_phase->at( Ntk::node_to_index( p ) ) );
    while ( Ntk::node_to_index( p ) != Ntk::node_to_index( n ) )
    {
      size++;
      p = _choice_repr->at( Ntk::node_to_index( p ) );
    }
    return size;
  }

  template<typename Fn>
  void foreach_choice( node const& n, Fn&& fn ) const
  {
    auto p = n;
    if ( !fn( p ) )
    {
      return;
    }
    while ( Ntk::node_to_index( p ) != Ntk::node_to_index( _choice_repr->at( Ntk::node_to_index( p ) ) ) )
    {
      p = _choice_repr->at( Ntk::node_to_index( p ) );
      if ( !fn( p ) )
      {
        return;
      }
    }
    p = Ntk::get_node( _choice_phase->at( Ntk::node_to_index( p ) ) );
    while ( Ntk::node_to_index( p ) != Ntk::node_to_index( n ) )
    {
      if ( !fn( p ) )
      {
        return;
      }
      p = _choice_repr->at( Ntk::node_to_index( p ) );
    }
  }

  /* redefine node substitution */
  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      if ( _ps.add_choices_on_substitute )
      {
        add_choice( _old, _new );
      }
      // TODO: add replace choice mode

      for ( auto idx = 1u; idx < Ntk::_storage->nodes.size(); ++idx )
      {
        if ( Ntk::is_ci( idx ) || Ntk::is_dead( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = Ntk::replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      Ntk::replace_in_outputs( _old, _new );

      // set old node as choice, reset fanout
      Ntk::_storage->nodes[_old].data[0].h1 &= UINT32_C( 0xC0000000 );
      take_out_choice( _old );

      if ( is_choice( Ntk::get_node( _new ) ) && fanout_size( Ntk::get_node( _new ) ) > 0u )
      {
        take_in_choice( Ntk::get_node( _new ) );
      }
    }
  }

  void take_out_choice( node const& n )
  {
    /* we cannot delete CIs or constants */
    if ( n == 0 || Ntk::is_ci( n ) )
      return;

    auto& nobj = Ntk::_storage->nodes[n];
    set_choice_flag( n );
    Ntk::_storage->hash.erase( nobj );

    for ( auto i = 0u; i < Ntk::fanin_size( n ); ++i )
    {
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }
      /* set childrens in MFFC as choice, decrement the fanout count */
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_choice( nobj.children[i].index );
      }
    }
  }

  void take_in_choice( node const& n )
  {
    /* we cannot delete CIs or constants */
    if ( n == 0 || Ntk::is_ci( n ) )
      return;

    auto& nobj = Ntk::_storage->nodes[n];
    reset_choice_flag( n );
    Ntk::_storage->hash[nobj] = n;

    for ( auto i = 0u; i < Ntk::fanin_size( n ); ++i )
    {
      /* restore choice childrens in MFFC, increment the fanout count */
      if ( incr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_in_choice( nobj.children[i].index );
      }
    }
  }

  /* redefine methods for choice flag: storage h1 = dead(31), choice(30), fanout_size(29 to 0) */
  uint32_t fanout_size( node const& n ) const
  {
    return Ntk::_storage->nodes[n].data[0].h1 & UINT32_C( 0x3FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return Ntk::_storage->nodes[n].data[0].h1++ & UINT32_C( 0x3FFFFFFF );
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --Ntk::_storage->nodes[n].data[0].h1 & UINT32_C( 0x3FFFFFFF );
  }

  inline bool is_choice( node const& n ) const
  {
    return ( Ntk::_storage->nodes[n].data[0].h1 >> 30 ) & 1;
  }

private:
  inline void set_choice_flag( node const& n ) const
  {
    Ntk::_storage->nodes[n].data[0].h1 |= UINT32_C( 0x40000000 );
  }

  inline void reset_choice_flag( node const& n ) const
  {
    Ntk::_storage->nodes[n].data[0].h1 &= UINT32_C( 0xBFFFFFFF );
  }

  void init_choice_classes()
  {
    _choice_repr = std::make_shared<std::vector<node>>( Ntk::size() );
    _choice_phase = std::make_shared<std::vector<signal>>( Ntk::size() );
    // Ntk::foreach_node( [&]( auto n ) {
    for ( auto i = 0u; i < Ntk::size(); i++ )
    {
      _choice_repr->at( i ) = Ntk::index_to_node( i );
      _choice_phase->at( i ) = Ntk::make_signal( Ntk::index_to_node( i ) );
    }
  }

  void invert_phases_in_class( node const& rep )
  {
    assert( Ntk::node_to_index( rep ) < Ntk::size() );
    assert( is_choice_representative( rep ) );

    auto p = Ntk::get_node( _choice_phase->at( rep ) );

    while ( Ntk::node_to_index( p ) != Ntk::node_to_index( _choice_repr->at( Ntk::node_to_index( p ) ) ) )
    {
      _choice_phase->at( p ) = !_choice_phase->at( p );
      p = _choice_repr->at( Ntk::node_to_index( p ) );
    }
  }

  void on_add( node const& n )
  {
    if ( Ntk::size() > _choice_repr->size() )
    {
      _choice_repr->push_back( n );
      _choice_phase->push_back( Ntk::make_signal( n ) );
    }
  }

private:
  std::shared_ptr<std::vector<node>> _choice_repr;
  std::shared_ptr<std::vector<signal>> _choice_phase;
  choice_view_params _ps;
  std::shared_ptr<typename network_events<Ntk>::add_event_type> _add_event;
};

template<class T>
choice_view( T const&, choice_view_params const& ps = {} ) -> choice_view<T>;

} // namespace mockturtle
