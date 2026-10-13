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
  \file utils.hpp
  \brief Helper functions

  \author Mathias Soeken
*/

/*! \cond PRIVATE */
#pragma once

#include <algorithm>
#include <string>

namespace kitty
{

namespace detail
{

/* string utils are from https://stackoverflow.com/a/217605 */
inline void ltrim( std::string& s )
{
  s.erase( s.begin(), std::find_if( s.begin(), s.end(), []( int ch )
                                    { return std::isspace( ch ) == 0; } ) );
}

inline void rtrim( std::string& s )
{
  s.erase( std::find_if( s.rbegin(), s.rend(), []( int ch )
                         { return std::isspace( ch ) == 0; } )
               .base(),
           s.end() );
}

inline void trim( std::string& s )
{
  ltrim( s );
  rtrim( s );
}

inline std::string ltrim_copy( std::string s )
{
  ltrim( s );
  return s;
}

inline std::string rtrim_copy( std::string s )
{
  rtrim( s );
  return s;
}

inline std::string trim_copy( std::string s )
{
  trim( s );
  return s;
}
} /* namespace detail */
} /* namespace kitty */
  /*! \endcond */