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
  \file hash_functions.hpp
  \brief hash specializations for collections.

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace mockturtle
{

template<typename T>
struct hash
{
  std::hash<T> h;
  size_t operator()( const T& t ) const { return h( t ); }
};

template<typename A, typename B, typename C>
struct hash<std::tuple<A, B, C>>;

template<typename A, typename B>
struct hash<std::tuple<A, B>>;

template<typename A>
struct hash<std::vector<A>>;

template<typename A>
struct hash<std::multiset<A>>;

template<typename A, typename B>
struct hash<std::map<A, B>>;

template<typename A, typename B>
struct hash<std::tuple<A, B>>
{
public:
  size_t operator()( const std::tuple<const A, const B>& key ) const
  {
    size_t seed = ha( std::get<0>( key ) );
    seed ^= hb( std::get<1>( key ) ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    return seed;
  }

private:
  hash<A> ha;
  hash<B> hb;
};

template<typename A, typename B, typename C>
struct hash<std::tuple<A, B, C>>
{
public:
  size_t operator()( const std::tuple<const A, const B, const C>& key ) const
  {
    size_t seed = ha( std::get<0>( key ) );
    seed ^= hb( std::get<1>( key ) ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    seed ^= hc( std::get<2>( key ) ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    return seed;
  }

private:
  hash<A> ha;
  hash<B> hb;
  hash<C> hc;
};

template<typename A>
struct hash<std::vector<A>>
{
public:
  size_t operator()( const std::vector<A>& key ) const
  {
    std::size_t seed = key.size();
    for ( auto& i : key )
    {
      seed ^= ha( i ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }
    return seed;
  }

private:
  hash<A> ha;
};

template<typename A>
struct hash<std::multiset<std::multiset<A>>>
{
public:
  size_t operator()( const std::multiset<std::multiset<A>>& key ) const
  {
    size_t seed = key.size();
    for ( auto& x : key )
    {
      seed ^= ha( x ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }
    return seed;
  }

private:
  hash<A> ha;
};

template<typename A, typename B>
struct hash<std::map<A, B>>
{
public:
  size_t operator()( const std::map<A, B>& key ) const
  {
    size_t seed = key.size();
    for ( auto it = key.begin(); it != key.end(); it++ )
    {
      seed ^= ha( it->first ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
      seed ^= hb( it->second ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
    }
    return seed;
  }

private:
  hash<A> ha;
  hash<B> hb;
};

} // namespace mockturtle