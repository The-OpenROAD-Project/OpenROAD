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
  \file decomposition.hpp
  \brief Shannon and Davio decomposition

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "../traits.hpp"
#include "node_resynthesis/null.hpp"

#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <kitty/operations.hpp>
#include <kitty/operators.hpp>

namespace mockturtle
{

namespace detail
{

template<class Ntk, class SynthesisFn>
class shannon_decomposition_impl
{
public:
  shannon_decomposition_impl( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<uint32_t> const& vars, std::vector<signal<Ntk>> const& children, SynthesisFn const& resyn )
      : ntk_( ntk ),
        func_( func ),
        vars_( vars ),
        pis_( children ),
        resyn_( resyn )
  {
    cache_.insert( { func.construct(), ntk_.get_constant( false ) } );

    for ( auto i = 0u; i < pis_.size(); ++i )
    {
      auto var = func.construct();
      kitty::create_nth_var( var, i );
      cache_.insert( { func.construct(), pis_[i] } );
    }
  }

  signal<Ntk> run()
  {
    return decompose( 0u, func_ );
  }

private:
  signal<Ntk> decompose( uint32_t var_index, kitty::dynamic_truth_table const& func )
  {
    /* cache lookup... */
    auto it = cache_.find( func );
    if ( it != cache_.end() )
    {
      return it->second;
    }

    /* ...and for the complement */
    it = cache_.find( ~func );
    if ( it != cache_.end() )
    {
      return ntk_.create_not( it->second );
    }

    signal<Ntk> f;
    if ( var_index == vars_.size() )
    {
      auto copy = func;
      const auto support = kitty::min_base_inplace( copy );
      const auto small_func = kitty::shrink_to( copy, static_cast<unsigned int>( support.size() ) );
      std::vector<signal<Ntk>> small_pis( support.size() );
      for ( auto i = 0u; i < support.size(); ++i )
      {
        small_pis[i] = pis_[support[i]];
      }
      resyn_( ntk_, small_func, small_pis.begin(), small_pis.end(), [&]( auto const& _f ) {
        f = _f;
        return false;
      } );
    }
    else
    {
      /* decompose */
      const auto f0 = decompose( var_index + 1, kitty::cofactor0( func, vars_[var_index] ) );
      const auto f1 = decompose( var_index + 1, kitty::cofactor1( func, vars_[var_index] ) );
      f = ntk_.create_ite( pis_[vars_[var_index]], f1, f0 );
    }
    cache_.insert( { func, f } );
    return f;
  }

private:
  Ntk& ntk_;
  kitty::dynamic_truth_table func_;
  std::vector<uint32_t> vars_;
  std::vector<signal<Ntk>> const& pis_;
  SynthesisFn const& resyn_;
  std::unordered_map<kitty::dynamic_truth_table, signal<Ntk>, kitty::hash<kitty::dynamic_truth_table>> cache_;
};

template<class Ntk, class SynthesisFn>
class davio_decomposition_impl
{
public:
  davio_decomposition_impl( Ntk& ntk, bool polarity, kitty::dynamic_truth_table const& func, std::vector<uint32_t> const& vars, std::vector<signal<Ntk>> const& children, SynthesisFn const& resyn )
      : ntk_( ntk ),
        polarity_( polarity ),
        func_( func ),
        pis_( children ),
        vars_( vars ),
        resyn_( resyn )
  {
    cache_.insert( { func.construct(), ntk_.get_constant( false ) } );

    for ( auto i = 0u; i < pis_.size(); ++i )
    {
      auto var = func.construct();
      kitty::create_nth_var( var, i );
      cache_.insert( { func.construct(), pis_[i] } );
    }
  }

  signal<Ntk> run()
  {
    return decompose( 0u, func_ );
  }

private:
  signal<Ntk> decompose( uint32_t var_index, kitty::dynamic_truth_table const& func )
  {
    /* cache lookup... */
    auto it = cache_.find( func );
    if ( it != cache_.end() )
    {
      return it->second;
    }

    /* ...and for the complement */
    it = cache_.find( ~func );
    if ( it != cache_.end() )
    {
      return ntk_.create_not( it->second );
    }

    signal<Ntk> f;
    if ( var_index == vars_.size() )
    {
      auto copy = func;
      const auto support = kitty::min_base_inplace( copy );
      const auto small_func = kitty::shrink_to( copy, static_cast<unsigned int>( support.size() ) );
      std::vector<signal<Ntk>> small_pis( support.size() );
      for ( auto i = 0u; i < support.size(); ++i )
      {
        small_pis[i] = pis_[support[i]];
      }
      resyn_( ntk_, small_func, small_pis.begin(), small_pis.end(), [&]( auto const& _f ) {
        f = _f;
        return false;
      } );
    }
    else
    {
      /* decompose */
      const auto f0 = decompose( var_index + 1, kitty::cofactor0( func, vars_[var_index] ) );
      const auto f1 = decompose( var_index + 1, kitty::cofactor1( func, vars_[var_index] ) );

      if ( polarity_ )
      {
        f = ntk_.create_xor( f0, ntk_.create_and( pis_[vars_[var_index]], ntk_.create_xor( f0, f1 ) ) );
      }
      else
      {
        f = ntk_.create_xor( f1, ntk_.create_and( ntk_.create_not( pis_[vars_[var_index]] ), ntk_.create_xor( f0, f1 ) ) );
      }
    }
    cache_.insert( { func, f } );
    return f;
  }

private:
  Ntk& ntk_;
  bool polarity_;
  kitty::dynamic_truth_table func_;
  std::vector<signal<Ntk>> const& pis_;
  std::vector<uint32_t> const& vars_;
  SynthesisFn const& resyn_;
  std::unordered_map<kitty::dynamic_truth_table, signal<Ntk>, kitty::hash<kitty::dynamic_truth_table>> cache_;
};

} // namespace detail

/*! \brief Shannon decomposition
 *
 * This function applies Shannon decomposition on an input truth table and
 * constructs a network based.  The variable ordering can be specified as
 * an input.  If not all variables are specified, the remaining co-factors
 * are synthesizes using the resynthesis function.
 *
 * **Required network functions:**
 * - `create_not`
 * - `create_ite`
 * - `get_constant`
 */
template<class Ntk, class SynthesisFn = null_resynthesis<Ntk>>
signal<Ntk> shannon_decomposition( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<uint32_t> const& vars, std::vector<signal<Ntk>> const& children, SynthesisFn const& resyn = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_ite method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  detail::shannon_decomposition_impl<Ntk, SynthesisFn> impl( ntk, func, vars, children, resyn );
  return impl.run();
}

/*! \brief Positive Davio decomposition
 *
 * This function applies positive Davio decomposition on an input truth table and
 * constructs a network based.  The variable ordering can be specified as
 * an input.  If not all variables are specified, the remaining co-factors
 * are synthesizes using the resynthesis function.
 *
 * **Required network functions:**
 * - `create_not`
 * - `create_and`
 * - `create_xor`
 * - `get_constant`
 */
template<class Ntk, class SynthesisFn = null_resynthesis<Ntk>>
signal<Ntk> positive_davio_decomposition( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<uint32_t> const& vars, std::vector<signal<Ntk>> const& children, SynthesisFn const& resyn = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_ite method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_ite method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  detail::davio_decomposition_impl<Ntk, SynthesisFn> impl( ntk, true, func, vars, children, resyn );
  return impl.run();
}

/*! \brief Negative Davio decomposition
 *
 * This function applies positive Davio decomposition on an input truth table and
 * constructs a network based.  The variable ordering can be specified as
 * an input.  If not all variables are specified, the remaining co-factors
 * are synthesizes using the resynthesis function.
 *
 * **Required network functions:**
 * - `create_not`
 * - `create_and`
 * - `create_xor`
 * - `get_constant`
 */
template<class Ntk, class SynthesisFn = null_resynthesis<Ntk>>
signal<Ntk> negative_davio_decomposition( Ntk& ntk, kitty::dynamic_truth_table const& func, std::vector<uint32_t> const& vars, std::vector<signal<Ntk>> const& children, SynthesisFn const& resyn = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_ite method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_ite method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  detail::davio_decomposition_impl<Ntk, SynthesisFn> impl( ntk, false, func, vars, children, resyn );
  return impl.run();
}

} // namespace mockturtle
