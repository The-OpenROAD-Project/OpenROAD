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
  \file dsd_decomposition.hpp
  \brief DSD decomposition

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <cstdint>
#include <vector>

#include "../traits.hpp"

#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/decomposition.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/operators.hpp>
#include <kitty/print.hpp>

namespace mockturtle
{

/*! \brief Parameters for dsd_decomposition */
struct dsd_decomposition_params
{
  /*! \brief Apply XOR decomposition. */
  bool with_xor{ true };
};

namespace detail
{

template<class Ntk, class Fn>
class dsd_decomposition_impl
{
public:
  dsd_decomposition_impl( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children, Fn&& on_prime, dsd_decomposition_params const& ps )
      : _ntk( ntk ),
        remainder( func ),
        pis( children ),
        _on_prime( on_prime ),
        _ps( ps )
  {
    for ( auto i = 0u; i < func.num_vars(); ++i )
    {
      if ( kitty::has_var( func, i ) )
      {
        support.push_back( i );
      }
    }
  }

  signal<Ntk> run()
  {
    /* terminal cases */
    if ( kitty::is_const0( remainder ) )
    {
      return _ntk.get_constant( false );
    }
    if ( kitty::is_const0( ~remainder ) )
    {
      return _ntk.get_constant( true );
    }

    /* projection case */
    if ( support.size() == 1u )
    {
      auto var = remainder.construct();
      kitty::create_nth_var( var, support.front() );
      if ( remainder == var )
      {
        return pis[support.front()];
      }
      else
      {
        if ( remainder != ~var )
        {
          fmt::print( "remainder = {}, vars = {}\n", kitty::to_binary( remainder ), remainder.num_vars() );
          assert( false );
        }
        assert( remainder == ~var );
        return _ntk.create_not( pis[support.front()] );
      }
    }

    /* try top decomposition */
    for ( auto var : support )
    {
      if ( auto res = kitty::is_top_decomposable( remainder, var, &remainder, _ps.with_xor );
           res != kitty::top_decomposition::none )
      {
        /* remove var from support, pis do not change */
        support.erase( std::remove( support.begin(), support.end(), var ), support.end() );
        const auto right = run();

        switch ( res )
        {
        default:
          assert( false );
        case kitty::top_decomposition::and_:
          return _ntk.create_and( pis[var], right );
        case kitty::top_decomposition::or_:
          return _ntk.create_or( pis[var], right );
        case kitty::top_decomposition::lt_:
          return _ntk.create_lt( pis[var], right );
        case kitty::top_decomposition::le_:
          return _ntk.create_le( pis[var], right );
        case kitty::top_decomposition::xor_:
          return _ntk.create_xor( pis[var], right );
        }
      }
    }

    /* try bottom decomposition */
    for ( auto j = 1u; j < support.size(); ++j )
    {
      for ( auto i = 0u; i < j; ++i )
      {
        if ( auto res = kitty::is_bottom_decomposable( remainder, support[i], support[j], &remainder, _ps.with_xor );
             res != kitty::bottom_decomposition::none )
        {
          /* update pis based on decomposition type */
          switch ( res )
          {
          default:
            assert( false );
          case kitty::bottom_decomposition::and_:
            pis[support[i]] = _ntk.create_and( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::or_:
            pis[support[i]] = _ntk.create_or( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::lt_:
            pis[support[i]] = _ntk.create_lt( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::le_:
            pis[support[i]] = _ntk.create_le( pis[support[i]], pis[support[j]] );
            break;
          case kitty::bottom_decomposition::xor_:
            pis[support[i]] = _ntk.create_xor( pis[support[i]], pis[support[j]] );
            break;
          }

          /* remove var from support */
          support.erase( support.begin() + j );

          return run();
        }
      }
    }

    /* cannot decompose anymore */
    std::vector<signal<Ntk>> new_pis;
    for ( auto var : support )
    {
      new_pis.push_back( pis[var] );
    }
    auto prime_large = remainder;
    kitty::min_base_inplace( prime_large );
    auto prime = kitty::shrink_to( prime_large, static_cast<unsigned int>( support.size() ) );
    return _on_prime( prime, new_pis );
  }

private:
  Ntk& _ntk;
  kitty::dynamic_truth_table remainder;
  std::vector<uint8_t> support;
  std::vector<signal<Ntk>> pis;
  Fn&& _on_prime;
  dsd_decomposition_params const& _ps;
};

} // namespace detail

/*! \brief DSD decomposition
 *
 * This function applies DSD decomposition on an input truth table and
 * constructs a network based on all possible decompositions.  If the truth
 * table is only partially decomposable, then the remaining *prime function*
 * is returned back to the caller using the call back `on_prime` together with
 * the computed primary inputs for that remainder.
 *
 * The `on_prime` function must be of type `NtkDest::signal(
 * kitty::dynamic_truth_table const&, std::vector<NtkDest::signal> const&)`.
 *
 * **Required network functions:**
 * - `create_not`
 * - `create_and`
 * - `create_or`
 * - `create_lt`
 * - `create_le`
 * - `create_xor`
 */
template<class Ntk, class Fn>
signal<Ntk> dsd_decomposition( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<signal<Ntk>> const& children, Fn&& on_prime, dsd_decomposition_params const& ps = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_or_v<Ntk>, "Ntk does not implement the create_or method" );
  static_assert( has_create_lt_v<Ntk>, "Ntk does not implement the create_lt method" );
  static_assert( has_create_le_v<Ntk>, "Ntk does not implement the create_le method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );

  detail::dsd_decomposition_impl<Ntk, Fn> impl( ntk, func, children, on_prime, ps );
  return impl.run();
}

} // namespace mockturtle