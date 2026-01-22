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
  \file optimal_buffer_insertion.hpp
  \brief Global-optimal buffer insertion for AQFP using SMT

  \author Siang-Yun (Sonia) Lee
*/

// NOTE: This file is included inside the class `mockturtle::buffer_insertion`
// It should not be included anywhere else.

#pragma region Compute timeframe for SMT solving
  /*! \brief Compute the earliest and latest possible timeframe by eager ASAP and ALAP */
  uint32_t compute_timeframe( uint32_t max_depth )
  {
    // TODO: Consider max_depth % _ps.assume.num_phases == 0 constraint
    _timeframes.reset( std::make_pair( 0, 0 ) );
    uint32_t min_depth{ 0 };

    _ntk.incr_trav_id();
    _ntk.foreach_po( [&]( auto const& f ) {
      auto const no = _ntk.get_node( f );
      auto clevel = compute_levels_ASAP_eager( no ) + ( _ntk.fanout_size( no ) > 1 ? 1 : 0 );
      min_depth = std::max( min_depth, clevel );
    } );

    _ntk.incr_trav_id();
    _ntk.foreach_po( [&]( auto const& f ) {
      const auto n = _ntk.get_node( f );
      if ( !_ntk.is_constant( n ) && _ntk.visited( n ) != _ntk.trav_id() )
      {
        _timeframes[n].second = max_depth - ( _ntk.fanout_size( n ) > 1 ? 1 : 0 );
        compute_levels_ALAP_eager( n );
      }
    } );

    return min_depth;
  }

  uint32_t compute_levels_ASAP_eager( node const& n )
  {
    if ( _ntk.visited( n ) == _ntk.trav_id() )
    {
      return _timeframes[n].first;
    }
    _ntk.set_visited( n, _ntk.trav_id() );

    if ( _ntk.is_constant( n ) )
    {
      return _timeframes[n].first = 0;
    }
    if ( _ntk.is_pi( n ) )
    {
      return _timeframes[n].first = _ps.assume.ci_phases[0];
    }

    uint32_t level{ 0 };
    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        level = std::max( level, compute_levels_ASAP_eager( ni ) + ( _ntk.fanout_size( ni ) > 1 ? 1 : 0 ) );
      }
    } );

    return _timeframes[n].first = level + 1;
  }

  void compute_levels_ALAP_eager( node const& n )
  {
    _ntk.set_visited( n, _ntk.trav_id() );

    _ntk.foreach_fanin( n, [&]( auto const& fi ) {
      auto const ni = _ntk.get_node( fi );
      if ( !_ntk.is_constant( ni ) )
      {
        if ( _ps.assume.balance_cios && _ntk.is_pi( ni ) )
        {
          assert( _timeframes[n].second > _ps.assume.ci_phases[0] );
          _timeframes[ni].second = _ps.assume.ci_phases[0];
        }
        else
        {
          assert( _timeframes[n].second > num_splitter_levels( ni ) );
          auto fi_level = _timeframes[n].second - ( _ntk.fanout_size( ni ) > 1 ? 2 : 1 );
          if ( _ntk.visited( ni ) != _ntk.trav_id() || _timeframes[ni].second > fi_level )
          {
            _timeframes[ni].second = fi_level;
            compute_levels_ALAP_eager( ni );
          }
        }
      }
    } );
  }
#pragma

#if __GNUC__ == 7

void optimize_with_smt( std::string name = "" )
{}

#else

void optimize_with_smt( std::string name = "" )
{
  std::ofstream os( "model_" + name + ".smt2", std::ofstream::out );
  dump_smt_model( os );
  os.close();
  std::string command = "z3 -v:1 model_" + name + ".smt2 &> sol_" + name + ".txt";
  std::system( command.c_str() );
  parse_z3_result( "sol_" + name + ".txt" );
  return;
}

// ILP formulation
void dump_smt_model( std::ostream& os = std::cout )
{
  os << "(set-logic QF_LIA)\n";
  /* hard assumptions to bound the number of variables */
  uint32_t const max_depth = depth(); // + 3;
  uint32_t const max_relative_depth = max_depth;
  count_buffers();
  uint32_t const upper_bound = num_buffers();
  uint32_t const min_depth = compute_timeframe( max_depth );

  /* depth variable */
  if ( _ps.assume.balance_pos ) /* depth is a variable */
  {
    os << fmt::format( "(declare-const depth Int)\n" );
    assert( min_depth <= max_depth );
    os << fmt::format( "(assert (<= depth {}))\n", max_depth );
    os << fmt::format( "(assert (>= depth {}))\n", min_depth );
  }

  /* declare variables for the level of each node */
  std::vector<std::string> level_vars;
  if ( _ps.assume.branch_pis && !_ps.assume.balance_pis )
  {
    /* Only if we branch but not balance PIs, they are free variables as gates */
    _ntk.foreach_pi( [&]( auto const& n ) {
      level_vars.emplace_back( fmt::format( "l{}", n ) );
      os << fmt::format( "(declare-const l{} Int)\n", n );
      if ( _ps.assume.balance_pos )
        os << fmt::format( "(assert (<= l{} depth))\n", n );
      assert( _timeframes[n].second <= max_depth );
      os << fmt::format( "(assert (<= l{} {}))\n", n, _timeframes[n].second );
      os << fmt::format( "(assert (>= l{} 0))\n", n );
    } );
  }

  _ntk.foreach_gate( [&]( auto const& n ) {
    level_vars.emplace_back( fmt::format( "l{}", n ) );
    os << fmt::format( "(declare-const l{} Int)\n", n );
    if ( _ps.assume.balance_pos )
      os << fmt::format( "(assert (<= l{} depth))\n", n );
    assert( _timeframes[n].first >= 1 && _timeframes[n].second <= max_depth && _timeframes[n].first <= _timeframes[n].second );
    os << fmt::format( "(assert (>= l{} {}))\n", n, _timeframes[n].first );
    os << fmt::format( "(assert (<= l{} {}))\n", n, _timeframes[n].second );
  } );

  /* constraints for each gate's fanout */
  std::vector<std::string> bufs;
  if ( _ps.assume.branch_pis )
  {
    /* If PIs are balanced, they are always at level 0 so we don't have variables for them */
    if ( _ps.assume.balance_pis )
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        smt_constraints_balanced_pis( os, n, max_depth );
        bufs.emplace_back( fmt::format( "bufs{}", n ) );
      } );
    }
    else
    {
      _ntk.foreach_pi( [&]( auto const& n ) {
        smt_constraints( os, n, max_depth );
        bufs.emplace_back( fmt::format( "bufs{}", n ) );
      } );
    }
  }
  _ntk.foreach_gate( [&]( auto const& n ) {
    smt_constraints( os, n, max_depth );
    bufs.emplace_back( fmt::format( "bufs{}", n ) );
  } );

  os << "\n(declare-const total Int)\n";
  os << fmt::format( "(assert (= total (+ {})))\n", fmt::join( bufs, " " ) );
  os << fmt::format( "(assert (<= total {}))\n", upper_bound );
  os << "(minimize total)\n(check-sat)\n";
  if ( _ps.assume.balance_pos )
    os << "(get-value (total depth))\n";
  else
    os << "(get-value (total))\n";
  os << fmt::format( "(get-value ({}))\n(exit)\n", fmt::join( level_vars, " " ) );
}

void smt_constraints_balanced_pis( std::ostream& os, node const& n, uint32_t const& max_depth )
{
  os << fmt::format( "\n;constraints for node {}\n", n );
  os << fmt::format( "(declare-const bufs{} Int)\n", n );
  /* special cases */
  if ( _ntk.fanout_size( n ) == 0 ) /* dangling */
  {
    os << fmt::format( "(assert (= bufs{} 0))\n", n );
    return;
  }
  else if ( _ntk.fanout_size( n ) == 1 )
  {
    /* single fanout: only sequencing constraint */
    if ( _external_ref_count[n] > 0 )
    {
      if ( _ps.assume.balance_pos )
      {
        os << fmt::format( "(assert (= bufs{} depth))\n", n );
      }
      else
      {
        os << fmt::format( "(assert (= bufs{} 0))\n", n );
      }
    }
    else
    {
      node const& no = _fanouts[n].front().fanouts.front();
      os << fmt::format( "(assert (= bufs{} (- l{} 1)))\n", n, no );
    }
    return;
  }
  else if ( _ntk.fanout_size( n ) <= _ps.assume.splitter_capacity )
  {
    /* only one splitter is needed */

    /* sequencing */
    std::vector<node> fos;
    foreach_fanout( n, [&]( auto const& no ) {
      os << fmt::format( "(assert (>= l{} 2))\n", no );
      fos.emplace_back( no );
    } );

    if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
    {
      os << fmt::format( "(assert (= bufs{} depth))\n", n );
    }
    else
    {
      if ( fos.size() == 0 ) /* not balance PO; have multiple PO refs */
      {
        os << fmt::format( "(assert (= bufs{} 1))\n", n );
      }
      else if ( fos.size() == 1 ) /* not balance PO; have one gate fanout and PO ref(s) */
      {
        os << fmt::format( "(assert (= bufs{} (- l{} 1)))\n", n, fos[0] );
      }
      else if ( fos.size() >= 2 )
      {
        os << fmt::format( "(declare-const g{}maxfo Int)\n", n ); /* the max level of fanouts */

        /* bound it -- just to hint the solver; should not matter if we have optimization objective */
        if ( _ps.assume.balance_pos )
          os << fmt::format( "(assert (<= g{}maxfo depth))\n", n );
        else
          os << fmt::format( "(assert (<= g{}maxfo {}))\n", n, max_depth );

        /* taking the max (lower bounded by the max) */
        for ( auto const& no : fos )
          os << fmt::format( "(assert (>= g{}maxfo l{}))\n", n, no );

        os << fmt::format( "(assert (= bufs{} (- g{}maxfo 1)))\n", n, n );
      }
    }
    return;
  }

  /* sequencing & range constraints */
  foreach_fanout( n, [&]( auto const& no ) {
    os << fmt::format( "(assert (>= l{} 2))\n", no );
  } );

  /* PO branching */
  if ( _external_ref_count[n] > 0 )
  {
    os << fmt::format( "(assert (>= depth {}))\n", num_splitter_levels( n ) );
  }

  /* max possible relative depth */
  uint32_t highest_fanout = 0;
  if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
  {
    highest_fanout = max_depth + 1;
  }
  else
  {
    foreach_fanout( n, [&]( auto const& no ) {
      highest_fanout = std::max( highest_fanout, _timeframes[no].second );
    } );
  }
  uint32_t max_relative_depth = highest_fanout;

  uint32_t l = max_relative_depth;
  bool add_po_refs = false;
  if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
  {
    add_po_refs = true;
    os << fmt::format( "(assert (<= depth {}))\n", l - 1 );
  }

  /* initial case */
  os << fmt::format( "(declare-const g{}e{} Int)\n", n, l ); // edges at relative depth l
  os << fmt::format( "(assert (= g{}e{} (+", n, l );
  foreach_fanout( n, [&]( auto const& no ) {
    os << fmt::format( " (ite (= l{} {}) 1 0)", no, l );
  } );
  if ( add_po_refs )
    os << fmt::format( " (ite (= depth {}) {} 0)", l - 1, _external_ref_count[n] );
  os << ")))\n";

  /* general case */
  for ( --l; l > 0; --l )
  {
    os << fmt::format( "(declare-const g{}b{} Int)\n", n, l ); // buffers at relative depth l
    /* g{n}b{l} = ceil( g{n}e{l+1} / s_b ) --> s_b * ( g{n}b{l} - 1 ) < g{n}e{l+1} <= s_b * g{n}b{l} */
    os << fmt::format( "(assert (< (* {} (- g{}b{} 1)) g{}e{}))\n", _ps.assume.splitter_capacity, n, l, n, l + 1 );
    os << fmt::format( "(assert (<= g{}e{} (* {} g{}b{})))\n", n, l + 1, _ps.assume.splitter_capacity, n, l );

    os << fmt::format( "(declare-const g{}e{} Int)\n", n, l ); // edges at relative depth l
    os << fmt::format( "(assert (= g{}e{} (+", n, l );
    foreach_fanout( n, [&]( auto const& no ) {
      os << fmt::format( " (ite (= l{} {}) 1 0)", no, l );
    } );
    if ( add_po_refs )
      os << fmt::format( " (ite (= depth {}) {} 0)", l - 1, _external_ref_count[n] );
    os << fmt::format( " g{}b{})))\n", n, l );
  }

  /* end of loop */
  os << fmt::format( "(assert (= g{}e1 1))\n", n ); // legal
  os << fmt::format( "(assert (= bufs{} (+", n );
  for ( l = max_relative_depth - 1; l > 0; --l )
  {
    os << fmt::format( " g{}b{}", n, l );
  }
  os << ")))\n";
}

void smt_constraints( std::ostream& os, node const& n, uint32_t const& max_depth )
{
  os << fmt::format( "\n;constraints for node {}\n", n );
  os << fmt::format( "(declare-const bufs{} Int)\n", n );
  /* special cases */
  if ( _ntk.fanout_size( n ) == 0 ) /* dangling */
  {
    os << fmt::format( "(assert (= bufs{} 0))\n", n );
    return;
  }
  else if ( _ntk.fanout_size( n ) == 1 )
  {
    /* single fanout: only sequencing constraint */
    if ( _external_ref_count[n] > 0 )
    {
      if ( _ps.assume.balance_pos )
      {
        os << fmt::format( "(assert (= bufs{} (- depth l{})))\n", n, n );
      }
      else
      {
        os << fmt::format( "(assert (= bufs{} 0))\n", n );
      }
    }
    else
    {
      node const& no = _fanouts[n].front().fanouts.front();
      os << fmt::format( "(assert (> l{} l{}))\n", no, n );
      os << fmt::format( "(assert (= bufs{} (- l{} l{} 1)))\n", n, no, n );
    }
    return;
  }
  else if ( _ntk.fanout_size( n ) <= _ps.assume.splitter_capacity )
  {
    /* only one splitter is needed */

    /* sequencing */
    std::vector<node> fos;
    foreach_fanout( n, [&]( auto const& no ) {
      os << fmt::format( "(assert (>= l{} (+ l{} 2)))\n", no, n );
      fos.emplace_back( no );
    } );

    if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
    {
      os << fmt::format( "(assert (> depth l{}))\n", n );
      os << fmt::format( "(assert (= bufs{} (- depth l{})))\n", n, n );
    }
    else
    {
      if ( fos.size() == 0 ) /* not balance PO; have multiple PO refs */
      {
        os << fmt::format( "(assert (= bufs{} 1))\n", n );
      }
      else if ( fos.size() == 1 ) /* not balance PO; have one gate fanout and PO ref(s) */
      {
        os << fmt::format( "(assert (= bufs{} (- l{} l{} 1)))\n", n, fos[0], n );
      }
      else if ( fos.size() >= 2 )
      {
        os << fmt::format( "(declare-const g{}maxfo Int)\n", n ); /* the max level of fanouts */

        /* bound it -- just to hint the solver; should not matter if we have optimization objective */
        if ( _ps.assume.balance_pos )
          os << fmt::format( "(assert (<= g{}maxfo depth))\n", n );
        else
          os << fmt::format( "(assert (<= g{}maxfo {}))\n", n, max_depth );

        /* taking the max (lower bounded by the max) */
        for ( auto const& no : fos )
          os << fmt::format( "(assert (>= g{}maxfo l{}))\n", n, no );

        os << fmt::format( "(assert (= bufs{} (- g{}maxfo l{} 1)))\n", n, n, n );
      }
    }
    return;
  }

  /* max possible relative depth */
  uint32_t highest_fanout = 0;
  if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
  {
    highest_fanout = max_depth + 1;
  }
  else
  {
    foreach_fanout( n, [&]( auto const& no ) {
      highest_fanout = std::max( highest_fanout, _timeframes[no].second );
    } );
  }
  uint32_t max_relative_depth = highest_fanout - _timeframes[n].first;

  /* sequencing & range constraints */
  foreach_fanout( n, [&]( auto const& no ) {
    os << fmt::format( "(assert (<= l{} (+ l{} {})))\n", no, n, max_relative_depth );
    os << fmt::format( "(assert (>= l{} (+ l{} 2)))\n", no, n );
  } );

  /* PO branching */
  if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
  {
    os << fmt::format( "(assert (>= depth (+ l{} {})))\n", n, num_splitter_levels( n ) );
  }

  uint32_t l = max_relative_depth;
  bool add_po_refs = false;
  if ( _external_ref_count[n] > 0 && _ps.assume.balance_pos )
  {
    add_po_refs = true;
    os << fmt::format( "(assert (<= depth (+ l{} {})))\n", n, l - 1 );
  }

  /* initial case */
  os << fmt::format( "(declare-const g{}e{} Int)\n", n, l ); // edges at relative depth l
  os << fmt::format( "(assert (= g{}e{} (+", n, l );
  foreach_fanout( n, [&]( auto const& no ) {
    os << fmt::format( " (ite (= (- l{} l{}) {}) 1 0)", no, n, l );
  } );
  if ( add_po_refs )
    os << fmt::format( " (ite (= (+ l{} {}) depth) {} 0)", n, l - 1, _external_ref_count[n] );
  os << ")))\n";

  /* general case */
  for ( --l; l > 0; --l )
  {
    os << fmt::format( "(declare-const g{}b{} Int)\n", n, l ); // buffers at relative depth l
    /* g{n}b{l} = ceil( g{n}e{l+1} / s_b ) --> s_b * ( g{n}b{l} - 1 ) < g{n}e{l+1} <= s_b * g{n}b{l} */
    os << fmt::format( "(assert (< (* {} (- g{}b{} 1)) g{}e{}))\n", _ps.assume.splitter_capacity, n, l, n, l + 1 );
    os << fmt::format( "(assert (<= g{}e{} (* {} g{}b{})))\n", n, l + 1, _ps.assume.splitter_capacity, n, l );

    os << fmt::format( "(declare-const g{}e{} Int)\n", n, l ); // edges at relative depth l
    os << fmt::format( "(assert (= g{}e{} (+", n, l );
    foreach_fanout( n, [&]( auto const& no ) {
      os << fmt::format( " (ite (= (- l{} l{}) {}) 1 0)", no, n, l );
    } );
    if ( add_po_refs )
      os << fmt::format( " (ite (= (+ l{} {}) depth) {} 0)", n, l - 1, _external_ref_count[n] );
    os << fmt::format( " g{}b{})))\n", n, l );
  }

  /* end of loop */
  os << fmt::format( "(assert (= g{}e1 1))\n", n ); // legal
  os << fmt::format( "(assert (= bufs{} (+", n );
  for ( l = max_relative_depth - 1; l > 0; --l )
  {
    os << fmt::format( " g{}b{}", n, l );
  }
  os << ")))\n";
}

template<class Fn>
void foreach_fanout( node const& n, Fn&& fn )
{
  for ( auto it1 = _fanouts[n].begin(); it1 != _fanouts[n].end(); ++it1 )
  {
    for ( auto it2 = it1->fanouts.begin(); it2 != it1->fanouts.end(); ++it2 )
    {
      fn( *it2 );
    }
  }
}

void parse_z3_result( std::string filename )
{
  std::ifstream fin( filename, std::ifstream::in );
  assert( fin.is_open() );

  /* parsing */
  std::string line;
  do
  {
    std::getline( fin, line ); /* first line: "sat" */
  } while ( line != "sat" );
  assert( line == "sat" );
  std::getline( fin, line ); /* second line: "((total <>)" */
  uint32_t total = std::stoi( line.substr( 8, line.find_first_of( ')' ) - 8 ) );
  std::cout << "[i] total = " << total;
  if ( _ps.assume.balance_pos )
  {
    std::getline( fin, line ); /* third line: " (depth <>))" */
    uint32_t depth = std::stoi( line.substr( 8, line.find_first_of( ')' ) - 8 ) );
    std::cout << ", depth = " << depth;
  }
  std::cout << "\n";

  while ( std::getline( fin, line ) ) /* remaining lines: "((l<> <>)" or " (l<> <>)" or " (l<> <>))" */
  {
    line = line.substr( line.find( 'l' ) + 1 );
    uint32_t n = std::stoi( line.substr( 0, line.find( ' ' ) ) );
    line = line.substr( line.find( ' ' ) + 1 );
    _levels[n] = std::stoi( line.substr( 0, line.find_first_of( ')' ) ) );
  }
  adjust_depth();
}

#endif