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
  \file window_utils.hpp
  \brief Utilities to collect small-scale sets of nodes

  \author Heinz Riener
*/

#pragma once

#include <algorithm>
#include <optional>
#include <set>
#include <type_traits>
#include <vector>

namespace mockturtle
{

namespace detail
{

template<typename Ntk>
inline void collect_nodes_recur( Ntk const& ntk, typename Ntk::node const& n, std::vector<typename Ntk::node>& nodes )
{
  using signal = typename Ntk::signal;

  if ( ntk.eval_color( n, [&]( auto c ) { return c == ntk.current_color(); } ) )
  {
    return;
  }
  ntk.paint( n );

  ntk.foreach_fanin( n, [&]( signal const& fi ) {
    if ( ntk.is_constant( ntk.get_node( fi ) ) )
      return;
    collect_nodes_recur( ntk, ntk.get_node( fi ), nodes );
  } );
  nodes.push_back( n );
}

} /* namespace detail */

/*! \brief Collect nodes in between of two node sets
 *
 * \param ntk A network
 * \param inputs A node set
 * \param outputs A signal set
 * \return Nodes enclosed by inputs and outputs
 *
 * The output set has to be chosen in a way such that every path from
 * PIs to outputs passes through at least one input.
 *
 * Uses a new color.
 *
 * **Required network functions:**
 * - `current_color`
 * - `eval_color`
 * - `foreach_fanin`
 * - `get_node`
 * - `new_color`
 * - `paint`
 */
template<typename Ntk, typename = std::enable_if_t<!std::is_same_v<typename Ntk::signal, typename Ntk::node>>>
inline std::vector<typename Ntk::node> collect_nodes( Ntk const& ntk,
                                                      std::vector<typename Ntk::node> const& inputs,
                                                      std::vector<typename Ntk::signal> const& outputs )
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /* convert output signals to nodes */
  std::vector<node> _outputs;
  std::transform( std::begin( outputs ), std::end( outputs ), std::back_inserter( _outputs ),
                  [&ntk]( signal const& s ) {
                    return ntk.get_node( s );
                  } );
  return collect_nodes( ntk, inputs, _outputs );
}

/*! \brief Collect nodes in between of two node sets
 *
 * \param ntk A network
 * \param inputs A node set
 * \param outputs A node set
 * \return Nodes enclosed by inputs and outputs
 *
 * The output set has to be chosen in a way such that every path from
 * PIs to outputs passes through at least one input.
 *
 * Uses a new color.
 *
 * **Required network functions:**
 * - `current_color`
 * - `eval_color`
 * - `foreach_fanin`
 * - `get_node`
 * - `new_color`
 * - `paint`
 */
template<typename Ntk>
inline std::vector<typename Ntk::node> collect_nodes( Ntk const& ntk,
                                                      std::vector<typename Ntk::node> const& inputs,
                                                      std::vector<typename Ntk::node> const& outputs )
{
  using node = typename Ntk::node;

  ntk.new_color();

  /* mark inputs visited */
  for ( auto const& i : inputs )
  {
    if ( ntk.eval_color( i, [&]( auto c ) { return c == ntk.current_color(); } ) )
    {
      continue;
    }
    ntk.paint( i );
  }

  /* recursively collect all nodes in between inputs and outputs */
  std::vector<node> nodes;
  for ( auto const& o : outputs )
  {
    detail::collect_nodes_recur( ntk, o, nodes );
  }
  return nodes;
}

/*! \brief Identify inputs using reference counting
 *
 * Uses a new_color and marks all nodes and inputs.
 *
 * **Required network functions:**
 * - `current_color`
 * - `eval_color`
 * - `foreach_fanin`
 * - `get_node`
 * - `new_color`
 * - `paint`
 */
template<typename Ntk>
std::vector<typename Ntk::node> collect_inputs( Ntk const& ntk, std::vector<typename Ntk::node> const& nodes )
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /* mark all nodes with a new color */
  ntk.new_color();
  for ( const auto& n : nodes )
  {
    ntk.paint( n );
  }

  /* if a fanin is not colored, then it's an input */
  std::vector<node> inputs;
  for ( const auto& n : nodes )
  {
    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      node const i = ntk.get_node( fi );
      if ( ntk.eval_color( i, [&ntk]( auto c ) { return c != ntk.current_color(); } ) )
      {
        if ( std::find( std::begin( inputs ), std::end( inputs ), i ) == std::end( inputs ) )
        {
          inputs.push_back( i );
        }
      }
      return true;
    } );
  }

  /* mark all inputs */
  for ( const auto& n : inputs )
  {
    ntk.paint( n );
  }

  return inputs;
}

/*! \brief Identify outputs using reference counting
 *
 * Identify outputs using a reference counting approach.  The
 * algorithm counts the references of the fanins of all nodes and
 * compares them with the fanout_sizes of the respective nodes.  If
 * reference count and fanout_size do not match, then the node is
 * references outside of the node set and the respective is identified
 * as an output.
 *
 * \param ntk A network
 * \param inputs Inputs of a window
 * \param nodes Inner nodes of a window (i.e., the intersection of
 *              inputs and nodes is assumed to be empty)
 * \param refs Reference counters (in the size of the network and
 *             initialized to 0)
 * \return Output signals of the window
 *
 * **Required network functions:**
 * - `current_color`
 * - `eval_color`
 * - `fanout_size`
 * - `foreach_fanin`
 * - `get_node`
 * - `is_ci`
 * - `is_constant`
 * - `make_signal`
 */
template<typename Ntk>
inline std::vector<typename Ntk::signal> collect_outputs( Ntk const& ntk,
                                                          std::vector<typename Ntk::node> const& inputs,
                                                          std::vector<typename Ntk::node> const& nodes,
                                                          std::vector<uint32_t>& refs )
{
  using signal = typename Ntk::signal;

  std::vector<signal> outputs;

  /* mark the inputs visited */
  ntk.new_color();
  for ( auto const& i : inputs )
  {
    ntk.paint( i );
  }

  /* reference fanins of nodes */
  for ( auto const& n : nodes )
  {
    if ( ntk.eval_color( n, [&ntk]( auto c ) { return c == ntk.current_color(); } ) )
    {
      continue;
    }

    assert( !ntk.is_constant( n ) && !ntk.is_ci( n ) );
    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      refs[ntk.get_node( fi )] += 1;
    } );
  }

  /* if the fanout_size of a node does not match the reference count,
     the node has fanouts outside of the window is an output */
  for ( const auto& n : nodes )
  {
    if ( ntk.eval_color( n, [&ntk]( auto c ) { return c == ntk.current_color(); } ) )
    {
      continue;
    }

    if ( ntk.fanout_size( n ) != refs[n] )
    {
      outputs.emplace_back( ntk.make_signal( n ) );
    }
  }

  /* dereference fanins of nodes */
  for ( auto const& n : nodes )
  {
    if ( ntk.eval_color( n, [&ntk]( auto c ) { return c == ntk.current_color(); } ) )
    {
      continue;
    }

    assert( !ntk.is_constant( n ) && !ntk.is_ci( n ) );
    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      refs[ntk.get_node( fi )] -= 1;
    } );
  }

  return outputs;
}

namespace detail
{

template<typename Ntk>
inline bool cut_is_trivial( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs )
{
  for ( const auto& n : inputs )
  {
    if ( !ntk.is_constant( n ) && !ntk.is_ci( n ) )
    {
      return false;
    }
  }
  return true;
}

} // namespace detail

/*! \brief Performs in-place zero-cost expansion of a set of nodes towards TFI
 *
 * The algorithm attempts to derive a different cut of the same size
 * that is closer to the network's PIs.  This expansion towards TFI is
 * called zero-cost because it merges nodes only if the number of
 * inputs does not increase.
 *
 * Precondition: This procedure presumes that nodes and inputs are
 * painted in the current color.
 *
 * Uses the current color to mark nodes.  Only nodes not painted with
 * the current color are considered for expanding the cut.  Nodes
 * marked are considered already in the cut.
 *
 * \param ntk A network
 * \param inputs Input nodes
 * \return True if and only if the inputs form a trivial cut that
 *         cannot be further extended, e.g., when the cut only
 *         consists of PIs.
 *
 * **Required network functions:**
 * - `current_color`
 * - `eval_color`
 * - `foreach_fanin`
 * - `get_node`
 * - `paint`
 * - `size`
 */
template<typename Ntk>
bool expand0_towards_tfi( Ntk const& ntk, std::vector<typename Ntk::node>& inputs )
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /* we call a set of inputs (= a cut) trivial if all nodes are either
     constants or CIs, such that they cannot be further expanded towards
     the TFI */
  bool trivial_cut{ true };

  /* repeat expansion towards TFI until a fix-point is reached */
  bool changed{ true };
  std::vector<node> new_inputs;
  while ( changed )
  {
    trivial_cut = true;
    changed = false;

    for ( auto it = std::begin( inputs ); it != std::end( inputs ); )
    {
      assert( ntk.color( *it ) == ntk.current_color() );
      if ( ntk.is_constant( *it ) || ntk.is_ci( *it ) )
      {
        ++it;
        continue;
      }
      trivial_cut = false;

      /* count how many fanins are already in the cut */
      uint32_t count_fanin_outside{ 0 };
      std::optional<node> ep;
      ntk.foreach_fanin( *it, [&]( signal const& fi ) {
        node const n = ntk.get_node( fi );
        if ( ntk.eval_color( n, [&ntk]( auto c ) { return c == ntk.current_color(); } ) )
        {
          ++count_fanin_outside;
        }
        else
        {
          ep = n;
        }
      } );

      /* if the expansion is not cost-free, then proceeded with the next leaf */
      if ( count_fanin_outside + 1 < ntk.fanin_size( *it ) )
      {
        ++it;
        continue;
      }

      if ( ep )
      {
        if ( ntk.eval_color( *ep, [&ntk]( auto c ) { return c != ntk.current_color(); } ) )
        {
          new_inputs.push_back( *ep );
          ntk.paint( *ep );
        }
      }
      it = inputs.erase( it );
      changed = true;
    }

    std::copy( std::begin( new_inputs ), std::end( new_inputs ),
               std::back_inserter( inputs ) );
    new_inputs.clear();
  }

  assert( trivial_cut == detail::cut_is_trivial( ntk, inputs ) );
  return trivial_cut;
}

namespace detail
{

template<typename Ntk>
inline void evaluate_fanin( typename Ntk::node const& n, std::vector<std::pair<typename Ntk::node, uint32_t>>& candidates )
{
  auto it = std::find_if( std::begin( candidates ), std::end( candidates ),
                          [&n]( auto const& p ) {
                            return p.first == n;
                          } );
  if ( it == std::end( candidates ) )
  {
    /* new fanin: referenced for the 1st time */
    candidates.push_back( std::make_pair( n, 1u ) );
  }
  else
  {
    /* otherwise, if not new, then just increase the reference counter */
    ++it->second;
  }
}

template<typename Ntk>
inline typename Ntk::node select_next_fanin_to_expand_tfi( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs )
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  assert( inputs.size() > 0u && "inputs must not be empty" );
  assert( !cut_is_trivial( ntk, inputs ) );

  /* evaluate the fanins with respect to their costs (how often are they referenced?) */
  std::vector<std::pair<node, uint32_t>> candidates;
  for ( auto const& i : inputs )
  {
    if ( ntk.is_constant( i ) || ntk.is_ci( i ) )
    {
      continue;
    }

    ntk.foreach_fanin( i, [&]( signal const& fi ) {
      if ( ntk.is_constant( ntk.get_node( fi ) ) )
      {
        return true;
      }
      detail::evaluate_fanin<Ntk>( ntk.get_node( fi ), candidates );
      return true;
    } );
  }

  assert( candidates.size() > 0u );

  /* select the fanin with maximum reference count; if two fanins have equal reference count, select the one with more fanouts */
  std::pair<node, uint32_t> best_fanin{ candidates[0] };
  for ( auto const& candidate : candidates )
  {
    if ( candidate.second > best_fanin.second ||
         ( candidate.second == best_fanin.second && ntk.fanout_size( candidate.first ) > ntk.fanout_size( best_fanin.first ) ) )
    {
      best_fanin = candidate;
    }
  }

  /* as long as the inputs do not form a trivial cut, this procedure will always find a fanin to expand */
  assert( best_fanin.first != 0 );

  return best_fanin.first;
}

} /* namespace detail */

/*! \brief Performs in-place expansion of a set of nodes towards TFI
 *
 * Expand the inputs towards TFI by iteratively selecting the fanins
 * with the highest reference count within the cut and highest number
 * of fanouts.  Expansion continues until either `inputs` forms a
 * trivial cut or the `inputs`'s size reaches `input_limit`.  The
 * procedure allows a temporary increase of `inputs` beyond the
 * `input_limit` for at most `MAX_ITERATIONS`.
 *
 * Precondition: This procedure presumes that nodes and inputs are
 * painted in the current color.
 *
 * Uses a new color.
 *
 * \param ntk A network
 * \param inputs Input nodes
 * \param input_limit Size limit for the maximum number of input nodes
 */
template<typename Ntk>
void expand_towards_tfi( Ntk const& ntk, std::vector<typename Ntk::node>& inputs, uint32_t input_limit )
{
  using node = typename Ntk::node;

  static constexpr uint32_t const MAX_ITERATIONS{ 5u };

  if ( expand0_towards_tfi( ntk, inputs ) )
  {
    return;
  }

  std::optional<std::vector<node>> best_cut;
  if ( inputs.size() <= input_limit )
  {
    best_cut = inputs;
  }

  bool trivial_cut = false;
  uint32_t iterations{ 0 };
  while ( !trivial_cut && ( inputs.size() <= input_limit || iterations < MAX_ITERATIONS ) )
  {
    node const n = detail::select_next_fanin_to_expand_tfi( ntk, inputs );
    inputs.push_back( n );
    ntk.paint( n );

    trivial_cut = expand0_towards_tfi( ntk, inputs );
    assert( trivial_cut == detail::cut_is_trivial( ntk, inputs ) );

    iterations = inputs.size() > input_limit ? iterations + 1 : 0;
    if ( inputs.size() <= input_limit &&
         ( !best_cut || best_cut->size() <= inputs.size() ) )
    {
      best_cut = inputs;
    }
  }

  if ( best_cut )
  {
    inputs = *best_cut;
  }
  else
  {
    assert( inputs.size() > input_limit );
  }
}

/*! \brief Performs in-place expansion of a set of nodes towards TFO
 *
 * Iteratively expands the inner nodes of the window with those
 * fanouts that are supported by the window until a fixed-point is
 * reached.
 *
 * Uses a new color.
 *
 * \param ntk A network
 * \param inputs Input nodes of a window
 * \param nodes Inner nodes of a window
 *
 * **Required network functions:**
 * - `current_color`
 * - `eval_color`
 * - `foreach_fanin`
 * - `foreach_fanout`
 * - `get_node`
 * - `is_ci`
 * - `new_color`
 */
template<typename Ntk>
void expand_towards_tfo( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs, std::vector<typename Ntk::node>& nodes )
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  auto explore_fanouts = [&]( Ntk const& ntk, node const& n, std::set<node>& result ) {
    ntk.foreach_fanout( n, [&]( node const& fo, uint64_t index ) {
      /* only look at the first few fanouts */
      if ( index > 5 )
      {
        return false;
      }
      /* skip all nodes that are already in nodes */
      if ( ntk.eval_color( fo, [&]( auto c ) { return c == ntk.current_color(); } ) )
      {
        return true;
      }
      result.insert( fo );
      return true;
    } );
  };

  /* create a new traversal ID */
  ntk.new_color();

  /* mark the inputs visited */
  std::for_each( std::begin( inputs ), std::end( inputs ),
                 [&ntk]( node const& n ) { ntk.paint( n ); } );

  /* mark the nodes visited */
  std::for_each( std::begin( nodes ), std::end( nodes ),
                 [&ntk]( node const& n ) { ntk.paint( n ); } );

  /* collect all nodes that have fanouts not yet contained in nodes */
  std::set<node> eps;
  for ( const auto& i : inputs )
  {
    explore_fanouts( ntk, i, eps );
  }
  for ( const auto& n : nodes )
  {
    explore_fanouts( ntk, n, eps );
  }

  bool changed = true;
  std::set<node> new_eps;
  while ( changed )
  {
    new_eps.clear();
    changed = false;

    auto it = std::begin( eps );
    while ( it != std::end( eps ) )
    {
      node const ep = *it;
      if ( ntk.eval_color( ep, [&]( auto c ) { return c == ntk.current_color(); } ) )
      {
        it = eps.erase( it );
        continue;
      }

      bool all_children_belong_to_window = true;
      ntk.foreach_fanin( ep, [&]( signal const& fi ) {
        node const child = ntk.get_node( fi );
        if ( ntk.eval_color( child, [&]( auto c ) { return c != ntk.current_color(); } ) )
        {
          all_children_belong_to_window = false;
          return false;
        }
        return true;
      } );

      if ( all_children_belong_to_window )
      {
        assert( ep != 0 );
        assert( !ntk.is_ci( ep ) );
        nodes.emplace_back( ep );
        ntk.paint( ep );
        it = eps.erase( it );

        explore_fanouts( ntk, ep, new_eps );
      }

      if ( it != std::end( eps ) )
      {
        ++it;
      }
    }

    if ( !new_eps.empty() )
    {
      eps.insert( std::begin( new_eps ), std::end( new_eps ) );
      changed = true;
    }
  }
}

namespace detail
{

template<typename Ntk, bool auto_resize = true>
void levelized_expand_towards_tfo( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs, std::vector<typename Ntk::node>& nodes,
                                   std::vector<std::vector<typename Ntk::node>>& levels )
{
  using node = typename Ntk::node;

  static constexpr uint32_t const MAX_FANOUTS{ 5u };

  ntk.new_color();

  /* mapping from level to nodes (which nodes are on a certain level?) */
  levels.resize( ntk.depth() + 1 );

  /* list of indices of used levels (avoid iterating over all levels) */
  std::vector<uint32_t> used;

  /* mark all inputs and fill their level information into `levels` and `used` */
  for ( const auto& i : inputs )
  {
    uint32_t const node_level = ntk.level( i );
    ntk.paint( i );
    if constexpr ( auto_resize )
    {
      if ( levels.size() <= node_level )
      {
        levels.resize( std::max( uint32_t( 2 * levels.size() ), node_level ) );
      }
    }
    levels.at( node_level ).push_back( i );
    if ( std::find( std::begin( used ), std::end( used ), node_level ) == std::end( used ) )
    {
      used.push_back( node_level );
    }
  }

  /* mark all nodes and fill their level information into `levels` and `used` */
  for ( const auto& n : nodes )
  {
    uint32_t const node_level = ntk.level( n );
    ntk.paint( n );
    if constexpr ( auto_resize )
    {
      if ( levels.size() <= node_level )
      {
        levels.resize( std::max( uint32_t( 2 * levels.size() ), node_level ) );
      }
    }
    levels.at( node_level ).push_back( n );
    if ( std::find( std::begin( used ), std::end( used ), node_level ) == std::end( used ) )
    {
      used.push_back( node_level );
    }
  }

  std::stable_sort( std::begin( used ), std::end( used ) );

  for ( uint32_t index = 0u; index < used.size(); ++index )
  {
    std::vector<node>& level = levels.at( used[index] );
    for ( auto j = 0u; j < level.size(); ++j )
    {
      ntk.foreach_fanout( level[j], [&]( node const& fo, uint64_t index ) {
        /* avoid getting stuck on nodes with many fanouts */
        if ( index == MAX_FANOUTS )
        {
          return false;
        }

        /* ignore nodes without fanins */
        if ( ntk.is_constant( fo ) || ntk.is_ci( fo ) )
        {
          return true;
        }

        if ( ntk.eval_color( fo, [&ntk]( auto c ) { return c != ntk.current_color(); } ) &&
             ntk.eval_fanins_color( fo, [&ntk]( auto c ) { return c == ntk.current_color(); } ) )
        {
          /* add fanout to nodes */
          nodes.push_back( fo );

          /* update data structured */
          uint32_t const node_level = ntk.level( fo );
          ntk.paint( fo );
          if constexpr ( auto_resize )
          {
            if ( levels.size() <= node_level )
            {
              levels.resize( std::max( uint32_t( 2 * levels.size() ), node_level ) );
            }
          }
          levels.at( node_level ).push_back( fo );
          if ( std::find( std::begin( used ), std::end( used ), node_level ) == std::end( used ) )
          {
            used.push_back( node_level );
            std::stable_sort( std::begin( used ), std::end( used ) );
          }
        }

        return true;
      } );
    }
    level.clear();
  }
}

} // namespace detail

/*! \brief Performs in-place expansion of a set of nodes towards TFO
 *
 * Iteratively expands the inner nodes of the window with those
 * fanouts that are supported by the window.  Explores the fanouts
 * level by level.  Starting with those that are closest to the
 * inputs.
 *
 * Uses a new color.
 *
 * \param ntk A network
 * \param inputs Input nodes of a window
 * \param nodes Inner nodes of a window
 *
 * **Required network functions:**
 * - `current_color`
 * - `depth`
 * - `eval_color`
 * - `eval_fanins_color`
 * - `foreach_fanin`
 * - `foreach_fanout`
 * - `get_node`
 * - `is_ci`
 * - `is_constant`
 * - `level`
 * - `new_color`
 * - `paint`
 */
template<typename Ntk, bool auto_resize = true>
void levelized_expand_towards_tfo( Ntk const& ntk, std::vector<typename Ntk::node> const& inputs, std::vector<typename Ntk::node>& nodes )
{
  std::vector<std::vector<typename Ntk::node>> levels;
  detail::levelized_expand_towards_tfo<Ntk, auto_resize>( ntk, inputs, nodes, levels );
}

namespace detail
{

template<typename Ntk>
void cover_recursive( Ntk const& ntk, typename Ntk::node const& root, std::vector<typename Ntk::node>& nodes )
{
  if ( ntk.color( root ) == ntk.current_color() )
  {
    return;
  }

  ntk.foreach_fanin( root, [&]( auto const& fi ) {
    cover_recursive( ntk, ntk.get_node( fi ), nodes );
  } );

  nodes.push_back( root );
}

} // namespace detail

template<typename Ntk>
std::vector<typename Ntk::node> cover( Ntk const& ntk, typename Ntk::node const& root, std::vector<typename Ntk::node> const& leaves )
{
  ntk.new_color();
  for ( auto const& l : leaves )
  {
    ntk.paint( l );
  }

  std::vector<typename Ntk::node> nodes;
  detail::cover_recursive( ntk, root, nodes );

  /* remove duplicates */
  std::stable_sort( std::begin( nodes ), std::end( nodes ) );
  auto last = std::unique( std::begin( nodes ), std::end( nodes ) );
  nodes.erase( last, std::end( nodes ) );

  return nodes;
}

/*! \brief Create a (l,k)-window around a pivot.
 *
 * Expands a reconvergency rooted in a given pivot node `p` into a
 * window with l inputs and k outputs.
 *
 * Uses a new color.
 *
 * **Required network functions:**
 * - `current_color`
 * - `depth`
 * - `eval_color`
 * - `eval_fanins_color`
 * - `foreach_fanin`
 * - `foreach_fanout`
 * - `get_node`
 * - `is_ci`
 * - `is_constant`
 * - `level`
 * - `new_color`
 * - `paint`
 */
template<typename Ntk>
class create_window_impl
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  struct window
  {
    std::vector<node> inputs;
    std::vector<node> nodes;
    std::vector<signal> outputs;
  };

protected:
  /* constant node used to denotes invalid window element */
  static constexpr node INVALID_NODE{ 0 };

  /* number of iterations */
  static constexpr uint32_t NUM_ITERATIONS{ 5 };

public:
  create_window_impl( Ntk const& ntk )
      : ntk( ntk ), path( ntk.size() ), refs( ntk.size() )
  {
  }

  void resize( uint32_t size )
  {
    path.resize( size );
    refs.resize( size );
  }

  std::optional<window> run( node const& pivot, uint32_t cut_size, uint32_t num_levels )
  {
    /* find a reconvergence from the pivot and collect the nodes */
    std::optional<std::vector<node>> nodes;
    if ( !( nodes = identify_reconvergence( pivot, num_levels ) ) )
    {
      /* if there is no reconvergence, then optimization is not possible */
      return std::nullopt;
    }

    /* collect the fanins for these nodes */
    std::vector<node> inputs = collect_inputs( ntk, *nodes );
    if ( inputs.size() <= cut_size + 3 )
    {
      /* expand the nodes towards the TFI */
      expand_towards_tfi( ntk, inputs, cut_size );

      /* compute the cover of the (pivot, inputs)-cut */
      *nodes = cover( ntk, pivot, inputs );

      /* expand the nodes towards the TFO */
      std::stable_sort( std::begin( inputs ), std::end( inputs ) );
      detail::levelized_expand_towards_tfo( ntk, inputs, *nodes, levels );
    }

    if ( inputs.size() > cut_size || nodes->empty() )
    {
      return std::nullopt;
    }

    /* top. sort nodes */
    std::stable_sort( std::begin( inputs ), std::end( inputs ) );
    std::stable_sort( std::begin( *nodes ), std::end( *nodes ) );

    /* collect the nodes with fanout outside of nodes */
    std::vector<signal> outputs = collect_outputs( ntk, inputs, *nodes, refs );
    assert( outputs.size() > 0u );

    return window{ inputs, *nodes, outputs };
  }

protected:
  std::optional<std::vector<node>> identify_reconvergence( node const& pivot, uint64_t num_iterations )
  {
    assert( !ntk.is_ci( pivot ) && !ntk.is_constant( pivot ) );

    ntk.new_color();
    visited.clear();
    ntk.foreach_fanin( pivot, [&]( signal const& fi ) {
      uint32_t const color = ntk.new_color();
      node const& n = ntk.get_node( fi );
      path[n] = INVALID_NODE;
      visited.push_back( n );
      ntk.paint( n, color );
    } );

    uint64_t start{ 0 };
    uint64_t stop;
    for ( uint32_t iteration = 0u; iteration < num_iterations; ++iteration )
    {
      stop = visited.size();
      for ( uint32_t i = start; i < stop; ++i )
      {
        node const n = visited.at( i );
        std::optional<node> meet = explore_frontier_of_node( n );
        if ( meet )
        {
          visited.clear();
          gather_nodes_recursively( path[*meet] );
          gather_nodes_recursively( n );
          visited.push_back( pivot );
          return visited;
        }
      }
      start = stop;
    }

    return std::nullopt;
  }

  std::optional<node> explore_frontier_of_node( node const& n )
  {
    if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
    {
      return std::nullopt;
    }

    std::optional<node> meet;
    ntk.foreach_fanin( n, [&]( signal const& fi ) {
      node const& fi_node = ntk.get_node( fi );
      if ( ntk.eval_color( n, [this]( auto c ) { return c > ntk.current_color() - ntk.max_fanin_size; } ) &&
           ntk.eval_color( fi_node, [this]( auto c ) { return c > ntk.current_color() - ntk.max_fanin_size; } ) &&
           ntk.eval_color( n, fi_node, []( auto c0, auto c1 ) { return c0 != c1; } ) )
      {
        meet = fi_node;
        return false;
      }

      if ( ntk.eval_color( fi_node, [this]( auto c ) { return c > ntk.current_color() - ntk.max_fanin_size; } ) )
      {
        return true; /* next */
      }

      ntk.paint( fi_node, n );
      path[fi_node] = n;
      visited.push_back( fi_node );

      return true; /* next */
    } );

    return meet;
  }

  /* collect nodes recursively following along the `path` until INVALID_NODE is reached */
  void gather_nodes_recursively( node const& n )
  {
    if ( n == INVALID_NODE )
    {
      return;
    }

    visited.push_back( n );

    node const pred = path[n];
    if ( pred == INVALID_NODE )
    {
      return;
    }

    assert( ntk.eval_color( n, pred, []( auto c0, auto c1 ) { return c0 == c1; } ) );
    gather_nodes_recursively( pred );
  }

protected:
  Ntk const& ntk;
  std::vector<node> visited;
  std::vector<node> path;
  std::vector<uint32_t> refs;
  std::vector<std::vector<node>> levels;
}; /* create_window_impl */

} /* namespace mockturtle */