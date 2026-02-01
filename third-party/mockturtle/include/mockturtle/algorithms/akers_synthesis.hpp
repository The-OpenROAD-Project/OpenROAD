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
  \file akers_synthesis.hpp
  \brief Akers synthesis

  \author Alessandro Tempia Calvino
  \author Eleonora Testa
  \author Heinz Riener
  \author Marcel Walter
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <kitty/bit_operations.hpp>
#include <kitty/constructors.hpp>
#include <kitty/cube.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include "../traits.hpp"

namespace mockturtle
{

/* cube operators needed from Kitty :
- & (bitwise and )
- is_subset_of
- is_const0
*/

inline kitty::cube operator&( kitty::cube a, kitty::cube b )
{
  assert( a.num_literals() == b.num_literals() );
  kitty::cube c;
  for ( auto h = 0; h < a.num_literals(); h++ )
  {
    c.add_literal( h, a.get_bit( h ) & b.get_bit( h ) );
  }
  return c;
}

inline bool is_subset_of( kitty::cube a, kitty::cube b )
{
  assert( a.num_literals() == b.num_literals() );
  for ( auto h = 0; h < a.num_literals(); h++ )
  {
    if ( ( a.get_bit( h ) == 1 ) && ( b.get_bit( h ) == 1 ) )
      continue;
    else if ( ( a.get_bit( h ) == 1 ) && ( b.get_bit( h ) == 0 ) )
      return false;
  }
  return true;
}

inline bool all_zeros( kitty::cube a )
{
  for ( auto g = 0; g < a.num_literals(); g++ )
  {
    if ( a.get_bit( g ) == 0 )
      continue;
    else
      return false;
  }
  return true;
}

class unitized_table
{
public:
  using row_t = kitty::cube;

  unitized_table( const std::string& columns )
      : columns( columns )
  {
  }

  row_t create_row() const
  {
    row_t r( 0, ( 1 << columns.size() ) - 1 );
    // for ( auto i = 0u; i < columns.size(); i++ )
    // r.add_literal( i, 1 );
    return r;
  }

  row_t create_mask() const
  {
    row_t r( ( 1 << columns.size() ) - 1, ( 1 << columns.size() ) - 1 );
    // for ( auto i = 0u; i < columns.size(); i++ )
    // r.add_literal( i, 0 );
    return r;
  }

  void add_row( const row_t& row )
  {
    rows.push_back( row );
  }

  void reduce()
  {
    auto progress{ true };

    while ( progress )
    {
      progress = reduce_columns();
      progress = progress || reduce_rows();
    }
  }

  friend std::ostream& operator<<( std::ostream& os, const unitized_table& table );

  inline std::vector<row_t>::const_iterator begin() const
  {
    return rows.begin();
  }

  inline std::vector<row_t>::const_iterator end() const
  {
    return rows.end();
  }

  inline int operator[]( unsigned index ) const
  {
    return (int)(unsigned char)columns[index];
  }

  inline bool is_opposite( unsigned c1, unsigned c2 ) const
  {
    const auto _n1 = columns[c1];
    const auto _n2 = columns[c2];

    const auto n1 = std::min( _n1, _n2 );
    const auto n2 = std::max( _n1, _n2 );

    return ( n1 == 0x30 && n2 == 0x31 ) || ( ( n1 + 0x20 ) == n2 );
  }

  inline auto num_columns() const
  {
    return columns.size();
  }

  int add_gate( const std::set<unsigned>& gate )
  {
    assert( gate.size() == 3u );

    auto it = gate.begin();
    const auto c1 = *it++;
    const auto c2 = *it++;
    const auto c3 = *it;

    for ( auto& r : rows )
    {
      const auto bi1 = r.get_bit( c1 );
      const auto bi2 = r.get_bit( c2 );
      const auto bi3 = r.get_bit( c3 );

      r.add_literal( r.num_literals(), ( bi1 && bi2 ) || ( bi1 && bi3 ) || ( bi2 && bi3 ) );
    }

    auto ret = (int)next_gate_id;
    columns.push_back( next_gate_id++ );
    return ret;
  }

  unsigned count_essential_ones( bool skip_last_column = true ) const
  {
    auto count = 0u;
    auto end = columns.size();
    if ( skip_last_column )
    {
      --end;
    }

    for ( auto column = 0u; column < end; ++column )
    {
      std::vector<row_t> one_rows;

      /* find rows with a 1 at column */
      for ( const auto& row : rows )
      {
        if ( row.get_bit( column ) )
        {
          auto rt = row;
          rt.clear_bit( column );
          one_rows.push_back( rt );
        }
      }

      /* find essential rows */
      for ( auto i = 0u; i < one_rows.size(); ++i )
      {
        for ( auto j = 0u; j < one_rows.size(); ++j )
        {
          if ( i == j )
          {
            continue;
          }
          if ( all_zeros( one_rows[i] & one_rows[j] ) )
          {
            ++count; /* entry i is essential in column */
            break;
          }
        }
      }
    }

    return count;
  }

private:
  bool reduce_rows()
  {
    std::vector<unsigned> to_be_removed;

    for ( auto i = 0u; i < rows.size(); ++i )
    {
      for ( auto j = i + 1u; j < rows.size(); ++j )
      {
        if ( rows[i] == rows[j] )
        {
          to_be_removed.push_back( i );
        }
        else
        {
          if ( is_subset_of( rows[i], rows[j] ) )
          {
            to_be_removed.push_back( j );
          }
          if ( is_subset_of( rows[j], rows[i] ) )
          {
            to_be_removed.push_back( i );
          }
        }
      }
    }

    std::stable_sort( to_be_removed.begin(), to_be_removed.end() );
    to_be_removed.erase( std::unique( to_be_removed.begin(), to_be_removed.end() ), to_be_removed.end() );

    std::reverse( std::begin( to_be_removed ), std::end( to_be_removed ) );

    for ( auto index : to_be_removed )
    {
      rows.erase( rows.begin() + index );
    }
    return !to_be_removed.empty();
  }

  bool reduce_columns()
  {
    std::vector<unsigned> to_be_removed;

    auto mask = create_mask();

    for ( auto c = 0u; c < columns.size(); ++c )
    {
      mask.clear_bit( c );
      auto can_be_removed = true;

      /* pretend column c is removed */
      for ( auto i = 0u; i < rows.size(); ++i )
      {
        for ( auto j = i + 1u; j < rows.size(); ++j )
        {
          const auto result = ( rows[i] & rows[j] ) & mask;
          if ( all_zeros( result ) )
          {
            can_be_removed = false;
            break;
          }
        }

        if ( !can_be_removed )
        {
          break;
        }
      }

      if ( can_be_removed )
      {
        to_be_removed.push_back( c );
      }
      else
      {
        mask.set_bit( c );
      }
    }

    /* remove columns */
    std::reverse( to_be_removed.begin(), to_be_removed.end() );

    for ( auto index : to_be_removed )
    {
      columns.erase( columns.begin() + index );

      for ( auto& row : rows )
      {
        erase_bit( row, index );
      }
    }

    return !to_be_removed.empty();
  }

  void erase_bit( row_t& row, unsigned pos ) const
  {
    for ( auto i = pos + 1; i < unsigned( row.num_literals() ); ++i )
    {
      if ( row.get_bit( i ) == 1 )
        row.set_bit( i - 1u );
      else
        row.clear_bit( i - 1u );
    }
    row.remove_literal( row.num_literals() - 1u );
  }

public:
  std::string columns;

  std::vector<row_t> rows;

  unsigned char next_gate_id = 'z' + 0x21;
};

inline std::ostream& operator<<( std::ostream& os, const unitized_table& table )
{
  os << table.columns << std::endl;

  for ( const auto& row : table.rows )
  {
    std::string buffer;
    std::bitset<32> to_print;
    for ( auto i = 0; i < row.num_literals(); i++ )
      to_print[i] = row.get_bit( i );
    buffer = to_print.to_string();
    std::string reversed = buffer;
    std::reverse( reversed.begin(), reversed.end() );
    os << reversed << std::endl;
  }

  return os;
}

inline bool operator==( unitized_table const& table, unitized_table const& original_table )
{
  if ( table.rows.size() != original_table.rows.size() )
    return false;
  for ( auto x = 0u; x < table.rows.size(); x++ )
  {
    if ( table.rows[x] == original_table.rows[x] )
      continue;
    else
      return false;
  }

  return true;
}

namespace detail
{
template<typename Ntk, typename LeavesIterator>
class akers_synthesis_impl
{
public:
  akers_synthesis_impl( Ntk ntk, kitty::dynamic_truth_table const& func, kitty::dynamic_truth_table const& care, LeavesIterator begin, LeavesIterator end )
      : ntk( ntk ),
        func( func ),
        care( care ),
        begin( begin ),
        end( end )
  {
  }

public:
  signal<Ntk> run()
  {
    auto table = create_unitized_table();
    return synthesize( table );
  }

private:
  unitized_table create_unitized_table()
  {
    const unsigned int num_vars = func.num_vars();

    /* create column names */
    std::string columns;

    for ( auto i = 0u; i < num_vars; ++i )
    {
      columns += 'a' + i;
    }
    for ( auto i = 0u; i < num_vars; ++i )
    {
      columns += 'A' + i;
    }
    columns += '0';
    columns += '1';

    unitized_table table( columns );

    /* add rows */
    for ( auto pos = 0u; pos < care.num_bits(); pos++ )
    {
      auto half = std::bitset<16>( pos );
      auto row = table.create_row();
      /* copy the values */
      for ( auto i = 0u; i < num_vars; ++i )
      {
        if ( half[i] == 0 )
        {
          row.clear_bit( i );
          row.set_bit( i + num_vars );
        }
        else
        {
          row.set_bit( i );
          row.clear_bit( i + num_vars );
        }
      }
      row.clear_bit( num_vars << 1 );
      row.set_bit( ( num_vars << 1 ) + 1 );

      if ( !kitty::get_bit( func, pos ) )
      {
        for ( auto i = 0; i < row.num_literals(); ++i )
        {
          if ( row.get_bit( i ) == 0 )
          {
            row.set_bit( i );
          }
          else
          {
            row.clear_bit( i );
          }
        }
      }
      table.add_row( row );
    }
    table.reduce();

    return table;
  }

  std::set<std::set<unsigned>> find_gates_for_column( const unitized_table& table, unsigned column ) const
  {
    std::vector<unitized_table::row_t> one_rows;
    std::vector<bool> matrix;
    /* find rows with a 1 at column */

    for ( const auto& row : table )
    {
      if ( row.get_bit( column ) )
      {
        auto rt = row;
        rt.clear_bit( column );
        one_rows.push_back( rt );
      }
    }

    /* find essential rows */
    for ( auto i = 0u; i < one_rows.size(); ++i )
    {
      for ( auto j = 0u; j < one_rows.size(); ++j )
      {
        if ( i == j )
        {
          continue;
        }
        if ( all_zeros( one_rows[i] & one_rows[j] ) )
        {
          for ( auto k = 0; k < one_rows[i].num_literals(); ++k )
            matrix.push_back( one_rows[i].get_bit( k ) );
          break;
        }
      }
    }
    return clauses_to_products_enumerative( table, column, matrix );
  }

  std::set<unsigned> find_gate_for_table( unitized_table& table )
  {

    std::map<std::set<unsigned>, unsigned> gates;
    std::vector<std::set<unsigned>> random_gates;
    auto g_count = 0u;

    for ( auto c = 0u; c < table.num_columns(); ++c )
    {
      for ( const auto& g : find_gates_for_column( table, c ) )
      {
        assert( g.size() == 3u );
        gates[g]++;
        g_count++;
      }
    }

    if ( gates.empty() )
    {
      reduce++;
      return find_gate_for_table_brute_force( table );
    }
    if ( gates.size() == previous_size )
    {
      reduce++;
      return find_gate_for_table_brute_force( table );
    }

    assert( !gates.empty() );
    reduce = 0;
    previous_size = gates.size();
    using pair_t = decltype( gates )::value_type;

    for ( auto f = 0u; f < g_count; f++ )
    {
      auto pr = std::max_element( std::begin( gates ), std::end( gates ), []( const pair_t& p1, const pair_t& p2 ) { return p1.second < p2.second; } );
      random_gates.push_back( pr->first );
      gates.erase( pr->first );
      if ( gates.size() == 0 )
        break;
    }

    auto this_table = table;
    for ( auto f = 0u; f < random_gates.size(); f++ )
    {
      table.add_gate( random_gates[f] );
      table.reduce();
      if ( ( table.rows.size() != this_table.rows.size() ) || ( table.columns.size() != this_table.columns.size() - 1 ) )
      {
        table = this_table;
        return random_gates[f];
      }
      table = this_table;
    }

    reduce++;
    return random_gates[0u];
  }

  std::set<unsigned> find_gate_for_table_brute_force( const unitized_table& table ) const
  {
    auto best_count_iter = std::numeric_limits<unsigned>::max();
    std::set<unsigned> best_gate_iter;

    std::vector<unsigned> numbers( table.num_columns() );
    std::iota( numbers.begin(), numbers.end(), 0u );

    for ( auto i = 0u; i < table.num_columns(); i++ )
    {
      for ( auto j = i + 1u; j < table.num_columns(); j++ )
      {
        for ( auto k = j + 1u; k < table.num_columns(); k++ )
        {
          std::set<unsigned> gate;
          gate.insert( numbers[i] );
          gate.insert( numbers[j] );
          gate.insert( numbers[k] );

          auto table_copy = table;
          table_copy.add_gate( gate );

          const auto new_count = table_copy.count_essential_ones();
          if ( new_count < best_count_iter )
          {
            best_count_iter = new_count;
            best_gate_iter = gate;
          }
        }
      }
    }
    return best_gate_iter;
  }

  signal<Ntk> synthesize( unitized_table& table )
  {

    std::unordered_map<int, signal<Ntk>> c_to_f;

    c_to_f[0x30] = ntk.get_constant( false );
    c_to_f[0x31] = ntk.get_constant( true );

    for ( auto i = 0u; i < func.num_vars(); ++i )
    {
      auto pi = *begin++; // should take the leaves values
      c_to_f[0x41 + i] = !pi;
      c_to_f[0x61 + i] = pi;
    }

    auto last_gate_id = 0;

    while ( table.num_columns() )
    {
      auto gate = find_gate_for_table( table );

      auto it = gate.begin();
      const auto f1 = *it++;
      const auto f2 = *it++;
      const auto f3 = *it;

      last_gate_id = table.add_gate( gate );

      c_to_f[last_gate_id] = ntk.create_maj( c_to_f[table[f1]], c_to_f[table[f2]], c_to_f[table[f3]] );

      if ( reduce == 0 )
        table.reduce();
    }

    if ( ntk.node_to_index( ntk.get_node( c_to_f[last_gate_id] ) ) == 0 )
      return ntk.get_constant( 0 ^ ntk.is_complemented( c_to_f[last_gate_id] ) );

    return c_to_f[last_gate_id];
  }

  std::vector<std::vector<int>> create_gates( const unitized_table& table )
  {
    const auto num_vars = func.num_vars();
    std::vector<int> count( table.columns.size(), 0 );
    auto best_count = 0;

    std::vector<std::vector<int>> gates;

    for ( auto c = 0u; c < table.columns.size(); ++c )
    {
      for ( const auto& row : table )
      {
        best_count++;
        if ( row.get_bit( c ) == 0 )
        {
          count[c]++;
        }
      }
    }

    auto best_column = 0u;

    for ( auto c = 0u; c < count.size(); ++c )
    {
      if ( count[c] < best_count )
      {
        best_column = c;
        best_count = count[c];
      }
    }

    auto icx = 0;
    auto name = table.columns[best_column];
    if ( islower( name ) )
      icx = name - 'a';
    else if ( isupper( name ) )
      icx = name - 'A' + num_vars;
    else if ( name == '0' )
      icx = num_vars * 2;
    else
      icx = num_vars * 2 + 1;

    std::vector<int> best_c;

    best_c.push_back( icx );
    gates.push_back( best_c );

    for ( const auto& row : table )
    {

      if ( row.get_bit( best_column ) == 0 )
      {
        std::vector<int> gate1;
        for ( auto c = 0u; c < table.num_columns(); ++c )
        {
          if ( c == best_column )
            continue;
          if ( row.get_bit( c ) == 1 )
          {
            unsigned icx;
            auto name = table.columns[c];
            if ( islower( name ) )
              icx = name - 'a';
            else if ( isupper( name ) )
              icx = name - 'A' + num_vars;
            else if ( name == '0' )
              icx = num_vars * 2;
            else
              icx = num_vars * 2 + 1;
            gate1.push_back( icx );
          }
        }
        gates.push_back( gate1 );
      }
    }
    std::vector<int> g;
    g.push_back( num_vars );

    gates.push_back( g );
    return gates;
  }

  std::set<std::set<unsigned>> clauses_to_products_enumerative( const unitized_table& table, unsigned column,
                                                                const std::vector<bool>& matrix ) const
  {
    std::set<std::set<unsigned>> products;

    const auto num_columns = table.num_columns();
    const auto num_rows = matrix.size() / num_columns;

    for ( auto i = 0u; i < num_columns; ++i )
    {
      if ( table.is_opposite( column, i ) )
      {
        continue;
      }
      if ( column == i )
        continue;
      for ( auto j = i + 1u; j < num_columns; ++j )
      {
        if ( table.is_opposite( i, j ) || table.is_opposite( column, j ) )
        {
          continue;
        }
        if ( column == j )
          continue;
        auto found = true;
        std::size_t offset = 0u;
        for ( auto r = 0u; r < num_rows; ++r, offset += num_columns )
        {
          if ( !matrix[offset + i] && !matrix[offset + j] )
          {
            found = false;
            break;
          }
        }

        if ( found )
        {
          std::set<unsigned> product;
          product.insert( i );
          product.insert( j );
          product.insert( column );
          assert( product.size() == 3 );
          products.insert( product );
        }
      }
    }

    return products;
  }

private:
  Ntk ntk;
  kitty::dynamic_truth_table const& func;
  kitty::dynamic_truth_table const& care;
  LeavesIterator begin;
  LeavesIterator end;

  unsigned reduce{ 0 };
  std::size_t previous_size{ 0 };
};

} // namespace detail

/*! \brief Performs Akers majority-3 synthesis inside network.
 *
 * Note that the number of variables in `func` and `care` must be the same.
 * Also the distance between `begin` and `end` must equal the number of
 * variables in `func`.
 *
 * **Required network functions:**
 * - `create_maj`
 *
 * \param ntk Network
 * \param func Function as truth table
 * \param care Care set of the function (as truth table)
 * \param begin Begin iterator to child signals
 * \param end End iterator to child signals
 * \return Signal that realizes function in terms of child signals
 */
template<typename Ntk, typename LeavesIterator>
signal<Ntk> akers_synthesis( Ntk& ntk, kitty::dynamic_truth_table const& func, kitty::dynamic_truth_table const& care, LeavesIterator begin, LeavesIterator end )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_maj_v<Ntk>, "Ntk does not implement the create_maj method" );

  assert( func.num_vars() == care.num_vars() );
  assert( std::distance( begin, end ) == func.num_vars() );

  auto pi = begin;

  if ( is_const0( func ) )
    return ntk.get_constant( 0 );
  auto tt_1 = unary_not( func );
  if ( is_const0( tt_1 ) )
    return ntk.get_constant( 1 );

  for ( auto i = 0u; i < func.num_vars(); i++ )
  {
    create_nth_var( tt_1, i );
    auto it = *pi++;
    if ( tt_1 == func )
    {
      return it;
    }
    tt_1 = unary_not( tt_1 );
    if ( tt_1 == func )
    {
      return !it;
    }
  }

  detail::akers_synthesis_impl<Ntk, LeavesIterator> tt( ntk, func, care, begin, end );
  return tt.run();
}

/*! \brief Performs Akers majority-3 synthesis to create network.
 *
 * Note that the number of variables in `func` and `care` must be the same.
 * The function will create a network with as many primary inputs as number of
 * variables in `func` and a single output.
 *
 * **Required network functions:**
 * - `create_pi`
 * - `create_po`
 * - `create_maj`
 *
 * \param func Function as truth table
 * \param care Care set of the function (as truth table)
 * \return A network that realizes the function
 */
template<typename Ntk>
Ntk akers_synthesis( kitty::dynamic_truth_table const& func, kitty::dynamic_truth_table const& care )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );
  static_assert( has_create_po_v<Ntk>, "Ntk does not implement the create_po method" );

  Ntk ntk;
  std::vector<signal<Ntk>> pis;

  for ( auto i = 0u; i < func.num_vars(); ++i )
  {
    pis.push_back( ntk.create_pi() );
  }

  const auto f = akers_synthesis( ntk, func, care, pis.begin(), pis.end() );
  ntk.create_po( f );
  return ntk;
}
} // namespace mockturtle