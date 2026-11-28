// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include <memory>
#include <utility>
#include <vector>

#include "db/obj/frBlock.h"
#include "db/obj/frBoundary.h"
#include "db/obj/frGCellPattern.h"
#include "db/obj/frGuide.h"
#include "db/obj/frNet.h"
#include "db/obj/frShape.h"
#include "db/obj/frTrackPattern.h"
#include "db/tech/frConstraint.h"
#include "fixture.h"
#include "frBaseTypes.h"
#include "frDesign.h"
#include "frRegionQuery.h"
#include "gtest/gtest.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "pa/FlexPA.h"
#include "ta/FlexTA.h"

using odb::dbTechLayerDir;
using odb::dbTechLayerType;

namespace drt {

// Fixture for FlexTAWorker tests. Adds TA-specific scaffolding (die area,
// gcell patterns, track patterns, guides) on top of the shared Fixture.
struct TAFixture : public Fixture
{
  void setDieArea(frCoord xl, frCoord yl, frCoord xh, frCoord yh)
  {
    frBoundary boundary;
    std::vector<odb::Point> pts = {{xl, yl}, {xh, yl}, {xh, yh}, {xl, yh}};
    boundary.setPoints(pts);
    std::vector<frBoundary> boundaries{boundary};
    design->getTopBlock()->setBoundaries(boundaries);
  }

  void createGCellPattern(frUInt4 gcell_width, frUInt4 gcell_height)
  {
    const odb::Rect die = design->getTopBlock()->getDieBox();
    if (die.dx() % gcell_width != 0) {
      logger->error(DRT, 185, "die width is not divisible by gcell width");
    }
    if (die.dy() % gcell_height != 0) {
      logger->error(DRT, 188, "die height is not divisible by gcell height");
    }
    design->getTopBlock()->setGCellPatterns(
        {frGCellPattern(/*horizontal=*/true,
                        die.xMin(),
                        gcell_width,
                        die.dx() / gcell_width),
         frGCellPattern(/*horizontal=*/false,
                        die.yMin(),
                        gcell_height,
                        die.dy() / gcell_height)});
  }
  // is_horizontal_pattern follows the frTrackPattern convention: a pattern
  // marked horizontal=true generates vertical tracks (varying x) and
  // horizontal=false generates horizontal tracks (varying y).
  void addTrackPattern(frLayerNum layer_num,
                       bool is_horizontal_pattern,
                       frCoord start_coord,
                       frUInt4 num_tracks,
                       frUInt4 spacing)
  {
    design->getTopBlock()->addTrackPattern(std::make_unique<frTrackPattern>(
        is_horizontal_pattern, start_coord, num_tracks, spacing, layer_num));
  }

  frGuide* makeGuide(frNet* net,
                     frLayerNum layer_num,
                     const odb::Point& begin,
                     const odb::Point& end)
  {
    auto guide = std::make_unique<frGuide>(begin, layer_num, end);
    auto* ptr = guide.get();
    net->addGuide(std::move(guide));
    return ptr;
  }

  void initTAQueries()
  {
    initRegionQuery();
    design->getRegionQuery()->initGuide();
  }

  void runHorizontalInitTA()
  {
    FlexTAWorker worker(design.get(),
                        logger.get(),
                        router_cfg.get(),
                        /*save_updates=*/false);
    worker.setRouteBox(design->getTopBlock()->getDieBox());
    worker.setExtBox(design->getTopBlock()->getDieBox());
    worker.setDir(odb::dbTechLayerDir::HORIZONTAL);
    worker.setTAIter(0);
    worker.main_mt();
    worker.end();
    EXPECT_EQ(worker.getNumAssigned(), 2);
  }
};

// A single guide on a horizontal routing layer should be assigned to one of
// the available horizontal tracks during initial track assignment.
TEST_F(TAFixture, single_horizontal_iroute_assigned)
{
  // m1 (layer 2) is set up by the Fixture as HORIZONTAL. Horizontal tracks
  // run along x, varying in y -> use is_horizontal_pattern=false.
  setDieArea(0, 0, 2000, 2000);
  createGCellPattern(1000, 1000);
  addTrackPattern(/*layer_num=*/2,
                  /*is_horizontal_pattern=*/false,
                  /*start_coord=*/100,
                  /*num_tracks=*/10,
                  /*spacing=*/200);

  frNet* n1 = makeNet("n1");
  frGuide* guide = makeGuide(n1, /*layer_num=*/2, {500, 500}, {1500, 500});

  initTAQueries();

  FlexTAWorker worker(
      design.get(), logger.get(), router_cfg.get(), /*save_updates=*/false);
  worker.setRouteBox(design->getTopBlock()->getDieBox());
  worker.setExtBox(design->getTopBlock()->getDieBox());
  worker.setDir(odb::dbTechLayerDir::HORIZONTAL);
  worker.setTAIter(0);

  EXPECT_TRUE(worker.isInitTA());
  EXPECT_EQ(worker.getNumAssigned(), 0);

  worker.main_mt();
  worker.end();

  EXPECT_EQ(worker.getNumAssigned(), 1);

  // Tracks built by initTracks() from the pattern above: y = 100, 300, ...
  // 1900.
  const std::vector<frCoord> expected_tracks
      = {100, 300, 500, 700, 900, 1100, 1300, 1500, 1700, 1900};
  EXPECT_EQ(worker.getTrackLocs(2), expected_tracks);

  // After end(), the guide carries the assigned frPathSeg. Its y-coord must
  // match one of the available tracks.
  ASSERT_EQ(guide->getRoutes().size(), 1);
  const auto* path_seg
      = static_cast<frPathSeg*>(guide->getRoutes().front().get());
  ASSERT_EQ(path_seg->typeId(), frcPathSeg);
  const auto [bp, ep] = path_seg->getPoints();
  EXPECT_EQ(bp.y(), ep.y());
  // assignIroute_bestTrack picks the midpoint track (idx 2, y=500) from the
  // five tracks in gcell (0,0,1000,1000).
  EXPECT_EQ(bp.y(), expected_tracks[2]);
}

TEST_F(TAFixture, two_parallel_no_coupling)
{
  setDieArea(0, 0, 21000, 2000);
  createGCellPattern(1000, 1000);
  addTrackPattern(/*layer_num=*/2,
                  /*is_horizontal_pattern=*/false,
                  /*start_coord=*/100,
                  /*num_tracks=*/19,
                  /*spacing=*/100);
  makeSimpleSpacingConstraint(2, 100);

  frGuide* g1
      = makeGuide(makeNet("n1"), /*layer_num=*/2, {500, 500}, {5500, 500});
  frGuide* g2
      = makeGuide(makeNet("n2"), /*layer_num=*/2, {500, 500}, {5500, 500});
  initTAQueries();

  runHorizontalInitTA();
  const auto* path_seg1
      = static_cast<frPathSeg*>(g1->getRoutes().front().get());
  const auto* path_seg2
      = static_cast<frPathSeg*>(g2->getRoutes().front().get());
  EXPECT_EQ(path_seg1->getPoints().first.y(), 500);
  EXPECT_EQ(path_seg2->getPoints().first.y(), 700);
}

TEST_F(TAFixture, pa_off_track_pin_with_min_step)
{
  auto* tech = design->getTech();
  addLayer(tech, "v1", dbTechLayerType::CUT);
  addLayer(tech, "m2", dbTechLayerType::ROUTING, dbTechLayerDir::VERTICAL);
  addTrackPattern(2, false, 500, 3, 200);
  addTrackPattern(4, true, 500, 3, 200);
  tech->getLayer(3)->addViaDef(makeViaDef("V1", 3, {-50, -50}, {50, 50}));
  frLef58RightWayOnGridOnlyConstraint rwo;
  tech->getLayer(4)->setLef58RightWayOnGridOnlyConstraint(&rwo);
  makeMinStep58Constraint(4)->setMinAdjacentLength(200);
  router_cfg->VIAINPIN_BOTTOMLAYERNUM = router_cfg->VIAINPIN_TOPLAYERNUM = 2;
  router_cfg->VIA_ACCESS_LAYERNUM = 2;
  auto [blk, db_m] = makeMacro("C", 0, 0, 1000, 1000);
  makeMacroPin(blk, "a", 150, 450, 250, 550, 2);
  makeInst("i1", blk, db_m)->getInstTerms()[0]->addToNet(makeNet("n1"));
  initRegionQuery();
  FlexPA pa(design.get(), logger.get(), nullptr, router_cfg.get());
  EXPECT_NO_THROW(pa.main());
}

TEST_F(TAFixture, pa_conflicting_pins_fallback_pattern)
{
  auto* tech = design->getTech();
  addLayer(tech, "v1", dbTechLayerType::CUT);
  addLayer(tech, "m2", dbTechLayerType::ROUTING, dbTechLayerDir::VERTICAL);
  addTrackPattern(2, false, 500, 3, 200);
  addTrackPattern(4, true, 500, 3, 200);
  tech->getLayer(3)->addViaDef(makeViaDef("V1", 3, {-50, -50}, {50, 50}));
  router_cfg->VIA_ACCESS_LAYERNUM = 2;
  auto [blk, db_m] = makeMacro("C", 0, 0, 1000, 1000);
  makeMacroPin(blk, "a", 450, 450, 550, 550, 2);
  makeMacroPin(blk, "b", 500, 450, 600, 550, 2);
  frInst* inst = makeInst("i1", blk, db_m);
  inst->getInstTerms()[0]->addToNet(makeNet("n1"));
  inst->getInstTerms()[1]->addToNet(makeNet("n2"));
  initRegionQuery();
  FlexPA pa(design.get(), logger.get(), nullptr, router_cfg.get());
  EXPECT_NO_THROW(pa.main());
}

TEST_F(TAFixture, pa_nearby_grid_skips_half_track)
{
  auto* tech = design->getTech();
  addLayer(tech, "v1", dbTechLayerType::CUT);
  addLayer(tech, "m2", dbTechLayerType::ROUTING, dbTechLayerDir::VERTICAL);
  addTrackPattern(2, false, 500, 3, 200);
  addTrackPattern(4, true, 100, 2, 800);
  tech->getLayer(3)->addViaDef(makeViaDef("V1", 3, {-50, -50}, {50, 50}));
  frLef58RightWayOnGridOnlyConstraint rwo;
  tech->getLayer(4)->setLef58RightWayOnGridOnlyConstraint(&rwo);
  router_cfg->VIAINPIN_BOTTOMLAYERNUM = router_cfg->VIAINPIN_TOPLAYERNUM = 2;
  auto [blk, db_m] = makeMacro("C", 0, 0, 1200, 1000);
  auto* term = makeMacroPin(blk, "a", 620, 450, 720, 550, 2);
  makeInst("i1", blk, db_m)->getInstTerms()[0]->addToNet(makeNet("n1"));
  initRegionQuery();
  FlexPA(design.get(), logger.get(), nullptr, router_cfg.get()).main();
  const auto* pa = static_cast<frMTerm*>(term)->getPins()[0]->getPinAccess(0);
  EXPECT_EQ(pa->getAccessPoint(0)->getPoint().x(), 900);
}

}  // namespace drt
