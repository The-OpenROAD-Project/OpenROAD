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
  \file cached.hpp
  \brief Generic resynthesis with a cache

  \author Heinz Riener
  \author Mathias Soeken
  \author Shubham Rai
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#if !__clang__ || __clang_major__ > 10

#include <cstdint>
#if __GNUC__ == 7
#include <experimental/filesystem>
#else
#include <filesystem>
#endif
#include <fstream>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/hash.hpp>
#include <nlohmann/json.hpp>
#include <unordered_set>

#include "../../algorithms/cleanup.hpp"
#include "../../traits.hpp"
#include "../../utils/json_utils.hpp"
#include "../../utils/network_cache.hpp"
#include "traits.hpp"

namespace mockturtle
{

struct no_blacklist_cache_info
{
  bool retry( no_blacklist_cache_info const& old_info ) const
  {
    (void)old_info;
    return false;
  }
};

inline void to_json( nlohmann::json& j, no_blacklist_cache_info const& info )
{
  (void)info;
  j = nullptr;
}

inline void from_json( nlohmann::json const& j, no_blacklist_cache_info& info )
{
  (void)j;
  (void)info;
}

template<class Ntk, class ResynthesisFn, class BlacklistCacheInfo = no_blacklist_cache_info>
class cached_resynthesis
{
public:
  explicit cached_resynthesis( ResynthesisFn const& resyn_fn, uint32_t max_pis, std::string const& cache_filename = {}, BlacklistCacheInfo const& blacklist_cache_info = {} )
      : _resyn_fn( resyn_fn ),
        _cache( max_pis ),
        _cache_filename( cache_filename ),
        _blacklist_cache_info( blacklist_cache_info ),
        _initial_size( max_pis )
  {
    if ( !_cache_filename.empty() )
    {
      load();
    }
  }

  ~cached_resynthesis()
  {
    if ( !_cache_filename.empty() )
    {
      save();
    }
  }

private:
  using cache_key_t = std::pair<kitty::dynamic_truth_table, std::vector<kitty::dynamic_truth_table>>;

  struct cache_hash
  {
    std::size_t operator()( const cache_key_t& p ) const
    {
      auto seed = _h( p.first );
      std::for_each( p.second.begin(), p.second.end(), [&]( auto const& tt ) { kitty::hash_combine( seed, _h( tt ) ); } );
      return seed;
    }

  private:
    kitty::hash<kitty::dynamic_truth_table> _h;
  };

  using blacklist_cache_key_t = std::pair<kitty::dynamic_truth_table, BlacklistCacheInfo>;

  struct blacklist_cache_hash
  {
    std::size_t operator()( const blacklist_cache_key_t& p ) const
    {
      return _h( p.first );
    }

  private:
    kitty::hash<kitty::dynamic_truth_table> _h;
  };

  struct blacklist_cache_equal
  {
    bool operator()( const blacklist_cache_key_t& lhs, const blacklist_cache_key_t& rhs ) const
    {
      return lhs.first == rhs.first;
    }
  };

  bool is_blacklisted( kitty::dynamic_truth_table const& tt ) const
  {
    auto it = _blacklist_cache.find( { tt, _blacklist_cache_info } );

    /* function cannot be found in black list cache */
    if ( it == _blacklist_cache.end() )
    {
      return false;
    }
    /* newer black list info, erase old entry from cache */
    else if ( _blacklist_cache_info.retry( it->second ) )
    {
      _blacklist_cache.erase( it );
      return false;
    }
    /* function is black listed */
    else
    {
      return true;
    }
  }

public:
  template<typename LeavesIterator, typename Fn>
  void operator()( Ntk& ntk, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    if ( auto const key = std::make_pair( function, _existing_functions );
         _cache.has( key ) )
    {
      ++_cache_hits;
      std::vector<signal<Ntk>> signals( _cache.pis().size(), ntk.get_constant( false ) );
      std::copy( begin, end, signals.begin() );
      std::copy( _existing_signals.begin(), _existing_signals.end(), signals.begin() + _initial_size );
      fn( cleanup_dangling( _cache.get_view( key ), ntk, signals.begin(), signals.end() ).front() );
    }
    else if ( is_blacklisted( function ) )
    {
      ++_cache_hits;
      return; /* do nothing */
    }
    else
    {
      bool found_one = false;
      auto on_signal = [&]( signal<Ntk> const& f ) -> bool {
        if ( !found_one )
        {
          ++_cache_misses;
          _cache.insert_signal( key, f );
          found_one = true;

          std::vector<signal<Ntk>> signals( _cache.pis().size(), ntk.get_constant( false ) );
          std::copy( begin, end, signals.begin() );
          std::copy( _existing_signals.begin(), _existing_signals.end(), signals.begin() + _initial_size );
          fn( cleanup_dangling( _cache.get_view( key ), ntk, signals.begin(), signals.end() ).front() );
        }
        return false;
      };

      _resyn_fn( _cache.network(), function, _cache.pis().begin(), _cache.pis().begin() + function.num_vars(), on_signal );

      if ( !found_one )
      {
        _blacklist_cache.insert( { function, _blacklist_cache_info } );
      }
    }
  }

  void set_bounds( std::optional<uint32_t> const& lower_bound, std::optional<uint32_t> const& upper_bound )
  {
    if constexpr ( has_set_bounds_v<ResynthesisFn> )
    {
      _resyn_fn.set_bounds( lower_bound, upper_bound );
    }
  }

  void clear_functions()
  {
    if constexpr ( has_clear_functions_v<ResynthesisFn> )
    {
      _existing_signals.clear();
      _existing_functions.clear();
      _resyn_fn.clear_functions();
    }
  }

  void add_function( signal<Ntk> const& s, kitty::dynamic_truth_table const& tt )
  {
    if constexpr ( has_add_function_v<ResynthesisFn, Ntk> )
    {
      // index of cache PI to forward
      const auto pi_index = _initial_size + _existing_signals.size();

      _existing_signals.push_back( s );
      _existing_functions.push_back( tt );
      _cache.ensure_pis( _initial_size + _existing_functions.size() );

      _resyn_fn.add_function( _cache.pis()[pi_index], tt );
    }
    else
    {
      // TODO assert or warn?
    }
  }

  void report() const
  {
    fmt::print( "[i] cache hits              = {}\n", _cache_hits );
    fmt::print( "[i] cache misses            = {}\n", _cache_misses );
    fmt::print( "[i] size of cache           = {}\n", _cache.size() );
    fmt::print( "[i] size of blacklist cache = {}\n", _blacklist_cache.size() );
  }

private:
  void load()
  {
    std::ifstream is( _cache_filename.c_str(), std::ifstream::in );
    if ( !is.good() )
      return;
    nlohmann::json data;
    is >> data;

    _cache.insert_json( data["cache"] );
    data["blacklist_cache"].get_to( _blacklist_cache );
    data["initial_size"].get_to( _initial_size );
  }

  void save()
  {
#if __GNUC__ == 7
    namespace fs = std::experimental::filesystem::v1;
#else
    namespace fs = std::filesystem;
#endif

    nlohmann::json data{
        { "cache", _cache.to_json() },
        { "blacklist_cache", _blacklist_cache },
        { "initial_size", _initial_size } };

    // make a backup of existing cache file, if it exists
    std::string _backup_filename = fmt::format( "{}.bak", _cache_filename );
    if ( fs::exists( _cache_filename ) )
    {
      fs::copy( _cache_filename, _backup_filename );
    }

    std::ofstream os( _cache_filename.c_str(), std::ofstream::out );
    os << data.dump() << "\n";
    os.close();

    if ( fs::exists( _backup_filename ) )
    {
      fs::remove( _backup_filename );
    }
  }

private:
  ResynthesisFn _resyn_fn;
  mutable network_cache<Ntk, cache_key_t, cache_hash> _cache;
  mutable std::unordered_set<blacklist_cache_key_t, blacklist_cache_hash, blacklist_cache_equal> _blacklist_cache;
  std::string _cache_filename;
  BlacklistCacheInfo _blacklist_cache_info;
  uint32_t _initial_size{};

  std::vector<kitty::dynamic_truth_table> _existing_functions;
  std::vector<signal<Ntk>> _existing_signals;

  /* statistics */
  mutable uint32_t _cache_hits{};
  mutable uint32_t _cache_misses{};
};
} /* namespace mockturtle */

#endif
