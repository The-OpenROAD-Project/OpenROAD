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
  \file decompose_multioutput.hpp
  \brief Decomposes the multi-output gates into single output

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <iostream>
#include <type_traits>
#include <vector>

#include <kitty/operations.hpp>

#include "../../traits.hpp"
#include "../../utils/node_map.hpp"
#include "../../views/topo_view.hpp"
#include "../cleanup.hpp"

namespace mockturtle
{

struct decompose_multioutput_params
{
  bool set_multioutput_as_dont_touch{ false };
};

namespace detail
{

template<typename NtkSrc, typename NtkDest, typename LeavesIterator>
void decompose_multioutput_impl( NtkSrc const& ntk, NtkDest& dest, LeavesIterator begin, LeavesIterator end, std::unordered_map<uint64_t, signal<NtkDest>>& old_to_new, decompose_multioutput_params const& ps )
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
    old_to_new[ntk.make_signal( node )] = *it++;
  } );
  if constexpr ( has_foreach_ro_v<NtkSrc> )
  {
    ntk.foreach_ro( [&]( auto node ) {
      old_to_new[ntk.make_signal( node )] = *it++;
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
      const auto child_no_complement = child ^ ntk.is_complemented( child );
      const auto f = old_to_new[child_no_complement];

      assert( dest.get_node( f ) != dest.get_node( dest.get_constant( false ) ) );
      assert( dest.get_node( f ) != dest.get_node( dest.get_constant( true ) ) );

      children.push_back( f ^ ntk.is_complemented( child ) );
    } );

    /* clone node */
    if ( ntk.is_multioutput( node ) )
    {
      for ( auto i = 0; i < ntk.num_outputs( node ); ++i )
      {
        auto f = ntk.make_signal( node, i );
        do
        {
          if constexpr ( has_is_and_v<NtkSrc> )
          {
            static_assert( has_create_and_v<NtkDest>, "NtkDest cannot create AND gates" );
            if ( ntk.is_and( f ) )
            {
              old_to_new[f] = dest.create_and( children[0], children[1] );
              break;
            }
          }
          if constexpr ( has_is_or_v<NtkSrc> )
          {
            static_assert( has_create_or_v<NtkDest>, "NtkDest cannot create OR gates" );
            if ( ntk.is_or( f ) )
            {
              old_to_new[f] = dest.create_or( children[0], children[1] );
              break;
            }
          }
          if constexpr ( has_is_xor_v<NtkSrc> )
          {
            static_assert( has_create_xor_v<NtkDest>, "NtkDest cannot create XOR gates" );
            if ( ntk.is_xor( f ) )
            {
              old_to_new[f] = dest.create_xor( children[0], children[1] );
              break;
            }
          }
          if constexpr ( has_is_maj_v<NtkSrc> )
          {
            static_assert( has_create_maj_v<NtkDest>, "NtkDest cannot create MAJ gates" );
            if ( ntk.is_maj( f ) )
            {
              old_to_new[f] = dest.create_maj( children[0], children[1], children[2] );
              break;
            }
          }
          if constexpr ( has_is_ite_v<NtkSrc> )
          {
            static_assert( has_create_ite_v<NtkDest>, "NtkDest cannot create ITE gates" );
            if ( ntk.is_ite( f ) )
            {
              old_to_new[f] = dest.create_ite( children[0], children[1], children[2] );
              break;
            }
          }
          if constexpr ( has_is_xor3_v<NtkSrc> )
          {
            static_assert( has_create_xor3_v<NtkDest>, "NtkDest cannot create XOR3 gates" );
            if ( ntk.is_xor3( f ) )
            {
              old_to_new[f] = dest.create_xor3( children[0], children[1], children[2] );
              break;
            }
          }
          if constexpr ( has_is_function_v<NtkSrc> && has_create_node_v<NtkDest> )
          {
            old_to_new[f] = dest.create_node( children, ntk.node_function_pin( node, i ) );
            break;
          }
          std::cerr << "[e] something went wrong, could not copy node " << ntk.node_to_index( node ) << "\n";
        } while ( false );

        /* set dont touch */
        if constexpr ( has_select_dont_touch_v<NtkDest> )
        {
          if ( ps.set_multioutput_as_dont_touch )
            dest.select_dont_touch( dest.get_node( old_to_new[f] ) );
        }

        /* copy name */
        if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
        {
          if ( ntk.has_name( f ) )
          {
            dest.set_name( old_to_new[f], ntk.get_name( f ) );
          }
          if ( ntk.has_name( !f ) )
          {
            dest.set_name( !old_to_new[f], ntk.get_name( !f ) );
          }
        }
      }
    }
    else
    {
      auto f = ntk.make_signal( node );
      if constexpr ( std::is_same_v<NtkSrc, NtkDest> )
      {
        old_to_new[f] = dest.clone_node( ntk, node, children );
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
              old_to_new[f] = dest.create_and( children[0], children[1] );
              break;
            }
          }
          if constexpr ( has_is_or_v<NtkSrc> )
          {
            static_assert( has_create_or_v<NtkDest>, "NtkDest cannot create OR gates" );
            if ( ntk.is_or( node ) )
            {
              old_to_new[f] = dest.create_or( children[0], children[1] );
              break;
            }
          }
          if constexpr ( has_is_xor_v<NtkSrc> )
          {
            static_assert( has_create_xor_v<NtkDest>, "NtkDest cannot create XOR gates" );
            if ( ntk.is_xor( node ) )
            {
              old_to_new[f] = dest.create_xor( children[0], children[1] );
              break;
            }
          }
          if constexpr ( has_is_maj_v<NtkSrc> )
          {
            static_assert( has_create_maj_v<NtkDest>, "NtkDest cannot create MAJ gates" );
            if ( ntk.is_maj( node ) )
            {
              old_to_new[f] = dest.create_maj( children[0], children[1], children[2] );
              break;
            }
          }
          if constexpr ( has_is_ite_v<NtkSrc> )
          {
            static_assert( has_create_ite_v<NtkDest>, "NtkDest cannot create ITE gates" );
            if ( ntk.is_ite( node ) )
            {
              old_to_new[f] = dest.create_ite( children[0], children[1], children[2] );
              break;
            }
          }
          if constexpr ( has_is_xor3_v<NtkSrc> )
          {
            static_assert( has_create_xor3_v<NtkDest>, "NtkDest cannot create XOR3 gates" );
            if ( ntk.is_xor3( node ) )
            {
              old_to_new[f] = dest.create_xor3( children[0], children[1], children[2] );
              break;
            }
          }
          if constexpr ( has_is_nary_and_v<NtkSrc> )
          {
            static_assert( has_create_nary_and_v<NtkDest>, "NtkDest cannot create n-ary AND gates" );
            if ( ntk.is_nary_and( node ) )
            {
              old_to_new[f] = dest.create_nary_and( children );
              break;
            }
          }
          if constexpr ( has_is_nary_or_v<NtkSrc> )
          {
            static_assert( has_create_nary_or_v<NtkDest>, "NtkDest cannot create n-ary OR gates" );
            if ( ntk.is_nary_or( node ) )
            {
              old_to_new[f] = dest.create_nary_or( children );
              break;
            }
          }
          if constexpr ( has_is_nary_xor_v<NtkSrc> )
          {
            static_assert( has_create_nary_xor_v<NtkDest>, "NtkDest cannot create n-ary XOR gates" );
            if ( ntk.is_nary_xor( node ) )
            {
              old_to_new[f] = dest.create_nary_xor( children );
              break;
            }
          }
          if constexpr ( has_is_function_v<NtkSrc> && has_create_node_v<NtkDest> )
          {
            old_to_new[f] = dest.create_node( children, ntk.node_function( node ) );
            break;
          }
          std::cerr << "[e] something went wrong, could not copy node " << ntk.node_to_index( node ) << "\n";
        } while ( false );
      }
      /* copy name */
      if constexpr ( has_has_name_v<NtkSrc> && has_get_name_v<NtkSrc> && has_set_name_v<NtkDest> )
      {
        if ( ntk.has_name( f ) )
        {
          dest.set_name( old_to_new[f], ntk.get_name( f ) );
        }
        if ( ntk.has_name( !f ) )
        {
          dest.set_name( !old_to_new[f], ntk.get_name( !f ) );
        }
      }
    }
  } );

  /* POs */
  ntk.foreach_po( [&]( auto const& po ) {
    const auto po_no_complement = po ^ ntk.is_complemented( po );
    auto const f = old_to_new[po_no_complement];
    dest.create_po( f ^ ntk.is_complemented( po ) );
  } );

  /* RIs */
  if constexpr ( has_foreach_ri_v<NtkSrc> && has_create_ri_v<NtkDest> )
  {
    ntk.foreach_ri( [&]( auto const& f ) {
      dest.create_ri( old_to_new[f ^ ntk.is_complemented( f )] ^ ntk.is_complemented( f ) );
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

/*! \brief Decomposes the multi-output gates into single output.
 *
 * This method reconstructs a network decomposing the multi-output gates into
 * single output gates. Moreover, it omits all dangling nodes.
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
 * - `has_multioutput`
 */
template<class NtkSrc, class NtkDest = NtkSrc>
[[nodiscard]] NtkDest decompose_multioutput( NtkSrc const& ntk, decompose_multioutput_params const& ps = {} )
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
  static_assert( has_is_complemented_v<NtkSrc>, "NtkSrc does not implement the is_complemented method" );
  static_assert( has_is_multioutput_v<NtkSrc>, "NtkSource does not implement the is_complemented method" );
  static_assert( has_node_function_pin_v<NtkSrc>, "NtkSource does not implement the node_function_pin" );
  static_assert( has_num_outputs_v<NtkSrc>, "NtkSource does not implement the has_num_outputs" );

  NtkDest dest;

  std::vector<signal<NtkDest>> cis;
  detail::clone_inputs( ntk, dest, cis, false );

  std::unordered_map<uint64_t, signal<NtkDest>> old_to_new;
  detail::decompose_multioutput_impl( ntk, dest, cis.begin(), cis.end(), old_to_new, ps );

  return dest;
}

} // namespace mockturtle
