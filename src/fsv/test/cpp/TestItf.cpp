// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <clocale>
#include <cmath>
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

Stack parse(const std::string& text,
            std::vector<ItfWarning>* warnings = nullptr)
{
  std::istringstream in(text);
  return readItf(in, warnings);
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
  // The stack checks the resistance and etch fields of a hand-built layer.
  Stack::Conductor odd;
  odd.name = "M3";
  odd.bottom = 2;
  odd.top = 3;
  odd.rpsq_vs_width_spacing.widths = {0.1, 0.2};
  odd.rpsq_vs_width_spacing.spacings = {0.1};
  odd.rpsq_vs_width_spacing.values = {1};  // one short
  Stack fresh;
  fresh.addDielectric({"d", 0, 10, 4});
  EXPECT_THROW(fresh.addConductor(odd), std::invalid_argument);
  odd.rpsq_vs_width_spacing.values = {1, -2};
  EXPECT_THROW(fresh.addConductor(odd), std::invalid_argument);
  odd.rpsq_vs_width_spacing = {};
  Stack::EtchTable directional;
  directional.direction = Stack::EtchTable::Direction::kParallelToReference;
  directional.table = {{0.1}, {0.1}, {0.01}};
  odd.etch_tables.push_back(directional);
  EXPECT_THROW(fresh.addConductor(odd), std::invalid_argument);
  fresh.setReferenceDirection(Stack::ReferenceDirection::kVertical);
  const Stack::Layer m3 = fresh.addConductor(odd);
  Stack::Via half;
  half.name = "v";
  half.upper = m3;
  half.rpv = 4;  // without an area
  EXPECT_THROW(fresh.addVia(half), std::invalid_argument);
  half.area = 0.01;
  half.rho = 1;  // a second form
  EXPECT_THROW(fresh.addVia(half), std::invalid_argument);
  half.rho = 0;
  half.crt.crt1 = std::nan("");
  EXPECT_THROW(fresh.addVia(half), std::invalid_argument);
  half.crt.crt1 = 0;
  EXPECT_TRUE(fresh.isVia(fresh.addVia(half)));
  Stack::Conductor two_forms = odd;
  two_forms.name = "M4";
  two_forms.rpsq = 1;
  two_forms.rho = 1;
  EXPECT_THROW(fresh.addConductor(two_forms), std::invalid_argument);
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
  expectError(head + "CONDUCTOR M { THICKNESS=1 ETCH_VS_WIDTH_AND_SPACING { "
                     "} }\n" + sub,
              "ETCH_VS_WIDTH_AND_SPACING is missing SPACINGS");
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

// The sky130 file's resistance, etch and temperature values.
TEST(Itf, Sky130Resistance)
{
  const Stack stack = readItfFile("src/fsv/test/sky130.itf");
  EXPECT_EQ(stack.globalTemperature(), 25);
  EXPECT_EQ(stack.halfNodeScaleFactor(), 1);
  EXPECT_EQ(stack.referenceDirection(), Stack::ReferenceDirection::kNone);
  const Stack::Conductor& met1 = stack.conductor(stack.layer("met1"));
  EXPECT_EQ(met1.rpsq, 0.125);
  EXPECT_EQ(met1.rho, 0);
  EXPECT_TRUE(met1.rpsq_vs_width.empty());
  EXPECT_EQ(met1.crt.crt1, 0.003179);
  EXPECT_EQ(met1.crt.crt2, 3.094e-7);
  EXPECT_EQ(met1.t0, 25);
  EXPECT_EQ(met1.capacitive_etch, 0);
  EXPECT_EQ(met1.resistive_etch, -0.0195);
  EXPECT_TRUE(met1.etch_tables.empty());
  EXPECT_EQ(met1.layer_type, "");
  EXPECT_EQ(stack.conductor(stack.layer("DIFF")).rpsq, 197);
  const Stack::Via& via = stack.via(stack.layer("mcon"));
  EXPECT_EQ(via.rpv, 9.3);
  EXPECT_EQ(via.area, 0.0289);
  EXPECT_EQ(via.rho, 0);
  EXPECT_EQ(via.t0, 25);
}

// The GT2 file reads, with its layer types and via resistances.
TEST(Itf, Gt2Stack)
{
  const Stack stack = readItfFile("src/fsv/test/GT2.itf");
  EXPECT_EQ(stack.technology(), "GT2");
  EXPECT_EQ(stack.conductors().size(), 24u);
  EXPECT_EQ(stack.vias().size(), 22u);
  const Stack::Conductor& m1 = stack.conductor(stack.layer("M1"));
  EXPECT_GT(m1.rpsq, 0);
  EXPECT_EQ(m1.layer_type, "");
  for (const Stack::Via& via : stack.vias()) {
    EXPECT_GT(via.rpv, 0) << via.name;
    EXPECT_GT(via.area, 0) << via.name;
  }
}

// Every table form and header keyword in scope, read into the stack.
TEST(Itf, ResistanceEtchAndTemperature)
{
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "GLOBAL_TEMPERATURE = 85\n"
      "HALF_NODE_SCALE_FACTOR = 0.9\n"
      "REFERENCE_DIRECTION = HORIZONTAL\n"
      "DIELECTRIC D3 { THICKNESS=1 ER=4 }\n"
      "CONDUCTOR M3 { THICKNESS=0.2 WMIN=0.1 SMIN=0.1 LAYER_TYPE=BUMP\n"
      "  RHO_VS_WIDTH_AND_SPACING { WIDTHS { 0.1 0.2 } SPACINGS { 0.1 }\n"
      "    VALUES { 0.03 0.02 } }\n"
      "  ETCH=0.01 T0=50 CRT1=0.002\n"
      "  ETCH_VS_WIDTH_AND_SPACING CAPACITIVE_ONLY PERPENDICULAR_TO_REFERENCE "
      "{\n"
      "    SPACINGS { 0.1 0.2 0.4 } WIDTHS { 0.1 1 }\n"
      "    VALUES { 0.01 0.02 0.03 -0.01, -0.02, -0.03 } }\n"
      "}\n"
      "DIELECTRIC D2 { THICKNESS=1 ER=4 }\n"
      "CONDUCTOR M2 { THICKNESS=0.2 WMIN=0.1 SMIN=0.1\n"
      "  RPSQ_VS_WIDTH_AND_SPACING { SPACINGS { 0.1 0.2 } WIDTHS { 0.1 }\n"
      "    VALUES { 0.5 0.4 } }\n"
      "  CAPACITIVE_ONLY_ETCH=0.02 RESISTIVE_ONLY_ETCH=-0.01\n"
      "  CRT_VS_SI_WIDTH { (0.1, 0.003, 1e-7) (1, 0.002, 2e-7) }\n"
      "}\n"
      "DIELECTRIC D1 { THICKNESS=1 ER=4 }\n"
      "CONDUCTOR M1 { THICKNESS=0.2 WMIN=0.1 SMIN=0.1\n"
      "  RPSQ_VS_SI_WIDTH { (0.1, 0.6) (0.5, 0.5) (2, 0.45) }\n"
      "  ETCH_VS_WIDTH_AND_SPACING RESISTIVE_ONLY {\n"
      "    WIDTHS { 0.1 } SPACINGS { 0.1 } VALUES { 0.005 } }\n"
      "  ETCH_VS_WIDTH_AND_SPACING PARALLEL_TO_GATE {\n"
      "    WIDTHS { 0.1 } SPACINGS { 0.1 } VALUES { 0.005 } }\n"
      "}\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n"
      "VIA V1 { FROM=M1 TO=M2 RPV=4 AREA=0.01 CRT1=0.001 CRT2=-1e-7 }\n"
      "VIA V2 { FROM=M2 TO=M3 RHO=0.3 T0=60\n"
      "  CRT_VS_AREA { (0.01, 0.001, 0) (0.04, 0.002, 0) } }\n"
      "VIA V3 { FROM=M3 TO=M2 RPV_VS_AREA { (0.01, 5) (0.04, 2) } }\n");
  EXPECT_EQ(stack.globalTemperature(), 85);
  EXPECT_EQ(stack.halfNodeScaleFactor(), 0.9);
  EXPECT_EQ(stack.referenceDirection(), Stack::ReferenceDirection::kHorizontal);

  const Stack::Conductor& m1 = stack.conductor(stack.layer("M1"));
  ASSERT_EQ(m1.rpsq_vs_width.x.size(), 3u);
  EXPECT_EQ(m1.rpsq_vs_width.x[1], 0.5);
  EXPECT_EQ(m1.rpsq_vs_width.y[2], 0.45);
  EXPECT_EQ(m1.rpsq, 0);
  EXPECT_EQ(m1.t0, 85);  // the global temperature
  // Two tables are two etch steps; a PARALLEL_TO_GATE one is device-level
  // and dropped.
  ASSERT_EQ(m1.etch_tables.size(), 1u);
  EXPECT_EQ(m1.etch_tables[0].affects, Stack::EtchTable::Affects::kResistance);
  EXPECT_EQ(m1.etch_tables[0].direction, Stack::EtchTable::Direction::kAll);
  EXPECT_EQ(m1.etch_tables[0].table.at(0, 0), 0.005);

  const Stack::Conductor& m2 = stack.conductor(stack.layer("M2"));
  EXPECT_EQ(m2.capacitive_etch, 0.02);
  EXPECT_EQ(m2.resistive_etch, -0.01);
  ASSERT_EQ(m2.rpsq_vs_width_spacing.widths.size(), 1u);
  ASSERT_EQ(m2.rpsq_vs_width_spacing.spacings.size(), 2u);
  EXPECT_EQ(m2.rpsq_vs_width_spacing.at(0, 1), 0.4);
  ASSERT_EQ(m2.crt_vs_width.x.size(), 2u);
  EXPECT_EQ(m2.crt_vs_width.crt[1].crt1, 0.002);
  EXPECT_EQ(m2.crt_vs_width.crt[1].crt2, 2e-7);
  EXPECT_EQ(m2.crt.crt1, 0);

  const Stack::Conductor& m3 = stack.conductor(stack.layer("M3"));
  EXPECT_EQ(m3.layer_type, "BUMP");
  EXPECT_EQ(m3.capacitive_etch, 0.01);  // ETCH sets both
  EXPECT_EQ(m3.resistive_etch, 0.01);
  EXPECT_EQ(m3.t0, 50);
  EXPECT_EQ(m3.crt.crt1, 0.002);
  EXPECT_EQ(m3.rho_vs_width_spacing.at(1, 0), 0.02);
  ASSERT_EQ(m3.etch_tables.size(), 1u);
  const Stack::EtchTable& etch = m3.etch_tables[0];
  EXPECT_EQ(etch.affects, Stack::EtchTable::Affects::kCapacitance);
  EXPECT_EQ(etch.direction,
            Stack::EtchTable::Direction::kPerpendicularToReference);
  EXPECT_EQ(etch.table.widths, (std::vector<double>{0.1, 1}));
  EXPECT_EQ(etch.table.spacings, (std::vector<double>{0.1, 0.2, 0.4}));
  EXPECT_EQ(etch.table.at(1, 2), -0.03);  // commas separate like spaces

  const Stack::Via& v1 = stack.via(stack.layer("V1"));
  EXPECT_EQ(v1.rpv, 4);
  EXPECT_EQ(v1.area, 0.01);
  EXPECT_EQ(v1.crt.crt1, 0.001);
  EXPECT_EQ(v1.crt.crt2, -1e-7);
  EXPECT_EQ(v1.t0, 85);
  const Stack::Via& v2 = stack.via(stack.layer("V2"));
  EXPECT_EQ(v2.rho, 0.3);
  EXPECT_EQ(v2.t0, 60);
  ASSERT_EQ(v2.crt_vs_area.x.size(), 2u);
  EXPECT_EQ(v2.crt_vs_area.crt[1].crt1, 0.002);
  const Stack::Via& v3 = stack.via(stack.layer("V3"));
  EXPECT_EQ(v3.rpv, 0);
  ASSERT_EQ(v3.rpv_vs_area.x.size(), 2u);
  EXPECT_EQ(v3.rpv_vs_area.y[1], 2);
  EXPECT_EQ(stack.name(v3.upper), "M3");
}

// Blocks the reader parses without using: devices, TSVs, variation and the
// electromigration section, which no braces delimit.
TEST(Itf, SkipsDeviceAndVariationBlocks)
{
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "USE_SI_DENSITY = YES\n"
      "DROP_FACTOR_LATERAL_SPACING = 1\n"
      "DIELECTRIC D1 { THICKNESS=1 ER=4 }\n"
      "CONDUCTOR M1 { THICKNESS=0.2 WMIN=0.1 SMIN=0.1 RPSQ=1\n"
      "  LAYER_TYPE=DIFFUSION DEVICE_TYPE { nmos pmos } IS_PLANAR\n"
      "  GATE_TO_CONTACT_SMIN=0.05 RAISED_DIFFUSION_THICKNESS=0.03\n"
      "  GATE_TO_DIFFUSION_CAP { CONTACT_TO_CONTACT_SPACINGS { 0.1 }\n"
      "    GATE_TO_CONTACT_SPACINGS { 0.1 } CAPS_PER_MICRON { 0.05 } }\n"
      "}\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n"
      "VIA C1 { FROM=SUBSTRATE TO=M1 RPV=10 AREA=0.01\n"
      "  ETCH_VS_CONTACT_AND_GATE_SPACINGS CAPACITIVE_ONLY {\n"
      "    CONTACT_TO_CONTACT_SPACINGS { 0.1 } GATE_TO_CONTACT_SPACINGS { 0.1 "
      "}\n"
      "    VALUES { 0.001 } } }\n"
      "MULTIGATE fin { FIN_WIDTH=0.01 GATE_DIFFUSION_LAYER_PAIR { (M1 M1) } "
      "}\n"
      "TSV t1 { FROM=M1 TO=M1 RHO=1 AREA=1 THICKNESS=10\n"
      "  CSUB_VS_SPACING { (1, 2) } }\n"
      "VARIATION_PARAMETERS { thick = { (M1, THICKNESS, 0.1) } }\n"
      "EM_INFORMATION\n"
      "COMMON_INFO { ITF_EM_VERSION = 1 REFERENCE_TEMPERATURE = 110 }\n"
      "LAYER M1 { stray } brace { without end\n"
      "END_OF_EM_INFORMATION\n");
  EXPECT_EQ(stack.conductors().size(), 1u);
  EXPECT_EQ(stack.conductor(stack.layer("M1")).layer_type, "DIFFUSION");
  EXPECT_EQ(stack.conductor(stack.layer("M1")).rpsq, 1);
  ASSERT_EQ(stack.vias().size(), 1u);
  EXPECT_FALSE(stack.vias()[0].lower.valid());
  EXPECT_EQ(stack.vias()[0].rpv, 10);
}

// Keywords whose effect the solver cannot model are refused by name, and
// the forms the format makes exclusive are checked.
TEST(Itf, RefusesAndChecksResistanceForms)
{
  const std::string head
      = "TECHNOLOGY = t\nDIELECTRIC D { THICKNESS=1 ER=4 }\n";
  const std::string sub = "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n";
  auto conductor = [&](const std::string& body) {
    return head + "CONDUCTOR M { THICKNESS=0.2 WMIN=0.1 SMIN=0.1 " + body
           + " }\n" + sub;
  };
  const std::string table
      = "{ WIDTHS { 0.1 } SPACINGS { 0.1 } VALUES { 0.01 } }";
  expectError(conductor("THICKNESS_VS_WIDTH_AND_SPACING " + table),
              "THICKNESS_VS_WIDTH_AND_SPACING in CONDUCTOR M is not "
              "supported");
  expectError(conductor("FILL_RATIO=0.2"),
              "FILL_RATIO in CONDUCTOR M is not supported");
  expectError(conductor("THICKNESS_VS_DENSITY { (0.1, 0.01) }"),
              "THICKNESS_VS_DENSITY in CONDUCTOR M is not supported");
  expectError(conductor("ETCH_VS_WIDTH_AND_SPACING ETCH_FROM_TOP " + table),
              "ETCH_FROM_TOP");
  expectError(conductor("ETCH_VS_WIDTH_AND_SPACING { NUMBER_OF_MASKS=2 "
                        "MASKS(1,2) WIDTHS { 0.1 } SPACINGS { 0.1 } VALUES { "
                        "0.01 } }"),
              "multiple-mask etch tables (NUMBER_OF_MASKS)");
  expectError(conductor("ETCH_VS_WIDTH_AND_SPACING SIDEWAYS " + table),
              "unsupported option SIDEWAYS of ETCH_VS_WIDTH_AND_SPACING");
  expectError(
      conductor("ETCH_VS_WIDTH_AND_SPACING PARALLEL_TO_REFERENCE " + table),
      "names the reference direction, which the file does not give");
  expectError(conductor("ETCH_VS_WIDTH_AND_SPACING { WIDTHS { 0.1 0.2 } "
                        "SPACINGS { 0.1 } VALUES { 0.01 } }"),
              "has 1 values for 2 widths by 1 spacings");
  expectError(conductor("ETCH_VS_WIDTH_AND_SPACING { WIDTHS { 0.2 0.1 } "
                        "SPACINGS { 0.1 } VALUES { 0.01 0.02 } }"),
              "WIDTHS must be ascending");
  expectError(conductor("ETCH=0.01 CAPACITIVE_ONLY_ETCH=0.01"),
              "ETCH of M cannot be combined");
  expectError(conductor("RPSQ=1 RHO=1"),
              "gives its resistance in more than one form");
  expectError(conductor("RPSQ=1 RPSQ_VS_SI_WIDTH { (0.1, 1) }"),
              "gives its resistance in more than one form");
  expectError(conductor("RPSQ=-1"), "has a negative resistance");
  expectError(conductor("RPSQ_VS_SI_WIDTH { (0.1, -1) }"),
              "has a negative resistance");
  expectError(conductor("RPSQ_VS_SI_WIDTH { (0.1, 1, 2) }"),
              "RPSQ_VS_SI_WIDTH expects 2 numbers per ( )");
  expectError(conductor("RPSQ_VS_SI_WIDTH { 0.1 1 }"),
              "RPSQ_VS_SI_WIDTH expects { (2 numbers) ... }");
  expectError(conductor("CRT1=0.001 CRT_VS_SI_WIDTH { (0.1, 0.001, 0) }"),
              "CRT_VS_SI_WIDTH of M cannot be combined with CRT1 or CRT2");
  expectError(conductor("LAYER_TYPE"), "LAYER_TYPE needs a value");
  expectError(conductor("RPSQ_VS_SI_WIDTH foo { (0.1, 1) }"),
              "RPSQ_VS_SI_WIDTH expects { (2 numbers) ... }");
  expectError(conductor("ETCH_VS_WIDTH_AND_SPACING { WIDTHS foo { 0.1 } "
                        "SPACINGS { 0.1 } VALUES { 0.01 } }"),
              "WIDTHS expects { numbers }");
  expectError(
      head + "DIELECTRIC C { THICKNESS=0 ER=4 IS_CONFORMAL(1, 2) }\n" + sub,
      "IS_CONFORMAL takes no value");
  expectError(head + "DIELECTRIC C { THICKNESS=0 ER=4 IS_CONFORMAL=1 }\n" + sub,
              "IS_CONFORMAL takes no value");
  expectError(conductor("RPSQ=1 RPSQ=2"), "RPSQ given twice in CONDUCTOR M");
  expectError(head + "DIELECTRIC E { THICKNESS=1 ER=4 ER_VS_SI_SPACING { "
                     "(0.1, 4) } }\n" + sub,
              "ER_VS_SI_SPACING in DIELECTRIC E is not supported");
  expectError(head + "DROP_FACTOR_LATERAL_SPACING = 1\n"
                     "DROP_FACTOR_LATERAL_SPACING = 2\n" + sub,
              "DROP_FACTOR_LATERAL_SPACING is given twice");
  expectError(head + "REFERENCE_DIRECTION = UP\n" + sub,
              "REFERENCE_DIRECTION must be VERTICAL or HORIZONTAL");
  expectError(head + "HALF_NODE_SCALE_FACTOR = 0\n" + sub,
              "HALF_NODE_SCALE_FACTOR must be positive");

  const std::string two = head + "CONDUCTOR M2 { THICKNESS=0.2 }\n"
                          + "DIELECTRIC D0 { THICKNESS=0.5 ER=4 }\n"
                          + "CONDUCTOR M1 { THICKNESS=0.2 }\n" + sub;
  expectError(two + "VIA v { FROM=M1 TO=M2 RPV=1 }\n",
              "VIA v needs both RPV and AREA");
  expectError(two + "VIA v { FROM=M1 TO=M2 RPV=1 AREA=0 }\n",
              "VIA v needs a non-negative resistance and a positive area");
  // A zero resistance per via with an area is a zero-ohm via, in both the
  // reader and the stack.
  EXPECT_EQ(
      parse(two + "VIA v { FROM=M1 TO=M2 RPV=0 AREA=0.01 }\n").vias()[0].area,
      0.01);
  expectError(two + "VIA v { FROM=M1 TO=M2 RPV_VS_AREA { (1, -1) } }\n",
              "VIA v has a negative resistance");
  expectError(two + "VIA M1 { FROM=M1 TO=M2 }\n",
              "VIA M1 has the name of a conductor");
  expectError(two + "VIA v { FROM=M1 TO=M2 RPV=1 AREA=1 RHO=1 }\n",
              "VIA v gives its resistance in more than one form");
  expectError(two + "VIA v { FROM=M1 TO=M2 RHO=1 RPV_VS_AREA { (1, 1) } }\n",
              "VIA v gives its resistance in more than one form");
  expectError(two + "VIA v { FROM=M1 TO=M2 CRT1=1 CRT_VS_AREA { (1, 1, 1) } "
                    "}\n",
              "CRT_VS_AREA of v cannot be combined");
  expectError(two + "VIA v { FROM=M1 TO=M2 RPV_VS_AREA { (2, 1) (1, 1) } }\n",
              "RPV_VS_AREA areas must be ascending");
  std::vector<ItfWarning> warnings;
  parse(two + "VIA v { FROM=M1 TO=M2 BOGUS=1 }\n", &warnings);
  ASSERT_EQ(warnings.size(), 1u);
  EXPECT_EQ(warnings[0].message, "unknown keyword BOGUS in VIA v, skipped");
}

// Unknown keywords are skipped with one warning each, with the line; an
// unknown block gets one warning, whatever it holds.
TEST(Itf, SkipsUnknownKeywordsWithAWarning)
{
  std::vector<ItfWarning> warnings;
  const Stack stack = parse(
      "TECHNOLOGY = t\n"
      "FOO = 1\n"
      "NEW_SECTION bar { DEPTH=3 TABLE { (1, 2) } INNER { X=1 } }\n"
      "DIELECTRIC D { THICKNESS=1 ER=4 DAMAGE_ER=3\n"
      "  DAMAGE { WIDTHS { 1 } VALUES { 2 } } }\n"
      "CONDUCTOR M { THICKNESS=0.2 WMIN=0.1 SMIN=0.1\n"
      "  RPSQ_VS_WIDTH_AND_SPACING { SPACINGS { 0.1 } WIDTHS { 0.1 }\n"
      "    VALUES { 0.5 } EXTRA=1 } }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n",
      &warnings);
  EXPECT_EQ(stack.conductor(stack.layer("M")).rpsq_vs_width_spacing.at(0, 0),
            0.5);
  ASSERT_EQ(warnings.size(), 5u);
  EXPECT_EQ(warnings[0].line, 2);
  EXPECT_EQ(warnings[0].message, "unknown keyword FOO, skipped");
  EXPECT_EQ(warnings[1].line, 3);
  EXPECT_EQ(warnings[1].message, "unknown keyword NEW_SECTION bar, skipped");
  EXPECT_EQ(warnings[2].line, 4);
  EXPECT_EQ(warnings[2].message,
            "unknown keyword DAMAGE_ER in DIELECTRIC D, skipped");
  EXPECT_EQ(warnings[3].line, 5);
  EXPECT_EQ(warnings[3].message,
            "unknown keyword DAMAGE in DIELECTRIC D, skipped");
  EXPECT_EQ(warnings[4].line, 8);
  EXPECT_EQ(warnings[4].message,
            "unknown keyword EXTRA in RPSQ_VS_WIDTH_AND_SPACING, skipped");
  // An unknown flag before a known block cannot be told from a block
  // name, so it is an error rather than a silent loss of the block.
  expectError(
      "TECHNOLOGY = t\nNEW_FLAG DIELECTRIC D { THICKNESS=1 ER=4 }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n",
      "unknown keyword NEW_FLAG before DIELECTRIC { ... }");
  expectError(
      "TECHNOLOGY = t\nDIELECTRIC D { THICKNESS=1 ER=4 }\n"
      "CONDUCTOR M { THICKNESS=0.2 NEW_FLAG RPSQ_VS_SI_WIDTH { (0.1, 1) } }\n"
      "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n",
      "unknown keyword NEW_FLAG before RPSQ_VS_SI_WIDTH { ... } in "
      "CONDUCTOR M");
  // Without a sink the warnings are dropped, not raised.
  EXPECT_NO_THROW(
      parse("TECHNOLOGY = t\nFOO = 1\n"
            "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n"));
}

// The syntax pass: malformed structure is reported with its line.
TEST(Itf, SyntaxErrors)
{
  const std::string head = "TECHNOLOGY = t\n";
  const std::string sub = "DIELECTRIC SUB { THICKNESS=0.1 ER=11.9 }\n";
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=4 \n",
              "line 2: unexpected end of file");
  // Numbers and tuples belong in tables, not among a layer's keywords.
  expectError(head + "DIELECTRIC D { THICKNESS=1 0.5 ER=4 }\n" + sub,
              "unexpected number '0.5' in DIELECTRIC D");
  expectError(head + "DIELECTRIC D { THICKNESS=1 (1, 2) ER=4 }\n" + sub,
              "unexpected ( ) in DIELECTRIC D");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=4 } }\n" + sub,
              "line 2: expected a keyword, got '}'");
  expectError(head + "DIELECTRIC D { THICKNESS= ER=4 }\n" + sub,
              "unexpected '=' in DIELECTRIC D");
  expectError(head + "DIELECTRIC D { THICKNESS= }\n" + sub,
              "expected a value for THICKNESS, got '}'");
  expectError(head + "DIELECTRIC { THICKNESS=1 ER=4 }\n" + sub,
              "DIELECTRIC expects a name and { ... }");
  expectError(head + "DIELECTRIC D E { THICKNESS=1 ER=4 }\n" + sub,
              "DIELECTRIC expects a name and { ... }");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=4 ) }\n" + sub,
              "unexpected ')' in DIELECTRIC D");
  expectError(head + "DIELECTRIC D { THICKNESS=1 ER=4 THICKNESS=2 }\n" + sub,
              "THICKNESS given twice in DIELECTRIC D");
  // A flag never opens a block: the table after it is its own statement.
  const Stack planar = parse(
      head + "DIELECTRIC D { THICKNESS=1 ER=4 }\n"
      + "CONDUCTOR M { THICKNESS=0.2 IS_PLANAR ETCH_VS_WIDTH_AND_SPACING { "
        "WIDTHS { 0.1 } SPACINGS { 0.1 } VALUES { 0.01 } } }\n"
      + sub);
  EXPECT_EQ(planar.conductor(planar.layer("M")).etch_tables.size(), 1u);
  expectError(head + "EM_INFORMATION\nLAYER M1 { }\n" + sub,
              "EM_INFORMATION without END_OF_EM_INFORMATION");
  expectError(head + "TSV t = 1\n" + sub, "TSV expects { ... }");
}

}  // namespace
}  // namespace fsv
