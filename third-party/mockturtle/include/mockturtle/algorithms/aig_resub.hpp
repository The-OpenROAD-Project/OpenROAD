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
  \file aig_resub.hpp
  \brief Resubstitution

  \author Alessandro Tempia Calvino
  \author Eleonora Testa
  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
*/

#pragma once

#include "../networks/aig.hpp"
#include "../utils/index_list/index_list.hpp"
#include "../utils/truth_table_utils.hpp"
#include "resubstitution.hpp"
#include "resyn_engines/xag_resyn.hpp"

namespace mockturtle
{

struct aig_resub_stats
{
  /*! \brief Accumulated runtime for const-resub */
  stopwatch<>::duration time_resubC{ 0 };

  /*! \brief Accumulated runtime for zero-resub */
  stopwatch<>::duration time_resub0{ 0 };

  /*! \brief Accumulated runtime for collecting unate divisors. */
  stopwatch<>::duration time_collect_unate_divisors{ 0 };

  /*! \brief Accumulated runtime for one-resub */
  stopwatch<>::duration time_resub1{ 0 };

  /*! \brief Accumulated runtime for 12-resub. */
  stopwatch<>::duration time_resub12{ 0 };

  /*! \brief Accumulated runtime for collecting unate divisors. */
  stopwatch<>::duration time_collect_binate_divisors{ 0 };

  /*! \brief Accumulated runtime for two-resub. */
  stopwatch<>::duration time_resub2{ 0 };

  /*! \brief Accumulated runtime for three-resub. */
  stopwatch<>::duration time_resub3{ 0 };

  /*! \brief Number of accepted constant resubsitutions */
  uint32_t num_const_accepts{ 0 };

  /*! \brief Number of accepted zero resubsitutions */
  uint32_t num_div0_accepts{ 0 };

  /*! \brief Number of accepted one resubsitutions */
  uint64_t num_div1_accepts{ 0 };

  /*! \brief Number of accepted single AND-resubsitutions */
  uint64_t num_div1_and_accepts{ 0 };

  /*! \brief Number of accepted single OR-resubsitutions */
  uint64_t num_div1_or_accepts{ 0 };

  /*! \brief Number of accepted two resubsitutions using triples of unate divisors */
  uint64_t num_div12_accepts{ 0 };

  /*! \brief Number of accepted single 2AND-resubsitutions */
  uint64_t num_div12_2and_accepts{ 0 };

  /*! \brief Number of accepted single 2OR-resubsitutions */
  uint64_t num_div12_2or_accepts{ 0 };

  /*! \brief Number of accepted two resubsitutions */
  uint64_t num_div2_accepts{ 0 };

  /*! \brief Number of accepted double AND-OR-resubsitutions */
  uint64_t num_div2_and_or_accepts{ 0 };

  /*! \brief Number of accepted double OR-AND-resubsitutions */
  uint64_t num_div2_or_and_accepts{ 0 };

  /*! \brief Number of accepted three resubsitutions */
  uint64_t num_div3_accepts{ 0 };

  /*! \brief Number of accepted AND-2OR-resubsitutions */
  uint64_t num_div3_and_2or_accepts{ 0 };

  /*! \brief Number of accepted OR-2AND-resubsitutions */
  uint64_t num_div3_or_2and_accepts{ 0 };

  void report() const
  {
    std::cout << "[i] kernel: aig_resub_functor\n";
    std::cout << fmt::format( "[i]     constant-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_const_accepts, to_seconds( time_resubC ) );
    std::cout << fmt::format( "[i]            0-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div0_accepts, to_seconds( time_resub0 ) );
    std::cout << fmt::format( "[i]            collect unate divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_unate_divisors ) );
    std::cout << fmt::format( "[i]            1-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div1_accepts, to_seconds( time_resub1 ) );
    std::cout << fmt::format( "[i]           12-resub {:6d} = {:6d} 2AND    + {:6d} 2OR     ({:>5.2f} secs)\n",
                              num_div12_accepts, num_div12_2and_accepts, num_div12_2or_accepts, to_seconds( time_resub12 ) );
    std::cout << fmt::format( "[i]            collect binate divisors                          ({:>5.2f} secs)\n", to_seconds( time_collect_binate_divisors ) );
    std::cout << fmt::format( "[i]            2-resub {:6d} = {:6d} AND-OR  + {:6d} OR-AND  ({:>5.2f} secs)\n",
                              num_div2_accepts, num_div2_and_or_accepts, num_div2_or_and_accepts, to_seconds( time_resub2 ) );
    std::cout << fmt::format( "[i]            3-resub {:6d} = {:6d} AND-2OR + {:6d} OR-2AND ({:>5.2f} secs)\n",
                              num_div3_accepts, num_div3_and_2or_accepts, num_div3_or_2and_accepts, to_seconds( time_resub3 ) );
    std::cout << fmt::format( "[i]            total   {:6d}\n",
                              ( num_const_accepts + num_div0_accepts + num_div1_accepts + num_div12_accepts + num_div2_accepts + num_div3_accepts ) );
  }
}; /* aig_resub_stats */

template<typename Ntk, typename Simulator, typename TT>
struct aig_resub_functor
{
public:
  using node = aig_network::node;
  using signal = aig_network::signal;
  using stats = aig_resub_stats;

  struct unate_divisors
  {
    using signal = typename aig_network::signal;

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
    using signal = typename aig_network::signal;

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
  explicit aig_resub_functor( Ntk& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, stats& st )
      : ntk( ntk ), sim( sim ), divs( divs ), num_divs( num_divs ), st( st )
  {
  }

  std::optional<signal> operator()( node const& root, TT care, uint32_t max_depth, uint32_t max_inserts, uint32_t num_mffc, uint32_t& last_gain )
  {
    (void)care;
    assert( is_const0( ~care ) );

    /* consider constants */
    auto g = call_with_stopwatch( st.time_resubC, [&]() {
      return resub_const( root );
    } );
    if ( g )
    {
      ++st.num_const_accepts;
      last_gain = num_mffc;
      return g; /* accepted resub */
    }

    /* consider equal nodes */
    g = call_with_stopwatch( st.time_resub0, [&]() {
      return resub_div0( root, max_depth );
    } );
    if ( g )
    {
      ++st.num_div0_accepts;
      last_gain = num_mffc;
      return g; /* accepted resub */
    }

    if ( max_inserts == 0 || num_mffc == 1 )
      return std::nullopt;

    /* collect level one divisors */
    call_with_stopwatch( st.time_collect_unate_divisors, [&]() {
      collect_unate_divisors( root, max_depth );
    } );

    /* consider equal nodes */
    g = call_with_stopwatch( st.time_resub1, [&]() {
      return resub_div1( root, max_depth );
    } );
    if ( g )
    {
      ++st.num_div1_accepts;
      last_gain = num_mffc - 1;
      return g; /* accepted resub */
    }

    if ( max_inserts == 1 || num_mffc == 2 )
      return std::nullopt;

    /* consider triples */
    g = call_with_stopwatch( st.time_resub12, [&]() { return resub_div12( root, max_depth ); } );
    if ( g )
    {
      ++st.num_div12_accepts;
      last_gain = num_mffc - 2;
      return g; /* accepted resub */
    }

    /* collect level two divisors */
    call_with_stopwatch( st.time_collect_binate_divisors, [&]() {
      collect_binate_divisors( root, max_depth );
    } );

    /* consider two nodes */
    g = call_with_stopwatch( st.time_resub2, [&]() { return resub_div2( root, max_depth ); } );
    if ( g )
    {
      ++st.num_div2_accepts;
      last_gain = num_mffc - 2;
      return g; /* accepted resub */
    }

    if ( max_inserts == 2 || num_mffc == 3 )
      return std::nullopt;

    /* consider three nodes */
    g = call_with_stopwatch( st.time_resub3, [&]() { return resub_div3( root, max_depth ); } );
    if ( g )
    {
      ++st.num_div3_accepts;
      last_gain = num_mffc - 3;
      return g; /* accepted resub */
    }

    return std::nullopt;
  }

  std::optional<signal> resub_const( node const& root ) const
  {
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    if ( tt == sim.get_tt( ntk.get_constant( false ) ) )
    {
      return sim.get_phase( root ) ? ntk.get_constant( true ) : ntk.get_constant( false );
    }
    return std::nullopt;
  }

  std::optional<signal> resub_div0( node const& root, uint32_t max_depth ) const
  {
    (void)max_depth;
    auto const tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const d = divs.at( i );
      if ( tt != sim.get_tt( ntk.make_signal( d ) ) )
        continue; /* next */

      assert( ntk.level( d ) <= max_depth );
      return ( sim.get_phase( d ) ^ sim.get_phase( root ) ) ? !ntk.make_signal( d ) : ntk.make_signal( d );
    }

    return std::nullopt;
  }

  void collect_unate_divisors( node const& root, uint32_t max_depth )
  {
    udivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < num_divs; ++i )
    {
      auto const d = divs.at( i );

      if ( ntk.level( d ) > max_depth - 1 )
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

      if ( true ) // ( ps.fix_bug )
      {
        /* unreachable case */
        // if ( kitty::implies( ~tt_d, tt ) )
        // {
        //   udivs.positive_divisors.emplace_back( !ntk.make_signal( d ) );
        //   continue;
        // }
        if ( kitty::implies( tt, ~tt_d ) )
        {
          udivs.negative_divisors.emplace_back( !ntk.make_signal( d ) );
          continue;
        }
      }

      udivs.next_candidates.emplace_back( ntk.make_signal( d ) );
    }
  }

  std::optional<signal> resub_div1( node const& root, uint32_t max_depth )
  {
    (void)max_depth;
    auto const& tt = sim.get_tt( ntk.make_signal( root ) );

    /* check for positive unate divisors */
    for ( auto i = 0u; i < udivs.positive_divisors.size(); ++i )
    {
      auto const& s0 = udivs.positive_divisors.at( i );

      for ( auto j = i + 1; j < udivs.positive_divisors.size(); ++j )
      {
        auto const& s1 = udivs.positive_divisors.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );

        if ( ( tt_s0 | tt_s1 ) == tt )
        {
          ++st.num_div1_or_accepts;
          auto const l = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const r = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          assert( ntk.level( ntk.get_node( l ) ) <= max_depth - 1 && ntk.level( ntk.get_node( r ) ) <= max_depth - 1 );
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

        if ( ( tt_s0 & tt_s1 ) == tt )
        {
          ++st.num_div1_and_accepts;
          auto const l = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const r = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          assert( ntk.level( ntk.get_node( l ) ) <= max_depth - 1 && ntk.level( ntk.get_node( r ) ) <= max_depth - 1 );
          return sim.get_phase( root ) ? !ntk.create_and( l, r ) : ntk.create_and( l, r );
        }
      }
    }

    return std::nullopt;
  }

  std::optional<signal> resub_div12( node const& root, uint32_t max_depth )
  {
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );

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

          if ( ( tt_s0 | tt_s1 | tt_s2 ) == tt )
          {
            auto const max_level = std::max( { ntk.level( ntk.get_node( s0 ) ),
                                               ntk.level( ntk.get_node( s1 ) ),
                                               ntk.level( ntk.get_node( s2 ) ) } );
            assert( max_level <= max_depth - 1 );

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

            if ( ntk.level( ntk.get_node( min0 ) ) > max_level - 2 || ntk.level( ntk.get_node( min1 ) ) > max_level - 2 )
              continue;

            auto const a = sim.get_phase( ntk.get_node( max ) ) ? !max : max;
            auto const b = sim.get_phase( ntk.get_node( min0 ) ) ? !min0 : min0;
            auto const c = sim.get_phase( ntk.get_node( min1 ) ) ? !min1 : min1;

            ++st.num_div12_2or_accepts;
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

          if ( ( tt_s0 & tt_s1 & tt_s2 ) == tt )
          {
            auto const max_level = std::max( { ntk.level( ntk.get_node( s0 ) ),
                                               ntk.level( ntk.get_node( s1 ) ),
                                               ntk.level( ntk.get_node( s2 ) ) } );
            assert( max_level <= max_depth - 1 );

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

            if ( ntk.level( ntk.get_node( min0 ) ) > max_level - 2 || ntk.level( ntk.get_node( min1 ) ) > max_level - 2 )
              continue;

            auto const a = sim.get_phase( ntk.get_node( max ) ) ? !max : max;
            auto const b = sim.get_phase( ntk.get_node( min0 ) ) ? !min0 : min0;
            auto const c = sim.get_phase( ntk.get_node( min1 ) ) ? !min1 : min1;

            ++st.num_div12_2and_accepts;
            return sim.get_phase( root ) ? !ntk.create_and( a, ntk.create_and( b, c ) ) : ntk.create_and( a, ntk.create_and( b, c ) );
          }
        }
      }
    }

    return std::nullopt;
  }

  void collect_binate_divisors( node const& root, uint32_t max_depth )
  {
    bdivs.clear();

    auto const& tt = sim.get_tt( ntk.make_signal( root ) );
    for ( auto i = 0u; i < udivs.next_candidates.size(); ++i )
    {
      auto const& s0 = udivs.next_candidates.at( i );
      if ( ntk.level( ntk.get_node( s0 ) ) > max_depth - 2 )
        continue;

      for ( auto j = i + 1; j < udivs.next_candidates.size(); ++j )
      {
        auto const& s1 = udivs.next_candidates.at( j );
        if ( ntk.level( ntk.get_node( s1 ) ) > max_depth - 2 )
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

  std::optional<signal> resub_div2( node const& root, uint32_t max_depth )
  {
    (void)max_depth;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );

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

        if ( ( tt_s0 | ( tt_s1 & tt_s2 ) ) == tt )
        {
          ++st.num_div2_or_and_accepts;
          assert( ntk.level( ntk.get_node( a ) ) <= max_depth - 1 );
          assert( ntk.level( ntk.get_node( b ) ) <= max_depth - 2 && ntk.level( ntk.get_node( c ) ) <= max_depth - 2 );
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

        if ( ( tt_s0 | ( tt_s1 & tt_s2 ) ) == tt )
        {
          ++st.num_div2_or_and_accepts;
          assert( ntk.level( ntk.get_node( a ) ) <= max_depth - 1 );
          assert( ntk.level( ntk.get_node( b ) ) <= max_depth - 2 && ntk.level( ntk.get_node( c ) ) <= max_depth - 2 );
          return sim.get_phase( root ) ? !ntk.create_and( a, ntk.create_or( b, c ) ) : ntk.create_and( a, ntk.create_or( b, c ) );
        }
      }
    }

    return std::nullopt;
  }

  std::optional<signal> resub_div3( node const& root, uint32_t max_depth )
  {
    (void)max_depth;
    auto const s = ntk.make_signal( root );
    auto const& tt = sim.get_tt( s );

    for ( auto i = 0u; i < bdivs.positive_divisors0.size(); ++i )
    {
      auto const s0 = bdivs.positive_divisors0.at( i );
      auto const s1 = bdivs.positive_divisors1.at( i );

      for ( auto j = i + 1; j < bdivs.positive_divisors0.size(); ++j )
      {
        auto const s2 = bdivs.positive_divisors0.at( j );
        auto const s3 = bdivs.positive_divisors1.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );
        auto const& tt_s2 = sim.get_tt( s2 );
        auto const& tt_s3 = sim.get_tt( s3 );

        if ( ( ( tt_s0 | tt_s1 ) & ( tt_s2 | tt_s3 ) ) == tt )
        {
          auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;
          auto const d = sim.get_phase( ntk.get_node( s3 ) ) ? !s3 : s3;

          ++st.num_div3_and_2or_accepts;
          assert( ntk.level( ntk.get_node( a ) ) <= max_depth - 2 && ntk.level( ntk.get_node( b ) ) <= max_depth - 2 );
          assert( ntk.level( ntk.get_node( c ) ) <= max_depth - 2 && ntk.level( ntk.get_node( d ) ) <= max_depth - 2 );
          return sim.get_phase( root ) ? !ntk.create_and( ntk.create_or( a, b ), ntk.create_or( c, d ) ) : ntk.create_and( ntk.create_or( a, b ), ntk.create_or( c, d ) );
        }
      }
    }

    for ( auto i = 0u; i < bdivs.negative_divisors0.size(); ++i )
    {
      auto const s0 = bdivs.negative_divisors0.at( i );
      auto const s1 = bdivs.negative_divisors1.at( i );

      for ( auto j = i + 1; j < bdivs.negative_divisors0.size(); ++j )
      {
        auto const s2 = bdivs.negative_divisors0.at( j );
        auto const s3 = bdivs.negative_divisors1.at( j );

        auto const& tt_s0 = sim.get_tt( s0 );
        auto const& tt_s1 = sim.get_tt( s1 );
        auto const& tt_s2 = sim.get_tt( s2 );
        auto const& tt_s3 = sim.get_tt( s3 );

        if ( ( ( tt_s0 & tt_s1 ) | ( tt_s2 & tt_s3 ) ) == tt )
        {
          auto const a = sim.get_phase( ntk.get_node( s0 ) ) ? !s0 : s0;
          auto const b = sim.get_phase( ntk.get_node( s1 ) ) ? !s1 : s1;
          auto const c = sim.get_phase( ntk.get_node( s2 ) ) ? !s2 : s2;
          auto const d = sim.get_phase( ntk.get_node( s3 ) ) ? !s3 : s3;

          ++st.num_div3_or_2and_accepts;
          assert( ntk.level( ntk.get_node( a ) ) <= max_depth - 2 && ntk.level( ntk.get_node( b ) ) <= max_depth - 2 );
          assert( ntk.level( ntk.get_node( c ) ) <= max_depth - 2 && ntk.level( ntk.get_node( d ) ) <= max_depth - 2 );
          return sim.get_phase( root ) ? !ntk.create_or( ntk.create_and( a, b ), ntk.create_and( c, d ) ) : ntk.create_or( ntk.create_and( a, b ), ntk.create_and( c, d ) );
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
}; /* aig_resub_functor */

struct aig_resyn_resub_stats
{
  /*! \brief Time for finding dependency function. */
  stopwatch<>::duration time_compute_function{ 0 };

  /*! \brief Number of found solutions. */
  uint32_t num_success{ 0 };

  /*! \brief Number of times that no solution can be found. */
  uint32_t num_fail{ 0 };

  void report() const
  {
    fmt::print( "[i]     <ResubFn: aig_resyn_functor>\n" );
    fmt::print( "[i]         #solution = {:6d}\n", num_success );
    fmt::print( "[i]         #invoke   = {:6d}\n", num_success + num_fail );
    fmt::print( "[i]         engine time: {:>5.2f} secs\n", to_seconds( time_compute_function ) );
  }
}; /* aig_resyn_resub_stats */

/*! \brief Interfacing resubstitution functor with AIG resynthesis engines for `window_based_resub_engine`.
 */
template<typename Ntk, typename Simulator, typename TTcare, typename ResynEngine = xag_resyn_decompose<typename Simulator::truthtable_t, aig_resyn_static_params_for_win_resub<Ntk>>>
struct aig_resyn_functor
{
public:
  using node = aig_network::node;
  using signal = aig_network::signal;
  using stats = aig_resyn_resub_stats;
  using TT = typename ResynEngine::truth_table_t;

  static_assert( std::is_same_v<TT, typename Simulator::truthtable_t>, "truth table type of the simulator does not match" );

public:
  explicit aig_resyn_functor( Ntk& ntk, Simulator const& sim, std::vector<node> const& divs, uint32_t num_divs, stats& st )
      : ntk( ntk ), sim( sim ), tts( ntk ), divs( divs ), st( st )
  {
    assert( divs.size() == num_divs );
    (void)num_divs;
    div_signals.reserve( divs.size() );
  }

  std::optional<signal> operator()( node const& root, TTcare care, uint32_t required, uint32_t max_inserts, uint32_t potential_gain, uint32_t& real_gain )
  {
    (void)required;
    TT target = sim.get_tt( sim.get_phase( root ) ? !ntk.make_signal( root ) : ntk.make_signal( root ) );
    TT care_transformed = target.construct();
    care_transformed = care;

    typename ResynEngine::stats st_eng;
    ResynEngine engine( st_eng );
    for ( auto const& d : divs )
    {
      div_signals.emplace_back( sim.get_phase( d ) ? !ntk.make_signal( d ) : ntk.make_signal( d ) );
      tts[d] = sim.get_tt( ntk.make_signal( d ) );
    }

    auto const res = call_with_stopwatch( st.time_compute_function, [&]() {
      return engine( target, care_transformed, std::begin( divs ), std::end( divs ), tts, std::min( potential_gain - 1, max_inserts ) );
    } );
    if ( res )
    {
      ++st.num_success;
      signal ret;
      real_gain = potential_gain - ( *res ).num_gates();
      insert( ntk, div_signals.begin(), div_signals.end(), *res, [&]( signal const& s ) { ret = s; } );
      return ret;
    }
    else
    {
      ++st.num_fail;
      return std::nullopt;
    }
  }

private:
  Ntk& ntk;
  Simulator const& sim;
  unordered_node_map<TT, Ntk> tts;
  std::vector<node> const& divs;
  std::vector<signal> div_signals;
  stats& st;
}; /* aig_resyn_functor */

template<class Ntk>
void aig_resubstitution( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  /* TODO: check if basetype of ntk is aig */
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

  if ( ps.max_pis == 8 )
  {
    using truthtable_t = kitty::static_truth_table<8u>;
    using truthtable_dc_t = kitty::dynamic_truth_table;
    using resub_impl_t = detail::resubstitution_impl<resub_view_t, typename detail::window_based_resub_engine<resub_view_t, truthtable_t, truthtable_dc_t, aig_resub_functor<resub_view_t, typename detail::window_simulator<resub_view_t, truthtable_t>, truthtable_dc_t>>>;

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
  else
  {
    using truthtable_t = kitty::dynamic_truth_table;
    using truthtable_dc_t = kitty::dynamic_truth_table;
    using resub_impl_t = detail::resubstitution_impl<resub_view_t, typename detail::window_based_resub_engine<resub_view_t, truthtable_t, truthtable_dc_t, aig_resub_functor<resub_view_t, typename detail::window_simulator<resub_view_t, truthtable_t>, truthtable_dc_t>>>;

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
}

/*! \brief AIG-specific resubstitution algorithm.
 *
 * This algorithms iterates over each node, creates a
 * reconvergence-driven cut, and attempts to re-express the node's
 * function using existing nodes from the cut.  Node which are no
 * longer used (including nodes in their transitive fanins) can then
 * be removed.  The objective is to reduce the size of the network as
 * much as possible while maintaining the global input-output
 * functionality.
 *
 * **Required network functions:**
 *
 * - `clear_values`
 * - `fanout_size`
 * - `foreach_fanin`
 * - `foreach_fanout`
 * - `foreach_gate`
 * - `foreach_node`
 * - `get_constant`
 * - `get_node`
 * - `is_complemented`
 * - `is_pi`
 * - `level`
 * - `make_signal`
 * - `set_value`
 * - `set_visited`
 * - `size`
 * - `substitute_node`
 * - `value`
 * - `visited`
 *
 * \param ntk A network type derived from aig_network
 * \param ps Resubstitution parameters
 * \param pst Resubstitution statistics
 */
template<class Ntk>
void aig_resubstitution2( Ntk& ntk, resubstitution_params const& ps = {}, resubstitution_stats* pst = nullptr )
{
  static_assert( is_network_type_v<Ntk>, "Ntk is not a network type" );
  static_assert( std::is_same_v<typename Ntk::base_type, aig_network>, "Network type is not aig_network" );

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
  static_assert( has_level_v<Ntk>, "Ntk does not implement the level method" );
  static_assert( has_foreach_fanout_v<Ntk>, "Ntk does not implement the foreach_fanout method" );

  using truthtable_t = kitty::dynamic_truth_table;
  using truthtable_dc_t = kitty::dynamic_truth_table;
  using functor_t = aig_resyn_functor<Ntk, detail::window_simulator<Ntk, truthtable_t>, truthtable_dc_t>;

  using resub_impl_t = detail::resubstitution_impl<Ntk, detail::window_based_resub_engine<Ntk, truthtable_t, truthtable_dc_t, functor_t>>;

  resubstitution_stats st;
  typename resub_impl_t::engine_st_t engine_st;
  typename resub_impl_t::collector_st_t collector_st;

  resub_impl_t p( ntk, ps, st, engine_st, collector_st );
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

} /* namespace mockturtle */