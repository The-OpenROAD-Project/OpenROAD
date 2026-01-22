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
  \file equivalence_classes.hpp
  \brief Synthesis routines based on equivalence classes

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <type_traits>
#include <vector>

#include "../traits.hpp"

#include <kitty/detail/constants.hpp>
#include <kitty/spectral.hpp>

namespace mockturtle
{

/*! \brief Applies a sequence of transformations to a network
 *
 * A synthesis function `synthesis_fn` computes a network into `dest` and
 * computes an output signal.  Both the inputs and the outputs to that synthesis
 * function are transformed according to `transformations`, a sequence of
 * spectral transformations.  The vector `leaves` are the original inputs to the
 * output function, which is returned in terms of a signal into the network.
 *
 * The signature of `synthesis_fn` is `signal<Ntk>(Ntk&, std::vector<signal<Ntk>> const&)`.
 *
 * \param dest Destination network for synthesis
 * \param transformations Sequence of spectral operations (see kitty)
 * \param leaves Original inputs, which might be transformed
 * \param synthesis_fn Synthesis function to create the inner function (without transformations)
 *
   \verbatim embed:rst

   .. note::

      An example on how to transform the AND function into the MAJ function is
      provided as test.
   \endverbatim
 */
template<class Ntk, class SynthesisFn>
signal<Ntk> apply_spectral_transformations( Ntk& dest, std::vector<kitty::detail::spectral_operation> const& transformations, std::vector<signal<Ntk>> const& leaves, SynthesisFn&& synthesis_fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_nary_xor_v<Ntk>, "Ntk does not implement the create_nary_xor method" );
  static_assert( std::is_invocable_r_v<signal<Ntk>, SynthesisFn, Ntk&, std::vector<signal<Ntk>> const&>, "SynthesisFn does not have expected signature" );

  auto _leaves = leaves;
  std::vector<signal<Ntk>> _final_xors;
  bool _output_neg = false;

  for ( auto const& t : transformations )
  {
    switch ( t._kind )
    {
    default:
      assert( false );
    case kitty::detail::spectral_operation::kind::permutation:
    {
      const auto v1 = kitty::detail::log2[t._var1];
      const auto v2 = kitty::detail::log2[t._var2];
      std::swap( _leaves[v1], _leaves[v2] );
    }
    break;
    case kitty::detail::spectral_operation::kind::input_negation:
    {
      const auto v1 = kitty::detail::log2[t._var1];
      _leaves[v1] = dest.create_not( _leaves[v1] );
    }
    break;
    case kitty::detail::spectral_operation::kind::output_negation:
      _output_neg = !_output_neg;
      break;
    case kitty::detail::spectral_operation::kind::spectral_translation:
    {
      const auto v1 = kitty::detail::log2[t._var1];
      const auto v2 = kitty::detail::log2[t._var2];
      _leaves[v1] = dest.create_xor( _leaves[v1], _leaves[v2] );
    }
    break;
    case kitty::detail::spectral_operation::kind::disjoint_translation:
    {
      const auto v1 = kitty::detail::log2[t._var1];
      _final_xors.push_back( _leaves[v1] );
    }
    break;
    }
  }

  _final_xors.push_back( synthesis_fn( dest, _leaves ) );
  const auto output = dest.create_nary_xor( _final_xors );
  return _output_neg ? dest.create_not( output ) : output;
}

/*! \brief Applies NPN transformations to a network
 *
 * A synthesis function `synthesis_fn` computes a network into `dest` and
 * computes an output signal.  Both the inputs and the outputs to that synthesis
 * function are transformed according to `phase` and `perm`, based on NPN
 * classification.  The vector `leaves` are the original inputs to the
 * output function, which is returned in terms of a signal into the network.
 *
 * The signature of `synthesis_fn` is `signal<Ntk>(Ntk&, std::vector<signal<Ntk>> const&)`.
 *
 * \param dest Destination network for synthesis
 * \param phase Input and output complementation
 * \param perm Input permutation
 * \param leaves Original inputs, which might be transformed
 * \param synthesis_fn Synthesis function to create the inner function (without transformations)
 */
template<class Ntk, class SynthesisFn>
signal<Ntk> apply_npn_transformations( Ntk& dest, uint32_t phase, std::vector<uint8_t> const& perm, std::vector<signal<Ntk>> const& leaves, SynthesisFn&& synthesis_fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_not_v<Ntk>, "Ntk does not implement the create_not method" );
  static_assert( has_create_nary_xor_v<Ntk>, "Ntk does not implement the create_nary_xor method" );
  static_assert( std::is_invocable_r_v<signal<Ntk>, SynthesisFn, Ntk&, std::vector<signal<Ntk>> const&>, "SynthesisFn does not have expected signature" );

  std::vector<signal<Ntk>> _leaves( perm.size() );
  std::transform( perm.begin(), perm.end(), _leaves.begin(), [&]( auto const& i ) { return ( phase >> i ) & 1 ? dest.create_not( leaves[i] ) : leaves[i]; } );

  const auto f = synthesis_fn( dest, _leaves );
  return ( phase >> leaves.size() ) & 1 ? dest.create_not( f ) : f;
}

} /* namespace mockturtle */
