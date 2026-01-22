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
  \file aiger_reader.hpp
  \brief Lorina reader for AIGER files

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/aig.hpp"
#include "../networks/sequential.hpp"
#include "../traits.hpp"
#include <lorina/aiger.hpp>

namespace mockturtle
{

/*! \brief Lorina reader callback for Aiger files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `get_constant`
 * - `create_not`
 * - `create_and`
 *
 * **Optional network functions to support sequential networks:**
 * - `create_ri`
 * - `create_ro`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      aig_network aig;
      lorina::read_aiger( "file.aig", aiger_reader( aig ) );

      mig_network mig;
      lorina::read_aiger( "file.aig", aiger_reader( mig ) );
   \endverbatim
 */
template<typename Ntk>
class aiger_reader : public lorina::aiger_reader
{
public:
  explicit aiger_reader( Ntk& ntk ) : _ntk( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and function" );
  }

  ~aiger_reader()
  {
    uint32_t output_id{ 0 };
    for ( auto out : outputs )
    {
      auto const lit = std::get<0>( out );
      auto signal = signals[lit >> 1];
      if ( lit & 1 )
      {
        signal = _ntk.create_not( signal );
      }
      if constexpr ( has_set_output_name_v<Ntk> )
      {
        if ( !std::get<1>( out ).empty() )
        {
          _ntk.set_output_name( output_id++, std::get<1>( out ) );
        }
      }
      _ntk.create_po( signal );
    }

    if constexpr ( has_create_ri_v<Ntk> )
    {
      for ( auto i = 0u; i < latches.size(); ++i )
      {
        auto& latch = latches[i];
        auto const lit = std::get<0>( latch );
        auto const reset = std::get<1>( latch );

        auto signal = signals[lit >> 1];
        if ( lit & 1 )
        {
          signal = _ntk.create_not( signal );
        }

        if constexpr ( has_set_name_v<Ntk> )
        {
          _ntk.set_name( signal, std::get<2>( latch ) + "_next" );
        }

        _ntk.create_ri( signal );
        register_t reg;
        reg.init = reset;
        _ntk.set_register( i, reg );
      }
    }
  }

  void on_header( uint64_t, uint64_t num_inputs, uint64_t num_latches, uint64_t, uint64_t ) const override
  {
    (void)num_latches;
    if constexpr ( !has_create_ri_v<Ntk> || !has_create_ro_v<Ntk> )
    {
      assert( num_latches == 0 && "network type does not support the creation of latches" );
    }

    _num_inputs = static_cast<uint32_t>( num_inputs );

    /* constant */
    signals.push_back( _ntk.get_constant( false ) );

    /* create primary inputs (pi) */
    for ( auto i = 0u; i < num_inputs; ++i )
    {
      signals.push_back( _ntk.create_pi() );
    }

    if constexpr ( has_create_ro_v<Ntk> )
    {
      /* create latch outputs (ro) */
      for ( auto i = 0u; i < num_latches; ++i )
      {
        signals.push_back( _ntk.create_ro() );
      }
    }
  }

  void on_input_name( unsigned index, const std::string& name ) const override
  {
    if constexpr ( has_set_name_v<Ntk> )
    {
      _ntk.set_name( signals[1 + index], name );
    }
  }

  void on_output_name( unsigned index, const std::string& name ) const override
  {
    std::get<1>( outputs[index] ) = name;
  }

  void on_latch_name( unsigned index, const std::string& name ) const override
  {
    if constexpr ( has_create_ri_v<Ntk> && has_create_ro_v<Ntk> )
    {
      if constexpr ( has_set_name_v<Ntk> )
      {
        _ntk.set_name( signals[1 + _num_inputs + index], name );
      }
      std::get<2>( latches[index] ) = name;
    }
  }

  void on_and( unsigned index, unsigned left_lit, unsigned right_lit ) const override
  {
    (void)index;
    assert( signals.size() == index );

    auto left = signals[left_lit >> 1];
    if ( left_lit & 1 )
    {
      left = _ntk.create_not( left );
    }

    auto right = signals[right_lit >> 1];
    if ( right_lit & 1 )
    {
      right = _ntk.create_not( right );
    }

    signals.push_back( _ntk.create_and( left, right ) );
  }

  void on_latch( unsigned index, unsigned next, latch_init_value reset ) const override
  {
    if constexpr ( has_create_ri_v<Ntk> && has_create_ro_v<Ntk> )
    {
      (void)index;
      int8_t r = reset == latch_init_value::NONDETERMINISTIC ? -1 : ( reset == latch_init_value::ONE ? 1 : 0 );
      latches.push_back( std::make_tuple( next, r, "" ) );
    }
  }

  void on_output( unsigned index, unsigned lit ) const override
  {
    (void)index;
    assert( index == outputs.size() );
    outputs.emplace_back( lit, "" );
  }

private:
  Ntk& _ntk;

  mutable uint32_t _num_inputs{ 0 };
  mutable std::vector<std::tuple<unsigned, std::string>> outputs;
  mutable std::vector<typename Ntk::signal> signals;
  mutable std::vector<std::tuple<unsigned, int8_t, std::string>> latches;
};

} /* namespace mockturtle */