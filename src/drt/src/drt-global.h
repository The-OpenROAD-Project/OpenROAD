// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>

#include "db/obj/frMarker.h"
#include "frBaseTypes.h"

namespace odb {
class Point;
class Rect;
}  // namespace odb

namespace drt {

struct RouterConfiguration
{
  std::string DBPROCESSNODE;
  std::string OUT_MAZE_FILE;
  std::string DRC_RPT_FILE;
  int DRC_RPT_ITER_STEP = 0;  // 0 means disabled
  std::string GUIDE_REPORT_FILE;

  // to be removed
  int OR_SEED = -1;
  double OR_K = 0;

  int MAX_THREADS = 1;
  int BATCHSIZE = 1024;
  int BATCHSIZETA = 8;
  int MTSAFEDIST = 2000;
  int DRCSAFEDIST = 500;
  int VERBOSE = 1;
  int BOTTOM_ROUTING_LAYER = 2;
  int TOP_ROUTING_LAYER = std::numeric_limits<int>::max();
  bool ALLOW_PIN_AS_FEEDTHROUGH = true;
  bool USENONPREFTRACKS = true;
  bool USEMINSPACING_OBS = true;
  bool ENABLE_BOUNDARY_MAR_FIX = true;
  bool ENABLE_VIA_GEN = true;
  bool CLEAN_PATCHES = false;
  bool DO_PA = true;
  bool SINGLE_STEP_DR = false;
  bool SAVE_GUIDE_UPDATES = false;

  // Multi-patterning / mask-coloring awareness. Default OFF: with this
  // false, behavior is exactly as before (no mask logic is active and the
  // check_mask_drc audit refuses to run). Only when true does the
  // mask-aware DRC audit engage. This is the safety gate that guarantees
  // default routing/DRC results are unchanged.
  bool MASK_AWARE_DRC = false;

  // Relaxed different-mask spacing (DBU) used by the check_mask_drc audit.
  // Two shapes of DIFFERENT mask colors on a multi-mask layer are flagged
  // only when their edge-to-edge gap is below this value. Default 0 means
  // different-mask pairs are always legal (down to a short), matching the
  // original audit behavior. Only consulted when MASK_AWARE_DRC is true.
  int MASK_DIFFERENT_SPACING = 0;

  // Conflict-graph mask-coloring solver (multi-patterning slice 5). Default
  // OFF: with this false the solve_mask_coloring command refuses to run and
  // no coloring is ever computed or written, so routed output is unchanged.
  // Only when true does solveMaskColoring build the conflict graph, solve a
  // legal k-coloring, and write the solved MASK colors back to odb.
  bool MASK_COLOR_SOLVE = false;

  // Number of mask colors the solver targets (k in k-coloring). 2 =
  // double-patterning (bipartite), 3 = triple-patterning. Default 2. The
  // solver clamps to a layer's getNumMasks() so it never emits a color the
  // technology does not declare.
  int MASK_NUM_COLORS = 2;

  // Extend the conflict-graph mask-coloring solver to CUT/VIA layers
  // (multi-patterning slice 6). Default OFF: with this false the solver only
  // touches multi-mask ROUTING layers, so its behavior -- including the
  // routed DEF it produces -- is byte-identical to the metal-only solver and
  // no cut MASK tokens are ever emitted. Only when true does the solver build
  // a conflict graph over CUT shapes (vias) on multi-mask CUT layers, solve a
  // legal k-coloring, and write the solved cut MASK colors back to odb.
  // Independent of MASK_COLOR_SOLVE only in scope: this flag is meaningless
  // unless MASK_COLOR_SOLVE is also set (the solver gate still guards the
  // command).
  bool MASK_COLOR_SOLVE_CUTS = false;

  std::string VIAINPIN_BOTTOMLAYER_NAME;
  std::string VIAINPIN_TOPLAYER_NAME;
  frLayerNum VIAINPIN_BOTTOMLAYERNUM = std::numeric_limits<frLayerNum>::max();
  frLayerNum VIAINPIN_TOPLAYERNUM = std::numeric_limits<frLayerNum>::max();

  std::string VIA_ACCESS_LAYER_NAME;
  frLayerNum VIA_ACCESS_LAYERNUM = 2;

  int MINNUMACCESSPOINT_MACROCELLPIN = 3;
  int MINNUMACCESSPOINT_STDCELLPIN = 3;
  int ACCESS_PATTERN_END_ITERATION_NUM = 10;
  float CONGESTION_THRESHOLD = 0.4;
  int MAX_CLIPSIZE_INCREASE = 18;

  int END_ITERATION = 80;

  int NDR_NETS_RIPUP_HARDINESS = 3;  // max ripup avoids
  int CLOCK_NETS_TRUNK_RIPUP_HARDINESS = 100;
  int CLOCK_NETS_LEAF_RIPUP_HARDINESS = 10;
  bool AUTO_TAPER_NDR_NETS = true;
  int TAPERBOX_RADIUS = 3;
  int NDR_NETS_ABS_PRIORITY = 2;
  int CLOCK_NETS_ABS_PRIORITY = 4;

  frUInt4 TAPINCOST = 4;
  frUInt4 TAALIGNCOST = 4;
  frUInt4 TADRCCOST = 32;
  float TASHAPEBLOATWIDTH = 1.5;

  frUInt4 VIACOST = 4;
  // new cost used
  frUInt4 GRIDCOST = 2;
  frUInt4 ROUTESHAPECOST = 8;
  frUInt4 MARKERCOST = 32;
  frUInt4 MARKERBLOATWIDTH = 1;  // unused
  frUInt4 BLOCKCOST = 32;
  frUInt4 GUIDECOST = 1;      // disabled change getNextPathCost to enable
  float SHAPEBLOATWIDTH = 3;  // unused

  std::string REPAIR_PDN_LAYER_NAME;
  frLayerNum REPAIR_PDN_LAYER_NUM = -1;
  frLayerNum GC_IGNORE_PDN_LAYER_NUM = -1;

  // unidirectional layers (stored as layer names)
  std::set<std::string> unidirectional_layer_names_;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
};

constexpr int DIRBITSIZE = 3;
constexpr int WAVEFRONTBUFFERSIZE = 2;
constexpr int WAVEFRONTBITSIZE = (WAVEFRONTBUFFERSIZE * DIRBITSIZE);
constexpr int WAVEFRONTBUFFERHIGHMASK
    = (111 << ((WAVEFRONTBUFFERSIZE - 1) * DIRBITSIZE));

constexpr int LARGE_NET_FANOUT_THRESHOLD = 100;

class drConnFig;
class drNet;
class frBPin;
class frBTerm;
class frBlock;
class frBlockObject;
class frConnFig;
class frGuide;
class frInst;
class frInstTerm;
class frMTerm;
class frMaster;
class frNet;
class frPathSeg;
class frPin;
class frPolygon;
class frRect;
class frShape;
class frTerm;
class frViaDef;

// These need to be in the fr namespace to support argument-dependent
// lookup
std::ostream& operator<<(std::ostream& os, const frViaDef& viaDefIn);
std::ostream& operator<<(std::ostream& os, const frBlock& blockIn);
std::ostream& operator<<(std::ostream& os, const frInst& instIn);
std::ostream& operator<<(std::ostream& os, const frInstTerm& instTermIn);
std::ostream& operator<<(std::ostream& os, const frBTerm& termIn);
std::ostream& operator<<(std::ostream& os, const frRect& pinFig);
std::ostream& operator<<(std::ostream& os, const frPolygon& pinFig);
std::ostream& operator<<(std::ostream& os, const drConnFig& fig);
std::ostream& operator<<(std::ostream& os, const frShape& fig);
std::ostream& operator<<(std::ostream& os, const frConnFig& fig);
std::ostream& operator<<(std::ostream& os, const frPathSeg& p);
std::ostream& operator<<(std::ostream& os, const frGuide& p);
std::ostream& operator<<(std::ostream& os, const frBlockObject& fig);
std::ostream& operator<<(std::ostream& os, const frNet& n);
std::ostream& operator<<(std::ostream& os, const drNet& n);
std::ostream& operator<<(std::ostream& os, const frMarker& m);

using utl::format_as;

}  // namespace drt
