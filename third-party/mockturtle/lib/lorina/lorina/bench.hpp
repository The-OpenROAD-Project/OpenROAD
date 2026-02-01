/* lorina: C++ parsing library
 * Copyright (C) 2018-2021  EPFL
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
  \file bench.hpp
  \brief Implements bench parser

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "common.hpp"
#include "diagnostics.hpp"
#include "detail/utils.hpp"
#include <fstream>
#include <regex>
#include <iostream>

namespace lorina
{

/*! \brief A reader visitor for the BENCH format.
 *
 * Callbacks for the BENCH format.
 */
class bench_reader
{
public:
  /*! \brief Callback method for parsed input.
   *
   * \param name Name of the input
   */
  virtual void on_input( const std::string& name ) const
  {
    (void)name;
  }

  /*! \brief Callback method for parsed output.
   *
   * \param name Name of the output
   */
  virtual void on_output( const std::string& name ) const
  {
    (void)name;
  }

  /*! \brief Callback method for parsed input of DFF (before output is available).
   *
   * \param input Name of the input
   */
  virtual void on_dff_input( const std::string& input ) const
  {
    (void)input;
  }

  /*! \brief Callback method for parsed DFF (when input and output are available).
   *
   * \param input Name of the input
   * \param output Name of the output
   */
  virtual void on_dff( const std::string& input, const std::string& output ) const
  {
    (void)input;
    (void)output;
  }

  /*! \brief Callback method for parsed gate.
   *
   * \param inputs A list of inputs
   * \param output An output
   * \param type Either a name that specified the gate or a hexadecimal number that describes the logic function of the gate.
   */
  virtual void on_gate( const std::vector<std::string>& inputs, const std::string& output, const std::string& type ) const
  {
    (void)inputs;
    (void)output;
    (void)type;
  }

  /*! \brief Callback method for parsed gate assignment.
   *
   * \param input An input
   * \param output An output
   */
  virtual void on_assign( const std::string& input, const std::string& output ) const
  {
    (void)input;
    (void)output;
  }
}; /* bench_reader */

/*! \brief A BENCH reader for prettyprinting BENCH.
 *
 * Callbacks for prettyprinting of BENCH.
 *
 */
class bench_pretty_printer : public bench_reader
{
public:
  /*! \brief Constructor of the BENCH pretty printer.
   *
   * \param os Output stream
   */
  bench_pretty_printer( std::ostream& os = std::cout )
      : _os( os )
  {
  }

  virtual void on_input( const std::string& name ) const override
  {
    _os << fmt::format( "INPUT({0})", name ) << std::endl;
  }

  virtual void on_output( const std::string& name ) const override
  {
    _os << fmt::format( "OUTPUT({0})", name ) << std::endl;
  }

  virtual void on_gate( const std::vector<std::string>& inputs, const std::string& output, const std::string& type ) const override
  {
    _os << fmt::format( "{0} = {1}({2})", output, type, detail::join( inputs, "," ) ) << std::endl;
  }

  virtual void on_assign( const std::string& input, const std::string& output ) const override
  {
    _os << fmt::format( "{0} = {1}", output, input ) << std::endl;
  }

  std::ostream& _os; /*!< Output stream */
}; /* bench_pretty_printer */

namespace bench_regex
{
static std::regex input( R"(INPUT\((.*)\))" );
static std::regex output( R"(OUTPUT\((.*)\))" );
static std::regex gate( R"((.*)\s+=\s+(.*)\((.*)\))" );
static std::regex dff( R"((.*)\s+=\s+DFF\((.+)\))" );
static std::regex lut( R"((.*)\s+=\s+LUT\s+(.*)\((.*)\))" );
static std::regex gate_asgn( R"((.*)\s+=\s+(.*))" );
} // namespace bench_regex

/*! \brief Reader function for the BENCH format.
 *
 * Reads BENCH format from a stream and invokes a callback
 * method for each parsed primitive and each detected parse error.
 *
 * \param in Input stream
 * \param reader A BENCH reader with callback methods invoked for parsed primitives
 * \param diag An optional diagnostic engine with callback methods for parse errors
 * \return Success if parsing has been successful, or parse error if parsing has failed
 */
[[nodiscard]] inline return_code read_bench( std::istream& in, const bench_reader& reader, diagnostic_engine* diag = nullptr )
{
  return_code result = return_code::success;

  /* Function signature */
  using GateFn = detail::Func<
                   std::vector<std::string>,
                   std::string,
                   std::string
                 >;

  /* Parameter maps */
  using GateParamMap = detail::ParamPackMap<
                         /* Key */
                         std::string,
                         /* Params */
                         std::vector<std::string>,
                         std::string,
                         std::string
                       >;

  constexpr static const int GATE_FN{0};

  using ParamMaps = detail::ParamPackMapN<GateParamMap>;
  using PackedFns = detail::FuncPackN<GateFn>;

  detail::call_in_topological_order<PackedFns, ParamMaps>
    on_action( PackedFns( GateFn( [&]( std::vector<std::string> inputs,
                                       std::string output,
                                       std::string type )
                                  {
                                    if ( type == "" )
                                    {
                                      reader.on_assign( inputs.front(), output );
                                    }
                                    else if ( type == "DFF" )
                                    {
                                      reader.on_dff( inputs.front(), output );
                                    }
                                    else
                                    {
                                      reader.on_gate( inputs, output, type );
                                    }
                                  } ) ) );
  on_action.declare_known( "vdd" );
  on_action.declare_known( "gnd" );

  std::smatch m;
  detail::foreach_line_in_file_escape( in, [&]( const std::string& line ) {
    /* empty line or comment */
    if ( line.empty() || line[0] == '#' )
      return true;

    /* INPUT(<string>) */
    if ( std::regex_search( line, m, bench_regex::input ) )
    {
      const auto s = detail::trim_copy( m[1] );
      on_action.declare_known( s );
      reader.on_input( s );
      return true;
    }

    /* OUTPUT(<string>) */
    if ( std::regex_search( line, m, bench_regex::output ) )
    {
      reader.on_output( detail::trim_copy( m[1] ) );
      return true;
    }

    /* (<string> = LUT <HEX>(<list of strings>)) */
    if ( std::regex_search( line, m, bench_regex::lut ) )
    {
      const auto output = detail::trim_copy( m[1] );
      const auto type = detail::trim_copy( m[2] );
      const auto args = detail::trim_copy( m[3] );
      const auto inputs = detail::split( args, "," );
      on_action.call_deferred<GATE_FN>( inputs, { output }, std::make_tuple( inputs, output, type ) );
      return true;
    }

    /* (<string> = DFF(<string>)) */
    if ( std::regex_search( line, m, bench_regex::dff ) )
    {
      const auto output = detail::trim_copy( m[1] );
      const auto arg = detail::trim_copy( m[2] );
      reader.on_dff_input( output );
      on_action.declare_known( output );
      on_action.call_deferred<GATE_FN>( { arg }, { output }, std::make_tuple( std::vector<std::string>{ arg }, output, "DFF" ) );
      return true;
    }

    /* (<string> = <GATE_TYPE>(<list of strings>)) */
    if ( std::regex_search( line, m, bench_regex::gate ) )
    {
      const auto output = detail::trim_copy( m[1] );
      const auto type = detail::trim_copy( m[2] );
      const auto args = detail::trim_copy( m[3] );
      const auto inputs = detail::split( args, "," );
      on_action.call_deferred<GATE_FN>( inputs, { output }, std::make_tuple( inputs, output, type ) );
      return true;
    }

    /* (<string> = <string> */
    if ( std::regex_search( line, m, bench_regex::gate_asgn ) )
    {
      const auto output = detail::trim_copy( m[1] );
      const auto input = detail::trim_copy( m[2] );
      on_action.call_deferred<GATE_FN>( { input }, { output }, std::make_tuple( std::vector<std::string>{ input }, output, "" ) );
      return true;
    }

    if ( diag )
    {
      diag->report( diag_id::ERR_PARSE_LINE ).add_argument( line );
    }

    result = return_code::parse_error;
    return true;
  } );

  /* check dangling objects */
  const auto& deps = on_action.unresolved_dependencies();
  if ( deps.size() > 0 )
  {
    result = return_code::parse_error;
  }

  for ( const auto& r : deps )
  {
    if ( diag )
    {
      diag->report( diag_id::WRN_UNRESOLVED_DEPENDENCY )
        .add_argument( r.first )
        .add_argument( r.second );
    }
  }

  return result;
}

/*! \brief Reader function for BENCH format.
 *
 * Reads BENCH format from a file and invokes a callback
 * method for each parsed primitive and each detected parse error.
 *
 * \param filename Name of the file
 * \param reader A BENCH reader with callback methods invoked for parsed primitives
 * \param diag An optional diagnostic engine with callback methods for parse errors
 * \return Success if parsing has been successful, or parse error if parsing has failed
 */
[[nodiscard]] inline return_code read_bench( const std::string& filename, const bench_reader& reader, diagnostic_engine* diag = nullptr )
{
  std::ifstream in( detail::word_exp_filename( filename ), std::ifstream::in );
  if ( !in.is_open() )
  {
    if ( diag )
    {
      diag->report( diag_id::ERR_FILE_OPEN ).add_argument( filename );
    }
    return return_code::parse_error;
  }
  else
  {
    auto const ret = read_bench( in, reader, diag );
    in.close();
    return ret;
  }
}

} // namespace lorina
