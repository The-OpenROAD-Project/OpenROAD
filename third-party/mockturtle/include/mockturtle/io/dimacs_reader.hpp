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
  \file dimacs_reader.hpp
  \brief Lorina reader for DIMACS files

  \author Bruno Schmitt
*/

#pragma once

#include "../traits.hpp"
#include <lorina/dimacs.hpp>

namespace mockturtle
{

/*! \brief Lorina reader callback for DIMACS files.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `create_not`
 * - `create_nary_and`
 * - `create_nary_or`
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      xag_network xag;
      lorina::read_dimacs( "file.cnf", dimacs_reader( xag ) );
   \endverbatim
 */
template<typename Ntk>
class dimacs_reader : public lorina::dimacs_reader
{
public:
  explicit dimacs_reader( Ntk& ntk ) : _ntk( ntk )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi function" );
    static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po function" );
    static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not function" );
    static_assert( has_create_nary_and_v<Ntk>, "Ntk does not implement the create_nary_and function" );
    static_assert( has_create_nary_or_v<Ntk>, "Ntk does not implement the create_nary_or function" );
  }

  void on_number_of_variables( uint64_t number_of_inputs ) const override
  {
    _pis.resize( number_of_inputs );
    std::generate( _pis.begin(), _pis.end(), [this]() { return _ntk.create_pi(); } );
  }

  void on_clause( const std::vector<int>& clause ) const override
  {
    std::vector<signal<Ntk>> literals;

    for ( int lit : clause )
    {
      uint32_t var = std::abs( lit ) - 1;
      if ( lit < 0 )
      {
        literals.push_back( !_pis.at( var ) );
      }
      else
      {
        literals.push_back( _pis.at( var ) );
      }
    }

    const auto sum = _ntk.create_nary_or( literals );
    _sums.push_back( sum );
  }

  void on_end() const override
  {
    const auto output = _ntk.create_nary_and( _sums );
    _ntk.create_po( output );
  }

private:
  Ntk& _ntk;
  mutable std::vector<signal<Ntk>> _pis;
  mutable std::vector<signal<Ntk>> _sums;
};

} /* namespace mockturtle */