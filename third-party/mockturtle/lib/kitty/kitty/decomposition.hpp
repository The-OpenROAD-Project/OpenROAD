/* kitty: C++ truth table library
 * Copyright (C) 2017-2025  EPFL
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
  \brief Check decomposition properties (and perform decomposition) of a function

  \author Mahyar Emami (mahyar.emami@epfl.ch)
  \author Mathias Soeken
  \author Eleonora Testa
*/

#pragma once

#include <cstdint>
#include <unordered_map>

#include "constructors.hpp"
#include "operations.hpp"
#include "implicant.hpp"
#include "traits.hpp"

namespace kitty
{

enum class top_decomposition
{
  none,
  and_,
  or_,
  lt_,
  le_,
  xor_
};

enum class bottom_decomposition
{
  none,
  and_,
  or_,
  lt_,
  le_,
  xor_
};

enum class bi_decomposition
{
  none,
  and_,
  or_,
  xor_,
  weak_and_,
  weak_or_
};

/*! \brief Checks, whether function is top disjoint decomposable

  \verbatim embed:rst
  Checks whether the input function ``tt`` can be represented by the function
  :math:`f = g(h(X_1), a)`, where :math:`a \notin X_1`.  The return value
  is :math:`g`:

  * ``top_decomposition::and_``: :math:`g = a \land h(X_1)`
  * ``top_decomposition::or_``: :math:`g = a \lor h(X_1)`
  * ``top_decomposition::lt_``: :math:`g = \bar a \land h(X_1)`
  * ``top_decomposition::le_``: :math:`g = \bar a \lor h(X_1)`
  * ``top_decomposition::xor_``: :math:`g = a \oplus h(X_1)`
  * ``top_decomposition::none``: decomposition does not exist

  The function can return the remainder function :math:`h`, whic will not depend
  on :math:`a`.
  \endverbatim

  \param tt Input function \f$f\f$
  \param var_index Variable \f$a\f$
  \param func If not ``null`` and decomposition exists, its value is assigned the remainder \f$h\f$
  \param allow_xor Set to false to disable XOR decomposition
*/
template<class TT>
top_decomposition is_top_decomposable( const TT& tt, uint32_t var_index, TT* func = nullptr, bool allow_xor = true )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  auto var = tt.construct();
  kitty::create_nth_var( var, var_index );

  if ( implies( tt, var ) )
  {
    if ( func )
    {
      *func = cofactor1( tt, var_index );
    }
    return top_decomposition::and_;
  }
  else if ( implies( var, tt ) )
  {
    if ( func )
    {
      *func = cofactor0( tt, var_index );
    }
    return top_decomposition::or_;
  }
  else if ( implies( tt, ~var ) )
  {
    if ( func )
    {
      *func = cofactor0( tt, var_index );
    }
    return top_decomposition::lt_;
  }
  else if ( implies( ~var, tt ) )
  {
    if ( func )
    {
      *func = cofactor1( tt, var_index );
    }
    return top_decomposition::le_;
  }

  if ( allow_xor )
  {
    /* try XOR */
    const auto co0 = cofactor0( tt, var_index );
    const auto co1 = cofactor1( tt, var_index );

    if ( equal( co0, ~co1 ) )
    {
      if ( func )
      {
        *func = co0;
      }
      return top_decomposition::xor_;
    }
  }

  return top_decomposition::none;
}

/*! \brief Checks, whether function is bottom disjoint decomposable

  \verbatim embed:rst
  Checks whether the input function ``tt`` can be represented by the function
  :math:`f = h(X_1, g(a, b))`, where :math:`a, b \notin X_1`.  The return value
  is :math:`g`:

  * ``bottom_decomposition::and_``: :math:`g = a \land b`
  * ``bottom_decomposition::or_``: :math:`g = a \lor b`
  * ``bottom_decomposition::lt_``: :math:`g = \bar a \land b`
  * ``bottom_decomposition::le_``: :math:`g = \bar a \lor b`
  * ``bottom_decomposition::xor_``: :math:`g = a \oplus b`
  * ``bottom_decomposition::none``: decomposition does not exist

  The function can return the remainder function :math:`h` in where :math:`g`
  is substituted by :math:`a`.  The remainder function will not depend on
  :math:`b`.
  \endverbatim

  \param tt Input function \f$f\f$
  \param var_index1 Variable \f$a\f$
  \param var_index2 Variable \f$b\f$
  \param func If not ``null`` and decomposition exists, its value is assigned the remainder \f$h\f$
  \param allow_xor Set to false to disable XOR decomposition
*/
template<class TT>
bottom_decomposition is_bottom_decomposable( const TT& tt, uint32_t var_index1, uint32_t var_index2, TT* func = nullptr, bool allow_xor = true )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto tt0 = cofactor0( tt, var_index1 );
  const auto tt1 = cofactor1( tt, var_index1 );

  const auto tt00 = cofactor0( tt0, var_index2 );
  const auto tt01 = cofactor1( tt0, var_index2 );
  const auto tt10 = cofactor0( tt1, var_index2 );
  const auto tt11 = cofactor1( tt1, var_index2 );

  const auto eq01 = equal( tt00, tt01 );
  const auto eq02 = equal( tt00, tt10 );
  const auto eq03 = equal( tt00, tt11 );
  const auto eq12 = equal( tt01, tt10 );
  const auto eq13 = equal( tt01, tt11 );
  const auto eq23 = equal( tt10, tt11 );

  const auto num_pairs =
      static_cast<uint32_t>( eq01 ) +
      static_cast<uint32_t>( eq02 ) +
      static_cast<uint32_t>( eq03 ) +
      static_cast<uint32_t>( eq12 ) +
      static_cast<uint32_t>( eq13 ) +
      static_cast<uint32_t>( eq23 );

  if ( num_pairs != 2u && num_pairs != 3 )
  {
    return bottom_decomposition::none;
  }

  if ( !eq01 && !eq02 && !eq03 ) // 00 is different
  {
    if ( func )
    {
      *func = mux_var( var_index1, tt11, tt00 );
    }
    return bottom_decomposition::or_;
  }
  else if ( !eq01 && !eq12 && !eq13 ) // 01 is different
  {
    if ( func )
    {
      *func = mux_var( var_index1, tt01, tt10 );
    }
    return bottom_decomposition::lt_;
  }
  else if ( !eq02 && !eq12 && !eq23 ) // 10 is different
  {
    if ( func )
    {
      *func = mux_var( var_index1, tt01, tt10 );
    }
    return bottom_decomposition::le_;
  }
  else if ( !eq03 && !eq13 && !eq23 ) // 11 is different
  {
    if ( func )
    {
      *func = mux_var( var_index1, tt11, tt00 );
    }
    return bottom_decomposition::and_;
  }
  else if ( allow_xor ) // XOR
  {
    if ( func )
    {
      *func = mux_var( var_index1, tt01, tt00 );
    }
    return bottom_decomposition::xor_;
  }

  return bottom_decomposition::none;
}

/*! \cond PRIVATE */
namespace detail
{

template<class TT>
TT exist_set( const TT& tt, const std::vector<int>& set )
{
  auto exist = tt;
  for ( auto x = 0u; x < set.size(); x++ )
  {
    auto ra_0 = cofactor0( exist, set[x] );
    auto ra_1 = cofactor1( exist, set[x] );
    exist = binary_or( ra_0, ra_1 );
  }
  return exist;
}

template<class TT>
TT select_one_cube( const TT& q )
{
  auto m = q.construct();
  auto minterms = kitty::get_minterms( q );
  const uint64_t min = minterms[0];
  set_bit( m, min );
  for ( auto i = 0u; i < q.num_vars(); i++ )
  {
    std::vector<int> h( 1, i );
    auto m_p = exist_set( m, h );
    if ( binary_and( m_p, q ) == m_p )
    {
      m = m_p;
    }
  }
  return m;
}

template<class TT>
bool check_or_decomp( const TT& tt, const TT& dc, const std::vector<int>& i, const std::vector<int>& j )
{
  auto q = binary_and( tt, dc );
  auto r = binary_and( ~tt, dc );
  auto p = binary_and( q, binary_and( exist_set( r, i ), exist_set( r, j ) ) );

  return is_const0( p );
}

template<class TT>
std::pair<bool, std::vector<TT>> check_xor_decomp( const TT& tt, const TT& dc, const std::vector<int>& i, const std::vector<int>& j )
{
  auto q = binary_and( tt, dc );
  auto r = binary_and( ~tt, dc );

  auto qa = tt.construct();
  auto ra = tt.construct();
  auto qb = tt.construct();
  auto rb = tt.construct();

  std::vector<TT> q_and_rs{ qa, ra, qb, rb };
  while ( !is_const0( q ) )
  {
    auto cube = select_one_cube( q );
    qa = binary_or( qa, exist_set( cube, j ) );
    while ( !is_const0( binary_or( qa, ra ) ) )
    {
      qb = exist_set( binary_or( binary_and( q, ra ), binary_and( r, qa ) ), i );
      rb = exist_set( binary_or( binary_and( q, qa ), binary_and( r, ra ) ), i );
      if ( !is_const0( binary_and( qb, rb ) ) )
      {
        return { false, q_and_rs };
      }
      q = binary_and( q, ~binary_or( qa, ra ) );
      r = binary_and( r, ~binary_or( qa, ra ) );

      q_and_rs[0] = binary_or( q_and_rs[0], qa );
      q_and_rs[1] = binary_or( q_and_rs[1], ra );

      qa = exist_set( binary_or( binary_and( q, rb ), binary_and( r, qb ) ), j );
      ra = exist_set( binary_or( binary_and( q, qb ), binary_and( r, rb ) ), j );
      if ( !is_const0( binary_and( qa, ra ) ) )
      {
        return { false, q_and_rs };
      }
      q = binary_and( q, ~binary_or( qb, rb ) );
      r = binary_and( r, ~binary_or( qb, rb ) );

      q_and_rs[2] = binary_or( q_and_rs[2], qb );
      q_and_rs[3] = binary_or( q_and_rs[3], rb );
    }
  }
  if ( !is_const0( r ) )
  {
    q_and_rs[1] = binary_or( q_and_rs[1], exist_set( r, j ) );
    q_and_rs[3] = binary_or( q_and_rs[3], exist_set( r, i ) );
  }
  return { true, q_and_rs };
}

template<class TT>
bool check_weak_decomp( const TT& tt, const TT& dc, const std::vector<int>& i )
{
  auto p = exist_set( binary_and( ~tt, dc ), i );
  return !is_const0( binary_and( binary_and( tt, dc ), ~p ) );
}

template<class TT>
std::pair<std::vector<int>, std::vector<int>> find_initial_or( const TT& tt, const TT& dc, const std::vector<int>& s )
{
  std::vector<int> var_a_vect, var_b_vect;
  for ( auto& i : s )
  {
    for ( auto& j : s )
    {
      if ( j == i )
      {
        continue;
      }
      std::vector<int> var_a_p( 1, i );
      std::vector<int> var_b_p( 1, j );
      if ( check_or_decomp( tt, dc, var_a_p, var_b_p ) )
      {
        var_a_vect.push_back( i );
        var_b_vect.push_back( j );
        return { var_a_vect, var_b_vect };
      }
    }
  }
  return { var_a_vect, var_b_vect };
}

template<class TT>
std::tuple<std::vector<int>, std::vector<int>, std::vector<TT>> find_initial_xor( const TT& tt, const TT& dc, const std::vector<int>& s )
{
  std::vector<int> var_a_vect, var_b_vect;
  std::vector<TT> q_and_r;

  for ( auto& i : s )
  {
    for ( auto& j : s )
    {
      if ( j == i )
      {
        continue;
      }
      std::vector<int> var_a_p( 1, i );
      std::vector<int> var_b_p( 1, j );
      auto f = check_xor_decomp( tt, dc, var_a_p, var_b_p );
      if ( f.first )
      {
        var_a_vect.push_back( i );
        var_b_vect.push_back( j );
        q_and_r = f.second;
        return std::make_tuple( var_a_vect, var_b_vect, q_and_r );
      }
    }
  }
  return std::make_tuple( var_a_vect, var_b_vect, q_and_r );
}

template<class TT>
std::pair<std::vector<int>, std::vector<int>> find_initial_weak_or( const TT& tt, const TT& dc, const std::vector<int>& s )
{
  std::vector<int> var_a_vect, var_b_vect;
  for ( auto& i : s )
  {
    std::vector<int> var_a_p( 1, i );
    var_a_p.push_back( i );
    if ( check_weak_decomp( tt, dc, var_a_p ) )
    {
      var_a_vect.push_back( i );
      var_b_vect.push_back( i );
      return { var_a_vect, var_b_vect };
    }
  }
  return { var_a_vect, var_b_vect };
}

template<class TT>
std::pair<std::vector<int>, std::vector<int>> group_variables_or( const TT& tt, const TT& dc, const std::vector<int>& s )
{
  std::vector<int> xa, xb;
  auto var_x = find_initial_or( tt, dc, s );
  xa = var_x.first;
  xb = var_x.second;
  if ( ( xa.size() == 0 ) && ( xb.size() == 0 ) )
  {
    return var_x;
  }
  for ( auto h : s )
  {
    auto it = std::find( xa.begin(), xa.end(), h );
    if ( it != xa.end() )
    {
      continue;
    }
    it = std::find( xb.begin(), xb.end(), h );
    if ( it != xb.end() )
    {
      continue;
    }

    if ( xa.size() <= xb.size() )
    {
      auto xa_p = xa;
      auto xb_p = xb;
      xa_p.push_back( h );
      xb_p.push_back( h );
      if ( check_or_decomp( tt, dc, xa_p, xb ) )
      {
        xa.push_back( h );
      }
      else if ( check_or_decomp( tt, dc, xa, xb_p ) )
      {
        xb.push_back( h );
      }
    }
    else
    {
      auto xa_p = xa;
      auto xb_p = xb;
      xa_p.push_back( h );
      xb_p.push_back( h );
      if ( check_or_decomp( tt, dc, xa, xb_p ) )
      {
        xb.push_back( h );
      }
      else if ( check_or_decomp( tt, dc, xa_p, xb ) )
      {
        xa.push_back( h );
      }
    }
  }
  return { xa, xb };
}

template<class TT>
std::pair<std::vector<int>, std::vector<int>> group_variables_weak_or( const TT& tt, const TT& dc, const std::vector<int>& s )
{
  std::vector<int> xa, xb;
  auto var_x = find_initial_weak_or( tt, dc, s );
  xa = var_x.first;
  xb = var_x.second;
  if ( ( xa.size() == 0 ) && ( xb.size() == 0 ) )
  {
    return var_x;
  }
  for ( auto h : s )
  {
    auto it = std::find( xa.begin(), xa.end(), h );
    if ( it != xa.end() )
    {
      continue;
    }
    auto xa_p = xa;
    xa_p.push_back( h );
    if ( check_weak_decomp( tt, dc, xa_p ) )
    {
      xa.push_back( h );
    }
  }
  return { xa, xb };
}

template<class TT>
std::tuple<std::vector<int>, std::vector<int>, std::vector<TT>> group_variables_xor( const TT& tt, const TT& dc, const std::vector<int>& s )
{
  std::vector<int> xa, xb;
  std::vector<TT> q_and_r;
  auto var_x = find_initial_xor( tt, dc, s );
  xa = std::get<0>( var_x );
  xb = std::get<1>( var_x );
  q_and_r = std::get<2>( var_x );
  if ( ( xa.size() == 0 ) && ( xb.size() == 0 ) )
  {
    return var_x;
  }
  for ( auto h : s )
  {
    auto it = std::find( xa.begin(), xa.end(), h );
    if ( it != xa.end() )
    {
      continue;
    }
    it = std::find( xb.begin(), xb.end(), h );
    if ( it != xb.end() )
    {
      continue;
    }

    if ( xa.size() <= xb.size() )
    {
      auto xa_p = xa;
      auto xb_p = xb;
      xa_p.push_back( h );
      xb_p.push_back( h );
      auto f = check_xor_decomp( tt, dc, xa_p, xb );
      auto f2 = check_xor_decomp( tt, dc, xa, xb_p );
      if ( f.first )
      {
        xa.push_back( h );
        q_and_r = f.second;
      }
      else if ( f2.first )
      {
        xb.push_back( h );
        q_and_r = f2.second;
      }
    }
    else
    {
      auto xa_p = xa;
      auto xb_p = xb;
      xa_p.push_back( h );
      xb_p.push_back( h );
      auto f = check_xor_decomp( tt, dc, xa_p, xb );
      auto f2 = check_xor_decomp( tt, dc, xa, xb_p );
      if ( f2.first )
      {
        xb.push_back( h );
        q_and_r = f2.second;
      }
      else if ( f.first )
      {
        xa.push_back( h );
        q_and_r = f.second;
      }
    }
  }
  return std::make_tuple( xa, xb, q_and_r );
}

inline std::tuple<std::vector<int>, std::vector<int>, bi_decomposition> best_variable_grouping( const std::pair<std::vector<int>, std::vector<int>>& x_or, const std::pair<std::vector<int>, std::vector<int>>& x_and, const std::pair<std::vector<int>, std::vector<int>>& x_xor, bool xor_cost )
{
  if ( xor_cost )
  {
    if ( ( x_xor.first.size() != 0 ) && ( x_xor.second.size() != 0 ) )
    {
      return std::make_tuple( x_xor.first, x_xor.second, bi_decomposition::xor_ );
    }
  }

  int diff_or = static_cast<int>( x_or.first.size() ) - static_cast<int>( x_or.second.size() );
  if ( ( x_or.first.size() == 0 ) || ( x_or.second.size() == 0 ) )
  {
    diff_or = 100;
  }
  if ( diff_or < 0 )
  {
    diff_or = -diff_or;
  }

  int diff_and = static_cast<int>( x_and.first.size() ) - static_cast<int>( x_and.second.size() );
  if ( ( x_and.first.size() == 0 ) || ( x_and.second.size() == 0 ) )
  {
    diff_and = 100;
  }
  if ( diff_and < 0 )
  {
    diff_and = -diff_and;
  }

  int diff_xor = static_cast<int>( x_xor.first.size() ) - static_cast<int>( x_xor.second.size() );
  if ( ( x_xor.first.size() == 0 ) || ( x_xor.second.size() == 0 ) )
  {
    diff_xor = 100;
  }
  if ( diff_xor < 0 )
  {
    diff_xor = -diff_xor;
  }
  if ( ( diff_or == 100 ) && ( diff_and == 100 ) && ( diff_xor == 100 ) )
  {
    return std::make_tuple( x_or.first, x_or.second, bi_decomposition::none );
  }
  if ( ( diff_xor <= diff_and ) && ( diff_xor <= diff_or ) )
  {
    return std::make_tuple( x_xor.first, x_xor.second, bi_decomposition::xor_ );
  }
  else if ( ( diff_and <= diff_xor ) && ( diff_and <= diff_or ) )
  {
    return std::make_tuple( x_and.first, x_and.second, bi_decomposition::and_ );
  }
  else // if ( ( diff_xor <= diff_or ) && ( diff_xor <= diff_and ) )
  {
    return std::make_tuple( x_or.first, x_or.second, bi_decomposition::or_ );
  }
}

template<typename TT>
std::vector<TT> derive_b( const TT& tt, const TT& dc, const std::tuple<std::vector<int>, std::vector<int>, bi_decomposition>& x_best, const TT& fa )
{
  std::vector<TT> qb;
  if ( ( std::get<2>( x_best ) == bi_decomposition::or_ ) || ( std::get<2>( x_best ) == bi_decomposition::weak_or_ ) )
  {
    auto rb = exist_set( binary_and( ~tt, dc ), std::get<0>( x_best ) );
    qb.push_back( exist_set( binary_and( binary_and( tt, dc ), ~fa ), std::get<0>( x_best ) ) );
    assert( !is_const0( rb ) );
    qb.push_back( binary_or( qb[0], rb ) );
    return qb;
  }
  else // (( std::get<2>( x_best ) == bi_decomposition::and_) || ( std::get<2>( x_best ) == bi_decomposition::weak_and_) )
  {
    qb.push_back( exist_set( binary_and( tt, dc ), std::get<0>( x_best ) ) );
    auto rb = exist_set( binary_and( binary_and( ~tt, dc ), ~fa ), std::get<0>( x_best ) );
    qb.push_back( binary_or( qb[0], rb ) );
    assert( !is_const0( qb[0] ) );
    return qb;
  }
}

template<class TT>
std::vector<TT> derive_a( const TT& tt, const TT& dc, const std::tuple<std::vector<int>, std::vector<int>, bi_decomposition>& x_best )
{
  std::vector<TT> qa;
  if ( std::get<2>( x_best ) == bi_decomposition::or_ )
  {
    qa.push_back( exist_set( binary_and( binary_and( tt, dc ), exist_set( binary_and( ~tt, dc ), std::get<0>( x_best ) ) ), std::get<1>( x_best ) ) );
    auto ra = exist_set( binary_and( ~tt, dc ), std::get<1>( x_best ) );
    qa.push_back( binary_or( qa[0], ra ) );
    return qa;
  }
  else if ( std::get<2>( x_best ) == bi_decomposition::and_ )
  {
    auto ra = exist_set( binary_and( binary_and( ~tt, dc ), exist_set( binary_and( tt, dc ), std::get<0>( x_best ) ) ), std::get<1>( x_best ) );
    qa.push_back( exist_set( binary_and( tt, dc ), std::get<1>( x_best ) ) );
    qa.push_back( binary_or( qa[0], ra ) );
    return qa;
  }
  else if ( std::get<2>( x_best ) == bi_decomposition::weak_or_ )
  {
    qa.push_back( binary_and( binary_and( tt, dc ), exist_set( binary_and( ~tt, dc ), std::get<0>( x_best ) ) ) );
    auto ra = binary_and( ~tt, dc );
    qa.push_back( binary_or( qa[0], ra ) );
    return qa;
  }
  else // if ( std::get<2>( x_best ) == bi_decomposition::weak_and_ )
  {
    qa.push_back( binary_and( tt, dc ) );
    auto ra = binary_and( binary_and( ~tt, dc ), exist_set( binary_and( tt, dc ), std::get<0>( x_best ) ) );
    qa.push_back( binary_or( qa[0], ra ) );
    return qa;
  }
}

template<class TT>
std::tuple<TT, bi_decomposition, std::vector<TT>> is_bi_decomposable( const TT& tt, const TT& dc, bool cost )
{

  auto fa = tt.construct();
  auto fb = tt.construct();
  std::vector<TT> qa, qb;

  if ( is_const0( dc ) )
  {
    return std::make_tuple( dc, bi_decomposition::none, qa );
  }

  std::vector<int> support;
  for ( auto x = 0u; x < tt.num_vars(); x++ )
  {
    if ( has_var( binary_and( tt, dc ), x ) )
    {
      support.push_back( x );
    }
  }

  if ( support.size() <= 1 )
  {
    return std::make_tuple( tt, bi_decomposition::none, qa );
  }

  auto x_or = detail::group_variables_or( tt, dc, support );
  auto x_and = detail::group_variables_or( ~tt, dc, support );
  auto x_xor = detail::group_variables_xor( tt, dc, support );
  auto x_best = detail::best_variable_grouping( x_or, x_and, std::make_pair( std::get<0>( x_xor ), std::get<1>( x_xor ) ), cost );

  if ( std::get<2>( x_best ) == bi_decomposition::none )
  {
    x_or = detail::group_variables_weak_or( tt, dc, support );
    if ( x_or.first.size() == 0 )
    {
      x_and = detail::group_variables_weak_or( ~tt, dc, support );
      x_best = make_tuple( x_and.first, x_and.second, bi_decomposition::weak_and_ );
    }
    else
      x_best = make_tuple( x_or.first, x_or.second, bi_decomposition::weak_or_ );
  }
  else if ( std::get<2>( x_best ) == bi_decomposition::xor_ )
  {
    fa = std::get<2>( x_xor )[0];
    fb = std::get<2>( x_xor )[2];

    qa = std::get<2>( x_xor );
    qa[1] = binary_or( qa[1], qa[0] );
    qa[3] = binary_or( qa[3], qa[2] );
    return std::make_tuple( binary_xor( fa, fb ), bi_decomposition::xor_, qa );
  }

  qa = detail::derive_a( tt, dc, x_best );
  if ( ( std::get<2>( x_best ) == bi_decomposition::or_ ) || ( std::get<2>( x_best ) == bi_decomposition::weak_or_ ) )
  {
    qb = detail::derive_b( tt, dc, x_best, qa[0] );
  }
  else
  {
    qb = detail::derive_b( tt, dc, x_best, binary_and( ~qa[0], qa[1] ) );
  }

  fa = qa[0];
  fb = qb[0];

  qa.push_back( qb[0] );
  qa.push_back( qb[1] );

  if ( ( std::get<2>( x_best ) == bi_decomposition::and_ ) || ( std::get<2>( x_best ) == bi_decomposition::weak_and_ ) )
  {
    return std::make_tuple( binary_and( fa, fb ), std::get<2>( x_best ), qa );
  }
  else
  {
    return std::make_tuple( binary_or( fa, fb ), std::get<2>( x_best ), qa );
  }
}

} /* namespace detail */
/* \endcond */

/*! \brief Checks whether a function is bi-decomposable.

  \verbatim embed:rst
  Checks whether an incompletely specified function (ISF) ``tt`` can be represented by the function
  :math:`f = h(l(X_1, X_3), g(X_2, X_3))`.
  It returns a tuple of:
  1. :math:'f', which is a completely specified Boolean function compatible with the input ISF
  2. :math:'h', which is the type of decomposition (and, or, xor, weak and, weak or)
  3. :math:'l' and :math:'g', given as ISF (ON-set and DC-set)

  The algorithm is inspired by "An Algorithm for Bi-Decomposition of Logic Functions" by A. Mishchenko et al.
  presented in DAC 2001.

  \endverbatim

  \param tt ON-set of the input function \f$f\f$
  \param dc DC-set of the input function \f$f\f$
*/

template<class TT>
std::tuple<TT, bi_decomposition, std::vector<TT>> is_bi_decomposable( const TT& tt, const TT& dc )
{
  return detail::is_bi_decomposable( tt, dc, false );
}

/*! \brief Checks whether a function is bi-decomposable using XOR as preferred operation.

  \verbatim embed:rst
  Checks whether an incompletely specified function (ISF) ``tt`` can be represented by the function
  :math:`f = h(l(X_1, X_3), g(X_2, X_3))`.
  It returns a tuple of:
  1. :math:'f', which is a completely specified Boolean function compatible with the input ISF
  2. :math:'h', which is the type of decomposition (and, or, xor, weak and, weak or)
  3. :math:'l' and :math:'g', given as ISF (ON-set and DC-set)

  The algorithm is inspired by "An Algorithm for Bi-Decomposition of Logic Functions" by A. Mishchenko et al.
  presented in DAC 2001.

  The cost is changed to add more XOR gates compared to AND/OR gates. This cost function is motivated by
  minimizing the number of AND gates in XAGs for cryptography and security applications. For these applications
  XOR gates are "free".

  \endverbatim

  \param tt ON-set of the input function \f$f\f$
  \param dc DC-set of the input function \f$f\f$
*/

template<class TT>
std::tuple<TT, bi_decomposition, std::vector<TT>> is_bi_decomposable_mc( const TT& tt, const TT& dc )
{
  return detail::is_bi_decomposable( tt, dc, true );
}

namespace detail
{

/*! \brief A helper function to enumerate missing indices.

  \param ys_index A list of already selected indices
  \param max_index The maximum value for an index
  \return Remaining indices
*/
inline std::vector<uint32_t> enumerate_zs_index( const std::vector<uint32_t>& ys_index, uint32_t max_index )
{
  std::vector<uint32_t> zs_index;
  for ( uint32_t i = 0u; i <= max_index; ++i )
  {
    if ( std::find( ys_index.begin(), ys_index.end(), i ) == ys_index.end() )
    {
      zs_index.push_back( i );
    }
  }

  return zs_index;
}

} // namespace detail

/*! \brief Checks, whether a function is Ashenhurst decomposable.

  Given functions f(.), g(.), and h(.) and a partition
  on arguments into z and y. This function determines whether
  f(x) is decomposable into g(z, h(y)) where x = union(z,y) and
  intersect(z, y) = null.
  This function does not check for permutation of variables given by
  zs_index and ys_index. The elements in these vectors are treated as ordered
  values.

  \param tt The function to the check the decomposition on (function f)
  \param zs_index The ordered set of indices of vector x (input to f) that
                  are the inputs to outer_func (g).
  \param ys_index The ordered set of indices of vector x (input to f) that are
                  input to the inner_func (h).
  \param outer_func The outer decomposition function (function g).
  \param inner_func The inner decomposition function (function h).
  \return true if the given decomposition is a valid one, false otherwise.
*/
template<class TTf, class TTg, class TTh>
bool is_ashenhurst_decomposable( const TTf& tt,
                                 const std::vector<uint32_t>& zs_index,
                                 const std::vector<uint32_t>& ys_index,
                                 const TTg& outer_func,
                                 const TTh& inner_func )
{
  static_assert( is_complete_truth_table<TTf>::value, "Can only be applied on complete truth tables." );
  static_assert( is_complete_truth_table<TTg>::value, "Can only be applied on complete truth tables." );
  static_assert( is_complete_truth_table<TTh>::value, "Can only be applied on complete truth tables." );

  std::vector<TTf> y_vars;
  std::vector<TTf> z_vars;

  for ( const auto idx : ys_index )
  {
    auto var = tt.construct();
    create_nth_var( var, idx );
    y_vars.push_back( var );
  }
  for ( const auto idx : zs_index )
  {
    auto var = tt.construct();
    create_nth_var( var, idx );
    z_vars.push_back( var );
  }
  auto h = compose_truth_table( inner_func, y_vars );
  z_vars.push_back( h );
  auto f = compose_truth_table( outer_func, z_vars );
  return equal( f, tt );
}

/*! \brief Finds all of the possible Ashenhurst decompositions of a function
  given an input partitioning.

  \param tt The function to find all of its decompositions
  \param ys_index Indices indicating the partitioning of inputs
  \param decomposition A vector of decomposition pairs. This serves as a return
                       return container.
  \return Returns the number of possible decompositions.
*/
template<class TTf, class TTg, class TTh>
uint32_t ashenhurst_decomposition( const TTf& tt, const std::vector<uint32_t>& ys_index, std::vector<std::pair<TTg, TTh>>& decomposition )
{
  static_assert( is_complete_truth_table<TTf>::value, "Can only be applied on complete truth tables." );
  static_assert( is_complete_truth_table<TTg>::value, "Can only be applied on complete truth tables." );
  static_assert( is_complete_truth_table<TTh>::value, "Can only be applied on complete truth tables." );

  std::vector<uint32_t> zs_index = detail::enumerate_zs_index( ys_index, tt.num_vars() - 1 );
  decomposition.clear();

  // TODO: this does not work for dynamic_truth_table (number of variables not known)
  TTg g;
  do
  {
    TTh h;
    do
    {
      if ( is_ashenhurst_decomposable( tt, zs_index, ys_index, g, h ) )
      {
        decomposition.emplace_back( g, h );
      }
      next_inplace( h );
    } while ( !is_const0( h ) );
    next_inplace( g );
  } while ( !is_const0( g ) );
  return static_cast<uint32_t>( decomposition.size() );
}

} // namespace kitty