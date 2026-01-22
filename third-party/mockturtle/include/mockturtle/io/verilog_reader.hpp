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
  \file verilog_reader.hpp
  \brief Lorina reader for VERILOG files

  \author Heinz Riener
  \author Marcel Walter
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <lorina/verilog.hpp>

#include "../generators/arithmetic.hpp"
#include "../generators/modular_arithmetic.hpp"
#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Lorina reader callback for VERILOG files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `get_constant`
 * - `create_not`
 * - `create_and`
 * - `create_or`
 * - `create_xor`
 * - `create_maj`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      mig_network mig;
      lorina::read_verilog( "file.v", verilog_reader( mig ) );
   \endverbatim
 */
template<typename Ntk>
class verilog_reader : public lorina::verilog_reader
{
public:
  explicit verilog_reader( Ntk& ntk, std::string const& top_module_name = "top" ) : ntk_( ntk ), top_module_name_( top_module_name )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and function" );
    static_assert( has_create_or_v<Ntk>, "Ntk does not implement the create_or function" );
    static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor function" );
    static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite function" );
    static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj function" );

    signals_["0"] = ntk_.get_constant( false );
    signals_["1"] = ntk_.get_constant( true );
    signals_["1'b0"] = ntk_.get_constant( false );
    signals_["1'b1"] = ntk_.get_constant( true );
  }

  void on_module_header( const std::string& module_name, const std::vector<std::string>& inouts ) const override
  {
    (void)inouts;
    if constexpr ( has_set_network_name_v<Ntk> )
    {
      ntk_.set_network_name( module_name );
    }

    name_ = module_name;
  }

  void on_inputs( const std::vector<std::string>& names, std::string const& size = "" ) const override
  {
    (void)size;
    if ( name_ != top_module_name_ )
      return;

    for ( const auto& name : names )
    {
      if ( size.empty() )
      {
        signals_[name] = ntk_.create_pi();
        input_names_.emplace_back( name, 1u );
        if constexpr ( has_set_name_v<Ntk> )
        {
          ntk_.set_name( signals_[name], name );
        }
      }
      else
      {
        std::vector<signal<Ntk>> word;
        const auto length = parse_size( size );
        for ( auto i = 0u; i < length; ++i )
        {
          const auto sname = fmt::format( "{}[{}]", name, i );
          word.push_back( ntk_.create_pi() );
          signals_[sname] = word.back();
          if constexpr ( has_set_name_v<Ntk> )
          {
            ntk_.set_name( signals_[sname], sname );
          }
        }
        registers_[name] = word;
        input_names_.emplace_back( name, length );
      }
    }
  }

  void on_outputs( const std::vector<std::string>& names, std::string const& size = "" ) const override
  {
    (void)size;
    if ( name_ != top_module_name_ )
      return;

    for ( const auto& name : names )
    {
      if ( size.empty() )
      {
        outputs_.emplace_back( name );
        output_names_.emplace_back( name, 1u );
      }
      else
      {
        const auto length = parse_size( size );
        for ( auto i = 0u; i < length; ++i )
        {
          outputs_.emplace_back( fmt::format( "{}[{}]", name, i ) );
        }
        output_names_.emplace_back( name, length );
      }
    }
  }

  void on_assign( const std::string& lhs, const std::pair<std::string, bool>& rhs ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if ( signals_.find( rhs.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", rhs.first );

    auto r = signals_[rhs.first];
    signals_[lhs] = rhs.second ? ntk_.create_not( r ) : r;
  }

  void on_nand( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    signals_[lhs] = ntk_.create_nand( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b );
  }

  void on_and( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      if ( !op1.second && !op2.second )
        signals_[lhs] = ntk_.create_and( a, b );
      else if ( !op1.second && op2.second )
        signals_[lhs] = ntk_.create_gt( a, b ); // a & !b
      else if ( op1.second && !op2.second )
        signals_[lhs] = ntk_.create_lt( a, b ); // !a & b
      else
        signals_[lhs] = ntk_.create_nor( a, b ); // !a & !b = !(a | b)
    }
    else
    {
      signals_[lhs] = ntk_.create_and( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b );
    }
  }

  void on_or( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      if ( !op1.second && !op2.second )
        signals_[lhs] = ntk_.create_or( a, b );
      else if ( !op1.second && op2.second )
        signals_[lhs] = ntk_.create_ge( a, b ); // a | !b
      else if ( op1.second && !op2.second )
        signals_[lhs] = ntk_.create_le( a, b ); // !a | b
      else
        signals_[lhs] = ntk_.create_nand( a, b ); // !a | !b = !(a & b)
    }
    else
    {
      signals_[lhs] = ntk_.create_or( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b );
    }
  }

  void on_xor( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      if ( !op1.second == !op2.second )
        signals_[lhs] = ntk_.create_xor( a, b );
      else
        signals_[lhs] = ntk_.create_xnor( a, b );
    }
    else
    {
      signals_[lhs] = ntk_.create_xor( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b );
    }
  }

  void on_xor3( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2, const std::pair<std::string, bool>& op3 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      assert( false && "3-input gates in crossed_network are not supported (to be implemented)" );
    }

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );
    if ( signals_.find( op3.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op3.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    auto c = signals_[op3.first];

    if constexpr ( has_create_xor3_v<Ntk> )
    {
      signals_[lhs] = ntk_.create_xor3( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b, op3.second ? ntk_.create_not( c ) : c );
    }
    else
    {
      signals_[lhs] = ntk_.create_xor( ntk_.create_xor( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b ), op3.second ? ntk_.create_not( c ) : c );
    }
  }

  void on_maj3( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2, const std::pair<std::string, bool>& op3 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      assert( false && "3-input gates in crossed_network are not supported (to be implemented)" );
    }

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );
    if ( signals_.find( op3.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op3.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    auto c = signals_[op3.first];
    signals_[lhs] = ntk_.create_maj( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b, op3.second ? ntk_.create_not( c ) : c );
  }

  void on_mux21( const std::string& lhs, const std::pair<std::string, bool>& op1, const std::pair<std::string, bool>& op2, const std::pair<std::string, bool>& op3 ) const override
  {
    if ( name_ != top_module_name_ )
      return;

    if constexpr ( is_crossed_network_type_v<Ntk> )
    {
      assert( false && "3-input gates in crossed_network are not supported (to be implemented)" );
    }

    if ( signals_.find( op1.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op1.first );
    if ( signals_.find( op2.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op2.first );
    if ( signals_.find( op3.first ) == signals_.end() )
      fmt::print( stderr, "[w] undefined signal {} assigned 0\n", op3.first );

    auto a = signals_[op1.first];
    auto b = signals_[op2.first];
    auto c = signals_[op3.first];
    signals_[lhs] = ntk_.create_ite( op1.second ? ntk_.create_not( a ) : a, op2.second ? ntk_.create_not( b ) : b, op3.second ? ntk_.create_not( c ) : c );
  }

  void on_module_instantiation( std::string const& module_name, std::vector<std::string> const& params, std::string const& inst_name,
                                std::vector<std::pair<std::string, std::string>> const& args ) const override
  {
    (void)inst_name;
    if ( name_ != top_module_name_ )
      return;

    /* check routines */
    const auto num_args_equals = [&]( uint32_t expected_count ) {
      if ( args.size() != expected_count )
      {
        fmt::print( stderr, "[e] {} module expects {} arguments\n", module_name, expected_count );
        return false;
      }
      return true;
    };

    const auto num_params_equals = [&]( uint32_t expected_count ) {
      if ( params.size() != expected_count )
      {
        fmt::print( stderr, "[e] {} module expects {} parameters\n", module_name, expected_count );
        return false;
      }
      return true;
    };

    const auto register_exists = [&]( std::string const& name ) {
      if ( registers_.find( name ) == registers_.end() )
      {
        fmt::print( stderr, "[e] register {} does not exist\n", name );
        return false;
      }
      return true;
    };

    const auto register_has_size = [&]( std::string const& name, uint32_t size ) {
      if ( !register_exists( name ) || registers_[name].size() != size )
      {
        fmt::print( stderr, "[e] register {} must have size {}\n", name, size );
        return false;
      }
      return true;
    };

    const auto add_register = [&]( std::string const& name, std::vector<signal<Ntk>> const& fs ) {
      for ( auto i = 0u; i < fs.size(); ++i )
      {
        signals_[fmt::format( "{}[{}]", name, i )] = fs[i];
      }
      registers_[name] = fs;
    };

    if ( module_name == "ripple_carry_adder" )
    {
      if ( !num_args_equals( 3u ) )
        return;
      if ( !num_params_equals( 1u ) )
        return;
      const auto bitwidth = static_cast<uint32_t>( parse_small_value( params[0u] ) );
      if ( !register_has_size( args[0].second, bitwidth ) )
        return;
      if ( !register_has_size( args[1].second, bitwidth ) )
        return;

      auto a_copy = registers_[args[0].second];
      const auto& b = registers_[args[1].second];
      auto carry = ntk_.get_constant( false );
      carry_ripple_adder_inplace( ntk_, a_copy, b, carry );
      a_copy.push_back( carry );
      add_register( args[2].second, a_copy );
    }
    else if ( module_name == "montgomery_multiplier" )
    {
      if ( !num_args_equals( 3u ) )
        return;
      if ( !num_params_equals( 3u ) )
        return;
      const auto bitwidth = static_cast<uint32_t>( parse_small_value( params[0u] ) );
      if ( !register_has_size( args[0].second, bitwidth ) )
        return;
      if ( !register_has_size( args[1].second, bitwidth ) )
        return;

      auto N = parse_value( params[1u] );
      auto NN = parse_value( params[2u] );

      N.resize( bitwidth );
      NN.resize( bitwidth );

      add_register( args[2].second, montgomery_multiplication( ntk_, registers_[args[0].second], registers_[args[1].second], N, NN ) );
    }
    else if ( module_name == "buffer" || module_name == "inverter" )
    {
      if constexpr ( is_buffered_network_type_v<Ntk> )
      {
        static_assert( has_create_buf_v<Ntk>, "Ntk does not implement the create_buf method" );
        if ( !num_args_equals( 2u ) )
          fmt::print( stderr, "[e] number of arguments of a `{}` instance is not 2\n", module_name );

        signal<Ntk> fi = ntk_.get_constant( false );
        std::string lhs;
        for ( auto const& arg : args )
        {
          if ( arg.first == ".i" )
          {
            if ( signals_.find( arg.second ) == signals_.end() )
              fmt::print( stderr, "[w] undefined signal {} assigned 0\n", arg.second );
            else
              fi = signals_[arg.second];
          }
          else if ( arg.first == ".o" )
            lhs = arg.second;
          else
            fmt::print( stderr, "[e] unknown argument {} to a `{}` instance\n", arg.first, module_name );
        }
        signals_[lhs] = ntk_.create_buf( fi );
        if ( module_name == "inverter" )
          ntk_.invert( ntk_.get_node( signals_[lhs] ) );
      }
    }
    else if ( module_name == "crossing" )
    {
      if constexpr ( is_crossed_network_type_v<Ntk> )
      {
        if ( !num_args_equals( 4u ) )
          fmt::print( stderr, "[e] number of arguments of a `{}` instance is not 4\n", module_name );

        signal<Ntk> fi1 = ntk_.get_constant( false );
        signal<Ntk> fi2 = ntk_.get_constant( false );
        std::string fo1, fo2;
        for ( auto const& arg : args )
        {
          if ( arg.first == ".i1" )
          {
            if ( signals_.find( arg.second ) == signals_.end() )
              fmt::print( stderr, "[w] undefined signal {} assigned 0\n", arg.second );
            else
              fi1 = signals_[arg.second];
          }
          else if ( arg.first == ".i2" )
          {
            if ( signals_.find( arg.second ) == signals_.end() )
              fmt::print( stderr, "[w] undefined signal {} assigned 0\n", arg.second );
            else
              fi2 = signals_[arg.second];
          }
          else if ( arg.first == ".o1" )
            fo1 = arg.second;
          else if ( arg.first == ".o2" )
            fo2 = arg.second;
          else
            fmt::print( stderr, "[e] unknown argument {} to a `{}` instance\n", arg.first, module_name );
        }

        if ( signals_.find( fo1 ) == signals_.end() ) // due to lorina bug, each crossing will be instantiated twice...
        {
          auto p = ntk_.create_crossing( fi1, fi2 );
          signals_[fo1] = p.first;
          signals_[fo2] = p.second;
        }
      }
    }
    else
    {
      fmt::print( stderr, "[e] unknown module name {}\n", module_name );
    }
  }

  void on_endmodule() const override
  {
    if ( name_ != top_module_name_ )
      return;

    for ( auto const& o : outputs_ )
    {
      ntk_.create_po( signals_[o] );
    }

    if constexpr ( has_set_output_name_v<Ntk> )
    {
      uint32_t ctr{ 0u };
      for ( auto const& output_name : output_names_ )
      {
        if ( output_name.second == 1u )
        {
          ntk_.set_output_name( ctr++, output_name.first );
        }
        else
        {
          for ( auto i = 0u; i < output_name.second; ++i )
          {
            ntk_.set_output_name( ctr++, fmt::format( "{}[{}]", output_name.first, i ) );
          }
        }
      }
      assert( ctr == ntk_.num_pos() );
    }
  }

  const std::string& name() const
  {
    return name_;
  }

  const std::vector<std::pair<std::string, uint32_t>> input_names()
  {
    return input_names_;
  }

  const std::vector<std::pair<std::string, uint32_t>> output_names()
  {
    return output_names_;
  }

private:
  std::vector<bool> parse_value( const std::string& value ) const
  {
    std::smatch match;

    if ( std::all_of( value.begin(), value.end(), isdigit ) )
    {
      std::vector<bool> res( 64u );
      bool_vector_from_dec( res, static_cast<uint64_t>( std::stoul( value ) ) );
      return res;
    }
    else if ( std::regex_match( value, match, hex_string ) )
    {
      std::vector<bool> res( static_cast<uint64_t>( std::stoul( match.str( 1 ) ) ) );
      bool_vector_from_hex( res, match.str( 2 ) );
      return res;
    }
    else
    {
      fmt::print( stderr, "[e] cannot parse number '{}'\n", value );
    }
    assert( false );
    return {};
  }

  uint64_t parse_small_value( const std::string& value ) const
  {
    return bool_vector_to_long( parse_value( value ) );
  }

  uint32_t parse_size( const std::string& size ) const
  {
    if ( size.empty() )
    {
      return 1u;
    }

    if ( auto const l = size.size(); l > 2 && size[l - 2] == ':' && size[l - 1] == '0' )
    {
      return static_cast<uint32_t>( parse_small_value( size.substr( 0u, l - 2 ) ) + 1u );
    }

    assert( false );
    return 0u;
  }

private:
  Ntk& ntk_;

  std::string const top_module_name_;

  mutable std::map<std::string, signal<Ntk>> signals_;
  mutable std::map<std::string, std::vector<signal<Ntk>>> registers_;
  mutable std::vector<std::string> outputs_;
  mutable std::string name_;
  mutable std::vector<std::pair<std::string, uint32_t>> input_names_;
  mutable std::vector<std::pair<std::string, uint32_t>> output_names_;

  std::regex hex_string{ "(\\d+)'h([0-9a-fA-F]+)" };
};

} /* namespace mockturtle */