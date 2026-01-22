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
  \file mapper.hpp
  \brief Mapper

  \author Alessandro Tempia Calvino
*/

#pragma once

#include <cstdint>
#include <limits>

#include <fmt/format.h>

#include "../networks/aig.hpp"
#include "../networks/klut.hpp"
#include "../networks/mig.hpp"
#include "../networks/sequential.hpp"
#include "../networks/xag.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../utils/tech_library.hpp"
#include "../views/binding_view.hpp"
#include "../views/color_view.hpp"
#include "../views/depth_view.hpp"
#include "../views/topo_view.hpp"
#include "../views/window_view.hpp"
#include "cleanup.hpp"
#include "cut_enumeration.hpp"
#include "cut_enumeration/exact_map_cut.hpp"
#include "cut_enumeration/tech_map_cut.hpp"
#include "detail/mffc_utils.hpp"
#include "detail/switching_activity.hpp"
#include "reconv_cut.hpp"
#include "resyn_engines/mig_resyn.hpp"
#include "resyn_engines/xag_resyn.hpp"
#include "simulation.hpp"

namespace mockturtle
{

/*! \brief Parameters for map.
 *
 * The data structure `map_params` holds configurable parameters
 * with default arguments for `map`.
 */
struct map_params
{
  map_params()
  {
    cut_enumeration_ps.cut_limit = 49;
    cut_enumeration_ps.minimize_truth_table = true;
  }

  /*! \brief Parameters for cut enumeration
   *
   * The default cut limit is 49. By default,
   * truth table minimization is performed.
   */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Required time for delay optimization. */
  double required_time{ 0.0f };

  /*! \brief Skip delay round for area optimization. */
  bool skip_delay_round{ false };

  /*! \brief Number of rounds for area flow optimization. */
  uint32_t area_flow_rounds{ 1u };

  /*! \brief Number of rounds for exact area optimization. */
  uint32_t ela_rounds{ 2u };

  /*! \brief Number of rounds for exact switching power optimization. */
  uint32_t eswp_rounds{ 0u };

  /*! \brief Number of patterns for switching activity computation. */
  uint32_t switching_activity_patterns{ 2048u };

  /*! \brief Exploit logic sharing in exact area optimization of graph mapping. */
  bool enable_logic_sharing{ false };

  /*! \brief Maximum number of cuts evaluated for logic sharing. */
  uint32_t logic_sharing_cut_limit{ 8u };

  /*! \brief Use satisfiability don't cares for optimization. */
  bool use_dont_cares{ false };

  /*! \brief Window size for don't cares calculation. */
  uint32_t window_size{ 12u };

  /*! \brief Be verbose. */
  bool verbose{ false };
};

/*! \brief Statistics for mapper.
 *
 * The data structure `map_stats` provides data collected by running
 * `map`.
 */
struct map_stats
{
  /*! \brief Area result. */
  double area{ 0 };
  /*! \brief Worst delay result. */
  double delay{ 0 };
  /*! \brief Power result. */
  double power{ 0 };

  /*! \brief Runtime for covering. */
  stopwatch<>::duration time_mapping{ 0 };
  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  /*! \brief Cut enumeration stats. */
  cut_enumeration_stats cut_enumeration_st{};

  /*! \brief Delay and area stats for each round. */
  std::vector<std::string> round_stats{};

  /*! \brief Mapping error. */
  bool mapping_error{ false };

  void report() const
  {
    for ( auto const& stat : round_stats )
    {
      std::cout << stat;
    }
    std::cout << fmt::format( "[i] Area = {:>5.2f}; Delay = {:>5.2f};", area, delay );
    if ( power != 0 )
      std::cout << fmt::format( " Power = {:>5.2f};\n", power );
    else
      std::cout << "\n";
    std::cout << fmt::format( "[i] Mapping runtime = {:>5.2f} secs\n", to_seconds( time_mapping ) );
    std::cout << fmt::format( "[i] Total runtime   = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

template<unsigned NInputs>
struct cut_match_tech
{
  /* list of supergates matching the cut for positive and negative output phases */
  std::array<std::vector<supergate<NInputs>> const*, 2> supergates = { nullptr, nullptr };
  /* input negations, 0: pos, 1: neg */
  std::array<uint8_t, 2> negations{ 0, 0 };
};

template<unsigned NInputs>
struct node_match_tech
{
  /* best gate match for positive and negative output phases */
  supergate<NInputs> const* best_supergate[2] = { nullptr, nullptr };
  /* fanin pin phases for both output phases */
  uint8_t phase[2];
  /* best cut index for both phases */
  uint32_t best_cut[2];
  /* node is mapped using only one phase */
  bool same_match{ false };

  /* arrival time at node output */
  double arrival[2];
  /* required time at node output */
  double required[2];
  /* area of the best matches */
  float area[2];

  /* number of references in the cover 0: pos, 1: neg, 2: pos+neg */
  uint32_t map_refs[3];
  /* references estimation */
  float est_refs[3];
  /* area flow */
  float flows[3];
};

template<class Ntk, unsigned CutSize, typename CutData, unsigned NInputs, classification_type Configuration>
class tech_map_impl
{
public:
  using network_cuts_t = fast_network_cuts<Ntk, CutSize, true, CutData>;
  using cut_t = typename network_cuts_t::cut_t;
  using match_map = std::unordered_map<uint32_t, std::vector<cut_match_tech<NInputs>>>;
  using klut_map = std::unordered_map<uint32_t, std::array<signal<klut_network>, 2>>;
  using map_ntk_t = binding_view<klut_network>;
  using seq_map_ntk_t = binding_view<sequential<klut_network>>;

public:
  explicit tech_map_impl( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, map_params const& ps, map_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        matches(),
        switch_activity( ps.eswp_rounds ? switching_activity( ntk, ps.switching_activity_patterns ) : std::vector<float>( 0 ) ),
        cuts( fast_cut_enumeration<Ntk, CutSize, true, CutData>( ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st ) )
  {
    std::tie( lib_inv_area, lib_inv_delay, lib_inv_id ) = library.get_inverter_info();
    std::tie( lib_buf_area, lib_buf_delay, lib_buf_id ) = library.get_buffer_info();
  }

  explicit tech_map_impl( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, std::vector<float> const& switch_activity, map_params const& ps, map_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        node_match( ntk.size() ),
        matches(),
        switch_activity( switch_activity ),
        cuts( fast_cut_enumeration<Ntk, NInputs, true, CutData>( ntk, ps.cut_enumeration_ps, &st.cut_enumeration_st ) )
  {
    std::tie( lib_inv_area, lib_inv_delay, lib_inv_id ) = library.get_inverter_info();
    std::tie( lib_buf_area, lib_buf_delay, lib_buf_id ) = library.get_buffer_info();
  }

  map_ntk_t run()
  {
    stopwatch t( st.time_mapping );

    auto [res, old2new] = initialize_map_network();

    /* compute and save topological order */
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );

    /* match cuts with gates */
    compute_matches();

    /* init the data structure */
    init_nodes();

    /* execute mapping */
    if ( !execute_mapping() )
      return res;

    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover<map_ntk_t>( res, old2new );

    return res;
  }

  seq_map_ntk_t run_seq()
  {
    stopwatch t( st.time_mapping );

    auto [res, old2new] = initialize_map_seq_network();

    /* compute and save topological order */
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );

    /* match cuts with gates */
    compute_matches();

    /* init the data structure */
    init_nodes();

    /* execute mapping */
    if ( !execute_mapping() )
      return res;

    /* insert buffers for POs driven by PIs */
    insert_buffers();

    /* generate the output network */
    finalize_cover<seq_map_ntk_t>( res, old2new );

    return res;
  }

private:
  bool execute_mapping()
  {
    /* compute mapping for delay */
    if ( !ps.skip_delay_round )
    {
      if ( !compute_mapping<false>() )
      {
        return false;
      }
    }

    /* compute mapping using global area flow */
    while ( iteration < ps.area_flow_rounds + 1 )
    {
      compute_required_time();
      if ( !compute_mapping<true>() )
      {
        return false;
      }
    }

    /* compute mapping using exact area */
    while ( iteration < ps.ela_rounds + ps.area_flow_rounds + 1 )
    {
      compute_required_time();
      if ( !compute_mapping_exact<false>() )
      {
        return false;
      }
    }

    /* compute mapping using exact switching activity estimation */
    while ( iteration < ps.eswp_rounds + ps.ela_rounds + ps.area_flow_rounds + 1 )
    {
      compute_required_time();
      if ( !compute_mapping_exact<true>() )
      {
        return false;
      }
    }

    return true;
  }

  void init_nodes()
  {
    ntk.foreach_node( [this]( auto const& n, auto ) {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      node_data.est_refs[0] = node_data.est_refs[1] = node_data.est_refs[2] = static_cast<float>( ntk.fanout_size( n ) );

      if ( ntk.is_constant( n ) )
      {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = node_data.arrival[1] = 0.0f;
        match_constants( index );
      }
      else if ( ntk.is_ci( n ) )
      {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = 0.0f;
        /* PIs have the negative phase implemented with an inverter */
        node_data.arrival[1] = lib_inv_delay;
      }
    } );
  }

  void compute_matches()
  {
    /* match gates */
    ntk.foreach_gate( [&]( auto const& n ) {
      const auto index = ntk.node_to_index( n );

      std::vector<cut_match_tech<NInputs>> node_matches;

      auto i = 0u;
      for ( auto& cut : cuts.cuts( index ) )
      {
        /* ignore unit cut */
        if ( cut->size() == 1 && *cut->begin() == index )
        {
          ( *cut )->data.ignore = true;
          continue;
        }
        if ( cut->size() > NInputs )
        {
          /* Ignore cuts too big to be mapped using the library */
          ( *cut )->data.ignore = true;
          continue;
        }
        const auto tt = cuts.truth_table( *cut );
        const auto fe = kitty::extend_to<6>( tt );
        auto fe_canon = fe;

        uint8_t negations_pos = 0;
        uint8_t negations_neg = 0;

        /* match positive polarity */
        if constexpr ( Configuration == classification_type::p_configurations )
        {
          auto canon = kitty::exact_n_canonization( fe );
          fe_canon = std::get<0>( canon );
          negations_pos = std::get<1>( canon );
        }
        auto const supergates_pos = library.get_supergates( fe_canon );

        /* match negative polarity */
        if constexpr ( Configuration == classification_type::p_configurations )
        {
          auto canon = kitty::exact_n_canonization( ~fe );
          fe_canon = std::get<0>( canon );
          negations_neg = std::get<1>( canon );
        }
        else
        {
          fe_canon = ~fe;
        }
        auto const supergates_neg = library.get_supergates( fe_canon );

        if ( supergates_pos != nullptr || supergates_neg != nullptr )
        {
          cut_match_tech<NInputs> match{ { supergates_pos, supergates_neg }, { negations_pos, negations_neg } };

          node_matches.push_back( match );
          ( *cut )->data.match_index = i++;
        }
        else
        {
          /* Ignore not matched cuts */
          ( *cut )->data.ignore = true;
        }
      }

      matches[index] = node_matches;
    } );
  }

  template<bool DO_AREA>
  bool compute_mapping()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
      {
        continue;
      }

      /* match positive phase */
      match_phase<DO_AREA>( n, 0u );

      /* match negative phase */
      match_phase<DO_AREA>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>( n, 0 );
    }

    double area_old = area;
    bool success = set_mapping_refs<false>();

    /* round stats */
    if ( ps.verbose )
    {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if ( iteration != 1 )
        area_gain = float( ( area_old - area ) / area_old * 100 );

      if constexpr ( DO_AREA )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      }
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  template<bool SwitchActivity>
  bool compute_mapping_exact()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;

      auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      if ( node_data.same_match && node_data.map_refs[2] != 0 )
      {
        if ( node_data.best_supergate[0] != nullptr )
          cut_deref<SwitchActivity>( cuts.cuts( index )[node_data.best_cut[0]], n, 0u );
        else
          cut_deref<SwitchActivity>( cuts.cuts( index )[node_data.best_cut[1]], n, 1u );
      }

      /* match positive phase */
      match_phase_exact<SwitchActivity>( n, 0u );

      /* match negative phase */
      match_phase_exact<SwitchActivity>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<true, true>( n, 0 );
    }

    double area_old = area;
    bool success = set_mapping_refs<true>();

    /* round stats */
    if ( ps.verbose )
    {
      float area_gain = float( ( area_old - area ) / area_old * 100 );
      std::stringstream stats{};
      if constexpr ( SwitchActivity )
        stats << fmt::format( "[i] Switching: Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      else
        stats << fmt::format( "[i] Area     : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  template<bool ELA>
  bool set_mapping_refs()
  {
    const auto coef = 1.0f / ( 2.0f + ( iteration + 1 ) * ( iteration + 1 ) );

    if constexpr ( !ELA )
    {
      for ( auto i = 0u; i < node_match.size(); ++i )
      {
        node_match[i].map_refs[0] = node_match[i].map_refs[1] = node_match[i].map_refs[2] = 0u;
      }
    }

    /* compute the current worst delay and update the mapping refs */
    delay = 0.0f;
    ntk.foreach_co( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );

      if ( ntk.is_complemented( s ) )
        delay = std::max( delay, node_match[index].arrival[1] );
      else
        delay = std::max( delay, node_match[index].arrival[0] );

      if constexpr ( !ELA )
      {
        node_match[index].map_refs[2]++;
        if ( ntk.is_complemented( s ) )
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    } );

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    for ( auto it = top_order.rbegin(); it != top_order.rend(); ++it )
    {
      const auto index = ntk.node_to_index( *it );
      auto& node_data = node_match[index];

      /* skip constants and PIs */
      if ( ntk.is_constant( *it ) )
      {
        if ( node_match[index].map_refs[2] > 0u )
        {
          /* if used and not available in the library launch a mapping error */
          if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          {
            std::cerr << "[i] MAP ERROR: technology library does not contain constant gates, impossible to perform mapping" << std::endl;
            st.mapping_error = true;
            return false;
          }
        }
        continue;
      }
      else if ( ntk.is_ci( *it ) )
      {
        if ( node_match[index].map_refs[1] > 0u )
        {
          /* Add inverter area over the negated fanins */
          area += lib_inv_area;
        }
        continue;
      }

      /* continue if not referenced in the cover */
      if ( node_match[index].map_refs[2] == 0u )
        continue;

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;

      if ( node_data.best_supergate[use_phase] == nullptr )
      {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[i] MAP ERROR: technology library is not complete, impossible to perform mapping" << std::endl;
        st.mapping_error = true;
        return false;
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
          auto ctr = 0u;

          for ( auto const leaf : best_cut )
          {
            node_match[leaf].map_refs[2]++;
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
        {
          area += lib_inv_area;
        }
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if ( !node_data.same_match && node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
          auto ctr = 0u;
          for ( auto const leaf : best_cut )
          {
            node_match[leaf].map_refs[2]++;
            if ( ( node_data.phase[use_phase] >> ctr++ ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }
    }

    /* blend estimated references */
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      node_match[i].est_refs[2] = coef * node_match[i].est_refs[2] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[2] ) );
      node_match[i].est_refs[1] = coef * node_match[i].est_refs[1] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[1] ) );
      node_match[i].est_refs[0] = coef * node_match[i].est_refs[0] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[0] ) );
    }

    ++iteration;
    return true;
  }

  void compute_required_time()
  {
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required[0] = node_match[i].required[1] = std::numeric_limits<double>::max();
    }

    /* return in case of `skip_delay_round` */
    if ( iteration == 0 )
      return;

    auto required = delay;

    if ( ps.required_time != 0.0f )
    {
      /* Global target time constraint */
      if ( ps.required_time < delay - epsilon )
      {
        if ( !ps.skip_delay_round && iteration == 1 )
          std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {:.2f}", ps.required_time ) << std::endl;
      }
      else
      {
        required = ps.required_time;
      }
    }

    /* set the required time at POs */
    ntk.foreach_co( [&]( auto const& s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      if ( ntk.is_complemented( s ) )
        node_match[index].required[1] = required;
      else
        node_match[index].required[0] = required;
    } );

    /* propagate required time to the PIs */
    for ( auto it = top_order.rbegin(); it != top_order.rend(); ++it )
    {
      if ( ntk.is_ci( *it ) || ntk.is_constant( *it ) )
        break;

      const auto index = ntk.node_to_index( *it );

      if ( node_match[index].map_refs[2] == 0 )
        continue;

      auto& node_data = node_match[index];

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;
      unsigned other_phase = use_phase ^ 1;

      assert( node_data.best_supergate[0] != nullptr || node_data.best_supergate[1] != nullptr );
      assert( node_data.map_refs[0] || node_data.map_refs[1] );

      /* propagate required time over the output inverter if present */
      if ( node_data.same_match && node_data.map_refs[other_phase] > 0 )
      {
        node_data.required[use_phase] = std::min( node_data.required[use_phase], node_data.required[other_phase] - lib_inv_delay );
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        auto ctr = 0u;
        auto best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
        auto const& supergate = node_data.best_supergate[use_phase];
        for ( auto leaf : best_cut )
        {
          auto phase = ( node_data.phase[use_phase] >> ctr ) & 1;
          node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[use_phase] - supergate->tdelay[ctr] );
          ++ctr;
        }
      }

      if ( !node_data.same_match && node_data.map_refs[other_phase] > 0 )
      {
        auto ctr = 0u;
        auto best_cut = cuts.cuts( index )[node_data.best_cut[other_phase]];
        auto const& supergate = node_data.best_supergate[other_phase];
        for ( auto leaf : best_cut )
        {
          auto phase = ( node_data.phase[other_phase] >> ctr ) & 1;
          node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[other_phase] - supergate->tdelay[ctr] );
          ++ctr;
        }
      }
    }
  }

  template<bool DO_AREA>
  void match_phase( node<Ntk> const& n, uint8_t phase )
  {
    double best_arrival = std::numeric_limits<double>::max();
    double best_area_flow = std::numeric_limits<double>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area_flow = best_supergate->area + cut_leaves_flow( cut, n, phase );
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for ( auto l : cut )
      {
        double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_supergate->tdelay[ctr];
        best_arrival = std::max( best_arrival, arrival_pin );
        ++ctr;
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->data.ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index].supergates;
      auto const negation = cut_matches[( *cut )->data.match_index].negations[phase];

      if ( supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates[phase] )
      {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        double area_local = gate.area + cut_leaves_flow( *cut, n, phase );
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for ( auto l : *cut )
        {
          double arrival_pin = node_match[l].arrival[( gate_polarity >> ctr ) & 1] + gate.tdelay[ctr];
          worst_arrival = std::max( worst_arrival, arrival_pin );
          ++ctr;
        }

        if constexpr ( DO_AREA )
        {
          if ( worst_arrival > node_data.required[phase] + epsilon )
            continue;
        }

        if ( compare_map<DO_AREA>( worst_arrival, best_arrival, area_local, best_area_flow, cut->size(), best_size ) )
        {
          best_arrival = worst_arrival;
          best_area_flow = area_local;
          best_size = cut->size();
          best_cut = cut_index;
          best_area = gate.area;
          best_phase = gate_polarity;
          best_supergate = &gate;
        }
      }

      ++cut_index;
    }

    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
  }

  template<bool SwitchActivity>
  void match_phase_exact( node<Ntk> const& n, uint8_t phase )
  {
    double best_arrival = std::numeric_limits<double>::max();
    float best_exact_area = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    supergate<NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();

      auto ctr = 0u;
      for ( auto l : cut )
      {
        double arrival_pin = node_match[l].arrival[( best_phase >> ctr ) & 1] + best_supergate->tdelay[ctr];
        best_arrival = std::max( best_arrival, arrival_pin );
        ++ctr;
      }

      /* if cut is implemented, remove it from the cover */
      if ( !node_data.same_match && node_data.map_refs[phase] )
      {
        best_exact_area = cut_deref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
      }
      else
      {
        best_exact_area = cut_ref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
        cut_deref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->data.ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index].supergates;
      auto const negation = cut_matches[( *cut )->data.match_index].negations[phase];

      if ( supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates[phase] )
      {
        uint8_t gate_polarity = gate.polarity ^ negation;
        node_data.phase[phase] = gate_polarity;
        node_data.area[phase] = gate.area;
        float area_exact = cut_ref<SwitchActivity>( *cut, n, phase );
        cut_deref<SwitchActivity>( *cut, n, phase );
        double worst_arrival = 0.0f;

        auto ctr = 0u;
        for ( auto l : *cut )
        {
          double arrival_pin = node_match[l].arrival[( gate_polarity >> ctr ) & 1] + gate.tdelay[ctr];
          worst_arrival = std::max( worst_arrival, arrival_pin );
          ++ctr;
        }

        if ( worst_arrival > node_data.required[phase] + epsilon )
          continue;

        if ( compare_map<true>( worst_arrival, best_arrival, area_exact, best_exact_area, cut->size(), best_size ) )
        {
          best_arrival = worst_arrival;
          best_exact_area = area_exact;
          best_area = gate.area;
          best_size = cut->size();
          best_cut = cut_index;
          best_phase = gate_polarity;
          best_supergate = &gate;
        }
      }

      ++cut_index;
    }

    node_data.flows[phase] = best_exact_area;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;

    if ( !node_data.same_match && node_data.map_refs[phase] )
    {
      best_exact_area = cut_ref<SwitchActivity>( cuts.cuts( index )[best_cut], n, phase );
    }
  }

  template<bool DO_AREA, bool ELA>
  void match_drop_phase( node<Ntk> const& n, float required_margin_factor )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    /* compute arrival adding an inverter to the other match phase */
    double worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
    double worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
    bool use_zero = false;
    bool use_one = false;

    /* only one phase is matched */
    if ( node_data.best_supergate[0] == nullptr )
    {
      set_match_complemented_phase( index, 1, worst_arrival_npos );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
      }
      return;
    }
    else if ( node_data.best_supergate[1] == nullptr )
    {
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
      }
      return;
    }

    /* try to use only one match to cover both phases */
    if constexpr ( !DO_AREA )
    {
      /* if arrival improves matching the other phase and inserting an inverter */
      if ( worst_arrival_npos < node_data.arrival[0] + epsilon )
      {
        use_one = true;
      }
      if ( worst_arrival_nneg < node_data.arrival[1] + epsilon )
      {
        use_zero = true;
      }
    }
    else
    {
      /* check if both phases + inverter meet the required time */
      use_zero = worst_arrival_nneg < ( node_data.required[1] + epsilon - required_margin_factor * lib_inv_delay );
      use_one = worst_arrival_npos < ( node_data.required[0] + epsilon - required_margin_factor * lib_inv_delay );
    }

    /* condition on not used phases, evaluate a substitution during exact area recovery */
    if constexpr ( ELA )
    {
      if ( iteration != 0 )
      {
        if ( node_data.map_refs[0] == 0 || node_data.map_refs[1] == 0 )
        {
          /* select the used match */
          auto phase = 0;
          auto nphase = 0;
          if ( node_data.map_refs[0] == 0 )
          {
            phase = 1;
            use_one = true;
            use_zero = false;
          }
          else
          {
            nphase = 1;
            use_one = false;
            use_zero = true;
          }
          /* select the not used match instead if it leads to area improvement and doesn't violate the required time */
          if ( node_data.arrival[nphase] + lib_inv_delay < node_data.required[phase] + epsilon )
          {
            auto size_phase = cuts.cuts( index )[node_data.best_cut[phase]].size();
            auto size_nphase = cuts.cuts( index )[node_data.best_cut[nphase]].size();

            if ( compare_map<DO_AREA>( node_data.arrival[nphase] + lib_inv_delay, node_data.arrival[phase], node_data.flows[nphase] + lib_inv_area, node_data.flows[phase], size_nphase, size_phase ) )
            {
              /* invert the choice */
              use_zero = !use_zero;
              use_one = !use_one;
            }
          }
        }
      }
    }

    if ( !use_zero && !use_one )
    {
      /* use both phases */
      node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
      node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
      node_data.flows[2] = node_data.flows[0] + node_data.flows[1];
      node_data.same_match = false;
      return;
    }

    /* use area flow as a tiebreaker */
    if ( use_zero && use_one )
    {
      auto size_zero = cuts.cuts( index )[node_data.best_cut[0]].size();
      auto size_one = cuts.cuts( index )[node_data.best_cut[1]].size();
      if ( compare_map<DO_AREA>( worst_arrival_nneg, worst_arrival_npos, node_data.flows[0], node_data.flows[1], size_zero, size_one ) )
        use_one = false;
      else
        use_zero = false;
    }

    if ( use_zero )
    {
      if constexpr ( ELA )
      {
        /* set cut references */
        if ( !node_data.same_match )
        {
          /* dereference the negative phase cut if in use */
          if ( node_data.map_refs[1] > 0 )
            cut_deref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
          /* reference the positive cut if not in use before */
          if ( node_data.map_refs[0] == 0 && node_data.map_refs[2] )
            cut_ref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
        }
        else if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
      }
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
    }
    else
    {
      if constexpr ( ELA )
      {
        /* set cut references */
        if ( !node_data.same_match )
        {
          /* dereference the positive phase cut if in use */
          if ( node_data.map_refs[0] > 0 )
            cut_deref<false>( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
          /* reference the negative cut if not in use before */
          if ( node_data.map_refs[1] == 0 && node_data.map_refs[2] )
            cut_ref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
        }
        else if ( node_data.map_refs[2] )
          cut_ref<false>( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
      }
      set_match_complemented_phase( index, 1, worst_arrival_npos );
    }
  }

  inline void set_match_complemented_phase( uint32_t index, uint8_t phase, double worst_arrival_n )
  {
    auto& node_data = node_match[index];
    auto phase_n = phase ^ 1;
    node_data.same_match = true;
    node_data.best_supergate[phase_n] = nullptr;
    node_data.best_cut[phase_n] = node_data.best_cut[phase];
    node_data.phase[phase_n] = node_data.phase[phase];
    node_data.arrival[phase_n] = worst_arrival_n;
    node_data.area[phase_n] = node_data.area[phase];
    node_data.flows[phase] = node_data.flows[phase] / node_data.est_refs[2];
    node_data.flows[phase_n] = node_data.flows[phase];
    node_data.flows[2] = node_data.flows[phase];
  }

  void match_constants( uint32_t index )
  {
    auto& node_data = node_match[index];

    kitty::static_truth_table<6> zero_tt;
    auto const supergates_zero = library.get_supergates( zero_tt );
    auto const supergates_one = library.get_supergates( ~zero_tt );

    /* Not available in the library */
    if ( supergates_zero == nullptr && supergates_one == nullptr )
    {
      return;
    }
    /* if only one is available, the other is obtained using an inverter */
    if ( supergates_zero != nullptr )
    {
      node_data.best_supergate[0] = &( ( *supergates_zero )[0] );
      node_data.arrival[0] = node_data.best_supergate[0]->tdelay[0];
      node_data.area[0] = node_data.best_supergate[0]->area;
      node_data.phase[0] = 0;
    }
    if ( supergates_one != nullptr )
    {
      node_data.best_supergate[1] = &( ( *supergates_one )[0] );
      node_data.arrival[1] = node_data.best_supergate[1]->tdelay[0];
      node_data.area[1] = node_data.best_supergate[1]->area;
      node_data.phase[1] = 0;
    }
    else
    {
      node_data.same_match = true;
      node_data.arrival[1] = node_data.arrival[0] + lib_inv_delay;
      node_data.area[1] = node_data.area[0] + lib_inv_area;
      node_data.phase[1] = 1;
    }
    if ( supergates_zero == nullptr )
    {
      node_data.same_match = true;
      node_data.arrival[0] = node_data.arrival[1] + lib_inv_delay;
      node_data.area[0] = node_data.area[1] + lib_inv_area;
      node_data.phase[0] = 1;
    }
  }

  inline double cut_leaves_flow( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    double flow{ 0.0f };
    auto const& node_data = node_match[ntk.node_to_index( n )];

    uint8_t ctr = 0u;
    for ( auto leaf : cut )
    {
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;
      flow += node_match[leaf].flows[leaf_phase];
    }

    return flow;
  }

  template<bool SwitchActivity>
  float cut_ref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }
      else if ( ntk.is_ci( ntk.index_to_node( leaf ) ) )
      {
        /* reference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( node_match[leaf].map_refs[1]++ == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          ++node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if not present yet and leaf node is implemented in the opposite phase */
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u && node_match[leaf].best_supergate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive referencing if leaf was not referenced */
        if ( node_match[leaf].map_refs[2]++ == 0u )
        {
          count += cut_ref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        ++node_match[leaf].map_refs[2];
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u )
        {
          count += cut_ref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }

  template<bool SwitchActivity>
  float cut_deref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    float count;

    if constexpr ( SwitchActivity )
      count = switch_activity[ntk.node_to_index( n )];
    else
      count = node_data.area[phase];

    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> ctr++ ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        continue;
      }
      else if ( ntk.is_ci( ntk.index_to_node( leaf ) ) )
      {
        /* dereference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( --node_match[leaf].map_refs[1] == 0u )
          {
            if constexpr ( SwitchActivity )
              count += switch_activity[leaf];
            else
              count += lib_inv_area;
          }
        }
        else
        {
          --node_match[leaf].map_refs[0];
        }
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if it is used only by the current gate and leaf node is implemented in the opposite phase */
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u && node_match[leaf].best_supergate[leaf_phase] == nullptr )
        {
          if constexpr ( SwitchActivity )
            count += switch_activity[leaf];
          else
            count += lib_inv_area;
        }
        /* Recursive dereferencing */
        if ( --node_match[leaf].map_refs[2] == 0u )
        {
          count += cut_deref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        --node_match[leaf].map_refs[2];
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u )
        {
          count += cut_deref<SwitchActivity>( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
    }
    return count;
  }

  void insert_buffers()
  {
    if ( lib_buf_id != UINT32_MAX )
    {
      double area_old = area;
      bool buffers = false;

      ntk.foreach_co( [&]( auto const& f ) {
        auto const& n = ntk.get_node( f );
        if ( !ntk.is_constant( n ) && ntk.is_ci( n ) && !ntk.is_complemented( f ) )
        {
          area += lib_buf_area;
          delay = std::max( delay, node_match[ntk.node_to_index( n )].arrival[0] + lib_inv_delay );
          buffers = true;
        }
      } );

      /* round stats */
      if ( ps.verbose && buffers )
      {
        std::stringstream stats{};
        float area_gain = 0.0f;

        area_gain = float( ( area_old - area ) / area_old * 100 );

        stats << fmt::format( "[i] Buffering: Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
        st.round_stats.push_back( stats.str() );
      }
    }
  }

  std::pair<map_ntk_t, klut_map> initialize_map_network()
  {
    map_ntk_t dest( library.get_gates() );
    klut_map old2new;

    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][0] = dest.get_constant( false );
    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][1] = dest.get_constant( true );

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_pi();
    } );

    return { dest, old2new };
  }

  std::pair<seq_map_ntk_t, klut_map> initialize_map_seq_network()
  {
    seq_map_ntk_t dest( library.get_gates() );
    klut_map old2new;

    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][0] = dest.get_constant( false );
    old2new[ntk.node_to_index( ntk.get_node( ntk.get_constant( false ) ) )][1] = dest.get_constant( true );

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_pi();
    } );
    ntk.foreach_ro( [&]( auto const& n ) {
      old2new[ntk.node_to_index( n )][0] = dest.create_ro();
    } );

    return { dest, old2new };
  }

  template<class NtkDest>
  void finalize_cover( NtkDest& res, klut_map& old2new )
  {
    for ( auto const& n : top_order )
    {
      auto index = ntk.node_to_index( n );
      auto const& node_data = node_match[index];

      /* add inverter at PI if needed */
      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_ci( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
        {
          old2new[index][1] = res.create_not( old2new[n][0] );
          res.add_binding( res.get_node( old2new[index][1] ), lib_inv_id );
        }
        continue;
      }

      /* continue if cut is not in the cover */
      if ( node_data.map_refs[2] == 0u )
        continue;

      unsigned phase = ( node_data.best_supergate[0] != nullptr ) ? 0 : 1;

      /* add used cut */
      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate<NtkDest>( res, old2new, index, phase );

        /* add inverted version if used */
        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
        {
          old2new[index][phase ^ 1] = res.create_not( old2new[index][phase] );
          res.add_binding( res.get_node( old2new[index][phase ^ 1] ), lib_inv_id );
        }
      }

      phase = phase ^ 1;
      /* add the optional other match if used */
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        create_lut_for_gate<NtkDest>( res, old2new, index, phase );
      }
    }

    /* create POs */
    ntk.foreach_po( [&]( auto const& f ) {
      if ( ntk.is_complemented( f ) )
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
      }
      else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_ci( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
      {
        /* create buffers for POs */
        static uint64_t _buf = 0x2;
        kitty::dynamic_truth_table tt_buf( 1 );
        kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
        const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
        res.create_po( buf );
        res.add_binding( res.get_node( buf ), lib_buf_id );
      }
      else
      {
        res.create_po( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
      }
    } );

    if constexpr ( has_foreach_ri_v<Ntk> )
    {
      ntk.foreach_ri( [&]( auto const& f ) {
        if ( ntk.is_complemented( f ) )
        {
          res.create_ri( old2new[ntk.node_to_index( ntk.get_node( f ) )][1] );
        }
        else if ( !ntk.is_constant( ntk.get_node( f ) ) && ntk.is_ci( ntk.get_node( f ) ) && lib_buf_id != UINT32_MAX )
        {
          /* create buffers for RIs */
          static uint64_t _buf = 0x2;
          kitty::dynamic_truth_table tt_buf( 1 );
          kitty::create_from_words( tt_buf, &_buf, &_buf + 1 );
          const auto buf = res.create_node( { old2new[ntk.node_to_index( ntk.get_node( f ) )][0] }, tt_buf );
          res.create_ri( buf );
          res.add_binding( res.get_node( buf ), lib_buf_id );
        }
        else
        {
          res.create_ri( old2new[ntk.node_to_index( ntk.get_node( f ) )][0] );
        }
      } );
    }

    /* write final results */
    st.area = area;
    st.delay = delay;
    if ( ps.eswp_rounds )
      st.power = compute_switching_power();
  }

  template<class NtkDest>
  void create_lut_for_gate( NtkDest& res, klut_map& old2new, uint32_t index, unsigned phase )
  {
    auto const& node_data = node_match[index];
    auto& best_cut = cuts.cuts( index )[node_data.best_cut[phase]];
    auto const& gate = node_data.best_supergate[phase]->root;

    /* permutate and negate to obtain the matched gate truth table */
    std::vector<signal<klut_network>> children( gate->num_vars );

    auto ctr = 0u;
    for ( auto l : best_cut )
    {
      if ( ctr >= gate->num_vars )
        break;
      children[node_data.best_supergate[phase]->permutation[ctr]] = old2new[l][( node_data.phase[phase] >> ctr ) & 1];
      ++ctr;
    }

    if ( !gate->is_super )
    {
      /* create the node */
      auto f = res.create_node( children, gate->function );
      res.add_binding( res.get_node( f ), gate->root->id );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
    else
    {
      /* supergate, create sub-gates */
      auto f = create_lut_for_gate_rec<NtkDest>( res, *gate, children );

      /* add the node in the data structure */
      old2new[index][phase] = f;
    }
  }

  template<class NtkDest>
  signal<klut_network> create_lut_for_gate_rec( NtkDest& res, composed_gate<NInputs> const& gate, std::vector<signal<klut_network>> const& children )
  {
    std::vector<signal<klut_network>> children_local( gate.fanin.size() );

    auto i = 0u;
    for ( auto const fanin : gate.fanin )
    {
      if ( fanin->root == nullptr )
      {
        /* terminal condition */
        children_local[i] = children[fanin->id];
      }
      else
      {
        children_local[i] = create_lut_for_gate_rec<NtkDest>( res, *fanin, children );
      }
      ++i;
    }

    auto f = res.create_node( children_local, gate.root->function );
    res.add_binding( res.get_node( f ), gate.root->id );
    return f;
  }

  template<bool DO_AREA>
  inline bool compare_map( double arrival, double best_arrival, double area_flow, double best_area_flow, uint32_t size, uint32_t best_size )
  {
    if constexpr ( DO_AREA )
    {
      if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
      else if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
    }
    else
    {
      if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
      else if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
    }
    if ( size < best_size )
    {
      return true;
    }
    return false;
  }

  double compute_switching_power()
  {
    double power = 0.0f;

    for ( auto const& n : top_order )
    {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      if ( ntk.is_constant( n ) )
      {
        if ( node_data.best_supergate[0] == nullptr && node_data.best_supergate[1] == nullptr )
          continue;
      }
      else if ( ntk.is_ci( n ) )
      {
        if ( node_data.map_refs[1] > 0 )
          power += switch_activity[ntk.node_to_index( n )];
        continue;
      }

      /* continue if cut is not in the cover */
      if ( node_match[index].map_refs[2] == 0u )
        continue;

      unsigned phase = ( node_data.best_supergate[0] != nullptr ) ? 0 : 1;

      if ( node_data.same_match || node_data.map_refs[phase] > 0 )
      {
        power += switch_activity[ntk.node_to_index( n )];

        if ( node_data.same_match && node_data.map_refs[phase ^ 1] > 0 )
          power += switch_activity[ntk.node_to_index( n )];
      }

      phase = phase ^ 1;
      if ( !node_data.same_match && node_data.map_refs[phase] > 0 )
      {
        power += switch_activity[ntk.node_to_index( n )];
      }
    }

    return power;
  }

private:
  Ntk const& ntk;
  tech_library<NInputs, Configuration> const& library;
  map_params const& ps;
  map_stats& st;

  uint32_t iteration{ 0 };       /* current mapping iteration */
  double delay{ 0.0f };          /* current delay of the mapping */
  double area{ 0.0f };           /* current area of the mapping */
  const float epsilon{ 0.005f }; /* epsilon */

  /* lib inverter info */
  float lib_inv_area;
  float lib_inv_delay;
  uint32_t lib_inv_id;

  /* lib buffer info */
  float lib_buf_area;
  float lib_buf_delay;
  uint32_t lib_buf_id;

  std::vector<node<Ntk>> top_order;
  std::vector<node_match_tech<NInputs>> node_match;
  match_map matches;
  std::vector<float> switch_activity;
  network_cuts_t cuts;
};

} /* namespace detail */

/*! \brief Technology mapping.
 *
 * This function implements a technology mapping algorithm. It is controlled by a
 * template argument `CutData` (defaulted to `cut_enumeration_tech_map_cut`).
 * The argument is similar to the `CutData` argument in `cut_enumeration`, which can
 * specialize the cost function to select priority cuts and store additional data.
 * The default argument gives priority firstly to the cut size, then delay, and lastly
 * to area flow. Thus, it is more suited for delay-oriented mapping.
 * The type passed as `CutData` must implement the following four fields:
 *
 * - `uint32_t delay`
 * - `float flow`
 * - `uint8_t match_index`
 * - `bool ignore`
 *
 * See `include/mockturtle/algorithms/cut_enumeration/cut_enumeration_tech_map_cut.hpp`
 * for one example of a CutData type that implements the cost function that is used in
 * the technology mapper.
 *
 * The function takes the size of the cuts in the template parameter `CutSize`.
 *
 * The function returns a k-LUT network. Each LUT abstracts a gate of the technology library.
 *
 * **Required network functions:**
 * - `size`
 * - `is_ci`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_co`
 * - `foreach_node`
 * - `fanout_size`
 *
 * \param ntk Network
 * \param library Technology library
 * \param ps Mapping params
 * \param pst Mapping statistics
 *
 * The implementation of this algorithm was inspired by the
 * mapping command ``map`` in ABC.
 */
template<class Ntk, unsigned CutSize = 5u, typename CutData = cut_enumeration_tech_map_cut, unsigned NInputs, classification_type Configuration>
binding_view<klut_network> map( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, map_params const& ps = {}, map_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  map_stats st;
  detail::tech_map_impl<Ntk, CutSize, CutData, NInputs, Configuration> p( ntk, library, ps, st );
  auto res = p.run();

  st.time_total = st.time_mapping + st.cut_enumeration_st.time_total;
  if ( ps.verbose && !st.mapping_error )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
  return res;
}

/*! \brief Technology mapping for sequential networks.
 *
 * Version of `map` for technology mapping of sequential networks.
 *
 * The function returns a sequential k-LUT network. Each LUT abstracts a gate of the technology library.
 *
 * **Required network functions:**
 * - `size`
 * - `is_ci`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_pi`
 * - `foreach_po`
 * - `foreach_ro`
 * - `foreach_co`
 * - `foreach_ri`
 * - `foreach_node`
 * - `fanout_size`
 *
 * \param ntk Sequential network
 * \param library Technology library
 * \param ps Mapping params
 * \param pst Mapping statistics
 *
 */
template<class Ntk, unsigned CutSize = 5u, typename CutData = cut_enumeration_tech_map_cut, unsigned NInputs, classification_type Configuration>
binding_view<sequential<klut_network>> seq_map( Ntk const& ntk, tech_library<NInputs, Configuration> const& library, map_params const& ps = {}, map_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co method" );
  static_assert( has_foreach_ri_v<Ntk>, "Ntk does not implement the has_foreach_ri method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the has_foreach_po method" );
  static_assert( has_foreach_ro_v<Ntk>, "Ntk does not implement the has_foreach_ro method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );

  map_stats st;
  detail::tech_map_impl<Ntk, CutSize, CutData, NInputs, Configuration> p( ntk, library, ps, st );
  auto res = p.run_seq();

  st.time_total = st.time_mapping + st.cut_enumeration_st.time_total;
  if ( ps.verbose && !st.mapping_error )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
  return res;
}

namespace detail
{

template<typename Ntk, unsigned NInputs>
struct cut_match_t
{
  /* list of supergates matching the cut for positive and negative output phases */
  std::vector<exact_supergate<Ntk, NInputs>> const* supergates[2] = { nullptr, nullptr };
  /* input permutations, at index i, it contains the permutated position of i */
  std::array<uint8_t, NInputs> permutation{};
  /* permutated input negations */
  uint8_t negation{ 0 };
};

template<typename Ntk, unsigned NInputs>
struct node_match_t
{
  /* best supergate match for positive and negative output phases */
  exact_supergate<Ntk, NInputs> const* best_supergate[2] = { nullptr, nullptr };
  /* fanin pin phases for both output phases */
  uint8_t phase[2];
  /* best cut index for both phases */
  uint32_t best_cut[2];
  /* node is mapped using only one phase */
  bool same_match{ false };

  /* arrival time at node output */
  double arrival[2];
  /* required time at node output */
  double required[2];
  /* area of the best matches */
  float area[2];

  /* number of references in the cover 0: pos, 1: neg, 2: pos+neg */
  uint32_t map_refs[3];
  /* references estimation */
  float est_refs[3];
  /* area flow */
  float flows[3];
};

template<class NtkDest, unsigned CutSize, typename CutData, class Ntk, unsigned NInputs>
class exact_map_impl
{
public:
  static constexpr uint32_t max_window_size = 12;
  using network_cuts_t = fast_network_cuts<Ntk, CutSize, true, CutData>;
  using cut_t = typename network_cuts_t::cut_t;

public:
  explicit exact_map_impl( Ntk& ntk, exact_library<NtkDest, NInputs> const& library, map_params const& ps, map_stats& st )
      : ntk( ntk ),
        library( library ),
        ps( ps ),
        st( st ),
        lib_database( library.get_database() ),
        node_match( ntk.size() ),
        matches(),
        cuts( fast_cut_enumeration<Ntk, CutSize, true, CutData>( ntk, ps.cut_enumeration_ps ) )
  {
    std::tie( lib_inv_area, lib_inv_delay ) = library.get_inverter_info();
  }

  NtkDest run()
  {
    stopwatch t( st.time_mapping );

    auto [res, old2new] = initialize_dest();

    /* compute and save topological order */
    top_order.reserve( ntk.size() );
    topo_view<Ntk>( ntk ).foreach_node( [this]( auto n ) {
      top_order.push_back( n );
    } );

    /* match cuts with gates */
    if ( ps.use_dont_cares )
    {
      compute_matches_dc();
    }
    else
    {
      compute_matches();
    }

    /* init the data structure */
    init_nodes();

    /* compute mapping delay */
    if ( !ps.skip_delay_round )
    {
      if ( !compute_mapping<false>() )
      {
        return res;
      }
    }

    /* compute mapping using global area flow */
    while ( iteration < ps.area_flow_rounds + 1 )
    {
      compute_required_time();
      if ( !compute_mapping<true>() )
      {
        return res;
      }
    }

    /* compute mapping using exact area */
    while ( iteration < ps.ela_rounds + ps.area_flow_rounds + 1 )
    {
      compute_required_time();
      if ( ps.enable_logic_sharing && iteration == ps.ela_rounds + ps.area_flow_rounds )
      {
        if ( !compute_exact_area_aggressive( res, old2new ) )
        {
          return res;
        }
      }
      else
      {
        if ( !compute_exact_area() )
        {
          return res;
        }
      }
    }

    /* generate the output network using the computed mapping */
    finalize_cover( res, old2new );

    if ( ps.enable_logic_sharing )
      return cleanup_dangling( res );
    else
      return res;
  }

private:
  void init_nodes()
  {
    ntk.foreach_node( [this]( auto const& n, auto ) {
      const auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      node_data.est_refs[0] = node_data.est_refs[1] = node_data.est_refs[2] = static_cast<float>( ntk.fanout_size( n ) );

      if ( ntk.is_constant( n ) )
      {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = node_data.arrival[1] = 0.0f;
      }
      else if ( ntk.is_ci( n ) )
      {
        /* all terminals have flow 1.0 */
        node_data.flows[0] = node_data.flows[1] = node_data.flows[2] = 0.0f;
        node_data.arrival[0] = 0.0f;
        /* PIs have the negative phase implemented with an inverter */
        node_data.arrival[1] = lib_inv_delay;
      }
    } );
  }

  void compute_matches()
  {
    /* match gates */
    ntk.foreach_gate( [&]( auto const& n ) {
      const auto index = ntk.node_to_index( n );

      std::vector<cut_match_t<NtkDest, NInputs>> node_matches;

      auto i = 0u;
      for ( auto& cut : cuts.cuts( index ) )
      {
        /* ignore unit cut */
        if ( cut->size() == 1 && *cut->begin() == index )
        {
          ( *cut )->data.ignore = true;
          continue;
        }

        if ( cut->size() > NInputs )
        {
          /* Ignore cuts too big to be mapped using the library */
          ( *cut )->data.ignore = true;
          continue;
        }

        /* match the cut using canonization and get the gates */
        const auto tt = cuts.truth_table( *cut );
        const auto fe = kitty::extend_to<NInputs>( tt );
        const auto config = kitty::exact_npn_canonization( fe );
        auto const supergates_npn = library.get_supergates( std::get<0>( config ) );
        auto const supergates_npn_neg = library.get_supergates( ~std::get<0>( config ) );

        if ( supergates_npn != nullptr || supergates_npn_neg != nullptr )
        {
          auto neg = std::get<1>( config );
          auto perm = std::get<2>( config );
          uint8_t phase = ( neg >> NInputs ) & 1;
          cut_match_t<NtkDest, NInputs> match;

          match.supergates[phase] = supergates_npn;
          match.supergates[phase ^ 1] = supergates_npn_neg;

          /* store permutations and negations */
          match.negation = 0;
          for ( auto j = 0u; j < perm.size() && j < NInputs; ++j )
          {
            match.permutation[perm[j]] = j;
            match.negation |= ( ( neg >> perm[j] ) & 1 ) << j;
          }
          node_matches.push_back( match );
          ( *cut )->data.match_index = i++;
        }
        else
        {
          /* Ignore not matched cuts */
          ( *cut )->data.ignore = true;
        }
      }

      matches[index] = node_matches;
    } );
  }

  void compute_matches_dc()
  {
    reconvergence_driven_cut_parameters rps;
    rps.max_leaves = ps.window_size;
    reconvergence_driven_cut_statistics rst;
    detail::reconvergence_driven_cut_impl<Ntk, false, false> reconv_cuts( ntk, rps, rst );

    color_view<Ntk> color_ntk{ ntk };
    std::array<uint32_t, NInputs> divisors;
    for ( uint32_t i = 0; i < NInputs; ++i )
    {
      divisors[i] = i;
    }

    /* match gates */
    ntk.foreach_gate( [&]( auto const& n ) {
      const auto index = ntk.node_to_index( n );
      std::vector<cut_match_t<NtkDest, NInputs>> node_matches;

      std::vector<node<Ntk>> roots = { n };
      auto const extended_leaves = reconv_cuts.run( roots ).first;

      std::vector<node<Ntk>> gates{ collect_nodes( color_ntk, extended_leaves, roots ) };
      window_view window_ntk{ color_ntk, extended_leaves, roots, gates };

      default_simulator<kitty::static_truth_table<max_window_size>> sim;
      const auto tts = simulate_nodes<kitty::static_truth_table<max_window_size>>( window_ntk, sim );

      auto i = 0u;
      for ( auto& cut : cuts.cuts( index ) )
      {
        /* ignore unit cut */
        if ( cut->size() == 1 && *cut->begin() == index )
        {
          ( *cut )->data.ignore = true;
          continue;
        }

        if ( cut->size() > NInputs )
        {
          /* Ignore cuts too big to be mapped using the library */
          ( *cut )->data.ignore = true;
          continue;
        }

        /* match the cut using canonization and get the gates */
        const auto tt = cuts.truth_table( *cut );
        const auto fe = kitty::shrink_to<NInputs>( tt );

        auto [tt_npn, neg, perm] = kitty::exact_npn_canonization( fe );
        auto perm_neg = perm;
        auto neg_neg = neg;

        /* dont cares computation */
        kitty::static_truth_table<NInputs> care;

        bool containment = true;
        bool filter = false;
        for ( auto const& l : *cut )
        {
          if ( color_ntk.color( ntk.index_to_node( l ) ) != color_ntk.current_color() )
          {
            containment = false;
            break;
          }
        }

        if ( containment )
        {
          /* compute care set */
          for ( auto i = 0u; i < ( 1u << window_ntk.num_pis() ); ++i )
          {
            uint32_t entry{ 0u };
            auto j = 0u;
            for ( auto const& l : *cut )
            {
              entry |= kitty::get_bit( tts[l], i ) << j;
              ++j;
            }
            kitty::set_bit( care, entry );
          }
        }
        else
        {
          /* completely specified */
          care = ~care;
        }

        auto const dc_npn = apply_npn_transformation( ~care, neg & ~( 1 << NInputs ), perm );
        const std::vector<exact_supergate<NtkDest, NInputs>>* supergates_npn = library.get_supergates( tt_npn, dc_npn, neg, perm );
        const std::vector<exact_supergate<NtkDest, NInputs>>* supergates_npn_neg = library.get_supergates( ~tt_npn, dc_npn, neg_neg, perm_neg );

        if ( supergates_npn != nullptr || supergates_npn_neg != nullptr )
        {
          cut_match_t<NtkDest, NInputs> match;

          if ( supergates_npn == nullptr )
          {
            perm = perm_neg;
            neg = neg_neg;
          }

          uint8_t phase = ( neg >> NInputs ) & 1;

          match.supergates[phase] = supergates_npn;
          match.supergates[phase ^ 1] = supergates_npn_neg;

          /* store permutations and negations */
          match.negation = 0;
          for ( auto j = 0u; j < perm.size() && j < NInputs; ++j )
          {
            match.permutation[perm[j]] = j;
            match.negation |= ( ( neg >> perm[j] ) & 1 ) << j;
          }
          node_matches.push_back( match );
          ( *cut )->data.match_index = i++;
        }
        else
        {
          /* Ignore not matched cuts */
          ( *cut )->data.ignore = true;
        }
      }

      matches[index] = node_matches;
    } );
  }

  template<bool DO_AREA>
  bool compute_mapping()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;

      /* match positive phase */
      match_phase<DO_AREA>( n, 0u );

      /* match negative phase */
      match_phase<DO_AREA>( n, 1u );

      /* try to drop one phase */
      match_drop_phase<DO_AREA, false>( n, 0u );
    }

    double area_old = area;
    bool success = set_mapping_refs<false>();

    /* round stats */
    if ( ps.verbose )
    {
      std::stringstream stats{};
      float area_gain = 0.0f;

      if ( iteration != 1 )
        area_gain = float( ( area_old - area ) / area_old * 100 );

      if constexpr ( DO_AREA )
      {
        stats << fmt::format( "[i] AreaFlow : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      }
      else
      {
        stats << fmt::format( "[i] Delay    : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      }
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  bool compute_exact_area()
  {
    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;

      auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      if ( node_data.same_match && node_data.map_refs[2] != 0 )
      {
        if ( node_data.best_supergate[0] != nullptr )
          cut_deref( cuts.cuts( index )[node_data.best_cut[0]], n, 0u );
        else
          cut_deref( cuts.cuts( index )[node_data.best_cut[1]], n, 1u );
      }

      /* match positive phase */
      match_phase_exact( n, 0u );

      /* match negative phase */
      match_phase_exact( n, 1u );

      /* try to drop one phase */
      match_drop_phase<true, true>( n, 0u );
    }

    double area_old = area;
    bool success = set_mapping_refs<true>();

    /* round stats */
    if ( ps.verbose )
    {
      float area_gain = float( ( area_old - area ) / area_old * 100 );
      std::stringstream stats{};
      stats << fmt::format( "[i] Area     : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  std::pair<NtkDest, node_map<signal<NtkDest>, Ntk>> initialize_dest()
  {
    node_map<signal<NtkDest>, Ntk> old2new( ntk );
    NtkDest dest;

    old2new[ntk.get_constant( false )] = dest.get_constant( false );
    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
    {
      old2new[ntk.get_constant( true )] = dest.get_constant( true );
    }

    ntk.foreach_pi( [&]( auto const& n ) {
      old2new[n] = dest.create_pi();
    } );

    if constexpr ( has_foreach_ro_v<Ntk> )
    {
      ntk.foreach_ro( [&]( auto const& n ) {
        old2new[n] = dest.create_ro();
      } );
    }

    return { dest, old2new };
  }

  void finalize_cover( NtkDest& res, node_map<signal<NtkDest>, Ntk>& old2new )
  {
    if ( !ps.enable_logic_sharing || iteration == ps.area_flow_rounds + 1 )
    {
      auto const& db = library.get_database();

      ntk.foreach_node( [&]( auto const& n ) {
        if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
          return true;
        auto index = ntk.node_to_index( n );
        if ( node_match[index].map_refs[2] == 0u )
          return true;

        /* get the implemented phase and map the best cut */
        unsigned phase = ( node_match[index].best_supergate[0] != nullptr ) ? 0 : 1;
        auto& best_cut = cuts.cuts( index )[node_match[index].best_cut[phase]];

        std::vector<signal<NtkDest>> children( NInputs, res.get_constant( false ) );
        auto const& match = matches[index][best_cut->data.match_index];
        auto const& supergate = node_match[index].best_supergate[phase];
        auto ctr = 0u;
        for ( auto l : best_cut )
        {
          children[match.permutation[ctr++]] = old2new[ntk.index_to_node( l )];
        }
        for ( auto i = 0u; i < NInputs; ++i )
        {
          if ( ( match.negation >> i ) & 1 )
          {
            children[i] = !children[i];
          }
        }
        topo_view topo{ db, supergate->root };
        auto f = cleanup_dangling( topo, res, children.begin(), children.end() ).front();

        if ( phase == 1 )
          f = !f;

        old2new[n] = f;
        return true;
      } );
    }

    /* create POs */
    ntk.foreach_po( [&]( auto const& f ) {
      res.create_po( ntk.is_complemented( f ) ? res.create_not( old2new[f] ) : old2new[f] );
    } );

    if constexpr ( has_foreach_ri_v<Ntk> )
    {
      ntk.foreach_ri( [&]( auto const& f ) {
        res.create_ri( ntk.is_complemented( f ) ? res.create_not( old2new[f] ) : old2new[f] );
      } );
    }

    /* write final results */
    st.area = area;
    st.delay = delay;
  }

  template<bool ELA>
  bool set_mapping_refs()
  {
    const auto coef = 1.0f / ( 2.0f + ( iteration + 1 ) * ( iteration + 1 ) );

    if constexpr ( !ELA )
    {
      for ( auto i = 0u; i < node_match.size(); ++i )
      {
        node_match[i].map_refs[0] = node_match[i].map_refs[1] = node_match[i].map_refs[2] = 0u;
      }
    }

    /* compute current delay and update mapping refs */
    delay = 0.0f;
    ntk.foreach_co( [this]( auto s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      if ( ntk.is_complemented( s ) )
        delay = std::max( delay, node_match[index].arrival[1] );
      else
        delay = std::max( delay, node_match[index].arrival[0] );

      if constexpr ( !ELA )
      {
        node_match[index].map_refs[2]++;
        if ( ntk.is_complemented( s ) )
          node_match[index].map_refs[1]++;
        else
          node_match[index].map_refs[0]++;
      }
    } );

    /* compute current area and update mapping refs in top-down order */
    area = 0.0f;
    for ( auto it = top_order.rbegin(); it != top_order.rend(); ++it )
    {
      const auto index = ntk.node_to_index( *it );
      /* skip constants and PIs */
      if ( ntk.is_constant( *it ) )
      {
        continue;
      }
      else if ( ntk.is_ci( *it ) )
      {
        if ( node_match[index].map_refs[1] > 0u )
        {
          /* Add inverter over the negated fanins */
          area += lib_inv_area;
        }
        continue;
      }

      if ( node_match[index].map_refs[2] == 0u )
        continue;

      auto& node_data = node_match[index];
      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;

      if ( node_data.best_supergate[use_phase] == nullptr )
      {
        /* Library is not complete, mapping is not possible */
        std::cerr << "[i] MAP ERROR: library is not complete, impossible to perform mapping" << std::endl;
        st.mapping_error = true;
        return false;
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
          auto const& match = matches[index][best_cut->data.match_index];
          auto ctr = 0u;
          for ( auto const leaf : best_cut )
          {
            node_match[leaf].map_refs[2]++;
            if ( ( node_data.phase[use_phase] >> match.permutation[ctr++] ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
        if ( node_data.same_match && node_data.map_refs[use_phase ^ 1] > 0 )
        {
          area += lib_inv_area;
        }
      }

      /* invert the phase */
      use_phase = use_phase ^ 1;

      /* if both phases are implemented and used */
      if ( !node_data.same_match && node_data.map_refs[use_phase] > 0 )
      {
        if constexpr ( !ELA )
        {
          auto const& best_cut = cuts.cuts( index )[node_data.best_cut[use_phase]];
          auto const& match = matches[index][best_cut->data.match_index];
          auto ctr = 0u;
          for ( auto const leaf : best_cut )
          {
            node_match[leaf].map_refs[2]++;
            if ( ( node_data.phase[use_phase] >> match.permutation[ctr++] ) & 1 )
              node_match[leaf].map_refs[1]++;
            else
              node_match[leaf].map_refs[0]++;
          }
        }
        area += node_data.area[use_phase];
      }
    }

    /* blend flow references */
    for ( auto i = 0u; i < ntk.size(); ++i )
    {
      node_match[i].est_refs[2] = coef * node_match[i].est_refs[2] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[2] ) );
      node_match[i].est_refs[1] = coef * node_match[i].est_refs[1] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[1] ) );
      node_match[i].est_refs[0] = coef * node_match[i].est_refs[0] + ( 1.0f - coef ) * std::max( 1.0f, static_cast<float>( node_match[i].map_refs[0] ) );
    }

    ++iteration;
    return true;
  }

  void compute_required_time()
  {
    for ( auto i = 0u; i < node_match.size(); ++i )
    {
      node_match[i].required[0] = node_match[i].required[1] = std::numeric_limits<float>::max();
    }

    /* return in case of `skip_delay_round` */
    if ( iteration == 0 )
      return;

    auto required = delay;

    if ( ps.required_time != 0.0f )
    {
      /* Global target time constraint */
      if ( ps.required_time < delay - epsilon )
      {
        if ( !ps.skip_delay_round && iteration == 1 )
          std::cerr << fmt::format( "[i] MAP WARNING: cannot meet the target required time of {:.2f}", ps.required_time ) << std::endl;
      }
      else
      {
        required = ps.required_time;
      }
    }

    /* set the required time at POs */
    ntk.foreach_co( [&]( auto const& s ) {
      const auto index = ntk.node_to_index( ntk.get_node( s ) );
      if ( ntk.is_complemented( s ) )
        node_match[index].required[1] = required;
      else
        node_match[index].required[0] = required;
    } );

    /* propagate required time to the PIs */
    auto i = ntk.size();
    while ( i-- > 0u )
    {
      const auto n = ntk.index_to_node( i );
      if ( ntk.is_ci( n ) || ntk.is_constant( n ) )
        break;

      if ( node_match[i].map_refs[2] == 0 )
        continue;

      auto& node_data = node_match[i];

      unsigned use_phase = node_data.best_supergate[0] == nullptr ? 1u : 0u;
      unsigned other_phase = use_phase ^ 1;

      assert( node_data.best_supergate[0] != nullptr || node_data.best_supergate[1] != nullptr );
      assert( node_data.map_refs[0] || node_data.map_refs[1] );

      /* propagate required time over output inverter if present */
      if ( node_data.same_match && node_data.map_refs[other_phase] > 0 )
      {
        node_data.required[use_phase] = std::min( node_data.required[use_phase], node_data.required[other_phase] - lib_inv_delay );
      }

      if ( node_data.same_match || node_data.map_refs[use_phase] > 0 )
      {
        auto ctr = 0u;
        auto best_cut = cuts.cuts( i )[node_data.best_cut[use_phase]];
        auto const& match = matches[i][best_cut->data.match_index];
        auto const& supergate = node_data.best_supergate[use_phase];
        for ( auto leaf : best_cut )
        {
          auto phase = ( node_data.phase[use_phase] >> match.permutation[ctr] ) & 1;
          node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[use_phase] - supergate->tdelay[match.permutation[ctr]] );
          ctr++;
        }
      }

      if ( !node_data.same_match && node_data.map_refs[other_phase] > 0 )
      {
        auto ctr = 0u;
        auto best_cut = cuts.cuts( i )[node_data.best_cut[other_phase]];
        auto const& match = matches[i][best_cut->data.match_index];
        auto const& supergate = node_data.best_supergate[other_phase];
        for ( auto leaf : best_cut )
        {
          auto phase = ( node_data.phase[other_phase] >> match.permutation[ctr] ) & 1;
          node_match[leaf].required[phase] = std::min( node_match[leaf].required[phase], node_data.required[other_phase] - supergate->tdelay[match.permutation[ctr]] );
          ctr++;
        }
      }
    }
  }

  template<bool DO_AREA>
  void match_phase( node<Ntk> const& n, uint8_t phase )
  {
    float best_arrival = std::numeric_limits<float>::max();
    float best_area_flow = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    exact_supergate<NtkDest, NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];
      auto& supergates = cut_matches[( cut )->data.match_index];

      /* permutate the children to the NPN-represenentative configuration */
      std::vector<uint32_t> children( NInputs, 0u );
      auto ctr = 0u;
      for ( auto l : cut )
      {
        children[supergates.permutation[ctr++]] = l;
      }

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area_flow = best_supergate->area + cut_leaves_flow( cut, n, phase );
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();
      for ( auto pin = 0u; pin < NInputs; pin++ )
      {
        float arrival_pin = node_match[children[pin]].arrival[( best_phase >> pin ) & 1] + best_supergate->tdelay[pin];
        best_arrival = std::max( best_arrival, arrival_pin );
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->data.ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index];

      if ( supergates.supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* permutate the children to the NPN-represenentative configuration */
      std::vector<uint32_t> children( NInputs, 0u );
      auto ctr = 0u;
      for ( auto l : *cut )
      {
        children[supergates.permutation[ctr++]] = l;
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates.supergates[phase] )
      {
        uint8_t complement = supergates.negation ^ gate.polarity;
        node_data.phase[phase] = complement;
        float area_local = gate.area + cut_leaves_flow( *cut, n, phase );
        float worst_arrival = 0.0f;
        for ( auto pin = 0u; pin < NInputs; pin++ )
        {
          float arrival_pin = node_match[children[pin]].arrival[( complement >> pin ) & 1] + gate.tdelay[pin];
          worst_arrival = std::max( worst_arrival, arrival_pin );
        }

        if constexpr ( DO_AREA )
        {
          if ( worst_arrival > node_data.required[phase] + epsilon )
            continue;
        }

        if ( compare_map<DO_AREA>( worst_arrival, best_arrival, area_local, best_area_flow, cut->size(), best_size ) )
        {
          best_arrival = worst_arrival;
          best_area_flow = area_local;
          best_size = cut->size();
          best_cut = cut_index;
          best_area = gate.area;
          best_phase = complement;
          best_supergate = &gate;
        }
      }

      ++cut_index;
    }

    node_data.flows[phase] = best_area_flow;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;
  }

  void match_phase_exact( node<Ntk> const& n, uint8_t phase )
  {
    float best_arrival = std::numeric_limits<float>::max();
    float best_exact_area = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    exact_supergate<NtkDest, NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* recompute best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];
      auto const& supergates = cut_matches[( cut )->data.match_index];

      /* permutate the children to the NPN-represenentative configuration */
      std::vector<uint32_t> children( NInputs, 0u );
      auto ctr = 0u;
      for ( auto l : cut )
      {
        children[supergates.permutation[ctr++]] = l;
      }

      best_phase = node_data.phase[phase];
      best_arrival = 0.0f;
      best_area = best_supergate->area;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();
      for ( auto pin = 0u; pin < NInputs; pin++ )
      {
        float arrival_pin = node_match[children[pin]].arrival[( best_phase >> pin ) & 1] + best_supergate->tdelay[pin];
        best_arrival = std::max( best_arrival, arrival_pin );
      }

      /* if cut is implemented, remove it from the cover */
      if ( !node_data.same_match && node_data.map_refs[phase] )
      {
        best_exact_area = cut_deref( cuts.cuts( index )[best_cut], n, phase );
      }
      else
      {
        best_exact_area = cut_ref( cuts.cuts( index )[best_cut], n, phase );
        cut_deref( cuts.cuts( index )[best_cut], n, phase );
      }
    }

    /* foreach cut */
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts or not matched cuts */
      if ( ( *cut )->data.ignore )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index];

      if ( supergates.supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      /* permutate the children to the NPN-represenentative configuration */
      std::vector<uint32_t> children( NInputs, 0u );
      auto ctr = 0u;
      for ( auto l : *cut )
      {
        children[supergates.permutation[ctr++]] = l;
      }

      for ( auto const& gate : *supergates.supergates[phase] )
      {
        uint8_t complement = supergates.negation ^ gate.polarity;
        node_data.phase[phase] = complement;
        node_data.area[phase] = gate.area;
        auto area_exact = cut_ref( *cut, n, phase );
        cut_deref( *cut, n, phase );
        float worst_arrival = 0.0f;
        for ( auto pin = 0u; pin < NInputs; pin++ )
        {
          float arrival_pin = node_match[children[pin]].arrival[( complement >> pin ) & 1] + gate.tdelay[pin];
          worst_arrival = std::max( worst_arrival, arrival_pin );
        }

        if ( worst_arrival > node_data.required[phase] + epsilon )
          continue;

        if ( compare_map<true>( worst_arrival, best_arrival, area_exact, best_exact_area, cut->size(), best_size ) )
        {
          best_arrival = worst_arrival;
          best_exact_area = area_exact;
          best_area = gate.area;
          best_size = cut->size();
          best_cut = cut_index;
          best_phase = complement;
          best_supergate = &gate;
        }
      }

      ++cut_index;
    }

    node_data.flows[phase] = best_exact_area;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;

    if ( !node_data.same_match && node_data.map_refs[phase] )
    {
      best_exact_area = cut_ref( cuts.cuts( index )[best_cut], n, phase );
    }
  }

  bool compute_exact_area_aggressive( NtkDest& res, node_map<signal<NtkDest>, Ntk>& old2new )
  {
    depth_view<NtkDest> res_d{ res };

    for ( auto const& n : top_order )
    {
      if ( ntk.is_constant( n ) || ntk.is_ci( n ) )
        continue;

      auto index = ntk.node_to_index( n );
      auto& node_data = node_match[index];

      /* recursively deselect the best cut shared between
       * the two phases if in use in the cover */
      if ( node_data.same_match && node_data.map_refs[2] != 0 )
      {
        if ( node_data.best_supergate[0] != nullptr )
          cut_deref( cuts.cuts( index )[node_data.best_cut[0]], n, 0u );
        else
          cut_deref( cuts.cuts( index )[node_data.best_cut[1]], n, 1u );
      }

      /* match positive phase */
      auto sig0 = match_phase_exact_aggressive( res_d, old2new, n, 0u );

      /* match negative phase */
      auto sig1 = match_phase_exact_aggressive( res_d, old2new, n, 1u );

      /* try to drop one phase */
      float worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
      float worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
      bool use_zero = false;
      bool use_one = false;
      if ( node_data.best_supergate[0] == nullptr )
      {
        set_match_complemented_phase( index, 1, worst_arrival_npos );
        if ( node_data.map_refs[2] )
        {
          cut_ref( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
          recursive_ref<NtkDest>( res, res.get_node( sig1 ) );
        }
        old2new[n] = sig1;
        continue;
      }
      else if ( node_data.best_supergate[1] == nullptr )
      {
        set_match_complemented_phase( index, 0, worst_arrival_nneg );
        if ( node_data.map_refs[2] )
        {
          cut_ref( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
          recursive_ref<NtkDest>( res, res.get_node( sig0 ) );
        }
        old2new[n] = sig0;
        continue;
      }
      use_zero = worst_arrival_nneg < node_data.required[1] + epsilon;
      use_one = worst_arrival_npos < node_data.required[0] + epsilon;

      if ( use_zero && use_one )
      {
        auto size_zero = cuts.cuts( index )[node_data.best_cut[0]].size();
        auto size_one = cuts.cuts( index )[node_data.best_cut[1]].size();
        if ( compare_map<true>( worst_arrival_nneg, worst_arrival_npos, node_data.flows[0], node_data.flows[1], size_zero, size_one ) )
          use_one = false;
        else
          use_zero = false;
      }

      if ( use_zero )
      {
        if ( node_data.map_refs[2] )
        {
          cut_ref( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
          recursive_ref<NtkDest>( res, res.get_node( sig0 ) );
        }
        set_match_complemented_phase( index, 0, worst_arrival_nneg );
        old2new[n] = sig0;
      }
      else
      {
        if ( node_data.map_refs[2] )
        {
          cut_ref( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
          recursive_ref<NtkDest>( res, res.get_node( sig1 ) );
        }
        set_match_complemented_phase( index, 1, worst_arrival_npos );
        old2new[n] = sig1;
      }
    }

    double area_old = area;
    bool success = set_mapping_refs<true>();

    /* round stats */
    if ( ps.verbose )
    {
      float area_gain = float( ( area_old - area ) / area_old * 100 );
      std::stringstream stats{};
      stats << fmt::format( "[i] Area RW  : Delay = {:>12.2f}  Area = {:>12.2f}  {:>5.2f} %\n", delay, area, area_gain );
      st.round_stats.push_back( stats.str() );
    }

    return success;
  }

  signal<NtkDest> match_phase_exact_aggressive( depth_view<NtkDest>& res, node_map<signal<NtkDest>, Ntk>& old2new, node<Ntk> const& n, uint8_t phase )
  {
    signal<NtkDest> best_signal = res.get_constant( false );

    float best_arrival = std::numeric_limits<float>::max();
    float best_exact_area = std::numeric_limits<float>::max();
    float best_area = std::numeric_limits<float>::max();
    uint32_t best_size = UINT32_MAX;
    uint8_t best_cut = 0u;
    uint8_t best_phase = 0u;
    uint8_t cut_index = 0u;
    auto index = ntk.node_to_index( n );

    auto& node_data = node_match[index];
    auto& cut_matches = matches[index];
    exact_supergate<NtkDest, NInputs> const* best_supergate = node_data.best_supergate[phase];

    /* create best match info */
    if ( best_supergate != nullptr )
    {
      auto const& cut = cuts.cuts( index )[node_data.best_cut[phase]];
      auto const& supergates = cut_matches[( cut )->data.match_index];

      /* permutate the children to the NPN-represenentative configuration */
      std::vector<signal<NtkDest>> children( NInputs, res.get_constant( false ) );
      auto ctr = 0u;

      for ( auto l : cut )
      {
        children[supergates.permutation[ctr++]] = old2new[ntk.index_to_node( l )];
      }

      best_phase = supergates.negation;
      best_cut = node_data.best_cut[phase];
      best_size = cut.size();
      for ( auto i = 0u; i < NInputs; ++i )
      {
        if ( ( best_phase >> i ) & 1 )
        {
          children[i] = !children[i];
        }
      }
      topo_view topo{ lib_database, best_supergate->root };
      auto f = cleanup_dangling( topo, res, children.begin(), children.end() ).front();

      if ( phase == 1 )
        f = !f;

      best_signal = f;

      best_arrival = res.level( res.get_node( f ) );

      /* if cut is implemented, remove it from the cover */
      if ( !node_data.same_match && node_data.map_refs[phase] )
      {
        best_area = recursive_ref<NtkDest>( res, res.get_node( f ) );
        recursive_deref<NtkDest>( res, res.get_node( f ) );
        best_exact_area = cut_deref( cuts.cuts( index )[best_cut], n, phase );
      }
      else
      {
        best_area = recursive_ref<NtkDest>( res, res.get_node( f ) );
        recursive_deref<NtkDest>( res, res.get_node( f ) );
        best_exact_area = cut_ref( cuts.cuts( index )[best_cut], n, phase );
        cut_deref( cuts.cuts( index )[best_cut], n, phase );
      }
    }

    /* foreach cut */
    unsigned int rewrite_count = 1u;
    for ( auto& cut : cuts.cuts( index ) )
    {
      /* trivial cuts, not matched cuts, or rewriting limit reached */
      if ( ( *cut )->data.ignore || ( rewrite_count > ps.logic_sharing_cut_limit && cut_index != best_cut ) )
      {
        ++cut_index;
        continue;
      }

      auto const& supergates = cut_matches[( *cut )->data.match_index];

      if ( supergates.supergates[phase] == nullptr )
      {
        ++cut_index;
        continue;
      }

      ++rewrite_count;

      std::vector<signal<NtkDest>> children( NInputs, res.get_constant( false ) );

      auto ctr = 0u;
      for ( auto l : *cut )
      {
        children[supergates.permutation[ctr++]] = old2new[ntk.index_to_node( l )];
      }

      /* match each gate and take the best one */
      for ( auto const& gate : *supergates.supergates[phase] )
      {
        uint8_t complement = supergates.negation;
        node_data.phase[phase] = complement;

        /* rewrite each structure and measure the logic sharing */
        std::vector<signal<NtkDest>> children_loc( NInputs );

        for ( auto ctr = 0u; ctr < NInputs; ++ctr )
        {
          children_loc[ctr] = children[ctr] ^ ( ( ( complement >> ctr ) & 1 ) == 1 );
        }
        topo_view topo{ lib_database, gate.root };
        auto f = cleanup_dangling( topo, res, children_loc.begin(), children_loc.end() ).front();

        if ( phase == 1 )
          f = !f;

        float worst_arrival = res.level( res.get_node( f ) );

        float area_hashed = recursive_ref<NtkDest>( res, res.get_node( f ) );
        node_data.area[phase] = area_hashed;
        recursive_deref<NtkDest>( res, res.get_node( f ) );
        auto area_exact = cut_ref( *cut, n, phase );
        cut_deref( *cut, n, phase );

        if ( worst_arrival > node_data.required[phase] + epsilon )
          continue;

        if ( compare_map<true>( worst_arrival, best_arrival, area_exact, best_exact_area, cut->size(), best_size ) )
        {
          best_arrival = worst_arrival;
          best_exact_area = area_exact;
          best_area = area_hashed;
          best_size = cut->size();
          best_cut = cut_index;
          best_phase = complement;
          best_supergate = &gate;
          best_signal = f;
        }
      }

      ++cut_index;
    }
    old2new[n] = best_signal;
    node_data.flows[phase] = best_exact_area;
    node_data.arrival[phase] = best_arrival;
    node_data.area[phase] = best_area;
    node_data.best_cut[phase] = best_cut;
    node_data.phase[phase] = best_phase;
    node_data.best_supergate[phase] = best_supergate;

    if ( !node_data.same_match && node_data.map_refs[phase] )
    {
      recursive_ref<NtkDest>( res, res.get_node( best_signal ) );
      best_exact_area = cut_ref( cuts.cuts( index )[best_cut], n, phase );
    }
    return best_signal;
  }

  template<bool DO_AREA, bool ELA>
  void match_drop_phase( node<Ntk> const& n, unsigned area_margin_factor )
  {
    auto index = ntk.node_to_index( n );
    auto& node_data = node_match[index];

    /* compute arrival adding an inverter to the other match phase */
    float worst_arrival_npos = node_data.arrival[1] + lib_inv_delay;
    float worst_arrival_nneg = node_data.arrival[0] + lib_inv_delay;
    bool use_zero = false;
    bool use_one = false;

    /* only one phase is matched */
    if ( node_data.best_supergate[0] == nullptr )
    {
      set_match_complemented_phase( index, 1, worst_arrival_npos );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[2] )
          cut_ref( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
      }
      return;
    }
    else if ( node_data.best_supergate[1] == nullptr )
    {
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
      if constexpr ( ELA )
      {
        if ( node_data.map_refs[2] )
          cut_ref( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
      }
      return;
    }

    /* try to use only one match to cover both phases */
    if constexpr ( !DO_AREA )
    {
      /* if arrival is less matching the other phase and inserting an inverter */
      if ( worst_arrival_npos < node_data.arrival[0] + epsilon )
      {
        use_one = true;
      }
      if ( worst_arrival_nneg < node_data.arrival[1] + epsilon )
      {
        use_zero = true;
      }
      if ( !use_zero && !use_one )
      {
        /* use both phases to improve delay */
        node_data.flows[2] = ( node_data.flows[0] + node_data.flows[1] ) / node_data.est_refs[2];
        node_data.flows[0] = node_data.flows[0] / node_data.est_refs[0];
        node_data.flows[1] = node_data.flows[1] / node_data.est_refs[1];
        return;
      }
    }
    else
    {
      /* check if both phases + inverter meet the required time */
      use_zero = worst_arrival_nneg < node_data.required[1] + epsilon - area_margin_factor * lib_inv_delay;
      use_one = worst_arrival_npos < node_data.required[0] + epsilon - area_margin_factor * lib_inv_delay;
    }

    /* use area flow as a tiebreaker. Unfortunately cannot keep
     * the both phases since `node_map` does not support that */
    if ( use_zero && use_one )
    {
      auto size_zero = cuts.cuts( index )[node_data.best_cut[0]].size();
      auto size_one = cuts.cuts( index )[node_data.best_cut[1]].size();
      if ( compare_map<DO_AREA>( worst_arrival_nneg, worst_arrival_npos, node_data.flows[0], node_data.flows[1], size_zero, size_one ) )
        use_one = false;
      else
        use_zero = false;
    }

    if ( use_zero )
    {
      if constexpr ( ELA )
      {
        if ( !node_data.same_match )
        {
          if ( node_data.map_refs[1] > 0 )
            cut_deref( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
          if ( node_data.map_refs[0] == 0 )
            cut_ref( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
        }
        else if ( node_data.map_refs[2] )
          cut_ref( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
      }
      set_match_complemented_phase( index, 0, worst_arrival_nneg );
    }
    else
    {
      if constexpr ( ELA )
      {
        if ( !node_data.same_match )
        {
          if ( node_data.map_refs[0] > 0 )
            cut_deref( cuts.cuts( index )[node_data.best_cut[0]], n, 0 );
          if ( node_data.map_refs[1] == 0 && node_data.map_refs[2] )
            cut_ref( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
        }
        else if ( node_data.map_refs[2] )
          cut_ref( cuts.cuts( index )[node_data.best_cut[1]], n, 1 );
      }
      set_match_complemented_phase( index, 1, worst_arrival_npos );
    }
  }

  inline void set_match_complemented_phase( uint32_t index, uint8_t phase, float worst_arrival_n )
  {
    auto& node_data = node_match[index];
    auto phase_n = phase ^ 1;
    node_data.same_match = true;
    node_data.best_supergate[phase_n] = nullptr;
    node_data.best_cut[phase_n] = node_data.best_cut[phase];
    node_data.phase[phase_n] = node_data.phase[phase] ^ ( 1 << NInputs );
    node_data.arrival[phase_n] = worst_arrival_n;
    node_data.area[phase_n] = node_data.area[phase];
    node_data.flows[phase] = node_data.flows[phase] / node_data.est_refs[2];
    node_data.flows[phase_n] = node_data.flows[phase];
    node_data.flows[2] = node_data.flows[phase];
  }

  inline float cut_leaves_flow( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    float flow{ 0.0f };
    auto const& node_data = node_match[ntk.node_to_index( n )];
    auto const& match = matches[ntk.node_to_index( n )][cut->data.match_index];

    uint8_t ctr = 0u;
    for ( auto leaf : cut )
    {
      uint8_t leaf_phase = ( node_data.phase[phase] >> match.permutation[ctr++] ) & 1;
      flow += node_match[leaf].flows[leaf_phase];
    }

    return flow;
  }

  float cut_ref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    auto const& match = matches[ntk.node_to_index( n )][cut->data.match_index];
    float count = node_data.area[phase];
    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> match.permutation[ctr] ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        ++ctr;
        continue;
      }
      else if ( ntk.is_ci( ntk.index_to_node( leaf ) ) )
      {
        /* reference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( node_match[leaf].map_refs[1]++ == 0u )
            count += lib_inv_area;
        }
        else
        {
          ++node_match[leaf].map_refs[0];
        }
        ++ctr;
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if not present yet and leaf node is implemented in the opposite phase */
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u && node_match[leaf].best_supergate[leaf_phase] == nullptr )
          count += lib_inv_area;
        /* Recursive referencing if leaf was not referenced */
        if ( node_match[leaf].map_refs[2]++ == 0u )
        {
          count += cut_ref( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        ++node_match[leaf].map_refs[2];
        if ( node_match[leaf].map_refs[leaf_phase]++ == 0u )
        {
          count += cut_ref( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      ++ctr;
    }
    return count;
  }

  float cut_deref( cut_t const& cut, node<Ntk> const& n, uint8_t phase )
  {
    auto const& node_data = node_match[ntk.node_to_index( n )];
    auto const& match = matches[ntk.node_to_index( n )][cut->data.match_index];
    float count = node_data.area[phase];
    uint8_t ctr = 0;
    for ( auto leaf : cut )
    {
      /* compute leaf phase using the current gate */
      uint8_t leaf_phase = ( node_data.phase[phase] >> match.permutation[ctr] ) & 1;

      if ( ntk.is_constant( ntk.index_to_node( leaf ) ) )
      {
        ++ctr;
        continue;
      }
      else if ( ntk.is_ci( ntk.index_to_node( leaf ) ) )
      {
        /* dereference PIs, add inverter cost for negative phase */
        if ( leaf_phase == 1u )
        {
          if ( --node_match[leaf].map_refs[1] == 0u )
            count += lib_inv_area;
        }
        else
        {
          --node_match[leaf].map_refs[0];
        }
        ++ctr;
        continue;
      }

      if ( node_match[leaf].same_match )
      {
        /* Add inverter area if it is used only by the current gate and leaf node is implemented in the opposite phase */
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u && node_match[leaf].best_supergate[leaf_phase] == nullptr )
          count += lib_inv_area;
        /* Recursive dereferencing */
        if ( --node_match[leaf].map_refs[2] == 0u )
        {
          count += cut_deref( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      else
      {
        --node_match[leaf].map_refs[2];
        if ( --node_match[leaf].map_refs[leaf_phase] == 0u )
        {
          count += cut_deref( cuts.cuts( leaf )[node_match[leaf].best_cut[leaf_phase]], ntk.index_to_node( leaf ), leaf_phase );
        }
      }
      ++ctr;
    }
    return count;
  }

  template<bool DO_AREA>
  inline bool compare_map( float arrival, float best_arrival, float area_flow, float best_area_flow, uint32_t size, uint32_t best_size )
  {
    if constexpr ( DO_AREA )
    {
      if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
      else if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
    }
    else
    {
      if ( arrival < best_arrival - epsilon )
      {
        return true;
      }
      else if ( arrival > best_arrival + epsilon )
      {
        return false;
      }
      else if ( area_flow < best_area_flow - epsilon )
      {
        return true;
      }
      else if ( area_flow > best_area_flow + epsilon )
      {
        return false;
      }
    }
    if ( size < best_size )
    {
      return true;
    }
    return false;
  }

private:
  Ntk& ntk;
  exact_library<NtkDest, NInputs> const& library;
  map_params const& ps;
  map_stats& st;

  uint32_t iteration{ 0 };       /* current mapping iteration */
  double delay{ 0.0f };          /* current delay of the mapping */
  double area{ 0.0f };           /* current area of the mapping */
  const float epsilon{ 0.005f }; /* epsilon */

  /* lib inverter info */
  float lib_inv_area;
  float lib_inv_delay;

  NtkDest const& lib_database;

  std::vector<node<Ntk>> top_order;
  std::vector<node_match_t<NtkDest, NInputs>> node_match;
  std::unordered_map<uint32_t, std::vector<cut_match_t<NtkDest, NInputs>>> matches;
  network_cuts_t cuts;
};

} /* namespace detail */

/*! \brief Exact mapping.
 *
 * This function implements a mapping algorithm using a database of structures.
 * It is controlled by a template argument `CutData` (defaulted to
 * `cut_enumeration_exact_map_cut`). The argument is similar to the `CutData` argument
 * in `cut_enumeration`, which can specialize the cost function to select priority
 * cuts and store additional data. The default argument gives priority firstly to
 * area flow, then delay, and lastly to the cut size.
 * The type passed as `CutData` must implement the following four fields:
 *
 * - `uint32_t delay`
 * - `float flow`
 * - `uint8_t match_index`
 * - `bool ignore`
 *
 * See `include/mockturtle/algorithms/cut_enumeration/cut_enumeration_exact_map_cut.hpp`
 * for one example of a CutData type that implements the cost function that is used in
 * the technology mapper.
 *
 * The function takes the size of the cuts in the template parameter `CutSize`.
 *
 * The function returns a mapped network representation generated using the exact
 * synthesis entries in the `exact_library`. This function supports also sequential networks.
 *
 * **Required network functions:**
 * - `size`
 * - `is_ci`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_po`
 * - `foreach_node`
 * - `fanout_size`
 *
 * \param ntk Network
 * \param library Exact library
 * \param ps Mapping params
 * \param pst Mapping statistics
 */
template<class Ntk, unsigned CutSize = 4u, typename CutData = cut_enumeration_exact_map_cut, class NtkDest, unsigned NInputs>
NtkDest map( Ntk& ntk, exact_library<NtkDest, NInputs> const& library, map_params const& ps = {}, map_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_ci_v<Ntk>, "Ntk does not implement the is_ci method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_pi_v<Ntk>, "Ntk does not implement the foreach_pi method" );
  static_assert( has_foreach_po_v<Ntk>, "Ntk does not implement the foreach_po method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_incr_value_v<NtkDest>, "Ntk does not implement the incr_value method" );
  static_assert( has_decr_value_v<NtkDest>, "Ntk does not implement the decr_value method" );
  static_assert( has_foreach_ri_v<Ntk> == has_create_ri_v<NtkDest>, "Ntk and NtkDest networks are not both sequential" );
  static_assert( has_foreach_ro_v<Ntk> == has_create_ro_v<NtkDest>, "Ntk and NtkDest networks are not both sequential" );

  map_stats st;
  detail::exact_map_impl<NtkDest, CutSize, CutData, Ntk, NInputs> p( ntk, library, ps, st );
  auto res = p.run();

  st.time_total = st.time_mapping + st.cut_enumeration_st.time_total;
  if ( ps.verbose )
  {
    st.report();
  }

  if ( pst )
  {
    *pst = st;
  }

  return res;
}

} /* namespace mockturtle */
