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
  \file standard_cell.hpp
  \brief Defines logic cells.

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <unordered_map>
#include <vector>

#include "../io/genlib_reader.hpp"

namespace mockturtle
{

struct standard_cell
{
  /* Unique name */
  std::string name;

  /* Unique ID */
  uint32_t id;

  /* Pointer to a gate representing each individual output */
  std::vector<gate> gates;

  /* Area */
  double area;
};

/*! \brief Reconstruct standard cells from GENLIB gates.
 *
 * This function returns a vector of standard cells given
 * GENLIB gates.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      std::vector<gate> gates;
      lorina::read_genlib( in, genlib_reader( gates ) );

      // Extract standard cells
      std::vector<standard_cell> cells = get_standard_cells( gates );
   \endverbatim
 */
inline std::vector<standard_cell> get_standard_cells( std::vector<gate> const& gates )
{
  std::unordered_map<std::string, uint32_t> name_to_index;
  std::vector<standard_cell> cells;

  for ( gate const& g : gates )
  {
    if ( auto it = name_to_index.find( g.name ); it != name_to_index.end() )
    {
      /* add to existing cell (multi-output) */
      cells[it->second].gates.push_back( g );
    }
    else
    {
      name_to_index[g.name] = cells.size();
      cells.emplace_back( standard_cell{ g.name, static_cast<uint32_t>( cells.size() ), { g }, g.area } );
    }
  }

  return cells;
}

} // namespace mockturtle