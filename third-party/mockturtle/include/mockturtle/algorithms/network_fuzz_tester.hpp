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
  \file network_fuzz_tester.hpp
  \brief Network fuzz tester

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#include "../io/aiger_reader.hpp"
#include "../io/verilog_reader.hpp"
#include "../io/write_aiger.hpp"
#include "../io/write_verilog.hpp"
#include "../utils/stopwatch.hpp"

#include <array>
#include <cstdio>
#include <fmt/format.h>
#include <lorina/lorina.hpp>
#include <optional>

namespace mockturtle
{

/*! \brief Parameters for testcase_minimizer. */
struct fuzz_tester_params
{
  /*! \brief File format to be generated. */
  enum
  {
    verilog,
    aiger
  } file_format = verilog;

  /*! \brief Name of the generated testcase file. */
  std::string filename{ "fuzz_test.v" };

  /*! \brief Filename written out by the command (to do CEC with the input testcase). */
  std::optional<std::string> outputfile{ std::nullopt };

  /*! \brief Max number of networks to test: nullopt means infinity. */
  std::optional<uint64_t> num_iterations{ std::nullopt };

  /*! \brief Timeout in seconds: nullopt means infinity. */
  std::optional<uint64_t> timeout{ std::nullopt };
}; /* fuzz_tester_params */

/*! \brief Network fuzz tester
 *
 * Runs an algorithm on many small random logic networks.  Fuzz
 * testing is often useful to detect potential segmentation faults in
 * new implementations.  The generated benchmarks are saved first in a
 * file.  If a segmentation fault or unexpected behavior occurs, the
 * file can be used to reproduce and debug the problem.
 *
 * The entry function `run` generates different networks with the same
 * number of PIs and gates. The function `run_incremental`, on the other
 * hand, generates networks of increasing sizes. These functions return
 * true if it was terminated by an unexpected behavior, or return false
 * if it terminates normally after the specified number of iterations
 * without observing any defect.
 *
 * The script of algorithm(s) to be tested can be provided as (1) a
 * lambda function taking a network as input and returning a Boolean,
 * which is true if the algorithm behaves as expected; or (2) a lambda
 * function making a command string to be called, taking a filename string
 * as input (not supported on Windows platform). If the command exits
 * normally (with return value 0), CEC will be performed on the output
 * file; otherwise (segfault, assertion fail, or return value is not 0),
 * the fuzzer is terminated.
 *
  \verbatim embed:rst

   Usage

   .. code-block:: c++

     #include <mockturtle/algorithms/aig_resub.hpp>
     #include <mockturtle/algorithms/cleanup.hpp>
     #include <mockturtle/algorithms/network_fuzz_tester.hpp>
     #include <mockturtle/algorithms/resubstitution.hpp>
     #include <mockturtle/generators/random_logic_generator.hpp>

     auto opt = [&]( aig_network aig ) -> bool {
       resubstitution_params ps;
       resubstitution_stats st;
       aig_resubstitution( aig, ps, &st );
       aig = cleanup_dangling( aig );
       return true;
     };

     fuzz_tester_params ps;
     ps.num_iterations = 100;
     auto gen = default_random_aig_generator();
     network_fuzz_tester fuzzer( gen, ps );
     fuzzer.run( opt );
   \endverbatim
*/
template<typename Ntk, class NetworkGenerator>
class network_fuzz_tester
{
public:
  explicit network_fuzz_tester( NetworkGenerator& gen, fuzz_tester_params const ps = {} )
      : gen( gen ), ps( ps )
  {}

#ifndef _MSC_VER
  uint64_t run( std::function<std::string( std::string const& )>&& make_command )
  {
    return run( make_callback( make_command ) );
  }
#endif

  uint64_t run( std::function<bool( Ntk )>&& fn )
  {
    uint64_t counter{ 0 };
    stopwatch<>::duration time{ 0 };
    while ( ( !ps.num_iterations || counter < ps.num_iterations ) &&
            ( !ps.timeout || to_seconds( time ) < ps.timeout ) )
    {
      stopwatch t( time );
      auto ntk = gen.generate();
      fmt::print( "[i] create network #{}: I/O = {}/{} gates = {} nodes = {}, write into `{}`\n",
                  ++counter, ntk.num_pis(), ntk.num_pos(), ntk.num_gates(), ntk.size(), ps.filename );

      switch ( ps.file_format )
      {
      case fuzz_tester_params::verilog:
        write_verilog( ntk, ps.filename );
        break;
      case fuzz_tester_params::aiger:
        write_aiger( ntk, ps.filename );
        break;
      default:
        fmt::print( "[w] unsupported format\n" );
        return 0;
      }

      /* run optimization algorithm */
      if ( !fn( ntk ) )
      {
        return counter;
      }

      if ( ps.outputfile )
      {
        if ( !abc_cec() )
          return counter;
      }
    }
    return 0;
  }

private:
#ifndef _MSC_VER
  inline std::function<bool( Ntk )> make_callback( std::function<std::string( std::string const& )>& make_command )
  {
    std::function<bool( Ntk )> fn = [&]( Ntk ntk ) -> bool {
      (void)ntk;
      int status = std::system( make_command( ps.filename ).c_str() );
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
          {
            if ( ps.outputfile )
              return abc_cec();
            return true;
          }
          else if ( WEXITSTATUS( status ) == 1 || WEXITSTATUS( status ) == 134 ) // buggy or assertion fail
          {
            return false;
          }
          else
          {
            std::cout << "[e] Unexpected return value: " << WEXITSTATUS( status ) << '\n';
            return false;
          }
        }
        else // segfault
        {
          return false;
        }
      }
    };
    return fn;
  }
#endif

  inline bool abc_cec()
  {
    std::string command = fmt::format( "abc -q \"cec -n {} {}\"", ps.filename, *ps.outputfile );

    std::array<char, 128> buffer;
    std::string result;
#ifdef _MSC_VER
    std::unique_ptr<FILE, decltype( &_pclose )> pipe( _popen( command.c_str(), "r" ), _pclose );
#else
    std::unique_ptr<FILE, decltype( &pclose )> pipe( popen( command.c_str(), "r" ), pclose );
#endif
    if ( !pipe )
    {
      throw std::runtime_error( "popen() failed" );
    }
    while ( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr )
    {
      result += buffer.data();
    }

    /* search for one line which says "Networks are equivalent" and ignore all other debug output from ABC */
    std::stringstream ss( result );
    std::string line;
    while ( std::getline( ss, line, '\n' ) )
    {
      if ( line.size() >= 23u && line.substr( 0u, 23u ) == "Networks are equivalent" )
      {
        return true;
      }
    }

    fmt::print( "[e] Files are not equivalent\n" );
    return false;
  }

private:
  NetworkGenerator& gen;
  fuzz_tester_params const ps;
}; /* network_fuzz_tester */

} /* namespace mockturtle */