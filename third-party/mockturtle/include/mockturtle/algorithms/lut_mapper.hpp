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
  \file lut_mapper.hpp
  \brief LUT mapper

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <queue>
#include <sstream>
#include <string>

#include <fmt/format.h>
#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/esop.hpp>
#include <kitty/isop.hpp>
#include <kitty/operations.hpp>

#include "../networks/klut.hpp"
#include "../utils/cost_functions.hpp"
#include "../utils/cuts.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/truth_table_cache.hpp"
#include "../views/choice_view.hpp"
#include "../views/mapping_view.hpp"
#include "../views/mffc_view.hpp"
#include "../views/topo_view.hpp"
#include "cleanup.hpp"
#include "collapse_mapped.hpp"
#include "cut_enumeration.hpp"
#include "exorcism.hpp"
#include "simulation.hpp"

namespace mockturtle
{

/*! \brief Parameters for map.
 *
 * The data structure `map_params` holds configurable parameters
 * with default arguments for `map`.
 */
struct lut_map_params
{
  lut_map_params()
  {
    cut_enumeration_ps.cut_size = 6u;
    cut_enumeration_ps.cut_limit = 8u;
    cut_enumeration_ps.minimize_truth_table = true;
  }

  /*! \brief Parameters for cut enumeration
   *
   * The default cut limit is 8. The maximum value
   * is 16. The maxiumum cut size is 16. By default,
   * truth table minimization is performed.
   */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Do area-oriented mapping. */
  bool area_oriented_mapping{ false };

  /*! \brief Required depth for depth relaxation. */
  uint32_t required_delay{ 0u };

  /*! \brief Required depth relaxation ratio (%). */
  uint32_t relax_required{ 0u };

  /*! \brief Recompute cuts at each step. */
  bool recompute_cuts{ true };

  /*! \brief Number of rounds for area sharing optimization. */
  uint32_t area_share_rounds{ 2u };

  /*! \brief Number of rounds for area flow optimization. */
  uint32_t area_flow_rounds{ 1u };

  /*! \brief Number of rounds for exact area optimization. */
  uint32_t ela_rounds{ 2u };

  /*! \brief Use edge count reduction. */
  bool edge_optimization{ true };

  /*! \brief Try to expand the cuts. */
  bool cut_expansion{ true };

  /*! \brief Remove the cuts that are contained in others */
  bool remove_dominated_cuts{ true };

  /*! \brief Maps by collapsing MFFCs */
  bool collapse_mffcs{ false };

  /*! \brief Depth optimization by balancing ISOPs */
  bool sop_balancing{ false };

  /*! \brief Depth optimization by balancing ESOPs */
  bool esop_balancing{ false };

  /*! \brief Maximum number variables for cost function caching */
  uint32_t cost_cache_vars{ 3u };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

/*! \brief Statistics for mapper.
 *
 * The data structure `map_stats` provides data collected by running
 * `map`.
 */
struct lut_map_stats
{
  /*! \brief Area result. */
  uint32_t area{ 0 };
  /*! \brief Worst delay result. */
  uint32_t delay{ 0 };
  /*! \brief Edge result. */
  uint32_t edges{ 0 };

  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Cut enumeration stats. */
  cut_enumeration_stats cut_enumeration_st{};

  /*! \brief Depth and size stats for each round. */
  std::vector<std::string> round_stats{};

  void report() const
  {
    for ( auto const& stat : round_stats )
    {
      std::cout << stat;
    }
    std::cout << fmt::format( "[i] Total runtime           = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

#pragma region cut set
/* cut data */
struct cut_enumeration_lut_cut
{
  uint32_t delay{ 0 };
  uint32_t lut_area{ 0 };
  uint32_t lut_delay{ 0 };
  float area_flow{ 0 };
  float edge_flow{ 0 };
  bool ignore{ false };
};

enum class lut_cut_sort_type
{
  DELAY,
  DELAY2,
  AREA,
  AREA2,
  NONE
};

template<typename CutType, int MaxCuts>
class lut_cut_set
{
public:
  /*! \brief Standard constructor.
   */
  lut_cut_set()
  {
    clear();
  }

  /*! \brief Assignment operator.
   */
  lut_cut_set& operator=( lut_cut_set const& other )
  {
    if ( this != &other )
    {
      _pcend = _pend = _pcuts.begin();

      auto it = other.begin();
      while ( it != other.end() )
      {
        **_pend++ = **it++;
        ++_pcend;
      }
    }

    return *this;
  }

  /*! \brief Clears a cut set.
   */
  void clear()
  {
    _pcend = _pend = _pcuts.begin();
    auto pit = _pcuts.begin();
    for ( auto& c : _cuts )
    {
      *pit++ = &c;
    }
  }

  /*! \brief Adds a cut to the end of the set.
   *
   * This function should only be called to create a set of cuts which is known
   * to be sorted and irredundant (i.e., no cut in the set dominates another
   * cut).
   *
   * \param begin Begin iterator to leaf indexes
   * \param end End iterator (exclusive) to leaf indexes
   * \return Reference to the added cut
   */
  template<typename Iterator>
  CutType& add_cut( Iterator begin, Iterator end )
  {
    assert( _pend != _pcuts.end() );

    auto& cut = **_pend++;
    cut.set_leaves( begin, end );

    ++_pcend;
    return cut;
  }

  /*! \brief Checks whether cut is dominates by any cut in the set.
   *
   * \param cut Cut outside of the set
   */
  bool is_dominated( CutType const& cut ) const
  {
    return std::find_if( _pcuts.begin(), _pcend, [&cut]( auto const* other ) { return other->dominates( cut ); } ) != _pcend;
  }

  static bool sort_delay( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->data.ignore && c2->data.ignore )
      return true;
    if ( c1->data.ignore && !c2->data.ignore )
      return false;
    if ( c1->data.delay < c2->data.delay )
      return true;
    if ( c1->data.delay > c2->data.delay )
      return false;
    if ( c1.size() < c2.size() )
      return true;
    if ( c1.size() > c2.size() )
      return false;
    if ( c1->data.area_flow < c2->data.area_flow - eps )
      return true;
    if ( c1->data.area_flow > c2->data.area_flow + eps )
      return false;
    return c1->data.edge_flow < c2->data.edge_flow - eps;
  }

  static bool sort_delay2( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->data.ignore && c2->data.ignore )
      return true;
    if ( c1->data.ignore && !c2->data.ignore )
      return false;
    if ( c1->data.delay < c2->data.delay )
      return true;
    if ( c1->data.delay > c2->data.delay )
      return false;
    if ( c1->data.area_flow < c2->data.area_flow - eps )
      return true;
    if ( c1->data.area_flow > c2->data.area_flow + eps )
      return false;
    if ( c1->data.edge_flow < c2->data.edge_flow - eps )
      return true;
    if ( c1->data.edge_flow > c2->data.edge_flow + eps )
      return false;
    return c1.size() < c2.size();
  }

  static bool sort_area( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->data.ignore && c2->data.ignore )
      return true;
    if ( c1->data.ignore && !c2->data.ignore )
      return false;
    if ( c1->data.area_flow < c2->data.area_flow - eps )
      return true;
    if ( c1->data.area_flow > c2->data.area_flow + eps )
      return false;
    if ( c1->data.delay < c2->data.delay )
      return true;
    if ( c1->data.delay > c2->data.delay )
      return false;
    return c1.size() < c2.size();
  }

  static bool sort_area2( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->data.ignore && c2->data.ignore )
      return true;
    if ( c1->data.ignore && !c2->data.ignore )
      return false;
    if ( c1->data.area_flow < c2->data.area_flow - eps )
      return true;
    if ( c1->data.area_flow > c2->data.area_flow + eps )
      return false;
    if ( c1->data.edge_flow < c2->data.edge_flow - eps )
      return true;
    if ( c1->data.edge_flow > c2->data.edge_flow + eps )
      return false;
    if ( c1.size() < c2.size() )
      return true;
    if ( c1.size() > c2.size() )
      return false;
    return c1->data.delay < c2->data.delay;
  }

  /*! \brief Compare two cuts using sorting functions.
   *
   * This method compares two cuts using a sorting function.
   *
   * \param cut1 first cut.
   * \param cut2 second cut.
   * \param sort sorting function.
   */
  static bool compare( CutType const& cut1, CutType const& cut2, lut_cut_sort_type sort = lut_cut_sort_type::NONE )
  {
    if ( sort == lut_cut_sort_type::DELAY )
    {
      return sort_delay( cut1, cut2 );
    }
    else if ( sort == lut_cut_sort_type::DELAY2 )
    {
      return sort_delay2( cut1, cut2 );
    }
    else if ( sort == lut_cut_sort_type::AREA )
    {
      return sort_area( cut1, cut2 );
    }
    else if ( sort == lut_cut_sort_type::AREA2 )
    {
      return sort_area2( cut1, cut2 );
    }
    else
    {
      return false;
    }
  }

  /*! \brief Inserts a cut into a set without checking dominance.
   *
   * This method will insert a cut into a set and maintain an order.  This
   * method doesn't remove the cuts that are dominated by `cut`.
   *
   * If `cut` is dominated by any of the cuts in the set, it will still be
   * inserted.  The caller is responsible to check whether `cut` is dominated
   * before inserting it into the set.
   *
   * \param cut Cut to insert.
   * \param sort Cut prioritization function.
   */
  void simple_insert( CutType const& cut, lut_cut_sort_type sort = lut_cut_sort_type::NONE )
  {
    /* insert cut in a sorted way */
    typename std::array<CutType*, MaxCuts>::iterator ipos = _pcuts.begin();

    if ( sort == lut_cut_sort_type::DELAY )
    {
      ipos = std::lower_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return sort_delay( *a, *b ); } );
    }
    else if ( sort == lut_cut_sort_type::DELAY2 )
    {
      ipos = std::lower_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return sort_delay2( *a, *b ); } );
    }
    else if ( sort == lut_cut_sort_type::AREA )
    {
      ipos = std::lower_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return sort_area( *a, *b ); } );
    }
    else if ( sort == lut_cut_sort_type::AREA2 )
    {
      ipos = std::lower_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return sort_area2( *a, *b ); } );
    }
    else /* NONE */
    {
      ipos == _pend;
    }

    /* too many cuts, we need to remove one */
    if ( _pend == _pcuts.end() )
    {
      /* cut to be inserted is worse than all the others, return */
      if ( ipos == _pend )
      {
        return;
      }
      else
      {
        /* remove last cut */
        --_pend;
        --_pcend;
      }
    }

    /* copy cut */
    auto& icut = *_pend;
    icut->set_leaves( cut.begin(), cut.end() );
    icut->data() = cut.data();

    if ( ipos != _pend )
    {
      auto it = _pend;
      while ( it > ipos )
      {
        std::swap( *it, *( it - 1 ) );
        --it;
      }
    }

    /* update iterators */
    _pcend++;
    _pend++;
  }

  /*! \brief Inserts a cut into a set.
   *
   * This method will insert a cut into a set and maintain an order.  Before the
   * cut is inserted into the correct position, it will remove all cuts that are
   * dominated by `cut`. Variable `skip0` tell to skip the dominance check on
   * cut zero.
   *
   * If `cut` is dominated by any of the cuts in the set, it will still be
   * inserted.  The caller is responsible to check whether `cut` is dominated
   * before inserting it into the set.
   *
   * \param cut Cut to insert.
   * \param skip0 Skip dominance check on cut zero.
   * \param sort Cut prioritization function.
   */
  void insert( CutType const& cut, bool skip0 = false, lut_cut_sort_type sort = lut_cut_sort_type::NONE )
  {
    auto begin = _pcuts.begin();

    if ( skip0 && _pend != _pcuts.begin() )
      ++begin;

    /* remove elements that are dominated by new cut */
    _pcend = _pend = std::stable_partition( begin, _pend, [&cut]( auto const* other ) { return !cut.dominates( *other ); } );

    /* insert cut in a sorted way */
    simple_insert( cut, sort );
  }

  /*! \brief Replaces a cut of the set.
   *
   * This method replaces the cut at position `index` in the set by `cut`
   * and maintains the cuts order. The function does not check whether
   * index is in the valid range.
   *
   * \param index Index of the cut to replace.
   * \param cut Cut to insert.
   */
  void replace( uint32_t index, CutType const& cut )
  {
    *_pcuts[index] = cut;
  }

  /*! \brief Begin iterator (constant).
   *
   * The iterator will point to a cut pointer.
   */
  auto begin() const { return _pcuts.begin(); }

  /*! \brief End iterator (constant). */
  auto end() const { return _pcend; }

  /*! \brief Begin iterator (mutable).
   *
   * The iterator will point to a cut pointer.
   */
  auto begin() { return _pcuts.begin(); }

  /*! \brief End iterator (mutable). */
  auto end() { return _pend; }

  /*! \brief Number of cuts in the set. */
  auto size() const { return _pcend - _pcuts.begin(); }

  /*! \brief Returns reference to cut at index.
   *
   * This function does not return the cut pointer but dereferences it and
   * returns a reference.  The function does not check whether index is in the
   * valid range.
   *
   * \param index Index
   */
  auto const& operator[]( uint32_t index ) const { return *_pcuts[index]; }

  /*! \brief Returns the best cut, i.e., the first cut.
   */
  auto& best() const { return *_pcuts[0]; }

  /*! \brief Updates the best cut.
   *
   * This method will set the cut at index `index` to be the best cut.  All
   * cuts before `index` will be moved one position higher.
   *
   * \param index Index of new best cut
   */
  void update_best( uint32_t index )
  {
    auto* best = _pcuts[index];
    for ( auto i = index; i > 0; --i )
    {
      _pcuts[i] = _pcuts[i - 1];
    }
    _pcuts[0] = best;
  }

  /*! \brief Resize the cut set, if it is too large.
   *
   * This method will resize the cut set to `size` only if the cut set has more
   * than `size` elements.  Otherwise, the size will remain the same.
   */
  void limit( uint32_t size )
  {
    if ( std::distance( _pcuts.begin(), _pend ) > static_cast<long>( size ) )
    {
      _pcend = _pend = _pcuts.begin() + size;
    }
  }

  /*! \brief Prints a cut set. */
  friend std::ostream& operator<<( std::ostream& os, lut_cut_set const& set )
  {
    for ( auto const& c : set )
    {
      os << *c << "\n";
    }
    return os;
  }

private:
  std::array<CutType, MaxCuts> _cuts;
  std::array<CutType*, MaxCuts> _pcuts;
  typename std::array<CutType*, MaxCuts>::const_iterator _pcend{ _pcuts.begin() };
  typename std::array<CutType*, MaxCuts>::iterator _pend{ _pcuts.begin() };
};
#pragma endregion

#pragma region LUT mapper
struct node_lut
{
  /* required time at node output */
  uint32_t required;
  /* number of references in the cover */
  uint32_t map_refs;
  /* references estimation */
  float est_refs;
};

template<class Ntk, bool StoreFunction, class LUTCostFn>
class lut_map_impl
{
private:
  /* special map for output drivers to perform some optimizations */
  enum class driver_type : uint32_t
  {
    none = 0,
    pos = 1,
    neg = 2,
    mixed = 3
  };

public:
  static constexpr uint32_t max_cut_num = 32;
  static constexpr uint32_t max_cut_size = 16;
  static constexpr uint32_t max_cubes = 64;
  static constexpr uint32_t max_sop_decomp_size = max_cut_size * ( max_cubes + 1 );
  using cut_t = cut<max_cut_size, cut_data<StoreFunction, cut_enumeration_lut_cut>>;
  using cut_set_t = lut_cut_set<cut_t, max_cut_num>;
  using node = typename Ntk::node;
  using cut_merge_t = typename std::array<cut_set_t*, Ntk::max_fanin_size + 1>;
  using TT = kitty::dynamic_truth_table;
  using tt_cache = truth_table_cache<TT>;
  using cost_cache = std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>>;
  using sop_t = std::vector<kitty::cube>;
  using isop_cache = std::vector<sop_t>;
  using cubes_queue_t = std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>>;
  using lut_info = std::pair<kitty::dynamic_truth_table, std::vector<signal<klut_network>>>;

public:
  explicit lut_map_impl( Ntk& ntk, lut_map_params const& ps, lut_map_stats& st )
      : ntk( ntk ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        cuts( ntk.size() )
  {
    assert( ps.cut_enumeration_ps.cut_limit < max_cut_num && "cut_limit exceeds the compile-time limit for the maximum number of cuts" );

    if constexpr ( StoreFunction )
    {
      TT zero( 0u ), proj( 1u );
      kitty::create_nth_var( proj, 0u );

      tmp_visited.reserve( 100 );
      truth_tables.resize( 32768 );

      truth_tables.insert( zero );
      truth_tables.insert( proj );

      /* reserve cost cache */
      if constexpr ( !std::is_same<LUTCostFn, lut_unitary_cost>::value )
      {
        truth_tables_cost.reserve( 1000 );
      }

      /* reserve ISOP cache */
      if ( ps.sop_balancing || ps.esop_balancing )
      {
        isops.reserve( 32768 );
        isops.emplace_back();                       /* empty ISOP for constant */
        isops.push_back( { kitty::cube{ 1, 1 } } ); /* ISOP for variable */
      }
    }
  }

  klut_network run()
  {
    stopwatch t( st.time_total );

    /* compute and save topological order */
    topo_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      topo_order.push_back( n );
    } );

    perform_mapping();
    return create_lut_network();
  }

  void run_inplace()
  {
    stopwatch t( st.time_total );

    /* compute and save topological order */
    topo_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      topo_order.push_back( n );
    } );

    if ( ps.collapse_mffcs )
    {
      compute_mffcs_mapping();
      return;
    }

    perform_mapping();
    derive_mapping();
  }

private:
  void perform_mapping()
  {
    /* define area sorting function */
    lut_cut_sort_type area_sort = ( ps.area_oriented_mapping && !ps.edge_optimization ) ? lut_cut_sort_type::AREA : lut_cut_sort_type::AREA2;

    /* init the data structure */
    init_nodes();
    init_cuts();

    /* compute mapping for depth or area */
    if ( !ps.area_oriented_mapping )
    {
      compute_required_time();

      if ( ps.recompute_cuts )
      {
        compute_mapping<false, false>( lut_cut_sort_type::DELAY, true, true );
        compute_required_time();
        compute_mapping<false, false>( lut_cut_sort_type::DELAY2, true, true );
        compute_required_time();
        compute_mapping<true, false>( area_sort, true, true );
      }
      else
      {
        compute_mapping<false, false>( lut_cut_sort_type::DELAY2, true, true );
      }
    }
    else
    {
      compute_required_time();
      compute_mapping<true, false>( area_sort, false, true );
    }

    if ( ps.cut_expansion )
    {
      compute_required_time();
      expand_cuts<false>();
    }

    /* try backward area iterations */
    uint32_t i = 0;
    while ( i < ps.area_share_rounds )
    {
      compute_share_mapping( area_sort, i == 0 );

      if ( ps.cut_expansion )
      {
        expand_cuts<false>();
      }
      ++i;
    }

    /* compute mapping using global area flow */
    i = 0;
    while ( i < ps.area_flow_rounds )
    {
      compute_required_time();
      compute_mapping<true, false>( area_sort, false, ps.recompute_cuts );

      if ( ps.cut_expansion )
      {
        compute_required_time();
        expand_cuts<false>();
      }
      ++i;
    }

    /* compute mapping using exact area/edge */
    i = 0;
    while ( i < ps.ela_rounds )
    {
      compute_required_time();
      compute_mapping<true, true>( area_sort, false, ps.recompute_cuts );

      if ( ps.cut_expansion )
      {
        compute_required_time();
        expand_cuts<true>();
      }
      ++i;
    }
  }

  void init_nodes()
  {
    ntk.foreach_node( [this]( auto const& n ) {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      node_data.map_refs = ntk.fanout_size( n );
      node_data.est_refs = static_cast<float>( ntk.fanout_size( n ) );
    } );
  }

  void init_cuts()
  {
    /* init constant cut */
    add_zero_cut( ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) ), false );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
      add_zero_cut( ntk.node_to_index( ntk.get_node( ntk.get_constant( true ) ) ), true );

    /* init PIs cuts */
    ntk.foreach_ci( [&]( auto const& n ) {
      add_unit_cut( ntk.node_to_index( n ) );
    } );
  }

  template<bool DO_AREA, bool ELA>
  void compute_mapping( lut_cut_sort_type const sort, bool preprocess, bool recompute_cuts )
  {
    cuts_total = 0;
    for ( auto const& n : topo_order )
    {
      if constexpr ( !ELA )
      {
        auto const index = ntk.node_to_index( n );
        if ( !preprocess && iteration != 0 )
        {
          node_match[index].est_refs = ( 2.0 * node_match[index].est_refs + 1.0 * node_match[index].map_refs ) / 3.0;
        }
        else
        {
          node_match[index].est_refs = static_cast<float>( node_match[index].map_refs );
        }
      }

      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
      {
        continue;
      }

      if ( recompute_cuts )
      {
        if constexpr ( Ntk::min_fanin_size == 2 && Ntk::max_fanin_size == 2 )
        {
          compute_best_cut2<DO_AREA, ELA>( n, sort, preprocess );
        }
        else
        {
          compute_best_cut<DO_AREA, ELA>( n, sort, preprocess );
        }
      }
      else
      {
        /* update cost the function and move the best one first */
        update_cut_data<DO_AREA, ELA>( n, sort );
      }
    }

    set_mapping_refs<ELA>();

    if constexpr ( DO_AREA )
    {
      ++area_iteration;
    }

    /* round stats */
    {
      std::stringstream stats;

      if ( ( sort == lut_cut_sort_type::AREA || sort == lut_cut_sort_type::AREA2 ) && ELA )
      {
        stats << fmt::format( "[i] Area     : Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total );
      }
      else if ( sort == lut_cut_sort_type::AREA || sort == lut_cut_sort_type::AREA2 )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total );
      }
      else if ( sort == lut_cut_sort_type::DELAY2 )
      {
        stats << fmt::format( "[i] Delay2   : Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total );
      }
      st.round_stats.push_back( stats.str() );
    }
  }

  void compute_share_mapping( lut_cut_sort_type const sort, bool first )
  {
    /* reset required times and references except for POs */
    compute_share_mapping_init( first );

    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      auto const index = ntk.node_to_index( *it );

      /* skip not used nodes */
      if ( node_match[index].map_refs == 0 )
        continue;

      /* update cost the function and move the best one first */
      update_cut_data_share( *it, sort );
    }

    /* propagate correct arrival times and compute stats */
    propagate_arrival_times();

    /* round stats */
    {
      st.round_stats.push_back( fmt::format( "[i] AreaSh   : Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total ) );
    }
  }

  template<bool ELA>
  void expand_cuts()
  {
    /* cut expansion is not yet compatible with truth table computation */
    if constexpr ( StoreFunction )
      return;

    /* don't expand if cut recomputed cuts is off */
    if ( !ps.recompute_cuts )
      return;

    for ( auto const& n : topo_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
      {
        continue;
      }

      expand_cuts_node( n );
    }

    set_mapping_refs<ELA>();

    std::string stats = fmt::format( "[i] Reduce   : Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total );
    st.round_stats.push_back( stats );
  }

  template<bool ELA>
  void set_mapping_refs()
  {
    if constexpr ( !ELA )
    {
      for ( auto i = 0u; i < node_match.size(); ++i )
      {
        node_match[i].map_refs = 0u;
      }
    }

    /* compute the current worst delay and update the mapping refs */
    delay = 0;
    ntk.foreach_co( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );

      delay = std::max( delay, cuts[index][0]->data.delay );

      if constexpr ( !ELA )
      {
        ++node_match[index].map_refs;
      }
    } );

    /* compute current area and update mapping refs in top-down order */
    area = 0;
    edges = 0;
    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      /* skip constants and PIs */
      if ( ntk.is_constant( *it ) || ntk.is_ci( *it ) )
      {
        continue;
      }

      const auto index = ntk.node_to_index( *it );
      auto& node_data = node_match[index];

      /* continue if not referenced in the cover */
      if ( node_match[index].map_refs == 0u )
        continue;

      auto& best_cut = cuts[index][0];

      if constexpr ( !ELA )
      {
        for ( auto const leaf : best_cut )
        {
          node_match[leaf].map_refs++;
        }
      }
      area += best_cut->data.lut_area;
      edges += best_cut.size();
    }

    ++iteration;
  }

  void compute_required_time()
  {
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required = UINT32_MAX >> 1;
    }

    /* return in case of area_oriented_mapping */
    if ( iteration == 0 || ps.area_oriented_mapping )
      return;

    uint32_t required = delay;

    /* relax delay constraints */
    if ( ps.required_delay == 0.0f && ps.relax_required > 0.0f )
    {
      required *= ( 100.0 + ps.relax_required ) / 100.0;
    }

    if ( ps.required_delay != 0 )
    {
      /* Global target time constraint */
      if ( ps.required_delay < delay )
      {
        if ( !ps.area_oriented_mapping && iteration == 1 )
          std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {}", ps.required_delay ) << std::endl;
      }
      else
      {
        required = ps.required_delay;
      }
    }

    /* set the required time at POs */
    ntk.foreach_co( [&]( auto const& s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      node_match[index].required = required;
    } );

    /* propagate required time to the PIs */
    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      if ( ntk.is_ci( *it ) || ntk.is_constant( *it ) )
        continue;

      const auto index = ntk.node_to_index( *it );

      if ( node_match[index].map_refs == 0 )
        continue;

      /* in case of decomposition cost */
      if constexpr ( StoreFunction )
      {
        if ( ps.sop_balancing || ps.esop_balancing )
        {
          compute_balancing_cost_required( index );
          continue;
        }
      }

      for ( auto leaf : cuts[index][0] )
      {
        node_match[leaf].required = std::min( node_match[leaf].required, node_match[index].required - cuts[index][0]->data.lut_delay );
      }
    }
  }

  void propagate_arrival_times()
  {
    area = 0;
    edges = 0;
    for ( auto const& n : topo_order )
    {
      auto index = ntk.node_to_index( n );

      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
      {
        continue;
      }

      /* propagate arrival time */
      uint32_t node_delay = 0;
      auto& best_cut = cuts[index].best();

      for ( auto leaf : best_cut )
      {
        const auto& best_leaf_cut = cuts[leaf][0];
        node_delay = std::max( node_delay, best_leaf_cut->data.delay );
      }

      best_cut->data.delay = node_delay + best_cut->data.lut_delay;

      /* continue if not referenced in the cover */
      if ( node_match[index].map_refs == 0u )
        continue;

      /* update stats */
      area += best_cut->data.lut_area;
      edges += best_cut.size();
    }

    /* update worst delay */
    delay = 0;
    ntk.foreach_co( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      delay = std::max( delay, cuts[index][0]->data.delay );
    } );
  }

  void compute_share_mapping_init( bool first )
  {
    /* reset the mapping references and the required time */
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required = UINT32_MAX >> 1;
      if ( !first )
        node_match[i].est_refs = ( 2.0 * node_match[i].est_refs + 1.0 * node_match[i].map_refs ) / 3.0;
      else
        node_match[i].est_refs = std::max( 1.0, ( 1.0 * node_match[i].est_refs + 2.0 * node_match[i].map_refs ) / 3.0 );
      node_match[i].map_refs = 0;

      /* update flows if in area-oriented mapping */
      if ( ps.area_oriented_mapping )
        compute_cut_data<false>( cuts[i].best(), ntk.index_to_node( i ), false );
    }

    uint32_t required = delay;
    if ( ps.required_delay == 0.0f && ps.relax_required > 0.0f )
    {
      required *= ( 100.0 + ps.relax_required ) / 100.0;
    }

    if ( ps.required_delay != 0 )
    {
      /* Global target time constraint */
      if ( ps.required_delay < delay )
      {
        if ( !ps.area_oriented_mapping && iteration == 1 )
          std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {}", ps.required_delay ) << std::endl;
      }
      else
      {
        required = ps.required_delay;
      }
    }

    /* set the required time at POs */
    ntk.foreach_co( [&]( auto const& s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      node_match[index].required = required;
      node_match[index].map_refs++;
    } );
  }

  template<bool DO_AREA, bool ELA>
  void compute_best_cut2( node const& n, lut_cut_sort_type const sort, bool preprocess )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    cut_t best_cut;

    /* compute cuts */
    const auto fanin = 2;
    uint32_t pairs{ 1 };
    ntk.foreach_fanin( ntk.index_to_node( index ), [this, &pairs]( auto child, auto i ) {
      lcuts[i] = &cuts[ntk.node_to_index( ntk.get_node( child ) )];
      pairs *= static_cast<uint32_t>( lcuts[i]->size() );
    } );
    lcuts[2] = &cuts[index];
    auto& rcuts = *lcuts[fanin];

    if constexpr ( DO_AREA )
    {
      if ( iteration != 0 && node_data.map_refs > 0 )
      {
        cut_deref( rcuts[0] );
      }
    }

    /* recompute the data of the best cut */
    if ( iteration != 0 )
    {
      best_cut = rcuts[0];
      compute_cut_data<ELA>( best_cut, n, true );
    }

    /* clear cuts */
    rcuts.clear();

    /* insert the previous best cut */
    if ( iteration != 0 && !preprocess )
    {
      rcuts.simple_insert( best_cut, sort );
    }

    cut_t new_cut;
    std::vector<cut_t const*> vcuts( fanin );

    for ( auto const& c1 : *lcuts[0] )
    {
      for ( auto const& c2 : *lcuts[1] )
      {
        if ( !c1->merge( *c2, new_cut, ps.cut_enumeration_ps.cut_size ) )
        {
          continue;
        }

        if ( ps.remove_dominated_cuts && rcuts.is_dominated( new_cut ) )
        {
          continue;
        }

        if constexpr ( StoreFunction )
        {
          vcuts[0] = c1;
          vcuts[1] = c2;
          new_cut->func_id = compute_truth_table( index, vcuts, new_cut );
        }

        compute_cut_data<ELA>( new_cut, ntk.index_to_node( index ), true );

        /* check required time */
        if constexpr ( DO_AREA )
        {
          if ( preprocess || new_cut->data.delay <= node_data.required )
          {
            if ( ps.remove_dominated_cuts )
              rcuts.insert( new_cut, false, sort );
            else
              rcuts.simple_insert( new_cut, sort );
          }
        }
        else
        {
          if ( ps.remove_dominated_cuts )
            rcuts.insert( new_cut, false, sort );
          else
            rcuts.simple_insert( new_cut, sort );
        }
      }
    }

    cuts_total += rcuts.size();

    /* limit the maximum number of cuts */
    rcuts.limit( ps.cut_enumeration_ps.cut_limit );

    /* replace the new best cut with previous one */
    if ( preprocess && rcuts[0]->data.delay > node_data.required )
      rcuts.replace( 0, best_cut );

    /* add trivial cut */
    if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 )
    {
      add_unit_cut( index );
    }

    if constexpr ( DO_AREA )
    {
      if ( iteration != 0 && node_data.map_refs > 0 )
      {
        cut_ref( rcuts[0] );
      }
    }
  }

  template<bool DO_AREA, bool ELA>
  void compute_best_cut( node const& n, lut_cut_sort_type const sort, bool preprocess )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    cut_t best_cut;

    /* compute cuts */
    uint32_t pairs{ 1 };
    std::vector<uint32_t> cut_sizes;
    ntk.foreach_fanin( ntk.index_to_node( index ), [this, &pairs, &cut_sizes]( auto child, auto i ) {
      lcuts[i] = &cuts[ntk.node_to_index( ntk.get_node( child ) )];
      cut_sizes.push_back( static_cast<uint32_t>( lcuts[i]->size() ) );
      pairs *= cut_sizes.back();
    } );
    const auto fanin = cut_sizes.size();
    lcuts[fanin] = &cuts[index];
    auto& rcuts = *lcuts[fanin];

    if constexpr ( DO_AREA )
    {
      if ( iteration != 0 && node_data.map_refs > 0 )
      {
        cut_deref( rcuts[0] );
      }
    }

    /* recompute the data of the best cut */
    if ( iteration != 0 )
    {
      best_cut = rcuts[0];
      compute_cut_data<ELA>( best_cut, n, true );
    }

    /* clear cuts */
    rcuts.clear();

    /* insert the previous best cut */
    if ( iteration != 0 && !preprocess )
    {
      rcuts.simple_insert( best_cut, sort );
    }

    if ( fanin > 1 && fanin <= ps.cut_enumeration_ps.fanin_limit )
    {
      cut_t new_cut, tmp_cut;

      std::vector<cut_t const*> vcuts( fanin );

      foreach_mixed_radix_tuple( cut_sizes.begin(), cut_sizes.end(), [&]( auto begin, auto end ) {
        auto it = vcuts.begin();
        auto i = 0u;
        while ( begin != end )
        {
          *it++ = &( ( *lcuts[i++] )[*begin++] );
        }

        if ( !vcuts[0]->merge( *vcuts[1], new_cut, ps.cut_enumeration_ps.cut_size ) )
        {
          return true; /* continue */
        }

        for ( i = 2; i < fanin; ++i )
        {
          tmp_cut = new_cut;
          if ( !vcuts[i]->merge( tmp_cut, new_cut, ps.cut_enumeration_ps.cut_size ) )
          {
            return true; /* continue */
          }
        }

        if ( ps.remove_dominated_cuts && rcuts.is_dominated( new_cut ) )
        {
          return true; /* continue */
        }

        if constexpr ( StoreFunction )
        {
          new_cut->func_id = compute_truth_table( index, vcuts, new_cut );
        }

        compute_cut_data<ELA>( new_cut, index, true );

        /* check required time */
        if constexpr ( DO_AREA )
        {
          if ( preprocess || new_cut->data.delay <= node_data.required )
          {
            if ( ps.remove_dominated_cuts )
              rcuts.insert( new_cut, false, sort );
            else
              rcuts.simple_insert( new_cut, sort );
          }
        }
        else
        {
          if ( ps.remove_dominated_cuts )
            rcuts.insert( new_cut, false, sort );
          else
            rcuts.simple_insert( new_cut, sort );
        }

        return true;
      } );

      /* limit the maximum number of cuts */
      rcuts.limit( ps.cut_enumeration_ps.cut_limit );
    }
    else if ( fanin == 1 )
    {
      for ( auto const& cut : *lcuts[0] )
      {
        cut_t new_cut = *cut;

        if constexpr ( StoreFunction )
        {
          new_cut->func_id = compute_truth_table( index, { cut }, new_cut );
        }

        compute_cut_data<ELA>( new_cut, index, true );

        if constexpr ( DO_AREA )
        {
          if ( preprocess || new_cut->data.delay <= node_data.required )
          {
            if ( ps.remove_dominated_cuts )
              rcuts.insert( new_cut, false, sort );
            else
              rcuts.simple_insert( new_cut, sort );
          }
        }
        else
        {
          if ( ps.remove_dominated_cuts )
            rcuts.insert( new_cut, false, sort );
          else
            rcuts.simple_insert( new_cut, sort );
        }
      }

      /* limit the maximum number of cuts */
      rcuts.limit( ps.cut_enumeration_ps.cut_limit );
    }

    cuts_total += rcuts.size();

    /* replace the new best cut with previous one */
    if ( preprocess && rcuts[0]->data.delay > node_data.required )
      rcuts.replace( 0, best_cut );

    add_unit_cut( index );

    if constexpr ( DO_AREA )
    {
      if ( iteration != 0 && node_data.map_refs > 0 )
      {
        cut_ref( rcuts[0] );
      }
    }
  }

  template<bool DO_AREA, bool ELA>
  void update_cut_data( node const& n, lut_cut_sort_type const sort )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    auto& node_cut_set = cuts[index];
    uint32_t best_cut_index = 0;
    uint32_t cut_index = 0;

    cut_t const* best_cut = &node_cut_set.best();

    if constexpr ( DO_AREA )
    {
      if ( iteration != 0 && node_data.map_refs > 0 )
      {
        cut_deref( *best_cut );
      }
    }

    /* recompute the data for all the cuts and pick the best */
    for ( cut_t* cut : node_cut_set )
    {
      /* skip trivial cut */
      if ( cut->size() == 1 && *cut->begin() == index )
      {
        ++cut_index;
        continue;
      }

      compute_cut_data<ELA>( *cut, n, false );

      /* update best */
      if constexpr ( DO_AREA )
      {
        if ( ( *cut )->data.delay <= node_data.required )
        {
          if ( node_cut_set.compare( *cut, *best_cut, sort ) )
          {
            best_cut = cut;
            best_cut_index = cut_index;
          }
        }
      }
      else
      {
        if ( node_cut_set.compare( *cut, *best_cut, sort ) )
        {
          best_cut = cut;
          best_cut_index = cut_index;
        }
      }

      ++cut_index;
    }

    if constexpr ( DO_AREA || ELA )
    {
      if ( iteration != 0 && node_data.map_refs > 0 )
      {
        cut_ref( *best_cut );
      }
    }

    /* update the best cut */
    node_cut_set.update_best( best_cut_index );
  }

  void update_cut_data_share( node const& n, lut_cut_sort_type const sort )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    auto& node_cut_set = cuts[index];
    uint32_t best_cut_index = 0;
    uint32_t cut_index = 0;

    cut_t const* best_cut = &node_cut_set.best();

    /* recompute the data for all the cuts and pick the best */
    for ( cut_t* cut : node_cut_set )
    {
      /* skip trivial cut */
      if ( cut->size() == 1 && *cut->begin() == index )
      {
        ++cut_index;
        continue;
      }

      compute_cut_data_share( *cut );

      /* update best */
      if ( ( *cut )->data.delay <= node_data.required )
      {
        if ( node_cut_set.compare( *cut, *best_cut, sort ) )
        {
          best_cut = cut;
          best_cut_index = cut_index;
        }
      }

      ++cut_index;
    }

    /* propagate required times backward and reference the leaves */
    for ( auto leaf : *best_cut )
    {
      node_match[leaf].required = std::min( node_match[leaf].required, node_data.required - ( *best_cut )->data.lut_delay );
      node_match[leaf].map_refs++;
    }

    /* update the best cut */
    node_cut_set.update_best( best_cut_index );
  }

  void expand_cuts_node( node const& n )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    cut_t best_cut = cuts[index][0];

    if ( node_data.map_refs == 0 )
      return;

    /* update delay */
    uint32_t delay_update = 0;
    for ( auto const leaf : best_cut )
    {
      delay_update = std::max( delay_update, cuts[leaf][0]->data.delay + best_cut->data.lut_delay );
    }
    best_cut->data.delay = delay_update;

    auto const area_before = cut_deref( best_cut );

    uint32_t cost_before = 0;

    std::vector<uint32_t> leaves;

    /* mark volume */
    ntk.incr_trav_id();
    for ( auto const leaf : best_cut )
    {
      ntk.set_visited( ntk.index_to_node( leaf ), ntk.trav_id() );
      leaves.push_back( leaf );

      /* MFFC leaves */
      if ( node_match[leaf].map_refs == 0 )
        ++cost_before;
    }
    mark_cut_volume_rec( n );

    /* improve cut */
    while ( improve_cut( leaves ) )
      ;

    /* measure improvement */
    uint32_t cost_after = 0;
    for ( auto const leaf : leaves )
    {
      /* MFFC leaves */
      if ( node_match[leaf].map_refs == 0 )
        ++cost_after;
    }

    assert( cost_after <= cost_before );

    /* create the new cut */
    cut_t new_cut;
    new_cut.set_leaves( leaves.begin(), leaves.end() );
    new_cut->data = best_cut->data;

    uint32_t delay_after = 0;
    for ( auto const leaf : leaves )
    {
      delay_after = std::max( delay_after, cuts[leaf][0]->data.delay + new_cut->data.lut_delay );
    }
    new_cut->data.delay = delay_after;

    auto const area_after = cut_ref( new_cut );

    /* new cut is better */
    if ( area_after <= area_before && new_cut->data.delay <= node_data.required )
    {
      cuts[index].replace( 0, new_cut );
    }
    else
    {
      /* restore */
      cut_deref( new_cut );
      cut_ref( best_cut );
    }
  }

  bool improve_cut( std::vector<uint32_t>& leaves )
  {
    if ( improve_cut_expand0( leaves ) )
      return true;

    if ( leaves.size() < ps.cut_enumeration_ps.cut_size && improve_cut_expand1( leaves ) )
      return true;

    assert( leaves.size() <= ps.cut_enumeration_ps.cut_size );
    return false;
  }

  bool improve_cut_expand0( std::vector<uint32_t>& leaves )
  {
    for ( auto it = leaves.begin(); it != leaves.end(); ++it )
    {
      if ( ntk.is_ci( *it ) )
        continue;

      /* test if expansion would increase the number of leaves */
      int marked = 0;
      ntk.foreach_fanin( ntk.index_to_node( *it ), [&]( auto const& f ) {
        if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() )
          ++marked;
      } );

      if ( marked > 1 )
        continue;

      /* check that the cost does not increase */
      marked = 0;
      if ( node_match[*it].map_refs == 0 )
        --marked;

      ntk.foreach_fanin( ntk.index_to_node( *it ), [&]( auto const& f ) {
        if ( ntk.is_constant( ntk.get_node( f ) ) )
          return;
        auto const index = ntk.node_to_index( ntk.get_node( f ) );
        if ( ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() && node_match[index].map_refs == 0 )
          ++marked;
      } );

      /* not referenced leaves don't increase from the transformation */
      if ( marked <= 0 )
      {
        /* update leaves */
        uint32_t n = *it;
        leaves.erase( it );
        ntk.foreach_fanin( n, [&]( auto const& f ) {
          auto const index = ntk.node_to_index( ntk.get_node( f ) );
          if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() )
          {
            leaves.push_back( index );
            ntk.set_visited( ntk.get_node( f ), ntk.trav_id() );
          }
        } );
        return true;
      }
    }

    return false;
  }

  bool improve_cut_expand1( std::vector<uint32_t>& leaves )
  {
    for ( auto it = leaves.begin(); it != leaves.end(); ++it )
    {
      if ( ntk.is_ci( *it ) )
        continue;

      /* test if expansion would increase the number of leaves by more than 1*/
      int marked = 0;
      ntk.foreach_fanin( ntk.index_to_node( *it ), [&]( auto const& f ) {
        if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() )
          ++marked;
      } );

      if ( marked > 2 )
        continue;

      /* check that the cost reduces */
      marked = 0;
      if ( node_match[*it].map_refs == 0 )
        --marked;

      ntk.foreach_fanin( ntk.index_to_node( *it ), [&]( auto const& f ) {
        if ( ntk.is_constant( ntk.get_node( f ) ) )
          return;
        auto const index = ntk.node_to_index( ntk.get_node( f ) );
        if ( ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() && node_match[index].map_refs == 0 )
          ++marked;
      } );

      /* not referenced leaves should be reduced by the transformation */
      if ( marked < 0 )
      {
        /* update leaves */
        uint32_t n = *it;
        leaves.erase( it );
        ntk.foreach_fanin( n, [&]( auto const& f ) {
          auto const index = ntk.node_to_index( ntk.get_node( f ) );
          if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() )
          {
            leaves.push_back( index );
            ntk.set_visited( ntk.get_node( f ), ntk.trav_id() );
          }
        } );
        return true;
      }
    }

    return false;
  }

  uint32_t cut_ref( cut_t const& cut )
  {
    uint32_t count = cut->data.lut_area;

    for ( auto leaf : cut )
    {
      if ( ntk.is_ci( ntk.index_to_node( leaf ) ) || ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }

      /* Recursive referencing if leaf was not referenced */
      if ( node_match[leaf].map_refs++ == 0u )
      {
        count += cut_ref( cuts[leaf][0] );
      }
    }

    return count;
  }

  uint32_t cut_deref( cut_t const& cut )
  {
    uint32_t count = cut->data.lut_area;

    for ( auto leaf : cut )
    {
      if ( ntk.is_ci( ntk.index_to_node( leaf ) ) || ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }

      /* Recursive referencing if leaf was not referenced */
      if ( --node_match[leaf].map_refs == 0u )
      {
        count += cut_deref( cuts[leaf][0] );
      }
    }

    return count;
  }

  uint32_t cut_measure_mffc( cut_t const& cut )
  {
    tmp_visited.clear();

    uint32_t count = cut_ref_visit( cut );

    /* dereference visited */
    for ( auto const& s : tmp_visited )
    {
      --node_match[s].map_refs;
    }

    return count;
  }

  uint32_t cut_ref_visit( cut_t const& cut )
  {
    uint32_t count = cut->data.lut_area;

    for ( auto leaf : cut )
    {
      if ( ntk.is_ci( ntk.index_to_node( leaf ) ) || ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }

      /* add to visited */
      tmp_visited.push_back( leaf );

      /* Recursive referencing if leaf was not referenced */
      if ( node_match[leaf].map_refs++ == 0u )
      {
        count += cut_ref_visit( cuts[leaf][0] );
      }
    }

    return count;
  }

  uint32_t cut_edge_ref( cut_t const& cut )
  {
    uint32_t count = cut.size();

    for ( auto leaf : cut )
    {
      if ( ntk.is_ci( ntk.index_to_node( leaf ) ) || ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }

      /* Recursive referencing if leaf was not referenced */
      if ( node_match[leaf].map_refs++ == 0u )
      {
        count += cut_edge_ref( cuts[leaf][0] );
      }
    }
    return count;
  }

  uint32_t cut_edge_deref( cut_t const& cut )
  {
    uint32_t count = cut.size();

    for ( auto leaf : cut )
    {
      if ( ntk.is_ci( ntk.index_to_node( leaf ) ) || ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }

      /* Recursive referencing if leaf was not referenced */
      if ( --node_match[leaf].map_refs == 0u )
      {
        count += cut_edge_deref( cuts[leaf][0] );
      }
    }
    return count;
  }

  void mark_cut_volume_rec( node const& n )
  {
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;

    ntk.set_visited( n, ntk.trav_id() );

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      mark_cut_volume_rec( ntk.get_node( f ) );
    } );
  }

  /* compute positions of leave indices in cut `sub` (subset) with respect to
   * leaves in cut `sup` (super set).
   *
   * Example:
   *   compute_truth_table_support( {1, 3, 6}, {0, 1, 2, 3, 6, 7} ) = {1, 3, 4}
   */
  void compute_truth_table_support( cut_t const& sub, cut_t const& sup, TT& tt )
  {
    std::vector<uint8_t> support( sub.size() );

    size_t j = 0;
    auto itp = sup.begin();
    for ( auto i : sub )
    {
      itp = std::find( itp, sup.end(), i );
      support[j++] = static_cast<uint8_t>( std::distance( sup.begin(), itp ) );
    }

    /* swap variables in the truth table */
    for ( int i = j - 1; i >= 0; --i )
    {
      assert( i <= support[i] );
      kitty::swap_inplace( tt, i, support[i] );
    }
  }

  template<bool ELA>
  void compute_cut_data( cut_t& cut, node const& n, bool recompute_cut_cost )
  {
    uint32_t lut_area = 0;
    uint32_t lut_delay = 0;

    if ( recompute_cut_cost )
    {
      cut->data.ignore = false;
      if constexpr ( StoreFunction )
      {
        if ( ps.sop_balancing || ps.esop_balancing )
        {
          compute_isop( cut );
        }
        else
        {
          if constexpr ( !std::is_same<LUTCostFn, lut_unitary_cost>::value )
          {
            if ( auto it = truth_tables_cost.find( cut->func_id ); it != truth_tables_cost.end() )
            {
              std::tie( lut_area, lut_delay ) = it->second;
            }
            else
            {
              auto cost = lut_cost( truth_tables[cut->func_id] );
              if ( truth_tables[cut->func_id].num_vars() <= ps.cost_cache_vars )
              {
                /* cache it */
                truth_tables_cost[cut->func_id] = cost;
              }
              lut_area = cost.first;
              lut_delay = cost.second;
            }
          }
          else
          {
            std::tie( lut_area, lut_delay ) = lut_cost( truth_tables[cut->func_id] );
          }
        }
      }
      else
      {
        std::tie( lut_area, lut_delay ) = lut_cost( cut.size() );
      }
    }
    else
    {
      lut_area = cut->data.lut_area;
      lut_delay = cut->data.lut_delay;

      if constexpr ( StoreFunction )
      {
        if ( ps.sop_balancing || ps.esop_balancing )
        {
          /* reset fields to be recomputed */
          cut->data.lut_area = 0;
          cut->data.lut_delay = 0;
        }
      }
    }

    if constexpr ( ELA )
    {
      uint32_t delay{ 0 };
      for ( auto leaf : cut )
      {
        const auto& best_leaf_cut = cuts[leaf][0];
        delay = std::max( delay, best_leaf_cut->data.delay );
      }

      cut->data.delay = lut_delay + delay;
      cut->data.lut_area = lut_area;
      cut->data.lut_delay = lut_delay;
      if ( ps.edge_optimization )
      {
        cut->data.area_flow = static_cast<float>( cut_ref( cut ) );
        cut->data.edge_flow = static_cast<float>( cut_edge_deref( cut ) );
      }
      else
      {
        cut->data.area_flow = static_cast<float>( cut_measure_mffc( cut ) );
        cut->data.edge_flow = 0;
      }
    }
    else
    {
      uint32_t delay{ 0 };

      float area_flow = static_cast<float>( lut_area );
      float edge_flow = cut.size();

      for ( auto leaf : cut )
      {
        const auto& best_leaf_cut = cuts[leaf][0];
        delay = std::max( delay, best_leaf_cut->data.delay );
        if ( node_match[leaf].map_refs > 0 && leaf != 0 )
        {
          area_flow += best_leaf_cut->data.area_flow / node_match[leaf].est_refs;
          edge_flow += best_leaf_cut->data.edge_flow / node_match[leaf].est_refs;
        }
        else
        {
          area_flow += best_leaf_cut->data.area_flow;
          edge_flow += best_leaf_cut->data.edge_flow;
        }
      }

      cut->data.delay = lut_delay + delay;
      cut->data.lut_area = lut_area;
      cut->data.lut_delay = lut_delay;
      cut->data.area_flow = area_flow;
      cut->data.edge_flow = edge_flow;
    }

    if constexpr ( StoreFunction )
    {
      if ( ps.sop_balancing || ps.esop_balancing )
      {
        /* compute delay and area */
        compute_balancing_cost( cut );
      }
    }
  }

  void compute_cut_data_share( cut_t& cut )
  {
    uint32_t delay{ 0 };
    float area_flow = static_cast<float>( cut->data.lut_area );
    float edge_flow = cut.size();

    for ( auto leaf : cut )
    {
      const auto& best_leaf_cut = cuts[leaf][0];
      delay = std::max( delay, best_leaf_cut->data.delay );
      /* flow contribution is added only for not shared leaves */
      if ( node_match[leaf].map_refs == 0 && leaf != 0 )
      {
        area_flow += best_leaf_cut->data.area_flow / node_match[leaf].est_refs;
        edge_flow += best_leaf_cut->data.edge_flow / node_match[leaf].est_refs;
      }
    }

    cut->data.delay = cut->data.lut_delay + delay;
    cut->data.area_flow = area_flow;
    cut->data.edge_flow = edge_flow;
  }

  void add_zero_cut( uint32_t index, bool phase )
  {
    auto& cut = cuts[index].add_cut( &index, &index ); /* fake iterator for emptyness */

    if constexpr ( StoreFunction )
    {
      if ( phase )
        cut->func_id = 1;
      else
        cut->func_id = 0;
    }
  }

  void add_unit_cut( uint32_t index )
  {
    auto& cut = cuts[index].add_cut( &index, &index + 1 );

    if constexpr ( StoreFunction )
    {
      cut->func_id = 2;
    }
  }

  inline bool fast_support_minimization( TT& tt, cut_t& res )
  {
    uint32_t support = 0u;
    uint32_t support_size = 0u;
    for ( uint32_t i = 0u; i < tt.num_vars(); ++i )
    {
      if ( kitty::has_var( tt, i ) )
      {
        support |= 1u << i;
        ++support_size;
      }
    }

    /* has not minimized support? */
    if ( ( support & ( support + 1u ) ) != 0u )
    {
      return false;
    }

    /* variables not in the support are the most significative */
    if ( support_size != res.size() )
    {
      std::vector<uint32_t> leaves( res.begin(), res.begin() + support_size );
      res.set_leaves( leaves.begin(), leaves.end() );
      tt = kitty::shrink_to( tt, support_size );
    }

    return true;
  }

  uint32_t compute_truth_table( uint32_t index, std::vector<cut_t const*> const& vcuts, cut_t& res )
  {
    // stopwatch t( st.cut_enumeration_st.time_truth_table ); /* runtime optimized */

    std::vector<TT> tt( vcuts.size() );
    auto i = 0;
    for ( auto const& cut : vcuts )
    {
      tt[i] = kitty::extend_to( truth_tables[( *cut )->func_id], res.size() );
      compute_truth_table_support( *cut, res, tt[i] );
      ++i;
    }

    auto tt_res = ntk.compute( ntk.index_to_node( index ), tt.begin(), tt.end() );

    if ( ps.cut_enumeration_ps.minimize_truth_table && !fast_support_minimization( tt_res, res ) )
    {
      const auto support = kitty::min_base_inplace( tt_res );
      if ( support.size() != res.size() )
      {
        auto tt_res_shrink = shrink_to( tt_res, static_cast<unsigned>( support.size() ) );
        std::vector<uint32_t> leaves_before( res.begin(), res.end() );
        std::vector<uint32_t> leaves_after( support.size() );

        auto it_support = support.begin();
        auto it_leaves = leaves_after.begin();
        while ( it_support != support.end() )
        {
          *it_leaves++ = leaves_before[*it_support++];
        }
        res.set_leaves( leaves_after.begin(), leaves_after.end() );
        return truth_tables.insert( tt_res_shrink );
      }
    }

    return truth_tables.insert( tt_res );
  }

  void compute_mffcs_mapping()
  {
    ntk.clear_mapping();

    /* map POs */
    ntk.foreach_co( [&]( auto const& f ) {
      node const& n = ntk.get_node( f );
      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
        return;

      compute_mffc_mapping_node( n );
    } );

    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      node const& n = *it;
      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
        continue;
      if ( ntk.fanout_size( n ) <= 1 ) /* it should be unnecessary */
        continue;

      /* create MFFC cut */
      compute_mffc_mapping_node( n );
    }

    st.area = area;
    st.delay = delay;
    st.edges = edges;

    {
      std::stringstream stats;
      stats << fmt::format( "[i] Area MFFC: Delay = {:8d}  Area = {:8d}  Edges = {:8d}  Cuts = {:8d}\n", delay, area, edges, cuts_total );
      st.round_stats.push_back( stats.str() );
    }
  }

  void compute_mffc_mapping_node( node const& n )
  {
    uint32_t lut_area, lut_delay;

    /* create FC cut */
    std::vector<node> inner, leaves;
    ntk.incr_trav_id();
    get_fc_nodes_rec( n, inner );

    /* extract leaves */
    for ( auto const& g : inner )
    {
      ntk.foreach_fanin( g, [&]( auto const& f ) {
        if ( ntk.visited( ntk.get_node( f ) ) != ntk.trav_id() && !ntk.is_constant( ntk.get_node( f ) ) )
        {
          leaves.push_back( ntk.get_node( f ) );
          ntk.set_visited( ntk.get_node( f ), ntk.trav_id() );
        }
      } );
    }

    /* sort leaves in topo order */
    std::stable_sort( leaves.begin(), leaves.end() );

    ntk.add_to_mapping( n, leaves.begin(), leaves.end() );

    delay = std::max( delay, static_cast<uint32_t>( leaves.size() ) );

    if constexpr ( StoreFunction )
    {
      default_simulator<TT> sim( leaves.size() );
      unordered_node_map<TT, Ntk> node_to_value( ntk );

      /* populate simulation values for constants */
      node_to_value[ntk.get_node( ntk.get_constant( false ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( false ) ) ) );
      if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
      {
        node_to_value[ntk.get_node( ntk.get_constant( true ) )] = sim.compute_constant( ntk.constant_value( ntk.get_node( ntk.get_constant( true ) ) ) );
      }

      /* populate simulation values for leaves */
      uint32_t i = 0u;
      for ( auto const& g : leaves )
      {
        node_to_value[g] = sim.compute_pi( i++ );
      }

      /* simulate recursively */
      simulate_fc_rec( n, node_to_value );

      ntk.set_cell_function( n, node_to_value[n] );

      std::tie( lut_area, lut_delay ) = lut_cost( node_to_value[n] );
    }
    else
    {
      std::tie( lut_area, lut_delay ) = lut_cost( leaves.size() );
    }

    area += lut_area;
  }

  void get_fc_nodes_rec( node const& n, std::vector<node>& nodes )
  {
    if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
      return;

    nodes.push_back( n );
    ntk.set_visited( n, ntk.trav_id() );

    /* expand cut for single fanout nodes */
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      auto g = ntk.get_node( f );
      if ( ntk.fanout_size( g ) == 1 )
      {
        get_fc_nodes_rec( g, nodes );
      }
    } );
  }

  void simulate_fc_rec( node const& n, unordered_node_map<TT, Ntk>& node_to_value )
  {
    std::vector<TT> fanin_values( ntk.fanin_size( n ) );

    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      if ( !node_to_value.has( ntk.get_node( f ) ) )
      {
        simulate_fc_rec( ntk.get_node( f ), node_to_value );
      }

      fanin_values[i] = node_to_value[ntk.get_node( f )];
    } );

    node_to_value[n] = ntk.compute( n, fanin_values.begin(), fanin_values.end() );
  }

#pragma region Dump network
  klut_network create_lut_network()
  {
    /* specialized method: does not support buffer/inverter sweeping */
    if ( StoreFunction && ps.cut_enumeration_ps.minimize_truth_table )
    {
      return create_lut_network_mapped();
    }

    klut_network res;
    node_map<signal<klut_network>, Ntk> node_to_signal( ntk );

    node_map<driver_type, Ntk> node_driver_type( ntk, driver_type::none );

    /* opposites are filled for nodes with mixed driver types, since they have
       two nodes in the network. */
    std::unordered_map<node, signal<klut_network>> opposites;

    /* initial driver types */
    ntk.foreach_co( [&]( auto const& f ) {
      switch ( node_driver_type[f] )
      {
      case driver_type::none:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::pos;
        break;
      case driver_type::pos:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::mixed : driver_type::pos;
        break;
      case driver_type::neg:
        node_driver_type[f] = ntk.is_complemented( f ) ? driver_type::neg : driver_type::mixed;
        break;
      case driver_type::mixed:
      default:
        break;
      }
    } );

    /* constants */
    auto add_constant_to_map = [&]( bool value ) {
      const auto n = ntk.get_node( ntk.get_constant( value ) );
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = res.get_constant( value );
        break;

      case driver_type::neg:
        node_to_signal[n] = res.get_constant( !value );
        break;

      case driver_type::mixed:
        node_to_signal[n] = res.get_constant( value );
        opposites[n] = res.get_constant( !value );
        break;
      }
    };

    add_constant_to_map( false );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    {
      add_constant_to_map( true );
    }

    /* primary inputs */
    ntk.foreach_pi( [&]( auto n ) {
      signal<klut_network> res_signal;
      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        res_signal = res.create_pi();
        node_to_signal[n] = res_signal;
        break;

      case driver_type::neg:
        res_signal = res.create_pi();
        node_to_signal[n] = res.create_not( res_signal );
        break;

      case driver_type::mixed:
        res_signal = res.create_pi();
        node_to_signal[n] = res_signal;
        opposites[n] = res.create_not( node_to_signal[n] );
        break;
      }
    } );

    /* TODO: add sequential compatibility */
    edges = 0;
    for ( auto const& n : topo_order )
    {
      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
        continue;

      const auto index = ntk.node_to_index( n );
      if ( node_match[index].map_refs == 0 )
        continue;

      auto const& best_cut = cuts[index][0];

      kitty::dynamic_truth_table tt;
      std::vector<signal<klut_network>> children;
      std::tie( tt, children ) = create_lut( n, node_to_signal, node_driver_type );
      edges += children.size();

      switch ( node_driver_type[n] )
      {
      default:
      case driver_type::none:
      case driver_type::pos:
        node_to_signal[n] = res.create_node( children, tt );
        break;

      case driver_type::neg:
        node_to_signal[n] = res.create_node( children, ~tt );
        break;

      case driver_type::mixed:
        node_to_signal[n] = res.create_node( children, tt );
        opposites[n] = res.create_node( children, ~tt );
        edges += children.size();
        break;
      }
    }

    /* outputs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) && node_driver_type[f] == driver_type::mixed )
        res.create_po( opposites[ntk.get_node( f )] );
      else
        res.create_po( node_to_signal[f] );
    } );

    st.area = area;
    st.delay = delay;
    st.edges = edges;

    return res;
  }

  klut_network create_lut_network_mapped()
  {
    klut_network res;
    mapping_view<Ntk, true> mapping_ntk{ ntk };

    /* load mapping info */
    for ( auto const& n : topo_order )
    {
      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
        continue;

      const auto index = ntk.node_to_index( n );
      if ( node_match[index].map_refs == 0 )
        continue;

      std::vector<node> nodes;
      auto const& best_cut = cuts[index][0];

      for ( auto const& l : best_cut )
      {
        nodes.push_back( ntk.index_to_node( l ) );
      }
      mapping_ntk.add_to_mapping( n, nodes.begin(), nodes.end() );

      if constexpr ( StoreFunction )
      {
        mapping_ntk.set_cell_function( n, truth_tables[best_cut->func_id] );
      }
    }

    /* generate mapped network */
    collapse_mapped_network( res, mapping_ntk );

    st.area = area;
    st.delay = delay;
    st.edges = edges;

    return res;
  }

  inline lut_info create_lut( node const& n, node_map<signal<klut_network>, Ntk>& node_to_signal, node_map<driver_type, Ntk> const& node_driver_type )
  {
    auto const& best_cut = cuts[ntk.node_to_index( n )][0];

    std::vector<signal<klut_network>> children;
    for ( auto const& l : best_cut )
    {
      children.push_back( node_to_signal[ntk.index_to_node( l )] );
    }

    /* recursively compute the function for each choice until success */
    ntk.incr_trav_id();
    unordered_node_map<kitty::dynamic_truth_table, Ntk> node_to_value( ntk );

    /* add constants */
    node_to_value[ntk.get_node( ntk.get_constant( false ) )] = kitty::dynamic_truth_table( best_cut.size() );
    ntk.set_visited( ntk.get_node( ntk.get_constant( false ) ), ntk.trav_id() );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    {
      node_to_value[ntk.get_node( ntk.get_constant( true ) )] = ~kitty::dynamic_truth_table( best_cut.size() );
      ntk.set_visited( ntk.get_node( ntk.get_constant( true ) ), ntk.trav_id() );
    }

    /* add leaves */
    uint32_t ctr = 0;
    for ( uint32_t leaf : best_cut )
    {
      kitty::dynamic_truth_table tt_leaf( best_cut.size() );
      kitty::create_nth_var( tt_leaf, ctr++, node_driver_type[ntk.index_to_node( leaf )] == driver_type::neg );
      node_to_value[ntk.index_to_node( leaf )] = tt_leaf;
      ntk.set_visited( ntk.index_to_node( leaf ), ntk.trav_id() );
    }

    /* recursively compute the function */
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      compute_function_rec( ntk.get_node( f ), node_to_value );
    } );

    std::vector<kitty::dynamic_truth_table> tts;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      tts.push_back( node_to_value[ntk.get_node( f )] );
    } );
    TT tt = ntk.compute( n, tts.begin(), tts.end() );

    minimize_support( tt, children );

    return { tt, children };
  }

  void compute_function_rec( node const& n, unordered_node_map<kitty::dynamic_truth_table, Ntk>& node_to_value )
  {
    if ( ntk.visited( n ) == ntk.trav_id() )
    {
      assert( node_to_value.has( n ) );
      return;
    }

    assert( !ntk.is_ci( n ) );
    ntk.set_visited( n, ntk.trav_id() );

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      compute_function_rec( ntk.get_node( f ), node_to_value );
    } );

    /* compute the function */
    std::vector<kitty::dynamic_truth_table> tts;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      tts.push_back( node_to_value[ntk.get_node( f )] );
    } );

    node_to_value[n] = ntk.compute( n, tts.begin(), tts.end() );
  }

  void derive_mapping()
  {
    ntk.clear_mapping();

    for ( auto const& n : topo_order )
    {
      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
        continue;

      const auto index = ntk.node_to_index( n );
      if ( node_match[index].map_refs == 0 )
        continue;

      std::vector<node> nodes;
      auto const& best_cut = cuts[index][0];

      for ( auto const& l : best_cut )
      {
        nodes.push_back( ntk.index_to_node( l ) );
      }
      ntk.add_to_mapping( n, nodes.begin(), nodes.end() );

      if constexpr ( StoreFunction )
      {
        ntk.set_cell_function( n, truth_tables[best_cut->func_id] );
      }
    }

    st.area = area;
    st.delay = delay;
    st.edges = edges;
  }

  void minimize_support( TT& tt, std::vector<signal<klut_network>>& children )
  {
    uint32_t support = 0u;
    uint32_t support_size = 0u;
    for ( uint32_t i = 0u; i < tt.num_vars(); ++i )
    {
      if ( kitty::has_var( tt, i ) )
      {
        support |= 1u << i;
        ++support_size;
      }
    }

    /* variables not in the support are the most significative */
    if ( ( support & ( support + 1u ) ) == 0u )
    {
      if ( support_size != children.size() )
      {
        children.erase( children.begin() + support_size, children.end() );
        tt = kitty::shrink_to( tt, support_size );
      }

      return;
    }

    /* vacuous variables */
    const auto support_vector = kitty::min_base_inplace( tt );
    assert( support_vector.size() != children.size() );

    auto tt_shrink = shrink_to( tt, support_size );
    std::vector<signal<klut_network>> children_support( support_size );

    auto it_support = support_vector.begin();
    auto it_children = children_support.begin();
    while ( it_support != support_vector.end() )
    {
      *it_children++ = children[*it_support++];
    }

    children = std::move( children_support );
  }
#pragma endregion

#pragma region balancing
  void compute_isop( cut_t& cut, bool both_phases = true )
  {
    uint32_t func_id = cut->func_id >> 1;

    if ( func_id < isops.size() )
    {
      auto const& sop = isops[func_id];
      if ( sop.size() > max_cubes )
      {
        cut->data.ignore = true;
      }
      return;
    }

    assert( func_id == isops.size() );

    sop_t sop, sop_n;
    if ( ps.sop_balancing )
    {
      sop = kitty::isop( truth_tables[func_id << 1u] );
    }
    else
    {
      sop = exorcism( truth_tables[func_id << 1u] );
    }

    if ( both_phases )
    {
      sop_t n_sop;
      if ( ps.sop_balancing )
      {
        n_sop = kitty::isop( ~truth_tables[func_id << 1u] );
      }
      else
      {
        n_sop = exorcism( ~truth_tables[func_id << 1u] );
      }

      if ( n_sop.size() < sop.size() )
      {
        sop.swap( n_sop );
      }
      else if ( n_sop.size() == sop.size() )
      {
        /* compute literal cost */
        uint32_t lit = 0, n_lit = 0;
        for ( auto const& c : sop )
        {
          lit += c.num_literals();
        }
        for ( auto const& c : n_sop )
        {
          n_lit += c.num_literals();
        }

        if ( n_lit < lit )
        {
          sop.swap( n_sop );
        }
      }
    }

    /* check size of SOP < max_cubes */
    if ( sop.size() > max_cubes )
    {
      cut->data.ignore = true;
    }

    isops.push_back( sop );
    return;
  }

  void compute_balancing_cost( cut_t& cut )
  {
    uint32_t decomposition_size = 0;
    uint32_t decomposition_delay = 0;

    auto const& sop = isops[cut->func_id >> 1];

    if ( cut->data.ignore || sop.size() > max_cubes )
      return;

    /* specific case size = 0 or = 1 */
    if ( cut.size() < 2 )
      return;

    /* collect arrival times for fanin */
    std::array<uint32_t, max_cut_size> arrival_pin;
    unsigned i = 0;
    for ( auto l : cut )
      arrival_pin[i++] = cuts[l].best()->data.delay;

    cubes_queue_t terms;

    /* get terms delay */
    assert( sop.size() <= max_cubes );
    for ( kitty::cube const& c : sop )
    {
      cubes_queue_t lits;
      for ( i = 0; i < cut.size(); ++i )
      {
        if ( c.get_mask( i ) )
        {
          lits.push( arrival_pin[i] );
        }
      }

      if ( lits.size() == 0 )
        continue;

      decomposition_size += lits.size() - 1;
      terms.push( compute_balancing_cost_term( lits ) );
    }

    assert( terms.size() > 0 );

    decomposition_size += terms.size() - 1;
    decomposition_delay = compute_balancing_cost_term( terms );

    cut->data.delay = decomposition_delay;
    cut->data.lut_area = decomposition_size;
    cut->data.lut_delay = 1; /* not used */
    cut->data.area_flow += decomposition_size;
    /* edge flow not used */
  }

  inline uint32_t compute_balancing_cost_term( cubes_queue_t& terms )
  {
    while ( terms.size() != 1 )
    {
      uint32_t l0 = terms.top();
      terms.pop();
      uint32_t l1 = terms.top();
      terms.pop();
      terms.push( std::max( l0, l1 ) + 1 );
    }

    return terms.top();
  }

  void compute_balancing_cost_required( uint32_t index )
  {
    cut_t const& cut = cuts[index][0];

    if ( cut.size() == 0 )
      return;

    /* propagate unit delay back */
    if ( cut.size() == 1 )
    {
      for ( auto l : cut )
      {
        node_match[l].required = std::min( node_match[l].required, node_match[index].required );
      }
    }

    /* collect arrival times for fanin */
    std::array<std::pair<uint16_t, uint16_t>, max_sop_decomp_size> connections;
    std::array<uint32_t, max_cut_size> arrival_pin;
    std::array<uint32_t, max_sop_decomp_size> required;
    unsigned size = 0;
    for ( auto l : cut )
    {
      arrival_pin[size] = cuts[l].best()->data.delay;
      connections[size] = std::make_pair( size, size );
      ++size;
    }

    auto priority_cmp = []( std::pair<uint32_t, uint16_t> const& a, std::pair<uint32_t, uint16_t> const& b ) { return a.first > b.first; };
    using cubes_queue2_t = std::priority_queue<std::pair<uint32_t, uint16_t>, std::vector<std::pair<uint32_t, uint16_t>>, decltype( priority_cmp )>;

    cubes_queue2_t terms( priority_cmp );

    /* get terms delay */
    auto const& sop = isops[cut->func_id >> 1];
    assert( sop.size() <= max_cubes );
    for ( kitty::cube const& c : sop )
    {
      cubes_queue2_t lits( priority_cmp );
      for ( auto i = 0; i < cut.size(); ++i )
      {
        if ( c.get_mask( i ) )
        {
          lits.push( { arrival_pin[i], i } );
        }
      }

      if ( lits.size() == 0 )
        continue;

      compute_balancing_cost_required_term( lits, connections, size );
      assert( size <= connections.size() );

      terms.push( lits.top() );
    }

    assert( terms.size() > 0 );
    compute_balancing_cost_required_term( terms, connections, size );

    uint32_t required_node = node_match[index].required;
    assert( terms.top().first == cut->data.delay );
    assert( required_node >= terms.top().first );

    /* init required times */
    for ( auto i = 0; i < size - 1; ++i )
      required[i] = UINT32_MAX;
    required[size - 1] = required_node;

    compute_balancing_cost_propagate_required( connections, required, size, cut.size() );

    /* assign required time */
    uint32_t ctr = 0;
    for ( auto l : cut )
    {
      node_match[l].required = std::min( node_match[l].required, required[ctr++] );
      assert( node_match[l].required >= cuts[l][0]->data.delay );
    }
  }

  template<typename Queue>
  inline void compute_balancing_cost_required_term( Queue& terms, std::array<std::pair<uint16_t, uint16_t>, max_sop_decomp_size>& connections, uint32_t& size )
  {
    while ( terms.size() != 1 )
    {
      std::pair<uint32_t, uint16_t> l0 = terms.top();
      terms.pop();
      std::pair<uint32_t, uint16_t> l1 = terms.top();
      terms.pop();
      uint32_t arrival = std::max( l0.first, l1.first ) + 1;
      terms.push( { arrival, size } );
      connections[size] = std::make_pair( l0.second, l1.second );
      ++size;
    }
  }

  inline void compute_balancing_cost_propagate_required( std::array<std::pair<uint16_t, uint16_t>, max_sop_decomp_size> const& connections, std::array<uint32_t, max_sop_decomp_size>& required, uint32_t size, uint32_t leaves )
  {
    for ( auto i = size - 1; i >= leaves; --i )
    {
      uint32_t time = required[i] - 1;
      required[connections[i].first] = std::min( time, required[connections[i].first] );
      required[connections[i].second] = std::min( time, required[connections[i].second] );
    }
  }
#pragma endregion

private:
  Ntk& ntk;
  lut_map_params const& ps;
  lut_map_stats& st;

  uint32_t iteration{ 0 };       /* current mapping iteration */
  uint32_t area_iteration{ 0 };  /* current area iteration */
  uint32_t delay{ 0 };           /* current delay of the mapping */
  uint32_t area{ 0 };            /* current area of the mapping */
  uint32_t edges{ 0 };           /* current edges of the mapping */
  uint32_t cuts_total{ 0 };      /* current computed cuts */
  const float epsilon{ 0.005f }; /* epsilon */
  LUTCostFn lut_cost{};

  std::vector<node> topo_order;
  std::vector<uint32_t> tmp_visited;
  std::vector<node_lut> node_match;

  std::vector<cut_set_t> cuts;  /* compressed representation of cuts */
  cut_merge_t lcuts;            /* cut merger container */
  tt_cache truth_tables;        /* cut truth tables */
  cost_cache truth_tables_cost; /* truth tables cost */
  isop_cache isops;             /* cache for isops */
};
#pragma endregion

} /* namespace detail */

/*! \brief LUT mapper.
 *
 * This function implements a LUT mapping algorithm.  It is controlled by one
 * template argument `ComputeTruth` (defaulted to `false`) which controls
 * whether the LUT function is computed or the mapping is structural. In the
 * former case, truth tables are computed during cut enumeration,
 * which requires more runtime.
 *
 * This function returns a k-LUT network.
 *
 * The template `LUTCostFn` sets the cost function to evaluate depth and
 * size of a truth table given its support size if `ComputeTruth` is set
 * to false, or its function if `ComputeTruth` is set to true.
 *
 * This implementation offers more options such as delay oriented mapping
 * and edges minimization compared to the command `lut_mapping`.
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
 */
template<class Ntk, bool ComputeTruth = false, class LUTCostFn = lut_unitary_cost>
klut_network lut_map( Ntk& ntk, lut_map_params ps = {}, lut_map_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_ci_v<Ntk>, "Ntk does not implement the foreach_ci method" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  lut_map_params tps = ps;
  lut_map_stats st;
  klut_network klut;

  /* adjust params for balancing */
  if ( ps.sop_balancing || ps.esop_balancing )
  {
    tps.area_oriented_mapping = false;
    tps.recompute_cuts = false;
    tps.area_share_rounds = 0;
    tps.edge_optimization = false;
    tps.cut_expansion = false;
  }

  detail::lut_map_impl<Ntk, ComputeTruth, LUTCostFn> p( ntk, tps, st );
  klut = p.run();

  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst != nullptr )
  {
    *pst = st;
  }

  return klut;
}

/*! \brief LUT mapper inplace.
 *
 * This function implements a LUT mapping algorithm.  It is controlled by one
 * template argument `StoreFunction` (defaulted to `false`) which controls
 * whether the LUT function is stored in the mapping. In that case
 * truth tables are computed during cut enumeration, which requires more
 * runtime.
 *
 * The input network must be wrapped in a `mapping_view`. The computed mapping
 * is stored in the view. In this version, some features of the mapper are
 * disabled, such as on-the-fly decompositions, due to incompatibility.
 *
 * The template `LUTCostFn` sets the cost function to evaluate depth and
 * size of a truth table given its support size, if `StoreFunction` is set
 * to false, or its function, if `StoreFunction` is set to true.
 *
 * This implementation offers more options such as delay oriented mapping
 * and edges minimization compared to the command `lut_mapping`.
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
 *
   \verbatim embed:rst

   .. note::

      The implementation of this algorithm was inspired by the LUT
      mapping command ``&if`` in ABC.
   \endverbatim
 */
template<class Ntk, bool StoreFunction = false, class LUTCostFn = lut_unitary_cost>
void lut_map_inplace( Ntk& ntk, lut_map_params const& ps = {}, lut_map_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_ci_v<Ntk>, "Ntk does not implement the foreach_ci method" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_clear_mapping_v<Ntk>, "Ntk does not implement the clear_mapping method" );
  static_assert( has_add_to_mapping_v<Ntk>, "Ntk does not implement the add_to_mapping method" );

  lut_map_params tps = ps;
  lut_map_stats st;

  /* adjust params for balancing */
  if ( ps.sop_balancing || ps.esop_balancing )
  {
    tps.area_oriented_mapping = false;
    tps.recompute_cuts = false;
    tps.area_share_rounds = 0;
    tps.edge_optimization = false;
    tps.cut_expansion = false;
  }

  detail::lut_map_impl<Ntk, StoreFunction, LUTCostFn> p( ntk, tps, st );
  p.run_inplace();

  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst != nullptr )
  {
    *pst = st;
  }
}

} /* namespace mockturtle */