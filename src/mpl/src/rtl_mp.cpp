// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "mpl/rtl_mp.h"

#include <memory>
#include <string>
#include <vector>

#include "MplObserver.h"
#include "hier_rtlmp.h"
#include "object.h"
#include "odb/db.h"
#include "odb/geom.h"
#include "snapper.h"
#include "utl/Logger.h"
#include "utl/timer.h"

namespace mpl {
using std::string;
using utl::MPL;

class Snapper;

MacroPlacer::MacroPlacer(odb::dbDatabase* db,
                         sta::dbSta* sta,
                         utl::Logger* logger,
                         par::PartitionMgr* tritonpart)
{
  hier_rtlmp_ = std::make_unique<HierRTLMP>(db, logger, tritonpart);
  logger_ = logger;
  db_ = db;
}

MacroPlacer::~MacroPlacer() = default;

bool MacroPlacer::run(int threads, const MacroPlacerOptions& options)
{
  utl::Timer timer;
  if (options.anneal_end_acceptance.has_value()) {
    setAnnealEndAcceptance(*options.anneal_end_acceptance);
  }
  if (options.anneal_patience != 0.0f) {
    setAnnealPatience(options.anneal_patience);
  }
  if (options.placement_seeds != 1) {
    setPlacementSeeds(options.placement_seeds);
  }
  if (options.random_seed != 0) {
    setRandomSeed(options.random_seed);
  }
  hier_rtlmp_->init();
  hier_rtlmp_->configure(threads, options);
  hier_rtlmp_->setGuidanceRegions(guidance_regions_);
  hier_rtlmp_->setRandomSeed(random_seed_);
  random_seed_ = 0;
  hier_rtlmp_->setAnnealEndAcceptance(anneal_end_acceptance_);
  anneal_end_acceptance_ = 0.0f;
  hier_rtlmp_->setAnnealPatience(anneal_patience_);
  anneal_patience_ = 0.0f;
  hier_rtlmp_->setPlacementSeeds(placement_seeds_);
  placement_seeds_ = 1;

  hier_rtlmp_->run();
  logger_->info(MPL, 500, "Runtime: {:.2f}s", timer.elapsed());

  return true;
}

void MacroPlacer::placeMacro(odb::dbInst* inst,
                             const float& x_origin,
                             const float& y_origin,
                             const odb::dbOrientType& orientation,
                             const bool exact,
                             const bool allow_overlap)
{
  odb::dbBlock* block = inst->getBlock();

  const int x1 = block->micronsToDbu(x_origin);
  const int y1 = block->micronsToDbu(y_origin);

  // Orientation must be set before checking if the macro fits in the core and
  // before setting its location.
  inst->setOrient(orientation);

  const int x2 = x1 + inst->getBBox()->getDX();
  const int y2 = y1 + inst->getBBox()->getDY();

  odb::Rect macro_new_bbox(x1, y1, x2, y2);
  odb::Rect core_area = inst->getBlock()->getCoreArea();

  if (!core_area.contains(macro_new_bbox)) {
    logger_->error(MPL,
                   34,
                   "Cannot place {} at ({}, {}) ({}, {}), outside of the core "
                   "({}, {}) ({}, {}).",
                   inst->getName(),
                   block->dbuToMicrons(macro_new_bbox.xMin()),
                   block->dbuToMicrons(macro_new_bbox.yMin()),
                   block->dbuToMicrons(macro_new_bbox.xMax()),
                   block->dbuToMicrons(macro_new_bbox.yMax()),
                   block->dbuToMicrons(core_area.xMin()),
                   block->dbuToMicrons(core_area.yMin()),
                   block->dbuToMicrons(core_area.xMax()),
                   block->dbuToMicrons(core_area.yMax()));
  }

  inst->setLocation(x1, y1);

  if (orientation.isRightAngleRotation()) {
    logger_->warn(MPL,
                  36,
                  "Orientation {} specified for macro {} is a right angle "
                  "rotation. Snapping is not possible.",
                  orientation.getString(),
                  inst->getName());
  } else if (!exact) {
    Snapper snapper(logger_, inst);
    snapper.snapMacro();
  }

  if (!allow_overlap) {
    std::vector<odb::dbInst*> overlapped_macros = findOverlappedMacros(inst);
    if (!overlapped_macros.empty()) {
      std::string overlapped_macros_names;
      for (odb::dbInst* overlapped_macro : overlapped_macros) {
        overlapped_macros_names
            += fmt::format(" {}", overlapped_macro->getName());
      }

      logger_->error(MPL,
                     41,
                     "Couldn't place {}. Found overlap with other macros:{}.",
                     inst->getName(),
                     overlapped_macros_names);
    }
  }

  inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);

  logger_->info(MPL,
                35,
                "Macro {} placed. Bounding box ({:.3f}um, {:.3f}um), "
                "({:.3f}um, {:.3f}um). Orientation {}",
                inst->getName(),
                block->dbuToMicrons(inst->getBBox()->xMin()),
                block->dbuToMicrons(inst->getBBox()->yMin()),
                block->dbuToMicrons(inst->getBBox()->xMax()),
                block->dbuToMicrons(inst->getBBox()->yMax()),
                orientation.getString());
}

std::vector<odb::dbInst*> MacroPlacer::findOverlappedMacros(odb::dbInst* macro)
{
  std::vector<odb::dbInst*> overlapped_macros;
  odb::dbBlock* block = macro->getBlock();
  const odb::Rect& source_macro_bbox = macro->getBBox()->getBox();

  for (odb::dbInst* inst : block->getInsts()) {
    if (inst == macro || !inst->isBlock() || !inst->isPlaced()) {
      continue;
    }

    const odb::Rect& target_macro_bbox = inst->getBBox()->getBox();
    if (source_macro_bbox.overlaps(target_macro_bbox)) {
      overlapped_macros.push_back(inst);
    }
  }

  return overlapped_macros;
}

void MacroPlacer::addGuidanceRegion(odb::dbInst* macro, odb::Rect region)
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  const odb::Rect& core = block->getCoreArea();

  if (!core.contains(region)) {
    logger_->error(MPL,
                   42,
                   "Specified guidance region ({}, {}) ({}, {}) for the macro "
                   "{} is outside of the core ({}, {}) ({}, {}).",
                   region.xMin(),
                   region.yMin(),
                   region.xMax(),
                   region.yMax(),
                   macro->getName(),
                   block->dbuToMicrons(core.xMin()),
                   block->dbuToMicrons(core.yMin()),
                   block->dbuToMicrons(core.xMax()),
                   block->dbuToMicrons(core.yMax()));
  }

  if (guidance_regions_.find(macro) != guidance_regions_.end()) {
    logger_->warn(
        MPL, 44, "Overwriting guidance region for macro {}", macro->getName());
  }

  guidance_regions_[macro] = region;
}

void MacroPlacer::setMinChannelSize(int width, int height)
{
  hier_rtlmp_->setMinChannelSize(width, height);
}

void MacroPlacer::setRandomSeed(const int seed)
{
  random_seed_ = seed;
  hier_rtlmp_->setRandomSeed(seed);
}

void MacroPlacer::setAnnealEndAcceptance(const float end_acceptance)
{
  if (end_acceptance <= 0.0f || end_acceptance >= 0.9f) {
    logger_->error(
        MPL, 81, "-anneal_end_acceptance must be in the range (0.0, 0.9).");
  }
  anneal_end_acceptance_ = end_acceptance;
  hier_rtlmp_->setAnnealEndAcceptance(end_acceptance);
}

void MacroPlacer::setAnnealPatience(const float patience)
{
  if (patience < 0.0f || patience > 1.0f) {
    logger_->error(
        MPL, 82, "-anneal_patience must be in the range [0.0, 1.0].");
  }
  anneal_patience_ = patience;
  hier_rtlmp_->setAnnealPatience(patience);
}

void MacroPlacer::setPlacementSeeds(const int seeds)
{
  if (seeds < 1) {
    logger_->error(MPL, 83, "-placement_seeds must be >= 1.");
  }
  placement_seeds_ = seeds;
  hier_rtlmp_->setPlacementSeeds(seeds);
}

void MacroPlacer::blockMacroChannels()
{
  hier_rtlmp_->blockMacroChannels();
}

void MacroPlacer::configureDebug(MacroPlacerDebugOptions options)
{
  hier_rtlmp_->configureDebug(std::move(options));
}

}  // namespace mpl
