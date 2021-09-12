/* Authors: Lutong Wang and Bangqi Xu */
/*
 * Copyright (c) 2019, The Regents of the University of California
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

#ifndef _FR_BASE_TYPES_H_
#define _FR_BASE_TYPES_H_

#include <boost/geometry/strategies/strategies.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "odb/dbTypes.h"
#include "utl/Logger.h"

namespace fr {
using Logger = utl::Logger;
const utl::ToolId DRT = utl::DRT;
using frLayerNum = int;
using frCoord = int;
using frArea = uint64_t;
using frSquaredDistance = uint64_t;
using frUInt4 = unsigned int;
using frDist = double;
using frString = std::string;
using frCost = unsigned int;
using frMIdx = int;  // negative value expected
template <typename T>
using frCollection = std::vector<T>;
template <typename T>
using frVector = std::vector<T>;
template <typename T>
using frList = std::list<T>;
template <typename T>
using frListIter = typename std::list<T>::iterator;
using dbTechLayerDir = odb::dbTechLayerDir;
using dbTechLayerType = odb::dbTechLayerType;
using dbMasterType = odb::dbMasterType;
using dbSigType = odb::dbSigType;

enum frEndStyleEnum
{
  frcTruncateEndStyle = 0,  // ext = 0
  frcExtendEndStyle = 1,    // ext = half width
  frcVariableEndStyle = 2   // ext = variable
};
enum frBlockObjectEnum
{
  frcNet,
  frcTerm,
  frcInst,
  frcVia,
  frcPin,
  frcInstTerm,
  frcRect,
  frcPolygon,
  frcSteiner,
  frcRoute,
  frcPathSeg,
  frcGuide,
  frcBlockage,
  frcLayerBlockage,
  frcBlock,
  frcBoundary,
  frcInstBlockage,
  frcAccessPattern,
  frcMarker,
  frcNode,
  frcPatchWire,
  frcRPin,
  frcAccessPoint,
  frcAccessPoints,
  frcPinAccess,
  frcCMap,
  frcGCellPattern,
  frcTrackPattern,
  grcNode,
  grcNet,
  grcPin,
  grcAccessPattern,
  grcPathSeg,
  grcRef,
  grcVia,
  drcNet,
  drcPin,
  drcAccessPattern,
  drcPathSeg,
  drcVia,
  drcMazeMarker,
  drcPatchWire,
  tacTrack,
  tacPin,
  tacPathSeg,
  tacVia,
  gccNet,
  gccPin,
  gccEdge,
  gccRect,
  gccPolygon
};
enum class frGuideEnum
{
  frcGuideX,
  frcGuideGlobal,
  frcGuideTrunk,
  frcGuideShortConn
};
enum class frTermDirectionEnum
{
  UNKNOWN,
  INPUT,
  OUTPUT,
  INOUT,
  FEEDTHRU,
};
enum class frNodeTypeEnum
{
  frcSteiner,
  frcBoundaryPin,
  frcPin
};

enum class frConstraintTypeEnum
{  // check FlexDR.h fixMode
  frcShortConstraint = 0,
  frcAreaConstraint = 1,
  frcMinWidthConstraint = 2,
  frcSpacingConstraint = 3,
  frcSpacingEndOfLineConstraint = 4,
  frcSpacingEndOfLineParallelEdgeConstraint = 5,  // not supported
  frcSpacingTableConstraint = 6,                  // not supported
  frcSpacingTablePrlConstraint = 7,
  frcSpacingTableTwConstraint = 8,
  frcLef58SpacingTableConstraint = 9,               // not supported
  frcLef58CutSpacingTableConstraint = 10,           // not supported
  frcLef58CutSpacingTablePrlConstraint = 11,        // not supported
  frcLef58CutSpacingTableLayerConstraint = 12,      // not supported
  frcLef58CutSpacingConstraint = 13,                // not supported
  frcLef58CutSpacingParallelWithinConstraint = 14,  // not supported
  frcLef58CutSpacingAdjacentCutsConstraint = 15,    // not supported
  frcLef58CutSpacingLayerConstraint = 16,           // not supported
  frcCutSpacingConstraint = 17,
  frcMinStepConstraint,
  frcLef58MinStepConstraint,
  frcMinimumcutConstraint,
  frcOffGridConstraint,
  frcMinEnclosedAreaConstraint,
  frcLef58CornerSpacingConstraint,               // not supported
  frcLef58CornerSpacingConcaveCornerConstraint,  // not supported
  frcLef58CornerSpacingConvexCornerConstraint,   // not supported
  frcLef58CornerSpacingSpacingConstraint,        // not supported
  frcLef58CornerSpacingSpacing1DConstraint,      // not supported
  frcLef58CornerSpacingSpacing2DConstraint,      // not supported
  frcLef58SpacingEndOfLineConstraint,
  frcLef58SpacingEndOfLineWithinConstraint,
  frcLef58SpacingEndOfLineWithinEndToEndConstraint,  // not supported
  frcLef58SpacingEndOfLineWithinEncloseCutConstraint,
  frcLef58SpacingEndOfLineWithinParallelEdgeConstraint,
  frcLef58SpacingEndOfLineWithinMaxMinLengthConstraint,
  frcLef58CutClassConstraint,  // not supported
  frcNonSufficientMetalConstraint,
  frcSpacingSamenetConstraint,
  frcLef58RightWayOnGridOnlyConstraint,
  frcLef58RectOnlyConstraint,
  frcRecheckConstraint,
  frcSpacingTableInfluenceConstraint,
  frcLef58EolExtensionConstraint,
  frcLef58EolKeepOutConstraint
};

enum class frCornerTypeEnum
{
  UNKNOWN,
  CONCAVE,
  CONVEX
};

enum class frCornerDirEnum
{
  UNKNOWN,
  NE,
  SE,
  SW,
  NW
};

enum class frMinimumcutConnectionEnum
{
  UNKNOWN = -1,
  FROMABOVE = 0,
  FROMBELOW = 1
};

enum class frMinstepTypeEnum
{
  UNKNOWN = -1,
  INSIDECORNER = 0,
  OUTSIDECORNER = 1,
  STEP = 2
};

#define OPPOSITEDIR 7  // used in FlexGC_main.cpp
enum class frDirEnum
{
  UNKNOWN = 0,
  D = 1,
  S = 2,
  W = 3,
  E = 4,
  N = 5,
  U = 6
};

enum class AccessPointTypeEnum
{
  Ideal,
  Good,
  Offgrid,
  None
};

// note: In ascending cost order for FlexPA
enum class frAccessPointEnum
{
  OnGrid = 0,
  HalfGrid = 1,
  Center = 2,
  EncOpt = 3,
  NearbyGrid = 4  // nearby grid or 1/2 grid
};

namespace bg = boost::geometry;

typedef bg::model::d2::point_xy<frCoord, bg::cs::cartesian> point_t;
typedef bg::model::box<point_t> box_t;
typedef bg::model::segment<point_t> segment_t;

class frBox;

template <typename T>
using rq_box_value_t = std::pair<frBox, T>;

struct frDebugSettings
{
  frDebugSettings()
      : debugDR(false),
        debugMaze(false),
        debugPA(false),
        draw(true),
        allowPause(true),
        gcellX(-1),
        gcellY(-1),
        iter(0),
        paMarkers(false),
        paCombining(false)
  {
  }

  bool is_on() const { return debugDR || debugPA; }

  bool debugDR;
  bool debugMaze;
  bool debugPA;
  bool draw;
  bool allowPause;
  std::string netName;
  std::string pinName;
  int gcellX;
  int gcellY;
  int iter;
  bool paMarkers;
  bool paCombining;
};
}  // namespace fr

#endif
