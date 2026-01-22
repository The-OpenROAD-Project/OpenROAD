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
  \file dag_util.hpp
  \brief Utilities for DAG generation

  \author Dewmini Sudara Marakkalage
*/

#pragma once

#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "../../../utils/hash_functions.hpp"

namespace mockturtle
{

namespace detail
{

/*! \brief Computes and returns the frequency map for a given collection of elements.
 *  Use std::map instead of std::unordered_map because we use it as a key in a hash-table so the order is important to compute the hash
 */
template<typename ElemT>
inline std::map<ElemT, uint32_t> get_frequencies( const std::vector<ElemT>& elems )
{
  std::map<ElemT, uint32_t> elem_counts;
  std::for_each( elems.begin(), elems.end(), [&elem_counts]( auto e ) { elem_counts[e]++; } );
  return elem_counts;
}

template<typename ElemT>
class partition_generator
{
  using part = std::multiset<ElemT>;
  using partition = std::multiset<part>;
  using partition_set = std::set<partition>;

  using inner_cache_key_t = std::vector<ElemT>;
  using inner_cache_t = std::unordered_map<inner_cache_key_t, partition_set, hash<inner_cache_key_t>>;

  using outer_cache_key_t = std::tuple<std::vector<uint32_t>, uint32_t, uint32_t>;
  using outer_cache_t = std::unordered_map<outer_cache_key_t, inner_cache_t, hash<outer_cache_key_t>>;

public:
  /*! \brief Computes and returns a set of partitions for a given list of elements
   * such that no part contains any element `e` more than `max_counts[e]` times.
   */
  partition_set operator()(
      std::vector<int> elems,
      const std::vector<uint32_t>& max_counts = {},
      uint32_t max_parts = 0,
      uint32_t max_part_size = 0 )
  {
    _elems = elems;
    _max_counts = max_counts;
    _max_parts = max_parts;
    _max_part_size = max_part_size;

    const outer_cache_key_t key = { _max_counts, _max_parts, _max_part_size };
    partition_cache = outer_cache.insert( { key, inner_cache_t() } ).first;

    return get_all_partitions();
  }

private:
  outer_cache_t outer_cache;

  typename outer_cache_t::iterator partition_cache;
  std::vector<ElemT> _elems;
  std::vector<uint32_t> _max_counts;
  uint32_t _max_parts;
  uint32_t _max_part_size;

  partition_set get_all_partitions()
  {
    if ( _elems.size() == 0 )
    {
      return { {} }; // return the empty partition.
    }

    inner_cache_key_t key = _elems;
    if ( partition_cache->second.count( key ) )
    {
      return partition_cache->second.at( key );
    }

    partition_set result;

    auto last = _elems.back();
    _elems.pop_back();

    auto temp = get_all_partitions();

    for ( auto&& t : temp )
    {
      partition cpy;

      // take 'last' in its own partition
      cpy = t;

      if ( _max_parts == 0u || _max_parts > cpy.size() )
      {
        cpy.insert( { last } );
        result.insert( cpy );
      }

      // add 'last' to one of the existing partitions
      for ( auto it = t.begin(); it != t.end(); )
      {
        if ( _max_counts.empty() || it->count( last ) < _max_counts[last] )
        {

          if ( _max_part_size == 0 || _max_part_size > it->size() )
          {
            cpy = t;
            auto elem_it = cpy.find( *it );
            auto cpy_elem = *elem_it;
            cpy_elem.insert( last );
            cpy.erase( elem_it );
            cpy.insert( cpy_elem );
            result.insert( cpy );
          }
        }

        std::advance( it, t.count( *it ) );
      }
    }

    return ( partition_cache->second[key] = result );
  }
};

template<typename ElemT>
class partition_extender
{
  using part = std::multiset<ElemT>;
  using partition = std::multiset<part>;
  using partition_set = std::set<partition>;

  using inner_cache_key_t = std::vector<ElemT>;
  using inner_cache_t = std::unordered_map<inner_cache_key_t, partition_set, hash<inner_cache_key_t>>;

  using outer_cache_key_t = std::tuple<partition, std::vector<uint32_t>, uint32_t>;
  using outer_cache_t = std::map<outer_cache_key_t, inner_cache_t>;

public:
  /*! \brief Compute a list of different partitions that can be obtained by adding elements
   * in `elems` to the parts of `base` such that no part contains any element `e` more than
   * `max_counts[e]` times
   */
  partition_set operator()( std::vector<ElemT> elems, partition base, const std::vector<uint32_t>& max_counts, uint32_t max_part_size = 0 )
  {
    _elems = elems;
    _base = base;
    _max_counts = max_counts;
    _max_part_size = max_part_size;

    const outer_cache_key_t key = { _base, _max_counts, _max_part_size };
    partition_cache = outer_cache.insert( { key, inner_cache_t() } ).first;

    return extend_partitions();
  }

private:
  outer_cache_t outer_cache;

  typename outer_cache_t::iterator partition_cache;
  std::vector<ElemT> _elems;
  partition _base;
  std::vector<uint32_t> _max_counts;
  uint32_t _max_part_size;

  partition_set extend_partitions()
  {
    if ( _elems.size() == 0 )
    {
      return { _base };
    }

    inner_cache_key_t key = _elems;
    if ( partition_cache->second.count( key ) )
    {
      return partition_cache->second.at( key );
    }

    partition_set result;

    auto last = _elems.back();
    _elems.pop_back();

    auto temp = extend_partitions();
    for ( auto&& t : temp )
    {
      partition cpy;

      for ( auto it = t.begin(); it != t.end(); )
      {
        if ( it->count( last ) < _max_counts.at( last ) )
        {

          if ( _max_part_size == 0 || _max_part_size > it->size() )
          {
            cpy = t;
            auto elem_it = cpy.find( *it );
            auto cpy_elem = *elem_it;
            cpy_elem.insert( last );
            cpy.erase( elem_it );
            cpy.insert( cpy_elem );
            result.insert( cpy );
          }
        }

        std::advance( it, t.count( *it ) );
      }
    }

    return ( partition_cache->second[key] = result );
  }
};

template<typename ElemT>
struct sublist_generator
{
  using sub_list_cache_key_t = std::map<ElemT, uint32_t>;

public:
  /**
   * \brief Given a list of elements `elems`, generate all sub lists of those elements.
   * Ex: if `elems` = [1, 2, 2, 3], this will generate the following lists:
   * [0], [1], [1, 2], [1, 2, 2], [1, 2, 2, 3], [1, 2, 3], [1, 3], [2], [2, 2], [2, 2, 3], [2, 3], and [3].
   */
  std::set<std::vector<ElemT>> operator()( std::vector<ElemT> elems )
  {
    elem_counts = get_frequencies( elems );
    return get_sub_lists_recur();
  }

private:
  std::unordered_map<sub_list_cache_key_t, std::set<std::vector<ElemT>>, hash<sub_list_cache_key_t>> sub_list_cache;
  std::map<ElemT, uint32_t> elem_counts;

  std::set<std::vector<ElemT>> get_sub_lists_recur()
  {
    if ( elem_counts.size() == 0u )
    {
      return { {} };
    }

    sub_list_cache_key_t key = elem_counts;
    if ( !sub_list_cache.count( key ) )
    {
      auto last = std::prev( elem_counts.end() );
      auto last_elem = last->first;
      auto last_count = last->second;
      elem_counts.erase( last );

      std::set<std::vector<int>> result;

      std::vector<int> t;
      for ( auto i = last_count; i > 0; --i )
      {
        t.push_back( last_elem );
        result.insert( t ); // insert a copy of t, and note that t is already sorted.
      }

      auto temp = get_sub_lists_recur();

      for ( std::vector<int> t : temp )
      {
        result.insert( t );
        for ( auto i = last_count; i > 0; --i )
        {
          t.push_back( last_elem );
          std::stable_sort( t.begin(), t.end() );
          result.insert( t );
        }
      }

      sub_list_cache[key] = result;
    }

    return sub_list_cache[key];
  }
};

} // namespace detail

} // namespace mockturtle