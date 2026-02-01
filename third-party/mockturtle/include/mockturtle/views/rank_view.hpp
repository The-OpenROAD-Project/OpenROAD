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
 \file rank_view.hpp
 \brief Implements rank orders for a network

 \author Marcel Walter
*/

#pragma once

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "../utils/node_map.hpp"
#include "depth_view.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace mockturtle
{

/*! \brief Implements rank orders for a network.
*
* This view assigns manipulable relative orders to the nodes of each level.
* A sequence of nodes in the same level is called a rank. The width
* of a rank is thereby defined as the number of nodes in the rank. The width of
* the network is equal to the width of the widest rank. This view implements
* functions to retrieve the assigned node position within a rank (`rank_position`)
* as well as to fetch the node in a certain rank at a certain position (`at_rank_position`).
* The ranks are assigned at construction and can be manipulated by calling the `swap` function.
*
* This view also automatically inserts new nodes into their respective rank (at the end),
* however, it does not update the information, when modifying or deleting nodes.
*
* **Required network functions:**
* -  `foreach_node`
* -  `get_node`
* -  `num_pis`
* -  `is_ci`
* -  `is_constant`
*
* Example
*
  \verbatim embed:rst

  .. code-block:: c++

     // create network somehow
     aig_network aig = ...;

     // create a rank view on the network
     rank_view aig_rank{aig_depth};

     // print width
     std::cout << "Width: " << aig_rank.width() << "\n";
  \endverbatim
*/
template<class Ntk, bool has_rank_interface = has_rank_position_v<Ntk>&& has_at_rank_position_v<Ntk>&& has_swap_v<Ntk>&& has_width_v<Ntk>&& has_foreach_node_in_rank_v<Ntk>&& has_foreach_gate_in_rank_v<Ntk>>
class rank_view
{
};

template<class Ntk>
class rank_view<Ntk, true> : public depth_view<Ntk>
{
public:
  rank_view( Ntk const& ntk ) : depth_view<Ntk>( ntk )
  {
  }
};

template<class Ntk>
class rank_view<Ntk, false> : public depth_view<Ntk>
{
public:
  static constexpr bool is_topologically_sorted = true;
  using storage = typename Ntk::storage;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

  explicit rank_view()
      : depth_view<Ntk>(), rank_pos{ *this }, ranks{}, max_rank_width{ 0 }
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
    static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );

    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
  }

  /*! \brief Standard constructor.
   *
   * \param ntk Base network
   */
  explicit rank_view( Ntk const& ntk )
      : depth_view<Ntk>{ ntk }, rank_pos{ ntk }, ranks{ this->depth() + 1 }, max_rank_width{ 0 }
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_num_pis_v<Ntk>, "Ntk does not implement the num_pis method" );
    static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
    static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );

    init_ranks();

    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
  }

  /*! \brief Copy constructor. */
  rank_view( rank_view<Ntk, false> const& other )
      : depth_view<Ntk>( other ), rank_pos{ other.rank_pos }, ranks{ other.ranks }, max_rank_width{ other.max_rank_width }
  {
    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );
  }

  rank_view<Ntk, false>& operator=( rank_view<Ntk, false> const& other )
  {
    /* delete the event of this network */
    Ntk::events().release_add_event( add_event );

    /* update the base class */
    this->_storage = other._storage;
    this->_events = other._events;

    /* copy */
    rank_pos = other.rank_pos;
    ranks = other.ranks;
    max_rank_width = other.max_rank_width;

    /* register new event in the other network */
    add_event = Ntk::events().register_add_event( [this]( auto const& n ) { on_add( n ); } );

    return *this;
  }

  ~rank_view()
  {
    Ntk::events().release_add_event( add_event );
  }
  /**
   * \brief Returns the rank position of a node.
   *
   * @param n Node to get the rank position of.
   * @return Rank position of node `n`.
   */
  uint32_t rank_position( node const& n ) const noexcept
  {
    assert( !this->is_constant( n ) && "node must not be constant" );

    return rank_pos[n];
  }
  /**
   * \brief Returns the node at a certain rank position.
   *
   * @param level Level in the network, i.e., rank to get the node from.
   * @param pos Position in the rank to get the node from.
   * @return Node at position `pos` in rank `level`.
   */
  node at_rank_position( uint32_t const level, uint32_t const pos ) const noexcept
  {
    assert( level < ranks.size() && "level must be less than the number of ranks" );
    assert( pos < ranks[level].size() && "pos must be less than the number of nodes in rank" );

    return ranks[level][pos];
  }
  /**
   * \brief Returns the width of the widest rank in the network.
   *
   * @return Width of the widest rank in the network.
   */
  uint32_t width() const noexcept
  {
    return max_rank_width;
  }
  /**
   * \brief Swaps the positions of two nodes in the same rank.
   *
   * @param n1 First node to swap.
   * @param n2 Second node to swap.
   */
  void swap( node const& n1, node const& n2 ) noexcept
  {
    assert( this->level( n1 ) == this->level( n2 ) && "nodes must be in the same rank" );

    auto& pos1 = rank_pos[n1];
    auto& pos2 = rank_pos[n2];

    std::swap( ranks[this->level( n1 )][pos1], ranks[this->level( n2 )][pos2] );
    std::swap( pos1, pos2 );
  }
  /**
   * \brief Sorts the given rank according to a comparator.
   *
   * @tparam Cmp Functor type that compares two nodes. It needs to fulfill the requirements of `Compare` (named C++ requirement).
   * @param level The level of the rank to sort.
   * @param cmp The comparator to use.
   */
  template<typename Cmp>
  void sort_rank( uint32_t const level, Cmp const& cmp )
  {
    // level must be less than the number of ranks
    if ( level < ranks.size() )
    {
      auto& rank = ranks[level];

      std::stable_sort( rank.begin(), rank.end(), cmp );
      std::for_each( rank.cbegin(), rank.cend(), [this, i = 0u]( auto const& n ) mutable { rank_pos[n] = i++; } );
    }
  }
  /**
   * \brief Applies a given function to each node in the rank level in order.
   *
   * @tparam Fn Functor type.
   * @param level The rank to apply fn to.
   * @param fn The function to apply.
   */
  template<typename Fn>
  void foreach_node_in_rank( uint32_t const level, Fn&& fn ) const
  {
    // level must be less than the number of ranks
    if ( level < ranks.size() )
    {
      auto const& rank = ranks[level];

      detail::foreach_element( rank.cbegin(), rank.cend(), std::forward<Fn>( fn ) );
    }
  }
  /**
   * \brief Applies a given function to each node in rank order.
   *
   * This function overrides the `foreach_node` method of the base class.
   *
   * @tparam Fn Functor type.
   * @param fn The function to apply.
   */
  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    for ( auto l = 0; l < ranks.size(); ++l )
    {
      foreach_node_in_rank( l, std::forward<Fn>( fn ) );
    }
  }
  /**
   * \brief Applies a given function to each gate in the rank level in order.
   *
   * @tparam Fn Functor type.
   * @param level The rank to apply fn to.
   * @param fn The function to apply.
   */
  template<typename Fn>
  void foreach_gate_in_rank( uint32_t const level, Fn&& fn ) const
  {
    // level must be less than the number of ranks
    if ( level < ranks.size() )
    {
      auto const& rank = ranks[level];

      detail::foreach_element_if(
          rank.cbegin(), rank.cend(), [this]( auto const& n ) { return !this->is_ci( n ); }, std::forward<Fn>( fn ) );
    }
  }
  /**
   * \brief Applies a given function to each gate in rank order.
   *
   * This function overrides the `foreach_gate` method of the base class.
   *
   * @tparam Fn Functor type.
   * @param fn The function to apply.
   */
  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    for ( auto l = 0; l < ranks.size(); ++l )
    {
      foreach_gate_in_rank( l, std::forward<Fn>( fn ) );
    }
  }
  /**
   * \brief Applies a given function to each PI in rank order.
   *
   * This function overrides the `foreach_pi` method of the base class.
   *
   * @tparam Fn Functor type.
   * @param fn The function to apply.
   */
  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    std::vector<node> pis{};
    pis.reserve( this->num_pis() );

    depth_view<Ntk>::foreach_pi( [&pis]( auto const& pi ) { pis.push_back( pi ); } );
    std::stable_sort( pis.begin(), pis.end(), [this]( auto const& n1, auto const& n2 ) { return rank_pos[n1] < rank_pos[n2]; } );
    detail::foreach_element( pis.cbegin(), pis.cend(), std::forward<Fn>( fn ) );
  }
  /**
   * Overrides the base class method to also call the add_event on create_pi().
   *
   * @note This can (and in fact will) lead to issues if Ntk already calls add_event functions on create_pi()!
   *
   * @return Newly created PI signal.
   */
  signal create_pi()
  {
    auto const n = depth_view<Ntk>::create_pi();
    this->resize_levels();
    on_add( this->get_node( n ) );
    return n;
  }

private:
  node_map<uint32_t, Ntk> rank_pos;
  std::vector<std::vector<node>> ranks;
  uint32_t max_rank_width;

  std::shared_ptr<typename network_events<Ntk>::add_event_type> add_event;

  void insert_in_rank( node const& n ) noexcept
  {
    auto& rank = ranks[this->level( n )];
    rank_pos[n] = rank.size();
    rank.push_back( n );
    max_rank_width = std::max( max_rank_width, static_cast<uint32_t>( rank.size() ) );
  }

  void on_add( node const& n ) noexcept
  {
    if ( this->level( n ) >= ranks.size() )
    {
      // add sufficient ranks to store the new node
      ranks.insert( ranks.end(), this->level( n ) - ranks.size() + 1, {} );
    }
    rank_pos.resize();

    insert_in_rank( n );
  }

  void init_ranks() noexcept
  {
    depth_view<Ntk>::foreach_node( [this]( auto const& n ) {
     if (!this->is_constant(n))
     {
       insert_in_rank(n);
     } } );
  }
};

template<class T>
rank_view( T const& ) -> rank_view<T>;

} // namespace mockturtle
