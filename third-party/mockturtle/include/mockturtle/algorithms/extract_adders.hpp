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
  \file extract_adders.hpp
  \brief Maps adders in the network

  \author Alessandro Tempia Calvino
*/

#include <algorithm>
#include <array>
#include <vector>

#include <fmt/format.h>
#include <kitty/dynamic_truth_table.hpp>
#include <kitty/static_truth_table.hpp>
#include <parallel_hashmap/phmap.h>

#include "../networks/block.hpp"
#include "../networks/storage.hpp"
#include "../utils/node_map.hpp"
#include "../utils/stopwatch.hpp"
#include "../views/choice_view.hpp"
#include "cut_enumeration.hpp"

namespace mockturtle
{

struct extract_adders_params
{
  extract_adders_params()
  {
    cut_enumeration_ps.cut_limit = 49;
    cut_enumeration_ps.minimize_truth_table = false;
  }

  /*! \brief Parameters for cut enumeration
   *
   * The default cut limit is 49. By default,
   * truth table minimization is performed.
   */
  cut_enumeration_params cut_enumeration_ps{};

  /*! \brief Map inverted (NAND2-XNOR2, MIN3-XNOR3) */
  bool map_inverted{ false };

  /*! \brief Filter HAs/FAs using MFFC inclusion */
  bool use_mffc_filter{ true };

  /*! \brief Be verbose */
  bool verbose{ false };
};

struct extract_adders_stats
{
  /*! \brief Computed cuts. */
  uint32_t cuts_total{ 0 };

  /*! \brief Gates count. */
  uint32_t and2{ 0 };
  uint32_t maj3{ 0 };
  uint32_t xor2{ 0 };
  uint32_t xor3{ 0 };

  /*! \brief Hashed classes. */
  uint32_t num_classes{ 0 };

  /*! \brief Hash size. */
  uint32_t mapped_ha{ 0 };
  uint32_t mapped_fa{ 0 };

  /*! \brief Total runtime. */
  stopwatch<>::duration time_total{ 0 };

  void report() const
  {
    std::cout << fmt::format( "[i] Cuts = {}\t And2 = {}\t Xor2 = {}\t Maj3 = {}\t Xor3 = {}\n",
                              cuts_total, and2, xor2, maj3, xor3 );
    std::cout << fmt::format( "[i] Classes = {} \tMapped HA = {}\t Mapped FA:{}\n", num_classes, mapped_ha, mapped_fa );
    std::cout << fmt::format( "[i] Total runtime = {:>5.2f} secs\n", to_seconds( time_total ) );
  }
};

namespace detail
{

struct triple_hash
{
  uint64_t operator()( const std::array<uint32_t, 3>& p ) const
  {
    uint64_t seed = hash_block( p[0] );

    hash_combine( seed, hash_block( p[1] ) );
    hash_combine( seed, hash_block( p[2] ) );

    return seed;
  }
};

struct cut_enumeration_fa_cut
{
  /* stats */
  bool is_xor{ false };
};

template<class Ntk>
class extract_adders_impl
{
public:
  using network_cuts_t = fast_network_cuts<Ntk, 3, true, cut_enumeration_fa_cut>;
  using cut_t = typename network_cuts_t::cut_t;
  using leaves_hash_t = phmap::flat_hash_map<std::array<uint32_t, 3>, std::vector<uint64_t>, triple_hash>;
  using match_pair_t = std::pair<uint64_t, uint64_t>;
  using matches_t = std::vector<match_pair_t>;
  using block_map = node_map<signal<block_network>, Ntk>;

public:
  explicit extract_adders_impl( Ntk& ntk, extract_adders_params const& ps, extract_adders_stats& st )
      : ntk( ntk ),
        ps( ps ),
        st( st ),
        cuts( fast_cut_enumeration<Ntk, 3, true, cut_enumeration_fa_cut>( ntk, ps.cut_enumeration_ps ) ),
        cuts_classes(),
        half_adders(),
        full_adders(),
        node_match( ntk.size(), UINT32_MAX )
  {
    cuts_classes.reserve( 2000 );
    tmp_visited.reserve( 20 );
  }

  block_network run()
  {
    stopwatch t( st.time_total );

    auto [res, old2new] = initialize_map_network();
    create_classes();
    match_adders();
    map();
    topo_sort();
    finalize( res, old2new );

    return res;
  }

private:
  void create_classes()
  {
    uint32_t counter = 0;
    std::array<uint32_t, 3> leaves = { 0, 0, 0 };

    st.cuts_total = cuts.total_cuts();

    ntk.foreach_gate( [&]( auto const& n ) {
      uint32_t cut_index = 0;
      for ( auto& cut : cuts.cuts( ntk.node_to_index( n ) ) )
      {
        kitty::static_truth_table<3> tt = cuts.truth_table( *cut );

        bool to_add = false;
        if ( cut->size() == 2 )
        {
          /* check for and2 */
          for ( uint32_t func : and2func )
          {
            if ( tt._bits == func )
            {
              ++st.and2;
              to_add = true;
              break;
            }
          }

          /* check for xor2 */
          for ( uint32_t func : xor2func )
          {
            if ( tt._bits == func )
            {
              ++st.xor2;
              ( *cut )->data.is_xor = true;
              to_add = true;
              break;
            }
          }
        }
        else if ( cut->size() == 3 )
        {
          /* check for maj3 */
          for ( uint32_t func : maj3func )
          {
            if ( tt._bits == func )
            {
              ++st.maj3;
              to_add = true;
              break;
            }
          }

          /* check xor3 */
          for ( uint32_t func : xor3func )
          {
            if ( tt._bits == func )
            {
              ++st.xor3;
              ( *cut )->data.is_xor = true;
              to_add = true;
              break;
            }
          }
        }

        if ( !to_add )
        {
          ++cut_index;
          continue;
        }

        uint64_t data = ( static_cast<uint64_t>( ntk.node_to_index( n ) ) << 16 ) | cut_index;
        leaves[2] = 0;
        uint32_t i = 0;
        for ( auto l : *cut )
          leaves[i++] = l;

        /* add to hash table */
        auto& v = cuts_classes[leaves];
        v.push_back( data );

        ++cut_index;
      }
    } );

    st.num_classes = cuts_classes.size();
  }

  void match_adder2( std::pair<std::array<uint32_t, 3>, std::vector<uint64_t>> const& it )
  {
    for ( uint32_t i = 0; i < it.second.size() - 1; ++i )
    {
      uint64_t data_i = it.second[i];
      uint32_t index_i = data_i >> 16;
      uint32_t cut_index_i = data_i & UINT16_MAX;
      auto const& cut_i = cuts.cuts( index_i )[cut_index_i];

      /* TODO: find unique matches */
      for ( uint32_t j = i + 1; j < it.second.size(); ++j )
      {
        uint64_t data_j = it.second[j];
        uint32_t index_j = data_j >> 16;
        uint32_t cut_index_j = data_j & UINT16_MAX;
        auto const& cut_j = cuts.cuts( index_j )[cut_index_j];

        /* not compatible */
        if ( cut_i->data.is_xor == cut_j->data.is_xor )
          continue;

        /* check compatibility */
        if ( !check_adder( index_i, index_j, cut_i ) )
          continue;

        assert( cut_i.size() == 2 );
        assert( cut_j.size() == 2 );

        half_adders.push_back( { data_i, data_j } );
      }
    }
  }

  void match_adders()
  {
    half_adders.reserve( cuts_classes.size() );
    full_adders.reserve( cuts_classes.size() );
    ntk.clear_values();

    for ( auto& it : cuts_classes )
    {
      /* not matched */
      if ( it.second.size() < 2 )
        continue;

      /* half adder */
      if ( it.first[2] == 0 )
      {
        match_adder2( it );
        continue;
      }

      for ( uint32_t i = 0; i < it.second.size() - 1; ++i )
      {
        uint64_t data_i = it.second[i];
        uint32_t index_i = data_i >> 16;
        uint32_t cut_index_i = data_i & UINT16_MAX;
        auto const& cut_i = cuts.cuts( index_i )[cut_index_i];

        /* TODO: find unique matches */
        for ( uint32_t j = i + 1; j < it.second.size(); ++j )
        {
          uint64_t data_j = it.second[j];
          uint32_t index_j = data_j >> 16;
          uint32_t cut_index_j = data_j & UINT16_MAX;
          auto const& cut_j = cuts.cuts( index_j )[cut_index_j];

          /* not compatible */
          if ( cut_i->data.is_xor == cut_j->data.is_xor )
            continue;

          /* check compatibility */
          if ( !check_adder( index_i, index_j, cut_i ) )
            continue;

          assert( cut_i.size() == 3 );
          assert( cut_j.size() == 3 );

          full_adders.push_back( { data_i, data_j } );
        }
      }
    }
  }

  void map()
  {
    selected.reserve( full_adders.size() + half_adders.size() );

    ntk.incr_trav_id();

    for ( uint32_t i = 0; i < full_adders.size(); ++i )
    {
      auto& pair = full_adders[i];
      uint32_t index1 = pair.first >> 16;
      uint32_t index2 = pair.second >> 16;
      uint32_t cut_index1 = pair.first & UINT16_MAX;
      cut_t const& cut = cuts.cuts( index1 )[cut_index1];

      /* remove overlapping multi-output gates */
      if ( !gate_mark( index1, index2, cut ) )
        continue;

      selected.push_back( 2 * i );
      node_match[std::max( index1, index2 )] = 2 * i;
      node_match[std::min( index1, index2 )] = UINT32_MAX - 1;

      ++st.mapped_fa;
    }

    for ( uint32_t i = 0; i < half_adders.size(); ++i )
    {
      auto& pair = half_adders[i];
      uint32_t index1 = pair.first >> 16;
      uint32_t index2 = pair.second >> 16;
      uint32_t cut_index1 = pair.first & UINT16_MAX;
      cut_t const& cut = cuts.cuts( index1 )[cut_index1];

      if ( !gate_mark( index1, index2, cut ) )
        continue;

      selected.push_back( 2 * i + 1 );
      node_match[std::max( index1, index2 )] = 2 * i + 1;
      node_match[std::min( index1, index2 )] = UINT32_MAX - 1;

      ++st.mapped_ha;
    }
  }

  void topo_sort()
  {
    topo_order.reserve( ntk.size() );

    /* add map choices */
    choice_view<Ntk> choice_ntk{ ntk };
    add_choices( choice_ntk );

    ntk.incr_trav_id();
    ntk.incr_trav_id();

    /* add constants and CIs */
    const auto c0 = ntk.get_node( ntk.get_constant( false ) );
    ntk.set_visited( c0, ntk.trav_id() );

    if ( const auto c1 = ntk.get_node( ntk.get_constant( true ) ); ntk.visited( c1 ) != ntk.trav_id() )
    {
      ntk.set_visited( c1, ntk.trav_id() );
    }

    ntk.foreach_ci( [&]( auto const& n ) {
      if ( ntk.visited( n ) != ntk.trav_id() )
      {
        ntk.set_visited( n, ntk.trav_id() );
      }
    } );

    /* sort topologically */
    ntk.foreach_co( [&]( auto const& f ) {
      if ( ntk.visited( ntk.get_node( f ) ) == ntk.trav_id() )
        return;
      topo_sort_rec( choice_ntk, ntk.get_node( f ) );
    } );
  }

  void add_choices( choice_view<Ntk>& choice_ntk )
  {
    for ( uint32_t index : selected )
    {
      auto& pair = ( index & 1 ) ? half_adders[index >> 1] : full_adders[index >> 1];
      uint32_t index1 = pair.first >> 16;
      uint32_t index2 = pair.second >> 16;

      if ( index1 > index2 )
        std::swap( index1, index2 );

      choice_ntk.add_choice( ntk.index_to_node( index1 ), ntk.index_to_node( index2 ) );

      assert( choice_ntk.count_choices( ntk.index_to_node( index1 ) ) == 2 );
    }
  }

  inline bool check_adder( uint32_t index1, uint32_t index2, cut_t const& cut )
  {
    bool valid = true;

    /* check containment of cut1 in cut2 and vice versa */
    if ( index1 > index2 )
    {
      std::swap( index1, index2 );
    }

    ntk.foreach_fanin( ntk.index_to_node( index2 ), [&]( auto const& f ) {
      auto g = ntk.get_node( f );
      if ( ntk.node_to_index( g ) == index1 && ntk.fanout_size( g ) == 1 )
      {
        valid = false;
      }
      return valid;
    } );

    if ( !valid )
      return false;

    /* check containment when node is reachable from middle nodes with multiple fanouts */
    return check_adder_tfi_valid( ntk.index_to_node( index2 ), ntk.index_to_node( index1 ), cut );
  }

  inline bool gate_mark( uint32_t index1, uint32_t index2, cut_t const& cut )
  {
    bool contained = false;

    /* mark leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    contained = mark_visited_rec<false>( ntk.index_to_node( index1 ) );
    contained |= mark_visited_rec<false>( ntk.index_to_node( index2 ) );

    if ( contained )
    {
      /* unmark leaves */
      for ( auto leaf : cut )
      {
        ntk.decr_value( ntk.index_to_node( leaf ) );
      }
      return false;
    }

    /* mark*/
    mark_visited_rec<true>( ntk.index_to_node( index1 ) );
    mark_visited_rec<true>( ntk.index_to_node( index2 ) );

    /* unmark leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }

    return true;
  }

  template<bool MARK>
  bool mark_visited_rec( node<Ntk> const& n )
  {
    /* leaf */
    if ( ntk.value( n ) )
      return false;

    /* already visited */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return true;

    if constexpr ( MARK )
    {
      ntk.set_visited( n, ntk.trav_id() );
    }

    bool contained = false;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      contained |= mark_visited_rec<MARK>( ntk.get_node( f ) );

      if constexpr ( !MARK )
      {
        if ( contained )
          return false;
      }

      return true;
    } );

    return contained;
  }

  inline bool check_adder_tfi_valid( node<Ntk> const& root, node<Ntk> const& n, cut_t const& cut )
  {
    /* reference cut leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    bool valid = true;
    if ( ps.use_mffc_filter )
    {
      tmp_visited.clear();
      dereference_node_rec( root );

      if ( ntk.fanout_size( n ) == 0 )
        valid = false;

      for ( auto g : tmp_visited )
        ntk.incr_fanout_size( g );
    }
    else
    {
      ntk.incr_trav_id();
      check_adder_tfi_valid_rec( root, root, n, valid );
    }

    /* dereference leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }

    return valid;
  }

  bool check_adder_tfi_valid_rec( node<Ntk> const& n, node<Ntk> const& root, node<Ntk> const& target, bool& valid )
  {
    /* leaf */
    if ( ntk.value( n ) )
      return false;

    /* already visited */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return false;

    ntk.set_visited( n, ntk.trav_id() );

    if ( n == target )
      return true;

    bool found = false;
    ntk.foreach_fanin( n, [&]( auto const& f ) {
      found |= check_adder_tfi_valid_rec( ntk.get_node( f ), root, target, valid );
      return valid;
    } );

    if ( found && n != root && ntk.fanout_size( n ) > 1 )
      valid = false;

    return found;
  }

  void dereference_node_rec( node<Ntk> const& n )
  {
    /* leaf */
    if ( ntk.value( n ) )
      return;

    ntk.foreach_fanin( n, [&]( auto const& f ) {
      node<Ntk> g = ntk.get_node( f );
      if ( ntk.decr_fanout_size( g ) == 0 )
      {
        dereference_node_rec( g );
      }
      tmp_visited.push_back( g );
    } );
  }

  inline bool is_in_tfi( node<Ntk> const& root, node<Ntk> const& n, cut_t const& cut )
  {
    /* reference cut leaves */
    for ( auto leaf : cut )
    {
      ntk.incr_value( ntk.index_to_node( leaf ) );
    }

    ntk.incr_trav_id();
    mark_visited_rec<true>( root );
    bool contained = ntk.visited( n ) == ntk.trav_id();

    /* dereference leaves */
    for ( auto leaf : cut )
    {
      ntk.decr_value( ntk.index_to_node( leaf ) );
    }

    return contained;
  }

  void topo_sort_rec( choice_view<Ntk>& choice_ntk, node<Ntk> const& n )
  {
    /* is permanently marked? */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;

    /* get the representative (smallest index) */
    node<Ntk> repr = choice_ntk.get_choice_representative( n );

    /* multioutput gate */
    if ( choice_ntk.count_choices( repr ) > 1 )
    {
      /* get the cut */
      uint32_t max_index = 0;
      choice_ntk.foreach_choice( repr, [&]( auto const& g ) {
        /* ensure that the node is not visited or temporarily marked */
        assert( ntk.visited( g ) != ntk.trav_id() );
        assert( ntk.visited( g ) != ntk.trav_id() - 1 );

        /* mark node temporarily */
        ntk.set_visited( g, ntk.trav_id() - 1 );

        max_index = std::max( max_index, ntk.node_to_index( g ) );
        return true;
      } );

      uint32_t cindex = node_match[max_index];
      auto& pair = ( cindex & 1 ) ? half_adders[cindex >> 1] : full_adders[cindex >> 1];
      cut_t const& cut = cuts.cuts( pair.first >> 16 )[pair.first & UINT16_MAX];

      for ( auto l : cut )
      {
        topo_sort_rec( choice_ntk, ntk.index_to_node( l ) );
      }

      choice_ntk.foreach_choice( repr, [&]( auto const& g ) {
        /* ensure that the node is not visited */
        assert( ntk.visited( g ) != ntk.trav_id() );

        /* mark node n permanently */
        ntk.set_visited( g, ntk.trav_id() );

        /* visit node */
        topo_order.push_back( g );

        return true;
      } );
    }
    else
    {
      /* ensure that the node is not visited or temporarily marked */
      assert( ntk.visited( n ) != ntk.trav_id() );
      assert( ntk.visited( n ) != ntk.trav_id() - 1 );

      /* mark node temporarily */
      ntk.set_visited( n, ntk.trav_id() - 1 );

      /* mark cut leaves */
      ntk.foreach_fanin( n, [&]( auto const& f ) {
        topo_sort_rec( choice_ntk, ntk.get_node( f ) );
      } );

      /* ensure that the node is not visited */
      assert( ntk.visited( n ) != ntk.trav_id() );

      /* mark node n permanently */
      ntk.set_visited( n, ntk.trav_id() );

      /* visit node */
      topo_order.push_back( n );
    }
  }

  std::pair<block_network, block_map> initialize_map_network()
  {
    block_network dest;
    block_map old2new( ntk );

    old2new[ntk.get_node( ntk.get_constant( false ) )] = dest.get_constant( false );
    if ( ntk.get_node( ntk.get_constant( true ) ) != ntk.get_node( ntk.get_constant( false ) ) )
      old2new[ntk.get_node( ntk.get_constant( true ) )] = dest.get_constant( true );

    ntk.foreach_ci( [&]( auto const& n ) {
      old2new[n] = dest.create_pi();
    } );
    return { dest, old2new };
  }

  void finalize( block_network& res, block_map& old2new )
  {
    for ( auto const& n : topo_order )
    {
      if ( ntk.is_pi( n ) || ntk.is_constant( n ) )
        continue;

      /* is a multioutput gate root? */
      if ( node_match[ntk.node_to_index( n )] == UINT32_MAX )
      {
        finalize_simple_gate( res, old2new, n );
      }
      else if ( node_match[ntk.node_to_index( n )] < UINT32_MAX - 1 )
      {
        finalize_multi_gate( res, old2new, n );
      }
    }

    /* create POs */
    ntk.foreach_co( [&]( auto const& f ) {
      res.create_po( ntk.is_complemented( f ) ? !old2new[f] : old2new[f] );
    } );
  }

  inline void finalize_simple_gate( block_network& res, block_map& old2new, node<Ntk> const& n )
  {
    kitty::dynamic_truth_table tt = ntk.node_function( n );

    std::vector<signal<block_network>> children;
    ntk.foreach_fanin( n, [&]( auto const& f, auto i ) {
      auto s = old2new[f] ^ ntk.is_complemented( f );
      children.push_back( s );
    } );

    old2new[n] = res.create_node( children, tt );
  }

  inline void finalize_multi_gate( block_network& res, block_map& old2new, node<Ntk> const& n )
  {
    uint32_t index = node_match[ntk.node_to_index( n )];
    assert( index < UINT32_MAX - 1 );

    /* extract the match */
    if ( index & 1 )
      finalize_multi_gate_ha( res, old2new, n, index >> 1 );
    else
      finalize_multi_gate_fa( res, old2new, n, index >> 1 );
  }

  inline void finalize_multi_gate_ha( block_network& res, block_map& old2new, node<Ntk> const& n, uint32_t index )
  {
    auto& pair = half_adders[index];
    uint32_t index1 = pair.first >> 16;
    uint32_t index2 = pair.second >> 16;
    uint32_t cut_index1 = pair.first & UINT16_MAX;
    uint32_t cut_index2 = pair.second & UINT16_MAX;
    cut_t const& cut1 = cuts.cuts( index1 )[cut_index1];
    cut_t const& cut2 = cuts.cuts( index2 )[cut_index2];

    kitty::static_truth_table<3> tt1 = cuts.truth_table( cut1 );
    kitty::static_truth_table<3> tt2 = cuts.truth_table( cut2 );
    bool xor_is_1 = false;

    /* find the XOR2 */
    xor_is_1 = cut1->data.is_xor;

    /* find the negation vector of AND2 and XOR2*/
    kitty::static_truth_table<3> tt = xor_is_1 ? tt2 : tt1;
    uint32_t neg_and = 0;
    for ( uint32_t func : and2func )
    {
      if ( tt._bits == func )
        break;
      ++neg_and;
    }

    tt = xor_is_1 ? tt1 : tt2;
    uint32_t neg_xor = 0;
    for ( uint32_t func : xor2func )
    {
      if ( tt._bits == func )
        break;
      ++neg_xor;
    }
    neg_xor ^= neg_and;
    neg_xor = ( neg_xor & 1 ) ^ ( ( neg_xor >> 1 ) & 1 ) ^ ( ( neg_xor >> 2 ) & 1 );

    /* normalize and create multioutput gate */
    std::array<signal<block_network>, 2> children;
    uint32_t ctr = 0;
    for ( auto l : cut1 )
    {
      signal<block_network> f = old2new[ntk.index_to_node( l )];
      bool phase = ( ( neg_and >> ctr ) & 1 ) ? true : false;
      children[ctr] = f ^ phase;
      ++ctr;
    }

    if ( ps.map_inverted )
    {
      signal<block_network> ha = res.create_hai( children[0], children[1] );
      old2new[ntk.index_to_node( xor_is_1 ? index2 : index1 )] = ha ^ ( ( neg_and >> 2 ) ? false : true );
      old2new[ntk.index_to_node( xor_is_1 ? index1 : index2 )] = res.next_output_pin( ha ) ^ ( neg_xor ? false : true );
      return;
    }

    signal<block_network> ha = res.create_ha( children[0], children[1] );
    old2new[ntk.index_to_node( xor_is_1 ? index2 : index1 )] = ha ^ ( ( neg_and >> 2 ) ? true : false );
    old2new[ntk.index_to_node( xor_is_1 ? index1 : index2 )] = res.next_output_pin( ha ) ^ ( neg_xor ? true : false );
  }

  inline void finalize_multi_gate_fa( block_network& res, block_map& old2new, node<Ntk> const& n, uint32_t index )
  {
    auto& pair = full_adders[index];
    uint32_t index1 = pair.first >> 16;
    uint32_t index2 = pair.second >> 16;
    uint32_t cut_index1 = pair.first & UINT16_MAX;
    uint32_t cut_index2 = pair.second & UINT16_MAX;
    cut_t const& cut1 = cuts.cuts( index1 )[cut_index1];
    cut_t const& cut2 = cuts.cuts( index2 )[cut_index2];

    kitty::static_truth_table<3> tt1 = cuts.truth_table( cut1 );
    kitty::static_truth_table<3> tt2 = cuts.truth_table( cut2 );

    bool xor_is_1 = false;

    /* find the XOR3 */
    xor_is_1 = cut1->data.is_xor;

    /* find the phase and permutation of MAJ3 and XOR3*/
    kitty::static_truth_table<3> tt = xor_is_1 ? tt2 : tt1;
    uint32_t neg_maj = 0;
    for ( uint32_t func : maj3func )
    {
      if ( tt._bits == func )
        break;
      ++neg_maj;
    }

    if ( ps.map_inverted )
    {
      neg_maj = ( ~neg_maj ) & 0x7;
    }

    tt = xor_is_1 ? tt1 : tt2;
    uint32_t neg_xor = 0;
    for ( uint32_t func : xor3func )
    {
      if ( tt._bits == func )
        break;
      ++neg_xor;
    }
    neg_xor ^= neg_maj;
    neg_xor = ( neg_xor & 1 ) ^ ( ( neg_xor >> 1 ) & 1 ) ^ ( ( neg_xor >> 2 ) & 1 );

    /* normalize and create the multioutput gate */
    std::array<signal<block_network>, 3> children;
    uint32_t ctr = 0;
    for ( auto l : cut1 )
    {
      signal<block_network> f = old2new[ntk.index_to_node( l )];
      bool phase = ( ( neg_maj >> ctr ) & 1 ) ? true : false;
      children[ctr] = f ^ phase;
      ++ctr;
    }

    if ( ps.map_inverted )
    {
      signal<block_network> fa = res.create_fai( children[0], children[1], children[2] );
      old2new[ntk.index_to_node( xor_is_1 ? index2 : index1 )] = fa;
      old2new[ntk.index_to_node( xor_is_1 ? index1 : index2 )] = res.next_output_pin( fa ) ^ ( neg_xor ? false : true );
      return;
    }

    signal<block_network> fa = res.create_fa( children[0], children[1], children[2] );
    old2new[ntk.index_to_node( xor_is_1 ? index2 : index1 )] = fa;
    old2new[ntk.index_to_node( xor_is_1 ? index1 : index2 )] = res.next_output_pin( fa ) ^ ( neg_xor ? true : false );
  }

private:
  Ntk& ntk;
  extract_adders_params const& ps;
  extract_adders_stats& st;

  network_cuts_t cuts;
  leaves_hash_t cuts_classes;
  matches_t half_adders;
  matches_t full_adders;
  std::vector<uint32_t> selected;
  std::vector<uint32_t> node_match;

  std::vector<node<Ntk>> topo_order;
  std::vector<node<Ntk>> tmp_visited;

  const std::array<uint64_t, 8> and2func = { 0x88, 0x44, 0x22, 0x11, 0x77, 0xbb, 0xdd, 0xee };
  const std::array<uint64_t, 8> maj3func = { 0xe8, 0xd4, 0xb2, 0x71, 0x8e, 0x4d, 0x2b, 0x17 };
  const std::array<uint64_t, 2> xor2func = { 0x66, 0x99 };
  const std::array<uint64_t, 2> xor3func = { 0x96, 0x69 };
};

} /* namespace detail */

/*! \brief Adders extraction.
 *
 * This function extracts half and full adders from a network.
 * It returns a `block_network` with extracted half and full adder
 * blocks.
 *
 * **Required network functions:**
 * - `size`
 * - `is_pi`
 * - `is_constant`
 * - `node_to_index`
 * - `index_to_node`
 * - `get_node`
 * - `foreach_co`
 * - `foreach_node`
 * - `foreach_gate`
 *
 * \param ntk Network
 * \param ps Parameters
 * \param pst Stats
 *
 */
template<class Ntk>
block_network extract_adders( Ntk& ntk, extract_adders_params const& ps = {}, extract_adders_stats* pst = {} )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the size method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_is_constant_v<Ntk>, "Ntk does not implement the is_constant method" );
  static_assert( has_node_to_index_v<Ntk>, "Ntk does not implement the node_to_index method" );
  static_assert( has_index_to_node_v<Ntk>, "Ntk does not implement the index_to_node method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_foreach_co_v<Ntk>, "Ntk does not implement the foreach_co method" );

  extract_adders_stats st;

  detail::extract_adders_impl p( ntk, ps, st );
  block_network res = p.run();

  if ( ps.verbose )
    st.report();

  if ( pst )
    *pst = st;

  return res;
}

} /* namespace mockturtle */