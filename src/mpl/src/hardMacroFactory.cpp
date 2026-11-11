// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "hardMacroFactory.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "mpl-util.h"
#include "object.h"
#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace mpl {

using utl::MPL;

HardMacroFactory::HardMacroFactory(odb::dbBlock* block,
                                   utl::Logger* logger,
                                   const odb::Rect& floorplan_shape,
                                   const Channel min_channel,
                                   const bool pin_aware_channels)
    : block_(block),
      logger_(logger),
      floorplan_shape_(floorplan_shape),
      min_channel_(min_channel),
      pin_aware_channels_(pin_aware_channels)
{
}

InstToHardMap HardMacroFactory::create() const
{
  InstToHardMap inst_to_hard;
  const odb::Rect& core = block_->getCoreArea();
  const int minimum_spacing = getMinimumSpacing();

  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->isBlock()) {
      if (inst->isFixed()) {
        logger_->info(MPL, 62, "Found fixed macro {}.", inst->getName());

        if (!inst->getBBox()->getBox().overlaps(floorplan_shape_)) {
          logger_->info(MPL,
                        63,
                        "{} is outside the macro placement area and will be "
                        "ignored.",
                        inst->getName());
          auto macro = std::make_unique<HardMacro>(inst, HardMacro::Halo());
          macro->setIgnorable(true);
          inst_to_hard[inst] = std::move(macro);
          continue;
        }
      }

      HardMacro::Halo halo = buildHalo(inst, minimum_spacing);

      auto macro = std::make_unique<HardMacro>(inst, halo);

      if (macro->getWidth() > core.dx() || macro->getHeight() > core.dy()) {
        logger_->error(
            MPL,
            6,
            "Found macro that does not fit in the core.\nName: {}\n{}",
            inst->getName(),
            generateMacroAndCoreDimensionsTable(macro.get(), core));
      }

      inst_to_hard[inst] = std::move(macro);
    }
  }

  return inst_to_hard;
}

HardMacro::Halo HardMacroFactory::buildHalo(odb::dbInst* inst,
                                            int minimum_spacing) const
{
  HardMacro::Halo halo(minimum_spacing);
  halo = halo.flooredToChannel(min_channel_);

  if (inst->getHalo() != nullptr) {
    const HardMacro::Halo inst_halo(inst->getHalo());
    halo = halo.flooredToHalo(inst_halo);
  } else if (pin_aware_channels_) {
    halo = buildPinAwareHalo(inst, minimum_spacing);
  }

  // Adjust halo orientation for fixed macros here, since those
  // are skipped during orientation improve and thus never properly adjusted
  if (inst->getPlacementStatus().isFixed()) {
    auto orient = inst->getOrient();
    if (orient == odb::dbOrientType::MX || orient == odb::dbOrientType::R180) {
      std::swap(halo.bottom, halo.top);
    }
    if (orient == odb::dbOrientType::MY || orient == odb::dbOrientType::R180) {
      std::swap(halo.left, halo.right);
    }
  }

  return halo;
}

HardMacro::Halo HardMacroFactory::buildPinAwareHalo(odb::dbInst* inst,
                                                    int minimum_spacing) const
{
  HardMacro::Halo halo(minimum_spacing);
  halo = halo.flooredToChannel(min_channel_);

  HardMacro::Halo min_halo(minimum_spacing);

  odb::dbMaster* master = inst->getMaster();

  for (odb::dbMTerm* mterm : master->getMTerms()) {
    if (mterm->getSigType().isSupply()) {
      continue;
    }

    for (odb::dbMPin* mpin : mterm->getMPins()) {
      for (odb::dbBox* box : mpin->getGeometry()) {
        odb::Rect pin_rect = box->getBox();

        std::vector<std::pair<int, Boundary>> dist_to_boundary{
            {pin_rect.xMin(), Boundary::L},
            {pin_rect.yMin(), Boundary::B},
            {master->getWidth() - pin_rect.xMax(), Boundary::R},
            {master->getHeight() - pin_rect.yMax(), Boundary::T}};

        std::ranges::sort(dist_to_boundary);

        Boundary closest = dist_to_boundary[0].second;

        auto& candidate = dist_to_boundary[0];
        auto& second_candidate = dist_to_boundary[1];

        // When a pin is equally distant from two or more edges (i.e. in the
        // corner) the pin's layer direction is used to choose between
        // candidates
        if (isEquidistantDifferentDirections(candidate, second_candidate)) {
          auto direction
              = (mpin->getGeometry().begin())->getTechLayer()->getDirection();
          if (direction == odb::dbTechLayerDir::VERTICAL) {
            closest = isVertical(candidate.second) ? second_candidate.second
                                                   : candidate.second;
          } else {
            closest = isVertical(candidate.second) ? candidate.second
                                                   : second_candidate.second;
          }
        }

        switch (closest) {
          case Boundary::B:
            min_halo.bottom = halo.bottom;
            break;
          case Boundary::L:
            min_halo.left = halo.left;
            break;
          case Boundary::T:
            min_halo.top = halo.top;
            break;
          case Boundary::R:
            min_halo.right = halo.right;
            break;
        }
      }
    }
  }

  return min_halo;
}

int HardMacroFactory::getMinimumSpacing() const
{
  int spacing = 0;

  for (odb::dbInst* inst : block_->getInsts()) {
    if (inst->isBlock()) {
      odb::dbMaster* master = inst->getMaster();

      for (odb::dbBox* obs : master->getObstructions()) {
        if (auto layer = obs->getTechLayer()) {
          spacing = std::max(spacing, layer->getSpacing());
        }
      }

      for (odb::dbMTerm* mterm : master->getMTerms()) {
        for (odb::dbMPin* mpin : mterm->getMPins()) {
          for (odb::dbBox* geom : mpin->getGeometry()) {
            if (auto layer = geom->getTechLayer()) {
              spacing = std::max(spacing, layer->getSpacing());
            }
          }
        }
      }
    }
  }
  return spacing;
}

bool HardMacroFactory::isEquidistantDifferentDirections(
    std::pair<int, Boundary> candidate,
    std::pair<int, Boundary> second_candidate) const
{
  return (candidate.first == second_candidate.first)
         && ((isVertical(candidate.second)
              && !isVertical(second_candidate.second))
             || (!isVertical(candidate.second)
                 && isVertical(second_candidate.second)));
}

std::string HardMacroFactory::generateMacroAndCoreDimensionsTable(
    const HardMacro* hard_macro,
    const odb::Rect& core) const
{
  std::string table;

  table += fmt::format("\n          |   Macro + Halos   |   Core   ");
  table += fmt::format("\n-----------------------------------------");
  table += fmt::format("\n   Width  | {:>17.2f} | {:>8.2f}",
                       block_->dbuToMicrons(hard_macro->getWidth()),
                       block_->dbuToMicrons(core.dx()));
  table += fmt::format("\n  Height  | {:>17.2f} | {:>8.2f}\n",
                       block_->dbuToMicrons(hard_macro->getHeight()),
                       block_->dbuToMicrons(core.dy()));

  return table;
}

}  // namespace mpl
