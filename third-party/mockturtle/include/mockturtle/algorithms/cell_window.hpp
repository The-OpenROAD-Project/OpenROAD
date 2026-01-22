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
  \file cell_window.hpp
  \brief Windowing in mapped network

  \author Bruno Schmitt
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#include <parallel_hashmap/phmap.h>

#include "../networks/detail/foreach.hpp"
#include "../traits.hpp"
#include "../utils/algorithm.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"

template<>
struct std::hash<std::array<uint64_t, 2>>
{
  auto operator()( std::array<uint64_t, 2> const& key ) const
  {
    std::hash<uint64_t> hasher;
    return hasher( key[0] ) * 31 + hasher( key[1] );
  }
};

namespace mockturtle
{

namespace detail
{

template<class Ntk>
class cell_window_storage
{
public:
  cell_window_storage( Ntk const& ntk ) : _cell_refs( ntk ),
                                          _cell_parents( ntk )
  {
    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
    {
      _num_constants++;
    }

    _nodes.reserve( _max_gates >> 1 );
    _gates.reserve( _max_gates );
  }

  phmap::flat_hash_set<node<Ntk>> _nodes;   /* cell roots in current window */
  phmap::flat_hash_set<node<Ntk>> _gates;   /* gates in current window */
  phmap::flat_hash_set<node<Ntk>> _leaves;  /* leaves of current window */
  phmap::flat_hash_set<signal<Ntk>> _roots; /* roots of current window */

  std::array<uint64_t, 2> _window_mask;
  phmap::flat_hash_set<std::array<uint64_t, 2>> _window_hash;

  node_map<uint32_t, Ntk> _cell_refs;                  /* ref counts for cells */
  node_map<std::vector<node<Ntk>>, Ntk> _cell_parents; /* parent cells */

  std::vector<node<Ntk>> _index_to_node;
  phmap::flat_hash_map<node<Ntk>, uint32_t> _node_to_index;

  uint32_t _num_constants{ 1u };
  uint32_t _max_gates{};
  bool _has_mapping{ true };
};

} // namespace detail

template<class Ntk>
class cell_window : public Ntk
{
public:
  using storage = typename std::shared_ptr<detail::cell_window_storage<Ntk>>;
  using node = typename Ntk::node;
  using signal = typename Ntk::signal;

public:
  cell_window( Ntk const& ntk, uint32_t max_gates = 64 )
      : Ntk( ntk ),
        _storage( std::make_shared<detail::cell_window_storage<Ntk>>( ntk ) )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_is_cell_root_v<Ntk>, "Ntk does not implement the is_cell_root method" );
    static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
    static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
    static_assert( has_foreach_cell_fanin_v<Ntk>, "Ntk does not implement the foreach_cell_fanin method" );
    static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
    static_assert( has_incr_trav_id_v<Ntk>, "Ntk does not implement the incr_trav_id method" );
    static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
    static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
    static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
    static_assert( has_has_mapping_v<Ntk>, "Ntk does not implement the has_mapping method" );

    assert( Ntk::has_mapping() );
    _storage->_max_gates = max_gates;
  }

  bool compute_window_for( node const& pivot )
  {
    init_cell_refs();

    // print_time<> pt;
    assert( Ntk::is_cell_root( pivot ) );

    // reset old window
    _storage->_nodes.clear();
    _storage->_gates.clear();

    std::vector<node> gates;
    gates.reserve( _storage->_max_gates );
    collect_mffc( pivot, gates );
    add_node( pivot, gates );

    if ( gates.size() > _storage->_max_gates )
    {
      assert( false );
    }

    std::optional<node> next;
    while ( ( next = find_next_pivot() ) )
    {
      gates.clear();
      collect_mffc( *next, gates );

      if ( _storage->_gates.size() + gates.size() > _storage->_max_gates )
      {
        break;
      }
      add_node( *next, gates );
    }

    find_leaves_and_roots();
    set_indexes();

    return _storage->_window_hash.insert( _storage->_window_mask ).second;
  }

  uint32_t num_pis() const
  {
    return _storage->_leaves.size();
  }

  uint32_t num_pos() const
  {
    return _storage->_roots.size();
  }

  uint32_t num_gates() const
  {
    return _storage->_gates.size();
  }

  uint32_t num_cells() const
  {
    return _storage->_nodes.size();
  }

  uint32_t size() const
  {
    return _storage->_num_constants + _storage->_leaves.size() + _storage->_gates.size();
  }

  bool is_pi( node const& n ) const
  {
    return _storage->_leaves.count( n );
  }

  bool is_cell_root( node const& n ) const
  {
    return _storage->_nodes.count( n );
  }

  uint32_t node_to_index( node const& n ) const
  {
    return _storage->_node_to_index.at( n );
  }

  node index_to_node( uint32_t index ) const
  {
    return _storage->_index_to_node[index];
  }

  bool has_mapping() const
  {
    return _storage->_has_mapping;
  }

  void clear_mapping()
  {
    _storage->_has_mapping = false;
    for ( auto const& n : _storage->_nodes )
    {
      Ntk::remove_from_mapping( n );
    }
    _storage->_nodes.clear();
  }

  template<typename LeavesIterator>
  void add_to_mapping( node const& n, LeavesIterator begin, LeavesIterator end )
  {
    _storage->_has_mapping = true;
    _storage->_nodes.insert( n );
    Ntk::add_to_mapping( n, begin, end );
  }

  template<typename Fn>
  void foreach_pi( Fn&& fn ) const
  {
    detail::foreach_element( _storage->_leaves.begin(), _storage->_leaves.end(), fn );
  }

  template<typename Fn>
  void foreach_po( Fn&& fn ) const
  {
    detail::foreach_element( _storage->_roots.begin(), _storage->_roots.end(), fn );
  }

  template<typename Fn>
  void foreach_gate( Fn&& fn ) const
  {
    detail::foreach_element( _storage->_gates.begin(), _storage->_gates.end(), fn );
  }

  template<typename Fn>
  void foreach_node( Fn&& fn ) const
  {
    detail::foreach_element( _storage->_index_to_node.begin(), _storage->_index_to_node.end(), fn );
  }

private:
  void init_cell_refs()
  {
    _storage->_cell_refs.reset();
    _storage->_cell_parents.reset();

    /* initial ref counts for cells */
    Ntk::foreach_gate( [&]( auto const& n ) {
      if ( Ntk::is_cell_root( n ) )
      {
        Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
          _storage->_cell_refs[n2]++;
          _storage->_cell_parents[n2].push_back( n );
        } );
    } } );
    Ntk::foreach_po( [&]( auto const& f ) {
      _storage->_cell_refs[f]++;
    } );
  }

  void collect_mffc( node const& pivot, std::vector<node>& gates )
  {
    Ntk::incr_trav_id();
    collect_gates( pivot, gates );
    const auto it = std::remove_if( gates.begin(), gates.end(), [&]( auto const& g ) { return _storage->_gates.count( g ); } );
    gates.erase( it, gates.end() );
  }

  void collect_gates( node const& pivot, std::vector<node>& gates )
  {
    assert( !Ntk::is_pi( pivot ) );

    Ntk::set_visited( Ntk::get_node( Ntk::get_constant( false ) ), Ntk::trav_id() );
    Ntk::set_visited( Ntk::get_node( Ntk::get_constant( true ) ), Ntk::trav_id() );

    Ntk::foreach_cell_fanin( pivot, [this]( auto const& n ) {
      Ntk::set_visited( n, Ntk::trav_id() );
    } );

    collect_gates_rec( pivot, gates );
  }

  void collect_gates_rec( node const& n, std::vector<node>& gates )
  {
    if ( Ntk::visited( n ) == Ntk::trav_id() )
      return;
    if ( Ntk::is_constant( n ) || Ntk::is_pi( n ) )
      return;

    Ntk::set_visited( n, Ntk::trav_id() );
    Ntk::foreach_fanin( n, [&]( auto const& f ) {
      collect_gates_rec( Ntk::get_node( f ), gates );
    } );
    gates.push_back( n );
  }

  void add_node( node const& pivot, std::vector<node> const& gates )
  {
    /*std::cout << "add_node(" << pivot << ", { ";
    for ( auto const& g : gates ) {
      std::cout << g << " ";
    }
    std::cout << "})\n";*/
    _storage->_nodes.insert( pivot );
    std::copy( gates.begin(), gates.end(), std::insert_iterator( _storage->_gates, _storage->_gates.begin() ) );
  }

  std::optional<node> find_next_pivot()
  {
    /* deref */
    for ( auto const& n : _storage->_nodes )
    {
      Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _storage->_cell_refs[n2]--;
      } );
    }

    std::vector<node> candidates;
    std::unordered_set<node> inputs;

    do
    {
      for ( auto const& n : _storage->_nodes )
      {
        Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
          if ( !_storage->_nodes.count( n2 ) && !Ntk::is_pi( n2 ) && !_storage->_cell_refs[n2] )
          {
            candidates.push_back( n2 );
            inputs.insert( n2 );
          }
        } );
      }

      if ( !candidates.empty() )
      {
        const auto best = max_element_unary(
            candidates.begin(), candidates.end(),
            [&]( auto const& cand ) {
              auto cnt{ 0 };
              this->foreach_cell_fanin( cand, [&]( auto const& n2 ) {
                cnt += inputs.count( n2 );
              } );
              return cnt;
            },
            -1 );
        candidates[0] = *best;
        break;
      }

      for ( auto const& n : _storage->_nodes )
      {
        Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
          if ( !_storage->_nodes.count( n2 ) && !Ntk::is_pi( n2 ) )
          {
            candidates.push_back( n2 );
            inputs.insert( n2 );
          }
        } );
      }

      for ( auto const& n : _storage->_nodes )
      {
        if ( _storage->_cell_refs[n] == 0 )
          continue;
        if ( _storage->_cell_refs[n] >= 5 )
          continue;
        if ( _storage->_cell_refs[n] == 1 && _storage->_cell_parents[n].size() == 1 && !_storage->_nodes.count( _storage->_cell_parents[n].front() ) )
        {
          candidates.clear();
          candidates.push_back( _storage->_cell_parents[n].front() );
          break;
        }
        std::copy_if( _storage->_cell_parents[n].begin(), _storage->_cell_parents[n].end(),
                      std::back_inserter( candidates ),
                      [&]( auto const& g ) {
                        return !_storage->_nodes.count( g );
                      } );
      }

      if ( !candidates.empty() )
      {
        const auto best = max_element_unary(
            candidates.begin(), candidates.end(),
            [&]( auto const& cand ) {
              auto cnt{ 0 };
              this->foreach_cell_fanin( cand, [&]( auto const& n2 ) {
                cnt += inputs.count( n2 );
              } );
              return cnt;
            },
            -1 );
        candidates[0] = *best;
      }
    } while ( false );

    /* ref */
    for ( auto const& n : _storage->_nodes )
    {
      Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _storage->_cell_refs[n2]++;
      } );
    }

    if ( candidates.empty() )
    {
      return std::nullopt;
    }
    else
    {
      return candidates.front();
    }
  }

  void find_leaves_and_roots()
  {
    _storage->_leaves.clear();
    _storage->_window_mask[0] = 0u;
    for ( auto const& g : _storage->_gates )
    {
      Ntk::foreach_fanin( g, [&]( auto const& f ) {
        auto const child = Ntk::get_node( f );
        if ( !_storage->_gates.count( child ) )
        {
          _storage->_leaves.insert( child );
          _storage->_window_mask[0] |= UINT64_C( 1 ) << ( Ntk::node_to_index( child ) % 64 );
        }
      } );
    }

    _storage->_roots.clear();
    _storage->_window_mask[1] = 0u;
    for ( auto const& n : _storage->_nodes )
    {
      Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _storage->_cell_refs[n2]--;
      } );
    }
    for ( auto const& n : _storage->_nodes )
    {
      if ( _storage->_cell_refs[n] )
      {
        _storage->_roots.insert( Ntk::make_signal( n ) );
        _storage->_window_mask[1] |= UINT64_C( 1 ) << ( Ntk::node_to_index( n ) % 64 );
      }
    }
    for ( auto const& n : _storage->_nodes )
    {
      Ntk::foreach_cell_fanin( n, [&]( auto const& n2 ) {
        _storage->_cell_refs[n2]++;
      } );
    }
  }

  void set_indexes()
  {
    _storage->_index_to_node.resize( _storage->_num_constants + _storage->_leaves.size() + _storage->_gates.size() );
    _storage->_node_to_index.clear();

    _storage->_node_to_index[_storage->_index_to_node[0] = Ntk::get_node( Ntk::get_constant( false ) )] = 0;

    if ( _storage->_num_constants == 2u )
    {
      _storage->_node_to_index[_storage->_index_to_node[1] = Ntk::get_node( Ntk::get_constant( true ) )] = 1;
    }

    auto idx = _storage->_num_constants;
    for ( auto const& n : _storage->_leaves )
    {
      _storage->_node_to_index[_storage->_index_to_node[idx] = n] = idx;
      ++idx;
    }
    for ( auto const& n : _storage->_gates )
    {
      _storage->_node_to_index[_storage->_index_to_node[idx] = n] = idx;
      ++idx;
    }

    assert( _storage->_index_to_node.size() == idx );
  }

private:
  std::shared_ptr<detail::cell_window_storage<Ntk>> _storage;
};

} // namespace mockturtle