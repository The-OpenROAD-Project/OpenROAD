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
  \file bristol_reader.hpp
  \brief Lorina reader for Bristol files

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <vector>

#include <fmt/format.h>
#include <lorina/bristol.hpp>

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Lorina reader callback for Bristol files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `create_and`
 * - `create_xor`
 * - `create_not`
 * - `get_constant`
 *
   \verbatim embed:rst
   Example
   .. code-block:: c++
      xag_network xag;
      lorina::read_bristol( "file.txt", bristol_reader( xag ) );
   \endverbatim
 */
template<typename Ntk>
class bristol_reader : public lorina::bristol_reader
{
public:
  explicit bristol_reader( Ntk& ntk )
      : ntk_( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and function" );
    static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant function" );
  }

  ~bristol_reader()
  {
  }

  virtual void on_header( uint32_t num_gates, uint32_t num_wires, uint32_t num_inputs, std::vector<uint32_t> const& num_wires_per_input, uint32_t num_outputs, std::vector<uint32_t> const& num_wires_per_output ) const override
  {
    (void)num_inputs;
    (void)num_outputs;

    const auto num_pis = std::accumulate( num_wires_per_input.begin(), num_wires_per_input.end(), 0u );
    num_pos_ = std::accumulate( num_wires_per_output.begin(), num_wires_per_output.end(), 0u );
    num_gates_ = num_gates;
    gate_ctr_ = 0u;
    signal_.resize( num_wires );

    for ( auto i = 0u; i < num_pis; ++i )
    {
      signal_[i] = ntk_.create_pi();
    }
  }

  virtual void on_gate( std::vector<uint32_t> const& in, uint32_t out, std::string const& gate ) const override
  {
    if ( gate == "XOR" )
    {
      signal_[out] = ntk_.create_xor( signal_[in[0]], signal_[in[1]] );
    }
    else if ( gate == "AND" )
    {
      signal_[out] = ntk_.create_and( signal_[in[0]], signal_[in[1]] );
    }
    else if ( gate == "INV" )
    {
      signal_[out] = ntk_.create_not( signal_[in[0]] );
    }
    else if ( gate == "EQW" )
    {
      signal_[out] = signal_[in[0]];
    }
    else
    {
      fmt::print( "[e] unknown {} gate {} with {} inputs!", gate, out, in.size() );
      std::abort();
    }

    ++gate_ctr_;

    if ( gate_ctr_ == num_gates_ )
    {
      for ( auto i = signal_.size() - num_pos_; i < signal_.size(); ++i )
      {
        ntk_.create_po( signal_[i] );
      }
    }
    else if ( gate_ctr_ > num_gates_ )
    {
      fmt::print( "[w] adding dangling {} gate with inputs {} and output {}\n", gate, fmt::join( in, ", " ), out );
    }
  }

private:
  Ntk& ntk_;

  mutable uint32_t num_pos_;
  mutable uint32_t num_gates_;
  mutable uint32_t gate_ctr_;
  mutable std::vector<signal<Ntk>> signal_;
}; /* bristol_reader */

} /* namespace mockturtle */