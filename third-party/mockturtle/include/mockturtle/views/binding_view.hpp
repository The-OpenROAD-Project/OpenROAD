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
  \file binding_view.hpp
  \brief Implements methods to bind the network to a standard cell library

  \author Alessandro Tempia Calvino
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../io/genlib_reader.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

#include <iostream>
#include <map>

namespace mockturtle
{

/*! \brief Adds bindings to a technology library and mapping API methods.
 *
 * This view adds methods to create and manage a mapped network that
 * implements gates contained in a technology library. This view
 * is returned by the technology mapping command `map`. It can be used
 * to report statistics about the network and write the network into
 * a verilog file. It always adds the functions `has_binding`,
 * `remove_binding`, `add_binding`, `add_binding_with_check`, `get_binding`,
 * `get_binding_index`, `get_library`, `compute_area`, `compute_worst_delay`,
 * `report_stats`, and `report_gates_usage`.
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
      binding_view<klut_network> res = map( aig, tech_lib );

      // prints stats and gates usage
      res.report_stats();
      res.report_gates_usage();

      // write the mapped network in verilog
      write_verilog_with_binding( res, "file.v" );
   \endverbatim
 */
template<class Ntk>
class binding_view : public Ntk
{
public:
  using node = typename Ntk::node;

public:
  explicit binding_view( std::vector<gate> const& library )
      : Ntk(), _library{ library }, _bindings( *this )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  }

  explicit binding_view( Ntk const& ntk, std::vector<gate> const& library )
      : Ntk( ntk ), _library{ library }, _bindings( *this )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
    static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  }

  binding_view<Ntk>& operator=( binding_view<Ntk> const& binding_ntk )
  {
    Ntk::operator=( binding_ntk );
    _library = binding_ntk._library;
    _bindings = binding_ntk._bindings;
    return *this;
  }

  void add_binding( node const& n, uint32_t gate_id )
  {
    assert( gate_id < _library.size() );
    _bindings[n] = gate_id;
  }

  bool add_binding_with_check( node const& n, uint32_t gate_id )
  {
    assert( gate_id < _library.size() );

    auto const& cell = _library[gate_id];

    if ( Ntk::node_function( n ) == cell.function )
    {
      _bindings[n] = gate_id;
      return true;
    }
    return false;
  }

  void remove_binding( node const& n ) const
  {
    _bindings.erase( n );
  }

  const gate& get_binding( node const& n ) const
  {
    return _library[_bindings[n]];
  }

  bool has_binding( node const& n ) const
  {
    return _bindings.has( n );
  }

  unsigned int get_binding_index( node const& n ) const
  {
    return _bindings[n];
  }

  const std::vector<gate>& get_library() const
  {
    return _library;
  }

  double compute_area() const
  {
    double area = 0;
    Ntk::foreach_node( [&]( auto const& n, auto ) {
      if ( has_binding( n ) )
      {
        area += get_binding( n ).area;
      }
    } );

    return area;
  }

  double compute_worst_delay() const
  {
    topo_view ntk_topo{ *this };
    node_map<double, Ntk> delays( *this );
    double worst_delay = 0;

    ntk_topo.foreach_node( [&]( auto const& n, auto ) {
      if ( Ntk::is_constant( n ) || Ntk::is_pi( n ) )
      {
        delays[n] = 0;
        return true;
      }

      if ( has_binding( n ) )
      {
        auto const& g = get_binding( n );
        double gate_delay = 0;
        Ntk::foreach_fanin( n, [&]( auto const& f, auto i ) {
          gate_delay = std::max( gate_delay, (double)( delays[f] + std::max( g.pins[i].rise_block_delay, g.pins[i].fall_block_delay ) ) );
        } );
        delays[n] = gate_delay;
        worst_delay = std::max( worst_delay, gate_delay );
      }
      return true;
    } );

    return worst_delay;
  }

  void report_stats( std::ostream& os = std::cout ) const
  {
    os << fmt::format( "[i] Report stats: area = {:>5.2f}; delay = {:>5.2f};\n", compute_area(), compute_worst_delay() );
  }

  void report_gates_usage( std::ostream& os = std::cout ) const
  {
    std::vector<uint32_t> gates_profile( _library.size(), 0u );

    double area = 0;
    Ntk::foreach_node( [&]( auto const& n, auto ) {
      if ( has_binding( n ) )
      {
        auto const& g = get_binding( n );
        ++gates_profile[g.id];
        area += g.area;
      }
    } );

    os << "[i] Report gates usage:\n";

    uint32_t tot_instances = 0u;
    for ( auto i = 0u; i < gates_profile.size(); ++i )
    {
      if ( gates_profile[i] > 0u )
      {
        float tot_gate_area = gates_profile[i] * _library[i].area;

        os << fmt::format( "[i] {:<25}", _library[i].name )
           << fmt::format( "\t Instance = {:>10d}", gates_profile[i] )
           << fmt::format( "\t Area = {:>12.2f}", tot_gate_area )
           << fmt::format( " {:>8.2f} %\n", tot_gate_area / area * 100 );

        tot_instances += gates_profile[i];
      }
    }

    os << fmt::format( "[i] {:<25}", "TOTAL" )
       << fmt::format( "\t Instance = {:>10d}", tot_instances )
       << fmt::format( "\t Area = {:>12.2f}   100.00 %\n", area );
  }

private:
  std::vector<gate> _library;
  node_map<uint32_t, Ntk, std::unordered_map<node, uint32_t>> _bindings;
}; /* binding_view */

template<class T>
binding_view( T const& ) -> binding_view<T>;

} // namespace mockturtle