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
  \file write_verilog.hpp
  \brief Write networks to structural Verilog format

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../utils/string_utils.hpp"
#include "../views/binding_view.hpp"
#include "../views/topo_view.hpp"

#include <fmt/format.h>
#include <kitty/print.hpp>
#include <lorina/verilog.hpp>

#include <array>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace mockturtle
{

using namespace std::string_literals;

namespace detail
{

template<class Ntk>
std::vector<std::pair<bool, std::string>>
format_fanin( Ntk const& ntk, node<Ntk> const& n, node_map<std::string, Ntk>& node_names )
{
  std::vector<std::pair<bool, std::string>> children;
  ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      std::string postfix = ntk.is_crossing( ntk.get_node( f ) ) ? ( ntk.is_second( f ) ? "_2" : "_1" ) : "";
      children.emplace_back( std::make_pair( ntk.get_fanin_negations( n )[i], node_names[f] + postfix ) );
    }
    else
    {
      children.emplace_back( std::make_pair( ntk.is_complemented( f ), node_names[f] ) );
    }
  } );
  return children;
}

template<class Ntk>
std::vector<std::pair<bool, std::string>>
format_fanin( Ntk const& ntk, node<Ntk> const& n, node_map<std::vector<std::string>, Ntk>& node_names )
{
  std::vector<std::pair<bool, std::string>> children;
  ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      std::string postfix = ntk.is_crossing( ntk.get_node( f ) ) ? ( ntk.is_second( f ) ? "_2" : "_1" ) : "";
      children.emplace_back( std::make_pair( ntk.get_fanin_negations( n )[i], node_names[f].front() + postfix ) );
    }
    else if constexpr ( has_is_multioutput_v<Ntk> )
    {
      children.emplace_back( std::make_pair( ntk.is_complemented( f ), node_names[f][ntk.get_output_pin( f )] ) );
    }
    else
    {
      children.emplace_back( std::make_pair( ntk.is_complemented( f ), node_names[f].front() ) );
    }
  } );
  return children;
}

template<typename Signal>
struct verilog_writer_signal_hash
{
  uint64_t operator()( const Signal& f ) const
  {
    return f.data;
  }
};

} // namespace detail

struct write_verilog_params
{
  std::optional<std::string> module_name{ std::nullopt };
  std::vector<std::pair<std::string, uint32_t>> input_names;
  std::vector<std::pair<std::string, uint32_t>> output_names;
  bool verbose{ false };
};

/*! \brief Writes network in structural Verilog format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `is_and`
 * - `is_or`
 * - `is_xor`
 * - `is_xor3`
 * - `is_maj`
 * - `is_ite`
 * - `node_to_index`
 *
 * \param ntk Network
 * \param os Output stream
 */
template<class Ntk>
void write_verilog( Ntk const& ntk, std::ostream& os, write_verilog_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_is_or_v<Ntk>, "Ntk does not implement the is_or method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_is_xor3_v<Ntk>, "Ntk does not implement the is_xor3 method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_is_ite_v<Ntk>, "Ntk does not implement the is_ite method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );

  assert( ntk.is_combinational() && "Network has to be combinational" );

  lorina::verilog_writer writer( os );

  if constexpr ( is_buffered_network_type_v<Ntk> )
  {
    writer.on_module_begin( "buffer", { "i" }, { "o" } );
    writer.on_input( "i" );
    writer.on_output( "o" );
    writer.on_module_end();

    writer.on_module_begin( "inverter", { "i" }, { "o" } );
    writer.on_input( "i" );
    writer.on_output( "o" );
    writer.on_module_end();
  }
  if constexpr ( is_crossed_network_type_v<Ntk> )
  {
    writer.on_module_begin( "crossing", { "i1", "i2" }, { "o1", "o2" } );
    writer.on_input( std::vector<std::string>( { "i1", "i2" } ) );
    writer.on_output( std::vector<std::string>( { "o1", "o2" } ) );
    writer.on_module_end();
  }

  std::vector<std::string> xs, inputs;
  if ( ps.input_names.empty() )
  {
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
    {
      ntk.foreach_pi( [&]( auto const& i, uint32_t index ) {
        if ( ntk.has_name( ntk.make_signal( i ) ) )
        {
          xs.emplace_back( ntk.get_name( ntk.make_signal( i ) ) );
        }
        else
        {
          xs.emplace_back( fmt::format( "x{}", index ) );
        }
      } );
    }
    else
    {
      ntk.foreach_pi( [&]( auto const& i, uint32_t index ) {
        (void)i;
        xs.emplace_back( fmt::format( "x{}", index ) );
      } );
    }
    inputs = xs;
  }
  else
  {
    uint32_t ctr{ 0u };
    for ( auto const& [name, width] : ps.input_names )
    {
      inputs.emplace_back( name );
      ctr += width;
      for ( auto i = 0u; i < width; ++i )
      {
        xs.emplace_back( fmt::format( "{}[{}]", name, i ) );
      }
    }
    if ( ctr != ntk.num_pis() )
    {
      std::cerr << "[e] input names do not partition all inputs\n";
    }
  }

  std::vector<std::string> ys, outputs;
  if ( ps.output_names.empty() )
  {
    if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
    {
      ntk.foreach_po( [&]( auto const& o, uint32_t index ) {
        if ( ntk.has_output_name( index ) )
        {
          ys.emplace_back( ntk.get_output_name( index ) );
        }
        else
        {
          ys.emplace_back( fmt::format( "y{}", index ) );
        }
      } );
    }
    else
    {
      ntk.foreach_po( [&]( auto const& o, uint32_t index ) {
        (void)o;
        ys.emplace_back( fmt::format( "y{}", index ) );
      } );
    }
    outputs = ys;
  }
  else
  {
    uint32_t ctr{ 0u };
    for ( auto const& [name, width] : ps.output_names )
    {
      outputs.emplace_back( name );
      ctr += width;
      for ( auto i = 0u; i < width; ++i )
      {
        ys.emplace_back( fmt::format( "{}[{}]", name, i ) );
      }
    }
    if ( ctr != ntk.num_pos() )
    {
      std::cerr << "[e] output names do not partition all outputs\n";
    }
  }

  std::vector<std::string> ws;

  if constexpr ( is_buffered_network_type_v<Ntk> )
  {
    static_assert( has_is_buf_v<Ntk>, "Ntk does not implement the is_buf method" );
    ntk.foreach_node( [&]( auto const& n ) {
      if ( ntk.fanin_size( n ) > 0 )
        ws.emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
    } );
  }
  else
  {
    ntk.foreach_gate( [&]( auto const& n ) {
      ws.emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
    } );
  }

  std::string module_name = "top";
  if ( ps.module_name )
  {
    module_name = *ps.module_name;
  }
  else
  {
    if constexpr ( has_get_network_name_v<Ntk> )
    {
      if ( ntk.get_network_name().length() > 0 )
      {
        module_name = ntk.get_network_name();
      }
    }
  }
  writer.on_module_begin( module_name, inputs, outputs );

  if ( ps.input_names.empty() )
  {
    writer.on_input( xs );
  }
  else
  {
    for ( auto const& [name, width] : ps.input_names )
    {
      writer.on_input( width, name );
    }
  }
  if ( ps.output_names.empty() )
  {
    writer.on_output( ys );
  }
  else
  {
    for ( auto const& [name, width] : ps.output_names )
    {
      writer.on_output( width, name );
    }
  }
  if ( !ws.empty() )
  {
    writer.on_wire( ws );
  }

  node_map<std::string, Ntk> node_names( ntk );
  node_names[ntk.get_constant( false )] = "1'b0";
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    node_names[ntk.get_constant( true )] = "1'b1";

  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    node_names[n] = xs[i];
  } );

  topo_view ntk_topo{ ntk };

  ntk_topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
      return true;

    /* assign a name */
    node_names[n] = fmt::format( "n{}", ntk.node_to_index( n ) );

    if constexpr ( has_is_buf_v<Ntk> )
    {
      if ( ntk.is_buf( n ) )
      {
        auto const fanin = detail::format_fanin<Ntk>( ntk, n, node_names );
        assert( fanin.size() == 1 );
        std::vector<std::pair<std::string, std::string>> args;
        if ( fanin[0].first ) /* input negated */
        {
          args.emplace_back( std::make_pair( "i", fanin[0].second ) );
          args.emplace_back( std::make_pair( "o", node_names[n] ) );
          writer.on_module_instantiation( "inverter", {}, "inv_" + node_names[n], args );
        }
        else
        {
          args.emplace_back( std::make_pair( "i", fanin[0].second ) );
          args.emplace_back( std::make_pair( "o", node_names[n] ) );
          writer.on_module_instantiation( "buffer", {}, "buf_" + node_names[n], args );
        }
        return true;
      }
    }

    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      if ( ntk.is_crossing( n ) )
      {
        auto const fanin = detail::format_fanin<Ntk>( ntk, n, node_names );
        assert( fanin.size() == 2 );
        std::vector<std::pair<std::string, std::string>> args;
        args.emplace_back( std::make_pair( "i1", fanin[0].second ) );
        args.emplace_back( std::make_pair( "i2", fanin[1].second ) );
        args.emplace_back( std::make_pair( "o1", node_names[n] + "_1" ) );
        args.emplace_back( std::make_pair( "o2", node_names[n] + "_2" ) );
        writer.on_module_instantiation( "crossing", {}, "cross_" + node_names[n], args );
        return true;
      }
    }

    if ( ntk.is_and( n ) )
    {
      writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "&" );
    }
    else if ( ntk.is_or( n ) )
    {
      writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "|" );
    }
    else if ( ntk.is_xor( n ) || ntk.is_xor3( n ) )
    {
      writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "^" );
    }
    else if ( ntk.is_maj( n ) )
    {
      std::array<signal<Ntk>, 3> children;
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) { children[i] = f; } );

      if ( ntk.is_constant( ntk.get_node( children[0u] ) ) )
      {
        std::vector<std::pair<bool, std::string>> vs;
        vs.emplace_back( std::make_pair( ntk.is_complemented( children[1u] ), node_names[ntk.get_node( children[1u] )] ) );
        vs.emplace_back( std::make_pair( ntk.is_complemented( children[2u] ), node_names[ntk.get_node( children[2u] )] ) );

        if ( ntk.is_complemented( children[0u] ) )
        {
          // or
          writer.on_assign( node_names[n], { vs[0u], vs[1u] }, "|" );
        }
        else
        {
          // and
          writer.on_assign( node_names[n], { vs[0u], vs[1u] }, "&" );
        }
      }
      else
      {
        writer.on_assign_maj3( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ) );
      }
    }
    else if ( ntk.is_ite( n ) )
    {
      std::array<signal<Ntk>, 3> children;
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) { children[i] = f; } );

      if ( ntk.is_constant( ntk.get_node( children[1u] ) ) )
      {
        assert( children[1u] == ntk.get_constant( false ) );
        // a ? 0 : c = ~a & c
        std::vector<std::pair<bool, std::string>> ins;
        ins.emplace_back( std::make_pair( !ntk.is_complemented( children[0u] ), node_names[ntk.get_node( children[0u] )] ) );
        ins.emplace_back( std::make_pair( ntk.is_complemented( children[2u] ), node_names[ntk.get_node( children[2u] )] ) );
        writer.on_assign( node_names[n], ins, "&" );
      }
      else if ( ntk.get_node( children[1u] ) == ntk.get_node( children[2u] ) )
      {
        assert( !ntk.is_complemented( children[1u] ) && ntk.is_complemented( children[2u] ) );
        // a ? b : ~b = a ^ ~b
        std::vector<std::pair<bool, std::string>> ins;
        ins.emplace_back( std::make_pair( ntk.is_complemented( children[0u] ), node_names[ntk.get_node( children[0u] )] ) );
        ins.emplace_back( std::make_pair( ntk.is_complemented( children[2u] ), node_names[ntk.get_node( children[2u] )] ) );
        writer.on_assign( node_names[n], ins, "^" );
      }
      else
      {
        writer.on_assign_mux21( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ) );
      }
    }
    else
    {
      if constexpr ( has_is_nary_and_v<Ntk> )
      {
        if ( ntk.is_nary_and( n ) )
        {
          writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "&" );
          return true;
        }
      }
      if constexpr ( has_is_nary_or_v<Ntk> )
      {
        if ( ntk.is_nary_or( n ) )
        {
          writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "|" );
          return true;
        }
      }
      if constexpr ( has_is_nary_xor_v<Ntk> )
      {
        if ( ntk.is_nary_xor( n ) )
        {
          writer.on_assign( node_names[n], detail::format_fanin<Ntk>( ntk, n, node_names ), "^" );
          return true;
        }
      }
      if constexpr ( has_is_function_v<Ntk> )
      {
        fmt::print( stderr, "[w] unknown node function {}\n", kitty::to_hex( ntk.node_function( n ) ) );
      }
      writer.on_assign_unknown_gate( node_names[n] );
    }

    return true;
  } );

  ntk.foreach_po( [&]( auto const& f, auto i ) {
    writer.on_assign_po( ys[i], std::make_pair( ntk.is_complemented( f ), node_names[f] ) );
  } );

  writer.on_module_end();
}

/*! \brief Writes mapped network in structural Verilog format into output stream
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `node_to_index`
 * - `has_binding`
 * - `get_binding_index`
 *
 * \param ntk Mapped network
 * \param os Output stream
 * \param ps Verilog parameters
 */
template<class Ntk>
void write_verilog_with_binding( Ntk const& ntk, std::ostream& os, write_verilog_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_has_binding_v<Ntk>, "Ntk does not implement the has_binding method" );
  static_assert( has_get_binding_index_v<Ntk>, "Ntk does not implement the get_binding_index method" );

  assert( ntk.is_combinational() && "Network has to be combinational" );

  lorina::verilog_writer writer( os );

  std::vector<std::string> xs, inputs;
  if ( ps.input_names.empty() )
  {
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
    {
      ntk.foreach_pi( [&]( auto const& i, uint32_t index ) {
        if ( ntk.has_name( ntk.make_signal( i ) ) )
        {
          xs.emplace_back( ntk.get_name( ntk.make_signal( i ) ) );
        }
        else
        {
          xs.emplace_back( fmt::format( "x{}", index ) );
        }
      } );
    }
    else
    {
      for ( auto i = 0u; i < ntk.num_pis(); ++i )
      {
        xs.emplace_back( fmt::format( "x{}", i ) );
      }
    }
    inputs = xs;
  }
  else
  {
    uint32_t ctr{ 0u };
    for ( auto const& [name, width] : ps.input_names )
    {
      inputs.emplace_back( name );
      ctr += width;
      for ( auto i = 0u; i < width; ++i )
      {
        xs.emplace_back( fmt::format( "{}[{}]", name, i ) );
      }
    }
    if ( ctr != ntk.num_pis() )
    {
      std::cerr << "[e] input names do not partition all inputs\n";
    }
  }

  std::vector<std::string> ys, outputs;
  if ( ps.output_names.empty() )
  {
    if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
    {
      ntk.foreach_po( [&]( auto const& o, uint32_t index ) {
        if ( ntk.has_output_name( index ) )
        {
          ys.emplace_back( ntk.get_output_name( index ) );
        }
        else
        {
          ys.emplace_back( fmt::format( "y{}", index ) );
        }
      } );
    }
    else
    {
      for ( auto i = 0u; i < ntk.num_pos(); ++i )
      {
        ys.emplace_back( fmt::format( "y{}", i ) );
      }
    }
    outputs = ys;
  }
  else
  {
    uint32_t ctr{ 0u };
    for ( auto const& [name, width] : ps.output_names )
    {
      outputs.emplace_back( name );
      ctr += width;
      for ( auto i = 0u; i < width; ++i )
      {
        ys.emplace_back( fmt::format( "{}[{}]", name, i ) );
      }
    }
    if ( ctr != ntk.num_pos() )
    {
      std::cerr << "[e] output names do not partition all outputs\n";
    }
  }

  /* compute which nodes are POs and register index */
  node_map<std::vector<uint32_t>, Ntk, std::unordered_map<typename Ntk::node, std::vector<uint32_t>>> po_nodes( ntk );
  ntk.foreach_po( [&]( auto const& f, auto i ) {
    po_nodes[f].push_back( i );
  } );

  std::vector<std::string> ws;
  node_map<std::string, Ntk> node_names( ntk );

  /* constants */
  if ( ntk.has_binding( ntk.get_constant( false ) ) )
  {
    node_names[ntk.get_constant( false )] = fmt::format( "n{}", ntk.node_to_index( ntk.get_constant( false ) ) );
    if ( !po_nodes.has( ntk.get_constant( false ) ) )
    {
      ws.emplace_back( node_names[ntk.get_constant( false )] );
    }
  }
  else
  {
    node_names[ntk.get_constant( false )] = "1'b0";
  }
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    if ( ntk.has_binding( ntk.get_constant( true ) ) )
    {
      node_names[ntk.get_constant( true )] = fmt::format( "n{}", ntk.node_to_index( ntk.get_constant( true ) ) );
      if ( !po_nodes.has( ntk.get_constant( true ) ) )
      {
        ws.emplace_back( node_names[ntk.get_constant( true )] );
      }
    }
    else
    {
      node_names[ntk.get_constant( true )] = "1'b1";
    }
  }

  /* add wires */
  ntk.foreach_gate( [&]( auto const& n ) {
    if ( !po_nodes.has( n ) )
    {
      ws.emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
    }
  } );

  std::string module_name = "top";
  if ( ps.module_name )
  {
    module_name = *ps.module_name;
  }
  else
  {
    if constexpr ( has_get_network_name_v<Ntk> )
    {
      if ( ntk.get_network_name().length() > 0 )
      {
        module_name = ntk.get_network_name();
      }
    }
  }
  writer.on_module_begin( module_name, inputs, outputs );
  if ( ps.input_names.empty() )
  {
    writer.on_input( xs );
  }
  else
  {
    for ( auto const& [name, width] : ps.input_names )
    {
      writer.on_input( width, name );
    }
  }
  if ( ps.output_names.empty() )
  {
    writer.on_output( ys );
  }
  else
  {
    for ( auto const& [name, width] : ps.output_names )
    {
      writer.on_output( width, name );
    }
  }
  if ( !ws.empty() )
  {
    writer.on_wire( ws );
  }

  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    node_names[n] = xs[i];
  } );

  auto const& gates = ntk.get_library();

  int nDigits = (int)std::floor( std::log10( ntk.num_gates() ) );
  unsigned int length = 0;
  unsigned counter = 0;

  for ( auto const& gate : gates )
  {
    length = std::max( length, static_cast<unsigned int>( gate.name.length() ) );
  }

  topo_view ntk_topo{ ntk };

  ntk_topo.foreach_node( [&]( auto const& n ) {
    if ( po_nodes.has( n ) )
    {
      node_names[n] = ys[po_nodes[n][0]];
    }
    else if ( !ntk.is_constant( n ) && !ntk.is_pi( n ) )
    {
      node_names[n] = fmt::format( "n{}", ntk.node_to_index( n ) );
    }

    if ( ntk.has_binding( n ) )
    {
      auto const& gate = gates[ntk.get_binding_index( n )];
      std::string name = gate.name;

      int digits = counter == 0 ? 0 : (int)std::floor( std::log10( counter ) );
      auto fanin_names = detail::format_fanin<Ntk>( ntk, n, node_names );
      std::vector<std::pair<std::string, std::string>> args;

      auto i = 0;
      for ( auto pair : fanin_names )
      {
        args.emplace_back( std::make_pair( gate.pins[i++].name, pair.second ) );
      }
      args.emplace_back( std::make_pair( gate.output_name, node_names[n] ) );

      writer.on_module_instantiation( name.append( std::string( length - name.length(), ' ' ) ),
                                      {},
                                      std::string( "g" ) + std::string( nDigits - digits, '0' ) + std::to_string( counter ),
                                      args );
      ++counter;

      /* if node drives multiple POs, duplicate */
      if ( po_nodes.has( n ) && po_nodes[n].size() > 1 )
      {
        if ( ps.verbose )
        {
          std::cerr << "[i] node " << n << " driving multiple POs has been duplicated.\n";
        }
        auto const& po_list = po_nodes[n];
        for ( auto i = 1u; i < po_list.size(); ++i )
        {
          digits = counter == 0 ? 0 : (int)std::floor( std::log10( counter ) );
          args[args.size() - 1] = std::make_pair( gate.output_name, ys[po_list[i]] );

          writer.on_module_instantiation( name.append( std::string( length - name.length(), ' ' ) ),
                                          {},
                                          std::string( "g" ) + std::string( nDigits - digits, '0' ) + std::to_string( counter ),
                                          args );
          ++counter;
        }
      }
    }
    else if ( !ntk.is_constant( n ) && !ntk.is_pi( n ) )
    {
      std::cerr << "[e] internal node " << n << " is not mapped.\n";
    }

    return true;
  } );

  writer.on_module_end();
}

/*! \brief Writes mapped network in structural Verilog format into output stream
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `node_to_index`
 * - `has_cell`
 * - `get_cell_index`
 *
 * \param ntk Mapped network
 * \param os Output stream
 * \param ps Verilog parameters
 */
template<class Ntk>
void write_verilog_with_cell( Ntk const& ntk, std::ostream& os, write_verilog_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_has_cell_v<Ntk>, "Ntk does not implement the has_cell method" );
  static_assert( has_get_cell_index_v<Ntk>, "Ntk does not implement the get_cell_index method" );

  assert( ntk.is_combinational() && "Network has to be combinational" );

  lorina::verilog_writer writer( os );

  std::vector<std::string> xs, inputs;
  if ( ps.input_names.empty() )
  {
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
    {
      ntk.foreach_pi( [&]( auto const& i, uint32_t index ) {
        if ( ntk.has_name( ntk.make_signal( i ) ) )
        {
          xs.emplace_back( ntk.get_name( ntk.make_signal( i ) ) );
        }
        else
        {
          xs.emplace_back( fmt::format( "x{}", index ) );
        }
      } );
    }
    else
    {
      for ( auto i = 0u; i < ntk.num_pis(); ++i )
      {
        xs.emplace_back( fmt::format( "x{}", i ) );
      }
    }
    inputs = xs;
  }
  else
  {
    uint32_t ctr{ 0u };
    for ( auto const& [name, width] : ps.input_names )
    {
      inputs.emplace_back( name );
      ctr += width;
      for ( auto i = 0u; i < width; ++i )
      {
        xs.emplace_back( fmt::format( "{}[{}]", name, i ) );
      }
    }
    if ( ctr != ntk.num_pis() )
    {
      std::cerr << "[e] input names do not partition all inputs\n";
    }
  }

  std::vector<std::string> ys, outputs;
  if ( ps.output_names.empty() )
  {
    if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
    {
      ntk.foreach_po( [&]( auto const& o, uint32_t index ) {
        if ( ntk.has_output_name( index ) )
        {
          ys.emplace_back( ntk.get_output_name( index ) );
        }
        else
        {
          ys.emplace_back( fmt::format( "y{}", index ) );
        }
      } );
    }
    else
    {
      for ( auto i = 0u; i < ntk.num_pos(); ++i )
      {
        ys.emplace_back( fmt::format( "y{}", i ) );
      }
    }
    outputs = ys;
  }
  else
  {
    uint32_t ctr{ 0u };
    for ( auto const& [name, width] : ps.output_names )
    {
      outputs.emplace_back( name );
      ctr += width;
      for ( auto i = 0u; i < width; ++i )
      {
        ys.emplace_back( fmt::format( "{}[{}]", name, i ) );
      }
    }
    if ( ctr != ntk.num_pos() )
    {
      std::cerr << "[e] output names do not partition all outputs\n";
    }
  }

  /* compute which nodes are POs and register index */
  uint32_t additional_buffers = 0;
  std::unordered_map<typename Ntk::signal, std::vector<uint32_t>, detail::verilog_writer_signal_hash<typename Ntk::signal>> po_nodes;
  ntk.foreach_po( [&]( auto const& f, auto i ) {
    po_nodes[f ^ ntk.is_complemented( f )].push_back( i );
    additional_buffers += po_nodes[f ^ ntk.is_complemented( f )].size() > 1 ? 1 : 0;
  } );

  std::vector<std::string> ws;
  node_map<std::vector<std::string>, Ntk> node_names( ntk );

  /* constants */
  if ( ntk.has_cell( ntk.get_node( ntk.get_constant( false ) ) ) )
  {
    if ( po_nodes.find( ntk.get_constant( false ) ) == po_nodes.end() )
    {
      node_names[ntk.get_node( ntk.get_constant( false ) )].push_back( fmt::format( "n{}", ntk.node_to_index( ntk.get_constant( false ) ) ) );
      ws.emplace_back( node_names[ntk.get_constant( false )].front() );
    }
  }
  else
  {
    node_names[ntk.get_node( ntk.get_constant( false ) )].push_back( "1'b0" );
  }
  if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
  {
    if ( ntk.has_cell( ntk.get_node( ntk.get_constant( true ) ) ) )
    {
      if ( po_nodes.find( ntk.get_constant( true ) ) == po_nodes.end() )
      {
        node_names[ntk.get_node( ntk.get_constant( true ) )].push_back( fmt::format( "n{}", ntk.node_to_index( ntk.get_constant( true ) ) ) );
        ws.emplace_back( node_names[ntk.get_constant( true )].front() );
      }
    }
    else
    {
      node_names[ntk.get_constant( true )].push_back( "1'b1" );
    }
  }

  /* add wires */
  ntk.foreach_gate( [&]( auto const& n ) {
    if constexpr ( has_is_multioutput_v<Ntk> )
    {
      /* create wire for each individual output */
      if ( !ntk.is_multioutput( n ) && po_nodes.find( ntk.make_signal( n ) ) == po_nodes.end() )
      {
        ws.emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
        return;
      }

      for ( uint32_t i = 0; i < ntk.num_outputs( n ); ++i )
      {
        if ( po_nodes.find( ntk.make_signal( n, i ) ) == po_nodes.end() )
        {
          ws.emplace_back( fmt::format( "n{}_{}", ntk.node_to_index( n ), i ) );
        }
      }

      return;
    }

    if ( po_nodes.find( ntk.make_signal( n ) ) == po_nodes.end() )
    {
      ws.emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
    }
  } );

  std::string module_name = "top";
  if ( ps.module_name )
  {
    module_name = *ps.module_name;
  }
  else
  {
    if constexpr ( has_get_network_name_v<Ntk> )
    {
      if ( ntk.get_network_name().length() > 0 )
      {
        module_name = ntk.get_network_name();
      }
    }
  }
  writer.on_module_begin( module_name, inputs, outputs );
  if ( ps.input_names.empty() )
  {
    writer.on_input( xs );
  }
  else
  {
    for ( auto const& [name, width] : ps.input_names )
    {
      writer.on_input( width, name );
    }
  }
  if ( ps.output_names.empty() )
  {
    writer.on_output( ys );
  }
  else
  {
    for ( auto const& [name, width] : ps.output_names )
    {
      writer.on_output( width, name );
    }
  }
  if ( !ws.empty() )
  {
    writer.on_wire( ws );
  }

  ntk.foreach_pi( [&]( auto const& n, auto i ) {
    node_names[n].push_back( xs[i] );
  } );

  auto const& cells = ntk.get_library();

  /* get buffer */
  uint32_t buf_id = UINT32_MAX;
  double buf_area = std::numeric_limits<double>::max();
  for ( uint32_t i = 0; i < cells.size(); ++i )
  {
    auto const& g = cells[i].gates.front();
    if ( cells[i].gates.size() > 1 || g.num_vars != 1 )
      continue;
    if ( g.function._bits[0] != 0x2 )
      continue;

    if ( buf_id == UINT32_MAX || g.area < buf_area )
    {
      buf_id = i;
      buf_area = g.area;
    }
  }

  int nDigits = (int)std::floor( std::log10( ntk.num_gates() + additional_buffers ) );
  unsigned int length = 0;
  unsigned counter = 0;

  for ( auto const& cell : cells )
  {
    length = std::max( length, static_cast<unsigned int>( cell.name.length() ) );
  }

  topo_view ntk_topo{ ntk };

  ntk_topo.foreach_node( [&]( auto const& n ) {
    /* load names of n */
    if constexpr ( has_is_multioutput_v<Ntk> )
    {
      /* create wire for each individual output */
      if ( !ntk.is_multioutput( n ) )
      {
        if ( auto el = po_nodes.find( ntk.make_signal( n ) ); el != po_nodes.end() )
        {
          node_names[n].emplace_back( ys[el->second.front()] );
        }
        else if ( !ntk.is_constant( n ) && !ntk.is_pi( n ) )
        {
          node_names[n].emplace_back( fmt::format( "n{}", ntk.node_to_index( n ) ) );
        }
      }
      else
      {
        for ( uint32_t i = 0; i < ntk.num_outputs( n ); ++i )
        {
          if ( auto el = po_nodes.find( ntk.make_signal( n, i ) ); el != po_nodes.end() )
          {
            node_names[n].emplace_back( ys[el->second.front()] );
          }
          else
          {
            node_names[n].emplace_back( fmt::format( "n{}_{}", ntk.node_to_index( n ), i ) );
          }
        }
      }
    }
    else
    {
      if ( auto el = po_nodes.find( ntk.make_signal( n ) ); el != po_nodes.end() )
      {
        node_names[n] = ys[el->second.front()];
      }
      else if ( !ntk.is_constant( n ) && !ntk.is_pi( n ) )
      {
        node_names[n] = fmt::format( "n{}", ntk.node_to_index( n ) );
      }
    }

    if ( ntk.has_cell( n ) )
    {
      auto const& cell = cells[ntk.get_cell_index( n )];
      std::string name = cell.name;

      int digits = counter == 0 ? 0 : (int)std::floor( std::log10( counter ) );
      auto fanin_names = detail::format_fanin<Ntk>( ntk, n, node_names );
      std::vector<std::pair<std::string, std::string>> args;

      auto i = 0;
      for ( auto pair : fanin_names )
      {
        args.emplace_back( std::make_pair( cell.gates[0].pins[i++].name, pair.second ) );
      }

      assert( cell.gates.size() == node_names[n].size() );

      i = 0;
      for ( auto const& ogate : cell.gates )
      {
        args.emplace_back( std::make_pair( ogate.output_name, node_names[n][i++] ) );
      }

      writer.on_module_instantiation( name.append( std::string( length - name.length(), ' ' ) ),
                                      {},
                                      std::string( "g" ) + std::string( nDigits - digits, '0' ) + std::to_string( counter ),
                                      args );
      ++counter;

      /* if node drives multiple POs, buffer */
      if constexpr ( has_is_multioutput_v<Ntk> )
      {
        i = 0;
        for ( i = 0; i < ntk.num_outputs( n ); ++i )
        {
          if ( auto el = po_nodes.find( ntk.make_signal( n, i ) ); el != po_nodes.end() && el->second.size() > 1 )
          {
            if ( buf_id == UINT32_MAX )
            {
              std::cerr << "[e] Error: cell library does not contain a buffer cell\n";
              return false;
            }

            if ( ps.verbose )
            {
              std::cerr << "[i] Buffering node " << n << " driving multiple POs.\n";
            }

            gate const& g = cells[buf_id].gates.front();
            std::string buf_name = g.name;
            auto const& po_list = el->second;
            args.clear();
            args.emplace_back( std::make_pair( g.pins.front().name, node_names[n].at( i ) ) );
            args.emplace_back( std::make_pair( "", "" ) );
            for ( uint32_t j = 1u; j < po_list.size(); ++j )
            {
              digits = counter == (int)std::floor( std::log10( counter ) );
              args[args.size() - 1] = std::make_pair( g.output_name, ys[po_list[j]] );

              writer.on_module_instantiation( buf_name.append( std::string( length - buf_name.length(), ' ' ) ),
                                              {},
                                              std::string( "g" ) + std::string( nDigits - digits, '0' ) + std::to_string( counter ),
                                              args );
              ++counter;
            }
          }
        }
      }
      else
      {
        if ( auto el = po_nodes.find( ntk.make_signal( n ) ); el != po_nodes.end() && el->second.size() > 1 )
        {
          if ( buf_id == UINT32_MAX )
          {
            std::cerr << "[e] Error: cell library does not contain a buffer cell\n";
            return false;
          }

          std::cerr << "[i] Buffering node " << n << " driving multiple POs.\n";

          gate const& g = cells[buf_id].gates.front();
          std::string buf_name = g.name;
          auto const& po_list = el->second;
          args.clear();
          args.emplace_back( std::make_pair( g.pins.front().name, node_names[n].front() ) );
          args.emplace_back( std::make_pair( "", "" ) );
          for ( i = 1u; i < po_list.size(); ++i )
          {
            digits = counter == (int)std::floor( std::log10( counter ) );
            args[args.size() - 1] = std::make_pair( g.output_name, ys[po_list[i]] );

            writer.on_module_instantiation( buf_name.append( std::string( length - buf_name.length(), ' ' ) ),
                                            {},
                                            std::string( "g" ) + std::string( nDigits - digits, '0' ) + std::to_string( counter ),
                                            args );
            ++counter;
          }
        }
      }
    }
    else if ( !ntk.is_constant( n ) && !ntk.is_pi( n ) )
    {
      std::cerr << "[e] internal node " << n << " is not mapped.\n";
    }

    return true;
  } );

  writer.on_module_end();
}

/*! \brief Writes network in structural Verilog format into a file
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `is_and`
 * - `is_or`
 * - `is_xor`
 * - `is_xor3`
 * - `is_maj`
 * - `node_to_index`
 *
 * \param ntk Network
 * \param filename Filename
 */
template<class Ntk>
void write_verilog( Ntk const& ntk, std::string const& filename, write_verilog_params const& ps = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_verilog( ntk, os, ps );
  os.close();
}

/*! \brief Writes mapped network in structural Verilog format into a file
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `node_to_index`
 * - `has_binding`
 * - `get_binding_index`
 *
 * \param ntk Network (binding_view)
 * \param filename Filename
 */
template<class Ntk>
void write_verilog_with_binding( Ntk const& ntk, std::string const& filename, write_verilog_params const& ps = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_verilog_with_binding( ntk, os, ps );
  os.close();
}

/*! \brief Writes mapped network in structural Verilog format into a file
 *
 * **Required network functions:**
 * - `num_pis`
 * - `num_pos`
 * - `foreach_pi`
 * - `foreach_node`
 * - `foreach_fanin`
 * - `get_node`
 * - `get_constant`
 * - `is_constant`
 * - `is_pi`
 * - `node_to_index`
 * - `has_cell`
 * - `get_cell_index`
 *
 * \param ntk Network (cell_view)
 * \param filename Filename
 */
template<class Ntk>
void write_verilog_with_cell( Ntk const& ntk, std::string const& filename, write_verilog_params const& ps = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_verilog_with_cell( ntk, os, ps );
  os.close();
}

} /* namespace mockturtle */