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

#include <fstream>
#include <iostream>

#include "db/tech/frTechObject.h"
#include "dr/FlexDR.h"
#include "dr/FlexDR_graphics.h"
#include "frDesign.h"
#include "gc/FlexGC.h"
#include "global.h"
#include "gr/FlexGR.h"
#include "gui/gui.h"
#include "io/io.h"
#include "pa/FlexPA.h"
#include "rp/FlexRP.h"
#include "sta/StaMain.hh"
#include "ta/FlexTA.h"
#include "triton_route/TritonRoute.h"

using namespace std;
using namespace fr;
using namespace triton_route;

namespace sta {
// Tcl files encoded into strings.
extern const char* TritonRoute_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Tritonroute_Init(Tcl_Interp* interp);
}

TritonRoute::TritonRoute()
    : debug_(std::make_unique<frDebugSettings>()),
      num_drvs_(-1),
      gui_(gui::Gui::get())
{
}

TritonRoute::~TritonRoute()
{
}

void TritonRoute::setDebugDR(bool on)
{
  debug_->debugDR = on;
}

void TritonRoute::setDebugMaze(bool on)
{
  debug_->debugMaze = on;
}

void TritonRoute::setDebugPA(bool on)
{
  debug_->debugPA = on;
}

void TritonRoute::setDebugNetName(const char* name)
{
  debug_->netName = name;
}

void TritonRoute::setDebugPinName(const char* name)
{
  debug_->pinName = name;
}

void TritonRoute::setDebugGCell(int x, int y)
{
  debug_->gcellX = x;
  debug_->gcellY = y;
}

void TritonRoute::setDebugIter(int iter)
{
  debug_->iter = iter;
}

void TritonRoute::setDebugPaMarkers(bool on)
{
  debug_->paMarkers = on;
}

int TritonRoute::getNumDRVs() const
{
  if (num_drvs_ < 0) {
    logger_->error(DRT, 2, "Detailed routing has not been run yet.");
  }
  return num_drvs_;
}

void TritonRoute::init(Tcl_Interp* tcl_interp,
                       odb::dbDatabase* db,
                       Logger* logger)
{
  db_ = db;
  logger_ = logger;
  design_ = std::make_unique<frDesign>(logger_);
  // Define swig TCL commands.
  Tritonroute_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::TritonRoute_tcl_inits);
  FlexDRGraphics::init();
}

void TritonRoute::init()
{
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    VIAINPIN_BOTTOMLAYERNUM = 2;
    VIAINPIN_TOPLAYERNUM = 2;
    USENONPREFTRACKS = false;
    BOTTOM_ROUTING_LAYER = 4;
    TOP_ROUTING_LAYER = 18;
    ENABLE_VIA_GEN = false;
  }

  io::Parser parser(getDesign(), logger_);
  parser.readDb(db_);

  auto tech = getDesign()->getTech();
  if (!BOTTOM_ROUTING_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(BOTTOM_ROUTING_LAYER_NAME);
    if (layer) {
      BOTTOM_ROUTING_LAYER = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    251,
                    "bottomRoutingLayer {} not found",
                    BOTTOM_ROUTING_LAYER_NAME);
    }
  }

  if (!TOP_ROUTING_LAYER_NAME.empty()) {
    frLayer* layer = tech->getLayer(TOP_ROUTING_LAYER_NAME);
    if (layer) {
      TOP_ROUTING_LAYER = layer->getLayerNum();
    } else {
      logger_->warn(utl::DRT,
                    252,
                    "topRoutingLayer {} not found",
                    TOP_ROUTING_LAYER_NAME);
    }
  }

  if (GUIDE_FILE != string("")) {
    parser.readGuide();
  } else {
    ENABLE_VIA_GEN = false;
  }
  parser.postProcess();
  FlexPA pa(getDesign(), logger_);
  pa.setDebug(debug_.get(), db_);
  pa.main();
  if (GUIDE_FILE != string("")) {
    parser.postProcessGuide();
  }
  // GR-related
  parser.initRPin();
}

void TritonRoute::prep()
{
  FlexRP rp(getDesign(), getDesign()->getTech(), logger_);
  rp.main();
}

void TritonRoute::gr()
{
  FlexGR gr(getDesign(), logger_);
  gr.main(db_);
}

void TritonRoute::ta()
{
  FlexTA ta(getDesign(), logger_);
  ta.main();
}

void TritonRoute::dr()
{
  num_drvs_ = -1;
  FlexDR dr(getDesign(), logger_, db_);
  dr.setDebug(debug_.get());
  dr.main();
}

void TritonRoute::endFR()
{
  io::Writer writer(getDesign(), logger_);
  writer.updateDb(db_);
}

void TritonRoute::reportConstraints()
{
  getDesign()->getTech()->printAllConstraints(logger_);
}

int TritonRoute::main()
{
  MAX_THREADS = ord::OpenRoad::openRoad()->getThreadCount();
  init();
  if (GUIDE_FILE == string("")) {
    gr();
    io::Parser parser(getDesign(), logger_);
    GUIDE_FILE = OUTGUIDE_FILE;
    ENABLE_VIA_GEN = true;
    parser.readGuide();
    parser.initDefaultVias();
    parser.postProcessGuide();
  }
  prep();
  ta();
  dr();
  endFR();

  num_drvs_ = design_->getTopBlock()->getNumMarkers();

  return 0;
}

void TritonRoute::readParams(const string& fileName)
{
  logger_->warn(utl::DRT, 998, "params file is deprecated. Use tcl arguments.");

  int readParamCnt = 0;
  ifstream fin(fileName.c_str());
  string line;
  if (fin.is_open()) {
    while (fin.good()) {
      getline(fin, line);
      if (line[0] != '#') {
        char delimiter = ':';
        int pos = line.find(delimiter);
        string field = line.substr(0, pos);
        string value = line.substr(pos + 1);
        stringstream ss(value);
        if (field == "lef") {
          logger_->warn(utl::DRT, 148, "deprecated lef param in params file");
        } else if (field == "def") {
          logger_->warn(utl::DRT, 227, "deprecated def param in params file");
        } else if (field == "guide") {
          GUIDE_FILE = value;
          ++readParamCnt;
        } else if (field == "outputTA") {
          logger_->warn(
              utl::DRT, 171, "deprecated outputTA param in params file");
        } else if (field == "output") {
          logger_->warn(
              utl::DRT, 205, "deprecated output param in params file");
        } else if (field == "outputguide") {
          OUTGUIDE_FILE = value;
          ++readParamCnt;
        } else if (field == "outputMaze") {
          OUT_MAZE_FILE = value;
          ++readParamCnt;
        } else if (field == "outputDRC") {
          DRC_RPT_FILE = value;
          ++readParamCnt;
        } else if (field == "outputCMap") {
          CMAP_FILE = value;
          ++readParamCnt;
        } else if (field == "threads") {
          logger_->warn(
              utl::DRT, 999, "deprecated threads param in params file."
                             " Use 'set_thread_count'");
          ++readParamCnt;
        } else if (field == "verbose")
          VERBOSE = atoi(value.c_str());
        else if (field == "dbProcessNode") {
          DBPROCESSNODE = value;
          ++readParamCnt;
        } else if (field == "drouteViaInPinBottomLayerNum") {
          VIAINPIN_BOTTOMLAYERNUM = atoi(value.c_str());
          ++readParamCnt;
        } else if (field == "drouteViaInPinTopLayerNum") {
          VIAINPIN_TOPLAYERNUM = atoi(value.c_str());
          ++readParamCnt;
        } else if (field == "drouteEndIterNum") {
          END_ITERATION = atoi(value.c_str());
          ++readParamCnt;
        } else if (field == "OR_SEED") {
          OR_SEED = atoi(value.c_str());
          ++readParamCnt;
        } else if (field == "OR_K") {
          OR_K = atof(value.c_str());
          ++readParamCnt;
        } else if (field == "bottomRoutingLayer") {
          BOTTOM_ROUTING_LAYER_NAME = value;
          ++readParamCnt;
        } else if (field == "topRoutingLayer") {
          TOP_ROUTING_LAYER_NAME = value;
          ++readParamCnt;
        } else if (field == "initRouteShapeCost") {
          ROUTESHAPECOST = atoi(value.c_str());
          ++readParamCnt;
        }
      }
    }
    fin.close();
  }

  if (readParamCnt < 2) {
    logger_->error(DRT, 1, "Error reading param file: {}", fileName);
  }
}

void TritonRoute::setParams(const string& guide_file,
                            const string& output_guide_file,
                            const string& output_maze_file,
                            const string& output_DRC_file,
                            const string& output_CMap_file,
                            const string& dbProcessNode,
                            int drouteEndIterNum,
                            int drouteViaInPinBottomLayerNum,
                            int drouteViaInPinTopLayerNum,
                            int or_seed,
                            double or_k,
                            int bottomRoutingLayer,
                            int topRoutingLayer,
                            int initRouteShapeCost,
                            int verbose)
{
  GUIDE_FILE = guide_file;
  OUTGUIDE_FILE = output_guide_file;
  OUT_MAZE_FILE = output_maze_file;
  DRC_RPT_FILE = output_DRC_file;
  CMAP_FILE = output_CMap_file;
  VERBOSE = verbose;
  DBPROCESSNODE = dbProcessNode;
  if (drouteViaInPinBottomLayerNum > 0) {
    VIAINPIN_BOTTOMLAYERNUM = drouteViaInPinBottomLayerNum;
  }
  if (drouteViaInPinTopLayerNum > 0) {
    VIAINPIN_TOPLAYERNUM = drouteViaInPinTopLayerNum;
  }
  if (drouteEndIterNum > 0) {
    END_ITERATION = drouteEndIterNum;
  }
  OR_SEED = or_seed;
  OR_K = or_k;
  if (bottomRoutingLayer > 0) {
    BOTTOM_ROUTING_LAYER_NAME = bottomRoutingLayer;
  }
  if (topRoutingLayer > 0) {
    TOP_ROUTING_LAYER_NAME = topRoutingLayer;
  }
  if (initRouteShapeCost > 0) {
    ROUTESHAPECOST = static_cast<frUInt4>(initRouteShapeCost);
  }
}

bool fr::isPad(MacroClassEnum e)
{
  return e == MacroClassEnum::PAD || e == MacroClassEnum::PAD_INPUT
         || e == MacroClassEnum::PAD_OUTPUT || e == MacroClassEnum::PAD_INOUT
         || e == MacroClassEnum::PAD_POWER || e == MacroClassEnum::PAD_SPACER
         || e == MacroClassEnum::PAD_AREAIO;
}

bool fr::isEndcap(MacroClassEnum e)
{
  return e == MacroClassEnum::ENDCAP || e == MacroClassEnum::ENDCAP_PRE
         || e == MacroClassEnum::ENDCAP_POST
         || e == MacroClassEnum::ENDCAP_TOPLEFT
         || e == MacroClassEnum::ENDCAP_TOPRIGHT
         || e == MacroClassEnum::ENDCAP_BOTTOMLEFT
         || e == MacroClassEnum::ENDCAP_BOTTOMRIGHT;
}
