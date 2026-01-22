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
  \file linear_resynthesis.hpp
  \brief Resynthesize linear circuit

  \author Eleonora Testa
  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <iostream>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../algorithms/cnf.hpp"
#include "../algorithms/simulation.hpp"
#include "../networks/xag.hpp"
#include "../traits.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/cnf_view.hpp"

#include <fmt/format.h>

namespace mockturtle
{

namespace detail
{

class linear_sum_simulator
{
public:
  std::vector<uint32_t> compute_constant( bool ) const { return {}; }
  std::vector<uint32_t> compute_pi( uint32_t index ) const { return { index }; }
  std::vector<uint32_t> compute_not( std::vector<uint32_t> const& value ) const
  {
    assert( false && "No NOTs in linear forms allowed" );
    std::abort();
    return value;
  }
};

class linear_matrix_simulator
{
public:
  linear_matrix_simulator( uint32_t num_inputs ) : num_inputs_( num_inputs ) {}

  std::vector<bool> compute_constant( bool ) const { return std::vector<bool>( num_inputs_, false ); }
  std::vector<bool> compute_pi( uint32_t index ) const
  {
    std::vector<bool> row( num_inputs_, false );
    row[index] = true;
    return row;
  }
  std::vector<bool> compute_not( std::vector<bool> const& value ) const
  {
    assert( false && "No NOTs in linear forms allowed" );
    std::abort();
    return value;
  }

private:
  uint32_t num_inputs_;
};

class linear_xag : public xag_network
{
public:
  linear_xag( xag_network const& xag ) : xag_network( xag ) {}

  template<typename Iterator>
  iterates_over_t<Iterator, std::vector<uint32_t>>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_pi( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto set1 = *begin++;
    auto set2 = *begin++;

    if ( c1.index < c2.index )
    {
      assert( false );
      std::abort();
      return {};
    }
    else
    {
      std::vector<uint32_t> result;
      auto it1 = set1.begin();
      auto it2 = set2.begin();

      while ( it1 != set1.end() && it2 != set2.end() )
      {
        if ( *it1 < *it2 )
        {
          result.push_back( *it1++ );
        }
        else if ( *it1 > *it2 )
        {
          result.push_back( *it2++ );
        }
        else
        {
          ++it1;
          ++it2;
        }
      }

      if ( it1 != set1.end() )
      {
        std::copy( it1, set1.end(), std::back_inserter( result ) );
      }
      else if ( it2 != set2.end() )
      {
        std::copy( it2, set2.end(), std::back_inserter( result ) );
      }

      return result;
    }
  }

  template<typename Iterator>
  iterates_over_t<Iterator, std::vector<bool>>
  compute( node const& n, Iterator begin, Iterator end ) const
  {
    (void)end;

    assert( n != 0 && !is_pi( n ) );

    auto const& c1 = _storage->nodes[n].children[0];
    auto const& c2 = _storage->nodes[n].children[1];

    auto set1 = *begin++;
    auto set2 = *begin++;

    if ( c1.index < c2.index )
    {
      assert( false );
      std::abort();
      return {};
    }
    else
    {
      std::vector<bool> result( set1.size() );
      std::transform( set1.begin(), set1.end(), set2.begin(), result.begin(), std::not_equal_to<bool>{} );
      return result;
    }
  }
};

struct pair_hash
{
  template<class T1, class T2>
  std::size_t operator()( std::pair<T1, T2> const& p ) const
  {
    return std::hash<T1>()( p.first ) ^ std::hash<T2>()( p.second );
  }
};

template<class Ntk>
struct linear_resynthesis_paar_impl
{
public:
  using index_pair_t = std::pair<uint32_t, uint32_t>;

  linear_resynthesis_paar_impl( Ntk const& xag ) : xag( xag ) {}

  Ntk run()
  {
    xag.foreach_pi( [&]( auto const& ) {
      signals.push_back( dest.create_pi() );
    } );

    extract_linear_equations();

    while ( !occurrence_to_pairs.empty() )
    {
      const auto p = *( occurrence_to_pairs.back().begin() );
      replace_one_pair( p );
    }

    xag.foreach_po( [&]( auto const& f, auto i ) {
      if ( linear_equations[i].empty() )
      {
        dest.create_po( dest.get_constant( xag.is_complemented( f ) ) );
      }
      else
      {
        assert( linear_equations[i].size() == 1u );
        dest.create_po( signals[linear_equations[i].front()] ^ xag.is_complemented( f ) );
      }
    } );

    return dest;
  }

private:
  void extract_linear_equations()
  {
    occurrence_to_pairs.resize( 1u );

    linear_xag lxag{ xag };
    linear_equations = simulate<std::vector<uint32_t>>( lxag, linear_sum_simulator{} );

    for ( auto o = 0u; o < linear_equations.size(); ++o )
    {
      const auto& lin_eq = linear_equations[o];
      for ( auto j = 1u; j < lin_eq.size(); ++j )
      {
        for ( auto i = 0u; i < j; ++i )
        {
          const auto p = std::make_pair( lin_eq[i], lin_eq[j] );
          pairs_to_output[p].push_back( o );
          add_pair( p );
        }
      }
    }
  }

  void add_pair( index_pair_t const& p )
  {
    if ( auto it = pair_to_occurrence.find( p ); it != pair_to_occurrence.end() )
    {
      // found another time
      const auto occ = it->second;
      occurrence_to_pairs[occ - 1u].erase( p );
      if ( occurrence_to_pairs.size() <= occ + 1u )
      {
        occurrence_to_pairs.resize( occ + 1u );
      }
      occurrence_to_pairs[occ].insert( p );
      it->second++;
    }
    else
    {
      // first time found
      pair_to_occurrence[p] = 1u;
      occurrence_to_pairs[0u].insert( p );
    }
  }

  void remove_all_pairs( index_pair_t const& p )
  {
    auto it = pair_to_occurrence.find( p );
    const auto occ = it->second;
    pair_to_occurrence.erase( it );
    occurrence_to_pairs[occ - 1u].erase( p );
    while ( !occurrence_to_pairs.empty() && occurrence_to_pairs.back().empty() )
    {
      occurrence_to_pairs.pop_back();
    }
    pairs_to_output.erase( p );
  }

  void remove_one_pair( index_pair_t const& p, uint32_t output )
  {
    auto it = pair_to_occurrence.find( p );
    const auto occ = it->second;
    occurrence_to_pairs[occ - 1u].erase( p );
    if ( occ > 1u )
    {
      occurrence_to_pairs[occ - 2u].insert( p );
    }
    it->second--;
    pairs_to_output[p].erase( std::remove( pairs_to_output[p].begin(), pairs_to_output[p].end(), output ), pairs_to_output[p].end() );
  }

  void replace_one_pair( index_pair_t const& p )
  {
    const auto [a, b] = p;
    auto c = static_cast<uint32_t>( signals.size() );
    signals.push_back( dest.create_xor( signals[a], signals[b] ) );

    /* update data structures */
    for ( auto o : pairs_to_output[p] )
    {
      auto& leq = linear_equations[o];
      leq.erase( std::remove( leq.begin(), leq.end(), a ), leq.end() );
      leq.erase( std::remove( leq.begin(), leq.end(), b ), leq.end() );
      for ( auto i : leq )
      {
        remove_one_pair( { std::min( i, a ), std::max( i, a ) }, o );
        remove_one_pair( { std::min( i, b ), std::max( i, b ) }, o );
        add_pair( { i, c } );
        pairs_to_output[{ i, c }].push_back( o );
      }
      leq.push_back( c );
    }
    remove_all_pairs( p );
  }

  void print_linear_matrix()
  {
    for ( auto const& le : linear_equations )
    {
      auto it = le.begin();
      for ( auto i = 0u; i < signals.size(); ++i )
      {
        if ( it != le.end() && *it == i )
        {
          std::cout << " 1";
          it++;
        }
        else
        {
          std::cout << " 0";
        }
      }
      assert( it == le.end() );
      std::cout << "\n";
    }
  }

private:
  Ntk const& xag;
  Ntk dest;
  std::vector<signal<Ntk>> signals;
  std::vector<std::vector<uint32_t>> linear_equations;
  std::vector<std::unordered_set<index_pair_t, pair_hash>> occurrence_to_pairs;
  std::unordered_map<index_pair_t, uint32_t, pair_hash> pair_to_occurrence;
  std::unordered_map<index_pair_t, std::vector<uint32_t>, pair_hash> pairs_to_output;
};

} // namespace detail

/*! \brief Linear circuit resynthesis (Paar's algorithm)
 *
 * This algorithm works on an XAG that is only composed of XOR gates.  It
 * extracts a matrix representation of the linear output equations and
 * resynthesizes them in a greedy manner by always substituting the most
 * frequent pair of variables using the computed function of an XOR gate.
 *
 * Reference: [C. Paar, IEEE Int'l Symp. on Inf. Theo. (1997), page 250]
 */
template<typename Ntk>
Ntk linear_resynthesis_paar( Ntk const& xag )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Ntk is not XAG-like" );

  return detail::linear_resynthesis_paar_impl<Ntk>( xag ).run();
}

struct exact_linear_synthesis_params
{
  /*! \brief Upper bound on number of XOR gates. If used, best solution is found decreasing */
  std::optional<uint32_t> upper_bound{};

  /*! \brief Conflict limit for SAT solving (default 0 = no limit). */
  int conflict_limit{ 0 };

  /*! \brief Solution must be cancellation-free. */
  bool cancellation_free{ false };

  /*! \brief Ignore inputs in any step to compute this output.
   *
   * Either the vector is empty, if no inputs should be ignored, or it has as
   * many entries as rows in the input matrix.  Each entry is a vector of input
   * indexes (starting from 0) to be ignored, an entry can be the empty vector,
   * if no inputs should be ignored for some output.
   */
  std::vector<std::vector<uint32_t>> ignore_inputs;

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Be very verbose (debug messages). */
  bool very_verbose{ false };
};

struct exact_linear_synthesis_stats
{
  /*! \brief Total time. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Time for SAT solving. */
  stopwatch<>::duration time_solving{ 0 };

  /*! \brief Prints report. */
  void report() const
  {
    fmt::print( "[i] total time   = {:>5.2f} secs\n", to_seconds( time_total ) );
    fmt::print( "[i] solving time = {:>5.2f} secs\n", to_seconds( time_solving ) );
  }
};

namespace detail
{

template<bill::solvers Solver>
struct exact_linear_synthesis_problem_network
{
  using problem_network_t = cnf_view<xag_network, false, Solver>;

  exact_linear_synthesis_problem_network( uint32_t num_steps, std::vector<std::vector<bool>> const& linear_matrix, std::vector<std::vector<uint32_t>> const& ignore_inputs, std::vector<std::pair<uint32_t, uint32_t>> const& trivial_pos, exact_linear_synthesis_params const& ps )
      : linear_matrix_( linear_matrix ),
        k_( num_steps ),
        n_( static_cast<uint32_t>( linear_matrix.front().size() ) ),
        m_( static_cast<uint32_t>( linear_matrix.size() ) ),
        bs_( k_ * n_ ),
        cs_( ( ( k_ - 1 ) * k_ ) / 2 ),
        fs_( k_ * m_ ),
        psis_( k_ * n_ ),
        phis_( k_ * n_ ),
        ignore_inputs_( ignore_inputs ),
        trivial_pos_( trivial_pos ),
        ps_( ps )
  {
    std::generate( bs_.begin(), bs_.end(), [&]() { return pntk_.create_pi(); } );
    std::generate( cs_.begin(), cs_.end(), [&]() { return pntk_.create_pi(); } );
    std::generate( fs_.begin(), fs_.end(), [&]() { return pntk_.create_pi(); } );

    ensure_row_size2();
    ensure_connectivity();
    ensure_outputs();
  }

  std::optional<bool> solve()
  {
    return pntk_.solve( ps_.conflict_limit );
  }

  template<class Ntk>
  Ntk extract_solution()
  {
    Ntk ntk;

    std::vector<signal<Ntk>> nodes( n_ );
    std::generate( nodes.begin(), nodes.end(), [&]() { return ntk.create_pi(); } );

    for ( auto i = 0u; i < k_; ++i )
    {
      std::array<signal<Ntk>, 2> children;
      auto it = children.begin();
      for ( auto j = 0u; j < n_ + i; ++j )
      {
        if ( pntk_.model_value( b_or_c( i, j ) ) )
        {
          *it++ = nodes[j];
        }
      }
      nodes.push_back( ntk.create_xor( children[0], children[1] ) );
    }

    auto it = trivial_pos_.begin();
    auto poctr = 0u;
    for ( auto l = 0u; l < m_; ++l )
    {
      while ( it != trivial_pos_.end() && it->first == poctr )
      {
        ntk.create_po( it->second == n_ ? ntk.get_constant( false ) : nodes[it->second] );
        poctr++;
        ++it;
      }

      for ( auto i = 0u; i < k_; ++i )
      {
        if ( pntk_.model_value( f( l, i ) ) )
        {
          ntk.create_po( nodes[n_ + i] );
          poctr++;
          break;
        }
      }
    }

    /* maybe some trivial POs are still left. */
    while ( it != trivial_pos_.end() && it->first == poctr )
    {
      ntk.create_po( it->second == n_ ? ntk.get_constant( false ) : nodes[it->second] );
      poctr++;
      ++it;
    }

    return ntk;
  }

  void debug_solution()
  {
    for ( auto i = 0u; i < k_; ++i )
    {
      fmt::print( i == 0 ? "B =" : "   " );
      for ( auto j = 0u; j < n_; ++j )
      {
        fmt::print( " {}", (int)pntk_.model_value( b( i, j ) ) );
      }
      fmt::print( i == 0 ? " C =" : "    " );
      for ( auto p = 0u; p < i; ++p )
      {
        fmt::print( " {}", (int)pntk_.model_value( c( i, p ) ) );
      }
      fmt::print( std::string( 2 * ( k_ - i ), ' ' ) );
      fmt::print( i == 0u ? " F =" : "    " );
      for ( auto l = 0u; l < m_; ++l )
      {
        fmt::print( " {}", (int)pntk_.model_value( f( l, i ) ) );
      }
      fmt::print( "\n" );
    }
  }

private:
  void ensure_row_size2()
  {
    for ( auto i = 0u; i < k_; ++i )
    {
      /* at least 2 */
      for ( auto cpl = 0u; cpl <= n_ + i; ++cpl )
      {
        std::vector<signal<problem_network_t>> lits( n_ + i );
        for ( auto j = 0u; j < n_ + i; ++j )
        {
          lits[j] = b_or_c( i, j ) ^ ( cpl == j );
        }
        pntk_.add_clause( lits );
      }

      /* at most 2 */
      for ( auto j = 2u; j < n_ + i; ++j )
      {
        for ( auto jj = 1u; jj < j; ++jj )
        {
          for ( auto jjj = 0u; jjj < jj; ++jjj )
          {
            pntk_.add_clause( !b_or_c( i, j ), !b_or_c( i, jj ), !b_or_c( i, jjj ) );
          }
        }
      }
    }
  }

  void ensure_connectivity()
  {
    // psi function
    for ( auto i = 0u; i < k_; ++i )
    {
      for ( auto j = 0u; j < n_; ++j )
      {
        std::vector<signal<problem_network_t>> xors( 1 + i );
        auto it = xors.begin();
        *it++ = b( i, j );
        for ( auto p = 0u; p < i; ++p )
        {
          *it++ = pntk_.create_and( c( i, p ), psi( j, p ) );
        }
        psi( j, i ) = pntk_.create_nary_xor( xors );
      }
    }

    for ( auto l = 0u; l < m_; ++l )
    {
      for ( auto i = 0u; i < k_; ++i )
      {
        std::vector<signal<problem_network_t>> ands( n_ );
        for ( auto j = 0u; j < n_; ++j )
        {
          ands[j] = pntk_.create_xnor( psi( j, i ), pntk_.get_constant( linear_matrix_[l][j] ) );
        }
        pntk_.add_clause( !f( l, i ), pntk_.create_nary_and( ands ) );
      }
    }

    // No two steps are the same
    for ( auto i = 0u; i < k_; ++i )
    {
      for ( auto p = 0u; p < i; ++p )
      {
        std::vector<signal<problem_network_t>> ors( n_ );
        for ( auto j = 0u; j < n_; ++j )
        {
          ors[j] = pntk_.create_xor( psi( j, p ), psi( j, i ) );
        }
        pntk_.add_clause( ors );
      }
    }

    if ( !ignore_inputs_.empty() || ps_.cancellation_free )
    {
      // phi function
      for ( auto i = 0u; i < k_; ++i )
      {
        for ( auto j = 0u; j < n_; ++j )
        {
          std::vector<signal<problem_network_t>> ors( 1 + i );
          auto it = ors.begin();
          *it++ = b( i, j );
          for ( auto p = 0u; p < i; ++p )
          {
            *it++ = pntk_.create_and( c( i, p ), phi( j, p ) );
          }
          phi( j, i ) = pntk_.create_nary_or( ors );
        }
      }

      // cancellation-free
      if ( ps_.cancellation_free )
      {
        for ( auto i = 0u; i < k_; ++i )
        {
          for ( auto j = 0u; j < n_; ++j )
          {
            pntk_.add_clause( !psi( j, i ), phi( j, i ) );
            pntk_.add_clause( psi( j, i ), !phi( j, i ) );
          }
        }
      }
    }

    // ignored inputs
    if ( !ignore_inputs_.empty() )
    {
      for ( auto l = 0u; l < m_; ++l )
      {
        for ( auto j : ignore_inputs_[l] )
        {
          for ( auto i = 0u; i < k_; ++i )
          {
            pntk_.add_clause( !f( l, i ), !phi( j, i ) );
          }
        }
      }
    }

    // at least 2 inputs in each compute form
    for ( auto i = 0u; i < k_; ++i )
    {
      /* at least 2 */
      for ( auto cpl = 0u; cpl <= n_; ++cpl )
      {
        std::vector<signal<problem_network_t>> lits( n_ );
        for ( auto j = 0u; j < n_; ++j )
        {
          lits[j] = psi( j, i ) ^ ( cpl == j );
        }
        pntk_.add_clause( lits );
      }
    }
  }

  void ensure_outputs()
  {
    // each output covers at least one row
    for ( auto l = 0u; l < m_; ++l )
    {
      std::vector<signal<problem_network_t>> lits( k_ );
      for ( auto i = 0u; i < k_; ++i )
      {
        lits[i] = f( l, i );
        for ( auto ii = i + 1; ii < k_; ++ii )
        {
          pntk_.add_clause( !f( l, i ), !f( l, ii ) );
        }
      }
      pntk_.add_clause( lits );
    }

    // at most one output (if no duplicates) per row
    // for ( auto i = 0u; i < k_; ++i )
    //{
    //  for ( auto l = 1u; l < m_; ++l )
    //  {
    //    for ( auto ll = 0u; ll < l; ++ll )
    //    {
    //      pntk_.add_clause( !f( l, i ), !f( ll, i ) );
    //    }
    //  }
    //}
  }

  // 0 <= i <= k - 1
  // 0 <= j <= n - 1
  const signal<problem_network_t>& b( uint32_t i, uint32_t j ) const
  {
    return bs_[i * n_ + j];
  }

  // 0 <= i <= k - 1
  // 0 <= p <= i - 1
  const signal<problem_network_t>& c( uint32_t i, uint32_t p ) const
  {
    return cs_[( ( ( i - 1 ) * i ) / 2 ) + p];
  }

  // 0 <= i <= k - 1
  // 0 <= j <= n + i - 1
  const signal<problem_network_t>& b_or_c( uint32_t i, uint32_t j ) const
  {
    return j < n_ ? b( i, j ) : c( i, j - n_ );
  }

  // 0 <= l <= m - 1
  // 0 <= i <= k - 1
  const signal<problem_network_t>& f( uint32_t l, uint32_t i ) const
  {
    return fs_[l * k_ + i];
  }

  // 0 <= j <= n - 1
  // 0 <= i <= k - 1
  signal<problem_network_t>& psi( uint32_t j, uint32_t i )
  {
    return psis_[i * n_ + j];
  }

  // 0 <= j <= n - 1
  // 0 <= i <= k - 1
  signal<problem_network_t>& phi( uint32_t j, uint32_t i )
  {
    return phis_[i * n_ + j];
  }

private:
  std::vector<std::vector<bool>> const& linear_matrix_;
  uint32_t k_;
  uint32_t n_;
  uint32_t m_;

  std::vector<signal<problem_network_t>> bs_;
  std::vector<signal<problem_network_t>> cs_;
  std::vector<signal<problem_network_t>> fs_;
  std::vector<signal<problem_network_t>> psis_;
  std::vector<signal<problem_network_t>> phis_;

  problem_network_t pntk_;
  std::vector<std::vector<uint32_t>> const& ignore_inputs_;
  std::vector<std::pair<uint32_t, uint32_t>> const& trivial_pos_;
  exact_linear_synthesis_params const& ps_;
};

template<class Ntk, bill::solvers Solver>
struct exact_linear_synthesis_impl
{
  exact_linear_synthesis_impl( std::vector<std::vector<bool>> const& linear_matrix, exact_linear_synthesis_params const& ps, exact_linear_synthesis_stats& st )
      : ps_( ps ),
        st_( st )
  {
    if ( ps_.very_verbose )
    {
      fmt::print( "[i] input matrix =\n" );
      debug_matrix( linear_matrix );
    }

    /* check whether ignore inputs matches matrix size */
    if ( !ps_.ignore_inputs.empty() && ps_.ignore_inputs.size() != linear_matrix.size() )
    {
      fmt::print( "[e] size of ignored inputs vector must match number of rows in linear matrix" );
      std::abort();
    }

    /* check matrix for trivial entries */
    for ( auto j = 0u; j < linear_matrix.size(); ++j )
    {
      const auto& row = linear_matrix[j];
      n_ = static_cast<uint32_t>( row.size() );

      auto cnt = 0u;
      auto idx = 0u;
      for ( auto i = 0u; i < row.size(); ++i )
      {
        if ( row[i] )
        {
          idx = i;
          if ( ++cnt == 2u )
          {
            break;
          }
        }
      }
      if ( cnt == 0u )
      {
        /* constant 0 is encoded as input n */
        trivial_pos_.emplace_back( j, n_ );
      }
      else if ( cnt == 1u )
      {
        trivial_pos_.emplace_back( j, idx );
      }
      else
      {
        linear_matrix_.push_back( row );
        if ( !ps_.ignore_inputs.empty() )
        {
          ignore_inputs_.push_back( ps_.ignore_inputs[j] );
        }
      }
    }

    m_ = static_cast<uint32_t>( linear_matrix_.size() );

    if ( ps_.very_verbose )
    {
      fmt::print( "[i] problem matrix =\n" );
      debug_matrix( linear_matrix_ );
      fmt::print( "\n[i] trivial POs =\n" );
      for ( auto const& [j, i] : trivial_pos_ )
      {
        if ( i == n_ )
        {
          fmt::print( "f{} = 0\n", j );
        }
        else
        {
          fmt::print( "f{} = x{}\n", j, i );
        }
      }
      if ( !ignore_inputs_.empty() )
      {
        fmt::print( "\n[i] ignored inputs =\n" );
        for ( auto j = 0u; j < ignore_inputs_.size(); ++j )
        {
          fmt::print( "for f{} ignore {{{}}}\n", j, fmt::join( ignore_inputs_[j], ", " ) );
        }
      }
    }
  }

  std::optional<Ntk> run()
  {
    if ( m_ == 0u )
    {
      Ntk ntk;

      std::vector<signal<Ntk>> nodes( n_ );
      std::generate( nodes.begin(), nodes.end(), [&]() { return ntk.create_pi(); } );

      for ( auto po : trivial_pos_ )
      {
        ntk.create_po( po.second == n_ ? ntk.get_constant( false ) : nodes[po.second] );
      }

      return ntk;
    }

    return ps_.upper_bound ? run_decreasing() : run_increasing();
  }

private:
  std::optional<Ntk> run_increasing()
  {
    auto k_ = m_;
    while ( true )
    {
      if ( ps_.verbose )
      {
        fmt::print( "[i] try to find a solution with {} steps, solving time so far = {:.2f} secs\n", k_, to_seconds( st_.time_solving ) );
      }

      exact_linear_synthesis_problem_network<Solver> pntk( k_, linear_matrix_, ignore_inputs_, trivial_pos_, ps_ );
      const auto res = call_with_stopwatch( st_.time_solving, [&]() { return pntk.solve(); } );
      if ( res && *res )
      {
        if ( ps_.very_verbose )
        {
          pntk.debug_solution();
        }
        return pntk.template extract_solution<Ntk>();
      }
      ++k_;
    }
  }

  std::optional<Ntk> run_decreasing()
  {
    std::optional<Ntk> best{};
    auto k_ = *ps_.upper_bound;
    while ( true )
    {
      if ( ps_.verbose )
      {
        fmt::print( "[i] try to find a solution with {} steps, solving time so far = {:.2f} secs\n", k_, to_seconds( st_.time_solving ) );
      }
      exact_linear_synthesis_problem_network<Solver> pntk( k_, linear_matrix_, ignore_inputs_, trivial_pos_, ps_ );
      const auto res = call_with_stopwatch( st_.time_solving, [&]() { return pntk.solve(); } );
      if ( res && *res )
      {
        if ( ps_.very_verbose )
        {
          pntk.debug_solution();
        }
        best = pntk.template extract_solution<Ntk>();
        --k_;
      }
      else /* if unsat or timeout */
      {
        return best;
      }
    }
  }

private:
  void debug_matrix( std::vector<std::vector<bool>> const& matrix ) const
  {
    for ( auto const& row : matrix )
    {
      for ( auto b : row )
      {
        fmt::print( "{}", b ? '1' : '0' );
      }
      fmt::print( "\n" );
    }
  }

private:
  uint32_t n_{};
  uint32_t m_{ 0u };
  std::vector<std::vector<bool>> linear_matrix_;
  std::vector<std::pair<uint32_t, uint32_t>> trivial_pos_;
  std::vector<std::vector<uint32_t>> ignore_inputs_;
  exact_linear_synthesis_params const& ps_;
  exact_linear_synthesis_stats& st_;
};

} // namespace detail

/*! \brief Extracts linear matrix from XOR-based XAG
 *
 * This algorithm can be used to extract the linear matrix represented by an
 * XAG that only contains XOR gates and no inverters at the outputs.  The matrix
 * can be passed as an argument to `exact_linear_synthesis`.
 */
template<class Ntk>
std::vector<std::vector<bool>> get_linear_matrix( Ntk const& ntk )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Ntk is not XAG-like" );

  detail::linear_matrix_simulator sim( ntk.num_pis() );
  return simulate<std::vector<bool>>( detail::linear_xag{ ntk }, sim );
}

/*! \brief Optimum linear circuit synthesis (based on SAT)
 *
 * This algorithm creates an XAG that is only composed of XOR gates.  It is
 * given as input a linear matrix, represented as vector of bool-vectors.  The
 * size of the outer vector corresponds to the number of outputs, the size of
 * each inner vector must be the same and corresponds to the number of inputs.
 *
 * Reference: [C. Fuhs and P. Schneider-Kamp, SAT (2010), page 71-84]
 */
template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::glucose_41>
std::optional<Ntk> exact_linear_synthesis( std::vector<std::vector<bool>> const& linear_matrix, exact_linear_synthesis_params const& ps = {}, exact_linear_synthesis_stats* pst = nullptr )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Ntk is not XAG-like" );

  exact_linear_synthesis_stats st;
  const auto xag = detail::exact_linear_synthesis_impl<Ntk, Solver>{ linear_matrix, ps, st }.run();

  if ( ps.verbose )
  {
    st.report();
  }
  if ( pst )
  {
    *pst = st;
  }
  return xag;
}

/*! \brief Optimum linear circuit resynthesis (based on SAT)
 *
 * This algorithm extracts the linear matrix from an XAG that only contains of
 * XOR gates and no inversions and returns a new XAG that has the optimum number
 * of XOR gates to represent the same function.
 *
 * Reference: [C. Fuhs and P. Schneider-Kamp, SAT (2010), page 71-84]
 */
template<class Ntk = xag_network, bill::solvers Solver = bill::solvers::glucose_41>
std::optional<Ntk> exact_linear_resynthesis( Ntk const& ntk, exact_linear_synthesis_params const& ps = {}, exact_linear_synthesis_stats* pst = nullptr )
{
  static_assert( std::is_same_v<typename Ntk::base_type, xag_network>, "Ntk is not XAG-like" );

  const auto linear_matrix = get_linear_matrix( ntk );
  return exact_linear_synthesis<Ntk, Solver>( linear_matrix, ps, pst );
}

} /* namespace mockturtle */
