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
  \file aqfp_legalization.hpp
  \brief Legalization + buffer optimization flow for AQFP networks

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <random>
#include <vector>

#include "../../networks/aqfp.hpp"
#include "../../networks/buffered.hpp"
#include "../../networks/mig.hpp"
#include "../../utils/stopwatch.hpp"
#include "../../views/depth_view.hpp"
#include "../cleanup.hpp"
#include "aqfp_assumptions.hpp"
#include "aqfp_rebuild.hpp"
#include "aqfp_retiming.hpp"
#include "buffer_insertion.hpp"

namespace mockturtle
{

struct aqfp_legalization_params
{
  aqfp_legalization_params()
  {
    aqfp_assumptions_ps.branch_pis = true;
    aqfp_assumptions_ps.balance_pis = true;
    aqfp_assumptions_ps.balance_pos = true;
  }

  /*! \brief legalization mode. */
  enum
  {
    better,
    portfolio
  } legalization_mode = portfolio;

  /*! \brief AQFP technology assumptions. */
  aqfp_assumptions_legacy aqfp_assumptions_ps{};

  /*! \brief Max number of optimization rounds (zero performs only insertion)*/
  uint32_t optimization_rounds{ 10 };

  /*! \brief Maximum chunk size for chunk optimization. */
  uint32_t max_chunk_size{ 100 };

  /*! \brief Maximum number of iterations for optimization using retiming. */
  uint32_t retime_iterations{ 250 };

  /*! \brief Enable optimization of splitters using retiming. */
  bool retime_splitters{ true };

  /*! \brief Enables the randomization of topological order. */
  bool topological_randomization{ true };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

struct aqfp_legalization_stats
{
  /*! \brief Number of Josephson Junctions (JJs). */
  uint32_t num_jjs{ 0 };

  /*! \brief Number of buffers and splitters. */
  uint32_t num_bufs{ 0 };

  /*! \brief Depth of the circuit. */
  uint32_t depth{ 0 };

  /*! \brief Total number of optimization rounds. */
  uint32_t rounds_total{ 0 };

  /*! \brief Time insertion. */
  stopwatch<>::duration time_insertion{ 0 };

  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  void report() const
  {
    std::cout << fmt::format( "[i] JJs    = {:7d}\t B/S     = {:7d}\t Depth = {:5d}\n", num_jjs, num_bufs, depth );
    std::cout << fmt::format( "[i] Rounds = {:7d}\t Time Insertion = {:>5.2f} secs\t Total Runtime = {:>5.2f} secs\n", rounds_total, to_seconds( time_insertion ), to_seconds( time_total ) );
  }
};

namespace detail
{

template<class Ntk>
class aqfp_legalization_impl
{
public:
  explicit aqfp_legalization_impl( Ntk const& ntk, aqfp_legalization_params const& ps, aqfp_legalization_stats& st )
      : _ntk( ntk ), _ps( ps ), _st( st )
  {
  }

public:
  buffered_aqfp_network run()
  {
    stopwatch t( _st.time_total );
    _st.rounds_total = 0;

    /* convert the initial circuit as an AQFP network */
    aqfp_network aqfp_start = cleanup_dangling<Ntk, aqfp_network>( _ntk );
    /* creates buffer_insertion instance for scheduling (can be reused) */
    buffer_insertion_params insertion_ps;
    insertion_ps.optimization_effort = buffer_insertion_params::none;
    insertion_ps.assume = legacy_to_realistic( _ps.aqfp_assumptions_ps );
    buffer_insertion buf_inst( aqfp_start, insertion_ps );

    bool retiming_backward_first = false;

    /* Starndard flow: insertion + optimization */
    if ( _ps.legalization_mode == aqfp_legalization_params::better )
    {
      buffered_aqfp_network buffered_aqfp = aqfp_buffer_insertion( buf_inst, retiming_backward_first );
      buffered_aqfp_network aqfp_res = aqfp_buffer_optimize( buffered_aqfp, retiming_backward_first );
      compute_stats( aqfp_res );
      return aqfp_res;
    }

    /* Portfolio flow: 2 insertions + 2 optimizations */
    buffered_aqfp_network buffered_aqfp_alap = aqfp_buffer_insertion( buf_inst, retiming_backward_first, true );
    buffered_aqfp_network aqfp_res_alap = aqfp_buffer_optimize( buffered_aqfp_alap, retiming_backward_first );

    buffered_aqfp_network buffered_aqfp_asap = aqfp_buffer_insertion( buf_inst, retiming_backward_first, false );
    buffered_aqfp_network aqfp_res_asap = aqfp_buffer_optimize( buffered_aqfp_asap, retiming_backward_first );

    buffered_aqfp_network aqfp_res;
    if ( aqfp_res_alap.size() < aqfp_res_asap.size() )
    {
      aqfp_res = aqfp_res_alap;
    }
    else
    {
      aqfp_res = aqfp_res_asap;
    }

    compute_stats( aqfp_res );
    return aqfp_res;
  }

private:
  buffered_aqfp_network aqfp_buffer_insertion( buffer_insertion<aqfp_network>& buf_inst, bool& direction, bool is_alap = false )
  {
    stopwatch t( _st.time_insertion );

    if ( _ps.legalization_mode == aqfp_legalization_params::portfolio )
    {
      if ( is_alap )
      {
        buf_inst.set_scheduling_policy( buffer_insertion_params::ALAP_depth );
      }
      else
      {
        buf_inst.set_scheduling_policy( buffer_insertion_params::ASAP_depth );
      }
    }
    else if ( _ps.legalization_mode == aqfp_legalization_params::better )
    {
      buf_inst.set_scheduling_policy( buffer_insertion_params::better_depth );
    }

    buffered_aqfp_network buffered_aqfp;
    buf_inst.run( buffered_aqfp );
    direction = buf_inst.is_scheduled_ASAP();

    return buffered_aqfp;
  }

  buffered_aqfp_network aqfp_buffer_optimize( buffered_aqfp_network& start, bool direction )
  {
    if ( _ps.optimization_rounds == 0 )
      return start;

    /* retiming params */
    aqfp_retiming_params aps;
    aps.aqfp_assumptions_ps = _ps.aqfp_assumptions_ps;
    aps.backwards_first = direction;
    aps.iterations = _ps.retime_iterations;
    aps.retime_splitters = _ps.retime_splitters;

    /* chunk movement params */
    buffer_insertion_params buf_ps;
    buf_ps.scheduling = buffer_insertion_params::provided;
    buf_ps.optimization_effort = buffer_insertion_params::until_sat;
    buf_ps.max_chunk_size = _ps.max_chunk_size;
    buf_ps.assume = legacy_to_realistic( _ps.aqfp_assumptions_ps );

    aqfp_reconstruct_params reconstruct_ps;
    aqfp_reconstruct_stats reconstruct_st;
    reconstruct_ps.buffer_insertion_ps = buf_ps;

    /* aqfp network */
    buffered_aqfp_network buffered_aqfp;

    /* first retiming */
    {
      auto buf_aqfp_ret = aqfp_retiming( start, aps );
      buffered_aqfp = buf_aqfp_ret;
    }

    /* repeat loop */
    uint32_t iterations = _ps.optimization_rounds;
    aps.det_randomization = _ps.topological_randomization;
    std::default_random_engine rng( 111 );
    while ( iterations-- > 0 )
    {
      uint32_t size_previous = buffered_aqfp.size();

      /* chunk movement */
      auto buf_aqfp_chunk = aqfp_reconstruct( buffered_aqfp, reconstruct_ps, &reconstruct_st );

      /* retiming */
      aps.seed = rng();
      auto buf_aqfp_ret = aqfp_retiming( buf_aqfp_chunk, aps );

      _st.rounds_total++;

      if ( buf_aqfp_ret.size() >= size_previous )
        break;

      buffered_aqfp = buf_aqfp_ret;
    }

    return buffered_aqfp;
  }

  void compute_stats( buffered_aqfp_network const& buffered_aqfp )
  {
    _st.depth = depth_view<buffered_aqfp_network>( buffered_aqfp ).depth();
    _st.num_bufs = 0;
    _st.num_jjs = 0;

    buffered_aqfp.foreach_node( [&]( auto const& n ) {
      if ( buffered_aqfp.is_pi( n ) || buffered_aqfp.is_constant( n ) )
        return;
      if ( buffered_aqfp.is_buf( n ) )
      {
        _st.num_jjs += 2;
        _st.num_bufs++;
      }
      else
      {
        _st.num_jjs += 6;
      }
    } );
  }

private:
  Ntk const& _ntk;
  aqfp_legalization_params const& _ps;
  aqfp_legalization_stats& _st;
};

} /* namespace detail */

/*! \brief AQFP legalization.
 *
 * This function returns an optimized AQFP circuit
 * derived from the input one by inserting buffer
 * and splitter elements and optimizing their number.
 *
 * Parameters can be used to set the B/S insertion and
 * optimization.
 *
 * \param ntk Boolean network as an MIG or AQFP network
 * \param ps AQFP legalization parameters
 */
template<class Ntk>
buffered_aqfp_network aqfp_legalization( Ntk const& ntk, aqfp_legalization_params const& ps = {}, aqfp_legalization_stats* pst = nullptr )
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
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( std::is_same_v<typename Ntk::base_type, mig_network> || std::is_same_v<typename Ntk::base_type, aqfp_network>, "Ntk in not an MIG or AQFP network type" );

  aqfp_legalization_stats st;

  detail::aqfp_legalization_impl p( ntk, ps, st );
  auto res = p.run();

  if ( ps.verbose )
    st.report();

  if ( pst )
    *pst = st;

  return res;
}

} // namespace mockturtle