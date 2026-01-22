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
  \file truth_table_utils.hpp
  \brief Truth table manipulation utils

  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <kitty/kitty.hpp>

namespace mockturtle
{

/*! \brief Replacement rule of MAJ3
 *
 * Given a majority gate with fanin functions `fanin0`, `fanin1` and `fanin2`,
 * check if the first fanin `fanin0` can be replaced by `replacement`
 * without changing the output function of the majority gate.
 *
 * By the replacement rule, `<x, y, z> = <w, y, z>` if and only if
 * `(x ^ w)(y ^ z) = 0`, i.e., `y != z` implies `w = x`.
 *
 * This check is used in relevance optimization in MIG resubstitution.
 *
 * \param fanin0 Truth table of the first fanin function; also the fanin to be replaced.
 * \param fanin1 Truth table of the second fanin function.
 * \param fanin2 Truth table of the third fanin function.
 * \param replacement Truth table of the candidate to replace `fanin0`.
 * \return `<fanin0, fanin1, fanin2> = <replacement, fanin1, fanin2>`
 */
template<typename TT, typename = std::enable_if_t<kitty::is_truth_table<TT>::value>>
bool can_replace_majority_fanin( TT const& fanin0, TT const& fanin1, TT const& fanin2, TT const& replacement )
{
  return kitty::is_const0( ( ( fanin0 ^ replacement ) & ( fanin1 ^ fanin2 ) ) );
}

} // namespace mockturtle