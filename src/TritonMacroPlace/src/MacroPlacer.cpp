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

#include "mpl/MacroPlacer.h"

#include <string>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/Corner.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Network.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Sta.hh"
#include "sta/Bfs.hh"
#include "sta/Sequential.hh"
#include "sta/FuncExpr.hh"
#include "sta/SearchPred.hh"

#include "utility/Logger.h"

namespace mpl {

using std::string;
using std::vector;
using std::pair;
using std::unordered_map;

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

typedef vector<pair<Partition, Partition>> TwoPartitions;

static CoreEdge getCoreEdge(int cx,
                            int cy,
                            int dieLx,
                            int dieLy,
                            int dieUx,
                            int dieUy);

static vector<pair<Partition, Partition>> GetPart(const Layout& layout,
                                                  const double siteSizeX,
                                                  const double siteSizeY,
                                                  const Partition& partition,
                                                  bool isHorizontal,
                                                  utl::Logger* log);

////////////////////////////////////////////////////////////////

MacroPlacer::MacroPlacer()
    : db_(nullptr),
      sta_(nullptr),
      logger_(nullptr),
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
      fenceRegionMode_(false),
      solCount_(0)
{
}

void MacroPlacer::init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log)
{
  db_ = db;
  sta_ = sta;
  logger_ = log;
}

void MacroPlacer::setHalo(double halo_x, double halo_y)
{
  haloX_ = halo_x;
  haloY_ = halo_y;
}

void MacroPlacer::setChannel(double channel_x, double channel_y)
{
  channelX_ = channel_x;
  channelY_ = channel_y;
}

void MacroPlacer::setVerboseLevel(int verbose)
{
  verbose_ = verbose;
}

void MacroPlacer::setFenceRegion(double lx, double ly, double ux, double uy)
{
  fenceRegionMode_ = true;
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

int MacroPlacer::getSolutionCount()
{
  return solCount_;
}

void MacroPlacer::init()
{
  dbBlock* block = db_->getChip()->getBlock();

  dbSet<dbRow> rows = block->getRows();
  if (rows.empty()) {
    logger_->error(MPL, 1, "no rows found.");
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

  FillMacroStor();

  // Timing-driven will be skipped if some instances are missing liberty cells.
  isTiming_ = !isMissingLiberty();

  if (isTiming_) {
    reportEdgePinCounts();
    findAdjacencies();
  } else {
    logger_->warn(MPL, 2, "Missing Liberty Detected. TritonMP will place macros without "
               "timing information");
  }
}

bool MacroPlacer::isMissingLiberty()
{
  sta::Network *network = sta_->network();
  sta::LeafInstanceIterator* instIter = network->leafInstanceIterator();
  while (instIter->hasNext()) {
    sta::Instance* inst = instIter->next();
    if (!network->libertyCell(inst)) {
      delete instIter;
      return true;
    }
  }
  delete instIter;
  return false;
}

void MacroPlacer::reportEdgePinCounts()
{
  int counts[core_edge_count] = {0};
  for (dbBTerm *bterm : db_->getChip()->getBlock()->getBTerms()) {
    CoreEdge edge = findNearestEdge(bterm);
    counts[coreEdgeIndex(edge)]++;
  }
  for (int i = 0; i < core_edge_count; i++) {
    CoreEdge edge = static_cast<CoreEdge>(i);
    
      logger_->info(MPL, 9, "{} pins {}",
                 coreEdgeString(edge),
                 counts[i]);
  }
}

void MacroPlacer::placeMacros()
{
  init();
  Layout layout(lx_, ly_, ux_, uy_);

  bool isHorizontal = true;

  Partition topLayout(PartClass::ALL,
                      layout.lx(),
                      layout.ly(),
                      layout.ux() - layout.lx(),
                      layout.uy() - layout.ly(),
                      logger_);
  topLayout.macroStor = macroStor;

  logger_->report("Begin One Level Partition");

  TwoPartitions oneLevelPart
      = GetPart(layout, siteSizeX_, siteSizeY_, topLayout, isHorizontal, logger_);

  logger_->report("End One Level Partition");
  TwoPartitions eastStor, westStor;

  vector<vector<Partition>> allSets;

  MacroPartMap globalMacroPartMap;
  UpdateMacroPartMap(topLayout, globalMacroPartMap);

  if (isTiming_) {
    topLayout.FillNetlistTable(this, globalMacroPartMap);
    UpdateNetlist(topLayout);
  }

  // push to the outer vector
  vector<Partition> layoutSet;
  layoutSet.push_back(topLayout);

  // push
  allSets.push_back(layoutSet);

  for (auto& curSet : oneLevelPart) {
    if (isHorizontal) {
      logger_->report("Begin Horizontal Partition");
      Layout eastInfo(layout, curSet.first);
      Layout westInfo(layout, curSet.second);

      logger_->report("Begin East Partition");
      TwoPartitions eastStor = GetPart(
          eastInfo, siteSizeX_, siteSizeY_, curSet.first, !isHorizontal, logger_);
      logger_->report("End East Partition");

      logger_->report("Begin West Partition");
      TwoPartitions westStor = GetPart(
          westInfo, siteSizeX_, siteSizeY_, curSet.second, !isHorizontal, logger_);
      logger_->report("End West Partition");

      // Zero case handling when eastStor = 0
      if (eastStor.size() == 0 && westStor.size() != 0) {
        for (size_t i = 0; i < westStor.size(); i++) {
          vector<Partition> oneSet;

          // one set is composed of two subblocks
          oneSet.push_back(westStor[i].first);
          oneSet.push_back(westStor[i].second);

          // Fill Macro Netlist
          // update macroPartMap
          MacroPartMap macroPartMap;
          for (auto& curSet : oneSet) {
            UpdateMacroPartMap(curSet, macroPartMap);
          }

          if (isTiming_) {
            for (auto& curSet : oneSet) {
              curSet.FillNetlistTable(this, macroPartMap);
            }
          }

          allSets.push_back(oneSet);
        }
      }
      // Zero case handling when westStor = 0
      else if (eastStor.size() != 0 && westStor.size() == 0) {
        for (size_t i = 0; i < eastStor.size(); i++) {
          vector<Partition> oneSet;

          // one set is composed of two subblocks
          oneSet.push_back(eastStor[i].first);
          oneSet.push_back(eastStor[i].second);

          // Fill Macro Netlist
          // update macroPartMap
          MacroPartMap macroPartMap;
          for (auto& curSet : oneSet) {
            UpdateMacroPartMap(curSet, macroPartMap);
          }

          if (isTiming_) {
            for (auto& curSet : oneSet) {
              curSet.FillNetlistTable(this, macroPartMap);
            }
          }

          allSets.push_back(oneSet);
        }
      } else {
        // for all possible combinations in partitions
        for (size_t i = 0; i < eastStor.size(); i++) {
          for (size_t j = 0; j < westStor.size(); j++) {
            vector<Partition> oneSet;

            // one set is composed of four subblocks
            oneSet.push_back(eastStor[i].first);
            oneSet.push_back(eastStor[i].second);
            oneSet.push_back(westStor[j].first);
            oneSet.push_back(westStor[j].second);

            // Fill Macro Netlist
            // update macroPartMap
            MacroPartMap macroPartMap;
            for (auto& curSet : oneSet) {
              UpdateMacroPartMap(curSet, macroPartMap);
            }

            if (isTiming_) {
              for (auto& curSet : oneSet) {
                curSet.FillNetlistTable(this, macroPartMap);
              }
            }

            allSets.push_back(oneSet);
          }
        }
      }
      logger_->report("End Horizontal Partition");
    } else {
      logger_->report("Begin Vertical Partition");
      // TODO
      logger_->report("End Vertical Partition");
    }
  }
  logger_->info(MPL, 70, "NumExtractedSets: {}", allSets.size() - 1);

  solCount_ = 0;
  int bestSetIdx = 0;
  double bestWwl = -DBL_MAX;
  for (auto& curSet : allSets) {
    // skip for top-topLayout partition
    if (curSet.size() == 1) {
      continue;
    }
    // For each partitions (four partition)
    //
    bool isFailed = false;
    for (auto& curPart : curSet) {
      // Annealing based on ParquetFP Engine
      if (!curPart.DoAnneal()) {
        isFailed = true;
        break;
      }
      // Update mckt frequently
      UpdateMacroCoordi(curPart);
    }
    if (isFailed) {
      continue;
    }

    // update partitons' macro info
    for (auto& curPart : curSet) {
      curPart.UpdateMacroCoordi(this);
    }

    double curWwl = GetWeightedWL();
    logger_->info(MPL, 71, "SetId: {}", &curSet - &allSets[0]);
    logger_->info(MPL, 72, "WeightedWL: {:g}", curWwl);

    if (curWwl > bestWwl) {
      bestWwl = curWwl;
      bestSetIdx = &curSet - &allSets[0];
    }
    solCount_++;
  }

  logger_->info(MPL, 73, "NumFinalSols: {}", solCount_);

  // bestset DEF writing
  std::vector<Partition> bestSet = allSets[bestSetIdx];

  for (auto& curBestPart : bestSet) {
    UpdateMacroCoordi(curBestPart);
  }
  UpdateOpendbCoordi();
}

int MacroPlacer::weight(int idx1, int idx2)
{
  return macroWeight[idx1][idx2];
}

// update opendb dataset from mckt.
void MacroPlacer::UpdateOpendbCoordi()
{
  odb::dbTech* tech = db_->getTech();
  const int dbu = tech->getDbUnitsPerMicron();

  for (auto& curMacro : macroStor) {
    curMacro.dbInstPtr->setLocation(round(curMacro.lx * dbu),
                                    round(curMacro.ly * dbu));
    curMacro.dbInstPtr->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  }
}

static void CutRoundUp(const Layout& layout,
                       const double siteSizeX,
                       const double siteSizeY,
                       double& cutLine,
                       bool isHorizontal)
{
  if (isHorizontal) {
    cutLine = std::round(cutLine / siteSizeX) * siteSizeX;
    cutLine = std::min(cutLine, layout.ux());
    cutLine = std::max(cutLine, layout.lx());
  } else {
    cutLine = round(cutLine / siteSizeY) * siteSizeY;
    cutLine = std::min(cutLine, layout.uy());
    cutLine = std::max(cutLine, layout.ly());
  }
}

// using partition,
// fill in macroPartMap
//
// macroPartMap will contain
//
// first: macro partition class info
// second: macro candidates.
void MacroPlacer::UpdateMacroPartMap(Partition& part,
                                     MacroPartMap& macroPartMap)
{
  // This does not look like it actually does anything -cherry
  vector<int> curMacroStor = macroPartMap[part.partClass];
  // convert macro Information into macroIdx
  for (auto& curMacro : part.macroStor) {
    int macro_index = macroInstMap[curMacro.dbInstPtr];
    curMacroStor.push_back(macro_index);
  }
  macroPartMap[part.partClass] = curMacroStor;
}

// only considers lx or ly coordinates for sorting
static bool SortMacroPair(const std::pair<int, double>& p1,
                          const std::pair<int, double>& p2)
{
  return p1.second < p2.second;
}

// Two partitioning functions:
// first : lower part
// second : upper part
//
// cutLine is sweeping from lower to upper coordinates in x / y
static vector<pair<Partition, Partition>> GetPart(const Layout& layout,
                                                  const double siteSizeX,
                                                  const double siteSizeY,
                                                  const Partition& partition,
                                                  bool isHorizontal,
                                                  utl::Logger* log)
{
  log->report("Begin Partition");
  log->info(MPL, 76, "NumMacros {}", partition.macroStor.size());

  // Return vector
  vector<pair<Partition, Partition>> ret;

  double maxWidth = -1e30;
  double maxHeight = -1e30;

  // segment stor
  // first: macroStor index
  // second: lx or ly values
  vector<std::pair<int, double>> segStor;

  // in parent partition, traverse macros
  for (auto& curMacro : partition.macroStor) {
    segStor.push_back(
        std::make_pair(&curMacro - &partition.macroStor[0],
                       (isHorizontal) ? curMacro.lx : curMacro.ly));

    maxWidth = std::max(maxWidth, curMacro.w);
    maxHeight = std::max(maxHeight, curMacro.h);
  }

  double cutLineLimit = (isHorizontal) ? maxWidth * 0.25 : maxHeight * 0.25;
  double prevPushLimit = -1e30;
  bool isFirst = true;
  vector<double> cutLineStor;

  // less than 4
  if (partition.macroStor.size() <= 4) {
    sort(segStor.begin(), segStor.end(), SortMacroPair);

    // first : macroStor index
    // second : macro lower coordinates
    for (auto& segPair : segStor) {
      if (isFirst) {
        cutLineStor.push_back(segPair.second);
        prevPushLimit = segPair.second;
        isFirst = false;
      } else if (std::abs(segPair.second - prevPushLimit) > cutLineLimit) {
        cutLineStor.push_back(segPair.second);
        prevPushLimit = segPair.second;
      }
    }
  }
  // more than 4
  else {
    int hardLimit = std::round(std::sqrt(partition.macroStor.size() / 3.0));
    for (int i = 0; i <= hardLimit; i++) {
      cutLineStor.push_back(
          (isHorizontal)
              ? layout.lx() + (layout.ux() - layout.lx()) / hardLimit * i
              : layout.ly() + (layout.uy() - layout.ly()) / hardLimit * i);
    }
  }
  log->info(MPL, 77, "NumCutLines {}", cutLineStor.size());

  // Macro checker array
  // 0 for uninitialize
  // 1 for lower
  // 2 for upper
  // 3 for both
  vector<int> chkArr(partition.macroStor.size());

  for (auto& cutLine : cutLineStor) {
    log->info(MPL, 78, "CutLine {:.2f}", cutLine);
    CutRoundUp(layout, siteSizeX, siteSizeY, cutLine, isHorizontal);

    log->info(MPL, 79, "RoundUpCutLine {:.2f}", cutLine);

    // chkArr initialize
    for (size_t i = 0; i < partition.macroStor.size(); i++) {
      chkArr[i] = 0;
    }

    bool isImpossible = false;
    for (auto& curMacro : partition.macroStor) {
      int i = &curMacro - &partition.macroStor[0];
      if (isHorizontal) {
        // lower is possible
        if (curMacro.w <= cutLine) {
          chkArr[i] += 1;
        }
        // upper is possible
        if (curMacro.w <= partition.lx + partition.width - cutLine) {
          chkArr[i] += 2;
        }
        // none of them
        if (chkArr[i] == 0) {
          isImpossible = true;
          break;
        }
      } else {
        // lower is possible
        if (curMacro.h <= cutLine) {
          chkArr[i] += 1;
        }
        // upper is possible
        if (curMacro.h <= partition.ly + partition.height - cutLine) {
          chkArr[i] += 2;
        }
        // none of
        if (chkArr[i] == 0) {
          isImpossible = true;
          break;
        }
      }
    }
    // impossible cuts, then skip
    if (isImpossible) {
      continue;
    }

    // Fill in the Partitioning information
    PartClass lClass = None, uClass = None;
    if (partition.partClass == PartClass::ALL) {
      lClass = (isHorizontal) ? W : S;
      uClass = (isHorizontal) ? E : N;
    }

    if (partition.partClass == W) {
      lClass = SW;
      uClass = NW;
    }
    if (partition.partClass == E) {
      lClass = SE;
      uClass = NE;
    }

    if (partition.partClass == N) {
      lClass = NW;
      uClass = NE;
    }

    if (partition.partClass == S) {
      lClass = SW;
      uClass = SE;
    }

    Partition lowerPart(
        lClass,
        partition.lx,
        partition.ly,
        (isHorizontal) ? cutLine - partition.lx : partition.width,
        (isHorizontal) ? partition.height : cutLine - partition.ly,
        log);

    Partition upperPart(
        uClass,
        (isHorizontal) ? cutLine : partition.lx,
        (isHorizontal) ? partition.ly : cutLine,
        (isHorizontal) ? partition.lx + partition.width - cutLine
                       : partition.width,
        (isHorizontal) ? partition.height
                       : partition.ly + partition.height - cutLine,
        log);

    //
    // Fill in child partitons' macroStor
    for (auto& curMacro : partition.macroStor) {
      int i = &curMacro - &partition.macroStor[0];
      if (chkArr[i] == 1) {
        lowerPart.macroStor.push_back(Macro(curMacro.lx,
                                            curMacro.ly,
                                            curMacro.w,
                                            curMacro.h,
                                            curMacro.haloX,
                                            curMacro.haloY,
                                            curMacro.channelX,
                                            curMacro.channelY,
                                            curMacro.dbInstPtr));
      } else if (chkArr[i] == 2) {
        upperPart.macroStor.push_back(
            Macro((isHorizontal) ? curMacro.lx - cutLine : curMacro.lx,
                  (isHorizontal) ? curMacro.ly : curMacro.ly - cutLine,
                  curMacro.w,
                  curMacro.h,
                  curMacro.haloX,
                  curMacro.haloY,
                  curMacro.channelX,
                  curMacro.channelY,
                  curMacro.dbInstPtr));
      } else if (chkArr[i] == 3) {
        double centerPoint = (isHorizontal) ? curMacro.lx + curMacro.w / 2.0
                                            : curMacro.ly + curMacro.h / 2.0;

        if (centerPoint < cutLine) {
          lowerPart.macroStor.push_back(Macro(curMacro.lx,
                                              curMacro.ly,
                                              curMacro.w,
                                              curMacro.h,
                                              curMacro.haloX,
                                              curMacro.haloY,
                                              curMacro.channelX,
                                              curMacro.channelY,
                                              curMacro.dbInstPtr));

        } else {
          upperPart.macroStor.push_back(
              Macro((isHorizontal) ? curMacro.lx - cutLine : curMacro.lx,
                    (isHorizontal) ? curMacro.ly : curMacro.ly - cutLine,
                    curMacro.w,
                    curMacro.h,
                    curMacro.haloX,
                    curMacro.haloY,
                    curMacro.channelX,
                    curMacro.channelY,
                    curMacro.dbInstPtr));
        }
      }
    }

    double lowerArea = lowerPart.width * lowerPart.height;
    double upperArea = upperPart.width * upperPart.height;

    double upperMacroArea = 0.0f;
    double lowerMacroArea = 0.0f;

    for (auto& curMacro : upperPart.macroStor) {
      upperMacroArea += curMacro.w * curMacro.h;
    }
    for (auto& curMacro : lowerPart.macroStor) {
      lowerMacroArea += curMacro.w * curMacro.h;
    }

    // impossible partitioning
    if (upperMacroArea > upperArea || lowerMacroArea > lowerArea) {
      log->info(MPL, 80, "Impossible partiton found. Continue");
      continue;
    }

    pair<Partition, Partition> curPart(lowerPart, upperPart);
    ret.push_back(curPart);
  }
  log->report("End Partition");

  return ret;
}

void MacroPlacer::FillMacroStor()
{
  dbBlock* block = db_->getChip()->getBlock();
  const int dbu = db_->getTech()->getDbUnitsPerMicron();
  for (dbInst* inst : block->getInsts()) {
    if (inst->getMaster()->getType().isBlock()) {
      // for Macro cells
      dbPlacementStatus dps = inst->getPlacementStatus();
      if (dps == dbPlacementStatus::NONE || dps == dbPlacementStatus::UNPLACED) {
        logger_->error(MPL,
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

      int placeX, placeY;
      inst->getLocation(placeX, placeY);

      macroInstMap[inst] = macroStor.size();
      Macro macro(1.0 * placeX / dbu,
                  1.0 * placeY / dbu,
                  1.0 * inst->getBBox()->getDX() / dbu,
                  1.0 * inst->getBBox()->getDY() / dbu,
                  curHaloX,
                  curHaloY,
                  curChannelX,
                  curChannelY,
                  inst);
      macroStor.push_back(macro);
    }
  }

  if (macroStor.empty()) {
    logger_->error(MPL, 4, "No macros found.");
  }

  logger_->info(MPL, 5, "NumMacros {}", macroStor.size());
}

static bool isWithIn(int val, int min, int max)
{
  return ((min <= val) && (val <= max));
}

static float getRoundUpFloat(float x, float unit)
{
  return std::round(x / unit) * unit;
}

void MacroPlacer::UpdateMacroCoordi(Partition& part)
{
  dbTech* tech = db_->getTech();
  dbTechLayer* fourLayer = tech->findRoutingLayer(4);
  if (!fourLayer) {
    logger_->warn(MPL,
               21,
               "Metal 4 not exist! Macro snapping will not be applied on "
               "Metal4 pitch");
  }

  const float pitchX = static_cast<float>(fourLayer->getPitchX())
    / static_cast<float>(tech->getDbUnitsPerMicron());
  const float pitchY = static_cast<float>(fourLayer->getPitchY())
    / static_cast<float>(tech->getDbUnitsPerMicron());

  for (auto& curMacro : part.macroStor) {
    auto mnPtr = macroInstMap.find(curMacro.dbInstPtr);
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

void MacroPlacer::UpdateNetlist(Partition& layout)
{
  if (netTable_) {
    delete[] netTable_;
    netTable_ = 0;
  }

  assert(layout.macroStor.size() == macroStor.size());
  size_t tableSize = (macroStor.size() + core_edge_count)
    * (macroStor.size() + core_edge_count);

  netTable_ = new double[tableSize];
  for (size_t i = 0; i < tableSize; i++) {
    netTable_[i] = layout.netTable[i];
  }
}

#define EAST_IDX (macroStor.size())
#define WEST_IDX (macroStor.size() + 1)
#define NORTH_IDX (macroStor.size() + 2)
#define SOUTH_IDX (macroStor.size() + 3)

double MacroPlacer::GetWeightedWL()
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

static CoreEdge getCoreEdge(int cx,
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
    return CoreEdge::West;
  } else if (minDiff == uxDx) {
    return CoreEdge::East;
  } else if (minDiff == lyDy) {
    return CoreEdge::South;
  } else if (minDiff == uyDy) {
    return CoreEdge::North;
  }
  return CoreEdge::West;
}

////////////////////////////////////////////////////////////////

// Use OpenSTA graph to find macro adjacencies.
// No delay calculation or arrival search is required,
// just gate connectivity in the levelized graph.
void MacroPlacer::findAdjacencies()
{
  sta::dbNetwork *network = sta_->getDbNetwork();
  sta::Graph *graph = sta_->ensureGraph();
  sta_->ensureLevelized();
  sta_->ensureClkNetwork();
  VertexFaninMap vertex_fanins;
  sta::SearchPred2 srch_pred(sta_);
  sta::BfsFwdIterator bfs(sta::BfsIndex::other, &srch_pred, sta_);

  seedFaninBfs(bfs, vertex_fanins);
  findFanins(bfs, vertex_fanins);

  // Propagate fanins through 3 levels of register D->Q.
  constexpr int reg_adjacency_depth = 3;
  for (int i = 0; i < reg_adjacency_depth; i++) {
    copyFaninsAcrossRegisters(bfs, vertex_fanins);
    findFanins(bfs, vertex_fanins);
  }

  AdjWeightMap adj_map;
  findAdjWeights(vertex_fanins, adj_map);
  
  fillMacroWeights(adj_map);
}

void MacroPlacer::seedFaninBfs(sta::BfsFwdIterator &bfs,
                               VertexFaninMap &vertex_fanins)
{
  sta::dbNetwork *network = sta_->getDbNetwork();
  sta::Graph *graph = sta_->ensureGraph();
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
      CoreEdge edge = findNearestEdge(bterm);
      vertex_fanins[vertex].insert(reinterpret_cast<Macro*>(edge));
      bfs.enqueueAdjacentVertices(vertex);
    }
  }
}

// BFS search forward union-ing fanins.
// BFS stops at register inputs because there are no timing arcs
// from register D->Q.
void MacroPlacer::findFanins(sta::BfsFwdIterator &bfs,
                             VertexFaninMap &vertex_fanins)
{
  sta::dbNetwork *network = sta_->getDbNetwork();
  sta::Graph *graph = sta_->ensureGraph();
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
        debugPrint(logger_, MPL, "find_fanins", 1, "{} + {}",
                   vertex->name(network),
                   faninName(fanin));
      }
    }
    bfs.enqueueAdjacentVertices(vertex);
  }
}

void MacroPlacer::copyFaninsAcrossRegisters(sta::BfsFwdIterator &bfs,
                                            VertexFaninMap &vertex_fanins)
{
  sta::dbNetwork *network = sta_->getDbNetwork();
  sta::Graph *graph = sta_->ensureGraph();
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
          sta::Pin *out_pin = findSeqOutPin(inst, out_port);
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
sta::Pin *MacroPlacer::findSeqOutPin(sta::Instance *inst,
                                     sta::LibertyPort *out_port)
{
  sta::dbNetwork *network = sta_->getDbNetwork();
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

void MacroPlacer::findAdjWeights(VertexFaninMap &vertex_fanins,
                                 AdjWeightMap &adj_map)
{
  sta::dbNetwork *network = sta_->getDbNetwork();
  sta::Graph *graph = sta_->ensureGraph();
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
      CoreEdge edge = findNearestEdge(bterm);
      debugPrint(logger_, MPL, "pin_edge", 1, "pin edge {} {}",
                 bterm->getConstName(),
                 coreEdgeString(edge));
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
}

// Fill macroWeight array.
void MacroPlacer::fillMacroWeights(AdjWeightMap &adj_map)
{
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
        debugPrint(logger_, MPL, "weights", 1, "{} -> {} {}",
                   faninName(from),
                   faninName(to),
                   weight);
    }
  }
}

std::string MacroPlacer::faninName(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  if (edge_index < core_edge_count)
    return coreEdgeString(static_cast<CoreEdge>(edge_index));
  else
    return macro->name();
}

int MacroPlacer::macroIndex(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  if (edge_index < core_edge_count)
    return edge_index;
  else
    return macro - &macroStor[0] + core_edge_count;
}

bool MacroPlacer::macroIndexIsEdge(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  return edge_index < 4;
}

// This is completely broken but I want to match FillPinGroup()
// until it is flushed.
// It assumes the pins are on the core boundary.
// It should look for the nearest edge to the pin center. -cherry
CoreEdge MacroPlacer::findNearestEdge(dbBTerm* bTerm)
{
  dbPlacementStatus status = bTerm->getFirstPinPlacementStatus();
  if (status == dbPlacementStatus::UNPLACED
      || status == dbPlacementStatus::NONE) {
    logger_->warn(MPL, 11, "pin {} is not placed. Using west.",
               bTerm->getConstName());
    return CoreEdge::West;
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

      if (isWithIn(dbuCoreLx, boxLx, boxUx)) {
        return CoreEdge::West;
      } else if (isWithIn(dbuCoreUx, boxLx, boxUx)) {
        return CoreEdge::East;
      } else if (isWithIn(dbuCoreLy, boxLy, boxUy)) {
        return CoreEdge::South;
      } else if (isWithIn(dbuCoreUy, boxLy, boxUy)) {
        return CoreEdge::North;
      }
    }
    if (!isAxisFound) {
      dbBPin* bPin = *(bTerm->getBPins().begin());
      Rect pin_bbox = bPin->getBBox();
      int boxLx = pin_bbox.xMin();
      int boxLy = pin_bbox.yMin();
      int boxUx = pin_bbox.xMax();
      int boxUy = pin_bbox.yMax();
      return getCoreEdge((boxLx + boxUx) / 2,
                         (boxLy + boxUy) / 2,
                         dbuCoreLx,
                         dbuCoreLy,
                         dbuCoreUx,
                         dbuCoreUy);
    }
  }
  return CoreEdge::West;
}

const char *coreEdgeString(CoreEdge edge)
{
  switch (edge) {
  case CoreEdge::West:
    return "West";
  case CoreEdge::East:
    return "East";
  case CoreEdge::North:
    return "North";
  case CoreEdge::South:
    return "South";
  default:
    return "??";
  }
}

int coreEdgeIndex(CoreEdge edge)
{
  return static_cast<int>(edge);
}

CoreEdge coreEdgeFromIndex(int edge_index)
{
  return static_cast<CoreEdge>(edge_index);
}

////////////////////////////////////////////////////////////////

// Most of what is in this class is the dbInst and should be functions
// instead of class variables. -cherry
Macro::Macro(double _lx,
             double _ly,
             double _w,
             double _h,
             double _haloX,
             double _haloY,
             double _channelX,
             double _channelY,
             odb::dbInst* _dbInstPtr)
    : lx(_lx),
      ly(_ly),
      w(_w),
      h(_h),
      haloX(_haloX),
      haloY(_haloY),
      channelX(_channelX),
      channelY(_channelY),
      dbInstPtr(_dbInstPtr)
{
}

std::string Macro::name()
{
  return dbInstPtr->getName();
}

std::string Macro::type()
{
  return dbInstPtr->getMaster()->getName();
}

MacroLocalInfo::MacroLocalInfo()
    : haloX_(0), haloY_(0), channelX_(0), channelY_(0)
{
}

}  // namespace mpl
