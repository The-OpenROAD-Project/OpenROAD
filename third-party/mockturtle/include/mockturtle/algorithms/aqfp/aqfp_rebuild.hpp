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
  \file aqfp_rebuild.hpp
  \brief Rebuilds buffer-splitter tree in AQFP networks

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <algorithm>
#include <list>
#include <random>
#include <vector>

#include "../../networks/buffered.hpp"
#include "../../networks/generic.hpp"
#include "../../utils/node_map.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/depth_view.hpp"
#include "aqfp_assumptions.hpp"
#include "aqfp_cleanup.hpp"
#include "buffer_insertion.hpp"
#include "buffer_verification.hpp"

namespace mockturtle
{

struct aqfp_reconstruct_params
{
  /*! \brief AQFP buffer insertion parameters. */
  buffer_insertion_params buffer_insertion_ps{};

  /*! \brief Randomize topological order. */
  bool det_randomization{ false };

  /*! \brief Seed for random selection of splitters to relocate. */
  std::default_random_engine::result_type seed{ 1 };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

struct aqfp_reconstruct_stats
{
  /*! \brief Number of buffers and splitters after reconstruction. */
  uint32_t num_buffers{ 0 };

  /*! \brief Total runtime */
  stopwatch<>::duration total_time{ 0 };

  /*! \brief Report stats */
  void report()
  {
    std::cout << fmt::format( "[i] Buffers = {}\t Total time = {}\n", num_buffers, to_seconds( total_time ) );
  }
};

namespace detail
{

class aqfp_reconstruct_impl
{
public:
  using node = typename aqfp_network::node;
  using signal = typename aqfp_network::signal;

public:
  explicit aqfp_reconstruct_impl( buffered_aqfp_network const& ntk, aqfp_reconstruct_params const& ps, aqfp_reconstruct_stats& st )
      : _ntk( ntk ), _ps( ps ), _st( st ), _topo_order()
  {
  }

  buffered_aqfp_network run()
  {
    stopwatch( _st.total_time );

    /* save the level of each node */
    depth_view ntk_level{ _ntk };

    /* create a network removing the splitter trees */
    aqfp_network clean_ntk;
    node_map<signal, buffered_aqfp_network> old2new( _ntk );
    remove_splitter_trees( clean_ntk, old2new );

    /* compute the node level on the new network */
    node_map<uint32_t, aqfp_network> levels( clean_ntk );
    _ntk.foreach_gate( [&]( auto const& n ) {
      levels[old2new[n]] = ntk_level.level( n );
    } );

    uint32_t max_po_level = 0;
    clean_ntk.foreach_po( [&]( auto const& f ){
      uint32_t spl = std::ceil( std::log( clean_ntk.fanout_size( clean_ntk.get_node( f ) ) ) / std::log( _ps.buffer_insertion_ps.assume.splitter_capacity ) );
      max_po_level = std::max( max_po_level, levels[f] + spl );
    });
    std::vector<uint32_t> po_levels;
    for ( auto i = 0u; i < _ntk.num_pos(); ++i )
    {
      po_levels.emplace_back( max_po_level + 1 );
    }

    /* recompute splitter trees and return the new buffered network */
    buffered_aqfp_network res;
    buffer_insertion buf_inst( clean_ntk, levels, po_levels, _ps.buffer_insertion_ps );
    _st.num_buffers = buf_inst.run( res );
    return res;
  }

private:
  void remove_splitter_trees( aqfp_network& res, node_map<signal, buffered_aqfp_network>& old2new )
  {
    topo_sorting();

    old2new[_ntk.get_constant( false )] = res.get_constant( false );
    _ntk.foreach_pi( [&]( auto const& n ) {
      old2new[n] = res.create_pi();
    } );

    for ( auto const& n : _topo_order )
    {
      if ( _ntk.is_pi( n ) || _ntk.is_constant( n ) )
        continue;

      std::vector<signal> children;
      _ntk.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( old2new[f] ^ _ntk.is_complemented( f ) );
      } );

      if ( _ntk.is_buf( n ) )
      {
        old2new[n] = children[0];
      }
      else if ( children.size() == 3 )
      {
        old2new[n] = res.create_maj( children[0], children[1], children[2] );
      }
      else
      {
        old2new[n] = res.create_maj( children );
      }
    }

    _ntk.foreach_po( [&]( auto const& f ) {
      res.create_po( old2new[f] ^ _ntk.is_complemented( f ) );
    } );
  }

  void topo_sorting()
  {
    _ntk.incr_trav_id();
    _ntk.incr_trav_id();
    _topo_order.reserve( _ntk.size() );

    seed = _ps.seed;

    /* constants and PIs */
    const auto c0 = _ntk.get_node( _ntk.get_constant( false ) );
    _topo_order.push_back( c0 );
    _ntk.set_visited( c0, _ntk.trav_id() );

    if ( const auto c1 = _ntk.get_node( _ntk.get_constant( true ) ); _ntk.visited( c1 ) != _ntk.trav_id() )
    {
      _topo_order.push_back( c1 );
      _ntk.set_visited( c1, _ntk.trav_id() );
    }

    _ntk.foreach_ci( [&]( auto const& n ) {
      if ( _ntk.visited( n ) != _ntk.trav_id() )
      {
        _topo_order.push_back( n );
        _ntk.set_visited( n, _ntk.trav_id() );
      }
    } );

    _ntk.foreach_co( [&]( auto const& f ) {
      /* node was already visited */
      if ( _ntk.visited( _ntk.get_node( f ) ) == _ntk.trav_id() )
        return;

      topo_sorting_rec( _ntk.get_node( f ) );
    } );
  }

  void topo_sorting_rec( node const& n )
  {
    /* is permanently marked? */
    if ( _ntk.visited( n ) == _ntk.trav_id() )
      return;

    /* ensure that the node is not temporarily marked */
    assert( _ntk.visited( n ) != _ntk.trav_id() - 1 );

    /* mark node temporarily */
    _ntk.set_visited( n, _ntk.trav_id() - 1 );

    /* mark children */
    if ( !_ps.det_randomization )
    {
      _ntk.foreach_fanin( n, [this]( signal const& f ) {
        topo_sorting_rec( _ntk.get_node( f ) );
      } );
    }
    else
    {
      std::vector<node> fanins;
      _ntk.foreach_fanin( n, [this, &fanins]( signal const& f ) {
        fanins.push_back( _ntk.get_node( f ) );
      } );
      std::shuffle( fanins.begin(), fanins.end(), std::default_random_engine( seed++ ) );

      for ( node const& g : fanins )
        topo_sorting_rec( g );
    }

    /* mark node n permanently */
    _ntk.set_visited( n, _ntk.trav_id() );

    /* visit node */
    _topo_order.push_back( n );
  }

private:
  buffered_aqfp_network const& _ntk;
  aqfp_reconstruct_params const& _ps;
  aqfp_reconstruct_stats& _st;

  std::vector<node> _topo_order;
  std::default_random_engine::result_type seed{ 1 };
};

} /* namespace detail */

/*! \brief Rebuilds buffer/splitter trees in an AQFP network.
 *
 * This function rebuilds buffer/splitter trees in an AQFP network.
 *
 * \param ntk Buffered AQFP network
 */
buffered_aqfp_network aqfp_reconstruct( buffered_aqfp_network const& ntk, aqfp_reconstruct_params const& ps = {}, aqfp_reconstruct_stats* pst = nullptr )
{
  aqfp_reconstruct_stats st;

  detail::aqfp_reconstruct_impl p( ntk, ps, st );
  auto res = p.run();

  if ( pst )
    *pst = st;

  if ( ps.verbose )
    st.report();

  return res;
}

} // namespace mockturtle