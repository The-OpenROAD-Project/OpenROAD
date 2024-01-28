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

#pragma once

#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/segment.hpp>
#include <boost/geometry/strategies/strategies.hpp>
#include <boost/serialization/base_object.hpp>
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace boost::serialization {
class access;
}

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
using odb::dbIoType;
using odb::dbMasterType;
using odb::dbSigType;
using odb::dbTechLayerDir;
using odb::dbTechLayerType;

enum frEndStyleEnum
{
  frcTruncateEndStyle = 0,  // ext = 0
  frcExtendEndStyle = 1,    // ext = half width
  frcVariableEndStyle = 2   // ext = variable
};
enum frBlockObjectEnum
{
  frcNet,
  frcBTerm,
  frcMTerm,
  frcInst,
  frcVia,
  frcBPin,
  frcMPin,
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
  frcMaster,
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

std::ostream& operator<<(std::ostream& os, frBlockObjectEnum type);

enum class frGuideEnum
{
  frcGuideX,
  frcGuideGlobal,
  frcGuideTrunk,
  frcGuideShortConn
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
  frcLef58SpacingWrongDirConstraint,
  frcLef58CutClassConstraint,  // not supported
  frcNonSufficientMetalConstraint,
  frcSpacingSamenetConstraint,
  frcLef58RightWayOnGridOnlyConstraint,
  frcLef58RectOnlyConstraint,
  frcRecheckConstraint,
  frcSpacingTableInfluenceConstraint,
  frcLef58EolExtensionConstraint,
  frcLef58EolKeepOutConstraint,
  frcLef58MinimumCutConstraint,
  frcMetalWidthViaConstraint,
  frcLef58AreaConstraint,
  frcLef58KeepOutZoneConstraint,
  frcSpacingRangeConstraint
};

std::ostream& operator<<(std::ostream& os, frConstraintTypeEnum type);

enum class frCornerTypeEnum
{
  UNKNOWN,
  CONCAVE,
  CONVEX
};

std::ostream& operator<<(std::ostream& os, frCornerTypeEnum type);

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

std::ostream& operator<<(std::ostream& os, frMinimumcutConnectionEnum conn);

enum class frMinstepTypeEnum
{
  UNKNOWN = -1,
  INSIDECORNER = 0,
  OUTSIDECORNER = 1,
  STEP = 2
};

std::ostream& operator<<(std::ostream& os, frMinstepTypeEnum type);

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

static constexpr frDirEnum frDirEnumAll[] = {frDirEnum::D,
                                             frDirEnum::S,
                                             frDirEnum::W,
                                             frDirEnum::E,
                                             frDirEnum::N,
                                             frDirEnum::U};

static constexpr frDirEnum frDirEnumPlanar[]
    = {frDirEnum::S, frDirEnum::W, frDirEnum::E, frDirEnum::N};

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

enum class RipUpMode
{
  DRC = 0,
  ALL = 1,
  NEARDRC = 2
};

namespace bg = boost::geometry;

typedef bg::model::d2::point_xy<frCoord, bg::cs::cartesian> point_t;
typedef bg::model::box<point_t> box_t;
typedef bg::model::segment<point_t> segment_t;

template <typename T>
using rq_box_value_t = std::pair<odb::Rect, T>;

struct frDebugSettings
{
  frDebugSettings()
      : debugDR(false),
        debugDumpDR(false),
        debugMaze(false),
        debugPA(false),
        debugTA(false),
        draw(true),
        allowPause(true),
        iter(0),
        paMarkers(false),
        paEdge(false),
        paCommit(false),
        mazeEndIter(-1),
        drcCost(-1),
        markerCost(-1),
        fixedShapeCost(-1),
        markerDecay(-1),
        ripupMode(-1),
        followGuide(-1),
        writeNetTracks(false),
        dumpLastWorker(false)

  {
  }

  bool is_on() const { return debugDR || debugPA; }

  bool debugDR;
  bool debugDumpDR;
  bool debugMaze;
  bool debugPA;
  bool debugTA;
  bool draw;
  bool allowPause;
  std::string netName;
  std::string pinName;
  odb::Rect box{-1, -1, -1, -1};
  int iter;
  bool paMarkers;
  bool paEdge;
  bool paCommit;
  std::string dumpDir;

  int mazeEndIter;
  int drcCost;
  int markerCost;
  int fixedShapeCost;
  float markerDecay;
  int ripupMode;
  int followGuide;
  bool writeNetTracks;
  bool dumpLastWorker;
};

// Avoids the need to split the whole serializer like
// BOOST_SERIALIZATION_SPLIT_MEMBER while still allowing for read/write
// specific code.
template <class Archive>
inline bool is_loading(const Archive& ar)
{
  return std::is_same<typename Archive::is_loading, boost::mpl::true_>::value;
}

using utl::format_as;

}  // namespace fr
