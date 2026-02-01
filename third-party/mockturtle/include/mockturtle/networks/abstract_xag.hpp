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
  \file abstract_xag.hpp
  \brief Abstract XAG logic network implementation

  This network type is (for now) only meant for experimental use cases.

  \author Mathias Soeken
*/

#pragma once

#include <memory>
#include <optional>
#include <stack>
#include <string>

#include <fmt/format.h>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <parallel_hashmap/phmap.h>

#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "detail/foreach.hpp"

namespace mockturtle
{

struct abstract_xag_storage
{
  struct node_type
  {
    uint32_t* fanin{};
    uint32_t fanin_size{};
    uint32_t fanout_size{};
    uint32_t value{};
    uint32_t visited{};
    uint32_t level{};
  };

  abstract_xag_storage()
  {
    /* constant 0 node */
    nodes.emplace_back();

    // nodes.reserve( 10000u );
  }

  ~abstract_xag_storage()
  {
    for ( auto& n : nodes )
    {
      if ( n.fanin )
      {
        delete[] n.fanin;
      }
    }
  }

  struct abstract_xag_node_eq
  {
    bool operator()( abstract_xag_storage::node_type const& a, abstract_xag_storage::node_type const& b ) const
    {
      return a.fanin_size == b.fanin_size && std::equal( a.fanin, a.fanin + a.fanin_size, b.fanin );
    }
  };

  struct abstract_xag_node_hash
  {
    uint64_t operator()( abstract_xag_storage::node_type const& n ) const
    {
      return std::accumulate( n.fanin, n.fanin + n.fanin_size, UINT64_C( 0 ), [&]( auto accu, auto c ) { return accu + std::hash<uint32_t>{}( c ); } );
    }
  };

  std::vector<node_type> nodes;
  std::vector<uint32_t> children;
  std::vector<uint32_t> inputs;
  std::vector<std::pair<uint32_t, bool>> outputs;
  phmap::flat_hash_map<node_type, uint32_t, abstract_xag_node_hash, abstract_xag_node_eq> hash;

  uint32_t trav_id = 0u;
  uint32_t depth = 0u;
};

class abstract_xag_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 1u;
  static constexpr auto max_fanin_size = std::numeric_limits<uint32_t>::max();

  using base_type = abstract_xag_network;
  using storage_type = abstract_xag_storage;
  using storage = std::shared_ptr<storage_type>;
  using node = uint32_t;

  struct signal
  {
    signal() = default;
    signal( uint32_t index, bool complement = false ) : index( index ), complement( complement ) {}

    uint32_t index;
    bool complement;

    signal operator!() const
    {
      return { index, !complement };
    }

    signal operator+() const
    {
      return { index, false };
    }

    signal operator-() const
    {
      return { index, true };
    }

    signal operator^( bool complement ) const
    {
      return { index, this->complement != complement };
    }

    bool operator==( signal const& other ) const
    {
      return index == other.index && complement == other.complement;
    }

    bool operator!=( signal const& other ) const
    {
      return index != other.index || complement != other.complement;
    }

    bool operator<( signal const& other ) const
    {
      return index < other.index || ( index == other.index && !complement && other.complement );
    }
  };

  abstract_xag_network()
      : _storage( std::make_shared<storage_type>() )
  {
  }

  abstract_xag_network( std::shared_ptr<storage_type> storage )
      : _storage( storage )
  {
  }
#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant( bool value ) const
  {
    return { 0, value };
  }

  signal create_pi()
  {
    const auto index = static_cast<uint32_t>( _storage->nodes.size() );
    _storage->nodes.emplace_back();
    _storage->inputs.emplace_back( index );
    return { index, 0 };
  }

  uint32_t create_po( signal const& f )
  {
    /* increase ref-count to children */
    _storage->nodes[f.index].fanout_size++;
    auto const po_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
    _storage->depth = std::max( _storage->depth, level( f.index ) );
    return po_index;
  }

  bool is_combinational() const
  {
    return true;
  }

  bool is_constant( node const& n ) const
  {
    return n == 0;
  }

  bool is_pi( node const& n ) const
  {
    return n > 0 && _storage->nodes[n].fanin_size == 0u;
  }

  bool is_ci( node const& n ) const
  {
    return n > 0 && _storage->nodes[n].fanin_size == 0u;
  }

  bool constant_value( node const& n ) const
  {
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Create unary functions
  signal create_buf( signal const& a )
  {
    return a;
  }

  signal create_not( signal const& a )
  {
    return !a;
  }
#pragma endregion

#pragma region Create binary functions
  signal _create_node( std::vector<uint32_t> const& fanin, uint32_t level_offset )
  {
    storage::element_type::node_type node;
    node.fanin = new uint32_t[fanin.size()];
    node.fanin_size = fanin.size();
    std::copy( fanin.begin(), fanin.end(), node.fanin );

    /* structural hashing */
    if ( const auto it = _storage->hash.find( node ); it != _storage->hash.end() )
    {
      delete[] node.fanin;
      return { it->second, 0 };
    }

    const auto index = static_cast<uint32_t>( _storage->nodes.size() );
    _storage->nodes.push_back( node );
    _storage->hash.emplace( node, index );

    /* increase ref-count to children */
    uint32_t _level = 0u;
    for ( auto const& f : fanin )
    {
      _storage->nodes[f].fanout_size++;
      _level = std::max( _level, level( f ) );
    }
    _storage->nodes[index].level = _level + level_offset;

    return { index, 0u };
  }

  signal create_and( signal a, signal b )
  {
    /* order inputs a > b it is a AND */
    if ( a.index < b.index )
    {
      std::swap( a, b );
    }
    /* trivial cases */
    if ( a.index == b.index )
    {
      return a.complement == b.complement ? a : get_constant( false );
    }
    else if ( b.index == 0 )
    {
      return b.complement == false ? get_constant( false ) : a;
    }
    /* constant propagation */
    else if ( a.complement && b.complement )
    {
      return !create_nary_xor( { +a, +b, create_and( +a, +b ) } );
    }
    else if ( a.complement )
    {
      return create_xor( create_and( +a, b ), b );
    }
    else if ( b.complement )
    {
      return create_xor( create_and( a, +b ), a );
    }

    /* subset resolution */
    const auto& anode = _storage->nodes[a.index];
    const auto& bnode = _storage->nodes[b.index];

    const auto a_begin = is_nary_xor( a.index ) ? anode.fanin : &a.index;
    const auto a_end = is_nary_xor( a.index ) ? anode.fanin + anode.fanin_size : &a.index + 1;
    const auto b_begin = is_nary_xor( b.index ) ? bnode.fanin : &b.index;
    const auto b_end = is_nary_xor( b.index ) ? bnode.fanin + bnode.fanin_size : &b.index + 1;

    if ( std::includes( a_begin, a_end, b_begin, b_end ) )
    {
      std::vector<uint32_t> set_anew;
      std::set_difference( a_begin, a_end, b_begin, b_end, std::back_inserter( set_anew ) );
      return create_xor( b, create_and( b, _create_nary_xor( set_anew ) ) );
    }
    else if ( std::includes( b_begin, b_end, a_begin, a_end ) )
    {
      std::vector<uint32_t> set_bnew;
      std::set_difference( b_begin, b_end, a_begin, a_end, std::back_inserter( set_bnew ) );
      return create_xor( a, create_and( a, _create_nary_xor( set_bnew ) ) );
    }

    return _create_node( std::vector<uint32_t>{ { a.index, b.index } }, 1u );
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_or( signal const& a, signal const& b )
  {
    return !create_and( !a, !b );
  }

  signal create_nor( signal const& a, signal const& b )
  {
    return create_and( !a, !b );
  }

  signal create_lt( signal const& a, signal const& b )
  {
    return create_and( !a, b );
  }

  signal create_le( signal const& a, signal const& b )
  {
    return !create_and( a, !b );
  }

  signal create_xor( signal const& a, signal const& b )
  {
    return create_nary_xor( { a, b } );
  }

  signal create_xnor( signal const& a, signal const& b )
  {
    return !create_xor( a, b );
  }
#pragma endregion

#pragma region Create ternary functions
  signal create_ite( signal cond, signal f_then, signal f_else )
  {
    bool f_compl{ false };
    if ( f_then.index < f_else.index )
    {
      std::swap( f_then, f_else );
      cond.complement ^= 1;
    }
    if ( f_then.complement )
    {
      f_then.complement = 0;
      f_else.complement ^= 1;
      f_compl = true;
    }

    return create_xor( create_and( !cond, create_xor( f_then, f_else ) ), f_then ) ^ f_compl;
  }

  signal create_maj( signal const& a, signal const& b, signal const& c )
  {
    auto c1 = create_xor( a, b );
    auto c2 = create_xor( a, c );
    auto c3 = create_and( c1, c2 );
    return create_xor( a, c3 );
  }

  signal create_xor3( signal const& a, signal const& b, signal const& c )
  {
    return create_nary_xor( { a, b, c } );
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

  signal _create_nary_xor( std::vector<uint32_t> const& fs )
  {
    std::vector<uint32_t> _fs;

    const auto merge_one = [&]( uint32_t f ) {
      const auto it = std::lower_bound( _fs.begin(), _fs.end(), f );
      if ( it != _fs.end() && *it == f )
      {
        _fs.erase( it );
      }
      else
      {
        _fs.insert( it, f );
      }
    };

    const auto merge_many = [&]( uint32_t* begin, uint32_t* end ) {
      std::vector<uint32_t> tmp;
      std::set_symmetric_difference( _fs.begin(), _fs.end(), begin, end, std::back_inserter( tmp ) );
      _fs = std::move( tmp );
    };

    for ( auto const& f : fs )
    {
      auto const& node = _storage->nodes[f];
      if ( node.fanin_size == 0u )
      {
        merge_one( f );
      }
      else if ( node.fanin[0] > node.fanin[1] )
      {
        merge_one( f );
      }
      else
      {
        merge_many( node.fanin, node.fanin + node.fanin_size );
      }
    }

    if ( _fs.empty() )
    {
      return get_constant( false );
    }
    else if ( _fs.size() == 1u )
    {
      return { _fs.front(), false };
    }
    else
    {
      return _create_node( _fs, 0u );
    }
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    std::vector<uint32_t> _fs;

    const auto merge_one = [&]( uint32_t f ) {
      const auto it = std::lower_bound( _fs.begin(), _fs.end(), f );
      if ( it != _fs.end() && *it == f )
      {
        _fs.erase( it );
      }
      else
      {
        _fs.insert( it, f );
      }
    };

    const auto merge_many = [&]( uint32_t* begin, uint32_t* end ) {
      std::vector<uint32_t> tmp;
      std::set_symmetric_difference( _fs.begin(), _fs.end(), begin, end, std::back_inserter( tmp ) );
      _fs = std::move( tmp );
    };

    bool complement{ false };
    for ( auto const& f : fs )
    {
      complement ^= f.complement;
      auto const& node = _storage->nodes[f.index];
      if ( f.index == 0 )
      {
        // do nothing
      }
      else if ( node.fanin_size == 0u )
      {
        merge_one( f.index );
      }
      else if ( node.fanin[0] > node.fanin[1] )
      {
        merge_one( f.index );
      }
      else
      {
        merge_many( node.fanin, node.fanin + node.fanin_size );
      }
    }

    if ( _fs.empty() )
    {
      return get_constant( complement );
    }
    else if ( _fs.size() == 1u )
    {
      return { _fs.front(), complement };
    }
    else
    {
      return _create_node( _fs, 0u ) ^ complement;
    }
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( abstract_xag_network const& other, node const& source, std::vector<signal> const& children )
  {
    if ( other.is_and( source ) )
    {
      return create_and( children[0u], children[1u] );
    }
    else
    {
      return create_nary_xor( children );
    }
  }
#pragma endregion

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return { n, false };
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement;
  }

  uint32_t node_to_index( node const& n ) const
  {
    return n;
  }

  node index_to_node( uint32_t index ) const
  {
    return index;
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _storage->inputs.size() );
    return _storage->inputs[index];
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _storage->outputs.size() );
    const auto po = _storage->outputs[index];
    return { po.first, po.second };
  }
#pragma endregion

#pragma region Node and signal iterators
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    auto r = range<uint32_t>( _storage->nodes.size() );
    detail::foreach_element( r.begin(), r.end(), fn );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    using Iterator = decltype( _storage->outputs.begin() );
    using ElementType = signal;
    detail::foreach_element_transform<Iterator, ElementType>(
        _storage->outputs.begin(), _storage->outputs.end(), []( auto const& po ) -> signal { return { po.first, po.second }; }, fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    using Iterator = decltype( _storage->outputs.begin() );
    detail::foreach_element_transform<Iterator, signal>(
        _storage->outputs.begin(), _storage->outputs.end(), []( auto const& po ) -> signal { return { po.first, po.second }; }, fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint32_t>( 1u, _storage->nodes.size() ); /* start from 1 to avoid constants */
    detail::foreach_element_if(
        r.begin(), r.end(),
        [this]( auto n ) { return !is_pi( n ); },
        fn );
  }

  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const
  {
    if ( n == 0 || is_pi( n ) )
      return;

    const auto& node = _storage->nodes[n];
    detail::foreach_element_transform<uint32_t*, signal>(
        node.fanin, node.fanin + node.fanin_size, []( auto c ) -> signal { return { c, false }; }, fn );
  }
#pragma endregion

#pragma region Structural properties
  uint32_t size() const
  {
    return num_gates() + num_pis() + 1u;
  }

  uint32_t num_pis() const
  {
    return static_cast<uint32_t>( _storage->inputs.size() );
  }

  uint32_t num_pos() const
  {
    return static_cast<uint32_t>( _storage->outputs.size() );
  }

  uint32_t num_gates() const
  {
    return static_cast<uint32_t>( _storage->hash.size() );
  }

  uint32_t fanin_size( node const& n ) const
  {
    return _storage->nodes[n].fanin_size;
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].fanout_size;
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].fanout_size++;
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].fanout_size;
  }

  uint32_t depth() const
  {
    return _storage->depth;
  }

  uint32_t level( node const& n ) const
  {
    return _storage->nodes[n].level;
  }

  bool is_and( node const& n ) const
  {
    const auto& node = _storage->nodes[n];
    return node.fanin_size == 2 && ( node.fanin[0] > node.fanin[1] );
  }

  bool is_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_maj( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_ite( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_xor3( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_and( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_or( node const& n ) const
  {
    (void)n;
    return false;
  }

  bool is_nary_xor( node const& n ) const
  {
    const auto& node = _storage->nodes[n];
    return node.fanin_size != 0u && ( node.fanin[0] < node.fanin[1] );
  }
#pragma endregion

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    assert( n != 0 && !is_pi( n ) );

    const auto& node = _storage->nodes[n];

    if ( node.fanin[0] > node.fanin[1] )
    {
      auto v1 = *begin++;
      auto v2 = *begin++;
      return v1 && v2;
    }
    else
    {
      auto v = *begin++;
      while ( begin != end )
      {
        v ^= *begin++;
      }
      return v;
    }
  }

  template<typename Iterator>
  iterates_over_truth_table_t<Iterator>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_pi( n ) );

    const auto& node = _storage->nodes[n];

    if ( node.fanin[0] > node.fanin[1] )
    {
      auto v1 = *begin++;
      auto v2 = *begin++;
      return v1 & v2;
    }
    else
    {
      auto v = *begin++;
      while ( begin != end )
      {
        v ^= *begin++;
      }
      return v;
    }
  }
#pragma endregion

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.value = 0; } );
  }

  auto value( node const& n ) const
  {
    return _storage->nodes[n].value;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].value = v;
  }

  auto incr_value( node const& n ) const
  {
    return _storage->nodes[n].value++;
  }

  auto decr_value( node const& n ) const
  {
    return --_storage->nodes[n].value;
  }
#pragma endregion

#pragma region Visited flags
  void clear_visited() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.visited = 0; } );
  }

  auto visited( node const& n ) const
  {
    return _storage->nodes[n].visited;
  }

  void set_visited( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].visited = v;
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

public:
  storage _storage;
};

} // namespace mockturtle

template<>
struct fmt::formatter<mockturtle::abstract_xag_network::signal>
{
  constexpr auto parse( format_parse_context& ctx )
  {
    auto it = ctx.begin(), end = ctx.end();
    if ( it != end && *it != '}' )
    {
      throw format_error( "invalid format" );
    }
    return it;
  }

  template<typename FormatContext>
  auto format( const mockturtle::abstract_xag_network::signal& f, FormatContext& ctx )
  {
    return format_to( ctx.out(), "{}{}", f.complement ? "~" : "", f.index );
  }
};

namespace std
{

template<>
struct hash<mockturtle::abstract_xag_network::signal>
{
  uint64_t operator()( mockturtle::abstract_xag_network::signal const& s ) const noexcept
  {
    uint64_t k = s.index;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    k ^= s.complement;
    return k;
  }
}; /* hash */

} // namespace std