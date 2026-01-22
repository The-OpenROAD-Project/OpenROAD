/* kitty: C++ truth table library
 * Copyright (C) 2017-2025  EPFL
 * Copyright (C) 2017-2025  University of Victoria
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
  \file spectral.hpp
  \brief Implements methods for spectral classification

  Original implementation by D. Michael Miller (University of
  Victoria, BC, Canada).  Modified C++ implementation and integration
  into kitty by Mathias Soeken.

  \author D. Michael Miller
  \author Mathias Soeken
*/

#pragma once

#include "bit_operations.hpp"
#include "constructors.hpp"
#include "esop.hpp"
#include "detail/mscfix.hpp"
#include "traits.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <unordered_map>

namespace kitty
{

namespace detail
{
struct spectral_operation
{
  enum class kind : uint16_t
  {
    none,
    permutation,
    input_negation,
    output_negation,
    spectral_translation,
    disjoint_translation
  };

  spectral_operation() = default;
  explicit spectral_operation( kind _kind, uint16_t _var1 = 0, uint16_t _var2 = 0 ) : _kind( _kind ), _var1( _var1 ), _var2( _var2 ) {}

  kind _kind{ kind::none };
  uint16_t _var1{ 0 };
  uint16_t _var2{ 0 };
};

inline void fast_hadamard_transform( std::vector<int32_t>& s, bool reverse = false )
{
  unsigned k{};
  int t{};

  for ( auto m = 1u; m < s.size(); m <<= 1u )
  {
    for ( auto i = 0u; i < s.size(); i += ( m << 1u ) )
    {
      for ( auto j = i, p = k = i + m; j < p; ++j, ++k )
      {
        t = s[j];
        s[j] += s[k];
        s[k] = t - s[k];
      }
    }
  }

  if ( reverse )
  {
    for ( auto i = 0u; i < s.size(); ++i )
    {
      s[i] /= static_cast<int>( s.size() );
    }
  }
}

class spectrum
{
public:
  spectrum() = delete;
  ~spectrum() = default;

  spectrum( const spectrum& other ) : _s( std::begin( other._s ), std::end( other._s ) ) {}
  spectrum( spectrum&& other ) noexcept : _s( std::move( other._s ) ) {}

  spectrum& operator=( const spectrum& other )
  {
    if ( this != &other )
    {
      _s = other._s;
    }
    return *this;
  }

  spectrum& operator=( spectrum&& other ) noexcept
  {
    _s = std::move( other._s );
    return *this;
  }

private:
  explicit spectrum( std::vector<int32_t> _s ) : _s( std::move( _s ) ) {}

public:
  template<typename TT>
  static spectrum from_truth_table( const TT& tt )
  {
    std::vector<int32_t> _s( tt.num_bits(), 1 );
    for_each_one_bit( tt, [&_s]( auto bit )
                      { _s[bit] = -1; } );
    fast_hadamard_transform( _s );
    return spectrum( _s );
  }

  template<typename TT>
  void to_truth_table( TT& tt ) const
  {
    auto copy = _s;
    fast_hadamard_transform( copy, true );

    clear( tt );
    for ( auto i = 0u; i < copy.size(); ++i )
    {
      if ( copy[i] == -1 )
      {
        set_bit( tt, i );
      }
    }
  }

  auto permutation( unsigned i, unsigned j )
  {
    spectral_operation op( spectral_operation::kind::permutation, i, j );

    for ( auto k = 0u; k < _s.size(); ++k )
    {
      if ( ( k & i ) > 0 && ( k & j ) == 0 )
      {
        std::swap( _s[k], _s[k - i + j] );
      }
    }

    return op;
  }

  auto input_negation( unsigned i )
  {
    spectral_operation op( spectral_operation::kind::input_negation, i );
    for ( auto k = 0u; k < _s.size(); ++k )
    {
      if ( ( k & i ) > 0 )
      {
        _s[k] = -_s[k];
      }
    }

    return op;
  }

  auto output_negation()
  {
    for ( auto& coeff : _s )
    {
      coeff = -coeff;
    }
    return spectral_operation( spectral_operation::kind::output_negation );
  }

  auto spectral_translation( int i, int j )
  {
    spectral_operation op( spectral_operation::kind::spectral_translation, i, j );

    for ( auto k = 0u; k < _s.size(); ++k )
    {
      if ( ( k & i ) > 0 && ( k & j ) == 0 )
      {
        std::swap( _s[k], _s[k + j] );
      }
    }

    return op;
  }

  auto disjoint_translation( int i )
  {
    spectral_operation op( spectral_operation::kind::disjoint_translation, i );

    for ( auto k = 0u; k < _s.size(); ++k )
    {
      if ( ( k & i ) > 0 )
      {
        std::swap( _s[k], _s[k - i] );
      }
    }

    return op;
  }

  void apply( const spectral_operation& op )
  {
    switch ( op._kind )
    {
    case spectral_operation::kind::none:
      assert( false );
      break;
    case spectral_operation::kind::permutation:
      permutation( op._var1, op._var2 );
      break;
    case spectral_operation::kind::input_negation:
      input_negation( op._var1 );
      break;
    case spectral_operation::kind::output_negation:
      output_negation();
      break;
    case spectral_operation::kind::spectral_translation:
      spectral_translation( op._var1, op._var2 );
      break;
    case spectral_operation::kind::disjoint_translation:
      disjoint_translation( op._var1 );
      break;
    }
  }

  inline auto operator[]( std::vector<int32_t>::size_type pos )
  {
    return _s[pos];
  }

  inline auto operator[]( std::vector<int32_t>::size_type pos ) const
  {
    return _s[pos];
  }

  inline auto size() const
  {
    return _s.size();
  }

  inline auto cbegin() const
  {
    return _s.cbegin();
  }

  inline auto cend() const
  {
    return _s.cend();
  }

  void print( std::ostream& os, const std::vector<uint32_t>& order ) const
  {
    os << std::setw( 4 ) << _s[order.front()];

    for ( auto it = order.begin() + 1; it != order.end(); ++it )
    {
      os << " " << std::setw( 4 ) << _s[*it];
    }
  }

  inline const auto& coefficients() const
  {
    return _s;
  }

private:
  std::vector<int32_t> _s;
};

inline std::vector<uint32_t> get_rw_coeffecient_order( uint32_t num_vars )
{
  auto size = uint32_t( 1 ) << num_vars;
  std::vector<uint32_t> map( size, 0u );
  auto p = std::begin( map ) + 1;

  for ( uint32_t i = 1u; i <= num_vars; ++i )
  {
    for ( uint32_t j = 1u; j < size; ++j )
    {
      if ( __builtin_popcount( j ) == static_cast<int>( i ) )
      {
        *p++ = j;
      }
    }
  }

  return map;
}

template<typename TT>
class miller_spectral_canonization_impl
{
public:
  explicit miller_spectral_canonization_impl( const TT& func, bool hasInputNegation = true, bool hasOutputNegation = true, bool hasDisjointTranslation = true )
      : func( func ),
        num_vars( func.num_vars() ),
        num_vars_exp( 1 << num_vars ),
        spec( spectrum::from_truth_table( func ) ),
        best_spec( spec ),
        transforms( 100u ),
        hasInputNegation( hasInputNegation ),
        hasOutputNegation( hasOutputNegation ),
        hasDisjointTranslation( hasDisjointTranslation )
  {
  }

  template<typename Callback>
  std::pair<TT, bool> run( Callback&& fn )
  {
    order = get_rw_coeffecient_order( num_vars );
    const auto exact = normalize();

    fn( best_transforms );

    TT tt = func.construct();
    spec.to_truth_table<TT>( tt );
    return { tt, exact };
  }

  void set_limit( unsigned limit )
  {
    step_limit = limit;
  }

private:
  unsigned transformation_costs( const std::vector<spectral_operation>& transforms )
  {
    auto costs = 0u;
    for ( const auto& t : transforms )
    {
      costs += ( t._kind == spectral_operation::kind::permutation ) ? 3u : 1u;
    }
    return costs;
  }

  void closer( spectrum& lspec )
  {
    for ( auto i = 0u; i < lspec.size(); ++i )
    {
      const auto j = order[i];
      if ( lspec[j] == best_spec[j] )
      {
        continue;
      }
      if ( abs( lspec[j] ) > abs( best_spec[j] ) ||
           ( abs( lspec[j] ) == abs( best_spec[j] ) && lspec[j] > best_spec[j] ) )
      {
        update_best( lspec );
        return;
      }

      if ( abs( lspec[j] ) < abs( best_spec[j] ) ||
           ( abs( lspec[j] ) == abs( best_spec[j] ) && lspec[j] < best_spec[j] ) )
      {
        return;
      }
    }

    if ( transformation_costs( transforms ) < transformation_costs( best_transforms ) )
    {
      update_best( lspec );
    }
  }

  bool normalize_rec( spectrum& lspec, unsigned v )
  {
    if ( ++step_counter == step_limit )
      return false;

    if ( v == num_vars_exp ) /* leaf case */
    {
      /* invert function if necessary */
      if ( hasOutputNegation && lspec[0u] < 0 )
      {
        insert( lspec.output_negation() );
      }
      /* invert any variable as necessary */
      if ( hasInputNegation )
      {
        for ( auto i = 1u; i < num_vars_exp; i <<= 1 )
        {
          if ( lspec[i] < 0 )
          {
            insert( lspec.input_negation( i ) );
          }
        }
      }

      closer( lspec );
      return true;
    }

    auto min = 0, max = 0;
    const auto p = std::accumulate( lspec.cbegin() + v, lspec.cend(),
                                    std::make_pair( min, max ),
                                    []( auto a, auto sv )
                                    {
                                      return std::make_pair( std::min( a.first, abs( sv ) ), std::max( a.second, abs( sv ) ) );
                                    } );
    min = p.first;
    max = p.second;

    if ( max == 0 )
    {
      auto& spec2 = specs.at( num_vars_exp );
      spec2 = lspec;
      if ( !normalize_rec( spec2, num_vars_exp ) )
        return false;
    }
    else
    {
      for ( auto i = 1u; i < lspec.size(); ++i )
      {
        auto j = order[i];
        if ( abs( lspec[j] ) != max )
        {
          continue;
        }

        /* k = first one bit in j starting from pos v */
        auto k = j & ~( v - 1 ); /* remove 1 bits until v */
        if ( k == 0 )
        {
          continue; /* are there bit left? */
        }
        k = k - ( k & ( k - 1 ) ); /* extract lowest bit */
        j ^= k;                    /* remove bit k from j */

        auto& spec2 = specs.at( v << 1 );
        spec2 = lspec;

        const auto save = transform_index;

        /* spectral translation to all other 1s in j */
        while ( j )
        {
          auto p = j - ( j & ( j - 1 ) );
          insert( spec2.spectral_translation( k, p ) );
          j ^= p;
        }

        if ( k != v )
        {
          insert( spec2.permutation( k, v ) );
        }

        if ( !normalize_rec( spec2, v << 1 ) )
          return false;

        if ( v == 1 && min == max )
        {
          return true;
        }
        transform_index = save;
      }
    }

    return true;
  }

  bool normalize()
  {
    /* find maximum absolute element index in spectrum (by order) */
    auto j = *std::max_element( order.cbegin(), order.cend(), [this]( auto p1, auto p2 )
                                { return abs( spec[p1] ) < abs( spec[p2] ); } );

    /* if max element is not the first element */
    if ( j )
    {
      auto k = j - ( j & ( j - 1 ) ); /* LSB of j */
      j ^= k;                         /* delete bit in j */

      while ( j )
      {
        auto p = j - ( j & ( j - 1 ) ); /* next LSB of j */
        j ^= p;                         /* delete bit in j */
        insert( spec.spectral_translation( k, p ) );
      }
      if ( hasDisjointTranslation )
      {
        insert( spec.disjoint_translation( k ) );
      }
    }

    for ( auto v = 1u; v <= num_vars_exp; v <<= 1 )
    {
      specs.insert( { v, spec } );
    }

    update_best( spec );
    const auto result = normalize_rec( spec, 1 );
    spec = best_spec;
    return result;
  }

  void insert( const spectral_operation& trans )
  {
    if ( transform_index >= transforms.size() )
    {
      transforms.resize( transforms.size() << 1 );
    }
    assert( transform_index < transforms.size() );
    transforms[transform_index++] = trans;
  }

  void update_best( const spectrum& lspec )
  {
    best_spec = lspec;
    best_transforms.resize( transform_index );
    std::copy( transforms.begin(), transforms.begin() + transform_index, best_transforms.begin() );
  }

  std::ostream& print_spectrum( std::ostream& os )
  {
    os << "[i]";

    for ( auto i = 0u; i < spec.size(); ++i )
    {
      auto j = order[i];
      if ( j > 0 && __builtin_popcount( order[i - 1] ) < __builtin_popcount( j ) )
      {
        os << " |";
      }
      os << " " << std::setw( 3 ) << spec[j];
    }
    return os << std::endl;
  }

private:
  const TT& func;

  unsigned num_vars;
  unsigned num_vars_exp;
  spectrum spec;
  spectrum best_spec;
  std::unordered_map<uint64_t, spectrum> specs;

  std::vector<uint32_t> order;
  std::vector<spectral_operation> transforms;
  std::vector<spectral_operation> best_transforms;
  unsigned transform_index{ 0u };

  unsigned step_counter{ 0u };
  unsigned step_limit{ 0u };

  bool hasInputNegation{ true };
  bool hasOutputNegation{ true };
  bool hasDisjointTranslation{ true };
};

inline void exact_spectral_canonization_null_callback( const std::vector<spectral_operation>& operations )
{
  (void)operations;
}
} /* namespace detail */

/*! \brief Exact spectral canonization

  The function can be passed as second argument a callback that is called with a
  vector of spectral operations necessary to transform the input function into
  the representative.

  \param tt Truth table
  \param fn Callback to retrieve list of transformations (optional)
 */
template<typename TT, typename Callback = decltype( detail::exact_spectral_canonization_null_callback )>
inline TT exact_spectral_canonization( const TT& tt, Callback&& fn = detail::exact_spectral_canonization_null_callback )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  detail::miller_spectral_canonization_impl<TT> impl( tt );
  return impl.run( fn ).first;
}

/*! \brief Exact spectral canonization (with recursion limit)

  This function gets as additional argument a recursion limit.  The canonization
  is stopped once this limit is reached and the current representative at this
  point is being returned.  The return value is a pair, where the first
  entry is the representative, and the second entry indicates whether the
  solution is known to be exact or not.

  A function can be passed as second argument that is callback called with a
  vector of spectral operations necessary to transform the input function into
  the representative.

  \param tt Truth table
  \param limit Recursion limit
  \param fn Callback to retrieve list of transformations (optional)
 */
template<typename TT, typename Callback = decltype( detail::exact_spectral_canonization_null_callback )>
inline std::pair<TT, bool> exact_spectral_canonization_limit( const TT& tt, unsigned step_limit, Callback&& fn = detail::exact_spectral_canonization_null_callback )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  detail::miller_spectral_canonization_impl<TT> impl( tt );
  impl.set_limit( step_limit );
  return impl.run( fn );
}

namespace detail
{

template<class TT>
struct anf_spectrum
{
public:
  explicit anf_spectrum( const TT& tt )
      : _anf( detail::algebraic_normal_form( tt ) ),
        _khots( tt.num_vars() + 1, tt.construct() )
  {
    for ( auto i = 0u; i <= tt.num_vars(); ++i )
    {
      create_equals( _khots[i], i );
    }
  }

private:
  auto permutation( unsigned i, unsigned j )
  {
    spectral_operation op( spectral_operation::kind::permutation, 1 << i, 1 << j );

    swap_inplace( _anf, i, j );

    return op;
  }

  auto input_negation( unsigned i )
  {
    spectral_operation op( spectral_operation::kind::input_negation, 1 << i );

    _anf ^= cofactor1( _anf, i ) & nth_var<TT>( _anf.num_vars(), i, true );

    return op;
  }

  auto output_negation()
  {
    flip_bit( _anf, 0 );

    return spectral_operation( spectral_operation::kind::output_negation );
  }

  auto spectral_translation( int i, int j )
  {
    spectral_operation op( spectral_operation::kind::spectral_translation, 1 << i, 1 << j );

    _anf ^= ( cofactor1( _anf, i ) ^ cofactor0( cofactor1( _anf, i ), j ) ) & nth_var<TT>( _anf.num_vars(), j ) & nth_var<TT>( _anf.num_vars(), i, true );

    return op;
  }

  auto disjoint_translation( int i )
  {
    spectral_operation op( spectral_operation::kind::disjoint_translation, 1 << i );

    flip_bit( _anf, UINT64_C( 1 ) << i );

    return op;
  }

  template<class Fn>
  void foreach_term_of_size( uint32_t k, Fn&& fn, int64_t start = 0 )
  {
    auto term = find_first_one_bit( _anf & _khots[k], start );

    while ( term != -1 )
    {
      fn( term );
      term = find_first_one_bit( _anf & _khots[k], term + 1 );
    }
  }

  void insert( const spectral_operation& op )
  {
    _transforms.push_back( op );
  }

public:
  bool classify()
  {
    bool repeat = true;
    auto rounds = 0u;
    while ( repeat && rounds++ < 2u )
    {
      if ( count_ones( _anf ) < 2 )
      {
        break;
      }

      repeat = false;
      for ( auto k = 2u; k < _anf.num_vars(); ++k )
      {
        foreach_term_of_size( k, [&]( auto term )
                              {
          auto mask = _anf.construct();
          create_from_cubes( mask, {cube( static_cast<uint32_t>( term ), static_cast<uint32_t>( term ) )} );

          const auto i = find_first_one_bit( _anf & _khots[k + 1] & mask );
          if ( i != -1 )
          {
            insert( input_negation( detail::log2[i & ~term] ) );
            repeat = true;
          }
          else
          {
            foreach_term_of_size( k, [&]( auto term2 ) {
              if ( __builtin_popcount( static_cast<uint32_t>( term ^ term2 ) ) == 2 )
              {
                const auto vari = detail::log2[term & ~term2];
                const auto varj = detail::log2[term2 & ~term];

                insert( spectral_translation( vari, varj ) );
                repeat = true;
              }
            }, term + 1 );
          } } );
      }
    }

    if ( get_bit( _anf, 0 ) )
    {
      insert( output_negation() );
    }

    for ( auto i = 0u; i < _anf.num_vars(); ++i )
    {
      if ( get_bit( _anf, UINT64_C( 1 ) << i ) )
      {
        insert( disjoint_translation( i ) );
      }
    }

    auto mask = 0u;
    bool disjoint = true;
    for_each_one_bit( _anf, [&]( auto word )
                      {
      if ( word & mask ) {
        disjoint = false;
      } else {
        mask |= word;
      } } );

    if ( disjoint )
    {
      auto dest_var = 0;
      for ( auto k = 2u; k < _anf.num_vars(); ++k )
      {
        foreach_term_of_size( k, [&]( auto term )
                              {
          while ( term != 0 )
          {
            const auto vari = detail::log2[term & ~( term - 1 )];

            // swap i with dest_var
            if ( vari != dest_var )
            {
              insert( permutation( vari, dest_var ) );
            }
            ++dest_var;

            term &= ( term - 1 );
          } } );
      }

      return true;
    }

    return false;
  }

  const TT& anf() const
  {
    return _anf;
  }

  TT truth_table() const
  {
    return detail::algebraic_normal_form( _anf );
  }

  template<class Callback>
  void get_transformations( Callback&& fn ) const
  {
    fn( _transforms );
  }

private:
  TT _anf;
  std::vector<TT> _khots;
  std::vector<spectral_operation> _transforms;
};

} // namespace detail

/*! \brief Exact spectral canonization (with Reed-Muller preprocessor)

  A function can be passed as second argument that is callback called with a
  vector of spectral operations necessary to transform the input function into
  the representative.

  The algorithm is based on the paper: M. Soeken, E. Testa, D.M. Miller: A
  hybrid spectral method for checking Boolean function equivalence, PACRIM 2019.

  \param tt Truth table
  \param fn Callback to retrieve list of transformations (optional)
 */
template<typename TT, typename Callback = decltype( detail::exact_spectral_canonization_null_callback )>
inline TT hybrid_exact_spectral_canonization( const TT& tt, Callback&& fn = detail::exact_spectral_canonization_null_callback )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  (void)fn;

  std::vector<detail::spectral_operation> transforms;

  detail::anf_spectrum<TT> anf( tt );

  bool is_disjoint = anf.classify();
  anf.get_transformations( [&]( const auto& t )
                           { std::copy( t.begin(), t.end(), std::back_inserter( transforms ) ); } );

  if ( !is_disjoint )
  {
    const auto miller_repr = exact_spectral_canonization( anf.truth_table(), [&]( const auto& t )
                                                          { std::copy( t.begin(), t.end(), std::back_inserter( transforms ) ); } );
    detail::anf_spectrum<TT> anf_post( miller_repr );
    anf_post.classify();
    anf_post.get_transformations( [&]( const auto& t )
                                  { std::copy( t.begin(), t.end(), std::back_inserter( transforms ) ); } );
    fn( transforms );
    return anf_post.truth_table();
  }
  else
  {
    fn( transforms );
    return anf.truth_table();
  }
}

/*! \brief Print spectral representation of a function in RW order

  \param tt Truth table
  \param os Output stream
 */
template<typename TT>
inline void print_spectrum( const TT& tt, std::ostream& os = std::cout )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto spectrum = detail::spectrum::from_truth_table( tt );
  spectrum.print( os, detail::get_rw_coeffecient_order( tt.num_vars() ) );
}

/*! \brief Returns the Rademacher-Walsh spectrum of a truth table

  The order of coefficients is in the same order as input assignments
  to the truth table.

  \param tt Truth table
 */
template<typename TT>
inline std::vector<int32_t> rademacher_walsh_spectrum( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  const auto spectrum = detail::spectrum::from_truth_table( tt );
  return spectrum.coefficients();
}

/*! \brief Returns the autocorrelation spectrum of a truth table

  The autocorrelation spectrum gives an indication of the imbalance of all first
  order derivates of a Boolean function.

  The spectral coefficient \f$r(x)\f$ for input \f$x \in \{0,1\}^n\f$ is
  computed by \f$r(x) = \sum_{y\in\{0,1\}^n}\hat f(y)\hat f(x \oplus y)\f$,
  where \f$\hat f\f$ is the \f$\{-1,1\}\f$ encoding of the input function
  \f$f\f$.

  \param tt Truth table for input function \f$f\f$
 */
template<typename TT>
inline std::vector<int32_t> autocorrelation_spectrum( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  std::vector<int32_t> spectrum( 1, static_cast<uint32_t>( tt.num_bits() ) );
  spectrum.reserve( tt.num_bits() );

  for ( uint64_t i = 1; i < tt.num_bits(); ++i )
  {
    int32_t sum{ 0 };
    for ( uint64_t j = 0; j < tt.num_bits(); ++j )
    {
      sum += ( get_bit( tt, j ) ? -1 : 1 ) * ( get_bit( tt, i ^ j ) ? -1 : 1 );
    }

    spectrum.push_back( sum );
  }

  return spectrum;
}

/*! \brief Returns distribution of absolute spectrum coefficients

  This functions returns a vector with \f$2^{n-1} + 1\f$ nonnegative entries,
  in which the entry at position \f$i\f$ indicates how many coeffecients in
  the spectum have absolute value \f$2i\f$.  The compression is possible, since
  all spectra in this package have positive coefficients.

  \param spectrum Spectrum
*/
inline std::vector<uint32_t> spectrum_distribution( const std::vector<int32_t>& spectrum )
{
  std::vector<uint32_t> dist( spectrum.size() / 2 + 1 );

  for ( auto c : spectrum )
  {
    dist[abs( c ) >> 1]++;
  }

  return dist;
}

/*! \brief Returns unique index for a spectral equivalence class

  This functions works for functions with up to 5 inputs.  It uses the
  distribution of coefficients in the Rademacher Walsh spectrum, and in case
  of ambiguity also the coeffcienents in the auto-correlation spectrum.  It
  does not compute the transformation sequences in order to get to the
  class representative and is therefore faster than canoninization.

  \param tt Truth table
*/
template<typename TT>
inline uint32_t get_spectral_class( const TT& tt )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  assert( tt.num_vars() <= 5 );

  const auto rwd = spectrum_distribution( rademacher_walsh_spectrum( tt ) );

  switch ( tt.num_vars() )
  {
  case 0u:
  case 1u:
    return 0;
  case 2u:
    return rwd[2] ? 0 : 1;
    break;
  case 3u:
  {
    if ( rwd[4] )
      return 0;
    else if ( rwd[3] )
      return 1;
    else if ( rwd[2] )
      return 2;
  }
  case 4u:
  {
    if ( rwd[8] )
      return 0;
    else if ( rwd[7] )
      return 1;
    else if ( rwd[6] )
      return 2;
    else if ( rwd[5] )
      return 3;
    else if ( rwd[4] )
      return rwd[4] == 4 ? 4 : 5;
    else if ( rwd[3] )
      return 6;
    else if ( rwd[2] )
      return 7;
  }
  break;

  case 5u:
  {
    if ( rwd[16] )
      return 0;
    else if ( rwd[15] )
      return 1;
    else if ( rwd[14] )
      return 2;
    else if ( rwd[13] )
      return 3;
    else if ( rwd[12] )
      return rwd[4] == 7 ? 4 : 5;
    else if ( rwd[11] )
      return rwd[5] == 1 ? 6 : 7;
    else if ( rwd[10] )
    {
      switch ( rwd[2] )
      {
      case 30:
        return 8;
      case 15:
        return 9;
      case 14:
        return 10;
      default:
        return 11;
      }
    }
    else if ( rwd[9] )
    {
      switch ( rwd[3] )
      {
      case 15:
        return 12;
      case 12:
        return 13;
      case 9:
        return 14;
      case 6:
        return 15;
      default:
        return 16;
      }
    }
    else if ( rwd[8] )
    {
      switch ( rwd[0] )
      {
      case 28:
        return 17;
      case 19:
        return 18;
      case 22:
        return 19;
      case 7:
        return 20;
      case 9:
        return 21;
      case 10:
        return 22;
      case 11:
        return 23;
      default:
        return 24;
      }
    }
    else if ( rwd[7] )
    {
      switch ( rwd[1] )
      {
      case 15:
      {
        const auto acd = spectrum_distribution( autocorrelation_spectrum( tt ) );
        return acd[2] == 22 ? 25 : 26;
      }
      break;
      case 18:
        return 27;
      case 19:
        return 28;
      case 21:
        return 29;
      default:
        return 30;
      }
    }
    else if ( rwd[6] )
    {
      switch ( rwd[2] )
      {
      case 28:
      {
        const auto acd = spectrum_distribution( autocorrelation_spectrum( tt ) );
        return acd[0] == 12 ? 31 : 32;
      }
      break;
      case 15:
        return 33;
      case 14:
      {
        const auto acd = spectrum_distribution( autocorrelation_spectrum( tt ) );
        return acd[0] == 15 ? 34 : 35;
      }
      break;
      case 13:
        return 36;
      case 12:
      {
        const auto acd = spectrum_distribution( autocorrelation_spectrum( tt ) );
        return acd[0] == 18 ? 37 : 38;
      }
      break;
      default:
        return 39;
      }
    }
    else if ( rwd[5] )
    {
      switch ( rwd[3] )
      {
      case 16:
        return 40;
      default:
      {
        const auto acd = spectrum_distribution( autocorrelation_spectrum( tt ) );
        switch ( acd[2] )
        {
        case 25:
          return 41;
        case 27:
          return 42;
        default:
          return 43;
        }
      }
      break;
      }
    }
    else if ( rwd[4] )
    {
      switch ( rwd[0] )
      {
      case 16:
      {
        const auto acd = spectrum_distribution( autocorrelation_spectrum( tt ) );
        switch ( acd[0] )
        {
        case 30:
          return 44;
        case 15:
          return 45;
        default:
          return 46;
        }
      }
      break;
      default:
        return 47;
      }
    }
  }
  break;
  }

  return 48;
}

namespace detail
{
static std::vector<uint64_t> spectral_repr[] = {
    { 0x0 },
    { 0x0 },
    { 0x0, 0x8 },
    { 0x00, 0x80, 0x88 },
    { 0x0000, 0x8000, 0x8080, 0x0888, 0x8888, 0x2a80, 0xf888, 0x7888 },
    { 0x00000000, 0x80000000, 0x80008000, 0x00808080, 0x80808080, 0x08888000, 0xaa2a2a80, 0x88080808, 0x2888a000, 0xf7788000, 0xa8202020, 0x08880888, 0xbd686868, 0xaa808080, 0x7e686868, 0x2208a208, 0x08888888, 0x88888888, 0xea404040, 0x2a802a80, 0x73d28c88, 0xea808080, 0xa28280a0, 0x13284c88, 0xa2220888, 0xaae6da80, 0x58d87888, 0x8c88ac28, 0x8880f880, 0x9ee8e888, 0x4268c268, 0x16704c80, 0x78888888, 0x4966bac0, 0x372840a0, 0x5208d288, 0x7ca00428, 0xf8880888, 0x2ec0ae40, 0xf888f888, 0x58362ec0, 0x0eb8f6c0, 0x567cea40, 0xf8887888, 0x78887888, 0xe72890a0, 0x268cea40, 0x6248eac0 } };
}

/*! \brief Returns spectral representative using lookup
 *
 * This function returns the spectral representative for functions with up to 5
 * variables, but does not give the possibility to obtain the transformation
 * sequence to obtain the function from the representative.
 *
 * \brief func Truth table for function with at most 5 variables.
 */
template<class TT>
TT spectral_representative( const TT& func )
{
  static_assert( is_complete_truth_table<TT>::value, "Can only be applied on complete truth tables." );

  auto r = func.construct();
  const auto index = get_spectral_class( func );
  const auto word = detail::spectral_repr[func.num_vars()][index];
  kitty::create_from_words( r, &word, &word + 1 );
  return r;
}

} /* namespace kitty */