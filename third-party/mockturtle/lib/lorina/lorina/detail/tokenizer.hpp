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

#pragma once

#include <lorina/detail/utils.hpp>
#include <iostream>
#include <string>

namespace lorina
{

namespace detail
{

enum class tokenizer_return_code
{
  invalid = 0
, valid   = 1
, comment = 2
};

class tokenizer
{
public:
  explicit tokenizer( std::istream& is )
    : _is( is )
  {}

  bool get_char( char& c )
  {
    if ( !lookahead.empty() )
    {
      c = *lookahead.rbegin();
      lookahead.pop_back();
      return true;
    }
    else
    {
      return bool(_is.get( c ));
    }
  }

  tokenizer_return_code get_token_internal( std::string& token )
  {
    if ( _done )
    {
      return tokenizer_return_code::invalid;
    }
    token = "";

    char c;
    while ( get_char( c ) )
    {
      if ( c == '\n' && _comment_mode )
      {
        _comment_mode = false;
        return tokenizer_return_code::comment;
      }
      else if ( !_comment_mode )
      {
        if ( ( c == ' ' || c == '\\' || c == '\n' ) && !_quote_mode )
        {
          return tokenizer_return_code::valid;
        }
        if ( ( c == '(' || c == ')' || c == '{' || c == '}' || c == ';' || c == ':' || c == ',' || c == '~' || c == '&' || c == '|' || c == '^' || c == '#' || c == '[' || c == ']' ) && !_quote_mode )
        {
          if ( token.empty() )
          {
            token = std::string() + c;
          }
          else
          {
            lookahead += c;
          }
          return tokenizer_return_code::valid;
        }

        if ( c == '\"' )
        {
          _quote_mode = !_quote_mode;
        }
      }

      token += c;
    }

    _done = true;
    return tokenizer_return_code::valid;
  }

  bool get_token( std::string& token )
  {
    tokenizer_return_code result;
    do
    {
      result = get_token_internal( token );
      detail::trim( token );

      /* keep parsing if token is empty */
    } while ( token == "" && result == tokenizer_return_code::valid );

    return ( result == tokenizer_return_code::valid );
  }

  void set_comment_mode( bool value = true )
  {
    _comment_mode = value;
  }

  bool get_comment_mode() const
  {
    return _comment_mode;
  }

protected:
  bool _done = false;
  bool _quote_mode = false;
  bool _comment_mode = false;
  std::istream& _is;
  std::string lookahead;
}; /* tokenizer */

} // namespace detail

} // namespace lorina
