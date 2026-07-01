#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <lorina/genlib.hpp>
#include <mockturtle/algorithms/emap.hpp>
#include <mockturtle/io/genlib_reader.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/networks/block.hpp>
#include <mockturtle/utils/tech_library.hpp>
#include <mockturtle/views/cell_view.hpp>

using namespace mockturtle;

std::string const large_library = "GATE   inv1    1 O=!a;            PIN * INV 1 999 0.9 0.3 0.9 0.3\n"
                                  "GATE   inv2    2 O=!a;            PIN * INV 2 999 1.0 0.1 1.0 0.1\n"
                                  "GATE   nand2   2 O=!(a*b);        PIN * INV 1 999 1.0 0.2 1.0 0.2\n"
                                  "GATE   xor2    5 O=a^b;           PIN * UNKNOWN 2 999 1.9 0.5 1.9 0.5\n"
                                  "GATE   mig3    3 O=a*b+a*c+b*c;   PIN * INV 1 999 2.0 0.2 2.0 0.2\n"
                                  "GATE   buf     2 O=a;             PIN * NONINV 1 999 1.0 0.0 1.0 0.0\n"
                                  "GATE   zero    0 O=CONST0;\n"
                                  "GATE   one     0 O=CONST1;\n"
                                  "GATE   nand8   8 O=!(a*b*c*d*e*f*g*h);   PIN * INV 1 999 4.0 0.2 4.0 0.2\n";

#define CHECK(val) if (!(val)) { std::abort(); }

TEST(mockturtle, emap)
{
  std::vector<gate> gates;

  std::istringstream in( large_library );
  auto result = lorina::read_genlib( in, genlib_reader( gates ) );
  EXPECT_EQ( result, lorina::return_code::success );

  tech_library<8> lib( gates );

  aig_network aig;
  const auto a = aig.create_pi();
  const auto b = aig.create_pi();
  const auto c = aig.create_pi();
  const auto d = aig.create_pi();
  const auto e = aig.create_pi();
  const auto f = aig.create_pi();
  const auto g = aig.create_pi();
  const auto h = aig.create_pi();

  const auto f1 = aig.create_and( !a, b );
  const auto f2 = aig.create_and( f1, !c );
  const auto f3 = aig.create_and( d, e );
  const auto f4 = aig.create_and( f, !g );
  const auto f5 = aig.create_and( f4, h );
  const auto f6 = aig.create_and( f2, f3 );
  const auto f7 = aig.create_and( f5, f6 );

  aig.create_po( f7 );

  emap_params ps;
  ps.matching_mode = emap_params::hybrid;
  emap_stats st;
  cell_view<block_network> ntk = emap<8>( aig, lib, ps, &st );

  const float eps{ 0.005f };

  EXPECT_EQ( ntk.size(), 15u );
  EXPECT_EQ( ntk.num_pis(), 8u );
  EXPECT_EQ( ntk.num_pos(), 1u );
  EXPECT_EQ( ntk.num_gates(), 5u );
  EXPECT_GT( st.area, 12.0f - eps );
  EXPECT_LT( st.area, 12.0f + eps );
  EXPECT_GT( st.delay, 5.8f - eps );
  EXPECT_LT( st.delay, 5.8f + eps );
}
