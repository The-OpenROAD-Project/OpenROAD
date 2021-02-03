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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <fstream>
#include "global.h"
#include "triton_route/TritonRoute.h"
#include "io/io.h"
#include "pa/FlexPA.h"
#include "ta/FlexTA.h"
#include "dr/FlexDR.h"
#include "gc/FlexGC.h"
#include "gr/FlexGR.h"
#include "rp/FlexRP.h"
#include "sta/StaMain.hh"

using namespace std;
using namespace fr;
using namespace triton_route;

namespace sta {
// Tcl files encoded into strings.
extern const char *triton_route_tcl_inits[];
}

extern "C" {
extern int Triton_route_Init(Tcl_Interp* interp);
}

TritonRoute::TritonRoute()
  : debug_(std::make_unique<frDebugSettings>()),
    num_drvs_(-1)
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

void TritonRoute::init(Tcl_Interp* tcl_interp, odb::dbDatabase* db, Logger* logger)
{
  db_ = db;
  logger_ = logger;
  design_ = std::make_unique<frDesign>(logger_);
  // Define swig TCL commands.
  Triton_route_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::triton_route_tcl_inits);
  }

void TritonRoute::init() {
  if (DBPROCESSNODE == "GF14_13M_3Mx_2Cx_4Kx_2Hx_2Gx_LB") {
    VIAINPIN_BOTTOMLAYERNUM = 2;
    VIAINPIN_TOPLAYERNUM = 2;
    USENONPREFTRACKS = false;
    BOTTOM_ROUTING_LAYER = 4;
    TOP_ROUTING_LAYER = 18;
    ENABLE_VIA_GEN = false;
  }

  io::Parser parser(getDesign(),logger_);
  parser.readLefDb(db_);
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

void TritonRoute::prep() {
  FlexRP rp(getDesign(), getDesign()->getTech(), logger_);
  rp.main();
}

void TritonRoute::gr() {
  FlexGR gr(getDesign());
  gr.main();
}

void TritonRoute::ta() {
  FlexTA ta(getDesign());
  ta.main();
  io::Writer writer(getDesign(),logger_);
  writer.writeFromTA();
}

void TritonRoute::dr() {
  num_drvs_ = -1;
  FlexDR dr(getDesign(),logger_);
  dr.setDebug(debug_.get(), db_);
  dr.main();
}

void TritonRoute::endFR() {
  io::Writer writer(getDesign(),logger_);
  writer.writeFromDR();
  writer.updateDb(db_);
}

int TritonRoute::main() {
  init();
  if (GUIDE_FILE == string("")) {
    gr();
    io::Parser parser(getDesign(),logger_);
    GUIDE_FILE = OUTGUIDE_FILE;
    ENABLE_VIA_GEN = true;
    parser.readGuide();
    parser.initDefaultVias();
    parser.writeRefDef();
    parser.postProcessGuide();
  }
  prep();
  ta();
  dr();
  endFR();

  num_drvs_ = design_->getTopBlock()->getNumMarkers();

  return 0;
}

void TritonRoute::readParams(const string &fileName)
{
  int readParamCnt = 0;
  fstream fin(fileName.c_str());
  string line;
  if (fin.is_open()){
    while (fin.good()){
      getline(fin, line);
      if (line[0] != '#'){
        char delimiter=':';
        int pos = line.find(delimiter);
        string field = line.substr(0, pos);
        string value = line.substr(pos + 1);
        stringstream ss(value);
        if (field == "lef")           { LEF_FILE = value; ++readParamCnt;}
        else if (field == "def")      { DEF_FILE = value; REF_OUT_FILE = DEF_FILE; ++readParamCnt;}
        else if (field == "guide")    { GUIDE_FILE = value; ++readParamCnt;}
        else if (field == "outputTA") { OUTTA_FILE = value; ++readParamCnt;}
        else if (field == "output")   { OUT_FILE = value; ++readParamCnt;}
        else if (field == "outputguide") { OUTGUIDE_FILE = value; ++readParamCnt;}
        else if (field == "outputMaze") { OUT_MAZE_FILE = value; ++readParamCnt;}
        else if (field == "outputDRC") { DRC_RPT_FILE = value; ++readParamCnt;}
        else if (field == "outputCMap") { CMAP_FILE = value; ++readParamCnt;}
        else if (field == "threads")  { MAX_THREADS = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "verbose")    VERBOSE = atoi(value.c_str());
        else if (field == "dbProcessNode") { DBPROCESSNODE = value; ++readParamCnt;}
        else if (field == "drouteOnGridOnlyPrefWireBottomLayerNum") { ONGRIDONLY_WIRE_PREF_BOTTOMLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteOnGridOnlyPrefWireTopLayerNum") { ONGRIDONLY_WIRE_PREF_TOPLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteOnGridOnlyNonPrefWireBottomLayerNum") { ONGRIDONLY_WIRE_NONPREF_BOTTOMLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteOnGridOnlyNonPrefWireTopLayerNum") { ONGRIDONLY_WIRE_NONPREF_TOPLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteOnGridOnlyViaBottomLayerNum") { ONGRIDONLY_VIA_BOTTOMLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteOnGridOnlyViaTopLayerNum") { ONGRIDONLY_VIA_TOPLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteViaInPinBottomLayerNum") { VIAINPIN_BOTTOMLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteViaInPinTopLayerNum") { VIAINPIN_TOPLAYERNUM = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "drouteEndIterNum") { END_ITERATION = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "OR_SEED") {OR_SEED = atoi(value.c_str()); ++readParamCnt;}
        else if (field == "OR_K") {OR_K = atof(value.c_str()); ++readParamCnt;}
      }
    }
    fin.close();
  }

  if (MAX_THREADS > 1 && debug_->is_on()) {
    logger_->info(DRT, 115, "Setting MAX_THREADS=1 for use with the GUI.");
    MAX_THREADS = 1;
  }

  if (readParamCnt < 5) {
    logger_->error(DRT, 1, "Error reading param file: {}", fileName);
  }
}
