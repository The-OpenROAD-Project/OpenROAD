/* lorina: C++ parsing library
 * Copyright (C) 2018-2021  EPFL
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
  \file dimacs.hpp
  \brief Implements DIMACS parser

  \author Bruno Schmitt
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "common.hpp"
#include "diagnostics.hpp"
#include "detail/utils.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>

namespace lorina
{

/*! \brief A reader visitor for the DIMACS format.
 *
 * Callbacks for reading the DIMACS format.
 */
class dimacs_reader
{
public:
  /*! \brief Callback method for parsed format.
   *
   * \param format The format (cnf or dnf)
   */
  virtual void on_format( const std::string& format ) const
  {
    (void)format;
  }

  /*! \brief Callback method for parsed number of variables.
   *
   * \param number_of_variables Number of variables
   */
  virtual void on_number_of_variables( uint64_t number_of_variables ) const
  {
    (void)number_of_variables;
  }

  /*! \brief Callback method for parsed number of clauses.
   *
   * \param number_of_clauses Number of clauses
   */
  virtual void on_number_of_clauses( uint64_t number_of_clauses ) const
  {
    (void)number_of_clauses;
  }

  /*! \brief Callback method for parsed clause.
   *
   * \param clause A clause of a logic function
   */
  virtual void on_clause( const std::vector<int>& clause ) const
  {
    (void)clause;
  }

  /*! \brief Callback method for parse end.
   *
   */
  virtual void on_end() const {}

}; /* dimacs_reader */

namespace dimacs_regex
{
static std::regex problem_spec( R"(^p\s+([cd]nf)\s+([0-9]+)\s+([0-9]+)$)" );
static std::regex clause( R"(((-?[1-9]+)+ +)+0)" );

} // namespace dimacs_regex

/*! \brief Reader function for the DIMACS format.
 *
 * Reads DIMACS format from a stream and invokes a callback
 * method for each parsed primitive and each detected parse error.
 *
 * \param in Input stream
 * \param reader A DIMACS reader with callback methods invoked for parsed primitives
 * \param diag An optional diagnostic engine with callback methods for parse errors
 * \return Success if parsing has been successful, or parse error if parsing has failed
 */
[[nodiscard]] inline return_code read_dimacs( std::istream& in, const dimacs_reader& reader, diagnostic_engine* diag = nullptr )
{
  auto loc = 0ul;
  auto errors = 0ul;
  bool found_spec = false;

  std::smatch m;
  detail::foreach_line_in_file_escape( in, [&]( const std::string& line ) {
    ++loc;

    /* empty line or comment */
    if ( line.empty() || line[0u] == 'c' )
      return true;

    if ( std::regex_search( line, m, dimacs_regex::problem_spec ) )
    {
      reader.on_format( std::string( m[1] ) );
      reader.on_number_of_variables( std::atol( std::string( m[2] ).c_str() ) );
      reader.on_number_of_clauses( std::atol( std::string( m[3] ).c_str() ) );
      found_spec = true;
      return true;
    }

    if ( found_spec == false ) 
    {
      if ( diag )
      {
        diag->report( diag_id::ERR_DIMACS_MISSING_SPEC );
      }
      ++errors;
      return false;
    }

    auto clauses_begin = std::sregex_iterator( line.begin(), line.end(), dimacs_regex::clause );
    auto clauses_end = std::sregex_iterator();

    if ( std::distance( clauses_begin, clauses_end ) == 0u )
    {
      if ( diag )
      {
        diag->report( diag_id::ERR_PARSE_LINE ).add_argument( line );
      }
      ++errors;
      return false;
    }

    for ( std::sregex_iterator i = clauses_begin; i != clauses_end; ++i ) 
    {
      std::smatch match = *i;
      std::stringstream ss( match[0].str() );
      std::string lit_str;
      std::vector<int> clause;
      while ( std::getline( ss, lit_str, ' ' ) )
      {
        int const lit = std::atol( lit_str.c_str() );
        if ( lit != 0 )
        {
          clause.push_back( lit );
        }
      }
      reader.on_clause( clause );
    } 
    return true;
  } );

  if ( errors > 0 )
  {
    return return_code::parse_error;
  }
  else
  {
    reader.on_end();
    return return_code::success;
  }
}

/*! \brief Reader function for DIMACS format.
 *
 * Reads DIMACS format from a file and invokes a callback method for each
 * parsed primitive and each detected parse error.
 *
 * \param filename Name of the file
 * \param reader A DIMACS reader with callback methods invoked for parsed primitives
 * \param diag An optional diagnostic engine with callback methods for parse errors
 * \return Success if parsing has been successful, or parse error if parsing has failed
 */
[[nodiscard]] inline return_code read_dimacs( const std::string& filename, const dimacs_reader& reader, diagnostic_engine* diag = nullptr )
{
  std::ifstream in( detail::word_exp_filename( filename ), std::ifstream::in );
  if ( !in.is_open() )
  {
    if ( diag )
    {
      diag->report( diag_id::ERR_FILE_OPEN ).add_argument( filename );
    }
    return return_code::parse_error;
  }
  else
  {
    auto const ret = read_dimacs( in, reader, diag );
    in.close();
    return ret;
  }
}

} // namespace lorina
