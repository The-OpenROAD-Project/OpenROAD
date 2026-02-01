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
  \file xmg_npn.hpp
  \brief Replace with size-optimum XMGs from NPN

  \author Heinz Riener
  \author Mathias Soeken
  \author Siang-Yun (Sonia) Lee
  \author Zhufei Chu
*/

#pragma once

#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_map>
#include <vector>

#include <kitty/dynamic_truth_table.hpp>
#include <kitty/npn.hpp>
#include <kitty/print.hpp>

#include "../../algorithms/cleanup.hpp"
#include "../../io/write_bench.hpp"
#include "../../networks/xmg.hpp"
#include "../../traits.hpp"
#include "../../views/topo_view.hpp"

namespace mockturtle
{

/*! \brief Resynthesis function based on pre-computed size-optimum XMGs.
 *
 * This resynthesis function can be passed to ``node_resynthesis``,
 * ``cut_rewriting``, and ``refactoring``.  It will produce an XMG based on
 * pre-computed size-optimum XMGs with up to at most 4 variables.
 * Consequently, the nodes' fan-in sizes in the input network must not exceed
 * 4.
 *
   \verbatim embed:rst

   Example

   .. code-block:: c++

      const klut_network klut = ...;
      xmg_npn_resynthesis resyn;
      const auto xmg = node_resynthesis<xmg_network>( klut, resyn );
   \endverbatim
 */
class xmg_npn_resynthesis
{
public:
  /*! \brief Default constructor.
   *
   */
  xmg_npn_resynthesis()
  {
    build_db();
  }

  template<typename LeavesIterator, typename Fn>
  void operator()( xmg_network& xmg, kitty::dynamic_truth_table const& function, LeavesIterator begin, LeavesIterator end, Fn&& fn ) const
  {
    assert( function.num_vars() <= 4 );
    const auto fe = kitty::extend_to( function, 4 );
    const auto config = kitty::exact_npn_canonization( fe );

    auto func_str = "0x" + kitty::to_hex( std::get<0>( config ) );
    const auto it = class2signal.find( func_str );
    assert( it != class2signal.end() );

    // const auto it = class2signal.find( static_cast<uint16_t>( std::get<0>( config )._bits[0] ) );

    std::vector<xmg_network::signal> pis( 4, xmg.get_constant( false ) );
    std::copy( begin, end, pis.begin() );

    std::vector<xmg_network::signal> pis_perm( 4 );
    auto perm = std::get<2>( config );
    for ( auto i = 0; i < 4; ++i )
    {
      pis_perm[i] = pis[perm[i]];
    }

    const auto& phase = std::get<1>( config );
    for ( auto i = 0; i < 4; ++i )
    {
      if ( ( phase >> perm[i] ) & 1 )
      {
        pis_perm[i] = !pis_perm[i];
      }
    }

    for ( auto const& po : it->second )
    {
      topo_view topo{ db, po };
      auto f = cleanup_dangling( topo, xmg, pis_perm.begin(), pis_perm.end() ).front();

      if ( !fn( ( ( phase >> 4 ) & 1 ) ? !f : f ) )
      {
        return; /* quit */
      }
    }
  }

private:
  std::unordered_map<std::string, std::string> opt_xmgs;

  inline std::vector<std::string> split( const std::string& str, const std::string& sep )
  {
    std::vector<std::string> result;

    size_t last = 0;
    size_t next = 0;
    while ( ( next = str.find( sep, last ) ) != std::string::npos )
    {
      result.push_back( str.substr( last, next - last ) );
      last = next + 1;
    }
    result.push_back( str.substr( last ) );

    return result;
  }

  void load_optimal_xmgs( const unsigned& strategy )
  {
    std::vector<std::string> result;

    switch ( strategy )
    {
    case 1:
      result = split( npn4_s, "\n" );
      break;

    case 2:
      result = split( npn4_sd, "\n" );
      break;

    case 3:
      result = split( npn4_ds, "\n" );
      break;

    default:
      break;
    }

    for ( auto record : result )
    {
      auto p = split( record, " " );
      assert( p.size() == 2u );
      opt_xmgs.insert( std::make_pair( p[0], p[1] ) );
    }
  }

  std::vector<xmg_network::signal> create_xmg_from_str( const std::string& str, const std::vector<xmg_network::signal>& signals )
  {
    auto sig = signals;
    std::vector<xmg_network::signal> result;

    std::stack<int> polar;
    std::stack<xmg_network::signal> inputs;

    for ( auto i = 0ul; i < str.size(); i++ )
    {
      // operators polarity
      if ( str[i] == '[' || str[i] == '<' )
      {
        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }

      // input signals
      if ( str[i] >= 'a' && str[i] <= 'd' )
      {
        inputs.push( sig[str[i] - 'a' + 1] );

        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }
      else if ( str[i] == '0' )
      {
        inputs.push( sig[0] );

        polar.push( i > 0 && str[i - 1] == '!' ? 1 : 0 );
      }

      // create signals
      if ( str[i] == '>' )
      {
        assert( inputs.size() >= 3u );
        auto x1 = inputs.top();
        inputs.pop();
        auto x2 = inputs.top();
        inputs.pop();
        auto x3 = inputs.top();
        inputs.pop();

        assert( polar.size() >= 4u );
        auto p1 = polar.top();
        polar.pop();
        auto p2 = polar.top();
        polar.pop();
        auto p3 = polar.top();
        polar.pop();

        auto p4 = polar.top();
        polar.pop();

        inputs.push( db.create_maj( x1 ^ p1, x2 ^ p2, x3 ^ p3 ) ^ p4 );
        polar.push( 0 );
      }

      if ( str[i] == ']' )
      {
        assert( inputs.size() >= 2u );
        auto x1 = inputs.top();
        inputs.pop();
        auto x2 = inputs.top();
        inputs.pop();

        assert( polar.size() >= 3u );
        auto p1 = polar.top();
        polar.pop();
        auto p2 = polar.top();
        polar.pop();

        auto p3 = polar.top();
        polar.pop();

        inputs.push( db.create_xor( x1 ^ p1, x2 ^ p2 ) ^ p3 );
        polar.push( 0 );
      }
    }

    assert( !polar.empty() );
    auto po = polar.top();
    polar.pop();
    db.create_po( inputs.top() ^ po );
    result.push_back( inputs.top() ^ po );
    return result;
  }

  void build_db()
  {
    std::vector<xmg_network::signal> signals;
    signals.push_back( db.get_constant( false ) );

    for ( auto i = 0u; i < 4; ++i )
    {
      signals.push_back( db.create_pi() );
    }

    load_optimal_xmgs( 1 ); // size optimization

    for ( const auto& e : opt_xmgs )
    {
      class2signal.insert( std::make_pair( e.first, create_xmg_from_str( e.second, signals ) ) );
    }
  }

  xmg_network db;
  std::unordered_map<std::string, std::vector<xmg_network::signal>> class2signal;

  std::string npn4_s = "0x3cc3 [d[!bc]]\n0x1bd8 [!<ac!d>!<a!bd>]\n0x19e3 [[!<!0a!c><!bc<!0a!c>>]<ad!<!0a!c>>]\n0x19e1 [<ac[!bc]><d[!bc]<a!b!<ac[!bc]>>>]\n0x17e8 [d<abc>]\n0x179a <[!a<!0b!c>][ad]!<abc>>\n0x178e [!d<!a!b[cd]>]\n0x16e9 [![!d<!0ab>]!<!0c<abc>>]\n0x16bc [[!c<0ad>]<a!b<!0!ac>>]\n0x16ad <!d<ad!<0!b!c>>!<ac!<0!bd>>>\n0x16ac [<acd>!<!b!d<!a[!bc]<acd>>>]\n0x16a9 <!<!0!bc>!<d!<!0!bc>![ac]><d<!b!d<!0!bc>>![ac]>>\n0x169e <[bc][a[bc]]<!c!d[a[bc]]>>\n0x169b [<c[!ab]<!b!cd>><a!cd>]\n0x169a <![!bc]<0a[!bc]>[a<cd![!bc]>]>\n0x1699 <<!a!b<0!cd>><ad!<0!cd>>!<!bd<ad!<0!cd>>>>\n0x1687 [!a!<b<!0!b<a!bd>>![c<a!bd>]>]\n0x167e <b!<bd[!ac]>[<bd[!ac]><ac![!ac]>]>\n0x166e [<cd<0ab>>!<!a!b<0ab>>]\n0x03cf <!c!d![bc]>\n0x166b <!c[a<bcd>]<!bc!d>>\n0x01be <!d!<!0b!<!bcd>><b!c![a<!0b!<!bcd>>]>>\n0x07f2 <!d![!cd]<!bd[ab]>>\n0x07e9 [<bc<!0a!b>><!ad<!0a!b>>]\n0x6996 [b[a[cd]]]\n0x01af <a!<0d<abd>>!<!0ac>>\n0x033c <!<!0bd>[c<!0bd>]![!bd]>\n0x07e3 <!c[!c<ab!d>]<!b!d[!c<ab!d>]>>\n0x1681 <<!b!cd>!<!0!a<!b!cd>>![[!ab]<!bc!d>]>\n0x01ae [!d<0<!a!bc>!<acd>>]\n0x07e2 [<!a!bd>!<!cd<0b!<!a!bd>>>]\n0x01ad [<!0c!<a!bd>><!acd>]\n0x07e1 [!<!a!bd>!<0c!<!cd!<!a!bd>>>]\n0x001f <!d<0!a!b><0!c!<0!a!b>>>\n0x01ac [<!0bc><d<!acd><!0bc>>]\n0x07e0 [d<cd<ab!d>>]\n0x07bc [<!0d!<!0a!b>><bc<!0a!b>>]\n0x03dc <b[!d<0!b!c>]!<ab!<0!b!c>>>\n0x06f6 <!d[cd][ab]>\n0x06bd [<c<a!bd>!<a!b!d>><!0d!<a!bd>>]\n0x077e <!<bcd><b![!cd]!<bcd>>[a<bcd>]>\n0x06b9 <!<ad<0!b!c>><a!b!d><0!cd>>\n0x06b7 <!d!<bc![!ac]>![b[!ac]]>\n0x06b6 <!<!0a!b><a!bc>!<0c!<0!d<a!bc>>>>\n0x077a [<bcd>!<0!a<b!c!d>>]\n0x06b5 [<a!d[ac]>!<cd<!b![ac]<a!d[ac]>>>]\n0x0ff0 [cd]\n0x0779 [<cd<ab!c>><!0!<ab!c><abc>>]\n0x06b4 [c<d![!ab]<!abc>>]\n0x0778 <!b<b!d!<!0a!b>>[c<bd<!0a!b>>]>\n0x007f <!d<0!ab>!<bc<0!ab>>>\n0x06b3 <!a<a!b!d><[cd][!bd]<a!b!d>>>\n0x007e <0!d![a<a!b!c>]>\n0x06b2 <a<!a!d[ab]>[d<cd![ab]>]>\n0x0776 <[ab]!<abd>[cd]>\n0x06b1 [!d!<c<!0!bd>![ab]>]\n0x06b0 [d<c[!ab]<ad<!0cd>>>]\n0x037c <d[d<!0bc>]!<ad<bcd>>>\n0x01ef <!c!d[d<!0ab>]>\n0x0696 <b<a!b!<!0!cd>>!<abc>>\n0x035e [!<0!a!d>!<0!c!<bd<0!a!d>>>]\n0x0678 <!<!abd>[c<abd>]!<!0a!b>>\n0x0676 <!<!0!cd>!<abc>[ab]>\n0x0669 [!d!<b<a!b<!0!cd>>!<ab!c>>]\n0x01bc [<0!b!c><!c!d<a!b!d>>]\n0x0663 [!<!acd><0b<b!c!d>>]\n0x07f0 <0[cd]<!b!<0ad>[cd]>>\n0x01bf <!b!d!<!bc![ad]>>\n0x0666 <0[ab]!<0cd>>\n0x0661 [<ab!c><b!d<!0a<a!b!c>>>]\n0x1689 [!<0d![ab]><c[ab]!<abd>>]\n0x0660 <0![!cd][ab]>\n0x0182 [!<!ab!c><!bd!<!0!c<!ab!c>>>]\n0x07b6 [c<d!<0!c[!ab]>!<ac[!ab]>>]\n0x03d7 <!d![bc]!<abc>>\n0x06f1 <c<a!c<!ab!d>><!b<!ab!d>![d<a!c<!ab!d>>]>>\n0x03dd <!<a!bd>!<!0bc>![!d<!0bc>]>\n0x0180 [!<!bc!d>!<!a!b<!bc!d>>]\n0x07b4 [<a!d![bd]>!<0c!<0d<a!d![bd]>>>]\n0x03db <!d<0!b!c>[a<!a!bc>]>\n0x07b0 <!<!0a!d>[cd]<a!b!d>>\n0x03d4 [d<bc<!0!ad>>]\n0x03c7 <[!bc]<0!a!c>!<0bd>>\n0x03c6 <!d!<!0c!d>[b<a!c!<!0c!d>>]>\n0x03c5 [!<bcd>!<!ad!<0c!d>>]\n0x03c3 <!c!<b!cd>![bc]>\n0x03c1 <!a[b<b!cd>]!<c<b!cd>![b<b!cd>]>>\n0x03c0 [d<bcd>]\n0x1798 [<!a!b<ab!c>><0!d<!c!d<ab!c>>>]\n0x036f <!c!d[b<!0ad>]>\n0x03fc [d!<0!b!c>]\n0x1698 <!<abc><0!d<abd>><!0c<abd>>>\n0x177e [!<0!a!c><bd![ac]>]\n0x066f <!c!d[ab]>\n0x035b <!<acd>!<0b!c><0a!c>>\n0x1697 [<!ab<!acd>>[!bc]]\n0x066b <a<!a!b[cd]>!<cd[b<!a!b[cd]>]>>\n0x07f8 [d<ac!<0a!b>>]\n0x035a <!d<0!bd>[c<!0ad>]>\n0x1696 <0<!0!b<b!c!d>>![a[!bc]]>\n0x003f <0!d!<bcd>>\n0x0673 <!d![!b<a!c!d>]!<bc<a!c!d>>>\n0x0359 [!<0b!c><ac![!ad]>]\n0x0672 <<0!b!d>[ab][cd]>\n0x0358 [!<b!d!<0a!d>>!<d<b!d!<0a!d>>![c<0a!d>]>]\n0x003d <0<!b!c<bc!d>><!a!d<bc!d>>>\n0x0357 <!a<!0a!d>!<!0bc>>\n0x003c <0!d[bc]>\n0x0356 [<0!a!d><0!b!c>]\n0x07e6 [<ab!d><!cd<0ab>>]\n0x033f <!b!c!d>\n0x033d [<0!d<a!cd>><!b!c<0!d!<a!cd>>>]\n0x0690 [c<cd![!ab]>]\n0x01e9 [!<bc<!0a!c>><a!<!0a!c>!<0d<bc<!0a!c>>>>]\n0x1686 [!a<!b[!bc]!<ad![!bc]>>]\n0x07f1 <!b[cd]!<a!bd>>\n0x01bd <!d[b<!a!bc>]<0!c<!a!bc>>>\n0x1683 [[!bc]<!ad<0bc>>]\n0x001e <0!d[c<!0ab>]>\n0x01ab <!d!<!0bc>[ad]>\n0x01aa <a![!ad]!<!0a<!0bc>>>\n0x01a9 <!d![a<!0bc>]<0!ad>>\n0x001b <0!<!abd>!<acd>>\n0x01a8 [<0!b!c><a!d<0!b!c>>]\n0x000f <0!c!d>\n0x0199 <!<bcd>[!ab]<0!d[!ab]>>\n0x0198 [b!<a!b[d<!0b!c>]>]\n0x0197 <!d<0!b!c>[!a<0bc>]>\n0x06f9 [d<!0c![ab]>]\n0x0196 <!b!<!0d<a!b!c>>!<!bc![ad]>>\n0x03d8 [d!<!b!c<0!d![!ab]>>]\n0x06f2 <[cd]<!d![!ab][cd]><0a![cd]>>\n0x03de <!d![!bd][!c<!ab[!bd]>]>\n0x018f <!c!d![a<!ab!d>]>\n0x0691 <<!a!bd><ab!c><!d!<ab!c><0c<ab!c>>>>\n0x01ea [!d!<!0a<bcd>>]\n0x07b1 <[cd]<0!a!c>!<!abd>>\n0x0189 <<a!b!c><0b!d>[a<a!b!c>]>\n0x036b <!c[b<!0a!c>]<a!d!<!0a!c>>>\n0x03d5 <!a[b<b!cd>]<!0a!d>>\n0x0186 [<!a!bc><c!d!<0ab>>]\n0x0119 <0[!ab]!<acd>>\n0x0368 [![c<0a!d>]!<!bc!<b!d![c<0a!d>]>>]\n0x0183 <[!bc]<0a!d><0!a!b>>\n0x07b5 <!<bcd>![ac][cd]>\n0x0181 [<b!c<!0!bd>><ab<b!c<!0!bd>>>]\n0x036e <b[b<!0ad>]!<bcd>>\n0x011f <!c!d!<!0ab>>\n0x016f <b<0!b!<!abc>>!<ad<!abc>>>\n0x016e <!d[a[bd]]<0!c[bd]>>\n0x013f <!a<0a!d>!<bcd>>\n0x019f <0<!c!d[!ab]>!<0bd>>\n0x013e [[!bd]!<!bc<a!d![!bd]>>]\n0x019e [d<a!<a!b!c><!0!c<!a!bd>>>]\n0x013d <b!<bcd><0<!0!ac>!<bcd>>>\n0x016b <!d![!a[!bc]]!<bc![!bc]>>\n0x01fe [d!<0!a!<!0bc>>]\n0x166a [<bcd><ab<d!<bcd>!<0b!c>>>]\n0x0019 [<!ab!d><0b<!ac<!ab!d>>>]\n0x006b <!d<a!b!c><0!a<0bc>>>\n0x069f <a<!ab!d>!<abc>>\n0x013c <0!<bcd><b!<0ad>!<!c!d<0ad>>>>\n0x037e [<!0ad><bc<!0!ad>>]\n0x012f <!d<a!b!c><0!a!c>>\n0x0693 [c!<!d<b!cd>![!b<!ac<b!cd>>]>]\n0x012d <0!<bcd><!0b[!ac]>>\n0x01ee <!c<ac!d>[!d<0!a!b>]>\n0x012c [!<0!b!c><cd<!0!ab>>]\n0x017f <!a!d!<bcd>>\n0x1796 <![!ad]<!b!c[!ad]>![!d<bc[!ad]>]>\n0x036d <<!ab!c><ac[bd]>!<bcd>>\n0x011e <d<0!d!<0!c<0!a!b>>>!<c!<0!a!b>!<0!d!<0!c<0!a!b>>>>>\n0x0679 <a<!a!b[cd]>!<ad<!0!bc>>>\n0x035f <!c!d!<ab[bc]>>\n0x067b <!<cd!<!acd>>![b<!acd>]!<!abd>>\n0x0118 <0[!d[a<!ab!c>]]!<b!<!ab!c>[a<!ab!c>]>>\n0x067a [<!0ac><cd<ab!<!0ac>>>]\n0x0117 <0!<abd>!<cd<ab!d>>>\n0x1ee1 [!d![!c<!0ab>]]\n0x016a <d<!d!<!0!ad><bcd>>!<a<bcd>!<!d!<!0!ad><bcd>>>>\n0x0169 <!c<!a!b<abc>><0!d<abc>>>\n0x037d <!a![d<0!b!c>]<a!b!c>>\n0x0697 <<0ac>!<abc>!<d<abc>!<abd>>>\n0x006f <!d<0!c!d>![!ab]>\n0x17ac [<abc><!ad<!0bd>>]\n0x0069 <0!d![a[bc]]>\n0x0667 <0!<0ab>!<cd!<!0ab>>>\n0x0168 <!b!<!0d!<abc>><!ab!<!bc!d>>>\n0x067e <!d[ab][!c<0!b!d>]>\n0x011b <a!<!0ab>!<acd>>\n0x036a [<!0ad><bcd>]\n0x018b <!c!<a!c[bc]><0a!d>>\n0x0001 <0<0!b!d><!ab!c>>\n0x1669 <<!a!c[bd]><ac[bd]>!<[bd]!<!a!c[bd]><bd<!a!c[bd]>>>>\n0x0018 <0!d[a<a!bc>]>\n0x168b [![d<!0bc>]<ab!<!c!d<!0bc>>>]\n0x0662 [<ab<!0!bc>><c!<!0!bc><abd>>]\n0x00ff !d\n0x168e [<!0a!<!0!bc>><c!<!0!bc><a!bd>>]\n0x0007 <0<0!c!d>!<abd>>\n0x19e6 [[ad]<b!c!<!0a!b>>]\n0x0116 <0!<abd>![!c<d[ab]!<abd>>]>\n0x036c [[bd]<0c!<!ab![bd]>>]\n0x01e8 <<!a!bc><a!c[cd]><0b!d>>\n0x06f0 <<0!ab><a!bc>![!cd]>\n0x03d6 [[c<!0!bc>]!<!0d!<!ab<!0!bc>>>]\n0x1be4 [!d!<bc[ab]>]\n0x0187 <a<!a!d[!bc]><0!c<!a!d[!bc]>>>\n0x0003 <0!d<0!b!c>>\n0x017e [[!ad]<a!b<!c[!ad]<!0bd>>>]\n0x01eb <!d[!b<!0ac>]<a!b!<!0ac>>>\n0x0006 <0!d<!cd![!ab]>>\n0x011a [a<a!<b!c!d>[cd]>]\n0x0369 <0[!c<0!d![!ab]>]!<0bd>>\n0x03d9 [!b<b!<!ab[!ad]><!bc!d>>]\n0x0000 0\n0x019b <!d!<!0bc>![ab]>\n0x1668 [<acd><ab<bcd>>]\n0x18e7 [d[!b<!abc>]]\n0x0017 <0!d!<abc>>\n0x019a <!d[a<!bcd>]<0!c<!bcd>>>\n0x0016 <0!<ab<!a!bc>>!<!cd<!a!bc>>>";
  std::string npn4_sd = "0x3cc3 [b[!cd]]\n0x1bd8 [!<b!cd><a!b!c>]\n0x19e3 [<a!b!c><c[bd]<0a!c>>]\n0x19e1 [<!b!c[bd]><a[bd]!<cd<!b!c[bd]>>>]\n0x17e8 [d<abc>]\n0x179a [<ad!<0ad>><c<0ad>[!bd]>]\n0x178e <!c[ad]![!bd]>\n0x16e9 [d<<a!b!c><abc>!<0a!b>>]\n0x16bc [<0!b!c><!d!<abc>[ad]>]\n0x16ad [!<!b!<!0a!b>[ac]><0d<!0a!b>>]\n0x16ac [!<acd><!b!d<!a[cd]<acd>>>]\n0x16a9 <0<!0a<!b!cd>>[!<0!b!c>[!ad]]>\n0x169e <!d[!a[!bc]]!<c!d[!bc]>>\n0x169b <<a!b!<0!cd>>!<ab!<0!cd>>!<!bcd>>\n0x169a [a<![!bc]<0ac><!0!bd>>]\n0x1699 <b<!a!b<0!cd>>!<b<0!cd><!abd>>>\n0x1687 [[ac]<b!<a!bd>!<0bc>>]\n0x167e <<!b!c<!abc>>[a<!abc>]!<!ad!<!abc>>>\n0x166e [<ab!<0ab>><cd<0ab>>]\n0x03cf <!c!d[bd]>\n0x166b <!c[a<bcd>]<!bc!d>>\n0x01be <c!<bcd><!<!0!bd>[ad]!<bcd>>>\n0x07f2 <[cd]<0a!b>!<0ad>>\n0x07e9 [<ac<!0!ab>><!bd<!0!ab>>]\n0x6996 [[bc][ad]]\n0x01af <[!ac]<0a!d>!<bcd>>\n0x033c <<b!cd><0c!d>!<bcd>>\n0x07e3 [c<d!<abc>!<0b!c>>]\n0x1681 <0[!<b!cd>[ab]]!<!d[ab]<0!ac>>>\n0x01ae <!<bcd>![!ad]<0b!d>>\n0x07e2 [<!a!bd>!<!cd<0b!<!a!bd>>>]\n0x01ad <!<bcd><0b!d>[!ac]>\n0x07e1 [c<d<!0!bc>!<ab!d>>]\n0x001f <!a<a!b!d><0!c!d>>\n0x01ac [d<a<!a!cd>!<0!b!c>>]\n0x07e0 [d<ac<!abd>>]\n0x07bc [<bc<!0a!b>>!<0!d<!0a!b>>]\n0x03dc <b[d<!0bc>]!<ab<!0bc>>>\n0x06f6 <!d[ab][cd]>\n0x06bd <<b!c<0!bd>><a!b!d>!<a!c<0!bd>>>\n0x077e <<!0a![!cd]><0!ab>!<bcd>>\n0x06b9 <!<!0ac><a!b!d><b<a!b!d>![!cd]>>\n0x06b7 <!d[c[ab]]<!b!c[ab]>>\n0x06b6 [!c!<<!abc>!<0c!d>[ab]>]\n0x077a [!<0a!<!bcd>>![cd]]\n0x06b5 <!<a!b!c><a!b!d>![!c<!0!ad>]>\n0x0ff0 [cd]\n0x0779 <<ac!d><b!c!<ac!d>>!<ab![cd]>>\n0x06b4 [!c<!d<a!b!c>![ab]>]\n0x0778 [[cd]<0<b!c!d><0ab>>]\n0x007f <!d<!ab!c><0!b!d>>\n0x06b3 [![!bd]!<d<0bc><a!c!d>>]\n0x007e <0!d![a<a!b!c>]>\n0x06b2 [!<0!c!<a!bd>><bd!<ab!c>>]\n0x0776 <[ab][cd]!<abc>>\n0x06b1 [d<c<!0!bd>[!ab]>]\n0x06b0 [<!a!c<ab!d>><!b<ab!d>!<0cd>>]\n0x037c [[!bd]<d<!0b!c>!<acd>>]\n0x01ef <!c!d![!c<0!a!b>]>\n0x0696 <<ab!c><0c!d>!<abc>>\n0x035e [<!0ad>!<0!c!<bd!<!0ad>>>]\n0x0678 <<!ab!d><0a!b>[c<abd>]>\n0x0676 <!<abc>[ab]<0c!d>>\n0x0669 [<0!c!d><[cd]<0!c!d>[ab]>]\n0x01bc [<bd<!acd>>!<0!b!c>]\n0x0663 <0[b<a!c!d>]!<0cd>>\n0x07f0 <[cd]!<!0!cd>!<0ab>>\n0x01bf <!d<!0a!c><0!a!b>>\n0x0666 <0[ab]!<0cd>>\n0x0661 <<a!c!d>!<ab<a!c!d>>[<a!c!d><b!c!d>]>\n0x1689 [[d<abd>]<0!c<!a!bd>>]\n0x0660 [<b!c!d>!<!acd>]\n0x0182 [<!ab!c><b!d<!0!c<!ab!c>>>]\n0x07b6 [<!0!a<!bcd>>[!c<!0bd>]]\n0x03d7 <!d[!bc]!<abc>>\n0x06f1 [<!ac<!0!bc>><d<!0!bc><0!ad>>]\n0x03dd <[bd]<0!a!d>!<0cd>>\n0x0180 [<acd>!<b!d!<acd>>]\n0x07b4 [<bd<!0!ad>><c<!0!ad><abc>>]\n0x03db <[!bc]<a!b!d>!<acd>>\n0x07b0 <<0ac>[cd]!<abc>>\n0x03d4 [!d<!b!c<0a!d>>]\n0x03c7 <!<0bd><0!a!c>[!bc]>\n0x03c6 <0!<0cd>[!b<!ac!d>]>\n0x03c5 <[bd]<0!a!c>![bc]>\n0x03c3 <!b<0b!d>[!bc]>\n0x03c1 <!<b!cd>[b<b!cd>]<!a!c<b!cd>>>\n0x03c0 [d<bcd>]\n0x1798 [<ab<!a!bc>><d<!a!bc><!0cd>>]\n0x036f <!c!d[!b<0!ac>]>\n0x03fc [d!<0!b!c>]\n0x1698 [b<<!abd>[ac][!ad]>]\n0x177e [<0!b!d>!<ac![bd]>]\n0x066f <!c!d[ab]>\n0x035b <<ac!d><0!b!c>!<0ac>>\n0x1697 <!<abc><ab!c><!bc!d>>\n0x066b [<bc<!a!bd>><a<!a!bd>!<0!cd>>]\n0x07f8 [!d!<ac!<0a!b>>]\n0x035a [!<!0c<0bd>><0!a!d>]\n0x1696 <!a<!d<0!bd><a!bc>><b!c<a!bc>>>\n0x003f <0!d!<bcd>>\n0x0673 <!b![d<ab!c>]<!a!d<ab!c>>>\n0x0359 [<!0!bc><ac[!cd]>]\n0x0672 <[cd]<0!b!d>[ab]>\n0x0358 <<!acd><0a!c>[!d<0!b!c>]>\n0x003d <!<bcd>[bc]<0!a!d>>\n0x0357 <!0!<!0bc><0!a!d>>\n0x003c <0!d![!bc]>\n0x0356 [!<0!a!d>!<0!b!c>]\n0x07e6 [<!cd!<0!a!b>><abc>]\n0x033f <!b!c!d>\n0x033d <!<bcd><bc!d><0!b<!0!ad>>>\n0x0690 [!d<!c!d[ab]>]\n0x01e9 <!d<0!b![ac]>![b<!0ac>]>\n0x1686 [[bc]<a[bc]!<!abd>>]\n0x07f1 <!b<!ab!d>[cd]>\n0x01bd <!d[c<!ab!c>]<0!b<!ab!c>>>\n0x1683 [[bc]!<!ad<0bc>>]\n0x001e <0!d![!c<ab!d>]>\n0x01ab <!d![!ad]<0!b!c>>\n0x01aa <[ad]<0!b!c><0a!d>>\n0x01a9 [a<d<0!b!c><!0a!d>>]\n0x001b <0<a!b!d>!<acd>>\n0x01a8 [a<ad!<!0bc>>]\n0x000f <0!c!d>\n0x0199 <[!ab]<a!b!d><0!a!c>>\n0x0198 [!<!abd><!b!c<0ac>>]\n0x0197 <!d[!a<0bc>]<!b!c<0bc>>>\n0x06f9 [d<!0c![ab]>]\n0x0196 <<0!d!<a!b!c>>[ad]!<bc[ad]>>\n0x03d8 [<!cd!<a!b!c>><!0b!<a!b!c>>]\n0x06f2 <a<0!a[cd]><!d[cd]![!ab]>>\n0x03de <!d![c<!ab!d>][bd]>\n0x018f <!c!d[b<!ab!c>]>\n0x0691 <0<!d<!a!bd><ab!c>><!0c<!a!bd>>>\n0x01ea [d<!0a<bcd>>]\n0x07b1 <<a!b!d><0!a!c>[cd]>\n0x0189 <[!ab]!<!0ac><0a!d>>\n0x036b <!a!<!ad![!bc]>!<!abc>>\n0x03d5 [<bcd>!<!d<bcd><0a!d>>]\n0x0186 [<!cd<0ab>>!<!a!bc>]\n0x0119 <0[!ab]!<acd>>\n0x0368 [<bcd><c<b!cd><!0ad>>]\n0x0183 <[!bc]<0a!d><0!a!b>>\n0x07b5 <!<bcd>[cd]![ac]>\n0x0181 <<!ab!c>[b<!ab!c>]<0!d!<!ab!c>>>\n0x036e <!c<bc!d>[!b<0!a!d>]>\n0x011f <!c!d!<!0ab>>\n0x016f <<!ab!c><0a!d>!<abd>>\n0x016e <!d<0!c[bd]>[a[bd]]>\n0x013f <0!<0ad>!<bcd>>\n0x019f <!d<b!c[ad]><0!a!b>>\n0x013e [[!cd]<!bc!<a!d![!cd]>>]\n0x019e <!d!<!bc[!ad]><0!b<!acd>>>\n0x013d <<bc!d>!<!0ac>!<bcd>>\n0x016b <!d<0!b!c>![!b[!ac]]>\n0x01fe [d<!0c<ab!c>>]\n0x166a <a[a<bcd>]!<ab<!bcd>>>\n0x0019 <!<!abd><0!a!b><0b!c>>\n0x006b <!d<0b<!a!bc>>!<!abc>>\n0x069f <!d<abd>!<abc>>\n0x013c <<0!ad><b!d!<!0b!c>>!<bc<0!ad>>>\n0x037e [<!0ad>!<!b!c<0a!d>>]\n0x012f <!d<a!b!c>!<!0ac>>\n0x0693 [<acd><d<!0c!d>![bd]>]\n0x012d <<0ac><!ab!c>!<bcd>>\n0x01ee [!<!0ab>!<d<0!cd><!0ab>>]\n0x012c [<0!a!b><!d[bc]<0!a!b>>]\n0x017f <!b!d!<acd>>\n0x1796 <!b!<!a!bc><!a<!0cd>!<!0!bd>>>\n0x036d <<abc>!<bcd><!a!c[bd]>>\n0x011e <d!<cd!<0!a!b>>!<d<0!a!b>!<0c!d>>>\n0x0679 <!d<!a<abd><0!b!c>>[c<abd>]>\n0x035f <!c!<0bd>!<!0ad>>\n0x067b <!d<!b!c[ad]>[![ad]<0b!c>]>\n0x0118 <<0!a!b><ac!d><!cd<0b!d>>>\n0x067a [<!0ac><cd<ab!<!0ac>>>]\n0x0117 <!b<!a!d<a!cd>><0!c!<a!cd>>>\n0x1ee1 [[cd]<0!a!b>]\n0x016a <b!<b<!0!ad>!<b!cd>>!<ab<b!cd>>>\n0x0169 <!a<a!b!c><0!d<abc>>>\n0x037d <!a<a!b!c>[!d<0!b!c>]>\n0x0697 <<!a!c<0a!b>>!<!ab!c><!ab!d>>\n0x006f <!d<0!c!d>[ab]>\n0x17ac [<abc><!0d<0!ab>>]\n0x0069 <0!d[a[!bc]]>\n0x0667 <![!ab]<0!b[!ab]>!<cd[!ab]>>\n0x0168 [<bd<!0!bc>><ad<!0bc>>]\n0x067e <!d![c<0!a!d>][ab]>\n0x011b <a!<acd>!<!0ab>>\n0x036a [!<bcd><0!a!d>]\n0x018b <!<0ad><0!b!c><0ab>>\n0x0001 <0<0!a!b><0!c!d>>\n0x1669 <<!cd<!a!bc>><b!d[!ac]>!<bd[!ac]>>\n0x0018 <0!d[a<a!bc>]>\n0x168b [[!a<!ab!d>]<!b!c<0d<!ab!d>>>]\n0x0662 <!<!0bd>[ab][d<abc>]>\n0x00ff !d\n0x168e [!<a!c!<0!b!c>><!0<0!b!c><!ab!d>>]\n0x0007 <0!<!0cd>!<0ab>>\n0x19e6 [[ad]<0b!<0ac>>]\n0x0116 [<!b!d[!ac]><0[!ac]!<!abd>>]\n0x036c [[bd]!<!0!c<!ab![bd]>>]\n0x01e8 <!d<a[bd][cd]><0!ad>>\n0x06f0 <[cd]<a!bc>!<a!bd>>\n0x03d6 [<d!<0!a!d>!<0bc>><bc!<0bc>>]\n0x1be4 [d!<!b!c[ac]>]\n0x0187 <a!<!0ac>!<ad![!bc]>>\n0x0003 <0!d<0!b!c>>\n0x017e [[!bd]<<!ab!c>[!bd]<!0bd>>]\n0x01eb <!d[b<0!a!c>]!<!ab!<0!a!c>>>\n0x0006 <0[ab]<0!c!d>>\n0x011a <0[d[ac]]!<bcd>>\n0x0369 <!b<0!d<abc>><b!c!<0a!d>>>\n0x03d9 [d<<!0!ab><bcd><a!b!d>>]\n0x0000 0\n0x019b <!d<0!b!c>![ab]>\n0x1668 [<bcd><ab<!bcd>>]\n0x18e7 [[!cd]<abc>]\n0x0017 <0!d!<abc>>\n0x019a <!d![a<b!c!d>]!<!0c<b!c!d>>>\n0x0016 <0<a!d<bc!d>>!<abc>>";
  std::string npn4_ds = "0x3cc3 [b[!cd]]\n0x1bd8 [!<b!cd><a!b!c>]\n0x19e3 [<a!b!c><c[bd]<0a!c>>]\n0x19e1 <![<a!bd>[bc]]<!d<a!bd>![bc]><!a!c[bc]>>\n0x17e8 [d<abc>]\n0x179a [<ad!<0ad>><c<0ad>[!bd]>]\n0x178e <!c[ad]![!bd]>\n0x16e9 [d<<a!b!c><abc>!<0a!b>>]\n0x16bc [<0!b!c><!d!<abc>[ad]>]\n0x16ad [!<!b!<!0a!b>[ac]><0d<!0a!b>>]\n0x16ac [<<!0!cd>[bc]<0ab>><ad!<!0!cd>>]\n0x16a9 <0<!0a<!b!cd>>[!<0!b!c>[!ad]]>\n0x169e <!d[!a[!bc]]!<c!d[!bc]>>\n0x169b <<a!b!<0!cd>>!<ab!<0!cd>>!<!bcd>>\n0x169a [a<![!bc]<0ac><!0!bd>>]\n0x1699 <b<!a!b<0!cd>>!<b<0!cd><!abd>>>\n0x1687 [[ac]<b!<a!bd>!<0bc>>]\n0x167e <<!b!c<!abc>>[a<!abc>]!<!ad!<!abc>>>\n0x166e [<ab!<0ab>><cd<0ab>>]\n0x03cf <!c!d[bd]>\n0x166b <!c[a<bcd>]<!bc!d>>\n0x01be <c!<bcd><!<!0!bd>[ad]!<bcd>>>\n0x07f2 <[cd]<0a!b>!<0ad>>\n0x07e9 [<ac<!0!ab>><!bd<!0!ab>>]\n0x6996 [[bc][ad]]\n0x01af <[!ac]<0a!d>!<bcd>>\n0x033c <<b!cd><0c!d>!<bcd>>\n0x07e3 [c<d!<abc>!<0b!c>>]\n0x1681 <0[!<b!cd>[ab]]!<!d[ab]<0!ac>>>\n0x01ae <!<bcd>![!ad]<0b!d>>\n0x07e2 [<b!cd><b<!0!bc><ab!d>>]\n0x01ad <!<bcd><0b!d>[!ac]>\n0x07e1 [c<d<!0!bc>!<ab!d>>]\n0x001f <!a<a!b!d><0!c!d>>\n0x01ac [d<a<!a!cd>!<0!b!c>>]\n0x07e0 [d<ac<!abd>>]\n0x07bc [<bc<!0a!b>>!<0!d<!0a!b>>]\n0x03dc <b[d<!0bc>]!<ab<!0bc>>>\n0x06f6 <!d[ab][cd]>\n0x06bd <<b!c<0!bd>><a!b!d>!<a!c<0!bd>>>\n0x077e <<!0a![!cd]><0!ab>!<bcd>>\n0x06b9 <!<!0ac><a!b!d><b<a!b!d>![!cd]>>\n0x06b7 <!d[c[ab]]<!b!c[ab]>>\n0x06b6 [!c!<<!abc>!<0c!d>[ab]>]\n0x077a [!<0a!<!bcd>>![cd]]\n0x06b5 <!<a!b!c><a!b!d>![!c<!0!ad>]>\n0x0ff0 [cd]\n0x0779 <<ac!d><b!c!<ac!d>>!<ab![cd]>>\n0x06b4 [!c<!d<a!b!c>![ab]>]\n0x0778 [[cd]<0<b!c!d><0ab>>]\n0x007f <!d<!ab!c><0!b!d>>\n0x06b3 [![!bd]!<d<0bc><a!c!d>>]\n0x007e <!<abd><bc!d><0a!c>>\n0x06b2 [!<0!c!<a!bd>><bd!<ab!c>>]\n0x0776 <[ab][cd]!<abc>>\n0x06b1 [d<c<!0!bd>[!ab]>]\n0x06b0 [<!a!c<ab!d>><!b<ab!d>!<0cd>>]\n0x037c [[!bd]<d<!0b!c>!<acd>>]\n0x01ef <<0b!d><a!b!c>!<0ad>>\n0x0696 <<ab!c><0c!d>!<abc>>\n0x035e <<!0d![!ac]><0b!d>!<bcd>>\n0x0678 <<!ab!d><0a!b>[c<abd>]>\n0x0676 <!<abc>[ab]<0c!d>>\n0x0669 [<0!c!d><[cd]<0!c!d>[ab]>]\n0x01bc [<bd<!acd>>!<0!b!c>]\n0x0663 <0[b<a!c!d>]!<0cd>>\n0x07f0 <[cd]!<!0!cd>!<0ab>>\n0x01bf <!d<!0a!c><0!a!b>>\n0x0666 <0[ab]!<0cd>>\n0x0661 <<a!c!d>!<ab<a!c!d>>[<a!c!d><b!c!d>]>\n0x1689 [[d<abd>]<0!c<!a!bd>>]\n0x0660 [<b!c!d>!<!acd>]\n0x0182 <0<!0a!<!ac!d>>!<ad![!bc]>>\n0x07b6 [<!0!a<!bcd>>[!c<!0bd>]]\n0x03d7 <!d[!bc]!<abc>>\n0x06f1 [<!ac<!0!bc>><d<!0!bc><0!ad>>]\n0x03dd <[bd]<0!a!d>!<0cd>>\n0x0180 [<acd>!<b!d!<acd>>]\n0x07b4 [<bd<!0!ad>><c<!0!ad><abc>>]\n0x03db <[!bc]<a!b!d>!<acd>>\n0x07b0 <<0ac>[cd]!<abc>>\n0x03d4 <[cd]<0!a!d>[bd]>\n0x03c7 <!<0bd><0!a!c>[!bc]>\n0x03c6 <0!<0cd>[!b<!ac!d>]>\n0x03c5 <[bd]<0!a!c>![bc]>\n0x03c3 <!b<0b!d>[!bc]>\n0x03c1 <!<b!cd>[b<b!cd>]<!a!c<b!cd>>>\n0x03c0 [d<bcd>]\n0x1798 [<ab<!a!bc>><d<!a!bc><!0cd>>]\n0x036f <!c!d[!b<0!ac>]>\n0x03fc [d!<0!b!c>]\n0x1698 [b<<!abd>[ac][!ad]>]\n0x177e [<0!b!d>!<ac![bd]>]\n0x066f <!c!d[ab]>\n0x035b <<ac!d><0!b!c>!<0ac>>\n0x1697 <!<abc><ab!c><!bc!d>>\n0x066b [<bc<!a!bd>><a<!a!bd>!<0!cd>>]\n0x07f8 [!d!<ac!<0a!b>>]\n0x035a [!<!0c<0bd>><0!a!d>]\n0x1696 <!a<!d<0!bd><a!bc>><b!c<a!bc>>>\n0x003f <0!d!<bcd>>\n0x0673 <!b![d<ab!c>]<!a!d<ab!c>>>\n0x0359 [<!0!bc><ac[!cd]>]\n0x0672 <[cd]<0!b!d>[ab]>\n0x0358 <<!acd><0a!c>[!d<0!b!c>]>\n0x003d <!<bcd>[bc]<0!a!d>>\n0x0357 <!0!<!0bc><0!a!d>>\n0x003c <0!d![!bc]>\n0x0356 [!<0!a!d>!<0!b!c>]\n0x07e6 [<!cd!<0!a!b>><abc>]\n0x033f <!b!c!d>\n0x033d <!<bcd><bc!d><0!b<!0!ad>>>\n0x0690 <<!a!bd><0c!d><ab!c>>\n0x01e9 <!d<0!b![ac]>![b<!0ac>]>\n0x1686 [[bc]<a[bc]!<!abd>>]\n0x07f1 <!b<!ab!d>[cd]>\n0x01bd <!d[c<!ab!c>]<0!b<!ab!c>>>\n0x1683 [[bc]!<!ad<0bc>>]\n0x001e <0!d![!c<ab!d>]>\n0x01ab <!d![!ad]<0!b!c>>\n0x01aa <[ad]<0!b!c><0a!d>>\n0x01a9 [a<d<0!b!c><!0a!d>>]\n0x001b <0<a!b!d>!<acd>>\n0x01a8 [a<ad!<!0bc>>]\n0x000f <0!c!d>\n0x0199 <[!ab]<a!b!d><0!a!c>>\n0x0198 [!<!abd><!b!c<0ac>>]\n0x0197 <!d[!a<0bc>]<!b!c<0bc>>>\n0x06f9 <<!ab!d><a!b!d>[cd]>\n0x0196 <<0!d!<a!b!c>>[ad]!<bc[ad]>>\n0x03d8 [<!cd!<a!b!c>><!0b!<a!b!c>>]\n0x06f2 <a<0!a[cd]><!d[cd]![!ab]>>\n0x03de <!d![c<!ab!d>][bd]>\n0x018f <!c!d[b<!ab!c>]>\n0x0691 <0<!d<!a!bd><ab!c>><!0c<!a!bd>>>\n0x01ea [d<!0a<bcd>>]\n0x07b1 <<a!b!d><0!a!c>[cd]>\n0x0189 <[!ab]!<!0ac><0a!d>>\n0x036b <!a!<!ad![!bc]>!<!abc>>\n0x03d5 [<bcd>!<!d<bcd><0a!d>>]\n0x0186 [<!cd<0ab>>!<!a!bc>]\n0x0119 <0[!ab]!<acd>>\n0x0368 [<bcd><c<b!cd><!0ad>>]\n0x0183 <[!bc]<0a!d><0!a!b>>\n0x07b5 <!<bcd>[cd]![ac]>\n0x0181 <<!ab!c>[b<!ab!c>]<0!d!<!ab!c>>>\n0x036e <!c<bc!d>[!b<0!a!d>]>\n0x011f <!c!d!<!0ab>>\n0x016f <<!ab!c><0a!d>!<abd>>\n0x016e <!d<0!c[bd]>[a[bd]]>\n0x013f <0!<0ad>!<bcd>>\n0x019f <!d<b!c[ad]><0!a!b>>\n0x013e [!<b<!0ac><!0!cd>>![!d<!0!cd>]]\n0x019e <!d!<!bc[!ad]><0!b<!acd>>>\n0x013d <<bc!d>!<!0ac>!<bcd>>\n0x016b <!d<0!b!c>![!b[!ac]]>\n0x01fe [d<!0c<ab!c>>]\n0x166a <a[a<bcd>]!<ab<!bcd>>>\n0x0019 <!<!abd><0!a!b><0b!c>>\n0x006b <!d<0b<!a!bc>>!<!abc>>\n0x069f <!d<abd>!<abc>>\n0x013c <<0!ad><b!d!<!0b!c>>!<bc<0!ad>>>\n0x037e [<!0ad>!<!b!c<0a!d>>]\n0x012f <!d<a!b!c>!<!0ac>>\n0x0693 [<acd><d<!0c!d>![bd]>]\n0x012d <<0ac><!ab!c>!<bcd>>\n0x01ee [!<!0ab>!<d<0!cd><!0ab>>]\n0x012c [<0!a!b><!d[bc]<0!a!b>>]\n0x017f <!b!d!<acd>>\n0x1796 <!b!<!a!bc><!a<!0cd>!<!0!bd>>>\n0x036d <<abc>!<bcd><!a!c[bd]>>\n0x011e <d!<cd!<0!a!b>>!<d<0!a!b>!<0c!d>>>\n0x0679 <!d<!a<abd><0!b!c>>[c<abd>]>\n0x035f <!c!<0bd>!<!0ad>>\n0x067b <!d<!b!c[ad]>[![ad]<0b!c>]>\n0x0118 <<0!a!b><ac!d><!cd<0b!d>>>\n0x067a <[c<abd>]<0a!b>!<acd>>\n0x0117 <!b<!a!d<a!cd>><0!c!<a!cd>>>\n0x1ee1 [[cd]<0!a!b>]\n0x016a <b!<b<!0!ad>!<b!cd>>!<ab<b!cd>>>\n0x0169 <!a<a!b!c><0!d<abc>>>\n0x037d <!a<a!b!c>[!d<0!b!c>]>\n0x0697 <<!a!c<0a!b>>!<!ab!c><!ab!d>>\n0x006f <!d<0!c!d>[ab]>\n0x17ac [<abc><!0d<0!ab>>]\n0x0069 <0!d[a[!bc]]>\n0x0667 <![!ab]<0!b[!ab]>!<cd[!ab]>>\n0x0168 [<bd<!0!bc>><ad<!0bc>>]\n0x067e <!d![c<0!a!d>][ab]>\n0x011b <a!<acd>!<!0ab>>\n0x036a [!<bcd><0!a!d>]\n0x018b <!<0ad><0!b!c><0ab>>\n0x0001 <0<0!a!b><0!c!d>>\n0x1669 <<!cd<!a!bc>><b!d[!ac]>!<bd[!ac]>>\n0x0018 <!<bcd><0ab>!<!0a!c>>\n0x168b <<0!b!c>!<a<a!bd><0!b!c>>!<!a<0!b!c>!<!b!cd>>>\n0x0662 <!<!0bd>[ab][d<abc>]>\n0x00ff !d\n0x168e [!<a!c!<0!b!c>><!0<0!b!c><!ab!d>>]\n0x0007 <0!<!0cd>!<0ab>>\n0x19e6 [[ad]<0b!<0ac>>]\n0x0116 [<!b!d[!ac]><0[!ac]!<!abd>>]\n0x036c [<!a!d<0a!c>>!<!0b<0cd>>]\n0x01e8 <!d<a[bd][cd]><0!ad>>\n0x06f0 <[cd]<a!bc>!<a!bd>>\n0x03d6 [<d!<0!a!d>!<0bc>><bc!<0bc>>]\n0x1be4 [d!<!b!c[ac]>]\n0x0187 <a!<!0ac>!<ad![!bc]>>\n0x0003 <0!d<0!b!c>>\n0x017e [[!bd]<<!ab!c>[!bd]<!0bd>>]\n0x01eb <!d[b<0!a!c>]!<!ab!<0!a!c>>>\n0x0006 <0[ab]<0!c!d>>\n0x011a <0[d[ac]]!<bcd>>\n0x0369 <!b<0!d<abc>><b!c!<0a!d>>>\n0x03d9 [d<<!0!ab><bcd><a!b!d>>]\n0x0000 0\n0x019b <!d<0!b!c>![ab]>\n0x1668 [<bcd><ab<!bcd>>]\n0x18e7 [[!cd]<abc>]\n0x0017 <0!d!<abc>>\n0x019a <!d![a<b!c!d>]!<!0c<b!c!d>>>\n0x0016 <0<a!d<bc!d>>!<abc>>";
};

} /* namespace mockturtle */