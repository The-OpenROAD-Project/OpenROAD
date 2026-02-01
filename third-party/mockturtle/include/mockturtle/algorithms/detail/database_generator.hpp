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
  \file database_generator.hpp
  \brief Utility class to generate a database in form of a logic
         network from functions.

  \author Heinz Riener
*/

#pragma once

#include <kitty/print.hpp>
#include <kitty/properties.hpp>

#include <vector>

namespace mockturtle::detail
{

struct database_generator_params
{
  uint32_t num_vars{ 4u };
  bool multiple_candidates{ false };
  bool verbose{ false };
}; /* database_generator_param */

template<typename Ntk, typename ResynFn>
class database_generator
{
public:
  using signal = mockturtle::signal<Ntk>;

public:
  explicit database_generator( Ntk& ntk, ResynFn const& resyn, database_generator_params const& ps )
      : ntk( ntk ), resyn( resyn ), ps( ps )
  {
    for ( auto i = 0u; i < ps.num_vars; ++i )
    {
      pis.emplace_back( ntk.create_pi() );
    }
  }

  void add_function( kitty::dynamic_truth_table tt )
  {
    /* normalize the function first if necessary */
    if ( !kitty::is_normal( tt ) )
    {
      tt = ~tt;
    }

    /* resynthesize the function and add it to the database */
    resyn( ntk, tt, std::begin( pis ), std::end( pis ),
           [&]( const signal& s ) {
             if ( ps.verbose )
             {
               std::cout << "[i] function: ";
               kitty::print_binary( tt );
               std::cout << " stored at PO #" << ntk.num_pos() << std::endl;
             }
             ntk.create_po( s );
             return ps.multiple_candidates;
           } );
  }

  Ntk& ntk;
  ResynFn const& resyn;
  database_generator_params const& ps;

  std::vector<signal> pis;
}; /* database_generator */

} // namespace mockturtle::detail