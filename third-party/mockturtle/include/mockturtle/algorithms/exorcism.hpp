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
  \file exorcism.hpp
  \brief Wrapper for ABC's exorcism

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <vector>

#include <eabc/exor.h>
#include <kitty/constructors.hpp>
#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/esop.hpp>

namespace abc::exorcism
{
extern int Abc_ExorcismMain( Vec_Wec_t* vEsop, int nIns, int nOuts, std::function<void( uint32_t, uint32_t )> const& onCube, int Quality, int Verbosity, int nCubesMax, int fUseQCost );
}

namespace mockturtle
{

inline std::vector<kitty::cube> exorcism( std::vector<kitty::cube> const& esop, uint32_t num_vars )
{
  auto vesop = abc::exorcism::Vec_WecAlloc( esop.size() );

  for ( auto const& cube : esop )
  {
    auto vcube = abc::exorcism::Vec_WecPushLevel( vesop );
    for ( auto i = 0u; i < num_vars; ++i )
    {
      if ( !cube.get_mask( i ) )
        continue;
      abc::exorcism::Vec_IntPush( vcube, cube.get_bit( i ) ? 2 * i : 2 * i + 1 );
    }
    abc::exorcism::Vec_IntPush( vcube, -1 );
  }

  std::vector<kitty::cube> exorcism_esop;
  abc::exorcism::Abc_ExorcismMain(
      vesop, num_vars, 1, [&]( uint32_t bits, uint32_t mask ) { exorcism_esop.emplace_back( bits, mask ); }, 2, 0, 4 * esop.size(), 0 );

  abc::exorcism::Vec_WecFree( vesop );

  return exorcism_esop;
}

inline std::vector<kitty::cube> exorcism( kitty::dynamic_truth_table const& func )
{
  return exorcism( kitty::esop_from_optimum_pkrm( func ), func.num_vars() );
}

} // namespace mockturtle