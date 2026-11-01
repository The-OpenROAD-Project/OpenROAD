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
  \file cell_view.hpp
  \brief Implements methods to bind the network to a standard cell library

  \author Alessandro Tempia Calvino
*/

#pragma once

#include "../utils/node_map.hpp"
#include "../utils/standard_cell.hpp"
#include "../views/topo_view.hpp"

#include <iostream>
#include <map>

namespace mockturtle
{

/*! \brief Adds cells to a technology library and mapping API methods.
 *
 * This view adds methods to create and manage a mapped network that
 * implements cells contained in a technology library. This view
 * is returned by the technology mapping command `emap`. It can be used
 * to report statistics about the network and write the network into
 * a verilog file. It always adds the functions `has_cell`,
 * `remove_cell`, `add_cell`, `add_cell_with_check`, `get_cell`,
 * `get_cell_index`, `get_library`, `compute_area`, `compute_worst_delay`,
 * `report_stats`, and `report_cells_usage`.
 *
 * **Required network functions:**
 * - `size`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `is_constant`
 * - `is_pi`
 *
 * Example
 *
   \verbatim embed:rst

   .. code-block:: c++

      // create network somehow
      aig_network aig = ...;

      // read cell library in genlib format
      std::vector<gate> gates;
      lorina::read_genlib( "file.genlib", genlib_reader( gates ) )
      tech_library tech_lib( gates );

      // call technology mapping to obtain the view
      cell_view<block_network> res = emap_block( aig, tech_lib );

      // prints stats and cells usage
      res.report_stats();
      res.report_cells_usage();
   \endverbatim
 */
template<class Ntk>
class cell_view : public Ntk
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit cell_view( std::vector<standard_cell> const& library )
      : Ntk(), _library{ library }, _cells( *this )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  }

  explicit cell_view( Ntk const& ntk, std::vector<standard_cell> const& library )
      : Ntk( ntk ), _library{ library }, _cells( *this )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  }

  cell_view<Ntk>& operator=( cell_view<Ntk> const& cell_ntk )
  {
    Ntk::operator=( cell_ntk );
    _library = cell_ntk._library;
    _cells = cell_ntk._cells;
    return *this;
  }

  void add_cell( node const& n, uint32_t cell_id )
  {
    assert( cell_id < _library.size() );
    _cells[n] = cell_id;
  }

  bool add_cell_with_check( node const& n, uint32_t cell_id )
  {
    assert( cell_id < _library.size() );

    auto const& cell = _library[cell_id];

    if constexpr ( has_num_outputs_v<Ntk> && has_node_function_v<Ntk> )
    {
      if ( Ntk::num_outputs( n ) != cell.gates.size() )
        return false;

      for ( uint32_t i = 0; i < Ntk::num_outputs( n ); ++i )
      {
        if ( Ntk::node_function_pin( n, i ) != cell.gates[i].function )
        {
          return false;
        }
      }

      _cells[n] = cell_id;
      return true;
    }

    if ( cell.gates.size() > 1 )
      return false;

    if ( Ntk::node_function( n ) == cell.gates[0].function )
    {
      _cells[n] = cell_id;
      return true;
    }

    return false;
  }

  void remove_cell( node const& n ) const
  {
    _cells.erase( n );
  }

  const standard_cell& get_cell( node const& n ) const
  {
    return _library[_cells[n]];
  }

  bool has_cell( node const& n ) const
  {
    return _cells.has( n );
  }

  unsigned int get_cell_index( node const& n ) const
  {
    return _cells[n];
  }

  const std::vector<standard_cell>& get_library() const
  {
    return _library;
  }

  double compute_area() const
  {
    double area = 0;
    Ntk::foreach_node( [&]( auto const& n, auto ) {
      if ( has_cell( n ) )
      {
        area += get_cell( n ).area;
      }
    } );

    return area;
  }

  double compute_worst_delay() const
  {
    topo_view ntk_topo{ *this };
    std::vector<std::vector<double>> delays( Ntk::size() );
    double worst_delay = 0;

    ntk_topo.foreach_node( [&]( auto const& n, auto ) {
      if ( Ntk::is_constant( n ) || Ntk::is_pi( n ) )
      {
        delays[n].push_back( 0 );
        return true;
      }

      if ( has_cell( n ) )
      {
        auto const& cell = get_cell( n );

        for ( gate const& g : cell.gates )
        {
          double cell_delay = 0;
          if constexpr ( has_get_output_pin_v<Ntk> )
          {
            Ntk::foreach_fanin( n, [&]( signal const& f, auto i ) {
              cell_delay = std::max( cell_delay, delays[Ntk::get_node( f )][Ntk::get_output_pin( f )] + std::max( g.pins[i].rise_block_delay, g.pins[i].fall_block_delay ) );
            } );
          }
          else
          {
            Ntk::foreach_fanin( n, [&]( signal const& f, auto i ) {
              cell_delay = std::max( cell_delay, delays[Ntk::get_node( f )].front() + std::max( g.pins[i].rise_block_delay, g.pins[i].fall_block_delay ) );
            } );
          }
          delays[n].push_back( cell_delay );
          worst_delay = std::max( worst_delay, cell_delay );
        }
      }
      else
      {
        worst_delay = -1;
        return false;
      }
      return true;
    } );

    return worst_delay;
  }

  void report_stats( std::ostream& os = std::cout ) const
  {
    os << fmt::format( "[i] Report stats: area = {:>5.2f}; delay = {:>5.2f};\n", compute_area(), compute_worst_delay() );
  }

  void report_cells_usage( std::ostream& os = std::cout ) const
  {
    std::vector<uint32_t> cells_profile( _library.size(), 0u );

    double area = 0;
    Ntk::foreach_node( [&]( node const& n, auto ) {
      if ( has_cell( n ) )
      {
        auto const& g = get_cell( n );
        ++cells_profile[g.id];
        area += g.area;
      }
    } );

    os << "[i] Report cells usage:\n";

    uint32_t tot_instances = 0u;
    for ( auto i = 0u; i < cells_profile.size(); ++i )
    {
      if ( cells_profile[i] > 0u )
      {
        float tot_cell_area = cells_profile[i] * _library[i].area;

        os << fmt::format( "[i] {:<25}", _library[i].name )
           << fmt::format( "\t Instance = {:>10d}", cells_profile[i] )
           << fmt::format( "\t Area = {:>12.2f}", tot_cell_area )
           << fmt::format( " {:>8.2f} %\n", tot_cell_area / area * 100 );

        tot_instances += cells_profile[i];
      }
    }

    os << fmt::format( "[i] {:<25}", "TOTAL" )
       << fmt::format( "\t Instance = {:>10d}", tot_instances )
       << fmt::format( "\t Area = {:>12.2f}   100.00 %\n", area );
  }

private:
  std::vector<standard_cell> _library;
  node_map<uint32_t, Ntk, std::unordered_map<node, uint32_t>> _cells;
}; /* cell_view */

template<class T>
cell_view( T const& ) -> cell_view<T>;

} // namespace mockturtle