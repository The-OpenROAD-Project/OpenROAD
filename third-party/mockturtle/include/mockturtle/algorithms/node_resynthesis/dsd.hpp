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
  \file dsd.hpp
  \brief Use DSD as pre-process to resynthesis

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <vector>

#include <kitty/dynamic_truth_table.hpp>

#include "../dsd_decomposition.hpp"
#include "traits.hpp"

namespace mockturtle
{

/*! \brief Parameters for dsd_resynthesis function. */
struct dsd_resynthesis_params
{
  /*! \brief Skip resynthesis on prime nodes, if it exceeds this limit. */
  std::optional<uint32_t> prime_input_limit;

  /*! \brief DSD decomposition parameters */
  dsd_decomposition_params dsd_ps;
};

/*! \brief Resynthesis function based on DSD decomposition.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  The given truth table will be
 * resynthized based on DSD decomposition.  Since DSD decomposition may not be
 * able to decompose the whole truth table, a different fall-back resynthesis
 * function must be passed to this function.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const aig_network aig = ...;

      exact_aig_resynthesis<aig_network> fallback; // fallback
      dsd_resynthesis<aig_network, decltype( fallback )> resyn( fallback );
      cut_rewriting( aig, resyn );
      aig = cleanup_dangling( aig );
   \endverbatim
 *
 */
template<class Ntk, class ResynthesisFn>
class dsd_resynthesis
{
public:
  explicit dsd_resynthesis( ResynthesisFn& resyn_fn, dsd_resynthesis_params const& ps = {} )
      : _resyn_fn( resyn_fn ),
        _ps( ps )
  {
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    bool success{ true };
    const auto on_prime = [&]( kitty::dynamic_truth_table const& remainder, std::vector<signal<Ntk>> const& leaves ) {
      success = false;
      signal<Ntk> f = ntk.get_constant( false );
      if ( _ps.prime_input_limit && leaves.size() > *_ps.prime_input_limit )
      {
        return f;
      }

      const auto on_signal = [&]( signal<Ntk> const& _f ) {
        if ( !success )
        {
          f = _f;
          success = true;
        }
        return true;
      };
      auto _leaves = leaves;

      if constexpr ( has_set_bounds_v<ResynthesisFn> )
      {
        _resyn_fn.set_bounds( static_cast<uint32_t>( _leaves.size() ), std::nullopt );
      }
      _resyn_fn( ntk, remainder, _leaves.begin(), _leaves.end(), on_signal );
      return f;
    };

    const auto f = dsd_decomposition( ntk, function, std::vector<signal<Ntk>>( begin, end ), on_prime, _ps.dsd_ps );
    if ( success )
    {
      fn( f );
    }
  }

  void clear_functions()
  {
    if constexpr ( has_clear_functions_v<ResynthesisFn> )
    {
      _resyn_fn.clear_functions();
    }
  }

  void add_function( signal<Ntk> const& s, kitty::dynamic_truth_table const& tt )
  {
    if constexpr ( has_add_function_v<ResynthesisFn, Ntk> )
    {
      _resyn_fn.add_function( s, tt );
    }
  }

private:
  ResynthesisFn& _resyn_fn;
  dsd_resynthesis_params _ps;
};

} /* namespace mockturtle */