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
  \file genlib_reader.hpp
  \brief Reader visitor for GENLIB files

  \author Alessandro Tempia Calvino
  \author Heinz Riener
*/

#pragma once

#include "../traits.hpp"

#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <lorina/genlib.hpp>

namespace mockturtle
{

enum class phase_type : uint8_t
{
  INV = 0,
  NONINV = 1,
  UNKNOWN = 2,
};

struct pin
{
  std::string name;
  phase_type phase;
  double input_load;
  double max_load;
  double rise_block_delay;
  double rise_fanout_delay;
  double fall_block_delay;
  double fall_fanout_delay;
}; /* pin */

struct gate
{
  unsigned int id;
  std::string name;
  std::string expression;
  uint32_t num_vars;
  kitty::dynamic_truth_table function;
  double area;
  std::vector<pin> pins;
  std::string output_name;
}; /* gate */

/*! \brief lorina callbacks for GENLIB files.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      std::vector<gate> gates;
      lorina::read_genlib( "file.genlib", genlib_reader( gates ) );
   \endverbatim
 */
class genlib_reader : public lorina::genlib_reader
{
public:
  explicit genlib_reader( std::vector<gate>& gates )
      : gates( gates )
  {}

  virtual void on_gate( std::string const& name, std::string const& expression, double area, std::vector<lorina::pin_spec> const& ps, std::string const& output_pin ) const override
  {
    std::vector<pin> pp;
    std::vector<std::string> pin_names;

    if ( ps.size() == 1 && ps[0].name == "*" )
    {
      /* pins not defined, use names and appearance order of the expression */
      std::vector<std::string> tokens;
      std::string const delimitators{ " ()!\'*&+|^\t\r\n" };
      std::size_t prev = 0, pos;
      while ( ( pos = expression.find_first_of( delimitators, prev ) ) != std::string::npos )
      {
        if ( pos > prev )
        {
          tokens.emplace_back( expression.substr( prev, pos - prev ) );
        }
        prev = pos + 1;
      }
      if ( prev < expression.length() )
      {
        tokens.emplace_back( expression.substr( prev, std::string::npos ) );
      }
      for ( auto const& pin_name : tokens )
      {
        if ( std::find( pin_names.begin(), pin_names.end(), pin_name ) == pin_names.end() )
        {
          pp.emplace_back( pin{ pin_name,
                                phase_type( static_cast<uint8_t>( ps[0].phase ) ),
                                ps[0].input_load, ps[0].max_load,
                                ps[0].rise_block_delay, ps[0].rise_fanout_delay, ps[0].fall_block_delay, ps[0].fall_fanout_delay } );
          pin_names.push_back( pin_name );
        }
      }
    }
    else
    {
      for ( const auto& p : ps )
      {
        pp.emplace_back( pin{ p.name,
                              phase_type( static_cast<uint8_t>( p.phase ) ),
                              p.input_load, p.max_load,
                              p.rise_block_delay, p.rise_fanout_delay, p.fall_block_delay, p.fall_fanout_delay } );
        pin_names.push_back( p.name );
      }
    }

    /* replace possible CONST0 or CONST1 by 0 and 1 */
    std::string formula( expression );
    std::size_t found = formula.find( "CONST" );
    if ( found != std::string::npos )
    {
      formula.erase( found, found + 5 );
    }

    uint32_t num_vars = pin_names.size();

    kitty::dynamic_truth_table tt{ num_vars };

    if ( !kitty::create_from_formula( tt, formula, pin_names ) )
    {
      /* formula error, skip gate */
      return;
    }

    gates.emplace_back( gate{ static_cast<unsigned int>( gates.size() ), name,
                              expression, num_vars, tt, area, pp, output_pin } );
  }

protected:
  std::vector<gate>& gates;
}; /* genlib_reader */

} /* namespace mockturtle */
