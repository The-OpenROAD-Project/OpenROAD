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

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
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
  std::optional<int> DRC_RPT_ITER_STEP = std::nullopt;
  std::string CMAP_FILE;
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
  std::string BOTTOM_ROUTING_LAYER_NAME;
  std::string TOP_ROUTING_LAYER_NAME;
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

  std::string VIAINPIN_BOTTOMLAYER_NAME;
  std::string VIAINPIN_TOPLAYER_NAME;
  frLayerNum VIAINPIN_BOTTOMLAYERNUM = std::numeric_limits<frLayerNum>::max();
  frLayerNum VIAINPIN_TOPLAYERNUM = std::numeric_limits<frLayerNum>::max();

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

  // GR
  int CONGCOST = 8;
  int HISTCOST = 32;

  std::string REPAIR_PDN_LAYER_NAME;
  frLayerNum REPAIR_PDN_LAYER_NUM = -1;
  frLayerNum GC_IGNORE_PDN_LAYER_NUM = -1;
};

constexpr int DIRBITSIZE = 3;
constexpr int WAVEFRONTBUFFERSIZE = 2;
constexpr int WAVEFRONTBITSIZE = (WAVEFRONTBUFFERSIZE * DIRBITSIZE);
constexpr int WAVEFRONTBUFFERHIGHMASK
    = (111 << ((WAVEFRONTBUFFERSIZE - 1) * DIRBITSIZE));

// GR
constexpr int GRWAVEFRONTBUFFERSIZE = 2;
constexpr int GRWAVEFRONTBITSIZE = (GRWAVEFRONTBUFFERSIZE * DIRBITSIZE);
constexpr int GRWAVEFRONTBUFFERHIGHMASK
    = (111 << ((GRWAVEFRONTBUFFERSIZE - 1) * DIRBITSIZE));

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
