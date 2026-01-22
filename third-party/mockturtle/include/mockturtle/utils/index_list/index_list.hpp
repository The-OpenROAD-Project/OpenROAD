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
  \file index_list.hpp
  \brief List of indices to represent small networks.

  \author Andrea Costamagna
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../../traits.hpp"

#include <fmt/format.h>

#include <array>
#include <vector>

namespace mockturtle
{

/*! \brief Index list for mux-inverter graphs.
 *
 * Small network consisting of mux gates and inverters
 * represented as a list of literals.
 *
 * Example: The following index list creates the output function
 * `<<x1 ? x2 : x3> ? x2 : x4>` with 4 inputs, 1 output, and 2 gates:
 * `{4 | 1 << 8 | 2 << 16, 2, 4, 6, 4, 8, 10, 12}`
 */
struct muxig_index_list
{
public:
  using element_type = uint32_t;

public:
  explicit muxig_index_list( uint32_t num_pis = 0 )
      : values( { num_pis } )
  {
  }

  explicit muxig_index_list( std::vector<element_type> const& values )
      : values( std::begin( values ), std::end( values ) )
  {}

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    return ( values.at( 0 ) >> 16 );
  }

  uint64_t num_pis() const
  {
    return values.at( 0 ) & 0xff;
  }

  uint64_t num_pos() const
  {
    return ( values.at( 0 ) >> 8 ) & 0xff;
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    assert( ( values.size() - 1u - num_pos() ) % 3 == 0 );
    for ( uint64_t i = 1u; i < values.size() - num_pos(); i += 3 )
    {
      fn( values.at( i ), values.at( i + 1 ), values.at( i + 2 ) );
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - num_pos(); i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void clear()
  {
    values.clear();
    values.emplace_back( 0 );
  }

  void add_inputs( uint32_t n = 1u )
  {
    assert( num_pis() + n <= 0xff );
    values.at( 0u ) += n;
  }

  element_type add_mux( element_type lit0, element_type lit1, element_type lit2 )
  {
    assert( num_gates() + 1u <= 0xffff );
    values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    values.push_back( lit0 );
    values.push_back( lit1 );
    values.push_back( lit2 );
    return ( num_gates() + num_pis() ) << 1;
  }

  void add_output( element_type lit )
  {
    assert( num_pos() + 1 <= 0xff );
    values.at( 0u ) = ( num_pos() + 1 ) << 8 | ( values.at( 0u ) & 0xffff00ff );
    values.push_back( lit );
  }

private:
  std::vector<element_type> values;
};

/*! \brief Inserts a muxig_index_list into an existing network
 *
 * **Required network functions:**
 * - `get_constant`
 * - `create_ite`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<bool useSignal = true, typename Ntk, typename BeginIter, typename EndIter, typename Fn>
void insert( Ntk& ntk, BeginIter begin, EndIter end, muxig_index_list const& indices, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_ite_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  if constexpr ( useSignal )
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal>, "BeginIter value_type must be Ntk signal type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal>, "EndIter value_type must be Ntk signal type" );
  }
  else
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, node>, "BeginIter value_type must be Ntk node type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, node>, "EndIter value_type must be Ntk node type" );
  }

  assert( uint64_t( std::distance( begin, end ) ) == indices.num_pis() );

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    if constexpr ( useSignal )
    {
      signals.push_back( *it );
    }
    else
    {
      signals.emplace_back( ntk.make_signal( *it ) );
    }
  }

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ) {
    signal const s0 = ( lit0 % 2 ) ? !signals.at( lit0 >> 1 ) : signals.at( lit0 >> 1 );
    signal const s1 = ( lit1 % 2 ) ? !signals.at( lit1 >> 1 ) : signals.at( lit1 >> 1 );
    signal const s2 = ( lit2 % 2 ) ? !signals.at( lit2 >> 1 ) : signals.at( lit2 >> 1 );
    signals.push_back( ntk.create_ite( s0, s1, s2 ) );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    uint32_t const i = lit >> 1;
    fn( ( lit % 2 ) ? !signals.at( i ) : signals.at( i ) );
  } );
}

/*! \brief Converts an mig_index_list to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_index_list_string( muxig_index_list const& indices )
{
  auto s = fmt::format( "{{{} pis | {} pos | {} gates", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ) {
    s += fmt::format( ", ({} ? {} : {})", lit0, lit1, lit2 );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  } );

  s += "}";

  return s;
}

/*! \brief Index list for majority-inverter graphs.
 *
 * Small network consisting of majority gates and inverters
 * represented as a list of literals.
 *
 * Example: The following index list creates the output function
 * `<<x1, x2, x3>, x2, x4>` with 4 inputs, 1 output, and 2 gates:
 * `{4 | 1 << 8 | 2 << 16, 2, 4, 6, 4, 8, 10, 12}`
 */
struct mig_index_list
{
public:
  using element_type = uint32_t;

public:
  explicit mig_index_list( uint32_t num_pis = 0 )
      : values( { num_pis } )
  {
  }

  explicit mig_index_list( std::vector<element_type> const& values )
      : values( std::begin( values ), std::end( values ) )
  {}

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    return ( values.at( 0 ) >> 16 );
  }

  uint64_t num_pis() const
  {
    return values.at( 0 ) & 0xff;
  }

  uint64_t num_pos() const
  {
    return ( values.at( 0 ) >> 8 ) & 0xff;
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    assert( ( values.size() - 1u - num_pos() ) % 3 == 0 );
    for ( uint64_t i = 1u; i < values.size() - num_pos(); i += 3 )
    {
      fn( values.at( i ), values.at( i + 1 ), values.at( i + 2 ) );
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - num_pos(); i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void clear()
  {
    values.clear();
    values.emplace_back( 0 );
  }

  void add_inputs( uint32_t n = 1u )
  {
    assert( num_pis() + n <= 0xff );
    values.at( 0u ) += n;
  }

  element_type add_maj( element_type lit0, element_type lit1, element_type lit2 )
  {
    assert( num_gates() + 1u <= 0xffff );
    values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    values.push_back( lit0 );
    values.push_back( lit1 );
    values.push_back( lit2 );
    return ( num_gates() + num_pis() ) << 1;
  }

  void add_output( element_type lit )
  {
    assert( num_pos() + 1 <= 0xff );
    values.at( 0u ) = ( num_pos() + 1 ) << 8 | ( values.at( 0u ) & 0xffff00ff );
    values.push_back( lit );
  }

private:
  std::vector<element_type> values;
};

/*! \brief Generates a mig_index_list from a network
 *
 * The function requires `ntk` to consist of majority gates.
 *
 * **Required network functions:**
 * - `foreach_fanin`
 * - `foreach_gate`
 * - `get_node`
 * - `is_complemented`
 * - `is_maj`
 * - `node_to_index`
 * - `num_gates`
 * - `num_pis`
 * - `num_pos`
 *
 * \param indices An index list
 * \param ntk A logic network
 */
template<typename Ntk>
void encode( mig_index_list& indices, Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.foreach_pi( [&]( node const& n, uint64_t index ) {
    if ( ntk.node_to_index( n ) != index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", index + 1 );
      std::abort();
    }
  } );

  /* inputs */
  indices.add_inputs( ntk.num_pis() );

  /* gates */
  ntk.foreach_gate( [&]( node const& n, uint64_t index ) {
    assert( ntk.is_maj( n ) );
    if ( ntk.node_to_index( n ) != ntk.num_pis() + index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    std::array<uint32_t, 3u> lits;
    ntk.foreach_fanin( n, [&]( signal const& fi, uint64_t index ) {
      if ( ntk.node_to_index( ntk.get_node( fi ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      lits[index] = 2 * ntk.node_to_index( ntk.get_node( fi ) ) + ntk.is_complemented( fi );
    } );
    indices.add_maj( lits[0u], lits[1u], lits[2u] );
  } );

  /* outputs */
  ntk.foreach_po( [&]( signal const& f ) {
    indices.add_output( 2 * ntk.node_to_index( ntk.get_node( f ) ) + ntk.is_complemented( f ) );
  } );

  assert( indices.size() == 1u + 3u * ntk.num_gates() + ntk.num_pos() );
}

/*! \brief Inserts a mig_index_list into an existing network
 *
 * **Required network functions:**
 * - `get_constant`
 * - `create_maj`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<bool useSignal = true, typename Ntk, typename BeginIter, typename EndIter, typename Fn>
void insert( Ntk& ntk, BeginIter begin, EndIter end, mig_index_list const& indices, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  if constexpr ( useSignal )
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal>, "BeginIter value_type must be Ntk signal type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal>, "EndIter value_type must be Ntk signal type" );
  }
  else
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, node>, "BeginIter value_type must be Ntk node type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, node>, "EndIter value_type must be Ntk node type" );
  }

  assert( uint64_t( std::distance( begin, end ) ) == indices.num_pis() );

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    if constexpr ( useSignal )
    {
      signals.push_back( *it );
    }
    else
    {
      signals.emplace_back( ntk.make_signal( *it ) );
    }
  }

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ) {
    signal const s0 = ( lit0 % 2 ) ? !signals.at( lit0 >> 1 ) : signals.at( lit0 >> 1 );
    signal const s1 = ( lit1 % 2 ) ? !signals.at( lit1 >> 1 ) : signals.at( lit1 >> 1 );
    signal const s2 = ( lit2 % 2 ) ? !signals.at( lit2 >> 1 ) : signals.at( lit2 >> 1 );
    signals.push_back( ntk.create_maj( s0, s1, s2 ) );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    uint32_t const i = lit >> 1;
    fn( ( lit % 2 ) ? !signals.at( i ) : signals.at( i ) );
  } );
}

/*! \brief Converts an mig_index_list to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_index_list_string( mig_index_list const& indices )
{
  auto s = fmt::format( "{{{} | {} << 8 | {} << 16", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1, uint32_t lit2 ) {
    s += fmt::format( ", {}, {}, {}", lit0, lit1, lit2 );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  } );

  s += "}";

  return s;
}

/*! \brief Index list for xor-and graphs.
 *
 * Small network represented as a list of literals. Supports XOR and
 * AND gates.  The list has the following 32-bit unsigned integer
 * elements.  It starts with a signature whose partitioned into `|
 * num_gates | num_pos | num_pis |`, where `num_gates` accounts for
 * the most-significant 16 bits, `num_pos` accounts for 8 bits, and
 * `num_pis` accounts for the least-significant 8 bits.  Afterwards,
 * gates are defined as literal indexes `(2 * i + c)`, where `i` is an
 * index, with 0 indexing the constant 0, 1 to `num_pis` indexing the
 * primary inputs, and all successive indexes for the gates.  Gate
 * literals come in pairs.  If the first literal has a smaller value
 * than the second one, an AND gate is created, otherwise, an XOR gate
 * is created.  Afterwards, all outputs are defined in terms of
 * literals.
 *
 * Example: The following index list creates the output function `(x1
 * AND x2) XOR (x3 AND x4)` with 4 inputs, 1 output, and 3 gates:
 * `{4 | 1 << 8 | 3 << 16, 2, 4, 6, 8, 12, 10, 14}`
 *
 * Note: if `separate_header = true`, the header will be split into 3
 * elements to support networks with larger number of PIs.
 */
template<bool separate_header = false>
struct xag_index_list
{
public:
  using element_type = uint32_t;
  /* Literal used for the first primary input */
  static constexpr uint32_t offset = 1;

public:
  explicit xag_index_list( uint32_t num_pis = 0 )
      : values( { num_pis } )
  {
    if constexpr ( separate_header )
    {
      values.emplace_back( 0 );
      values.emplace_back( 0 );
    }
  }

  explicit xag_index_list( std::vector<element_type> const& values )
      : values( std::begin( values ), std::end( values ) )
  {}

  std::vector<element_type> raw() const
  {
    return values;
  }

  uint64_t size() const
  {
    return values.size();
  }

  uint64_t num_gates() const
  {
    if constexpr ( separate_header )
    {
      return values.at( 2 );
    }
    return ( values.at( 0 ) >> 16 );
  }

  uint64_t num_pis() const
  {
    if constexpr ( separate_header )
    {
      return values.at( 0 );
    }
    return values.at( 0 ) & 0xff;
  }

  uint64_t num_pos() const
  {
    if constexpr ( separate_header )
    {
      return values.at( 1 );
    }
    return ( values.at( 0 ) >> 8 ) & 0xff;
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    if constexpr ( separate_header )
    {
      assert( ( values.size() - 3u - num_pos() ) % 2 == 0 );
      for ( uint64_t i = 3u; i < values.size() - num_pos(); i += 2 )
      {
        fn( values.at( i ), values.at( i + 1 ) );
      }
    }
    else
    {
      assert( ( values.size() - 1u - num_pos() ) % 2 == 0 );
      for ( uint64_t i = 1u; i < values.size() - num_pos(); i += 2 )
      {
        fn( values.at( i ), values.at( i + 1 ) );
      }
    }
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    for ( uint64_t i = values.size() - num_pos(); i < values.size(); ++i )
    {
      fn( values.at( i ) );
    }
  }

  void clear()
  {
    values.clear();
    values.emplace_back( 0 );
    if constexpr ( separate_header )
    {
      values.emplace_back( 0 );
      values.emplace_back( 0 );
    }
  }

  void add_inputs( uint32_t n = 1u )
  {
    if constexpr ( !separate_header )
    {
      assert( num_pis() + n <= 0xff );
    }
    values.at( 0u ) += n;
  }

  element_type add_and( element_type lit0, element_type lit1 )
  {
    if constexpr ( separate_header )
    {
      values.at( 2u ) += 1;
    }
    else
    {
      assert( num_gates() + 1u <= 0xffff );
      values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    }

    values.push_back( lit0 < lit1 ? lit0 : lit1 );
    values.push_back( lit0 < lit1 ? lit1 : lit0 );
    return ( num_gates() + num_pis() ) << 1;
  }

  element_type add_xor( element_type lit0, element_type lit1 )
  {
    if constexpr ( separate_header )
    {
      values.at( 2u ) += 1;
    }
    else
    {
      assert( num_gates() + 1u <= 0xffff );
      values.at( 0u ) = ( ( num_gates() + 1 ) << 16 ) | ( values.at( 0 ) & 0xffff );
    }

    values.push_back( lit0 > lit1 ? lit0 : lit1 );
    values.push_back( lit0 > lit1 ? lit1 : lit0 );
    return ( num_gates() + num_pis() ) << 1;
  }

  void add_output( element_type lit )
  {
    if constexpr ( separate_header )
    {
      values.at( 1u ) += 1;
    }
    else
    {
      assert( num_pos() + 1 <= 0xff );
      values.at( 0u ) = ( num_pos() + 1 ) << 8 | ( values.at( 0u ) & 0xffff00ff );
    }

    values.push_back( lit );
  }

  /*! \brief Shift right the literal, removing the complementation bit */
  inline uint32_t get_index( element_type const& lit ) const
  {
    return lit >> 1;
  }

  /*! \brief Returns the node index excluding the constants and the inputs */
  inline uint32_t get_node_index( element_type const& lit ) const
  {
    return get_index( lit ) - num_pis() - offset;
  }

  /*! \brief Returns index of an input literal */
  inline uint32_t get_pi_index( element_type const& lit ) const
  {
    return get_index( lit ) - offset;
  }

  /*! \brief When the literal is even ( bit0 = 0 ), it is not complemented */
  inline bool is_complemented( element_type const& lit ) const
  {
    return lit % 2;
  }

  /*! \brief The AND gate is distinguished by the EXOR gate based on the order of the input literals */
  inline bool is_and( element_type const& lit_lhs, element_type const& lit_rhs ) const
  {
    return lit_lhs < lit_rhs;
  }

  /*! \brief Check if a literal is a PI
   *
   * The index of the literal is shifted based on the usage of the separate_header.
   * There are two cases identifying that the literal is not an input:
   * -  The shifted index overflows : the literal was a constant 0.aig_hash
   * -  The shifted integer is larger than the number of inputs.
   */
  inline bool is_pi( element_type const& lit ) const
  {
    uint32_t index = get_index( lit );
    return index < num_pis() + 1;
  }

  inline bool is_constant( element_type const& lit ) const
  {
    return lit <= 1;
  }

  inline element_type po_at( uint32_t index )
  {
    assert( index < num_pos() );
    return *( values.end() - num_pos() + index );
  }

private:
  std::vector<element_type> values;
};

using large_xag_index_list = xag_index_list<true>;

/*! \brief Generates a xag_index_list from a network
 *
 * The function requires `ntk` to consist of XOR and AND gates.
 *
 * **Required network functions:**
 * - `foreach_fanin`
 * - `foreach_gate`
 * - `get_node`
 * - `is_and`
 * - `is_complemented`
 * - `is_xor`
 * - `node_to_index`
 * - `num_gates`
 * - `num_pis`
 * - `num_pos`
 *
 * \param indices An index list
 * \param ntk A logic network
 */
template<typename Ntk, bool separate_header = false>
void encode( xag_index_list<separate_header>& indices, Ntk const& ntk )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_num_gates_v<Ntk>, "Ntk does not implement the num_gates method" );
  static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
  static_assert( has_num_pos_v<Ntk>, "Ntk does not implement the num_pos method" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  ntk.foreach_pi( [&]( node const& n, uint64_t index ) {
    if ( ntk.node_to_index( n ) != index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by PI {})\n", index + 1 );
      std::abort();
    }
  } );

  /* inputs */
  indices.add_inputs( ntk.num_pis() );

  /* gates */
  ntk.foreach_gate( [&]( node const& n, uint64_t index ) {
    assert( ntk.is_and( n ) || ntk.is_xor( n ) );
    if ( ntk.node_to_index( n ) != ntk.num_pis() + index + 1 )
    {
      fmt::print( "[e] network is not in normalized index order (violated by node {})\n", ntk.node_to_index( n ) );
      std::abort();
    }

    std::array<uint32_t, 2u> lits;
    ntk.foreach_fanin( n, [&]( signal const& fi, uint64_t index ) {
      if ( ntk.node_to_index( ntk.get_node( fi ) ) > ntk.node_to_index( n ) )
      {
        fmt::print( "[e] node {} not in topological order\n", ntk.node_to_index( n ) );
        std::abort();
      }
      lits[index] = 2 * ntk.node_to_index( ntk.get_node( fi ) ) + ntk.is_complemented( fi );
    } );

    if ( ntk.is_and( n ) )
    {
      indices.add_and( lits[0u], lits[1u] );
    }
    else if ( ntk.is_xor( n ) )
    {
      indices.add_xor( lits[0u], lits[1u] );
    }
  } );

  /* outputs */
  ntk.foreach_po( [&]( signal const& f ) {
    indices.add_output( 2 * ntk.node_to_index( ntk.get_node( f ) ) + ntk.is_complemented( f ) );
  } );

  if constexpr ( separate_header )
  {
    assert( indices.size() == 3u + 2u * ntk.num_gates() + ntk.num_pos() );
  }
  else
  {
    assert( indices.size() == 1u + 2u * ntk.num_gates() + ntk.num_pos() );
  }
}

/*! \brief Inserts a xag_index_list into an existing network
 *
 * **Required network functions:**
 * - `create_and`
 * - `create_xor`
 * - `get_constant`
 *
 * \param ntk A logic network
 * \param begin Begin iterator of signal inputs
 * \param end End iterator of signal inputs
 * \param indices An index list
 * \param fn Callback function
 */
template<bool useSignal = true, typename Ntk, typename BeginIter, typename EndIter, typename Fn, bool separate_header = false>
void insert( Ntk& ntk, BeginIter begin, EndIter end, xag_index_list<separate_header> const& indices, Fn&& fn )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_and_v<Ntk>, "Ntk does not implement the create_and method" );
  static_assert( has_create_xor_v<Ntk>, "Ntk does not implement the create_xor method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );

  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  if constexpr ( useSignal )
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, signal>, "BeginIter value_type must be Ntk signal type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, signal>, "EndIter value_type must be Ntk signal type" );
  }
  else
  {
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<BeginIter>::value_type>, node>, "BeginIter value_type must be Ntk node type" );
    static_assert( std::is_same_v<std::decay_t<typename std::iterator_traits<EndIter>::value_type>, node>, "EndIter value_type must be Ntk node type" );
  }

  assert( uint64_t( std::distance( begin, end ) ) == indices.num_pis() );

  std::vector<signal> signals;
  signals.emplace_back( ntk.get_constant( false ) );
  for ( auto it = begin; it != end; ++it )
  {
    if constexpr ( useSignal )
    {
      signals.push_back( *it );
    }
    else
    {
      signals.emplace_back( ntk.make_signal( *it ) );
    }
  }

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1 ) {
    assert( lit0 != lit1 );
    uint32_t const i0 = indices.get_index( lit0 );
    uint32_t const i1 = indices.get_index( lit1 );
    signal const s0 = indices.is_complemented( lit0 ) ? ntk.create_not( signals.at( i0 ) ) : signals.at( i0 );
    signal const s1 = indices.is_complemented( lit1 ) ? ntk.create_not( signals.at( i1 ) ) : signals.at( i1 );
    signals.push_back( lit0 > lit1 ? ntk.create_xor( s0, s1 ) : ntk.create_and( s0, s1 ) );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    uint32_t const i = indices.get_index( lit );
    fn( indices.is_complemented( lit ) ? ntk.create_not( signals.at( i ) ) : signals.at( i ) );
  } );
}

/*! \brief Converts an xag_index_list to a string
 *
 * \param indices An index list
 * \return A string representation of the index list
 */
inline std::string to_index_list_string( xag_index_list<false> const& indices )
{
  auto s = fmt::format( "{{{} | {} << 8 | {} << 16", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1 ) {
    s += fmt::format( ", {}, {}", lit0, lit1 );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  } );

  s += "}";

  return s;
}

inline std::string to_index_list_string( xag_index_list<true> const& indices )
{
  auto s = fmt::format( "{{{}, {}, {}", indices.num_pis(), indices.num_pos(), indices.num_gates() );

  indices.foreach_gate( [&]( uint32_t lit0, uint32_t lit1 ) {
    s += fmt::format( ", {}, {}", lit0, lit1 );
  } );

  indices.foreach_po( [&]( uint32_t lit ) {
    s += fmt::format( ", {}", lit );
  } );

  s += "}";

  return s;
}

/*! \brief Generates a network from an index_list
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 *
 * \param ntk A logic network
 * \param indices An index list
 */
template<typename Ntk, typename IndexList>
void decode( Ntk& ntk, IndexList const& indices )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );

  using signal = typename Ntk::signal;

  std::vector<signal> signals( indices.num_pis() );
  std::generate( std::begin( signals ), std::end( signals ),
                 [&]() { return ntk.create_pi(); } );

  insert( ntk, std::begin( signals ), std::end( signals ), indices,
          [&]( signal const& s ) { ntk.create_po( s ); } );
}

/*! \brief Enumerate structured index_lists
 *
 * Enumerate concrete `xag_index_list`s from an abstract index list
 * specification.  The specifiation is provided in an extended index
 * list format, where a `-1` indicates an unspecified input.
 *
 * The algorithm concretizes unspecified inputs and negates nodes.
 *
 * The enumerator generates the following concrete index lists
 *    {2 | 1 << 8 | 1 << 16, 2, 4, 6}
 *    {2 | 1 << 8 | 1 << 16, 2, 4, 7}
 *    {2 | 1 << 8 | 1 << 16, 3, 4, 6}
 *    {2 | 1 << 8 | 1 << 16, 3, 4, 7}
 *    {2 | 1 << 8 | 1 << 16, 3, 5, 6}
 *    {2 | 1 << 8 | 1 << 16, 3, 5, 7}
 *    {2 | 1 << 8 | 1 << 16, 2, 5, 6}
 *    {2 | 1 << 8 | 1 << 16, 2, 5, 7}
 * from the abstract index list specification `{ -1, -1, 6 }`.
   \verbatim embed:rst

   Example

   .. code-block:: c++

      aig_index_list_enumerator e( { -1, -1, -1, 6, 8 }, 2u, 2u, 1u );
      e.run( [&]( xag_index_list const& il ) {
        aig_network aig;
        decode( aig, il );
      } );
   \endverbatim
 */
class aig_index_list_enumerator
{
public:
  explicit aig_index_list_enumerator( std::vector<int32_t> const& values, uint32_t num_pis, uint32_t num_gates, uint32_t num_pos )
      : values_( values ), num_pis( num_pis ), num_gates( num_gates ), num_pos( num_pos )
  {}

  template<typename Fn>
  void run( Fn&& fn )
  {
    recurse( values_, 0u, fn );
  }

protected:
  template<typename Fn>
  void recurse( std::vector<int32_t> values, uint32_t pos, Fn&& fn )
  {
    /* process gate */
    if ( pos < 2 * num_gates )
    {
      auto& a = values.at( pos );
      auto& b = values.at( pos + 1 );
      if ( a == -1 && b == -1 )
      {
        for ( uint32_t i = 0u; i < num_pis; ++i )
        {
          a = ( i + 1 ) << 1;
          for ( uint32_t j = i + 1; j < num_pis; ++j )
          {
            b = ( j + 1 ) << 1;
            recurse( values, pos + 2, fn );
            a = a ^ 1;
            recurse( values, pos + 2, fn );
            b = b ^ 1;
            recurse( values, pos + 2, fn );
            a = a ^ 1;
            recurse( values, pos + 2, fn );
            b = b ^ 1;
          }
        }
        return;
      }
      else if ( a == -1 )
      {
        for ( uint32_t i = 0u; i < num_pis; ++i )
        {
          a = ( i + 1 ) << 1;
          recurse( values, pos + 2, fn );
          a = a ^ 1;
          recurse( values, pos + 2, fn );
          b = b ^ 1;
          recurse( values, pos + 2, fn );
          a = a ^ 1;
          recurse( values, pos + 2, fn );
          b = b ^ 1;
        }
        return;
      }
      else if ( b == -1 )
      {
        for ( uint32_t i = 0u; i < num_pis; ++i )
        {
          b = ( i + 1 ) << 1;
          recurse( values, pos + 2, fn );
          a = a ^ 1;
          recurse( values, pos + 2, fn );
          b = b ^ 1;
          recurse( values, pos + 2, fn );
          a = a ^ 1;
          recurse( values, pos + 2, fn );
          b = b ^ 1;
        }
        return;
      }

      recurse( values, pos + 2, fn );
      a = a ^ 1;
      recurse( values, pos + 2, fn );
      b = b ^ 1;
      recurse( values, pos + 2, fn );
      a = a ^ 1;
      recurse( values, pos + 2, fn );
      b = b ^ 1;
      return;
    }

    /* process output */
    if ( pos < values.size() )
    {
      auto& o = values.at( pos );
      recurse( values, pos + 1u, fn );
      o = o ^ 1;
      recurse( values, pos + 1u, fn );
      return;
    }

    /* finished processing values */
    std::vector<uint32_t> index_list;
    index_list.emplace_back( num_pis | ( num_pos << 8 ) | ( num_gates << 16 ) );
    for ( int32_t const& v : values )
    {
      index_list.emplace_back( v );
    }
    fn( xag_index_list( index_list ) );
  }

protected:
  std::vector<int32_t> values_;
  uint32_t num_pis;
  uint32_t num_gates;
  uint32_t num_pos;
};

template<class T>
struct is_index_list : std::false_type
{
};

template<>
struct is_index_list<xag_index_list<true>> : std::true_type
{
};

template<>
struct is_index_list<xag_index_list<false>> : std::true_type
{
};

template<>
struct is_index_list<mig_index_list> : std::true_type
{
};

template<class T>
inline constexpr bool is_index_list_v = is_index_list<T>::value;

} // namespace mockturtle
