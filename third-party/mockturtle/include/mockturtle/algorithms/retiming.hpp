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
  \file retiming.hpp
  \brief Retiming

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <cstdint>
#include <limits>

#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/fanout_view.hpp"
#include "../views/topo_view.hpp"
#include <fmt/format.h>

namespace mockturtle
{
/*! \brief Parameters for retiming.
 *
 * The data structure `retime_params` holds configurable parameters
 * with default arguments for `retime`.
 */
struct retime_params
{
  /*! \brief Do forward only retiming. */
  bool forward_only{ false };

  /*! \brief Do backward only retiming. */
  bool backward_only{ false };

  /*! \brief Retiming max iterations. */
  uint32_t iterations{ UINT32_MAX };

  /*! \brief Be verbose */
  bool verbose{ false };
};

/*! \brief Statistics for retiming.
 *
 * The data structure `retime_stats` provides data collected by running
 * `retime`.
 */
struct retime_stats
{
  /*! \brief Initial number of registers. */
  uint32_t registers_pre{ 0 };

  /*! \brief Number of registers after retime. */
  uint32_t registers_post{ 0 };

  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  void report() const
  {
    std::cout << fmt::format( "[i] Initial registers = {:7d}\t Final registers = {:7d}\n", registers_pre, registers_post );
    std::cout << fmt::format( "[i] Total runtime   = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

template<class Ntk>
class retime_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  static constexpr uint32_t sink_node = UINT32_MAX;

public:
  explicit retime_impl( Ntk& ntk, retime_params const& ps, retime_stats& st )
      : _ntk( ntk ),
        _ps( ps ),
        _st( st ),
        _flow_path( ntk )
  {}

public:
  void run()
  {
    stopwatch t( _st.time_total );

    _st.registers_pre = _ntk.num_registers();

    if ( !_ps.backward_only )
    {
      bool improvement = true;
      for ( auto i = 0; i < _ps.iterations && improvement == true; ++i )
      {
        improvement = retime_area<true>( i + 1 );
      }
    }

    if ( !_ps.forward_only )
    {
      bool improvement = true;
      for ( auto i = 0; i < _ps.iterations && improvement == true; ++i )
      {
        improvement = retime_area<false>( i + 1 );
      }
    }

    _st.registers_post = _ntk.num_registers();
  }

private:
  template<bool forward>
  bool retime_area( uint32_t iteration )
  {
    auto const num_registers_pre = _ntk.num_registers();

    init_values<forward>();

    auto min_cut = max_flow<forward>( iteration );

    if ( _ps.verbose )
    {
      float register_improvement = ( (float)num_registers_pre - min_cut.size() ) / num_registers_pre * 100;
      std::cout << fmt::format( "[i] Retiming {}\t pre = {:7d}\t post = {:7d}\t improvement = {:>5.2f}%\n",
                                forward ? "forward" : "backward", num_registers_pre, min_cut.size(), register_improvement );
    }

    if ( min_cut.size() >= num_registers_pre )
      return false;

    /* move registers */
    update_registers_position<forward>( min_cut, iteration );

    return true;
  }

  template<bool forward>
  std::vector<node> max_flow( uint32_t iteration )
  {
    uint32_t flow = 0;

    _flow_path.reset();
    _ntk.incr_trav_id();

    /* run max flow from each register (capacity 1) */
    _ntk.foreach_register( [&]( auto const& n ) {
      uint32_t local_flow;
      if constexpr ( forward )
      {
        local_flow = max_flow_forwards_compute_rec( _ntk.fanout( n )[0] );
      }
      else
      {
        node fanin = _ntk.get_node( _ntk.get_fanin0( n ) );
        local_flow = max_flow_backwards_compute_rec( fanin );
      }

      flow += local_flow;

      if ( local_flow )
        _ntk.incr_trav_id();

      return true;
    } );

    /* run reachability */
    _ntk.incr_trav_id();
    _ntk.foreach_register( [&]( auto const& n ) {
      uint32_t local_flow;
      if constexpr ( forward )
      {
        local_flow = max_flow_forwards_compute_rec( _ntk.fanout( n )[0] );
      }
      else
      {
        node fanin = _ntk.get_node( _ntk.get_fanin0( n ) );
        local_flow = max_flow_backwards_compute_rec( fanin );
      }

      assert( local_flow == 0 );
      return true;
    } );

    auto min_cut = get_min_cut();

    // assert( check_min_cut<forward>( min_cut, iteration ) );

    legalize_retiming<forward>( min_cut, iteration );

    return min_cut;
  }

  uint32_t max_flow_forwards_compute_rec( node const& n )
  {
    uint32_t found_path = 0;

    if ( _ntk.visited( n ) == _ntk.trav_id() )
      return 0;

    _ntk.set_visited( n, _ntk.trav_id() );

    /* node is not in a flow path */
    if ( _flow_path[n] == 0 )
    {
      /* cut boundary (sink) */
      if ( _ntk.value( n ) )
      {
        _flow_path[n] = sink_node;
        return 1;
      }

      _ntk.foreach_fanout( n, [&]( auto const& f ) {
        /* there is a path for flow */
        if ( max_flow_forwards_compute_rec( f ) )
        {
          _flow_path[n] = _ntk.node_to_index( f );
          found_path = 1;
          return false;
        }
        return true;
      } );

      return found_path;
    }

    /* path has flow already, find alternative path from fanin with flow */
    node fanin_flow = 0;
    _ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( _ntk.is_constant( _ntk.get_node( f ) ) )
        return true;
      if ( _flow_path[f] == _ntk.node_to_index( n ) )
      {
        fanin_flow = _ntk.get_node( f );
        return false;
      }
      return true;
    } );

    if ( fanin_flow == 0 )
      return 0;

    /* augment path */
    _ntk.foreach_fanout( fanin_flow, [&]( auto const& f ) {
      /* there is a path for flow */
      if ( max_flow_forwards_compute_rec( f ) )
      {
        _flow_path[fanin_flow] = _ntk.node_to_index( f );
        found_path = 1;
        return false;
      }
      return true;
    } );

    if ( found_path )
      return 1;

    if ( max_flow_forwards_compute_rec( fanin_flow ) )
    {
      _flow_path[fanin_flow] = 0;
      return 1;
    }

    return 0;
  }

  uint32_t max_flow_backwards_compute_rec( node const& n )
  {
    uint32_t found_path = 0;

    if ( _ntk.visited( n ) == _ntk.trav_id() )
      return 0;

    _ntk.set_visited( n, _ntk.trav_id() );

    /* node is not in a flow path */
    if ( _flow_path[n] == 0 )
    {
      /* cut boundary (sink) */
      if ( _ntk.value( n ) )
      {
        _flow_path[n] = sink_node;
        return 1;
      }

      _ntk.foreach_fanin( n, [&]( auto const& f ) {
        if ( _ntk.is_constant( _ntk.get_node( f ) ) )
          return true;
        /* there is a path for flow */
        if ( max_flow_backwards_compute_rec( _ntk.get_node( f ) ) )
        {
          _flow_path[n] = _ntk.node_to_index( _ntk.get_node( f ) );
          found_path = 1;
          return false;
        }
        return true;
      } );

      return found_path;
    }

    /* path has flow already, find alternative path from fanout with flow */
    node fanout_flow = 0;
    _ntk.foreach_fanout( n, [&]( auto const& f ) {
      if ( _flow_path[f] == _ntk.node_to_index( n ) )
      {
        fanout_flow = _ntk.get_node( f );
        return false;
      }
      return true;
    } );

    if ( fanout_flow == 0 )
      return 0;

    /* augment path */
    _ntk.foreach_fanin( fanout_flow, [&]( auto const& f ) {
      if ( _ntk.is_constant( _ntk.get_node( f ) ) )
        return true;
      /* there is a path for flow */
      if ( max_flow_backwards_compute_rec( _ntk.get_node( f ) ) )
      {
        _flow_path[fanout_flow] = _ntk.node_to_index( _ntk.get_node( f ) );
        found_path = 1;
        return false;
      }
      return true;
    } );

    if ( found_path )
      return 1;

    if ( max_flow_backwards_compute_rec( fanout_flow ) )
    {
      _flow_path[fanout_flow] = 0;
      return 1;
    }

    return 0;
  }

  std::vector<node> get_min_cut()
  {
    std::vector<node> min_cut;
    min_cut.reserve( _ntk.num_registers() );

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( _flow_path[n] == 0 )
        return true;
      if ( _ntk.visited( n ) != _ntk.trav_id() )
        return true;

      if ( _ntk.value( n ) || _ntk.visited( _flow_path[n] ) != _ntk.trav_id() )
        min_cut.push_back( n );
      return true;
    } );

    return min_cut;
  }

  template<bool forward>
  void legalize_retiming( std::vector<node>& min_cut, uint32_t iteration )
  {
    _ntk.clear_values();

    _ntk.foreach_register( [&]( auto const& n ) {
      _ntk.set_value( _ntk.fanout( n )[0], 1 );
    } );

    for ( auto const& n : min_cut )
    {
      rec_mark_tfi( n );
    }

    min_cut.clear();

    if constexpr ( forward )
    {
      _ntk.foreach_gate( [&]( auto const& n ) {
        if ( _ntk.value( n ) == 1 )
        {
          /* if is sink or before a register */
          _ntk.foreach_fanout( n, [&]( auto const& f ) {
            if ( _ntk.value( f ) != 1 )
            {
              min_cut.push_back( n );
              return false;
            }
            return true;
          } );
        }
      } );
    }
    else
    {
      _ntk.incr_trav_id();
      _ntk.foreach_register( [&]( auto const& n ) {
        node fanin = _ntk.get_node( _ntk.get_fanin0( n ) );
        collect_cut_nodes_tfi( fanin, min_cut );
        return true;
      } );
      _ntk.foreach_node( [&]( auto const& n ) {
        if ( _ntk.visited( n ) == _ntk.trav_id() )
          _ntk.set_value( n, 1 );
        else
          _ntk.set_value( n, 0 );
      } );
      for ( auto const& n : min_cut )
        _ntk.set_value( n, 0 );
    }
  }

  void collect_cut_nodes_tfi( node const& n, std::vector<node>& min_cut )
  {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
      return;

    _ntk.set_visited( n, _ntk.trav_id() );

    if ( _ntk.value( n ) )
    {
      min_cut.push_back( n );
      return;
    }

    _ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( _ntk.is_constant( _ntk.get_node( f ) ) )
        return;
      collect_cut_nodes_tfi( _ntk.get_node( f ), min_cut );
    } );
  }

  template<bool forward>
  void init_values()
  {
    _ntk.clear_values();

    /* marks the frontiers */
    if constexpr ( forward )
    {
      /* mark POs as sink */
      _ntk.foreach_po( [&]( auto const& f ) {
        _ntk.set_value( _ntk.get_node( f ), 1 );
      } );

      /* mark registers as sink */
      _ntk.foreach_register( [&]( auto const& n ) {
        _ntk.set_value( n, 1 );
        _ntk.foreach_fanin( n, [&]( auto const& f ) {
          if ( _ntk.is_constant( _ntk.get_node( f ) ) )
            return;
          _ntk.set_value( _ntk.get_node( f ), 1 );
        } );
      } );

      /* exclude reachable nodes from PIs from retiming */
      _ntk.foreach_pi( [&]( auto const& n ) {
        rec_mark_tfo( n );
      } );

      /* mark childrens of marked nodes */
      std::vector<node> to_mark;
      to_mark.reserve( 200 );
      _ntk.foreach_gate( [&]( auto const& n ) {
        if ( _ntk.value( n ) == 1 )
        {
          _ntk.foreach_fanin( n, [&]( auto const& f ) {
            if ( _ntk.is_constant( _ntk.get_node( f ) ) )
              return;
            if ( _ntk.value( _ntk.get_node( f ) ) == 0 )
              to_mark.push_back( _ntk.get_node( f ) );
          } );
        }
      } );
      for ( auto const& n : to_mark )
      {
        _ntk.set_value( n, 1 );
      }
    }
    else
    {
      /* mark PIs as sink */
      _ntk.foreach_pi( [&]( auto const& n ) {
        _ntk.set_value( n, 1 );
      } );

      /* mark registers as sink */
      _ntk.foreach_register( [&]( auto const& n ) {
        _ntk.set_value( n, 1 );
        _ntk.foreach_fanout( n, [&]( auto const& f ) {
          _ntk.set_value( f, 1 );
        } );
      } );

      /* exclude reachable nodes from POs from retiming */
      _ntk.foreach_po( [&]( auto const& f ) {
        rec_mark_tfi( _ntk.get_node( f ) );
      } );
    }
  }

  template<bool forward>
  void update_registers_position( std::vector<node> const& min_cut, uint32_t iteration )
  {
    _ntk.incr_trav_id();

    /* create new registers and mark the ones to reuse */
    for ( auto const& n : min_cut )
    {
      if constexpr ( forward )
      {
        if ( _ntk.is_box_output( n ) )
        {
          /* reuse the current register */
          auto node_register = _ntk.get_node( _ntk.get_fanin0( n ) );
          auto in_register = _ntk.get_node( _ntk.get_fanin0( node_register ) );
          auto in_in_register = _ntk.get_node( _ntk.get_fanin0( in_register ) );

          /* check for marked fanouts to connect to register input */
          auto fanout = _ntk.fanout( n );
          for ( auto const& f : fanout )
          {
            if ( _ntk.value( f ) )
            {
              _ntk.replace_in_node( f, n, in_in_register );
              _ntk.decr_fanout_size( n );
            }
          }

          _ntk.set_visited( node_register, _ntk.trav_id() );
        }
        else
        {
          /* create a new register */
          auto const in_register = _ntk.create_box_input( _ntk.make_signal( n ) );
          auto const node_register = _ntk.create_register( in_register );
          auto const node_register_out = _ntk.create_box_output( node_register );

          /* replace in n fanout */
          auto fanout = _ntk.fanout( n );
          for ( auto const& f : fanout )
          {
            if ( f != _ntk.get_node( in_register ) && !_ntk.value( f ) )
            {
              _ntk.replace_in_node( f, n, node_register_out );
              _ntk.decr_fanout_size( n );
            }
          }

          _ntk.set_visited( _ntk.get_node( node_register ), _ntk.trav_id() );
        }
      }
      else
      {
        if ( _ntk.is_box_input( n ) )
        {
          _ntk.foreach_fanout( n, [&]( auto const& f ) {
            _ntk.set_visited( f, _ntk.trav_id() );
          } );
        }
        else
        {
          /* create a new register */
          auto const in_register = _ntk.create_box_input( _ntk.make_signal( n ) );
          auto const node_register = _ntk.create_register( in_register );
          auto const node_register_out = _ntk.create_box_output( node_register );

          /* replace in n fanout */
          auto fanout = _ntk.fanout( n );
          for ( auto const& f : fanout )
          {
            if ( f != _ntk.get_node( in_register ) && _ntk.value( f ) )
            {
              _ntk.replace_in_node( f, n, node_register_out );
              _ntk.decr_fanout_size( n );
            }
          }

          _ntk.set_visited( _ntk.get_node( node_register ), _ntk.trav_id() );
        }
      }
    }

    /* remove retimed registers */
    _ntk.foreach_register( [&]( auto const& n ) {
      if ( _ntk.visited( n ) == _ntk.trav_id() )
        return true;

      node node_register_out;
      node node_register_in = _ntk.get_node( _ntk.get_fanin0( n ) );
      signal node_register_in_in = _ntk.get_fanin0( node_register_in );

      _ntk.foreach_fanout( n, [&]( auto const& f ) {
        node_register_out = f;
      } );

      auto node_register_fanout = _ntk.fanout_size( node_register_out );
      auto fanin_fanout = _ntk.fanout_size( _ntk.get_node( node_register_in_in ) );
      auto fanin_type = _ntk.is_box_output( _ntk.get_node( node_register_in_in ) );

      _ntk.substitute_node( node_register_out, node_register_in_in );

      return true;
    } );
  }

  void rec_mark_tfo( node const& n )
  {
    if ( _ntk.value( n ) )
      return;

    _ntk.set_value( n, 1 );
    _ntk.foreach_fanout( n, [&]( auto const& f ) {
      rec_mark_tfo( f );
    } );
  }

  void rec_mark_tfi( node const& n )
  {
    if ( _ntk.value( n ) )
      return;

    _ntk.set_value( n, 1 );
    _ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( _ntk.is_constant( _ntk.get_node( f ) ) )
        return;
      rec_mark_tfi( _ntk.get_node( f ) );
    } );
  }

  template<bool forward>
  bool check_min_cut( std::vector<node> const& min_cut, uint32_t iteration )
  {
    _ntk.incr_trav_id();

    for ( node const& n : min_cut )
    {
      _ntk.set_visited( n, _ntk.trav_id() );
    }

    bool check = true;
    _ntk.foreach_register( [&]( auto const& n ) {
      if constexpr ( forward )
      {
        if ( !check_min_cut_rec<forward>( _ntk.fanout( n )[0] ) )
          check = false;
      }
      else
      {
        node fanin = _ntk.get_node( _ntk.get_fanin0( n ) );
        if ( !check_min_cut_rec<forward>( fanin ) )
          check = false;
      }

      return check;
    } );

    return check;
  }

  template<bool forward>
  bool check_min_cut_rec( node const& n )
  {
    bool check = true;

    if ( _ntk.visited( n ) == _ntk.trav_id() )
      return true;

    _ntk.set_visited( n, _ntk.trav_id() );

    if constexpr ( forward )
    {
      if ( _ntk.is_co( n ) )
      {
        check = false;
        return false;
      }

      _ntk.foreach_fanout( n, [&]( auto const& f ) {
        if ( !check_min_cut_rec<forward>( f ) )
        {
          check = false;
        }
      } );
    }
    else
    {
      if ( _ntk.is_ci( n ) )
      {
        check = false;
        return false;
      }

      _ntk.foreach_fanin( n, [&]( auto const& f ) {
        if ( _ntk.is_constant( _ntk.get_node( f ) ) )
          return true;
        if ( !check_min_cut_rec<forward>( _ntk.get_node( f ) ) )
        {
          check = false;
        }
        return check;
      } );

      return check;
    }

    return check;
  }

private:
  Ntk& _ntk;
  retime_params const& _ps;
  retime_stats& _st;

  node_map<uint32_t, Ntk> _flow_path;
};

} /* namespace detail */

/*! \brief Retiming.
 *
 * This function implements a retiming algorithm for registers minimization.
 * The only supported network type is the `generic_network`.
 * The algorithm excecutes the retiming inplace.
 * 
 * Currently, only area-based retiming is implemented. Mixed register types
 * such as (active high/low, rising/falling edge) are not supported yet.
 *
 * **Required network functions:**
 * - `size`
 * - `is_pi`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_po`
 * - `foreach_node`
 * - `fanout_size`
 * - `has_incr_value`
 * - `has_decr_value`
 * - `has_get_fanin0`
 *
 * \param ntk Network
 * \param ps Retiming params
 * \param pst Retiming statistics
 *
 * The implementation of this algorithm was inspired by the
 * mapping command ``retime`` in ABC.
 */
template<class Ntk>
void retime( Ntk& ntk, retime_params const& ps = {}, retime_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_incr_value_v<Ntk>, "Ntk does not implement the incr_value method" );
  static_assert( has_decr_value_v<Ntk>, "Ntk does not implement the decr_value method" );
  static_assert( has_get_fanin0_v<Ntk>, "Ntk does not implement the get_fanin0 method" );

  retime_stats st;

  using fanout_view_t = fanout_view<Ntk>;
  fanout_view_t fanout_view{ ntk };

  detail::retime_impl p( fanout_view, ps, st );
  p.run();

  if ( ps.verbose )
    st.report();

  if ( pst )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */
