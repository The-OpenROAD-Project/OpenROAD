/*
 * Copyright (c) 2022, The Regents of the University of California
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

#include "frBaseTypes.h"

#include <iostream>

namespace drt {

std::ostream& operator<<(std::ostream& os, frMinstepTypeEnum type)
{
  switch (type) {
    case frMinstepTypeEnum::UNKNOWN:
      return os << "UNKNOWN";
    case frMinstepTypeEnum::INSIDECORNER:
      return os << "INSIDECORNER";
    case frMinstepTypeEnum::OUTSIDECORNER:
      return os << "OUTSIDECORNER";
    case frMinstepTypeEnum::STEP:
      return os << "STEP";
  }
  return os << "Bad frMinstepTypeEnum";
}

std::ostream& operator<<(std::ostream& os, frMinimumcutConnectionEnum conn)
{
  switch (conn) {
    case frMinimumcutConnectionEnum::UNKNOWN:
      return os << "UNKNOWN";
    case frMinimumcutConnectionEnum::FROMABOVE:
      return os << "FROMABOVE";
    case frMinimumcutConnectionEnum::FROMBELOW:
      return os << "FROMBELOW";
  }
  return os << "Bad frMinimumcutConnectionEnum";
}

std::ostream& operator<<(std::ostream& os, frCornerTypeEnum type)
{
  switch (type) {
    case frCornerTypeEnum::UNKNOWN:
      return os << "UNKNOWN";
    case frCornerTypeEnum::CONCAVE:
      return os << "CONCAVE";
    case frCornerTypeEnum::CONVEX:
      return os << "CONVEX";
  }
  return os << "Bad frCornerTypeEnum";
}

std::ostream& operator<<(std::ostream& os, frBlockObjectEnum type)
{
  switch (type) {
    case frBlockObjectEnum::frcNet:
      return os << "frcNet";
    case frBlockObjectEnum::frcBTerm:
      return os << "frcBTerm";
    case frBlockObjectEnum::frcMTerm:
      return os << "frcMTerm";
    case frBlockObjectEnum::frcInst:
      return os << "frcInst";
    case frBlockObjectEnum::frcVia:
      return os << "frcVia";
    case frBlockObjectEnum::frcBPin:
      return os << "frcBPin";
    case frBlockObjectEnum::frcMPin:
      return os << "frcMPin";
    case frBlockObjectEnum::frcInstTerm:
      return os << "frcInstTerm";
    case frBlockObjectEnum::frcRect:
      return os << "frcRect";
    case frBlockObjectEnum::frcPolygon:
      return os << "frcPolygon";
    case frBlockObjectEnum::frcSteiner:
      return os << "frcSteiner";
    case frBlockObjectEnum::frcRoute:
      return os << "frcRoute";
    case frBlockObjectEnum::frcPathSeg:
      return os << "frcPathSeg";
    case frBlockObjectEnum::frcGuide:
      return os << "frcGuide";
    case frBlockObjectEnum::frcBlockage:
      return os << "frcBlockage";
    case frBlockObjectEnum::frcLayerBlockage:
      return os << "frcLayerBlockage";
    case frBlockObjectEnum::frcBlock:
      return os << "frcBlock";
    case frBlockObjectEnum::frcMaster:
      return os << "frcMaster";
    case frBlockObjectEnum::frcBoundary:
      return os << "frcBoundary";
    case frBlockObjectEnum::frcInstBlockage:
      return os << "frcInstBlockage";
    case frBlockObjectEnum::frcAccessPattern:
      return os << "frcAccessPattern";
    case frBlockObjectEnum::frcMarker:
      return os << "frcMarker";
    case frBlockObjectEnum::frcNode:
      return os << "frcNode";
    case frBlockObjectEnum::frcPatchWire:
      return os << "frcPatchWire";
    case frBlockObjectEnum::frcRPin:
      return os << "frcRPin";
    case frBlockObjectEnum::frcAccessPoint:
      return os << "frcAccessPoint";
    case frBlockObjectEnum::frcAccessPoints:
      return os << "frcAccessPoints";
    case frBlockObjectEnum::frcPinAccess:
      return os << "frcPinAccess";
    case frBlockObjectEnum::frcCMap:
      return os << "frcCMap";
    case frBlockObjectEnum::frcGCellPattern:
      return os << "frcGCellPattern";
    case frBlockObjectEnum::frcTrackPattern:
      return os << "frcTrackPattern";
    case frBlockObjectEnum::grcNode:
      return os << "grcNode";
    case frBlockObjectEnum::grcNet:
      return os << "grcNet";
    case frBlockObjectEnum::grcPin:
      return os << "grcPin";
    case frBlockObjectEnum::grcAccessPattern:
      return os << "grcAccessPattern";
    case frBlockObjectEnum::grcPathSeg:
      return os << "grcPathSeg";
    case frBlockObjectEnum::grcRef:
      return os << "grcRef";
    case frBlockObjectEnum::grcVia:
      return os << "grcVia";
    case frBlockObjectEnum::drcNet:
      return os << "drcNet";
    case frBlockObjectEnum::drcPin:
      return os << "drcPin";
    case frBlockObjectEnum::drcAccessPattern:
      return os << "drcAccessPattern";
    case frBlockObjectEnum::drcPathSeg:
      return os << "drcPathSeg";
    case frBlockObjectEnum::drcVia:
      return os << "drcVia";
    case frBlockObjectEnum::drcMazeMarker:
      return os << "drcMazeMarker";
    case frBlockObjectEnum::drcPatchWire:
      return os << "drcPatchWire";
    case frBlockObjectEnum::tacTrack:
      return os << "tacTrack";
    case frBlockObjectEnum::tacPin:
      return os << "tacPin";
    case frBlockObjectEnum::tacPathSeg:
      return os << "tacPathSeg";
    case frBlockObjectEnum::tacVia:
      return os << "tacVia";
    case frBlockObjectEnum::gccNet:
      return os << "gccNet";
    case frBlockObjectEnum::gccPin:
      return os << "gccPin";
    case frBlockObjectEnum::gccEdge:
      return os << "gccEdge";
    case frBlockObjectEnum::gccRect:
      return os << "gccRect";
    case frBlockObjectEnum::gccPolygon:
      return os << "gccPolygon";
  }
  return os << "Bad frBlockObjectEnum";
}

std::ostream& operator<<(std::ostream& os, frConstraintTypeEnum type)
{
  switch (type) {
    case frConstraintTypeEnum::frcShortConstraint:
      return os << "frcShortConstraint";
    case frConstraintTypeEnum::frcAreaConstraint:
      return os << "frcAreaConstraint";
    case frConstraintTypeEnum::frcMinWidthConstraint:
      return os << "frcMinWidthConstraint";
    case frConstraintTypeEnum::frcSpacingConstraint:
      return os << "frcSpacingConstraint";
    case frConstraintTypeEnum::frcSpacingEndOfLineConstraint:
      return os << "frcSpacingEndOfLineConstraint";
    case frConstraintTypeEnum::frcSpacingEndOfLineParallelEdgeConstraint:
      return os << "frcSpacingEndOfLineParallelEdgeConstraint";
    case frConstraintTypeEnum::frcSpacingTableConstraint:
      return os << "frcSpacingTableConstraint";
    case frConstraintTypeEnum::frcSpacingTablePrlConstraint:
      return os << "frcSpacingTablePrlConstraint";
    case frConstraintTypeEnum::frcSpacingTableTwConstraint:
      return os << "frcSpacingTableTwConstraint";
    case frConstraintTypeEnum::frcLef58SpacingTableConstraint:
      return os << "frcLef58SpacingTableConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingTableConstraint:
      return os << "frcLef58CutSpacingTableConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingTablePrlConstraint:
      return os << "frcLef58CutSpacingTablePrlConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingTableLayerConstraint:
      return os << "frcLef58CutSpacingTableLayerConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingConstraint:
      return os << "frcLef58CutSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingParallelWithinConstraint:
      return os << "frcLef58CutSpacingParallelWithinConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingAdjacentCutsConstraint:
      return os << "frcLef58CutSpacingAdjacentCutsConstraint";
    case frConstraintTypeEnum::frcLef58CutSpacingLayerConstraint:
      return os << "frcLef58CutSpacingLayerConstraint";
    case frConstraintTypeEnum::frcCutSpacingConstraint:
      return os << "frcCutSpacingConstraint";
    case frConstraintTypeEnum::frcMinStepConstraint:
      return os << "frcMinStepConstraint";
    case frConstraintTypeEnum::frcLef58MinStepConstraint:
      return os << "frcLef58MinStepConstraint";
    case frConstraintTypeEnum::frcMinimumcutConstraint:
      return os << "frcMinimumcutConstraint";
    case frConstraintTypeEnum::frcOffGridConstraint:
      return os << "frcOffGridConstraint";
    case frConstraintTypeEnum::frcMinEnclosedAreaConstraint:
      return os << "frcMinEnclosedAreaConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingConstraint:
      return os << "frcLef58CornerSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingConcaveCornerConstraint:
      return os << "frcLef58CornerSpacingConcaveCornerConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingConvexCornerConstraint:
      return os << "frcLef58CornerSpacingConvexCornerConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingSpacingConstraint:
      return os << "frcLef58CornerSpacingSpacingConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingSpacing1DConstraint:
      return os << "frcLef58CornerSpacingSpacing1DConstraint";
    case frConstraintTypeEnum::frcLef58CornerSpacingSpacing2DConstraint:
      return os << "frcLef58CornerSpacingSpacing2DConstraint";
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineConstraint:
      return os << "frcLef58SpacingEndOfLineConstraint";
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinConstraint:
      return os << "frcLef58SpacingEndOfLineWithinConstraint";
    case frConstraintTypeEnum::frcLef58SpacingEndOfLineWithinEndToEndConstraint:
      return os << "frcLef58SpacingEndOfLineWithinEndToEndConstraint";
    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinEncloseCutConstraint:
      return os << "frcLef58SpacingEndOfLineWithinEncloseCutConstraint";
    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinParallelEdgeConstraint:
      return os << "frcLef58SpacingEndOfLineWithinParallelEdgeConstraint";
    case frConstraintTypeEnum::
        frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint:
      return os << "frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint";
    case frConstraintTypeEnum::frcLef58SpacingWrongDirConstraint:
      return os << "frcLef58SpacingWrongDirConstraint";
    case frConstraintTypeEnum::frcLef58CutClassConstraint:
      return os << "frcLef58CutClassConstraint";
    case frConstraintTypeEnum::frcNonSufficientMetalConstraint:
      return os << "frcNonSufficientMetalConstraint";
    case frConstraintTypeEnum::frcSpacingSamenetConstraint:
      return os << "frcSpacingSamenetConstraint";
    case frConstraintTypeEnum::frcLef58RightWayOnGridOnlyConstraint:
      return os << "frcLef58RightWayOnGridOnlyConstraint";
    case frConstraintTypeEnum::frcLef58RectOnlyConstraint:
      return os << "frcLef58RectOnlyConstraint";
    case frConstraintTypeEnum::frcRecheckConstraint:
      return os << "frcRecheckConstraint";
    case frConstraintTypeEnum::frcSpacingTableInfluenceConstraint:
      return os << "frcSpacingTableInfluenceConstraint";
    case frConstraintTypeEnum::frcLef58EolExtensionConstraint:
      return os << "frcLef58EolExtensionConstraint";
    case frConstraintTypeEnum::frcLef58EolKeepOutConstraint:
      return os << "frcLef58EolKeepOutConstraint";
    case frConstraintTypeEnum::frcLef58MinimumCutConstraint:
      return os << "frcLef58MinimumCutConstraint";
    case frConstraintTypeEnum::frcMetalWidthViaConstraint:
      return os << "frcMetalWidthViaConstraint";
    case frConstraintTypeEnum::frcLef58AreaConstraint:
      return os << "frcLef58AreaConstraint";
    case frConstraintTypeEnum::frcLef58KeepOutZoneConstraint:
      return os << "frcLef58KeepOutZoneConstraint";
    case frConstraintTypeEnum::frcSpacingRangeConstraint:
      return os << "frcSpacingRangeConstraint";
    case frConstraintTypeEnum::frcLef58TwoWiresForbiddenSpcConstraint:
      return os << "frcLef58TwoWiresForbiddenSpcConstraint";
    case frConstraintTypeEnum::frcLef58ForbiddenSpcConstraint:
      return os << "frcLef58ForbiddenSpcConstraint";
    case frConstraintTypeEnum::frcLef58EnclosureConstraint:
      return os << "frcLef58EnclosureConstraint";
    case frConstraintTypeEnum::frcLef58MaxSpacingConstraint:
      return os << "frcLef58MaxSpacingConstraint";
    case frConstraintTypeEnum::frcSpacingTableOrth:
      return os << "frcSpacingTableOrth";
    case frConstraintTypeEnum::frcLef58WidthTableOrth:
      return os << "frcLef58WidthTableOrth";
  }
  return os << "Bad frConstraintTypeEnum";
}

std::ostream& operator<<(std::ostream& os, frDirEnum dir)
{
  switch (dir) {
    case frDirEnum::UNKNOWN:
      return os << "Unknown";
    case frDirEnum::D:
      return os << 'D';
    case frDirEnum::S:
      return os << 'S';
    case frDirEnum::W:
      return os << 'W';
    case frDirEnum::E:
      return os << 'E';
    case frDirEnum::N:
      return os << 'N';
    case frDirEnum::U:
      return os << 'U';
  }
  return os << "Bad frDirEnum";
}

}  // namespace drt
