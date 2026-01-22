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
  \file json_utils.hpp
  \brief JSON utils

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <kitty/dynamic_truth_table.hpp>
#include <nlohmann/json.hpp>

namespace kitty
{

inline void to_json( nlohmann::json& j, const dynamic_truth_table& tt )
{
  j = nlohmann::json{ { "_bits", tt._bits }, { "_num_vars", tt._num_vars } };
}

inline void from_json( const nlohmann::json& j, dynamic_truth_table& tt )
{
  j.at( "_bits" ).get_to( tt._bits );
  j.at( "_num_vars" ).get_to( tt._num_vars );
}

} // namespace kitty