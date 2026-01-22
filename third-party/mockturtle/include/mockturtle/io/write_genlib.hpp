/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2023  EPFL
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
  \file write_genlib.hpp
  \brief Writes library of gates to GENLIB format

  \author Alessandro tempia Calvino
*/

#pragma once

#include "genlib_reader.hpp"
#include "../traits.hpp"

#include <fmt/format.h>

#include <fstream>
#include <iostream>
#include <string>

namespace mockturtle
{

/*! \brief Writes library of gates to GENLIB format
 *
 * An overloaded variant exists that writes the network into a file.
 *
 * \param gates List of gates
 * \param os Output stream
 */
void write_genlib( std::vector<gate> const& gates, std::ostream& os )
{
  /* compute pad spaces */
  size_t max_name_size = 0;
  for ( gate const& g : gates )
  {
    max_name_size = std::max( max_name_size, g.name.size() );
  }
  ++max_name_size;

  for ( gate const& g : gates )
  {
    os << "GATE ";
    std::string name = g.name + std::string( max_name_size - g.name.size(), ' ' );
    os << fmt::format( "{} {:>5.4f} {}={};\n", name, g.area, g.output_name, g.expression );

    for ( pin const& p : g.pins )
    {
      std::string phase;
      if ( p.phase == phase_type::INV )
        phase = "INV";
      else if ( p.phase == phase_type::NONINV )
        phase = "NONINV";
      else
        phase = "UNKNOWN";

      os << fmt::format( "\tPIN {} {} {:>3} {:>3} {:>6.4f} {:>6.4f} {:>6.4f} {:>6.4f}\n",
        p.name,
        phase,
        ( uint32_t )p.input_load,
        ( uint32_t )p.max_load,
        p.rise_block_delay,
        p.rise_fanout_delay,
        p.fall_block_delay,
        p.fall_fanout_delay );
    }
  }
}

/*! \brief Write library of gates into a GENLIB file
 *
 * \param gates List of gates
 * \param filename Filename
 */
void write_genlib( std::vector<gate> const& gates, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_genlib( gates, os );
  os.close();
}

} /* namespace mockturtle */