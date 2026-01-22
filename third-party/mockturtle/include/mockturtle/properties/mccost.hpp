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
  \file mccost.hpp
  \brief Cost functions based on multiplicative-complexity

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>

#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "../views/topo_view.hpp"

namespace mockturtle
{

/*! \brief Computes the multiplicative complexity
 *
 * Computes and sums the multiplicative complexity of each gate in the network.
 * Returns `std::nullopt`, if multiplicative complexity cannot be determined
 * for some gate.
 *
 * \param ntk Network
 */
template<class Ntk>
std::optional<uint32_t> multiplicative_complexity( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );

  uint32_t total{ 0u };
  bool valid{ true };

  ntk.foreach_gate( [&]( auto const& n ) {
    if constexpr ( has_is_and_v<Ntk> )
    {
      if ( ntk.is_and( n ) )
      {
        total++;
        return true;
      }
    }

    if constexpr ( has_is_or_v<Ntk> )
    {
      if ( ntk.is_or( n ) )
      {
        total++;
        return true;
      }
    }

    if constexpr ( has_is_xor_v<Ntk> )
    {
      if ( ntk.is_xor( n ) )
      {
        return true;
      }
    }

    if constexpr ( has_is_maj_v<Ntk> )
    {
      if ( ntk.is_maj( n ) )
      {
        total++;
        return true;
      }
    }

    if constexpr ( has_is_ite_v<Ntk> )
    {
      if ( ntk.is_ite( n ) )
      {
        total++;
        return true;
      }
    }

    if constexpr ( has_is_xor3_v<Ntk> )
    {
      if ( ntk.is_xor3( n ) )
      {
        return true;
      }
    }

    if constexpr ( has_is_nary_and_v<Ntk> )
    {
      if ( ntk.is_nary_and( n ) )
      {
        if ( ntk.fanin_size( n ) > 1u )
        {
          total += ntk.fanin_size( n ) - 1u;
        }
        return true;
      }
    }

    if constexpr ( has_is_nary_or_v<Ntk> )
    {
      if ( ntk.is_nary_or( n ) )
      {
        if ( ntk.fanin_size( n ) > 1u )
        {
          total += ntk.fanin_size( n ) - 1u;
        }
        return true;
      }
    }

    if constexpr ( has_is_nary_xor_v<Ntk> )
    {
      if ( ntk.is_nary_xor( n ) )
      {
        return true;
      }
    }

    valid = false;
    return false; /* break */
  } );

  if ( valid )
  {
    return total;
  }
  else
  {
    return std::nullopt;
  }
}

/*! \brief Computes the multiplicative complexity depth
 *
 * Computes multiplicative complexity of each gate and the sum of them on the
 * critical path in the network. Returns `std::nullopt`, if multiplicative
 * complexity cannot be determined for some gate.
 *
 * \param ntk Network
 */
template<class Ntk>
std::optional<uint32_t> multiplicative_complexity_depth( Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );

  bool valid{ true };

  node_map<uint32_t, Ntk> level( ntk, 0u );
  topo_view<Ntk> topo{ ntk };

  topo.foreach_node( [&]( auto const& n ) {
    if ( ntk.is_constant( n ) || ntk.is_pi( n ) )
    {
      return true;
    }

    /* compute the maximum MC of children */
    uint32_t max_level{ 0u };
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      if ( level[f] > max_level )
      {
        max_level = level[f];
      }
    } );

    if ( has_is_and_v<Ntk> )
    {
      if ( ntk.is_and( n ) )
      {
        level[n] = max_level + 1u;
        return true;
      }
    }

    if ( has_is_or_v<Ntk> )
    {
      if ( ntk.is_or( n ) )
      {
        level[n] = max_level + 1u;
        return true;
      }
    }

    if ( has_is_xor_v<Ntk> )
    {
      if ( ntk.is_xor( n ) )
      {
        level[n] = max_level;
        return true;
      }
    }

    if ( has_is_maj_v<Ntk> )
    {
      if ( ntk.is_maj( n ) )
      {
        level[n] = max_level + 1u;
        return true;
      }
    }

    if ( has_is_ite_v<Ntk> )
    {
      if ( ntk.is_ite( n ) )
      {
        level[n] = max_level + 1u;
        return true;
      }
    }

    if ( has_is_xor3_v<Ntk> )
    {
      if ( ntk.is_xor3( n ) )
      {
        level[n] = max_level;
        return true;
      }
    }

    if ( has_is_nary_and_v<Ntk> )
    {
      if ( ntk.is_nary_and( n ) )
      {
        level[n] = max_level + static_cast<uint32_t>( std::ceil( std::log2( ntk.fanin_size( n ) ) ) );
        return true;
      }
    }

    if ( has_is_nary_or_v<Ntk> )
    {
      if ( ntk.is_nary_or( n ) )
      {
        level[n] = max_level + static_cast<uint32_t>( std::ceil( std::log2( ntk.fanin_size( n ) ) ) );
        return true;
      }
    }

    if ( has_is_nary_xor_v<Ntk> )
    {
      if ( ntk.is_nary_xor( n ) )
      {
        level[n] = max_level;
        return true;
      }
    }

    valid = false;
    return false; /* break */
  } );

  if ( valid )
  {
    uint32_t max_level{ 0u };
    ntk.foreach_po( [&]( const auto& f ) {
      if ( level[f] > max_level )
      {
        max_level = level[f];
      }
    } );
    return max_level;
  }
  else
  {
    return std::nullopt;
  }
}

} // namespace mockturtle