/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2024  EPFL
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
  \file emap.hpp
  \brief An extended technology mapper

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/static_truth_table.hpp>

#include <fmt/format.h>
#include <parallel_hashmap/phmap.h>

#include "../networks/aig.hpp"
#include "../networks/block.hpp"
#include "../networks/klut.hpp"
#include "../utils/cuts.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/tech_library.hpp"
#include "../views/binding_view.hpp"
#include "../views/cell_view.hpp"
#include "../views/choice_view.hpp"
#include "../views/topo_view.hpp"
#include "cleanup.hpp"
#include "cut_enumeration.hpp"
#include "detail/mffc_utils.hpp"
#include "detail/switching_activity.hpp"

namespace mockturtle
{

/*! \brief Parameters for emap.
 *
 * The data structure `emap_params` holds configurable parameters
 * with default arguments for `emap`.
 */
struct emap_params
{
  emap_params()
  {
    cut_enumeration_ps.cut_limit = 16;
    cut_enumeration_ps.minimize_truth_table = true;
  }

  /*! \brief Parameters for cut enumeration
   *
   * The default cut limit is 16.
   * The maximum cut limit is 19.
   * By default, truth table minimization
   * is performed.
   */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Do area-oriented mapping. */
  bool area_oriented_mapping{ false };

  /*! \brief Maps using multi-output gates */
  bool map_multioutput{ false };

  /*! \brief Matching mode
   *
   * Boolean uses Boolean matching (up to 6-input cells),
   * Structural uses pattern matching for fully-DSD cells,
   * Hybrid combines the two.
   */
  enum matching_mode_t
  {
    boolean,
    structural,
    hybrid
  } matching_mode = hybrid;

  /*! \brief Target required time (for each PO). */
  double required_time{ 0.0f };

  /*! \brief Required time relaxation in percentage (10 = 10%). */
  double relax_required{ 0.0f };

  /*! \brief Custom input arrival times. */
  std::vector<double> arrival_times{};

  /*! \brief Custom output required times. */
  std::vector<double> required_times{};

  /*! \brief Number of rounds for area flow optimization. */
  uint32_t area_flow_rounds{ 3u };

  /*! \brief Number of rounds for exact area optimization. */
  uint32_t ela_rounds{ 2u };

  /*! \brief Number of rounds for exact switching power optimization. */
  uint32_t eswp_rounds{ 0u };

  /*! \brief Number of patterns for switching activity computation. */
  uint32_t switching_activity_patterns{ 2048u };

  /*! \brief Compute area-oriented alternative matches */
  bool use_match_alternatives{ true };

  /*! \brief Remove the cuts that are contained in others */
  bool remove_dominated_cuts{ false };

  /*! \brief Remove overlapping multi-output cuts */
  bool remove_overlapping_multicuts{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

/*! \brief Statistics for emap.
 *
 * The data structure `emap_stats` provides data collected by running
 * `emap`.
 */
struct emap_stats
{
  /*! \brief Area result. */
  double area{ 0 };
  /*! \brief Worst delay result. */
  double delay{ 0 };
  /*! \brief Power result. */
  double power{ 0 };
  /*! \brief Power result. */
  uint32_t inverters{ 0 };

  /*! \brief Mapped multi-output gates. */
  uint32_t multioutput_gates{ 0 };

  /*! \brief Runtime for multi-output matching. */
  stopwatch<>::duration time_multioutput{ 0 };
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Cut enumeration stats. */
  cut_enumeration_stats cut_enumeration_st{};

  /*! \brief Delay and area stats for each round. */
  std::vector<std::string> round_stats{};

  /*! \brief Mapping error. */
  bool mapping_error{ false };

  void report() const
  {
    for ( auto const& stat : round_stats )
    {
      std::cout << stat;
    }
    std::cout << fmt::format( "[i] Area = {:>5.2f}; Delay = {:>5.2f};", area, delay );
    if ( power != 0 )
      std::cout << fmt::format( " Power = {:>5.2f};\n", power );
    else
      std::cout << "\n";
    if ( multioutput_gates )
    {
      std::cout << fmt::format( "[i] Multi-output gates   = {:>5}\n", multioutput_gates );
      std::cout << fmt::format( "[i] Multi-output runtime = {:>5.2f} secs\n", to_seconds( time_multioutput ) );
    }
    std::cout << fmt::format( "[i] Total runtime        = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

#pragma region cut set
template<unsigned NInputs>
struct cut_enumeration_emap_cut
{
  /* stats */
  uint32_t delay;
  float flow;
  bool ignore;

  /* pattern index for structural matching*/
  uint32_t pattern_index;

  /* function */
  kitty::static_truth_table<6> function;

  /* list of supergates matching the cut for positive and negative output phases */
  std::array<std::vector<supergate<NInputs>> const*, 2> supergates;
  /* input negations, 0: pos, 1: neg */
  std::array<uint16_t, 2> negations;
};

struct cut_enumeration_emap_multi_cut
{
  /* stats */
  uint64_t id{ 0 };
};

enum class emap_cut_sort_type
{
  DELAY = 0,
  DELAY2 = 1,
  AREA = 2,
  AREA2 = 3,
  NONE = 4
};

template<typename CutType, uint32_t MaxCuts>
class emap_cut_set
{
public:
  /*! \brief Standard constructor.
   */
  emap_cut_set()
  {
    clear();
  }

  /*! \brief Assignment operator.
   */
  emap_cut_set& operator=( emap_cut_set const& other )
  {
    if ( this != &other )
    {
      _pcend = _pend = _pcuts.begin();
      _set_limit = other._set_limit;

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

  /*! \brief Sets the cut limit.
   */
  void set_cut_limit( uint32_t limit )
  {
    _set_limit = std::min( MaxCuts, limit );
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

  /*! \brief Appends a cut to the end of the set.
   *
   * This function should only be called to create a set of cuts which is known
   * to be sorted and irredundant (i.e., no cut in the set dominates another
   * cut).
   *
   * \param cut Cut to insert
   */
  void append_cut( CutType const& cut )
  {
    assert( _pend != _pcuts.end() );

    **_pend++ = cut;
    ++_pcend;
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
    if ( !c1->ignore && c2->ignore )
      return true;
    if ( c1->ignore && !c2->ignore )
      return false;
    if ( c1->delay < c2->delay - eps )
      return true;
    if ( c1->delay > c2->delay + eps )
      return false;
    if ( c1->flow < c2->flow - eps )
      return true;
    if ( c1->flow > c2->flow + eps )
      return false;
    return c1.size() < c2.size();
  }

  static bool sort_delay2( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->ignore && c2->ignore )
      return true;
    if ( c1->ignore && !c2->ignore )
      return false;
    if ( c1.size() < c2.size() )
      return true;
    if ( c1.size() > c2.size() )
      return false;
    if ( c1->delay < c2->delay - eps )
      return true;
    if ( c1->delay > c2->delay + eps )
      return false;
    return c1->flow < c2->flow - eps;
  }

  static bool sort_area( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->ignore && c2->ignore )
      return true;
    if ( c1->ignore && !c2->ignore )
      return false;
    if ( c1->flow < c2->flow - eps )
      return true;
    if ( c1->flow > c2->flow + eps )
      return false;
    if ( c1.size() < c2.size() )
      return true;
    if ( c1.size() > c2.size() )
      return false;
    return c1->delay < c2->delay - eps;
  }

  static bool sort_area2( CutType const& c1, CutType const& c2 )
  {
    constexpr auto eps{ 0.005f };
    if ( !c1->ignore && c2->ignore )
      return true;
    if ( c1->ignore && !c2->ignore )
      return false;
    if ( c1->flow < c2->flow - eps )
      return true;
    if ( c1->flow > c2->flow + eps )
      return false;
    if ( c1->delay < c2->delay - eps )
      return true;
    if ( c1->delay > c2->delay + eps )
      return false;
    return c1.size() < c2.size();
  }

  /*! \brief Compare two cuts using sorting functions.
   *
   * This method compares two cuts using a sorting function.
   *
   * \param cut1 first cut.
   * \param cut2 second cut.
   * \param sort sorting function.
   */
  static bool compare( CutType const& cut1, CutType const& cut2, emap_cut_sort_type sort = emap_cut_sort_type::NONE )
  {
    if ( sort == emap_cut_sort_type::DELAY )
    {
      return sort_delay( cut1, cut2 );
    }
    else if ( sort == emap_cut_sort_type::DELAY2 )
    {
      return sort_delay2( cut1, cut2 );
    }
    else if ( sort == emap_cut_sort_type::AREA )
    {
      return sort_area( cut1, cut2 );
    }
    else if ( sort == emap_cut_sort_type::AREA2 )
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
  void simple_insert( CutType const& cut, emap_cut_sort_type sort = emap_cut_sort_type::NONE )
  {
    /* insert cut in a sorted way */
    typename std::array<CutType*, MaxCuts>::iterator ipos = _pcuts.begin();

    bool limit_reached = std::distance( _pcuts.begin(), _pend ) >= _set_limit;

    /* do not insert if worst than set_limit */
    if ( limit_reached )
    {
      if ( sort == emap_cut_sort_type::AREA && !sort_area( cut, **( ipos + _set_limit - 1 ) ) )
      {
        return;
      }
      else if ( sort != emap_cut_sort_type::AREA )
      {
        return;
      }
    }

    if ( sort == emap_cut_sort_type::NONE )
    {
      ipos = _pend;
    }
    else /* AREA */
    {
      ipos = std::upper_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return sort_area( *a, *b ); } );
    }

    /* check for redundant cut */
    typename std::array<CutType*, MaxCuts>::iterator jpos = ipos;
    if ( cut->ignore )
    {
      while ( jpos != _pcuts.begin() )
      {
        --jpos;
        if ( ( *jpos )->size() < cut.size() )
          break;
        if ( ( *jpos )->signature() == cut.signature() && std::equal( cut.begin(), cut.end(), ( *jpos )->begin() ) )
          return;
      }
    }
    else if ( ipos != _pcuts.begin() )
    {
      if ( ( *( ipos - 1 ) )->signature() == cut.signature() && std::equal( cut.begin(), cut.end(), ( *( ipos - 1 ) )->begin() ) )
      {
        return;
      }
    }

    /* too many cuts, we need to remove one */
    if ( _pend == _pcuts.end() || limit_reached )
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
  void insert( CutType const& cut, bool skip0 = false, emap_cut_sort_type sort = emap_cut_sort_type::NONE )
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
  auto const& best() const { return *_pcuts[0]; }

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
  friend std::ostream& operator<<( std::ostream& os, emap_cut_set const& set )
  {
    for ( auto const& c : set )
    {
      os << *c << "\n";
    }
    return os;
  }

  /*! \brief Returns if the cut set contains already `cut`. */
  bool is_contained( CutType const& cut )
  {
    typename std::array<CutType*, MaxCuts>::iterator ipos = _pcuts.begin();

    while ( ipos != _pend )
    {
      if ( ( *ipos )->signature() == cut.signature() && std::equal( cut.begin(), cut.end(), ( *ipos )->begin() ) )
        return true;
      ++ipos;
    }

    return false;
  }

private:
  std::array<CutType, MaxCuts> _cuts;
  std::array<CutType*, MaxCuts> _pcuts;
  typename std::array<CutType*, MaxCuts>::const_iterator _pcend{ _pcuts.begin() };
  typename std::array<CutType*, MaxCuts>::iterator _pend{ _pcuts.begin() };
  uint32_t _set_limit{ MaxCuts };
};
#pragma endregion

#pragma region Hashing
template<uint32_t max_multioutput_cut_size>
struct emap_triple_hash
{
  inline uint64_t operator()( const std::array<uint32_t, max_multioutput_cut_size>& p ) const
  {
    uint64_t seed = hash_block( p[0] );

    for ( uint32_t i = 1; i < max_multioutput_cut_size; ++i )
    {
      hash_combine( seed, hash_block( p[i] ) );
    }

    return seed;
  }
};
#pragma endregion

template<unsigned NInputs>
struct best_gate_emap
{
  supergate<NInputs> const* gate;
  double arrival;
  float area;
  float flow;
  unsigned phase : 16;
  unsigned cut : 12;
  unsigned size : 4;
};

template<unsigned NInputs>
struct node_match_emap
{
  /* best gate match for positive and negative output phases */
  supergate<NInputs> const* best_gate[2];
  /* alternative best gate for positibe and negative output phase */
  best_gate_emap<NInputs> best_alternative[2];
  /* fanin pin phases for both output phases */
  uint16_t phase[2];
  /* best cut index for both phases */
  uint16_t best_cut[2];
  /* node is mapped using only one phase */
  bool same_match;
  /* node is mapped to a multi-output gate */
  bool multioutput_match[2];

  /* arrival time at node output */
  double arrival[2];
  /* required time at node output */
  double required[2];
  /* area of the best matches */
  float area[2];

  /* number of references in the cover 0: pos, 1: neg */
  uint32_t map_refs[2];
  /* references estimation */
  float est_refs[2];
  /* area flow */
  float flows[2];
};

template<class Ntk, unsigned CutSize, unsigned NInputs, classification_type Configuration>
class emap_impl
{
private:
  union multi_match_data
  {
    uint64_t data{ 0 };
    struct
    {
      uint64_t in_tfi : 1;
      uint64_t cut_index : 31;
      uint64_t node_index : 32;
    };
  };
  union multioutput_info
  {
    uint32_t data;
    struct
    {
      unsigned index : 29;
      unsigned lowest_index : 1;
      unsigned highest_index : 1;
      unsigned has_info : 1;
    };
  };

public:
  static constexpr float epsilon = 0.0005;
  static constexpr uint32_t max_cut_num = 20;
  using cut_t = cut<CutSize, cut_enumeration_emap_cut<NInputs>>;
  using cut_set_t = emap_cut_set<cut_t, max_cut_num>;
  using cut_merge_t = typename std::array<cut_set_t*, Ntk::max_fanin_size + 1>;
  using fanin_cut_t = typename std::array<cut_t const*, Ntk::max_fanin_size>;
  using support_t = typename std::array<uint8_t, CutSize>;
  using TT = kitty::static_truth_table<6>;
  using truth_compute_t = typename std::array<TT, CutSize>;
  using node_match_t = std::vector<node_match_emap<NInputs>>;
  using klut_map = std::unordered_map<uint32_t, std::array<signal<klut_network>, 2>>;
  using block_map = std::unordered_map<uint32_t, std::array<signal<block_network>, 2>>;

  static constexpr uint32_t max_multioutput_cut_size = 3;
  static constexpr uint32_t max_multioutput_output_size = 2;
  using multi_cuts_t = fast_network_cuts<Ntk, max_multioutput_cut_size, true, cut_enumeration_emap_multi_cut>;
  using multi_cut_t = typename multi_cuts_t::cut_t;
  using multi_leaves_set_t = std::array<uint32_t, max_multioutput_cut_size>;
  using multi_output_set_t = std::vector<multi_match_data>;
  using multi_hash_t = phmap::flat_hash_map<multi_leaves_set_t, multi_output_set_t, emap_triple_hash<max_multioutput_cut_size>>;
  using multi_match_t = std::array<multi_match_data, max_multioutput_output_size>;
  using multi_cut_set_t = std::vector<std::array<cut_t, max_multioutput_output_size>>;
  using multi_single_matches_t = std::vector<multi_match_t>;
  using multi_matches_t = std::vector<std::vector<multi_match_t>>;

  using clock = typename std::chrono::steady_clock;
  using time_point = typename clock::time_point;

public:
  explicit emap_impl( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, emap_params const& ps, emap_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        node_tuple_match( ntk.size() ),
        switch_activity( ps.eswp_rounds ? switching_activity( ntk, ps.switching_activity_patterns ) : std::vector<float>( 0 ) ),
        cuts( ntk.size() )
  {
    std::memset( node_tuple_match.data(), 0, sizeof( multioutput_info ) * ntk.size() );
    std::tie( lib_inv_area, lib_inv_delay, lib_inv_id ) = library.get_inverter_info();
    std::tie( lib_buf_area, lib_buf_delay, lib_buf_id ) = library.get_buffer_info();
    tmp_visited.reserve( 100 );
  }

  explicit emap_impl( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, std::vector<float> const& switch_activity, emap_params const& ps, emap_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        node_tuple_match( ntk.size() ),
        switch_activity( switch_activity ),
        cuts( ntk.size() )
  {
    std::memset( node_tuple_match.data(), 0, sizeof( multioutput_info ) * ntk.size() );
    std::tie( lib_inv_area, lib_inv_delay, lib_inv_id ) = library.get_inverter_info();
    std::tie( lib_buf_area, lib_buf_delay, lib_buf_id ) = library.get_buffer_info();
    tmp_visited.reserve( 100 );
  }

  cell_view<block_network> run_block()
  {
    time_begin = clock::now();

    auto [res, old2new] = initialize_block_network();

    /* multi-output initialization */
    if ( ps.map_multioutput && ps.matching_mode != emap_params::structural )
    {
      compute_multioutput_match();
    }

    /* compute and save topological order */
    init_topo_order();

    /* init arrival time */
    if ( !init_arrivals() )
      return res;

    /* search for large matches */
    if ( ps.matching_mode == emap_params::structural || CutSize > 6 )
    {
      if ( !compute_struct_match() )
      {
        return res;
      }
    }

    /* compute cuts, matches, and initial mapping */
    if ( !ps.area_oriented_mapping )
    {
      if ( !compute_mapping_match<false>() )
      {
        return res;
      }
    }
    else
    {
      if ( !compute_mapping_match<true>() )
      {
        return res;
      }
    }

    /* run area recovery */
    if ( !improve_mapping() )
      return res;

    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover_block( res, old2new );
    st.time_total = ( clock::now() - time_begin );

    return res;
  }

  binding_view<klut_network> run_klut()
  {
    time_begin = clock::now();

    auto [res, old2new] = initialize_map_network();

    /* multi-output initialization */
    if ( ps.map_multioutput && ps.matching_mode != emap_params::structural )
    {
      compute_multioutput_match();
    }

    /* compute and save topological order */
    init_topo_order();

    /* init arrival time */
    if ( !init_arrivals() )
      return res;

    /* search for large matches */
    if ( ps.matching_mode == emap_params::structural || CutSize > 6 )
    {
      if ( !compute_struct_match() )
      {
        return res;
      }
    }

    /* compute cuts, matches, and initial mapping */
    if ( !ps.area_oriented_mapping )
    {
      if ( !compute_mapping_match<false>() )
      {
        return res;
      }
    }
    else
    {
      if ( !compute_mapping_match<true>() )
      {
        return res;
      }
    }

    /* run area recovery */
    if ( !improve_mapping() )
      return res;

    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover( res, old2new );
    st.time_total = ( clock::now() - time_begin );

    return res;
  }

  binding_view<klut_network> run_node_map()
  {
    time_begin = clock::now();

    auto [res, old2new] = initialize_map_network();

    /* [i] multi-output support is currently not implemented */

    /* compute and save topological order */
    init_topo_order();

    /* init arrival time */
    if ( !init_arrivals() )
      return res;

    /* compute cuts, matches, and initial mapping */
    if ( !ps.area_oriented_mapping )
    {
      if ( !compute_mapping_match_node<false>() )
      {
        return res;
      }
    }
    else
    {
      if ( !compute_mapping_match_node<true>() )
      {
        return res;
      }
    }

    /* run area recovery */
    if ( !improve_mapping() )
      return res;

    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover( res, old2new );
    st.time_total = ( clock::now() - time_begin );

    return res;
  }

private:
  bool improve_mapping()
  {
    /* compute mapping using global area flow */
    uint32_t i = 0;
    while ( i++ < ps.area_flow_rounds )
    {
      if ( !compute_mapping<true>() )
      {
        return false;
      }
    }

    /* compute mapping using exact area */
    i = 0;
    compute_required_time( true );
    while ( i++ < ps.ela_rounds )
    {
      if ( !compute_mapping_exact_reversed<false>() )
      {
        return false;
      }
    }

    /* compute mapping using exact switching activity estimation */
    i = 0;
    while ( i++ < ps.eswp_rounds )
    {
      if ( !compute_mapping_exact_reversed<true>() )
      {
        return false;
      }
    }

    return true;
  }

#pragma region Core
  template<bool DO_AREA>
  bool compute_mapping_match()
  {
    bool warning_box = false;

    for ( auto const& n : topo_order )
    {
      auto const index = ntk.node_to_index( n );

      if ( !compute_matches_node<DO_AREA>( n, warning_box ) )
      {
        continue;
      }

      /* load multi-output cuts and data */
      if ( ps.map_multioutput && node_tuple_match[index].has_info )
      {
        match_multi_add_cuts( n );
      }

      /* match positive phase */
      match_phase<DO_AREA>( n, 0u );

      /* match negative phase */
      match_phase<DO_AREA>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>( n );

      /* select alternative matches to use */
      select_alternatives<DO_AREA>( n );

      /* try multi-output matches */
      if constexpr ( DO_AREA )
      {
        if ( ps.map_multioutput && node_tuple_match[index].highest_index )
        {
          if ( match_multioutput<DO_AREA>( n ) )
            multi_node_update<DO_AREA>( n );
        }
      }
    }

    double area_old = area;
    bool success = set_mapping_refs_and_req<DO_AREA, false>();

    if ( warning_box )
    {
      std::cerr << "[i] MAP WARNING: not mapped don't touch gates are treated as sequential black boxes\n";
    }

    /* round stats */
    if ( ps.verbose )
    {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if ( iteration != 1 )
        area_gain = float( ( area_old - area ) / area_old * 100 );

      if constexpr ( DO_AREA )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      }
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  template<bool DO_AREA>
  inline bool compute_matches_node( node<Ntk> const& n, bool& warning_box )
  {
    auto const index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    node_data.est_refs[0] = node_data.est_refs[1] = static_cast<double>( ntk.fanout_size( n ) );
    node_data.map_refs[0] = node_data.map_refs[1] = 0;
    node_data.required[0] = node_data.required[1] = std::numeric_limits<float>::max();

    if ( ntk.is_constant( n ) )
    {
      /* all terminals have flow 0.0 */
      node_data.flows[0] = node_data.flows[1] = 0.0f;
      node_data.best_alternative[0].flow = node_data.best_alternative[1].flow = 0.0f;
      node_data.arrival[0] = node_data.arrival[1] = 0.0f;
      node_data.best_alternative[0].arrival = node_data.best_alternative[1].arrival = 0.0f;
      /* skip if cuts have been computed before */
      if ( cuts[index].size() == 0 )
      {
        add_zero_cut( index );
        match_constants( index );
      }
      return false;
    }
    else if ( ntk.is_pi( n ) )
    {
      node_data.flows[0] = 0.0f;
      node_data.best_alternative[0].flow = 0.0f;
      /* PIs have the negative phase implemented with an inverter */
      node_data.flows[1] = lib_inv_area / node_data.est_refs[1];
      node_data.best_alternative[1].flow = lib_inv_area / node_data.est_refs[1];
      /* skip if cuts have been computed before */
      if ( cuts[index].size() == 0 )
      {
        add_unit_cut( index );
      }
      return false;
    }

    if ( ps.matching_mode == emap_params::structural )
      return true;

    /* don't touch box */
    if constexpr ( has_is_dont_touch_v<Ntk> )
    {
      if ( ntk.is_dont_touch( n ) )
      {
        warning_box |= initialize_box( n );
        return false;
      }
    }

    /* compute cuts for node */
    if constexpr ( Ntk::min_fanin_size == 2 && Ntk::max_fanin_size == 2 )
    {
      merge_cuts2<DO_AREA>( n );
    }
    else
    {
      merge_cuts<DO_AREA>( n );
    }

    return true;
  }

  template<bool DO_AREA>
  void merge_cuts2( node<Ntk> const& n )
  {
    static constexpr uint32_t max_cut_size = CutSize > 6 ? 6 : CutSize;

    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    emap_cut_sort_type sort = emap_cut_sort_type::AREA;

    /* compute cuts */
    const auto fanin = 2;
    ntk.foreach_fanin( ntk.index_to_node( index ), [this]( auto child, auto i ) {
      lcuts[i] = &cuts[ntk.node_to_index( ntk.get_node( child ) )];
    } );
    lcuts[2] = &cuts[index];
    auto& rcuts = *lcuts[fanin];

    /* move pre-computed structural cuts to a temporary cutset */
    bool reinsert_cuts = false;
    if ( rcuts.size() )
    {
      temp_cuts.clear();
      for ( auto& cut : rcuts )
      {
        if ( ( *cut )->ignore )
          continue;
        recompute_cut_data( *cut, n );
        temp_cuts.simple_insert( *cut );
        reinsert_cuts = true;
      }
      rcuts.clear();
    }

    /* set cut limit for run-time optimization*/
    rcuts.set_cut_limit( ps.cut_enumeration_ps.cut_limit );

    cut_t new_cut;
    new_cut->pattern_index = 0;
    fanin_cut_t vcuts;

    for ( auto const& c1 : *lcuts[0] )
    {
      /* skip cuts of pattern matching */
      if ( ( *c1 )->pattern_index > 1 )
        continue;
      vcuts[0] = c1;

      for ( auto const& c2 : *lcuts[1] )
      {
        /* skip cuts of pattern matching */
        if ( ( *c2 )->pattern_index > 1 )
          continue;

        if ( !c1->merge( *c2, new_cut, max_cut_size ) )
        {
          continue;
        }

        if ( ps.remove_dominated_cuts && rcuts.is_dominated( new_cut ) )
        {
          continue;
        }

        /* compute function */
        vcuts[1] = c2;
        compute_truth_table( index, vcuts, fanin, new_cut );

        /* match cut and compute data */
        compute_cut_data( new_cut, n );

        if ( ps.remove_dominated_cuts )
          rcuts.insert( new_cut, false, sort );
        else
          rcuts.simple_insert( new_cut, sort );
      }
    }

    if ( reinsert_cuts )
    {
      for ( auto const& cut : temp_cuts )
      {
        rcuts.simple_insert( *cut, sort );
      }
    }

    cuts_total += rcuts.size();

    /* limit the maximum number of cuts */
    rcuts.limit( ps.cut_enumeration_ps.cut_limit );

    /* add trivial cut */
    if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 )
    {
      add_unit_cut( index );
    }
  }

  template<bool DO_AREA>
  void merge_cuts( node<Ntk> const& n )
  {
    static constexpr uint32_t max_cut_size = CutSize > 6 ? 6 : CutSize;

    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    emap_cut_sort_type sort = emap_cut_sort_type::AREA;
    cut_t best_cut;

    /* compute cuts */
    std::vector<uint32_t> cut_sizes;
    ntk.foreach_fanin( ntk.index_to_node( index ), [this, &cut_sizes]( auto child, auto i ) {
      lcuts[i] = &cuts[ntk.node_to_index( ntk.get_node( child ) )];
      cut_sizes.push_back( static_cast<uint32_t>( lcuts[i]->size() ) );
    } );
    const auto fanin = cut_sizes.size();
    lcuts[fanin] = &cuts[index];
    auto& rcuts = *lcuts[fanin];

    /* set cut limit for run-time optimization*/
    rcuts.set_cut_limit( ps.cut_enumeration_ps.cut_limit );
    fanin_cut_t vcuts;

    if ( fanin > 1 && fanin <= ps.cut_enumeration_ps.fanin_limit )
    {
      cut_t new_cut, tmp_cut;

      foreach_mixed_radix_tuple( cut_sizes.begin(), cut_sizes.end(), [&]( auto begin, auto end ) {
        auto it = vcuts.begin();
        auto i = 0u;
        while ( begin != end )
        {
          *it++ = &( ( *lcuts[i++] )[*begin++] );
        }

        if ( !vcuts[0]->merge( *vcuts[1], new_cut, max_cut_size ) )
        {
          return true; /* continue */
        }

        for ( i = 2; i < fanin; ++i )
        {
          tmp_cut = new_cut;
          if ( !vcuts[i]->merge( tmp_cut, new_cut, max_cut_size ) )
          {
            return true; /* continue */
          }
        }

        if ( ps.remove_dominated_cuts && rcuts.is_dominated( new_cut ) )
        {
          return true; /* continue */
        }

        compute_truth_table( index, vcuts, fanin, new_cut );

        /* match cut and compute data */
        compute_cut_data( new_cut, n );

        if ( ps.remove_dominated_cuts )
          rcuts.insert( new_cut, false, sort );
        else
          rcuts.simple_insert( new_cut, sort );

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
        vcuts[0] = cut;

        compute_truth_table( index, vcuts, fanin, new_cut );

        /* match cut and compute data */
        compute_cut_data( new_cut, n );

        if ( ps.remove_dominated_cuts )
          rcuts.insert( new_cut, false, sort );
        else
          rcuts.simple_insert( new_cut, sort );
      }

      /* limit the maximum number of cuts */
      rcuts.limit( ps.cut_enumeration_ps.cut_limit );
    }

    cuts_total += rcuts.size();

    add_unit_cut( index );
  }

  bool compute_struct_match()
  {
    if ( ps.matching_mode == emap_params::boolean )
      return true;

    /* compatible only with AIGs */
    if constexpr ( !is_aig_network_type_v<Ntk> )
    {
      if ( ps.matching_mode == emap_params::structural )
      {
        std::cerr << "[e] MAP ERROR: structural library works only with AIGs\n";
        return false;
      }
      return true;
    }

    /* no large gates identified */
    if ( library.num_structural_gates() == 0 )
    {
      if ( ps.matching_mode == emap_params::structural )
      {
        std::cerr << "[e] MAP ERROR: structural library is empty\n";
        return false;
      }
      return true;
    }

    bool warning_box = false;
    for ( auto const& n : topo_order )
    {
      auto const index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      if ( ntk.is_constant( n ) )
      {
        add_zero_cut( index );
        match_constants( index );
        continue;
      }
      else if ( ntk.is_pi( n ) )
      {
        add_unit_cut( index );
        continue;
      }

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        if ( ntk.is_dont_touch( n ) )
        {
          add_unit_cut( index );
          continue;
        }
      }

      /* compute cuts for node */
      merge_cuts_structural( n );
    }

    if ( warning_box )
    {
      std::cerr << "[i] MAP WARNING: not mapped don't touch gates are treated as sequential black boxes\n";
    }

    /* round stats */
    if ( ps.verbose )
    {
      st.round_stats.push_back( fmt::format( "[i] SCuts    : Cuts  = {:>12d}  Time = {:>12.2f}\n", cuts_total, to_seconds( clock::now() - time_begin ) ) );
    }

    return true;
  }

  void merge_cuts_structural( node<Ntk> const& n )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    emap_cut_sort_type sort = emap_cut_sort_type::AREA;

    /* compute cuts */
    const auto fanin = 2;
    std::array<uint32_t, 2> children_phase;
    ntk.foreach_fanin( ntk.index_to_node( index ), [&]( auto child, auto i ) {
      lcuts[i] = &cuts[ntk.node_to_index( ntk.get_node( child ) )];
      children_phase[i] = ntk.is_complemented( child ) ? 1 : 0;
    } );
    lcuts[2] = &cuts[index];
    auto& rcuts = *lcuts[fanin];

    /* set cut limit for run-time optimization*/
    rcuts.set_cut_limit( ps.cut_enumeration_ps.cut_limit );

    cut_t new_cut;
    std::vector<cut_t const*> vcuts( fanin );

    for ( auto const& c1 : *lcuts[0] )
    {
      for ( auto const& c2 : *lcuts[1] )
      {
        /* filter large cuts */
        if ( c1->size() + c2->size() > CutSize || c1->size() + c2->size() > NInputs )
          continue;
        /* filter cuts involving constants */
        if ( ( *c1 )->pattern_index == 0 || ( *c2 )->pattern_index == 0 )
          continue;

        vcuts[0] = c1;
        vcuts[1] = c2;
        uint32_t pattern_id1 = ( ( *c1 )->pattern_index << 1 ) | children_phase[0];
        uint32_t pattern_id2 = ( ( *c2 )->pattern_index << 1 ) | children_phase[1];
        if ( pattern_id1 > pattern_id2 )
        {
          std::swap( vcuts[0], vcuts[1] );
          std::swap( pattern_id1, pattern_id2 );
        }

        uint32_t new_pattern = library.get_pattern_id( pattern_id1, pattern_id2 );

        /* pattern not matched */
        if ( new_pattern == UINT32_MAX )
          continue;

        create_structural_cut( new_cut, vcuts, new_pattern, pattern_id1, pattern_id2 );

        if ( ps.remove_dominated_cuts && rcuts.is_dominated( new_cut ) )
          continue;

        /* match cut and compute data */
        compute_cut_data_structural( new_cut, n );

        if ( ps.remove_dominated_cuts )
          rcuts.insert( new_cut, false, sort );
        else
          rcuts.simple_insert( new_cut, sort );
      }
    }

    cuts_total += rcuts.size();

    /* limit the maximum number of cuts */
    rcuts.limit( ps.cut_enumeration_ps.cut_limit );

    /* add trivial cut */
    if ( rcuts.size() > 1 || ( *rcuts.begin() )->size() > 1 )
    {
      add_unit_cut( index );
    }
  }

  template<bool DO_AREA>
  bool compute_mapping_match_node()
  {
    for ( auto const& n : topo_order )
    {
      auto const index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      node_data.best_gate[0] = node_data.best_gate[1] = nullptr;
      node_data.same_match = 0;
      node_data.multioutput_match[0] = node_data.multioutput_match[1] = false;
      node_data.required[0] = node_data.required[1] = std::numeric_limits<float>::max();
      node_data.map_refs[0] = node_data.map_refs[1] = 0;
      node_data.est_refs[0] = node_data.est_refs[1] = static_cast<float>( ntk.fanout_size( n ) );

      if ( ntk.is_constant( n ) )
      {
        /* all terminals have flow 0 */
        node_data.flows[0] = node_data.flows[1] = 0.0f;
        node_data.arrival[0] = node_data.arrival[1] = 0.0f;
        add_zero_cut( index );
        match_constants( index );
        continue;
      }
      else if ( ntk.is_pi( n ) )
      {
        /* all terminals have flow 0 */
        node_data.flows[0] = 0.0f;
        /* PIs have the negative phase implemented with an inverter */
        node_data.flows[1] = lib_inv_area / node_data.est_refs[1];
        add_unit_cut( index );
        continue;
      }

      /* compute the node mapping */
      add_node_cut<DO_AREA>( n );

      /* match positive phase */
      match_phase<DO_AREA>( n, 0u );

      /* match negative phase */
      match_phase<DO_AREA>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>( n );

      /* select alternative matches to use */
      select_alternatives<DO_AREA>( n );
    }
    double area_old = area;
    bool success = set_mapping_refs_and_req<DO_AREA, false>();

    /* round stats */
    if ( ps.verbose )
    {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if ( iteration != 1 )
        area_gain = float( ( area_old - area ) / area_old * 100 );

      if constexpr ( DO_AREA )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      }
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  template<bool DO_AREA>
  void add_node_cut( node<Ntk> const& n )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    auto& rcuts = &cuts[index];

    std::vector<uint32_t> fanin_indexes;
    fanin_indexes.reserve( Ntk::max_fanin_size );

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      fanin_indexes.push_back( ntk.node_to_index( ntk.get_node( f ) ) );
    } );

    assert( fanin_indexes.size() <= CutSize );

    cut_t new_cut = rcuts.add_cut( fanin_indexes.begin(), fanin_indexes.end() );
    new_cut->function = kitty::extend_to<6>( ntk.node_function( n ) );

    /* match cut and compute data */
    compute_cut_data( new_cut, n );

    ++cuts_total;
  }

  template<bool DO_AREA>
  bool compute_mapping()
  {
    for ( auto const& n : topo_order )
    {
      uint32_t index = ntk.node_to_index( n );

      /* reset mapping */
      node_match[index].map_refs[0] = node_match[index].map_refs[1] = 0u;

      if ( ntk.is_constant( n ) )
        continue;
      if ( ntk.is_pi( n ) )
      {
        node_match[index].flows[1] = lib_inv_area / node_match[index].est_refs[1];
        node_match[index].best_alternative[1].flow = lib_inv_area / node_match[index].est_refs[1];
        continue;
      }

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        if ( ntk.is_dont_touch( n ) )
        {
          if constexpr ( has_has_binding_v<Ntk> )
          {
            propagate_data_forward_white_box( n );
          }
          continue;
        }
      }

      /* match positive phase */
      match_phase<DO_AREA>( n, 0u );

      /* match negative phase */
      match_phase<DO_AREA>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>( n );

      /* try a multi-output match */
      if constexpr ( DO_AREA )
      {
        if ( ps.map_multioutput && node_tuple_match[index].highest_index )
        {
          bool multi_success = match_multioutput<DO_AREA>( n );
          if ( multi_success )
            multi_node_update<DO_AREA>( n );
        }
      }

      assert( node_match[index].arrival[0] < node_match[index].required[0] + epsilon );
      assert( node_match[index].arrival[1] < node_match[index].required[1] + epsilon );
    }

    double area_old = area;
    bool success = set_mapping_refs_and_req<DO_AREA, false>();

    /* round stats */
    if ( ps.verbose )
    {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if ( iteration != 1 )
        area_gain = float( ( area_old - area ) / area_old * 100 );

      if constexpr ( DO_AREA )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      }
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  template<bool SwitchActivity>
  bool compute_mapping_exact_reversed()
  {
    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      if ( ntk.is_constant( *it ) || ntk.is_pi( *it ) )
        continue;

      const auto index = ntk.node_to_index( *it );
      auto& node_data = node_match[index];

      /* skip not mapped nodes */
      if ( !node_data.map_refs[0] && !node_data.map_refs[1] )
        continue;

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        node<Ntk> n = ntk.index_to_node( index );
        if ( ntk.is_dont_touch( n ) )
        {
          if constexpr ( has_has_binding_v<Ntk> )
          {
            propagate_data_backward_white_box( n );
          }
          continue;
        }
      }

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      uint8_t use_phase = node_data.best_gate[0] != nullptr ? 0 : 1;
      double old_required = -1;
      if ( node_data.same_match )
      {
        auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];
        cut_deref<SwitchActivity>( best_cut, *it, use_phase );

        /* propagate required time over the output inverter if present */
        if ( node_data.map_refs[use_phase ^ 1] > 0 )
        {
          old_required = node_data.required[use_phase];
          node_data.required[use_phase] = std::min( node_data.required[use_phase], node_data.required[use_phase ^ 1] - lib_inv_delay );
        }
      }
      else if ( !node_data.map_refs[0] || !node_data.map_refs[1] )
      {
        use_phase = node_data.map_refs[0] ? 0 : 1;
        auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];
        cut_deref<SwitchActivity>( best_cut, *it, use_phase );
        node_data.same_match = true;
      }

      /* match positive phase */
      match_phase_exact<SwitchActivity>( *it, 0u );

      /* match negative phase */
      match_phase_exact<SwitchActivity>( *it, 1u );

      /* restore required time */
      if ( old_required > 0 )
      {
        node_data.required[use_phase] = old_required;
      }

      /* try to drop one phase */
      match_drop_phase<true, true, SwitchActivity>( *it );

      /* try a multi-output match */ /* TODO: fix the required time*/
      if ( ps.map_multioutput && node_tuple_match[index].lowest_index )
      {
        bool mapped = match_multioutput_exact<SwitchActivity>( *it, true );

        /* propagate required time for the selected gates */
        if ( mapped )
        {
          match_multioutput_propagate_required( *it );
        }
        else
        {
          match_propagate_required( index );
        }
      }
      else
      {
        match_propagate_required( index );
      }
    }

    double area_old = area;

    propagate_arrival_times();

    /* round stats */
    if ( ps.verbose )
    {
      float area_gain = float( ( area_old - area ) / area_old * 100 );
      std::stringstream stats{};
      if constexpr ( SwitchActivity )
        stats << fmt::format( "[i] Switching: Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      else
        stats << fmt::format( "[i] Area Rev : Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
      st.round_stats.push_back( stats.str() );
    }

    return true;
  }

  inline void match_propagate_required( uint32_t index )
  {
    /* don't touch box */
    if constexpr ( has_is_dont_touch_v<Ntk> )
    {
      node<Ntk> n = ntk.index_to_node( index );
      if ( ntk.is_dont_touch( n ) )
      {
        if constexpr ( has_has_binding_v<Ntk> )
        {
          propagate_data_backward_white_box( n );
        }
        return;
      }
    }

    auto& node_data = node_match[index];

    /* propagate required time through the leaves */
    unsigned use_phase = node_data.best_gate[0] == nullptr ? 1u : 0u;
    unsigned other_phase = use_phase ^ 1;

    assert( node_data.best_gate[0] != nullptr || node_data.best_gate[1] != nullptr );
    // assert( node_data.map_refs[0] || node_data.map_refs[1] );

    /* propagate required time over the output inverter if present */
    if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
    {
      node_data.required[use_phase] = std::min( node_data.required[use_phase], node_data.required[other_phase] - lib_inv_delay );
    }

    if ( node_data.map_refs[0] )
      assert( node_data.arrival[0] < node_data.required[0] + epsilon );
    if ( node_data.map_refs[1] )
      assert( node_data.arrival[1] < node_data.required[1] + epsilon );

    if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
    {
      auto ctr = 0u;
      auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];
      auto const& supergate = node_data.best_gate[use_phase];
      for ( auto leaf : best_cut )
      {
        auto phase = ( node_data.phase[use_phase] >> ctr ) & 1;
        node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[use_phase] - supergate->tdelay[ctr] );
        ++ctr;
      }
    }

    if ( !node_data.same_match && node_data.map_refs[other_phase] > 0 )
    {
      auto ctr = 0u;
      auto const& best_cut = cuts[index][node_data.best_cut[other_phase]];
      auto const& supergate = node_data.best_gate[other_phase];
      for ( auto leaf : best_cut )
      {
        auto phase = ( node_data.phase[other_phase] >> ctr ) & 1;
        node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[other_phase] - supergate->tdelay[ctr] );
        ++ctr;
      }
    }
  }

  template<bool ELA>
  bool set_mapping_refs()
  {
    /* compute the current worst delay and update the mapping refs */
    delay = 0.0f;
    ntk.foreach_po( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );

      if ( ntk.is_complemented( s ) )
        delay = std::max( delay, node_match[index].arrival[1] );
      else
        delay = std::max( delay, node_match[index].arrival[0] );

      if constexpr ( !ELA )
      {
        if ( ntk.is_complemented( s ) )
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    } );

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    inv = 0;
    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      const auto index = ntk.node_to_index( *it );
      auto& node_data = node_match[index];

      /* skip constants and PIs */
      if ( ntk.is_constant( *it ) )
      {
        if ( node_data.map_refs[0] || node_data.map_refs[1] )
        {
          /* if used and not available in the library launch a mapping error */
          if ( node_data.best_gate[0] == nullptr && node_data.best_gate[1] == nullptr )
          {
            std::cerr << "[e] MAP ERROR: technology library does not contain constant gates, impossible to perform mapping" << std::endl;
            st.mapping_error = true;
            return false;
          }
        }
        continue;
      }
      else if ( ntk.is_pi( *it ) )
      {
        if ( node_match[index].map_refs[1] > 0u )
        {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
          ++inv;
        }
        continue;
      }

      /* continue if not referenced in the cover */
      if ( !node_match[index].map_refs[0] && !node_match[index].map_refs[1] )
        continue;

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        if ( ntk.is_dont_touch( *it ) )
        {
          set_mapping_refs_dont_touch<ELA>( *it );
          continue;
        }
      }

      unsigned use_phase = node_data.best_gate[0] == nullptr ? 1u : 0u;

      if ( node_data.best_gate[use_phase] == nullptr )
      {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[e] MAP ERROR: technology library is not complete, impossible to perform mapping" << std::endl;
        st.mapping_error = true;
        return false;
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];
          auto ctr = 0u;

          for ( auto const leaf : best_cut )
          {
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
        {
          if ( iteration < ps.area_flow_rounds )
          {
            ++node_data.map_refs[use_phase];
          }
          area += lib_inv_area;
          ++inv;
        }
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if ( !node_data.same_match && node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];

          auto ctr = 0u;
          for ( auto const leaf : best_cut )
          {
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }
    }

    ++iteration;

    if constexpr ( ELA )
    {
      return true;
    }

    /* blend estimated references */
    float const coef = 1.0f / ( ( iteration + 1.0f ) * ( iteration + 1.0f ) );
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      node_match[i].est_refs[0] = std::max( 1.0f, coef * node_match[i].est_refs[0] + ( 1 - coef ) * node_match[i].map_refs[0] );
      node_match[i].est_refs[1] = std::max( 1.0f, coef * node_match[i].est_refs[1] + ( 1 - coef ) * node_match[i].map_refs[1] );
    }

    return true;
  }

  template<bool DO_AREA, bool ELA>
  bool set_mapping_refs_and_req()
  {
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required[0] = node_match[i].required[1] = std::numeric_limits<float>::max();
    }

    /* compute the current worst delay and update the mapping refs */
    delay = 0.0f;
    ntk.foreach_po( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );

      if ( ntk.is_complemented( s ) )
        delay = std::max( delay, node_match[index].arrival[1] );
      else
        delay = std::max( delay, node_match[index].arrival[0] );

      if constexpr ( !ELA )
      {
        if ( ntk.is_complemented( s ) )
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    } );

    set_output_required_time( iteration == 0 );

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    inv = 0;
    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      const auto index = ntk.node_to_index( *it );
      auto& node_data = node_match[index];

      /* skip constants and PIs */
      if ( ntk.is_constant( *it ) )
      {
        if ( node_match[index].map_refs[0] || node_match[index].map_refs[1] )
        {
          /* if used and not available in the library launch a mapping error */
          if ( node_data.best_gate[0] == nullptr && node_data.best_gate[1] == nullptr )
          {
            std::cerr << "[e] MAP ERROR: technology library does not contain constant gates, impossible to perform mapping" << std::endl;
            st.mapping_error = true;
            return false;
          }
        }
        continue;
      }
      else if ( ntk.is_pi( *it ) )
      {
        if ( node_match[index].map_refs[1] > 0u )
        {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
          ++inv;
        }
        continue;
      }

      /* continue if not referenced in the cover */
      if ( !node_match[index].map_refs[0] && !node_match[index].map_refs[1] )
        continue;

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        if ( ntk.is_dont_touch( *it ) )
        {
          set_mapping_refs_dont_touch<ELA>( *it );
          continue;
        }
      }

      /* refine best matches with alternatives */
      if constexpr ( !DO_AREA )
      {
        if ( ps.use_match_alternatives )
          refine_best_matches( *it );
      }

      unsigned use_phase = node_data.best_gate[0] == nullptr ? 1u : 0u;
      if ( node_data.best_gate[use_phase] == nullptr )
      {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[e] MAP ERROR: technology library is not complete, impossible to perform mapping" << std::endl;
        st.mapping_error = true;
        return false;
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];
          auto ctr = 0u;

          for ( auto const leaf : best_cut )
          {
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
        {
          if ( iteration < ps.area_flow_rounds )
          {
            ++node_data.map_refs[use_phase];
          }
          area += lib_inv_area;
          ++inv;
        }
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if ( !node_data.same_match && node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];

          auto ctr = 0u;
          for ( auto const leaf : best_cut )
          {
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }

      if ( !ps.area_oriented_mapping )
      {
        match_propagate_required( index );
      }
    }

    ++iteration;

    if constexpr ( ELA )
    {
      return true;
    }

    /* blend estimated references */
    float const coef = 1.0f / ( ( iteration + 1.0f ) * ( iteration + 1.0f ) );
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      node_match[i].est_refs[0] = std::max( 1.0f, coef * node_match[i].est_refs[0] + ( 1 - coef ) * node_match[i].map_refs[0] );
      node_match[i].est_refs[1] = std::max( 1.0f, coef * node_match[i].est_refs[1] + ( 1 - coef ) * node_match[i].map_refs[1] );
    }

    return true;
  }

  template<bool ELA>
  inline void set_mapping_refs_dont_touch( node<Ntk> const& n )
  {
    if constexpr ( !ELA )
    {
      /* reference node */
      ntk.foreach_fanin( n, [&]( auto const& f ) {
        uint32_t leaf = ntk.node_to_index( ntk.get_node( f ) );
        uint8_t phase = ntk.is_complemented( f ) ? 1 : 0;
        node_match[leaf].map_refs[phase]++;
      } );
    }

    const auto index = ntk.node_to_index( n );

    if constexpr ( has_has_binding_v<Ntk> )
    {
      /* increase area */
      area += node_match[index].area[0];
      if ( node_match[index].map_refs[1] )
      {
        if ( iteration < ps.area_flow_rounds )
        {
          ++node_match[index].map_refs[0];
        }
        area += lib_inv_area;
        ++inv;
      }
    }
  }

  void set_output_required_time( bool warning )
  {
    double required = delay;
    /* relax delay constraints */
    if ( iteration == 0 && ps.required_time == 0.0f && ps.required_times.empty() && ps.relax_required > 0.0f )
    {
      required *= ( 100.0 + ps.relax_required ) / 100.0;
    }

    /* Global target time constraint */
    if ( ps.required_times.empty() )
    {
      if ( ps.required_time != 0.0f )
      {
        if ( ps.required_time < delay - epsilon )
        {
          if ( warning )
            std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {:.2f}", ps.required_time ) << std::endl;
        }
        else
        {
          required = ps.required_time;
        }
      }

      /* set the required time at POs */
      ntk.foreach_po( [&]( auto const& s ) {
        const auto index = ntk.node_to_index( ntk.get_node( s ) );
        if ( ntk.is_complemented( s ) )
          node_match[index].required[1] = required;
        else
          node_match[index].required[0] = required;
      } );

      return;
    }

    /* Output-specific target time constraint */
    ntk.foreach_po( [&]( auto const& s, uint32_t i ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      uint8_t phase = ntk.is_complemented( s ) ? 1 : 0;
      if ( node_match[index].arrival[phase] > ps.required_times[i] + epsilon )
      {
        /* maintain the same delay */
        node_match[index].required[phase] = node_match[index].arrival[phase];
        if ( warning )
          std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {:.2f} at output {}", ps.required_times[i], i ) << std::endl;
      }
      else
      {
        node_match[index].required[phase] = ps.required_times[i];
      }
    } );
  }

  void compute_required_time( bool exit_early = false )
  {
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required[0] = node_match[i].required[1] = std::numeric_limits<float>::max();
    }

    /* return if mapping is area oriented */
    if ( ps.area_oriented_mapping )
      return;

    set_output_required_time( iteration == 1 );

    if ( exit_early )
      return;

    /* propagate required time to the PIs */
    for ( auto it = topo_order.rbegin(); it != topo_order.rend(); ++it )
    {
      if ( ntk.is_pi( *it ) || ntk.is_constant( *it ) )
        break;

      const auto index = ntk.node_to_index( *it );

      if ( !node_match[index].map_refs[0] && !node_match[index].map_refs[1] )
        continue;

      match_propagate_required( index );
    }
  }

  void propagate_arrival_times()
  {
    area = 0.0f;
    inv = 0;
    for ( auto const& n : topo_order )
    {
      auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      /* measure area */
      if ( ntk.is_constant( n ) )
      {
        continue;
      }
      else if ( ntk.is_pi( n ) )
      {
        if ( node_data.map_refs[1] > 0u )
        {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
          ++inv;
        }
        continue;
      }

      /* reset required time */
      node_data.required[0] = std::numeric_limits<float>::max();
      node_data.required[1] = std::numeric_limits<float>::max();

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        node<Ntk> n = ntk.index_to_node( index );
        if ( ntk.is_dont_touch( n ) )
        {
          if constexpr ( has_has_binding_v<Ntk> )
          {
            propagate_data_forward_white_box( n );
            if ( node_match[index].map_refs[0] || node_match[index].map_refs[1] )
              area += node_data.area[0];
            if ( node_data.map_refs[1] )
            {
              area += lib_inv_area;
              ++inv;
            }
          }
          continue;
        }
      }

      uint8_t use_phase = node_data.best_gate[0] != nullptr ? 0 : 1;

      /* compute arrival of use_phase */
      supergate<NInputs> const* best_gate = node_data.best_gate[use_phase];
      double worst_arrival = 0;
      uint16_t best_phase = node_data.phase[use_phase];
      auto ctr = 0u;
      for ( auto l : cuts[index][node_data.best_cut[use_phase]] )
      {
        double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_gate->tdelay[ctr];
        worst_arrival = std::max( worst_arrival, arrival_pin );
        ++ctr;
      }

      node_data.arrival[use_phase] = worst_arrival;

      /* compute area */
      if ( node_data.map_refs[use_phase] > 0 || ( node_data.same_match && ( node_match[index].map_refs[0] || node_match[index].map_refs[1] ) ) )
      {
        area += node_data.area[use_phase];
        if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
        {
          area += lib_inv_area;
          ++inv;
        }
      }

      /* compute arrival of the other phase */
      use_phase ^= 1;
      if ( node_data.same_match )
      {
        node_data.arrival[use_phase] = worst_arrival + lib_inv_delay;
        continue;
      }

      assert( node_data.best_gate[use_phase] != nullptr );

      best_gate = node_data.best_gate[use_phase];
      worst_arrival = 0;
      best_phase = node_data.phase[use_phase];
      ctr = 0u;
      for ( auto l : cuts[index][node_data.best_cut[use_phase]] )
      {
        double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_gate->tdelay[ctr];
        worst_arrival = std::max( worst_arrival, arrival_pin );
        ++ctr;
      }

      node_data.arrival[use_phase] = worst_arrival;

      if ( node_data.map_refs[use_phase] > 0 )
      {
        area += node_data.area[use_phase];
      }
    }

    /* compute the current worst delay */
    delay = 0.0f;
    ntk.foreach_po( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );

      if ( ntk.is_complemented( s ) )
        delay = std::max( delay, node_match[index].arrival[1] );
      else
        delay = std::max( delay, node_match[index].arrival[0] );
    } );

    /* return if mapping is area oriented */
    ++iteration;
    if ( ps.area_oriented_mapping )
      return;

    /* set the required time at POs */
    ntk.foreach_po( [&]( auto const& s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      if ( ntk.is_complemented( s ) )
        node_match[index].required[1] = delay;
      else
        node_match[index].required[0] = delay;
    } );
  }

  void propagate_arrival_node( node<Ntk> const& n )
  {
    uint32_t index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    uint8_t use_phase = node_data.best_gate[0] != nullptr ? 0 : 1;

    /* compute arrival of use_phase */
    supergate<NInputs> const* best_gate = node_data.best_gate[use_phase];
    double worst_arrival = 0;
    uint16_t best_phase = node_data.phase[use_phase];
    auto ctr = 0u;
    for ( auto l : cuts[index][node_data.best_cut[use_phase]] )
    {
      double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_gate->tdelay[ctr];
      worst_arrival = std::max( worst_arrival, arrival_pin );
      ++ctr;
    }
    node_data.arrival[use_phase] = worst_arrival;

    /* compute arrival of the other phase */
    use_phase ^= 1;
    if ( node_data.same_match )
    {
      node_data.arrival[use_phase] = worst_arrival + lib_inv_delay;
      return;
    }

    assert( node_data.best_gate[0] != nullptr );

    best_gate = node_data.best_gate[use_phase];
    worst_arrival = 0;
    best_phase = node_data.phase[use_phase];
    ctr = 0u;
    for ( auto l : cuts[index][node_data.best_cut[use_phase]] )
    {
      double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_gate->tdelay[ctr];
      worst_arrival = std::max( worst_arrival, arrival_pin );
      ++ctr;
    }

    node_data.arrival[use_phase] = worst_arrival;
  }

  template<bool DO_AREA>
  void match_phase( node<Ntk> const& n, uint8_t phase )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    uint32_t cut_index = 0u;

    node_data.best_gate[phase] = nullptr;
    node_data.arrival[phase] = std::numeric_limits<float>::max();
    node_data.flows[phase] = std::numeric_limits<float>::max();
    node_data.area[phase] = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;

    best_gate_emap<NInputs>& gA = node_data.best_alternative[phase];
    gA.gate = nullptr;
    gA.arrival = std::numeric_limits<float>::max();
    gA.flow = std::numeric_limits<float>::max();
    uint32_t best_sizeA = UINT32_MAX;

    /* unmap multioutput */
    node_data.multioutput_match[phase] = false;

    /* foreach cut */
    for ( auto& cut : cuts[index] )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = ( *cut )->supergates;
      auto const negation = ( *cut )->negations[phase];

      if ( supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates[phase] )
      {
        uint16_t gate_polarity = gate.polarity ^ negation;
        double worst_arrival = 0.0f;
        double worst_arrivalA = 0.0f;
        float area_local = gate.area;
        float area_localA = gate.area;

        auto ctr = 0u;
        for ( auto l : *cut )
        {
          uint8_t leaf_phase = ( gate_polarity >> ctr ) & 1;

          double arrival_pinA = node_match[l].best_alternative[leaf_phase].arrival + gate.tdelay[ctr];
          worst_arrivalA = std::max( worst_arrivalA, arrival_pinA );

          // if constexpr ( DO_AREA )
          // {
          //   if ( worst_arrivalA > node_data.required[phase] + epsilon || worst_arrivalA >= std::numeric_limits<float>::max() )
          //     break;
          // }

          double arrival_pin = node_match[l].arrival[leaf_phase] + gate.tdelay[ctr];
          worst_arrival = std::max( worst_arrival, arrival_pin );

          area_local += node_match[l].flows[leaf_phase];
          area_localA += node_match[l].best_alternative[leaf_phase].flow;
          ++ctr;
        }

        bool skip = false;
        if constexpr ( DO_AREA )
        {
          if ( ctr < cut->size() )
            continue;
          if ( worst_arrival > node_data.required[phase] + epsilon || worst_arrival >= std::numeric_limits<float>::max() )
            skip = true;
        }

        if ( !skip && compare_map<DO_AREA>( worst_arrival, node_data.arrival[phase], area_local, node_data.flows[phase], cut->size(), best_size ) )
        {
          node_data.best_gate[phase] = &gate;
          node_data.arrival[phase] = worst_arrival;
          node_data.flows[phase] = area_local;
          node_data.best_cut[phase] = cut_index;
          node_data.area[phase] = gate.area;
          node_data.phase[phase] = gate_polarity;
          best_size = cut->size();
        }

        /* compute the alternative */
        if ( compare_map<!DO_AREA>( worst_arrivalA, gA.arrival, area_localA, gA.flow, cut->size(), best_sizeA ) )
        {
          gA.gate = &gate;
          gA.arrival = worst_arrivalA;
          gA.area = gate.area;
          gA.flow = area_localA;
          gA.phase = gate_polarity;
          gA.cut = cut_index;
          best_sizeA = cut->size();
          gA.size = cut->size();
        }
      }

      ++cut_index;
    }
  }

  template<bool SwitchActivity>
  void match_phase_exact( node<Ntk> const& n, uint8_t phase )
  {
    double best_arrival = std::numeric_limits<float>::max();
    float best_exact_area = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint16_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    supergate<NInputs> const* best_gate = node_data.best_gate[phase];

    /* unmap multioutput */
    if ( node_data.multioutput_match[phase] )
    {
      /* dereference multi-output */
      if ( !node_data.same_match && best_gate != nullptr && node_data.map_refs[phase] )
      {
        auto const& cut = multi_cut_set[node_data.best_cut[phase]][0];
        cut_deref<SwitchActivity>( cut, n, phase );
      }
      best_gate = nullptr;
      node_data.multioutput_match[phase] = false;
    }

    /* recompute best match info */
    if ( best_gate != nullptr )
    {
      /* if cut is implemented, remove it from the cover */
      if ( !node_data.same_match && node_data.map_refs[phase] )
      {
        auto const& cut = cuts[index][node_data.best_cut[phase]];
        cut_deref<SwitchActivity>( cut, n, phase );
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts[index] )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = ( *cut )->supergates;
      auto const negation = ( *cut )->negations[phase];

      if ( supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates[phase] )
      {
        uint16_t gate_polarity = gate.polarity ^ negation;
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for ( auto l : *cut )
        {
          double arrival_pin = node_match[l].arrival[( gate_polarity >> ctr ) & 1] + gate.tdelay[ctr];
          worst_arrival = std::max( worst_arrival, arrival_pin );
          ++ctr;
        }

        if ( worst_arrival > node_data.required[phase] + epsilon || worst_arrival >= std::numeric_limits<float>::max() )
          continue;

        node_data.phase[phase] = gate_polarity;
        node_data.area[phase] = gate.area;
        float area_exact = cut_measure_mffc<SwitchActivity>( *cut, n, phase );

        if ( compare_map<true>( worst_arrival, best_arrival, area_exact, best_exact_area, cut->size(), best_size ) )
        {
          best_arrival = worst_arrival;
          best_exact_area = area_exact;
          best_area = gate.area;
          best_size = cut->size();
          best_cut = cut_index;
          best_phase = gate_polarity;
          best_gate = &gate;
        }
      }

      ++cut_index;
    }

    node_data.flows[phase] = best_exact_area;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_gate[phase] = best_gate;

    if ( !node_data.same_match && node_data.map_refs[phase] )
    {
      best_exact_area = cut_ref<SwitchActivity>( cuts[index][best_cut], n, phase );
    }
  }

  template<bool DO_AREA, bool ELA, bool SwitchActivity = false>
  void match_drop_phase( node<Ntk> const& n )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    /* compute arrival adding an inverter to the other match phase */
    double worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
    double worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
    bool use_zero = false;
    bool use_one = false;

    /* only one phase is matched */
    if ( node_data.best_gate[0] == nullptr )
    {
      set_match_complemented_phase( index, 1, worst_arrival_npos );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[0] || node_data.map_refs[1] )
          cut_ref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
      }
      return;
    }
    else if ( node_data.best_gate[1] == nullptr )
    {
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[0] || node_data.map_refs[1] )
          cut_ref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
      }
      return;
    }

    /* try to use only one match to cover both phases */
    if constexpr ( !DO_AREA )
    {
      /* if arrival improves matching the other phase and inserting an inverter */
      if ( worst_arrival_npos < node_data.arrival[0] + epsilon )
      {
        use_one = true;
      }
      if ( worst_arrival_nneg < node_data.arrival[1] + epsilon )
      {
        use_zero = true;
      }
    }
    else
    {
      /* check if both phases + inverter meet the required time */
      use_zero = worst_arrival_nneg < ( node_data.required[1] + epsilon );
      use_one = worst_arrival_npos < ( node_data.required[0] + epsilon );
    }

    /* condition on not used phases, evaluate a substitution during exact area recovery */
    if constexpr ( ELA )
    {
      if ( node_data.map_refs[0] == 0 || node_data.map_refs[1] == 0 )
      {
        /* select the used match */
        auto phase = 0;
        auto nphase = 0;
        if ( node_data.map_refs[0] == 0 )
        {
          phase = 1;
          use_one = true;
          use_zero = false;
        }
        else
        {
          nphase = 1;
          use_one = false;
          use_zero = true;
        }
        /* select the not used match instead if it leads to area improvement and doesn't violate the required time */
        if ( node_data.arrival[nphase] + lib_inv_delay < node_data.required[phase] + epsilon )
        {
          auto size_phase = cuts[index][node_data.best_cut[phase]].size();
          auto size_nphase = cuts[index][node_data.best_cut[nphase]].size();

          if ( compare_map<DO_AREA>( node_data.arrival[nphase] + lib_inv_delay, node_data.arrival[phase], node_data.flows[nphase] + lib_inv_area, node_data.flows[phase], size_nphase, size_phase ) )
          {
            /* invert the choice */
            use_zero = !use_zero;
            use_one = !use_one;
          }
        }
      }
    }

    if ( ( !use_zero && !use_one ) )
    {
      /* use both phases */
      node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
      node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
      node_data.same_match = false;
      return;
    }

    /* use area flow as a tiebreaker */
    if ( use_zero && use_one )
    {
      auto size_zero = cuts[index][node_data.best_cut[0]].size();
      auto size_one = cuts[index][node_data.best_cut[1]].size();

      if constexpr ( ELA )
      {
        if ( !node_data.same_match )
        {
          /* both phases were implemented --> evaluate substitution */
          cut_deref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
          node_data.flows[1] = cut_deref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
          node_data.flows[0] = cut_ref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
          cut_ref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
        }
        /* evaluate based on inverter cost */
        if constexpr ( !SwitchActivity )
        {
          use_zero = lib_inv_area < node_data.flows[1] + epsilon;
          use_one = lib_inv_area < node_data.flows[0] + epsilon;
        }

        if ( use_one && use_zero )
        {
          if ( compare_map<DO_AREA>( worst_arrival_nneg, worst_arrival_npos, node_data.flows[0], node_data.flows[1], size_zero, size_one ) )
            use_one = false;
          else
            use_zero = false;
        }
        else if ( !use_one && !use_zero && node_data.same_match )
        {
          node_data.same_match = false;
          cut_ref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
          cut_ref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
          return;
        }
      }
      else
      {
        /* compare flows by looking at the most convinient and referenced */
        if ( node_data.flows[0] / node_data.est_refs[0] + lib_inv_area < node_data.flows[1] / node_data.est_refs[1] + epsilon )
        {
          use_one = false;
        }
        else if ( node_data.flows[1] / node_data.est_refs[1] + lib_inv_area < node_data.flows[0] / node_data.est_refs[0] + epsilon )
        {
          use_zero = false;
        }
        else
        {
          /* delay the decision on what to keep --> wait for better estimations */
          node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
          node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
          node_data.same_match = false;
          return;
        }
      }
    }

    if ( use_zero )
    {
      if constexpr ( ELA )
      {
        /* set cut references */
        if ( !node_data.same_match )
        {
          /* dereference the negative phase cut if in use */
          if ( node_data.map_refs[1] > 0 )
            cut_deref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
          /* reference the positive cut if not in use before */
          if ( node_data.map_refs[0] == 0 && node_data.map_refs[1] > 0 )
            cut_ref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
        }
        else if ( node_data.map_refs[0] || node_data.map_refs[1] )
          cut_ref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
      }
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
    }
    else
    {
      if constexpr ( ELA )
      {
        /* set cut references */
        if ( !node_data.same_match )
        {
          /* dereference the positive phase cut if in use */
          if ( node_data.map_refs[0] > 0 )
            cut_deref<false>( cuts[index][node_data.best_cut[0]], n, 0 );
          /* reference the negative cut if not in use before */
          if ( node_data.map_refs[1] == 0 && node_data.map_refs[0] > 0 )
            cut_ref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
        }
        else if ( node_data.map_refs[0] || node_data.map_refs[1] )
          cut_ref<false>( cuts[index][node_data.best_cut[1]], n, 1 );
      }
      set_match_complemented_phase( index, 1, worst_arrival_npos );
    }
  }

  inline void set_match_complemented_phase( uint32_t index, uint8_t phase, double worst_arrival_n )
  {
    auto& node_data = node_match[index];
    auto phase_n = phase ^ 1;
    node_data.same_match = true;
    node_data.best_gate[phase_n] = nullptr;
    node_data.best_cut[phase_n] = node_data.best_cut[phase];
    node_data.phase[phase_n] = node_data.phase[phase];
    node_data.arrival[phase_n] = worst_arrival_n;
    node_data.area[phase_n] = node_data.area[phase];
    node_data.flows[phase_n] = ( node_data.flows[phase] + lib_inv_area ) / node_data.est_refs[phase_n];
    node_data.flows[phase] = node_data.flows[phase] / node_data.est_refs[phase];
  }

  template<bool DO_AREA>
  inline void select_alternatives( node<Ntk> const& n )
  {
    if constexpr ( DO_AREA )
      return;

    if ( !ps.use_match_alternatives )
      return;

    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    best_gate_emap<NInputs>& g0 = node_data.best_alternative[0];
    best_gate_emap<NInputs>& g1 = node_data.best_alternative[1];
    float g0flow = g0.flow / node_data.est_refs[0];
    float g1flow = g1.flow / node_data.est_refs[1];

    /* process for best area */ /* removed check on required since this is executed only during a delay pass */
    if ( g0.gate != nullptr && g0flow + lib_inv_area < g1flow + epsilon )
    {
      g1 = g0;
      g1.gate = nullptr;
      g1.arrival += lib_inv_delay;
      g1.flow = ( g1.flow + lib_inv_area ) / node_data.est_refs[1];
      g0.flow = g0flow;
      return;
    }
    else if ( g1.gate != nullptr && g1flow + lib_inv_area < g0flow + epsilon )
    {
      g0 = g1;
      g0.gate = nullptr;
      g0.arrival += lib_inv_delay;
      g0.flow = ( g0.flow + lib_inv_area ) / node_data.est_refs[0];
      g1.flow = g1flow;
      return;
    }

    g0.flow = g0flow;
    g1.flow = g1flow;
  }

  inline void refine_best_matches( node<Ntk> const& n )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    /* evaluate to change the best matches with the best alternative */
    best_gate_emap<NInputs>& g0 = node_data.best_alternative[0];
    best_gate_emap<NInputs>& g1 = node_data.best_alternative[1];

    if ( node_data.map_refs[0] && node_data.map_refs[1] )
    {
      if ( node_data.same_match )
      {
        /* pick best implementation between the two alternatives */
        unsigned best_match_phase = node_data.best_gate[0] == nullptr ? 1 : 0;
        unsigned use_phase = g0.gate == nullptr ? 1 : 0;
        if ( g0.gate != nullptr && g1.gate != nullptr )
        {
          if ( g0.arrival > node_data.required[0] + epsilon || g1.arrival > node_data.required[1] + epsilon )
            return;

          refine_best_matches_copy_refinement( n, 0, false );
          refine_best_matches_copy_refinement( n, 1, false );
          node_data.same_match = false;
          return;
        }
        else
        {
          best_gate_emap<NInputs>& gUse = node_data.best_alternative[use_phase];
          if ( gUse.arrival > node_data.required[use_phase] + epsilon || gUse.arrival + lib_inv_delay > node_data.required[use_phase ^ 1] + epsilon )
          {
            return;
          }
          refine_best_matches_copy_refinement( n, use_phase, true );
          return;
        }
      }
      else
      {
        /* not same match: evaluate both zero and one phase */
        if ( g0.gate != nullptr && g0.arrival < node_data.required[0] + epsilon )
        {
          node_data.same_match = false;
          refine_best_matches_copy_refinement( n, 0, g1.gate == nullptr && g0.arrival + lib_inv_delay < node_data.required[1] + epsilon );
        }
        if ( g1.gate != nullptr && g1.arrival < node_data.required[1] + epsilon )
        {
          node_data.same_match = false;
          refine_best_matches_copy_refinement( n, 1, g0.gate == nullptr && g1.arrival + lib_inv_delay < node_data.required[0] + epsilon );
        }
      }
    }
    else if ( node_data.map_refs[0] )
    {
      if ( g0.gate != nullptr && g0.arrival < node_data.required[0] + epsilon )
      {
        node_data.same_match = false;
        refine_best_matches_copy_refinement( n, 0, false );
      }
      else if ( g0.gate == nullptr && g1.arrival + lib_inv_delay < node_data.required[0] + epsilon )
      {
        refine_best_matches_copy_refinement( n, 1, true );
      }
    }
    else
    {
      if ( g1.gate != nullptr && g1.arrival < node_data.required[1] + epsilon )
      {
        node_data.same_match = false;
        refine_best_matches_copy_refinement( n, 1, false );
      }
      else if ( g1.gate == nullptr && g0.arrival + lib_inv_delay < node_data.required[1] + epsilon )
      {
        refine_best_matches_copy_refinement( n, 0, true );
      }
    }
  }

  inline void refine_best_matches_copy_refinement( node<Ntk> const& n, unsigned phase, bool both_phases )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    best_gate_emap<NInputs>& bg = node_data.best_alternative[phase];

    node_data.best_gate[phase] = bg.gate;
    node_data.phase[phase] = bg.phase;
    node_data.best_cut[phase] = bg.cut;
    node_data.arrival[phase] = bg.arrival;
    node_data.area[phase] = bg.area;
    node_data.flows[phase] = bg.flow;

    if ( !both_phases )
      return;

    node_data.same_match = true;
    phase ^= 1;
    node_data.best_gate[phase] = nullptr;
    node_data.phase[phase] = bg.phase;
    node_data.best_cut[phase] = bg.cut;
    node_data.arrival[phase] = bg.arrival + lib_inv_delay;
    node_data.area[phase] = bg.area;
    node_data.flows[phase] = ( bg.flow * node_data.est_refs[phase ^ 1] + lib_inv_area ) / node_data.est_refs[phase];
  }

  bool initialize_box( node<Ntk> const& n )
  {
    uint32_t index = ntk.node_to_index( n );

    if ( cuts[index].size() == 0 )
      add_unit_cut( index );

    auto& node_data = node_match[index];
    node_data.same_match = true;

    /* if it has mapping data propagate the delays and measure the data */
    if constexpr ( has_has_binding_v<Ntk> )
    {
      propagate_data_forward_white_box( n );
      return false;
    }

    /* consider as a black box */
    node_data.flows[0] = 0.0f;
    node_data.flows[1] = lib_inv_area / node_data.est_ref[1];
    node_data.arrival[0] = 0.0f;
    node_data.arrival[1] = lib_inv_delay;
    node_data.area[0] = node_data.area[1] = 0;

    return true;
  }

  void propagate_data_forward_white_box( node<Ntk> const& n )
  {
    uint32_t index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    auto const& gate = ntk.get_binding( n );

    /* propagate arrival time */
    double arrival = 0;
    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      uint32_t f_index = ntk.node_to_index( ntk.get_node( f ) );
      uint8_t phase = ntk.is_complemented( f ) ? 1 : 0;
      double propagation_delay = std::max( gate.pins[i].rise_block_delay, gate.pins[i].fall_block_delay );
      arrival = std::max( arrival, node_match[f_index].arrival[phase] + propagation_delay );
    } );

    /* set data */
    node_data.arrival[0] = arrival;
    node_data.arrival[1] = arrival + lib_inv_delay;
    node_data.area[0] = node_data.area[1] = gate.area;
    node_data.flows[1] = ( node_data.flows[0] + lib_inv_area ) / node_data.est_refs[1];
    node_data.flows[0] = node_data.area[0] / node_data.est_refs[0];
  }

  void propagate_data_backward_white_box( node<Ntk> const& n )
  {
    uint32_t index = ntk.node_to_index( n );
    auto& node_data = node_match[index];
    auto const& gate = ntk.get_binding( n );

    assert( node_data.map_refs[0] || node_data.map_refs[1] );

    /* propagate required time over the output inverter if present */
    if ( node_data.map_refs[1] > 0 )
    {
      node_data.required[0] = std::min( node_data.required[0], node_data.required[1] - lib_inv_delay );
    }

    if ( node_data.map_refs[0] )
      assert( node_data.arrival[0] < node_data.required[0] + epsilon );
    if ( node_data.map_refs[1] )
      assert( node_data.arrival[1] < node_data.required[1] + epsilon );

    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      uint32_t f_index = ntk.node_to_index( ntk.get_node( f ) );
      uint8_t phase = ntk.is_complemented( f ) ? 1 : 0;
      double propagation_delay = std::max( gate.pins[i].rise_block_delay, gate.pins[i].fall_block_delay );
      node_match[f_index].required[phase] = std::min( node_match[f_index].required[phase], node_data.required[0] - propagation_delay );
    } );
  }

  void match_constants( uint32_t index )
  {
    auto& node_data = node_match[index];

    kitty::static_truth_table<6> zero_tt;
    auto const supergates_zero = library.get_supergates( zero_tt );
    auto const supergates_one = library.get_supergates( ~zero_tt );

    /* Not available in the library */
    if ( supergates_zero == nullptr && supergates_one == nullptr )
    {
      return;
    }
    /* if only one is available, the other is obtained using an inverter */
    if ( supergates_zero != nullptr )
    {
      node_data.best_gate[0] = &( ( *supergates_zero )[0] );
      node_data.arrival[0] = node_data.best_gate[0]->tdelay[0];
      node_data.area[0] = node_data.best_gate[0]->area;
      node_data.phase[0] = 0;
    }
    if ( supergates_one != nullptr )
    {
      node_data.best_gate[1] = &( ( *supergates_one )[0] );
      node_data.arrival[1] = node_data.best_gate[1]->tdelay[0];
      node_data.area[1] = node_data.best_gate[1]->area;
      node_data.phase[1] = 0;
    }
    else
    {
      node_data.same_match = true;
      node_data.arrival[1] = node_data.arrival[0] + lib_inv_delay;
      node_data.area[1] = node_data.area[0] + lib_inv_area;
      node_data.phase[1] = 1;
    }
    if ( supergates_zero == nullptr )
    {
      node_data.same_match = true;
      node_data.arrival[0] = node_data.arrival[1] + lib_inv_delay;
      node_data.area[0] = node_data.area[1] + lib_inv_area;
      node_data.phase[0] = 1;
    }
  }

  template<bool DO_AREA>
  bool match_multioutput( node<Ntk> const& n )
  {
    /* extract outputs tuple */
    uint32_t index = ntk.node_to_index( n );
    multi_match_t const& tuple_data = multi_node_match[node_tuple_match[index].index][0];

    /* get the cut */
    auto const& cut0 = cuts[tuple_data[0].node_index][tuple_data[0].cut_index];

    /* local values storage */
    std::array<double, max_multioutput_output_size> arrival;
    std::array<float, max_multioutput_output_size> area_flow;
    std::array<float, max_multioutput_output_size> area;
    std::array<uint8_t, max_multioutput_output_size> phase;
    std::array<uint16_t, max_multioutput_output_size> pin_phase;
    std::array<double, max_multioutput_output_size> est_refs;
    std::array<uint32_t, max_multioutput_output_size> cut_index;
    bool mapped_multioutput = false;

    uint8_t iteration_phase = cut0->supergates[0] == nullptr ? 1 : 0;

    /* iterate for each possible match */
    for ( auto i = 0; i < cut0->supergates[iteration_phase]->size(); ++i )
    {
      /* store local validity and comparison info */
      bool valid = true;
      bool is_best = true;
      bool respects_required = true;
      double old_flow_sum = 0;

      /* iterate for each output of the multi-output gate */
      for ( auto j = 0; j < max_multioutput_output_size; ++j )
      {
        uint32_t node_index = tuple_data[j].node_index;
        cut_index[j] = tuple_data[j].cut_index;
        auto& node_data = node_match[node_index];
        auto const& cut = cuts[node_index][cut_index[j]];
        uint8_t phase_inverted = cut->supergates[0] == nullptr ? 1 : 0;
        supergate<NInputs> const& gate = ( *( cut->supergates[phase_inverted] ) )[i];

        /* protection on complicated duplicated nodes to remap to multioutput */
        if ( !node_data.same_match )
          return false;

        /* get the output phase */
        pin_phase[j] = gate.polarity;
        phase[j] = ( gate.polarity >> NInputs ) ^ phase_inverted;

        /* compute arrival */
        arrival[j] = 0.0;
        auto ctr = 0u;
        for ( auto l : cut )
        {
          double arrival_pin = node_match[l].arrival[( gate.polarity >> ctr ) & 1] + gate.tdelay[ctr];
          arrival[j] = std::max( arrival[j], arrival_pin );
          ++ctr;
        }

        /* check required time: same_match is true */
        if constexpr ( DO_AREA )
        {
          if ( arrival[j] > node_data.required[phase[j]] + epsilon )
          {
            valid = false;
            break;
          }
          if ( arrival[j] + lib_inv_delay > node_data.required[phase[j] ^ 1] + epsilon )
          {
            valid = false;
            break;
          }
        }

        /* check required time of the current solution */
        if ( node_data.arrival[phase[j]] > node_data.required[phase[j]] )
          respects_required = false;
        if ( node_data.same_match && node_data.arrival[phase[j] ^ 1] > node_data.required[phase[j] ^ 1] )
          respects_required = false;

        /* compute area flow */
        if ( j == 0 || !node_data.multioutput_match[0] )
        {
          uint8_t current_phase = node_data.best_gate[0] == nullptr ? 1 : 0;
          old_flow_sum += node_data.flows[current_phase];
        }
        uint8_t old_phase = node_data.phase[phase[j]];
        node_data.phase[phase[j]] = gate.polarity;
        area[j] = gate.area;
        area_flow[j] = gate.area + cut_leaves_flow( cut, n, phase[j] );
        node_data.phase[phase[j]] = old_phase;

        /* current version may lead to delay increase */
        est_refs[j] = node_data.est_refs[phase[j]];
      }

      /* not better than individual gates */
      if ( !valid )
        continue;

      if constexpr ( !DO_AREA )
      {
        if ( !is_best )
          continue;
      }

      /* combine evaluation for precise area flow estimantion */
      /* compute equation AF(n) = ( Area(G) + |roots| * SUM_{l in leaves} AF(l) ) / SUM_{p in roots} est_refs( p ) */
      float flow_sum_pos = 0, flow_sum_neg;
      float combined_est_refs = 0;
      for ( auto j = 0; j < max_multioutput_output_size; ++j )
      {
        flow_sum_pos += area_flow[j];
        combined_est_refs += est_refs[j];
      }
      flow_sum_neg = flow_sum_pos;
      flow_sum_pos /= combined_est_refs;

      /* not better than individual gates */
      if ( respects_required && ( flow_sum_pos > old_flow_sum + epsilon ) )
        continue;

      mapped_multioutput = true;
      flow_sum_neg = ( flow_sum_neg + lib_inv_area ) / combined_est_refs;

      /* commit multi-output gate */
      for ( uint32_t j = 0; j < max_multioutput_output_size; ++j )
      {
        uint32_t node_index = tuple_data[j].node_index;
        auto& node_data = node_match[node_index];
        auto const& cut = cuts[node_index][cut_index[j]];
        uint8_t phase_inverted = cut->supergates[0] == nullptr ? 1 : 0;
        supergate<NInputs> const& gate = ( *( cut->supergates[phase_inverted] ) )[i];

        uint8_t mapped_phase = phase[j];
        node_data.multioutput_match[mapped_phase] = true;

        node_data.best_gate[mapped_phase] = &gate;
        node_data.best_cut[mapped_phase] = cut_index[j];
        node_data.phase[mapped_phase] = pin_phase[j];
        node_data.arrival[mapped_phase] = arrival[j];
        node_data.area[mapped_phase] = area[j]; /* partial area contribution */
        node_data.flows[mapped_phase] = flow_sum_pos;

        assert( node_data.arrival[mapped_phase] < node_data.required[mapped_phase] + epsilon );

        /* select opposite phase */
        mapped_phase ^= 1;
        node_data.multioutput_match[mapped_phase] = true;
        node_data.best_gate[mapped_phase] = nullptr;
        node_data.best_cut[mapped_phase] = cut_index[j];
        node_data.phase[mapped_phase] = pin_phase[j];
        node_data.arrival[mapped_phase] = arrival[j] + lib_inv_delay;
        node_data.area[mapped_phase] = area[j]; /* partial area contribution */
        node_data.flows[mapped_phase] = flow_sum_neg;

        assert( node_data.arrival[mapped_phase] < node_data.required[mapped_phase] + epsilon );
      }
    }

    return mapped_multioutput;
  }

  template<bool SwitchActivity>
  bool match_multioutput_exact( node<Ntk> const& n, bool last_round )
  {
    /* extract outputs tuple */
    uint32_t index = ntk.node_to_index( n );
    multi_match_t const& tuple_data = multi_node_match[node_tuple_match[index].index][0];

    /* local values storage */
    std::array<float, max_multioutput_output_size> best_exact_area;

    for ( int j = max_multioutput_output_size - 1; j >= 0; --j )
    {
      /* protection on complicated duplicated nodes to remap to multioutput */
      if ( !node_match[tuple_data[j].node_index].same_match )
        return false;
    }

    /* if one of the outputs is not referenced, do not use multi-output gate */
    if ( last_round )
    {
      for ( uint32_t j = 0; j < max_multioutput_output_size; ++j )
      {
        uint32_t node_index = tuple_data[j].node_index;
        if ( !node_match[node_index].map_refs[0] && !node_match[node_index].map_refs[1] )
        {
          return false;
        }
      }
    }

    /* if "same match" and used in the cover dereference the leaves (reverse topo order) */
    for ( int j = max_multioutput_output_size - 1; j >= 0; --j )
    {
      uint32_t node_index = tuple_data[j].node_index;
      uint8_t selected_phase = node_match[node_index].best_gate[0] == nullptr ? 1 : 0;

      if ( node_match[node_index].map_refs[0] || node_match[node_index].map_refs[1] )
      {
        /* match is always single output here */
        auto const& cut = cuts[node_index][node_match[node_index].best_cut[0]];
        uint8_t use_phase = node_match[node_index].best_gate[0] != nullptr ? 0 : 1;
        best_exact_area[j] = cut_deref<SwitchActivity>( cut, ntk.index_to_node( node_index ), use_phase );

        /* mapping a non referenced phase */
        if ( node_match[node_index].map_refs[selected_phase] == 0 )
          best_exact_area[j] += lib_inv_area;
      }
    }

    /* perform mapping */
    bool mapped_multioutput = false;
    mapped_multioutput = match_multioutput_exact_core<SwitchActivity>( tuple_data, best_exact_area );

    /* if "same match" and used in the cover reference the leaves (topo order) */
    for ( auto j = 0; j < max_multioutput_output_size; ++j )
    {
      uint32_t node_index = tuple_data[j].node_index;

      if ( node_match[node_index].map_refs[0] || node_match[node_index].map_refs[1] )
      {
        uint8_t use_phase = node_match[node_index].best_gate[0] != nullptr ? 0 : 1;
        auto const& best_cut = cuts[node_index][node_match[node_index].best_cut[use_phase]];
        cut_ref<SwitchActivity>( best_cut, ntk.index_to_node( node_index ), use_phase );
      }
    }

    return mapped_multioutput;
  }

  template<bool SwitchActivity>
  inline bool match_multioutput_exact_core( multi_match_t const& tuple_data, std::array<float, max_multioutput_output_size>& best_exact_area )
  {
    /* get the cut representative */
    auto const& cut0 = cuts[tuple_data[0].node_index][tuple_data[0].cut_index];

    /* local values storage */
    std::array<double, max_multioutput_output_size> arrival;
    std::array<float, max_multioutput_output_size> area_exact;
    std::array<float, max_multioutput_output_size> area;
    std::array<uint8_t, max_multioutput_output_size> phase;
    std::array<uint16_t, max_multioutput_output_size> pin_phase;
    std::array<uint32_t, max_multioutput_output_size> cut_index;

    uint8_t iteration_phase = cut0->supergates[0] == nullptr ? 1 : 0;

    bool mapped_multioutput = false;

    /* iterate for each possible match */
    for ( auto i = 0; i < cut0->supergates[iteration_phase]->size(); ++i )
    {
      /* store local validity and comparison info */
      bool valid = true;
      bool is_best = true;
      bool respects_required = true;
      uint32_t it_counter = 0;

      /* iterate for each output of the multi-output gate (reverse topo order) */
      for ( int j = max_multioutput_output_size - 1; j >= 0; --j )
      {
        uint32_t node_index = tuple_data[j].node_index;
        cut_index[j] = tuple_data[j].cut_index;
        auto& node_data = node_match[node_index];
        auto const& cut = cuts[node_index][cut_index[j]];
        uint8_t phase_inverted = cut->supergates[0] == nullptr ? 1 : 0;
        supergate<NInputs> const& gate = ( *( cut->supergates[phase_inverted] ) )[i];
        ++it_counter;

        /* get the output phase and area */
        pin_phase[j] = gate.polarity;
        phase[j] = ( gate.polarity >> NInputs ) ^ phase_inverted;
        area[j] = gate.area;

        /* compute arrival */
        arrival[j] = 0.0;
        auto ctr = 0u;
        for ( auto l : cut )
        {
          double arrival_pin = node_match[l].arrival[( gate.polarity >> ctr ) & 1] + gate.tdelay[ctr];
          arrival[j] = std::max( arrival[j], arrival_pin );
          ++ctr;
        }

        /* check required time */
        if ( arrival[j] > node_data.required[phase[j]] + epsilon )
        {
          valid = false;
          break;
        }
        if ( arrival[j] + lib_inv_delay > node_data.required[phase[j] ^ 1] + epsilon )
        {
          valid = false;
          break;
        }

        /* check required time of current solution */
        if ( node_data.arrival[phase[j]] > node_data.required[phase[j]] )
          respects_required = false;
        if ( node_data.arrival[phase[j] ^ 1] > node_data.required[phase[j] ^ 1] )
          respects_required = false;

        /* compute exact area for match: needed only for the first node (leaves are shared) */
        if ( it_counter == 1 )
        {
          auto old_phase = node_data.phase[phase[j]];
          auto old_area = node_data.area[phase[j]];
          node_data.phase[phase[j]] = pin_phase[j];
          node_data.area[phase[j]] = area[j];
          area_exact[j] = cut_measure_mffc<SwitchActivity>( cut, ntk.index_to_node( node_index ), phase[j] );
          node_data.phase[phase[j]] = old_phase;
          node_data.area[phase[j]] = old_area;
        }
        else
        {
          area_exact[j] = area[j];
        }

        /* Add output inverter cost if mapping a non referenced phase */
        if ( node_data.map_refs[phase[j]] == 0 && node_data.map_refs[phase[j] ^ 1] > 0 )
        {
          area_exact[j] += lib_inv_area;
        }
      }

      /* check quality: TODO add output inverter in the cost if necessary */
      float best_exact_area_total = 0;
      float area_exact_total = 0;
      for ( auto j = 0; j < max_multioutput_output_size; ++j )
      {
        best_exact_area_total += best_exact_area[j];
        area_exact_total += area_exact[j];
      }

      /* not better than individual gates */
      if ( !valid || ( area_exact_total > best_exact_area_total - epsilon && respects_required ) )
      {
        continue;
      }

      mapped_multioutput = true;

      /* commit multi-output gate (topo order) */
      for ( uint32_t j = 0; j < max_multioutput_output_size; ++j )
      {
        uint32_t node_index = tuple_data[j].node_index;
        auto& node_data = node_match[node_index];
        auto const& cut = cuts[node_index][cut_index[j]];
        uint8_t phase_inverted = cut->supergates[0] == nullptr ? 1 : 0;
        supergate<NInputs> const& gate = ( *( cut->supergates[phase_inverted] ) )[i];

        uint8_t mapped_phase = phase[j];
        best_exact_area[j] = area_exact[j];

        if ( node_data.map_refs[phase[j]] == 0 && node_data.map_refs[phase[j] ^ 1] > 0 )
        {
          best_exact_area[j] += lib_inv_area;
        }

        /* write data */
        node_data.multioutput_match[mapped_phase] = true;
        node_data.best_gate[mapped_phase] = &gate;
        node_data.best_cut[mapped_phase] = cut_index[j];
        node_data.phase[mapped_phase] = pin_phase[j];
        node_data.arrival[mapped_phase] = arrival[j];
        node_data.area[mapped_phase] = area[j]; /* partial area contribution */

        node_data.flows[mapped_phase] = area_exact[j]; /* partial exact area contribution */
        /* select opposite phase */
        mapped_phase ^= 1;
        node_data.multioutput_match[mapped_phase] = true;
        node_data.best_gate[mapped_phase] = nullptr;
        node_data.best_cut[mapped_phase] = cut_index[j];
        node_data.phase[mapped_phase] = pin_phase[j];
        node_data.arrival[mapped_phase] = arrival[j] + lib_inv_delay;
        node_data.area[mapped_phase] = area[j]; /* partial area contribution */
        node_data.flows[mapped_phase] = area_exact[j];

        assert( node_data.arrival[mapped_phase] < node_data.required[mapped_phase] + epsilon );
      }
    }

    return mapped_multioutput;
  }

  template<bool DO_AREA>
  void multi_node_update( node<Ntk> const& n )
  {
    uint32_t check_index = ntk.node_to_index( n );
    multi_match_t const& tuple_data = multi_node_match[node_tuple_match[ntk.node_to_index( n )].index][0];
    uint64_t signature = 0;

    /* check if a node is in TFI: there is a path of length > 1 */
    bool in_tfi = false;
    node<Ntk> min_node = n;
    for ( auto j = 0; j < max_multioutput_output_size - 1; ++j )
    {
      if ( tuple_data[j].in_tfi )
      {
        min_node = ntk.index_to_node( tuple_data[j].node_index );
        in_tfi = true;
        signature |= UINT64_C( 1 ) << ( tuple_data[j].node_index & 0x3f );
      }
    }

    if ( !in_tfi )
      return;

    /* recompute data in between: should I mark the leaves? (not necessary under some assumptions) */
    ntk.incr_trav_id();
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      /* TODO: this recursion works as it is for a maximum multioutput value of 2 */
      multi_node_update_rec<DO_AREA>( ntk.get_node( f ), min_node + 1, signature );
    } );
  }

  template<bool DO_AREA>
  void multi_node_update_rec( node<Ntk> const& n, uint32_t min_index, uint64_t& signature )
  {
    uint32_t index = ntk.node_to_index( n );

    if ( index < min_index )
      return;
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;

    ntk.set_visited( n, ntk.trav_id() );
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      multi_node_update_rec<DO_AREA>( ntk.get_node( f ), min_index, signature );
    } );

    /* update the node if uses an updated leaf */
    auto& node_data = node_match[index];
    bool leaf_used = multi_node_update_cut_check( index, signature, 0 );

    if ( !node_data.same_match )
      leaf_used |= multi_node_update_cut_check( index, signature, 1 );

    if ( !leaf_used )
      return;

    signature |= UINT64_C( 1 ) << ( index & 0x3f );

    /* avoid cycles by recomputing arrival times for multi-output gates or decomposing them */
    if ( node_data.same_match && node_data.multioutput_match[0] )
    {
      propagate_arrival_node( n );
      /* check required time */
      if ( node_data.arrival[0] < node_data.required[0] + epsilon && node_data.arrival[1] < node_data.required[1] + epsilon )
        return;
    }

    /* match positive phase */
    match_phase<DO_AREA>( n, 0u );

    /* match negative phase */
    match_phase<DO_AREA>( n, 1u );

    /* try to drop one phase */
    match_drop_phase<DO_AREA, false>( n );

    assert( node_data.arrival[0] < node_data.required[0] + epsilon );
    assert( node_data.arrival[1] < node_data.required[1] + epsilon );
  }

  template<bool SwitchActivity>
  void multi_node_update_exact( node<Ntk> const& n )
  {
    uint32_t check_index = ntk.node_to_index( n );
    multi_match_t const& tuple_data = multi_node_match[node_tuple_match[ntk.node_to_index( n )].index][0];
    uint64_t signature = 0;

    /* check if a node is in TFI: there is a path of length > 1 */
    bool in_tfi = false;
    node<Ntk> min_node = n;
    for ( auto j = 0; j < max_multioutput_output_size - 1; ++j )
    {
      if ( tuple_data[j].in_tfi )
      {
        min_node = ntk.index_to_node( tuple_data[j].node_index );
        in_tfi = true;
        signature |= UINT64_C( 1 ) << ( tuple_data[j].node_index & 0x3f );
      }
    }

    if ( !in_tfi )
      return;

    /* recompute data in between: should I mark the leaves? (not necessary under some assumptions) */
    ntk.incr_trav_id();
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      /* TODO: this recursion works as it is for a maximum multioutput value of 2 */
      multi_node_update_exact_rec<SwitchActivity>( ntk.get_node( f ), min_node + 1, signature );
    } );
  }

  template<bool SwitchActivity>
  void multi_node_update_exact_rec( node<Ntk> const& n, uint32_t min_index, uint64_t& signature )
  {
    uint32_t index = ntk.node_to_index( n );

    if ( index < min_index )
      return;
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;

    ntk.set_visited( n, ntk.trav_id() );
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      multi_node_update_exact_rec<SwitchActivity>( ntk.get_node( f ), min_index, signature );
    } );

    /* update the node if uses an updated leaf */
    auto& node_data = node_match[index];
    bool leaf_used = multi_node_update_cut_check( index, signature, 0 );

    if ( !node_data.same_match )
      leaf_used |= multi_node_update_cut_check( index, signature, 1 );

    if ( !leaf_used )
      return;

    signature |= UINT64_C( 1 ) << ( index & 0x3f );

    assert( !node_data.multioutput_match[0] );
    assert( !node_data.multioutput_match[1] );

    if ( node_data.same_match && ( node_data.map_refs[0] || node_data.map_refs[1] ) )
    {
      uint8_t use_phase = node_data.best_gate[0] != nullptr ? 0 : 1;
      auto const& best_cut = cuts[index][node_data.best_cut[use_phase]];
      cut_deref<SwitchActivity>( best_cut, n, use_phase );
    }

    /* match positive phase */
    match_phase_exact<SwitchActivity>( n, 0u );

    /* match negative phase */
    match_phase_exact<SwitchActivity>( n, 1u );

    /* try to drop one phase */
    match_drop_phase<true, true>( n );

    assert( node_data.arrival[0] < std::numeric_limits<float>::max() );
    assert( node_data.arrival[1] < std::numeric_limits<float>::max() );
  }

  inline void match_multioutput_propagate_required( node<Ntk> const& n )
  {
    /* extract outputs tuple */
    uint32_t index = ntk.node_to_index( n );
    multi_match_t const& tuple_data = multi_node_match[node_tuple_match[index].index][0];

    for ( int j = max_multioutput_output_size - 1; j >= 0; --j )
    {
      const auto node_index = tuple_data[j].node_index;
      match_propagate_required( node_index );
    }
  }

  void match_multi_add_cuts( node<Ntk> const& n )
  {
    /* assume a single cut (current version) */
    uint32_t index = ntk.node_to_index( n );
    multi_match_t& matches = multi_node_match[node_tuple_match[index].index][0];

    /* find the corresponding cut */
    uint32_t cut_p = 0;
    while ( matches[cut_p].node_index != index )
      ++cut_p;

    assert( cut_p < matches.size() );
    uint32_t cut_index = matches[cut_p].cut_index;
    auto& cut = multi_cut_set[cut_index][cut_p];
    auto single_cut = multi_cut_set[cut_index][cut_p];
    auto& rcuts = cuts[index];

    /* not enough space in the data structure: abort */
    if ( rcuts.size() == max_cut_num )
    {
      match_multi_add_cuts_remove_entry( matches );
      return;
    }

    /* insert single cut variation if unique (for delay preservation) */
    if ( !rcuts.is_contained( single_cut ) )
    {
      single_cut->pattern_index = 0;
      compute_cut_data( single_cut, ntk.index_to_node( index ) );
      rcuts.append_cut( single_cut );

      /* not enough space in the data structure: abort */
      if ( rcuts.size() == max_cut_num )
      {
        rcuts.limit( rcuts.size() - 1 );
        match_multi_add_cuts_remove_entry( matches );
        return;
      }
    }

    /* add multi-output cut */
    uint32_t num_cuts_pre = rcuts.size();
    cut->ignore = true;
    rcuts.append_cut( cut );

    uint32_t num_cuts_after = rcuts.size();
    assert( num_cuts_after == num_cuts_pre + 1 );

    rcuts.limit( num_cuts_pre );

    /* update tuple data */
    matches[cut_p].cut_index = num_cuts_pre;
  }

  inline void match_multi_add_cuts_remove_entry( multi_match_t const& matches )
  {
    /* reset matches */
    for ( multi_match_data const& entry : matches )
    {
      node_tuple_match[entry.node_index].data = 0;
    }
  }

  inline bool multi_node_update_cut_check( uint32_t index, uint64_t signature, uint8_t phase )
  {
    auto const& cut = cuts[index][node_match[index].best_cut[phase]];

    if ( ( signature & cut.signature() ) > 0 )
      return true;

    return false;
  }
#pragma endregion

#pragma region Mapping utils
  inline double cut_leaves_flow( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    double flow{ 0.0f };
    auto const& node_data = node_match[ntk.node_to_index( n )];

    uint8_t ctr = 0u;
    for ( auto leaf : cut )
    {
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;
      flow += node_match[leaf].flows[leaf_phase];
    }

    return flow;
  }

  template<bool SwitchActivity>
  float cut_ref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    /* don't touch box */
    if constexpr ( has_is_dont_touch_v<Ntk> )
    {
      if ( ntk.is_dont_touch( n ) )
      {
        return count;
      }
    }

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }
      else if ( ntk.is_pi( ntk.index_to_node( leaf ) ) )
      {
        /* reference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( node_match[leaf].map_refs[1]++ == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          ++node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Recursive referencing if leaf was not referenced */
        if ( !node_match[leaf].map_refs[0] && !node_match[leaf].map_refs[1] )
        {
          auto const& best_cut = cuts[leaf][node_match[leaf].best_cut[leaf_phase]];
          count += cut_ref<SwitchActivity>( best_cut, ntk.index_to_node( leaf ), leaf_phase );
        }

        /* Add inverter area if not present yet and leaf node is implemented in the opposite phase */
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u && node_match[leaf].best_gate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
      }
      else
      {
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u )
        {
          auto const& best_cut = cuts[leaf][node_match[leaf].best_cut[leaf_phase]];
          count += cut_ref<SwitchActivity>( best_cut, ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }

  template<bool SwitchActivity>
  float cut_deref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    /* don't touch box */
    if constexpr ( has_is_dont_touch_v<Ntk> )
    {
      if ( ntk.is_dont_touch( n ) )
      {
        return count;
      }
    }

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }
      else if ( ntk.is_pi( ntk.index_to_node( leaf ) ) )
      {
        /* dereference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( --node_match[leaf].map_refs[1] == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          --node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if it is used only by the current gate and leaf node is implemented in the opposite phase */
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u && node_match[leaf].best_gate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive dereferencing */
        if ( !node_match[leaf].map_refs[0] && !node_match[leaf].map_refs[1] )
        {
          auto const& best_cut = cuts[leaf][node_match[leaf].best_cut[leaf_phase]];
          count += cut_deref<SwitchActivity>( best_cut, ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u )
        {
          auto const& best_cut = cuts[leaf][node_match[leaf].best_cut[leaf_phase]];
          count += cut_deref<SwitchActivity>( best_cut, ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }

  template<bool SwitchActivity>
  float cut_measure_mffc( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    tmp_visited.clear();

    float count = cut_ref_visit<SwitchActivity>( cut, n, phase );

    /* dereference visited */
    for ( auto s : tmp_visited )
    {
      uint32_t leaf = s >> 1;
      --node_match[leaf].map_refs[s & 1];
    }

    return count;
  }

  template<bool SwitchActivity>
  float cut_ref_visit( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    /* don't touch box */
    if constexpr ( has_is_dont_touch_v<Ntk> )
    {
      if ( ntk.is_dont_touch( n ) )
      {
        return count;
      }
    }

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }

      /* add to visited */
      tmp_visited.push_back( ( static_cast<uint64_t>( leaf ) << 1 ) | leaf_phase );

      if ( ntk.is_pi( ntk.index_to_node( leaf ) ) )
      {
        /* reference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( node_match[leaf].map_refs[1]++ == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          ++node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Recursive referencing if leaf was not referenced */
        if ( !node_match[leaf].map_refs[0] && !node_match[leaf].map_refs[1] )
        {
          auto const& best_cut = cuts[leaf][node_match[leaf].best_cut[leaf_phase]];
          count += cut_ref_visit<SwitchActivity>( best_cut, ntk.index_to_node( leaf ), leaf_phase );
        }

        /* Add inverter area if not present yet and leaf node is implemented in the opposite phase */
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u && node_match[leaf].best_gate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
      }
      else
      {
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u )
        {
          auto const& best_cut = cuts[leaf][node_match[leaf].best_cut[leaf_phase]];
          count += cut_ref_visit<SwitchActivity>( best_cut, ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }
#pragma endregion

#pragma region Initialize and dump the mapped network
  void insert_buffers()
  {
    if ( lib_buf_id != UINT32_MAX )
    {
      double area_old = area;
      bool buffers = false;

      ntk.foreach_po( [&]( auto const& f ) {
        auto const& n = ntk.get_node( f );
        if ( !ntk.is_constant( n ) && ntk.is_pi( n ) && !ntk.is_complemented( f ) )
        {
          area += lib_buf_area;
          delay = std::max( delay, node_match[ntk.node_to_index( n )].arrival[0] + lib_inv_delay );
          buffers = true;
        }
      } );

      /* round stats */
      if ( ps.verbose && buffers )
      {
        std::stringstream stats{};
        float area_gain = 0.0f;

        area_gain = float( ( area_old - area ) / area_old * 100 );

        stats << fmt::format( "[i] Buffering: Delay = {:>12.2f}  Area = {:>12.2f}  Gain = {:>5.2f} %  Inverters = {:>5}  Time = {:>5.2f}\n", delay, area, area_gain, inv, to_seconds( clock::now() - time_begin ) );
        st.round_stats.push_back( stats.str() );
      }
    }
  }

  std::pair<binding_view<klut_network>, klut_map> initialize_map_network()
  {
    binding_view<klut_network> dest( library.get_gates() );
    klut_map old2new;

    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][0] = dest.get_constant( false );
    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][1] = dest.get_constant( true );

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_pi();
    } );
    return { dest, old2new };
  }

  std::pair<cell_view<block_network>, block_map> initialize_block_network()
  {
    cell_view<block_network> dest( library.get_cells() );
    block_map old2new;

    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][0] = dest.get_constant( false );
    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][1] = dest.get_constant( true );

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_pi();
    } );
    return { dest, old2new };
  }

  void init_topo_order()
  {
    topo_order.reserve( ntk.size() );

    if ( multi_node_match.size() > 0 )
    {
      multi_init_topo_order();
      return;
    }

    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      topo_order.push_back( n );
    } );
  }

  bool init_arrivals()
  {
    if ( ps.required_times.size() && ps.required_times.size() != ntk.num_pos() )
    {
      std::cerr << "[e] MAP ERROR: required time vector does not match the output size of the network" << std::endl;
      st.mapping_error = true;
      return false;
    }

    if ( ps.arrival_times.empty() )
    {
      ntk.foreach_pi( [&]( auto const& n ) {
        auto& node_data = node_match[ntk.node_to_index( n )];
        node_data.arrival[0] = node_data.best_alternative[0].arrival = 0;
        node_data.arrival[1] = node_data.best_alternative[1].arrival = lib_inv_delay;
      } );
      return true;
    }

    if ( ps.arrival_times.size() != ntk.num_pis() )
    {
      std::cerr << "[e] MAP ERROR: arrival time vector does not match the input size of the network" << std::endl;
      st.mapping_error = true;
      return false;
    }

    ntk.foreach_pi( [&]( auto const& n, uint32_t i ) {
      auto& node_data = node_match[ntk.node_to_index( n )];
      node_data.arrival[0] = node_data.best_alternative[0].arrival = ps.arrival_times[i];
      node_data.arrival[1] = node_data.best_alternative[1].arrival = ps.arrival_times[i] + lib_inv_delay;
    } );

    return true;
  }

  void finalize_cover( binding_view<klut_network>& res, klut_map& old2new )
  {
    uint32_t multioutput_count = 0;

    for ( auto const& n : topo_order )
    {
      auto index = ntk.node_to_index( n );
      auto const& node_data = node_match[index];

      /* add inverter at PI if needed */
      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_gate[0] == nullptr && node_data.best_gate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_pi( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
        {
          old2new[index][1] = res.create_not( old2new[n][0] );
          res.add_binding( res.get_node( old2new[index][1] ), lib_inv_id );
        }
        continue;
      }

      /* continue if cut is not in the cover */
      if ( !node_data.map_refs[0] && !node_data.map_refs[1] )
        continue;

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        if ( ntk.is_dont_touch( n ) )
        {
          clone_box( res, old2new, index );
          continue;
        }
      }

      unsigned phase = ( node_data.best_gate[0] != nullptr ) ? 0 : 1;

      /* add used cut */
      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate( res, old2new, index, phase );

        /* add inverted version if used */
        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
        {
          old2new[index][phase ^ 1] = res.create_not( old2new[index][phase] );
          res.add_binding( res.get_node( old2new[index][phase ^ 1] ), lib_inv_id );
        }

        /* count multioutput gates */
        if ( ps.map_multioutput && node_tuple_match[index].lowest_index && node_data.multioutput_match[phase] )
        {
          ++multioutput_count;
        }
      }

      phase = phase ^ 1;
      /* add the optional other match if used */
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate( res, old2new, index, phase );

        /* count multioutput gates */
        if ( ps.map_multioutput && node_tuple_match[index].lowest_index && node_data.multioutput_match[phase] )
        {
          ++multioutput_count;
        }
      }

      st.multioutput_gates = multioutput_count;
    }

    /* create POs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
      }
      else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_pi( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
      {
        /* create buffers for POs */
        static uint64_t _buf = 0x2;
        kitty::dynamic_truth_table tt_buf( 1 );
        kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
        const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
        res.create_po( buf );
        res.add_binding( res.get_node( buf ), lib_buf_id );
      }
      else
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
      }
    } );

    /* write final results */
    st.area = area;
    st.delay = delay;
    if ( ps.eswp_rounds )
      st.power = compute_switching_power();
  }

  void finalize_cover_block( cell_view<block_network>& res, block_map& old2new )
  {
    uint32_t multioutput_count = 0;

    /* get standard cells */
    std::vector<standard_cell> const& lib = res.get_library();

    /* get translation ID from GENLIB to STD_CELL */
    std::vector<uint32_t> genlib_to_cell( library.get_gates().size() );
    for ( standard_cell const& cell : lib )
    {
      for ( gate const& g : cell.gates )
      {
        genlib_to_cell[g.id] = cell.id;
      }
    }

    for ( auto const& n : topo_order )
    {
      auto index = ntk.node_to_index( n );
      auto const& node_data = node_match[index];

      /* add inverter at PI if needed */
      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_gate[0] == nullptr && node_data.best_gate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_pi( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
        {
          old2new[index][1] = res.create_not( old2new[n][0] );
          res.add_cell( res.get_node( old2new[index][1] ), genlib_to_cell[lib_inv_id] );
        }
        continue;
      }

      /* continue if cut is not in the cover */
      if ( !node_data.map_refs[0] && !node_data.map_refs[1] )
        continue;

      /* don't touch box */
      if constexpr ( has_is_dont_touch_v<Ntk> )
      {
        if ( ntk.is_dont_touch( n ) )
        {
          clone_box2( res, old2new, index, genlib_to_cell );
          continue;
        }
      }

      unsigned phase = ( node_data.best_gate[0] != nullptr ) ? 0 : 1;

      /* add used cut */
      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        /* create multioutput gates */
        if ( ps.map_multioutput && node_data.multioutput_match[phase] )
        {
          assert( node_data.same_match == true );

          if ( node_tuple_match[index].has_info && node_tuple_match[index].lowest_index )
          {
            ++multioutput_count;
            create_block_for_gate( res, old2new, index, phase, genlib_to_cell );
          }
          continue;
        }

        create_lut_for_gate2( res, old2new, index, phase, genlib_to_cell );

        /* add inverted version if used */
        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
        {
          old2new[index][phase ^ 1] = res.create_not( old2new[index][phase] );
          res.add_cell( res.get_node( old2new[index][phase ^ 1] ), genlib_to_cell[lib_inv_id] );
        }
      }

      phase = phase ^ 1;
      /* add the optional other match if used */
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        assert( !ps.map_multioutput || !node_data.multioutput_match[phase] );
        create_lut_for_gate2( res, old2new, index, phase, genlib_to_cell );
      }
    }

    /* create POs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
      }
      else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_pi( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
      {
        /* create buffers for POs */
        static uint64_t _buf = 0x2;
        kitty::dynamic_truth_table tt_buf( 1 );
        kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
        const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
        res.create_po( buf );
        res.add_cell( res.get_node( buf ), genlib_to_cell[lib_buf_id] );
      }
      else
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
      }
    } );

    /* write final results */
    st.area = area;
    st.delay = delay;
    st.multioutput_gates = multioutput_count;
    if ( ps.eswp_rounds )
      st.power = compute_switching_power();
  }

  void create_lut_for_gate( binding_view<klut_network>& res, klut_map& old2new, uint32_t index, unsigned phase )
  {
    auto const& node_data = node_match[index];
    auto const& best_cut = cuts[index][node_data.best_cut[phase]];
    auto const& gate = node_data.best_gate[phase]->root;

    /* permutate and negate to obtain the matched gate truth table */
    std::vector<signal<klut_network>> children( gate->num_vars );

    auto ctr = 0u;
    for ( auto l : best_cut )
    {
      if ( ctr >= gate->num_vars )
        break;
      children[node_data.best_gate[phase]->permutation[ctr]] = old2new[l][( node_data.phase[phase] >> ctr ) & 1];
      ++ctr;
    }

    if ( !gate->is_super )
    {
      /* create the node */
      auto f = res.create_node( children, gate->function );
      res.add_binding( res.get_node( f ), gate->root->id );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
    else
    {
      /* supergate, create sub-gates */
      auto f = create_lut_for_gate_rec( res, *gate, children );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
  }

  signal<klut_network> create_lut_for_gate_rec( binding_view<klut_network>& res, composed_gate<NInputs> const& gate, std::vector<signal<klut_network>> const& children )
  {
    std::vector<signal<klut_network>> children_local( gate.fanin.size() );

    auto i = 0u;
    for ( auto const fanin : gate.fanin )
    {
      if ( fanin->root == nullptr )
      {
        /* terminal condition */
        children_local[i] = children[fanin->id];
      }
      else
      {
        children_local[i] = create_lut_for_gate_rec( res, *fanin, children );
      }
      ++i;
    }

    auto f = res.create_node( children_local, gate.root->function );
    res.add_binding( res.get_node( f ), gate.root->id );
    return f;
  }

  void create_lut_for_gate2( cell_view<block_network>& res, block_map& old2new, uint32_t index, unsigned phase, std::vector<uint32_t> const& genlib_to_cell )
  {
    auto const& node_data = node_match[index];
    auto const& best_cut = cuts[index][node_data.best_cut[phase]];
    auto const& gate = node_data.best_gate[phase]->root;

    /* permutate and negate to obtain the matched gate truth table */
    std::vector<signal<block_network>> children( gate->num_vars );

    auto ctr = 0u;
    for ( auto l : best_cut )
    {
      if ( ctr >= gate->num_vars )
        break;
      children[node_data.best_gate[phase]->permutation[ctr]] = old2new[l][( node_data.phase[phase] >> ctr ) & 1];
      ++ctr;
    }

    if ( !gate->is_super )
    {
      /* create the node */
      auto f = res.create_node( children, gate->function );
      res.add_cell( res.get_node( f ), genlib_to_cell.at( gate->root->id ) );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
    else
    {
      /* supergate, create sub-gates */
      auto f = create_lut_for_gate2_rec( res, *gate, children, genlib_to_cell );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
  }

  signal<block_network> create_lut_for_gate2_rec( cell_view<block_network>& res, composed_gate<NInputs> const& gate, std::vector<signal<block_network>> const& children, std::vector<uint32_t> const& genlib_to_cell )
  {
    std::vector<signal<block_network>> children_local( gate.fanin.size() );

    auto i = 0u;
    for ( auto const fanin : gate.fanin )
    {
      if ( fanin->root == nullptr )
      {
        /* terminal condition */
        children_local[i] = children[fanin->id];
      }
      else
      {
        children_local[i] = create_lut_for_gate2_rec( res, *fanin, children, genlib_to_cell );
      }
      ++i;
    }

    auto f = res.create_node( children_local, gate.root->function );
    res.add_cell( res.get_node( f ), genlib_to_cell.at( gate.root->id ) );
    return f;
  }

  void create_block_for_gate( cell_view<block_network>& res, block_map& old2new, uint32_t index, unsigned phase, std::vector<uint32_t> const& genlib_to_cell )
  {
    std::vector<standard_cell> const& lib = res.get_library();
    composed_gate<NInputs> const* local_gate = node_match[index].best_gate[phase]->root;
    standard_cell const& cell = lib[genlib_to_cell.at( local_gate->root->id )];

    assert( !local_gate->is_super );
    auto const& best_cut = cuts[index][node_match[index].best_cut[phase]];

    /* permutate and negate to obtain the matched gate truth table */
    std::vector<signal<block_network>> children( cell.gates.front().num_vars );

    /* output negations have already been assigned by the mapper */
    auto ctr = 0u;
    for ( auto l : best_cut )
    {
      if ( ctr >= local_gate->num_vars )
        break;
      children[node_match[index].best_gate[phase]->permutation[ctr]] = old2new[l][( node_match[index].phase[phase] >> ctr ) & 1];
      ++ctr;
    }

    multi_match_t const& tuple_data = multi_node_match[node_tuple_match[index].index][0];
    std::vector<uint32_t> outputs;
    std::vector<kitty::dynamic_truth_table> functions;

    /* re-order outputs to match the ones of the cell */
    for ( gate const& g : cell.gates )
    {
      /* find the correct node */
      for ( auto j = 0; j < max_multioutput_output_size; ++j )
      {
        uint32_t node_index = tuple_data[j].node_index;
        assert( node_match[node_index].same_match );
        uint8_t node_phase = node_match[node_index].best_gate[0] != nullptr ? 0 : 1;
        assert( node_match[node_index].multioutput_match[node_phase] );

        gate const* node_gate = node_match[node_index].best_gate[node_phase]->root->root;

        /* wrong output */
        if ( node_gate->id != g.id )
          continue;

        outputs.push_back( node_index );
        functions.push_back( g.function );
      }
    }

    assert( outputs.size() == cell.gates.size() );

    /* create the block */
    auto f = res.create_node( children, functions );
    res.add_cell( res.get_node( f ), genlib_to_cell.at( local_gate->root->id ) );

    for ( uint32_t s : outputs )
    {
      /* add inverted version if used */
      uint8_t node_phase = node_match[s].best_gate[0] != nullptr ? 0 : 1;
      assert( node_match[s].same_match );

      /* add the node in the data structure */
      old2new[s][node_phase] = f;

      if ( node_match[s].map_refs[node_phase ^ 1] > 0 )
      {
        old2new[s][node_phase ^ 1] = res.create_not( f );
        res.add_cell( res.get_node( old2new[s][node_phase ^ 1] ), genlib_to_cell.at( lib_inv_id ) );
      }

      f = res.next_output_pin( f );
    }
  }

  void clone_box( binding_view<klut_network>& res, klut_map& old2new, uint32_t index )
  {
    node<Ntk> n = ntk.index_to_node( index );
    std::vector<signal<klut_network>> children;

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      children.push_back( old2new[ntk.get_node( f )][ntk.is_complemented( f ) ? 1 : 0] );
    } );

    /* create the node */
    auto const& tt = ntk.node_function( n );
    auto f = res.create_node( children, tt );

    /* add the node in the data structure */
    old2new[index][0] = f;
    if ( node_match[index].map_refs[1] )
    {
      old2new[index][1] = res.create_not( f );
      res.add_binding( res.get_node( old2new[index][1] ), lib_inv_id );
    }

    if constexpr ( has_has_binding_v<Ntk> )
    {
      if ( ntk.has_binding( n ) )
        res.add_binding( res.get_node( f ), ntk.get_binding_index( n ) );
    }
  }

  void clone_box2( cell_view<block_network>& res, klut_map& old2new, uint32_t index, std::vector<uint32_t> const& genlib_to_cell )
  {
    node<Ntk> n = ntk.index_to_node( index );
    std::vector<signal<block_network>> children;

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      children.push_back( old2new[ntk.get_node( f )][ntk.is_complemented( f ) ? 1 : 0] );
    } );

    /* check if multi-output */
    std::vector<standard_cell> const& lib = res.get_library();
    if constexpr ( has_has_binding_v<Ntk> )
    {
      bool is_multioutput = false;
      if ( ntk.has_binding( n ) )
      {
        uint32_t cell_id = genlib_to_cell.at( ntk.get_binding_index( n ) );
        if ( lib.at( cell_id ).gates.size() > 1 )
          is_multioutput = true;
      }

      /* create the multioutput node (partially dangling) */
      if ( is_multioutput )
      {
        standard_cell const& cell = lib.at( genlib_to_cell.at( ntk.get_binding_index( n ) ) );
        std::vector<kitty::dynamic_truth_table> functions;
        for ( auto const& g : cell.gates )
        {
          functions.push_back( g.function );
        }

        auto f = res.create_node( children, functions );

        /* find and connect the correct pin */
        for ( auto const& g : cell.gates )
        {
          if ( g.id == cell.id )
            break;
          res.next_output_pin( f );
        }

        old2new[index][0] = f;
        res.add_cell( res.get_node( f ), cell.id );
        if ( node_match[index].map_refs[1] )
        {
          old2new[index][1] = res.create_not( f );
          res.add_cell( res.get_node( old2new[index][1] ), genlib_to_cell.at( lib_inv_id ) );
        }
        return;
      }
    }

    /* create the single-output node */
    auto const& tt = ntk.node_function( n );
    auto f = res.create_node( children, tt );

    /* add the node in the data structure */
    old2new[index][0] = f;
    if ( node_match[index].map_refs[1] )
    {
      old2new[index][1] = res.create_not( f );
      res.add_cell( res.get_node( old2new[index][1] ), genlib_to_cell.at( lib_inv_id ) );
    }

    if constexpr ( has_has_binding_v<Ntk> )
    {
      if ( ntk.has_binding( n ) )
        res.add_cell( res.get_node( f ), genlib_to_cell.at( ntk.get_binding_index( n ) ) );
    }
  }
#pragma endregion

#pragma region Cuts and matching utils
  void compute_cut_data( cut_t& cut, node<Ntk> const& n )
  {
    cut->delay = std::numeric_limits<uint32_t>::max();
    cut->flow = std::numeric_limits<float>::max();
    cut->ignore = false;

    if ( cut.size() > NInputs || cut.size() > 6 )
    {
      /* Ignore cuts too big to be mapped using the library */
      cut->ignore = true;
      return;
    }

    const auto tt = cut->function;
    const kitty::static_truth_table<6> fe = kitty::extend_to<6>( tt );
    auto fe_canon = fe;

    uint16_t negations_pos = 0;
    uint16_t negations_neg = 0;

    /* match positive polarity */
    if constexpr ( Configuration == classification_type::p_configurations )
    {
      auto canon = kitty::exact_n_canonization_support( fe, cut.size() );
      fe_canon = std::get<0>( canon );
      negations_pos = std::get<1>( canon );
    }

    auto const supergates_pos = library.get_supergates( fe_canon );

    /* match negative polarity */
    if constexpr ( Configuration == classification_type::p_configurations )
    {
      auto canon = kitty::exact_n_canonization_support( ~fe, cut.size() );
      fe_canon = std::get<0>( canon );
      negations_neg = std::get<1>( canon );
    }
    else
    {
      fe_canon = ~fe;
    }

    auto const supergates_neg = library.get_supergates( fe_canon );

    if ( supergates_pos != nullptr || supergates_neg != nullptr )
    {
      cut->supergates = { supergates_pos, supergates_neg };
      cut->negations = { negations_pos, negations_neg };
    }
    else
    {
      /* Ignore not matched cuts */
      cut->ignore = true;
      return;
    }

    /* compute cut cost based on LUT area */
    recompute_cut_data( cut, n );
  }

  void compute_cut_data_structural( cut_t& cut, node<Ntk> const& n )
  {
    cut->delay = std::numeric_limits<uint32_t>::max();
    cut->flow = std::numeric_limits<float>::max();
    cut->ignore = false;

    assert( cut.size() <= NInputs );

    const auto supergates_pos = library.get_supergates_pattern( cut->pattern_index, false );
    const auto supergates_neg = library.get_supergates_pattern( cut->pattern_index, true );

    if ( supergates_pos != nullptr || supergates_neg != nullptr )
    {
      cut->supergates = { supergates_pos, supergates_neg };
    }
    else
    {
      /* Ignore not matched cuts */
      cut->ignore = true;
      return;
    }

    /* compute cut cost based on LUT area */
    recompute_cut_data( cut, n );
  }

  void recompute_cut_data( cut_t& cut, node<Ntk> const& n )
  {
    /* compute cut cost based on LUT area */
    uint32_t best_arrival = 0;
    float best_area_flow = cut.size() > 1 ? cut.size() : 0;

    for ( auto leaf : cut )
    {
      const auto& best_leaf_cut = cuts[leaf][0];
      best_arrival = std::max( best_arrival, best_leaf_cut->delay );
      best_area_flow += best_leaf_cut->flow;
    }

    cut->delay = best_arrival + ( cut.size() > 1 ) ? 1 : 0;
    cut->flow = best_area_flow / ntk.fanout_size( n );
  }

  /* compute positions of leave indices in cut `sub` (subset) with respect to
   * leaves in cut `sup` (super set).
   *
   * Example:
   *   compute_truth_table_support( {1, 3, 6}, {0, 1, 2, 3, 6, 7} ) = {1, 3, 4}
   */
  void compute_truth_table_support( cut_t const& sub, cut_t const& sup, TT& tt )
  {
    size_t j = 0;
    auto itp = sup.begin();
    for ( auto i : sub )
    {
      itp = std::find( itp, sup.end(), i );
      lsupport[j++] = static_cast<uint8_t>( std::distance( sup.begin(), itp ) );
    }

    /* swap variables in the truth table */
    for ( int i = j - 1; i >= 0; --i )
    {
      assert( i <= lsupport[i] );
      kitty::swap_inplace( tt, i, lsupport[i] );
    }
  }

  void add_zero_cut( uint32_t index )
  {
    auto& cut = cuts[index].add_cut( &index, &index ); /* fake iterator for emptyness */
    cut->ignore = true;
    cut->delay = 0;
    cut->flow = 0;
    cut->pattern_index = 0;
    cut->negations[0] = cut->negations[1] = 0;
  }

  void add_unit_cut( uint32_t index )
  {
    auto& cut = cuts[index].add_cut( &index, &index + 1 );

    kitty::create_nth_var( cut->function, 0 );
    cut->ignore = true;
    cut->delay = 0;
    cut->flow = 0;
    cut->pattern_index = 1;
    cut->negations[0] = cut->negations[1] = 0;
  }

  inline void create_structural_cut( cut_t& new_cut, std::vector<cut_t const*> const& vcuts, uint32_t new_pattern, uint32_t pattern_id1, uint32_t pattern_id2 )
  {
    new_cut.set_leaves( *vcuts[0] );
    new_cut.add_leaves( vcuts[1]->begin(), vcuts[1]->end() );
    new_cut->pattern_index = new_pattern;

    /* get the polarity of the leaves of the new cut */
    uint16_t neg_l = 0, neg_r = 0;
    if ( ( *vcuts[0] )->pattern_index == 1 )
    {
      neg_r = static_cast<uint16_t>( pattern_id1 & 1 );
    }
    else
    {
      neg_r = ( *vcuts[0] )->negations[0];
    }
    if ( ( *vcuts[1] )->pattern_index == 1 )
    {
      neg_l = static_cast<uint16_t>( pattern_id2 & 1 );
    }
    else
    {
      neg_l = ( *vcuts[1] )->negations[0];
    }

    new_cut->negations[0] = ( neg_l << vcuts[0]->size() ) | neg_r;
    new_cut->negations[1] = new_cut->negations[0];
  }

  inline bool fast_support_minimization( TT const& tt, cut_t& res )
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
    }

    return true;
  }

  void compute_truth_table( uint32_t index, fanin_cut_t const& vcuts, uint32_t fanin, cut_t& res )
  {
    for ( uint32_t i = 0; i < fanin; ++i )
    {
      cut_t const* cut = vcuts[i];
      ltruth[i] = ( *cut )->function;
      compute_truth_table_support( *cut, res, ltruth[i] );
    }

    auto tt_res = ntk.compute( ntk.index_to_node( index ), ltruth.begin(), ltruth.begin() + fanin );

    if ( ps.cut_enumeration_ps.minimize_truth_table && !fast_support_minimization( tt_res, res ) )
    {
      const auto support = kitty::min_base_inplace( tt_res );

      std::vector<uint32_t> leaves_before( res.begin(), res.end() );
      std::vector<uint32_t> leaves_after( support.size() );

      auto it_support = support.begin();
      auto it_leaves = leaves_after.begin();
      while ( it_support != support.end() )
      {
        *it_leaves++ = leaves_before[*it_support++];
      }
      res.set_leaves( leaves_after.begin(), leaves_after.end() );
    }

    res->function = tt_res;
  }
#pragma endregion

  template<bool DO_AREA>
  inline bool compare_map( double arrival, double best_arrival, float area_flow, float best_area_flow, uint32_t size, uint32_t best_size )
  {
    if constexpr ( DO_AREA )
    {
      if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
      else if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
      return size < best_size;
    }
    else
    {
      if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
      else if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
      return size < best_size;
    }
  }

  double compute_switching_power()
  {
    double power = 0.0f;

    for ( auto const& n : topo_order )
    {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_gate[0] == nullptr && node_data.best_gate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_pi( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
          power += switch_activity[ntk.node_to_index( n )];
        continue;
      }

      /* continue if cut is not in the cover */
      if ( !node_data.map_refs[0] && !node_data.map_refs[1] )
        continue;

      unsigned phase = ( node_data.best_gate[0] != nullptr ) ? 0 : 1;

      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        power += switch_activity[ntk.node_to_index( n )];

        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
          power += switch_activity[ntk.node_to_index( n )];
      }

      phase = phase ^ 1;
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        power += switch_activity[ntk.node_to_index( n )];
      }
    }

    return power;
  }

#pragma region multioutput
  /* Experimental code */
  void compute_multioutput_match()
  {
    stopwatch t( st.time_multioutput );

    if ( library.num_multioutput_gates() == 0 )
      return;

    /* compute cuts: first simple method without proper matching */
    cut_enumeration_params multi_ps;
    multi_ps.minimize_truth_table = false;
    multi_cuts_t multi_cuts = fast_cut_enumeration<Ntk, max_multioutput_cut_size, true, cut_enumeration_emap_multi_cut>( ntk, multi_ps );

    /* cuts leaves classes */
    multi_hash_t multi_cuts_classes;
    multi_cuts_classes.reserve( 2000 );

    /* Multi-output matching */
    multi_enumerate_matches( multi_cuts, multi_cuts_classes );

    multi_single_matches_t multi_node_match_local;
    multi_node_match_local.reserve( multi_cuts_classes.size() );

    multi_compute_matches( multi_cuts, multi_cuts_classes, multi_node_match_local );

    if ( ps.remove_overlapping_multicuts )
      multi_filter_and_match<true>( multi_cuts, multi_node_match_local ); /* it also adds the tuple for node mapping */
    else
      multi_filter_and_match<false>( multi_cuts, multi_node_match_local ); /* it also adds the tuple for node mapping */
  }

  void multi_init_topo_order()
  {
    /* create and initialize a choice view to store the tuples */
    choice_view<Ntk> choice_ntk{ ntk };
    multi_add_choices( choice_ntk );

    ntk.incr_trav_id();
    ntk.incr_trav_id();

    /* add constants and CIs */
    const auto c0 = ntk.get_node( ntk.get_constant( false ) );
    topo_order.push_back( c0 );
    ntk.set_visited( c0, ntk.trav_id() );

    if ( const auto c1 = ntk.get_node( ntk.get_constant( true ) ); ntk.visited( c1 ) != ntk.trav_id() )
    {
      topo_order.push_back( c1 );
      ntk.set_visited( c1, ntk.trav_id() );
    }

    ntk.foreach_ci( [&]( auto const& n ) {
      if ( ntk.visited( n ) != ntk.trav_id() )
      {
        topo_order.push_back( n );
        ntk.set_visited( n, ntk.trav_id() );
      }
    } );

    /* sort topologically */
    ntk.foreach_co( [&]( auto const& f ) {
      if ( ntk.visited( ntk.get_node( f ) ) == ntk.trav_id() )
        return;
      multi_topo_sort_rec( choice_ntk, ntk.get_node( f ) );
    } );
  }

  /* Experimental code resticted to only half adders and full adders */
  void multi_enumerate_matches( multi_cuts_t const& multi_cuts, multi_hash_t& multi_cuts_classes )
  {
    static_assert( max_multioutput_cut_size > 1 && max_multioutput_cut_size < 7 );

    uint32_t counter = 0;
    multi_leaves_set_t leaves = { 0 };

    ntk.foreach_gate( [&]( auto const& n ) {
      uint32_t cut_index = 0;
      for ( auto& cut : multi_cuts.cuts( ntk.node_to_index( n ) ) )
      {
        kitty::static_truth_table<max_multioutput_cut_size> tt = multi_cuts.truth_table( *cut );
        /* reduce support for matching ID */
        uint64_t tt_id = ( cut->size() < 3 ) ? ( tt._bits & 0xF ) : tt._bits;
        uint64_t id = library.get_multi_function_id( tt_id );

        if ( !id )
        {
          ++cut_index;
          continue;
        }

        ( *cut )->data.id = id;

        multi_match_data data;
        data.node_index = ntk.node_to_index( n );
        data.cut_index = cut_index;
        leaves[2] = 0;
        uint32_t i = 0;
        for ( auto l : *cut )
          leaves[i++] = l;

        /* add to hash table */
        multi_cuts_classes[leaves].push_back( data );

        ++cut_index;
      }
    } );
  }

  /* Experimental code */
  void multi_compute_matches( multi_cuts_t const& multi_cuts, multi_hash_t& multi_cuts_classes, multi_single_matches_t& multi_node_match_local )
  {
    ntk.clear_values();

    /* copy set and sort by gate size: improve, too slow */
    std::vector<std::pair<multi_leaves_set_t, multi_output_set_t>> class_list;
    class_list.reserve( multi_cuts_classes.size() );
    for ( auto& it : multi_cuts_classes )
    {
      /* insert multiple occurring cuts */
      if ( it.second.size() > 1 )
        class_list.push_back( it );
    }

    std::stable_sort( class_list.begin(), class_list.end(), [&]( auto const& a, auto const& b ) {
      return a.first[2] > b.first[2];
    } );

    /* combine and match: specific code for 2-output cells */
    for ( auto it : class_list )
    {
      for ( uint32_t i = 0; i < it.second.size() - 1; ++i )
      {
        multi_match_data data_i = it.second[i];
        uint32_t index_i = data_i.node_index;
        uint32_t cut_index_i = data_i.cut_index;
        auto const& cut_i = multi_cuts.cuts( index_i )[cut_index_i];

        for ( uint32_t j = i + 1; j < it.second.size(); ++j )
        {
          multi_match_data data_j = it.second[j];
          uint32_t index_j = data_j.node_index;
          uint32_t cut_index_j = data_j.cut_index;
          auto const& cut_j = multi_cuts.cuts( index_j )[cut_index_j];

          /* not compatible -> TODO: change */
          if ( cut_i->data.id == cut_j->data.id )
            continue;

          /* check compatibility */
          if ( !multi_check_partally_dangling( index_i, index_j, cut_i ) )
            continue;

          multi_node_match_local.push_back( { data_i, data_j } );
        }
      }
    }
  }

  /* Experimental code */
  template<bool OverlapFilter>
  void multi_filter_and_match( multi_cuts_t const& multi_cuts, multi_single_matches_t const& multi_node_match_local )
  {
    multi_cut_set.reserve( multi_node_match_local.size() );
    multi_node_match.reserve( multi_node_match_local.size() );

    ntk.incr_trav_id();

    for ( auto& pair : multi_node_match_local )
    {
      uint32_t index1 = pair[0].node_index;
      uint32_t index2 = pair[1].node_index;
      uint32_t cut_index1 = pair[0].cut_index;
      uint32_t cut_index2 = pair[1].cut_index;
      multi_cut_t const& cut1 = multi_cuts.cuts( index1 )[cut_index1];
      multi_cut_t const& cut2 = multi_cuts.cuts( index2 )[cut_index2];

      assert( index1 < index2 );

      /* remove incompatible multi-output cuts */
      bool is_new = true;
      uint32_t insertion_index = multi_node_match.size();
      if constexpr ( OverlapFilter )
      {
        if ( multi_gate_check_overlapping( index1, index2, cut1 ) )
          continue;
      }
      else
      {
        if ( multi_gate_check_incompatible( index1, index2, is_new, insertion_index ) )
          continue;
        // if ( is_new && multi_gate_check_overlapping( index1, index2, cut1 ) )
        //   continue;
      }

      /* copy cuts */
      cut_t new_cut1, new_cut2;
      new_cut1.set_leaves( cut1.begin(), cut1.end() );
      new_cut2.set_leaves( cut2.begin(), cut2.end() );
      new_cut1->function = kitty::extend_to<6>( multi_cuts.truth_table( cut1 ) );
      new_cut2->function = kitty::extend_to<6>( multi_cuts.truth_table( cut2 ) );

      /* Multi-output Boolean matching, continue if no match */
      std::array<cut_t, max_multioutput_output_size> cut_pair = { new_cut1, new_cut2 };
      if ( !multi_compute_cut_data( cut_pair ) )
        continue;

      /* mark multioutput gate */
      if constexpr ( OverlapFilter )
      {
        multi_gate_mark_visited( index1, index2, cut1 );
        node_tuple_match[index1].has_info = 1;
        node_tuple_match[index1].lowest_index = 1;
        node_tuple_match[index1].index = multi_node_match.size();
        node_tuple_match[index2].has_info = 1;
        node_tuple_match[index2].highest_index = 1;
        node_tuple_match[index2].index = multi_node_match.size();
      }
      else
      {
        // multi_gate_mark_visited( index1, index2, cut1 );
        multi_gate_mark_compatibility( index1, index2, insertion_index );
      }

      /* add cut */
      multi_cut_set.push_back( cut_pair );

      /* re-index data */
      multi_match_data new_data1, new_data2;
      new_data1.node_index = index1;
      new_data1.cut_index = multi_cut_set.size() - 1;
      new_data2.node_index = index2;
      new_data2.cut_index = multi_cut_set.size() - 1;
      multi_match_t p = { new_data1, new_data2 };

      /* add cuts to the correct bucket */
      if ( is_new )
      {
        multi_node_match.push_back( { p } );
      }
      else
      {
        multi_node_match[insertion_index].push_back( p );
      }
    }
  }

  bool multi_compute_cut_data( std::array<cut_t, max_multioutput_output_size>& cut_tuple )
  {
    std::array<kitty::static_truth_table<6>, max_multioutput_output_size> tts;
    std::array<kitty::static_truth_table<6>, max_multioutput_output_size> tts_order;
    std::array<size_t, max_multioutput_output_size> order = {};
    std::array<uint16_t, max_multioutput_output_size> phase = { 0 };
    std::array<uint8_t, max_multioutput_output_size> phase_order;

    std::iota( order.begin(), order.end(), 0 );

    for ( auto i = 0; i < max_multioutput_output_size; ++i )
    {
      tts[i] = kitty::extend_to<6>( cut_tuple[i]->function );
      if ( ( tts[i]._bits & 1 ) == 1 )
      {
        tts[i] = ~tts[i];
        phase[i] = 1;
      }
    }

    std::stable_sort( order.begin(), order.end(), [&]( size_t a, size_t b ) {
      return tts[a] < tts[b];
    } );

    std::transform( order.begin(), order.end(), tts_order.begin(), [&]( size_t a ) {
      return tts[a];
    } );

    std::transform( order.begin(), order.end(), phase_order.begin(), [&]( uint8_t a ) {
      return phase[a];
    } );

    auto const multigates_match = library.get_multi_supergates( tts_order );

    /* Ignore not matched cuts */
    if ( multigates_match == nullptr )
      return false;

    /* add cut matches */
    for ( auto i = 0; i < max_multioutput_output_size; ++i )
    {
      cut_tuple[order[i]]->supergates[0] = nullptr;
      cut_tuple[order[i]]->supergates[1] = nullptr;
      cut_tuple[order[i]]->ignore = false;
      std::vector<supergate<NInputs>> const* multigate = &( ( *multigates_match )[i] );
      cut_tuple[order[i]]->supergates[phase_order[i]] = multigate;
    }

    return true;
  }

  inline bool multi_check_partally_dangling( uint32_t index1, uint32_t index2, multi_cut_t const& cut1 )
  {
    bool valid = true;

    /* check containment of cut1 in cut2 and viceversa */
    if ( index1 > index2 )
    {
      std::swap( index1, index2 );
    }

    ntk.foreach_fanin( ntk.index_to_node( index2 ), [&]( auto const& f ) {
      auto g = ntk.get_node( f );
      if ( ntk.node_to_index( g ) == index1 && ntk.fanout_size( g ) == 1 )
      {
        valid = false;
      }
      return valid;
    } );

    if ( !valid )
      return false;

    if ( !is_contained_mffc( ntk.index_to_node( index2 ), ntk.index_to_node( index1 ), cut1 ) )
      return false;

    return true;
  }

  inline bool multi_gate_check_overlapping( uint32_t index1, uint32_t index2, multi_cut_t const& cut )
  {
    bool contained = false;

    /* mark leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    contained = multi_mark_visited_rec<false>( ntk.index_to_node( index1 ) );
    contained |= multi_mark_visited_rec<false>( ntk.index_to_node( index2 ) );

    /* unmark leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }

    return contained;
  }

  inline bool multi_gate_check_incompatible( uint32_t index1, uint32_t index2, bool& is_new, uint32_t& data_index )
  {
    /* check cut assigned cut outputs, specialized code for 2 outputs */
    if ( !node_tuple_match[index1].has_info && !node_tuple_match[index2].has_info )
      return false;

    if ( node_tuple_match[index1].has_info && node_tuple_match[index2].has_info )
    {
      uint32_t current_assignment = node_tuple_match[index1].index;
      if ( current_assignment != node_tuple_match[index2].index )
        return true;
      is_new = false;
      data_index = current_assignment;
      return false;
    }

    return true;
  }

  inline void multi_gate_mark_compatibility( uint32_t index1, uint32_t index2, uint32_t mark_value )
  {
    node_tuple_match[index1].has_info = 1;
    node_tuple_match[index1].lowest_index = 1;
    node_tuple_match[index1].index = mark_value;
    node_tuple_match[index2].has_info = 1;
    node_tuple_match[index2].highest_index = 1;
    node_tuple_match[index2].index = mark_value;
  }

  inline void multi_gate_mark_visited( uint32_t index1, uint32_t index2, multi_cut_t const& cut )
  {
    /* mark leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    /* mark */
    multi_mark_visited_rec<true>( ntk.index_to_node( index1 ) );
    multi_mark_visited_rec<true>( ntk.index_to_node( index2 ) );

    /* unmark leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }
  }

  template<bool MARK>
  bool multi_mark_visited_rec( node<Ntk> const& n )
  {
    /* leaf */
    if ( ntk.value( n ) )
      return false;

    /* already visited */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return true;

    if constexpr ( MARK )
    {
      ntk.set_visited( n, ntk.trav_id() );
    }

    bool contained = false;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      contained |= multi_mark_visited_rec<MARK>( ntk.get_node( f ) );

      if constexpr ( !MARK )
      {
        if ( contained )
          return false;
      }

      return true;
    } );

    return contained;
  }

  bool is_contained_mffc( node<Ntk> root, node<Ntk> n, multi_cut_t const& cut )
  {
    /* reference cut leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    bool valid = true;
    tmp_visited.clear();
    dereference_node_rec( root );

    if ( ntk.fanout_size( n ) == 0 )
      valid = false;

    for ( uint64_t g : tmp_visited )
      ntk.incr_fanout_size( ntk.index_to_node( g ) );

    /* dereference leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }

    return valid;
  }

  void dereference_node_rec( node<Ntk> const& n )
  {
    /* leaf */
    if ( ntk.value( n ) )
      return;

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      node<Ntk> g = ntk.get_node( f );
      if ( ntk.decr_fanout_size( g ) == 0 )
      {
        dereference_node_rec( g );
      }
      tmp_visited.push_back( ntk.node_to_index( g ) );
    } );
  }

  void multi_add_choices( choice_view<Ntk>& choice_ntk )
  {
    for ( auto& field : multi_node_match )
    {
      auto& pair = field.front();
      uint32_t index1 = pair[0].node_index;
      uint32_t index2 = pair[1].node_index;
      uint32_t cut_index1 = pair[0].cut_index;
      cut_t const& cut = multi_cut_set[cut_index1][0];

      /* don't add choice if in TFI, set TFI bit */
      if ( multi_is_in_tfi( ntk.index_to_node( index2 ), ntk.index_to_node( index1 ), cut ) )
      {
        /* if there is a path of length > 1 linking node 1 and 2, save as TFI node */
        uint32_t in_tfi = multi_is_in_direct_tfi( ntk.index_to_node( index2 ), ntk.index_to_node( index1 ) ) ? 0 : 1;
        for ( auto& match : field )
          match[0].in_tfi = in_tfi;
        /* add a TFI dependency */
        ntk.set_value( ntk.index_to_node( index1 ), index2 );
        // multi_set_tfi_dependency( ntk.index_to_node( index2 ), ntk.index_to_node( index1 ), cut );
        continue;
      }

      choice_ntk.add_choice( ntk.index_to_node( index1 ), ntk.index_to_node( index2 ) );

      assert( choice_ntk.count_choices( ntk.index_to_node( index1 ) ) == 2 );
    }
  }

  bool multi_topo_sort_rec( choice_view<Ntk>& choice_ntk, node<Ntk> const& n )
  {
    /* is permanently marked? */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return true;

    /* loop detected: backtrack to remove the cause */
    if ( ntk.visited( n ) == ntk.trav_id() - 1 )
      return false;

    /* get the representative (smallest index) */
    node<Ntk> repr = choice_ntk.get_choice_representative( n );

    /* loop detected: backtrack to remove the cause */
    if ( ntk.visited( repr ) == ntk.trav_id() - 1 )
      return false;

    /* solve the TFI dependency first */
    node<Ntk> dependency_node = ntk.index_to_node( ntk.value( n ) );
    if ( dependency_node > 0 && ntk.visited( dependency_node ) != ntk.trav_id() - 1 )
    {
      if ( !multi_topo_sort_rec( choice_ntk, dependency_node ) )
        return false;
      assert( ntk.visited( n ) == ntk.trav_id() );
      return true;
    }

    /* for all the choices */
    uint32_t i = 0;
    bool check = true;
    choice_ntk.foreach_choice( repr, [&]( auto const& g ) {
      /* ensure that the node is not visited or temporarily marked */
      assert( ntk.visited( g ) != ntk.trav_id() );
      assert( ntk.visited( g ) != ntk.trav_id() - 1 );

      /* mark node temporarily */
      ntk.set_visited( g, ntk.trav_id() - 1 );

      /* mark children */
      ntk.foreach_fanin( g, [&]( auto const& f ) {
        check = multi_topo_sort_rec( choice_ntk, ntk.get_node( f ) );
        return check;
      } );

      /* cycle detected: backtrack to the last choice jump */
      if ( !check )
      {
        /* revert visited */
        ntk.set_visited( g, ntk.trav_id() - 2 );
        if ( i > 0 && n == repr )
        {
          /* fix cycle: remove multi-output match */
          choice_ntk.foreach_choice( repr, [&]( auto const& p ) {
            node_tuple_match[ntk.node_to_index( p )].data = 0;
            return true;
          } );
          choice_ntk.remove_choice( g );
          check = true;
        }
        return false;
      }

      ++i;
      return true;
    } );

    if ( !check )
    {
      return false;
    }

    choice_ntk.foreach_choice( repr, [&]( auto const& g ) {
      /* ensure that the node is not visited */
      assert( ntk.visited( g ) != ntk.trav_id() );

      /* mark node n permanently */
      ntk.set_visited( g, ntk.trav_id() );

      /* visit node */
      topo_order.push_back( g );

      return true;
    } );

    return true;
  }

  inline bool multi_is_in_tfi( node<Ntk> const& root, node<Ntk> const& n, cut_t const& cut )
  {
    /* reference cut leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    ntk.incr_trav_id();
    multi_mark_visited_rec<true>( root );
    bool contained = ntk.visited( n ) == ntk.trav_id();

    /* dereference leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }

    return contained;
  }

  inline bool multi_is_in_direct_tfi( node<Ntk> const& root, node<Ntk> const& n )
  {
    bool contained = false;

    ntk.foreach_fanin( root, [&]( auto const& f ) {
      if ( ntk.get_node( f ) == n )
        contained = true;
    } );

    return contained;
  }

  inline void multi_set_tfi_dependency( node<Ntk> const& root, node<Ntk> const& n, cut_t const& cut )
  {
    /* reference cut leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    ntk.incr_trav_id();

    /* add a TFI dependencies */
    ntk.set_value( n, ntk.node_to_index( root ) );
    ntk.set_visited( n, ntk.trav_id() );
    multi_set_tfi_dependency_rec( root, ntk.node_to_index( root ) );

    /* reset root's dependency info */
    ntk.set_value( root, 0 );

    /* dereference leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }
  }

  void multi_set_tfi_dependency_rec( node<Ntk> const& n, uint32_t const dependency_info )
  {
    /* leaf */
    if ( ntk.value( n ) )
      return;

    /* already visited */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;

    ntk.set_visited( n, ntk.trav_id() );
    ntk.set_value( n, dependency_info );

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      multi_set_tfi_dependency_rec( ntk.get_node( f ), dependency_info );
    } );
  }
#pragma endregion

private:
  Ntk const& ntk;
  tech_library<NInputs, Configuration> const& library;
  emap_params const& ps;
  emap_stats& st;

  uint32_t iteration{ 0 }; /* current mapping iteration */
  double delay{ 0.0f };    /* current delay of the mapping */
  double area{ 0.0f };     /* current area of the mapping */
  uint32_t inv{ 0 };       /* current inverter count */

  /* lib inverter info */
  float lib_inv_area;
  float lib_inv_delay;
  uint32_t lib_inv_id;

  /* lib buffer info */
  float lib_buf_area;
  float lib_buf_delay;
  uint32_t lib_buf_id;

  std::vector<node<Ntk>> topo_order;
  node_match_t node_match;
  std::vector<multioutput_info> node_tuple_match;
  std::vector<float> switch_activity;
  std::vector<uint64_t> tmp_visited;

  /* cut computation */
  std::vector<cut_set_t> cuts; /* compressed representation of cuts */
  cut_merge_t lcuts;           /* cut merger container */
  cut_set_t temp_cuts;         /* temporary cut set container */
  truth_compute_t ltruth;      /* truth table merger container */
  support_t lsupport;          /* support merger container */
  uint32_t cuts_total{ 0 };    /* current computed cuts */

  /* multi-output matching */
  multi_cut_set_t multi_cut_set;    /* set of multi-output cuts */
  multi_matches_t multi_node_match; /* matched multi-output gates */

  time_point time_begin;
};

} /* namespace detail */

/*! \brief Technology mapping.
 *
 * This function implements a technology mapping algorithm.
 *
 * The function takes the size of the cuts in the template parameter `CutSize`.
 *
 * The function returns a block network that supports multi-output cells.
 *
 * The novelties of this mapper are contained in 2 publications:
 * - A. Tempia Calvino and G. De Micheli, "Technology Mapping Using Multi-Output Library Cells," ICCAD, 2023.
 * - G. Radi, A. Tempia Calvino, and G. De Micheli, "In Medio Stat Virtus: Combining Boolean and Pattern Matching," ASP-DAC, 2024.
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
 *
 * \param ntk Network
 * \param library Technology library
 * \param ps Mapping params
 * \param pst Mapping statistics
 *
 */
template<unsigned CutSize = 6u, class Ntk, unsigned NInputs, classification_type Configuration>
cell_view<block_network> emap( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, emap_params const& ps = {}, emap_stats* pst = nullptr )
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

  emap_stats st;
  detail::emap_impl<Ntk, CutSize, NInputs, Configuration> p( ntk, library, ps, st );
  auto res = p.run_block();

  if ( ps.verbose && !st.mapping_error )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
  return res;
}

/*! \brief Technology mapping.
 *
 * This function implements a technology mapping algorithm.
 *
 * The function takes the size of the cuts in the template parameter `CutSize`.
 *
 * The function returns a k-LUT network. Each LUT abstacts a gate of the technology library.
 *
 * The novelties of this mapper are contained in 2 publications:
 * - A. Tempia Calvino and G. De Micheli, "Technology Mapping Using Multi-Output Library Cells," ICCAD, 2023.
 * - G. Radi, A. Tempia Calvino, and G. De Micheli, "In Medio Stat Virtus: Combining Boolean and Pattern Matching," ASP-DAC, 2024.
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
 *
 * \param ntk Network
 * \param library Technology library
 * \param ps Mapping params
 * \param pst Mapping statistics
 *
 */
template<unsigned CutSize = 6u, class Ntk, unsigned NInputs, classification_type Configuration>
binding_view<klut_network> emap_klut( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, emap_params const& ps = {}, emap_stats* pst = nullptr )
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

  emap_stats st;
  detail::emap_impl<Ntk, CutSize, NInputs, Configuration> p( ntk, library, ps, st );
  auto res = p.run_klut();

  if ( ps.verbose && !st.mapping_error )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
  return res;
}

/*! \brief Technology node mapping.
 *
 * This function implements a simple technology mapping algorithm.
 * The algorithm maps each node to the best implementation in the technology library.
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
 * - `has_binding`
 *
 * \param ntk Network
 * \param library Technology library
 * \param ps Mapping params
 * \param pst Mapping statistics
 *
 */
template<unsigned CutSize = 6u, class Ntk, unsigned NInputs, classification_type Configuration>
binding_view<klut_network> emap_node_map( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, emap_params const& ps = {}, emap_stats* pst = nullptr )
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
  static_assert( has_has_binding_v<Ntk>, "Ntk does not implement the has_binding method" );

  emap_stats st;
  detail::emap_impl<Ntk, CutSize, NInputs, Configuration> p( ntk, library, ps, st );
  auto res = p.run_node_map();

  if ( ps.verbose && !st.mapping_error )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
  return res;
}

/*! \brief Technology node mapping.
 *
 * This function implements a simple technology mapping algorithm.
 * The algorithm maps each node to the first implementation in the technology library.
 *
 * The input must be a binding_view with the gates correctly loaded.
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
 * - `has_binding`
 *
 * \param ntk Network
 *
 */
template<class Ntk>
void emap_load_mapping( Ntk& ntk )
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
  static_assert( has_has_binding_v<Ntk>, "Ntk does not implement the has_binding method" );

  /* build the library map */
  using lib_t = std::unordered_map<kitty::dynamic_truth_table, uint32_t, kitty::hash<kitty::dynamic_truth_table>>;
  lib_t tt_to_gate;

  for ( auto const& g : ntk.get_library() )
  {
    tt_to_gate[g.function] = g.id;
  }

  ntk.foreach_gate( [&]( auto const& n ) {
    if ( auto it = tt_to_gate.find( ntk.node_function( n ) ); it != tt_to_gate.end() )
    {
      ntk.add_binding( n, it->second );
    }
    else
    {
      std::cout << fmt::format( "[e] node mapping for node {} failed: no match in the tech library\n", ntk.node_to_index( n ) );
    }
  } );
}

} /* namespace mockturtle */