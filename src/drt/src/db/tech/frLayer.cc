// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "db/tech/frLayer.h"

#include <algorithm>

namespace drt {

void frLayer::printAllConstraints(utl::Logger* logger)
{
  for (auto& constraint : constraints_) {
    constraint->report(logger);
  }
  if (lef58CutSpacingTableSameNetMetalConstraint_) {
    lef58CutSpacingTableSameNetMetalConstraint_->report(logger);
  }
  if (lef58CutSpacingTableDiffNetConstraint_) {
    lef58CutSpacingTableDiffNetConstraint_->report(logger);
  }
  if (lef58SameNetInterCutSpacingTableConstraint_) {
    lef58SameNetInterCutSpacingTableConstraint_->report(logger);
  }
  if (lef58SameMetalInterCutSpacingTableConstraint_) {
    lef58SameMetalInterCutSpacingTableConstraint_->report(logger);
  }
  if (lef58DefaultInterCutSpacingTableConstraint_) {
    lef58DefaultInterCutSpacingTableConstraint_->report(logger);
  }
  for (auto& constraint : lef58SpacingEndOfLineConstraints_) {
    constraint->report(logger);
  }
  if (minSpc_) {
    minSpc_->report(logger);
  }
  if (spacingSamenet_) {
    spacingSamenet_->report(logger);
  }
  for (auto& constraint : eols_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : cutConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : cutSpacingSamenetConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : interLayerCutSpacingConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : interLayerCutSpacingSamenetConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : lef58CutSpacingConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : lef58CutSpacingSamenetConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : minEnclosedAreaConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  if (areaConstraint_) {
    areaConstraint_->report(logger);
  }
  if (minStepConstraint_) {
    minStepConstraint_->report(logger);
  }
  for (auto& constraint : lef58MinStepConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  if (minWidthConstraint_) {
    minWidthConstraint_->report(logger);
  }
  for (auto& constraint : minimumcutConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  if (lef58RectOnlyConstraint_) {
    lef58RectOnlyConstraint_->report(logger);
  }
  if (lef58RightWayOnGridOnlyConstraint_) {
    lef58RightWayOnGridOnlyConstraint_->report(logger);
  }
  for (auto& constraint : lef58CornerSpacingConstraints_) {
    if (constraint) {
      constraint->report(logger);
    }
  }
}

frCoord frLayer::getMinSpacingValue(frCoord width1,
                                    frCoord width2,
                                    frCoord prl,
                                    bool use_min_spacing)
{
  frCoord rangeSpc = -1;
  if (hasSpacingRangeConstraints()) {
    for (auto con : getSpacingRangeConstraints()) {
      if (use_min_spacing) {
        if (rangeSpc == -1) {
          rangeSpc = con->getMinSpacing();
        } else {
          rangeSpc = std::min(rangeSpc, con->getMinSpacing());
        }
      } else if (con->inRange(width1) || con->inRange(width2)) {
        rangeSpc = std::max(rangeSpc, con->getMinSpacing());
      }
    }
  }

  auto con = getMinSpacing();
  if (!con) {
    return std::max(rangeSpc, 0);
  }
  frCoord minSpc = 0;
  if (con->typeId() == frConstraintTypeEnum::frcSpacingConstraint) {
    minSpc = static_cast<frSpacingConstraint*>(con)->getMinSpacing();
  } else if (con->typeId()
             == frConstraintTypeEnum::frcSpacingTablePrlConstraint) {
    if (use_min_spacing) {
      minSpc = static_cast<frSpacingTablePrlConstraint*>(con)->findMin();
    } else {
      minSpc = static_cast<frSpacingTablePrlConstraint*>(con)->find(
          std::max(width1, width2), prl);
    }
  } else if (con->typeId()
             == frConstraintTypeEnum::frcSpacingTableTwConstraint) {
    if (use_min_spacing) {
      minSpc = static_cast<frSpacingTableTwConstraint*>(con)->findMin();
    } else {
      minSpc = static_cast<frSpacingTableTwConstraint*>(con)->find(
          width1, width2, prl);
    }
  }
  if (use_min_spacing && rangeSpc != -1) {
    return std::min(rangeSpc, minSpc);
  }
  return std::max(rangeSpc, minSpc);
}

}  // namespace drt
