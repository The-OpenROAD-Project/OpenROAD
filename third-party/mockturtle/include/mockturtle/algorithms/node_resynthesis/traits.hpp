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
  \file traits.hpp
  \brief Traits for additional node_resynthesis methods

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include "../../traits.hpp"

#include <kitty/dynamic_truth_table.hpp>

#include <cstdint>
#include <optional>
#include <type_traits>

namespace mockturtle
{

#pragma region has_set_bounds
template<class ResynFn, class = void>
struct has_set_bounds : std::false_type
{
};

template<class ResynFn>
struct has_set_bounds<ResynFn, std::void_t<decltype( std::declval<ResynFn>().set_bounds( std::optional<uint32_t>(), std::optional<uint32_t>() ) )>> : std::true_type
{
};

template<class ResynFn>
inline constexpr bool has_set_bounds_v = has_set_bounds<ResynFn>::value;
#pragma endregion

#pragma region has_clear_functions
template<class ResynFn, class = void>
struct has_clear_functions : std::false_type
{
};

template<class ResynFn>
struct has_clear_functions<ResynFn, std::void_t<decltype( std::declval<ResynFn>().clear_functions() )>> : std::true_type
{
};

template<class ResynFn>
inline constexpr bool has_clear_functions_v = has_clear_functions<ResynFn>::value;
#pragma endregion

#pragma region has_add_function
template<class ResynFn, class Ntk, class = void>
struct has_add_function : std::false_type
{
};

template<class ResynFn, class Ntk>
struct has_add_function<ResynFn, Ntk, std::void_t<decltype( std::declval<ResynFn>().add_function( signal<Ntk>(), kitty::dynamic_truth_table() ) )>> : std::true_type
{
};

template<class ResynFn, class Ntk>
inline constexpr bool has_add_function_v = has_add_function<ResynFn, Ntk>::value;
#pragma endregion

} /* namespace mockturtle */