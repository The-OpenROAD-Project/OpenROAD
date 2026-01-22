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
  \file sequential.hpp
  \brief Sequential extension to networks

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"
#include "aig.hpp"
#include "aqfp.hpp"
#include "cover.hpp"
#include "detail/foreach.hpp"
#include "klut.hpp"
#include "mig.hpp"
#include "xag.hpp"
#include "xmg.hpp"

namespace mockturtle
{

namespace detail
{

template<class Ntk>
struct is_aig_like : std::false_type
{
};

template<>
struct is_aig_like<aig_network> : std::true_type
{
};
template<>
struct is_aig_like<xag_network> : std::true_type
{
};
template<>
struct is_aig_like<mig_network> : std::true_type
{
};
template<>
struct is_aig_like<xmg_network> : std::true_type
{
};
template<>
struct is_aig_like<aqfp_network> : std::true_type
{
};

template<class Ntk>
inline constexpr bool is_aig_like_v = is_aig_like<Ntk>::value;

} // namespace detail

/*! \brief Register information */
struct register_t
{
  /*! \brief Control (clocking or enabling) signal */
  std::string control = "";
  /*! \brief Initial (reset) value */
  uint8_t init = 3;
  /*! \brief Type of register or latch (active high/low, rising/falling edge) */
  std::string type = "";
};

template<typename Ntk, bool aig_like = detail::is_aig_like_v<typename Ntk::base_type>>
class sequential
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /*! \brief Creates a register output in the network.
   *
   * Each created register output is stored in a node and contributes
   * to the size of the network.  Register outputs must be created
   * after all primary inputs have been created and must have a
   * corresponding register input that is created with `create_ri`.
   *
   * Register outputs serve as inputs for the network.
   *
   * Register outputs and register inputs always have to be created in
   * pairs; they are associated to each other by index, i.e., the
   * first created register output corresponds to the first created
   * register input, etc.
   */
  signal create_ro();

  /*! \brief Creates a register input in the network.
   *
   * A register input is not stored in terms of a node, and it also
   * does not contribute to the size of the network.  A register input
   * is created for a signal in the network and it is possible that
   * multiple register inputs point to the same signal.  Register
   * inputs must be created after all primary outputs have been
   * created and must have a corresponding register output that is
   * created with `create_ro`.
   *
   * Register inputs serve as outputs for the network.
   *
   * Register outputs and register inputs always have to be created in
   * pairs; they are associated to each other by index, i.e., the
   * first created register output corresponds to the first created
   * register input, etc.
   *
   * \param f Signal that drives the created register input
   */
  uint32_t create_ri( signal const& f );

  /*! \brief Checks whether a node is a combinational input (PI or RO). */
  bool is_ci( node const& n ) const;

  /*! \brief Checks whether a node is a register output. */
  bool is_ro( node const& n ) const;

  /*! \brief Checks whether the network is combinational.
   *
   * Returns true if and only if the network has no registers (neither
   * register outputs nor register inputs).
   */
  bool is_combinational() const;

  /*! \brief Returns the number of combinational inputs (PIs and ROs). */
  auto num_cis() const;

  /*! \brief Returns the number of combinational outputs (POs and RIs). */
  auto num_cos() const;

  /*! \brief Returns the number of registers.
   *
   * This number is usually equal to the number of register outputs
   * and register inputs because they have to appear in pairs.  During
   * the construction of a network, the number of register outputs and
   * register inputs may diverge.
   */
  auto num_registers() const;

  /*! \brief Returns the combinational input node for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              combinational inputs (exclusive).
   */
  node ci_at( uint32_t index ) const;

  /*! \brief Returns the combinational output signal for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              combinational outputs (exclusive).
   */
  signal co_at( uint32_t index ) const;

  /*! \brief Returns the register output node for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              register outputs (exclusive).
   */
  node ro_at( uint32_t index ) const;

  /*! \brief Returns the register input signal for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              register inputs (exclusive).
   */
  signal ri_at( uint32_t index ) const;

  /*! \brief Returns the index of a combinational input node.
   *
   * \param n A combinational input node.
   * \return A value between 0 and num_cis()-1.
   */
  uint32_t ci_index( node const& n ) const;

  /*! \brief Returns the index of a combinational output signal.
   *
   * \param n A combinational output signal.
   * \return A value between 0 and num_cos()-1.
   */
  uint32_t co_index( signal const& s ) const;

  /*! \brief Returns the index of a register output node.
   *
   * \param n A register output node.
   * \return A value between 0 and num_cis()-num_pis()-1.
   */
  uint32_t ro_index( node const& n ) const;

  /*! \brief Returns the index of a register input signal.
   *
   * \param n A register input signal.
   * \return A value between 0 and num_cos()-num_pos()-1.
   */
  uint32_t ri_index( signal const& s ) const;

  /*! \brief Returns the register input signal to a register output node.
   *
   * \param signal A signal of a register output.
   */
  signal ro_to_ri( signal const& s ) const;

  /*! \brief Returns the register output node for a register input signal.
   *
   * \param signal A node of a register input.
   */
  node ri_to_ro( signal const& s ) const;

  /*! \brief Calls ``fn`` on every combinational input node in the network.
   *
   * The order is in the same order as combinational inputs have been
   * created with ``create_pi`` or ``create_ro``.  The parameter
   * ``fn`` is any callable that must have one of the following four
   * signatures.
   * - ``void(node const&)``
   * - ``void(node const&, uint32_t)``
   * - ``bool(node const&)``
   * - ``bool(node const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_ci( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every combinational output signal in the network.
   *
   * The order is in the same order as combinational outputs have been
   * created with ``create_po`` or ``create_ri``.  The function is
   * called on the signal that is driving the output and may occur
   * more than once in the iteration, if it drives more than one
   * output.  The parameter ``fn`` is any callable that must have one
   * of the following four
   * signatures.
   * - ``void(signal const&)``
   * - ``void(signal const&, uint32_t)``
   * - ``bool(signal const&)``
   * - ``bool(signal const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_co( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every register output node in the network.
   *
   * The order is in the same order as register outputs have been created with
   * ``create_ro``.  The parameter ``fn`` is any callable that must have one of
   * the following four signatures.
   * - ``void(node const&)``
   * - ``void(node const&, uint32_t)``
   * - ``bool(node const&)``
   * - ``bool(node const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_ro( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every register input signal in the network.
   *
   * The order is in the same order as register inputs have been created with
   * ``create_ri``.  The function is called on the signal that is driving the
   * output and may occur more than once in the iteration, if it drives more
   * than one output.  The parameter ``fn`` is any callable that must have one
   * of the following four signatures.
   * - ``void(signal const&)``
   * - ``void(signal const&, uint32_t)``
   * - ``bool(signal const&)``
   * - ``bool(signal const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_ri( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every pair of register input signal and
   *         register output node in the network.
   *
   * Calls each pair of a register input signal and the associated
   * register output node.  The parameter ``fn`` is any callable that
   * must have one of the following four signatures.
   * - ``void(std::pair<signal, node> const&)``
   * - ``void(std::pair<signal, node> const&, uint32_t)``
   * - ``bool(std::pair<signal, node> const&)``
   * - ``bool(std::pair<signal, node> const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_register( Fn&& fn ) const;

  /*! \brief Sets the register information for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              registers (exclusive).
   * \param reg Register information to set.
   */
  void set_register( uint32_t index, register_t reg );

  /*! \brief Returns the register information for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              registers (exclusive).
   * \return Register information (see ``register_t``).
   */
  register_t register_at( uint32_t index ) const;
};

template<typename Ntk>
class sequential<Ntk, true> : public Ntk
{
public:
  using base_type = typename Ntk::base_type;
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  struct sequential_information
  {
    uint32_t num_pis{ 0 };
    uint32_t num_pos{ 0 };
    std::vector<register_t> registers;
  };

  sequential()
      : _sequential_storage( std::make_shared<sequential_information>() )
  {
    static_assert( std::is_same_v<base_type, aig_network> || std::is_same_v<base_type, xag_network> ||
                       std::is_same_v<base_type, mig_network> || std::is_same_v<base_type, xmg_network> ||
                       std::is_same_v<base_type, aqfp_network>,
                   "Sequential interfaces extended for unknown network type. Please check the compatibility of implementations." );
  }

  sequential( storage base_storage )
      : Ntk( base_storage ), _sequential_storage( std::make_shared<sequential_information>() )
  {
    static_assert( std::is_same_v<base_type, aig_network> || std::is_same_v<base_type, xag_network> ||
                       std::is_same_v<base_type, mig_network> || std::is_same_v<base_type, xmg_network> ||
                       std::is_same_v<base_type, aqfp_network>,
                   "Sequential interfaces extended for unknown network type. Please check the compatibility of implementations." );
  }

  signal create_pi()
  {
    ++_sequential_storage->num_pis;
    return Ntk::create_pi();
  }

  uint32_t create_po( signal const& f )
  {
    ++_sequential_storage->num_pos;
    return Ntk::create_po( f );
  }

  signal create_ro()
  {
    _sequential_storage->registers.emplace_back();
    return Ntk::create_pi();
  }

  uint32_t create_ri( signal const& f )
  {
    return Ntk::create_po( f );
  }

  bool is_combinational() const
  {
    return ( static_cast<uint32_t>( this->_storage->inputs.size() ) == _sequential_storage->num_pis &&
             static_cast<uint32_t>( this->_storage->outputs.size() ) == _sequential_storage->num_pos );
  }

  bool is_ci( node const& n ) const
  {
    return Ntk::is_pi( n );
  }

  bool is_pi( node const& n ) const
  {
    return Ntk::is_pi( n ) && this->_storage->nodes[n].children[0].data < static_cast<uint64_t>( _sequential_storage->num_pis );
  }

  bool is_ro( node const& n ) const
  {
    return Ntk::is_pi( n ) && this->_storage->nodes[n].children[0].data >= static_cast<uint64_t>( _sequential_storage->num_pis );
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( this->_storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( this->_storage->outputs.size() );
  }

  auto num_pis() const
  {
    return _sequential_storage->num_pis;
  }

  auto num_pos() const
  {
    return _sequential_storage->num_pos;
  }

  auto num_registers() const
  {
    assert( static_cast<uint32_t>( this->_storage->inputs.size() - _sequential_storage->num_pis ) == static_cast<uint32_t>( this->_storage->outputs.size() - _sequential_storage->num_pos ) );
    return static_cast<uint32_t>( this->_storage->inputs.size() - _sequential_storage->num_pis );
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _sequential_storage->num_pis );
    return *( this->_storage->inputs.begin() + index );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _sequential_storage->num_pos );
    return *( this->_storage->outputs.begin() + index );
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < this->_storage->inputs.size() );
    return *( this->_storage->inputs.begin() + index );
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < this->_storage->outputs.size() );
    return *( this->_storage->outputs.begin() + index );
  }

  node ro_at( uint32_t index ) const
  {
    assert( index < this->_storage->inputs.size() - _sequential_storage->num_pis );
    return *( this->_storage->inputs.begin() + _sequential_storage->num_pis + index );
  }

  signal ri_at( uint32_t index ) const
  {
    assert( index < this->_storage->outputs.size() - _sequential_storage->num_pos );
    return *( this->_storage->outputs.begin() + _sequential_storage->num_pos + index );
  }

  void set_register( uint32_t index, register_t reg )
  {
    assert( index < _sequential_storage->registers.size() );
    _sequential_storage->registers[index] = reg;
  }

  register_t register_at( uint32_t index ) const
  {
    assert( index < _sequential_storage->registers.size() );
    return _sequential_storage->registers[index];
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( this->_storage->nodes[n].children[0].data < _sequential_storage->num_pis );
    return Ntk::pi_index( n );
  }

  uint32_t ci_index( node const& n ) const
  {
    return Ntk::pi_index( n );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index ) {
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true; } );
    return i;
  }

  uint32_t ro_index( node const& n ) const
  {
    assert( this->_storage->nodes[n].children[0].data >= _sequential_storage->num_pis );
    return Ntk::pi_index( n ) - _sequential_storage->num_pis;
  }

  uint32_t ri_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_ri( [&]( const auto& x, auto index ) {
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true; } );
    return i;
  }

  signal ro_to_ri( signal const& s ) const
  {
    return *( this->_storage->outputs.begin() + _sequential_storage->num_pos + this->_storage->nodes[s.index].children[0].data - _sequential_storage->num_pis );
  }

  node ri_to_ro( signal const& s ) const
  {
    return *( this->_storage->inputs.begin() + _sequential_storage->num_pis + ri_index( s ) );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->inputs.begin(), this->_storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->outputs.begin(), this->_storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->inputs.begin(), this->_storage->inputs.begin() + _sequential_storage->num_pis, fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->outputs.begin(), this->_storage->outputs.begin() + _sequential_storage->num_pos, fn );
  }

  template<typename Fn>
  void foreach_ro( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->inputs.begin() + _sequential_storage->num_pis, this->_storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_ri( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->outputs.begin() + _sequential_storage->num_pos, this->_storage->outputs.end(), fn );
  }

  template<typename Fn>
  void foreach_register( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, void> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal, node>, void> ||
                   detail::is_callable_with_index_v<Fn, std::pair<signal, node>, bool> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal, node>, bool> );

    assert( this->_storage->inputs.size() - _sequential_storage->num_pis == this->_storage->outputs.size() - _sequential_storage->num_pos );
    auto ro = this->_storage->inputs.begin() + _sequential_storage->num_pis;
    auto ri = this->_storage->outputs.begin() + _sequential_storage->num_pos;
    if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal, node>, bool> )
    {
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        if ( !fn( std::make_pair( ri++, ro++ ) ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, bool> )
    {
      uint32_t index{ 0 };
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        if ( !fn( std::make_pair( ri++, ro++ ), index++ ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal, node>, void> )
    {
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        fn( std::make_pair( *ri++, *ro++ ) );
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, void> )
    {
      uint32_t index{ 0 };
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        fn( std::make_pair( *ri++, *ro++ ), index++ );
      }
    }
  }

public:
  std::shared_ptr<sequential_information> _sequential_storage;
}; // class sequential<Ntk, true>

template<typename Ntk>
class sequential<Ntk, false> : public Ntk
{
public:
  using base_type = typename Ntk::base_type;
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  struct sequential_information
  {
    uint32_t num_pis{ 0 };
    uint32_t num_pos{ 0 };
    std::vector<register_t> registers;
  };

  sequential()
      : _sequential_storage( std::make_shared<sequential_information>() )
  {
    static_assert( std::is_same_v<base_type, klut_network> || std::is_same_v<base_type, cover_network>,
                   "Sequential interfaces extended for unknown network type. Please check the compatibility of implementations." );
  }

  sequential( storage base_storage )
      : Ntk( base_storage ), _sequential_storage( std::make_shared<sequential_information>() )
  {
    static_assert( std::is_same_v<base_type, klut_network> || std::is_same_v<base_type, cover_network>,
                   "Sequential interfaces extended for unknown network type. Please check the compatibility of implementations." );
  }

  signal create_pi()
  {
    ++_sequential_storage->num_pis;
    return Ntk::create_pi();
  }

  uint32_t create_po( signal const& f )
  {
    ++_sequential_storage->num_pos;
    return Ntk::create_po( f );
  }

  signal create_ro()
  {
    _sequential_storage->registers.emplace_back();
    return Ntk::create_pi();
  }

  uint32_t create_ri( signal const& f )
  {
    return Ntk::create_po( f );
  }

  bool is_combinational() const
  {
    return ( static_cast<uint32_t>( this->_storage->inputs.size() ) == _sequential_storage->num_pis &&
             static_cast<uint32_t>( this->_storage->outputs.size() ) == _sequential_storage->num_pos );
  }

  bool is_ci( node const& n ) const
  {
    return std::find( this->_storage->inputs.begin(), this->_storage->inputs.end(), n ) != this->_storage->inputs.end();
  }

  bool is_pi( node const& n ) const
  {
    const auto end = this->_storage->inputs.begin() + _sequential_storage->num_pis;
    return std::find( this->_storage->inputs.begin(), end, n ) != end;
  }

  bool is_ro( node const& n ) const
  {
    return std::find( this->_storage->inputs.begin() + _sequential_storage->num_pis, this->_storage->inputs.end(), n ) != this->_storage->inputs.end();
  }

  auto num_cis() const
  {
    return static_cast<uint32_t>( this->_storage->inputs.size() );
  }

  auto num_cos() const
  {
    return static_cast<uint32_t>( this->_storage->outputs.size() );
  }

  auto num_pis() const
  {
    return _sequential_storage->num_pis;
  }

  auto num_pos() const
  {
    return _sequential_storage->num_pos;
  }

  auto num_registers() const
  {
    assert( static_cast<uint32_t>( this->_storage->inputs.size() - _sequential_storage->num_pis ) == static_cast<uint32_t>( this->_storage->outputs.size() - _sequential_storage->num_pos ) );
    return static_cast<uint32_t>( this->_storage->inputs.size() - _sequential_storage->num_pis );
  }

  node pi_at( uint32_t index ) const
  {
    assert( index < _sequential_storage->num_pis );
    return *( this->_storage->inputs.begin() + index );
  }

  signal po_at( uint32_t index ) const
  {
    assert( index < _sequential_storage->num_pos );
    return ( this->_storage->outputs.begin() + index )->index;
  }

  node ci_at( uint32_t index ) const
  {
    assert( index < this->_storage->inputs.size() );
    return *( this->_storage->inputs.begin() + index );
  }

  signal co_at( uint32_t index ) const
  {
    assert( index < this->_storage->outputs.size() );
    return ( this->_storage->outputs.begin() + index )->index;
  }

  node ro_at( uint32_t index ) const
  {
    assert( index < this->_storage->inputs.size() - _sequential_storage->num_pis );
    return *( this->_storage->inputs.begin() + _sequential_storage->num_pis + index );
  }

  signal ri_at( uint32_t index ) const
  {
    assert( index < this->_storage->outputs.size() - _sequential_storage->num_pos );
    return ( this->_storage->outputs.begin() + _sequential_storage->num_pos + index )->index;
  }

  void set_register( uint32_t index, register_t reg )
  {
    assert( index < _sequential_storage->registers.size() );
    _sequential_storage->registers[index] = reg;
  }

  register_t register_at( uint32_t index ) const
  {
    assert( index < _sequential_storage->registers.size() );
    return _sequential_storage->registers[index];
  }

  uint32_t pi_index( node const& n ) const
  {
    assert( this->_storage->nodes[n].children[0].data < _sequential_storage->num_pis );
    return Ntk::pi_index( n );
  }

  uint32_t ci_index( node const& n ) const
  {
    return Ntk::pi_index( n );
  }

  uint32_t co_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_co( [&]( const auto& x, auto index ) {
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true; } );
    return i;
  }

  uint32_t ro_index( node const& n ) const
  {
    assert( this->_storage->nodes[n].children[0].data >= _sequential_storage->num_pis );
    return Ntk::pi_index( n ) - _sequential_storage->num_pis;
  }

  uint32_t ri_index( signal const& s ) const
  {
    uint32_t i = -1;
    foreach_ri( [&]( const auto& x, auto index ) {
        if ( x == s )
        {
          i = index;
          return false;
        }
        return true; } );
    return i;
  }

  signal ro_to_ri( signal const& s ) const
  {
    return *( this->_storage->outputs.begin() + _sequential_storage->num_pos + this->_storage->nodes[s.index].children[0].data - _sequential_storage->num_pis );
  }

  node ri_to_ro( signal const& s ) const
  {
    return *( this->_storage->inputs.begin() + _sequential_storage->num_pis + ri_index( s ) );
  }

  template<typename Fn>
  void foreach_ci( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->inputs.begin(), this->_storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_co( Fn&& fn ) const
  {
    using IteratorType = decltype( this->_storage->outputs.begin() );
    detail::foreach_element_transform<IteratorType, uint32_t>(
        this->_storage->outputs.begin(), this->_storage->outputs.end(), []( auto o ) { return o.index; }, fn );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->inputs.begin(), this->_storage->inputs.begin() + _sequential_storage->num_pis, fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    using IteratorType = decltype( this->_storage->outputs.begin() );
    detail::foreach_element_transform<IteratorType, uint32_t>(
        this->_storage->outputs.begin(), this->_storage->outputs.begin() + _sequential_storage->num_pos, []( auto o ) { return o.index; }, fn );
  }

  template<typename Fn>
  void foreach_ro( Fn&& fn ) const
  {
    detail::foreach_element( this->_storage->inputs.begin() + _sequential_storage->num_pis, this->_storage->inputs.end(), fn );
  }

  template<typename Fn>
  void foreach_ri( Fn&& fn ) const
  {
    using IteratorType = decltype( this->_storage->outputs.begin() );
    detail::foreach_element_transform<IteratorType, uint32_t>(
        this->_storage->outputs.begin() + _sequential_storage->num_pos, this->_storage->outputs.end(), []( auto o ) { return o.index; }, fn );
  }

  template<typename Fn>
  void foreach_register( Fn&& fn ) const
  {
    static_assert( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, void> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal, node>, void> ||
                   detail::is_callable_with_index_v<Fn, std::pair<signal, node>, bool> ||
                   detail::is_callable_without_index_v<Fn, std::pair<signal, node>, bool> );

    assert( this->_storage->inputs.size() - _sequential_storage->num_pis == this->_storage->outputs.size() - _sequential_storage->num_pos );
    auto ro = this->_storage->inputs.begin() + _sequential_storage->num_pis;
    auto ri = this->_storage->outputs.begin() + _sequential_storage->num_pos;
    if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal, node>, bool> )
    {
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        if ( !fn( std::make_pair( ri++, ro++ ) ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, bool> )
    {
      uint32_t index{ 0 };
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        if ( !fn( std::make_pair( ri++, ro++ ), index++ ) )
          return;
      }
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, std::pair<signal, node>, void> )
    {
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        fn( std::make_pair( *ri++, *ro++ ) );
      }
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, std::pair<signal, node>, void> )
    {
      uint32_t index{ 0 };
      while ( ro != this->_storage->inputs.end() && ri != this->_storage->outputs.end() )
      {
        fn( std::make_pair( *ri++, *ro++ ), index++ );
      }
    }
  }

public:
  std::shared_ptr<sequential_information> _sequential_storage;
}; // class sequential<Ntk, false>

} // namespace mockturtle
