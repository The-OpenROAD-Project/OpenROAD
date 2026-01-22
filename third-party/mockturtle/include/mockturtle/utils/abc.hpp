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
  \file abc.hpp
  \brief Utility functions for interfacing with ABC

  \author Siang-Yun (Sonia) Lee
*/

#pragma once
#ifdef ENABLE_ABC

#include "../networks/aig.hpp"
#include "../networks/gia.hpp"

namespace mockturtle
{

void aig_to_gia(gia_network &gia, aig_network aig) {
  using aig_node = aig_network::node;
  using aig_signal = aig_network::signal;

  std::vector<gia_signal> a_to_g(aig.size());

  /* constant */
  a_to_g[0] = gia.get_constant(false);

  /* pis */
  aig.foreach_pi([&](aig_node n) {
    a_to_g[n] = gia.create_pi();
  });

  /* ands */
  aig.foreach_gate([&](aig_node n){
    std::array<gia_signal, 2u> fis;
    aig.foreach_fanin(n, [&](aig_signal fi, int index){
      fis[index] = aig.is_complemented(fi) ? !a_to_g[aig.get_node(fi)] : a_to_g[aig.get_node(fi)];
    });
    a_to_g[n] = gia.create_and(fis[0], fis[1]);
  });

  /* pos */
  aig.foreach_po([&](aig_signal f){
    gia.create_po(aig.is_complemented(f) ? !a_to_g[aig.get_node(f)] : a_to_g[aig.get_node(f)]);
  });
}

void gia_to_aig(aig_network aig, const gia_network &gia) {
  using gia_node = gia_network::node;
  using gia_signal = gia_network::signal;

  std::vector<aig_network::signal> g_to_a(gia.size());

  /* constant */
  g_to_a[0] = aig.get_constant(false);

  /* pis */
  gia.foreach_pi([&](gia_network::node n){
    g_to_a[n] = aig.create_pi();
  });

  /* ands */
  gia.foreach_gate([&](gia_network::node n){
    std::array<aig_network::signal, 2u> fis;
    gia.foreach_fanin(n, [&](gia_signal fi, int index){
      fis[index] = gia.is_complemented(fi) ? !g_to_a[gia.get_node(fi)] : g_to_a[gia.get_node(fi)];
    });
    
    g_to_a[n] = aig.create_and(fis[0], fis[1]);
  });

  /* pos */
  gia.foreach_po([&](gia_network::signal f){
    aig.create_po(gia.is_complemented(f) ? !g_to_a[gia.get_node(f)] : g_to_a[gia.get_node(f)]);
  });
}

aig_network call_abc_script( aig_network const& aig, std::string const& script )
{
  gia_network gia( aig.size() << 1 );
  aig_to_gia( gia, aig );

  gia.load_rc();
  gia.run_opt_script( script );

  aig_network new_aig;
  gia_to_aig( new_aig, gia );

  new_aig = cleanup_dangling( new_aig );
  return new_aig;
}

}

#endif