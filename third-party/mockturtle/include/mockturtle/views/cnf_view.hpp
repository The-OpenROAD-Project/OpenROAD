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
  \file cnf_view.hpp
  \brief Creates a CNF while creating a network

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

#include "../algorithms/cnf.hpp"
#include "../traits.hpp"
#include "../utils/include/percy.hpp"

#include <bill/sat/interface/common.hpp>
#include <bill/sat/interface/glucose.hpp>
#include <fmt/format.h>

namespace mockturtle
{

struct cnf_view_params
{
  /*! \brief Write DIMACS file, whenever solve is called. */
  std::optional<std::string> write_dimacs{};

  /*! \brief Automatically update clauses when network is modified.
             Only meaningful when AllowModify = true. */
  bool auto_update{ true };
};

/* forward declaration */
template<typename Ntk, bool AllowModify = false, bill::solvers Solver = bill::solvers::glucose_41>
class cnf_view;

namespace detail
{

template<typename CnfView, typename Ntk, bool AllowModify = false, bill::solvers Solver = bill::solvers::glucose_41>
class cnf_view_impl : public Ntk
{
public:
  cnf_view_impl( CnfView& cnf_view )
      : Ntk()
  {
    (void)cnf_view;
  }
};

template<typename CnfView, typename Ntk, bill::solvers Solver>
class cnf_view_impl<CnfView, Ntk, true, Solver> : public Ntk
{
  friend class cnf_view<Ntk, true, Solver>;
  using node = typename Ntk::node;

public:
  cnf_view_impl( CnfView& cnf_view )
      : Ntk(), cnf_view_( cnf_view ), literals_( *this )
  {
  }

  cnf_view_impl( CnfView& cnf_view, Ntk& ntk )
      : Ntk( ntk ), cnf_view_( cnf_view ), literals_( *this )
  {
  }

  ~cnf_view_impl()
  {
  }

  void init()
  {
    cnf_view_.solver_.add_variables( Ntk::size() );

    /* unit clause for constants */
    bill::lit_type lit_const( 0, bill::lit_type::polarities::negative );
    cnf_view_.add_clause( lit_const );
    literals_[Ntk::get_constant( false )] = lit_const;
    if ( Ntk::get_node( Ntk::get_constant( false ) ) != Ntk::get_node( Ntk::get_constant( true ) ) )
    {
      literals_[Ntk::get_constant( true )] = ~lit_const;
    }

    uint32_t v = 0;
    Ntk::foreach_pi( [&]( auto const& n ) {
      literals_[n] = bill::lit_type( ++v, bill::lit_type::polarities::positive );
    } );

    Ntk::foreach_gate( [&]( auto const& n ) {
      literals_[n] = bill::lit_type( ++v, bill::lit_type::polarities::positive );
      cnf_view_.on_add( n, false );
    } );
  }

  inline bill::var_type add_var()
  {
    return cnf_view_.solver_.add_variable();
  }

  /*! \brief Returns the switching literal associated to a node. */
  inline bill::lit_type switch_lit( node const& n ) const
  {
    assert( !Ntk::is_pi( n ) && !Ntk::is_constant( n ) && "PI and constant node are not switch-able" );
    return switches_[Ntk::node_to_index( n )];
  }

  /*! \brief Whether a node is currently activated (included in CNF). */
  inline bool is_activated( node const& n ) const
  {
    return switch_lit( n ).is_complemented();
    /* clauses are activated if switch literal is complemented */
  }

  /*! \brief Deactivates the clauses for a node. */
  void deactivate( node const& n )
  {
    if ( is_activated( n ) )
    {
      switches_[Ntk::node_to_index( n )].complement();
    }
  }

  /*! \brief (Re-)activates the clauses for a node. */
  void activate( node const& n )
  {
    if ( !is_activated( n ) )
    {
      switches_[Ntk::node_to_index( n )].complement();
    }
  }

  void on_modified( node const& n )
  {
    deactivate( n );
    cnf_view_.add_clause( switch_lit( n ) );
    cnf_view_.on_add( n, false );
    /* reuse literals_[n] (so that the fanout clauses are still valid),
    but create a new switches_[n] to control a new set of gate clauses */
  }

  void on_delete( node const& n )
  {
    deactivate( n );
  }

private:
  CnfView& cnf_view_;

  node_map<bill::lit_type, Ntk> literals_;
  std::vector<bill::lit_type> switches_;
};

} /* namespace detail */

/*! \brief A view to connect logic network creation to SAT solving.
 *
 * When using this view to create a new network, it creates a CNF internally
 * while nodes are added to the network.  It also contains a SAT solver.  The
 * network can be solved by calling the `solve` method, which by default assumes
 * that each output should compute `true` (an overload of the `solve` method can
 * override this default behaviour and apply custom assumptions).  Further, the
 * methods `model_value` and `pi_vmodel_alues` can be used to access model
 * values in case solving was satisfiable.  Finally, methods `var` and `lit` can
 * be used to access variable and literal information for nodes and signals,
 * respectively, in order to add custom clauses with the `add_clause` methods.
 *
 * The `cnf_view` can also be wrapped around an existing network by setting the
 * `AllowModify` template parameter to true.  Then it also updates the CNF when
 * nodes are deleted or modified.  This comes with an addition cost in variable
 * and clause size.
 */
template<typename Ntk, bool AllowModify, bill::solvers Solver>
class cnf_view : public detail::cnf_view_impl<cnf_view<Ntk, AllowModify, Solver>, Ntk, AllowModify, Solver>
{
  friend class detail::cnf_view_impl<cnf_view<Ntk, AllowModify, Solver>, Ntk, AllowModify, Solver>;

public:
  using cnf_view_impl_t = detail::cnf_view_impl<cnf_view<Ntk, AllowModify, Solver>, Ntk, AllowModify, Solver>;

  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  // can only be constructed as empty network
  explicit cnf_view( cnf_view_params const& ps = {} )
      : cnf_view_impl_t( *this ),
        ps_( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

    if constexpr ( AllowModify )
    {
      cnf_view_impl_t::init();
    }
    else
    {
      const auto v = solver_.add_variable(); /* for the constant input */
      assert( v == var( Ntk::get_node( Ntk::get_constant( false ) ) ) );
      add_clause( bill::lit_type( v, bill::lit_type::polarities::negative ) );

      if ( Ntk::get_node( Ntk::get_constant( true ) ) != Ntk::get_node( Ntk::get_constant( false ) ) )
      {
        const auto v = solver_.add_variable(); /* for the constant input */
        assert( v == var( Ntk::get_node( Ntk::get_constant( true ) ) ) );
        add_clause( bill::lit_type( v, bill::lit_type::polarities::positive ) );
      }
    }

    register_events();
  }

  template<bool enabled = AllowModify, typename = std::enable_if_t<enabled>>
  explicit cnf_view( Ntk& ntk, cnf_view_params const& ps = {} )
      : cnf_view_impl_t( *this, ntk ),
        ps_( ps )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
    static_assert( has_node_function_v<Ntk>, "Ntk does not implement the node_function method" );

    cnf_view_impl_t::init();

    register_events();
  }

  ~cnf_view()
  {
    if ( add_event )
    {
      Ntk::events().release_add_event( add_event );
    }
    if ( modified_event )
    {
      Ntk::events().release_modified_event( modified_event );
    }
    if ( delete_event )
    {
      Ntk::events().release_delete_event( delete_event );
    }
  }

  signal create_pi()
  {
    const auto f = Ntk::create_pi();

    const auto v = solver_.add_variable();

    if constexpr ( AllowModify )
    {
      cnf_view_impl_t::literals_.resize( bill::lit_type( 0, bill::lit_type::polarities::positive ) );
      cnf_view_impl_t::literals_[f] = bill::lit_type( v, bill::lit_type::polarities::positive );
      return f;
    }

    assert( v == var( Ntk::get_node( f ) ) );
    (void)v;

    return f;
  }

  /* \brief Returns the variable associated to a node. */
  inline bill::var_type var( node const& n ) const
  {
    if constexpr ( AllowModify )
    {
      return cnf_view_impl_t::literals_[n].variable();
    }
    return Ntk::node_to_index( n );
  }

  /*! \brief Returns the literal associated to a node. */
  inline bill::lit_type lit( node const& n ) const
  {
    return bill::lit_type( var( n ), bill::lit_type::polarities::positive );
  }

  /*! \brief Returns the literal associated to a signal. */
  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  inline bill::lit_type lit( signal const& f ) const
  {
    return bill::lit_type( var( Ntk::get_node( f ) ), Ntk::is_complemented( f ) ? bill::lit_type::polarities::negative : bill::lit_type::polarities::positive );
  }

  /*! \brief Solves the network with a set of custom assumptions.
   *
   * This function does not assert any primary output, unless specified
   * explicitly through the assumptions.
   *
   * The function returns `nullopt`, if no solution can be found (due to a
   * conflict limit), or `true` in case of SAT, and `false` in case of UNSAT.
   *
   * \param assumptions Vector of literals to be assumped when solving
   * \param limit Conflict limit (unlimited if 0)
   */
  inline std::optional<bool> solve( bill::result::clause_type const& assumptions, uint32_t limit = 0 )
  {
    const auto _write_dimacs = [&]( bill::result::clause_type const& assumps ) {
      if ( ps_.write_dimacs )
      {
        for ( const auto& a : assumps )
        {
          auto l = pabc::Abc_Var2Lit( a.variable(), a.is_complemented() );
          dimacs_.add_clause( &l, &l + 1 );
        }
        dimacs_.set_nr_vars( solver_.num_variables() );
#ifdef _MSC_VER
        FILE* fd = nullptr;
        fopen_s( &fd, ps_.write_dimacs->c_str(), "w" );
#else
        FILE* fd = fopen( ps_.write_dimacs->c_str(), "w" );
#endif
        dimacs_.to_dimacs( fd );
        fclose( fd );
      }
    };

    const auto _solve = [&]( bill::result::clause_type const& assumps ) -> std::optional<bool> {
      const auto res = solver_.solve( assumps, limit );

      switch ( res )
      {
      case bill::result::states::satisfiable:
        model_ = solver_.get_model().model();
        return true;
      case bill::result::states::unsatisfiable:
        return false;
      default:
        return std::nullopt;
      }

      return std::nullopt;
    };

    if constexpr ( AllowModify )
    {
      bill::result::clause_type assumptions_copy = assumptions;
      for ( auto i = 1u; i < cnf_view_impl_t::switches_.size(); ++i )
      {
        if ( !Ntk::is_pi( Ntk::index_to_node( i ) ) )
        {
          assumptions_copy.push_back( cnf_view_impl_t::switches_[i] );
        }
      }

      _write_dimacs( assumptions_copy );
      return _solve( assumptions_copy );
    }

    _write_dimacs( assumptions );
    return _solve( assumptions );
  }

  /*! \brief Solves the network by asserting all primary outputs to be true
   *
   * The function returns `nullopt`, if no solution can be found (due to a
   * conflict limit), or `true` in case of SAT, and `false` in case of UNSAT.
   *
   * \param limit Conflict limit (unlimited if 0)
   */
  inline std::optional<bool> solve( int limit = 0 )
  {
    bill::result::clause_type assumptions;
    Ntk::foreach_po( [&]( auto const& f ) {
      assumptions.push_back( lit( f ) );
    } );
    return solve( assumptions, limit );
  }

  /*! \brief Return model value for a node. */
  inline bool model_value( node const& n ) const
  {
    return model_.at( var( n ) ) == bill::lbool_type::true_;
  }

  /*! \brief Return model value for a node (takes complementation into account). */
  template<typename _Ntk = Ntk, typename = std::enable_if_t<!std::is_same_v<typename _Ntk::signal, typename _Ntk::node>>>
  inline bool model_value( signal const& f ) const
  {
    return model_value( Ntk::get_node( f ) ) != Ntk::is_complemented( f );
  }

  /* \brief Returns all model values for all primary inputs. */
  std::vector<bool> pi_model_values()
  {
    std::vector<bool> values( Ntk::num_pis() );
    Ntk::foreach_pi( [&]( auto const& n, auto i ) {
      values[i] = model_value( n );
    } );
    return values;
  }

  /*! \brief Blocks last model for primary input values. */
  void block()
  {
    bill::result::clause_type blocking_clause;
    Ntk::foreach_pi( [&]( auto const& n ) {
      blocking_clause.push_back( bill::lit_type( var( n ), model_value( n ) ? bill::lit_type::polarities::negative : bill::lit_type::polarities::positive ) );
    } );
    add_clause( blocking_clause );
  }

  /*! \brief Number of variables. */
  inline uint32_t num_vars() const
  {
    return solver_.num_variables();
  }

  /*! \brief Number of clauses. */
  inline uint32_t num_clauses() const
  {
    return solver_.num_clauses();
  }

  /*! \brief Adds a clause to the solver. */
  void add_clause( bill::result::clause_type const& clause )
  {
    if ( ps_.write_dimacs )
    {
      std::vector<int> lits;
      for ( auto c : clause )
      {
        lits.push_back( pabc::Abc_Var2Lit( c.variable(), c.is_complemented() ) );
      }
      dimacs_.add_clause( &lits[0], &lits[0] + lits.size() );
    }
    solver_.add_clause( clause );
  }

  /*! \brief Adds a clause from signals to the solver. */
  void add_clause( std::vector<signal> const& clause )
  {
    bill::result::clause_type lits;
    std::transform( clause.begin(), clause.end(), std::back_inserter( lits ), [&]( auto const& s ) { return lit( s ); } );
    add_clause( lits );
  }

  /*! \brief Adds a clause to the solver.
   *
   * Entries are either all literals or network signals.
   */
  template<typename... Lit, typename = std::enable_if_t<
                                std::disjunction_v<
                                    std::conjunction<std::is_same<Lit, bill::lit_type>...>,
                                    std::conjunction<std::is_same<Lit, signal>...>>>>
  void add_clause( Lit... lits )
  {
    if constexpr ( std::conjunction_v<std::is_same<Lit, bill::lit_type>...> )
    {
      add_clause( bill::result::clause_type{ { lits... } } );
    }
    else
    {
      add_clause( bill::result::clause_type{ { lit( lits )... } } );
    }
  }

private:
  void register_events()
  {
    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
    modified_event = Ntk::events().register_modified_event( [this]( auto const& n, auto const& previous ) {
      (void)previous;
      if constexpr ( AllowModify )
      {
        if ( ps_.auto_update )
        {
          cnf_view_impl_t::on_modified( n );
        }
        return;
      }

      (void)n;
      (void)this;
      assert( false && "nodes should not be modified in cnf_view" );
      std::abort();
    } );
    delete_event = Ntk::events().register_delete_event( [this]( auto const& n ) {
      if constexpr ( AllowModify )
      {
        if ( ps_.auto_update )
        {
          cnf_view_impl_t::on_delete( n );
        }
        return;
      }

      (void)n;
      (void)this;
      assert( false && "nodes should not be deleted in cnf_view" );
      std::abort();
    } );
  }

  void on_add( node const& n, bool add_var = true ) /* add_var is only used when AllowModify = true */
  {
    bill::lit_type node_lit;
    bill::lit_type switch_lit;

    if constexpr ( AllowModify )
    {
      if ( add_var )
      {
        node_lit = bill::lit_type( solver_.add_variable(), bill::lit_type::polarities::positive );
        cnf_view_impl_t::literals_.resize();
        cnf_view_impl_t::literals_[n] = node_lit;
      }
      else
      {
        node_lit = cnf_view_impl_t::literals_[n];
      }

      switch_lit = bill::lit_type( solver_.add_variable(), bill::lit_type::polarities::positive );
      cnf_view_impl_t::switches_.resize( Ntk::size() );
      cnf_view_impl_t::switches_[Ntk::node_to_index( n )] = ~switch_lit;
    }
    else
    {
      (void)add_var;
      const auto v = solver_.add_variable();
      assert( v == var( n ) );
      (void)v;

      node_lit = lit( Ntk::make_signal( n ) );
    }

    const auto _add_clause = [&]( bill::result::clause_type const& clause ) {
      if constexpr ( AllowModify )
      {
        bill::result::clause_type clause_ = clause;
        clause_.push_back( switch_lit );
        add_clause( clause_ );
      }
      else
      {
        add_clause( clause );
      }
    };

    bill::result::clause_type child_lits;
    Ntk::foreach_fanin( n, [&]( auto const& f ) {
      child_lits.push_back( lit( f ) );
    } );

    if constexpr ( has_is_and_v<Ntk> )
    {
      if ( Ntk::is_and( n ) )
      {
        detail::on_and( node_lit, child_lits[0], child_lits[1], _add_clause );
        return;
      }
    }

    if constexpr ( has_is_or_v<Ntk> )
    {
      if ( Ntk::is_or( n ) )
      {
        detail::on_or( node_lit, child_lits[0], child_lits[1], _add_clause );
        return;
      }
    }

    if constexpr ( has_is_xor_v<Ntk> )
    {
      if ( Ntk::is_xor( n ) )
      {
        detail::on_xor( node_lit, child_lits[0], child_lits[1], _add_clause );
        return;
      }
    }

    if constexpr ( has_is_maj_v<Ntk> )
    {
      if ( Ntk::is_maj( n ) )
      {
        detail::on_maj( node_lit, child_lits[0], child_lits[1], child_lits[2], _add_clause );
        return;
      }
    }

    if constexpr ( has_is_ite_v<Ntk> )
    {
      if ( Ntk::is_ite( n ) )
      {
        detail::on_ite( node_lit, child_lits[0], child_lits[1], child_lits[2], _add_clause );
        return;
      }
    }

    if constexpr ( has_is_xor3_v<Ntk> )
    {
      if ( Ntk::is_xor3( n ) )
      {
        detail::on_xor3( node_lit, child_lits[0], child_lits[1], child_lits[2], _add_clause );
        return;
      }
    }

    if constexpr ( has_is_nary_and_v<Ntk> )
    {
      if ( Ntk::is_nary_and( n ) )
      {
        fmt::print( stderr, "[e] nary-AND not yet supported in generate_cnf" );
        std::abort();
        return;
      }
    }

    if constexpr ( has_is_nary_or_v<Ntk> )
    {
      if ( Ntk::is_nary_or( n ) )
      {
        fmt::print( stderr, "[e] nary-OR not yet supported in generate_cnf" );
        std::abort();
        return;
      }
    }

    if constexpr ( has_is_nary_xor_v<Ntk> )
    {
      if ( Ntk::is_nary_xor( n ) )
      {
        fmt::print( stderr, "[e] nary-XOR not yet supported in generate_cnf" );
        std::abort();
        return;
      }
    }

    detail::on_function( node_lit, child_lits, Ntk::node_function( n ), _add_clause );
  }

private:
  bill::solver<Solver> solver_;
  bill::result::model_type model_;
  percy::cnf_formula dimacs_;

  cnf_view_params ps_;

  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;
  std::shared_ptr<typename network_events<Ntk>::modified_event_type> modified_event;
  std::shared_ptr<typename network_events<Ntk>::delete_event_type> delete_event;
};

template<class T>
cnf_view( T const& ) -> cnf_view<T, true>;

} /* namespace mockturtle */