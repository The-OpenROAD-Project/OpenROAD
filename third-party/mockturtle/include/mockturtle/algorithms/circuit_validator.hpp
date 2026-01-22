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
  \file circuit_validator.hpp
  \brief Validate potential circuit optimization choices with SAT.

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/events.hpp"
#include "../utils/index_list/index_list.hpp"
#include "../utils/node_map.hpp"
#include "cnf.hpp"

#include <bill/sat/interface/abc_bsat2.hpp>
#include <bill/sat/interface/common.hpp>
#include <bill/sat/interface/glucose.hpp>
#include <bill/sat/interface/z3.hpp>

namespace mockturtle
{

struct validator_params
{
  /*! \brief Maximum number of clauses of the SAT solver. (incremental CNF construction) */
  uint32_t max_clauses{ 1000 };

  /*! \brief Whether to consider ODC, and how many levels. 0 = No consideration. -1 = Consider TFO until PO. */
  int32_t odc_levels{ 0 };

  /*! \brief Conflict limit of the SAT solver. */
  uint32_t conflict_limit{ 1000 };

  /*! \brief Seed for randomized solving. */
  uint32_t random_seed{ 0 };
};

template<class Ntk, bill::solvers Solver = bill::solvers::glucose_41, bool use_pushpop = false, bool randomize = false, bool use_odc = false>
class circuit_validator
{
public:
  static constexpr bool use_odc_ = use_odc;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;
  using add_clause_fn_t = std::function<void( std::vector<bill::lit_type> const& )>;

  enum gate_type
  {
    AND,
    XOR,
    MAJ,
    MUX
  };

  explicit circuit_validator( Ntk const& ntk, validator_params const& ps = {} )
      : ntk( ntk ), ps( ps ), literals( ntk ), constructed( ntk ), num_invoke( 0u ), cex( ntk.num_pis() )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
    static_assert( has_is_and_v<Ntk>, "Ntk does not implement the is_and method" );
    static_assert( has_is_xor_v<Ntk>, "Ntk does not implement the is_xor method" );
    static_assert( has_is_xor3_v<Ntk>, "Ntk does not implement the is_xor3 method" );
    static_assert( has_is_maj_v<Ntk>, "Ntk does not implement the is_maj method" );

    if constexpr ( use_pushpop )
    {
#if defined( BILL_HAS_Z3 )
      static_assert( Solver == bill::solvers::z3 || Solver == bill::solvers::bsat2, "Solver does not support push/pop" );
#else
      static_assert( Solver == bill::solvers::bsat2, "Solver does not support push/pop" );
#endif
    }
    if constexpr ( randomize )
    {
#if defined( BILL_HAS_Z3 )
      static_assert( Solver == bill::solvers::z3 || Solver == bill::solvers::bsat2, "Solver does not support set_random" );
#else
      static_assert( Solver == bill::solvers::bsat2, "Solver does not support set_random" );
#endif
    }
    if constexpr ( use_odc )
    {
      static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
      static_assert( has_visited_v<Ntk>, "Ntk does not implement the visited method" );
      static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
      static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );
    }

    add_event = ntk.events().register_add_event( [&]( node const& n ) {
      (void)n;
      literals.resize();
    } );

    /* constants are mapped to var 0 */
    literals[ntk.get_constant( false )] = bill::lit_type( 0, bill::lit_type::polarities::positive );
    if ( ntk.get_node( ntk.get_constant( false ) ) != ntk.get_node( ntk.get_constant( true ) ) )
    {
      literals[ntk.get_constant( true )] = bill::lit_type( 0, bill::lit_type::polarities::negative );
    }

    /* first indexes (starting from 1) are for PIs */
    ntk.foreach_pi( [&]( auto const& n, auto i ) {
      literals[n] = bill::lit_type( i + 1, bill::lit_type::polarities::positive );
    } );

    restart();
  }

  ~circuit_validator()
  {
    ntk.events().release_add_event( add_event );
  }

  /*! \brief Set ODC levels */
  void set_odc_levels( uint32_t odc_levels )
  {
    ps.odc_levels = odc_levels;
  }

  /*! \brief Validate functional equivalence of signals `f` and `d`. */
  std::optional<bool> validate( signal const& f, signal const& d )
  {
    if ( !constructed.has( d ) && !ntk.is_pi( ntk.get_node( d ) ) && !ntk.is_constant( ntk.get_node( d ) ) )
    {
      construct( ntk.get_node( d ) );
    }
    auto const res = validate( ntk.get_node( f ), lit_not_cond( literals[d], ntk.is_complemented( f ) ^ ntk.is_complemented( d ) ) );
    if ( solver.num_clauses() > ps.max_clauses && num_invoke >= MIN_NUM_INVOKE )
    {
      restart();
    }
    return res;
  }

  /*! \brief Validate functional equivalence of node `root` and signal `d`. */
  std::optional<bool> validate( node const& root, signal const& d )
  {
    if ( !constructed.has( d ) && !ntk.is_pi( ntk.get_node( d ) ) && !ntk.is_constant( ntk.get_node( d ) ) )
    {
      construct( ntk.get_node( d ) );
    }
    auto const res = validate( root, lit_not_cond( literals[d], ntk.is_complemented( d ) ) );
    if ( solver.num_clauses() > ps.max_clauses && num_invoke >= MIN_NUM_INVOKE )
    {
      restart();
    }
    return res;
  }

  /*! \brief Validate functional equivalence of signal `f` with a circuit represented by an index_list.
   *
   * \param id_list The index_list representing a circuit.
   * \param divs Existing nodes in the network, serving as PIs of `id_list`.
   * \param inverted Whether to validate equivalence or inverse equivalence.
   */
  template<class index_list_type>
  std::optional<bool> validate( signal const& f, std::vector<node> const& divs, index_list_type const& id_list, bool inverted = false )
  {
    return validate( ntk.get_node( f ), divs.begin(), divs.end(), id_list, inverted ^ ntk.is_complemented( f ) );
  }

  /*! \brief Validate functional equivalence of node `root` with an index_list. */
  template<class index_list_type>
  std::optional<bool> validate( node const& root, std::vector<node> const& divs, index_list_type const& id_list, bool inverted = false )
  {
    return validate( root, divs.begin(), divs.end(), id_list, inverted );
  }

  /*! \brief Validate functional equivalence of signal `f` with an index_list. */
  template<class iterator_type, class index_list_type>
  std::optional<bool> validate( signal const& f, iterator_type divs_begin, iterator_type divs_end, index_list_type const& id_list, bool inverted = false )
  {
    return validate( ntk.get_node( f ), divs_begin, divs_end, id_list, inverted ^ ntk.is_complemented( f ) );
  }

  /*! \brief Validate functional equivalence of node `root` with an index_list. */
  template<class iterator_type, class index_list_type>
  std::optional<bool> validate( node const& root, iterator_type divs_begin, iterator_type divs_end, index_list_type const& id_list, bool inverted = false )
  {
    static_assert( std::is_same_v<index_list_type, mig_index_list> ||
                   std::is_same_v<index_list_type, xag_index_list<true>> ||
                   std::is_same_v<index_list_type, xag_index_list<false>> ||
                   std::is_same_v<index_list_type, muxig_index_list>, "Unknown type of index list" );
    assert( uint64_t( std::distance( divs_begin, divs_end ) ) == id_list.num_pis() && "Size of the provided divisor list does not match number of PIs of the index list" );
    assert( id_list.num_pos() == 1u && "Index list must have exactly one PO" );

    if ( !constructed.has( root ) && !ntk.is_pi( root ) && !ntk.is_constant( root ) )
    {
      construct( root );
    }

    std::vector<bill::lit_type> lits;
    lits.reserve( id_list.num_pis() + id_list.num_gates() + 1 );
    lits.emplace_back( literals[ntk.get_constant( false )] );
    for ( auto it = divs_begin; it != divs_end; ++it )
    {
      if ( !constructed.has( *it ) && !ntk.is_pi( *it ) && !ntk.is_constant( *it ) )
      {
        construct( *it );
      }
      lits.emplace_back( literals[*it] );
    }

    if constexpr ( use_pushpop )
    {
      push();
    }

    if constexpr ( std::is_same_v<index_list_type, xag_index_list<true>> || std::is_same_v<index_list_type, xag_index_list<false>> )
    {
      id_list.foreach_gate( [&]( uint32_t id_lit0, uint32_t id_lit1 ) {
        uint32_t const node_pos0 = id_lit0 >> 1;
        uint32_t const node_pos1 = id_lit1 >> 1;
        assert( node_pos0 < lits.size() );
        assert( node_pos1 < lits.size() );
        lits.emplace_back( add_clauses_for_2input_gate( lit_not_cond( lits[node_pos0], id_lit0 & 0x1 ), lit_not_cond( lits[node_pos1], id_lit1 & 0x1 ), std::nullopt, id_lit0 < id_lit1 ? AND : XOR ) );
      } );
    }
    if constexpr ( std::is_same_v<index_list_type, mig_index_list> )
    {
      id_list.foreach_gate( [&]( uint32_t id_lit0, uint32_t id_lit1, uint32_t id_lit2 ) {
        uint32_t const node_pos0 = id_lit0 >> 1;
        uint32_t const node_pos1 = id_lit1 >> 1;
        uint32_t const node_pos2 = id_lit2 >> 1;
        assert( node_pos0 < lits.size() );
        assert( node_pos1 < lits.size() );
        assert( node_pos2 < lits.size() );
        lits.emplace_back( add_clauses_for_3input_gate( lit_not_cond( lits[node_pos0], id_lit0 & 0x1 ), lit_not_cond( lits[node_pos1], id_lit1 & 0x1 ), lit_not_cond( lits[node_pos2], id_lit2 & 0x1 ), std::nullopt, MAJ ) );
      } );
    }
    if constexpr ( std::is_same_v<index_list_type, muxig_index_list> )
    {
      id_list.foreach_gate( [&]( uint32_t id_lit0, uint32_t id_lit1, uint32_t id_lit2 ) {
        uint32_t const node_pos0 = id_lit0 >> 1;
        uint32_t const node_pos1 = id_lit1 >> 1;
        uint32_t const node_pos2 = id_lit2 >> 1;
        assert( node_pos0 < lits.size() );
        assert( node_pos1 < lits.size() );
        assert( node_pos2 < lits.size() );
        lits.emplace_back( add_clauses_for_3input_gate( lit_not_cond( lits[node_pos0], id_lit0 & 0x1 ), lit_not_cond( lits[node_pos1], id_lit1 & 0x1 ), lit_not_cond( lits[node_pos2], id_lit2 & 0x1 ), std::nullopt, MUX ) );
      } );
    }

    bill::lit_type lit_out;
    id_list.foreach_po( [&]( uint32_t id_lit ) {
      lit_out = lit_not_cond( lits[id_lit >> 1], ( id_lit & 0x1 ) ^ inverted );
    } );

    auto const res = validate( root, lit_out );

    if constexpr ( use_pushpop )
    {
      pop();
    }

    if ( solver.num_clauses() > ps.max_clauses && num_invoke >= MIN_NUM_INVOKE )
    {
      restart();
    }

    return res;
  }

  /*! \brief Validate whether signal `f` is a constant of `value`. */
  std::optional<bool> validate( signal const& f, bool value )
  {
    return validate( ntk.get_node( f ), value ^ ntk.is_complemented( f ) );
  }

  /*! \brief Validate whether node `root` is a constant of `value`. */
  std::optional<bool> validate( node const& root, bool value )
  {
    if ( !constructed.has( root ) && !ntk.is_pi( root ) && !ntk.is_constant( root ) )
    {
      construct( root );
    }

    std::optional<bool> res;
    if constexpr ( use_odc )
    {
      if ( ps.odc_levels != 0 )
      {
        if constexpr ( use_pushpop )
        {
          push();
        }
        res = solve( { build_odc_window( root, ~literals[root] ), lit_not_cond( literals[root], value ) } );
        if constexpr ( use_pushpop )
        {
          pop();
        }
      }
      else
      {
        res = solve( { lit_not_cond( literals[root], value ) } );
      }
    }
    else
    {
      res = solve( { lit_not_cond( literals[root], value ) } );
    }

    if ( solver.num_clauses() > ps.max_clauses && num_invoke >= MIN_NUM_INVOKE )
    {
      restart();
    }
    return res;
  }

  /*! \brief Generate pattern(s) for signal `f` to be `value`, optionally blocking several known patterns.
   *
   * Requires `use_pushpop = true`, which is only supported for `bsat2` and `z3`. If `bsat2` is used,
   * and if the network has more than 2048 PIs, the `BUFFER_SIZE` in `lib/bill/sat/interface/abc_bsat2.hpp`
   * has to be increased to at least `ntk.num_pis()`.
   *
   * If `block_patterns` and the returned vector are both empty, `f` is validated to be a constant of `!value`.
   *
   * \param block_patterns Patterns to be blocked in the solver. (Will not generate any of them.)
   * \param num_patterns Number of patterns to be generated, if possible. (The size of the result may be smaller than this number, but never larger.)
   */
  template<bool enabled = use_pushpop, typename = std::enable_if_t<enabled>>
  std::vector<std::vector<bool>> generate_pattern( signal const& f, bool value, std::vector<std::vector<bool>> const& block_patterns = {}, uint32_t num_patterns = 1u )
  {
    return generate_pattern( ntk.get_node( f ), value ^ ntk.is_complemented( f ), block_patterns, num_patterns );
  }

  /*! \brief Generate pattern(s) for node `root` to be `value`, optionally blocking several known patterns. */
  template<bool enabled = use_pushpop, typename = std::enable_if_t<enabled>>
  std::vector<std::vector<bool>> generate_pattern( node const& root, bool value, std::vector<std::vector<bool>> const& block_patterns = {}, uint32_t num_patterns = 1u )
  {
    if ( !constructed.has( root ) && !ntk.is_pi( root ) && !ntk.is_constant( root ) )
    {
      construct( root );
    }

    push();

    for ( auto const& pattern : block_patterns )
    {
      block_pattern( pattern );
    }

    std::vector<bill::lit_type> assumptions( { lit_not_cond( literals[root], !value ) } );
    if constexpr ( use_odc )
    {
      if ( ps.odc_levels != 0 )
      {
        assumptions.emplace_back( build_odc_window( root, ~literals[root] ) );
      }
    }

    std::optional<bool> res;
    std::vector<std::vector<bool>> generated;
    for ( auto i = 0u; i < num_patterns; ++i )
    {
      res = solve( assumptions );

      if ( !res || *res ) /* timeout or UNSAT */
      {
        break;
      }
      else /* SAT */
      {
        generated.emplace_back( cex );
        block_pattern( cex );
      }
    }

    pop();
    if ( solver.num_clauses() > ps.max_clauses && num_invoke >= MIN_NUM_INVOKE )
    {
      restart();
    }
    return generated;
  }

  /*! \brief Update CNF clauses.
   *
   * This function should be called when the function of one or more nodes
   * has been modified (typically when utilizing ODCs).
   */
  void update()
  {
    restart();
  }

private:
  void restart()
  {
    num_invoke = 0u;
    solver.restart();
    if constexpr ( randomize )
    {
      solver.set_random_phase( ps.random_seed );
    }

    constructed.reset();

    solver.add_variables( ntk.num_pis() + 1 );
    solver.add_clause( { ~literals[ntk.get_constant( false )] } );

    if constexpr ( has_EXCDC_interface_v<Ntk> )
    {
      ntk.add_EXCDC_clauses( solver );
    }

    if constexpr ( has_EXODC_interface_v<Ntk> )
    {
      if ( ps.odc_levels == -1 )
      {
        po_lits_link.clear();
        typename Ntk::base_type oec_ntk;
        ntk.build_oe_miter( oec_ntk );

        std::vector<bill::lit_type> po_lits;
        ntk.foreach_po( [&]( auto const& f ) {
          if ( !ntk.is_pi( ntk.get_node( f ) ) && !constructed.has( f ) )
          {
            construct( ntk.get_node( f ) );
          }
          po_lits.emplace_back( lit_not_cond( literals[f], ntk.is_complemented( f ) ) );
          po_lits_link.emplace_back( solver.add_variable(), bill::lit_type::polarities::positive );
        });


        /* OEC */
        assert( oec_ntk.num_pis() == ntk.num_pos() * 2 && oec_ntk.num_pos() == 1 );
        node_map<bill::lit_type, typename Ntk::base_type> oe_lits( oec_ntk );
        oe_lits[oec_ntk.get_constant( false )] = bill::lit_type( 0, bill::lit_type::polarities::positive );
        if ( oec_ntk.get_node( oec_ntk.get_constant( false ) ) != oec_ntk.get_node( oec_ntk.get_constant( true ) ) )
        {
          oe_lits[oec_ntk.get_constant( true )] = bill::lit_type( 0, bill::lit_type::polarities::negative );
        }
        oec_ntk.foreach_pi( [&]( auto const& n, auto i ) {
          oe_lits[n] = i < ntk.num_pos() ? po_lits[i] : po_lits_link[i - ntk.num_pos()];
        } );

        oec_ntk.foreach_gate( [&]( auto const& n ){
          oe_lits[n] = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
        });

        auto out_lits = generate_cnf<typename Ntk::base_type, bill::lit_type>( oec_ntk, add_clause_fn, oe_lits );
        solver.add_clause( {out_lits[0]} );
      }
    }
  }

  bill::lit_type construct( node const& n )
  {
    assert( !constructed.has( n ) && !ntk.is_pi( n ) && !ntk.is_constant( n ) );
    if constexpr ( use_pushpop )
    {
      if ( between_push_pop )
      {
        tmp.emplace_back( n );
      }
    }

    std::vector<bill::lit_type> child_lits;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      if ( !constructed.has( f ) && !ntk.is_pi( ntk.get_node( f ) ) && !ntk.is_constant( ntk.get_node( f ) ) )
      {
        construct( ntk.get_node( f ) );
      }
      child_lits.push_back( lit_not_cond( literals[f], ntk.is_complemented( f ) ) );
    } );
    bill::lit_type node_lit = literals[n] = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
    constructed[n] = true;

    if ( ntk.is_and( n ) )
    {
      detail::on_and<add_clause_fn_t>( node_lit, child_lits[0], child_lits[1], add_clause_fn );
    }
    else if ( ntk.is_xor( n ) )
    {
      detail::on_xor<add_clause_fn_t>( node_lit, child_lits[0], child_lits[1], add_clause_fn );
    }
    else if ( ntk.is_xor3( n ) )
    {
      detail::on_xor3<add_clause_fn_t>( node_lit, child_lits[0], child_lits[1], child_lits[2], add_clause_fn );
    }
    else if ( ntk.is_maj( n ) )
    {
      detail::on_maj<add_clause_fn_t>( node_lit, child_lits[0], child_lits[1], child_lits[2], add_clause_fn );
    }
    else if ( ntk.is_ite( n ) )
    {
      detail::on_ite<add_clause_fn_t>( node_lit, child_lits[0], child_lits[1], child_lits[2], add_clause_fn );
    }
    return node_lit;
  }

  void push()
  {
    solver.push();
    between_push_pop = true;
    tmp.clear();
  }

  void pop()
  {
    solver.pop();
    for ( auto& n : tmp )
    {
      constructed.erase( n );
    }
    between_push_pop = false;
  }

  bill::lit_type add_clauses_for_2input_gate( bill::lit_type a, bill::lit_type b, std::optional<bill::lit_type> c = std::nullopt, gate_type type = AND )
  {
    assert( type == AND || type == XOR );

    auto nlit = c ? *c : bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
    if ( type == AND )
    {
      detail::on_and<add_clause_fn_t>( nlit, a, b, add_clause_fn );
    }
    else if ( type == XOR )
    {
      detail::on_xor<add_clause_fn_t>( nlit, a, b, add_clause_fn );
    }

    return nlit;
  }

  bill::lit_type add_clauses_for_3input_gate( bill::lit_type a, bill::lit_type b, bill::lit_type c, std::optional<bill::lit_type> d = std::nullopt, gate_type type = MAJ )
  {
    assert( type == MAJ || type == XOR || type == MUX );

    auto nlit = d ? *d : bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
    if ( type == MAJ )
    {
      detail::on_maj<add_clause_fn_t>( nlit, a, b, c, add_clause_fn );
    }
    else if ( type == XOR )
    {
      detail::on_xor3<add_clause_fn_t>( nlit, a, b, c, add_clause_fn );
    }
    else if ( type == MUX )
    {
      detail::on_ite<add_clause_fn_t>( nlit, a, b, c, add_clause_fn );
    }

    return nlit;
  }

  std::optional<bool> solve( std::vector<bill::lit_type> assumptions )
  {
    ++num_invoke;
    auto const res = solver.solve( assumptions, ps.conflict_limit );

    if ( res == bill::result::states::satisfiable )
    {
      auto model = solver.get_model().model();
      for ( auto i = 0u; i < ntk.num_pis(); ++i )
      {
        cex.at( i ) = model.at( i + 1 ) == bill::lbool_type::true_;
      }

      if constexpr ( has_EXCDC_interface_v<Ntk> )
      {
        assert( !ntk.pattern_is_EXCDC( cex ) );
      }
      return false;
    }
    else if ( res == bill::result::states::unsatisfiable )
    {
      return true;
    }
    else
    {
      return std::nullopt; /* timeout or something wrong */
    }
  }

  std::optional<bool> validate( node const& root, bill::lit_type const& lit )
  {
    if ( !constructed.has( root ) && !ntk.is_pi( root ) && !ntk.is_constant( root ) )
    {
      construct( root );
    }

    std::optional<bool> res;
    if constexpr ( use_odc )
    {
      if ( ps.odc_levels != 0 )
      {
        if constexpr ( use_pushpop )
        {
          push();
        }
        res = solve( { build_odc_window( root, lit ) } );
        if constexpr ( use_pushpop )
        {
          pop();
        }
      }
      else
      {
        auto nlit = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
        solver.add_clause( { literals[root], lit, nlit } );
        solver.add_clause( { ~( literals[root] ), ~lit, nlit } );
        res = solve( { ~nlit } );
      }
    }
    else
    {
      auto nlit = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
      solver.add_clause( { literals[root], lit, nlit } );
      solver.add_clause( { ~( literals[root] ), ~lit, nlit } );
      res = solve( { ~nlit } );
    }

    return res;
  }

  void block_pattern( std::vector<bool> const& pattern )
  {
    assert( pattern.size() == ntk.num_pis() );
    std::vector<bill::lit_type> clause;
    ntk.foreach_pi( [&]( auto const& n, auto i ) {
      clause.emplace_back( lit_not_cond( literals[n], pattern[i] ) );
    } );
    solver.add_clause( clause );
  }

private:
  template<bool enabled = use_odc, typename = std::enable_if_t<enabled>>
  bill::lit_type build_odc_window( node const& root, bill::lit_type const& lit )
  {
    /* literals for the duplicated fanout cone */
    unordered_node_map<bill::lit_type, Ntk> lits( ntk );
    /* miter literals that should be empty */
    std::vector<bill::lit_type> miter;

    lits[root] = lit;
    ntk.incr_trav_id();
    make_lit_fanout_cone_rec( root, lits, miter, 1 );
    ntk.incr_trav_id();
    duplicate_fanout_cone_rec( root, lits, 1 );

    if constexpr ( has_EXODC_interface_v<Ntk> )
    {
      if ( ps.odc_levels == -1 )
      {
        assert( miter.size() == 0 );
        auto assump = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
        ntk.foreach_po( [&]( auto const& f, auto i ) {
          auto dup_po_lit = lit_not_cond( lits.has( ntk.get_node( f ) ) ? lits[f] : literals[f], ntk.is_complemented( f ) );
          solver.add_clause( {~assump, ~po_lits_link[i], dup_po_lit} );
          solver.add_clause( {~assump, po_lits_link[i], ~dup_po_lit} );
        } );
        return assump;
      }
    }

    /* miter for POs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( !lits.has( ntk.get_node( f ) ) )
        return true; /* PO not in TFO, skip */
      add_miter_clauses( ntk.get_node( f ), lits, miter );
      return true; /* next */
    } );

    assert( miter.size() > 0 && "no fanout node at distance odc_levels and there is no PO in TFO cone (possibly due to a dangling cone)" );
    auto assump = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );
    miter.emplace_back( ~assump );
    solver.add_clause( miter );
    return assump;
  }

  template<bool enabled = use_odc, typename = std::enable_if_t<enabled>>
  void duplicate_fanout_cone_rec( node const& n, unordered_node_map<bill::lit_type, Ntk> const& lits, int32_t level )
  {
    ntk.foreach_fanout( n, [&]( auto const& fo ) {
      if ( ntk.visited( fo ) == ntk.trav_id() )
        return true; /* skip */
      ntk.set_visited( fo, ntk.trav_id() );

      std::vector<bill::lit_type> l_fi;
      ntk.foreach_fanin( fo, [&]( auto const& fi ) {
        if ( !constructed.has( fi ) && !ntk.is_pi( ntk.get_node( fi ) ) && !ntk.is_constant( ntk.get_node( fi ) ) )
        {
          construct( ntk.get_node( fi ) );
        }
        l_fi.emplace_back( lit_not_cond( lits.has( ntk.get_node( fi ) ) ? lits[fi] : literals[fi], ntk.is_complemented( fi ) ) );
      } );
      if ( l_fi.size() == 2u )
      {
        assert( ntk.is_and( fo ) || ntk.is_xor( fo ) );
        add_clauses_for_2input_gate( l_fi[0], l_fi[1], lits[fo], ntk.is_and( fo ) ? AND : XOR );
      }
      else
      {
        assert( l_fi.size() == 3u );
        assert( ntk.is_maj( fo ) || ntk.is_xor3( fo ) );
        add_clauses_for_3input_gate( l_fi[0], l_fi[1], l_fi[2], lits[fo], ntk.is_maj( fo ) ? MAJ : XOR );
      }

      if ( level == ps.odc_levels )
        return true;

      duplicate_fanout_cone_rec( fo, lits, level + 1 );
      return true; /* next */
    } );
  }

  template<bool enabled = use_odc, typename = std::enable_if_t<enabled>>
  void make_lit_fanout_cone_rec( node const& n, unordered_node_map<bill::lit_type, Ntk>& lits, std::vector<bill::lit_type>& miter, int32_t level )
  {
    ntk.foreach_fanout( n, [&]( auto const& fo ) {
      if ( ntk.visited( fo ) == ntk.trav_id() )
        return true; /* skip */
      ntk.set_visited( fo, ntk.trav_id() );

      if ( !constructed.has( fo ) )
      {
        construct( fo );
      }

      lits[fo] = bill::lit_type( solver.add_variable(), bill::lit_type::polarities::positive );

      if ( level == ps.odc_levels )
      {
        add_miter_clauses( fo, lits, miter );
        return true;
      }

      make_lit_fanout_cone_rec( fo, lits, miter, level + 1 );
      return true; /* next */
    } );
  }

  template<bool enabled = use_odc, typename = std::enable_if_t<enabled>>
  void add_miter_clauses( node const& n, unordered_node_map<bill::lit_type, Ntk> const& lits, std::vector<bill::lit_type>& miter )
  {
    assert( constructed.has( n ) && literals[n] != literals[ntk.get_constant( false )] );
    miter.emplace_back( add_clauses_for_2input_gate( literals[n], lits[n], std::nullopt, XOR ) );
  }

private:
  Ntk const& ntk;

  validator_params ps;

  node_map<bill::lit_type, Ntk> literals;
  unordered_node_map<bool, Ntk> constructed;
  bill::solver<Solver> solver;
  add_clause_fn_t add_clause_fn = [&]( auto const& clause ) { solver.add_clause( clause ); };

  static const uint32_t MIN_NUM_INVOKE = 20u;
  uint32_t num_invoke;

  bool between_push_pop = false;
  std::vector<node> tmp;

  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;

  std::vector<bill::lit_type> po_lits_link;

public:
  std::vector<bool> cex;
};

} /* namespace mockturtle */