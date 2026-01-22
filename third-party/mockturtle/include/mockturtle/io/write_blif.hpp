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
  \file write_blif.hpp
  \brief Write networks to BLIF format

  \author Heinz Riener
  \author Mathias Soeken
  \author Max Austin
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/sequential.hpp"
#include "../traits.hpp"
#include "../views/topo_view.hpp"

#include <kitty/constructors.hpp>
#include <kitty/isop.hpp>
#include <kitty/operations.hpp>
#include <kitty/print.hpp>

#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <string>

namespace mockturtle
{

struct write_blif_params
{
  /**
    * ## `ps.rename_ri_using_node`
    * 
    * Rename registers using node name if `rename_ri_using_node` is set to 1 ( default: 0 )
    * 
    * A register is represented by a `ro_to_ri` mapping:
    * 
    * ```
    *  ri_node --> ro_signal
    * ```
    * where `ri_node` is the register input (a combinational output node), and `ro_signal` is 
    * the register output (a combinational input signal).
    * 
    * If `rename_ri_using_node` is set to 1, then `ri_node` will be renamed (from its default 
    * name) using the node name. We write the following to the BLIF file:
    * 
    * ```
    * .latch ri_node ro_signal
    * ```
    * 
    * Otherwise, if `rename_ri_using_node` is set to 1, `ri_node` will be named by its default 
    * name, `li_<idx>`, where `idx` is the index of the register (based on the definition order 
    * when calling `create_ro`). Then we write:
    * 
    * ```
    * .latch li_<idx> ro_signal
    * .names ri_node li_<idx>
    * 1 1
    * ```
   */
  uint32_t rename_ri_using_node = 0u;
};

/*! \brief Writes network in BLIF format into output stream
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * **Required network functions:**
 * - `fanin_size`
 * - `foreach_fanin`
 * - `foreach_pi`
 * - `foreach_po`
 * - `get_node`
 * - `is_constant`
 * - `is_pi`
 * - `node_function`
 * - `node_to_index`
 * - `num_pis`
 * - `num_pos`
 *
 * \param ntk Network
 * \param os Output stream
 */
template<class Ntk>
void write_blif( Ntk const& ntk, std::ostream& os, write_blif_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_fanin_size_v<Ntk>, "Ntk does not implement the fanin_size method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

  uint32_t num_latches{ 0 };
  if constexpr ( has_num_registers_v<Ntk> )
  {
    num_latches = ntk.num_registers();
  }

  topo_view topo_ntk{ ntk };
  std::unordered_set<std::string> defined_names;

  /* write model */
  os << ".model top\n";

  /* write inputs */
  if ( topo_ntk.num_pis() > 0u )
  {
    os << ".inputs ";
    topo_ntk.foreach_ci( [&]( auto const& n, auto index ) {
      if ( ( ( index + 1 ) <= topo_ntk.num_cis() - num_latches ) )
      {
        if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
        {
          signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.node_to_index( n ) );
          std::string const input_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "pi{}", topo_ntk.get_node( s ) );
          os << input_name << ' ';
          defined_names.insert( input_name ); /* we should not have collision here */
        }
        else
        {
          std::string const input_name = fmt::format( "pi{}", topo_ntk.node_to_index( n ) );
          os << input_name << ' ';
          defined_names.insert( input_name ); /* we should not have collision here */
        }
      }
    } );
    os << "\n";
  }

  /* write outputs */
  if ( topo_ntk.num_pos() > 0u )
  {
    os << ".outputs ";
    topo_ntk.foreach_co( [&]( auto const& f, auto index ) {
      (void)f;
      if ( index < topo_ntk.num_cos() - num_latches )
      {
        if constexpr ( has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> )
        {
          std::string const output_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : fmt::format( "po{}", index );
          os << output_name << ' ';
        }
        else
        {
          std::string const output_name = fmt::format( "po{}", index );
          os << output_name << ' ';
        }
      }
    } );
    os << "\n";
  }

  if constexpr ( has_num_registers_v<Ntk> )
  {
    if ( num_latches > 0u )
    {
      uint32_t latch_idx = 0;
      topo_ntk.foreach_co( [&]( auto const& f, auto index ) {
        if ( index >= topo_ntk.num_cos() - num_latches )
        {
          os << ".latch ";
          auto const ro_signal = topo_ntk.make_signal( topo_ntk.ro_at( latch_idx ) );
          auto const ri_signal = topo_ntk.ri_at( latch_idx );
          register_t latch_info = topo_ntk.register_at( latch_idx );
          if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
          {
            std::string const node_name = topo_ntk.has_name( ri_signal ) ? 
                topo_ntk.get_name( ri_signal ) 
              : fmt::format( "new_n{}", topo_ntk.get_node( ri_signal ) );
            std::string const latch_name = ps.rename_ri_using_node ? 
                node_name
              : fmt::format( "li{}", latch_idx );
            std::string const ri_name = topo_ntk.has_output_name( index ) ? 
                topo_ntk.get_output_name( index ) 
              : latch_name;
            std::string const ro_name = topo_ntk.has_name( ro_signal ) ? topo_ntk.get_name( ro_signal ) : fmt::format( "new_n{}", topo_ntk.get_node( ro_signal ) );
            os << fmt::format( "{} {} {} {} {}\n", ri_name, ro_name, latch_info.type, latch_info.control, latch_info.init );
            defined_names.insert( ro_name ); /* we should not have collision here */
          }
          else
          {
            std::string const ri_name = ps.rename_ri_using_node? 
                fmt::format( "new_n{}", topo_ntk.get_node( topo_ntk.ri_at( latch_idx ) ) )
              : fmt::format( "li{}", latch_idx );
            std::string const ro_name = fmt::format( "new_n{}", topo_ntk.get_node( ro_signal ) );
            os << fmt::format( "{} {} {} {} {}\n", ri_name, ro_name, latch_info.type, latch_info.control, latch_info.init );
            defined_names.insert( ro_name ); /* we should not have collision here */
          }
          latch_idx++;
        }
      } );
    }
  }

  /* write constants */
  os << ".names new_n0\n";
  os << "0\n";
  defined_names.insert( "new_n0" ); /* we should not have collision here */

  if ( topo_ntk.get_constant( false ) != topo_ntk.get_constant( true ) )
  {
    os << ".names new_n1\n";
    os << "1\n";
    defined_names.insert( "new_n1" ); /* we should not have collision here */
  }

  /* write nodes */
  topo_ntk.foreach_node( [&]( auto const& n ) {
    if ( topo_ntk.is_constant( n ) || topo_ntk.is_ci( n ) )
      return; /* continue */

    /* write truth table of node */
    auto func = topo_ntk.node_function( n );

    if ( isop( func ).size() == 0 ) /* constants */
    {
      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
      {
        auto const s = topo_ntk.make_signal( n );
        std::string const constant_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "new_n{}", topo_ntk.get_node( s ) );
        os << fmt::format( ".names {}\n", constant_name );
        os << "0" << '\n';
        defined_names.insert( constant_name ); /* we should not have collision here */
      }
      else
      {
        std::string const constant_name = fmt::format( "new_n{}", n );
        os << fmt::format( ".names {}\n", constant_name );
        os << "0" << '\n';
        defined_names.insert( constant_name ); /* we should not have collision here */
      }
      return;
    }

    os << fmt::format( ".names " );

    /* write fanins of node */
    topo_ntk.foreach_fanin( n, [&]( auto const& f ) {
      auto f_node = topo_ntk.get_node( f );
      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
      {
        signal<Ntk> const s = topo_ntk.make_signal( f_node );
        std::string const fanin_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : topo_ntk.is_pi( f_node ) ? fmt::format( "pi{}", f_node ) : fmt::format( "new_n{}", f_node );
        os << fanin_name << ' ';
      }
      else
      {
        std::string const fanin_name = topo_ntk.is_pi( f_node ) ? fmt::format( "pi{} ", f_node ) : fmt::format( "new_n{} ", f_node );
        os << fanin_name;
      }
    } );

    /* write fanout of node */
    if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> )
    {
      auto const s = topo_ntk.make_signal( n );
      std::string const fanout_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : fmt::format( "new_n{}", topo_ntk.get_node( s ) );
      os << fanout_name << '\n';
      defined_names.insert( fanout_name ); /* we should not have collision here */
    }
    else
    {
      std::string const fanout_name = fmt::format( "new_n{}", n );
      os << fanout_name << '\n';
      defined_names.insert( fanout_name ); /* we should not have collision here */
    }

    int count = 0;
    for ( auto cube : isop( func ) )
    {
      topo_ntk.foreach_fanin( n, [&]( auto const& f, auto index ) {
        if ( cube.get_mask( index ) && topo_ntk.is_complemented( f ) )
          cube.flip_bit( index );
      } );

      cube.print( topo_ntk.fanin_size( n ), os );
      os << " 1\n";
      count++;
    }
  } );

  auto latch_idx = 0;
  topo_ntk.foreach_co( [&]( auto const& f, auto index ) {
    auto f_node = topo_ntk.get_node( f );
    auto const minterm_string = topo_ntk.is_complemented( f ) ? "0" : "1";

    if ( index < topo_ntk.num_cos() - num_latches ) /* the signal f is a PO */
    {
      /* the default name depends on whether the signal is a PI or a regular signal */
      std::string const node_default_name = topo_ntk.is_pi( f_node ) ? fmt::format( "pi{}", f_node ) : fmt::format( "new_n{}", f_node );

      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> && has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> ) /* with name view */
      {
        signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.get_node( f ) );

        /* then we overwrite the default name if we assigned names from name_view */
        std::string const node_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : node_default_name;

        /* get the default name of PO */
        std::string default_output_name = fmt::format( "po{}", index );

        /* over write the name if we have name view */
        std::string const output_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : default_output_name;

        /* we need to bridge the nodes */
        if ( node_name != output_name && defined_names.find( output_name ) == defined_names.end() )
        {
          os << fmt::format( ".names {} {}\n{} 1\n", node_name, output_name, minterm_string );
          defined_names.insert( output_name );
        }
      }
      else /* without name view */
      {
        std::string const node_name = node_default_name;
        std::string const output_name = fmt::format( "po{}", index );
        if ( node_name != output_name && defined_names.find( output_name ) == defined_names.end() )
        {
          os << fmt::format( ".names {} {}\n{} 1\n", node_name, output_name, minterm_string );
          defined_names.insert( output_name );
        }
      }
    }
    else /* the signal f is a RI */
    {
      /* the default name depends on whether the signal is a PI or a regular signal */
      std::string const node_default_name = topo_ntk.is_pi( f_node ) ? fmt::format( "pi{}", f_node ) : fmt::format( "new_n{}", f_node );

      if constexpr ( has_has_name_v<Ntk> && has_get_name_v<Ntk> && has_has_output_name_v<Ntk> && has_get_output_name_v<Ntk> ) /* with name view */
      {
        signal<Ntk> const s = topo_ntk.make_signal( topo_ntk.get_node( f ) );

        /* then we overwrite the default name if we assigned names from name_view */
        std::string const node_name = topo_ntk.has_name( s ) ? topo_ntk.get_name( s ) : node_default_name;

        /* get the default name of RI */
        std::string default_ri_name = ps.rename_ri_using_node ? node_name : fmt::format( "li{}", latch_idx );

        /* overwrite the name if we have name view */
        std::string const ri_name = topo_ntk.has_output_name( index ) ? topo_ntk.get_output_name( index ) : default_ri_name;

        /* we need to bridge the nodes */
        if ( node_name != ri_name && defined_names.find( ri_name ) == defined_names.end() )
        {
          os << fmt::format( ".names {} {}\n{} 1\n", node_name, ri_name, minterm_string );
          defined_names.insert( ri_name );
        }
      }
      else /* without name view */
      {
        std::string const node_name = node_default_name;

        /* get the default name of RI */
        std::string ri_name = ps.rename_ri_using_node ? node_name : fmt::format( "li{}", latch_idx );

        if ( node_name != ri_name && defined_names.find( ri_name ) == defined_names.end() )
        {
          os << fmt::format( ".names {} {}\n{} 1\n", node_name, ri_name, minterm_string );
          defined_names.insert( ri_name );
        }
      }

      latch_idx++;
    }
  } );

  os << ".end\n";
  os << std::flush;
}

/*! \brief Writes network in BLIF format into a file
 *
 * **Required network functions:**
 * - `fanin_size`
 * - `foreach_fanin`
 * - `foreach_pi`
 * - `foreach_po`
 * - `get_node`
 * - `is_constant`
 * - `is_pi`
 * - `node_function`
 * - `node_to_index`
 * - `num_pis`
 * - `num_pos`
 *
 * \param ntk Network
 * \param filename Filename
 */
template<class Ntk>
void write_blif( Ntk const& ntk, std::string const& filename, write_blif_params const& ps = {} )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_blif( ntk, os, ps );
  os.close();
}

} /* namespace mockturtle */