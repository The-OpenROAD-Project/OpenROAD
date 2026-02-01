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
  \file xag_minmc2.hpp
  \brief XAG resynthesis

  \author Heinz Riener
  \author Mathias Soeken
*/

#pragma once

#include <cstdint>
#include <fstream>
#include <unordered_map>
#include <vector>

#include "../../networks/xag.hpp"
#include "../../utils/index_list/index_list.hpp"
#include "../detail/minmc_xags.hpp"
#include "../equivalence_classes.hpp"

#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/print.hpp>
#include <kitty/spectral.hpp>
#include <lorina/detail/utils.hpp>

namespace mockturtle::future
{

struct xag_minmc_resynthesis_params
{
  /*! \brief Be verbose. */
  bool verbose{ false };
};

struct xag_minmc_resynthesis_stats
{
  /*! \brief Database size in bytes. */
  uint64_t db_size{};
};

template<class Ntk = xag_network>
class xag_minmc_resynthesis
{
public:
  xag_minmc_resynthesis()
  {
    build_db();
  }

  void load_from_file( std::string const& filename )
  {
    if ( ps_.verbose )
    {
      fmt::print( "start loading\n" );
    }
    kitty::dynamic_truth_table func( 6u );
    std::ifstream in( filename, std::ifstream::in );
    std::string line;
    while ( std::getline( in, line ) )
    {
      const auto vline = lorina::detail::split( line, " " );
      kitty::create_from_hex_string( func, vline[0] );
      const auto sindexes = lorina::detail::split( vline[3], "," );
      std::vector<uint32_t> index_list( sindexes.size() );
      std::transform( sindexes.begin(), sindexes.end(), index_list.begin(), [&]( std::string const& s ) { return static_cast<uint32_t>( std::stoul( s ) ); } );
      db_[6u][*func.cbegin()] = index_list;
      st_.db_size += sizeof( uint64_t ) + sizeof( sindexes ) + sizeof( uint32_t ) * sindexes.size();
    }
    if ( ps_.verbose )
    {
      fmt::print( "done loading, size = {:>5.2f} Kb\n", st_.db_size / 1024.0f );
    }
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    const auto num_vars = function.num_vars();
    uint64_t repr;
    std::vector<kitty::detail::spectral_operation> trans;

    if ( const auto itCache = cache_[num_vars].find( *function.cbegin() ); itCache != cache_[num_vars].end() )
    {
      repr = itCache->second.first;
      trans = itCache->second.second;
    }
    else
    {
      repr = *kitty::hybrid_exact_spectral_canonization( function, [&]( auto const& ops ) { trans = ops; } ).cbegin();
      cache_[num_vars][*function.cbegin()] = { repr, trans };
    }

    const auto it = db_[num_vars].find( repr );
    if ( it == db_[num_vars].end() )
    {
      fmt::print( "[w] cannot find repr {:x} in database.\n", repr );
      return;
    }

    const auto f = apply_spectral_transformations( ntk, trans, std::vector<signal<Ntk>>( begin, end ), [&]( xag_network& ntk, std::vector<signal<Ntk>> const& leaves ) {
      xag_index_list il{ it->second };
      std::vector<xag_network::signal> pos;
      insert( ntk, std::begin( leaves ), std::begin( leaves ) + il.num_pis(), il,
              [&]( xag_network::signal const& f ) {
                pos.push_back( f );
              } );
      assert( pos.size() == 1u );
      return pos[0u];
    } );

    fn( f );
  }

private:
  void build_db()
  {
    st_.db_size += sizeof( db_ );

    for ( auto i = 0u; i < detail::minmc_xags.size(); ++i )
    {
      for ( auto const& [_, word, repr, expr] : detail::minmc_xags[i] )
      {
        (void)_;
        (void)expr;
        db_[i][word] = repr;
        st_.db_size += sizeof( word ) + sizeof( repr ) + sizeof( uint32_t ) * repr.size();
      }
    }

    if ( ps_.verbose )
    {
      fmt::print( "[i] db size = {:>5.2f} Kb\n", st_.db_size / 1024.0f );
    }
  }

private:
  std::vector<std::unordered_map<uint64_t, std::vector<uint32_t>>> db_{ 7u };
  mutable std::vector<std::unordered_map<uint64_t, std::pair<uint64_t, std::vector<kitty::detail::spectral_operation>>>> cache_{ 7u };

private:
  xag_minmc_resynthesis_params ps_;
  xag_minmc_resynthesis_stats st_;
};

} // namespace mockturtle::future