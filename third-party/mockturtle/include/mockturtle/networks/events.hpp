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
  \file events.hpp
  \brief Event API for updating a logic network.

  \author Heinz Riener
  \author Marcel Walter
  \author Mathias Soeken
*/

#pragma once

#include "../traits.hpp"

#include <functional>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>

namespace mockturtle
{

/*! \brief Network events.
 *
 * This data structure can be returned by a network.  Clients can add functions
 * to network events to call code whenever an event occurs.  Events are adding
 * a node, modifying a node, and deleting a node.
 */
template<class Ntk>
class network_events
{
public:
  using add_event_type = std::function<void( node<Ntk> const& n )>;
  using modified_event_type = std::function<void( node<Ntk> const& n, std::vector<signal<Ntk>> const& previous_children )>;
  using delete_event_type = std::function<void( node<Ntk> const& n )>;

public:
  std::shared_ptr<add_event_type> register_add_event( add_event_type const& fn )
  {
    auto pfn = std::make_shared<add_event_type>( fn );
    on_add.emplace_back( pfn );
    return pfn;
  }

  std::shared_ptr<modified_event_type> register_modified_event( modified_event_type const& fn )
  {
    auto pfn = std::make_shared<modified_event_type>( fn );
    on_modified.emplace_back( pfn );
    return pfn;
  }

  std::shared_ptr<delete_event_type> register_delete_event( delete_event_type const& fn )
  {
    auto pfn = std::make_shared<delete_event_type>( fn );
    on_delete.emplace_back( pfn );
    return pfn;
  }

  void release_add_event( std::shared_ptr<add_event_type>& fn )
  {
    /* first decrement the reference counter of the event */
    auto fn_ptr = fn.get();
    fn = nullptr;

    /* erase the event if the only instance remains in the vector */
    on_add.erase( std::remove_if( std::begin( on_add ), std::end( on_add ),
                                  [&]( auto&& event ) { return event.get() == fn_ptr && event.use_count() <= 1u; } ),
                  std::end( on_add ) );
  }

  void release_modified_event( std::shared_ptr<modified_event_type>& fn )
  {
    /* first decrement the reference counter of the event */
    auto fn_ptr = fn.get();
    fn = nullptr;

    /* erase the event if the only instance remains in the vector */
    on_modified.erase( std::remove_if( std::begin( on_modified ), std::end( on_modified ),
                                       [&]( auto&& event ) { return event.get() == fn_ptr && event.use_count() <= 1u; } ),
                       std::end( on_modified ) );
  }

  void release_delete_event( std::shared_ptr<delete_event_type>& fn )
  {
    /* first decrement the reference counter of the event */
    auto fn_ptr = fn.get();
    fn = nullptr;

    /* erase the event if the only instance remains in the vector */
    on_delete.erase( std::remove_if( std::begin( on_delete ), std::end( on_delete ),
                                     [&]( auto&& event ) { return event.get() == fn_ptr && event.use_count() <= 1u; } ),
                     std::end( on_delete ) );
  }

public:
  /*! \brief Event when node `n` is added. */
  std::vector<std::shared_ptr<add_event_type>> on_add;

  /*! \brief Event when `n` is modified.
   *
   * The event also informs about the previous children.  Note that the new
   * children are already available at the time the event is triggered.
   */
  std::vector<std::shared_ptr<modified_event_type>> on_modified;

  /*! \brief Event when `n` is deleted. */
  std::vector<std::shared_ptr<delete_event_type>> on_delete;
};

} // namespace mockturtle