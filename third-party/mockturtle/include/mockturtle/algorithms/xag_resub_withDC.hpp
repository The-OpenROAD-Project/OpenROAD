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
  \file xag_resub_withDC.hpp
  \brief Resubstitution with free xor (works for XAGs, XOR gates are considered for free)

  \author Eleonora Testa
  \author Heinz Riener
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/xag.hpp"
#include "dont_cares.hpp"
#include "resubstitution.hpp"
#include <kitty/operations.hpp>

namespace mockturtle
{

struct xag_resub_stats
{
  /*! \brief Accumulated runtime for const-resub */
  stopwatch<>::duration time_resubC{ 0 };

  /*! \brief Accumulated runtime for zero-resub */
  stopwatch<>::duration time_resub0{ 0 };

  /*! \brief Accumulated runtime for one-resub */
  stopwatch<>::duration time_resub1{ 0 };

  /*! \brief Accumulated runtime for two-resub. */
  stopwatch<>::duration time_resub2{ 0 };

  /*! \brief Accumulated runtime for three-resub. */
  stopwatch<>::duration time_resub3{ 0 };

  /*! \brief Accumulated runtime for one-resub */
  stopwatch<>::duration time_resub1_and{ 0 };

  /*! \brief Accumulated runtime for one-resub */
  stopwatch<>::duration time_resub2_and{ 0 };

  /*! \brief Accumulated runtime for collecting unate divisors. */
  stopwatch<>::duration time_collect_unate_divisors{ 0 };

  /*! \brief Accumulated runtime for collecting unate divisors. */
  stopwatch<>::duration time_collect_binate_divisors{ 0 };

  /*! \brief Accumulated runtime for 12-resub. */
  stopwatch<>::duration time_resub12{ 0 };

  /*! \brief Number of accepted constant resubsitutions */
  uint32_t num_const_accepts{ 0 };

  /*! \brief Number of accepted zero resubsitutions */
  uint32_t num_div0_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions */
  uint64_t num_div1_accepts{ 0 };

  /*! \brief Number of accepted two resubsitutions */
  uint64_t num_div2_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions for AND */
  uint64_t num_div1_and_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions for AND */
  uint64_t num_div2_and_accepts{ 0 };

  /*! \brief Number of accepted two resubsitutions using triples of unate divisors */
  uint64_t num_div12_accepts{ 0 };

  void report() const
  {
    std::cout << "[i] kernel: xag_resub_functor\n";
    std::cout << fmt::format( "[i]     constant-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_const_accepts, to_seconds( time_resubC ) );
    std::cout << fmt::format( "[i]            0-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div0_accepts, to_seconds( time_resub0 ) );
    std::cout << fmt::format( "[i]            1-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div1_accepts, to_seconds( time_resub1 ) );
    std::cout << fmt::format( "[i]            2-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div2_accepts, to_seconds( time_resub2 ) );
    std::cout << fmt::format( "[i]            1-resub AND {:6d}                                   ({:>5.2f} secs)\n",
                              num_div1_and_accepts, to_seconds( time_resub1_and ) );
    std::cout << fmt::format( "[i]           12-resub {:6d}                                      ({:>5.2f} secs)\n",
                              num_div12_accepts, to_seconds( time_resub12 ) );
    std::cout << fmt::format( "[i]            2-resub AND {:6d}                                   ({:>5.2f} secs)\n",
                              num_div2_and_accepts, to_seconds( time_resub2_and ) );
    std::cout << fmt::format( "[i]            collect unate divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_unate_divisors ) );
    std::cout << fmt::format( "[i]            collect binate divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_binate_divisors ) );
    std::cout << fmt::format( "[i]            total   {:6d}\n",
                              ( num_const_accepts + num_div0_accepts + num_div1_accepts + num_div2_accepts + num_div1_and_accepts + num_div12_accepts + num_div2_and_accepts ) );
  }
}; /* xag_resub_stats */

namespace detail
{

template<typename Ntk>
class node_mffc_inside_xag
{
public:
  using node = typename Ntk::node;

public:
  explicit node_mffc_inside_xag( Ntk const& ntk )
      : ntk( ntk )
  {
  }

  std::pair<int32_t, int32_t> run( node const& n, std::vector<node> const& leaves, std::vector<node>& inside )
  {
    /* increment the fanout counters for the leaves */
    for ( const auto& l : leaves )
      ntk.incr_fanout_size( l );

    /* dereference the node */
    auto count1 = node_deref_rec( n );

    /* collect the nodes inside the MFFC */
    node_mffc_cone( n, inside );

    /* reference it back */
    auto count2 = node_ref_rec( n );
    (void)count2;

    assert( count1.first == count2.first );
    assert( count1.second == count2.second );

    for ( const auto& l : leaves )
      ntk.decr_fanout_size( l );

    return count1;
  }

private:
  /* ! \brief Dereference the node's MFFC */
  std::pair<int32_t, int32_t> node_deref_rec( node const& n )
  {

    if ( ntk.is_pi( n ) )
      return { 0, 0 };

    int32_t counter_and = 0;
    int32_t counter_xor = 0;

    if ( ntk.is_and( n ) )
    {
      counter_and = 1;
    }
    else if ( ntk.is_xor( n ) )
    {
      counter_xor = 1;
    }

    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk.get_node( f );

      ntk.decr_fanout_size( p );
      if ( ntk.fanout_size( p ) == 0 )
      {
        auto counter = node_deref_rec( p );
        counter_and += counter.first;
        counter_xor += counter.second;
      }
    } );

    return { counter_and, counter_xor };
  }

  /* ! \brief Reference the node's MFFC */
  std::pair<int32_t, int32_t> node_ref_rec( node const& n )
  {
    if ( ntk.is_pi( n ) )
      return { 0, 0 };

    int32_t counter_and = 0;
    int32_t counter_xor = 0;

    if ( ntk.is_and( n ) )
    {
      counter_and = 1;
    }
    else if ( ntk.is_xor( n ) )
    {
      counter_xor = 1;
    }

    ntk.foreach_fanin( n, [&]( const auto& f ) {
      auto const& p = ntk.get_node( f );

      auto v = ntk.fanout_size( p );
      ntk.incr_fanout_size( p );
      if ( v == 0 )
      {
        auto counter = node_ref_rec( p );
        counter_and += counter.first;
        counter_xor += counter.second;
      }
    } );

    return { counter_and, counter_xor };
  }

  void node_mffc_cone_rec( node const& n, std::vector<node>& cone, bool top_most )
  {
    /* skip visited nodes */
    if ( ntk.visited( n ) == ntk.trav_id() )
      return;
    ntk.set_visited( n, ntk.trav_id() );

    if ( !top_most && ( ntk.is_pi( n ) || ntk.fanout_size( n ) > 0 ) )
      return;

    /* recurse on children */
    ntk.foreach_fanin( n, [&]( const auto& f ) {
      node_mffc_cone_rec( ntk.get_node( f ), cone, false );
    } );

    /* collect the internal nodes */
    cone.emplace_back( n );
  }

  void node_mffc_cone( node const& n, std::vector<node>& cone )
  {
    cone.clear();
    ntk.incr_trav_id();
    node_mffc_cone_rec( n, cone, true );
  }

private:
  Ntk const& ntk;
};

} /* namespace detail */

template<typename Ntk, typename Simulator, typename TT>
struct xag_resub_functor
{
public:
  using node = xag_network::node;
  using signal = xag_network::signal;
  using stats = xag_resub_stats;

  struct unate_divisors
  {
    using signal = typename xag_network::signal;

    std::vector<signal> positive_divisors;
    std::vector<signal> negative_divisors;
    std::vector<signal> next_candidates;

    void clear()
    {
      positive_divisors.clear();
      negative_divisors.clear();
      next_candidates.clear();
    }
  };

  struct binate_divisors
  {
    using signal = typename xag_network::signal;

    std::vector<signal> positive_divisors0;
    std::vector<signal> positive_divisors1;
    std::vector<signal> negative_divisors0;
    std::vector<signal> negative_divisors1;

    void clear()
    {
      positive_divisors0.clear();
      positive_divisors1.clear();
      negative_divisors0.clear();
      negative_divisors1.clear();
    }
  };

public:
  explicit xag_resub_functor( Ntk& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, stats& st )
      : ntk( ntk ), sim( sim ), divs( divs ), num_divs( num_divs ), st( st )
  {
  }

  std::optional<signal> operator()( node const& root, TT care, uint32_t required, uint32_t max_inserts, std::pair<uint32_t, uint32_t> num_mffc, uint32_t& last_gain )
  {

    uint32_t num_and_mffc = num_mffc.first;
    uint32_t num_xor_mffc = num_mffc.second;
    /* consider constants */
    auto g = call_with_stopwatch( st.time_resubC, [&]() {
      return resub_const( root, care, required );
    } );
    if ( g )
    {
      ++st.num_const_accepts;
      last_gain = num_and_mffc;
      return g; /* accepted resub */
    }

    /* consider equal nodes */
    g = call_with_stopwatch( st.time_resub0, [&]() {
      return resub_div0( root, care, required );
    } );
    if ( g )
    {
      ++st.num_div0_accepts;
      last_gain = num_and_mffc;
      return g; /* accepted resub */
    }

    if ( num_and_mffc == 0 )
    {
      return std::nullopt;
      if ( max_inserts == 0 || num_xor_mffc == 1 )
        return std::nullopt;

      g = call_with_stopwatch( st.time_resub1, [&]() {
        return resub_div1( root, care, required );
      } );
      if ( g )
      {
        ++st.num_div1_accepts;
        last_gain = 0;
        return g; /* accepted resub */
      }

      if ( max_inserts == 1 || num_xor_mffc == 2 )
        return std::nullopt;

      /* consider two nodes */
      g = call_with_stopwatch( st.time_resub2, [&]() { return resub_div2( root, care, required ); } );
      if ( g )
      {
        ++st.num_div2_accepts;
        last_gain = 0;
        return g; /* accepted resub */
      }
    }
    else
    {

      g = call_with_stopwatch( st.time_resub1, [&]() {
        return resub_div1( root, care, required );
      } );
      if ( g )
      {
        ++st.num_div1_accepts;
        last_gain = num_and_mffc;
        return g; /* accepted resub */
      }

      /* consider two nodes */
      g = call_with_stopwatch( st.time_resub2, [&]() { return resub_div2( root, care, required ); } );
      if ( g )
      {
        ++st.num_div2_accepts;
        last_gain = num_and_mffc;
        return g; /*  accepted resub */
      }

      if ( num_and_mffc < 2 ) /* it is worth trying also AND resub here */
        return std::nullopt;

      /* collect level one divisors */
      call_with_stopwatch( st.time_collect_unate_divisors, [&]() {
        collect_unate_divisors( root, required );
      } );

      g = call_with_stopwatch( st.time_resub1_and, [&]() { return resub_div1_and( root, care, required ); } );
      if ( g )
      {
        ++st.num_div1_and_accepts;
        last_gain = num_and_mffc - 1;
        return g; /*  accepted resub */
      }
      if ( num_and_mffc < 3 ) /* it is worth trying also AND-12 resub here */
        return std::nullopt;

      /* consider triples */
      g = call_with_stopwatch( st.time_resub12, [&]() { return resub_div12( root, care, required ); } );
      if ( g )
      {
        ++st.num_div12_accepts;
        last_gain = num_and_mffc - 2;
        return g; /* accepted resub */
      }

      /* collect level two divisors */
      call_with_stopwatch( st.time_collect_binate_divisors, [&]() {
        collect_binate_divisors( root, required );
      } );

      /* consider two nodes */
      g = call_with_stopwatch( st.time_resub2_and, [&]() { return resub_div2_and( root, care, required ); } );
      if ( g )
      {
        ++st.num_div2_and_accepts;
        last_gain = num_and_mffc - 2;
        return g; /* accepted resub */
      }
    }
    return std::nullopt;
  }

  std::optional<signal> resub_const( node const& root, TT care, uint32_t required ) const
  {
    (void)required;
    auto tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    if ( binary_and( tt, care ) == sim.get_tt( ntk.get_constant( false ) ) )
    {
      return sim.get_phase( root ) ? ntk.get_constant( true ) : ntk.get_constant( false );
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div0( node const& root, TT care, uint32_t required ) const
  {
    (void)required;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const d = divs.at( i );

      if ( binary_and( tt, care ) != binary_and( sim.get_tt( ntk.make_signal( d ) ), care ) )
        continue;
      return ( sim.get_phase( d ) ^ sim.get_phase( root ) ) ? !ntk.make_signal( d ) : ntk.make_signal( d );
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div1( node const& root, TT care, uint32_t required )
  {
    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );
    /* check for divisors  */
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const& s0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const& s1 = divs.at( j );
        auto const& tt_s0 = sim.get_tt( ntk.make_signal( s0 ) );
        auto const& tt_s1 = sim.get_tt( ntk.make_signal( s1 ) );

        if ( binary_and( ( tt_s0 ^ tt_s1 ), care ) == binary_and( tt, care ) )
        {
          auto const l = sim.get_phase( s0 ) ? !ntk.make_signal( s0 ) : ntk.make_signal( s0 );
          auto const r = sim.get_phase( s1 ) ? !ntk.make_signal( s1 ) : ntk.make_signal( s1 );
          return sim.get_phase( root ) ? !ntk.create_xor( l, r ) : ntk.create_xor( l, r );
        }
        else if ( binary_and( ( tt_s0 ^ tt_s1 ), care ) == binary_and( kitty::unary_not( tt ), care ) )
        {
          auto const l = sim.get_phase( s0 ) ? !ntk.make_signal( s0 ) : ntk.make_signal( s0 );
          auto const r = sim.get_phase( s1 ) ? !ntk.make_signal( s1 ) : ntk.make_signal( s1 );
          return sim.get_phase( root ) ? ntk.create_xor( l, r ) : !ntk.create_xor( l, r );
        }
      }
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div2( node const& root, TT care, uint32_t required )
  {
    (void)required;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const s0 = divs.at( i );

      for ( auto j = i + 1; j < num_divs; ++j )
      {
        auto const s1 = divs.at( j );

        for ( auto k = j + 1; k < num_divs; ++k )
        {
          auto const s2 = divs.at( k );
          auto const& tt_s0 = sim.get_tt( ntk.make_signal( s0 ) );
          auto const& tt_s1 = sim.get_tt( ntk.make_signal( s1 ) );
          auto const& tt_s2 = sim.get_tt( ntk.make_signal( s2 ) );

          if ( binary_and( ( tt_s0 ^ tt_s1 ^ tt_s2 ), care ) == binary_and( tt, care ) )
          {
            auto const max_level = std::max( { ntk.level( s0 ),
                                               ntk.level( s1 ),
                                               ntk.level( s2 ) } );
            assert( max_level <= required - 1 );

            signal max = ntk.make_signal( s0 );
            signal min0 = ntk.make_signal( s1 );
            signal min1 = ntk.make_signal( s2 );
            if ( ntk.level( s1 ) == max_level )
            {
              max = ntk.make_signal( s1 );
              min0 = ntk.make_signal( s0 );
              min1 = ntk.make_signal( s2 );
            }
            else if ( ntk.level( s2 ) == max_level )
            {
              max = ntk.make_signal( s2 );
              min0 = ntk.make_signal( s0 );
              min1 = ntk.make_signal( s1 );
            }

            auto const a = sim.get_phase( ntk.get_node( max ) ) ? !max : max;
            auto const b = sim.get_phase( ntk.get_node( min0 ) ) ? !min0 : min0;
            auto const c = sim.get_phase( ntk.get_node( min1 ) ) ? !min1 : min1;

            return sim.get_phase( root ) ? !ntk.create_xor( a, ntk.create_xor( b, c ) ) : ntk.create_xor( a, ntk.create_xor( b, c ) );
          }
          else if ( binary_and( ( tt_s0 ^ tt_s1 ^ tt_s2 ), care ) == binary_and( kitty::unary_not( tt ), care ) )
          {
            auto const max_level = std::max( { ntk.level( s0 ),
                                               ntk.level( s1 ),
                                               ntk.level( s2 ) } );
            assert( max_level <= required - 1 );

            signal max = ntk.make_signal( s0 );
            signal min0 = ntk.make_signal( s1 );
            signal min1 = ntk.make_signal( s2 );
            if ( ntk.level( s1 ) == max_level )
            {
              max = ntk.make_signal( s1 );
              min0 = ntk.make_signal( s0 );
              min1 = ntk.make_signal( s2 );
            }
            else if ( ntk.level( s2 ) == max_level )
            {
              max = ntk.make_signal( s2 );
              min0 = ntk.make_signal( s0 );
              min1 = ntk.make_signal( s1 );
            }

            auto const a = sim.get_phase( ntk.get_node( max ) ) ? !max : max;
            auto const b = sim.get_phase( ntk.get_node( min0 ) ) ? !min0 : min0;
            auto const c = sim.get_phase( ntk.get_node( min1 ) ) ? !min1 : min1;

            return sim.get_phase( root ) ? ntk.create_xor( a, ntk.create_xor( b, c ) ) : !ntk.create_xor( a, ntk.create_xor( b, c ) );
          }
        }
      }
    }
    return std::nullopt;
  }

  void collect_unate_divisors( node const& root, uint32_t required )
  {
    udivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const d = divs.at( i );

      if ( ntk.level( d ) > required - 1 )
        continue;

      auto const& tt_d = sim.get_tt( ntk.make_signal( d ) );

      /* check positive containment */
      if ( kitty::implies( tt_d, tt ) )
      {
        udivs.positive_divisors.emplace_back( ntk.make_signal( d ) );
        continue;
      }

      /* check negative containment */
      if ( kitty::implies( tt, tt_d ) )
      {
        udivs.negative_divisors.emplace_back( ntk.make_signal( d ) );
        continue;
      }

      udivs.next_candidates.emplace_back( ntk.make_signal( d ) );
    }
  }

  std::optional<signal> resub_div1_and( node const& root, TT care, uint32_t required )
  {
    (void)required;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    /* check for positive unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const& s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const& s1 = udivs.positive_divisors.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );

        if ( binary_and( ( tt_s0 | tt_s1 ), care ) == binary_and( tt, care ) )
        {
          auto const l = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const r = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          return sim.get_phase( root ) ? !ntk.create_or( l, r ) : ntk.create_or( l, r );
        }
      }
    }
    /* check for negative unate divisors */
    for ( auto i = 0u; i < udivs.negative_divisors.size(); ++i )
    {
      auto const& s0 = udivs.negative_divisors.at( i );

      for ( auto j = i + 1; j < udivs.negative_divisors.size(); ++j )
      {
        auto const& s1 = udivs.negative_divisors.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );

        if ( binary_and( ( tt_s0 & tt_s1 ), care ) == binary_and( tt, care ) )
        {
          auto const l = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const r = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          return sim.get_phase( root ) ? !ntk.create_and( l, r ) : ntk.create_and( l, r );
        }
      }
    }

    return std::nullopt;
  }

  std::optional<signal> resub_div12( node const& root, TT care, uint32_t required )
  {
    (void)required;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    /* check positive unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const s1 = udivs.positive_divisors.at( j );

        for ( auto k = j + 1; k < udivs.positive_divisors.size(); ++k )
        {
          auto const s2 = udivs.positive_divisors.at( k );

          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          auto const& tt_s2 = sim.get_tt( s2 );

          if ( binary_and( ( tt_s0 | tt_s1 | tt_s2 ), care ) == binary_and( tt, care ) )
          {
            auto const max_level = std::max( { ntk.level( ntk.get_node( s0 ) ),
                                               ntk.level( ntk.get_node( s1 ) ),
                                               ntk.level( ntk.get_node( s2 ) ) } );
            assert( max_level <= required - 1 );

            signal max = s0;
            signal min0 = s1;
            signal min1 = s2;
            if ( ntk.level( ntk.get_node( s1 ) ) == max_level )
            {
              max = s1;
              min0 = s0;
              min1 = s2;
            }
            else if ( ntk.level( ntk.get_node( s2 ) ) == max_level )
            {
              max = s2;
              min0 = s0;
              min1 = s1;
            }

            auto const a = sim.get_phase( ntk.get_node( max ) ) ? !max : max;
            auto const b = sim.get_phase( ntk.get_node( min0 ) ) ? !min0 : min0;
            auto const c = sim.get_phase( ntk.get_node( min1 ) ) ? !min1 : min1;

            return sim.get_phase( root ) ? !ntk.create_or( a, ntk.create_or( b, c ) ) : ntk.create_or( a, ntk.create_or( b, c ) );
          }
        }
      }
    }

    /* check negative unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const s1 = udivs.positive_divisors.at( j );

        for ( auto k = j + 1; k < udivs.positive_divisors.size(); ++k )
        {
          auto const s2 = udivs.positive_divisors.at( k );

          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          auto const& tt_s2 = sim.get_tt( s2 );

          if ( binary_and( ( tt_s0 & tt_s1 & tt_s2 ), care ) == binary_and( tt, care ) )
          {
            auto const max_level = std::max( { ntk.level( ntk.get_node( s0 ) ),
                                               ntk.level( ntk.get_node( s1 ) ),
                                               ntk.level( ntk.get_node( s2 ) ) } );
            assert( max_level <= required - 1 );

            signal max = s0;
            signal min0 = s1;
            signal min1 = s2;
            if ( ntk.level( ntk.get_node( s1 ) ) == max_level )
            {
              max = s1;
              min0 = s0;
              min1 = s2;
            }
            else if ( ntk.level( ntk.get_node( s2 ) ) == max_level )
            {
              max = s2;
              min0 = s0;
              min1 = s1;
            }

            auto const a = sim.get_phase( ntk.get_node( max ) ) ? !max : max;
            auto const b = sim.get_phase( ntk.get_node( min0 ) ) ? !min0 : min0;
            auto const c = sim.get_phase( ntk.get_node( min1 ) ) ? !min1 : min1;

            return sim.get_phase( root ) ? !ntk.create_and( a, ntk.create_and( b, c ) ) : ntk.create_and( a, ntk.create_and( b, c ) );
          }
        }
      }
    }

    return std::nullopt;
  }

  void collect_binate_divisors( node const& root, uint32_t required )
  {
    bdivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < udivs.next_candidates.size(); ++i )
    {
      auto const& s0 = udivs.next_candidates.at( i );
      if ( ntk.level( ntk.get_node( s0 ) ) > required - 2 )
        continue;

      for ( auto j = i + 1; j < udivs.next_candidates.size(); ++j )
      {
        auto const& s1 = udivs.next_candidates.at( j );
        if ( ntk.level( ntk.get_node( s1 ) ) > required - 2 )
          continue;

        if ( bdivs.positive_divisors0.size() < 500 ) // ps.max_divisors2
        {
          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          if ( kitty::implies( tt_s0 & tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( s0 );
            bdivs.positive_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( ~tt_s0 & tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( !s0 );
            bdivs.positive_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( tt_s0 & ~tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( s0 );
            bdivs.positive_divisors1.emplace_back( !s1 );
          }

          if ( kitty::implies( ~tt_s0 & ~tt_s1, tt ) )
          {
            bdivs.positive_divisors0.emplace_back( !s0 );
            bdivs.positive_divisors1.emplace_back( !s1 );
          }
        }

        if ( bdivs.negative_divisors0.size() < 500 ) // ps.max_divisors2
        {
          auto const& tt_s0 = sim.get_tt( s0 );
          auto const& tt_s1 = sim.get_tt( s1 );
          if ( kitty::implies( tt, tt_s0 & tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( s0 );
            bdivs.negative_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( tt, ~tt_s0 & tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( !s0 );
            bdivs.negative_divisors1.emplace_back( s1 );
          }

          if ( kitty::implies( tt, tt_s0 & ~tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( s0 );
            bdivs.negative_divisors1.emplace_back( !s1 );
          }

          if ( kitty::implies( tt, ~tt_s0 & ~tt_s1 ) )
          {
            bdivs.negative_divisors0.emplace_back( !s0 );
            bdivs.negative_divisors1.emplace_back( !s1 );
          }
        }
      }
    }
  }

  std::optional<signal> resub_div2_and( node const& root, TT care, uint32_t required )
  {
    (void)required;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );
    if ( care.num_vars() > tt.num_vars() )
      care = kitty::shrink_to( care, tt.num_vars() );
    else
      care = kitty::extend_to( care, tt.num_vars() );

    /* check positive unate divisors */
    for ( const auto& s0 : udivs.positive_divisors )
    {
      auto const& tt_s0 = sim.get_tt( s0 );

      for ( auto j = 0u; j < bdivs.positive_divisors0.size(); ++j )
      {
        auto const s1 = bdivs.positive_divisors0.at( j );
        auto const s2 = bdivs.positive_divisors1.at( j );

        auto const& tt_s1 = sim.get_tt( s1 );
        auto const& tt_s2 = sim.get_tt( s2 );

        auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
        auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
        auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;

        if ( binary_and( ( tt_s0 | ( tt_s1 & tt_s2 ) ), care ) == binary_and( tt, care ) )
        {
          return sim.get_phase( root ) ? !ntk.create_or( a, ntk.create_and( b, c ) ) : ntk.create_or( a, ntk.create_and( b, c ) );
        }
      }
    }

    /* check negative unate divisors */
    for ( const auto& s0 : udivs.negative_divisors )
    {
      auto const& tt_s0 = sim.get_tt( s0 );

      for ( auto j = 0u; j < bdivs.negative_divisors0.size(); ++j )
      {
        auto const s1 = bdivs.negative_divisors0.at( j );
        auto const s2 = bdivs.negative_divisors1.at( j );

        auto const& tt_s1 = sim.get_tt( s1 );
        auto const& tt_s2 = sim.get_tt( s2 );

        auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
        auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
        auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;

        if ( binary_and( ( tt_s0 | ( tt_s1 & tt_s2 ) ), care ) == binary_and( tt, care ) )
        {
          return sim.get_phase( root ) ? !ntk.create_and( a, ntk.create_or( b, c ) ) : ntk.create_and( a, ntk.create_or( b, c ) );
        }
      }
    }

    return std::nullopt;
  }

private:
  Ntk& ntk;
  Simulator const& sim;
  std::vector<node> const& divs;
  uint32_t const num_divs;
  stats& st;

  unate_divisors udivs;
  binate_divisors bdivs;
}; /* xag_resub_functor */

template<class Ntk>
void resubstitution_minmc_withDC( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( has_clear_values_v<Ntk>, "Ntk does not implement the clear_values method" );
  static_assert( has_fanout_size_v<Ntk>, "Ntk does not implement the fanout_size method" );
  static_assert( has_foreach_fanin_v<Ntk>, "Ntk does not implement the foreach_fanin method" );
  static_assert( has_foreach_gate_v<Ntk>, "Ntk does not implement the foreach_gate method" );
  static_assert( has_foreach_node_v<Ntk>, "Ntk does not implement the foreach_node method" );
  static_assert( has_get_constant_v<Ntk>, "Ntk does not implement the get_constant method" );
  static_assert( has_get_node_v<Ntk>, "Ntk does not implement the get_node method" );
  static_assert( has_is_complemented_v<Ntk>, "Ntk does not implement the is_complemented method" );
  static_assert( has_is_pi_v<Ntk>, "Ntk does not implement the is_pi method" );
  static_assert( has_make_signal_v<Ntk>, "Ntk does not implement the make_signal method" );
  static_assert( has_set_value_v<Ntk>, "Ntk does not implement the set_value method" );
  static_assert( has_set_visited_v<Ntk>, "Ntk does not implement the set_visited method" );
  static_assert( has_size_v<Ntk>, "Ntk does not implement the has_size method" );
  static_assert( has_substitute_node_v<Ntk>, "Ntk does not implement the has substitute_node method" );
  static_assert( has_value_v<Ntk>, "Ntk does not implement the has_value method" );
  static_assert( has_visited_v<Ntk>, "Ntk does not implement the has_visited method" );

  using resub_view_t = fanout_view<depth_view<Ntk>>;
  depth_view<Ntk> depth_view{ ntk };
  resub_view_t resub_view{ depth_view };

  using truthtable_t = kitty::dynamic_truth_table;
  using mffc_result_t = std::pair<uint32_t, uint32_t>;
  using resub_impl_t = detail::resubstitution_impl<resub_view_t, typename detail::window_based_resub_engine<resub_view_t, truthtable_t, truthtable_t, xag_resub_functor<resub_view_t, typename detail::window_simulator<resub_view_t, truthtable_t>, truthtable_t>, mffc_result_t>, typename detail::default_divisor_collector<Ntk, typename detail::node_mffc_inside_xag<Ntk>, mffc_result_t>>;

  resubstitution_stats st;
  typename resub_impl_t::engine_st_t engine_st;
  typename resub_impl_t::collector_st_t collector_st;

  resub_impl_t p( resub_view, ps, st, engine_st, collector_st );
  p.run();

  if ( ps.verbose )
  {
    st.report();
    collector_st.report();
    engine_st.report();
  }

  if ( pst )
  {
    *pst = st;
  }
}

} // namespace mockturtle