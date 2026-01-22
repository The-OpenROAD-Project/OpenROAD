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
  \file aqfp.hpp
  \brief AQFP network implementation

  \author Alessandro Tempia Calvino
  \author Dewmini Sudara Marakkalage
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <limits>
#include <memory>
#include <optional>
#include <stack>
#include <string>

#include <fmt/format.h>
#include <kitty/kitty.hpp>

#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "detail/foreach.hpp"
#include "events.hpp"
#include "storage.hpp"

namespace mockturtle
{

struct aqfp_storage_data
{
  std::unordered_map<uint32_t, kitty::dynamic_truth_table> node_fn_cache;
};

/*! \brief AQFP storage container

  We use one bit of the index pointer to store a complemented attribute.
  Every node has 64-bit of additional data used for the following purposes:

  `data[0].h1`: Fan-out size (we use MSB to indicate whether a node is dead)
  `data[0].h2`: Application-specific value
  `data[1].h1`: Visited flag
*/
using aqfp_storage = storage<mixed_fanin_node<2, 1>, aqfp_storage_data>;

class aqfp_network
{
public:
#pragma region Types and constructors
  static constexpr auto min_fanin_size = 3u;
  static constexpr auto max_fanin_size = 5u;

  using base_type = aqfp_network;
  using storage = std::shared_ptr<aqfp_storage>;
  using node = uint64_t;

  struct signal
  {
    signal() = default;

    signal( uint64_t index, uint64_t complement )
        : complement( complement ), index( index )
    {
    }

    explicit signal( uint64_t data )
        : data( data )
    {
    }

    signal( aqfp_storage::node_type::pointer_type const& p )
        : complement( p.weight ), index( p.index )
    {
    }

    union
    {
      struct
      {
        uint64_t complement : 1;
        uint64_t index : 63;
      };
      uint64_t data;
    };

    signal operator!() const
    {
      return signal( data ^ 1 );
    }

    signal operator+() const
    {
      return { index, 0 };
    }

    signal operator-() const
    {
      return { index, 1 };
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

    operator aqfp_storage::node_type::pointer_type() const
    {
      return { index, complement };
    }

#if __cplusplus > 201703L
    bool operator==( aqfp_storage::node_type::pointer_type const& other ) const
    {
      return data == other.data;
    }
#endif
  };

  aqfp_network()
      : _storage( std::make_shared<aqfp_storage>() ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
    _storage->nodes[0].children.resize( 3u );
    _storage->nodes[0].children[0].data = _storage->nodes[0].children[0].data = _storage->nodes[0].children[0].data = static_cast<uint64_t>( 0u );
  }

  aqfp_network( std::shared_ptr<aqfp_storage> storage )
      : _storage( storage ),
        _events( std::make_shared<decltype( _events )::element_type>() )
  {
  }
#pragma endregion

#pragma region Primary I / O and constants
  signal get_constant( bool value ) const
  {
    return { 0, static_cast<uint64_t>( value ? 1 : 0 ) };
  }

  signal create_pi()
  {
    const auto index = _storage->nodes.size();
    auto& node = _storage->nodes.emplace_back();
    node.children.resize( 3u );
    node.children[0].data = node.children[1].data = node.children[2].data = ~static_cast<uint64_t>( 0 );
    _storage->inputs.emplace_back( index );
    return { index, 0 };
  }

  uint32_t create_po( signal const& f )
  {
    /* increase ref-count to children */
    _storage->nodes[f.index].data[0].h1++;
    auto const po_index = static_cast<uint32_t>( _storage->outputs.size() );
    _storage->outputs.emplace_back( f.index, f.complement );
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

  bool is_ci( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data && _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data;
  }

  bool is_pi( node const& n ) const
  {
    return _storage->nodes[n].children[0].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[1].data == ~static_cast<uint64_t>( 0 ) && _storage->nodes[n].children[2].data == ~static_cast<uint64_t>( 0 );
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

#pragma region Create binary / ternary functions
  signal create_maj( signal a, signal b, signal c )
  {
    /* order inputs */
    if ( a.index > b.index )
    {
      std::swap( a, b );
      if ( b.index > c.index )
        std::swap( b, c );
      if ( a.index > b.index )
        std::swap( a, b );
    }
    else
    {
      if ( b.index > c.index )
        std::swap( b, c );
      if ( a.index > b.index )
        std::swap( a, b );
    }

    /* trivial cases */
    if ( a.index == b.index )
    {
      return ( a.complement == b.complement ) ? a : c;
    }
    else if ( b.index == c.index )
    {
      return ( b.complement == c.complement ) ? b : a;
    }

    /*  complemented edges minimization */
    auto node_complement = false;
    if ( static_cast<unsigned>( a.complement ) + static_cast<unsigned>( b.complement ) + static_cast<unsigned>( c.complement ) >= 2u )
    {
      node_complement = true;
      a.complement = !a.complement;
      b.complement = !b.complement;
      c.complement = !c.complement;
    }

    storage::element_type::node_type node;

    node.children.resize( 3u );
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    const auto index = _storage->nodes.size();

    _storage->nodes.push_back( node );

    /* increase ref-count to children */
    _storage->nodes[a.index].data[0].h1++;
    _storage->nodes[b.index].data[0].h1++;
    _storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
  }

  signal create_maj( std::vector<signal> children )
  {
    assert( children.size() > 0u );
    assert( children.size() % 2 == 1u );

    if ( children.size() == 1u )
    {
      return children[0u];
    }

    std::stable_sort( children.begin(), children.end(), []( auto f, auto s ) { return f.index < s.index; } );

    for ( auto i = 1u; i < children.size(); i++ )
    {
      if ( children[i - 1].index == children[i].index && children[i - 1].complement != children[i].complement )
      {
        children.erase( children.begin() + ( i - 1 ), children.begin() + ( i + 1 ) );
        return create_maj( children );
      }
    }

    for ( auto i = 0u; i < children.size(); i++ )
    {
      const auto index = children[i].index;
      const auto complement = children[i].complement;
      auto j = i + 1;
      while ( j < children.size() && children[j].index == index && children[j].complement == complement )
      {
        j++;
      }
      if ( j - i > children.size() / 2 )
      {
        return { index, complement };
      }
    }

    auto node_complement = false;

    auto num_complemented = 0u;
    for ( const auto& c : children )
    {
      num_complemented += static_cast<unsigned>( c.complement );
    }

    if ( num_complemented > children.size() / 2 )
    {
      node_complement = true;
      for ( auto& c : children )
      {
        c.complement = !c.complement;
      }
    }

    storage::element_type::node_type node;

    for ( const auto& c : children )
    {
      node.children.push_back( c );
    }

    const auto index = _storage->nodes.size();

    _storage->nodes.push_back( node );

    /* increase ref-count to children */
    for ( const auto& c : children )
    {
      _storage->nodes[c.index].data[0].h1++;
    }

    for ( auto const& fn : _events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
  }

  signal create_and( signal const& a, signal const& b )
  {
    return create_maj( get_constant( false ), a, b );
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_or( signal const& a, signal const& b )
  {
    return create_maj( get_constant( true ), a, b );
  }

  signal create_nor( signal const& a, signal const& b )
  {
    return !create_or( a, b );
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
    const auto fcompl = a.complement ^ b.complement;
    const auto c1 = create_and( +a, -b );
    const auto c2 = create_and( +b, -a );
    return create_and( !c1, !c2 ) ^ !fcompl;
  }

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

    return create_and( !create_and( !cond, f_else ), !create_and( cond, f_then ) ) ^ !f_compl;
  }

  signal create_xor3( signal const& a, signal const& b, signal const& c )
  {
    const auto f = create_maj( a, !b, c );
    const auto g = create_maj( a, b, !c );
    return create_maj( !a, f, g );
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
  signal clone_node( aqfp_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( children.size() > 1 && children.size() % 2 == 1 );
    return create_maj( children );
  }
#pragma endregion

#pragma region Restructuring
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal )
  {
    auto& node = _storage->nodes[n];

    std::vector<signal> old_children;

    bool replacement = false;
    for ( size_t i = 0u; i < node.children.size(); ++i )
    {
      old_children.push_back( signal{ node.children[i] } );

      if ( node.children[i].index == old_node )
      {
        node.children[i] = node.children[i].weight ? !new_signal : new_signal;
        replacement = true;

        // update the reference counter of the new signal
        _storage->nodes[new_signal.index].data[0].h1++;
      }
    }

    if ( !replacement )
    {
      return std::nullopt;
    }

    /* TODO: Do the simplifications if possible and ordering */

    for ( auto const& fn : _events->on_modified )
    {
      ( *fn )( n, old_children );
    }

    return std::nullopt;
  }

  void replace_in_outputs( node const& old_node, signal const& new_signal )
  {
    for ( auto& output : _storage->outputs )
    {
      if ( output.index == old_node )
      {
        output.index = new_signal.index;
        output.weight ^= new_signal.complement;

        // increment fan-in of new node
        _storage->nodes[new_signal.index].data[0].h1++;
      }
    }
  }

  void take_out_node( node const& n )
  {
    /* we cannot delete CIs or constants */
    if ( n == 0 || is_ci( n ) )
      return;

    auto& nobj = _storage->nodes[n];
    nobj.data[0].h1 = UINT32_C( 0x80000000 ); /* fanout size 0, but dead */

    for ( auto const& fn : _events->on_delete )
    {
      ( *fn )( n );
    }

    for ( auto i = 0u; i < nobj.children.size(); ++i )
    {
      if ( fanout_size( nobj.children[i].index ) == 0 )
      {
        continue;
      }
      if ( decr_fanout_size( nobj.children[i].index ) == 0 )
      {
        take_out_node( nobj.children[i].index );
      }
    }
  }

  inline bool is_dead( node const& n ) const
  {
    return ( _storage->nodes[n].data[0].h1 >> 31 ) & 1;
  }

  void substitute_node( node const& old_node, signal const& new_signal )
  {
    std::stack<std::pair<node, signal>> to_substitute;
    to_substitute.push( { old_node, new_signal } );

    while ( !to_substitute.empty() )
    {
      const auto [_old, _new] = to_substitute.top();
      to_substitute.pop();

      for ( auto idx = 1u; idx < _storage->nodes.size(); ++idx )
      {
        if ( is_ci( idx ) )
          continue; /* ignore CIs */

        if ( const auto repl = replace_in_node( idx, _old, _new ); repl )
        {
          to_substitute.push( *repl );
        }
      }

      /* check outputs */
      replace_in_outputs( _old, _new );

      // reset fan-in of old node
      take_out_node( _old );
    }
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
    return static_cast<uint32_t>( _storage->nodes.size() - 1u - _storage->inputs.size() );
  }

  uint32_t fanin_size( node const& n ) const
  {
    if ( is_constant( n ) || is_ci( n ) )
      return 0;
    return _storage->nodes[n].children.size();
  }

  uint32_t fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t incr_fanout_size( node const& n ) const
  {
    return _storage->nodes[n].data[0].h1++ & UINT32_C( 0x7FFFFFFF );
  }

  uint32_t decr_fanout_size( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h1 & UINT32_C( 0x7FFFFFFF );
  }

  bool is_and( node const& n ) const
  {
    (void)n;
    return false;
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
    return n > 0 && !is_ci( n );
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
    (void)n;
    return false;
  }
#pragma endregion

#pragma region Functional properties
  kitty::dynamic_truth_table node_function( const node& n ) const
  {
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

#pragma region Nodes and signals
  node get_node( signal const& f ) const
  {
    return f.index;
  }

  signal make_signal( node const& n ) const
  {
    return signal( n, 0 );
  }

  bool is_complemented( signal const& f ) const
  {
    return f.complement;
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

  uint32_t ci_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( _storage->nodes[n].children[0].data == _storage->nodes[n].children[1].data &&
            _storage->nodes[n].children[0].data == _storage->nodes[n].children[2].data );
    return static_cast<uint32_t>( _storage->nodes[n].children[0].data );
  }

  uint32_t po_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_po( [&]( const auto& x, auto index ) {
      if ( x == s )
      {
        i = index;
        return false;
      }
      return true;
    } );
    return i;
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
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->inputs.begin(), _storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _storage->outputs.begin(), _storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    auto r = range<uint64_t>( 1u, _storage->nodes.size() ); // start from 1 to avoid constant
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

  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute_majority_n_with_bool( Iterator begin, Iterator end ) const
  {
    std::vector<std::vector<typename Iterator::value_type>> dp;
    std::vector<typename Iterator::value_type> v( begin, end );

    const auto n = v.size();
    typename Iterator::value_type one = true;

    for ( auto i = 0u; i <= n; i++ )
    {
      dp.push_back( { one } );
      if ( i == 0u )
        continue;
      auto ith_var = v[i - 1];
      for ( auto j = 1u; j <= i && j <= ( n / 2 ) + 1; j++ )
      {
        dp[i].push_back( ( j < i ) ? ( ith_var & dp[i - 1][j - 1] ) || dp[i - 1][j] : ( ith_var && dp[i - 1][j - 1] ) );
      }
    }

    return ( dp[n][( n / 2 ) + 1] );
  }

  template<typename Iterator>
  auto compute_majority_n( Iterator begin, Iterator end ) const
  {
    std::vector<std::vector<typename Iterator::value_type>> dp;
    std::vector<typename Iterator::value_type> v( begin, end );

    const auto n = v.size();
    typename Iterator::value_type one = ~( v[0] ^ v[0] );

    for ( auto i = 0u; i <= n; i++ )
    {
      dp.push_back( { one } );
      if ( i == 0u )
        continue;
      auto ith_var = v[i - 1];
      for ( auto j = 1u; j <= i && j <= ( n / 2 ) + 1; j++ )
      {
        dp[i].push_back( ( j < i ) ? ( ith_var & dp[i - 1][j - 1] ) | dp[i - 1][j] : ( ith_var & dp[i - 1][j - 1] ) );
      }
    }

    return ( dp[n][( n / 2 ) + 1] );
  }

#pragma region Value simulation
  template<typename Iterator>
  iterates_over_t<Iterator, bool>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_ci( n ) );

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

#pragma region Custom node values
  void clear_values() const
  {
    std::for_each( _storage->nodes.begin(), _storage->nodes.end(), []( auto& n ) { n.data[0].h2 = 0; } );
  }

  auto value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2;
  }

  void set_value( node const& n, uint32_t v ) const
  {
    _storage->nodes[n].data[0].h2 = v;
  }

  auto incr_value( node const& n ) const
  {
    return _storage->nodes[n].data[0].h2++;
  }

  auto decr_value( node const& n ) const
  {
    return --_storage->nodes[n].data[0].h2;
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
  std::shared_ptr<aqfp_storage> _storage;
  std::shared_ptr<network_events<base_type>> _events;
};

} // namespace mockturtle

namespace std
{

template<>
struct hash<mockturtle::aqfp_network::signal>
{
  uint64_t operator()( mockturtle::aqfp_network::signal const& s ) const noexcept
  {
    uint64_t k = s.data;
    k ^= k >> 33;
    k *= 0xff51afd7ed558ccd;
    k ^= k >> 33;
    k *= 0xc4ceb9fe1a85ec53;
    k ^= k >> 33;
    return k;
  }
}; /* hash */

} // namespace std