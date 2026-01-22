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
  \file list_simulator.hpp
  \brief Simulator engine for index lists.

  \author Andrea Costamagna
*/

#pragma once

#include "index_list.hpp"

#include <algorithm>
#include <vector>

namespace mockturtle
{

/*! \brief Simulator engine for XAIG-index lists.
 *
 * This engine can be used to efficiently simulate many index lists.
 * In this context, a simulation pattern is a truth table corresponding
 * to a node’s Boolean behavior under the given input assignments.
 * It pre-allocates the memory necessary to store the simulation patterns
 * of an index list, and extends this memory when the evaluator is required
 * to perform Boolean evaluation of a list that is larger than the current capacity
 * of the simulator. To avoid unnecessary copies, the input simulation patterns
 * must be passed as a vector of raw pointers.
 *
 * \tparam TT Truth table type.
 * \tparam separate_header
 *
 * \verbatim embed:rst

   Example

   .. code-block:: c++

      xag_list_simulator<kitty::static_truth_table<4u>> sim;
      sim( list1, inputs1 );
      sim( list2, inputs2 );
   \endverbatim
 */
template<class TT>
class xag_list_simulator
{
public:
  /* Type of the literals of the index list */
  using element_type = typename xag_index_list<true>::element_type;

  xag_list_simulator()
      : const0( TT().construct() )
  {
    /* The value 20 is chosen as it allows us to store most practical XAIG index lists */
    sims.resize( 20u );
  }

  /*! \brief Extract the simulation of a literal
   *
   * Inline specifier used to ensure manual inlining.
   *
   * \param res Truth table where to store the result.
   * \param list Index list to be simulated.
   * \param inputs Vector of pointers to the input truth tables.
   * \param lit Literal whose simulation we want to extract.
   */
  template<bool separate_header>
  inline void get_simulation_inline( TT& res, xag_index_list<separate_header> const& list, std::vector<TT const*> const& inputs, element_type const& lit )
  {
    if ( list.is_constant( lit ) )
    {
      res = complement( const0, list.is_complemented( lit ) );
      return;
    }
    if ( list.is_pi( lit ) )
    {
      uint32_t index = list.get_pi_index( lit );
      TT const& sim = *inputs[index];
      res = complement( sim, list.is_complemented( lit ) );
      return;
    }
    uint32_t index = list.get_node_index( lit );
    TT const& sim = sims[index];
    res = complement( sim, list.is_complemented( lit ) );
  }

  /*! \brief Simulate the list in topological order.
   *
   * This method updates the internal state of the simulator by
   * storing in `sims` the simulation patterns of the nodes in the list.
   *
   * \param list Index list to be simulated.
   * \param inputs Vector of TT references corresponding to the input truth tables
   */
  template<bool separate_header>
  void operator()( xag_index_list<separate_header> const& list, std::vector<TT const*> const& inputs )
  {
    /* update the allocated memory */
    if ( sims.size() < list.num_gates() )
      sims.resize( std::max<size_t>( sims.size(), list.num_gates() ) );
    /* traverse the list in topological order and simulate each node */
    size_t i = 0;
    list.foreach_gate( [&]( element_type const& lit_lhs, element_type const& lit_rhs ) {
      auto const [tt_lhs_ptr, is_lhs_compl] = get_simulation( list, inputs, lit_lhs );
      auto const [tt_rhs_ptr, is_rhs_compl] = get_simulation( list, inputs, lit_rhs );
      sims[i++] = list.is_and( lit_lhs, lit_rhs )
                      ? complement( *tt_lhs_ptr, is_lhs_compl ) & complement( *tt_rhs_ptr, is_rhs_compl )
                      : complement( *tt_lhs_ptr, is_lhs_compl ) ^ complement( *tt_rhs_ptr, is_rhs_compl );
    } );
  }

private:
  inline TT complement( TT const& tt, bool c )
  {
    return c ? ~tt : tt;
  }

  /*! \brief Return the simulation associated to the literal
   *
   * \param list An XAIG index list, with or without separated header.
   * \param inputs A vector of pointers to the input simulation patterns.
   * \param lit The literal whose simulation we want to extract.
   *
   * The size of `inputs` should be equal to the number of inputs of the list.
   * Keep private to avoid giving external access to memory that could be later corrupted.
   *
   * \return A tuple containing a pointer to the simulation pattern and a flag for complementation.
   */
  template<bool separate_header>
  [[nodiscard]] std::tuple<TT const*, bool> get_simulation( xag_index_list<separate_header> const& list, std::vector<TT const*> const& inputs, element_type const& lit )
  {
    if ( list.num_pis() != inputs.size() )
      throw std::invalid_argument( "Mismatch between number of PIs and input simulations." );
    if ( list.is_constant( lit ) )
    {
      return { &const0, list.is_complemented( lit ) };
    }
    if ( list.is_pi( lit ) )
    {
      uint32_t index = list.get_pi_index( lit );
      TT const& sim = *inputs[index];
      return { &sim, list.is_complemented( lit ) };
    }
    uint32_t index = list.get_node_index( lit );
    TT const& sim = sims[index];
    return { &sim, list.is_complemented( lit ) };
  }

private:
  std::vector<TT> sims;
  TT const0;

}; /* list simulator */

} /* namespace mockturtle */