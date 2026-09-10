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
  \file print.hpp
  \brief Implements functions to print truth tables

  \author Mathias Soeken
  \author Rassul Bairamkulov
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "algorithm.hpp"
#include "karnaugh_map.hpp"
#include "operations.hpp"
#include "constructors.hpp"
#include "isop.hpp"

namespace kitty
{

namespace detail
{

inline std::string to_binary( uint16_t value, uint32_t num_vars )
{
  std::string res( num_vars, '0' );
  auto it = res.end() - 1;
  while ( value )
  {
    if ( value & 1 )
    {
      *it = '1';
    }
    value >>= 1;
    if ( it == res.begin() )
    {
      break;
    }
    --it;
  }
  return res;
}

inline void print_xmas_tree(
    std::ostream& os, uint32_t num_vars,
    const std::vector<std::pair<std::function<bool( uint16_t )>,
                                std::vector<int>>>& style_predicates = {} )
{
  /* create rows */
  std::vector<std::vector<uint16_t>> current( 1, { 0 } ), next;

  for ( auto i = 0u; i < num_vars; ++i )
  {
    for ( const auto& row : current )
    {
      if ( row.size() != 1u )
      {
        next.emplace_back();

        std::transform( row.begin() + 1, row.end(),
                        std::back_inserter( next.back() ),
                        []( auto cell )
                        { return cell << 1; } );
      }
      next.emplace_back( 1, row.front() << 1 );
      std::transform( row.begin(), row.end(), std::back_inserter( next.back() ),
                      []( auto cell )
                      { return ( cell << 1 ) ^ 1; } );
    }

    std::swap( current, next );
    next.clear();
  }

  for ( const auto& row : current )
  {
    /* white space padding to center columns */
    os << std::string( ( ( num_vars + 1 ) - row.size() ) / 2 * ( num_vars + 1 ), ' ' );
    for ( const auto& col : row )
    {
      os << " ";
      for ( const auto& pred : style_predicates )
      {
        if ( pred.first( col ) )
        {
          for ( auto style : pred.second )
          {
            os << "\033[" << style << "m";
          }
        }
      }
      os << to_binary( col, num_vars ) << "\033[0m";
    }
    os << "\n";
  }
}

} // namespace detail

/*! \brief Prints truth table in binary representation

  The most-significant bit will be the first character of the string.

  \param tt Truth table
  \param os Output stream
*/
template<typename TT>
void print_binary( const TT& tt, std::ostream& os = std::cout )
{
  auto const chunk_size = std::min<uint64_t>( tt.num_bits(), 64 );
  for_each_block_reversed( tt, [&os, chunk_size]( auto word )
                           {
    std::string chunk( chunk_size, '0' );

    auto it = chunk.rbegin();
    while (word && it != chunk.rend()) {
      if (word & 1) {
        *it = '1';
      }
      ++it;
      word >>= 1;
    }
    os << chunk; } );
}

/*! \cond PRIVATE */
inline void print_binary( const partial_truth_table& tt,
                          std::ostream& os = std::cout )
{
  auto const chunk_size = std::min<uint64_t>( tt.num_bits(), 64 );
  bool first = true;
  for_each_block_reversed( tt, [&tt, &os, chunk_size, &first]( auto word )
                           {
    std::string chunk( chunk_size, '0' );
    auto it = chunk.rbegin();
    while (word && it != chunk.rend()) {
      if (word & 1) {
        *it = '1';
      }
      ++it;
      word >>= 1;
    }

    if (first && (chunk_size == 64) && (tt.num_bits() % 64)) {
      first = false;
      os << chunk.substr(64 - (tt.num_bits() % 64));
    } else {
      os << chunk;
    } } );
}
/*! \endcond */

template<typename TT>
void print_binary( const ternary_truth_table<TT>& tt, std::ostream& os = std::cout )
{
  auto const chunk_size = std::min<uint64_t>( tt.num_bits(), 64 );
  std::string tt_string = "";
  for_each_block_reversed( tt._bits, [&tt_string, chunk_size]( auto word )
                           {
    std::string chunk( chunk_size, '0' );
    auto it = chunk.rbegin();
    while ( word && it != chunk.rend() )
    {
      if ( word & 1 )
      {
        *it = '1';
      }
      ++it;
      word >>= 1;
    }
    tt_string += chunk; } );
  for ( auto i = 0u; i < tt.num_bits(); i++ )
  {
    if ( is_dont_care( tt, tt.num_bits() - 1 - i ) )
    {
      tt_string[i] = '-';
    }
  }
  os << tt_string;
}

template<typename TT>
void print_binary( const quaternary_truth_table<TT>& tt, std::ostream& os = std::cout )
{
  auto const chunk_size = std::min<uint64_t>( tt.num_bits(), 64 );
  std::string tt_string = "";
  for_each_block_reversed( tt._onset, [&tt_string, chunk_size]( auto word )
                           {
    std::string chunk( chunk_size, '0' );
    auto it = chunk.rbegin();
    while ( word && it != chunk.rend() )
    {
      if ( word & 1 )
      {
        *it = '1';
      }
      ++it;
      word >>= 1;
    }
    tt_string += chunk; } );
  for ( auto i = 0u; i < tt.num_bits(); i++ )
  {
    if ( is_dont_care( tt, tt.num_bits() - 1 - i ) )
      tt_string[i] = '-';
    if ( is_dont_know( tt, tt.num_bits() - 1 - i ) )
      tt_string[i] = 'x';
  }
  os << tt_string;
}

/*! \brief Prints K-map of given truth table.

  Columns represent the values of the least significant variables, rows of the
  most significant ones.

  \param tt Truth table
  \param os Output stream (default = cout)
*/
template<typename TT>
void print_kmap( const TT& tt, std::ostream& os = std::cout )
{
  karnaugh_map<TT> kmap( tt );
  kmap.print( os );
}

/*! \brief Prints truth table in hexadecimal representation

  The most-significant bit will be the first character of the string.

  \param tt Truth table
  \param os Output stream
*/
template<typename TT, typename = std::enable_if_t<is_completely_specified_truth_table<TT>::value>>
void print_hex( const TT& tt, std::ostream& os = std::cout )
{
  auto const chunk_size =
      std::min<uint64_t>( tt.num_vars() <= 1 ? 1 : ( tt.num_bits() >> 2 ), 16 );

  for_each_block_reversed( tt, [&os, chunk_size]( auto word )
                           {
    std::string chunk( chunk_size, '0' );

    auto it = chunk.rbegin();
    while (word && it != chunk.rend()) {
      auto hex = word & 0xf;
      if (hex < 10) {
        *it = '0' + static_cast<char>(hex);
      } else {
        *it = 'a' + static_cast<char>(hex - 10);
      }
      ++it;
      word >>= 4;
    }
    os << chunk; } );
}

/*! \cond PRIVATE */
inline void print_hex( const partial_truth_table& tt,
                       std::ostream& os = std::cout )
{
  bool first = true;
  for_each_block_reversed( tt, [&tt, &os, &first]( auto word )
                           {
    std::string chunk( 16, '0' );

    auto it = chunk.rbegin();
    while (word && it != chunk.rend()) {
      auto hex = word & 0xf;
      if (hex < 10) {
        *it = '0' + static_cast<char>(hex);
      } else {
        *it = 'a' + static_cast<char>(hex - 10);
      }
      ++it;
      word >>= 4;
    }

    if (first && (tt.num_bits() % 64)) {
      first = false;
      os << chunk.substr((tt.num_bits() % 4)
                             ? (15 - ((tt.num_bits() >> 2) % 16))
                             : (16 - ((tt.num_bits() >> 2) % 16)));
    } else {
      os << chunk;
    } } );
}
/*! \endcond */

/*! \brief Prints truth table in raw binary presentation (for file I/O)

  This function is useful to store large truth tables in binary files
  or `std::stringstream`. Each word is stored into 8 characters.

  \param tt Truth table
  \param os Output stream
*/
template<typename TT, typename = std::enable_if_t<!std::is_same<TT, ternary_truth_table<dynamic_truth_table>>::value>>
void print_raw( const TT& tt, std::ostream& os )
{
  for_each_block( tt, [&os]( auto word )
                  { os.write( reinterpret_cast<char*>( &word ), sizeof( word ) ); } );
}

/*! \brief Returns truth table as a string in binary representation

  Calls `print_binary` internally on a string stream.

  \param tt Truth table
*/
template<typename TT>
inline std::string to_binary( const TT& tt )
{
  std::stringstream st;
  print_binary( tt, st );
  return st.str();
}

/*! \brief Returns truth table as a string in hexadecimal representation

  Calls `print_hex` internally on a string stream.

  \param tt Truth table
*/
template<typename TT, typename = std::enable_if_t<is_completely_specified_truth_table<TT>::value>>
inline std::string to_hex( const TT& tt )
{
  std::stringstream st;
  print_hex( tt, st );
  return st.str();
}

/*! \brief Prints minterms of a Boolean function in christmas tree pattern

  This function prints all minterms of a Boolean function and arranges them
  according to the christmas tree pattern as described in Section 7.2.1.6 in
  The Art of Computer Programming by Donald E. Knuth.  Minterms from the
  off-set are printed in red, minterms from the on-set are printed in green.

  \param tt Truth table
  \param os Output stream
*/
template<typename TT, typename = std::enable_if_t<!std::is_same<TT, partial_truth_table>::value>, typename = std::enable_if_t<is_completely_specified_truth_table<TT>::value>>

void print_xmas_tree_for_function( const TT& tt, std::ostream& os = std::cout )
{
  detail::print_xmas_tree( os, tt.num_vars(),
                           { { [&]( auto v )
                               { return get_bit( tt, v ); },
                               { 32 } },
                             { [&]( auto v )
                               { return !get_bit( tt, v ); },
                               { 31 } } } );
}

/*! \brief Prints all Boolean functions of n variables in christmas tree pattern

  This function prints all Boolean functions of n variables and arranges them
  according to the christmas tree pattern as described in Section 7.2.1.6 in
  The Art of Computer Programming by Donald E. Knuth.  Functions can be printed
  in different styles according to some properties.

  \param tt Number of variables
  \param style_predicates Each pair has a predicate `bool(TT const&)` to check
                          whether a certain property holds for the truth table
                          the element in the tree represents.  If this predicate
                          evaluates to true, then the second element in the pair
                          are indexes of style (ANSI term) to change the
                          string in the output.
  \param os Output stream
*/
template<class TT>
void print_xmas_tree_for_functions(
    uint32_t num_vars,
    const std::vector<std::pair<std::function<bool( TT const& )>,
                                std::vector<int>>>& style_predicates = {},
    std::ostream& os = std::cout )
{

  std::vector<std::pair<std::function<bool( uint16_t )>, std::vector<int>>>
      _preds;
  std::transform( style_predicates.begin(), style_predicates.end(),
                  std::back_inserter( _preds ), [&]( const auto& p )
                  { return std::make_pair(
                        [&]( uint16_t v )
                        {
                          auto tt = create<TT>( num_vars );
                          std::copy( &v, &v + 1, tt.begin() );
                          return p.first( tt );
                        },
                        p.second ); } );

  detail::print_xmas_tree( os, 1 << num_vars, _preds );
}

/*! \brief Creates an expression for an ANF form
 *
 * \param anf Truth table in ANF encoding
 */
template<typename TT, typename = std::enable_if_t<
                          !std::is_same<TT, partial_truth_table>::value>>
std::string anf_to_expression( const TT& anf )
{
  const auto terms = count_ones( anf );

  if ( terms == 0u )
  {
    return "0";
  }

  std::string expr;

  for_each_one_bit( anf, [&]( auto bit )
                    {
    if ( bit == 0 )
    {

      expr += "1";
      return;
    }
    auto weight = __builtin_popcount(static_cast<uint32_t>(bit));
    if (weight != 1) {
      expr += "(";
    }
    for (auto i = 0u; i < anf.num_vars(); ++i) {
      if ((bit >> i) & 1) {
        expr += std::string(1, 'a' + i);
      }
    }
    if (weight != 1) {
      expr += ")";
    } } );

  return terms == 1 ? expr : "[" + expr + "]";
}

/*! \brief Creates an expression for an ANF form

  All don't cares are considered to be zero.

  It is a "restricted" ANF for the ternary turth table.
 *
 * \param anf Ternary truth table in ANF encoding
 */
template<typename TT, typename = std::enable_if_t<!std::is_same<TT, partial_truth_table>::value>>
std::string anf_to_expression( const ternary_truth_table<TT>& anf )
{
  return anf_to_expression( anf._bits );
}

/*! \brief Creates an expression in the sum of products format.

  Does not perform any optimizations. Useful when constructing GENLIB-compatible expressions

  \param tt Truth table
  \param os Output stream
 */
template<typename TT, typename = std::enable_if_t<is_completely_specified_truth_table<TT>::value>>
void print_sop_expression( TT tt, std::ostream& os = std::cout )
{
  /* compare to constants */
  if ( is_const0( tt ) )
  {
    os << "CONST0";
    return;
  }
  else if ( is_const0( ~tt ) )
  {
    os << "CONST1";
    return;
  }

  /* extract product terms */
  auto cubes = kitty::isop( tt );

  bool first_cube = true; // Controls insertion of '|'. It's false for the first cube to avoid leading '|'.

  /* write product terms */
  for ( auto cube : cubes )
  {
    auto bits = cube._bits;
    auto mask = cube._mask;

    bool brackets = __builtin_popcount( mask ) > 1;

    if ( !first_cube )
    {
      os << '|';
    }

    if ( brackets )
    {
      os << '(';
    }

    bool first_literal = true;
    for ( auto i = 0u; i < tt.num_vars(); ++i )
    {
      if ( mask & 1 )
      {
        if ( !first_literal )
        {
          os << '&';
        }
        if ( !( bits & 1 ) )
        {
          os << '!';
        }
        os << static_cast<char>( 'a' + i );
        first_literal = false;
      }
      bits >>= 1;
      mask >>= 1;
    }

    if ( brackets )
    {
      os << ')';
    }
    first_cube = false;
  }
}

/*! \brief Creates an expression in the sum of products format.

  Does not perform any optimizations. Useful when constructing GENLIB-compatible expressions

  \param tt Truth table
 */
template<typename TT, typename = std::enable_if_t<is_completely_specified_truth_table<TT>::value>>
std::string to_sop_expression( TT tt )
{
  std::stringstream os;
  print_sop_expression( tt, os ); // Use the function to write into the stringstream
  return os.str();                // Convert the stringstream to string and return it
}

} /* namespace kitty */