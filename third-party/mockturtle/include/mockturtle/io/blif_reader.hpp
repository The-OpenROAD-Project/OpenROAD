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
  \file blif_reader.hpp
  \brief Lorina reader for BLIF files

  \author Andrea Costamagna
  \author Heinz Riener
  \author Marcel Walter
  \author Mathias Soeken
  \author Max Austin
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/aig.hpp"
#include "../networks/cover.hpp"
#include "../networks/sequential.hpp"
#include "../traits.hpp"

#include <kitty/kitty.hpp>
#include <lorina/blif.hpp>

#include <map>
#include <string>
#include <vector>

namespace mockturtle
{

/*! \brief Lorina reader callback for BLIF files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `create_node` or `create_cover_node`
 * - `get_constant`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      klut_network klut;
      lorina::read_blif( "file.blif", blif_reader( klut ) );

      cover_network cover;
      lorina::read_blif( "file.blif", blif_reader( cover ) );
   \endverbatim
 */
template<typename Ntk>
class blif_reader : public lorina::blif_reader
{
public:
  explicit blif_reader( Ntk& ntk )
      : ntk_( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_create_node_v<Ntk> || has_create_cover_node_v<Ntk>, "Ntk does not implement the create_node function or the create_cover_node function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
  }

  ~blif_reader()
  {
    for ( auto const& o : outputs )
    {
      ntk_.create_po( signals[o] );
    }

    if constexpr ( has_create_ri_v<Ntk> )
    {
      for ( auto const& latch : latches )
      {
        auto signal = signals[latch];
        ntk_.create_ri( signal );
      }
    }
  }

  virtual void on_model( const std::string& model_name ) const override
  {
    if constexpr ( has_set_network_name_v<Ntk> )
    {
      ntk_.set_network_name( model_name );
    }

    (void)model_name;
  }

  virtual void on_input( const std::string& name ) const override
  {
    signals[name] = ntk_.create_pi();
    if constexpr ( has_set_name_v<Ntk> )
    {
      ntk_.set_name( signals[name], name );
    }
  }

  virtual void on_output( const std::string& name ) const override
  {
    if constexpr ( has_set_output_name_v<Ntk> )
    {
      ntk_.set_output_name( outputs.size(), name );
    }
    outputs.emplace_back( name );
  }

  virtual void on_latch( const std::string& input, const std::string& output, const std::optional<latch_type>& l_type, const std::optional<std::string>& control, const std::optional<latch_init_value>& reset ) const override
  {
    if constexpr ( has_create_ro_v<Ntk> )
    {
      std::string type = "re";
      if ( l_type )
      {
        switch ( *l_type )
        {
        case latch_type::FALLING:
        {
          type = "fe";
        }
        break;
        case latch_type::RISING:
        {
          type = "re";
        }
        break;
        case latch_type::ACTIVE_HIGH:
        {
          type = "ah";
        }
        break;
        case latch_type::ACTIVE_LOW:
        {
          type = "al";
        }
        break;
        case latch_type::ASYNC:
        {
          type = "as";
        }
        break;
        default:
        {
          type = "";
        }
        break;
        }
      }

      uint32_t r = 3;
      if ( reset )
      {
        switch ( *reset )
        {
        case latch_init_value::NONDETERMINISTIC:
        {
          r = 2;
        }
        break;
        case latch_init_value::ONE:
        {
          r = 1;
        }
        break;
        case latch_init_value::ZERO:
        {
          r = 0;
        }
        break;
        default:
          break;
        }
      }

      std::string ctrl = control.has_value() ? control.value() : "clock";

      register_t reg;
      reg.control = ctrl;
      reg.init = r;
      reg.type = type;

      signals[output] = ntk_.create_ro();
      ntk_.set_register( latches.size(), reg );

      if constexpr ( has_set_name_v<Ntk> && has_set_output_name_v<Ntk> )
      {
        ntk_.set_name( signals[output], output );
        ntk_.set_output_name( outputs.size() + latches.size(), input );
      }

      latches.emplace_back( input );
    }
  }

  virtual void on_gate( const std::vector<std::string>& inputs, const std::string& output, const output_cover_t& cover ) const override
  {
    if ( inputs.size() == 0u )
    {
      if ( cover.size() == 0u )
      {
        signals[output] = ntk_.get_constant( false );
        return;
      }

      assert( cover.size() == 1u );
      assert( cover.at( 0u ).first.size() == 0u );
      assert( cover.at( 0u ).second.size() == 1u );

      auto const assigned_value = cover.at( 0u ).second.at( 0u );
      auto const const_value = ntk_.get_constant( assigned_value == '1' ? true : false );
      signals[output] = const_value;
      return;
    }

    assert( cover.size() > 0u );
    assert( cover.at( 0u ).second.size() == 1 );
    auto const first_output_value = cover.at( 0u ).second.at( 0u );

    if constexpr ( std::is_same<typename Ntk::base_type, cover_network>::value )
    {
      std::vector<kitty::cube> cubes;
      bool is_sop = ( first_output_value == '1' );
      for ( const auto& c : cover )
      {
        assert( c.second.size() == 1 );

        auto const output = c.second[0u];
        assert( output == '0' || output == '1' );
        assert( output == first_output_value );
        (void)first_output_value;

        cubes.emplace_back( kitty::cube( c.first ) );
      }

      std::vector<signal<Ntk>> input_signals;
      for ( const auto& i : inputs )
      {
        assert( signals.find( i ) != signals.end() );
        input_signals.push_back( signals.at( i ) );
      }

      if ( cubes.size() != 0 )
      {
        signals[output] = ntk_.create_cover_node( input_signals, std::make_pair( cubes, is_sop ) );
      }
    }
    else
    {

      std::vector<kitty::cube> minterms;
      std::vector<kitty::cube> maxterms;

      for ( const auto& c : cover )
      {
        assert( c.second.size() == 1 );

        auto const output = c.second[0u];
        assert( output == '0' || output == '1' );
        assert( output == first_output_value );
        (void)first_output_value;

        if ( output == '1' )
        {
          minterms.emplace_back( kitty::cube( c.first ) );
        }
        else if ( output == '0' )
        {
          maxterms.emplace_back( ~kitty::cube( c.first ) );
        }
      }

      assert( minterms.size() == 0u || maxterms.size() == 0u );

      kitty::dynamic_truth_table tt( int( inputs.size() ) );
      if ( minterms.size() != 0 )
      {
        kitty::create_from_cubes( tt, minterms, false );
      }
      else if ( maxterms.size() != 0 )
      {
        kitty::create_from_clauses( tt, maxterms, false );
      }
      std::vector<signal<Ntk>> input_signals;
      for ( const auto& i : inputs )
      {
        assert( signals.find( i ) != signals.end() );
        input_signals.push_back( signals.at( i ) );
      }
      signals[output] = ntk_.create_node( input_signals, tt );
    }
  }

  virtual void on_end() const override {}

  virtual void on_comment( const std::string& comment ) const override
  {
    (void)comment;
  }

private:
  Ntk& ntk_;

  mutable std::map<std::string, signal<Ntk>> signals;
  mutable std::vector<std::string> outputs;
  mutable std::vector<std::string> latches;
}; /* blif_reader */

} /* namespace mockturtle */