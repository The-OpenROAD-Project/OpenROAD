/* Author: Matt Liberty */
/*
 * Copyright (c) 2020, The Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the University nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define BOOST_TEST_MODULE gc

#ifdef HAS_BOOST_UNIT_TEST_LIBRARY
// Shared library version
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#else
// Header only version
#include <boost/test/included/unit_test.hpp>
#endif

#include <boost/test/data/test_case.hpp>
#include <iostream>

#include "fixture.h"
#include "frDesign.h"
#include "gc/FlexGC.h"

using namespace fr;
namespace bdata = boost::unit_test::data;

// Fixture for GC tests
struct GCFixture : public Fixture
{
  GCFixture() : worker(design->getTech(), logger.get()) {}

  void testMarker(frMarker* marker,
                  frLayerNum layer_num,
                  frConstraintTypeEnum type,
                  const frBox& expected_bbox)
  {
    frBox bbox;
    marker->getBBox(bbox);

    BOOST_TEST(marker->getLayerNum() == layer_num);
    BOOST_TEST(marker->getConstraint());
    TEST_ENUM_EQUAL(marker->getConstraint()->typeId(), type);
    BOOST_TEST(bbox == expected_bbox);
  }

  void runGC()
  {
    // Needs to be run after all the objects are created but before gc
    initRegionQuery();

    // Run the GC engine
    const frBox work(0, 0, 2000, 2000);
    worker.setExtBox(work);
    worker.setDrcBox(work);

    worker.init(design.get());
    worker.main();
    worker.end();
  }

  FlexGCWorker worker;
};

BOOST_FIXTURE_TEST_SUITE(gc, GCFixture);

// Two touching metal shape from different nets generate a short
BOOST_AUTO_TEST_CASE(metal_short)
{
  // Setup
  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n2, 2, {500, 0}, {1000, 0});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcShortConstraint,
             frBox(500, -50, 500, 50));
}

/*
 *
 *                     |---------------|(750,200)
 *                     |               |
 *                     |               |
 *                     |     i1        |
 *                     |     OBS       |
 *                     |               |
 *                     |****|(550,90)  |
 *                     | in |          |
 * --------------------|----|--(600,50)|
 * |           (450,40)|****| |        |
 * |         n1        |      |        |
 * --------------------|---------------|
 * (0,-50)        (450,-50)
 */
// short with obs
BOOST_AUTO_TEST_CASE(metal_short_obs)
{
  // Setup
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {600, 0});
  auto block = makeMacro("OBS");
  makeMacroObs(block, 450, -50, 750, 200, 2);
  auto pin = makeMacroPin(block, "in", 450, 40, 550, 90, 2);
  auto i1 = makeInst("i1", block, 0, 0);
  auto instTerm = i1->getInstTerms()[0].get();
  instTerm->addToNet(n1);

  n1->addInstTerm(instTerm);
  auto instTermNode = make_unique<frNode>();
  instTermNode->setPin(instTerm);
  instTermNode->setType(frNodeTypeEnum::frcPin);
  n1->addNode(instTermNode);
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 3);
  // short of pin+net (450,-50), (550,90)
  // with obs 450,-50), (750,200)
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcShortConstraint,
             frBox(450, -50, 550, 40));

  // shorts of net (0,-50), (600,50)
  // with obs (450,-50), (750,200)
  // 2 max rectangles generated
  testMarker(markers[1].get(),
             2,
             frConstraintTypeEnum::frcShortConstraint,
             frBox(550, -50, 600, 50));
  testMarker(markers[2].get(),
             2,
             frConstraintTypeEnum::frcShortConstraint,
             frBox(450, -50, 600, 40));
}

// Two touching metal shape from the same net must have sufficient
// overlap
BOOST_AUTO_TEST_CASE(metal_non_sufficient)
{
  // Setup
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {0, 500});
  makePathseg(n1, 2, {0, 0}, {500, 0});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcNonSufficientMetalConstraint,
             frBox(0, 0, 50, 50));
}

// Path seg less than min width flags a violation
BOOST_AUTO_TEST_CASE(min_width)
{
  // Setup
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0}, 60);

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcMinWidthConstraint,
             frBox(0, -30, 500, 30));
}

// Abutting Path seg less than min width don't flag a violation
// as their combined width is ok
BOOST_AUTO_TEST_CASE(min_width_combines_shapes)
{
  // Setup
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0}, 60);
  makePathseg(n1, 2, {0, 60}, {500, 60}, 60);

  runGC();

  // Test the results
  BOOST_TEST(worker.getMarkers().size() == 0);
}

// Check violation for off-grid points
BOOST_AUTO_TEST_CASE(off_grid)
{
  // Setup
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {1, 1}, {501, 1});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(worker.getMarkers().size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcOffGridConstraint,
             frBox(1, -49, 501, 51));
}

// Check violation for corner spacing
BOOST_AUTO_TEST_CASE(corner_basic)
{
  // Setup
  makeCornerConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n1, 2, {500, 200}, {1000, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(worker.getMarkers().size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58CornerSpacingConstraint,
             frBox(500, 50, 500, 150));
}

// Check no violation for corner spacing with EOL spacing
// (same as corner_basic but for eol)
BOOST_AUTO_TEST_CASE(corner_eol_no_violation)
{
  // Setup
  makeCornerConstraint(2, 200);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n1, 2, {500, 200}, {1000, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(worker.getMarkers().size() == 0);
}

// Check no violation for corner spacing with PRL > 0
// (same as corner_basic but for n2's pathseg begin pt)
BOOST_AUTO_TEST_CASE(corner_prl_no_violation)
{
  // Setup
  makeCornerConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n1, 2, {400, 200}, {1000, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(worker.getMarkers().size() == 0);
}

// Check violation for corner spacing on a concave corner
BOOST_AUTO_TEST_CASE(corner_concave, *boost::unit_test::disabled())
{
  // Setup
  makeCornerConstraint(2, /* no eol */ -1, frCornerTypeEnum::CONCAVE);

  frNet* n1 = makeNet("n1");

  makePathsegExt(n1, 2, {0, 0}, {500, 0});
  makePathsegExt(n1, 2, {0, 0}, {0, 500});
  makePathseg(n1, 2, {200, 200}, {1000, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(worker.getMarkers().size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58CornerSpacingConstraint,
             frBox(50, 50, 200, 200));
}

// Check violation for parallel-run-length (PRL) spacing tables
// This test runs over a variety of width / prl / spacing values
// where the spacing is both legal or illegal.
BOOST_DATA_TEST_CASE(spacing_prl,
                     (bdata::make({100, 220}) * bdata::make({300, 500})
                      ^ bdata::make({100, 200, 300, 400}))
                         * bdata::make({true, false}),
                     width,
                     prl,
                     spacing,
                     legal)
{
  // Setup
  makeSpacingConstraint(2);

  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");

  frCoord y = /* n2_width / 2 */ 50 + spacing + width / 2;
  if (!legal) {
    /* move too close making a violation */
    y -= 10;
  }
  makePathseg(n1, 2, {0, y}, {prl, y}, width);
  makePathseg(n2, 2, {0, 0}, {500, 0}, 100);

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  if (legal) {
    BOOST_TEST(worker.getMarkers().size() == 0);
  } else {
    BOOST_TEST(worker.getMarkers().size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcSpacingTablePrlConstraint,
               frBox(0, 50, prl, y - width / 2));
  }
}

// Check violation for spacing two widths with design rule width on macro
// obstruction
BOOST_DATA_TEST_CASE(design_rule_width, bdata::make({true, false}), legal)
{
  // Setup
  makeSpacingTableTwConstraint(2, {90, 190}, {-1, -1}, {{0, 50}, {50, 100}});
  /*
  WIDTH  90     0      50
  WIDTH 190     50    150
  */
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {500, 50}, 100);
  auto block = makeMacro("DRW");
  makeMacroObs(block, 0, 140, 500, 340, 2, legal ? 100 : -1);
  makeInst("i1", block, 0, 0);
  /*
  If DESIGNRULEWIDTH is 100
    width(n1) = 100      width(obs) = 100 : reqSpcVal = 0
  legal

  if DESIGNRULEWIDTH is -1 (undefined)
    width(n1) = 100      width(obs) = 200 : reqSpcVal = 100
  illegal
  */
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcSpacingTableTwConstraint,
               frBox(0, 100, 500, 140));
  }
}

// Check for a min step violation.  The checker seems broken
// so this test is disabled.
BOOST_AUTO_TEST_CASE(min_step, *boost::unit_test::disabled())
{
  // Setup
  makeMinStepConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {200, 0});
  makePathseg(n1, 2, {100, 20}, {200, 20});

  runGC();

  // Test the results
  BOOST_TEST(worker.getMarkers().size() == 1);
}

// Check for a lef58 style min step violation.  The checker is very
// limited and just supports NOBETWEENEOL style.
BOOST_AUTO_TEST_CASE(min_step58)
{
  // Setup
  makeMinStep58Constraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n1, 2, {200, 20}, {300, 20});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58MinStepConstraint,
             frBox(200, 50, 300, 70));
}

// Check for a lef58 rect only violation.  The markers are
// the concave corners expanded by min-width and intersected
// with the metal shapes.
BOOST_AUTO_TEST_CASE(rect_only)
{
  // Setup
  makeRectOnlyConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n1, 2, {200, 0}, {200, 100});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 3);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58RectOnlyConstraint,
             frBox(150, -50, 250, 100));
  testMarker(markers[1].get(),
             2,
             frConstraintTypeEnum::frcLef58RectOnlyConstraint,
             frBox(150, -50, 350, 50));
  testMarker(markers[2].get(),
             2,
             frConstraintTypeEnum::frcLef58RectOnlyConstraint,
             frBox(50, -50, 250, 50));
}

// Check for a min enclosed area violation.
BOOST_AUTO_TEST_CASE(min_enclosed_area)
{
  // Setup
  makeMinEnclosedAreaConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathsegExt(n1, 2, {0, 0}, {200, 0});
  makePathsegExt(n1, 2, {0, 0}, {0, 200});
  makePathsegExt(n1, 2, {0, 200}, {200, 200});
  makePathsegExt(n1, 2, {200, 0}, {200, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcMinEnclosedAreaConstraint,
             frBox(50, 50, 150, 150));
}

// Check for a spacing table influence violation.
BOOST_AUTO_TEST_CASE(spacing_table_infl_vertical)
{
  // Setup
  makeSpacingTableInfluenceConstraint(2, {10}, {{200, 100}});

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {50, 0}, {50, 200});
  makePathseg(n1, 2, {0, 100}, {350, 100});
  makePathseg(n1, 2, {0, 250}, {350, 250});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcSpacingTableInfluenceConstraint,
             frBox(100, 150, 300, 200));
}
// Check for a spacing table influence violation.
BOOST_AUTO_TEST_CASE(spacing_table_infl_horizontal)
{
  // Setup
  makeSpacingTableInfluenceConstraint(2, {10}, {{200, 150}});

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 500}, {500, 500});
  makePathseg(n1, 2, {100, 0}, {100, 500});
  makePathseg(n1, 2, {300, 0}, {300, 500});
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcSpacingTableInfluenceConstraint,
             frBox(150, 250, 250, 450));
}

// Check for a spacing table twowidths violation.
BOOST_AUTO_TEST_CASE(spacing_table_twowidth)
{
  // Setup
  makeSpacingTableTwConstraint(2, {90, 190}, {-1, -1}, {{0, 50}, {50, 100}});

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {500, 50}, 100);
  makePathseg(n1, 2, {0, 240}, {500, 240}, 200);

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcSpacingTableTwConstraint,
             frBox(0, 100, 500, 140));
}

// Check for a basic end-of-line (EOL) spacing violation.
BOOST_DATA_TEST_CASE(eol_basic, (bdata::make({true, false})), lef58)
{
  // Setup
  if (lef58)
    makeLef58SpacingEolConstraint(2);
  else
    makeSpacingEndOfLineConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {500, 0}, {500, 500});
  makePathseg(n1, 2, {0, 700}, {1000, 700});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             lef58 ? frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint
                   : frConstraintTypeEnum::frcSpacingEndOfLineConstraint,
             frBox(450, 500, 550, 650));
}

// Check for eol keepout violation.
BOOST_DATA_TEST_CASE(eol_keepout, (bdata::make({true, false})), legal)
{
  // Setup
  makeLef58EolKeepOutConstraint(2);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {500, 0}, {500, 500});
  frCoord x_extra = 0;
  if (legal)
    x_extra = 200;
  makePathseg(n1, 2, {400 + x_extra, 700}, {600 + x_extra, 700});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcLef58EolKeepOutConstraint,
               frBox(450, 500, 550, 650));
  }
}

// Check for eol keepout violation CORNERONLY.
BOOST_DATA_TEST_CASE(eol_keepout_corner,
                     (bdata::make({true, false}) * bdata::make({true, false})),
                     concave,
                     legal)
{
  // Setup
  makeLef58EolKeepOutConstraint(2, true);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {500, 0}, {500, 500});
  frCoord x_extra = 0;
  if (concave && !legal)
    makePathseg(n1, 2, {360, 400}, {360, 750});
  if (!concave && !legal)
    x_extra = 10;
  makePathseg(n1, 2, {400 + x_extra, 700}, {600 + x_extra, 700});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcLef58EolKeepOutConstraint,
               frBox(410, 500, 450, 650));
  }
}

// Check for an end-of-line (EOL) spacing violation involving one
// parallel edge
BOOST_DATA_TEST_CASE(eol_parallel_edge, (bdata::make({true, false})), lef58)
{
  // Setup
  if (lef58)
    makeLef58SpacingEolParEdgeConstraint(
        makeLef58SpacingEolConstraint(2), 200, 200);
  else
    makeSpacingEndOfLineConstraint(
        2, /* par_space */ 200, /* par_within */ 200);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {500, 0}, {500, 500});
  makePathseg(n1, 2, {0, 700}, {1000, 700});
  makePathseg(n1, 2, {300, 0}, {300, 450});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             lef58 ? frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint
                   : frConstraintTypeEnum::frcSpacingEndOfLineConstraint,
             frBox(450, 500, 550, 650));
}

// Check for an end-of-line (EOL) spacing violation involving two
// parallel edges
BOOST_DATA_TEST_CASE(eol_parallel_two_edge, (bdata::make({true, false})), lef58)
{
  // Setup
  if (lef58)
    makeLef58SpacingEolParEdgeConstraint(
        makeLef58SpacingEolConstraint(2), 200, 200, true);
  else
    makeSpacingEndOfLineConstraint(2,
                                   /* par_space */ 200,
                                   /* par_within */ 200,
                                   /* two_edges */ true);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {500, 0}, {500, 500});
  makePathseg(n1, 2, {0, 700}, {1000, 700});
  makePathseg(n1, 2, {300, 0}, {300, 450});
  makePathseg(n1, 2, {700, 0}, {700, 450});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             lef58 ? frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint
                   : frConstraintTypeEnum::frcSpacingEndOfLineConstraint,
             frBox(450, 500, 550, 650));
}

BOOST_DATA_TEST_CASE(eol_min_max,
                     (bdata::make({true, false}) * bdata::make({true, false})
                      * bdata::make({true, false})),
                     max,
                     twoSides,
                     legal)
{
  makeLef58SpacingEolMinMaxLenConstraint(
      makeLef58SpacingEolConstraint(2), 500, max, twoSides);
  frNet* n1 = makeNet("n1");
  frCoord y = 500;
  if (twoSides)  // both sides need to meet minMax for eolSpacing to be
                 // triggered and one of them need to violate minMax for
                 // eolSpacing to be neglected
  {
    if (max && legal)
      y += 10;  // right(510) > max(500) --> minMax violated --> legal
    else if (!max && !legal)
      y += 100;      // right(600) & left(500) >= min(500) --> minMax is met
                     // --> illegal
  } else if (legal)  // both sides need to violate minMax to have no eolSpacing
                     // violations
  {
    if (max)
      y += 110;  // right(610) & left(510) > max(500)
    else
      y -= 10;  // right(490) & left(390) < min(500)
  }
  makePathseg(n1, 2, {500, 0}, {500, y});
  makePathseg(n1, 2, {0, 700}, {1000, 700});

  makePathseg(n1, 2, {0, 50}, {450, 50});
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    if (markers.size() == 1)
      testMarker(markers[0].get(),
                 2,
                 frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint,
                 frBox(450, y, 550, 650));
  }
}
BOOST_DATA_TEST_CASE(eol_enclose_cut,
                     (bdata::make({0, 350})) ^ (bdata::make({true, false})),
                     y,
                     legal)
{
  addLayer(design->getTech(), "v2", frLayerTypeEnum::CUT);
  addLayer(design->getTech(), "m2", frLayerTypeEnum::ROUTING);
  makeLef58SpacingEolCutEncloseConstraint(makeLef58SpacingEolConstraint(4));
  frNet* n1 = makeNet("n1");
  frViaDef* vd = makeViaDef("v", 3, {0, 0}, {100, 100});

  makePathseg(n1, 4, {500, 0}, {500, 500});
  makePathseg(n1, 4, {0, 700}, {1000, 700});
  auto v = makeVia(vd, n1, {400, y});
  runGC();
  auto& markers = worker.getMarkers();
  if (legal)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    if (markers.size() == 1)
      testMarker(markers[0].get(),
                 4,
                 frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint,
                 frBox(450, 500, 550, 650));
  }
}
BOOST_AUTO_TEST_SUITE_END();
