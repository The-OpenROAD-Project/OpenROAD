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
  \file interface.hpp
  \brief Documentation of network interfaces

  \author Bruno Schmitt
  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>

#include "networks/events.hpp"
#include "traits.hpp"

namespace mockturtle
{

static_assert( false, "file interface.hpp cannot be included, it's only used for documentation" );

class network final
{
public:
  /*! \brief Type referring to itself.
   *
   * The ``base_type`` is the network type itself.  It is required, because
   * views may extend networks, and this type provides a way to determine the
   * underlying network type.
   */
  using base_type = network;

  /*! \brief Type representing a node.
   *
   * A ``node`` is a node in the logic network.  It could be a constant, a
   * primary input or a logic gate.
   */
  struct node
  {
  };

  /*! \brief Type representing a signal.
   *
   * A ``signal`` can be seen as a pointer to a node, or an outgoing edge of
   * a node towards its fanout.  Depending on the kind of logic network, it
   * may carry additional information such as a complement attribute.
   */
  struct signal
  {
  };

  /*! \brief Type representing the storage.
   *
   * A ``storage`` is some container that can contain all data necessary to
   * store the logic network.  It can be constructed outside of the logic network
   * and passed as a reference to the constructor.  It may be shared among
   * several logic networks.  A `std::shared_ptr<T>` is a convenient data
   * structure to hold a storage in a logic network.
   */
  struct storage
  {
  };

  /*! \brief Default constructor.
   *
   * Constructs an empty network.
   */
  network();

  /*! \brief Constructor taking a storage. */
  explicit network( storage s );

  /*! \brief Default copy assignment operator.
   *
   * Currently, most network implementations in mockturtle use `std::shared_ptr<storage>`
   * to hold and share the storage. Thus, the default behavior of copy-assigning
   * a network only copies the pointer, but not really duplicating the contents
   * in the storage data structure. In other words, it makes a shallow copy
   * by default.
   */
  network& operator=( const network& other ) = default;

  /*! \brief Explicitly duplicate a network.
   *
   * Deep copy a network by duplicating the storage. Note that this method
   * does not duplicate the network events.
   */
  network clone();

#pragma region Primary I / O and constants
  /*! \brief Gets constant value represented by network.
   *
   * A constant node is the only node that must be created when initializing
   * the network.  For this reason, this method has constant access and is not
   * called `create_constant`.
   *
   * \param value Constant value
   */
  signal get_constant( bool value ) const;

  /*! \brief Creates a primary input in the network.
   *
   * Each created primary input is stored in a node and contributes to the size
   * of the network.
   */
  signal create_pi();

  /*! \brief Creates a primary output in the network.
   *
   * A primary output is not stored in terms of a node, and it also does not
   * contribute to the size of the network.  A primary output is created for a
   * signal in the network and it is possible that multiple primary outputs
   * point to the same signal.
   *
   * \param s Signal that drives the created primary output
   */
  void create_po( signal const& s );

  /*! \brief Checks whether a node is a constant node. */
  bool is_constant( node const& n ) const;

  /*! \brief Checks whether a node is a primary input. */
  bool is_pi( node const& n ) const;

  /*! \brief Checks whether a node is a combinational input.
   *
   * This method should be effectively the same as ``is_pi`` in a
   * combinational network.
   */
  bool is_ci( node const& n ) const;

  /*! \brief Gets the Boolean value of the constant node.
   *
   * The method expects that `n` is a constant node.
   */
  bool constant_value( node const& n ) const;
#pragma endregion

#pragma region Create unary functions
  /*! \brief Creates signal that computes ``f``.
   *
   * This method is not required to create a gate in the network.  A network
   * implementation can also just return ``f``.
   *
   * \param f Child signal
   */
  signal create_buf( signal const& f );

  /*! \brief Creates a signal that inverts ``f``.
   *
   * This method is not required to create a gate in the network.  If a network
   * supports complemented attributes on signals, it can just return the
   * complemented signal ``f``.
   *
   * \param f Child signal
   */
  signal create_not( signal const& f );
#pragma endregion

#pragma region Create binary functions
  /*! \brief Creates a signal that computes the binary AND. */
  signal create_and( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary NAND. */
  signal create_nand( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary OR. */
  signal create_or( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary NOR. */
  signal create_nor( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary less-than.
   *
   * The signal is true if and only if ``f`` is 0 and ``g`` is 1.
   */
  signal create_lt( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary less-than-or-equal.
   *
   * The signal is true if and only if ``f`` is 0 or ``g`` is 1.
   */
  signal create_le( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary greater-than.
   *
   * The signal is true if and only if ``f`` is 1 and ``g`` is 0.
   */
  signal create_gt( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary greater-than-or-equal.
   *
   * The signal is true if and only if ``f`` is 1 or ``g`` is 0.
   */
  signal create_ge( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary XOR. */
  signal create_xor( signal const& f, signal const& g );

  /*! \brief Creates a signal that computes the binary XNOR. */
  signal create_xnor( signal const& f, signal const& g );
#pragma endregion

#pragma region Create ternary functions
  /*! \brief Creates a signal that computes the majority-of-3. */
  signal create_maj( signal const& f, signal const& g, signal const& h );

  /*! \brief Creates a signal that computes the if-then-else operation.
   *
   * \param cond Condition for ITE operator
   * \param f_then Then-case for ITE operator
   * \param f_else Else-case for ITE operator
   */
  signal create_ite( signal const& cond, signal const& f_then, signal const& f_else );

  /*! \brief Creates a signal that computes the ternary XOR operation. */
  signal create_xor3( signal const& a, signal const& b, signal const& c );
#pragma endregion

#pragma region Create nary functions
  /*! \brief Creates a signal that computes the n-ary AND.
   *
   * If `fs` is empty, it returns constant-1.
   */
  signal create_nary_and( std::vector<signal> const& fs );

  /*! \brief Creates a signal that computes the n-ary OR.
   *
   * If `fs` is empty, it returns constant-0.
   */
  signal create_nary_or( std::vector<signal> const& fs );

  /*! \brief Creates a signal that computes the n-ary XOR.
   *
   * If `fs` is empty, it returns constant-0.
   */
  signal create_nary_xor( std::vector<signal> const& fs );
#pragma endregion

#pragma region Create arbitrary functions
  /*! \brief Creates node with arbitrary function.
   *
   * The number of variables in ``function`` must match the number of fanin
   * signals in ``fanin``.  ``fanin[0]`` will correspond to the
   * least-significant variable in ``function``.
   *
   * \param fanin Fan-in signals
   * \param function Truth table for node function
   */
  signal create_node( std::vector<signal> const& fanin, kitty::dynamic_truth_table const& function );

  /*! \brief Clones a node from another network of same type.
   *
   * This method can clone a node from a different network ``other``, which is
   * from the same type.  The node ``source`` is a node in the source network
   * ``other``, but the signals in ``fanin`` refer to signals in the target
   * network, which are assumed to be in the same order as in the source
   * network.
   *
   * \param other Other network of same type
   * \param source Node in ``other``
   * \param children Fan-in signals from the current network
   * \return New signal representing node in current network
   */
  signal clone_node( network const& other, node const& source, std::vector<signal> const& fanin );
#pragma endregion

#pragma region Restructuring
  /*! \brief Replaces one node in a network by another signal.
   *
   * This method causes all nodes that have ``old_node`` as fanin to have
   * `new_signal` as fanin instead.  In doing so, a possible polarity of
   * `new_signal` is taken into account.  Afterwards, the fan-out count of
   * ``old_node`` is guaranteed to be 0.
   *
   * It does not update custom values or visited flags of a node.
   *
   * \param old_node Node to replace
   * \param new_signal Signal to replace ``old_node`` with
   */
  void substitute_node( node const& old_node, signal const& new_signal );

  /*! \brief Perform multiple node-signal replacements in a network.
   *
   * This method replaces all occurrences of a node with a signal for
   * all pairs (node, signal) in the substitution list.
   *
   * \param substitutions A list of (node, signal) replacement pairs
   */
  void substitute_nodes( std::list<std::pair<node, signal>> substitutions );

  /*! \brief Replaces a child node by a new signal in a node.
   *
   * If ``n`` has a child pointing to ``old_node``, then it will be replaced by
   * ``new_signal``.  If the replacement catches a trivial case, e.g., ``n``
   * becomes a constant, then this will be returned as an optional replacement
   * candidate by the function.
   *
   * The function updates the hash table. If no trivial case was found, it
   * updates the hash table according to the new structure of ``n``.
   *
   * \param n Node which may have ``old_node`` as a child
   * \param old_node Child to be replaced
   * \param new_signal Signel to replace ``old_node`` with
   * \return May return new recursive replacement candidate
   */
  std::optional<std::pair<node, signal>> replace_in_node( node const& n, node const& old_node, signal new_signal );

  /*! \brief Replaces a output driver by a new signal.
   *
   * If ``old_node`` is drive to some output, then it will be replaced by
   * ``new_signal``.
   *
   * \param old_node Driver to be replaced
   * \param new_signal Signal replace ``old_node`` with
   */
  void replace_in_outputs( node const& old_node, signal const& new_signal );

  /*! \brief Removes a node (and potentially its fanins) from the hash table.
   *
   * The node will be marked dead.  This status can be checked with
   * ``is_dead``. Taking out a node does not change the indexes of
   * other nodes.  The node will be removed from the hash table.
   * The reference counters of all fanin will be decremented and
   * ``take_out_node`` will be recursively invoked on all fanins
   * if their fanout count reach 0.
   *
   * \param n Node to be removed
   */
  void take_out_node( node const& n );

  /*! \brief Check if a node is dead.
   *
   * A dead node is no longer visited in the ``foreach_node`` and
   * ``foreach_gate`` methods.  It still contributes to the overall
   * ``size`` of the network, but ``num_gates`` does not take dead
   * nodes into account.
   *
   * \param n Node to check
   * \return Whether ``n`` is dead
   */
  bool is_dead( node const& n ) const;
#pragma endregion

#pragma region Structural properties
  /*! \brief Checks whether the network is combinational. */
  bool is_combinational() const;

  /*! \brief Returns the number of nodes (incl. constants and PIs and dead nodes). */
  uint32_t size() const;

  /*! \brief Returns the number of primary inputs. */
  uint32_t num_pis() const;

  /*! \brief Returns the number of primary outputs. */
  uint32_t num_pos() const;

  /*! \brief Returns the number of combinational inputs.
   *
   * This method should be effectively the same as `num_pis`` in a
   * combinational network.
   */
  uint32_t num_cis() const;

  /*! \brief Returns the number of combinational outputs.
   *
   * This method should be effectively the same as `num_pos`` in a
   * combinational network.
   */
  uint32_t num_cos() const;

  /*! \brief Returns the number of gates (without dead nodes) */
  uint32_t num_gates() const;

  /*! \brief Returns the fanin size of a node. */
  uint32_t fanin_size( node const& n ) const;

  /*! \brief Returns the fanout size of a node. */
  uint32_t fanout_size( node const& n ) const;

  /*! \brief Increments fanout size and returns old value.
   *
   * This is useful for ref-counting based algorithm.  The user of this function
   * should make sure to bring the value back to a consistent state.
   */
  uint32_t incr_fanout_size( node const& n ) const;

  /*! \brief Decrements fanout size and returns new value.
   *
   * This is useful for ref-counting based algorithm.  The user of this function
   * should make sure to bring the value back to a consistent state.
   */
  uint32_t decr_fanout_size( node const& n ) const;

  /*! \brief Returns the length of the critical path.
   *
   * For efficiency reasons, this interface is often not provided in the
   * network implementations, but has to be extended by wrapping with `depth_view`.
   */
  uint32_t depth() const;

  /*! \brief Returns the level of a node.
   *
   * For efficiency reasons, this interface is often not provided in the
   * network implementations, but has to be extended by wrapping with `depth_view`.
   */
  uint32_t level( node const& n ) const;

  /*! \brief Returns true if node is a 2-input AND gate. */
  bool is_and( node const& n ) const;

  /*! \brief Returns true if node is a 2-input OR gate. */
  bool is_or( node const& n ) const;

  /*! \brief Returns true if node is a 2-input XOR gate. */
  bool is_xor( node const& n ) const;

  /*! \brief Returns true if node is a majority-of-3 gate. */
  bool is_maj( node const& n ) const;

  /*! \brief Returns true if node is a if-then-else gate. */
  bool is_ite( node const& n ) const;

  /*! \brief Returns true if node is a 3-input XOR gate. */
  bool is_xor3( node const& n ) const;

  /*! \brief Returns true if node is a primitive n-ary AND gate. */
  bool is_nary_and( node const& n ) const;

  /*! \brief Returns true if node is a primitive n-ary OR gate. */
  bool is_nary_or( node const& n ) const;

  /*! \brief Returns true if node is a primitive n-ary XOR gate. */
  bool is_nary_xor( node const& n ) const;

  /*! \brief Returns true if node is a general function node. */
  bool is_function( node const& n ) const;
#pragma endregion

#pragma region Functional properties
  /*! \brief Returns the gate function of a node.
   *
   * Note that this function returns the gate function represented by a node
   * in terms of the *intended* gate.  For example, in an AIG, all gate
   * functions are AND, complemented edges are not taken into account.  Also,
   * in an MIG, all gate functions are MAJ, independently of complemented edges
   * and possible constant inputs.
   *
   * In order to retrieve a function with respect to complemented edges one can
   * use the `compute` function with a truth table as simulation value.
   */
  kitty::dynamic_truth_table node_function( node const& n ) const;
#pragma endregion

#pragma region Nodes and signals
  /*! \brief Get the node a signal is pointing to. */
  node get_node( signal const& f ) const;

  /*! \brief Create a signal from a node (without edge attributes). */
  signal make_signal( node const& n ) const;

  /*! \brief Check whether a signal is complemented.
   *
   * This method may also be provided by network implementations that do not
   * have complemented edges.  In this case, the method simply returns
   * ``false`` for each node.
   */
  bool is_complemented( signal const& f ) const;

  /*! \brief Returns the index of a node.
   *
   * The index of a node must be a unique for each node and must be between 0
   * (inclusive) and the size of a network (exclusive, value returned by
   * ``size()``).
   */
  uint32_t node_to_index( node const& n ) const;

  /*! \brief Returns the node for an index.
   *
   * This is the inverse function to ``node_to_index``.
   *
   * \param index A value between 0 (inclusive) and the size of the network
   *              (exclusive)
   */
  node index_to_node( uint32_t index ) const;

  /*! \brief Returns the primary input node for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              primary inputs (exclusive).
   */
  node pi_at( uint32_t index ) const;

  /*! \brief Returns the primary output signal for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              primary outputs (exclusive).
   */
  signal po_at( uint32_t index ) const;

  /*! \brief Returns the combinational input node for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              combinational inputs (exclusive).
   */
  node ci_at( uint32_t index ) const;

  /*! \brief Returns the combinational output signal for an index.
   *
   * \param index A value between 0 (inclusive) and the number of
   *              combinational outputs (exclusive).
   */
  signal co_at( uint32_t index ) const;

  /*! \brief Returns the index of a primary input node.
   *
   * \param n A primary input node.
   * \return A value between 0 and num_pis()-1.
   */
  uint32_t pi_index( node const& n ) const;

  /*! \brief Returns the index of a primary output signal.
   *
   * \param n A primary output signal.
   * \return A value between 0 and num_pos()-1.
   */
  uint32_t po_index( signal const& n ) const;

  /*! \brief Returns the index of a combinational input node.
   *
   * \param n A combinational input node.
   * \return A value between 0 and num_cis()-1.
   */
  uint32_t ci_index( node const& n ) const;

  /*! \brief Returns the index of a combinational output signal.
   *
   * \param n A combinational output signal.
   * \return A value between 0 and num_cos()-1.
   */
  uint32_t co_index( signal const& n ) const;
#pragma endregion

#pragma region Node and signal iterators
  /*! \brief Calls ``fn`` on every node in network.
   *
   * The order of nodes depends on the implementation and must not guarantee
   * topological order.  The parameter ``fn`` is any callable that must have
   * one of the following four signatures.
   * - ``void(node const&)``
   * - ``void(node const&, uint32_t)``
   * - ``bool(node const&)``
   * - ``bool(node const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_node( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every gate node in the network.
   *
   * Calls each node that is not constant and not a combinational input.  The
   * parameter ``fn`` is any callable that must have one of the following four
   * signatures.
   * - ``void(node const&)``
   * - ``void(node const&, uint32_t)``
   * - ``bool(node const&)``
   * - ``bool(node const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every primary input node in the network.
   *
   * The order is in the same order as primary inputs have been created with
   * ``create_pi``.  The parameter ``fn`` is any callable that must have one of
   * the following four signatures.
   * - ``void(node const&)``
   * - ``void(node const&, uint32_t)``
   * - ``bool(node const&)``
   * - ``bool(node const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_pi( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every primary output signal in the network.
   *
   * The order is in the same order as primary outputs have been created with
   * ``create_po``.  The function is called on the signal that is driving the
   * output and may occur more than once in the iteration, if it drives more
   * than one output.  The parameter ``fn`` is any callable that must have one
   * of the following four signatures.
   * - ``void(signal const&)``
   * - ``void(signal const&, uint32_t)``
   * - ``bool(signal const&)``
   * - ``bool(signal const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_po( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every combinational input node in the network.
   *
   * This method should be effectively the same as ``foreach_pi`` in a
   * combinational network.
   */
  template<typename Fn>
  void foreach_ci( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every combinational output signal in the network.
   *
   * This method should be effectively the same as ``foreach_po`` in a
   * combinational network.
   */
  template<typename Fn>
  void foreach_co( Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every fanin of a node.
   *
   * The order of the fanins is in the same order that was used to create the
   * node.  The parameter ``fn`` is any callable that must have one of the
   * following four signatures.
   * - ``void(signal const&)``
   * - ``void(signal const&, uint32_t)``
   * - ``bool(signal const&)``
   * - ``bool(signal const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   */
  template<typename Fn>
  void foreach_fanin( node const& n, Fn&& fn ) const;

  /*! \brief Calls ``fn`` on every fanout of a node.
   *
   * The method gives no guarantee on the order of the fanout.  The parameter
   * ``fn`` is any callable that must have one of the following signatures.
   * - ``void(node const&)``
   * - ``void(node const&, uint32_t)``
   * - ``bool(node const&)``
   * - ``bool(node const&, uint32_t)``
   *
   * If ``fn`` has two parameters, the second parameter is an index starting
   * from 0 and incremented in every iteration.  If ``fn`` returns a ``bool``,
   * then it can interrupt the iteration by returning ``false``.
   *
   * For efficiency reasons, this interface is often not provided in the
   * network implementations, but has to be extended by wrapping with `fanout_view`.
   */
  template<typename Fn>
  void foreach_fanout( node const& n, Fn&& fn ) const;
#pragma endregion

#pragma region Simulate values
  /*! \brief Simulates arbitrary value on a node.
   *
   * This is a generic simulation method that can be implemented multiple times
   * for a network interface for different types.  One only needs to change the
   * implementation and change the value for the type parameter ``T``, which
   * indicates the element type of the iterators.
   *
   * Examples for simulation types are ``bool``,
   * ``kitty::dynamic_truth_table``, bit masks, or BDDs.
   *
   * The ``begin`` and ``end`` iterator point to values which are assumed to be
   * assigned to the fanin of the node.  Consequently, the distance from
   * ``begin`` to ``end`` must equal the fanin size of the node.
   *
   * \param n Node to simulate (used to retrieve the node function)
   * \param begin Begin iterator to simulation values of fanin
   * \param end End iterator to simulation values of fanin
   * \return Returns computed simulation value of type ``T``
   */
  template<typename Iterator>
  iterates_over_t<Iterator, T>
  compute( node const& n, Iterator begin, Iterator end ) const;
#pragma endregion

#pragma region Custom node values
  /*! \brief Reset all values to 0. */
  void clear_values() const;

  /*! \brief Returns value of a node. */
  uint32_t value( node const& n ) const;

  /*! \brief Sets value of a node. */
  void set_value( node const& n, uint32_t value ) const;

  /*! \brief Increments value of a node and returns *previous* value. */
  uint32_t incr_value( node const& n ) const;

  /*! \brief Decrements value of a node and returns *new* value. */
  uint32_t decr_value( node const& n ) const;
#pragma endregion

#pragma region Visited flags
  /*! \brief Reset all visited values to 0. */
  void clear_visited() const;

  /*! \brief Returns the visited value of a node. */
  uint32_t visited( node const& n ) const;

  /*! \brief Sets the visited value of a node. */
  void set_visited( node const& n, uint32_t v ) const;

  /*! \brief Returns the current traversal id. */
  uint32_t trav_id() const;

  /*! \brief Increment the current traversal id. */
  void incr_trav_id() const;
#pragma endregion

#pragma region General methods
  /*! \brief Returns network events object.
   *
   * Clients can register callbacks for network events to this object.  Events
   * include adding nodes, modifying nodes, and deleting nodes.
   */
  network_events<base_type>& events() const;
#pragma endregion
};

} /* namespace mockturtle */
