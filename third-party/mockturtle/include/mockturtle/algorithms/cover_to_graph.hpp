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
  \file cover_to_graph.hpp
  \brief transforms a cover data structure into another network type

  \author Andrea Costamagna
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/cover.hpp"

#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

namespace mockturtle
{

namespace detail
{

template<class Ntk>
struct signals_connector
{
  signals_connector()
  {
    signals.reserve( 10000u );
  }
  void insert( signal<Ntk> signal_ntk, uint64_t node_index )
  {
    signals[node_index] = signal_ntk;
  }

  std::unordered_map<uint64_t, signal<Ntk>> signals;
};

/*! \brief cover_to_graph_converter
 * This data type is equipped with the main operations involved in the cover to graph conversion.
 * Given a cover network its features are mapped into the corresponding ones of a graph using the
 * signals_connector for storing the signal of the new network associated to the node index in the cover one.
 * The mapping is performed by the convert method, which must be called explicitly to perform the mapping.
 */
template<class Ntk>
class cover_to_graph_converter
{

  using type_cover_signals = std::vector<uint64_t>;

public:
  cover_to_graph_converter( Ntk& ntk, const cover_network& cover_ntk )
      : _ntk( ntk ),
        _cover_ntk( cover_ntk )
  {
  }

#pragma region recursive functions
  /*! \brief recursive_or
   * If only one signal is presented the function returns the signal itself.
   * If two signals are presented the function returns their disjunction.
   * In all other cases recursively split the input signals into two subsets of size differing by at most one.
   * These two will give rise to a unique output, which is the OR of two signals coming from the two subgraphs.
   * The problem of finding the network of each subgraph presents the same structure as the original problem.
   * Therefore, recursion can be performed.
   */
  signal<Ntk> recursive_or( const std::vector<signal<Ntk>>& signals )
  {
    if ( signals.size() == 0u )
    {
      std::cerr << "signals size is zero in recursive or\n";
      return _ntk.get_constant( 0 );
    }
    else if ( signals.size() == 1u )
    {
      return signals[0];
    }
    else if ( signals.size() == 2u )
    {
      signal<Ntk> signal_out = _ntk.create_or( signals[0], signals[1] );
      return signal_out;
    }
    else
    {
      std::size_t const half_size = signals.size() / 2;
      std::vector<signal<Ntk>> vector_l( signals.begin(), signals.begin() + half_size );
      std::vector<signal<Ntk>> vector_r( signals.begin() + half_size, signals.end() );

      return _ntk.create_or( recursive_or( vector_l ), recursive_or( vector_r ) );
    }
  }

  /*! \brief recursive_and
   * If only one signal is presented the function returns the signal itself.
   * If two signals are presented the function returns their conjunction.
   * In all other cases recursively split the input signals into two subsets of size differing by at most one.
   * These two will give rise to a unique output, which is the AND of two signals coming from the two subgraphs.
   * The problem of finding the network of each subgraph presents the same structure as the original problem.
   * Therefore, recursion can be performed.
   */
  signal<Ntk> recursive_and( std::vector<signal<Ntk>> const& signals )
  {
    if ( signals.size() == 0u )
    {
      std::cerr << "signals size is zero in recursive and\n";
      return _ntk.get_constant( 0 );
    }
    else if ( signals.size() == 1u )
    {
      return signals[0];
    }
    else if ( signals.size() == 2u )
    {
      signal<Ntk> signal_out = _ntk.create_and( signals[0], signals[1] );
      return signal_out;
    }
    else
    {
      std::size_t const half_size = signals.size() / 2;
      std::vector<signal<Ntk>> vector_l( signals.begin(), signals.begin() + half_size );
      std::vector<signal<Ntk>> vector_r( signals.begin() + half_size, signals.end() );
      return _ntk.create_and( recursive_and( vector_l ), recursive_and( vector_r ) );
    }
  }
#pragma endregion

  /*! \brief convert_cube_to_graph
   * Given a node, a cube stored into it and the boolean type associated to the SOP/POS, all children are scanned.
   * Unless the cube is independent of the value of the children ( don't care ), the signal is stored in a vector.
   * Finally, depending on the bit value of the cube, the signal influencing the cover or their negation are used
   * to create the subgraph. This create the products/sums in the SOP/POS.
   */
#pragma region converter functions
  signal<Ntk> convert_cube_to_graph( const mockturtle::cover_storage_node& Nde, const kitty::cube& cb, const bool& is_sop )
  {
    std::vector<signal<Ntk>> signals;

    for ( auto j = 0u; j < Nde.children.size(); j++ )
    {
      if ( cb.get_mask( j ) == 1 )
      {
        if ( cb.get_bit( j ) == 1 )
        {
          signals.emplace_back( ( is_sop ) ? _connector.signals[Nde.children[j].index] : !_connector.signals[Nde.children[j].index] );
        }
        else
        {
          signals.emplace_back( ( is_sop ) ? !_connector.signals[Nde.children[j].index] : _connector.signals[Nde.children[j].index] );
        }
      }
    }

    return is_sop ? recursive_and( signals ) : recursive_or( signals );
  }

  /*! \brief convert_node_to_graph
   * This helper function receives as input a node, storing the cover information.
   * This information corresponds to a vector of cubes and to a boolean determining whether
   * the cubes represent the ON set or the OFF set.
   * Each cube is mapped into a subgraph and the output signals are collected in a vector, corresponding to the
   * products/sums of the SOP/POS.
   * Depending on the boolean, the SOP/POS is finally performed using the recursive OR/AND.
   */
  signal<Ntk> convert_node_to_graph( const mockturtle::cover_storage_node& Nde )
  {
    auto& cbs = _cover_ntk._storage->data.covers[Nde.data[1].h1].first;

    std::vector<signal<Ntk>> signals_internal;
    bool is_sop = _cover_ntk._storage->data.covers[Nde.data[1].h1].second;

    for ( auto const& cb : cbs )
    {
      signals_internal.emplace_back( convert_cube_to_graph( Nde, cb, is_sop ) );
    }

    return ( is_sop ? recursive_or( signals_internal ) : recursive_and( signals_internal ) );
  }

  Ntk get_network()
  {
    return _ntk;
  }

  /*! \brief convert
   * This method combines the helper functions and performs the mapping of a cover network into the desired graph.
   */
  void run()
  {
    /* convert the pi */
    for ( auto const& inpt : _cover_ntk._storage->inputs )
    {
      _connector.insert( _ntk.create_pi(), inpt );
    }

    /* convert the nodes */
    for ( auto const& nde : _cover_ntk._storage->nodes )
    {
      uint64_t index = _cover_ntk._storage->hash[nde];
      bool condition1 = ( std::find( _cover_ntk._storage->inputs.begin(), _cover_ntk._storage->inputs.end(), index ) != _cover_ntk._storage->inputs.end() );
      bool condition2 = nde.data[1].h1 == 0 || nde.data[1].h1 == 1;

      /* convert only the nodes that are neither inputs nor constants */
      if ( !condition1 && !condition2 )
      {
        _connector.insert( convert_node_to_graph( nde ), _cover_ntk._storage->hash[nde] );
      } /* convert separately the constant 0 */
      else if ( nde.data[1].h1 == 0 )
      {
        _connector.insert( _ntk.get_constant( false ), _cover_ntk._storage->hash[nde] );
      } /* convert separately the constant 1 */
      else if ( nde.data[1].h1 == 1 )
      {
        _connector.insert( _ntk.get_constant( true ), _cover_ntk._storage->hash[nde] );
      }
    }

    /* convert the outputs */
    for ( const auto& outpt : _cover_ntk._storage->outputs )
    {
      _ntk.create_po( _connector.signals[outpt.index] );
    }
  }

private:
  Ntk& _ntk;
  cover_network const& _cover_ntk;
  signals_connector<Ntk> _connector;
};

} /* namespace detail */

/*! \brief Inline convert a `cover_network` into another network type.
 *
 * **Required network functions:**
 * - `create_and`
 * - `create_or`
 * - `create_buf`
 * - `create_not`
 *
 * \param cover_ntk Input network of type `cover_network`.
 * \param ntk Output network of type `Ntk`.
 */
template<class Ntk>
void convert_cover_to_graph( Ntk& ntk, const cover_network& cover_ntk )
{
  static_assert( has_create_and_v<Ntk>, "NtkDest does not implement the create_not method" );
  static_assert( has_create_or_v<Ntk>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_buf_v<Ntk>, "NtkDest does not implement the create_not method" );
  static_assert( has_create_not_v<Ntk>, "NtkDest does not implement the create_not method" );

  detail::cover_to_graph_converter<Ntk> converter( ntk, cover_ntk );
  converter.run();
}

/*! \brief Out-of-place convert a `cover_network` into another network type.
 *
 * **Required network functions:**
 * - `create_and`
 * - `create_or`
 * - `create_buf`
 * - `create_not`
 *
 * \param cover_ntk Input network of type `cover_network`.
 * \return ntk Output network of type `Ntk`.
 */
template<class Ntk>
Ntk convert_cover_to_graph( const cover_network& cover_ntk )
{
  static_assert( has_create_and_v<Ntk>, "NtkDest does not implement the create_not method" );
  static_assert( has_create_or_v<Ntk>, "NtkDest does not implement the create_po method" );
  static_assert( has_create_buf_v<Ntk>, "NtkDest does not implement the create_not method" );
  static_assert( has_create_not_v<Ntk>, "NtkDest does not implement the create_not method" );

  Ntk ntk;
  detail::cover_to_graph_converter<Ntk> converter( ntk, cover_ntk );
  converter.run();
  return converter.get_network();
}

} /* namespace mockturtle */
