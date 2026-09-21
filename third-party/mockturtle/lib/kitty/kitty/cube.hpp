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
  \file cube.hpp
  \brief A cube data structure for up to 32 variables

  \author Mathias Soeken
*/

#pragma once

#include <functional>
#include <iostream>
#include <string>

#include "hash.hpp"
#include "detail/mscfix.hpp"

namespace kitty
{
class cube
{
public:
  /*! \brief Constructs the empty cube

    Represents the one-cube
  */
  cube() : _value( 0 ) {} /* NOLINT */

  /*! \brief Constructs a cube from bits and mask

    For a valid cube and to be consistent in the ternary values, we
    assume that whenever a bit in the care bitmask is set to 0, also
    the polarity bitmask must be 0.

    \param bits Polarity bitmask of variables (0: negative, 1: positive)
    \param mask Care bitmask of variables (1: part of cube, 0: not part of cube)
  */
  cube( uint32_t bits, uint32_t mask ) : _bits( bits ), _mask( mask ) {} /* NOLINT */

  /*! \brief Constructs a cube from a string

    Each character corresponds to one literal in the cube.  Only up to first 32
    characters of the string will be considered, since this data structure
    cannot represent cubes with more than 32 literals.  A '1' in the string
    corresponds to a postive literal, a '0' corresponds to a negative literal.
    All other characters represent don't care, but it is customary to use '-'.

    \param str String representing a cube
  */
  // cppcheck-suppress noExplicitConstructor
  cube( const std::string& str ) /* NOLINT */
  {
    _bits = _mask = 0u;

    auto p = str.begin();
    if ( p == str.end() )
    {
      return;
    }

    for ( uint64_t i = 1; i <= ( uint64_t( 1u ) << 32u ); i <<= 1 )
    {
      switch ( *p )
      {
      default: /* don't care */
        break;
      case '1':
        _bits |= i;
        /* no break on purpose, jump to 0 and set mask */
      case '0':
        _mask |= i;
        break;
      }

      if ( ++p == str.end() )
      {
        return;
      }
    }
  }

  /*! \brief Returns number of literals */
  inline int num_literals() const
  {
    return __builtin_popcount( _mask );
  }

  /*! \brief Returns the difference to another cube */
  inline int difference( const cube& that ) const
  {
    return ( _bits ^ that._bits ) | ( _mask ^ that._mask );
  }

  /*! \brief Returns the distance to another cube */
  inline int distance( const cube& that ) const
  {
    return __builtin_popcount( difference( that ) );
  }

  /*! \brief Checks whether two cubes are equivalent */
  inline bool operator==( const cube& that ) const
  {
    return _value == that._value;
  }

  /*! \brief Checks whether two cubes are not equivalent */
  inline bool operator!=( const cube& that ) const
  {
    return _value != that._value;
  }

  /*! \brief Default comparison operator */
  inline bool operator<( const cube& that ) const
  {
    return _value < that._value;
  }

  /*! \brief Returns the negated cube */
  inline cube operator~() const
  {
    return { ~_bits, _mask };
  }

  /*! \brief Merges two cubes of distance-1 */
  inline cube merge( const cube& that ) const
  {
    const auto d = difference( that );
    return { _bits ^ ( ~that._bits & d ), _mask ^ ( that._mask & d ) };
  }

  /*! \brief Adds literal to cube */
  inline void add_literal( uint8_t var_index, bool polarity = true )
  {
    set_mask( var_index );

    if ( polarity )
    {
      set_bit( var_index );
    }
    else
    {
      clear_bit( var_index );
    }
  }

  /*! \brief Removes literal from cube */
  inline void remove_literal( uint8_t var_index )
  {
    clear_mask( var_index );
    clear_bit( var_index );
  }

  /*! \brief Constructs the elementary cube representing a single variable */
  static cube nth_var_cube( uint8_t var_index )
  {
    const auto _bits = uint32_t( 1 ) << var_index;
    return { _bits, _bits };
  }

  /*! \brief Constructs the elementary cube containing the first k positive literals */
  static cube pos_cube( uint8_t k )
  {
    const uint32_t _bits = ( uint64_t( 1 ) << k ) - 1;
    return { _bits, _bits };
  }

  /*! \brief Constructs the elementary cube containing the first k negative literals */
  static cube neg_cube( uint8_t k )
  {
    const uint32_t _bits = ( uint64_t( 1 ) << k ) - 1;
    return { 0u, _bits };
  }

  /*! \brief Prints a cube */
  inline void print( unsigned length = 32u, std::ostream& os = std::cout ) const
  {
    for ( auto i = 0u; i < length; ++i )
    {
      os << ( get_mask( i ) ? ( get_bit( i ) ? '1' : '0' ) : '-' );
    }
  }

  /*! \brief Gets bit at index */
  inline bool get_bit( uint8_t index ) const
  {
    return ( ( _bits >> index ) & 1 ) != 0;
  }

  /*! \brief Gets mask at index */
  inline bool get_mask( uint8_t index ) const
  {
    return ( ( _mask >> index ) & 1 ) != 0;
  }

  /*! \brief Sets bit at index */
  inline void set_bit( uint8_t index )
  {
    _bits |= ( 1 << index );
  }

  /*! \brief Sets mask at index */
  inline void set_mask( uint8_t index )
  {
    _mask |= ( 1 << index );
  }

  /*! \brief Clears bit at index */
  inline void clear_bit( uint8_t index )
  {
    _bits &= ~( 1 << index );
  }

  /*! \brief Clears mask at index */
  inline void clear_mask( uint8_t index )
  {
    _mask &= ~( 1 << index );
  }

  /*! \brief Flips bit at index */
  inline void flip_bit( uint8_t index )
  {
    _bits ^= ( 1 << index );
  }

  /*! \brief Flips mask at index */
  inline void flip_mask( uint8_t index )
  {
    _mask ^= ( 1 << index );
  }

  /*! \brief Iterates over all minterms in the cube
   *
   * The callback function takes a cube as input, which is actually
   * a minterm (i.e., all variables are set), and returns a boolean.
   * The loop terminates when the callback returns false.
   *
   * \param length Number of variables in the cube
   * \param fn Callback function on each minterm
   */
  template<class Fn>
  void foreach_minterm( uint8_t length, Fn&& fn ) const
  {
    foreach_minterm_rec( *this, length, fn );
  }

  template<class Fn>
  bool foreach_minterm_rec( cube const& c, uint8_t prev_index, Fn&& fn ) const
  {
    if ( prev_index == 0 )
    {
      return fn( c );
    }

    uint8_t index = prev_index - 1;
    if ( !get_mask( index ) )
    {
      cube c0 = c;
      c0.set_mask( index );
      if ( !foreach_minterm_rec( c0, index, fn ) )
      {
        return false;
      }
      c0.set_bit( index );
      return foreach_minterm_rec( c0, index, fn );
    }
    else
    {
      return foreach_minterm_rec( c, index, fn );
    }
  }

  /* cube data */
  union
  {
    struct
    {
      uint32_t _bits;
      uint32_t _mask;
    };
    uint64_t _value;
  };
};

/*! \brief Prints all cubes in a vector

  \param cubes Vector of cubes
  \param length Number of variables in each cube
  \param os Output stream
*/
inline void print_cubes( const std::vector<cube>& cubes, unsigned length = 32u, std::ostream& os = std::cout )
{
  for ( const auto& cube : cubes )
  {
    cube.print( length, os );
    os << '\n';
  }

  os << std::flush;
}

template<>
struct hash<cube>
{
  std::size_t operator()( const cube& c ) const
  {
    return std::hash<uint64_t>{}( c._value );
  }
};
} // namespace kitty