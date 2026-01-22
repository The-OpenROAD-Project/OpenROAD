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
  \file spectr_cut.hpp
  \brief Cut enumeration based on spectral properties of a function

  \author Giulia Meuli
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

#include "../../utils/cuts.hpp"
#include "../cut_enumeration.hpp"
#include "../lut_mapping.hpp"
#include <kitty/constructors.hpp>
#include <kitty/spectral.hpp>
#include <kitty/static_truth_table.hpp>

namespace mockturtle
{

/*! \brief Cut based on spectral properties.

  This cut type uses the number of non-zero coefficients in the cut function as
  cost function.  It requires truth table computation during cut enumeration
  or LUT mapping in order to work.
*/
struct cut_enumeration_spectr_cut
{
  uint32_t delay{ 0u };
  float flow{ 0.0f };
  float cost{ 0.0f };
};

template<bool ComputeTruth>
bool operator<( cut_type<ComputeTruth, cut_enumeration_spectr_cut> const& c1, cut_type<ComputeTruth, cut_enumeration_spectr_cut> const& c2 )
{
  constexpr auto eps{ 0.005f };

  if ( c1.size() == c2.size() )
  {
    if ( c1->data.cost < c2->data.cost )
      return true;
    if ( c1->data.cost > c2->data.cost )
      return false;
  }
  if ( c1.size() > c2.size() && c1->data.cost < c2->data.cost )
  {
    return false;
  }
  if ( c1.size() < c2.size() && c1->data.cost > c2->data.cost )
  {
    return true;
  }
  if ( c1->data.flow < c2->data.flow - eps )
    return true;
  if ( c1->data.flow > c2->data.flow + eps )
    return false;
  if ( c1->data.delay < c2->data.delay )
    return true;
  if ( c1->data.delay > c2->data.delay )
    return false;
  return c1.size() > c2.size();
}

template<>
struct lut_mapping_update_cuts<cut_enumeration_spectr_cut>
{

  template<typename Ntk>
  static void grow_xor_cut( Ntk const& ntk, node<Ntk> const& n, std::map<uint32_t, std::vector<uint32_t>>& node_to_cut )
  {
    ntk.foreach_fanin( n, [&]( auto ch ) {
      auto ch_node = ntk.get_node( ch );
      if ( ntk.is_xor( ch_node ) )
      {
        auto leaves_ch = node_to_cut[ch_node];

        if ( leaves_ch.size() + node_to_cut[n].size() < max_cut_size )
        {
          node_to_cut[n].insert( node_to_cut[n].end(), leaves_ch.begin(), leaves_ch.end() );
        }
        else
        {
          node_to_cut[n].push_back( ch_node );
        }
      }
      else
      {
        node_to_cut[n].push_back( ch_node );
      }
    } );

    std::stable_sort( node_to_cut[n].begin(), node_to_cut[n].end() );
    node_to_cut[n].erase( unique( node_to_cut[n].begin(), node_to_cut[n].end() ), node_to_cut[n].end() );
  }

  template<typename NetworkCuts, typename Ntk>
  static void apply( NetworkCuts& cuts, Ntk const& ntk )
  {

    std::map<uint32_t, std::vector<uint32_t>> node_to_cut;

    topo_view<Ntk>( ntk ).foreach_node( [&]( auto n ) {
      if ( ntk.is_xor( n ) )
      {
        const auto index = ntk.node_to_index( n );
        auto& cut_set = cuts.cuts( index );

        /* clear the cut set of the node */
        cut_set.clear();

        /* add an empty cut and modify its leaves */
        grow_xor_cut( ntk, n, node_to_cut );

        auto& my_cut = cut_set.add_cut( node_to_cut[n].begin(), node_to_cut[n].end() );

        assert( node_to_cut[n].size() <= 16 );
        /* set to zero cost */
        my_cut->data.cost = 0u;

        /* crate cut truth table */
        kitty::dynamic_truth_table tt( node_to_cut[n].size() );
        kitty::create_parity( tt );
        my_cut->func_id = cuts.insert_truth_table( tt );
      }
    } );
  }
};

template<>
struct cut_enumeration_update_cut<cut_enumeration_spectr_cut>
{
  template<typename Cut, typename NetworkCuts, typename Ntk>
  static void apply( Cut& cut, NetworkCuts const& cuts, Ntk const& ntk, node<Ntk> const& n )
  {
    uint32_t delay{ 0 };

    auto tt = cuts.truth_table( cut );
    auto spectrum = kitty::rademacher_walsh_spectrum( tt );
    cut->data.cost = std::count_if( spectrum.begin(), spectrum.end(), []( auto s ) { return s != 0; } );

    float flow = cut.size() < 2 ? 0.0f : 1.0f;
    for ( auto leaf : cut )
    {
      const auto& best_leaf_cut = cuts.cuts( leaf )[0];
      delay = std::max( delay, best_leaf_cut->data.delay );
      flow += best_leaf_cut->data.flow;
    }

    cut->data.delay = 1 + delay;
    cut->data.flow = flow / ntk.fanout_size( n );
  }
};

template<int MaxLeaves>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, cut_data<false, cut_enumeration_spectr_cut>> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}, D = " << std::setw( 3 ) << c->data.delay << " A = " << c->data.flow;
  return os;
}

} // namespace mockturtle