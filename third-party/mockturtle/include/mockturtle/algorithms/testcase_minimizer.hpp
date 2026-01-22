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
  \file testcase_minimizer.hpp
  \brief Minimize testcase for debugging

  \author Siang-Yun (Sonia) Lee
*/

#include "../io/aiger_reader.hpp"
#include "../io/verilog_reader.hpp"
#include "../io/write_aiger.hpp"
#include "../io/write_verilog.hpp"
#include "../networks/aig.hpp"
#include "../utils/debugging_utils.hpp"
#include "../views/color_view.hpp"
#include "cleanup.hpp"

#include <fmt/format.h>
#include <lorina/lorina.hpp>
#include <optional>
#include <utility>

namespace mockturtle
{

/*! \brief Parameters for testcase_minimizer. */
struct testcase_minimizer_params
{
  /*! \brief File format of the testcase. */
  enum
  {
    verilog,
    aiger
  } file_format = verilog;

  /*! \brief Path to find the initial test case and to store the minimized test case. */
  std::string path{ "." };

  /*! \brief File name of the initial test case (excluding extension). */
  std::string init_case{ "testcase" };

  /*! \brief File name of the minimized test case (excluding extension). */
  std::string minimized_case{ "minimized" };

  /*! \brief Maximum number of iterations in total. nullopt = infinity */
  std::optional<uint32_t> num_iterations{ std::nullopt };

  /*! \brief Step into the next stage if nothing works for this number of iterations. */
  std::optional<uint32_t> num_iterations_stage{ std::nullopt };

  /*! \brief Be verbose. */
  bool verbose{ false };

  /*! \brief Seed of the random generator. */
  uint64_t seed{ 0xcafeaffe };
}; /* testcase_minimizer_params */

/*! \brief Debugging testcase minimizer
 *
 * Given a (sequence of) algorithm(s) and a testcase that is
 * known to trigger a bug in the algorithm(s), this utility
 * minimizes the testcase by trying to remove parts of the network
 * with an increasing granularity in each stage. Only changes after
 * which the bug is still triggered are kept; otherwise, the change
 * is reverted.
 *
 * The script of algorithm(s) to be run can be provided as (1) a
 * lambda function taking a network as input and returning a Boolean,
 * which is true if the script runs normally and is false otherwise
 * (i.e. the buggy behavior is observed); or (2) (not supported on
 * Windows platform) a lambda function making a command string to be
 * called, taking a filename string as input. The command should return
 * 0 if the program runs normally, return 1 if the concerned buggy
 * behavior is observed, and return other values if the input network
 * is not valid (thus the latest change will not be kept). If the
 * command segfaults or an assertion fails, it is treated as observing
 * the buggy behavior.
 *
 *
 *
  \verbatim embed:rst

   Usage

   .. code-block:: c++

     auto opt = []( mig_network ntk ) -> bool {
       direct_resynthesis<mig_network> resyn;
       refactoring( ntk, resyn );
       return network_is_acyclic( ntk );
     };

     auto make_command = []( std::string const& filename ) -> std::string {
       return "./abc -c \"read " + filename + "; drw\"";
     };

     testcase_minimizer_params ps;
     ps.path = "."; // current directory
     testcase_minimizer<mig_network> minimizer( ps );
     minimizer.run( opt ); // debug algorithms implemented in mockturtle
     minimizer.run( make_command ); // debug external scripts
   \endverbatim
*/
template<typename Ntk>
class testcase_minimizer
{
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  explicit testcase_minimizer( testcase_minimizer_params const ps = {} )
      : ps( ps )
  {
    // assert( ps.file_format != testcase_minimizer_params::aiger || std::is_same_v<typename Ntk::base_type, aig_network> );
    switch ( ps.file_format )
    {
    case testcase_minimizer_params::verilog:
      file_extension = ".v";
      break;
    case testcase_minimizer_params::aiger:
      file_extension = ".aig";
      break;
    default:
      fmt::print( "[e] Unsupported format\n" );
    }
    std::srand( ps.seed );
  }

  void run( std::function<bool( Ntk )> const& fn )
  {
    if ( !read_initial_testcase() )
    {
      return;
    }

    if ( !test( fn ) )
    {
      fmt::print( "[e] The initial test case does not trigger the buggy behavior\n" );
      return;
    }

    uint32_t counter{ 0 };
    while ( !ps.num_iterations || counter++ < ps.num_iterations )
    {
      ntk_backup2 = cleanup_dangling( ntk );
      if ( !reduce() )
      {
        write_testcase( ps.minimized_case );
        break;
      }
      if ( ntk.num_gates() == 0 )
      {
        ++stage_counter;
        ntk = ntk_backup2;
        continue;
      }

      if ( test( fn ) )
      {
        fmt::print( "[i] Testcase with I/O = {}/{} gates = {} triggers the buggy behavior\n", ntk.num_pis(), ntk.num_pos(), ntk.num_gates() );
        write_testcase( ps.minimized_case );
        stage_counter = 0;
        sampled.clear();
      }
      else
      {
        ++stage_counter;
        ntk = ntk_backup2;
      }
    }

    if ( init_PIs != ntk.num_pis() || init_POs != ntk.num_pos() || init_gates != ntk.num_gates() )
    {
      fmt::print( "[i] Minimized the testcase from I/O = {}/{} gates = {}\n", init_PIs, init_POs, init_gates );
      fmt::print( "                             to I/O = {}/{} gates = {}\n", ntk.num_pis(), ntk.num_pos(), ntk.num_gates() );
    }
  }

#ifndef _MSC_VER
  void run( std::function<std::string( std::string const& )> const& make_command )
  {
    if ( !read_initial_testcase() )
    {
      return;
    }

    if ( !test( make_command, ps.init_case ) )
    {
      fmt::print( "[e] The initial test case does not trigger the buggy behavior\n" );
      return;
    }

    uint32_t counter{ 0 };
    while ( !ps.num_iterations || counter++ < ps.num_iterations )
    {
      ntk_backup2 = cleanup_dangling( ntk );
      if ( !reduce() )
      {
        break;
      }
      if ( ntk.num_gates() == 0 )
      {
        ++stage_counter;
        ntk = ntk_backup2;
        continue;
      }

      write_testcase( "tmp" );

      if ( test( make_command, "tmp" ) )
      {
        fmt::print( "[i] Testcase with I/O = {}/{} gates = {} triggers the buggy behavior\n", ntk.num_pis(), ntk.num_pos(), ntk.num_gates() );
        write_testcase( ps.minimized_case );
        stage_counter = 0;
        sampled.clear();
      }
      else
      {
        ++stage_counter;
        ntk = ntk_backup2;
      }
    }

    if ( init_PIs != ntk.num_pis() || init_POs != ntk.num_pos() || init_gates != ntk.num_gates() )
    {
      fmt::print( "[i] Minimized the testcase from I/O = {}/{} gates = {}\n", init_PIs, init_POs, init_gates );
      fmt::print( "                             to I/O = {}/{} gates = {}\n", ntk.num_pis(), ntk.num_pos(), ntk.num_gates() );
    }
  }
#endif

private:
  bool read_initial_testcase()
  {
    switch ( ps.file_format )
    {
    case testcase_minimizer_params::verilog:
      if ( lorina::read_verilog( ps.path + "/" + ps.init_case + file_extension, verilog_reader( ntk ) ) != lorina::return_code::success )
      {
        fmt::print( "[e] Could not read test case `{}`\n", ps.path + "/" + ps.init_case + file_extension );
        return false;
      }
      break;
    case testcase_minimizer_params::aiger:
      if ( lorina::read_aiger( ps.path + "/" + ps.init_case + file_extension, aiger_reader( ntk ) ) != lorina::return_code::success )
      {
        fmt::print( "[e] Could not read test case `{}`\n", ps.path + "/" + ps.init_case + file_extension );
        return false;
      }
      break;
    default:
      fmt::print( "[e] Unsupported format\n" );
      return false;
    }
    init_PIs = ntk.num_pis();
    init_POs = ntk.num_pos();
    init_gates = ntk.num_gates();
    return true;
  }

  void write_testcase( std::string const& filename )
  {
    switch ( ps.file_format )
    {
    case testcase_minimizer_params::verilog:
      write_verilog( ntk, ps.path + "/" + filename + file_extension );
      break;
    case testcase_minimizer_params::aiger:
      write_aiger( ntk, ps.path + "/" + filename + file_extension );
      break;
    default:
      fmt::print( "[e] Unsupported format\n" );
    }
  }

  bool test( std::function<bool( Ntk )> const& fn )
  {
    ntk_backup = cleanup_dangling( ntk );
    bool res = fn( ntk );
    ntk = ntk_backup;
    was_FIT = !res;
    return was_FIT;
  }

#ifndef _MSC_VER
  bool test( std::function<std::string( std::string const& )> const& make_command, std::string const& filename )
  {
    was_FIT = test_inner( make_command, filename );
    return was_FIT;
  }

  bool test_inner( std::function<std::string( std::string const& )> const& make_command, std::string const& filename )
  {
    std::string const command = make_command( ps.path + "/" + filename + file_extension );
    int status = std::system( command.c_str() );
    if ( status < 0 )
    {
      std::cout << "[e] Unexpected error when calling command: " << strerror( errno ) << '\n';
      return false;
    }
    else
    {
      if ( WIFEXITED( status ) )
      {
        if ( WEXITSTATUS( status ) == 0 ) // normal
          return false;
        else if ( WEXITSTATUS( status ) == 1 ) // buggy
          return true;
        else if ( WEXITSTATUS( status ) == 134 ) // assertion fail
          return true;
        else
        {
          std::cout << "[e] Unexpected return value: " << WEXITSTATUS( status ) << '\n';
          return false;
        }
      }
      else // segfault
      {
        return true;
      }
    }
  }
#endif

  bool reduce()
  {
    pos_to_remove = ntk.num_pos() >> 3; // 1/8

    if ( ( ps.num_iterations_stage && stage_counter >= *ps.num_iterations_stage ) ||
         ( reducing_stage == many_pos && pos_to_remove > ntk.num_pos() - sampled.size() ) ||
         ( reducing_stage == many_pos && pos_to_remove < 2 ) ||
         ( reducing_stage == po && ntk.num_pos() == 1 ) ||
         ( reducing_stage == po && sampled.size() == ntk.num_pos() ) ||
         ( reducing_stage == pi && ntk.num_pis() > ntk.num_pos() ) ||
         ( reducing_stage == pi && sampled.size() == ntk.num_pis() ) ||
         ( reducing_stage == const_gate && sampled.size() == ntk.num_gates() ) ||
         ( reducing_stage == mffc && sampled.size() == ntk.num_gates() ) ||
         ( reducing_stage == half_mffc && sampled.size() == ntk.num_gates() ) ||
         ( reducing_stage == simplify_tfo && sampled.size() == ntk.num_gates() ) ||
         ( reducing_stage == single_gate && sampled.size() == ntk.num_gates() ) )
    {
      reducing_stage = static_cast<stages>( static_cast<int>( reducing_stage ) + 1 );
      stage_counter = 0;
      sampled.clear();
    }

    switch ( reducing_stage )
    {
    case many_pos:
    {
      assert( pos_to_remove <= ntk.num_pos() - sampled.size() );
      if ( ps.verbose )
        fmt::print( "[i] Remove {} POs\n", pos_to_remove );
      for ( auto i = 0u; i < pos_to_remove; ++i )
      {
        auto const& [ith_po, n] = get_random_po();
        if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
          continue;
        ntk.substitute_node( n, ntk.get_constant( false ) );
      }
      break;
    }
    case po:
    {
      auto const& [ith_po, n] = get_random_po();
      if ( ps.verbose )
        fmt::print( "[i] Remove {}-th PO (node {})\n", ith_po, n );
      ntk.substitute_node( n, ntk.get_constant( false ) );
      break;
    }
    case pi:
    {
      const node n = get_random_pi();
      if ( ps.verbose )
        fmt::print( "[i] Remove PI {}\n", n );
      ntk.substitute_node( n, ntk.get_constant( false ) );
      break;
    }
    case const_gate:
    {
      node const& n = get_random_gate();
      if ( ps.verbose )
        fmt::print( "[i] Substitute gate {} with const0\n", n );
      ntk.substitute_node( n, ntk.get_constant( false ) );
      break;
    }
    case mffc:
    {
      node const& n = get_random_gate();
      signal fi = ntk.create_pi();
      if ( ps.verbose )
        fmt::print( "[i] Substitute gate {} with a new PI {}\n", n, ntk.get_node( fi ) );
      ntk.substitute_node( n, fi );
      break;
    }
    case half_mffc:
    {
      node const& n = get_random_gate();
      signal fi;
      uint32_t const ith_fanin = std::rand() % ntk.fanin_size( n );
      ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
        if ( i == ith_fanin )
          fi = f;
      } );
      if ( ps.verbose )
        fmt::print( "[i] Substitute gate {} with its {}-th fanin {}{}\n", n, ith_fanin, ntk.is_complemented( fi ) ? "!" : "", ntk.get_node( fi ) );
      ntk.substitute_node( n, fi );
      assert( network_is_acyclic( color_view{ ntk } ) );
      break;
    }
    case simplify_tfo:
    {
      node const& n = get_random_gate();
      if ( ps.verbose )
        fmt::print( "[i] Simplify TFO of gate {} with const0 but keep all its fanins\n", n );
      ntk.foreach_fanin( n, [&]( auto f ) {
        ntk.create_po( f );
      } );
      ntk.substitute_node( n, ntk.get_constant( false ) );
      break;
    }
    case single_gate:
    {
      node const& n = get_random_gate();
      if ( ps.verbose )
        fmt::print( "[i] Remove a single gate {}\n", n );
      ntk.foreach_fanin( n, [&]( auto f ) {
        ntk.create_po( f );
      } );
      signal fi = ntk.create_pi();
      ntk.substitute_node( n, fi );
      break;
    }
    default:
    {
      ntk = cleanup_dangling( ntk, true, true );
      return false; // all stages done, nothing to reduce
    }
    }

    ntk = cleanup_dangling( ntk, true, true );
    return true;
  }

  std::pair<uint32_t, node> get_random_po()
  {
    while ( true )
    {
      uint32_t const ith_po = std::rand() % ntk.num_pos();
      const node n = ntk.get_node( ntk.po_at( ith_po ) );
      if ( sampled.find( ith_po ) == sampled.end() )
      {
        sampled.insert( ith_po );
        return std::make_pair( ith_po, n );
      }
    }
  }

  node get_random_pi()
  {
    while ( true )
    {
      uint32_t const ith_pi = std::rand() % ntk.num_pis() + 1;
      if ( sampled.find( ith_pi ) == sampled.end() )
      {
        sampled.insert( ith_pi );
        return ntk.index_to_node( ith_pi );
      }
    }
  }

  node get_random_gate()
  {
    while ( true )
    {
      uint32_t const node_index = ( std::rand() % ntk.num_gates() ) + ntk.num_pis() + 1;
      node n = ntk.index_to_node( node_index );
      if ( sampled.find( node_index ) == sampled.end() )
      {
        sampled.insert( node_index );
        assert( !ntk.is_dead( n ) && !ntk.is_pi( n ) );
        return n;
      }
    }
  }

  std::pair<node, node> get_random_gate_with_gate_fanin()
  {
    while ( true )
    {
      node const& n = get_random_gate();
      node ni;
      bool has_gate_fanin = false;
      ntk.foreach_fanin( n, [&]( auto const& f ) {
        ni = ntk.get_node( f );
        if ( !ntk.is_pi( ni ) && !ntk.is_constant( ni ) )
        {
          has_gate_fanin = true;
          return false; // break
        }
        return true; // next fanin
      } );
      if ( has_gate_fanin )
      {
        return std::make_pair( n, ni );
      }
    }
  }

private:
  testcase_minimizer_params const ps;
  std::string file_extension;
  Ntk ntk, ntk_backup, ntk_backup2;

  enum stages : int
  {
    pi = 1, // remove a PI (substitute with const0)
    many_pos,
    po,           // remove a PO (substitute with const0)
    const_gate,   // substitute a gate with const0
    simplify_tfo, // create PO for all of a gate's fanins, then substitute it with const0
    mffc,         // substitute a gate with a new PI
    half_mffc,    // substitute a gate with one of its fanin
    single_gate   // remove a single gate by creating POs for its fanins and substitute it with a new PI
  } reducing_stage{ pi };
  uint32_t stage_counter{ 0u };
  uint32_t pos_to_remove{ 1000 };
  bool was_FIT{ true };
  std::set<uint32_t> sampled;

  uint32_t init_PIs, init_POs, init_gates;
}; /* testcase_minimizer */

} /* namespace mockturtle */