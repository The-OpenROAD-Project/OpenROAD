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
  \file supergate.hpp
  \brief Defines the composed gate and supergate data structure.

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <array>
#include <vector>

#include "../../io/genlib_reader.hpp"

#include <kitty/dynamic_truth_table.hpp>

namespace mockturtle
{

template<unsigned NInputs>
struct composed_gate
{
  /* unique ID */
  uint32_t id;

  /* gate is a supergate */
  bool is_super{ false };

  /* pointer to the root library gate */
  gate const* root{ nullptr };

  /* support of the composed gate */
  uint32_t num_vars{ 0 };

  /* function */
  kitty::dynamic_truth_table function;

  /* area */
  double area{ 0.0 };

  /* pin-to-pin delays */
  std::array<float, NInputs> tdelay{};

  /* fanin gates */
  std::vector<composed_gate<NInputs>*> fanin{};
};

template<unsigned NInputs>
struct supergate
{
  /* pointer to the root gate */
  composed_gate<NInputs> const* root{};

  /* area */
  double area{ 0.0 };

  /* pin-to-pin delay */
  std::array<float, NInputs> tdelay{};

  /* np permutation vector */
  std::vector<uint8_t> permutation{};

  /* pin negations */
  uint16_t polarity{ 0 };
};

} // namespace mockturtle