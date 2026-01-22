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
  \file karnaugh_map.hpp
  \brief karnaugh maps visualizer

  \author Gianluca Radi
*/

#pragma once

#include "bit_operations.hpp"
#include "constructors.hpp"
#include "operators.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace kitty
{
template<typename TT>
class karnaugh_map
{
public:
  /*! \brief Destroys K-map
   */
  karnaugh_map() = delete;

  /*! \brief Construncts K-map from truth table
      \param tt truth table
  */
  karnaugh_map( TT tt ) : truth_table( tt )
  {
    uint64_t num_var = log2( (double)tt.num_bits() );
    vars_col = num_var >> 1;
    vars_row = num_var - vars_col;
    col_seq = compute_seq_1ham_dist( vars_col );
    row_seq = compute_seq_1ham_dist( vars_row );
  }

  /*! \brief Prints K-map
      \param os output stream (default = cout)
 */

  void print( std::ostream& os = std::cout )
  {
    print_space( vars_col, os );
    os << "    ";
    for ( const auto& i : row_seq )
    {
      os << binary( i, vars_row );
      print_space( vars_row, os );
      os << "    ";
    }
    os << std::endl
       << std::endl;

    for ( const auto& j : col_seq )
    {
      os << binary( j, vars_col );
      os << "    ";
      for ( const auto& i : row_seq )
      {
        uint8_t middle_space = 0;
        if ( vars_row > 2 )
          middle_space = vars_row / 2;
        print_space( middle_space, os );
        if ( is_dont_care( truth_table, ( j << vars_row ) + i ) )
          os << "-";
        else
        {
          if ( is_dont_know( truth_table, ( j << vars_row ) + i ) )
            os << "x";
          else
            os << get_bit( truth_table, ( j << vars_row ) + i );
        }
        print_space( ( vars_row << 1 ) - 1 - middle_space, os );
        os << "    ";
      }
      os << std::endl
         << std::endl;
    }
  }

  /*! \brief Get sequence of values for the least significant variables
   */
  std::vector<uint8_t> get_row_seq() const { return row_seq; }

  /*! \brief Get sequence of values for the most significant variables
   */
  std::vector<uint8_t> get_col_seq() const { return col_seq; }

private:
  TT truth_table;
  unsigned vars_row;
  unsigned vars_col;
  std::vector<uint8_t> row_seq;
  std::vector<uint8_t> col_seq;

  /*! \brief Converts unsigned into binary string
      \param n unsigned to convert
      \param max_var number of bits of the string
  */
  std::string binary( uint8_t n, uint8_t max_var )
  {
    std::string result;
    uint8_t count = 0u;

    do
    {
      result.insert( result.begin(), '0' + ( n & 1 ) );
      count++;
    } while ( n >>= 1 );

    for ( uint8_t i = 0; i < max_var - count; i++ )
    {
      result.insert( result.begin(), '0' );
    }

    return result;
  }

  /*! \brief Prints white space.
      \param val number of white space to print
      \param os output stream
  */
  void print_space( uint8_t val, std::ostream& os )
  {
    for ( uint8_t i = 0; i < val; i++ )
    {
      os << " ";
    }
  }

  /*! \brief Computes sequence of values with unitary Hamming distance for a
     certain number of values

      For example, for 2 as the number of variables, the sequence would be = {0,
     1, 3, 2} (= {00, 01, 11, 10})

      \param num_var number of variables
  */
  std::vector<uint8_t> compute_seq_1ham_dist( uint8_t num_var )
  {
    if ( num_var == 1 )
    {
      return { 0, 1 };
    }
    else
    {
      std::vector<uint8_t> res = compute_seq_1ham_dist( num_var - 1 );
      std::vector<uint8_t> res_rev( res.rbegin(), res.rend() );
      for ( auto& i : res_rev )
      {
        i += ( 1 << ( num_var - 1 ) );
      }
      res.insert( res.end(), res_rev.begin(), res_rev.end() );
      return res;
    }
  }
};

} // namespace kitty