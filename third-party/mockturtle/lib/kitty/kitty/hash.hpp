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
  \file hash.hpp
  \brief Implements hash functions for truth tables

  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <cstdlib>

#include "static_truth_table.hpp"

namespace kitty
{

/*! \brief Hash function for 64-bit word */
inline std::size_t hash_block( uint64_t word )
{
  /* from boost::hash_detail::hash_value_unsigned */
  return word ^ ( word + ( word << 6 ) + ( word >> 2 ) );
}

/*! \brief Combines two hash values */
inline void hash_combine( std::size_t& seed, std::size_t other )
{
  /* from boost::hash_detail::hash_combine_impl */
  const uint64_t m = UINT64_C( 0xc6a4a7935bd1e995 );
  const int r = 47;

  other *= m;
  other ^= other >> r;
  other *= m;

  seed ^= other;
  seed *= m;

  seed += 0xe6546b64;
}

/*! \brief Computes hash values for truth tables */
template<typename TT>
struct hash
{
  std::size_t operator()( const TT& tt ) const
  {
    auto it = std::begin( tt._bits );
    auto seed = hash_block( *it++ );

    while ( it != std::end( tt._bits ) )
    {
      hash_combine( seed, hash_block( *it++ ) );
    }

    return seed;
  }
};

/*! \cond PRIVATE */
template<uint32_t NumVars>
struct hash<static_truth_table<NumVars, true>>
{
  inline std::size_t operator()( const static_truth_table<NumVars, true>& tt ) const
  {
    return hash_block( tt._bits );
  }
};
/*! \endcond */
} // namespace kitty