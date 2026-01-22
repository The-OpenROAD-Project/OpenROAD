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
  \file aig_enumerative.hpp
  \brief AIG enumerative resynthesis

  \author Hanyu Wang
  \author Siang-Yun (Sonia) Lee

  Based on previous implementation of AIG resubstitution by
  Eleonora Testa, Heinz Riener, and Mathias Soeken
*/

#pragma once

#include "../../utils/index_list/index_list.hpp"
#include "../../utils/null_utils.hpp"
#include <kitty/kitty.hpp>
#include <optional>
#include <vector>

namespace mockturtle
{

struct aig_enumerative_resyn_stats
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
    // clang-format off
    fmt::print( "[i] aig_enumerative_resyn_stats\n" );
    fmt::print( "[i]     constant-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_const_accepts, to_seconds( time_resubC ) );
    fmt::print( "[i]     0-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div0_accepts, to_seconds( time_resub0 ) );
    fmt::print( "[i]     collect unate divisors                           ({:>5.2f} secs)\n", to_seconds( time_collect_unate_divisors ) );
    fmt::print( "[i]     1-resub {:6d}                                   ({:>5.2f} secs)\n",
                              num_div1_accepts, to_seconds( time_resub1 ) );
    fmt::print( "[i]     2-resub {:6d} = {:6d} 2AND    + {:6d} 2OR     ({:>5.2f} secs)\n",
                              num_div12_accepts, num_div12_2and_accepts, num_div12_2or_accepts, to_seconds( time_resub12 ) );
    fmt::print( "[i]     collect binate divisors                          ({:>5.2f} secs)\n", to_seconds( time_collect_binate_divisors ) );
    fmt::print( "[i]     2-resub {:6d} = {:6d} AND-OR  + {:6d} OR-AND  ({:>5.2f} secs)\n",
                              num_div2_accepts, num_div2_and_or_accepts, num_div2_or_and_accepts, to_seconds( time_resub2 ) );
    fmt::print( "[i]     3-resub {:6d} = {:6d} AND-2OR + {:6d} OR-2AND ({:>5.2f} secs)\n",
                              num_div3_accepts, num_div3_and_2or_accepts, num_div3_or_2and_accepts, to_seconds( time_resub3 ) );
    fmt::print( "[i] total   {:6d}\n",
                              (num_const_accepts + num_div0_accepts + num_div1_accepts + num_div12_accepts + num_div2_accepts + num_div3_accepts) );
    // clang-format on
  }
}; /* aig_enumerative_resyn_stats */

template<typename TT, bool normalized = false>
struct aig_enumerative_resyn
{
public:
  using stats = aig_enumerative_resyn_stats;
  using index_list_t = xag_index_list<false>;
  using truth_table_t = TT;

public:
  explicit aig_enumerative_resyn( stats& st ) noexcept
      : st( st )
  {}

  template<class iterator_type, class truth_table_storage_type>
  std::optional<index_list_t> operator()( TT const& target, TT const& care, iterator_type begin, iterator_type end, truth_table_storage_type const& tts, uint32_t max_size = std::numeric_limits<uint32_t>::max() )
  {
    (void)care;
    assert( is_const0( ~care ) && "enumerative resynthesis does not support don't cares" );

    index_list_t il( std::distance( begin, end ) );
    const TT ntarget = ~target;
    uint32_t i, j, k, l;
    iterator_type it = begin;

    /* C-resub */
    if ( kitty::is_const0( target ) )
    {
      il.add_output( 0 );
      return il;
    }
    if constexpr ( !normalized )
    {
      if ( kitty::is_const0( ntarget ) )
      {
        il.add_output( 1 );
        return il;
      }
    }

    /* 0-resub */
    for ( it = begin, i = 0u; it != end; ++it, ++i )
    {
      if ( target == tts[*it] )
      {
        il.add_output( make_lit( i ) );
        return il;
      }
      if constexpr ( !normalized )
      {
        if ( ntarget == tts[*it] )
        {
          assert( !normalized );
          il.add_output( make_lit( i, true ) );
          return il;
        }
      }
    }

    if ( max_size == 0 )
    {
      return std::nullopt;
    }

    /* collect unate literals */
    std::vector<uint32_t> pos_unate, neg_unate, binate;
    for ( it = begin, i = 0u; it != end; ++it, ++i )
    {
      if ( kitty::implies( tts[*it], target ) )
      {
        pos_unate.emplace_back( make_lit( i ) );
      }
      else if ( kitty::implies( target, tts[*it] ) )
      {
        neg_unate.emplace_back( make_lit( i ) );
      }
      else if ( kitty::implies( target, ~tts[*it] ) )
      {
        neg_unate.emplace_back( make_lit( i, true ) );
      }
      else
      {
        if constexpr ( !normalized )
        {
          if ( kitty::implies( ~tts[*it], target ) )
          {
            pos_unate.emplace_back( make_lit( i, true ) );
          }
          else
          {
            binate.emplace_back( make_lit( i ) );
          }
        }
        else
        {
          binate.emplace_back( make_lit( i ) );
        }
      }
    }

    /* 1-resub */
    for ( i = 0u; i < pos_unate.size(); ++i )
    {
      for ( j = i + 1; j < pos_unate.size(); ++j )
      {
        if ( target == ( get_tt_from_lit( pos_unate[i], tts, begin ) | get_tt_from_lit( pos_unate[j], tts, begin ) ) )
        {
          il.add_output( il.add_and( pos_unate[i] ^ 0x1, pos_unate[j] ^ 0x1 ) ^ 0x1 ); // OR
          return il;
        }
      }
    }
    for ( i = 0u; i < neg_unate.size(); ++i )
    {
      for ( j = i + 1; j < neg_unate.size(); ++j )
      {
        if ( target == ( get_tt_from_lit( neg_unate[i], tts, begin ) & get_tt_from_lit( neg_unate[j], tts, begin ) ) )
        {
          il.add_output( il.add_and( neg_unate[i], neg_unate[j] ) ); // AND
          return il;
        }
      }
    }

    if ( max_size == 1 )
    {
      return std::nullopt;
    }

    /* 2-resub */
    for ( i = 0u; i < pos_unate.size(); ++i )
    {
      for ( j = i + 1; j < pos_unate.size(); ++j )
      {
        for ( k = j + 1; k < pos_unate.size(); ++k )
        {
          if ( target == ( get_tt_from_lit( pos_unate[i], tts, begin ) | get_tt_from_lit( pos_unate[j], tts, begin ) | get_tt_from_lit( pos_unate[k], tts, begin ) ) )
          {
            il.add_output( il.add_and( il.add_and( pos_unate[i] ^ 0x1, pos_unate[j] ^ 0x1 ), pos_unate[k] ^ 0x1 ) ^ 0x1 ); // OR-OR
            return il;
          }
        }
      }
    }

    for ( i = 0u; i < neg_unate.size(); ++i )
    {
      for ( j = i + 1; j < neg_unate.size(); ++j )
      {
        for ( k = j + 1; k < neg_unate.size(); ++k )
        {
          if ( target == ( get_tt_from_lit( neg_unate[i], tts, begin ) & get_tt_from_lit( neg_unate[j], tts, begin ) & get_tt_from_lit( neg_unate[k], tts, begin ) ) )
          {
            il.add_output( il.add_and( il.add_and( neg_unate[i], neg_unate[j] ), neg_unate[k] ) ); // AND-AND
            return il;
          }
        }
      }
    }

    /* collect binate divisors */
    std::vector<std::pair<uint32_t, uint32_t>> neg_binates, pos_binates;

    for ( i = 0u; i < binate.size(); ++i )
    {
      if ( neg_binates.size() >= 500 && pos_binates.size() >= 500 )
      {
        break;
      }
      for ( j = i + 1; j < binate.size(); ++j )
      {
        auto const& tt_s0 = get_tt_from_lit( binate[i], tts, begin );
        auto const& tt_s1 = get_tt_from_lit( binate[j], tts, begin );
        if ( pos_binates.size() < 500 )
        {
          if ( kitty::implies( tt_s0 & tt_s1, target ) )
          {
            pos_binates.emplace_back( std::make_pair( binate[i], binate[j] ) );
          }
          if ( kitty::implies( ~tt_s0 & tt_s1, target ) )
          {
            pos_binates.emplace_back( std::make_pair( binate[i] ^ 0x1, binate[j] ) );
          }

          if ( kitty::implies( tt_s0 & ~tt_s1, target ) )
          {
            pos_binates.emplace_back( std::make_pair( binate[i], binate[j] ^ 0x1 ) );
          }

          if ( kitty::implies( ~tt_s0 & ~tt_s1, target ) )
          {
            pos_binates.emplace_back( std::make_pair( binate[i] ^ 0x1, binate[j] ^ 0x1 ) );
          }
        }
        if ( neg_binates.size() < 500 )
        {
          if ( kitty::implies( target, tt_s0 | tt_s1 ) )
          {
            neg_binates.emplace_back( std::make_pair( binate[i], binate[j] ) );
          }
          if ( kitty::implies( target, ~tt_s0 | tt_s1 ) )
          {
            neg_binates.emplace_back( std::make_pair( binate[i] ^ 0x1, binate[j] ) );
          }

          if ( kitty::implies( target, tt_s0 | ~tt_s1 ) )
          {
            neg_binates.emplace_back( std::make_pair( binate[i], binate[j] ^ 0x1 ) );
          }

          if ( kitty::implies( target, ~tt_s0 | ~tt_s1 ) )
          {
            neg_binates.emplace_back( std::make_pair( binate[i] ^ 0x1, binate[j] ^ 0x1 ) );
          }
        }
      }
    }
    for ( i = 0u; i < pos_binates.size(); ++i )
    {
      auto const& tt_binate = get_tt_from_lit( pos_binates[i].first, tts, begin ) & get_tt_from_lit( pos_binates[i].second, tts, begin );
      for ( j = 0u; j < pos_unate.size(); ++j )
      {
        if ( target == ( get_tt_from_lit( pos_unate[j], tts, begin ) | tt_binate ) )
        {
          il.add_output( il.add_and( il.add_and( pos_binates[i].first, pos_binates[i].second ) ^ 0x1, pos_unate[j] ^ 0x1 ) ^ 0x1 ); // AND-OR
          return il;
        }
      }
    }
    for ( i = 0u; i < neg_binates.size(); ++i )
    {
      auto const& tt_binate = get_tt_from_lit( neg_binates[i].first, tts, begin ) | get_tt_from_lit( neg_binates[i].second, tts, begin );
      for ( j = 0u; j < neg_unate.size(); ++j )
      {
        if ( target == ( get_tt_from_lit( neg_unate[j], tts, begin ) & tt_binate ) )
        {
          il.add_output( il.add_and( il.add_and( neg_binates[i].first ^ 0x1, neg_binates[i].second ^ 0x1 ) ^ 0x1, neg_unate[j] ) ); // OR-AND
          return il;
        }
      }
    }

    if ( max_size == 2 )
    {
      return std::nullopt;
    }

    /* 3-resub */
    for ( i = 0u; i < neg_binates.size(); ++i )
    {
      auto const& tt_binate = get_tt_from_lit( neg_binates[i].first, tts, begin ) | get_tt_from_lit( neg_binates[i].second, tts, begin );
      for ( j = i + 1; j < neg_binates.size(); ++j )
      {
        if ( target == ( ( get_tt_from_lit( neg_binates[j].first, tts, begin ) | get_tt_from_lit( neg_binates[j].second, tts, begin ) ) & tt_binate ) )
        {
          il.add_output( il.add_and( il.add_and( neg_binates[i].first ^ 0x1, neg_binates[i].second ^ 0x1 ) ^ 0x1, il.add_and( neg_binates[j].first ^ 0x1, neg_binates[j].second ^ 0x1 ) ^ 0x1 ) ); // AND-2OR
          return il;
        }
      }
    }
    for ( i = 0u; i < pos_binates.size(); ++i )
    {
      auto const& tt_binate = get_tt_from_lit( pos_binates[i].first, tts, begin ) & get_tt_from_lit( pos_binates[i].second, tts, begin );
      for ( j = i + 1; j < pos_binates.size(); ++j )
      {
        if ( target == ( ( get_tt_from_lit( pos_binates[j].first, tts, begin ) & get_tt_from_lit( pos_binates[j].second, tts, begin ) ) | tt_binate ) )
        {
          il.add_output( il.add_and( il.add_and( pos_binates[i].first, pos_binates[i].second ) ^ 0x1, il.add_and( pos_binates[j].first, pos_binates[j].second ) ^ 0x1 ) ^ 0x1 ); // OR-2AND
          return il;
        }
      }
    }

    for ( i = 0u; i < pos_unate.size(); ++i )
    {
      for ( j = i + 1; j < pos_unate.size(); ++j )
      {
        for ( k = j + 1; k < pos_unate.size(); ++k )
        {
          for ( l = k + 1; l < pos_unate.size(); ++l )
          {
            if ( target == ( get_tt_from_lit( pos_unate[i], tts, begin ) | get_tt_from_lit( pos_unate[j], tts, begin ) | get_tt_from_lit( pos_unate[k], tts, begin ) | get_tt_from_lit( pos_unate[l], tts, begin ) ) )
            {
              il.add_output( il.add_and( il.add_and( pos_unate[i] ^ 0x1, pos_unate[j] ^ 0x1 ), il.add_and( pos_unate[k] ^ 0x1, pos_unate[l] ^ 0x1 ) ) ^ 0x1 ); // OR-2OR
              return il;
            }
          }
        }
      }
    }

    for ( i = 0u; i < neg_unate.size(); ++i )
    {
      for ( j = i + 1; j < neg_unate.size(); ++j )
      {
        for ( k = j + 1; k < neg_unate.size(); ++k )
        {
          for ( l = k + 1; l < neg_unate.size(); ++l )
          {
            if ( target == ( get_tt_from_lit( neg_unate[i], tts, begin ) & get_tt_from_lit( neg_unate[j], tts, begin ) & get_tt_from_lit( neg_unate[k], tts, begin ) & get_tt_from_lit( neg_unate[l], tts, begin ) ) )
            {
              il.add_output( il.add_and( il.add_and( neg_unate[i], neg_unate[j] ), il.add_and( neg_unate[k], neg_unate[l] ) ) ); // AND-2AND
              return il;
            }
          }
        }
      }
    }

    if ( max_size == 3 )
    {
      return std::nullopt;
    }

    return std::nullopt;
  }

private:
  uint32_t make_lit( uint32_t const& var, bool const& inv = false )
  {
    return ( var + 1 ) * 2 + (uint32_t)inv;
  }

  template<class truth_table_storage_type, class iterator_type>
  TT get_tt_from_lit( uint32_t const& lit, truth_table_storage_type const& tts, iterator_type const& begin )
  {
    return ( lit % 2 ) ? ~tts[*( begin + ( lit / 2 ) - 1 )] : tts[*( begin + ( lit / 2 ) - 1 )];
  }

private:
  stats& st;
}; /* aig_enumerative_resyn */

} // namespace mockturtle