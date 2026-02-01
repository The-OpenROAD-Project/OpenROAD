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
  \file fanout_limit_view.hpp
  \brief View that replicates nodes whose fanout size exceed a limit

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/mig.hpp"
#include "../utils/node_map.hpp"

namespace mockturtle
{

struct fanout_limit_view_params
{
  uint64_t fanout_limit{ 16 };
};

template<typename Ntk>
class fanout_limit_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  fanout_limit_view( fanout_limit_view_params const ps = {} )
      : replicas( *this ), ps( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    assert( ps.fanout_limit > 0u );
  }

  uint32_t create_po( signal const& f )
  {
    if ( Ntk::is_maj( Ntk::get_node( f ) ) && Ntk::fanout_size( Ntk::get_node( f ) ) + 1 > ps.fanout_limit )
    {
      return Ntk::create_po( replicate_node( f ) );
    }
    else
    {
      return Ntk::create_po( f );
    }
  }

  signal create_maj( signal const& a, signal const& b, signal const& c )
  {
    std::array<signal, 3u> fanins;
    fanins[0u] = ( Ntk::is_maj( Ntk::get_node( a ) ) && Ntk::fanout_size( Ntk::get_node( a ) ) > ps.fanout_limit - 1 ) ? replicate_node( a ) : a;
    fanins[1u] = ( Ntk::is_maj( Ntk::get_node( b ) ) && Ntk::fanout_size( Ntk::get_node( b ) ) > ps.fanout_limit - 1 ) ? replicate_node( b ) : b;
    fanins[2u] = ( Ntk::is_maj( Ntk::get_node( c ) ) && Ntk::fanout_size( Ntk::get_node( c ) ) > ps.fanout_limit - 1 ) ? replicate_node( c ) : c;
    return Ntk::create_maj( fanins[0u], fanins[1u], fanins[2u] );
  }

  signal create_and( signal const& a, signal const& b )
  {
    return create_maj( Ntk::get_constant( false ), a, b );
  }

  signal create_nand( signal const& a, signal const& b )
  {
    return !create_and( a, b );
  }

  signal create_or( signal const& a, signal const& b )
  {
    return create_maj( Ntk::get_constant( true ), a, b );
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
    return tree_reduce( fs.begin(), fs.end(), Ntk::get_constant( true ), [this]( auto const& a, auto const& b ) { return create_and( a, b ); } );
  }

  signal create_nary_or( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), Ntk::get_constant( false ), [this]( auto const& a, auto const& b ) { return create_or( a, b ); } );
  }

  signal create_nary_xor( std::vector<signal> const& fs )
  {
    return tree_reduce( fs.begin(), fs.end(), Ntk::get_constant( false ), [this]( auto const& a, auto const& b ) { return create_xor( a, b ); } );
  }
#pragma endregion

#pragma region Create arbitrary functions
  signal clone_node( mig_network const& other, node const& source, std::vector<signal> const& children )
  {
    (void)other;
    (void)source;
    assert( children.size() == 3u );
    return create_maj( children[0u], children[1u], children[2u] );
  }
#pragma endregion

  signal replicate_node( signal const& s )
  {
    return Ntk::is_complemented( s ) ? !replicate_node( Ntk::get_node( s ) ) : replicate_node( Ntk::get_node( s ) );
  }

  signal replicate_node( node const& n )
  {
    if ( replicas.has( n ) )
    {
      auto replica = replicas[n];
      if ( Ntk::fanout_size( replica ) < ps.fanout_limit )
      {
        return Ntk::make_signal( replica );
      }
    }

    std::array<signal, 3u> fanins;
    Ntk::foreach_fanin( n, [&]( signal const& f, auto index ) {
      fanins[index] = ( Ntk::is_maj( Ntk::get_node( f ) ) && Ntk::fanout_size( Ntk::get_node( f ) ) > ps.fanout_limit - 1u ) ? replicate_node( f ) : f;
    } );

    auto const new_signal = create_maj_overwrite_strash( fanins[0u], fanins[1u], fanins[2u] );
    replicas[n] = Ntk::get_node( new_signal );
    return new_signal;
  }

  uint32_t num_gates() const
  {
    return Ntk::num_gates() + count_hash_overwrites;
  }

protected:
  signal create_maj_overwrite_strash( signal a, signal b, signal c )
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
    if ( static_cast<unsigned>( a.complement ) + static_cast<unsigned>( b.complement ) +
             static_cast<unsigned>( c.complement ) >=
         2u )
    {
      node_complement = true;
      a.complement = !a.complement;
      b.complement = !b.complement;
      c.complement = !c.complement;
    }

    typename storage::element_type::node_type node;
    node.children[0] = a;
    node.children[1] = b;
    node.children[2] = c;

    /* structural hashing */
    if ( true )
    {
      const auto it = Ntk::_storage->hash.find( node );
      if ( it != Ntk::_storage->hash.end() )
      {
        ++count_hash_overwrites;
      }
    }

    const auto index = Ntk::_storage->nodes.size();

    if ( index >= .9 * Ntk::_storage->nodes.capacity() )
    {
      Ntk::_storage->nodes.reserve( static_cast<uint64_t>( 3.1415f * index ) );
      Ntk::_storage->hash.reserve( static_cast<uint64_t>( 3.1415f * index ) );
    }

    Ntk::_storage->nodes.push_back( node );
    Ntk::_storage->hash[node] = index;

    /* increase ref-count to children */
    Ntk::_storage->nodes[a.index].data[0].h1++;
    Ntk::_storage->nodes[b.index].data[0].h1++;
    Ntk::_storage->nodes[c.index].data[0].h1++;

    for ( auto const& fn : Ntk::_events->on_add )
    {
      ( *fn )( index );
    }

    return { index, node_complement };
  }

protected:
  uint32_t count_hash_overwrites{ 0 };
  unordered_node_map<node, Ntk> replicas;
  fanout_limit_view_params const ps;
};

} /* namespace mockturtle */