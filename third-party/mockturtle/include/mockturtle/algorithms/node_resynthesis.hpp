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
  \file node_resynthesis.hpp
  \brief Node resynthesis

  \author Heinz Riener
  \author Mathias Soeken
  \author Max Austin
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <iostream>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/topo_view.hpp"

#include <fmt/format.h>

namespace mockturtle
{

/*! \brief Parameters for node_resynthesis.
 *
 * The data structure `node_resynthesis_params` holds configurable parameters
 * with default arguments for `node_resynthesis`.
 */
struct node_resynthesis_params
{
  /*! \brief Be verbose. */
  bool verbose{ false };
};

/*! \brief Statistics for node_resynthesis.
 *
 * The data structure `node_resynthesis_stats` provides data collected by
 * running `node_resynthesis`.
 */
struct node_resynthesis_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  void report() const
  {
    std::cout << fmt::format( "[i] total time = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

template<class NtkDest, class NtkSource, class ResynthesisFn>
class node_resynthesis_impl
{
public:
  node_resynthesis_impl( NtkDest& ntk_dest, NtkSource const& ntk, ResynthesisFn&& resynthesis_fn, node_resynthesis_params const& ps, node_resynthesis_stats& st )
      : ntk_dest( ntk_dest ),
        ntk( ntk ),
        resynthesis_fn( resynthesis_fn ),
        ps( ps ),
        st( st )
  {
  }

  NtkDest run()
  {
    stopwatch t( st.time_total );

    node_map<signal<NtkDest>, NtkSource> node2new( ntk );

    /* map constants */
    node2new[ntk.get_node( ntk.get_constant( false ) )] = ntk_dest.get_constant( false );
    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
    {
      node2new[ntk.get_node( ntk.get_constant( true ) )] = ntk_dest.get_constant( true );
    }

    /* map primary inputs */
    ntk.foreach_pi( [&]( auto n ) {
      node2new[n] = ntk_dest.create_pi();

      if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
      {
        if ( ntk.has_name( ntk.make_signal( n ) ) )
          ntk_dest.set_name( node2new[n], ntk.get_name( ntk.make_signal( n ) ) );
      }
    } );

    if constexpr ( has_foreach_ro_v<NtkSource> && has_create_ro_v<NtkDest> )
    {
      ntk.foreach_ro( [&]( auto n, auto i ) {
        node2new[n] = ntk_dest.create_ro();
        ntk_dest.set_register( i, ntk.register_at( i ) );
        if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
        {
          if ( ntk.has_name( ntk.make_signal( n ) ) )
            ntk_dest.set_name( node2new[n], ntk.get_name( ntk.make_signal( n ) ) );
        }
      } );
    }

    /* map nodes */
    topo_view ntk_topo{ ntk };
    ntk_topo.foreach_node( [&]( auto n ) {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        return;

      std::vector<signal<NtkDest>> children;
      ntk.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( ntk.is_complemented( f ) ? ntk_dest.create_not( node2new[f] ) : node2new[f] );
      } );

      bool performed_resyn = false;
      resynthesis_fn( ntk_dest, ntk.node_function( n ), children.begin(), children.end(), [&]( auto const& f ) {
        node2new[n] = f;

        if constexpr ( has_has_name_v<NtkSource> && has_get_name_v<NtkSource> && has_set_name_v<NtkDest> )
        {
          if ( ntk.has_name( ntk.make_signal( n ) ) )
            ntk_dest.set_name( f, ntk.get_name( ntk.make_signal( n ) ) );
        }

        performed_resyn = true;
        return false;
      } );

      if ( !performed_resyn )
      {
        fmt::print( "[e] could not perform resynthesis for node {} in node_resynthesis\n", ntk.node_to_index( n ) );
        std::abort();
      }
    } );

    /* map primary outputs */
    ntk.foreach_po( [&]( auto const& f, auto index ) {
      (void)index;

      auto const o = ntk.is_complemented( f ) ? ntk_dest.create_not( node2new[f] ) : node2new[f];
      ntk_dest.create_po( o );

      if constexpr ( has_has_output_name_v<NtkSource> && has_get_output_name_v<NtkSource> && has_set_output_name_v<NtkDest> )
      {
        if ( ntk.has_output_name( index ) )
        {
          ntk_dest.set_output_name( index, ntk.get_output_name( index ) );
        }
      }
    } );

    if constexpr ( has_foreach_ri_v<NtkSource> && has_create_ri_v<NtkDest> )
    {
      ntk.foreach_ri( [&]( auto const& f, auto index ) {
        (void)index;

        auto const o = ntk.is_complemented( f ) ? ntk_dest.create_not( node2new[f] ) : node2new[f];
        ntk_dest.create_ri( o );

        if constexpr ( has_has_output_name_v<NtkSource> && has_get_output_name_v<NtkSource> && has_set_output_name_v<NtkDest> )
        {
          if ( ntk.has_output_name( index ) )
          {
            ntk_dest.set_output_name( index + ntk.num_pos(), ntk.get_output_name( index + ntk.num_pos() ) );
          }
        }
      } );
    }

    return ntk_dest;
  }

private:
  NtkDest& ntk_dest;
  NtkSource const& ntk;
  ResynthesisFn&& resynthesis_fn;
  node_resynthesis_params const& ps;
  node_resynthesis_stats& st;
};

} /* namespace detail */

/*! \brief Node resynthesis algorithm.
 *
 * This algorithm takes as input a network (of type `NtkSource`) and creates a
 * new network (of type `NtkDest`), by translating each node of the input
 * network into a subnetwork for the output network.  To find a new subnetwork,
 * the algorithm uses a resynthesis function that takes as input the input
 * node's truth table.  This algorithm can for example be used to translate
 * k-LUT networks into AIGs or MIGs.
 *
 * The resynthesis function must be of type `NtkDest::signal(NtkDest&,
 * kitty::dynamic_truth_table const&, LeavesIterator, LeavesIterator)` where
 * `LeavesIterator` can be dereferenced to a `NtkDest::signal`.  The last two
 * parameters compose an iterator pair where the distance matches the number of
 * variables of the truth table that is passed as second parameter.
 *
 * **Required network functions for parameter ntk (type NtkSource):**
 * - `get_node`
 * - `get_constant`
 * - `foreach_pi`
 * - `foreach_node`
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `foreach_fanin`
 * - `node_function`
 * - `foreach_po`
 *
 * **Required network functions for return value (type NtkDest):**
 * - `get_constant`
 * - `create_pi`
 * - `create_not`
 * - `create_po`
 *
 * \param ntk Input network of type `NtkSource`
 * \param resynthesis_fn Resynthesis function
 * \return An equivalent network of type `NtkDest`
 */
template<class NtkDest, class NtkSource, class ResynthesisFn>
NtkDest node_resynthesis( NtkSource const& ntk, ResynthesisFn&& resynthesis_fn, node_resynthesis_params const& ps = {}, node_resynthesis_stats* pst = nullptr )
{
  static_assert( is_network_type_v<NtkSource>, "NtkSource is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_get_node_v<NtkSource>, "NtkSource does not implement the get_node method" );
  static_assert( has_get_constant_v<NtkSource>, "NtkSource does not implement the get_constant method" );
  static_assert( has_foreach_pi_v<NtkSource>, "NtkSource does not implement the foreach_pi method" );
  static_assert( has_foreach_node_v<NtkSource>, "NtkSource does not implement the foreach_node method" );
  static_assert( has_is_constant_v<NtkSource>, "NtkSource does not implement the is_constant method" );
  static_assert( has_is_pi_v<NtkSource>, "NtkSource does not implement the is_pi method" );
  static_assert( has_is_complemented_v<NtkSource>, "NtkSource does not implement the is_complemented method" );
  static_assert( has_foreach_fanin_v<NtkSource>, "NtkSource does not implement the foreach_fanin method" );
  static_assert( has_node_function_v<NtkSource>, "NtkSource does not implement the node_function method" );
  static_assert( has_foreach_po_v<NtkSource>, "NtkSource does not implement the foreach_po method" );

  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );

  node_resynthesis_stats st;
  NtkDest ntk_dest;
  detail::node_resynthesis_impl<NtkDest, NtkSource, ResynthesisFn> p( ntk_dest, ntk, resynthesis_fn, ps, st );
  const auto ret = p.run();
  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
  return ret;
}

/*! \brief Node resynthesis algorithm.
 *
 * This algorithm takes as input a network (of type `NtkSource`) and creates a
 * new network (of type `NtkDest`), by translating each node of the input
 * network into a subnetwork for the output network.  To find a new subnetwork,
 * the algorithm uses a resynthesis function that takes as input the input
 * node's truth table.  This algorithm can for example be used to translate
 * k-LUT networks into AIGs or MIGs.
 *
 * The resynthesis function must be of type `NtkDest::signal(NtkDest&,
 * kitty::dynamic_truth_table const&, LeavesIterator, LeavesIterator)` where
 * `LeavesIterator` can be dereferenced to a `NtkDest::signal`.  The last two
 * parameters compose an iterator pair where the distance matches the number of
 * variables of the truth table that is passed as second parameter.
 *
 * **Required network functions for parameter ntk (type NtkSource):**
 * - `get_node`
 * - `get_constant`
 * - `foreach_pi`
 * - `foreach_node`
 * - `is_constant`
 * - `is_pi`
 * - `is_complemented`
 * - `foreach_fanin`
 * - `node_function`
 * - `foreach_po`
 *
 * **Required network functions for return value (type NtkDest):**
 * - `get_constant`
 * - `create_pi`
 * - `create_not`
 * - `create_po`
 *
 * \param ntk_dest Output network of type `NtkDest`
 * \param ntk Input network of type `NtkSource`
 * \param resynthesis_fn Resynthesis function
 */
template<class NtkDest, class NtkSource, class ResynthesisFn>
void node_resynthesis( NtkDest& ntk_dest, NtkSource const& ntk, ResynthesisFn&& resynthesis_fn, node_resynthesis_params const& ps = {}, node_resynthesis_stats* pst = nullptr )
{
  static_assert( is_network_type_v<NtkSource>, "NtkSource is not a network type" );
  static_assert( is_network_type_v<NtkDest>, "NtkDest is not a network type" );

  static_assert( has_get_node_v<NtkSource>, "NtkSource does not implement the get_node method" );
  static_assert( has_get_constant_v<NtkSource>, "NtkSource does not implement the get_constant method" );
  static_assert( has_foreach_pi_v<NtkSource>, "NtkSource does not implement the foreach_pi method" );
  static_assert( has_foreach_node_v<NtkSource>, "NtkSource does not implement the foreach_node method" );
  static_assert( has_is_constant_v<NtkSource>, "NtkSource does not implement the is_constant method" );
  static_assert( has_is_pi_v<NtkSource>, "NtkSource does not implement the is_pi method" );
  static_assert( has_is_complemented_v<NtkSource>, "NtkSource does not implement the is_complemented method" );
  static_assert( has_foreach_fanin_v<NtkSource>, "NtkSource does not implement the foreach_fanin method" );
  static_assert( has_node_function_v<NtkSource>, "NtkSource does not implement the node_function method" );
  static_assert( has_foreach_po_v<NtkSource>, "NtkSource does not implement the foreach_po method" );

  static_assert( has_get_constant_v<NtkDest>, "NtkDest does not implement the get_constant method" );
  static_assert( has_create_pi_v<NtkDest>, "NtkDest does not implement the create_pi method" );
  static_assert( has_create_not_v<NtkDest>, "NtkDest does not implement the create_not method" );
  static_assert( has_create_po_v<NtkDest>, "NtkDest does not implement the create_po method" );

  node_resynthesis_stats st;
  detail::node_resynthesis_impl<NtkDest, NtkSource, ResynthesisFn> p( ntk_dest, ntk, resynthesis_fn, ps, st );
  p.run();
  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} // namespace mockturtle