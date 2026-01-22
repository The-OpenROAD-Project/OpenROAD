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
  \file serialize.hpp
  \brief Serialize network into a file

  \author Bruno Schmitt
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee

  This file implements functions to serialize a (combinational)
  `aig_network` into a file.  The serializer should be used for
  debugging-purpose only.  It allows to store the current state of the
  network (including dangling and dead nodes), but does not guarantee
  platform-independence (use, e.g., `write_verilog` instead).
*/

#pragma once

#include "../networks/aig.hpp"
#include <fstream>
#include <optional>
#include <parallel_hashmap/phmap_dump.h>

namespace mockturtle
{

namespace detail
{

struct serializer
{
public:
  using node_type = typename aig_network::storage::element_type::node_type;
  using pointer_type = typename node_type::pointer_type;

public:
  bool operator()( phmap::BinaryOutputArchive& os, uint64_t const& data ) const
  {
    return os.dump( (char*)&data, sizeof( uint64_t ) );
  }

  bool operator()( phmap::BinaryInputArchive& ar_input, uint64_t* data ) const
  {
    return ar_input.load( (char*)data, sizeof( uint64_t ) );
  }

  template<int PointerFieldSize>
  bool operator()( phmap::BinaryOutputArchive& os, node_pointer<PointerFieldSize> const& ptr ) const
  {
    return os.dump( (char*)&ptr.data, sizeof( ptr.data ) );
  }

  template<int PointerFieldSize>
  bool operator()( phmap::BinaryInputArchive& ar_input, node_pointer<PointerFieldSize>* ptr ) const
  {
    return ar_input.load( (char*)&ptr->data, sizeof( ptr->data ) );
  }

  bool operator()( phmap::BinaryOutputArchive& os, cauint64_t const& data ) const
  {
    return os.dump( (char*)&data.n, sizeof( data.n ) );
  }

  bool operator()( phmap::BinaryInputArchive& ar_input, cauint64_t* data ) const
  {
    return ar_input.load( (char*)&data->n, sizeof( data->n ) );
  }

  template<int Fanin, int Size, int PointerFieldSize>
  bool operator()( phmap::BinaryOutputArchive& os, regular_node<Fanin, Size, PointerFieldSize> const& n ) const
  {
    uint64_t size = n.children.size();
    if ( !os.dump( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }

    for ( const auto& c : n.children )
    {
      bool result = this->operator()( os, c );
      if ( !result )
      {
        return false;
      }
    }

    size = n.data.size();
    if ( !os.dump( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( const auto& d : n.data )
    {
      bool result = this->operator()( os, d );
      if ( !result )
      {
        return false;
      }
    }

    return true;
  }

  template<int Fanin, int Size, int PointerFieldSize>
  bool operator()( phmap::BinaryInputArchive& ar_input, const regular_node<Fanin, Size, PointerFieldSize>* n ) const
  {
    uint64_t size;
    if ( !ar_input.load( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }

    for ( uint64_t i = 0; i < size; ++i )
    {
      pointer_type ptr;
      bool result = this->operator()( ar_input, &ptr );
      if ( !result )
      {
        return false;
      }
      const_cast<regular_node<Fanin, Size, PointerFieldSize>*>( n )->children[i] = ptr;
    }

    ar_input.load( (char*)&size, sizeof( uint64_t ) );
    for ( uint64_t i = 0; i < size; ++i )
    {
      cauint64_t data;
      bool result = this->operator()( ar_input, &data );
      if ( !result )
      {
        return false;
      }
      const_cast<regular_node<Fanin, Size, PointerFieldSize>*>( n )->data[i] = data;
    }

    return true;
  }

  bool operator()( phmap::BinaryOutputArchive& os, std::pair<const node_type, uint64_t> const& value ) const
  {
    return this->operator()( os, value.first ) && this->operator()( os, value.second );
  }

  bool operator()( phmap::BinaryInputArchive& ar_input, std::pair<const node_type, uint64_t>* value ) const
  {
    return this->operator()( ar_input, &value->first ) && this->operator()( ar_input, &value->second );
  }

  bool operator()( phmap::BinaryOutputArchive& os, aig_storage const& storage ) const
  {
    /* nodes */
    uint64_t size = storage.nodes.size();
    if ( !os.dump( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( const auto& n : storage.nodes )
    {
      if ( !this->operator()( os, n ) )
      {
        return false;
      }
    }

    /* inputs */
    size = storage.inputs.size();
    if ( !os.dump( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( const auto& i : storage.inputs )
    {
      if ( !this->operator()( os, i ) )
      {
        return false;
      }
    }

    /* outputs */
    size = storage.outputs.size();
    if ( !os.dump( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( const auto& o : storage.outputs )
    {
      if ( !this->operator()( os, o ) )
      {
        return false;
      }
    }

    /* hash */
    if ( !const_cast<aig_storage&>( storage ).hash.dump( os ) )
    {
      return false;
    }

    if ( !os.dump( (char*)&storage.trav_id, sizeof( uint32_t ) ) )
    {
      return false;
    }

    return true;
  }

  bool operator()( phmap::BinaryInputArchive& ar_input, aig_storage* storage ) const
  {
    /* nodes */
    uint64_t size;
    if ( !ar_input.load( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( uint64_t i = 0; i < size; ++i )
    {
      node_type n;
      if ( !this->operator()( ar_input, &n ) )
      {
        return false;
      }
      storage->nodes.push_back( n );
    }

    /* inputs */
    if ( !ar_input.load( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( uint64_t i = 0; i < size; ++i )
    {
      uint64_t value;
      if ( !ar_input.load( (char*)&value, sizeof( uint64_t ) ) )
      {
        return false;
      }
      storage->inputs.push_back( value );
    }

    /* outputs */
    if ( !ar_input.load( (char*)&size, sizeof( uint64_t ) ) )
    {
      return false;
    }
    for ( uint64_t i = 0; i < size; ++i )
    {
      pointer_type ptr;
      if ( !this->operator()( ar_input, &ptr ) )
      {
        return false;
      }
      storage->outputs.push_back( ptr );
    }

    /* hash */
    if ( !storage->hash.load( ar_input ) )
    {
      return false;
    }

    if ( !ar_input.load( (char*)&storage->trav_id, sizeof( uint32_t ) ) )
    {
      return false;
    }

    return true;
  }
}; /* struct serializer */

} /* namespace detail */

/*! \brief Serializes a combinational AIG network to a archive, returning false on failure
 *
 * \param aig Combinational AIG network
 * \param os Output archive
 */
inline bool serialize_network_fallible( aig_network const& aig, phmap::BinaryOutputArchive& os )
{
  detail::serializer _serializer;
  return _serializer( os, *aig._storage );
}

/*! \brief Serializes a combinational AIG network to a archive
 *
 * \param aig Combinational AIG network
 * \param os Output archive
 */
inline void serialize_network( aig_network const& aig, phmap::BinaryOutputArchive& os )
{
  bool const okay = serialize_network_fallible( aig, os );
  (void)okay;
  assert( okay && "failed to serialize the network onto stream" );
}

/*! \brief Serializes a combinational AIG network in a file
 *
 * \param aig Combinational AIG network
 * \param filename Filename
 */
inline void serialize_network( aig_network const& aig, std::string const& filename )
{
  phmap::BinaryOutputArchive ar_out( filename.c_str() );
  serialize_network( aig, ar_out );
}

/*! \brief Deserializes a combinational AIG network from a input archive, returning nullopt on failure
 *
 * \param ar_input Input archive
 * \return Deserialized AIG network
 */
inline std::optional<aig_network> deserialize_network_fallible( phmap::BinaryInputArchive& ar_input )
{
  detail::serializer _serializer;
  auto storage = std::make_shared<aig_storage>();
  storage->nodes.clear();
  storage->inputs.clear();
  storage->outputs.clear();
  storage->hash.clear();

  if ( _serializer( ar_input, storage.get() ) )
  {
    return aig_network{ storage };
  }

  return std::nullopt;
}

/*! \brief Deserializes a combinational AIG network from a input archive
 *
 * \param ar_input Input archive
 * \return Deserialized AIG network
 */
inline aig_network deserialize_network( phmap::BinaryInputArchive& ar_input )
{
  auto result = deserialize_network_fallible( ar_input );
  (void)result.has_value();
  assert( result.has_value() && "failed to deserialize the network onto stream" );
  return *result;
}

/*! \brief Deserializes a combinational AIG network from a file
 *
 * \param filename Filename
 * \return Deserialized AIG network
 */
inline aig_network deserialize_network( std::string const& filename )
{
  phmap::BinaryInputArchive ar_input( filename.c_str() );
  auto aig = deserialize_network( ar_input );
  return aig;
}

} /* namespace mockturtle */