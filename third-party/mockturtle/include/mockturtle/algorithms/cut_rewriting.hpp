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
  \file cut_rewriting.hpp
  \brief Cut rewriting

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <optional>
#include <set>
#include <type_traits>
#include <vector>

#include "../networks/klut.hpp"
#include "../networks/mig.hpp"
#include "../traits.hpp"
#include "../utils/cost_functions.hpp"
#include "../utils/node_map.hpp"
#include "../utils/progress_bar.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/cut_view.hpp"
#include "../views/depth_view.hpp"
#include "../views/fanout_view.hpp"
#include "cleanup.hpp"
#include "cut_enumeration.hpp"
#include "detail/mffc_utils.hpp"
#include "dont_cares.hpp"

#include <fmt/format.h>
#include <kitty/print.hpp>

namespace mockturtle
{

/*! \brief Parameters for cut_rewriting.
 *
 * The data structure `cut_rewriting_params` holds configurable parameters with
 * default arguments for `cut_rewriting`.
 */
struct cut_rewriting_params
{
  cut_rewriting_params()
  {
    cut_enumeration_ps.cut_size = 6;
    cut_enumeration_ps.cut_limit = 12;
    cut_enumeration_ps.minimize_truth_table = true;
  }

  /*! \brief Cut enumeration parameters. */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Allow zero-gain substitutions. */
  bool allow_zero_gain{ false };

  /*! \brief Use don't cares for optimization. */
  bool use_dont_cares{ false };

  /*! \brief Candidate selection strategy. */
  enum
  {
    minimize_weight,
    greedy
  } candidate_selection_strategy = minimize_weight;

  /*! \brief Minimum candidate cut size */
  uint32_t min_cand_cut_size{ 3u };

  /*! \brief Minimum candidate cut size override (in conflict graph) */
  std::optional<uint32_t> min_cand_cut_size_override{};

  /*! \brief If true, candidates are only accepted if they do not increase logic level of node. */
  bool preserve_depth{ false };

  /*! \brief Show progress. */
  bool progress{ false };

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Be very verbose. */
  bool very_verbose{ false };
};

/*! \brief Statistics for cut_rewriting.
 *
 * The data structure `cut_rewriting_stats` provides data collected by running
 * `cut_rewriting`.
 */
struct cut_rewriting_stats
{
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Runtime for cut enumeration. */
  stopwatch<>::duration time_cuts{ 0 };

  /*! \brief Accumulated runtime for rewriting. */
  stopwatch<>::duration time_rewriting{ 0 };

  /*! \brief Runtime to find minimal independent set. */
  stopwatch<>::duration time_mis{ 0 };

  void report( bool show_time_mis = true ) const
  {
    fmt::print( "[i] total time     = {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i] cut enum. time = {:>5.2f} secs\n", to_seconds( time_cuts ) );
    fmt::print( "[i] rewriting time = {:>5.2f} secs\n", to_seconds( time_rewriting ) );
    if ( show_time_mis )
    {
      fmt::print( "[i] ind. set time  = {:>5.2f} secs\n", to_seconds( time_mis ) );
    }
  }
};

namespace detail
{

class graph
{
public:
  auto add_vertex( uint32_t weight )
  {
    auto index = _weights.size();
    _weights.emplace_back( weight );
    _adjacent.emplace_back();
    ++_num_vertices;
    return index;
  }

  void add_edge( uint32_t v1, uint32_t v2 )
  {
    if ( v1 == v2 )
      return;
    if ( _adjacent[v1].count( v2 ) )
      return;

    _adjacent[v1].insert( v2 );
    _adjacent[v2].insert( v1 );
    ++_num_edges;
  }

  void remove_vertex( uint32_t vertex )
  {
    assert( _weights[vertex] != -1 );
    _weights[vertex] = -1;

    _num_edges -= _adjacent[vertex].size();

    for ( auto w : _adjacent[vertex] )
    {
      _adjacent[w].erase( vertex );
    }
    _adjacent[vertex].clear();

    --_num_vertices;
  }

  bool has_vertex( uint32_t vertex ) const
  {
    return _weights[vertex] >= 0;
  }

  template<typename Fn>
  void foreach_adjacent( uint32_t vertex, Fn&& fn ) const
  {
    std::for_each( _adjacent[vertex].begin(), _adjacent[vertex].end(), fn );
  }

  template<typename Fn>
  void foreach_vertex( Fn&& fn ) const
  {
    for ( auto i = 0u; i < _weights.size(); ++i )
    {
      if ( has_vertex( i ) )
      {
        fn( i );
      }
    }
  }

  auto degree( uint32_t vertex ) const { return _adjacent[vertex].size(); }
  auto weight( uint32_t vertex ) const { return _weights[vertex]; }
  auto gwmin_value( uint32_t vertex ) const { return (double)weight( vertex ) / ( degree( vertex ) + 1 ); }
  auto gwmax_value( uint32_t vertex ) const { return (double)weight( vertex ) / ( degree( vertex ) * ( degree( vertex ) + 1 ) ); }

  auto num_vertices() const { return _num_vertices; }
  auto num_edges() const { return _num_edges; }

private:
  uint32_t _num_vertices{ 0u };
  std::size_t _num_edges{ 0u };

  std::vector<std::set<uint32_t>> _adjacent;

  std::vector<int32_t> _weights; /* degree = -1 means vertex is removed */
};

inline std::vector<uint32_t> maximum_weighted_independent_set_gwmin( graph& g )
{
  std::vector<uint32_t> mwis;

  std::vector<uint32_t> vertices( g.num_vertices() );
  std::iota( vertices.begin(), vertices.end(), 0 );

  std::stable_sort( vertices.begin(), vertices.end(), [&g]( auto v, auto w ) {
    const auto value_v = g.gwmin_value( v );
    const auto value_w = g.gwmin_value( w );
    return value_v > value_w || ( value_v == value_w && g.degree( v ) > g.degree( w ) );
  } );

  for ( auto i : vertices )
  {
    if ( !g.has_vertex( i ) )
      continue;

    /* add vertex to independent set, then remove it and all its neighbors */
    mwis.emplace_back( i );
    std::vector<uint32_t> neighbors;
    g.foreach_adjacent( i, [&]( auto v ) { neighbors.emplace_back( v ); } );
    g.remove_vertex( i );

    for ( auto v : neighbors )
    {
      g.remove_vertex( v );
    }
  }

  return mwis;
}

inline std::vector<uint32_t> maximal_weighted_independent_set( graph& g )
{
  std::vector<uint32_t> mwis;

  auto num_vertices = g.num_vertices();
  for ( auto i = 0u; i < num_vertices; ++i )
  {
    if ( !g.has_vertex( i ) )
      continue;

    /* add vertex to independent set, then remove it and all its neighbors */
    mwis.emplace_back( i );
    std::vector<uint32_t> neighbors;
    g.foreach_adjacent( i, [&]( auto v ) { neighbors.emplace_back( v ); } );
    g.remove_vertex( i );

    for ( auto v : neighbors )
    {
      g.remove_vertex( v );
    }
  }

  return mwis;
}

struct cut_enumeration_cut_rewriting_cut
{
  int32_t gain{ -1 };
};

template<typename Ntk, bool ComputeTruth>
std::tuple<graph, std::vector<std::pair<node<Ntk>, uint32_t>>> network_cuts_graph( Ntk const& ntk, network_cuts<Ntk, ComputeTruth, cut_enumeration_cut_rewriting_cut> const& cuts, cut_rewriting_params const& ps )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_clear_visited_v<Ntk>, "Ntk does not implement the clear_visited method" );

  graph g;

  using cut_addr = std::pair<node<Ntk>, uint32_t>;
  std::vector<std::vector<cut_addr>> conflicts( cuts.nodes_size() );
  std::vector<cut_addr> vertex_to_cut_addr;
  std::vector<std::vector<uint32_t>> cut_addr_to_vertex( cuts.nodes_size() );

  ntk.clear_visited();

  ntk.foreach_node( [&]( auto const& n, auto index ) {
    if ( index >= cuts.nodes_size() || ntk.is_constant( n ) || ntk.is_pi( n ) )
      return;

    if ( mffc_size( ntk, n ) == 1 )
      return;

    const auto& set = cuts.cuts( ntk.node_to_index( n ) );

    auto cctr{ 0u };
    for ( auto const& cut : set )
    {
      if ( ps.min_cand_cut_size_override )
      {
        if ( cut->size() < *ps.min_cand_cut_size_override )
          continue;
      }
      else if ( cut->size() < ps.min_cand_cut_size )
        continue;

      if ( ( *cut )->data.gain < ( ps.allow_zero_gain ? 0 : 1 ) )
        continue;

      std::vector<node<Ntk>> leaves;
      for ( auto leaf_index : *cut )
      {
        leaves.push_back( ntk.index_to_node( leaf_index ) );
      }
      cut_view<Ntk> dcut( ntk, leaves, ntk.make_signal( n ) );
      dcut.foreach_gate( [&]( auto const& n2 ) {
        // if ( dcut.is_constant( n2 ) || dcut.is_pi( n2 ) )
        //   return;
        conflicts[ntk.node_to_index( n2 )].emplace_back( n, cctr );
      } );

      auto v = g.add_vertex( ( *cut )->data.gain );
      assert( v == vertex_to_cut_addr.size() );
      vertex_to_cut_addr.emplace_back( n, cctr );
      cut_addr_to_vertex[ntk.node_to_index( n )].emplace_back( static_cast<uint32_t>( v ) );

      ++cctr;
    }
  } );

  for ( auto n = 0u; n < conflicts.size(); ++n )
  {
    for ( auto j = 1u; j < conflicts[n].size(); ++j )
    {
      for ( auto i = 0u; i < j; ++i )
      {
        const auto [n1, c1] = conflicts[n][i];
        const auto [n2, c2] = conflicts[n][j];

        if ( cut_addr_to_vertex[ntk.node_to_index( n1 )][c1] != cut_addr_to_vertex[ntk.node_to_index( n2 )][c2] )
        {
          g.add_edge( cut_addr_to_vertex[ntk.node_to_index( n1 )][c1], cut_addr_to_vertex[ntk.node_to_index( n2 )][c2] );
        }
      }
    }
  }

  return { g, vertex_to_cut_addr };
}

template<class Ntk, class RewritingFn, class Iterator, class = void>
struct has_rewrite_with_dont_cares : std::false_type
{
};

template<class Ntk, class RewritingFn, class Iterator>
struct has_rewrite_with_dont_cares<Ntk,
                                   RewritingFn, Iterator,
                                   std::void_t<decltype( std::declval<RewritingFn>()( std::declval<Ntk&>(),
                                                                                      std::declval<kitty::dynamic_truth_table>(),
                                                                                      std::declval<kitty::dynamic_truth_table>(),
                                                                                      std::declval<Iterator const&>(),
                                                                                      std::declval<Iterator const&>(),
                                                                                      std::declval<void( signal<Ntk> )>() ) )>> : std::true_type
{
};

template<class Ntk, class RewritingFn, class Iterator>
inline constexpr bool has_rewrite_with_dont_cares_v = has_rewrite_with_dont_cares<Ntk, RewritingFn, Iterator>::value;

template<class Ntk, class RewritingFn, class NodeCostFn>
class cut_rewriting_with_compatibility_graph_impl
{
public:
  cut_rewriting_with_compatibility_graph_impl( Ntk& ntk, RewritingFn&& rewriting_fn, cut_rewriting_params const& ps, cut_rewriting_stats& st, NodeCostFn const& cost_fn )
      : ntk( ntk ),
        rewriting_fn( rewriting_fn ),
        ps( ps ),
        st( st ),
        cost_fn( cost_fn ) {}

  void run()
  {
    stopwatch t( st.time_total );

    /* enumerate cuts */
    const auto cuts = call_with_stopwatch( st.time_cuts, [&]() { return cut_enumeration<Ntk, true, cut_enumeration_cut_rewriting_cut>( ntk, ps.cut_enumeration_ps ); } );

    /* for cost estimation we use reference counters initialized by the fanout size */
    ntk.clear_values();
    ntk.foreach_node( [&]( auto const& n ) {
      ntk.set_value( n, ntk.fanout_size( n ) );
    } );

    /* store best replacement for each cut */
    node_map<std::vector<signal<Ntk>>, Ntk> best_replacements( ntk );

    /* iterate over all original nodes in the network */
    const auto size = ntk.size();
    auto max_total_gain = 0u;
    progress_bar pbar{ ntk.size(), "cut_rewriting |{0}| node = {1:>4}@{2:>2} / " + std::to_string( size ) + "   comm. gain = {3}", ps.progress };
    ntk.foreach_node( [&]( auto const& n, auto index ) {
      /* stop once all original nodes were visited */
      if ( index >= size )
        return false;

      /* do not iterate over constants or PIs */
      if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
        return true;

      /* skip cuts with small MFFC */
      if ( mffc_size( ntk, n ) == 1 )
        return true;

      /* foreach cut */
      for ( auto& cut : cuts.cuts( ntk.node_to_index( n ) ) )
      {
        /* skip trivial cuts */
        if ( cut->size() < ps.min_cand_cut_size )
          continue;

        const auto tt = cuts.truth_table( *cut );
        assert( cut->size() == static_cast<unsigned>( tt.num_vars() ) );

        pbar( index, ntk.node_to_index( n ), best_replacements[n].size(), max_total_gain );

        std::vector<signal<Ntk>> children;
        for ( auto l : *cut )
        {
          children.push_back( ntk.make_signal( ntk.index_to_node( l ) ) );
        }

        int32_t value = recursive_deref<Ntk, NodeCostFn>( ntk, n );
        {
          stopwatch t( st.time_rewriting );
          int32_t best_gain{ -1 };

          const auto on_signal = [&]( auto const& f_new ) {
            auto [v, contains] = recursive_ref_contains( ntk.get_node( f_new ), n );
            recursive_deref<Ntk, NodeCostFn>( ntk, ntk.get_node( f_new ) );

            int32_t gain = contains ? -1 : value - v;

            if ( gain > 0 || ( ps.allow_zero_gain && gain == 0 ) )
            {
              if ( best_gain == -1 )
              {
                ( *cut )->data.gain = best_gain = gain;
                best_replacements[n].push_back( f_new );
              }
              else if ( gain > best_gain )
              {
                ( *cut )->data.gain = best_gain = gain;
                best_replacements[n].back() = f_new;
              }
            }

            return true;
          };

          if ( ps.use_dont_cares )
          {
            if constexpr ( has_rewrite_with_dont_cares_v<Ntk, RewritingFn, decltype( children.begin() )> )
            {
              std::vector<node<Ntk>> pivots;
              for ( auto const& c : children )
              {
                pivots.push_back( ntk.get_node( c ) );
              }
              rewriting_fn( ntk, cuts.truth_table( *cut ), satisfiability_dont_cares( ntk, pivots ), children.begin(), children.end(), on_signal );
            }
            else
            {
              rewriting_fn( ntk, cuts.truth_table( *cut ), children.begin(), children.end(), on_signal );
            }
          }
          else
          {
            rewriting_fn( ntk, cuts.truth_table( *cut ), children.begin(), children.end(), on_signal );
          }

          if ( best_gain > 0 )
          {
            max_total_gain += best_gain;
          }
        }

        recursive_ref<Ntk, NodeCostFn>( ntk, n );
      }

      return true;
    } );

    stopwatch t2( st.time_mis );
    auto [g, map] = network_cuts_graph( ntk, cuts, ps );

    if ( ps.very_verbose )
    {
      std::cout << "[i] replacement dependency graph has " << g.num_vertices() << " vertices and " << g.num_edges() << " edges\n";
    }

    const auto is = ( ps.candidate_selection_strategy == cut_rewriting_params::minimize_weight ) ? maximum_weighted_independent_set_gwmin( g ) : maximal_weighted_independent_set( g );

    if ( ps.very_verbose )
    {
      std::cout << "[i] size of independent set is " << is.size() << "\n";
    }

    for ( const auto v : is )
    {
      const auto v_node = map[v].first;
      const auto v_cut = map[v].second;

      if ( ps.very_verbose )
      {
        std::cout << "[i] try to rewrite cut #" << v_cut << " in node #" << ntk.node_to_index( v_node ) << "\n";
      }

      if ( best_replacements[v_node].empty() )
        continue;

      const auto replacement = best_replacements[v_node][v_cut];

      if ( ntk.is_constant( ntk.get_node( replacement ) ) || v_node == ntk.get_node( replacement ) )
        continue;

      if ( ps.very_verbose )
      {
        std::cout << "[i] optimize cut #" << v_cut << " in node #" << ntk.node_to_index( v_node ) << " and replace with node " << ntk.node_to_index( ntk.get_node( replacement ) ) << "\n";
      }

      ntk.substitute_node( v_node, replacement );
    }
  }

private:
  std::pair<int32_t, bool> recursive_ref_contains( node<Ntk> const& n, node<Ntk> const& repl )
  {
    /* terminate? */
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return { 0, false };

    /* recursively collect nodes */
    int32_t value = cost_fn( ntk, n );
    bool contains = ( n == repl );
    ntk.foreach_fanin( n, [&]( auto const& s ) {
      contains = contains || ( ntk.get_node( s ) == repl );
      if ( ntk.incr_value( ntk.get_node( s ) ) == 0 )
      {
        const auto [v, c] = recursive_ref_contains( ntk.get_node( s ), repl );
        value += v;
        contains = contains || c;
      }
    } );
    return { value, contains };
  }

private:
  Ntk& ntk;
  RewritingFn&& rewriting_fn;
  cut_rewriting_params const& ps;
  cut_rewriting_stats& st;
  NodeCostFn cost_fn;
};

} /* namespace detail */

/*! \brief In-place cut rewriting algorithm with compatibility graph.
 *
 * This algorithm enumerates cut of a network and then tries to rewrite the cut
 * in terms of gates of the same network.  The rewritten structures are added
 * to the network, and if they lead to area improvement, will be used as new
 * parts of the logic.  The resulting network therefore has a lot of dangling
 * nodes from unsuccessful candidates, which can be removed by calling
 * `cleanup_dangling` after the rewriting algorithm.
 *
 * The rewriting function must be of type `NtkDest::signal(NtkDest&,
 * kitty::dynamic_truth_table const&, LeavesIterator, LeavesIterator)` where
 * `LeavesIterator` can be dereferenced to a `NtkDest::signal`.  The last two
 * parameters compose an iterator pair where the distance matches the number of
 * variables of the truth table that is passed as second parameter.  There are
 * some rewriting algorithms in the folder
 * `mockturtle/algorithms/node_resynthesis`, since the resynthesis functions
 * have the same signature.
 *
 * In contrast to node resynthesis, cut rewriting uses the same type for the
 * input and output network.  Consequently, the algorithm does not return a
 * new network but applies changes in-place to the input network.
 *
 * **Required network functions:**
 * - `fanout_size`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `is_constant`
 * - `is_pi`
 * - `clear_values`
 * - `incr_value`
 * - `decr_value`
 * - `set_value`
 * - `node_to_index`
 * - `index_to_node`
 * - `substitute_node`
 * - `make_signal`
 *
 * \param ntk Network (will be modified)
 * \param rewriting_fn Rewriting function
 * \param ps Rewriting params
 * \param pst Rewriting statistics
 * \param cost_fn Node cost function (a functor with signature `uint32_t(Ntk const&, node<Ntk> const&)`)
 */
template<class Ntk, class RewritingFn, class NodeCostFn = unit_cost<Ntk>>
void cut_rewriting_with_compatibility_graph( Ntk& ntk, RewritingFn&& rewriting_fn, cut_rewriting_params const& ps = {}, cut_rewriting_stats* pst = nullptr, NodeCostFn const& cost_fn = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_incr_value_v<Ntk>, "Ntk does not implement the incr_value method" );
  static_assert( has_decr_value_v<Ntk>, "Ntk does not implement the decr_value method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );

  cut_rewriting_stats st;
  if constexpr ( std::is_same_v<typename Ntk::base_type, klut_network> )
  {
    detail::cut_rewriting_with_compatibility_graph_impl<Ntk, RewritingFn, NodeCostFn> p( ntk, rewriting_fn, ps, st, cost_fn );
    p.run();
  }
  else
  {
    fanout_view_params fvps;
    fvps.update_on_delete = false;
    fanout_view<Ntk> ntk_fo{ ntk, fvps };
    detail::cut_rewriting_with_compatibility_graph_impl<fanout_view<Ntk>, RewritingFn, NodeCostFn> p( ntk_fo, rewriting_fn, ps, st, cost_fn );
    p.run();
  }

  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

namespace detail
{

template<class NtkDest, class Ntk, class RewritingFn, class NodeCostFn>
struct cut_rewriting_impl
{
  cut_rewriting_impl( Ntk const& ntk, RewritingFn const& rewriting_fn, cut_rewriting_params const& ps, cut_rewriting_stats& st )
      : ntk_( ntk ),
        rewriting_fn_( rewriting_fn ),
        ps_( ps ),
        st_( st ) {}

  NtkDest run()
  {
    stopwatch t( st_.time_total );

    /* initial node map */
    node_map<signal<Ntk>, Ntk> old2new( ntk_ );
    Ntk res;
    old2new[ntk_.get_constant( false )] = res.get_constant( false );
    if ( ntk_.get_node( ntk_.get_constant( true ) ) != ntk_.get_node( ntk_.get_constant( false ) ) )
    {
      old2new[ntk_.get_constant( true )] = res.get_constant( true );
    }
    ntk_.foreach_pi( [&]( auto const& n ) {
      old2new[n] = res.create_pi();
    } );

    /* enumerate cuts */
    const auto cuts = call_with_stopwatch( st_.time_cuts, [&]() { return cut_enumeration<Ntk, true, cut_enumeration_cut_rewriting_cut>( ntk_, ps_.cut_enumeration_ps ); } );

    /* for cost estimation we use reference counters initialized by the fanout size */
    initialize_values_with_fanout( ntk_ );

    /* original cost */
    const auto orig_cost = costs<Ntk, NodeCostFn>( ntk_ );

    progress_bar pbar{ ntk_.num_gates(), "cut_rewriting |{0}| node = {1:>4} / " + std::to_string( ntk_.num_gates() ) + "   original cost = " + std::to_string( orig_cost ), ps_.progress };
    ntk_.foreach_gate( [&]( auto const& n, auto i ) {
      pbar( i, i );

      /* nothing to optimize? */
      int32_t value = mffc_size<Ntk, NodeCostFn>( ntk_, n );
      if ( value == 1 )
      {
        std::vector<signal<Ntk>> children( ntk_.fanin_size( n ) );
        ntk_.foreach_fanin( n, [&]( auto const& f, auto i ) {
          children[i] = ntk_.is_complemented( f ) ? res.create_not( old2new[f] ) : old2new[f];
        } );

        old2new[n] = res.clone_node( ntk_, n, children );
      }
      else
      {
        /* foreach cut */
        int32_t best_gain = -1;
        signal<Ntk> best_signal;
        for ( auto& cut : cuts.cuts( ntk_.node_to_index( n ) ) )
        {
          /* skip small enough cuts */
          if ( cut->size() == 1 || cut->size() < ps_.min_cand_cut_size )
            continue;

          const auto tt = cuts.truth_table( *cut );
          assert( cut->size() == static_cast<unsigned>( tt.num_vars() ) );

          std::vector<signal<Ntk>> children( cut->size() );
          auto ctr = 0u;
          for ( auto l : *cut )
          {
            children[ctr++] = old2new[ntk_.index_to_node( l )];
          }

          const auto on_signal = [&]( auto const& f_new ) {
            auto value2 = recursive_ref<Ntk, NodeCostFn>( res, res.get_node( f_new ) );
            recursive_deref<Ntk, NodeCostFn>( res, res.get_node( f_new ) );
            int32_t gain = value - value2;

            if ( ( gain > 0 || ( ps_.allow_zero_gain && gain == 0 ) ) && gain > best_gain )
            {
              if constexpr ( has_level_v<Ntk> )
              {
                if ( !ps_.preserve_depth || res.level( res.get_node( f_new ) ) <= ntk_.level( n ) )
                {
                  best_gain = gain;
                  best_signal = f_new;
                }
              }
              else
              {
                best_gain = gain;
                best_signal = f_new;
              }
            }

            return true;
          };
          stopwatch<> t( st_.time_rewriting );
          rewriting_fn_( res, cuts.truth_table( *cut ), children.begin(), children.end(), on_signal );
        }

        if ( best_gain == -1 )
        {
          std::vector<signal<Ntk>> children( ntk_.fanin_size( n ) );
          ntk_.foreach_fanin( n, [&]( auto const& f, auto i ) {
            children[i] = ntk_.is_complemented( f ) ? res.create_not( old2new[f] ) : old2new[f];
          } );

          old2new[n] = res.clone_node( ntk_, n, children );
        }
        else
        {
          old2new[n] = best_signal;
        }
      }

      recursive_ref<Ntk, NodeCostFn>( res, res.get_node( old2new[n] ) );
    } );

    /* create POs */
    ntk_.foreach_po( [&]( auto const& f ) {
      res.create_po( ntk_.is_complemented( f ) ? res.create_not( old2new[f] ) : old2new[f] );
    } );

    NtkDest ret = cleanup_dangling<NtkDest>( res );

    /* new costs */
    return costs<NtkDest, NodeCostFn>( ret ) > orig_cost ? static_cast<NtkDest>( ntk_ ) : ret;
  }

private:
  Ntk const& ntk_;
  RewritingFn const& rewriting_fn_;
  cut_rewriting_params const& ps_;
  cut_rewriting_stats& st_;
};

} // namespace detail

/*! \brief Cut rewriting algorithm.
 *
 * This algorithm enumerates cut of a network and then tries to rewrite the cut
 * in terms of gates of the same network.  The rewritten structures are added
 * to the network, and if they lead to area improvement, will be used as new
 * parts of the logic.
 *
 * The rewriting function must be of type `NtkDest::signal(NtkDest&,
 * kitty::dynamic_truth_table const&, LeavesIterator, LeavesIterator)` where
 * `LeavesIterator` can be dereferenced to a `NtkDest::signal`.  The last two
 * parameters compose an iterator pair where the distance matches the number of
 * variables of the truth table that is passed as second parameter.  There are
 * some rewriting algorithms in the folder
 * `mockturtle/algorithms/node_resynthesis`, since the resynthesis functions
 * have the same signature.
 *
 * In contrast to node resynthesis, cut rewriting uses the same type for the
 * input and output network.
 *
 * \param ntk Network
 * \param rewriting_fn Rewriting function
 * \param ps Rewriting params
 * \param pst Rewriting statistics
 */
template<class Ntk, class RewritingFn, class NodeCostFn = unit_cost<Ntk>>
Ntk cut_rewriting( Ntk const& ntk, RewritingFn const& rewriting_fn = {}, cut_rewriting_params const& ps = {}, cut_rewriting_stats* pst = nullptr )
{
  cut_rewriting_stats st;
  const auto result = [&]() {
    if ( ps.preserve_depth )
    {
      depth_view<Ntk, NodeCostFn> depth_ntk{ ntk };
      return detail::cut_rewriting_impl<Ntk, depth_view<Ntk, NodeCostFn>, RewritingFn, NodeCostFn>( depth_ntk, rewriting_fn, ps, st ).run();
    }
    else
    {
      return detail::cut_rewriting_impl<Ntk, Ntk, RewritingFn, NodeCostFn>( ntk, rewriting_fn, ps, st ).run();
    }
  }();

  if ( ps.verbose )
  {
    st.report( false );
  }
  if ( pst )
  {
    *pst = st;
  }

  ntk.clear_values();
  return result;
}

} /* namespace mockturtle */