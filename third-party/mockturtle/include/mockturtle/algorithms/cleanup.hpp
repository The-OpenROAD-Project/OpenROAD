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
  \file cleanup.hpp
  \brief Cleans up networks

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/crossed.hpp"
#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

#include <kitty/operations.hpp>

#include <iostream>
#include <type_traits>
#include <vector>

namespace mockturtle
{

namespace detail
{

template<typename NtkSrc, typename NtkDest, typename LeavesIterator>
void cleanup_dangling_impl( NtkSrc const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end, node_map<signal<NtkDest>, NtkSrc>& old_to_new )
{
  /* constants */
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
  {
    old_to_new[ntk.get_constant( true )] = dest.get_constant( true );
  }

  /* create inputs in the same order */
  auto it = begin;
  ntk.foreach_pi( [&]( auto node ) {
    old_to_new[node] = *it++;
  } );
  if constexpr ( has_foreach_ro_v<NtkSrc> )
  {
    ntk.foreach_ro( [&]( auto node ) {
      old_to_new[node] = *it++;
    } );
  }
  assert( it == end );
  (void)end;

  /* foreach node in topological order */
  topo_view topo{ ntk };
  topo.foreach_node( [&]( auto node ) {
    if ( ntk.is_constant( node ) || ntk.is_ci( node ) )
      return;

    /* collect children */
    std::vector<signal<NtkDest>> children;
    ntk.foreach_fanin( node, [&]( auto child, auto ) {
      const auto f = old_to_new[child];
      if ( ntk.is_complemented( child ) )
      {
        children.push_back( dest.create_not( f ) );
      }
      else
      {
        children.push_back( f );
      }
    } );

    /* clone node */
    if constexpr ( std::is_same_v<NtkSrc, NtkDest> )
    {
      old_to_new[node] = dest.clone_node( ntk, node, children );
    }
    else
    {
      do
      {
        if constexpr ( has_is_and_v<NtkSrc> )
        {
          static_assert( has_create_and_v<NtkDest>, "NtkDest cannot create AND gates" );
          if ( ntk.is_and( node ) )
          {
            old_to_new[node] = dest.create_and( children[0], children[1] );
            break;
          }
        }
        if constexpr ( has_is_or_v<NtkSrc> )
        {
          static_assert( has_create_or_v<NtkDest>, "NtkDest cannot create OR gates" );
          if ( ntk.is_or( node ) )
          {
            old_to_new[node] = dest.create_or( children[0], children[1] );
            break;
          }
        }
        if constexpr ( has_is_xor_v<NtkSrc> )
        {
          static_assert( has_create_xor_v<NtkDest>, "NtkDest cannot create XOR gates" );
          if ( ntk.is_xor( node ) )
          {
            old_to_new[node] = dest.create_xor( children[0], children[1] );
            break;
          }
        }
        if constexpr ( has_is_maj_v<NtkSrc> )
        {
          static_assert( has_create_maj_v<NtkDest>, "NtkDest cannot create MAJ gates" );
          if ( ntk.is_maj( node ) )
          {
            old_to_new[node] = dest.create_maj( children[0], children[1], children[2] );
            break;
          }
        }
        if constexpr ( has_is_ite_v<NtkSrc> )
        {
          static_assert( has_create_ite_v<NtkDest>, "NtkDest cannot create ITE gates" );
          if ( ntk.is_ite( node ) )
          {
            old_to_new[node] = dest.create_ite( children[0], children[1], children[2] );
            break;
          }
        }
        if constexpr ( has_is_xor3_v<NtkSrc> )
        {
          static_assert( has_create_xor3_v<NtkDest>, "NtkDest cannot create XOR3 gates" );
          if ( ntk.is_xor3( node ) )
          {
            old_to_new[node] = dest.create_xor3( children[0], children[1], children[2] );
            break;
          }
        }
        if constexpr ( has_is_nary_and_v<NtkSrc> )
        {
          static_assert( has_create_nary_and_v<NtkDest>, "NtkDest cannot create n-ary AND gates" );
          if ( ntk.is_nary_and( node ) )
          {
            old_to_new[node] = dest.create_nary_and( children );
            break;
          }
        }
        if constexpr ( has_is_nary_or_v<NtkSrc> )
        {
          static_assert( has_create_nary_or_v<NtkDest>, "NtkDest cannot create n-ary OR gates" );
          if ( ntk.is_nary_or( node ) )
          {
            old_to_new[node] = dest.create_nary_or( children );
            break;
          }
        }
        if constexpr ( has_is_nary_xor_v<NtkSrc> )
        {
          static_assert( has_create_nary_xor_v<NtkDest>, "NtkDest cannot create n-ary XOR gates" );
          if ( ntk.is_nary_xor( node ) )
          {
            old_to_new[node] = dest.create_nary_xor( children );
            break;
          }
        }
        if constexpr ( has_is_not_v<NtkSrc> )
        {
          static_assert( has_create_not_v<NtkDest>, "NtkDest cannot create NOT gates" );
          if ( ntk.is_not( node ) )
          {
            old_to_new[node] = dest.create_not( children[0] );
            break;
          }
        }
        if constexpr ( has_is_buf_v<NtkSrc> )
        {
          static_assert( has_create_buf_v<NtkDest>, "NtkDest cannot create buffers" );
          if ( ntk.is_buf( node ) )
          {
            old_to_new[node] = dest.create_buf( children[0] );
            break;
          }
        }
        if constexpr ( has_is_function_v<NtkSrc> )
        {
          static_assert( has_create_node_v<NtkDest>, "NtkDest cannot create arbitrary function gates" );
          old_to_new[node] = dest.create_node( children, ntk.node_function( node ) );
          break;
        }
        std::cerr << "[e] something went wrong, could not copy node " << ntk.node_to_index( node ) << "\n";
      } while ( false );
    }

    /* copy name */
    if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
    {
      auto const s = ntk.make_signal( node );
      if ( ntk.has_name( s ) )
      {
        dest.set_name( old_to_new[node], ntk.get_name( s ) );
      }
      if ( ntk.has_name( !s ) )
      {
        dest.set_name( !old_to_new[node], ntk.get_name( !s ) );
      }
    }
  } );
}

template<typename NtkSrc, typename NtkDest, typename LeavesIterator>
void cleanup_dangling_with_crossings_impl( NtkSrc const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end, node_map<signal<NtkDest>, NtkSrc>& old_to_new )
{
  /* constants */
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
  {
    old_to_new[ntk.get_constant( true )] = dest.get_constant( true );
  }

  /* create inputs in the same order */
  auto it = begin;
  ntk.foreach_pi( [&]( auto node ) {
    old_to_new[node] = *it++;
  } );
  if constexpr ( has_foreach_ro_v<NtkSrc> )
  {
    ntk.foreach_ro( [&]( auto node ) {
      old_to_new[node] = *it++;
    } );
  }
  assert( it == end );
  (void)end;

  /* foreach node in topological order */
  topo_view topo{ ntk };
  topo.foreach_node( [&]( auto node ) {
    if ( ntk.is_constant( node ) || ntk.is_ci( node ) )
      return;

    /* collect children */
    std::vector<signal<NtkDest>> children;
    ntk.foreach_fanin( node, [&]( auto const& f ) {
      if ( ntk.is_crossing( ntk.get_node( f ) ) )
        children.push_back( ntk.is_second( f ) ? dest.make_second( old_to_new[f] ) : old_to_new[f] );
      else
        children.push_back( old_to_new[f] );
    } );

    /* clone node */
    if ( ntk.is_crossing( node ) )
    {
      assert( children.size() == 2 );
      old_to_new[node] = dest.create_crossing( children[0], children[1] ).first;
    }
    else
    {
      old_to_new[node] = dest.clone_node( ntk, node, children );
    }

    /* copy name */
    if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
    {
      auto const s = ntk.make_signal( node );
      if ( ntk.has_name( s ) )
      {
        dest.set_name( old_to_new[node], ntk.get_name( s ) );
      }
      if ( ntk.has_name( !s ) )
      {
        dest.set_name( !old_to_new[node], ntk.get_name( !s ) );
      }
    }
  } );
}

template<typename Ntk, typename LeavesIterator>
void cleanup_luts_impl( Ntk const& ntk, Ntk& dest, LeavesIterator begin, LeavesIterator end, node_map<signal<Ntk>, Ntk>& old_to_new )
{
  /* constants */
  old_to_new[ntk.get_constant( false )] = dest.get_constant( false );
  if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
  {
    old_to_new[ntk.get_constant( true )] = dest.get_constant( true );
  }

  /* create inputs in the same order */
  auto it = begin;
  ntk.foreach_pi( [&]( auto node ) {
    old_to_new[node] = *it++;
  } );
  if constexpr ( has_foreach_ro_v<Ntk> )
  {
    ntk.foreach_ro( [&]( auto node ) {
      old_to_new[node] = *it++;
    } );
  }
  assert( it == end );
  (void)end;

  /* iterate through nodes */
  topo_view topo{ ntk };
  topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
      return; /* continue */

    auto func = ntk.node_function( n );

    /* constant propagation */
    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      if ( dest.is_constant( old_to_new[f] ) )
      {
        if ( dest.constant_value( old_to_new[f] ) != ntk.is_complemented( f ) )
        {
          kitty::cofactor1_inplace( func, i );
        }
        else
        {
          kitty::cofactor0_inplace( func, i );
        }
      }
    } );

    const auto support = kitty::min_base_inplace( func );
    auto new_func = kitty::shrink_to( func, static_cast<unsigned int>( support.size() ) );

    std::vector<signal<Ntk>> children;
    if ( auto var = support.begin(); var != support.end() )
    {
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
        if ( *var == i )
        {
          auto const& new_f = old_to_new[f];
          children.push_back( ntk.is_complemented( f ) ? dest.create_not( new_f ) : new_f );
          if ( ++var == support.end() )
          {
            return false;
          }
        }
        return true;
      } );
    }

    if ( new_func.num_vars() == 0u )
    {
      old_to_new[n] = dest.get_constant( !kitty::is_const0( new_func ) );
    }
    else if ( new_func.num_vars() == 1u )
    {
      old_to_new[n] = *( new_func.begin() ) == 0b10 ? children.front() : dest.create_not( children.front() );
    }
    else
    {
      old_to_new[n] = dest.create_node( children, new_func );
    }

    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> && has_set_name_v<Ntk> )
    {
      auto const s = ntk.make_signal( n );
      if ( ntk.has_name( s ) )
      {
        dest.set_name( old_to_new[n], ntk.get_name( s ) );
      }
      if ( ntk.has_name( !s ) )
      {
        dest.set_name( !old_to_new[n], ntk.get_name( !s ) );
      }
    }
  } );
}

template<typename NtkSrc, typename NtkDest>
void clone_inputs( NtkSrc const& ntk, NtkDest& dest, std::vector<signal<NtkDest>>& cis, bool remove_dangling_PIs = false )
{
  /* network name */
  if constexpr ( has_get_network_name_v<NtkSrc> && has_set_network_name_v<NtkDest> )
  {
    dest.set_network_name( ntk.get_network_name() );
  }

  /* PIs & PI names */
  ntk.foreach_pi( [&]( auto n ) {
    if ( remove_dangling_PIs && ntk.fanout_size( n ) == 0 )
    {
      cis.push_back( dest.get_constant( false ) );
    }
    else
    {
      cis.push_back( dest.create_pi() );
      if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
      {
        auto const s = ntk.make_signal( n );
        if ( ntk.has_name( s ) )
        {
          dest.set_name( cis.back(), ntk.get_name( s ) );
        }
        if ( ntk.has_name( !s ) )
        {
          dest.set_name( !cis.back(), ntk.get_name( !s ) );
        }
      }
    }
  } );

  /* ROs & RO names & register information */
  if constexpr ( has_foreach_ro_v<NtkSrc> && has_create_ro_v<NtkDest> )
  {
    ntk.foreach_ro( [&]( auto const& n, auto i ) {
      cis.push_back( dest.create_ro() );
      if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
      {
        auto const s = ntk.make_signal( n );
        if ( ntk.has_name( s ) )
        {
          dest.set_name( cis.back(), ntk.get_name( s ) );
        }
        if ( ntk.has_name( !s ) )
        {
          dest.set_name( !cis.back(), ntk.get_name( !s ) );
        }
      }
      dest.set_register( i, ntk.register_at( i ) );
    } );
  }
}

template<typename NtkSrc, typename NtkDest>
void clone_outputs( NtkSrc const& ntk, NtkDest& dest, node_map<signal<NtkDest>, NtkSrc> const& old_to_new, bool remove_redundant_POs = false )
{
  /* POs */
  ntk.foreach_po( [&]( auto const& po ) {
    auto const f = old_to_new[po];
    auto const n = dest.get_node( f );
    if ( remove_redundant_POs && ( dest.is_pi( n ) || dest.is_constant( n ) ) )
    {
      return;
    }
    dest.create_po( ntk.is_complemented( po ) ? dest.create_not( f ) : f );
  } );

  /* RIs */
  if constexpr ( has_foreach_ri_v<NtkSrc> && has_create_ri_v<NtkDest> )
  {
    ntk.foreach_ri( [&]( auto const& f ) {
      dest.create_ri( ntk.is_complemented( f ) ? dest.create_not( old_to_new[f] ) : old_to_new[f] );
    } );
  }

  /* CO names */
  if constexpr ( has_has_output_name_v<NtkSrc> && has_get_output_name_v<NtkSrc> && has_set_output_name_v<NtkDest> )
  {
    ntk.foreach_co( [&]( auto co, auto index ) {
      (void)co;
      if ( ntk.has_output_name( index ) )
      {
        dest.set_output_name( index, ntk.get_output_name( index ) );
      }
    } );
  }
}

} // namespace detail

template<typename NtkSrc, typename NtkDest, typename LeavesIterator>
std::vector<signal<NtkDest>> cleanup_dangling( NtkSrc const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end )
{
  static_assert( is_network_type_v<NtkSrc>, "NtkSrc is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_get_node_v<NtkSrc>, "NtkSrc does not implement the get_node method" );
  static_assert( has_get_constant_v<NtkSrc>, "NtkSrc does not implement the get_constant method" );
  static_assert( has_foreach_pi_v<NtkSrc>, "NtkSrc does not implement the foreach_pi method" );
  static_assert( has_is_pi_v<NtkSrc>, "NtkSrc does not implement the is_pi method" );
  static_assert( has_is_constant_v<NtkSrc>, "NtkSrc does not implement the is_constant method" );
  static_assert( has_is_complemented_v<NtkSrc>, "NtkSrc does not implement the is_complemented method" );
  static_assert( has_foreach_po_v<NtkSrc>, "NtkSrc does not implement the foreach_po method" );

  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_clone_node_v<NtkDest>, "NtkDest does not implement the clone_node method" );

  node_map<signal<NtkDest>, NtkSrc> old_to_new( ntk );
  detail::cleanup_dangling_impl( ntk, dest, begin, end, old_to_new );
  std::vector<signal<NtkDest>> fs;

  /* create outputs in the same order */
  ntk.foreach_po( [&]( auto po ) {
    const auto f = old_to_new[po];
    fs.push_back( ntk.is_complemented( po ) ? dest.create_not( f ) : f );
  } );
  if constexpr ( has_foreach_ri_v<NtkSrc> )
  {
    ntk.foreach_ri( [&]( auto ri ) {
      const auto f = old_to_new[ri];
      fs.push_back( ntk.is_complemented( ri ) ? dest.create_not( f ) : f );
    } );
  }

  return fs;
}

/*! \brief Cleans up dangling nodes.
 *
 * This method reconstructs a network and omits all dangling nodes. If the flag
 * `remove_dangling_PIs` is true, dangling PIs are also omitted. If the flag
 * `remove_redundant_POs` is true, redundant POs, i.e. POs connected to a PI or
 * constant, are also omitted. The network types of the source and destination
 * network are the same.
 *
   \verbatim embed:rst

   .. note::

      This method returns the cleaned up network as a return value.  It does
      *not* modify the input network.
   \endverbatim
 *
 * **Required network functions:**
 * - `get_node`
 * - `node_to_index`
 * - `get_constant`
 * - `create_pi`
 * - `create_po`
 * - `create_not`
 * - `is_complemented`
 * - `foreach_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `clone_node`
 * - `is_pi`
 * - `is_constant`
 */
template<class NtkSrc, class NtkDest = NtkSrc>
[[nodiscard]] NtkDest cleanup_dangling( NtkSrc const& ntk, bool remove_dangling_PIs = false, bool remove_redundant_POs = false )
{
  static_assert( is_network_type_v<NtkSrc>, "NtkSrc is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );
  static_assert( has_get_node_v<NtkSrc>, "NtkSrc does not implement the get_node method" );
  static_assert( has_node_to_index_v<NtkSrc>, "NtkSrc does not implement the node_to_index method" );
  static_assert( has_get_constant_v<NtkSrc>, "NtkSrc does not implement the get_constant method" );
  static_assert( has_foreach_node_v<NtkSrc>, "NtkSrc does not implement the foreach_node method" );
  static_assert( has_foreach_pi_v<NtkSrc>, "NtkSrc does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<NtkSrc>, "NtkSrc does not implement the foreach_po method" );
  static_assert( has_is_pi_v<NtkSrc>, "NtkSrc does not implement the is_pi method" );
  static_assert( has_is_constant_v<NtkSrc>, "NtkSrc does not implement the is_constant method" );
  static_assert( has_clone_node_v<NtkDest>, "NtkDest does not implement the clone_node method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_is_complemented_v<NtkSrc>, "NtkDest does not implement the is_complemented method" );

  NtkDest dest;

  std::vector<signal<NtkDest>> cis;
  detail::clone_inputs( ntk, dest, cis, remove_dangling_PIs );

  node_map<signal<NtkDest>, NtkSrc> old_to_new( ntk );
  if constexpr ( is_crossed_network_type_v<NtkSrc> )
  {
    detail::cleanup_dangling_with_crossings_impl( ntk, dest, cis.begin(), cis.end(), old_to_new );
  }
  else
  {
    detail::cleanup_dangling_impl( ntk, dest, cis.begin(), cis.end(), old_to_new );
  }

  detail::clone_outputs( ntk, dest, old_to_new, remove_redundant_POs );

  return dest;
}

/*! \brief Cleans up LUT nodes.
 *
 * This method reconstructs a LUT network and optimizes LUTs when they do not
 * depend on all their fanin, or when some of the fanin are constant inputs.
 *
 * Constant gate inputs will be propagated.
 *
   \verbatim embed:rst

   .. note::

      This method returns the cleaned up network as a return value.  It does
      *not* modify the input network.
   \endverbatim
 *
 * **Required network functions:**
 * - `get_node`
 * - `get_constant`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `create_pi`
 * - `create_po`
 * - `create_node`
 * - `create_not`
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `node_function`
 */
template<class Ntk>
[[nodiscard]] Ntk cleanup_luts( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );
  static_assert( has_create_node_v<Ntk>, "Ntk does not implement the create_node method" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_constant_value_v<Ntk>, "Ntk does not implement the constant_value method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

  Ntk dest;

  std::vector<signal<Ntk>> cis;
  detail::clone_inputs( ntk, dest, cis );

  node_map<signal<Ntk>, Ntk> old_to_new( ntk );
  detail::cleanup_luts_impl( ntk, dest, cis.begin(), cis.end(), old_to_new );

  detail::clone_outputs( ntk, dest, old_to_new );

  return dest;
}

} // namespace mockturtle