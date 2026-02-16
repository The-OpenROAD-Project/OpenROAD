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
  \file storage.hpp
  \brief Configurable storage container

  \author Alessandro Tempia Calvino
  \author Andrea Costamagna
  \author Bruno Schmitt
  \author Heinz Riener
  \author Jinzheng Tu
  \author Mathias Soeken
  \author Max Austin
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <array>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <parallel_hashmap/phmap.h>

namespace mockturtle
{

template<int PointerFieldSize = 0>
struct node_pointer
{
private:
  static constexpr auto _len = sizeof( uint64_t ) * 8;

public:
  node_pointer() = default;
  node_pointer( uint64_t index, uint64_t weight ) : weight( weight ), index( index ) {}
  node_pointer( uint64_t data ) : data( data ) {}

  union
  {
    struct
    {
      uint64_t weight : PointerFieldSize;
      uint64_t index : _len - PointerFieldSize;
    };
    uint64_t data;
  };

  bool operator==( node_pointer<PointerFieldSize> const& other ) const
  {
    return data == other.data;
  }

  bool operator!=( node_pointer<PointerFieldSize> const& other ) const
  {
    return data != other.data;
  }
};

template<>
struct node_pointer<0>
{
public:
  node_pointer() = default;
  node_pointer( uint64_t index ) : index( index ) {}

  union
  {
    uint64_t index;
    uint64_t data;
  };

  bool operator==( node_pointer<0> const& other ) const
  {
    return data == other.data;
  }
};

union cauint64_t
{
  uint64_t n{ 0 };
  struct
  {
    uint64_t h1 : 32;
    uint64_t h2 : 32;
  };
  struct
  {
    uint64_t q1 : 16;
    uint64_t q2 : 16;
    uint64_t q3 : 16;
    uint64_t q4 : 16;
  };
};

template<int Fanin, int Size = 0, int PointerFieldSize = 0>
struct regular_node
{
  using pointer_type = node_pointer<PointerFieldSize>;

  std::array<pointer_type, Fanin> children;
  std::array<cauint64_t, Size> data;

  bool operator==( regular_node<Fanin, Size, PointerFieldSize> const& other ) const
  {
    return children == other.children;
  }
};

template<int Size = 0, int PointerFieldSize = 0>
struct mixed_fanin_node
{
  using pointer_type = node_pointer<PointerFieldSize>;

  std::vector<pointer_type> children;
  std::array<cauint64_t, Size> data;

  bool operator==( mixed_fanin_node<Size, PointerFieldSize> const& other ) const
  {
    return children == other.children;
  }
};

template<int PointerFieldSize = 0>
struct block_fanin_node
{
  using pointer_type = node_pointer<PointerFieldSize>;

  std::vector<pointer_type> children;
  std::vector<cauint64_t> data;

  bool operator==( block_fanin_node<PointerFieldSize> const& other ) const
  {
    return children == other.children;
  }
};

/*! \brief Hash function for 64-bit word */
inline uint64_t hash_block( uint64_t word )
{
  /* from boost::hash_detail::hash_value_unsigned */
  return word ^ ( word + ( word << 6 ) + ( word >> 2 ) );
}

/*! \brief Combines two hash values */
inline void hash_combine( uint64_t& seed, uint64_t other )
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

template<typename Node>
struct node_hash
{
  uint64_t operator()( const Node& n ) const
  {
    if ( n.children.size() == 0 )
      return 0;

    auto it = std::begin( n.children );
    auto seed = hash_block( it->data );
    ++it;

    while ( it != std::end( n.children ) )
    {
      hash_combine( seed, hash_block( it->data ) );
      ++it;
    }

    return seed;
  }
};

struct empty_storage_data
{
};

template<typename Node, typename T = empty_storage_data, typename NodeHasher = node_hash<Node>>
struct storage
{
  storage()
  {
    nodes.reserve( 10000u );
    hash.reserve( 10000u );

    /* we generally reserve the first node for a constant */
    nodes.emplace_back();
  }

  using node_type = Node;

  uint32_t trav_id = 0u;

  std::vector<node_type> nodes;
  std::vector<uint64_t> inputs;
  std::vector<typename node_type::pointer_type> outputs;

  phmap::flat_hash_map<node_type, uint64_t, NodeHasher> hash;

  T data;
};

template<typename Node, typename T = empty_storage_data>
struct storage_no_hash
{
  storage_no_hash()
  {
    nodes.reserve( 10000u );

    /* we generally reserve the first node for a constant */
    nodes.emplace_back();
  }

  using node_type = Node;

  uint32_t trav_id = 0u;

  std::vector<node_type> nodes;
  std::vector<uint64_t> inputs;
  std::vector<typename node_type::pointer_type> outputs;

  T data;
};

} /* namespace mockturtle */