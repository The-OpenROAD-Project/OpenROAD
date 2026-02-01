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
  \file composed.hpp
  \brief Traits for additional node_resynthesis methods

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#if !__clang__ || __clang_major__ > 10

#include <cstdint>
#include <string>

#include "../../networks/aig.hpp"
#include "../../networks/xag.hpp"
#include "cached.hpp"
#include "exact.hpp"

namespace mockturtle
{

struct exact_blacklist_cache_info
{
  bool retry( exact_blacklist_cache_info const& old_info ) const
  {
    return conflict_limit > old_info.conflict_limit;
  }

  int conflict_limit;
};

inline void to_json( nlohmann::json& j, exact_blacklist_cache_info const& info )
{
  j = info.conflict_limit;
}

inline void from_json( nlohmann::json const& j, exact_blacklist_cache_info& info )
{
  j.get_to( info.conflict_limit );
}

template<class Ntk>
auto cached_exact_xag_resynthesis( std::string const& cache_filename, uint32_t input_limit = 12u, int conflict_limit = 10e5 )
{
  exact_resynthesis_params exact_ps;
  exact_ps.conflict_limit = conflict_limit;
  exact_aig_resynthesis<Ntk> exact_resyn( std::is_same_v<typename Ntk::base_type, xag_network>, exact_ps );
  exact_blacklist_cache_info info;
  info.conflict_limit = conflict_limit;
  return cached_resynthesis<Ntk, decltype( exact_resyn ), exact_blacklist_cache_info>( exact_resyn, input_limit, cache_filename, info );
}

} // namespace mockturtle

#endif