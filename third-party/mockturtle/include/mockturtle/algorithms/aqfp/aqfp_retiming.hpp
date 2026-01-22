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
  \file aqfp_retiming.hpp
  \brief Retiming for AQFP networks

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <random>
#include <vector>

#include "../../networks/buffered.hpp"
#include "../../networks/generic.hpp"
#include "../../networks/klut.hpp"
#include "../../utils/node_map.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/choice_view.hpp"
#include "../../views/fanout_view.hpp"
#include "../../views/topo_view.hpp"
#include "../retiming.hpp"
#include "aqfp_assumptions.hpp"
#include "aqfp_rebuild.hpp"

namespace mockturtle
{

struct aqfp_retiming_params
{
  /*! \brief AQFP technology assumptions. */
  aqfp_assumptions aqfp_assumptions_ps{};

  /*! \brief Max number of iterations */
  uint32_t iterations{ UINT32_MAX };

  /*! \brief Enable splitter retiming. */
  bool retime_splitters{ true };

  /*! \brief Order of retiming is backward first. */
  bool backwards_first{ true };

  /*! \brief Adds an additional try for retiming */
  uint32_t additional_try_iterations{ 1 };

  /*! \brief Forward retiming only. */
  bool forward_only{ false };

  /*! \brief Backward retiming only. */
  bool backward_only{ false };

  /*! \brief Random seed. */
  std::default_random_engine::result_type seed{ 1 };

  /*! \brief Randomize the network. */
  bool det_randomization{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

struct aqfp_retiming_stats
{
  /*! \brief Initial number of buffers/splitters. */
  uint32_t buffers_pre{ 0 };

  /*! \brief Number of buffers/splitters after the algorithm. */
  uint32_t buffers_post{ 0 };

  /*! \brief Total iterations. */
  uint32_t rounds_total{ 0 };

  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  void report() const
  {
    std::cout << fmt::format( "[i] Initial B/S   = {:7d}\t Final B/S   = {:7d}\n", buffers_pre, buffers_post );
    std::cout << fmt::format( "[i] Total rounds  = {:7d}\n", rounds_total );
    std::cout << fmt::format( "[i] Total runtime = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

template<class Ntk>
class aqfp_retiming_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using signal_g = typename generic_network::signal;
  using classes_t = std::vector<std::vector<node>>;

public:
  explicit aqfp_retiming_impl( Ntk& ntk, aqfp_retiming_params const& ps, aqfp_retiming_stats& st )
      : _ntk( ntk ), _ps( ps ), _st( st )
  {
  }

public:
  Ntk run()
  {
    stopwatch t( _st.time_total );

    retime_params rps;
    std::default_random_engine::result_type seed = _ps.seed;

    _st.buffers_pre = get_stats( _ntk );

    Ntk ntk = _ntk;
    uint32_t additional_try = _ps.additional_try_iterations;

    if ( _ps.aqfp_assumptions_ps.splitter_capacity != 2 )
      rps.iterations = 1;

    buffer_insertion_params buf_ps;
    buf_ps.assume = legacy_to_realistic( _ps.aqfp_assumptions_ps );
    buf_ps.scheduling = buffer_insertion_params::provided;
    buf_ps.optimization_effort = buffer_insertion_params::none;
    aqfp_reconstruct_params reconstruct_ps;
    aqfp_reconstruct_stats reconstruct_st;
    reconstruct_ps.buffer_insertion_ps = buf_ps;
    reconstruct_ps.det_randomization = _ps.det_randomization;

    /* retiming first direction pass */
    rps.forward_only = !_ps.backwards_first;
    rps.backward_only = _ps.backwards_first;
    uint32_t i = _ps.iterations;

    if ( ( _ps.backwards_first && !_ps.forward_only ) || ( !_ps.backwards_first && !_ps.backward_only ) )
    {
      while ( i-- > 0 )
      {
        auto net = to_generic( ntk, seed, !_ps.backwards_first );
        auto num_registers_before = net.num_registers();

        retime( net, rps );

        if ( net.num_registers() >= num_registers_before )
        {
          if ( additional_try-- == 0 )
            break;
        }
        else if ( additional_try )
        {
          additional_try = _ps.additional_try_iterations;
        }

        ntk = to_buffered( net );
        ++_st.rounds_total;
      }
    }

    /* retiming second direction pass */
    rps.forward_only = _ps.backwards_first;
    rps.backward_only = !_ps.backwards_first;
    i = _ps.iterations;
    additional_try = _ps.additional_try_iterations;

    if ( ( !_ps.backwards_first && !_ps.forward_only ) || ( _ps.backwards_first && !_ps.backward_only ) )
    {
      while ( i-- > 0 )
      {
        auto net = to_generic( ntk, seed, _ps.backwards_first );
        auto num_registers_before = net.num_registers();

        retime( net, rps );

        if ( net.num_registers() >= num_registers_before )
        {
          if ( additional_try-- == 0 )
            break;
        }
        else if ( additional_try )
        {
          additional_try = _ps.additional_try_iterations;
        }
        ntk = to_buffered( net );
        ++_st.rounds_total;
      }
    }

    auto res = aqfp_reconstruct( ntk, reconstruct_ps, &reconstruct_st );
    _st.buffers_post = reconstruct_st.num_buffers;
    return res;
  }

private:
  uint32_t get_stats( Ntk& ntk )
  {
    uint32_t bs_count = 0;
    ntk.foreach_node( [&]( auto const& n ) {
      if ( ntk.is_buf( n ) )
        ++bs_count;
    } );

    return bs_count;
  }

  generic_network to_generic( Ntk& ntk, std::default_random_engine::result_type& seed, bool forward )
  {
    node_map<signal_g, Ntk> old2new( ntk );
    generic_network res;

    old2new[ntk.get_constant( false )] = res.get_constant( false );
    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
    {
      old2new[ntk.get_constant( true )] = res.get_constant( true );
    }
    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[n] = res.create_pi();
    } );

    /* suppose network is in topological order */
    if ( _ps.retime_splitters && _ps.aqfp_assumptions_ps.splitter_capacity != 2 )
    {
      select_retimeable_elements_random( ntk, seed, forward );
    }
    else
    {
      select_buffers( ntk );
    }

    create_generic_network( ntk, res, old2new );

    return res;
  }

  Ntk to_buffered( generic_network const& ntk )
  {
    node_map<signal, generic_network> old2new( ntk );
    Ntk res;

    old2new[ntk.get_constant( false )] = res.get_constant( false );
    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
    {
      old2new[ntk.get_constant( true )] = res.get_constant( true );
    }
    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[n] = res.create_pi();
    } );

    topo_view topo{ ntk };

    topo.foreach_node( [&]( auto const& n ) {
      if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
        return true;

      /* remove not represented nodes */
      if ( ntk.is_box_input( n ) || ntk.is_box_output( n ) || ntk.is_po( n ) )
      {
        signal children;
        ntk.foreach_fanin( n, [&]( auto const& f ) {
          children = old2new[f];
        } );

        old2new[n] = children;
        return true;
      }

      std::vector<signal> children;

      ntk.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( old2new[f] );
      } );

      signal f;

      if ( ntk.fanin_size( n ) >= 3 )
      {
        if constexpr ( has_create_maj_odd_v<Ntk> )
        {
          if ( ntk.fanin_size( n ) > 3 )
            f = res.create_maj( children );
          else
            f = res.create_maj( children[0], children[1], children[2] );
        }
        else
        {
          f = res.create_maj( children[0], children[1], children[2] );
        }
      }
      else if ( ntk.fanin_size( n ) == 1 && ntk.node_function( n )._bits[0] == 0x1 )
      {
        /* not */
        assert( children.size() == 1 );
        f = !children[0];
      }
      else
      {
        /* buffer */
        /* not balanced PIs */
        if ( !_ps.aqfp_assumptions_ps.balance_pis && ( res.is_pi( res.get_node( children[0] ) ) || res.is_constant( res.get_node( children[0] ) ) ) )
          f = children[0];
        else
          f = res.create_buf( children[0] );
      }

      old2new[n] = f;
      return true;
    } );

    ntk.foreach_po( [&]( auto const& f ) {
      res.create_po( old2new[f] );
    } );

    return res;
  }

  void select_retimeable_elements_random( Ntk& ntk, std::default_random_engine::result_type& seed, bool forward )
  {
    fanout_view fntk{ ntk };

    ntk.clear_values();

    /* select buffers and splitters to retime as soon as found some */
    ntk.incr_trav_id();
    ntk.foreach_node( [&]( auto const& n ) {
      if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
        return true;

      if ( ntk.is_buf( n ) )
      {
        if ( ntk.fanout_size( n ) == 1 )
        {
          if ( forward )
          {
            fntk.foreach_fanout( n, [&]( auto const& f ) {
              if ( !ntk.is_buf( f ) || ntk.fanout_size( f ) != 1 )
                ntk.set_visited( n, ntk.trav_id() );
            } );
          }
          else
          {
            ntk.foreach_fanin( n, [&]( auto const& f ) {
              if ( ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() || !ntk.is_buf( ntk.get_node( f ) ) || ntk.fanout_size( ntk.get_node( f ) ) != 1 )
                ntk.set_visited( n, ntk.trav_id() );
            } );
          }
        }
        else if ( ntk.visited( n ) != ntk.trav_id() || ntk.value( n ) > 0 )
        {
          int free_spots;
          if ( ntk.value( n ) > 0 )
          {
            free_spots = rec_fetch_root( ntk, n ); /* aparently useless */
            if ( free_spots == 0 )
              return true;
          }
          else
          {
            free_spots = _ps.aqfp_assumptions_ps.splitter_capacity - ntk.fanout_size( n );
          }

          int total_fanout = 0;
          std::vector<node> fanout_splitters;

          /* select retimeable splitters */
          fntk.foreach_fanout( n, [&]( auto const f ) {
            if ( ntk.is_buf( f ) && ntk.fanout_size( f ) > 1 && free_spots >= ntk.fanout_size( f ) - 1 )
            {
              fanout_splitters.push_back( f );
              total_fanout += ntk.fanout_size( f ) - 1;
            }
          } );

          /* check if they are all retimeable together */
          if ( free_spots >= total_fanout )
          {
            for ( auto f : fanout_splitters )
            {
              ntk.set_value( f, free_spots - total_fanout );
              ntk.set_visited( f, ntk.trav_id() );
            }
            rec_update_root( ntk, n, free_spots - total_fanout );
            return true;
          }
          /* select one randomly */
          std::default_random_engine gen( seed++ );
          std::uniform_int_distribution<uint32_t> dist( 0ul, fanout_splitters.size() - 1 );
          auto index = dist( gen );
          ntk.set_value( fanout_splitters[index], free_spots - ntk.fanout_size( fanout_splitters[index] ) + 1 );
          ntk.set_visited( fanout_splitters[index], ntk.trav_id() );
          rec_update_root( ntk, n, free_spots - ntk.fanout_size( fanout_splitters[index] ) + 1 );
        }
      }
      return true;
    } );
  }

  void create_generic_network( Ntk& ntk, generic_network& res, node_map<signal_g, Ntk>& old2new )
  {
    ntk.foreach_node( [&]( auto const& n ) {
      if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
        return true;

      std::vector<signal_g> children;

      ntk.foreach_fanin( n, [&]( auto const& f ) {
        if ( ntk.is_complemented( f ) )
          children.push_back( res.create_not( old2new[f] ) );
        else
          children.push_back( old2new[f] );
      } );

      if ( ntk.is_buf( n ) && ntk.visited( n ) == ntk.trav_id() )
      {
        auto const in_register = res.create_box_input( children[0] );
        auto const node_register = res.create_register( in_register );
        auto const node_register_out = res.create_box_output( node_register );
        old2new[n] = node_register_out;
      }
      else
      {
        const auto f = res.create_node( children, ntk.node_function( n ) );
        old2new[n] = f;
      }

      return true;
    } );

    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
        res.create_po( res.create_not( old2new[f] ) );
      else
        res.create_po( old2new[f] );
    } );
  }

  void forward_compatibility( Ntk& ntk, choice_view<Ntk>& choice_ntk )
  {
    ntk.foreach_node( [&]( auto const n ) {
      if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
        return;

      /* assign classes */
      unsigned value = 0;
      ntk.foreach_fanin( n, [&]( auto const f ) {
        if ( ntk.value( ntk.get_node( f ) ) )
        {
          if ( value )
            choice_ntk.add_choice( value, ntk.value( ntk.get_node( f ) ) );
          else
            value = ntk.value( ntk.get_node( f ) );
        }
      } );

      /* propagate classes */
      if ( ntk.visited( n ) != ntk.trav_id() && !ntk.value( n ) )
        ntk.set_value( n, value );
    } );
  }

  void backward_compatibility( Ntk& ntk, choice_view<Ntk>& choice_ntk, fanout_view<Ntk>& fntk )
  {
    std::vector<node> topo_order;
    topo_order.reserve( ntk.size() );
    ntk.foreach_node( [&]( auto const n ) {
      if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
        return;

      topo_order.push_back( n );
    } );

    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      /* assign classes */
      unsigned value = 0;
      fntk.foreach_fanout( *it, [&]( auto const f ) {
        if ( ntk.value( f ) )
        {
          if ( value )
            choice_ntk.add_choice( value, ntk.value( f ) );
          else
            value = ntk.value( f );
        }
      } );

      /* propagate classes */
      if ( ntk.visited( *it ) != ntk.trav_id() && !ntk.value( *it ) )
        ntk.set_value( *it, value );
    }
  }

  classes_t create_classes( choice_view<Ntk>& choice_ntk )
  {
    classes_t classes;
    choice_ntk.foreach_node( [&]( auto const n ) {
      if ( choice_ntk.is_pi( n ) || choice_ntk.is_constant( n ) )
        return;
      /* create unique classes */
      if ( choice_ntk.value( n ) == n )
      {
        std::vector<node> comp_class;
        choice_ntk.foreach_choice( n, [&]( auto const& f ) {
          comp_class.push_back( f );
          choice_ntk.set_value( f, 0 );
          return true;
        } );
        classes.push_back( comp_class );
      }
    } );

    std::stable_sort( classes.begin(), classes.end(), [&]( std::vector<node> const& a, std::vector<node> const& b ) { return a.size() > b.size(); } );

    return classes;
  }

  uint32_t rec_fetch_root( Ntk& ntk, node const n )
  {
    uint32_t value;

    if ( ntk.visited( n ) != ntk.trav_id() )
      return ntk.value( n );

    ntk.foreach_fanin( n, [&]( auto const f ) {
      auto g = ntk.get_node( f );
      if ( !ntk.is_buf( g ) || ntk.fanout_size( g ) == 1 )
        value = ntk.value( n );
      else
        value = rec_fetch_root( ntk, g );
    } );

    return value;
  }

  void rec_update_root( Ntk& ntk, node const n, uint32_t const update )
  {
    if ( !ntk.is_buf( n ) || ntk.fanout_size( n ) == 1 )
      return;

    ntk.set_value( n, update );

    if ( ntk.visited( n ) != ntk.trav_id() )
      return;

    ntk.foreach_fanin( n, [&]( auto const f ) {
      rec_update_root( ntk, ntk.get_node( f ), update );
    } );
  }

  void select_buffers( Ntk& ntk )
  {
    ntk.incr_trav_id();
    ntk.foreach_node( [&]( auto const& n ) {
      if ( ntk.is_buf( n ) && ntk.fanout_size( n ) == 1 )
        ntk.set_visited( n, ntk.trav_id() );
    } );
  }

  Ntk create_supersplitters()
  {
    Ntk res;
    node_map<signal, Ntk> old2new( _ntk );

    old2new[_ntk.get_constant( false )] = res.get_constant( false );
    if ( _ntk.get_node( _ntk.get_constant( true ) ) != _ntk.get_node( _ntk.get_constant( false ) ) )
    {
      old2new[_ntk.get_constant( true )] = res.get_constant( true );
    }
    _ntk.foreach_pi( [&]( auto const& n ) {
      old2new[n] = res.create_pi();
    } );

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( _ntk.is_pi( n ) || _ntk.is_constant( n ) )
        return;

      std::vector<signal> children;

      _ntk.foreach_fanin( n, [&]( auto const& f ) {
        children.push_back( old2new[f] ^ _ntk.is_complemented( f ) );
      } );

      signal f;
      if ( _ntk.is_buf( n ) )
      {
        uint32_t supersplitter = res.value( res.get_node( children[0] ) );
        if ( !supersplitter )
        {
          bool is_complemented = res.is_complemented( children[0] );
          f = res.create_buf( children[0] ^ is_complemented );
          res.set_value( res.get_node( children[0] ), res.node_to_index( res.get_node( f ) ) );
          f = f ^ is_complemented;
        }
        else
        {
          f = res.make_signal( res.index_to_node( supersplitter ) ) ^ res.is_complemented( children[0] );
        }
      }
      else
      {
        f = res.clone_node( _ntk, n, children );
      }
      old2new[n] = f;
    } );

    _ntk.foreach_po( [&]( auto const& f ) {
      if ( _ntk.is_complemented( f ) )
        res.create_po( res.create_not( old2new[f] ) );
      else
        res.create_po( old2new[f] );
    } );

    return res;
  }

private:
  Ntk& _ntk;
  aqfp_retiming_params const& _ps;
  aqfp_retiming_stats& _st;
};

} /* namespace detail */

/*! \brief AQFP retiming.
 *
 * This function applies a retiming-based approach
 * for splitters and buffers minimization.
 *
 * \param ntk Buffered network
 * \param ps AQFP retiming params
 */
template<class Ntk>
Ntk aqfp_retiming( Ntk& ntk, aqfp_retiming_params const& ps = {}, aqfp_retiming_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_complemented_v<Ntk>, "NtkDest does not implement the is_complemented method" );
  static_assert( is_buffered_network_type_v<Ntk>, "BufNtk is not a buffered network type" );
  static_assert( has_is_buf_v<Ntk>, "BufNtk does not implement the is_buf method" );
  static_assert( has_create_buf_v<Ntk>, "BufNtk does not implement the create_buf method" );

  aqfp_retiming_stats st;

  detail::aqfp_retiming_impl p( ntk, ps, st );
  auto res = p.run();

  if ( ps.verbose )
    st.report();

  if ( pst )
    *pst = st;

  return res;
}

} // namespace mockturtle