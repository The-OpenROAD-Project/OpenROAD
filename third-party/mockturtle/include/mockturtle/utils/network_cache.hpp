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
  \file network_cache.hpp
  \brief Network cache

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <kitty/print.hpp>
#include <nlohmann/json.hpp>

#include "../algorithms/simulation.hpp"
#include "../io/verilog_reader.hpp"
#include "../io/write_verilog.hpp"
#include "../traits.hpp"
#include "../views/topo_view.hpp"

namespace mockturtle
{

/*! \brief Network cache.
 *
 * ...
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      ...
   \endverbatim
 */
template<typename Ntk, typename Key, class Hash = std::hash<Key>>
class network_cache
{
public:
  explicit network_cache( uint32_t num_vars )
  {
    static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
    static_assert( has_create_pi_v<Ntk>, "Ntk does not implement the create_pi method" );

    ensure_pis( num_vars );
  }

  Ntk& network()
  {
    return _db;
  }

  std::vector<signal<Ntk>> const& pis() const
  {
    return _pis;
  }

  void ensure_pis( uint32_t count )
  {
    if ( count > _pis.size() )
    {
      for ( auto i = _pis.size(); i < count; ++i )
      {
        _pis.emplace_back( _db.create_pi() );
      }
    }
  }

  auto size() const
  {
    return _db.num_pos();
  }

  bool has( Key const& key ) const
  {
    return _map.find( key ) != _map.end();
  }

  template<class _Ntk>
  bool insert( Key const& key, _Ntk const& ntk )
  {
    /* ntk must have one primary output and not too many primary inputs */
    if ( ntk.num_pos() != 1u || ntk.num_pis() > _pis.size() )
    {
      return false;
    }

    /* insert ntk into _db, create an output and return the index of the output */
    const auto f = cleanup_dangling( ntk, _db, _pis.begin(), _pis.begin() + ntk.num_pis() ).front();
    insert_signal( key, f );
    return true;
  }

  void insert_signal( Key const& key, signal<Ntk> const& f )
  {
    _map.emplace( key, f );
    _db.create_po( f );
    _output_functions.push_back( key );
  }

  signal<Ntk> get( Key const& key ) const
  {
    return _map.at( key ).first;
  }

  auto get_view( Key const& key ) const
  {
    return topo_view<Ntk>( _db, _map.at( key ) );
  }

  void insert_json( nlohmann::json const& data )
  {
    ensure_pis( data["num_pis"].get<uint32_t>() );

    std::istringstream sstr( data["db"].get<std::string>() );
    Ntk read_ntk;
    lorina::read_verilog( sstr, verilog_reader( read_ntk ) );
    const auto pos = cleanup_dangling( read_ntk, _db, _pis.begin(), _pis.end() );

    auto cntr = 0u;
    for ( auto const& tt : data["output_functions"].get<std::vector<Key>>() )
    {
      insert_signal( tt, pos[cntr++] );
    }
  }

  nlohmann::json to_json() const
  {
    std::stringstream sstr;
    write_verilog( _db, sstr );

    return nlohmann::json{ { "num_pis", _pis.size() }, { "output_functions", _output_functions }, { "db", sstr.str() } };
  }

private:
  Ntk _db;
  std::vector<signal<Ntk>> _pis;
  std::unordered_map<Key, signal<Ntk>, Hash> _map;
  std::vector<Key> _output_functions;
};

} /* namespace mockturtle */