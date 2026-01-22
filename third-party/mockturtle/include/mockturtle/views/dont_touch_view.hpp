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
  \file dont_touch_view.hpp
  \brief Select nodes to be "don't touch"

  \author Alessandro Tempia Calvino
*/

#pragma once

#include "../traits.hpp"

#include <type_traits>
#include <unordered_set>

namespace mockturtle
{

/*! \brief Mark nodes as "don't touch".
 *
 * This view adds methods to mark nodes as don't touch. A don't touch node
 * will be skipped during logic optimization or mapping.
 * It always adds the functions `select_dont_touch`, `remove_dont_touch`,
 * `is_dont_touch`.
 *
 * **Required network functions:**
 * - `size`
 *
 * Example
 *
   \verbatim embed:rst

   .. code-block:: c++

      // create network somehow
      klut_network klut = ...;
      dont_touch_view klut_dont_touch{ klut };

      // select dont touch nodes
      klut_dont_touch.select_dont_touch( 20 );

      // call technology mapping to map the rest of the network
      binding_view<klut_network> res = emap( klut_dont_touch, tech_lib );
   \endverbatim
 */
template<class Ntk>
class dont_touch_view : public Ntk
{
public:
  using node = typename Ntk::node;

public:
  explicit dont_touch_view()
      : Ntk(), _dont_touch()
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  }

  explicit dont_touch_view( Ntk const& ntk )
      : Ntk( ntk ), _dont_touch()
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  }

  dont_touch_view<Ntk>& operator=( dont_touch_view<Ntk> const& dont_touch_ntk )
  {
    Ntk::operator=( dont_touch_ntk );
    _dont_touch = dont_touch_ntk._dont_touch;
    return *this;
  }

  void select_dont_touch( node const& n )
  {
    _dont_touch.insert( Ntk::node_to_index( n ) );
  }

  void remove_dont_touch( node const& n )
  {
    if ( auto it = _dont_touch.find( Ntk::node_to_index( n ) ); it != _dont_touch.end() )
    {
      _dont_touch.erase( it );
    }
  }

  bool is_dont_touch( node const& n ) const
  {
    return _dont_touch.find( Ntk::node_to_index( n ) ) != _dont_touch.end();
  }

  template<typename Fn>
  void foreach_dont_touch( Fn&& fn ) const
  {
    constexpr auto is_bool_f = std::is_invocable_r_v<bool, Fn, node>;
    constexpr auto is_void_f = std::is_invocable_r_v<void, Fn, node>;

    for ( auto el : _dont_touch )
    {
      if constexpr ( is_bool_f )
      {
        if ( !fn( Ntk::index_to_node( el ) ) )
          return;
      }
      else
      {
        fn( Ntk::index_to_node( el ) );
      }
    }
  }

private:
  std::unordered_set<uint32_t> _dont_touch;
}; /* dont_touch_view */

template<class T>
dont_touch_view( T const& ) -> dont_touch_view<T>;

} // namespace mockturtle
