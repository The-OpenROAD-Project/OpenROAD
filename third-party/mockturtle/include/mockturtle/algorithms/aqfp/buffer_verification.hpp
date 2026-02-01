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
  \file buffer_verification.hpp
  \brief Verify buffered networks according to AQFP constraints

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../traits.hpp"
#include "../../utils/node_map.hpp"
#include "../../views/depth_view.hpp"
#include "aqfp_assumptions.hpp"

namespace mockturtle
{

namespace detail
{

template<class Ntk>
void schedule_fanin_cone( Ntk& ntk, typename Ntk::node const& n, uint32_t l )
{
  if ( ntk.visited( n ) == ntk.trav_id() )
    return;
  ntk.set_visited( n, ntk.trav_id() );
  ntk.set_level( n, l );

  ntk.foreach_fanin( n, [&]( auto const& fi ) {
    schedule_fanin_cone( ntk, ntk.get_node( fi ), l - 1 );
  } );
}

template<class Ntk>
uint32_t recompute_level( Ntk& ntk, typename Ntk::node const& n )
{
  if ( ntk.visited( n ) == ntk.trav_id() )
    return ntk.level( n );
  ntk.set_visited( n, ntk.trav_id() );

  uint32_t max_fi_level{ 0u };
  ntk.foreach_fanin( n, [&]( auto const& fi ) {
    max_fi_level = std::max( max_fi_level, recompute_level( ntk, ntk.get_node( fi ) ) );
  } );
  ntk.set_level( n, max_fi_level + 1 );
  return max_fi_level + 1;
}

} // namespace detail

/*! \brief Find a reasonable level assignment for a buffered network given PI levels.
 *
 * \param ntk Buffered network
 * \param pi_levels Levels of PIs
 * \return Level assignment to all nodes
 */
template<class Ntk>
node_map<uint32_t, Ntk> schedule_buffered_network_with_PI_levels( Ntk const& ntk, std::vector<uint32_t> const& pi_levels )
{
  assert( pi_levels.size() == ntk.num_pis() );

  using node = typename Ntk::node;
  node_map<uint32_t, Ntk> levels( ntk );
  depth_view dv{ ntk };

  ntk.incr_trav_id();
  ntk.set_visited( ntk.get_node( ntk.get_constant( false ) ), ntk.trav_id() );
  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    ntk.set_visited( n, ntk.trav_id() );
    dv.set_level( n, pi_levels[i] );
  } );

  ntk.foreach_po( [&]( auto const& f ){
    detail::recompute_level( dv, ntk.get_node( f ) );
  });

  ntk.foreach_node( [&]( auto const& n ) {
    levels[n] = dv.level( n );
  } );

  return levels;
}

/*! \brief Verify a buffered network according to AQFP assumptions with provided level assignment.
 *
 * \param ntk Buffered network
 * \param ps AQFP assumptions
 * \param levels Level assignment for all nodes
 * \return Whether `ntk` is path-balanced and properly-branched
 */
template<class Ntk>
bool verify_aqfp_buffer( Ntk const& ntk, aqfp_assumptions_legacy const& ps, node_map<uint32_t, Ntk> const& levels )
{
  static_assert( is_buffered_network_type_v<Ntk>, "Ntk is not a buffered network" );
  static_assert( has_is_buf_v<Ntk>, "Ntk does not implement the is_buf method" );
  bool legal = true;

  /* fanout branching */
  ntk.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) )
      return true;
    if ( !ps.branch_pis && ntk.is_pi( n ) )
      return true;

    if ( ntk.is_buf( n ) )
      legal &= ( ntk.fanout_size( n ) <= ps.splitter_capacity );
    else /* logic gate */
      legal &= ( ntk.fanout_size( n ) <= 1 );

    return true;
  } );

  /* path balancing */
  ntk.foreach_node( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto ni = ntk.get_node( fi );
      if ( !ntk.is_constant( ni ) && ( ps.balance_pis || !ntk.is_pi( ni ) ) )
        legal &= ( levels[ni] == levels[n] - 1 );
      assert( legal );
    } );
  } );

  if ( ps.balance_pis )
  {
    ntk.foreach_pi( [&]( auto const& n ) {
      legal &= ( levels[n] == 0 );
    } );
  }

  if ( ps.balance_pos )
  {
    uint32_t depth{ 0u };
    ntk.foreach_po( [&]( auto const& f ) {
      auto n = ntk.get_node( f );
      if ( !ntk.is_constant( n ) && ( ps.balance_pis || !ntk.is_pi( n ) ) )
      {
        if ( depth == 0u )
          depth = levels[n];
        else
          legal &= ( levels[n] == depth );
      }
    } );
  }

  return legal;
}

/*! \brief Verify a buffered network according to AQFP assumptions with provided level assignment.
 *
 * \param ntk Buffered network
 * \param ps AQFP assumptions
 * \param levels Level assignment for all nodes
 * \return Whether `ntk` is path-balanced and properly-branched
 */
template<class Ntk>
bool verify_aqfp_buffer( Ntk const& ntk, aqfp_assumptions_realistic const& ps, node_map<uint32_t, Ntk> const& levels )
{
  static_assert( is_buffered_network_type_v<Ntk>, "Ntk is not a buffered network" );
  static_assert( has_is_buf_v<Ntk>, "Ntk does not implement the is_buf method" );
  bool legal = true;

  /* fanout branching */
  ntk.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) )
      return;
    if ( ntk.is_pi( n ) )
    {
      legal &= ( ntk.fanout_size( n ) <= ps.ci_capacity );
    }
    else if ( ntk.is_buf( n ) )
    {
      legal &= ( ntk.fanout_size( n ) <= ps.splitter_capacity );
    }
    else /* logic gate */
    {
      legal &= ( ntk.fanout_size( n ) <= 1 );
    }
    assert( legal );
  } );

  /* path balancing */
  ntk.foreach_node( [&]( auto const& n ) {
    ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto ni = ntk.get_node( fi );
      if ( !ntk.is_constant( ni ) )
        legal &= ( levels[ni] == levels[n] - 1 );
      assert( legal );
    } );
  } );

  if ( ps.balance_cios )
  {
    auto const check_pi_fn = [&]( uint32_t level ){
      for ( auto const& p : ps.ci_phases )
      {
        if ( level == p )
          return true;
      }
      return false;
    };

    ntk.foreach_pi( [&]( auto const& n ) {
      legal &= check_pi_fn( levels[n] );
      assert( legal );
    } );

    uint32_t depth{ 0u };
    ntk.foreach_po( [&]( auto const& f ) {
      auto n = ntk.get_node( f );
      if ( !ntk.is_constant( n ) )
      {
        if ( depth == 0u )
          depth = levels[n];
        else
          legal &= ( levels[n] == depth );
        assert( legal );
      }
    } );
    legal &= ( depth % ps.num_phases == 0 );
    assert( legal );
  }
  else
  {
    auto const check_pi_fn = [&]( uint32_t level ){
      for ( auto const& p : ps.ci_phases )
      {
        if ( level >= p && ( level - p ) % ps.num_phases == 0 )
          return true;
      }
      return false;
    };

    ntk.foreach_pi( [&]( auto const& n ) {
      legal &= check_pi_fn( levels[n] );
      assert( legal );
    } );

    ntk.foreach_po( [&]( auto const& f ) {
      auto n = ntk.get_node( f );
      if ( !ntk.is_constant( n ) )
      {
        legal &= ( levels[n] % ps.num_phases == 0 );
        assert( legal );
      }
    } );
  }

  // TODO: max_phase_skip

  return legal;
}

/*! \brief Verify a buffered network according to AQFP assumptions with provided PI level assignment.
 *
 * \param ntk Buffered network
 * \param ps AQFP assumptions
 * \param pi_levels Levels of PIs
 * \return Whether `ntk` is path-balanced, phase-aligned, and properly-branched
 */
template<class Ntk, typename Asmp = aqfp_assumptions>
bool verify_aqfp_buffer( Ntk const& ntk, Asmp const& ps, std::vector<uint32_t> const& pi_levels )
{
  auto const levels = schedule_buffered_network_with_PI_levels( ntk, pi_levels );
  return verify_aqfp_buffer( ntk, ps, levels );
}

} // namespace mockturtle