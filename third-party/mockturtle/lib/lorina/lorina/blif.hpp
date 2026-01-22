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
  \file blif.hpp
  \brief Implements BLIF parser

  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "common.hpp"
#include "diagnostics.hpp"
#include "detail/utils.hpp"
#include <regex>
#include <iostream>
#include <optional>

namespace lorina
{

/*! \brief A reader visitor for the BLIF format.
 *
 * Callbacks for the BLIF (Berkeley Logic Interchange Format) format.
 */
class blif_reader
{
public:
  /*! \brief Type of the output cover as truth table. */
  using output_cover_t = std::vector<std::pair<std::string, std::string>>;

  /*! Latch input values */
  enum latch_init_value
  {
    ZERO = 0 /*!< Initialized with 0 */
  , ONE /*!< Initialized with 1 */
  , NONDETERMINISTIC /*!< Not initialized (non-deterministic value) */
  , UNKNOWN
  };

  enum latch_type
  {
    FALLING = 0
  , RISING
  , ACTIVE_HIGH
  , ACTIVE_LOW
  , ASYNC
  , NONE
  };

public:
  static std::optional<latch_init_value> latch_init_value_from_string( std::string const& s )
  {
    if ( s == "0" )
    {
      return blif_reader::latch_init_value::ZERO;
    }
    else if ( s == "1" )
    {
      return blif_reader::latch_init_value::ONE;
    }
    else if ( s == "2" )
    {
      return blif_reader::latch_init_value::NONDETERMINISTIC;
    }
    else
    {
      return blif_reader::latch_init_value::UNKNOWN;
    }
  }

  static std::optional<latch_type> latch_type_from_string( std::string const& s )
  {
    if ( s == "fe" )
    {
      return blif_reader::latch_type::FALLING;
    }
    else if ( s == "re" )
    {
      return blif_reader::latch_type::RISING;
    }
    else if ( s == "ah" )
    {
      return blif_reader::latch_type::ACTIVE_HIGH;
    }
    else if ( s == "al" )
    {
      return blif_reader::latch_type::ACTIVE_LOW;
    }
    else
    {
      return blif_reader::latch_type::ASYNC;
    }
  }

public:
  /*! \brief Callback method for parsed model.
   *
   * \param model_name Name of the model
   */
  virtual void on_model( const std::string& model_name ) const
  {
    (void)model_name;
  }

  /*! \brief Callback method for parsed input.
   *
   * \param name Input name
   */
  virtual void on_input( const std::string& name ) const
  {
    (void)name;
  }

  /*! \brief Callback method for parsed output.
   *
   * \param name Output name
   */
  virtual void on_output( const std::string& name ) const
  {
    (void)name;
  }

  /*! \brief Callback method for parsed latch.
   *
   * \param input Input name
   * \param output Output name
   * \param type Optional type of the latch
   * \param control Optional name of control signal
   * \param reset Optional initial value of the latch
   */
  virtual void on_latch( const std::string& input, const std::string& output, const std::optional<latch_type>& type, const std::optional<std::string>& control, const std::optional<latch_init_value>& init_value ) const
  {
    (void)input;
    (void)output;
    (void)type;
    (void)control;
    (void)init_value;
  }

  /*! \brief Callback method for parsed gate.
   *
   * \param inputs A list of input names
   * \param output Name of output of the gate
   * \param cover N-input, 1-output PLA description of the logic function corresponding to the logic gate
   */
  virtual void on_gate( const std::vector<std::string>& inputs, const std::string& output, const output_cover_t& cover ) const
  {
    (void)inputs;
    (void)output;
    (void)cover;
  }

  /*! \brief Callback method for parsed end.
   *
   */
  virtual void on_end() const {}

  /*! \brief Callback method for parsed comment.
   *
   * \param comment Comment
   */
  virtual void on_comment( const std::string& comment ) const
  {
    (void)comment;
  }
}; /* blif_reader */

/*! \brief A BLIF reader for prettyprinting BLIF.
 *
 * Callbacks for prettyprinting of BLIF.
 *
 */
class blif_pretty_printer : public blif_reader
{
public:
  /*! \brief Constructor of the BLIF pretty printer.
   *
   * \param os Output stream
   */
  blif_pretty_printer( std::ostream& os = std::cout )
      : _os( os )
  {
  }

  virtual void on_model( const std::string& model_name ) const
  {
    _os << ".model " << model_name << std::endl;
  }

  virtual void on_input( const std::string& name ) const
  {
    if ( first_input ) _os << std::endl << ".inputs ";
    _os << name << " ";
    first_input = false;
  }

  virtual void on_output( const std::string& name ) const
  {
    if ( first_output ) _os << std::endl << ".output ";
    _os << name << " ";
    first_output = false;
  }

  virtual void on_latch( const std::string& input, const std::string& output, const std::optional<latch_type>& type, const std::optional<std::string>& control, const std::optional<latch_init_value>& init_value ) const
  {
    _os << fmt::format( "\n.latch {0} {1} {2} {3} {4}", input, output,
                        ( type ? *type : latch_type::NONE ), ( control ? *control : "" ), ( init_value ? *init_value : latch_init_value::UNKNOWN ) ) << std::endl;
  }

  virtual void on_gate( const std::vector<std::string>& inputs, const std::string& output, const output_cover_t& cover ) const
  {
    _os << std::endl << fmt::format( ".names {0} {1}", detail::join( inputs, "," ), output ) << std::endl;
    for ( const auto& c : cover )
    {
      _os << c.first << ' ' << c.second << std::endl;
    }
  }

  virtual void on_end() const
  {
    _os << std::endl << ".end" << std::endl;
  }

  virtual void on_comment( const std::string& comment ) const
  {
    _os << std::endl << "# " << comment << std::endl;
  }

  mutable bool first_input = true; /*!< Predicate that is true until the first input was parsed */
  mutable bool first_output = true; /*!< Predicate that is true until the first output was parsed */
  std::ostream& _os; /*!< Output stream */
}; /* blif_pretty_printer */

namespace blif_regex
{
static std::regex model( R"(.model\s+(.*))" );
static std::regex names( R"(.names\s+(.*))" );
static std::regex line_of_truthtable( R"(([01\-]*)\s*([01\-]))" );
static std::regex end( R"(.end)" );
} // namespace blif_regex

/*! \brief Reader function for the BLIF format.
 *
 * Reads BLIF format from a stream and invokes a callback
 * method for each parsed primitive and each detected parse error.
 *
 * \param in Input stream
 * \param reader A BLIF reader with callback methods invoked for parsed primitives
 * \param diag An optional diagnostic engine with callback methods for parse errors
 * \return Success if parsing has been successful, or parse error if parsing has failed
 */
[[nodiscard]] inline return_code read_blif( std::istream& in, const blif_reader& reader, diagnostic_engine* diag = nullptr )
{
  return_code result = return_code::success;

  /* Function signature */
  using GateFn = detail::Func<
                   std::vector<std::string>,
                   std::string,
                   std::vector<std::pair<std::string, std::string>>
                 >;

  /* Parameter maps */
  using GateParamMap = detail::ParamPackMap<
                         /* Key */
                         std::string,
                         /* Params */
                         std::vector<std::string>,
                         std::string,
                         std::vector<std::pair<std::string, std::string>>
                       >;

  constexpr static const int GATE_FN{0};

  using ParamMaps = detail::ParamPackMapN<GateParamMap>;
  using PackedFns = detail::FuncPackN<GateFn>;

  detail::call_in_topological_order<PackedFns, ParamMaps>
    on_action( PackedFns( GateFn( [&]( std::vector<std::string> inputs,
				       std::string output,
				       std::vector<std::pair<std::string, std::string>> tt )
    {
      /* ignore latches */
      if ( output == "" )
      {
        assert( inputs.size() == 0u );
        return;
      }

      reader.on_gate( inputs, output, tt );
    } ) ) );

  std::smatch m;
  detail::foreach_line_in_file_escape( in, [&]( std::string line ) {
    redo:
      /* empty line or comment */
      if ( line.empty() || line[0] == '#' )
        return true;

      /* names */
      if ( std::regex_search( line, m, blif_regex::names ) )
      {
        auto args = detail::split( detail::trim_copy( m[1] ), " " );
        const auto output = *( args.rbegin() );
        args.pop_back();

        std::vector<std::pair<std::string, std::string>> tt;
        std::string copy_line;
        detail::foreach_line_in_file_escape( in, [&]( const std::string& line ) {
          copy_line = line;

          /* terminate when the next '.' declaration starts */
          if ( line[0] == '.' )
            return false;

          /* empty line or comment */
          if ( line.empty() || line[0] == '#' )
            return true;

          if ( std::regex_search( line, m, blif_regex::line_of_truthtable ) )
          {
            tt.push_back( std::make_pair( detail::trim_copy( m[1] ), detail::trim_copy( m[2] ) ) );
            return true;
          }

          return false;
        } );

        on_action.call_deferred<GATE_FN>( args, { output }, std::tuple( args, output, tt ) );

        if ( in.eof() )
        {
          return true;
        }
        else
        {
          /* restart parsing with the line of the subparser */
          line = copy_line;
          goto redo;
        }
      }

      /* .model <string> */
      if ( std::regex_search( line, m, blif_regex::model ) )
      {
        reader.on_model( detail::trim_copy( m[1] ) );
        return true;
      }

      /* .inputs <list of whitespace separated strings> */
      if ( detail::starts_with( line, ".inputs" ) )
      {
        std::string const input_declaration = line.substr( 7 );
        for ( const auto& input : detail::split( detail::trim_copy( input_declaration ), " " ) )
        {
          auto const s = detail::trim_copy( input );
          on_action.declare_known( s );
          reader.on_input( s );
        }
        return true;
      }

      /* .latch <list of whitespace separated strings> */
      if ( detail::starts_with( line, ".latch" ) )
      {
        std::string const latch_declaration = line.substr( 6 );

        std::vector<std::string> latch_elements = detail::split( detail::trim_copy( latch_declaration ), " " );
        if ( latch_elements.size() >= 2 )
        {
          std::string const& input = latch_elements.at( 0 );
          std::string const& output = latch_elements.at( 1 );
          std::optional<blif_reader::latch_init_value> init_value;
          std::optional<blif_reader::latch_type> type;
          std::optional<std::string> control;

          if ( latch_elements.size() == 3u )
          {
            init_value = blif_reader::latch_init_value_from_string( latch_elements.at( 2 ) );
          }
          else if ( latch_elements.size() == 4u )
          {
            type = blif_reader::latch_type_from_string( latch_elements.at( 2 ) );
            control = latch_elements.at( 3 );
          }
          else if ( latch_elements.size() == 5u )
          {
            type = blif_reader::latch_type_from_string( latch_elements.at( 2 ) );
            control = latch_elements.at( 3 );
            init_value = blif_reader::latch_init_value_from_string( latch_elements.at( 4 ) );
          }

          on_action.declare_known( output );
          reader.on_latch( input, output, type, control, init_value );
          on_action.compute_dependencies<GATE_FN>( { output } );

          return true;
        }

        diag->report( diag_id::ERR_BLIF_LATCH_FORMAT ).add_argument( line );

        result = return_code::parse_error;
      }

      /* .outputs <list of whitespace separated strings> */
      if ( detail::starts_with( line, ".outputs" ) )
      {
        std::string const output_declaration = line.substr( 8 );
        for ( const auto& output : detail::split( detail::trim_copy( output_declaration ), " " ) )
        {
          reader.on_output( detail::trim_copy( output ) );
        }
        return true;
      }

      /* .end */
      if ( std::regex_search( line, m, blif_regex::end ) )
      {
        reader.on_end();
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
  auto const& deps = on_action.unresolved_dependencies();
  if ( deps.size() > 0 )
    result = return_code::parse_error;

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

/*! \brief Reader function for BLIF format.
 *
 * Reads binary BLIF format from a file and invokes a callback
 * method for each parsed primitive and each detected parse error.
 *
 * \param filename Name of the file
 * \param reader A BLIF reader with callback methods invoked for parsed primitives
 * \param diag An optional diagnostic engine with callback methods for parse errors
 * \return Success if parsing has been successful, or parse error if parsing has failed
 */
[[nodiscard]] inline return_code read_blif( const std::string& filename, const blif_reader& reader, diagnostic_engine* diag = nullptr )
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
    auto const ret = read_blif( in, reader, diag );
    in.close();
    return ret;
  }
}

} // namespace lorina
