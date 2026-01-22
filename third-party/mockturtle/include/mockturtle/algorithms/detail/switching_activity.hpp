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
  \file switching_activity.hpp
  \brief Utility to compute the switching activity

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <vector>

#include "../simulation.hpp"

#include <kitty/bit_operations.hpp>
#include <kitty/partial_truth_table.hpp>

namespace mockturtle::detail
{

/*! \brief Switching Activity.
 *
 * This function computes the switching activity for each node
 * in the network by performing random simulation.
 *
 * \param ntk Network
 * \param simulation_size Number of simulation bits
 */
template<typename Ntk>
std::vector<float> switching_activity( Ntk const& ntk, unsigned simulation_size = 2048 )
{
  std::vector<float> sw_map( ntk.size() );
  partial_simulator sim( ntk.num_pis(), simulation_size );

  auto tts = simulate_nodes<kitty::partial_truth_table, Ntk, partial_simulator>( ntk, sim );

  ntk.foreach_node( [&]( auto const& n ) {
    float ones = static_cast<float>( kitty::count_ones( tts[n] ) );
    float activity = 2.0 * ones / simulation_size * ( simulation_size - ones ) / simulation_size;
    sw_map[ntk.node_to_index( n )] = activity;
  } );

  return sw_map;
}

} // namespace mockturtle::detail