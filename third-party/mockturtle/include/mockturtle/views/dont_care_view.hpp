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
  \file dont_care_view.hpp
  \brief Implements methods to store external don't-cares

  \author Siang-Yun Lee
*/

#pragma once

#include "../traits.hpp"
#include "../algorithms/simulation.hpp"
#include "../algorithms/cnf.hpp"
#include "../utils/node_map.hpp"
#include "../utils/window_utils.hpp"
#include "../views/color_view.hpp"
#include "../views/window_view.hpp"

#include <bill/sat/interface/common.hpp>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/cube.hpp>

#include <vector>
#include <set>
#include <utility>

namespace mockturtle
{

/*! \brief A view holding external don't care information of a network
 *
 * This view helps storing and managing external don't care information.
 * There are two types of external don't cares that may be given together
 * or independently:
 *
 * External controllability don't cares (EXCDCs) are primary input patterns
 * that will never happen or can be ignored. They are given as another
 * network `cdc_ntk` having the same number of PIs as the main network and
 * one PO. An input assignment making `cdc_ntk` output 1 is an EXCDC.
 *
 * External observability don't cares (EXODCs) are conditions for one or
 * more primary outputs under which their values can be altered. EXODCs may
 * be given for a PO as value combinations of other POs (for example, “the
 * second PO is don't care whenever the first PO is 1”). They may also be
 * given as pairs of observably equivalent PO values (for example, “the output
 * values 01 and 10 are considered equivalent and interchangable”).
 *
 * By wrapping a network with this view and giving some external don't care
 * conditions, some algorithms supporting the consideration of external don't
 * cares can make use of them. Currently, only `sim_resubstitution`
 * (which makes use of `circuit_validator`) and `equivalence_checking_bill`
 * support external don't cares.
 *
 * \tparam hasEXCDC Enables interfaces and data structure holding external
 * controllability don't cares (external don't cares at the primary inputs)
 * \tparam hasEXODC Enables interfaces and data structure holding external
 * observability don't cares (external dont cares at the primary outputs)
 */
template<class Ntk, bool hasEXCDC = true, bool hasEXODC = true>
class dont_care_view;

namespace detail
{

/*! \brief A manager to classify bit-strings into equivalence classes
 *
 * This data structure holds and manages equivalence classes of
 * bit-strings of the same length (i.e. complete or partial binary
 * truth tables).
 * The three properties of an equivalence relation are maintained:
 * - Reflexive (x = x)
 * - Symmetric (if x = y then y = x)
 * - Transitive (if x = y and y = z then x = z)
 *
 * In the current implementation, the big-string length may not be
 * larger than 31.
 */
class equivalence_classes_mgr
{
public:
  equivalence_classes_mgr() {}

  equivalence_classes_mgr( uint32_t num_bits ) : _num_bits( num_bits )
  {
    assert( num_bits < 32 );
    uint32_t max_val = 1u << num_bits;
    _classes.resize( max_val );
    for ( uint32_t i = 0u; i < max_val; ++i )
    {
      _classes[i] = i;
    }
  }

  equivalence_classes_mgr& operator=( equivalence_classes_mgr const& other )
  {
    _num_bits = other._num_bits;
    _classes = other._classes;
    return *this;
  }

  void set_equivalent( uint32_t const& a, uint32_t const& b )
  {
    uint32_t repr_class = _classes.at( a );
    uint32_t to_be_replaced = _classes.at( b );
    for ( auto i = 0u; i < _classes.size(); ++i )
    {
      if ( _classes[i] == to_be_replaced )
        _classes[i] = repr_class;
    }
  }

  /*! \brief Set two bit strings to be equivalent. */
  void set_equivalent( std::vector<bool> const& a, std::vector<bool> const& b )
  {
    set_equivalent( vector_bool_to_uint32( a ), vector_bool_to_uint32( b ) );
  }

  /*! \brief Check equivalence of fully-assigned bit-strings */
  bool are_equivalent( uint32_t const& a, uint32_t const& b ) const
  {
    return _classes.at( a ) == _classes.at( b );
  }

  /*! \brief Check equivalence of fully-assigned bit-strings */
  bool are_equivalent( std::vector<bool> const& a, std::vector<bool> const& b ) const
  {
    return are_equivalent( vector_bool_to_uint32( a ), vector_bool_to_uint32( b ) );
  }

  /*! \brief Check equivalence of partially-assigned bit-strings
   *
   * The don't-care bit positions in the two cubes should be the same.
   * Two cubes are equivalent if for all possible assignments to the
   * don't-care bits, they are always equivalent.
   */
  bool are_equivalent( kitty::cube const& a, kitty::cube const& b ) const
  {
    assert( a._mask == b._mask && "The don't-care bit positions in the two cubes should be the same." );
    return are_equivalent_rec( a, b, 0 );
  }

  uint32_t num_classes() const
  {
    std::set<uint32_t> unique_ids;
    for ( auto const& id : _classes )
    {
      unique_ids.insert( id );
    }
    return unique_ids.size();
  }

  template<class Fn>
  void foreach_class( Fn&& fn ) const
  {
    std::unordered_map<uint32_t, std::vector<uint32_t>> class2pats;
    for ( auto pat = 0u; pat < _classes.size(); ++pat )
    {
      auto const& id = _classes[pat];
      class2pats.try_emplace( id );
      class2pats[id].emplace_back( pat );
    }
    
    for ( auto const& p : class2pats )
    {
      if ( !fn( p.second ) )
        break;
    }
  }

private:
  bool are_equivalent_rec( kitty::cube const& a, kitty::cube const& b, uint32_t i ) const
  {
    if ( i == _num_bits )
    {
      return are_equivalent( cube_to_uint32( a ), cube_to_uint32( b ) );
    }

    if ( a.get_mask( i ) )
    {
      return are_equivalent_rec( a, b, i + 1 );
    }
    else
    {
      kitty::cube a0 = a;
      a0.set_mask( i );
      kitty::cube b0 = b;
      b0.set_mask( i );
      if ( !are_equivalent_rec( a0, b0, i + 1 ) )
        return false;
      a0.set_bit( i );
      b0.set_bit( i );
      return are_equivalent_rec( a0, b0, i + 1 );
    }
  }

  uint32_t vector_bool_to_uint32( std::vector<bool> const& vec ) const
  {
    assert( vec.size() == _num_bits );
    uint32_t res{0u};
    for ( auto i = 0u; i < _num_bits; ++i )
    {
      if ( vec[i] )
        res |= 1u << i;
    }
    return res;
  }

  uint32_t cube_to_uint32( kitty::cube const& c ) const
  {
    assert( c.num_literals() == _num_bits ); // fully assigned
    return c._bits;
  }

private:
  uint32_t _num_bits;
  std::vector<uint32_t> _classes;
}; // equivalence_classes_mgr

template<class Ntk, bool hasEXCDC>
class dont_care_view_impl : public Ntk
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  template<bool enabled = !hasEXCDC, typename = std::enable_if_t<enabled>>
  dont_care_view_impl( Ntk const& ntk )
    : Ntk( ntk )
  {}

  template<bool enabled = hasEXCDC, typename = std::enable_if_t<enabled>>
  dont_care_view_impl( Ntk const& ntk, Ntk const& cdc_ntk )
    : Ntk( ntk ), _excdc( cdc_ntk )
  {
    assert( cdc_ntk.num_pis() == ntk.num_pis() );
    assert( cdc_ntk.num_pos() == 1 );
  }

  /*! \brief Checks whether an input pattern is EXCDC
   *
   * \param pattern The PI value combination to be checked
   * \return Whether `pattern` is EXCDC
   */
  template<bool enabled = hasEXCDC, typename = std::enable_if_t<enabled>>
  bool pattern_is_EXCDC( std::vector<bool> const& pattern ) const
  {
    assert( pattern.size() == this->num_pis() );

    default_simulator<bool> sim( pattern );
    auto const vals = simulate<bool>( _excdc, sim );
    return vals[0];
  }

  template<typename solver_t, bool enabled = hasEXCDC, typename = std::enable_if_t<enabled>>
  void add_EXCDC_clauses( solver_t& solver ) const
  {
    using add_clause_fn_t = std::function<void( std::vector<bill::lit_type> const& )>;
    add_clause_fn_t const add_clause_fn = [&]( auto const& clause ) { solver.add_clause( clause ); };

    // topological order of the gates in _excdc is assumed
    node_map<bill::lit_type, Ntk> cdc_lits( _excdc );
    cdc_lits[_excdc.get_constant( false )] = bill::lit_type( 0, bill::lit_type::polarities::positive );
    if ( _excdc.get_node( _excdc.get_constant( false ) ) != _excdc.get_node( _excdc.get_constant( true ) ) )
    {
      cdc_lits[_excdc.get_constant( true )] = bill::lit_type( 0, bill::lit_type::polarities::negative );
    }
    _excdc.foreach_pi( [&]( auto const& n, auto i ) {
      cdc_lits[n] = bill::lit_type( i + 1, bill::lit_type::polarities::positive );
    } );

    _excdc.foreach_gate( [&]( auto const& n ){
      cdc_lits[n] = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
    });

    auto out_lits = generate_cnf<Ntk, bill::lit_type>( _excdc, add_clause_fn, cdc_lits );
    solver.add_clause( {~out_lits[0]} );
  }

private:
  Ntk _excdc;
}; /* dont_care_view_impl */
} // namespace detail

template<class Ntk, bool hasEXCDC>
class dont_care_view<Ntk, hasEXCDC, false> : public detail::dont_care_view_impl<Ntk, hasEXCDC>
{
public:
  static constexpr bool has_EXCDC_interface = hasEXCDC;
  static constexpr bool has_EXODC_interface = false;

public:
  template<bool enabled = !hasEXCDC, typename = std::enable_if_t<enabled>>
  dont_care_view( Ntk const& ntk )
    : detail::dont_care_view_impl<Ntk, hasEXCDC>( ntk )
  {}

  template<bool enabled = hasEXCDC, typename = std::enable_if_t<enabled>>
  dont_care_view( Ntk const& ntk, Ntk const& cdc_ntk )
    : detail::dont_care_view_impl<Ntk, hasEXCDC>( ntk, cdc_ntk )
  {}
};

template<class Ntk, bool hasEXCDC>
class dont_care_view<Ntk, hasEXCDC, true> : public detail::dont_care_view_impl<Ntk, hasEXCDC>
{
public:
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  static constexpr bool has_EXCDC_interface = hasEXCDC;
  static constexpr bool has_EXODC_interface = true;

public:
  /*! \brief Constructor when no EXCDC is provided
   *
   * \param ntk The main network
   */
  template<bool enabled = !hasEXCDC, typename = std::enable_if_t<enabled>>
  dont_care_view( Ntk const& ntk )
    : detail::dont_care_view_impl<Ntk, hasEXCDC>( ntk ), _exoec( ntk.num_pos() )
  {}

  /*! \brief Constructor when EXCDC is provided
   *
   * \param ntk The main network
   * \param cdc_ntk The network representing EXCDC conditions, having the same
   * number of PIs as `ntk` and one PO
   */
  template<bool enabled = hasEXCDC, typename = std::enable_if_t<enabled>>
  dont_care_view( Ntk const& ntk, Ntk const& cdc_ntk )
    : detail::dont_care_view_impl<Ntk, hasEXCDC>( ntk, cdc_ntk ), _exoec( ntk.num_pos() )
  {}

  /*! \brief Adds an EXODC condition for a PO in terms of other POs
   *
   * \param cond Condition in terms of other POs for the concerned PO to be don't care
   * \param po_id Index of the concerned PO
   */
  void add_EXODC( kitty::cube const& cond, uint32_t po_id )
  {
    cond.foreach_minterm( this->num_pos(), [&]( kitty::cube const& c ){
      assert( c.num_literals() == this->num_pos() );
      if ( c.get_bit( po_id ) ) return true;
      kitty::cube c2 = c;
      c2.set_bit( po_id );
      _exoec.set_equivalent( c._bits, c2._bits );
      return true;
    });
  }

  /*! \brief Adds a pair of PO values that are considered observably equivalent
   *
   * \param pat1 The first PO value combination
   * \param pat2 The second PO value combination
   */
  void add_EXOEC_pair( std::vector<bool> const& pat1, std::vector<bool> const& pat2 )
  {
    _exoec.set_equivalent( pat1, pat2 );
  }

  /*! \brief Checks whether a pair of PO value assignments are observably equivalent
   *
   * \param pat1 The first PO value combination
   * \param pat2 The second PO value combination
   * \return Whether `pat1` and `pat2` are observably equivalent
   */
  bool are_observably_equivalent( std::vector<bool> const& pat1, std::vector<bool> const& pat2 ) const
  {
    return _exoec.are_equivalent( pat1, pat2 );
  }

  /*! \brief Checks whether a pair of partial PO value assignments are observably equivalent
   *
   * For a pair of partial assignments to be equivalent, all pairs of expansions of
   * the partial assignments have to be equivalent.
   *
   * \param pat1 The first partial PO value
   * \param pat2 The second partial PO value
   * \return Whether `pat1` and `pat2` are observably equivalent
   */
  bool are_observably_equivalent( kitty::cube const& pat1, kitty::cube const& pat2 ) const
  {
    return _exoec.are_equivalent( pat1, pat2 );
  }

  /*! \brief Builds an observability-equivalence miter network
   *
   * An observability-equivalence miter network is a generalization of a miter network.
   * This network takes two PO value combinations as inputs (thus it has
   * `2 * ntk.num_pos()` PIs) and outputs 1 if the two PO value combinations are
   * _not_ observably equivalent.
   *
   * \param miter An empty network where the miter will be built
   */
  void build_oe_miter( Ntk& miter ) const
  {
    std::vector<signal> pos1, pos2;
    for ( auto i = 0u; i < this->num_pos(); ++i )
    {
      pos1.emplace_back( miter.create_pi() );
    }
    for ( auto i = 0u; i < this->num_pos(); ++i )
    {
      pos2.emplace_back( miter.create_pi() );
    }
    build_oe_miter( miter, pos1, pos2 );
  }

  /*! \brief Builds an observability-equivalence miter network
   *
   * An observability-equivalence miter network is a generalization of a miter network.
   * This network takes two PO value combinations as inputs (thus it has
   * `2 * ntk.num_pos()` PIs) and outputs 1 if the two PO value combinations are
   * _not_ observably equivalent.
   *
   * \param miter The network where the miter will be built
   * \param pos1 Signals of the first set of (main network's) POs
   * \param pos2 Signals of the second set of (main network's) POs
   */
  template<class NtkMiter>
  void build_oe_miter( NtkMiter& miter, std::vector<typename NtkMiter::signal> const& pos1, std::vector<typename NtkMiter::signal> const& pos2 ) const
  {
    assert( pos1.size() == this->num_pos() );
    assert( pos2.size() == this->num_pos() );

    std::vector<typename NtkMiter::signal> are_both_in_class_i;
    std::vector<typename NtkMiter::signal> is_in_class1, is_in_class2;
    std::vector<typename NtkMiter::signal> ins1, ins2;
    ins1.resize( this->num_pos() );
    ins2.resize( this->num_pos() );
    _exoec.foreach_class( [&]( std::vector<uint32_t> const& pats ){
      is_in_class1.clear();
      is_in_class2.clear();
      for ( uint32_t pat : pats )
      {
        for ( auto i = 0u; i < this->num_pos(); ++i )
        {
          ins1[i] = ( pat & 0x1 ) ? pos1[i] : !pos1[i];
          ins2[i] = ( pat & 0x1 ) ? pos2[i] : !pos2[i];
          pat >>= 1;
        }
        is_in_class1.emplace_back( miter.create_nary_and( ins1 ) );
        is_in_class2.emplace_back( miter.create_nary_and( ins2 ) );
      }
      are_both_in_class_i.emplace_back( miter.create_and( miter.create_nary_or( is_in_class1 ), miter.create_nary_or( is_in_class2 ) ) );
      return true;
    });
    miter.create_po( !miter.create_nary_or( are_both_in_class_i ) );
    /* miter output = 1 <=> there is no class i that pos1 and pos2 are both in <=> pos1 and pos2 are not OE */
  }

private:
  detail::equivalence_classes_mgr _exoec;
}; /* dont_care_view */

} // namespace mockturtle
