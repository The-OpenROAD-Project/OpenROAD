// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <clocale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Itf.h"
#include "fsv/Stack.h"
#include "gtest/gtest.h"

namespace fsv {
namespace {

constexpr double kTol = 1e-9;

Stack parse(const std::string& text)
{
  std::istringstream in(text);
  return readItf(in);
}

// Expects parsing to fail with a message containing `fragment`.
void expectError(const std::string& text, const std::string_view fragment)
{
  try {
    parse(text);
    ADD_FAILURE() << "expected an error containing '" << fragment << "'";
  } catch (const std::runtime_error& e) {
    EXPECT_NE(std::string(e.what()).find(fragment), std::string::npos)
        << e.what();
  }
}

// z ranges in µm, measured from the ground plane at the top of PSUB.
TEST(Itf, Sky130Stack)
{
  const Stack stack = readItfFile("src/fsv/test/sky130.itf");
  EXPECT_EQ(stack.technology(), "skywater130_nominal");
  EXPECT_EQ(stack.backgroundEps(), 1.0);

  struct Layer
  {
    const char* name;
    double bottom;
    double top;
    double eps;
  };
  const std::vector<Layer> dielectrics = {{"FOX", 0.0, 0.3262, 3.9},
                                          {"PSG", 0.3262, 0.9361, 3.9},
                                          {"lint", 0.9361, 1.1111, 7.3},
                                          {"NILD2", 1.1111, 1.3011, 4.05},
                                          {"NILD3", 1.3011, 1.9311, 4.5},
                                          {"NILD4", 1.9311, 2.7111, 4.2},
                                          {"NILD5", 2.7111, 3.9461, 4.1},
                                          {"NILD6", 3.9461, 5.2961, 4.0},
                                          {"TOPNIT", 5.2961, 5.6738, 7.5},
                                          {"PI1", 5.6738, 11.8084, 2.94}};
  ASSERT_EQ(stack.dielectrics().size(), dielectrics.size());
  for (size_t i = 0; i < dielectrics.size(); i++) {
    EXPECT_EQ(stack.dielectrics()[i].name, dielectrics[i].name);
    EXPECT_NEAR(stack.dielectrics()[i].bottom, dielectrics[i].bottom, kTol);
    EXPECT_NEAR(stack.dielectrics()[i].top, dielectrics[i].top, kTol);
    EXPECT_EQ(stack.dielectrics()[i].eps, dielectrics[i].eps);
  }

  const std::vector<Layer> conductors = {{"DIFF", 0.0, 0.12, 0},
                                         {"fieldpoly", 0.3262, 0.5062, 0},
                                         {"li1", 0.9361, 1.0361, 0},
                                         {"met1", 1.3011, 1.6611, 0},
                                         {"met2", 1.9311, 2.2911, 0},
                                         {"met3", 2.7111, 3.5561, 0},
                                         {"met4", 3.9461, 4.7911, 0},
                                         {"met5", 5.2961, 6.5561, 0}};
  ASSERT_EQ(stack.conductors().size(), conductors.size());
  for (const Layer& expected : conductors) {
    const Stack::Layer layer = stack.layer(expected.name);
    ASSERT_TRUE(stack.isConductor(layer)) << expected.name;
    const Stack::Conductor& c = stack.conductor(layer);
    EXPECT_NEAR(c.bottom, expected.bottom, kTol) << expected.name;
    EXPECT_NEAR(c.top, expected.top, kTol) << expected.name;
  }
  EXPECT_EQ(stack.conductor(stack.layer("met1")).min_width, 0.14);
  EXPECT_EQ(stack.conductor(stack.layer("met1")).min_spacing, 0.14);

  // Conformal dielectrics, innermost first. TOPNIT_C and SPNIT have no
  // ASSOCIATED_CONDUCTOR and wrap the conductor that rises above the
  // planar top below them.
  auto conformals = [&](const char* name) {
    std::vector<std::string> names;
    for (const Stack::ConformalDielectric& conformal :
         stack.conductor(stack.layer(name)).conformals) {
      names.push_back(conformal.name);
    }
    return names;
  };
  EXPECT_EQ(conformals("met5"),
            (std::vector<std::string>{"TOPOX", "TOPNIT_C"}));
  EXPECT_EQ(conformals("fieldpoly"),
            (std::vector<std::string>{"IOX", "SPNIT"}));
  EXPECT_EQ(conformals("li1"), (std::vector<std::string>{"lint_C"}));
  EXPECT_EQ(conformals("met1"), (std::vector<std::string>{"NILD3_C"}));
  EXPECT_EQ(conformals("met2"), (std::vector<std::string>{"NILD4_C"}));
  EXPECT_TRUE(conformals("met3").empty());
  EXPECT_TRUE(conformals("DIFF").empty());
  const Stack::ConformalDielectric& topnit_c
      = stack.conductor(stack.layer("met5")).conformals[1];
  EXPECT_NEAR(topnit_c.side_thickness, 0.423, kTol);
  EXPECT_NEAR(topnit_c.top_thickness, 0.54, kTol);
  EXPECT_EQ(topnit_c.eps, 7.5);

  // FOX and PSG share ER 3.9, so they form no interface; PI1 meets air.
  const std::vector<Stack::PlanarInterface> expected_interfaces
      = {{0.9361, 3.9, 7.3},
         {1.1111, 7.3, 4.05},
         {1.3011, 4.05, 4.5},
         {1.9311, 4.5, 4.2},
         {2.7111, 4.2, 4.1},
         {3.9461, 4.1, 4.0},
         {5.2961, 4.0, 7.5},
         {5.6738, 7.5, 2.94},
         {11.8084, 2.94, 1.0}};
  const std::vector<Stack::PlanarInterface> interfaces
      = stack.planarInterfaces();
  ASSERT_EQ(interfaces.size(), expected_interfaces.size());
  for (size_t i = 0; i < interfaces.size(); i++) {
    EXPECT_NEAR(interfaces[i].z, expected_interfaces[i].z, kTol);
    EXPECT_EQ(interfaces[i].eps_below, expected_interfaces[i].eps_below);
    EXPECT_EQ(interfaces[i].eps_above, expected_interfaces[i].eps_above);
  }
  EXPECT_EQ(stack.epsAt(0.5), 3.9);
  EXPECT_EQ(stack.epsAt(1.0), 7.3);
  EXPECT_EQ(stack.epsAt(20.0), 1.0);

  EXPECT_EQ(stack.vias().size(), 7u);
  EXPECT_EQ(stack.vias()[0].name, "via4");
  EXPECT_EQ(stack.name(stack.vias()[0].upper), "met5");
  EXPECT_EQ(stack.name(stack.vias()[0].lower), "met4");
}

TEST(Itf, CommentsAndBackground)
{
  const Stack stack = parse(
      "$ a comment line\n"
      "TECHNOLOGY = t\n"
      "BACKGROUND_ER = 3.0\n"
      "  $ indented comment\n"
      "DIELECTRIC D2 { THICKNESS=1.0 ER=4.0 }\n"
      "CONDUCTOR M1 { THICKNESS=0.2 WMIN=0.1 SMIN=0.1 RPSQ=0.1 }\n"
      "DIELECTRIC D1 { THICKNESS=0.5 ER=4.0 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n");
  EXPECT_EQ(stack.backgroundEps(), 3.0);
  ASSERT_EQ(stack.dielectrics().size(), 2u);
  EXPECT_NEAR(stack.conductor(stack.layer("M1")).bottom, 0.5, kTol);
  EXPECT_NEAR(stack.conductor(stack.layer("M1")).top, 0.7, kTol);
  // D1 and D2 share ER 4.0; the only interface is the top of D2, against
  // the background.
  const std::vector<Stack::PlanarInterface> interfaces
      = stack.planarInterfaces();
  ASSERT_EQ(interfaces.size(), 1u);
  EXPECT_NEAR(interfaces[0].z, 1.5, kTol);
  EXPECT_EQ(interfaces[0].eps_below, 4.0);
  EXPECT_EQ(interfaces[0].eps_above, 3.0);
}

// Numbers may use a leading point, a sign and an exponent.
TEST(Itf, NumberForms)
{
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "DIELECTRIC D3 { THICKNESS=.5 ER=4 }\n"
      "DIELECTRIC D2 { THICKNESS=5e-1 ER=+4.0 }\n"
      "DIELECTRIC D1 { THICKNESS=0.5E0 ER=4 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n");
  ASSERT_EQ(stack.dielectrics().size(), 3u);
  for (const Stack::Dielectric& d : stack.dielectrics()) {
    EXPECT_DOUBLE_EQ(d.top - d.bottom, 0.5) << d.name;
    EXPECT_DOUBLE_EQ(d.eps, 4.0) << d.name;
  }
}

// FROM and TO may come in either order; the upper layer is decided by
// height. SUBSTRATE is the ground plane.
TEST(Itf, ViaLayersOrderedByHeight)
{
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "DIELECTRIC D2 { THICKNESS=1.0 ER=4.0 }\n"
      "CONDUCTOR M2 { THICKNESS=0.2 }\n"
      "DIELECTRIC D1 { THICKNESS=0.5 ER=4.0 }\n"
      "CONDUCTOR M1 { THICKNESS=0.2 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n"
      "VIA up { FROM=M1 TO=M2 }\n"
      "VIA down { FROM=M2 TO=M1 }\n"
      "VIA tie { FROM=SUBSTRATE TO=M1 }\n");
  ASSERT_EQ(stack.vias().size(), 3u);
  for (int i = 0; i < 2; i++) {
    EXPECT_EQ(stack.name(stack.vias()[i].upper), "M2");
    EXPECT_EQ(stack.name(stack.vias()[i].lower), "M1");
  }
  EXPECT_EQ(stack.name(stack.vias()[2].upper), "M1");
  EXPECT_FALSE(stack.vias()[2].lower.valid());
}

// A conductor spans its own thickness and a via the gap between its layers;
// D1 is measured from the bottom of M1, so M2 starts at 0.5.
TEST(Itf, ZExtent)
{
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "DIELECTRIC D2 { THICKNESS=1.0 ER=4.0 }\n"
      "CONDUCTOR M2 { THICKNESS=0.2 }\n"
      "DIELECTRIC D1 { THICKNESS=0.5 ER=4.0 }\n"
      "CONDUCTOR M1 { THICKNESS=0.2 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n"
      "VIA V1 { FROM=M1 TO=M2 }\n"
      "VIA tie { FROM=SUBSTRATE TO=M1 }\n");

  const Stack::Layer m1 = stack.layer("M1");
  ASSERT_TRUE(stack.isConductor(m1));
  EXPECT_FALSE(stack.isVia(m1));
  EXPECT_EQ(stack.name(m1), "M1");
  EXPECT_NEAR(stack.zExtent(m1).bottom, 0.0, kTol);
  EXPECT_NEAR(stack.zExtent(m1).top, 0.2, kTol);

  const Stack::Layer v1 = stack.layer("V1");
  ASSERT_TRUE(stack.isVia(v1));
  EXPECT_FALSE(stack.isConductor(v1));
  EXPECT_EQ(stack.via(v1).upper, stack.layer("M2"));
  EXPECT_EQ(stack.via(v1).lower, m1);
  EXPECT_NEAR(stack.zExtent(v1).bottom, 0.2, kTol);
  EXPECT_NEAR(stack.zExtent(v1).top, 0.5, kTol);

  // A substrate via has no lower conductor and starts at the ground plane.
  const Stack::Layer tie = stack.layer("tie");
  EXPECT_FALSE(stack.via(tie).lower.valid());
  EXPECT_NEAR(stack.zExtent(tie).bottom, 0.0, kTol);
  EXPECT_NEAR(stack.zExtent(tie).top, 0.0, kTol);

  // A name the stack lacks gives a handle that names no layer, which the
  // lookups refuse.
  const Stack::Layer none = stack.layer("M9");
  EXPECT_FALSE(none.valid());
  EXPECT_FALSE(stack.isConductor(none));
  EXPECT_FALSE(stack.isVia(none));
  EXPECT_THROW(stack.zExtent(none), std::invalid_argument);
  EXPECT_THROW(stack.name(none), std::invalid_argument);
  EXPECT_THROW(stack.conductor(v1), std::invalid_argument);
  EXPECT_THROW(stack.via(m1), std::invalid_argument);
  // A via needs a conductor above it; the stack refuses one without.
  Stack broken = stack;
  Stack::Via v9;
  v9.name = "v9";
  v9.lower = m1;
  EXPECT_THROW(broken.addVia(v9), std::invalid_argument);
  // A conductor after the vias would shift their handles.
  Stack::Conductor late;
  late.name = "M3";
  late.bottom = 2;
  late.top = 3;
  EXPECT_THROW(broken.addConductor(late), std::invalid_argument);
  // Handles are indices: equal for the same layer, ordered, and the same
  // on a copy of the stack.
  EXPECT_EQ(stack.layer("M1"), m1);
  EXPECT_NE(stack.layer("M2"), m1);
  const Stack copy = stack;
  EXPECT_EQ(copy.name(v1), "V1");
  EXPECT_EQ(stack.conductorLayers().size(), 2u);
  EXPECT_EQ(stack.viaLayers().size(), 2u);
  EXPECT_EQ(stack.viaLayers()[0], v1);
}

// Without ASSOCIATED_CONDUCTOR a conformal dielectric wraps only conductors
// that rise above the planar top. M0 is flush with the top of D1 up to
// rounding (0.1 + 0.2 vs (0.1 + 0.15) + 0.05) and must not be wrapped.
TEST(Itf, ConformalWithoutAssociatedConductor)
{
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "DIELECTRIC D3 { THICKNESS=1.0 ER=4.0 }\n"
      "DIELECTRIC C { THICKNESS=0 ER=7.0 SW_T=0.05 TW_T=0.05 IS_CONFORMAL }\n"
      "DIELECTRIC D2 { THICKNESS=0.05 ER=4.0 }\n"
      "CONDUCTOR M1 { THICKNESS=0.2 }\n"
      "DIELECTRIC D1B { THICKNESS=0.05 MEASURED_FROM=D1A ER=4.0 }\n"
      "DIELECTRIC D1A { THICKNESS=0.15 ER=4.0 }\n"
      "CONDUCTOR M0 { THICKNESS=0.2 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n");
  ASSERT_EQ(stack.conductor(stack.layer("M1")).conformals.size(), 1u);
  EXPECT_EQ(stack.conductor(stack.layer("M1")).conformals[0].name, "C");
  EXPECT_TRUE(stack.conductor(stack.layer("M0")).conformals.empty());
}

// A '$' starts a comment anywhere on a line.
TEST(Itf, TrailingComment)
{
  const Stack stack = parse(
      "TECHNOLOGY = t $ test stack\n"
      "DIELECTRIC D1 { THICKNESS=0.5 ER=4.0 } $ ILD\n"
      "CONDUCTOR M1 { THICKNESS=0.2 $ nominal\n"
      "  WMIN=0.1 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n");
  EXPECT_EQ(stack.technology(), "t");
  EXPECT_NEAR(stack.conductor(stack.layer("M1")).top, 0.2, kTol);
  EXPECT_EQ(stack.conductor(stack.layer("M1")).min_width, 0.1);
}

// Numbers parse the same under a comma-decimal C locale.
TEST(Itf, IndependentOfLocale)
{
  const std::string saved = std::setlocale(LC_NUMERIC, nullptr);
  if (!std::setlocale(LC_NUMERIC, "de_DE.UTF-8")
      && !std::setlocale(LC_NUMERIC, "fr_FR.UTF-8")) {
    GTEST_SKIP() << "no comma-decimal locale installed";
  }
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "DIELECTRIC D1 { THICKNESS=0.5 ER=3.9 }\n"
      "CONDUCTOR M1 { THICKNESS=0.25 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n");
  std::setlocale(LC_NUMERIC, saved.c_str());
  EXPECT_NEAR(stack.conductor(stack.layer("M1")).top, 0.25, kTol);
  EXPECT_EQ(stack.dielectrics()[0].eps, 3.9);
}

TEST(Itf, Errors)
{
  const std::string head = "TECHNOLOGY = t\n";
  const std::string sub = "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n";
  expectError(head + "FOO = 1\n" + sub, "unsupported ITF statement FOO");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=4 DAMAGE_ER=3 }\n" + sub,
              "unsupported keyword DAMAGE_ER");
  expectError(head + "CONDUCTOR M { THICKNESS=1 ETCH_VS_WIDTH_AND_SPACING { "
                     "} }\n" + sub,
              "unsupported table ETCH_VS_WIDTH_AND_SPACING");
  expectError(
      head + "CONDUCTOR M { THICKNESS=1 CAPACITIVE_ONLY_ETCH=0.01 }\n" + sub,
      "non-zero CAPACITIVE_ONLY_ETCH");
  expectError(
      head + "DIELECTRIC D { THICKNESS=1 ER=4 MEASURED_FROM=X }\n" + sub,
      "MEASURED_FROM X of D is not a planar dielectric below it");
  expectError(head
                  + "DIELECTRIC C { THICKNESS=0 ER=4 IS_CONFORMAL "
                    "ASSOCIATED_CONDUCTOR=M9 SW_T=0.1 }\n"
                  + sub,
              "ASSOCIATED_CONDUCTOR M9 of C is not a conductor below it");
  expectError(head + "DIELECTRIC D { THICKNESS=x ER=4 }\n" + sub,
              "line 2: THICKNESS expects a finite number");
  expectError(head + "DIELECTRIC D { THICKNESS=0.5x ER=4 }\n" + sub,
              "THICKNESS expects a finite number, got '0.5x'");
  expectError(head + "DIELECTRIC D { THICKNESS=1e999 ER=4 }\n" + sub,
              "THICKNESS expects a finite number, got '1e999'");
  expectError(head + "DIELECTRIC D { ER=4 }\n" + sub,
              "DIELECTRIC D is missing THICKNESS");
  expectError("DIELECTRIC D { THICKNESS=1 ER=4 }\n", "missing TECHNOLOGY");
  // Numbers must be finite and permittivities positive.
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=0 }\n" + sub,
              "ER of D must be positive");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=-3.9 }\n" + sub,
              "ER of D must be positive");
  expectError(head + "BACKGROUND_ER = 0\n" + sub,
              "BACKGROUND_ER must be positive");
  expectError(head + "BACKGROUND_ER = nan\n" + sub,
              "BACKGROUND_ER expects a finite number, got 'nan'");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=-4 }\n" + sub,
              "ER of D must be positive");
  expectError(head + "CONDUCTOR M { THICKNESS=nan }\n" + sub,
              "THICKNESS expects a finite number, got 'nan'");
  expectError(head
                  + "DIELECTRIC C { THICKNESS=0 ER=4 IS_CONFORMAL "
                    "ASSOCIATED_CONDUCTOR=M SW_T=inf }\n"
                  + "CONDUCTOR M { THICKNESS=1 }\n" + sub,
              "SW_T expects a finite number, got 'inf'");
  // Duplicates.
  expectError(head + "TECHNOLOGY = u\n" + sub,
              "line 2: TECHNOLOGY is given twice");
  expectError(head + "BACKGROUND_ER = 2\nBACKGROUND_ER = 3\n" + sub,
              "BACKGROUND_ER is given twice");
  const std::string two_layers
      = "DIELECTRIC D { THICKNESS=1 ER=4 }\nCONDUCTOR M2 { THICKNESS=0.2 }\n"
        "DIELECTRIC D0 { THICKNESS=0.5 ER=4 }\nCONDUCTOR M1 { THICKNESS=0.2 "
        "}\n";
  expectError(head + two_layers + sub
                  + "VIA v { FROM=M1 TO=M2 }\nVIA v { FROM=M2 TO=M1 }\n",
              "VIA v is defined twice");
  // Via layers must exist and must not overlap in height.
  expectError(head + two_layers + sub + "VIA v { FROM=M1 TO=M9 }\n",
              "VIA v names unknown layer M9");
  expectError(head + two_layers + sub + "VIA v { FROM=M1 TO=M1 }\n",
              "VIA v connects M1 to itself");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=4 }\n"
                  + "CONDUCTOR M2 { THICKNESS=0.2 }\n"
                  + "CONDUCTOR M1 { THICKNESS=0.2 }\n" + sub
                  + "VIA v { FROM=M1 TO=M2 }\n",
              "which overlap in height");
}

}  // namespace
}  // namespace fsv
