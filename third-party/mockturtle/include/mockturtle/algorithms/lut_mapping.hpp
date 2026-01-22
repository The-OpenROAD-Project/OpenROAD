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
  \file lut_mapping.hpp
  \brief LUT mapping

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>

#include <fmt/format.h>

#include "../utils/stopwatch.hpp"
#include "../views/topo_view.hpp"
#include "cut_enumeration.hpp"
#include "cut_enumeration/mf_cut.hpp"

namespace mockturtle
{

/*! \brief Parameters for lut_mapping.
 *
 * The data structure `lut_mapping_params` holds configurable parameters
 * with default arguments for `lut_mapping`.
 */
struct lut_mapping_params
{
  lut_mapping_params()
  {
    cut_enumeration_ps.cut_size = 6;
    cut_enumeration_ps.cut_limit = 8;
  }

  /*! \brief Parameters for cut enumeration
   *
   * The default cut size is 6, the default cut limit is 8.
   */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Number of rounds for area flow optimization.
   *
   * The first round is used for delay optimization.
   */
  uint32_t rounds{ 2u };

  /*! \brief Number of rounds for exact area optimization. */
  uint32_t rounds_ela{ 1u };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

/*! \brief Statistics for lut_mapping.
 *
 * The data structure `lut_mapping_stats` provides data collected by running
 * `lut_mapping`.
 */
struct lut_mapping_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  void report() const
  {
    std::cout << fmt::format( "[i] total time = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

/* function to update all cuts after cut enumeration */
template<typename CutData>
struct lut_mapping_update_cuts
{
  template<typename NetworkCuts, typename Ntk>
  static void apply( NetworkCuts const& cuts, Ntk const& ntk )
  {
    (void)cuts;
    (void)ntk;
  }
};

namespace detail
{

template<class Ntk, bool StoreFunction, typename CutData>
class lut_mapping_impl
{
public:
  using network_cuts_t = network_cuts<Ntk, StoreFunction, CutData>;
  using cut_t = typename network_cuts_t::cut_t;

public:
  lut_mapping_impl( Ntk& ntk, lut_mapping_params const& ps, lut_mapping_stats& st )
      : ntk( ntk ),
        ps( ps ),
        st( st ),
        flow_refs( ntk.size() ),
        map_refs( ntk.size(), 0 ),
        flows( ntk.size() ),
        delays( ntk.size() ),
        cuts( cut_enumeration<Ntk, StoreFunction, CutData>( ntk, ps.cut_enumeration_ps ) )
  {
    lut_mapping_update_cuts<CutData>().apply( cuts, ntk );
  }

  void run()
  {
    stopwatch t( st.time_total );

    /* compute and save topological order */
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );

    init_nodes();
    // print_state();

    set_mapping_refs<false>();
    // print_state();

    while ( iteration < ps.rounds )
    {
      compute_mapping<false>();
    }

    while ( iteration < ps.rounds + ps.rounds_ela )
    {
      compute_mapping<true>();
    }

    derive_mapping();
  }

private:
  uint32_t cut_area( cut_t const& cut ) const
  {
    return static_cast<uint32_t>( cut->data.cost );
  }

  void init_nodes()
  {
    ntk.foreach_node( [this]( auto n, auto ) {
      const auto index = ntk.node_to_index( n );
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
      {
        /* all terminals have flow 1.0 */
        flow_refs[index] = 1.0f;
      }
      else
      {
        flow_refs[index] = static_cast<float>( ntk.fanout_size( n ) );
      }

      flows[index] = cuts.cuts( index )[0]->data.flow;
      delays[index] = cuts.cuts( index )[0]->data.delay;
    } );
  }

  template<bool ELA>
  void compute_mapping()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;
      compute_best_cut<ELA>( ntk.node_to_index( n ) );
    }
    set_mapping_refs<ELA>();
    // print_state();
  }

  template<bool ELA>
  void set_mapping_refs()
  {
    const auto coef = 1.0f / ( 1.0f + ( iteration + 1 ) * ( iteration + 1 ) );

    /* compute current delay and update mapping refs */
    delay = 0;
    ntk.foreach_co( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      delay = std::max( delay, delays[index] );

      if constexpr ( !ELA )
      {
        map_refs[index]++;
      }
    } );

    /* compute current area and update mapping refs */
    area = 0;
    for ( auto it = top_order.rbegin(); it != top_order.rend(); ++it )
    {
      /* skip constants and PIs (TODO: stop earlier) */
      if ( ntk.is_constant( *it ) || ntk.is_ci( *it ) )
        continue;

      const auto index = ntk.node_to_index( *it );
      if ( map_refs[index] == 0 )
        continue;

      if constexpr ( !ELA )
      {
        for ( auto leaf : cuts.cuts( index )[0] )
        {
          map_refs[leaf]++;
        }
      }
      area++;
    }

    /* blend flow references */
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      flow_refs[i] = coef * flow_refs[i] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( map_refs[i] ) );
    }

    ++iteration;
  }

  std::pair<float, uint32_t> cut_flow( cut_t const& cut )
  {
    uint32_t time{ 0u };
    float flow{ 0.0f };

    for ( auto leaf : cut )
    {
      time = std::max( time, delays[leaf] );
      flow += flows[leaf];
    }

    return { flow + cut_area( cut ), time + 1u };
  }

  /* reference cut:
   *   adds cut to current mapping and recursively adds best cuts of leaf
   *   nodes, if they are not part of the current mapping.
   */
  uint32_t cut_ref( cut_t const& cut )
  {
    uint32_t count = cut_area( cut );
    for ( auto leaf : cut )
    {
      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) || ntk.is_ci( ntk.index_to_node( leaf ) ) )
        continue;

      if ( map_refs[leaf]++ == 0 )
      {
        count += cut_ref( cuts.cuts( leaf )[0] );
      }
    }
    return count;
  }

  /* dereference cut:
   *   removes cut from current mapping and recursively removes best cuts of
   *   leaf nodes, if they are part of the current mapping.
   *   (this is the inverse operation to cut_ref)
   */
  uint32_t cut_deref( cut_t const& cut )
  {
    uint32_t count = cut_area( cut );
    for ( auto leaf : cut )
    {
      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) || ntk.is_ci( ntk.index_to_node( leaf ) ) )
        continue;

      if ( --map_refs[leaf] == 0 )
      {
        count += cut_deref( cuts.cuts( leaf ).best() );
      }
    }
    return count;
  }

  /* reference cut (special version):
   *   this special version of cut_ref does two additional things:
   *   1. it stops recursing if it has found `limit` cuts
   *   2. it remembers all cuts for which the reference count increases in the
   *      vector `tmp_area`.
   */
  uint32_t cut_ref_limit_save( cut_t const& cut, uint32_t limit )
  {
    uint32_t count = cut_area( cut );
    if ( limit == 0 )
      return count;

    for ( auto leaf : cut )
    {
      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) || ntk.is_ci( ntk.index_to_node( leaf ) ) )
        continue;

      tmp_area.push_back( leaf );
      if ( map_refs[leaf]++ == 0 )
      {
        count += cut_ref_limit_save( cuts.cuts( leaf ).best(), limit - 1 );
      }
    }
    return count;
  }

  /* estimates the cost of adding this cut to the mapping:
   *   This algorithm references cuts recursively to estimate how many cuts
   *   would be needed to add to the mapping if `cut` were to be added.  It
   *   temporarily modifies the reference counters but reverts them eventually.
   */
  uint32_t cut_area_estimation( cut_t const& cut )
  {
    tmp_area.clear();
    const auto count = cut_ref_limit_save( cut, 8 );
    for ( auto const& n : tmp_area )
    {
      map_refs[n]--;
    }
    return count;
  }

  template<bool ELA>
  void compute_best_cut( uint32_t index )
  {
    constexpr auto mf_eps{ 0.005f };

    float flow;
    uint32_t time{ 0 };
    int32_t best_cut{ -1 };
    float best_flow{ std::numeric_limits<float>::max() };
    uint32_t best_time{ std::numeric_limits<uint32_t>::max() };
    int32_t cut_index{ -1 };

    if constexpr ( ELA )
    {
      if ( map_refs[index] > 0 )
      {
        cut_deref( cuts.cuts( index )[0] );
      }
    }

    for ( auto* cut : cuts.cuts( index ) )
    {
      ++cut_index;
      if ( cut->size() == 1 )
        continue;

      if constexpr ( ELA )
      {
        flow = static_cast<float>( cut_area_estimation( *cut ) );
      }
      else
      {
        std::tie( flow, time ) = cut_flow( *cut );
      }

      if ( best_cut == -1 || best_flow > flow + mf_eps || ( best_flow > flow - mf_eps && best_time > time ) )
      {
        best_cut = cut_index;
        best_flow = flow;
        best_time = time;
      }
    }

    if ( best_cut == -1 )
      return;

    if constexpr ( ELA )
    {
      if ( map_refs[index] > 0 )
      {
        cut_ref( cuts.cuts( index )[best_cut] );
      }
    }
    else
    {
      map_refs[index] = 0;
    }
    if constexpr ( ELA )
    {
      best_time = cut_flow( cuts.cuts( index )[best_cut] ).second;
    }
    delays[index] = best_time;
    flows[index] = best_flow / flow_refs[index];

    if ( best_cut != 0 )
    {
      cuts.cuts( index ).update_best( best_cut );
    }
  }

  void derive_mapping()
  {
    ntk.clear_mapping();

    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;

      const auto index = ntk.node_to_index( n );
      if ( map_refs[index] == 0 )
        continue;

      std::vector<node<Ntk>> nodes;
      for ( auto const& l : cuts.cuts( index ).best() )
      {
        nodes.push_back( ntk.index_to_node( l ) );
      }
      ntk.add_to_mapping( n, nodes.begin(), nodes.end() );

      if constexpr ( StoreFunction )
      {
        ntk.set_cell_function( n, cuts.truth_table( cuts.cuts( index ).best() ) );
      }
    }
  }

  void print_state()
  {
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      std::cout << fmt::format( "*** Obj = {:>3} (node = {:>3})  FlowRefs = {:5.2f}  MapRefs = {:>2}  Flow = {:5.2f}  Delay = {:>3}\n", i, ntk.index_to_node( i ), flow_refs[i], map_refs[i], flows[i], delays[i] );
      // std::cout << cuts.cuts( i );
    }
    std::cout << fmt::format( "Level = {}  Area = {}\n", delay, area );
  }

private:
  Ntk& ntk;
  lut_mapping_params const& ps;
  lut_mapping_stats& st;

  uint32_t iteration{ 0 }; /* current mapping iteration */
  uint32_t delay{ 0 };     /* current delay of the mapping */
  uint32_t area{ 0 };      /* current area of the mapping */
  // bool ela{false};       /* compute exact area */

  std::vector<node<Ntk>> top_order;
  std::vector<float> flow_refs;
  std::vector<uint32_t> map_refs;
  std::vector<float> flows;
  std::vector<uint32_t> delays;
  network_cuts_t cuts;

  std::vector<uint32_t> tmp_area; /* temporary vector to compute exact area */
};

}; /* namespace detail */

/*! \brief LUT mapping.
 *
 * This function implements a LUT mapping algorithm.  It is controlled by two
 * template arguments `StoreFunction` (defaulted to `true`) and `CutData`
 * (defaulted to `cut_enumeration_mf_cut`).  The first argument `StoreFunction`
 * controls whether the LUT function is stored in the mapping.  In that case
 * truth tables are computed during cut enumeration, which requires more
 * runtime.  The second argument is simuilar to the `CutData` argument in
 * `cut_enumeration`, which can specialize the cost function to select priority
 * cuts and store additional data.  For LUT mapping using this function the
 * type passed as `CutData` must implement the following three fields:
 *
 * - `uint32_t delay`
 * - `float flow`
 * - `float costs`
 *
 * See `include/mockturtle/algorithms/cut_enumeration/mf_cut.hpp` for one
 * example of a CutData type that implements the cost function that is used in
 * the LUT mapper `&mf` in ABC.
 *
 * **Required network functions:**
 * - `size`
 * - `is_ci`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_co`
 * - `foreach_node`
 * - `fanout_size`
 * - `clear_mapping`
 * - `add_to_mapping`
 * - `set_lut_function` (if `StoreFunction` is true)
 *
   \verbatim embed:rst

   .. note::

      The implementation of this algorithm was heavily inspired but the LUT
      mapping command ``&mf`` in ABC.
   \endverbatim
 */
template<class Ntk, bool StoreFunction = false, typename CutData = cut_enumeration_mf_cut>
void lut_mapping( Ntk& ntk, lut_mapping_params const& ps = {}, lut_mapping_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_clear_mapping_v<Ntk>, "Ntk does not implement the clear_mapping method" );
  static_assert( has_add_to_mapping_v<Ntk>, "Ntk does not implement the add_to_mapping method" );
  static_assert( !StoreFunction || has_set_cell_function_v<Ntk>, "Ntk does not implement the set_cell_function method" );

  lut_mapping_stats st;
  detail::lut_mapping_impl<Ntk, StoreFunction, CutData> p( ntk, ps, st );
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

} /* namespace mockturtle */
