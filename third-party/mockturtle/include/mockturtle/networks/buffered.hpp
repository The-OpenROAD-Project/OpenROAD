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
  \file buffered.hpp
  \brief Buffered networks implementation

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "aig.hpp"
#include "aqfp.hpp"
#include "crossed.hpp"
#include "mig.hpp"

#include <cassert>

namespace mockturtle
{

class buffered_aig_network : public aig_network
{
public:
  static constexpr bool is_buffered_network_type = true;

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0] = a;
    node.children[1] = !a;

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, 0 };
  }

  void invert( node const& n )
  {
    assert( !is_constant( n ) && !is_pi( n ) );
    assert( fanout_size( n ) == 0 );
    _storage->nodes[n].children[0].weight ^= 1;
    _storage->nodes[n].children[1].weight ^= 1;
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( aig_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( other.is_and( source ) );
    assert( children.size() == 2u );
    return create_and( children[0u], children[1u] );
  }
#pragma endregion

#pragma region Restructuring
  // disable restructuring
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal ) = delete;
  void replace_in_outputs( node const& old_node, signal const& new_signal ) = delete;
  void take_out_node( node const& n ) = delete;
  void substitute_node( node const& old_node, signal const& new_signal ) = delete;
  void substitute_nodes( std::list<std::pair<node, signal>> substitutions ) = delete;
#pragma endregion

#pragma region Structural properties
  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    else if ( is_buf( n ) )
      return 1;
    else
      return 2;
  }

  // including buffers, splitters, and inverters
  bool is_buf( node const& n ) const
  {
    return _storage->nodes[n].children[0].index == _storage->nodes[n].children[1].index && _storage->nodes[n].children[0].weight != _storage->nodes[n].children[1].weight;
  }

  bool is_not( node const& n ) const
  {
    return _storage->nodes[n].children[0].weight;
  }

  bool is_and( node const& n ) const
  {
    return n > 0 && !is_ci( n ) && !is_buf( n );
  }

#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    if ( is_buf( n ) )
    {
      kitty::dynamic_truth_table _buf( 1 );
      _buf._bits[0] = 0x2;
      return _buf;
    }

    kitty::dynamic_truth_table _and( 2 );
    _and._bits[0] = 0x8;
    return _and;
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); /* start from 1 to avoid constant */
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_dead( n ) && !is_buf( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    /* we don't use foreach_element here to have better performance */
    if ( is_buf( n ) )
    {
      if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
      {
        fn( signal{ _storage->nodes[n].children[0] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
      {
        fn( signal{ _storage->nodes[n].children[0] }, 0 );
      }
      else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] }, 0 );
      }
    }
    else
    {
      if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
      {
        if ( !fn( signal{ _storage->nodes[n].children[0] } ) )
          return;
        fn( signal{ _storage->nodes[n].children[1] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
      {
        if ( !fn( signal{ _storage->nodes[n].children[0] }, 0 ) )
          return;
        fn( signal{ _storage->nodes[n].children[1] }, 1 );
      }
      else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] } );
        fn( signal{ _storage->nodes[n].children[1] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] }, 0 );
        fn( signal{ _storage->nodes[n].children[1] }, 1 );
      }
    }
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
      return is_complemented( _storage->nodes[n].children[0] ) ? !( *begin ) : *begin;

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto v1 = *begin++;
    auto v2 = *begin++;

    return ( v1 ^ c1.weight ) && ( v2 ^ c2.weight );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
      return is_complemented( _storage->nodes[n].children[0] ) ? ~( *begin ) : *begin;

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto tt1 = *begin++;
    auto tt2 = *begin++;

    return ( c1.weight ? ~tt1 : tt1 ) & ( c2.weight ? ~tt2 : tt2 );
  }

  /*! \brief Re-compute the last block. */
  template<typename Iterator>
  void compute( node const& n, kitty::partial_truth_table& result, Iterator begin, Iterator end ) const
  {
    static_assert( iterates_over_v<Iterator, kitty::partial_truth_table>, "begin and end have to iterate over partial_truth_tables" );

    (void)end;
    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
    {
      result.resize( begin->num_bits() );
      result._bits.back() = is_complemented( _storage->nodes[n].children[0] ) ? ~( begin->_bits.back() ) : begin->_bits.back();
      result.mask_bits();
      return;
    }

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto tt1 = *begin++;
    auto tt2 = *begin++;

    assert( tt1.num_bits() > 0 && "truth tables must not be empty" );
    assert( tt1.num_bits() == tt2.num_bits() );
    assert( tt1.num_bits() >= result.num_bits() );
    assert( result.num_blocks() == tt1.num_blocks() || ( result.num_blocks() == tt1.num_blocks() - 1 && result.num_bits() % 64 == 0 ) );

    result.resize( tt1.num_bits() );
    result._bits.back() = ( c1.weight ? ~( tt1._bits.back() ) : tt1._bits.back() ) & ( c2.weight ? ~( tt2._bits.back() ) : tt2._bits.back() );
    result.mask_bits();
  }
#pragma endregion
}; /* buffered_aig_network */

class buffered_mig_network : public mig_network
{
public:
  static constexpr bool is_buffered_network_type = true;

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children[0] = a;
    node.children[1] = !a;
    // node.children[2] = a; // not used

    if ( index >= .9 * _storage->nodes.capacity() )
    {
      _storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, 0 };
  }

  void invert( node const& n )
  {
    assert( !is_constant( n ) && !is_pi( n ) );
    assert( fanout_size( n ) == 0 );
    _storage->nodes[n].children[0].weight ^= 1;
    _storage->nodes[n].children[1].weight ^= 1;
    _storage->nodes[n].children[2].weight ^= 1;
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( mig_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( other.is_maj( source ) );
    assert( children.size() == 3u );
    return create_maj( children[0u], children[1u], children[2u] );
  }
#pragma endregion

#pragma region Restructuring
  // disable restructuring
  void replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    assert( is_buf( old_node ) );
    auto& node = _storage->nodes[n];

    if ( is_buf( n ) )
    {
      assert( node.children[0].index == old_node );
      new_signal.complement ^= node.children[0].weight;
      node.children[0] = new_signal;
      node.children[1] = !new_signal;
      _storage->nodes[new_signal.index].data[0].h1++;
      return;
    }

    uint32_t fanin = 3u;
    for ( auto i = 0u; i < 3u; ++i )
    {
      if ( node.children[i].index == old_node )
      {
        fanin = i;
        new_signal.complement ^= node.children[i].weight;
        break;
      }
    }
    assert( fanin < 3 );
    signal child2 = new_signal;
    signal child1 = node.children[( fanin + 1 ) % 3];
    signal child0 = node.children[( fanin + 2 ) % 3];
    if ( child0.index > child1.index )
    {
      std::swap( child0, child1 );
    }
    if ( child1.index > child2.index )
    {
      std::swap( child1, child2 );
    }
    if ( child0.index > child1.index )
    {
      std::swap( child0, child1 );
    }

    _storage->hash.erase( node );
    node.children[0] = child0;
    node.children[1] = child1;
    node.children[2] = child2;
    _storage->hash[node] = n;

    // update the reference counter of the new signal
    _storage->nodes[new_signal.index].data[0].h1++;
  }
  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    assert( !is_dead( old_node ) );

    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        if ( old_node != new_signal.index )
        {
          // increment fan-in of new node
          _storage->nodes[new_signal.index].data[0].h1++;
        }
      }
    }
  }
  void take_out_node( node const& n )
  {
    assert( is_buf( n ) );

    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */

    for ( auto const& fn : _events->on_delete )
    {
      ( *fn )( n );
    }

    if ( decr_fanout_size( nobj.children[0].index ) == 0 )
    {
      take_out_node( nobj.children[0].index );
    }
  }
  void substitute_node( node const& old_node, signal const& new_signal ) = delete;
  void substitute_nodes( std::list<std::pair<node, signal>> substitutions ) = delete;
#pragma endregion

#pragma region Structural properties
  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    else if ( is_buf( n ) )
      return 1;
    else
      return 3;
  }

  // including buffers, splitters, and inverters
  bool is_buf( node const& n ) const
  {
    return _storage->nodes[n].children[0].index == _storage->nodes[n].children[1].index && _storage->nodes[n].children[0].weight != _storage->nodes[n].children[1].weight;
  }

  bool is_not( node const& n ) const
  {
    return _storage->nodes[n].children[0].weight;
  }

  bool is_maj( node const& n ) const
  {
    return n > 0 && !is_ci( n ) && !is_buf( n );
  }

#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    if ( is_buf( n ) )
    {
      kitty::dynamic_truth_table _buf( 1 );
      _buf._bits[0] = 0x2;
      return _buf;
    }

    kitty::dynamic_truth_table _maj( 3 );
    _maj._bits[0] = 0xe8;
    return _maj;
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); /* start from 1 to avoid constant */
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_dead( n ) && !is_buf( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    /* we don't use foreach_element here to have better performance */
    if ( is_buf( n ) )
    {
      if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
      {
        fn( signal{ _storage->nodes[n].children[0] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
      {
        fn( signal{ _storage->nodes[n].children[0] }, 0 );
      }
      else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] }, 0 );
      }
    }
    else
    {
      if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
      {
        if ( !fn( signal{ _storage->nodes[n].children[0] } ) )
          return;
        if ( !fn( signal{ _storage->nodes[n].children[1] } ) )
          return;
        fn( signal{ _storage->nodes[n].children[2] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
      {
        if ( !fn( signal{ _storage->nodes[n].children[0] }, 0 ) )
          return;
        if ( !fn( signal{ _storage->nodes[n].children[1] }, 1 ) )
          return;
        fn( signal{ _storage->nodes[n].children[2] }, 2 );
      }
      else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] } );
        fn( signal{ _storage->nodes[n].children[1] } );
        fn( signal{ _storage->nodes[n].children[2] } );
      }
      else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
      {
        fn( signal{ _storage->nodes[n].children[0] }, 0 );
        fn( signal{ _storage->nodes[n].children[1] }, 1 );
        fn( signal{ _storage->nodes[n].children[2] }, 2 );
      }
    }
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
      return is_complemented( _storage->nodes[n].children[0] ) ? !( *begin ) : *begin;

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto v1 = *begin++;
    auto v2 = *begin++;
    auto v3 = *begin++;

    return ( ( v1 ^ c1.weight ) && ( v2 ^ c2.weight ) ) || ( ( v3 ^ c3.weight ) && ( v1 ^ c1.weight ) ) || ( ( v3 ^ c3.weight ) && ( v2 ^ c2.weight ) );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
      return is_complemented( _storage->nodes[n].children[0] ) ? ~( *begin ) : *begin;

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;

    return kitty::ternary_majority( c1.weight ? ~tt1 : tt1, c2.weight ? ~tt2 : tt2, c3.weight ? ~tt3 : tt3 );
  }

  /*! \brief Re-compute the last block. */
  template<typename Iterator>
  void compute( node const& n, kitty::partial_truth_table& result, Iterator begin, Iterator end ) const
  {
    static_assert( iterates_over_v<Iterator, kitty::partial_truth_table>, "begin and end have to iterate over partial_truth_tables" );

    (void)end;
    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
    {
      result.resize( begin->num_bits() );
      result._bits.back() = is_complemented( _storage->nodes[n].children[0] ) ? ~( begin->_bits.back() ) : begin->_bits.back();
      result.mask_bits();
      return;
    }

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];
    auto const& c3 = _storage->nodes[n].children[2];

    auto tt1 = *begin++;
    auto tt2 = *begin++;
    auto tt3 = *begin++;

    assert( tt1.num_bits() > 0 && "truth tables must not be empty" );
    assert( tt1.num_bits() == tt2.num_bits() );
    assert( tt1.num_bits() == tt3.num_bits() );
    assert( tt1.num_bits() >= result.num_bits() );
    assert( result.num_blocks() == tt1.num_blocks() || ( result.num_blocks() == tt1.num_blocks() - 1 && result.num_bits() % 64 == 0 ) );

    result.resize( tt1.num_bits() );
    result._bits.back() =
        ( ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) & ( c2.weight ? ~tt2._bits.back() : tt2._bits.back() ) ) |
        ( ( c1.weight ? ~tt1._bits.back() : tt1._bits.back() ) & ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() ) ) |
        ( ( c2.weight ? ~tt2._bits.back() : tt2._bits.back() ) & ( c3.weight ? ~tt3._bits.back() : tt3._bits.back() ) );
    result.mask_bits();
  }
#pragma endregion
}; /* buffered_mig_network */

class buffered_aqfp_network : public aqfp_network
{
public:
  static constexpr bool is_buffered_network_type = true;

#pragma region Primary I / O and constants
  bool is_ci( node const& n ) const
  {
    if ( is_buf( n ) )
      return false;

    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data;
  }

  bool is_pi( node const& n ) const
  {
    if ( is_buf( n ) )
      return false;

    return _storage->nodes[n].children[0].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[1].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[2].data == ~static_cast<uint64_t>( 0 );
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    if ( is_constant( get_node( a ) ) )
      return a;

    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();

    node.children.resize( 1u );
    node.children[0] = a;

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, 0 };
  }

  void invert( node const& n )
  {
    assert( !is_constant( n ) && !is_pi( n ) );
    assert( fanout_size( n ) == 0 );
    for ( auto& s : _storage->nodes[n].children )
    {
      s.weight ^= 1;
    }
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( aqfp_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( other.is_maj( source ) );
    assert( children.size() > 1 && children.size() % 2 == 1 );
    return create_maj( children );
  }
#pragma endregion

#pragma region Structural properties
  /* redefinition of num_gates counting the gates */
  auto num_gates() const
  {
    uint32_t gate_count = 0;
    foreach_gate( [&gate_count]( auto const& n ) {
      ++gate_count;
    } );
    return gate_count;
  }

  bool is_buf( node const& n ) const
  {
    return _storage->nodes[n].children.size() == 1;
  }

  bool is_not( node const& n ) const
  {
    return _storage->nodes[n].children.size() == 1 && _storage->nodes[n].children[0].weight;
  }

  bool is_maj( node const& n ) const
  {
    return n > 0 && !is_ci( n ) && !is_buf( n );
  }

#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    if ( is_buf( n ) )
    {
      kitty::dynamic_truth_table _buf( 1 );
      _buf._bits[0] = 0x2;
      return _buf;
    }

    const auto num_fanin = _storage->nodes[n].children.size();

    if ( num_fanin == 3u )
    {
      kitty::dynamic_truth_table _maj( 3u );
      _maj._bits[0] = 0xe8;
      return _maj;
    }
    else if ( num_fanin == 5u )
    {
      kitty::dynamic_truth_table _maj( 5u );
      _maj._bits[0] = 0xfee8e880;
      return _maj;
    }
    else
    {
      if ( _storage->data.node_fn_cache.count( num_fanin ) )
      {
        return _storage->data.node_fn_cache[num_fanin];
      }

      std::vector<std::vector<kitty::dynamic_truth_table>> dp;
      for ( auto i = 0u; i <= num_fanin; i++ )
      {
        dp.push_back( { ~kitty::dynamic_truth_table( num_fanin ) } );
        if ( i == 0u )
          continue;
        auto ith_var = kitty::nth_var<kitty::dynamic_truth_table>( num_fanin, i - 1 );
        for ( auto j = 1u; j <= i && j <= ( num_fanin / 2 ) + 1; j++ )
        {
          dp[i].push_back( ( j < i ) ? ( ith_var & dp[i - 1][j - 1] ) | dp[i - 1][j] : ( ith_var & dp[i - 1][j - 1] ) );
        }
      }

      return ( _storage->data.node_fn_cache[num_fanin] = dp[num_fanin][( num_fanin / 2 ) + 1] );
    }
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); /* start from 1 to avoid constant */
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_dead( n ) && !is_buf( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      for ( auto i = 0u; i < _storage->nodes[n].children.size(); i++ )
      {
        if ( !fn( signal{ _storage->nodes[n].children[i] } ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      for ( auto i = 0u; i < _storage->nodes[n].children.size(); i++ )
      {
        if ( !fn( signal{ _storage->nodes[n].children[i] }, i ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      for ( auto i = 0u; i < _storage->nodes[n].children.size(); i++ )
      {
        fn( signal{ _storage->nodes[n].children[i] } );
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      for ( auto i = 0u; i < _storage->nodes[n].children.size(); i++ )
      {
        fn( signal{ _storage->nodes[n].children[i] }, i );
      }
    }
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
      return is_complemented( _storage->nodes[n].children[0] ) ? !( *begin ) : *begin;

    std::vector<typename Iterator::value_type> v;
    auto i = 0u;
    for ( auto it = begin; it != end; it++, i++ )
    {
      v.push_back( ( *it ) ^ _storage->nodes[n].children[i].weight );
    }
    return compute_majority_n_with_bool( v.begin(), v.end() );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
      return is_complemented( _storage->nodes[n].children[0] ) ? ~( *begin ) : *begin;

    std::vector<typename Iterator::value_type> v;
    auto i = 0u;
    for ( auto it = begin; it != end; it++, i++ )
    {
      v.push_back( _storage->nodes[n].children[i].weight ? ~( *it ) : ( *it ) );
    }

    return compute_majority_n( v.begin(), v.end() );
  }

  /*! \brief Re-compute the last block. */
  template<typename Iterator>
  void compute( node const& n, kitty::partial_truth_table& result, Iterator begin, Iterator end ) const
  {
    static_assert( iterates_over_v<Iterator, kitty::partial_truth_table>, "begin and end have to iterate over partial_truth_tables" );

    (void)end;
    assert( n != 0 && !is_ci( n ) );

    if ( is_buf( n ) )
    {
      result.resize( begin->num_bits() );
      result._bits.back() = is_complemented( _storage->nodes[n].children[0] ) ? ~( begin->_bits.back() ) : begin->_bits.back();
      result.mask_bits();
      return;
    }

    assert( begin->num_bits() > 0 && "truth tables must not be empty" );
    for ( auto it = begin; it != end; it++ )
    {
      assert( begin->num_bits() == it->num_bits() );
    }
    assert( begin->num_bits() >= result.num_bits() );
    assert( result.num_blocks() == begin->num_blocks() || ( result.num_blocks() == begin->num_blocks() - 1 && result.num_bits() % 64 == 0 ) );

    result.resize( begin->num_bits() );

    std::vector<uint64_t> v;
    auto i = 0u;
    for ( auto it = begin; it != end; it++, i++ )
    {
      v.push_back( _storage->nodes[n].children[i].weight ? ~( it->_bits.back() ) : ( it->_bits.back() ) );
    }

    result._bits.back() = compute_majority_n( v.begin(), v.end() );

    result.mask_bits();
  }
#pragma endregion
}; /* buffered_aqfp_network */

class buffered_crossed_klut_network : public crossed_klut_network
{
public:
  static constexpr bool is_buffered_network_type = true;

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return _create_node( { a }, 2 );
  }

  void invert( node const& n )
  {
    if ( _storage->nodes[n].data[1].h1 == 2 )
      _storage->nodes[n].data[1].h1 = 3;
    else if ( _storage->nodes[n].data[1].h1 == 3 )
      _storage->nodes[n].data[1].h1 = 2;
    else
      assert( false );
  }
#pragma endregion

#pragma region Crossings
  /*! \brief Merges two buffer nodes into a crossing cell
   *
   * After this operation, the network will not be in a topological order. Additionally, buf1 and buf2 will be dangling.
   *
   * \param buf1 First buffer node.
   * \param buf2 Second buffer node.
   * \return The created crossing cell.
   */
  node merge_into_crossing( node const& buf1, node const& buf2 )
  {
    assert( is_buf( buf1 ) && is_buf( buf2 ) );

    auto const& in_buf1 = _storage->nodes[buf1].children[0];
    auto const& in_buf2 = _storage->nodes[buf2].children[0];

    node out_buf1{}, out_buf2{};
    uint32_t fanin_index1 = std::numeric_limits<uint32_t>::max(), fanin_index2 = std::numeric_limits<uint32_t>::max();
    foreach_node( [&]( node const& n ) {
      foreach_fanin( n, [&]( auto const& f, auto i ) {
        if ( auto const fin = get_node( f ); fin == buf1 )
        {
          out_buf1 = n;
          fanin_index1 = i;
        }
        else if ( fin == buf2 )
        {
          out_buf2 = n;
          fanin_index2 = i;
        }
      } );
    } );
    assert( out_buf1 != 0 && out_buf2 != 0 );
    assert( fanin_index1 != std::numeric_limits<uint32_t>::max() && fanin_index2 != std::numeric_limits<uint32_t>::max() );

    auto const [fout1, fout2] = create_crossing( in_buf1, in_buf2 );

    _storage->nodes[out_buf1].children[fanin_index1] = fout1;
    _storage->nodes[out_buf2].children[fanin_index2] = fout2;

    /* decrease ref-count to children (was increased in `create_crossing`) */
    _storage->nodes[in_buf1.index].data[0].h1--;
    _storage->nodes[in_buf2.index].data[0].h1--;

    _storage->nodes[buf1].children.clear();
    _storage->nodes[buf2].children.clear();

    return get_node( fout1 );
  }

#pragma endregion

#pragma region Structural properties
  // including buffers, splitters, and inverters
  bool is_buf( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1 == 2 || _storage->nodes[n].data[1].h1 == 3;
  }

  bool is_not( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1 == 3;
  }
#pragma endregion

#pragma region Node and signal iterators
  /* Note: crossings are included; buffers, splitters, inverters are not */
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 2u, _storage->nodes.size() ); /* start from 2 to avoid constants */
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_buf( n ); },
        fn );
  }
#pragma endregion
}; /* buffered_crossed_klut_network */

} // namespace mockturtle