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

#include <iostream>

#include "frBaseTypes.h"

namespace fr {

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

}  // namespace fr
