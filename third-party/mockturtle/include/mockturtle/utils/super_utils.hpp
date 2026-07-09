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
  \file super_utils.hpp
  \brief Implements utilities to create supergates for technology mapping

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <cassert>
#include <deque>
#include <unordered_map>
#include <vector>

#include <fmt/format.h>
#include <kitty/constructors.hpp>
#include <kitty/dynamic_truth_table.hpp>

#include "../io/genlib_reader.hpp"
#include "../io/super_reader.hpp"
#include "include/supergate.hpp"

namespace mockturtle
{

struct super_utils_params
{
  /*! \brief load multi-output gates in simple supergates */
  bool load_multioutput_in_single{ false };

  /*! \brief reports loaded supergates */
  bool verbose{ false };
};

/*! \brief Utilities to generate supergates
 *
 * This class creates supergates starting from supergates
 * specifications contained in `supergates_spec` extracted
 * from a SUPER file.
 *
 * Multi-output gates are also extracted from the list of
 * GENLIB gates. However multi-output gates are currently not
 * supported as supergates members.
 *
 * This utility is called by `tech_library` to construct
 * the library for technology mapping.
 */
template<unsigned NInputs = 6u>
class super_utils
{
private:
  static constexpr uint32_t truth_table_size = 6;

public:
  explicit super_utils( std::vector<gate> const& gates, super_lib const& supergates_spec = {}, super_utils_params const ps = {} )
      : _gates( gates ),
        _supergates_spec( supergates_spec ),
        _ps( ps ),
        _supergates(),
        _multioutput_gates()
  {
    if ( _supergates_spec.supergates.size() == 0 )
    {
      generate_library_with_genlib();
    }
    else
    {
      generate_library_with_super();
    }
  }

  /*! \brief Get the all the supergates.
   *
   * Returns a list of supergates created accordingly to
   * the standard library and the supergates specifications.
   */
  const std::deque<composed_gate<NInputs>>& get_super_library() const
  {
    return _supergates;
  }

  /*! \brief Get the number of standard gates.
   *
   * Returns the number of standard gates contained in the
   * supergate library.
   */
  const uint32_t get_standard_library_size() const
  {
    return simple_gates_size;
  }

  /*! \brief Get multi-output gates.
   *
   * Returns a list of multioutput gates.
   */
  const std::vector<std::vector<composed_gate<NInputs>>>& get_multioutput_library() const
  {
    return _multioutput_gates;
  }

public:
  void generate_library_with_genlib()
  {
    uint32_t initial_size = _supergates.size();

    std::unordered_map<std::string, uint32_t> multioutput_map;
    std::unordered_map<std::string, uint32_t> multioutput_idx;
    multioutput_map.reserve( _gates.size() );

    /* look for multi-output gates (gates with the same name) */
    uint32_t multioutput_i = 0;
    for ( const auto& g : _gates )
    {
      if ( multioutput_map.find( g.name ) != multioutput_map.end() )
      {
        /* assign an index */
        if ( multioutput_map[g.name] == 1 )
          multioutput_idx[g.name] = multioutput_i++;

        multioutput_map[g.name] += 1;
      }
      else
      {
        multioutput_map[g.name] = 1;
        multioutput_idx[g.name] = UINT32_MAX;
      }
    }

    /* create composed gates */
    uint32_t ignored = 0;
    uint32_t ignored_id = 0;
    uint32_t large_gates = 0;
    for ( const auto& g : _gates )
    {
      std::array<float, NInputs> pin_to_pin_delays{};

      if ( g.function.num_vars() > NInputs )
      {
        ++ignored;
        ignored_id = g.id;
        continue;
      }
      if ( g.function.num_vars() > truth_table_size )
      {
        ++large_gates;
        continue;
      }

      auto i = 0u;
      for ( auto const& pin : g.pins )
      {
        /* use worst pin delay */
        pin_to_pin_delays[i++] = std::max( pin.rise_block_delay, pin.fall_block_delay );
      }

      if ( multioutput_map[g.name] == 1 || _ps.load_multioutput_in_single )
      {
        _supergates.emplace_back( composed_gate<NInputs>{ static_cast<unsigned int>( _supergates.size() ),
                                                          false,
                                                          &g,
                                                          g.num_vars,
                                                          g.function,
                                                          g.area,
                                                          pin_to_pin_delays,
                                                          {} } );
      }

      if ( multioutput_map[g.name] > 1 )
      {
        uint32_t idx = multioutput_idx[g.name];
        if ( _multioutput_gates.size() <= idx )
          _multioutput_gates.emplace_back( std::vector<composed_gate<NInputs>>() );

        _multioutput_gates[multioutput_idx[g.name]].emplace_back(
            composed_gate<NInputs>{ static_cast<unsigned int>( idx ),
                                    false,
                                    &g,
                                    g.num_vars,
                                    g.function,
                                    g.area,
                                    pin_to_pin_delays,
                                    {} } );
      }
    }

    simple_gates_size = _supergates.size() - initial_size;

    if ( _ps.verbose )
    {
      std::cout << fmt::format( "[i] Loading {} simple library cells\n", simple_gates_size + large_gates );
      std::cout << fmt::format( "[i] Loading {} multi-output library cells\n", _multioutput_gates.size() );
    }

    if ( ignored > 0 )
    {
      std::cerr << fmt::format( "[i] WARNING: {} gates IGNORED (e.g., {}), too many inputs for the library settings\n", ignored, _gates[ignored_id].name );
    }
  }

  void generate_library_with_super()
  {
    if ( _supergates_spec.max_num_vars > NInputs || _supergates_spec.max_num_vars > truth_table_size )
    {
      std::cerr << fmt::format(
          "[e] ERROR: NInputs ({}) should be greater or equal than the max number of variables ({}) in the super file.\n", NInputs, _supergates_spec.max_num_vars );
      std::cerr << "[i] WARNING: ignoring supergates, proceeding with standard library." << std::endl;
      generate_library_with_genlib();
      return;
    }

    /* create a map for the gates IDs */
    std::unordered_map<std::string, uint32_t> gates_map;

    for ( auto const& g : _gates )
    {
      if ( gates_map.find( g.name ) != gates_map.end() )
      {
        std::cerr << fmt::format( "[i] WARNING: ignoring genlib gate {}, duplicated name entry in supergates.", g.name ) << std::endl;
      }
      else
      {
        gates_map[g.name] = g.id;
      }
    }

    /* creating input variables */
    for ( uint8_t i = 0; i < _supergates_spec.max_num_vars; ++i )
    {
      kitty::dynamic_truth_table tt{ NInputs };
      kitty::create_nth_var( tt, i );

      _supergates.emplace_back( composed_gate<NInputs>{ static_cast<unsigned int>( i ),
                                                        false,
                                                        nullptr,
                                                        0,
                                                        tt,
                                                        0.0,
                                                        {},
                                                        {} } );
    }

    generate_library_with_genlib();

    uint32_t super_count = 0;

    /* add supergates */
    for ( auto const& g : _supergates_spec.supergates )
    {
      uint32_t root_match_id;
      if ( auto it = gates_map.find( g.name ); it != gates_map.end() )
      {
        root_match_id = it->second;
      }
      else
      {
        std::cerr << fmt::format( "[i] WARNING: ignoring supergate {}, no reference in genlib.", g.id ) << std::endl;
        continue;
      }

      uint32_t num_vars = _gates[root_match_id].num_vars;

      if ( num_vars != g.fanin_id.size() )
      {
        std::cerr << fmt::format( "[i] WARNING: ignoring supergate {}, wrong number of fanins.", g.id ) << std::endl;
        continue;
      }
      if ( num_vars > _supergates_spec.max_num_vars )
      {
        std::cerr << fmt::format( "[i] WARNING: ignoring supergate {}, too many variables for the library settings.", g.id ) << std::endl;
        continue;
      }

      std::vector<composed_gate<NInputs>*> sub_gates;

      bool error = false;
      bool simple_gate = true;
      for ( uint32_t f : g.fanin_id )
      {
        if ( f >= g.id + _supergates_spec.max_num_vars )
        {
          error = true;
          std::cerr << fmt::format( "[i] WARNING: ignoring supergate {}, wrong fanins.", g.id ) << std::endl;
        }
        if ( f < _supergates_spec.max_num_vars )
        {
          sub_gates.emplace_back( &_supergates[f] );
        }
        else
        {
          sub_gates.emplace_back( &_supergates[f + simple_gates_size] );
          simple_gate = false;
        }
      }

      if ( error )
      {
        continue;
      }

      /* force at `is_super = false` simple gates considered as supergates.
       * This is necessary to not have duplicates since tech_library
       * computes indipendently the permutations for simple gates.
       * Moreover simple gates permutations could be incomplete in SUPER
       * libraries which are constrained by the number of gates. */
      bool is_super_verified = g.is_super;
      if ( simple_gate )
      {
        is_super_verified = false;
      }

      float area = compute_area( root_match_id, sub_gates );
      const kitty::dynamic_truth_table tt = compute_truth_table( root_match_id, sub_gates );

      _supergates.emplace_back( composed_gate<NInputs>{ static_cast<unsigned int>( _supergates.size() ),
                                                        is_super_verified,
                                                        &_gates[root_match_id],
                                                        0,
                                                        tt,
                                                        area,
                                                        {},
                                                        sub_gates } );

      if ( g.is_super )
      {
        ++super_count;
      }

      auto& s = _supergates[_supergates.size() - 1];
      s.num_vars = compute_support( s );
      compute_delay_parameters( s );
    }

    /* minimize supergates */
    for ( auto& g : _supergates )
    {
      if ( g.is_super )
      {
        g.function = shrink_to( g.function, static_cast<unsigned>( g.num_vars ) );
      }
    }

    if ( _ps.verbose )
    {
      std::cout << fmt::format( "[i] Loaded {} supergates in the library\n", super_count );
    }
  }

private:
  inline float compute_area( uint32_t root_id, std::vector<composed_gate<NInputs>*> const& sub_gates )
  {
    float area = _gates[root_id].area;
    for ( auto const f : sub_gates )
    {
      area += f->area;
    }

    return area;
  }

  inline uint32_t compute_support( composed_gate<NInputs>& s )
  {
    std::array<uint8_t, NInputs> used_pins{};
    uint32_t support = 0;

    return compute_support_rec( s, used_pins );
  }

  uint32_t compute_support_rec( composed_gate<NInputs>& s, std::array<uint8_t, NInputs>& used_pins )
  {
    /* termination: input variable */
    if ( s.root == nullptr )
    {
      if ( used_pins[s.id]++ == 0u )
      {
        return 1;
      }
      return 0;
    }

    uint32_t support = 0;
    for ( auto const pin : s.fanin )
    {
      support += compute_support_rec( *pin, used_pins );
    }
    return support;
  }

  inline kitty::dynamic_truth_table compute_truth_table( uint32_t root_id, std::vector<composed_gate<NInputs>*> const& sub_gates )
  {
    std::vector<kitty::dynamic_truth_table> ttv;

    for ( auto const f : sub_gates )
    {
      ttv.emplace_back( f->function );
    }

    return kitty::compose_truth_table( _gates[root_id].function, ttv );
  }

  inline void compute_delay_parameters( composed_gate<NInputs>& s )
  {
    const auto& root = *( s.root );

    auto i = 0u;
    for ( auto const& pin : root.pins )
    {
      float worst_delay = std::max( pin.rise_block_delay, pin.fall_block_delay );

      compute_delay_pin_rec( s, *( s.fanin[i++] ), worst_delay );
    }
  }

  void compute_delay_pin_rec( composed_gate<NInputs>& root, composed_gate<NInputs>& s, float delay )
  {
    /* termination: input variable */
    if ( s.root == nullptr )
    {
      root.tdelay[s.id] = std::max( root.tdelay[s.id], delay );
      return;
    }

    auto i = 0u;
    for ( auto const& pin : s.root->pins )
    {
      float worst_delay = delay + std::max( pin.rise_block_delay, pin.fall_block_delay );

      compute_delay_pin_rec( root, *( s.fanin[i++] ), worst_delay );
    }
  }

protected:
  uint32_t simple_gates_size{ 0 };

  std::vector<gate> const& _gates;
  super_lib const& _supergates_spec;
  super_utils_params const _ps;
  std::deque<composed_gate<NInputs>> _supergates;
  std::vector<std::vector<composed_gate<NInputs>>> _multioutput_gates;
}; /* class super_utils */

} /* namespace mockturtle */
