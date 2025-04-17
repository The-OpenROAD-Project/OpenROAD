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
#include <vector>
#else
// Header only version
#include <boost/test/included/unit_test.hpp>
#endif

#include <boost/test/data/test_case.hpp>
#include <iostream>

#include "fixture.h"
#include "frDesign.h"
#include "gc/FlexGC.h"
#include "odb/db.h"

namespace drt {

namespace bdata = boost::unit_test::data;

// Fixture for GC tests
struct GCFixture : public Fixture
{
  GCFixture() : worker(design->getTech(), logger.get(), router_cfg.get()) {}

  void testMarker(frMarker* marker,
                  frLayerNum layer_num,
                  frConstraintTypeEnum type,
                  const Rect& expected_bbox)
  {
    Rect bbox = marker->getBBox();

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
    const Rect work(0, 0, 2000, 2000);
    worker.setExtBox(work);
    worker.setDrcBox(work);

    worker.init(design.get());
    worker.main();
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
             Rect(499, -50, 501, 50));
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
  auto [block, master] = makeMacro("OBS");
  makeMacroObs(block, 450, -50, 750, 200, 2);
  makeMacroPin(block, "in", 450, 40, 550, 90, 2);
  auto i1 = makeInst("i1", block, master);
  auto instTerm = i1->getInstTerms()[0].get();
  instTerm->addToNet(n1);

  n1->addInstTerm(instTerm);
  auto instTermNode = std::make_unique<frNode>();
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
             Rect(450, -50, 550, 40));

  // shorts of net (0,-50), (600,50)
  // with obs (450,-50), (750,200)
  // 2 max rectangles generated
  testMarker(markers[1].get(),
             2,
             frConstraintTypeEnum::frcShortConstraint,
             Rect(550, -50, 600, 50));
  testMarker(markers[2].get(),
             2,
             frConstraintTypeEnum::frcShortConstraint,
             Rect(450, -50, 600, 40));
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
             Rect(0, 0, 50, 50));
}

// Path seg less than min width flags a violation
BOOST_DATA_TEST_CASE(min_cut,
                     (bdata::make({1000, 199}) ^ bdata::make({false, true})),
                     spacing,
                     legal)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeMinimumCut(2, 200, 200, spacing);
  frNet* n1 = makeNet("n1");
  frViaDef* vd1 = makeViaDef("v1", 3, {0, 0}, {200, 200});
  makeVia(vd1, n1, {0, 0});
  makePathseg(n1, 2, {0, 100}, {200, 100}, 200);
  makePathseg(n1, 2, {200, 100}, {400, 100}, 100);
  frViaDef* vd2 = makeViaDef("v2", 3, {0, 0}, {100, 100});
  makeVia(vd2, n1, {400, 50});
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcMinimumcutConstraint,
               Rect(200, 50, 400, 150));
  }
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
             Rect(0, -30, 500, 30));
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
             Rect(1, -49, 501, 51));
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
             Rect(500, 50, 500, 150));
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
  BOOST_TEST(worker.getMarkers().size() == 0);
}

BOOST_DATA_TEST_CASE(corner_to_corner, bdata::make({true, false}), legal)
{
  // Setup
  auto con = makeCornerConstraint(2);
  con->setCornerToCorner(legal);

  frNet* n1 = makeNet("n1");
  makePathseg(n1, 2, {0, 0}, {200, 0});
  makePathseg(n1, 2, {350, 250}, {1000, 250});

  runGC();

  // Test the results
  BOOST_TEST(worker.getMarkers().size() == (legal ? 0 : 1));
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
             Rect(50, 50, 200, 200));
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
               Rect(0, 50, prl, y - width / 2));
  }
}

// Check violation for spacing two widths with design rule width on macro
// obstruction
BOOST_DATA_TEST_CASE(design_rule_width, bdata::make({true, false}), legal)
{
  // Setup
  auto dbLayer = db_tech->findLayer("m1");
  dbLayer->initTwoWidths(2);
  dbLayer->addTwoWidthsIndexEntry(90);
  dbLayer->addTwoWidthsIndexEntry(190);
  dbLayer->addTwoWidthsSpacingTableEntry(0, 0, 0);
  dbLayer->addTwoWidthsSpacingTableEntry(0, 1, 50);
  dbLayer->addTwoWidthsSpacingTableEntry(1, 0, 50);
  dbLayer->addTwoWidthsSpacingTableEntry(1, 1, 150);
  makeSpacingTableTwConstraint(2, {90, 190}, {-1, -1}, {{0, 50}, {50, 100}});
  /*
  WIDTH  90     0      50
  WIDTH 190     50    150
  */
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {500, 50}, 100);
  auto [block, master] = makeMacro("DRW");
  makeMacroObs(block, 0, 140, 500, 340, 2, legal ? 100 : -1);
  makeInst("i1", block, master);
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
               Rect(0, 100, 500, 140));
  }
}

// Check for a min step violation.
BOOST_AUTO_TEST_CASE(min_step)
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
BOOST_AUTO_TEST_CASE(min_step58_nobetweeneol)
{
  // Setup
  auto con = makeMinStep58Constraint(2);
  con->setEolWidth(200);

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
             Rect(200, 50, 300, 70));
}

// Check for a lef58 style min step violation.  The checker is very
// limited and just supports NOBETWEENEOL style.
BOOST_AUTO_TEST_CASE(min_step58_minadjlength)
{
  // Setup
  auto con = makeMinStep58Constraint(2);
  con->setMinAdjacentLength(100);
  con->setNoAdjEol(200);
  con->setMaxEdges(1);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 0}, {500, 0});
  makePathseg(n1, 2, {200, -30}, {200, 70});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 2);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58MinStepConstraint,
             Rect(150, 50, 500, 70));
  testMarker(markers[1].get(),
             2,
             frConstraintTypeEnum::frcLef58MinStepConstraint,
             Rect(0, 50, 250, 70));
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
             Rect(150, -50, 250, 100));
  testMarker(markers[1].get(),
             2,
             frConstraintTypeEnum::frcLef58RectOnlyConstraint,
             Rect(150, -50, 350, 50));
  testMarker(markers[2].get(),
             2,
             frConstraintTypeEnum::frcLef58RectOnlyConstraint,
             Rect(50, -50, 250, 50));
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
             Rect(50, 50, 150, 150));
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
             Rect(100, 150, 300, 200));
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
             Rect(150, 250, 250, 450));
}

// Check for a spacing table twowidths violation.
BOOST_AUTO_TEST_CASE(spacing_table_twowidth)
{
  // Setup
  auto dbLayer = db_tech->findLayer("m1");
  dbLayer->initTwoWidths(2);
  dbLayer->addTwoWidthsIndexEntry(90);
  dbLayer->addTwoWidthsIndexEntry(190);
  dbLayer->addTwoWidthsSpacingTableEntry(0, 0, 0);
  dbLayer->addTwoWidthsSpacingTableEntry(0, 1, 50);
  dbLayer->addTwoWidthsSpacingTableEntry(1, 0, 50);
  dbLayer->addTwoWidthsSpacingTableEntry(1, 1, 150);
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
             Rect(0, 100, 500, 140));
}

// Check for a SPACING RANGE violation.
BOOST_DATA_TEST_CASE(spacing_range,
                     bdata::make({0, 200, 200}) ^ bdata::make({200, 200, 100})
                         ^ bdata::make({false, true, false}),
                     minWidth,
                     y,
                     legal)
{
  // Setup
  makeSpacingRangeConstraint(2, 500, minWidth, 400);

  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");

  makePathseg(n1, 2, {0, 50}, {1000, 50});
  makePathseg(n2, 2, {0, y}, {1000, y});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal) {
    BOOST_TEST(markers.size() == 0);
  } else {
    BOOST_TEST(markers.size() == 1);
    if (y == 100) {
      BOOST_TEST(markers[0]->getConstraint()->typeId()
                 == frConstraintTypeEnum::frcShortConstraint);
    } else {
      testMarker(markers[0].get(),
                 2,
                 frConstraintTypeEnum::frcSpacingRangeConstraint,
                 Rect(0, 100, 1000, y - 50));
    }
  }
}

// Check for a SPACING RANGE SAME/DIFF net violation.
BOOST_DATA_TEST_CASE(spacing_range_same_diff_net,
                     bdata::make({true, false}) ^ bdata::make({true, false}),
                     samenet,
                     legal)
{
  // Setup
  makeSpacingRangeConstraint(2, 500, 0, 400);

  frNet* n1 = makeNet("n1");
  frNet* n2 = n1;
  if (!samenet) {
    n2 = makeNet("n2");
  }

  makePathseg(n1, 2, {0, 50}, {1000, 50});
  makePathseg(n2, 2, {0, 200}, {1000, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (legal) {
    BOOST_TEST(markers.size() == 0);
  } else {
    BOOST_TEST(markers.size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcSpacingRangeConstraint,
               Rect(0, 100, 1000, 150));
  }
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
             Rect(450, 500, 550, 650));
}

// Check for a basic end-of-line (EOL) spacing violation.
BOOST_AUTO_TEST_CASE(eol_endtoend)
{
  // Setup
  auto con = makeLef58SpacingEolConstraint(2);
  auto endToEnd
      = std::make_shared<frLef58SpacingEndOfLineWithinEndToEndConstraint>();
  con->getWithinConstraint()->setEndToEndConstraint(endToEnd);
  endToEnd->setEndToEndSpace(300);
  con->getWithinConstraint()->setSameMask(true);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {100, 50});
  makePathseg(n1, 2, {350, 50}, {1000, 50});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint,
             Rect(100, 0, 350, 100));
}
// Check for a basic end-of-line (EOL) spacing violation with extension.
BOOST_AUTO_TEST_CASE(eol_endtoend_ext)
{
  // Setup
  auto con = makeLef58SpacingEolConstraint(2);
  auto endToEnd
      = std::make_shared<frLef58SpacingEndOfLineWithinEndToEndConstraint>();
  endToEnd->setEndToEndSpace(300);
  endToEnd->setExtension(50);
  con->getWithinConstraint()->setEndToEndConstraint(endToEnd);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {100, 50});
  makePathseg(n1, 2, {350, 200}, {1000, 200});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint,
             Rect(100, 100, 350, 150));
}

BOOST_AUTO_TEST_CASE(eol_wrongdirspc)
{
  // Setup
  auto con = makeLef58SpacingEolConstraint(2);
  con->setWrongDirSpace(100);
  db_tech->findLayer("m1")->setDirection(odb::dbTechLayerDir::VERTICAL);
  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {100, 50});
  makePathseg(n1, 2, {250, 50}, {1000, 50});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 0);
}

BOOST_DATA_TEST_CASE(eol_ext_basic,
                     (bdata::make({30, 50})) ^ (bdata::make({true, false})),
                     ext,
                     legal)
{
  // Setup
  makeEolExtensionConstraint(2, 100, {51, 101}, {20, ext}, false);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 100}, {500, 100});
  makePathseg(n1, 2, {690, 100}, {1000, 100});

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
                 frConstraintTypeEnum::frcLef58EolExtensionConstraint,
                 Rect(500, 50, 690, 150));
  }
}

BOOST_AUTO_TEST_CASE(eol_prlend)
{
  // Setup
  makeLef58SpacingEolConstraint(2,    // layer_num
                                200,  // space
                                200,  // width
                                50,   // within
                                400,  // end_prl_spacing
                                50);  // end_prl

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 50}, {100, 50});
  makePathseg(n1, 2, {320, 100}, {1000, 100});

  // Test if you have a non-prl case when you have prl rule
  // this yields no violation
  makePathseg(n1, 2, {0, 500}, {100, 500});
  makePathseg(n1, 2, {320, 500}, {1000, 500});

  // Test if you have a non-prl case when you have prl rule
  // this still yields a violation
  makePathseg(n1, 2, {0, 1000}, {100, 1000});
  makePathseg(n1, 2, {220, 1000}, {1000, 1000});

  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 2);
  testMarker(markers[0].get(),
             2,
             frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint,
             Rect(100, 50, 320, 100));
  testMarker(markers[1].get(),
             2,
             frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint,
             Rect(100, 950, 220, 1050));
}

BOOST_DATA_TEST_CASE(eol_ext_paronly, (bdata::make({true, false})), parOnly)
{
  // Setup
  makeEolExtensionConstraint(2, 100, {101}, {50}, parOnly);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {0, 100}, {500, 100});
  makePathseg(n1, 2, {520, 290}, {910, 290});
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();
  if (parOnly)
    BOOST_TEST(markers.size() == 0);
  else {
    BOOST_TEST(markers.size() == 1);
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcLef58EolExtensionConstraint,
               Rect(500, 150, 520, 240));
  }
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
  makePathseg(n1, 2, {400 + x_extra, 700}, {700 + x_extra, 700});

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
               Rect(450, 500, 550, 650));
  }
}

BOOST_AUTO_TEST_CASE(eol_keepout_except_within)
{
  // Setup
  makeLef58EolKeepOutConstraint(2, false, true);

  frNet* n1 = makeNet("n1");

  makePathseg(n1, 2, {500, 0}, {500, 500});
  makePathseg(n1, 2, {400, 700}, {700, 700});

  runGC();

  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 0);
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
               Rect(410, 500, 450, 650));
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
             Rect(450, 500, 550, 650));
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
             Rect(450, 500, 550, 650));
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
      y += 10;  // right(510) > std::max(500) --> minMax violated --> legal
    else if (!max && !legal)
      y += 100;  // right(600) & left(500) >= std::min(500) --> minMax is met
                 // --> illegal
  } else if (legal)  // both sides need to violate minMax to have no
                     // eolSpacing violations
  {
    if (max)
      y += 110;  // right(610) & left(510) > std::max(500)
    else
      y -= 10;  // right(490) & left(390) < std::min(500)
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
                 Rect(450, y, 550, 650));
  }
}
BOOST_DATA_TEST_CASE(eol_enclose_cut,
                     (bdata::make({0, 350})) ^ (bdata::make({true, false})),
                     y,
                     legal)
{
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeLef58SpacingEolCutEncloseConstraint(makeLef58SpacingEolConstraint(4));
  frNet* n1 = makeNet("n1");
  frViaDef* vd = makeViaDef("v", 3, {0, 0}, {100, 100});

  makePathseg(n1, 4, {500, 0}, {500, 500});
  makePathseg(n1, 4, {0, 700}, {1000, 700});
  makeVia(vd, n1, {400, y});
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
                 Rect(450, 500, 550, 650));
  }
}

BOOST_DATA_TEST_CASE(cut_spc_tbl, (bdata::make({true, false})), viol)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeCutClass(3, "Vx", 100, 200);
  auto layer = db_tech->findLayer("v2");
  auto dbRule = odb::dbTechLayerCutSpacingTableDefRule::create(layer);
  dbRule->setDefault(100);
  dbRule->setVertical(true);
  std::map<std::string, uint> row_map;
  std::map<std::string, uint> col_map;
  std::vector<std::vector<std::pair<int, int>>> table;
  row_map["Vx/SIDE"] = 1;
  row_map["Vx/END"] = 0;
  col_map["Vx/SIDE"] = 1;
  col_map["Vx/END"] = 0;
  if (viol) {
    table.push_back({{300, 300}, {300, 300}});
    table.push_back({{300, 300}, {300, 301}});
  } else {
    table.push_back({{301, 301}, {301, 301}});
    table.push_back({{301, 301}, {301, 300}});
  }

  dbRule->setSpacingTable(
      std::move(table), std::move(row_map), std::move(col_map));
  makeLef58CutSpcTbl(3, dbRule);
  frNet* n1 = makeNet("n1");

  frViaDef* vd = makeViaDef("v", 3, {0, 0}, {200, 100});

  makeVia(vd, n1, {0, 0});
  makeVia(vd, n1, {0, 400});

  runGC();
  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == (viol ? 1 : 0));
}

BOOST_DATA_TEST_CASE(cut_spc_tbl_ex_aligned,
                     (bdata::make({0, 10})) ^ (bdata::make({1, 0})),
                     x,
                     viol)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeCutClass(3, "Vx", 100, 100);
  auto layer = db_tech->findLayer("v2");
  auto dbRule = odb::dbTechLayerCutSpacingTableDefRule::create(layer);
  dbRule->setDefault(200);
  dbRule->addExactElignedEntry("Vx", 250);
  dbRule->setVertical(true);
  makeLef58CutSpcTbl(3, dbRule);
  frNet* n1 = makeNet("n1");

  frViaDef* vd = makeViaDef("v", 3, {0, 0}, {100, 100});

  makeVia(vd, n1, {0, 0});
  makeVia(vd, n1, {x, 300});

  runGC();
  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == viol);
}

BOOST_AUTO_TEST_CASE(metal_width_via_map)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  frViaDef* vd_bar = makeViaDef("V2_BAR", 3, {0, 0}, {100, 100});
  frViaDef* vd_large = makeViaDef("V2_LARGE", 3, {0, 0}, {200, 200});

  auto db_layer = db_tech->findLayer("v2");
  auto dbRule = odb::dbMetalWidthViaMap::create(db_tech);
  dbRule->setAboveLayerWidthLow(300);
  dbRule->setAboveLayerWidthHigh(300);
  dbRule->setBelowLayerWidthLow(300);
  dbRule->setBelowLayerWidthHigh(300);
  dbRule->setCutLayer(db_layer);
  dbRule->setViaName("V2_LARGE");

  makeMetalWidthViaMap(3, dbRule);

  frNet* n1 = makeNet("n1");
  makePathseg(n1, 2, {0, 150}, {1000, 150}, 300);
  makePathseg(n1, 4, {0, 150}, {1000, 150}, 300);
  makeVia(vd_bar, n1, {100, 0});
  makeVia(vd_large, n1, {300, 0});
  runGC();

  // Test the results
  auto& markers = worker.getMarkers();

  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             3,
             frConstraintTypeEnum::frcMetalWidthViaConstraint,
             Rect(100, 0, 200, 100));
}

BOOST_DATA_TEST_CASE(cut_spc_parallel_overlap,
                     (bdata::make({100, 50}) ^ bdata::make({false, true})),
                     spacing,
                     legal)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  frViaDef* vd_bar = makeViaDef("V2_BAR", 3, {0, 0}, {100, 100});

  makeLef58CutSpacingConstraint_parallelOverlap(3, spacing);

  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");
  makeVia(vd_bar, n1, {0, 0});
  makeVia(vd_bar, n2, {150, 0});
  runGC();

  // // Test the results
  auto& markers = worker.getMarkers();
  if (legal) {
    BOOST_TEST(markers.size() == 0);
  } else {
    BOOST_TEST(markers.size() == 1);
  }
  if (!markers.empty()) {
    testMarker(markers[0].get(),
               3,
               frConstraintTypeEnum::frcLef58CutSpacingConstraint,
               Rect(100, 0, 150, 100));
  }
}

BOOST_DATA_TEST_CASE(cut_spc_adjacent_cuts, (bdata::make({true, false})), lef58)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeCutClass(3, "VA", 110, 110);
  if (lef58) {
    makeLef58CutSpacingConstraint_adjacentCut(3, 190, 3, 1, 200);
    frNet* n1 = makeNet("n1");
    frNet* n2 = makeNet("n2");
    frNet* n3 = makeNet("n3");
    frNet* n4 = makeNet("n4");
    frViaDef* vd = makeViaDef("v", 3, {0, 0}, {110, 110});

    makeVia(vd, n1, {1000, 1000});
    makeVia(vd, n2, {1000, 820});
    makeVia(vd, n3, {820, 1000});
    makeVia(vd, n4, {1000, 1180});

    runGC();

    // Test the results
    auto& markers = worker.getMarkers();

    BOOST_TEST(markers.size() == 3);
  }
}

BOOST_AUTO_TEST_CASE(cut_keepoutzone)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  frViaDef* vd_bar = makeViaDef("V2_BAR", 3, {0, 0}, {200, 150});
  makeCutClass(3, "Vx", 150, 200);

  auto db_layer = db_tech->findLayer("v2");
  auto dbRule = odb::dbTechLayerKeepOutZoneRule::create(db_layer);
  dbRule->setFirstCutClass("Vx");
  dbRule->setEndSideExtension(51);
  dbRule->setEndForwardExtension(51);
  dbRule->setSideSideExtension(51);
  dbRule->setSideForwardExtension(51);
  makeKeepOutZoneRule(3, dbRule);

  frNet* n1 = makeNet("n1");
  makeVia(vd_bar, n1, {0, 0});
  makeVia(vd_bar, n1, {150, 200});
  runGC();

  // // Test the results
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  testMarker(markers[0].get(),
             3,
             frConstraintTypeEnum::frcLef58KeepOutZoneConstraint,
             Rect(150, 150, 200, 200));
}

BOOST_DATA_TEST_CASE(route_wrong_direction_spc,
                     (bdata::make({100, 50, 100, 100})
                      ^ bdata::make({false, true, true, true})
                      ^ bdata::make({false, false, true, false})
                      ^ bdata::make({150, 150, 150, 150})
                      ^ bdata::make({0, 0, 0, 50})),
                     spacing,
                     legal,
                     noneolValid,
                     noneolWidth,
                     prlLength)
{
  // Setup
  auto db_layer = db_tech->findLayer("m1");
  db_layer->setDirection(odb::dbTechLayerDir::VERTICAL);
  auto dbRule = odb::dbTechLayerWrongDirSpacingRule::create(db_layer);
  dbRule->setWrongdirSpace(spacing);
  if (noneolValid) {
    dbRule->setNoneolValid(noneolValid);
    dbRule->setNoneolWidth(noneolWidth);
  }
  if (prlLength != 0) {
    dbRule->setPrlLength(prlLength);
  }

  makeLef58WrongDirSpcConstraint(2, dbRule);

  frNet* n1 = makeNet("n1");
  frNet* n2 = makeNet("n2");
  makePathseg(n1, 2, {0, 50}, {100, 50}, 100);
  if (prlLength != 0) {
    makePathseg(n2, 2, {50, 200}, {150, 200}, 100);
  } else {
    makePathseg(n2, 2, {0, 200}, {100, 200}, 100);
  }

  runGC();

  // // Test the results
  auto& markers = worker.getMarkers();
  if (legal) {
    BOOST_TEST(markers.size() == 0);
  } else {
    BOOST_TEST(markers.size() == 1);
  }
  if (!markers.empty()) {
    testMarker(markers[0].get(),
               2,
               frConstraintTypeEnum::frcLef58SpacingWrongDirConstraint,
               Rect(0, 100, 100, 150));
  }
}

BOOST_AUTO_TEST_CASE(twowires_forbidden_spc)
{
  // Setup
  auto db_layer = db_tech->findLayer("m1");
  auto rule = odb::dbTechLayerTwoWiresForbiddenSpcRule::create(db_layer);
  rule->setMinSpacing(0);
  rule->setMaxSpacing(300);
  rule->setMinSpanLength(0);
  rule->setMaxSpanLength(500);
  rule->setPrl(0);
  makeLef58TwoWiresForbiddenSpc(2, rule);
  frNet* n1 = makeNet("n1");
  makePathseg(n1, 2, {0, 50}, {500, 50});
  makePathseg(n1, 2, {0, 200}, {500, 200});

  runGC();

  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
}

BOOST_AUTO_TEST_CASE(forbidden_spc)
{
  // Setup
  auto db_layer = db_tech->findLayer("m1");
  auto rule = odb::dbTechLayerForbiddenSpacingRule::create(db_layer);
  rule->setForbiddenSpacing({550, 800});
  rule->setPrl(1);
  rule->setWidth(300);
  rule->setTwoEdges(300);
  makeLef58ForbiddenSpc(2, rule);
  frNet* n1 = makeNet("n1");
  makePathseg(n1, 2, {0, 50}, {500, 50});
  makePathseg(n1, 2, {0, 700}, {500, 700});
  // wire in between
  makePathseg(n1, 2, {0, 300}, {500, 300});
  // wire above
  makePathseg(n1, 2, {0, 900}, {500, 900});

  runGC();

  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
}

BOOST_AUTO_TEST_CASE(lef58_enclosure)
{
  // Setup
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeCutClass(3, "Vx", 100, 200);
  makeLef58EnclosureConstrainut(3, 0, 0, 0, 0);
  makeLef58EnclosureConstrainut(3, 0, 200, 100, 50);

  frViaDef* vd = makeViaDef("v", 3, {0, 0}, {200, 100});

  frNet* n1 = makeNet("n1");
  makeVia(vd, n1, {0, 0});
  makePathseg(n1, 4, {-50, 50}, {250, 50}, 200);

  runGC();
  // BELOW ENC VALID, ABOVE ENCLOSURE VIOLATING
  auto& markers = worker.getMarkers();
  BOOST_TEST(markers.size() == 1);
  if (!markers.empty()) {
    testMarker(markers[0].get(),
               4,
               frConstraintTypeEnum::frcLef58EnclosureConstraint,
               Rect(0, 0, 200, 100));
  }
}

BOOST_DATA_TEST_CASE(cut_spc_tbl_orth,
                     (bdata::make({true, false}) ^ bdata::make({140, 150})),
                     violating,
                     y)
{
  addLayer(design->getTech(), "v2", dbTechLayerType::CUT);
  addLayer(design->getTech(), "m2", dbTechLayerType::ROUTING);
  makeSpacingTableOrthConstraint(3, 150, 50);
  frViaDef* vd = makeViaDef("v", 3, {0, 0}, {100, 100});
  frNet* n1 = makeNet("n1");
  makeVia(vd, n1, {0, 0});
  makeVia(vd, n1, {0, 240});
  makeVia(vd, n1, {y, 110});
  runGC();
  auto& markers = worker.getMarkers();
  if (violating) {
    BOOST_TEST(markers.size() == 1);
  } else {
    BOOST_TEST(markers.size() == 0);
  }
}

BOOST_DATA_TEST_CASE(width_tbl_orth,
                     (bdata::make({40, 50, 60}) * bdata::make({40, 50, 60})),
                     horz_spc,
                     vert_spc)
{
  makeWidthTblOrthConstraint(2, horz_spc, vert_spc);
  design->getTech()->getLayer(2)->setMinWidth(
      10);  // to ignore NSMetal violations
  frNet* n1 = makeNet("n1");
  makePathseg(n1, 2, {0, 100}, {200, 100}, 100);
  makePathseg(n1, 2, {150, 50}, {350, 50}, 100);
  const frCoord dx = 50;
  const frCoord dy = 50;
  const bool violating = dx < horz_spc && dy < vert_spc;
  runGC();
  auto& markers = worker.getMarkers();
  if (violating) {
    BOOST_TEST(markers.size() == 1);
  } else {
    BOOST_TEST(markers.size() == 0);
  }
}

BOOST_AUTO_TEST_SUITE_END();

}  // namespace drt
