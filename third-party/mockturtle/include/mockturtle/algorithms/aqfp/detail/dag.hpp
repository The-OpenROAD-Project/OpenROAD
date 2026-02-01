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
  \file dag.hpp
  \brief AQFP DAG data structure

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <sstream>
#include <vector>

#include <fmt/format.h>

namespace mockturtle
{

/*! \brief Represents a single-output connected Partial DAG or DAG.
 *
 * A partial DAG is a network with majority gates where some gates may have unconnected fanin slots.
 * A DAG is a network with majority gates obtained from a partial DAG by specifying which
 * unconnected fanin slots connect to the same primary input.
 * Optionally, a DAG may designate which fanin slots are connected the constant 0.
 * Unlike logic networks elsewhere in mockturtle, gates are numbered from 0 starting from the top gate.
 */
template<typename NodeT = int>
struct aqfp_dag
{
  using node_type = NodeT;

  std::vector<std::vector<NodeT>> nodes; // fanins of nodes
  std::vector<NodeT> input_slots;        // identifiers of the input slots (bundles of fanins where the inputs will be connected)
  NodeT zero_input = 0;                  // id of the input slot that is connected to constant 0

  aqfp_dag( const std::vector<std::vector<NodeT>>& nodes = {}, const std::vector<NodeT>& input_slots = {}, node_type zero_input = {} )
      : nodes( nodes ), input_slots( input_slots ), zero_input( zero_input ) {}

  aqfp_dag( const std::string& str )
  {
    decode_from_string( str );
  }

  /*! \brief Compare to logical networks for equality. */
  bool operator==( const aqfp_dag& rhs ) const
  {
    if ( nodes.size() != rhs.nodes.size() )
      return false;

    for ( auto i = 0u; i < nodes.size(); i++ )
    {
      auto x1 = nodes[i];
      auto x2 = rhs.nodes[i];
      std::stable_sort( x1.begin(), x1.end() );
      std::stable_sort( x2.begin(), x2.end() );
      if ( x1 != x2 )
        return false;
    }

    auto y1 = input_slots;
    auto y2 = rhs.input_slots;
    std::stable_sort( y1.begin(), y1.end() );
    std::stable_sort( y2.begin(), y2.end() );
    if ( y1 != y2 )
      return false;

    if ( zero_input != rhs.zero_input )
      return false;

    return true;
  }

  /*! \brief Return the number of majority gates. */
  uint32_t num_gates() const
  {
    return nodes.size() - input_slots.size();
  }

  /*! \brief Decode a string representation of a DAG into a DAG.
   *
   * Format: ng ni zi k0 g0f0 g0f1 .. g0fk0 k1 g1f0 g1f1 .. g1fk1 ....
   * ng := num gates, ni := num inputs, zi := zero input, ki := num fanin of i-th gate, gifj = j-th fanin of i-th gate
   */
  void decode_from_string( const std::string& str )
  {
    std::istringstream iss( str );
    auto ng = 0u;
    auto ni = 0u;
    auto zi = 0u;

    iss >> ng >> ni >> zi;
    zero_input = zi;

    std::vector<uint32_t> level( ng + ni, 0u );
    for ( auto i = 0u; i < ng; i++ )
    {
      auto nf = 0u;
      iss >> nf;

      nodes.push_back( {} );

      for ( auto j = 0u; j < nf; j++ )
      {
        auto t = 0u;
        iss >> t;
        nodes[i].push_back( t );
      }
    }

    for ( auto i = 0u; i < ni; i++ )
    {
      nodes.push_back( {} );
      input_slots.push_back( nodes.size() - 1 );
    }
  }

  /*! \brief Encode a DAG as a string.
   *
   * Format: ng ni zi k0 g0f0 g0f1 .. g0fk0 k1 g1f0 g1f1 .. g1fk1 ....
   * ng := num gates, ni := num inputs, zi := zero input, ki := num fanin of i-th gate, gifj = j-th fanin of i-th gate
   */
  std::string encode_as_string() const
  {
    std::stringstream ss;
    ss << num_gates() << " " << input_slots.size() << " " << zero_input;
    for ( auto i = 0u; i < num_gates(); i++ )
    {
      ss << fmt::format( " {} {}", nodes[i].size(), fmt::join( nodes[i], " " ) );
    }

    return ss.str();
  }
};

} // namespace mockturtle
