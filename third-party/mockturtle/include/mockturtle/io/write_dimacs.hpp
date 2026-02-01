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
/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2019  EPFL EPFL
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
  \file write_dimacs.hpp
  \brief Write networks CNF encoding to DIMACS format

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <fmt/format.h>

#include "../algorithms/cnf.hpp"
#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Writes network into CNF DIMACS format
 *
 * It also adds unit clauses for the outputs.  Therefore a satisfying solution
 * is one that makes all outputs 1.
 *
 * \param ntk Logic network
 * \param out Output stream
 */
template<class Ntk>
void write_dimacs( Ntk const& ntk, std::ostream& out = std::cout )
{
  std::stringstream clauses;
  uint32_t num_clauses = 0u;

  const auto lits = generate_cnf( ntk, [&]( std::vector<uint32_t> const& clause ) {
    for ( auto lit : clause )
    {
      const auto var = ( lit / 2 ) + 1;
      const auto pol = lit % 2;
      clauses << fmt::format( "{}{} ", pol ? "-" : "", var );
    }
    clauses << fmt::format( "0\n" );
    ++num_clauses;
  } );

  for ( auto lit : lits )
  {
    const auto var = ( lit / 2 ) + 1;
    const auto pol = lit % 2;
    clauses << fmt::format( "{}{} 0\n", pol ? "-" : "", var );
    ++num_clauses;
  }

  out << fmt::format( "p cnf {} {}\n{}", ntk.size(), num_clauses, clauses.str() );
}

/*! \brief Writes network into CNF DIMACS format
 *
 * It also adds unit clauses for the outputs.  Therefore a satisfying solution
 * is one that makes all outputs 1.
 *
 * \param ntk Logic network
 * \param filename Filename
 */
template<class Ntk>
void write_dimacs( Ntk const& ntk, std::string const& filename )
{
  std::ofstream os( filename.c_str(), std::ofstream::out );
  write_dimacs( ntk, os );
  os.close();
}

} /* namespace mockturtle */