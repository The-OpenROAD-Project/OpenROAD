/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2023  EPFL
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
  \file block.hpp
  \brief Block logic network implementation with multi-output support

  \author Alessandro Tempia Calvino
*/

#pragma once

#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "../utils/truth_table_cache.hpp"
#include "detail/foreach.hpp"
#include "events.hpp"
#include "storage.hpp"

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include <algorithm>
#include <memory>

namespace mockturtle
{

struct block_storage_data
{
  truth_table_cache<kitty::dynamic_truth_table> cache;
};

/*! \brief Block node
 *
 * `data[0].h1`  : Application-specific value
 * `data[1].h1`  : Visited flags
 * `data[1].h2`  : Total fan-out size (we use MSB to indicate whether a node is dead)
 * `data[2+i].h1`: Function literal in truth table cache for the fanout
 * `data[2+i].h2`: Fan-out size
 *
 */
struct block_storage_node : block_fanin_node<2>
{
  block_storage_node()
  {
    data = decltype( data )( 3 );
  }

  bool operator==( block_storage_node const& other ) const
  {
    if ( data.size() != other.data.size() )
      return false;

    for ( auto i = 2; i < data.size() + 2; ++i )
      if ( ( data[i].h1 != other.data[i].h1 ) || ( children != other.children ) )
        return false;

    return true;
  }
};

/*! \brief Block storage container

  ...
*/
using block_storage = storage_no_hash<block_storage_node, block_storage_data>;

class block_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 1;
  static constexpr auto max_fanin_size = 32;
  static constexpr auto min_gate_output_size = 1;
  static constexpr auto max_gate_output_size = 2;
  static constexpr auto output_signal_bits = 1;

  using base_type = block_network;
  using storage = std::shared_ptr<block_storage>;
  using node = uint64_t;

  struct signal
  {
    signal() = default;

    signal( uint64_t index, uint64_t complement )
        : complement( complement ), output( 0 ), index( index )
    {
    }

    signal( uint32_t index )
        : complement( 0 ), output( 0 ), index( index )
    {
    }

    signal( uint64_t index, uint64_t complement, uint64_t output )
        : complement( complement ), output( output ), index( index )
    {
    }

    explicit signal( uint64_t data )
        : data( data )
    {
    }

    signal( block_storage::node_type::pointer_type const& p )
        : complement( p.weight & 1 ), output( p.weight >> 1 ), index( p.index )
    {
    }

    union
    {
      struct
      {
        uint64_t complement : 1;
        uint64_t output : output_signal_bits;
        uint64_t index : 63 - output_signal_bits;
      };
      uint64_t data;
    };

    signal operator!() const
    {
      return signal( data ^ 1 );
    }

    signal operator+() const
    {
      return { index, output, 0 };
    }

    signal operator-() const
    {
      return { index, output, 1 };
    }

    signal operator^( bool complement ) const
    {
      return signal( data ^ ( complement ? 1 : 0 ) );
    }

    bool operator==( signal const& other ) const
    {
      return data == other.data;
    }

    bool operator!=( signal const& other ) const
    {
      return data != other.data;
    }

    bool operator<( signal const& other ) const
    {
      return data < other.data;
    }

    operator block_storage::node_type::pointer_type() const
    {
      return { index, ( output << 1 ) | complement };
    }

    operator uint64_t() const
    {
      return data;
    }

#if __cplusplus > 201703L
    bool operator==( block_storage::node_type::pointer_type const& other ) const
    {
      return data == other.data;
    }
#endif
  };

  block_network()
      : _storage( std::make_shared<block_storage>() ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
    _init();
  }

  block_network( std::shared_ptr<block_storage> storage )
      : _storage( storage ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
    _init();
  }

  block_network clone() const
  {
    return { std::make_shared<block_storage>( *_storage ) };
  }

protected:
  inline void _init()
  {
    /* reserve the second node for constant 1 */
    _storage->nodes.emplace_back();

    /* reserve some truth tables for nodes */
    kitty::dynamic_truth_table tt_zero( 0 );
    _storage->data.cache.insert( tt_zero );

    static uint64_t _not = 0x1;
    kitty::dynamic_truth_table tt_not( 1 );
    kitty::create_from_words( tt_not, &_not, &_not + 1 );
    _storage->data.cache.insert( tt_not );

    static uint64_t _and = 0x8;
    kitty::dynamic_truth_table tt_and( 2 );
    kitty::create_from_words( tt_and, &_and, &_and + 1 );
    _storage->data.cache.insert( tt_and );

    static uint64_t _or = 0xe;
    kitty::dynamic_truth_table tt_or( 2 );
    kitty::create_from_words( tt_or, &_or, &_or + 1 );
    _storage->data.cache.insert( tt_or );

    static uint64_t _lt = 0x4;
    kitty::dynamic_truth_table tt_lt( 2 );
    kitty::create_from_words( tt_lt, &_lt, &_lt + 1 );
    _storage->data.cache.insert( tt_lt );

    static uint64_t _le = 0xd;
    kitty::dynamic_truth_table tt_le( 2 );
    kitty::create_from_words( tt_le, &_le, &_le + 1 );
    _storage->data.cache.insert( tt_le );

    static uint64_t _xor = 0x6;
    kitty::dynamic_truth_table tt_xor( 2 );
    kitty::create_from_words( tt_xor, &_xor, &_xor + 1 );
    _storage->data.cache.insert( tt_xor );

    static uint64_t _maj = 0xe8;
    kitty::dynamic_truth_table tt_maj( 3 );
    kitty::create_from_words( tt_maj, &_maj, &_maj + 1 );
    _storage->data.cache.insert( tt_maj );

    static uint64_t _ite = 0xd8;
    kitty::dynamic_truth_table tt_ite( 3 );
    kitty::create_from_words( tt_ite, &_ite, &_ite + 1 );
    _storage->data.cache.insert( tt_ite );

    static uint64_t _xor3 = 0x96;
    kitty::dynamic_truth_table tt_xor3( 3 );
    kitty::create_from_words( tt_xor3, &_xor3, &_xor3 + 1 );
    _storage->data.cache.insert( tt_xor3 );

    /* truth tables for constants */
    _storage->nodes[0].data[2].h1 = 0;
    _storage->nodes[1].data[2].h1 = 1;
  }
#pragma endregion

#pragma region Primary I / O and constants
public:
  signal get_constant( bool value = false ) const
  {
    return value ? signal( 1, 0 ) : signal( 0, 0 );
  }

  signal create_pi()
  {
    const auto index = _storage->nodes.size();
    _storage->nodes.emplace_back();
    _storage->inputs.emplace_back( index );
    _storage->nodes[index].data[2].h1 = 2;
    return { index, 0 };
  }

  uint32_t create_po( signal const& f )
  {
    /* increase ref-count to children */
    _storage->nodes[f.index].data[1].h2++;
    _storage->nodes[f.index].data[2 + f.output].h2++;
    auto const po_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, ( f.output << 1 ) | f.complement );
    return po_index;
  }

  bool is_combinational() const
  {
    return true;
  }

  bool is_multioutput( node const& n ) const
  {
    return _storage->nodes[n].data.size() > 3;
  }

  bool is_constant( node const& n ) const
  {
    return n <= 1;
  }

  bool is_ci( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].children.size() == 0u;
  }

  bool is_pi( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].children.size() == 0u;
  }

  bool constant_value( node const& n ) const
  {
    return n != 0;
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return _create_node( { a }, 2 );
  }

  signal create_not( signal const& a )
  {
    return _create_node( { a }, 3 );
  }
#pragma endregion

#pragma region Create binary functions
  signal create_and( signal a, signal b )
  {
    return _create_node( { a, b }, 4 );
  }

  signal create_nand( signal a, signal b )
  {
    return _create_node( { a, b }, 5 );
  }

  signal create_or( signal a, signal b )
  {
    return _create_node( { a, b }, 6 );
  }

  signal create_lt( signal a, signal b )
  {
    return _create_node( { a, b }, 8 );
  }

  signal create_le( signal a, signal b )
  {
    return _create_node( { a, b }, 11 );
  }

  signal create_xor( signal a, signal b )
  {
    return _create_node( { a, b }, 12 );
  }
#pragma endregion

#pragma region Create ternary functions
  signal create_maj( signal a, signal b, signal c )
  {
    return _create_node( { a, b, c }, 14 );
  }

  signal create_ite( signal a, signal b, signal c )
  {
    return _create_node( { a, b, c }, 16 );
  }

  signal create_xor3( signal a, signal b, signal c )
  {
    return _create_node( { a, b, c }, 18 );
  }

  signal create_ha( signal a, signal b )
  {
    /* PO0: carry, PO1: sum */
    return _create_node( { a, b }, { 4, 12 } );
  }

  signal create_hai( signal a, signal b )
  {
    /* PO0: carry, PO1: sum */
    return _create_node( { a, b }, { 5, 13 } );
  }

  signal create_fa( signal a, signal b, signal c )
  {
    /* PO0: carry, PO1: sum */
    return _create_node( { a, b, c }, { 14, 18 } );
  }

  signal create_fai( signal a, signal b, signal c )
  {
    /* PO0: carry, PO1: sum */
    return _create_node( { a, b, c }, { 15, 19 } );
  }
#pragma endregion

#pragma region Create nary functions
  signal create_nary_and( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( true ), [this]( auto const& a, auto const& b ) { return create_and( a, b ); } );
  }

  signal create_nary_or( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_or( a, b ); } );
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), get_constant( false ), [this]( auto const& a, auto const& b ) { return create_xor( a, b ); } );
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal _create_node( std::vector<signal> const& children, uint32_t literal )
  {
    storage::element_type::node_type node;
    std::copy( children.begin(), children.end(), std::back_inserter( node.children ) );
    node.data[2].h1 = literal;

    const auto index = _storage->nodes.size();
    _storage->nodes.push_back( node );

    /* increase ref-count to children */
    for ( auto c : children )
    {
      _storage->nodes[c.index].data[1].h2++; /* TODO: increase fanout count for output */
      _storage->nodes[c.index].data[2 + c.output].h2++;
    }

    set_value( index, 0 );

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, 0 };
  }

  signal _create_node( std::vector<signal> const& children, std::vector<uint32_t> const& literals )
  {
    storage::element_type::node_type node;
    std::copy( children.begin(), children.end(), std::back_inserter( node.children ) );

    node.data = decltype( node.data )( 2 + literals.size() );

    for ( auto i = 0; i < literals.size(); ++i )
      node.data[2 + i].h1 = literals[i];

    const auto index = _storage->nodes.size();
    _storage->nodes.push_back( node );

    /* increase ref-count to children */
    for ( auto c : children )
    {
      _storage->nodes[c.index].data[1].h2++;
      _storage->nodes[c.index].data[2 + c.output].h2++;
    }

    set_value( index, 0 );

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, 0 };
  }

  signal create_node( std::vector<signal> const& children, kitty::dynamic_truth_table const& function )
  {
    if ( children.size() == 0u )
    {
      assert( function.num_vars() == 0u );
      return get_constant( !kitty::is_const0( function ) );
    }
    return _create_node( children, _storage->data.cache.insert( function ) );
  }

  signal create_node( std::vector<signal> const& children, std::vector<kitty::dynamic_truth_table> const& functions )
  {
    assert( functions.size() > 0 );

    if ( children.size() == 0u )
    {
      assert( functions[0].num_vars() == 0u );
      return get_constant( !kitty::is_const0( functions[0] ) );
    }
    std::vector<uint32_t> literals;
    for ( auto const& tt : functions )
      literals.push_back( _storage->data.cache.insert( tt ) );

    return _create_node( children, literals );
  }

  signal clone_node( block_network const& other, node const& source, std::vector<signal> const& children )
  {
    assert( !children.empty() );
    if ( other.is_multioutput( source ) )
    {
      std::vector<kitty::dynamic_truth_table> tts;
      for ( auto i = 2; i < other._storage->nodes[source].data.size(); ++i )
        tts.push_back( other._storage->data.cache[other._storage->nodes[source].data[i].h1] );
      return create_node( children, tts );
    }
    else
    {
      const auto tt = other._storage->data.cache[other._storage->nodes[source].data[2].h1];
      return create_node( children, tt );
    }
  }
#pragma endregion

#pragma region Restructuring
  void replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    bool in_fanin = false;
    auto& nobj = _storage->nodes[n];
    for ( auto& child : nobj.children )
    {
      if ( child.index == old_node )
      {
        in_fanin = true;
        break;
      }
    }

    if ( !in_fanin )
      return;

    // remember before
    std::vector<signal> old_children( nobj.children.size() );
    std::transform( nobj.children.begin(), nobj.children.end(), old_children.begin(), []( auto c ) { return signal{ c }; } );

    /* replace in node */
    for ( auto& child : nobj.children )
    {
      if ( child.index == old_node )
      {
        child = signal{ new_signal.data ^ ( child.data & 1 ) };
        // increment fan-out of new node
        _storage->nodes[new_signal.index].data[1].h2++;
        _storage->nodes[new_signal.index].data[2 + new_signal.output].h2++;
      }
    }

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, old_children );
    }
  }

  void replace_in_node_no_restrash( node const& n, node const& old_node, signal new_signal )
  {
    replace_in_node( n, old_node, new_signal );
  }

  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    if ( is_dead( old_node ) )
      return;

    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output = signal{ new_signal.data ^ ( output.data & 1 ) };

        if ( old_node != new_signal.index )
        {
          /* increment fan-in of new node */
          _storage->nodes[new_signal.index].data[1].h2++;
          _storage->nodes[new_signal.index].data[2 + new_signal.output].h2++;
        }
      }
    }
  }

  void take_out_node( node const& n )
  {
    /* we cannot delete CIs, constants, or already dead nodes */
    if ( n < 2 || is_ci( n ) )
      return;

    /* delete the node */
    auto& nobj = _storage->nodes[n];
    nobj.data[1].h2 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */

    /* remove fanout count over output pins */
    for ( uint32_t i = 2; i < _storage->nodes[n].data.size(); ++i )
    {
      nobj.data[i].h2 = 0;
    }

    for ( auto const& fn : _events->on_delete )
    {
      ( *fn )( n );
    }

    /* if the node has been deleted, then deref fanout_size of
       fanins and try to take them out if their fanout_size become 0 */
    for ( auto i = 0; i < nobj.children.size(); ++i )
    {
      auto& child = nobj.children[i];
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }

      decr_fanout_size_pin( nobj.children[i].index, signal{ child }.output );
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_node( nobj.children[i].index );
      }
    }
  }

  void revive_node( node const& n )
  {
    assert( !is_dead( n ) );
    return;
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    /* find all parents from old_node */
    for ( auto idx = 2u; idx < _storage->nodes.size(); ++idx )
    {
      if ( is_ci( idx ) || is_dead( idx ) )
        continue; /* ignore CIs and dead nodes */

      replace_in_node( idx, old_node, new_signal );
    }

    /* check outputs */
    replace_in_outputs( old_node, new_signal );

    /* recursively reset old node */
    if ( old_node != new_signal.index )
    {
      take_out_node( old_node );
    }
  }

  void substitute_node_no_restrash( node const& old_node, signal const& new_signal )
  {
    substitute_node( old_node, new_signal );
  }

  inline bool is_dead( node const& n ) const
  {
    /* A dead node is simply a dangling node */
    return ( _storage->nodes[n].data[1].h2 >> 31 ) & 1;
  }
#pragma endregion

#pragma region Structural properties
  auto size() const
  {
    return static_cast<uint32_t>( _storage->nodes.size() );
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  auto num_pis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  auto num_pos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  auto num_gates() const
  {
    return static_cast<uint32_t>( _storage->nodes.size() - _storage->inputs.size() - 2 );
  }

  uint32_t num_outputs( node const& n ) const
  {
    return static_cast<uint32_t>( _storage->nodes[n].data.size() - 2 );
  }

  uint32_t fanin_size( node const& n ) const
  {
    return static_cast<uint32_t>( _storage->nodes[n].children.size() );
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[1].h2 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[1].h2++ & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].data[1].h2 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size_pin( node const& n, uint32_t pin_index ) const
  {
    return _storage->nodes[n].data[2 + pin_index].h2++;
  }

  uint32_t decr_fanout_size_pin( node const& n, uint32_t pin_index ) const
  {
    return --_storage->nodes[n].data[2 + pin_index].h2;
  }

  uint32_t fanout_size_pin( node const& n, uint32_t pin_index ) const
  {
    return _storage->nodes[n].data[2 + pin_index].h2;
  }

  bool is_function( node const& n ) const
  {
    return n > 1 && !is_ci( n );
  }

  bool is_and( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].data.size() == 3 && _storage->nodes[n].data[2].h1 == 4;
  }

  bool is_and( signal const& f ) const
  {
    return f.index > 1 && _storage->nodes[f.index].data[2 + f.output].h1 == 4;
  }

  bool is_or( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].data.size() == 3 && _storage->nodes[n].data[2].h1 == 6;
  }

  bool is_or( signal const& f ) const
  {
    return f.index > 1 && _storage->nodes[f.index].data[2 + f.output].h1 == 6;
  }

  bool is_xor( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].data.size() == 3 && _storage->nodes[n].data[2].h1 == 12;
  }

  bool is_xor( signal const& f ) const
  {
    return f.index > 1 && _storage->nodes[f.index].data[2 + f.output].h1 == 12;
  }

  bool is_maj( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].data.size() == 3 && _storage->nodes[n].data[2].h1 == 14;
  }

  bool is_maj( signal const& f ) const
  {
    return f.index > 1 && _storage->nodes[f.index].data[2 + f.output].h1 == 14;
  }

  bool is_ite( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].data.size() == 3 && _storage->nodes[n].data[2].h1 == 16;
  }

  bool is_ite( signal const& f ) const
  {
    return f.index > 1 && _storage->nodes[f.index].data[2 + f.output].h1 == 16;
  }

  bool is_xor3( node const& n ) const
  {
    return n > 1 && _storage->nodes[n].data.size() == 3 && _storage->nodes[n].data[2].h1 == 18;
  }

  bool is_xor3( signal const& f ) const
  {
    return f.index > 1 && _storage->nodes[f.index].data[2 + f.output].h1 == 18;
  }
#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
    return _storage->data.cache[_storage->nodes[n].data[2].h1];
  }

  kitty::dynamic_truth_table node_function_pin( const node& n, uint32_t pin_index ) const
  {
    return _storage->data.cache[_storage->nodes[n].data[2 + pin_index].h1];
  }
#pragma endregion

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return { n, 0 };
  }

  signal make_signal( node const& n, uint32_t output_pin ) const
  {
    return { n, 0, output_pin };
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement ? true : false;
  }

  uint32_t get_output_pin( signal const& f ) const
  {
    return static_cast<uint32_t>( f.output );
  }

  signal next_output_pin( signal const& f ) const
  {
    return { f.index, f.complement, f.output + 1 };
  }

  uint32_t node_to_index( node const& n ) const
  {
    return static_cast<uint32_t>( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *( _storage->inputs.begin() + index );
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *( _storage->outputs.begin() + index );
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return *( _storage->inputs.begin() + index );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    return *( _storage->outputs.begin() + index );
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    auto r = range<uint64_t>( _storage->nodes.size() );
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    using IteratorType = decltype( _storage->outputs.begin() );
    detail::foreach_element_transform<IteratorType, signal>(
        _storage->outputs.begin(), _storage->outputs.end(), []( auto f ) { return signal( f ); }, fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    using IteratorType = decltype( _storage->outputs.begin() );
    detail::foreach_element_transform<IteratorType, signal>(
        _storage->outputs.begin(), _storage->outputs.end(), []( auto f ) { return signal( f ); }, fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 2u, _storage->nodes.size() ); /* start from 2 to avoid constants */
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_ci( n ) && !is_dead( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_ci( n ) )
      return;

    using IteratorType = decltype( _storage->outputs.begin() );
    detail::foreach_element_transform<IteratorType, signal>(
        _storage->nodes[n].children.begin(), _storage->nodes[n].children.end(), []( auto f ) { return signal( f ); }, fn );
  }
#pragma endregion

#pragma region Simulate values // (Works on single-output gates only)
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    uint32_t index{ 0 };
    auto it = _storage->nodes[n].children.begin();
    while ( begin != end )
    {
      index <<= 1;
      index ^= *begin++ ? ( ~( it->weight ) & 1 ) : ( ( it->weight ) & 1 );
      ++it;
    }
    return kitty::get_bit( _storage->data.cache[_storage->nodes[n].data[2].h1], index );
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    const auto nfanin = _storage->nodes[n].children.size();

    std::vector<typename std::iterator_traits<Iterator>::value_type> tts( begin, end );

    assert( nfanin != 0 );
    assert( tts.size() == nfanin );

    /* adjust polarities */
    for ( auto j = 0u; j < nfanin; ++j )
    {
      if ( _storage->nodes[n].children[j].weight & 1 )
        tts[j] = ~tts[j];
    }

    /* resulting truth table has the same size as any of the children */
    auto result = tts.front().construct();
    const auto gate_tt = _storage->data.cache[_storage->nodes[n].data[2].h1];

    for ( uint32_t i = 0u; i < static_cast<uint32_t>( result.num_bits() ); ++i )
    {
      uint32_t pattern = 0u;
      for ( auto j = 0u; j < nfanin; ++j )
      {
        pattern |= kitty::get_bit( tts[j], i ) << j;
      }
      if ( kitty::get_bit( gate_tt, pattern ) )
      {
        kitty::set_bit( result, i );
      }
    }

    return result;
  }
#pragma endregion

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[0].h1 = 0; } );
  }

  uint32_t value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[0].h1 = v;
  }

  uint32_t incr_value( node const& n ) const
  {
    return static_cast<uint32_t>( _storage->nodes[n].data[0].h1++ );
  }

  uint32_t decr_value( node const& n ) const
  {
    return static_cast<uint32_t>( --_storage->nodes[n].data[0].h1 );
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[1].h1 = 0; } );
  }

  auto visited( node const& n ) const
  {
    return _storage->nodes[n].data[1].h1;
  }

  void set_visited( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[1].h1 = v;
  }

  uint32_t trav_id() const
  {
    return _storage->trav_id;
  }

  void incr_trav_id() const
  {
    ++_storage->trav_id;
  }
#pragma endregion

#pragma region General methods
  auto& events() const
  {
    return *_events;
  }
#pragma endregion

public:
  std::shared_ptr<block_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;
};

} // namespace mockturtle
