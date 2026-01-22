/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2021  EPFL
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
  \file xag_algebraic_rewriting.hpp
  \brief xag algebraric rewriting

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <iostream>
#include <optional>

#include "../views/fanout_view.hpp"
#include "../views/topo_view.hpp"

namespace mockturtle
{

/*! \brief Parameters for xag_algebraic_depth_rewriting.
 *
 * The data structure `xag_algebraic_depth_rewriting_params` holds configurable
 * parameters with default arguments for `xag_algebraic_depth_rewriting`.
 */
struct xag_algebraic_depth_rewriting_params
{
  /*! \brief Rewriting strategy. */
  enum strategy_t
  {
    /*! \brief DFS rewriting strategy.
     *
     * Applies depth rewriting once to all output cones whose drivers have
     * maximum levels
     */
    dfs,
    /*! \brief Aggressive rewriting strategy.
     *
     * Applies depth reduction multiple times until the number of nodes, which
     * cannot be rewritten, matches the number of nodes, in the current
     * network; or the new network size is larger than the initial size w.r.t.
     * to an `overhead`.
     */
    aggressive,
    /*! \brief Selective rewriting strategy.
     *
     * Like `aggressive`, but only applies rewriting to nodes on critical paths
     * and without `overhead`.
     */
    selective
  } strategy = dfs;

  /*! \brief Overhead factor in aggressive rewriting strategy.
   *
   * When comparing to the initial size in aggressive depth rewriting, also the
   * number of dangling nodes are taken into account.
   */
  float overhead{ 2.0f };

  /*! \brief Allow area increase while optimizing depth. */
  bool allow_area_increase{ true };

  /*! \brief Try rules that are rarely applied. */
  bool allow_rare_rules{ false };
};

namespace detail
{

template<class Ntk>
class xag_algebraic_depth_rewriting_impl
{
public:
  xag_algebraic_depth_rewriting_impl( Ntk& ntk, xag_algebraic_depth_rewriting_params const& ps )
      : ntk( ntk ), ps( ps )
  {
  }

  void run()
  {
    switch ( ps.strategy )
    {
    case xag_algebraic_depth_rewriting_params::dfs:
      run_dfs();
      break;
    case xag_algebraic_depth_rewriting_params::selective:
      run_selective();
      break;
    case xag_algebraic_depth_rewriting_params::aggressive:
      run_aggressive();
      break;
    }
  }

private:
  void run_dfs()
  {
    ntk.foreach_po( [this]( auto po ) {
      const auto driver = ntk.get_node( po );
      if ( ntk.level( driver ) < ntk.depth() )
        return;
      topo_view topo{ ntk, po };
      topo.foreach_node( [&]( auto n ) {
        bool res = reduce_depth_and_associativity( n );
        res |= reduce_depth_xor_associativity( n );

        if ( ps.allow_area_increase && !res )
        {
          reduce_depth_and_or_distributivity( n );

          reduce_depth_and_xor_distributivity( n );
        }

        if ( ps.allow_rare_rules && !res )
          res = reduce_depth_and_distributity( n );

        return true;
      } );
    } );
  }

  void run_selective()
  {
    uint32_t counter{ 0 };
    while ( true )
    {
      mark_critical_paths();

      topo_view topo{ ntk };
      topo.foreach_node( [this, &counter]( auto n ) {
        if ( ntk.fanout_size( n ) == 0 || ntk.value( n ) == 0 )
          return;

        bool res = reduce_depth_and_associativity( n );
        res |= reduce_depth_xor_associativity( n );

        if ( ps.allow_area_increase && !res )
        {
          reduce_depth_and_or_distributivity( n );

          reduce_depth_and_xor_distributivity( n );
        }

        if ( ps.allow_rare_rules && !res )
          res = reduce_depth_and_distributity( n );

        if ( res )
        {
          mark_critical_paths();
        }
        else
        {
          ++counter;
        }
      } );

      if ( counter > ntk.size() )
        break;
    }
  }

  void run_aggressive()
  {
    uint32_t counter{ 0 }, init_size{ ntk.size() };
    while ( true )
    {
      topo_view topo{ ntk };
      topo.foreach_node( [this, &counter]( auto n ) {
        if ( ntk.fanout_size( n ) == 0 )
          return;

        bool res = reduce_depth_and_associativity( n );
        res |= reduce_depth_xor_associativity( n );

        if ( ps.allow_area_increase && !res )
        {
          reduce_depth_and_or_distributivity( n );

          reduce_depth_and_xor_distributivity( n );
        }

        if ( ps.allow_rare_rules && !res )
          res = reduce_depth_and_distributity( n );

        if ( !res )
        {
          ++counter;
        }
      } );

      if ( ntk.size() > ps.overhead * init_size )
        break;
      if ( counter > ntk.size() )
        break;
    }
  }

private:
  /* AND associativity */
  bool reduce_depth_and_associativity( node<Ntk> const& n )
  {
    if ( !ntk.is_and( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_and( ntk.get_node( ocs[1] ) ) || ntk.is_complemented( ocs[1] ) )
      return false;

    /* depth of second child must be (significantly) higher than depth of first child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;

    /* child must have single fanout, if no area overhead is allowed */
    if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( ocs[1] ) ) != 1 )
      return false;

    /* get children of second child */
    auto ocs1 = ordered_children( ntk.get_node( ocs[1] ) );

    /* depth of second grand-child must be higher than depth of first grand-child */
    if ( ntk.level( ntk.get_node( ocs1[1] ) ) == ntk.level( ntk.get_node( ocs1[0] ) ) )
      return false;

    auto opt = ntk.create_and( ocs1[1], ntk.create_and( ocs[0], ocs1[0] ) );
    ntk.substitute_node( n, opt );
    ntk.update_levels();

    return true;
  }

  /* XOR associativity */
  bool reduce_depth_xor_associativity( node<Ntk> const& n )
  {
    if ( !ntk.is_xor( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_xor( ntk.get_node( ocs[1] ) ) )
      return false;

    /* depth of second child must be (significantly) higher than depth of first child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 1 )
      return false;

    /* child must have single fanout, if no area overhead is allowed */
    if ( !ps.allow_area_increase && ntk.fanout_size( ntk.get_node( ocs[1] ) ) != 1 )
      return false;

    /* get children of last child */
    auto ocs1 = ordered_children( ntk.get_node( ocs[1] ) );

    /* depth of second grand-child must be higher than depth of first grand-child */
    if ( ntk.level( ntk.get_node( ocs1[1] ) ) == ntk.level( ntk.get_node( ocs1[0] ) ) )
      return false;

    auto opt = ntk.create_xor( ocs1[1], ntk.create_xor( ocs[0], ocs1[0] ) );

    if ( ntk.is_complemented( ocs[1] ) )
      opt = !opt;

    ntk.substitute_node( n, opt );
    ntk.update_levels();

    return true;
  }

  /* AND distributivity */
  bool reduce_depth_and_distributity( node<Ntk> const& n )
  {
    if ( !ntk.is_and( n ) )
      return false;

    if ( ntk.level( n ) == 0 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_and( ntk.get_node( ocs[0] ) ) || !ntk.is_complemented( ocs[0] ) )
      return false;

    if ( !ntk.is_and( ntk.get_node( ocs[1] ) ) || !ntk.is_complemented( ocs[1] ) )
      return false;

    /* children must have single fanout, if no area overhead is allowed */
    if ( !ps.allow_area_increase && ( ntk.fanout_size( ntk.get_node( ocs[0] ) ) != 1 || ntk.fanout_size( ntk.get_node( ocs[1] ) ) != 1 ) )
      return false;

    /* get children of first child */
    auto ocs0 = ordered_children( ntk.get_node( ocs[0] ) );

    /* get children of second child */
    auto ocs1 = ordered_children( ntk.get_node( ocs[1] ) );

    /* find common support */
    bool critical_common = false;
    signal<Ntk> common, x, y;
    if ( ocs0[0] == ocs1[0] )
    {
      common = ocs0[0];
      x = ocs0[1];
      y = ocs1[1];
    }
    else if ( ocs0[0] == ocs1[1] )
    {
      common = ocs0[0];
      x = ocs0[1];
      y = ocs1[0];
    }
    else if ( ocs0[1] == ocs1[0] )
    {
      common = ocs0[1];
      x = ocs0[0];
      y = ocs1[1];
    }
    else if ( ocs0[1] == ocs1[1] )
    {
      common = ocs0[1];
      x = ocs0[0];
      y = ocs1[0];
      critical_common = true;
    }
    else
    {
      return false;
    }

    /* common signal is not critical, children must have single fanout, to not increase the area */
    if ( !critical_common && ( ntk.fanout_size( ntk.get_node( ocs[0] ) ) != 1 || ntk.fanout_size( ntk.get_node( ocs[1] ) ) != 1 ) )
      return false;

    auto opt = !ntk.create_and( common, !ntk.create_and( !x, !y ) );
    ntk.substitute_node( n, opt );
    ntk.update_levels();

    return true;
  }

  /* AND-OR distributivity */
  bool reduce_depth_and_or_distributivity( node<Ntk> const& n )
  {
    if ( !ntk.is_and( n ) )
      return false;

    if ( ntk.level( n ) < 3 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_and( ntk.get_node( ocs[1] ) ) || !ntk.is_complemented( ocs[1] ) )
      return false;

    /* depth of second child must be significantly higher than depth of first child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 2 )
      return false;

    /* get children of last child */
    auto ocs1 = ordered_children( ntk.get_node( ocs[1] ) );

    if ( !ntk.is_and( ntk.get_node( ocs1[1] ) ) || !ntk.is_complemented( ocs1[1] ) )
      return false;

    /* depth of second grand-child must be higher than depth of first grand-child */
    if ( ntk.level( ntk.get_node( ocs1[1] ) ) == ntk.level( ntk.get_node( ocs1[0] ) ) )
      return false;

    /* get children of last grand-child */
    auto ocs11 = ordered_children( ntk.get_node( ocs1[1] ) );

    /* depth of second grand-grand-child must be higher than depth of first grand-grand-child */
    if ( ntk.level( ntk.get_node( ocs11[1] ) ) == ntk.level( ntk.get_node( ocs11[0] ) ) )
      return false;

    auto opt = !ntk.create_and( !ntk.create_and( ocs[0], !ocs1[0] ), !ntk.create_and( ntk.create_and( ocs[0], ocs11[0] ), ocs11[1] ) );
    ntk.substitute_node( n, opt );
    ntk.update_levels();

    return true;
  }

  /* AND-XOR distributivity */
  bool reduce_depth_and_xor_distributivity( node<Ntk> const& n )
  {
    if ( !ntk.is_and( n ) )
      return false;

    if ( ntk.level( n ) < 3 )
      return false;

    /* get children of top node, ordered by node level (ascending) */
    const auto ocs = ordered_children( n );

    if ( !ntk.is_xor( ntk.get_node( ocs[1] ) ) )
      return false;

    /* depth of second child must be significantly higher than depth of first child */
    if ( ntk.level( ntk.get_node( ocs[1] ) ) <= ntk.level( ntk.get_node( ocs[0] ) ) + 2 )
      return false;

    /* get children of last child */
    auto ocs1 = ordered_children( ntk.get_node( ocs[1] ) );

    if ( !ntk.is_and( ntk.get_node( ocs1[1] ) ) )
      return false;

    /* depth of second grand-child must be higher than depth of first grand-child */
    if ( ntk.level( ntk.get_node( ocs1[1] ) ) == ntk.level( ntk.get_node( ocs1[0] ) ) )
      return false;

    /* get children of last grand-child */
    auto ocs11 = ordered_children( ntk.get_node( ocs1[1] ) );

    /* depth of second grand-grand-child must be higher than depth of first grand-grand-child */
    if ( ntk.level( ntk.get_node( ocs11[1] ) ) == ntk.level( ntk.get_node( ocs11[0] ) ) )
      return false;

    /* if XOR is complemented, complement first grand-child */
    if ( ntk.is_complemented( ocs[1] ) != ntk.is_complemented( ocs1[1] ) )
    {
      ocs1[0] = !ocs1[0];
    }

    auto opt = ntk.create_xor( ntk.create_and( ocs[0], ocs1[0] ), ntk.create_and( ntk.create_and( ocs[0], ocs11[0] ), ocs11[1] ) );
    ntk.substitute_node( n, opt );
    ntk.update_levels();

    return true;
  }

  inline std::array<signal<Ntk>, 2> ordered_children( node<Ntk> const& n ) const
  {
    std::array<signal<Ntk>, 2> children;
    ntk.foreach_fanin( n, [&children]( auto const& f, auto i ) {
      children[i] = f;
    } );
    if ( ntk.level( ntk.get_node( children[0] ) ) > ntk.level( ntk.get_node( children[1] ) ) )
    {
      std::swap( children[0], children[1] );
    }
    return children;
  }

  void mark_critical_path( node<Ntk> const& n )
  {
    if ( ntk.is_pi( n ) || ntk.is_constant( n ) || ntk.value( n ) )
      return;

    const auto level = ntk.level( n );
    ntk.set_value( n, 1 );
    ntk.foreach_fanin( n, [this, level]( auto const& f ) {
      if ( ntk.level( ntk.get_node( f ) ) == level - 1 )
      {
        mark_critical_path( ntk.get_node( f ) );
      }
    } );
  }

  void mark_critical_paths()
  {
    ntk.clear_values();
    ntk.foreach_po( [this]( auto const& f ) {
      if ( ntk.level( ntk.get_node( f ) ) == ntk.depth() )
      {
        mark_critical_path( ntk.get_node( f ) );
      }
    } );
  }

private:
  Ntk& ntk;
  xag_algebraic_depth_rewriting_params const& ps;
};

} // namespace detail

/*! \brief XAG algebraic depth rewriting.
 *
 * This algorithm tries to rewrite a network with AND/XOR gates for depth
 * optimization using the associativity and distributivity rule in
 * AND-XOR logic.  It can be applied to networks other than XAGs, but
 * only considers pairs of nodes which both implement the AND
 * function and the XOR function.
 *
 * **Required network functions:**
 * - `get_node`
 * - `level`
 * - `update_levels`
 * - `create_and`
 * - `create_xor`
 * - `substitute_node`
 * - `foreach_node`
 * - `foreach_po`
 * - `foreach_fanin`
 * - `is_and`
 * - `is_xor`
 * - `clear_values`
 * - `set_value`
 * - `value`
 * - `fanout_size`
 *
   \verbatim embed:rst

  .. note::

   \endverbatim
 */
template<class Ntk>
void xag_algebraic_depth_rewriting( Ntk& ntk, xag_algebraic_depth_rewriting_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the substitute_node method" );
  static_assert( has_update_levels_v<Ntk>, "Ntk does not implement the update_levels method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the value method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  detail::xag_algebraic_depth_rewriting_impl<Ntk> p( ntk, ps );
  p.run();
}

} /* namespace mockturtle */