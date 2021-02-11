///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019-2020, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include "circuit.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PathRef.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/Bfs.hh"
#include "sta/Sequential.hh"
#include "sta/FuncExpr.hh"
#include "sta/SearchPred.hh"
#include "utility/Logger.h"

namespace mpl {

using std::endl;
using std::make_pair;
using std::pair;
using std::string;
using std::to_string;
using std::unordered_map;
using std::unordered_set;
using std::vector;

using utl::MPL;

using odb::dbTech;
using odb::dbTechLayer;
using odb::dbBlock;
using odb::dbRow;
using odb::dbSet;
using odb::dbSite;
using odb::Rect;
using odb::dbInst;
using odb::dbPlacementStatus;
using odb::dbSigType;
using odb::dbBTerm;
using odb::dbBPin;
using odb::dbITerm;

constexpr int edge_count = 4;

static size_t TrimWhiteSpace(char* out, size_t len, const char* str);

static PinGroupLocation getPinGroupLocation(int cx,
                                                 int cy,
                                                 int dieLx,
                                                 int dieLy,
                                                 int dieUx,
                                                 int dieUy);

static std::string getPinGroupLocationString(PinGroupLocation pg);

static bool isMacroType(odb::dbMasterType mType);

static bool isMissingLiberty(sta::Sta* sta, vector<Macro>& macroStor);

MacroCircuit::MacroCircuit()
    : db_(nullptr),
      sta_(nullptr),
      log_(nullptr),
      isTiming_(false),
      lx_(0),
      ly_(0),
      ux_(0),
      uy_(0),
      siteSizeX_(0),
      siteSizeY_(0),
      haloX_(0),
      haloY_(0),
      channelX_(0),
      channelY_(0),
      netTable_(nullptr),
      verbose_(1),
      fenceRegionMode_(false)
{
}

MacroCircuit::MacroCircuit(odb::dbDatabase* db,
                           sta::dbSta* sta,
                           utl::Logger* log)
    : MacroCircuit()
{
  db_ = db;
  sta_ = sta;
  log_ = log;
  init();
}

void MacroCircuit::reset()
{
  db_ = nullptr;
  sta_ = nullptr;
  isTiming_ = false;
  lx_ = ly_ = ux_ = uy_ = 0;
  siteSizeX_ = siteSizeY_ = 0;
  haloX_ = haloY_ = 0;
  channelX_ = channelY_ = 0;
  netTable_ = nullptr;
  globalConfig_ = localConfig_ = "";
  verbose_ = 1;
  fenceRegionMode_ = false;
}

void MacroCircuit::init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log)
{
  db_ = db;
  sta_ = sta;
  log_ = log;
}

void MacroCircuit::setGlobalConfig(const char* globalConfig)
{
  globalConfig_ = globalConfig;
}

void MacroCircuit::setLocalConfig(const char* localConfig)
{
  localConfig_ = localConfig;
}

void MacroCircuit::setVerboseLevel(int verbose)
{
  verbose_ = verbose;
}

void MacroCircuit::setFenceRegion(double lx, double ly, double ux, double uy)
{
  fenceRegionMode_ = true;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

void MacroCircuit::init()
{
  dbBlock* block = db_->getChip()->getBlock();

  dbSet<dbRow> rows = block->getRows();
  if (rows.empty()) {
    log_->error(MPL, 1, "no rows found.");
  }

  const double dbu = db_->getTech()->getDbUnitsPerMicron();
  dbSite* site = rows.begin()->getSite();
  siteSizeX_ = site->getWidth() / dbu;
  siteSizeY_ = site->getHeight() / dbu;

  // if fenceRegion is not set
  // (lx, ly) - (ux, uy) becomes core area
  if (!fenceRegionMode_) {
    odb::Rect coreRect;
    block->getCoreArea(coreRect);

    lx_ = coreRect.xMin() / dbu;
    ly_ = coreRect.yMin() / dbu;
    ux_ = coreRect.xMax() / dbu;
    uy_ = coreRect.yMax() / dbu;
  }

  // parsing from cfg file
  // global config
  ParseGlobalConfig(globalConfig_);
  // local config (optional)
  if (localConfig_ != "") {
    ParseLocalConfig(localConfig_);
  }

  FillMacroStor();
  UpdateInstanceToMacroStor();

  // Timing-driven will be skipped if some instances are missing liberty cells.
  isTiming_ = !isMissingLiberty(sta_, macroStor);

  if (isTiming_) {
    reportEdgePinCounts();
    findAdjacencies();
  } else {
    log_->warn(MPL, 2, "Missing Liberty Detected. TritonMP will place macros without "
               "timing information");
  }
}

static bool isMacroType(odb::dbMasterType mType)
{
  return mType.isBlock();
}

void MacroCircuit::FillMacroStor()
{
  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbRow> rows = block->getRows();

  Rect dieBox;
  rows.begin()->getBBox(dieBox);

  // This should be looking at site height. -cherry
  int cellHeight = dieBox.dy();
  const int dbu = db_->getTech()->getDbUnitsPerMicron();

  for (dbInst* inst : block->getInsts()) {
    // This is broken - it should be looking at master class. -cherry
    // Skip for standard cells
    if ((int) inst->getBBox()->getDY() <= cellHeight) {
      continue;
    }

    if (!isMacroType(inst->getMaster()->getType())) {
      continue;
    }

    // for Macro cells
    dbPlacementStatus dps = inst->getPlacementStatus();
    if (dps == dbPlacementStatus::NONE || dps == dbPlacementStatus::UNPLACED) {
      log_->error(MPL,
                  3,
                  "Macro {} is unplaced. Use global_placement to get an initial placement before macro placment.",
                  inst->getConstName());
    }

    double curHaloX = 0, curHaloY = 0, curChannelX = 0, curChannelY = 0;
    auto mlPtr = macroLocalMap.find(inst->getConstName());
    if (mlPtr == macroLocalMap.end()) {
      curHaloX = haloX_;
      curHaloY = haloY_;
      curChannelX = channelX_;
      curChannelY = channelY_;
    } else {
      MacroLocalInfo& m = mlPtr->second;
      curHaloX = (m.GetHaloX() == 0) ? haloX_ : m.GetHaloX();
      curHaloY = (m.GetHaloY() == 0) ? haloY_ : m.GetHaloY();
      curChannelX = (m.GetChannelX() == 0) ? channelX_ : m.GetChannelX();
      curChannelY = (m.GetChannelY() == 0) ? channelY_ : m.GetChannelY();
    }

    macroNameMap[inst->getConstName()] = macroStor.size();

    int placeX, placeY;
    inst->getLocation(placeX, placeY);

    macroStor.push_back(Macro(1.0 * placeX / dbu,
                              1.0 * placeY / dbu,
                              1.0 * inst->getBBox()->getDX() / dbu,
                              1.0 * inst->getBBox()->getDY() / dbu,
                              curHaloX,
                              curHaloY,
                              curChannelX,
                              curChannelY,
                              nullptr,
                              nullptr,
                              inst));
  }

  if (macroStor.empty()) {
    log_->error(MPL, 4, "Cannot find any macros in this design");
  }

  log_->info(MPL, 5, "NumMacros {}", macroStor.size());
}

// macroStr & macroInstMap update
void MacroCircuit::UpdateInstanceToMacroStor()
{
  for (auto& macro : macroStor) {
    sta::Instance* staInst = sta_->getDbNetwork()->dbToSta(macro.dbInstPtr);
    macro.staInstPtr = staInst;
    macroInstMap[staInst] = &macro - &macroStor[0];
  }
}

static bool isWithIn(int val, int min, int max)
{
  return ((min <= val) && (val <= max));
}

static float getRoundUpFloat(float x, float unit)
{
  return std::round(x / unit) * unit;
}

void MacroCircuit::UpdateMacroCoordi(Partition& part)
{
  dbTech* tech = db_->getTech();
  dbTechLayer* fourLayer = tech->findRoutingLayer(4);
  if (!fourLayer) {
    log_->warn(MPL,
               21,
               "Metal 4 not exist! Macro snapping will not be applied on "
               "Metal4 pitch");
  }

  const float pitchX = static_cast<float>(fourLayer->getPitchX())
    / static_cast<float>(tech->getDbUnitsPerMicron());
  const float pitchY = static_cast<float>(fourLayer->getPitchY())
    / static_cast<float>(tech->getDbUnitsPerMicron());

  for (auto& curMacro : part.macroStor) {
    auto mnPtr = macroNameMap.find(curMacro.name());
    if (mnPtr == macroNameMap.end()) {
      log_->error(MPL, 22, "{} is not in MacroCircuit", curMacro.name());
    }

    // update macro coordi
    float macroX
      = (fourLayer) ? getRoundUpFloat(curMacro.lx, pitchX) : curMacro.lx;
    float macroY
      = (fourLayer) ? getRoundUpFloat(curMacro.ly, pitchY) : curMacro.ly;

    // Update Macro Location
    int macroIdx = mnPtr->second;
    macroStor[macroIdx].lx = macroX;
    macroStor[macroIdx].ly = macroY;
  }
}

static bool stringExists(std::string varname, std::string str)
{
  return varname.find(str) != std::string::npos;
}

void MacroCircuit::ParseGlobalConfig(string fileName)
{
  std::ifstream gConfFile(fileName);
  if (!gConfFile.is_open()) {
    log_->error(MPL, 25, "Cannot open file {}", fileName);
  }

  string lineStr = "";
  while (getline(gConfFile, lineStr)) {
    char trimChar[256] = {
        0,
    };
    TrimWhiteSpace(trimChar, lineStr.length() + 1, lineStr.c_str());
    string trimStr(trimChar);

    std::stringstream oStream(trimStr);
    string buf1, varName;
    double val = 0.0f;

    oStream >> buf1;
    string skipStr = "//";
    // skip for slash
    if (buf1.substr(0, skipStr.size()) == skipStr) {
      continue;
    }
    if (buf1 != "set") {
      log_->error(MPL, 26, "Cannot parse {}", buf1);
    }

    oStream >> varName >> val;

    if (stringExists(varName, "FIN_PITCH")) {
      // TODO
      // ?
    } else if (stringExists(varName, "ROW_HEIGHT")) {
      // TODO
      // No Need
    } else if (stringExists(varName, "SITE_WIDTH")) {
      // TODO
      // No Need
    } else if (stringExists(varName, "HALO_WIDTH_V")) {
      haloY_ = val;
    } else if (stringExists(varName, "HALO_WIDTH_H")) {
      haloX_ = val;
    } else if (stringExists(varName, "CHANNEL_WIDTH_V")) {
      channelY_ = val;
    } else if (stringExists(varName, "CHANNEL_WIDTH_H")) {
      channelX_ = val;
    } else {
      log_->error(MPL, 27, "Cannot parse {}", varName);
    }
  }
  log_->report("End Parsing Global Config");
}

void MacroCircuit::ParseLocalConfig(string fileName)
{
  std::ifstream gConfFile(fileName);
  if (!gConfFile.is_open()) {
    log_->error(MPL, 28, "Cannot open file {}", fileName);
  }

  string lineStr = "";
  while (getline(gConfFile, lineStr)) {
    char trimChar[256] = {
        0,
    };
    TrimWhiteSpace(trimChar, lineStr.length() + 1, lineStr.c_str());
    string trimStr(trimChar);

    std::stringstream oStream(trimStr);
    string buf1, varName, masterName;
    double val = 0.0f;

    oStream >> buf1;
    string skipStr = "//";
    // skip for slash
    if (buf1.substr(0, skipStr.size()) == skipStr) {
      continue;
    }

    if (buf1 == "") {
      continue;
    }
    if (buf1 != "set") {
      log_->error(MPL, 29, "Cannot parse {}", buf1);
    }

    oStream >> varName >> masterName >> val;

    if (stringExists(varName, "ROW_HEIGHT")) {
      // TODO
      // No Need
    } else if (stringExists(varName, "HALO_WIDTH_V")) {
      macroLocalMap[masterName].putHaloY(val);
    } else if (stringExists(varName, "HALO_WIDTH_H")) {
      macroLocalMap[masterName].putHaloX(val);
    } else if (stringExists(varName, "CHANNEL_WIDTH_V")) {
      macroLocalMap[masterName].putChannelY(val);
    } else if (stringExists(varName, "CHANNEL_WIDTH_H")) {
      macroLocalMap[masterName].putChannelX(val);
    } else {
      log_->error(MPL, 30, "Cannot parse {}", varName);
    }
  }
  log_->report("End Parsing Local Config");
}

void MacroCircuit::UpdateNetlist(Partition& layout)
{
  if (netTable_) {
    delete[] netTable_;
    netTable_ = 0;
  }

  assert(layout.macroStor.size() == macroStor.size());
  size_t tableSize = (macroStor.size() + 4) * (macroStor.size() + 4);

  netTable_ = new double[tableSize];
  for (size_t i = 0; i < tableSize; i++) {
    netTable_[i] = layout.netTable[i];
  }
}

#define EAST_IDX (macroStor.size())
#define WEST_IDX (macroStor.size() + 1)
#define NORTH_IDX (macroStor.size() + 2)
#define SOUTH_IDX (macroStor.size() + 3)

double MacroCircuit::GetWeightedWL()
{
  double wwl = 0.0f;

  double width = ux_ - lx_;
  double height = uy_ - ly_;

  for (size_t i = 0; i < macroStor.size() + 4; i++) {
    for (size_t j = 0; j < macroStor.size() + 4; j++) {
      if (j >= i) {
        continue;
      }

      double pointX1 = 0, pointY1 = 0;
      if (i == EAST_IDX) {
        pointX1 = lx_ + width;
        pointY1 = ly_ + height / 2.0;
      } else if (i == WEST_IDX) {
        pointX1 = lx_;
        pointY1 = ly_ + height / 2.0;
      } else if (i == NORTH_IDX) {
        pointX1 = lx_ + width / 2.0;
        pointY1 = ly_ + height;
      } else if (i == SOUTH_IDX) {
        pointX1 = lx_ + width / 2.0;
        pointY1 = ly_;
      } else {
        pointX1 = macroStor[i].lx + macroStor[i].w;
        pointY1 = macroStor[i].ly + macroStor[i].h;
      }

      double pointX2 = 0, pointY2 = 0;
      if (j == EAST_IDX) {
        pointX2 = lx_ + width;
        pointY2 = ly_ + height / 2.0;
      } else if (j == WEST_IDX) {
        pointX2 = lx_;
        pointY2 = ly_ + height / 2.0;
      } else if (j == NORTH_IDX) {
        pointX2 = lx_ + width / 2.0;
        pointY2 = ly_ + height;
      } else if (j == SOUTH_IDX) {
        pointX2 = lx_ + width / 2.0;
        pointY2 = ly_;
      } else {
        pointX2 = macroStor[j].lx + macroStor[j].w;
        pointY2 = macroStor[j].ly + macroStor[j].h;
      }

      float edgeWeight = 0.0f;
      if (isTiming_) {
        edgeWeight = netTable_[i * (macroStor.size()) + j];
      } else {
        edgeWeight = 1;
      }

      wwl += edgeWeight
             * std::sqrt((pointX1 - pointX2) * (pointX1 - pointX2)
                         + (pointY1 - pointY2) * (pointY1 - pointY2));
    }
  }

  return wwl;
}

Layout::Layout() : lx_(0), ly_(0), ux_(0), uy_(0)
{
}

Layout::Layout(double lx, double ly, double ux, double uy)
    : lx_(lx), ly_(ly), ux_(ux), uy_(uy)
{
}

Layout::Layout(Layout& orig, Partition& part)
    : lx_(part.lx),
      ly_(part.ly),
      ux_(part.lx + part.width),
      uy_(part.ly + part.height)
{
}

void Layout::setLx(double lx)
{
  lx_ = lx;
}

void Layout::setLy(double ly)
{
  ly_ = ly;
}

void Layout::setUx(double ux)
{
  ux_ = ux;
}

void Layout::setUy(double uy)
{
  uy_ = uy;
}

///////////////////////////////////////////////////
//  static funcs

// Stores the trimmed input string into the given output buffer, which must be
// large enough to store the result.  If it is too small, the output is
// truncated.

// referenced from
// https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way

static size_t TrimWhiteSpace(char* out, size_t len, const char* str)
{
  if (len == 0)
    return 0;

  const char* end;
  size_t out_size;

  // Trim leading space
  while (isspace((unsigned char) *str))
    str++;

  if (*str == 0)  // All spaces?
  {
    *out = 0;
    return 1;
  }

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char) *end))
    end--;
  end++;

  // Set output size to minimum of trimmed string length and buffer size minus 1
  out_size = (end - str) < ((int) len - 1) ? (end - str) : ((int) len - 1);

  // Copy trimmed string and add null terminator
  memcpy(out, str, out_size);
  out[out_size] = 0;

  return out_size;
}

static PinGroupLocation getPinGroupLocation(int cx,
                                            int cy,
                                            int dieLx,
                                            int dieLy,
                                            int dieUx,
                                            int dieUy)
{
  int lxDx = abs(cx - dieLx);
  int uxDx = abs(cx - dieUx);

  int lyDy = abs(cy - dieLy);
  int uyDy = abs(cy - dieUy);

  int minDiff = std::min(lxDx, std::min(uxDx, std::min(lyDy, uyDy)));
  if (minDiff == lxDx) {
    return West;
  } else if (minDiff == uxDx) {
    return East;
  } else if (minDiff == lyDy) {
    return South;
  } else if (minDiff == uyDy) {
    return North;
  }
  return West;
}

static std::string getPinGroupLocationString(PinGroupLocation pg)
{
  if (pg == West) {
    return "West";
  } else if (pg == East) {
    return "East";
  } else if (pg == North) {
    return "North";
  } else if (pg == South) {
    return "South";
  } else
    return "??";
}

static bool isMissingLiberty(sta::Sta* sta, vector<Macro>& macroStor)
{
  sta::LeafInstanceIterator* instIter = sta->network()->leafInstanceIterator();
  while (instIter->hasNext()) {
    sta::Instance* inst = instIter->next();
    if (!sta->network()->libertyCell(inst)) {
      delete instIter;
      return true;
    }
  }
  delete instIter;

  for (auto& macro : macroStor) {
    sta::Instance* staInst = macro.staInstPtr;
    sta::LibertyCell* libCell = sta->network()->libertyCell(staInst);
    if (!libCell) {
      return true;
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////

typedef std::pair<Macro*, Macro*> MacroPair;

void MacroCircuit::findAdjacencies()
{
  sta::dbNetwork *network = sta_->getDbNetwork();
  sta::Graph *graph = sta_->ensureGraph();
  sta_->ensureLevelized();
  sta_->ensureClkNetwork();
  VertexFaninMap vertex_fanins;
  sta::SearchPred2 srch_pred(sta_);
  sta::BfsFwdIterator bfs(sta::BfsIndex::other, &srch_pred, sta_);
  // Seed the BFS with macro output pins.
  for (Macro& macro : macroStor) {
    for (dbITerm *iterm : macro.dbInstPtr->getITerms()) {
      sta::Pin *pin = network->dbToSta(iterm);
      if (network->direction(pin)->isAnyOutput()
          && !sta_->isClock(pin)) {
        sta::Vertex *vertex = graph->pinDrvrVertex(pin);
        vertex_fanins[vertex].insert(&macro);
        bfs.enqueueAdjacentVertices(vertex);
      }
    }
  }
  // Seed top level ports input ports.
  for (dbBTerm *bterm : db_->getChip()->getBlock()->getBTerms()) {
    sta::Pin *pin = network->dbToSta(bterm);
    if (network->direction(pin)->isAnyInput()
        && !sta_->isClock(pin)) {
      sta::Vertex *vertex = graph->pinDrvrVertex(pin);
      PinGroupLocation edge = findNearestEdge(bterm);
      vertex_fanins[vertex].insert(reinterpret_cast<Macro*>(edge));
      bfs.enqueueAdjacentVertices(vertex);
    }
  }
  findFanins(bfs, vertex_fanins, network, graph);
  // Propagate fanins through 3 levels of register D->Q.
  constexpr int reg_adjacency_depth = 3;
  for (int i = 0; i < reg_adjacency_depth; i++) {
    copyFaninsAcrossRegisters(bfs, vertex_fanins, network, graph);
    findFanins(bfs, vertex_fanins, network, graph);
  }

  ////////////////////////////////////////////////////////////////

  // from/to -> weight
  // weight = from/pin -> to/pin count
  std::map<MacroPair, int> adj_map;
  // Find adjacencies from macro input pin fanins.
  for (Macro& macro : macroStor) {
    for (dbITerm *iterm : macro.dbInstPtr->getITerms()) {
      sta::Pin *pin = network->dbToSta(iterm);
      if (network->direction(pin)->isAnyInput()) {
        sta::Vertex *vertex = graph->pinLoadVertex(pin);
        MacroSet &pin_fanins = vertex_fanins[vertex];
        for (Macro *pin_fanin : pin_fanins) {
          // Adjacencies are symmetric so only fill in one side.
          if (pin_fanin != &macro) {
            MacroPair from_to = (pin_fanin > &macro)
              ? MacroPair(pin_fanin, &macro)
              : MacroPair(&macro, pin_fanin);
            adj_map[from_to]++;
          }
        }
      }
    }
  }
  // Find adjacencies from output pin fanins.
  for (dbBTerm *bterm : db_->getChip()->getBlock()->getBTerms()) {
    sta::Pin *pin = network->dbToSta(bterm);
    if (network->direction(pin)->isAnyOutput()
        && !sta_->isClock(pin)) {
      sta::Vertex *vertex = graph->pinDrvrVertex(pin);
      PinGroupLocation edge = findNearestEdge(bterm);
      debugPrint(log_, MPL, "pin_edge", 1, "pin edge {} {}",
                 bterm->getConstName(),
                 getPinGroupLocationString(edge));
      int edge_index = static_cast<int>(edge);
      Macro *macro = reinterpret_cast<Macro*>(edge_index);
      MacroSet &edge_fanins = vertex_fanins[vertex];
      for (Macro *edge_fanin : edge_fanins) {
        if (edge_fanin != macro) {
          // Adjacencies are symmetric so only fill in one side.
          MacroPair from_to = (edge_fanin > macro)
            ? MacroPair(edge_fanin, macro)
            : MacroPair(macro, edge_fanin);
          adj_map[from_to]++;
        }
      }
    }
  }

  // Fill macroWeight array.
  size_t weight_size = macroStor.size() + 4;
  macroWeight.resize(weight_size);
  for (size_t i = 0; i < weight_size; i++) {
    macroWeight[i].resize(weight_size);
    macroWeight[i] = {0};
  }

  for (auto pair_weight : adj_map) {
    const MacroPair &from_to = pair_weight.first;
    Macro *from = from_to.first;
    Macro *to = from_to.second;
    float weight = pair_weight.second;
    if (!(macroIndexIsEdge(from) && macroIndexIsEdge(to))) {
      macroWeight[macroIndex(from)][macroIndex(to)] = weight;
      if (weight > 0)
        debugPrint(log_, MPL, "weights", 1, "{} -> {} {}",
                   faninName(from),
                   faninName(to),
                   weight);
    }
  }
}

std::string MacroCircuit::faninName(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  if (edge_index == static_cast<intptr_t>(West))
    return "West";
  else if (edge_index == static_cast<intptr_t>(East))
    return "East";
  else if (edge_index == static_cast<intptr_t>(North))
    return "North";
  else if (edge_index == static_cast<intptr_t>(South))
    return "South";
  else
    return macro->name();
}

int MacroCircuit::macroIndex(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  if (edge_index < edge_count)
    return edge_index;
  else
    return macro - &macroStor[0] + edge_count;
}

bool MacroCircuit::macroIndexIsEdge(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  return edge_index < 4;
}

// BFS search forward union-ing fanins.
// BFS stops at register inputs because there are no timing arcs
// from register D->Q.
void MacroCircuit::findFanins(sta::BfsFwdIterator &bfs,
                              VertexFaninMap &vertex_fanins,
                              sta::dbNetwork *network,
                              sta::Graph *graph)
{
  while (bfs.hasNext()) {
    sta::Vertex *vertex = bfs.next();
    MacroSet &fanins = vertex_fanins[vertex];
    sta::VertexInEdgeIterator fanin_iter(vertex, graph);
    while (fanin_iter.hasNext()) {
      sta::Edge *edge = fanin_iter.next();
      sta::Vertex *fanin = edge->from(graph);
      // Union fanins sets of fanin vertices.
      for (Macro *fanin : vertex_fanins[fanin]) {
        fanins.insert(fanin);
        debugPrint(log_, MPL, "find_fanins", 1, "{} + {}",
                   vertex->name(network),
                   faninName(fanin));
      }
    }
    bfs.enqueueAdjacentVertices(vertex);
  }
}

void MacroCircuit::copyFaninsAcrossRegisters(sta::BfsFwdIterator &bfs,
                                             VertexFaninMap &vertex_fanins,
                                             sta::dbNetwork *network,
                                             sta::Graph *graph)
{
  sta::Instance *top_inst = network->topInstance();
  sta::LeafInstanceIterator *leaf_iter = network->leafInstanceIterator(top_inst);
  while (leaf_iter->hasNext()) {
    sta::Instance *inst = leaf_iter->next();
    sta::LibertyCell *lib_cell = network->libertyCell(inst);
    if (lib_cell->hasSequentials()
        && !lib_cell->isMacro()) {
      sta::LibertyCellSequentialIterator seq_iter(lib_cell);
      while (seq_iter.hasNext()) {
        sta::Sequential *seq = seq_iter.next();
        sta::FuncExpr *data_expr = seq->data();
        sta::FuncExprPortIterator data_port_iter(data_expr);
        while (data_port_iter.hasNext()) {
          sta::LibertyPort *data_port = data_port_iter.next();
          sta::Pin *data_pin = network->findPin(inst, data_port);
          sta::LibertyPort *out_port = seq->output();
          sta::Pin *out_pin = findSeqOutPin(inst, out_port, network);
          if (data_pin && out_pin) {
            sta::Vertex *data_vertex = graph->pinLoadVertex(data_pin);
            sta::Vertex *out_vertex = graph->pinDrvrVertex(out_pin);
            // Copy fanins from D to Q on register.
            vertex_fanins[out_vertex] = vertex_fanins[data_vertex];
            bfs.enqueueAdjacentVertices(out_vertex);
          }
        }
      }
    }
  }
  delete leaf_iter;
}

// Sequential outputs are generally to internal pins that are not physically
// part of the instance. Find the output port with a function that uses
// the internal port.
sta::Pin *MacroCircuit::findSeqOutPin(sta::Instance *inst,
                                      sta::LibertyPort *out_port,
                                      sta::Network *network)
{
  if (out_port->direction()->isInternal()) {
    sta::InstancePinIterator *pin_iter = network->pinIterator(inst);
    while (pin_iter->hasNext()) {
      sta::Pin *pin = pin_iter->next();
      sta::LibertyPort *lib_port = network->libertyPort(pin);
      if (lib_port->direction()->isAnyOutput()) {
        sta::FuncExpr *func = lib_port->function();
        if (func->hasPort(out_port)) {
          sta::Pin *out_pin = network->findPin(inst, lib_port);
          if (out_pin) {
            delete pin_iter;
            return out_pin;
          }
        }
      }
    }
    delete pin_iter;
    return nullptr;
  }
  else
    return network->findPin(inst, out_port);
}

// This is completely broken but I want to match FillPinGroup()
// until it is flushed.
PinGroupLocation MacroCircuit::findNearestEdge(dbBTerm* bTerm)
{
  dbPlacementStatus status = bTerm->getFirstPinPlacementStatus();
  if (status == dbPlacementStatus::UNPLACED
      || status == dbPlacementStatus::NONE) {
    log_->warn(MPL, 11, "pin {} is not placed. Using west.",
               bTerm->getConstName());
    return West;
  } else {
    const double dbu = db_->getTech()->getDbUnitsPerMicron();

    int dbuCoreLx = round(lx_ * dbu);
    int dbuCoreLy = round(ly_ * dbu);
    int dbuCoreUx = round(ux_ * dbu);
    int dbuCoreUy = round(uy_ * dbu);

    int placeX = 0, placeY = 0;
    bool isAxisFound = false;
    bTerm->getFirstPinLocation(placeX, placeY);
    for (dbBPin* bPin : bTerm->getBPins()) {
      Rect pin_bbox = bPin->getBBox();
      int boxLx = pin_bbox.xMin();
      int boxLy = pin_bbox.yMin();
      int boxUx = pin_bbox.xMax();
      int boxUy = pin_bbox.yMax();

      // This is broken. It assumes the pins are on the core boundary.
      // It should look for the nearest edge to the pin center. -cherry
      if (isWithIn(dbuCoreLx, boxLx, boxUx)) {
        return West;
      } else if (isWithIn(dbuCoreUx, boxLx, boxUx)) {
        return East;
      } else if (isWithIn(dbuCoreLy, boxLy, boxUy)) {
        return South;
      } else if (isWithIn(dbuCoreUy, boxLy, boxUy)) {
        return North;
      }
    }
    if (!isAxisFound) {
      dbBPin* bPin = *(bTerm->getBPins().begin());
      Rect pin_bbox = bPin->getBBox();
      int boxLx = pin_bbox.xMin();
      int boxLy = pin_bbox.yMin();
      int boxUx = pin_bbox.xMax();
      int boxUy = pin_bbox.yMax();
      return getPinGroupLocation((boxLx + boxUx) / 2,
                                 (boxLy + boxUy) / 2,
                                 dbuCoreLx,
                                 dbuCoreLy,
                                 dbuCoreUx,
                                 dbuCoreUy);
    }
  }
  return West;
}

void MacroCircuit::reportEdgePinCounts()
{
  int counts[edge_count] = {0};
  for (dbBTerm *bterm : db_->getChip()->getBlock()->getBTerms()) {
    PinGroupLocation edge = findNearestEdge(bterm);
    counts[edge]++;
  }
  for (int i = 0; i < edge_count; i++) {
    PinGroupLocation edge = static_cast<PinGroupLocation>(i);
    
      log_->info(MPL, 9, "{} pins {}",
                 getPinGroupLocationString(edge),
                 counts[i]);
  }
}

}  // namespace mpl
