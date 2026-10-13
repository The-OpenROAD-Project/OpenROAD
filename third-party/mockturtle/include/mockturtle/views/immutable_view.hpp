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
  \file immutable_view.hpp
  \brief Disables all methods to change the network

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../traits.hpp"

namespace mockturtle
{

/*! \brief Deletes all methods that can change the network.
 *
 * This view deletes all methods that can change the network structure such as
 * `create_not`, `create_and`, or `create_node`.  This view is convenient to
 * use as a base class for other views that make some computations based on the
 * structure when being constructed.  Then, changes to the structure invalidate
 * these data.
 */
template<typename Ntk>
class immutable_view : public Ntk
{
public:
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  /*! \brief Default constructor.
   *
   * Constructs immutable view on another network.
   */
  immutable_view( Ntk const& ntk ) : Ntk( ntk )
  {
  }

  signal create_pi() = delete;
  void create_po( signal const& s ) = delete;
  signal create_ro() = delete;
  void create_ri( signal const& s ) = delete;
  signal create_buf( signal const& f ) = delete;
  signal create_not( signal const& f ) = delete;
  signal create_and( signal const& f, signal const& g ) = delete;
  signal create_nand( signal const& f, signal const& g ) = delete;
  signal create_or( signal const& f, signal const& g ) = delete;
  signal create_nor( signal const& f, signal const& g ) = delete;
  signal create_lt( signal const& f, signal const& g ) = delete;
  signal create_le( signal const& f, signal const& g ) = delete;
  signal create_gt( signal const& f, signal const& g ) = delete;
  signal create_ge( signal const& f, signal const& g ) = delete;
  signal create_xor( signal const& f, signal const& g ) = delete;
  signal create_xnor( signal const& f, signal const& g ) = delete;
  signal create_maj( signal const& f, signal const& g, signal const& h ) = delete;
  signal create_ite( signal const& cond, signal const& f_then, signal const& f_else ) = delete;
  signal create_node( std::vector<signal> const& fanin, kitty::dynamic_truth_table const& function ) = delete;
  signal clone_node( immutable_view<Ntk> const& other, node const& source, std::vector<signal> const& fanin ) = delete;
  void substitute_node( node const& old_node, node const& new_node ) = delete;
};

} // namespace mockturtle