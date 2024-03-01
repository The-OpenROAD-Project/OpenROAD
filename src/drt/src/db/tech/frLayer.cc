/* Authors: Matt Liberty */
/*
 * Copyright (c) 2021, The Regents of the University of California
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

#include "db/tech/frLayer.h"

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
