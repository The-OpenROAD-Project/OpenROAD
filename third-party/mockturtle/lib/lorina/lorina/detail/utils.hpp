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

/*! \cond PRIVATE */

#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <locale>
#include <memory>
#include <numeric>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef _WIN32
#include <libgen.h>
#include <wordexp.h>
#endif

#include "call_in_topological_order.hpp"

namespace lorina
{

namespace detail
{

/* https://stackoverflow.com/questions/6089231/getting-std-ifstream-to-handle-lf-cr-and-crlf */
inline std::istream& getline( std::istream& is, std::string& t )
{
  t.clear();

  /* The characters in the stream are read one-by-one using a std::streambuf.
   * That is faster than reading them one-by-one using the std::istream.
   * Code that uses streambuf this way must be guarded by a sentry object.
   * The sentry object performs various tasks,
   * such as thread synchronization and updating the stream state.
   */

  std::istream::sentry se( is, true );
  std::streambuf* sb = is.rdbuf();

  for ( ;; )
  {
    int const c = sb->sbumpc();
    switch ( c )
    {
    /* deal with file endings */
    case '\n':
      return is;
    case '\r':
      if ( sb->sgetc() == '\n' )
      {
        sb->sbumpc();
      }
      return is;

    /* handle the case when the last line has no line ending */
    case std::streambuf::traits_type::eof():
      if ( t.empty() )
      {
        is.setstate( std::ios::eofbit );
      }
      return is;

    default:
      t += (char)c;
    }
  }
}

template<typename T>
inline std::string join( const T& t, const std::string& sep )
{
  return std::accumulate(
      std::next( t.begin() ), t.end(), std::string( t[0] ),
      [&]( const std::string& s, const typename T::value_type& v ) { return s + sep + std::string( v ); } );
}

/* string utils are from https://stackoverflow.com/a/217605 */

inline void ltrim( std::string& s )
{
  s.erase( s.begin(), std::find_if( s.begin(), s.end(), []( int ch ) {
             return !std::isspace( ch );
           } ) );
}

inline void rtrim( std::string& s )
{
  s.erase( std::find_if( s.rbegin(), s.rend(), []( int ch ) {
             return !std::isspace( ch );
           } )
               .base(),
           s.end() );
}

inline void trim( std::string& s )
{
  ltrim( s );
  rtrim( s );
}

inline std::string trim_copy( std::string s )
{
  trim( s );
  return s;
}

inline void foreach_line_in_file_escape( std::istream& in, const std::function<bool( const std::string& )>& f )
{
  std::string line, line2;

  while ( !getline( in, line ).eof() )
  {
    trim( line );

    while ( line.back() == '\\' )
    {
      line.pop_back();
      trim( line );

      /* check if failbit has been set */
      if ( !getline( in, line2 ) )
      {
        assert( false );
        std::abort();
      }
      line += line2;
    }

    if ( !f( line ) )
    {
      break;
    }
  }
}

// https://stackoverflow.com/a/14266139
inline std::vector<std::string> split( const std::string& str, const std::string& sep )
{
  std::vector<std::string> result;

  size_t last = 0;
  size_t next = 0;
  std::string substring;
  while ( ( next = str.find( sep, last ) ) != std::string::npos )
  {
    substring = str.substr( last, next - last );
    if ( substring.length() > 0 )
    {
      std::string sub = str.substr( last, next - last );
      sub.erase( std::remove( sub.begin(), sub.end(), ' ' ), sub.end() );
      result.push_back( sub );
    }
    last = next + 1;
  }

  substring = str.substr( last );
  substring.erase( std::remove( substring.begin(), substring.end(), ' ' ), substring.end() );
  result.push_back( substring );

  return result;
}

#ifndef _WIN32
inline std::string word_exp_filename( const std::string& filename )
{
  std::string result;

  wordexp_t p;
  wordexp( filename.c_str(), &p, 0 );

  for ( auto i = 0u; i < p.we_wordc; ++i )
  {
    if ( !result.empty() )
    {
      result += " ";
    }
    result += std::string( p.we_wordv[i] );
  }

  wordfree( &p );

  return result;
}
#else
inline const std::string& word_exp_filename( const std::string& filename )
{
  return filename;
}
#endif

#ifndef _WIN32
inline std::string basename( const std::string& filepath )
{
  return std::string( ::basename( const_cast<char*>( filepath.c_str() ) ) );
}
#endif

inline bool starts_with( std::string const& s, std::string const& match )
{
  return ( s.substr( 0, match.size() ) == match );
}

} // namespace detail
} // namespace lorina

/*! \endcond */
