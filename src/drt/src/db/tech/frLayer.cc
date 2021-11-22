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

using namespace fr;

void frLayer::printAllConstraints(utl::Logger* logger)
{
  for (auto& constraint : constraints) {
    if (auto c = constraint.lock()) {
      c->report(logger);
    }
  }
  for (auto& constraint : lef58CutSpacingTableConstraints) {
    if (auto c = constraint.lock()) {
      c->report(logger);
    }
  }
  for (auto& constraint : lef58SpacingEndOfLineConstraints) {
    if (auto c = constraint.lock()) {
      c->report(logger);
    }
  }
  if (minSpc) {
    minSpc->report(logger);
  }
  if (spacingSamenet) {
    spacingSamenet->report(logger);
  }
  for (auto& constraint : eols) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : cutConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : cutSpacingSamenetConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : interLayerCutSpacingConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : interLayerCutSpacingSamenetConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : lef58CutSpacingConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : lef58CutSpacingSamenetConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  for (auto& constraint : minEnclosedAreaConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  if (areaConstraint) {
    areaConstraint->report(logger);
  }
  if (minStepConstraint) {
    minStepConstraint->report(logger);
  }
  for (auto& constraint : lef58MinStepConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  if (minWidthConstraint) {
    minWidthConstraint->report(logger);
  }
  for (auto& constraint : minimumcutConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
  if (lef58RectOnlyConstraint) {
    lef58RectOnlyConstraint->report(logger);
  }
  if (lef58RightWayOnGridOnlyConstraint) {
    lef58RightWayOnGridOnlyConstraint->report(logger);
  }
  for (auto& constraint : lef58CornerSpacingConstraints) {
    if (constraint) {
      constraint->report(logger);
    }
  }
}
