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
  \file pla_reader.hpp
  \brief Lorina reader for PLA files

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include "../traits.hpp"
#include <lorina/pla.hpp>

namespace mockturtle
{

/*! \brief Lorina reader callback for PLA files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `create_not`
 * - `create_nary_and`
 * - `create_nary_or`
 * - `create_nary_xor`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      aig_network aig;
      lorina::read_pla( "file.pla", pla_reader( aig ) );

      mig_network mig;
      lorina::read_pla( "file.pla", pla_reader( mig ) );
   \endverbatim
 */
template<typename Ntk>
class pla_reader : public lorina::pla_reader
{
public:
  explicit pla_reader( Ntk& ntk ) : _ntk( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( has_create_nary_and_v<Ntk>, "Ntk does not implement the create_nary_and function" );
    static_assert( has_create_nary_or_v<Ntk>, "Ntk does not implement the create_nary_or function" );
    static_assert( has_create_nary_xor_v<Ntk>, "Ntk does not implement the create_nary_xor function" );
  }

  void on_number_of_inputs( uint64_t number_of_inputs ) const override
  {
    _pis.resize( number_of_inputs );
    std::generate( _pis.begin(), _pis.end(), [this]() { return _ntk.create_pi(); } );
  }

  void on_number_of_outputs( uint64_t number_of_outputs ) const override
  {
    _products.resize( number_of_outputs );
  }

  bool on_keyword( const std::string& keyword, const std::string& value ) const override
  {
    if ( keyword == "type" && value == "esop" )
    {
      _is_xor = true;
      return true;
    }
    return false;
  }

  void on_end() const override
  {
    for ( auto const& v : _products )
    {
      _ntk.create_po( _is_xor ? _ntk.create_nary_xor( v ) : _ntk.create_nary_or( v ) );
    }
  }

  void on_term( const std::string& term, const std::string& out ) const override
  {
    std::vector<signal<Ntk>> literals;
    for ( auto i = 0u; i < term.size(); ++i )
    {
      switch ( term[i] )
      {
      default:
        std::cerr << "[w] unknown character '" << term[i] << "' in PLA input term, treat as don't care\n";
      case '-':
        break;

      case '0':
        literals.push_back( _ntk.create_not( _pis[i] ) );
        break;
      case '1':
        literals.push_back( _pis[i] );
        break;
      }
    }

    const auto product = _ntk.create_nary_and( literals );
    for ( auto i = 0u; i < out.size(); ++i )
    {
      switch ( out[i] )
      {
      default:
        std::cerr << "[w] unknown character '" << out[i] << "' in PLA output term, treat is 0\n";
      case '0':
        break;

      case '1':
        _products[i].push_back( product );
        break;
      }
    }
  }

private:
  Ntk& _ntk;
  mutable std::vector<signal<Ntk>> _pis;
  mutable std::vector<std::vector<signal<Ntk>>> _products;
  mutable bool _is_xor{ false };
};

} /* namespace mockturtle */