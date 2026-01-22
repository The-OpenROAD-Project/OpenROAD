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
  \file bi_decomposition.hpp
  \brief BI decomposition

  \author Eleonora Testa
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <cstdint>
#include <vector>

#include "../traits.hpp"

#include <kitty/constructors.hpp>
#include <kitty/decomposition.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/print.hpp>

namespace mockturtle
{

namespace detail
{

template<class Ntk>
class bi_decomposition_impl
{
public:
  bi_decomposition_impl( Ntk& ntk, kitty::dynamic_truth_table const& func, kitty::dynamic_truth_table const& dc, std::vector<signal<Ntk>> const& children )
      : _ntk( ntk ),
        remainder( func ),
        dc_remainder( dc ),
        pis( children ) {}

  signal<Ntk> run()
  {
    /* bi_decomposition */
    if ( kitty::is_const0( binary_and( remainder, dc_remainder ) ) )
    {
      return _ntk.get_constant( false );
    }
    else if ( kitty::is_const0( binary_and( ~remainder, dc_remainder ) ) )
    {
      return _ntk.get_constant( true );
    }
    else
    {
      for ( auto h = 0u; h < remainder.num_vars(); h++ )
      {
        auto var = remainder.construct();
        kitty::create_nth_var( var, h );
        if ( binary_and( remainder, dc_remainder ) == binary_and( var, dc_remainder ) )
        {
          return pis[h];
        }
        else if ( binary_and( remainder, dc_remainder ) == binary_and( ~var, dc_remainder ) )
        {
          return _ntk.create_not( pis[h] );
        }
      }
    }

    auto bi_dec = kitty::is_bi_decomposable_mc( remainder, dc_remainder );
    auto res = std::get<1>( bi_dec );

    remainder = std::get<2>( bi_dec )[0];
    dc_remainder = std::get<2>( bi_dec )[1];
    const auto right = run();

    remainder = std::get<2>( bi_dec )[2];
    dc_remainder = std::get<2>( bi_dec )[3];
    const auto left = run();

    if ( res == kitty::bi_decomposition::and_ )
    {
      return _ntk.create_and( left, right );
    }
    else if ( res == kitty::bi_decomposition::or_ )
    {
      return _ntk.create_or( left, right );
    }
    else if ( res == kitty::bi_decomposition::weak_and_ )
    {
      return _ntk.create_and( left, right );
    }
    else if ( res == kitty::bi_decomposition::weak_or_ )
    {
      return _ntk.create_or( left, right );
    }
    else if ( res == kitty::bi_decomposition::xor_ )
    {
      return _ntk.create_xor( left, right );
    }
    else
    {
      assert( false );
      return signal<Ntk>();
    }
  }

private:
  Ntk& _ntk;
  kitty::dynamic_truth_table remainder;
  kitty::dynamic_truth_table dc_remainder;
  std::vector<signal<Ntk>> pis;
};

} // namespace detail

/*! \brief Bi decomposition
 *
 * This function applies bi-decomposition on a truth table inside the network.
 *
 * Note that the number of variables in `func` and `care` must be the same.
 * The function will create a network composed on two-input gates with as many primary inputs as the number of
 * variables in `func` and a single output.
 *
 * **Required network functions:**
 * - `create_and`
 * - `create_or`
 * - `create_xor`
 * - `create_not`
 *
 * \param func Function as truth table
 * \param care Care set of the function (as truth table)
 * \return An internal signal of the network
 */

template<class Ntk>
signal<Ntk> bi_decomposition( Ntk& ntk, kitty::dynamic_truth_table const& func, kitty::dynamic_truth_table const& care, std::vector<signal<Ntk>> const& children )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_or_v<Ntk>, "Ntk does not implement the create_or method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );

  detail::bi_decomposition_impl<Ntk> impl( ntk, func, care, children );
  return impl.run();
}

} // namespace mockturtle