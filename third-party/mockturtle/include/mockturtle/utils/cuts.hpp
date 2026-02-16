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
  \file cuts.hpp
  \brief Data structure for cuts

  \author Alessandro Tempia Calvino
  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <array>
#include <cassert>
#include <iostream>
#include <iterator>

#include <kitty/detail/mscfix.hpp>

#include "algorithm.hpp"

namespace mockturtle
{

struct empty_cut_data
{
};

/*! \brief A data-structure to hold a cut.
 *
 * The cut class is specialized via two template arguments, `MaxLeaves` and `T`.
 * `MaxLeaves` controls the maximum number of leaves a cut can hold.  To
 * guarantee an efficient implementation, this value should be \f$k \cdot l\f$,
 * where \f$k\f$ is the maximum cut size and \f$l\f$ is the maximum fanin size
 * of a gate in the logic network.  The second template argument `T` can be a
 * type for which a data entry is created in the cut to store additional data,
 * e.g., to compute the cost of a cut.  It defaults to `empty_cut_data`, which
 * is an empty struct that does not consume memory.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      std::vector<uint32_t> data{1, 2, 3, 4};

      cut<10> cut1;
      cut1.set_leaves( data.begin(), data.end() );

      cut<10, uint32_t> cut2;
      cut2.set_leaves( data.begin(), data.end() );
      cut2.data() = 42u;

      struct cut_data { uint32_t costs; };
      cut<20, cut_data> c3;
      c3.set_leaves( std::vector<uint32_t>{1, 2, 3} );
      c3->costs = 37u;
   \endverbatim
 */
template<int MaxLeaves, typename T = empty_cut_data>
class cut
{
public:
  /*! \brief Default constructor.
   */
  cut() = default;

  /*! \brief Copy constructor.
   *
   * Copies leaves, length, signature, and data.
   *
   * \param other Other cut
   */
  cut( cut const& other )
  {
    _cend = _end = std::copy( other.begin(), other.end(), _leaves.begin() );
    _length = other._length;
    _signature = other._signature;
    _data = other._data;
  }

  /*! \brief Assignment operator.
   *
   * Copies leaves, length, signature, and data.
   *
   * \param other Other cut
   */
  cut& operator=( cut const& other );

  /*! \brief Sets leaves (using iterators).
   *
   * \param begin Begin iterator to leaves
   * \param end End iterator to leaves (exclusive)
   */
  template<typename Iterator>
  void set_leaves( Iterator begin, Iterator end );

  /*! \brief Sets leaves (using container).
   *
   * Convenience function, which extracts the begin and end iterators from the
   * container.
   */
  template<typename Container>
  void set_leaves( Container const& c );

  /*! \brief Add leaves (using iterators).
   *
   * \param begin Begin iterator to leaves
   * \param end End iterator to leaves (exclusive)
   */
  template<typename Iterator>
  void add_leaves( Iterator begin, Iterator end );

  /*! \brief Signature of the cut. */
  auto signature() const { return _signature; }

  /*! \brief Returns the size of the cut (number of leaves). */
  auto size() const { return _length; }

  /*! \brief Begin iterator (constant). */
  auto begin() const { return _leaves.begin(); }

  /*! \brief End iterator (constant). */
  auto end() const { return _cend; }

  /*! \brief Begin iterator (mutable). */
  auto begin() { return _leaves.begin(); }

  /*! \brief End iterator (mutable). */
  auto end() { return _end; }

  /*! \brief Access to data (mutable). */
  T* operator->() { return &_data; }

  /*! \brief Access to data (constant). */
  T const* operator->() const { return &_data; }

  /*! \brief Access to data (mutable). */
  T& data() { return _data; }

  /*! \brief Access to data (constant). */
  T const& data() const { return _data; }

  /*! \brief Checks whether the cut is a subset of another cut.
   *
   * If \f$L_1\f$ are the leaves of the current cut and \f$L_2\f$ are the leaves
   * of `that`, then this method returns true if and only if
   * \f$L_1 \subseteq L_2\f$.
   *
   * \param that Other cut
   */
  bool dominates( cut const& that ) const;

  /*! \brief Merges two cuts.
   *
   * This method merges two cuts and stores the result in `res`.  The merge of
   * two cuts is the union \f$L_1 \cup L_2\f$ of the two leaf sets \f$L_1\f$ of
   * the cut and \f$L_2\f$ of `that`.  The merge is only successful if the
   * union has not more than `cut_size` elements.  In that case, the function
   * returns `false`, otherwise `true`.
   *
   * \param that Other cut
   * \param res Resulting cut
   * \param cut_size Maximum cut size
   * \return True, if resulting cut is small enough
   */
  bool merge( cut const& that, cut& res, uint32_t cut_size ) const;

private:
  std::array<uint32_t, MaxLeaves> _leaves;
  uint32_t _length;
  uint64_t _signature;
  typename std::array<uint32_t, MaxLeaves>::const_iterator _cend;
  typename std::array<uint32_t, MaxLeaves>::iterator _end;

  T _data;
};

/*! \brief Compare two cuts.
 *
 * Default comparison function for two cuts.  A cut is smaller than another
 * cut, if it has fewer leaves.
 *
 * This function should be specialized for custom cuts, if additional data
 * changes the cost of a cut.
 */
template<int MaxLeaves, typename T>
bool operator<( cut<MaxLeaves, T> const& c1, cut<MaxLeaves, T> const& c2 )
{
  return c1.size() < c2.size();
}

/*! \brief Prints a cut.
 */
template<int MaxLeaves, typename T>
std::ostream& operator<<( std::ostream& os, cut<MaxLeaves, T> const& c )
{
  os << "{ ";
  std::copy( c.begin(), c.end(), std::ostream_iterator<uint32_t>( os, " " ) );
  os << "}";
  return os;
}

template<int MaxLeaves, typename T>
cut<MaxLeaves, T>& cut<MaxLeaves, T>::operator=( cut<MaxLeaves, T> const& other )
{
  if ( &other != this )
  {
    _cend = _end = std::copy( other.begin(), other.end(), _leaves.begin() );
    _length = other._length;
    _signature = other._signature;
    _data = other._data;
  }
  return *this;
}

template<int MaxLeaves, typename T>
template<typename Iterator>
void cut<MaxLeaves, T>::set_leaves( Iterator begin, Iterator end )
{
  _cend = _end = std::copy( begin, end, _leaves.begin() );
  _length = static_cast<uint32_t>( std::distance( begin, end ) );
  _signature = 0;

  while ( begin != end )
  {
    _signature |= UINT64_C( 1 ) << ( *begin++ & 0x3f );
  }
}

template<int MaxLeaves, typename T>
template<typename Container>
void cut<MaxLeaves, T>::set_leaves( Container const& c )
{
  set_leaves( std::begin( c ), std::end( c ) );
}

template<int MaxLeaves, typename T>
template<typename Iterator>
void cut<MaxLeaves, T>::add_leaves( Iterator begin, Iterator end )
{
  _cend = _end = std::copy( begin, end, _end );
  _length = static_cast<uint32_t>( std::distance( _leaves.begin(), _end ) );

  while ( begin != end )
  {
    _signature |= UINT64_C( 1 ) << ( *begin++ & 0x3f );
  }
}

template<int MaxLeaves, typename T>
bool cut<MaxLeaves, T>::dominates( cut const& that ) const
{
  /* quick check for counter example */
  if ( _length > that._length || ( _signature & that._signature ) != _signature )
  {
    return false;
  }

  if ( _length == that._length )
  {
    return std::equal( begin(), end(), that.begin() );
  }

  if ( _length == 0 )
  {
    return true;
  }

  // this is basically
  //     return std::includes( that.begin(), that.end(), begin(), end() )
  // but it turns out that this code is faster compared to the standard
  // implementation.
  for ( auto it2 = that.begin(), it1 = begin(); it2 != that.end(); ++it2 )
  {
    if ( *it2 > *it1 )
    {
      return false;
    }
    if ( ( *it2 == *it1 ) && ( ++it1 == end() ) )
    {
      return true;
    }
  }

  return false;
}

template<int MaxLeaves, typename T>
bool cut<MaxLeaves, T>::merge( cut const& that, cut& res, uint32_t cut_size ) const
{
  if ( _length + that._length > cut_size )
  {
    const auto sign = _signature + that._signature;
    if ( uint32_t( __builtin_popcount( static_cast<uint32_t>( sign & 0xffffffff ) ) ) + uint32_t( __builtin_popcount( static_cast<uint32_t>( sign >> 32 ) ) ) > cut_size )
    {
      return false;
    }
  }

  int32_t length = set_union_safe( begin(), end(), that.begin(), that.end(), res.begin(), cut_size );
  if ( length >= 0 )
  {
    res._cend = res._end = res.begin() + length;
    res._length = static_cast<uint32_t>( length );
    res._signature = _signature | that._signature;
    return true;
  }
  return false;
}

/*! \brief A data-structure to hold a set of cuts.
 *
 * The aim of a cut set is to contain cuts and maintain two properties.  First,
 * all cuts are ordered according to the `<` operator, and second, all cuts
 * are irredundant, i.e., no cut in the set dominates another cut in the set.
 *
 * The cut set is defined using the `CutType` of cuts it should hold and a
 * maximum number of cuts it can hold.  No check is performed whether a cut set
 * is full, and therefore the caller must not insert cuts into a full set.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      cut_set<cut<10>, 30> cuts;

      cut<10> c1, c2, c3, c4;

      c1.set_leaves( {1, 2, 3} );
      c2.set_leaves( {4, 5} );
      c3.set_leaves( {1, 2} );
      c4.set_leaves( {1, 3, 4} );

      cuts.insert( c1 );
      cuts.insert( c2 );
      cuts.insert( c3 );
      cuts.insert( c4 );

      assert( cuts.size() == 3 );

      std::cout << cuts << std::endl;

      // will print:
      //   { 4, 5 }
      //   { 1, 2 }
      //   { 1, 3, 4 }
   \endverbatim
 */
template<typename CutType, int MaxCuts>
class cut_set
{
public:
  /*! \brief Standard constructor.
   */
  cut_set();

  /*! \brief Clears a cut set.
   */
  void clear();

  /*! \brief Adds a cut to the end of the set.
   *
   * This function should only be called to create a set of cuts which is known
   * to be sorted and irredundant (i.e., no cut in the set dominates another
   * cut).
   *
   * \param begin Begin iterator to leaf indexes
   * \param end End iterator (exclusive) to leaf indexes
   * \return Reference to the added cut
   */
  template<typename Iterator>
  CutType& add_cut( Iterator begin, Iterator end );

  /*! \brief Checks whether cut is dominates by any cut in the set.
   *
   * \param cut Cut outside of the set
   */
  bool is_dominated( CutType const& cut ) const;

  /*! \brief Inserts a cut into a set.
   *
   * This method will insert a cut into a set and maintain an order.  Before the
   * cut is inserted into the correct position, it will remove all cuts that are
   * dominated by `cut`.
   *
   * If `cut` is dominated by any of the cuts in the set, it will still be
   * inserted.  The caller is responsible to check whether `cut` is dominated
   * before inserting it into the set.
   *
   * \param cut Cut to insert.
   */
  void insert( CutType const& cut );

  /*! \brief Begin iterator (constant).
   *
   * The iterator will point to a cut pointer.
   */
  auto begin() const { return _pcuts.begin(); }

  /*! \brief End iterator (constant). */
  auto end() const { return _pcend; }

  /*! \brief Begin iterator (mutable).
   *
   * The iterator will point to a cut pointer.
   */
  auto begin() { return _pcuts.begin(); }

  /*! \brief End iterator (mutable). */
  auto end() { return _pend; }

  /*! \brief Number of cuts in the set. */
  auto size() const { return _pcend - _pcuts.begin(); }

  /*! \brief Returns reference to cut at index.
   *
   * This function does not return the cut pointer but dereferences it and
   * returns a reference.  The function does not check whether index is in the
   * valid range.
   *
   * \param index Index
   */
  auto const& operator[]( uint32_t index ) const { return *_pcuts[index]; }

  /*! \brief Returns the best cut, i.e., the first cut.
   */
  auto const& best() const { return *_pcuts[0]; }

  /*! \brief Updates the best cut.
   *
   * This method will set the cut at index `index` to be the best cut.  All
   * cuts before `index` will be moved one position higher.
   *
   * \param index Index of new best cut
   */
  void update_best( uint32_t index );

  /*! \brief Resize the cut set, if it is too large.
   *
   * This method will resize the cut set to `size` only if the cut set has more
   * than `size` elements.  Otherwise, the size will remain the same.
   */
  void limit( uint32_t size );

  /*! \brief Prints a cut set. */
  friend std::ostream& operator<<( std::ostream& os, cut_set const& set )
  {
    for ( auto const& c : set )
    {
      os << *c << "\n";
    }
    return os;
  }

private:
  std::array<CutType, MaxCuts> _cuts;
  std::array<CutType*, MaxCuts> _pcuts;
  typename std::array<CutType*, MaxCuts>::const_iterator _pcend{ _pcuts.begin() };
  typename std::array<CutType*, MaxCuts>::iterator _pend{ _pcuts.begin() };
};

template<typename CutType, int MaxCuts>
cut_set<CutType, MaxCuts>::cut_set()
{
  clear();
}

template<typename CutType, int MaxCuts>
void cut_set<CutType, MaxCuts>::clear()
{
  _pcend = _pend = _pcuts.begin();
  auto pit = _pcuts.begin();
  for ( auto& c : _cuts )
  {
    *pit++ = &c;
  }
}

template<typename CutType, int MaxCuts>
template<typename Iterator>
CutType& cut_set<CutType, MaxCuts>::add_cut( Iterator begin, Iterator end )
{
  assert( _pend != _pcuts.end() );

  auto& cut = **_pend++;
  cut.set_leaves( begin, end );

  ++_pcend;
  return cut;
}

template<typename CutType, int MaxCuts>
bool cut_set<CutType, MaxCuts>::is_dominated( CutType const& cut ) const
{
  return std::find_if( _pcuts.begin(), _pcend, [&cut]( auto const* other ) { return other->dominates( cut ); } ) != _pcend;
}

template<typename CutType, int MaxCuts>
void cut_set<CutType, MaxCuts>::insert( CutType const& cut )
{
  /* remove elements that are dominated by new cut */
  _pcend = _pend = std::stable_partition( _pcuts.begin(), _pend, [&cut]( auto const* other ) { return !cut.dominates( *other ); } );

  /* insert cut in a sorted way */
  auto ipos = std::lower_bound( _pcuts.begin(), _pend, &cut, []( auto a, auto b ) { return *a < *b; } );

  /* too many cuts, we need to remove one */
  if ( _pend == _pcuts.end() )
  {
    /* cut to be inserted is worse than all the others, return */
    if ( ipos == _pend )
    {
      return;
    }
    else
    {
      /* remove last cut */
      --_pend;
      --_pcend;
    }
  }

  /* copy cut */
  auto& icut = *_pend;
  icut->set_leaves( cut.begin(), cut.end() );
  icut->data() = cut.data();

  if ( ipos != _pend )
  {
    auto it = _pend;
    while ( it > ipos )
    {
      std::swap( *it, *( it - 1 ) );
      --it;
    }
  }

  /* update iterators */
  _pcend++;
  _pend++;
}

template<typename CutType, int MaxCuts>
void cut_set<CutType, MaxCuts>::update_best( uint32_t index )
{
  auto* best = _pcuts[index];
  for ( auto i = index; i > 0; --i )
  {
    _pcuts[i] = _pcuts[i - 1];
  }
  _pcuts[0] = best;
}

template<typename CutType, int MaxCuts>
void cut_set<CutType, MaxCuts>::limit( uint32_t size )
{
  if ( std::distance( _pcuts.begin(), _pend ) > static_cast<long>( size ) )
  {
    _pcend = _pend = _pcuts.begin() + size;
  }
}

} /* namespace mockturtle */
