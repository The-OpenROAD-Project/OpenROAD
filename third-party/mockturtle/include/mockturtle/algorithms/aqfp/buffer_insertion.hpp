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
  \file buffer_insertion.hpp
  \brief Insert buffers and splitters for the AQFP technology

  \author Siang-Yun (Sonia) Lee
  \author Alessandro Tempia Calvino
*/

#pragma once

#include "../../traits.hpp"
#include "../../utils/node_map.hpp"
#include "../../views/fanout_view.hpp"
#include "../../views/topo_view.hpp"
#include "aqfp_assumptions.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <set>
#include <string>
#include <vector>

namespace mockturtle
{

/*! \brief Parameters for (AQFP) buffer insertion.
 */
struct buffer_insertion_params
{
  /*! \brief Technology assumptions. */
  aqfp_assumptions_realistic assume;

  /*! \brief The scheduling strategy to get the initial depth assignment.
   * - `provided` = An initial level assignment is given in the constructor, thus
   * no scheduling is performed. It is the user's responsibility to ensure that
   * the provided assignment is legal.
   * - `ASAP` = Classical As-Soon-As-Possible scheduling
   * - `ASAP_depth` = As-Soon-As-Possible scheduling with depth optimality
   * - `ALAP` = ASAP (to obtain depth) followed by As-Late-As-Possible scheduling
   * - `ALAP_depth` = As-Late-As-Possible scheduling with depth optimality
   * - `better` = ASAP followed by ALAP, then count buffers for both assignments
   * and choose the better one
   * - `better_depth` = ALAP_depth followed by ASAP_depth, then count buffers for both assignments
   * and choose the better one
   */
  enum scheduling_policy
  {
    provided,
    ASAP,
    ASAP_depth,
    ALAP,
    ALAP_depth,
    better,
    better_depth
  } scheduling = ASAP;

  /*! \brief The level of optimization effort.
   * - `none` = No optimization
   * - `one_pass` = Try to form a chunk starting from each gate, once
   * for all gates
   * - `until_sat` = Iterate over all gates until no more beneficial
   * chunk movement can be found
   * - `optimal` = Use an SMT solver to find the global optimal
   */
  enum
  {
    none,
    one_pass,
    until_sat,
    optimal
  } optimization_effort = none;

  /*! \brief The maximum size of a chunk. */
  uint32_t max_chunk_size{ 100u };
};

/*! \brief Insert buffers and splitters for the AQFP technology.
 *
 * In the AQFP technology, (1) logic gates can only have one fanout. If more than one
 * fanout is needed, a splitter has to be inserted in between, which also
 * takes one clocking phase (counted towards the network depth). (2) All fanins of
 * a logic gate have to arrive at the same time (be at the same level). If one
 * fanin path is shorter, buffers have to be inserted to balance it.
 * Buffers and splitters are essentially the same component in this technology.
 *
 * With a given level assignment to all gates in the network, the minimum number of
 * buffers needed is determined. This class implements algorithms to count such
 * "irredundant buffers" and to insert them to obtain a buffered network. Moreover,
 * as buffer optimization is essentially a problem of obtaining a good level assignment,
 * this class also implements algorithms to obtain an initial, legal assignment using
 * scheduling algorithms and to further adjust and optimize it.
 *
 * This class provides two easy-to-use top-level functions which wrap all the above steps
 * together: `run` and `dry_run`. In addition, the following interfaces are kept for
 * more fine-grained usage:
 * - Query the current level assignment (`level`, `depth`)
 * - Count irredundant buffers based on the current level assignment (`count_buffers`,
 * `num_buffers`)
 * - Optimize buffer count by scheduling (`schedule`, `ASAP`, `ALAP`) and by adjusting
 * the level assignment with chunked movement (`optimize`)
 * - Dump the resulting network into a network type which provides representation for
 * buffers (`dump_buffered_network`)
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      mig_network mig = ...

      buffer_insertion_params ps;
      ps.scheduling = buffer_insertion_params::ALAP;
      ps.optimization_effort = buffer_insertion_params::one_pass;

      buffer_insertion buffering( mig, ps );
      buffered_mig_network buffered_mig;
      auto const num_buffers = buffering.run( buffered_mig );

      std::cout << num_buffers << std::endl;
      assert( verify_aqfp_buffer( buffered_mig, ps.assume ) );
      write_verilog( buffered_mig, "buffered.v" );
   \endverbatim
 *
 * **Required network functions:**
 * - `foreach_node`
 * - `foreach_gate`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_pi`
 * - `is_constant`
 * - `get_node`
 * - `fanout_size`
 * - `size`
 * - `set_visited`
 * - `set_value`
 *
 */
template<typename Ntk>
class buffer_insertion
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit buffer_insertion( Ntk const& ntk, buffer_insertion_params const& ps = {} )
      : _ntk( ntk ), _ps( ps ), _levels( _ntk ), _po_levels( _ntk.num_pos(), 0u ), _timeframes( _ntk ), _fanouts( _ntk ), _num_buffers( _ntk )
  {
    static_assert( !is_buffered_network_type_v<Ntk>, "Ntk is already buffered" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );

    assert( _ps.scheduling != buffer_insertion_params::provided );

    // checks for assumptions
    assert( _ps.assume.ci_phases.size() > 0 );
    assert( _ps.assume.ignore_co_negation ); // consideration of CO negation is too complicated and neglected for now
  }

  explicit buffer_insertion( Ntk const& ntk, node_map<uint32_t, Ntk> const& levels, std::vector<uint32_t> const& po_levels, buffer_insertion_params const& ps = {} )
      : _ntk( ntk ), _ps( ps ), _levels( levels ), _po_levels( po_levels ), _timeframes( _ntk ), _fanouts( _ntk ), _num_buffers( _ntk )
  {
    static_assert( !is_buffered_network_type_v<Ntk>, "Ntk is already buffered" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );

    assert( _ps.scheduling == buffer_insertion_params::provided );
    assert( _po_levels.size() == _ntk.num_pos() );
  }

  /*! \brief Insert buffers and obtain a buffered network.
   *
   * \param bufntk An empty network of an appropriate buffered network type to
   * to store the buffer-insertion result
   * \return The number of buffers in the resulting network
   */
  template<class BufNtk>
  uint32_t run( BufNtk& bufntk )
  {
    dry_run();
    dump_buffered_network( bufntk );
    return num_buffers();
  }

  /*! \brief Insert buffers and obtain a buffered network.
   *
   * It is suggested to write the `pi_levels` information into a dumped file
   * for easier recovery of the scheduled phase assignment.
   *
   * \param bufntk An empty network of an appropriate buffered network type to
   * to store the buffer-insertion result
   * \param pi_lvls A vector which will store the PI level assignment (it is
   * recommended to store this information together with the buffered network)
   * \return The number of buffers in the resulting network
   */
  template<class BufNtk>
  uint32_t run( BufNtk& bufntk, std::vector<uint32_t>& pi_lvls )
  {
    dry_run();
    dump_buffered_network( bufntk );
    pi_lvls = pi_levels();
    return num_buffers();
  }

  /*! \brief Count the number of buffers without dumping the result into a buffered network.
   *
   * This function saves some runtime for dumping the resulting network and
   * allows users to experiment on the algorithms with new network types whose
   * corresponding buffered_network are not implemented yet.
   *
   * `pLevels` and `pPOLevels` can be used to create another `buffer_insertion` instance of
   * the same state (current schedule), which also define a unique buffered network. (Set
   * `ps.scheduling = provided` and `ps.optimization_effort = none`)
   *
   * \return The number of buffers in the resulting network
   */
  uint32_t dry_run()
  {
    schedule();
    optimize();
    count_buffers();
    return num_buffers();
  }

#pragma region Query
  node_map<uint32_t, Ntk> const& levels() const
  {
    return _levels;
  }

  /*! \brief Level of node `n` considering buffer/splitter insertion. */
  uint32_t level( node const& n ) const
  {
    assert( n < _ntk.size() );
    return _levels[n];
  }

  std::vector<uint32_t> const& po_levels() const
  {
    return _po_levels;
  }

  /*! \brief Level of the `idx`-th PO (imaginary dummy PO node, not counted in depth). */
  uint32_t po_level( uint32_t idx ) const
  {
    assert( idx < _ntk.num_pos() );
    return _po_levels[idx];
  }

  std::vector<uint32_t> pi_levels() const
  {
    std::vector<uint32_t> lvls;
    _ntk.foreach_pi( [&]( auto n ){
      lvls.emplace_back( _levels[n] );
    } );
    return lvls;
  }

  /*! \brief Network depth considering AQFP buffers/splitters.
   *
   * Should be equal to `max( po_level(i) - 1 )`.
   *
   * This is the number of phases from the previous-stage register to the
   * next-stage register, including the depth of the previous-stage register
   * (i.e., from one register input to the next register input).
   */
  uint32_t depth() const
  {
    return _depth;
  }

  /*! \brief The total number of buffers in the network under the current
   * level assignment. */
  uint32_t num_buffers() const
  {
    assert( !_outdated && "Please call `count_buffers()` first." );

    uint32_t count = 0u;
    _ntk.foreach_node( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) )
        count += num_buffers( n );
    } );
    return count;
  }

  /*! \brief The number of buffers between `n` and all of its fanouts under
   * the current level assignment. */
  uint32_t num_buffers( node const& n ) const
  {
    assert( !_outdated && "Please call `count_buffers()` first." );
    return _num_buffers[n];
  }

  /*! \brief The chosen schedule is ASAP */
  bool is_scheduled_ASAP() const
  {
    return _is_scheduled_ASAP;
  }
#pragma endregion

#pragma region Count buffers
  /*! \brief Count the number of buffers needed at the fanout of each gate
   * according to the current level assignment.
   *
   * This function must be called after level (re-)assignment and before
   * querying `num_buffers`.
   */
  void count_buffers()
  {
    if ( _outdated )
    {
      update_fanout_info();
    }

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) )
        _num_buffers[n] = count_buffers( n );
    } );
  }

private:
  uint32_t count_buffers( node const& n ) const
  {
    assert( !_outdated && "Please call `update_fanout_info()` first." );
    auto const& fo_infos = _fanouts[n];

    if ( _ntk.fanout_size( n ) == 0u ) /* dangling */
    {
      if ( !_ntk.is_pi( n ) )
        std::cerr << "[w] node " << n << " (which is not a PI) is dangling.\n";
      return 0u;
    }

    if ( _ntk.fanout_size( n ) == 1u ) /* single fanout */
    {
      assert( fo_infos.size() == 1u );
      return fo_infos.front().relative_depth - 1u;
    }

    if ( _ps.assume.ci_capacity > 1 && _ntk.is_pi( n ) )
    {
      if ( fo_infos.size() == 1u )
      {
        assert( fo_infos.front().relative_depth == 1u );
        return 0u;
      }
    }

    assert( fo_infos.size() > 1u );
    auto it = fo_infos.begin();
    uint32_t count = it->num_edges - it->fanouts.size() - it->extrefs.size();
    uint32_t rd = it->relative_depth;
    for ( ++it; it != fo_infos.end(); ++it )
    {
      count += it->num_edges - it->fanouts.size() - it->extrefs.size() + it->relative_depth - rd - 1;
      rd = it->relative_depth;
    }

    return count;
  }

  /* (Upper bound on) the additional depth caused by a balanced splitter tree at the output of node `n`. */
  uint32_t num_splitter_levels( node const& n ) const
  {
    assert( n < _ntk.size() );
    if ( _ntk.is_pi( n ) )
    {
      if ( _ntk.fanout_size( n ) > _ps.assume.ci_capacity )
        return std::ceil( std::log( _ntk.fanout_size( n ) - _ps.assume.ci_capacity + 1 ) / std::log( _ps.assume.splitter_capacity ) );
      else
        return 0u;
    }
    return std::ceil( std::log( _ntk.fanout_size( n ) ) / std::log( _ps.assume.splitter_capacity ) );
  }

  /* Update fanout_information of all nodes */
  void update_fanout_info()
  {
    _fanouts.reset();

    _ntk.foreach_gate( [&]( auto const& n ) {
      _ntk.foreach_fanin( n, [&]( auto const& fi ) {
        auto const ni = _ntk.get_node( fi );
        if ( !_ntk.is_constant( ni ) )
          insert_fanout( ni, n );
      } );
    } );

    _ntk.foreach_po( [&]( auto const& f, auto i ){
      insert_extref( _ntk.get_node( f ), i );
    } );

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) )
        count_edges( n );
    } );

    _outdated = false;
  }

  /* Update the fanout_information of a node */
  template<bool verify = false>
  bool update_fanout_info( node const& n )
  {
    std::vector<node> fos;
    std::vector<uint32_t> extrefs;
    for ( auto it = _fanouts[n].begin(); it != _fanouts[n].end(); ++it )
    {
      if ( it->fanouts.size() )
      {
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
          fos.push_back( *it2 );
      }
      if ( it->extrefs.size() )
      {
        for ( auto it2 = it->extrefs.begin(); it2 != it->extrefs.end(); ++it2 )
          extrefs.push_back( *it2 );
      }
    }

    _fanouts[n].clear();
    for ( auto& fo : fos )
      insert_fanout( n, fo );
    for ( auto& po : extrefs )
      insert_extref( n, po );

    return count_edges<verify>( n );
  }

  void insert_fanout( node const& n, node const& fanout )
  {
    assert( _levels[fanout] > _levels[n] );
    auto const rd = _levels[fanout] - _levels[n];
    auto& fo_infos = _fanouts[n];
    for ( auto it = fo_infos.begin(); it != fo_infos.end(); ++it )
    {
      if ( it->relative_depth == rd )
      {
        it->fanouts.push_back( fanout );
        ++it->num_edges;
        return;
      }
      else if ( it->relative_depth > rd )
      {
        fo_infos.insert( it, { rd, { fanout }, {}, 1u } );
        return;
      }
    }
    fo_infos.push_back( { rd, { fanout }, {}, 1u } );
  }

  void insert_extref( node const& n, uint32_t idx )
  {
    assert( _po_levels[idx] > _levels[n] );
    auto const rd = _po_levels[idx] - _levels[n];
    auto& fo_infos = _fanouts[n];
    for ( auto it = fo_infos.begin(); it != fo_infos.end(); ++it )
    {
      if ( it->relative_depth == rd )
      {
        it->extrefs.push_back( idx );
        ++it->num_edges;
        return;
      }
      else if ( it->relative_depth > rd )
      {
        fo_infos.insert( it, { rd, {}, {idx}, 1u } );
        return;
      }
    }
    fo_infos.push_back( { rd, {}, {idx}, 1u } );
  }

  template<bool verify = false>
  bool count_edges( node const& n )
  {
    auto& fo_infos = _fanouts[n];

    if ( fo_infos.size() == 0u || ( fo_infos.size() == 1u && fo_infos.front().num_edges == 1u ) )
    {
      return true;
    }

    if ( _ntk.is_pi( n ) && _ps.assume.ci_capacity > 1 )
    {
      if ( fo_infos.front().relative_depth > 1u )
        fo_infos.push_front( { 1u, {}, {}, 0u } );
    }
    else
    {
      assert( fo_infos.front().relative_depth > 1u );
      fo_infos.push_front( { 1u, {}, {}, 0u } );
    }

    auto it = fo_infos.end();
    --it;
    uint32_t splitters;
    while ( it != fo_infos.begin() )
    {
      splitters = num_splitters( it->num_edges );
      auto rd = it->relative_depth;
      --it;
      if ( it->relative_depth < rd - 1 && splitters > 1 )
      {
        it = fo_infos.insert( ++it, { rd - 1, {}, {}, splitters } );
      }
      else
      {
        it->num_edges += splitters;
      }
    }

    assert( fo_infos.front().relative_depth == 1u );
    if constexpr ( verify )
    {
      return _ntk.is_pi( n ) ? fo_infos.front().num_edges <= _ps.assume.ci_capacity : fo_infos.front().num_edges == 1u;
    }
    else
    {
      assert( _ntk.is_pi( n ) ? fo_infos.front().num_edges <= _ps.assume.ci_capacity : fo_infos.front().num_edges == 1u );
      return true;
    }
  }

  /* Return the number of splitters needed in one level lower */
  uint32_t num_splitters( uint32_t const& num_fanouts ) const
  {
    return std::ceil( float( num_fanouts ) / float( _ps.assume.splitter_capacity ) );
  }
#pragma endregion

#pragma region Initial level assignment
public:
  void set_scheduling_policy( buffer_insertion_params::scheduling_policy p )
  {
    _ps.scheduling = p;
  }

  /*! \brief Obtain the initial level assignment using the specified scheduling policy */
  void schedule()
  {
    if ( _ps.scheduling == buffer_insertion_params::provided )
    {
      _ntk.foreach_po( [&]( auto const& f, auto i ) {
        assert( _po_levels[i] > _levels[f] );
        _depth = std::max( _depth, _po_levels[i] - 1 );
      } );
      assert( _depth % _ps.assume.num_phases == 0 );
      return;
    }

    if ( _ps.scheduling == buffer_insertion_params::better_depth || _ps.scheduling == buffer_insertion_params::ASAP_depth || _ps.scheduling == buffer_insertion_params::ALAP_depth )
    {
      fanout_view<Ntk> f_ntk{ _ntk };
      /* Optimum-depth ALAP scheduling */
      ALAP_depth( f_ntk );
      count_buffers();
      auto const num_buf_ALAP_depth = num_buffers();

      if ( _ps.scheduling == buffer_insertion_params::ALAP_depth )
        return;

      /* Optimum-depth ALAP scheduling: no balanced trees */
      ASAP_depth( f_ntk, false );
      count_buffers();
      auto const num_buf_ASAP_depth = num_buffers();

      if ( _ps.scheduling == buffer_insertion_params::ASAP_depth )
        return;

      /* Revert to optimum-depth ALAP scheduling if better */
      if ( num_buf_ALAP_depth < num_buf_ASAP_depth )
      {
        ALAP_depth( f_ntk );
      }
      return;
    }

    ASAP();
    if ( _ps.scheduling == buffer_insertion_params::ALAP )
    {
      ALAP();
    }
    else if ( _ps.scheduling == buffer_insertion_params::better )
    {
      count_buffers();
      auto num_buf_ASAP = num_buffers();
      ALAP();
      count_buffers();
      if ( num_buffers() > num_buf_ASAP )
      {
        ASAP();
      }
    }
  }

  /*! \brief ASAP scheduling */
  void ASAP()
  {
    _depth = 0;
    _levels.reset( 0 );
    _ntk.incr_trav_id();

    _ntk.foreach_po( [&]( auto const& f, auto i ) {
      auto const no = _ntk.get_node( f );
      _po_levels[i] = compute_levels_ASAP( no ) + num_splitter_levels( no ) + 1;
      if ( ( _po_levels[i] - 1 ) % _ps.assume.num_phases != 0 ) // phase alignment
      {
        _po_levels[i] += _ps.assume.num_phases - ( ( _po_levels[i] - 1 ) % _ps.assume.num_phases );
      }
      _depth = std::max( _depth, _po_levels[i] - 1 );
    } );
    assert( _depth % _ps.assume.num_phases == 0 );

    if ( _ps.assume.balance_cios )
    {
      _ntk.foreach_po( [&]( auto const& f, auto i ) {
        (void)f;
        _po_levels[i] = _depth + 1;
      } );
    }

    /* dangling PIs */
    _ntk.foreach_pi( [&]( auto const& n ){
      if ( _ntk.visited( n ) != _ntk.trav_id() )
        _levels[n] = _ps.assume.ci_phases[0];
    } );

    _outdated = true;
    _is_scheduled_ASAP = true;
  }

  /*! \brief ASAP optimal-depth scheduling
   *
   * ASAP_depth should follow right after ALAP_depth (i.e., initialization).
   *
   * \param try_regular tries to insert balanced trees when sufficient slack.
   */
  void ASAP_depth( fanout_view<Ntk> const& f_ntk, bool try_regular )
  {
    node_map<uint32_t, Ntk> mobility( _ntk, std::numeric_limits<uint32_t>::max() );

    if ( !_ps.assume.balance_cios )
    {
      _ntk.foreach_po( [&]( auto const& f, auto i ) {
        (void)f;
        _po_levels[i] = 0;
      } );
    }

    _ntk.foreach_pi( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) )
      {
        mobility[n] = _levels[n] - _ps.assume.ci_phases[0];
        compute_mobility_ASAP( f_ntk, n, mobility, try_regular );
      }
    } );

    _ntk.foreach_gate( [&]( auto const& n ) {
      compute_mobility_ASAP( f_ntk, n, mobility, try_regular );
    } );

    if ( !_ps.assume.balance_cios )
    {
      _ntk.foreach_po( [&]( auto const& f, auto i ) {
        if ( _po_levels[i] == 0 )
        {
          assert( _ntk.is_constant( _ntk.get_node( f ) ) );
          _po_levels[i] = 1;
        }
        else if ( ( _po_levels[i] - 1 ) % _ps.assume.num_phases != 0 ) // phase alignment
        {
          _po_levels[i] += _ps.assume.num_phases - ( ( _po_levels[i] - 1 ) % _ps.assume.num_phases );
        }
        _depth = std::max( _depth, _po_levels[i] - 1 );
      } );
      assert( _depth % _ps.assume.num_phases == 0 );
    }

    _outdated = true;
    _is_scheduled_ASAP = true;
  }

  /*! \brief ALAP scheduling.
   *
   * ALAP should follow right after ASAP (i.e., initialization) without other optimization in between.
   */
  void ALAP()
  {
    assert( _depth % _ps.assume.num_phases == 0 );
    _levels.reset( 0 );
    _ntk.incr_trav_id();

    _ntk.foreach_po( [&]( auto const& f, auto i ) {
      _po_levels[i] = _depth + 1;
      const auto n = _ntk.get_node( f );

      if ( !_ntk.is_constant( n ) && _ntk.visited( n ) != _ntk.trav_id() )
      {
        _levels[n] = _depth - num_splitter_levels( n );
        compute_levels_ALAP( n );
      }
    } );

    /* dangling PIs */
    _ntk.foreach_pi( [&]( auto const& n ){
      if ( _ntk.visited( n ) != _ntk.trav_id() )
        _levels[n] = _ps.assume.ci_phases[0];
    } );

    _outdated = true;
    _is_scheduled_ASAP = false;
  }

  /*! \brief ALAP depth-optimal sheduling */
  void ALAP_depth( fanout_view<Ntk> const& f_ntk )
  {
    _levels.reset( 0 );
    topo_view<Ntk> topo_ntk{ _ntk };

    /* compute ALAP */
    _depth = std::numeric_limits<uint32_t>::max() - 1;
    uint32_t min_level = std::numeric_limits<uint32_t>::max() - 1;
    topo_ntk.foreach_node_reverse( [&]( auto const& n ) {
      if ( !_ntk.is_constant( n ) && _ntk.fanout_size( n ) > 0 )
      {
        compute_levels_ALAP_depth( f_ntk, n );
        min_level = std::min( min_level, _levels[n] );
      }
    } );

    /* move everything down by `delta` */
    uint32_t delta = min_level;
    /* phase alignment for PO: depth % num_phases = 0 */
    if ( ( _depth - delta ) % _ps.assume.num_phases != 0 )
    {
      delta -= _ps.assume.num_phases - ( ( _depth - delta ) % _ps.assume.num_phases );
    }

    /* level of the lowest PI >= ci_phases[0] */
    while ( min_level - delta < _ps.assume.ci_phases[0] )
    {
      delta -= _ps.assume.num_phases;
    }
    /* move PIs down to an acceptable level */
    if ( _ps.assume.balance_cios )
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        if ( _ntk.fanout_size( n ) == 0 )
        {
          _levels[n] = _ps.assume.ci_phases[0];
        }
        else if ( !_ntk.is_constant( n ) )
        {
          _levels[n] = _levels[n] - delta;
          for ( auto rit = _ps.assume.ci_phases.rbegin(); rit != _ps.assume.ci_phases.rend(); ++rit )
          {
            if ( *rit <= _levels[n] )
            {
              _levels[n] = *rit;
              return;
            }
          }
          assert( false );
        }
      } );
    }
    else
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        if ( _ntk.fanout_size( n ) == 0 )
        {
          _levels[n] = _ps.assume.ci_phases[0];
        }
        else if ( !_ntk.is_constant( n ) )
        {
          _levels[n] = _levels[n] - delta;
          while ( !is_acceptable_ci_lvl( _levels[n] ) )
          {
            assert( _levels[n] > 0 );
            --_levels[n];
          }
        }
      } );
    }

    _ntk.foreach_gate( [&]( auto const& n ) {
      _levels[n] = _levels[n] - delta;
    } );
    _depth -= delta;
    assert( _depth % _ps.assume.num_phases == 0 );
    if ( _ps.assume.balance_cios )
    {
      _ntk.foreach_po( [&]( auto const& f, auto i ) {
        (void)f;
        _po_levels[i] = _depth + 1;
      } );
    }
    else
    {
      _ntk.foreach_po( [&]( auto const& f, auto i ) {
        if ( _ntk.is_constant( _ntk.get_node( f ) ) )
          _po_levels[i] = 1;
        else
        {
          _po_levels[i] = _levels[f] + num_splitter_levels( _ntk.get_node( f ) );
          if ( _po_levels[i] % _ps.assume.num_phases > 0 )
            _po_levels[i] += _ps.assume.num_phases - ( _po_levels[i] % _ps.assume.num_phases );
          ++_po_levels[i];
        }
      } );
    }

    _outdated = true;
    _is_scheduled_ASAP = false;
  }

private:
  uint32_t compute_levels_ASAP( node const& n )
  {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
    {
      return _levels[n];
    }
    _ntk.set_visited( n, _ntk.trav_id() );

    if ( _ntk.is_constant( n ) )
    {
      return _levels[n] = 0;
    }
    else if ( _ntk.is_pi( n ) )
    {
      return _levels[n] = _ps.assume.ci_phases[0];
    }

    uint32_t level{ 0 };
    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        auto fi_level = compute_levels_ASAP( ni ) + num_splitter_levels( ni );
        level = std::max( level, fi_level );
      }
    } );

    return _levels[n] = level + 1;
  }

  bool is_acceptable_ci_lvl( uint32_t lvl ) const
  {
    if ( _ps.assume.balance_cios )
    {
      for ( auto const& p : _ps.assume.ci_phases )
      {
        if ( lvl == p )
          return true;
      }
      return false;
    }
    else
    {
      for ( auto const& p : _ps.assume.ci_phases )
      {
        // for example, if num_phases = 4, ci_phases = {5},
        // then lvl = 1 will not be acceptable, but lvl = 5 or lvl = 9 will
        if ( lvl % _ps.assume.num_phases == p % _ps.assume.num_phases && lvl >= p )
          return true;
      }
      return false;
    }
  }

  void compute_levels_ALAP( node const& n )
  {
    _ntk.set_visited( n, _ntk.trav_id() );

    if ( _ntk.is_pi( n ) )
    {
      if ( _ps.assume.balance_cios )
      {
        for ( auto rit = _ps.assume.ci_phases.rbegin(); rit != _ps.assume.ci_phases.rend(); ++rit )
        {
          if ( *rit <= _levels[n] )
          {
            _levels[n] = *rit;
            return;
          }
        }
        assert( false );
      }
      else
      {
        while ( !is_acceptable_ci_lvl( _levels[n] ) )
        {
          assert( _levels[n] > 0 );
          --_levels[n];
        }
      }
      return;
    }

    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        assert( _levels[n] > num_splitter_levels( ni ) );
        auto fi_level = _levels[n] - num_splitter_levels( ni ) - 1;
        if ( _ntk.visited( ni ) != _ntk.trav_id() || _levels[ni] > fi_level )
        {
          _levels[ni] = fi_level;
          compute_levels_ALAP( ni );
        }
      }
    } );
  }

  void compute_levels_ALAP_depth( fanout_view<Ntk> const& ntk, node const& n )
  {
    std::vector<uint32_t> level_assignment;
    level_assignment.reserve( ntk.fanout_size( n ) );

    /* if node is a PO, add levels */
    for ( auto i = ntk.fanout( n ).size(); i < ntk.fanout_size( n ); ++i )
      level_assignment.push_back( _depth + 1 );

    /* get fanout levels */
    ntk.foreach_fanout( n, [&]( auto const& f ) {
      level_assignment.push_back( _levels[f] );
    } );

    /* sort by descending order of levels */
    std::stable_sort( level_assignment.begin(), level_assignment.end(), std::greater<uint32_t>() );

    /* simulate splitter tree reconstruction */
    uint32_t nodes_in_level = 0;
    uint32_t last_level = level_assignment.front();
    for ( int const l : level_assignment )
    {
      if ( l == last_level )
      {
        ++nodes_in_level;
      }
      else
      {
        /* update splitters */
        for ( auto i = 0; ( i < last_level - l ) && ( nodes_in_level != 1 ); ++i )
          nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );

        ++nodes_in_level;
        last_level = l;
      }
    }

    /* search for a feasible level for node n */
    --last_level;
    if ( _ntk.is_pi( n ) )
    {
      while ( nodes_in_level > _ps.assume.ci_capacity )
      {
        nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
        --last_level;
      }
    }
    else
    {
      while ( nodes_in_level > 1 )
      {
        nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
        --last_level;
      }
    }

    _levels[n] = last_level;
  }

  void compute_mobility_ASAP( fanout_view<Ntk> const& ntk, node const& n, node_map<uint32_t, Ntk>& mobility, bool try_regular )
  {
    assert( mobility[n] <= _levels[n] );
    /* commit ASAP scheduling */
    uint32_t level_n = _levels[n] - mobility[n];
    _levels[n] = level_n;

    /* try to fit a balanced tree */
    if ( try_regular )
    {
      uint32_t fo_level = num_splitter_levels( n );
      bool valid = true;
      ntk.foreach_fanout( n, [&]( auto const& f ) {
        if ( level_n + fo_level + 1 > _levels[f] )
          valid = false;
        return valid;
      } );

      if ( valid )
      {
        ntk.foreach_fanout( n, [&]( auto const& f ) {
          mobility[f] = std::min( mobility[f], _levels[f] - level_n - fo_level - 1 );
        } );
        return;
      }
    }

    /* keep current splitter structure, selecting the mobility based on the buffers */
    std::vector<std::array<uint32_t, 3>> level_assignment;
    level_assignment.reserve( _ntk.fanout_size( n ) );

    /* if node is a PO, add levels */
    if ( ntk.fanout( n ).size() < ntk.fanout_size( n ) )
    {
      ntk.foreach_po( [&]( auto const& f, auto i ){
        if ( ntk.get_node( f ) == n )
          level_assignment.push_back( { i, _depth + 1, 0 } );
      } );
      assert( level_assignment.size() == ntk.fanout_size( n ) - ntk.fanout( n ).size() );
    }

    /* get fanout levels */
    ntk.foreach_fanout( n, [&]( auto const& f ) {
      level_assignment.push_back( { ntk.node_to_index( f ), _levels[f], 0 } );
    } );

    /* dangling PI */
    if ( level_assignment.empty() )
    {
      return;
    }

    /* sort by descending order of levels */
    std::stable_sort( level_assignment.begin(), level_assignment.end(), []( auto const& a, auto const& b ) {
      return a[1] > b[1];
    } );

    /* simulate splitter tree reconstruction */
    uint32_t nodes_in_level = 0;
    uint32_t nodes_in_last_level = _ps.assume.splitter_capacity;
    uint32_t last_level = level_assignment.front()[1];
    for ( auto i = 0; i < level_assignment.size(); ++i )
    {
      uint32_t l = level_assignment[i][1];
      if ( l == last_level )
      {
        ++nodes_in_level;
      }
      else
      {
        /* update splitters */
        uint32_t mobility_update = 0;
        for ( auto j = 0; j < last_level - l; ++j )
        {
          if ( nodes_in_level == 1 )
          {
            ++mobility_update;
          }
          nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
        }

        if ( mobility_update )
        {
          for ( auto j = 0; j < i; ++j )
            level_assignment[j][2] += mobility_update;
        }

        ++nodes_in_level;
        last_level = l;
      }
    }

    /* search a feasible level for node n */
    uint32_t mobility_update = 0;
    for ( auto i = level_n + 1; i < last_level; ++i )
    {
      if ( nodes_in_level == 1 || ( _ntk.is_pi( n ) && nodes_in_level <= _ps.assume.ci_capacity ) )
        ++mobility_update;
      nodes_in_level = std::ceil( float( nodes_in_level ) / float( _ps.assume.splitter_capacity ) );
    }

    /* update mobilities */
    for ( auto const& v : level_assignment )
    {
      if ( v[1] != _depth + 1 )
      {
        mobility[v[0]] = std::min( mobility[v[0]], v[2] + mobility_update );
      }
    }

    /* update po_level, if possible */
    if ( !_ps.assume.balance_cios )
    {
      for ( auto const& v : level_assignment )
      {
        if ( v[1] == _depth + 1 )
        {
          _po_levels[v[0]] = std::max( _po_levels[v[0]], _depth + 1 - v[2] - mobility_update );
        }
        else
        {
          break;
        }
      }
    }
  }
#pragma endregion

#pragma region Dump buffered network
public:
  /*! \brief Dump buffered network
   *
   * After level assignment, (optimization), and buffer counting, this method
   * can be called to dump the resulting buffered network.
   */
  template<class BufNtk>
  void dump_buffered_network( BufNtk& bufntk ) const
  {
    static_assert( is_buffered_network_type_v<BufNtk>, "BufNtk is not a buffered network type" );
    static_assert( has_is_buf_v<BufNtk>, "BufNtk does not implement the is_buf method" );
    static_assert( has_create_buf_v<BufNtk>, "BufNtk does not implement the create_buf method" );
    assert( !_outdated && "Please call `count_buffers()` first." );

    using buf_signal = typename BufNtk::signal;
    using fanout_tree = std::vector<std::list<buf_signal>>;

    node_map<buf_signal, Ntk> node_to_signal( _ntk );
    node_map<fanout_tree, Ntk> buffers( _ntk );

    /* constants */
    node_to_signal[_ntk.get_constant( false )] = bufntk.get_constant( false );
    buffers[_ntk.get_constant( false )].emplace_back( 1, bufntk.get_constant( false ) );
    if ( _ntk.get_node( _ntk.get_constant( false ) ) != _ntk.get_node( _ntk.get_constant( true ) ) )
    {
      node_to_signal[_ntk.get_constant( true )] = bufntk.get_constant( true );
      buffers[_ntk.get_constant( true )].emplace_back( 1, bufntk.get_constant( true ) );
    }

    /* PIs */
    _ntk.foreach_pi( [&]( auto const& n ) {
      node_to_signal[n] = bufntk.create_pi();
      create_buffer_chain( bufntk, buffers, n, node_to_signal[n] );
    } );

    /* gates: assume topological order */
    _ntk.foreach_gate( [&]( auto const& n ) {
      std::vector<buf_signal> children;
      _ntk.foreach_fanin( n, [&]( auto const& fi ) {
        buf_signal s;
        if ( _ntk.is_constant( _ntk.get_node( fi ) ) )
          s = node_to_signal[fi];
        else
          s = get_buffer_at_relative_depth( bufntk, buffers[fi], _levels[n] - _levels[fi] - 1 );
        children.push_back( _ntk.is_complemented( fi ) ? !s : s );
      } );
      node_to_signal[n] = bufntk.clone_node( _ntk, n, children );
      create_buffer_chain( bufntk, buffers, n, node_to_signal[n] );
    } );

    /* POs */
    _ntk.foreach_po( [&]( auto const& f, auto i ) {
      buf_signal s;
      if ( _ntk.is_constant( _ntk.get_node( f ) ) )
        s = node_to_signal[f];
      else
        s = get_buffer_at_relative_depth( bufntk, buffers[f], _po_levels[i] - _levels[f] - 1 );
      assert( _ps.assume.ignore_co_negation );
      bufntk.create_po( _ntk.is_complemented( f ) ? !s : s );
    } );

    assert( bufntk.size() - bufntk.num_pis() - bufntk.num_gates() - 1 == num_buffers() );
  }

private:
  template<class BufNtk, typename Buffers>
  void create_buffer_chain( BufNtk& bufntk, Buffers& buffers, node const& n, typename BufNtk::signal const& s ) const
  {
    if ( _ntk.fanout_size( n ) == 0 )
      return; /* dangling */

    assert( _fanouts[n].size() > 0u );
    buffers[n].resize( _fanouts[n].back().relative_depth );
    auto& fot = buffers[n];
    fot[0].push_back( s );
    for ( auto i = 1u; i < fot.size(); ++i )
    {
      fot[i].push_back( bufntk.create_buf( fot[i-1].back() ) );
    }
  }

  template<class BufNtk, typename FOT>
  typename BufNtk::signal get_buffer_at_relative_depth( BufNtk& bufntk, FOT& fot, uint32_t rd ) const
  {
    typename BufNtk::signal b = fot[rd].back();
    if ( rd == 0 && bufntk.is_pi( bufntk.get_node( b ) ) )
    {
      assert( bufntk.fanout_size( bufntk.get_node( b ) ) < _ps.assume.ci_capacity );
      return b;
    }
    if ( bufntk.fanout_size( bufntk.get_node( b ) ) == _ps.assume.splitter_capacity )
    {
      assert( rd > 0 );
      typename BufNtk::signal b_lower = get_buffer_at_relative_depth( bufntk, fot, rd - 1 );
      b = bufntk.create_buf( b_lower );
      fot[rd].push_back( b );
    }
    return b;
  }
#pragma endregion

#pragma region Post-dump optimization
public:
template<class BufNtk>
uint32_t remove_buffer_chains( BufNtk& ntk ) const
{
  static_assert( is_buffered_network_type_v<BufNtk>, "BufNtk is not a buffered network" );

  uint32_t max_chain = 0;
  ntk.incr_trav_id();
  ntk.foreach_po( [&]( auto f ){
    remove_buffer_chains_rec( ntk, ntk.get_node( f ), 0, max_chain );
  } );
  return max_chain;
}

private:
template<class BufNtk>
std::pair<uint32_t, typename BufNtk::node> remove_buffer_chains_rec( BufNtk& ntk, typename BufNtk::node n, typename BufNtk::node parent, uint32_t& max_chain ) const
{
  if ( ntk.visited( n ) == ntk.trav_id() )
    return std::make_pair( 0, n );
  ntk.set_visited( n, ntk.trav_id() );
  if ( ntk.is_pi( n ) )
    return std::make_pair( 0, n );

  if ( ntk.is_buf( n ) )
  {
    // splitter
    if ( ntk.fanout_size( n ) > 1 )
    {
      ntk.foreach_fanin( n, [&]( auto f ){
        remove_buffer_chains_rec( ntk, ntk.get_node( f ), n, max_chain );
      } );
      return std::make_pair( 0, n );
    }

    // single-output buffer: can be part of a chain to be removed
    std::pair<uint32_t, typename BufNtk::node> ret;
    ntk.foreach_fanin( n, [&]( auto f ){
      auto [count, origin] = remove_buffer_chains_rec( ntk, ntk.get_node( f ), n, max_chain );
      if ( count % _ps.assume.num_phases == _ps.assume.num_phases - 1 )
      {
        // TODO: take care of complementation
        if ( parent != 0 )
        {
          ntk.replace_in_node( parent, n, ntk.make_signal( origin ) );
          ntk.take_out_node( n );
        }
        else
        {
          ntk.replace_in_outputs( n, ntk.make_signal( origin ) );
          ntk.take_out_node( n );
        }
        max_chain = std::max( count + 1, max_chain );
      }
      ret = std::make_pair( count + 1, origin );
    } );
    return ret;
  }

  // gate
  ntk.foreach_fanin( n, [&]( auto f ){
    remove_buffer_chains_rec( ntk, ntk.get_node( f ), n, max_chain );
  } );
  return std::make_pair( 0, n );
}
#pragma endregion

public:
  /*! \brief Optimize with chunked movement using the specified optimization policy. */
  void optimize()
  {
    if ( _ps.optimization_effort == buffer_insertion_params::none )
    {
      return;
    }
    //else if ( _ps.optimization_effort == buffer_insertion_params::optimal )
    //{
    //  if constexpr ( has_get_network_name_v<Ntk> )
    //    optimize_with_smt( _ntk.get_network_name() );
    //  else
    //    optimize_with_smt( "" );
    //  return;
    //}

    if ( _outdated )
    {
      update_fanout_info();
    }

    bool updated;
    do
    {
      updated = find_and_move_chunks();
    } while ( updated && _ps.optimization_effort == buffer_insertion_params::until_sat );
    single_gate_movement();
  }

#pragma region Chunked movement
private:
  struct io_interface
  {
    node c; // chunk node
    node o; // outside node
  };

  struct po_interface
  {
    node c; // chunk node
    uint32_t o; // PO index
  };

  struct chunk
  {
    uint32_t id;
    std::vector<node> members{};
    std::vector<io_interface> input_interfaces{};
    std::vector<io_interface> output_interfaces{};
    std::vector<po_interface> po_interfaces{};
    int32_t slack{ std::numeric_limits<int32_t>::max() };
    int32_t benefits{ 0 };
  };

  bool is_ignored( node const& n ) const
  {
    return _ntk.is_constant( n );
  }

  bool is_fixed( node const& n ) const
  {
    return _ps.assume.balance_cios && _ps.assume.ci_phases.size() == 1 && _ntk.is_pi( n );
  }

  bool find_and_move_chunks()
  {
    bool updated = false;
    count_buffers();
    uint32_t num_buffers_before = num_buffers();
    _start_id = _ntk.trav_id();

    _ntk.foreach_node( [&]( auto const& n ) {
      if ( is_ignored( n ) || is_fixed( n ) || _ntk.visited( n ) > _start_id /* belongs to a chunk */ )
      {
        return true;
      }

      _ntk.incr_trav_id();
      chunk c{ _ntk.trav_id() };
      recruit( n, c );
      if ( c.members.size() > _ps.max_chunk_size )
      {
        return true; /* skip */
      }
      cleanup_interfaces( c );

      auto moved = analyze_chunk_down( c );
      if ( !moved )
        moved = analyze_chunk_up( c );
      updated |= moved;
      return true;
    } );

    count_buffers();
    assert( num_buffers() <= num_buffers_before );
    return updated && num_buffers() < num_buffers_before;
  }

  void single_gate_movement()
  {
    _ntk.foreach_node( [&]( auto const& n ) {
      if ( is_ignored( n ) || is_fixed( n ) )
        return;

      _ntk.incr_trav_id();
      chunk c{ _ntk.trav_id() };
      c.members.emplace_back( n );
      _ntk.foreach_fanin( n, [&]( auto const& fi ) {
        auto const ni = _ntk.get_node( fi );
        if ( !is_ignored( ni )  )
          c.input_interfaces.push_back( { n, ni } );
      } );
      auto const& fanout_info = _fanouts[n];
      for ( auto it = fanout_info.begin(); it != fanout_info.end(); ++it )
      {
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
          c.output_interfaces.push_back( { n, *it2 } );
        for ( auto it2 = it->extrefs.begin(); it2 != it->extrefs.end(); ++it2 )
          c.po_interfaces.push_back( { n, *it2 } );
      }

      if ( !analyze_chunk_down( c ) )
        analyze_chunk_up( c );
    } );
  }

  void recruit( node const& n, chunk& c )
  {
    if ( _ntk.visited( n ) == c.id )
      return;

    assert( _ntk.visited( n ) <= _start_id );
    assert( !is_fixed( n ) );
    assert( !is_ignored( n ) );

    _ntk.set_visited( n, c.id );
    c.members.emplace_back( n );
    recruit_fanins( n, c );
    recruit_fanouts( n, c );
  }

  void recruit_fanins( node const& n, chunk& c )
  {
    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !is_ignored( ni ) && _ntk.visited( ni ) != c.id )
      {
        if ( is_fixed( ni ) )
          c.input_interfaces.push_back( { n, ni } );
        else if ( are_close( ni, n ) )
          recruit( ni, c );
        else
          c.input_interfaces.push_back( { n, ni } );
      }
    } );
  }

  void recruit_fanouts( node const& n, chunk& c )
  {
    auto const& fanout_info = _fanouts[n];
    if ( fanout_info.size() == 0 ) /* dangling */
      return;

    auto it = fanout_info.begin();
    if ( _ntk.fanout_size( n ) == 1 ) /* single fanout */
    {
      assert( fanout_info.size() == 1 );
      if ( it->fanouts.size() == 1 ) /* single gate fanout */
      {
        if ( it->relative_depth == 1 )
          recruit( it->fanouts.front(), c );
        else
          c.output_interfaces.push_back( { n, it->fanouts.front() } );
      }
      else /* single PO fanout */
      {
        assert( it->extrefs.size() == 1 );
        c.po_interfaces.push_back( { n, it->extrefs.front() } );
      }
      return;
    }

    for ( ; it != fanout_info.end(); ++it )
    {
      for ( auto it2 = it->extrefs.begin(); it2 != it->extrefs.end(); ++it2 )
        c.po_interfaces.push_back( { n, *it2 } );
    }
    it = fanout_info.begin();

    if ( _ps.assume.ci_capacity > 1 && _ntk.is_pi( n ) )
    {
      if ( it->relative_depth == 1 )
      {
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
          recruit( *it2, c );
        it++;
      }
      if ( it->relative_depth == 2 && fanout_info.front().num_edges == _ps.assume.ci_capacity )
      {
        assert( fanout_info.front().relative_depth == 1 );
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
          recruit( *it2, c );
        it++;
      }
      for ( ; it != fanout_info.end(); ++it )
      {
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
        {
          if ( _ntk.visited( *it2 ) != c.id )
            c.output_interfaces.push_back( { n, *it2 } );
        }
      }
      return;
    }

    for ( ; it != fanout_info.end(); ++it )
    {
      for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
      {
        if ( it->relative_depth == 2 )
          recruit( *it2, c );
        else if ( _ntk.visited( *it2 ) != c.id )
          c.output_interfaces.push_back( { n, *it2 } );
      }
    }
  }

  bool are_close( node const& ni, node const& n )
  {
    auto const& fanout_info = _fanouts[ni];

    if ( _ps.assume.ci_capacity > 1 && _ntk.is_pi( ni ) )
    {
      auto const& front_fanouts = fanout_info.front().fanouts;
      if ( fanout_info.front().relative_depth == 1 )
      {
        if ( std::find( front_fanouts.begin(), front_fanouts.end(), n ) != front_fanouts.end() )
          return true;
        if ( fanout_info.front().num_edges < _ps.assume.ci_capacity )
          return false;
      }
      else if ( _ntk.fanout_size( ni ) <= _ps.assume.ci_capacity )
        return false;
      assert( fanout_info.size() > 1 );
    }

    if ( fanout_info.size() == 1 && fanout_info.front().relative_depth == 1 )
    {
      assert( fanout_info.front().fanouts.front() == n );
      return true;
    }
    if ( fanout_info.size() > 1 )
    {
      auto it = fanout_info.begin();
      it++;
      if ( it->relative_depth > 2 )
        return false;
      for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
      {
        if ( *it2 == n )
          return true;
      }
    }
    return false;
  }

  void cleanup_interfaces( chunk& c )
  {
    for ( int i = 0; i < c.input_interfaces.size(); ++i )
    {
      if ( _ntk.visited( c.input_interfaces[i].o ) == c.id )
      {
        c.input_interfaces.erase( c.input_interfaces.begin() + i );
        --i;
      }
    }
    for ( int i = 0; i < c.output_interfaces.size(); ++i )
    {
      if ( _ntk.visited( c.output_interfaces[i].o ) == c.id )
      {
        c.output_interfaces.erase( c.output_interfaces.begin() + i );
        --i;
      }
    }
  }

  bool analyze_chunk_down( chunk c )
  {
    count_buffers();
    auto buffers_before = num_buffers();

    std::set<node> marked_oi;
    for ( auto oi : c.output_interfaces )
    {
      if ( marked_oi.find( oi.c ) == marked_oi.end() )
      {
        marked_oi.insert( oi.c );
        --c.benefits;
      }
    }

    for ( auto ii : c.input_interfaces )
    {
      auto const rd = _levels[ii.c] - _levels[ii.o];
      auto const lowest = lowest_spot( ii.o );
      if ( rd <= lowest )
      {
        c.slack = 0;
        break;
      }
      c.slack = std::min( c.slack, int32_t( rd - lowest ) );
      pseudo_move( ii.o, ii.c, rd, lowest );
      if ( _fanouts[ii.o].back().relative_depth == rd && _fanouts[ii.o].back().num_edges == 0 ) // `ii.c` is the last highest fanout of `ii.o`
      {
        ++c.benefits;
      }
    }

    if ( c.po_interfaces.size() > 0 )
    {
      if ( !_ps.assume.balance_cios && c.slack >= _ps.assume.num_phases )
      {
        c.slack -= c.slack % _ps.assume.num_phases;
      }
      else
      {
        for ( auto poi : c.po_interfaces )
        {
          if ( marked_oi.find( poi.c ) == marked_oi.end() )
            --c.benefits;
        }
      }
    }

    std::vector<node> pi_members;
    for ( auto m : c.members )
    {
      if ( _ntk.is_pi( m ) )
      {
        pi_members.emplace_back( m );
        c.slack = std::min( c.slack, int32_t( _levels[m] ) );
      }
    }
    if ( pi_members.size() > 0 )
    {
      while ( c.slack > 0 )
      {
        bool ok = true;
        for ( auto m : pi_members )
        {
          if ( _levels[m] < c.slack || !is_acceptable_ci_lvl( _levels[m] - c.slack ) )
          {
            ok = false;
            break;
          }
        }
        if ( !ok )
          --c.slack;
        else
          break;
      }
    }

    if ( c.benefits > 0 && c.slack > 0 )
    {
      bool legal = true;

      for ( auto m : c.members )
        _levels[m] -= c.slack;
      if ( !_ps.assume.balance_cios && c.slack >= _ps.assume.num_phases )
      {
        for ( auto poi : c.po_interfaces )
          _po_levels[poi.o] -= c.slack;
      }
      for ( auto m : c.members )
        update_fanout_info( m );
      for ( auto ii : c.input_interfaces )
        legal &= update_fanout_info<true>( ii.o );

      _outdated = true;
      if ( legal )
        count_buffers();
      if ( !legal || num_buffers() >= buffers_before )
      {
        /* UNDO */
        for ( auto m : c.members )
          _levels[m] += c.slack;
        if ( !_ps.assume.balance_cios && c.slack >= _ps.assume.num_phases )
        {
          for ( auto poi : c.po_interfaces )
            _po_levels[poi.o] += c.slack;
        }
        for ( auto m : c.members )
          update_fanout_info( m );
        for ( auto ii : c.input_interfaces )
          update_fanout_info( ii.o );
        _outdated = true;
        return false;
      }

      _start_id = _ntk.trav_id();
      return true;
    }
    else
    {
      /* reset fanout_infos of input_interfaces because num_edges may be modified by pseudo_move */
      for ( auto ii : c.input_interfaces )
        update_fanout_info( ii.o );
      _outdated = true;
      return false;
    }
  }

  /* relative_depth of the lowest available spot in the fanout tree of n */
  uint32_t lowest_spot( node const& n ) const
  {
    auto const& fanout_info = _fanouts[n];
    assert( fanout_info.size() );

    auto it = fanout_info.begin();
    uint32_t rd_prev = 1;
    uint32_t num_splitters_prev = 1;
    if ( _ntk.is_pi( n ) && _ps.assume.ci_capacity > 1 )
    {
      if ( it->num_edges <= _ps.assume.ci_capacity )
        return 1;
      else
        num_splitters_prev = _ps.assume.ci_capacity - it->fanouts.size() - it->extrefs.size();
    }
    else if ( fanout_info.size() == 1 ) // single fanout
    {
      return 1;
    }
    
    ++it; // skip the first splitter at rd=1
    for ( ; it != fanout_info.end(); ++it )
    {
      if ( it->relative_depth > rd_prev + 1 ) // level skip => must not full
      {
        return rd_prev + 1;
      }
      else if ( it->num_edges == _ps.assume.splitter_capacity * num_splitters_prev ) // full layer
      {
        num_splitters_prev = it->num_edges - it->fanouts.size() - it->extrefs.size();
        rd_prev = it->relative_depth;
      }
      else
      {
        return it->relative_depth;
      }
    }
    // all full
    return fanout_info.back().relative_depth + 1;
  }

  /* move `no`, which is a fanout of `n`, from `from_rd` to `to_rd` */
  void pseudo_move( node const& n, node const& no, uint32_t from_rd, uint32_t to_rd )
  {
    assert( from_rd > to_rd );
    auto& fanout_info = _fanouts[n];
    auto it = fanout_info.begin();
    for ( ; it != fanout_info.end(); ++it )
    {
      if ( it->relative_depth == to_rd )
      {
        ++it->num_edges;
        it->fanouts.push_back( no );
        break;
      }
      else if ( it->relative_depth > to_rd )
      {
        fanout_info.insert( it, {to_rd, {no}, {}, 2} );
        break;
      }
    }
    for ( ; it != fanout_info.end(); ++it )
    {
      if ( it->relative_depth == from_rd )
      {
        --it->num_edges;
        for ( auto it2 = it->fanouts.begin(); it2 != it->fanouts.end(); ++it2 )
        {
          if ( *it2 == no )
          {
            it->fanouts.erase( it2 );
            return;
          }
        }
        assert( false );
      }
    }
    assert( false );
  }

  bool analyze_chunk_up( chunk c )
  {
    for ( auto ii : c.input_interfaces )
    {
      if ( _fanouts[ii.o].back().relative_depth == _levels[ii.c] - _levels[ii.o] ) // is highest fanout
        --c.benefits;
    }

    std::set<node> marked_oi;
    for ( auto oi : c.output_interfaces )
    {
      if ( marked_oi.find( oi.c ) == marked_oi.end() )
      {
        marked_oi.insert( oi.c );
        ++c.benefits;
      }
      auto const& fanout_info = _fanouts[oi.c];
      if ( fanout_info.size() == 1 ) /* single fanout */
        c.slack = std::min( c.slack, int32_t( fanout_info.front().relative_depth - 1 ) );
      else
        c.slack = std::min( c.slack, int32_t( _levels[oi.o] - _levels[oi.c] - 2 ) );
    }

    std::vector<uint32_t> po_to_move;
    if ( c.po_interfaces.size() > 0 )
    {
      for ( auto poi : c.po_interfaces )
      {
        if ( _levels[poi.c] + num_splitter_levels( poi.c ) + c.slack >= _po_levels[poi.o] )
        {
          if ( _ps.assume.balance_cios )
            c.slack = std::min( c.slack, int32_t( _po_levels[poi.o] - _levels[poi.c] - num_splitter_levels( poi.c ) - 1 ) );
          else
          {
            c.slack = std::min( c.slack, int32_t( _depth + 1 - _po_levels[poi.o] ) );
            po_to_move.emplace_back( poi.o );
          }
        }
        else
        {
          if ( marked_oi.find( poi.c ) == marked_oi.end() )
            ++c.benefits;
        }
      }
    }

    if ( c.benefits <= 0 || c.slack <= 0 )
      return false;

    std::vector<node> pi_members;
    for ( auto m : c.members )
    {
      if ( _ntk.is_pi( m ) )
        pi_members.emplace_back( m );
    }
    if ( pi_members.size() > 0 )
    {
      while ( c.slack > 0 )
      {
        bool ok = true;
        for ( auto m : pi_members )
        {
          if ( !is_acceptable_ci_lvl( _levels[m] + c.slack ) )
          {
            ok = false;
            break;
          }
        }
        if ( !ok )
          --c.slack;
        else
          break;
      }
    }
    if ( po_to_move.size() > 0 )
      c.slack -= c.slack % _ps.assume.num_phases;

    if ( c.benefits > 0 && c.slack > 0 )
    {
      count_buffers();
      bool legal = true;
      auto buffers_before = num_buffers();

      for ( auto m : c.members )
        _levels[m] += c.slack;
      for ( auto po : po_to_move )
        _po_levels[po] += c.slack;
      for ( auto m : c.members )
        legal &= update_fanout_info<true>( m );
      for ( auto ii : c.input_interfaces )
        legal &= update_fanout_info<true>( ii.o );

      _outdated = true;
      if ( legal )
        count_buffers();
      if ( !legal || num_buffers() >= buffers_before )
      {
        /* UNDO */
        for ( auto m : c.members )
          _levels[m] -= c.slack;
        for ( auto po : po_to_move )
          _po_levels[po] -= c.slack;
        for ( auto m : c.members )
          update_fanout_info( m );
        for ( auto ii : c.input_interfaces )
          update_fanout_info( ii.o );
        _outdated = true;
        return false;
      }

      _start_id = _ntk.trav_id();
      return true;
    }
    else
    {
      return false;
    }
  }
#pragma endregion

#pragma region Global optimal by SMT
private:
//#include "optimal_buffer_insertion.hpp"
#pragma endregion

private:
  struct fanout_information
  {
    uint32_t relative_depth{ 0u };
    std::list<node> fanouts;
    std::list<uint32_t> extrefs; // IDs of POs (as in `_ntk.foreach_po`)
    uint32_t num_edges{ 0u };
  };
  using fanouts_by_level = std::list<fanout_information>;

  Ntk const& _ntk;
  buffer_insertion_params _ps;
  bool _outdated{ true };
  bool _is_scheduled_ASAP{ true };

  /* The following data structures uniquely define the state (i.e. schedule) of the algorithm/flow.
     The rest (`_fanouts` and `_num_buffers`) are computed from these by calling `count_buffers()`. */
  node_map<uint32_t, Ntk> _levels;
  std::vector<uint32_t> _po_levels; // imaginary node, must be at `num_phases * k + 1`
  uint32_t _depth{ 0u };

  /* Guarantees on `_fanouts` (when not `_outdated`):
   * - Sum of `_fanouts[n][l].fanouts.length() + _fanouts[n][l].extrefs.length()` over all `l`s
   *   should be equal to `ntk.fanout_size( n )`.
   * - If having only one fanout: `_fanouts[n].size() == 1`.
   * - If having multiple fanouts: `_fanouts[n]` must have at least two elements,
   *   and the first element must have `relative_depth == 1` and `num_edges == 1`.
   * - If `ci_capacity > 1`, `_fanouts[PI].size()` may be 1.
   */
  node_map<fanouts_by_level, Ntk> _fanouts;
  node_map<uint32_t, Ntk> _num_buffers;

  node_map<std::pair<uint32_t, uint32_t>, Ntk> _timeframes; // only for SMT; the most extreme min/max
  uint32_t _start_id; // for chunked movement
}; /* buffer_insertion */

} // namespace mockturtle