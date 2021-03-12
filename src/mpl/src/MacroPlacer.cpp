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
#include "graphics.h"

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

using std::round;
using std::min;
using std::max;

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

////////////////////////////////////////////////////////////////

MacroPlacer::MacroPlacer()
    : db_(nullptr),
      sta_(nullptr),
      logger_(nullptr),
      connection_driven_(false),
      lx_(0),
      ly_(0),
      ux_(0),
      uy_(0),
      verbose_(1),
      solution_count_(0),
      gui_debug_(false),
      gui_debug_partitions_(false)
{
}

void MacroPlacer::init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log)
{
  db_ = db;
  sta_ = sta;
  logger_ = log;
}

void MacroPlacer::setDebug(bool partitions)
{
  gui_debug_ = true;
  gui_debug_partitions_ = partitions;
}

void MacroPlacer::setHalo(double halo_x, double halo_y)
{
  default_macro_spacings_.setHalo(halo_x, halo_y);
}

void MacroPlacer::setChannel(double channel_x, double channel_y)
{
  default_macro_spacings_.setChannel(channel_x, channel_y);
}

void MacroPlacer::setVerboseLevel(int verbose)
{
  verbose_ = verbose;
}

void MacroPlacer::setFenceRegion(double lx, double ly, double ux, double uy)
{
  lx_ = lx;
  ly_ = ly;
  ux_ = ux;
  uy_ = uy;
}

void MacroPlacer::setSnapLayer(odb::dbTechLayer *snap_layer)
{
  snap_layer_ = snap_layer;
}

int MacroPlacer::getSolutionCount()
{
  return solution_count_;
}

void MacroPlacer::init()
{
  findMacros();

  // Connection driven will be disabled if some instances are missing liberty cells.
  connection_driven_ = !isMissingLiberty();

  if (connection_driven_) {
    reportEdgePinCounts();
    findAdjacencies();
  } else {
    logger_->warn(MPL, 2, "Some instances do not have liberty models. TritonMP will place macros without connection information.");
  }
}

bool MacroPlacer::isMissingLiberty()
{
  sta::Network *network = sta_->network();
  sta::LeafInstanceIterator* instIter = network->leafInstanceIterator();
  while (instIter->hasNext()) {
    sta::Instance* inst = instIter->next();
    if (network->libertyCell(inst) == nullptr) {
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
    CoreEdge edge = coreEdgeFromIndex(i);
    logger_->info(MPL, 9, "{} pins {}",
                  coreEdgeString(edge),
                  counts[i]);
  }
}

// Use parquefp on all the macros.
void MacroPlacer::placeMacrosCenterSpread()
{
  init();

  double wl = getWeightedWL();
  logger_->info(MPL, 71, "Initial weighted wire length {:g}", wl);

  // All of this partitioning garbage is unnecessary but survives to support
  // the multiple partition algorithm.

  Layout layout(lx_, ly_, ux_, uy_);
  bool horizontal = true;
  Partition partition(PartClass::ALL, lx_, ly_, ux_ - lx_, uy_ - ly_, this, logger_);
  partition.macros_ = macros_;

  MacroPartMap globalMacroPartMap;
  updateMacroPartMap(partition, globalMacroPartMap);

  if (connection_driven_) {
    partition.fillNetlistTable(globalMacroPartMap);
  }

  // Annealing based on ParquetFP Engine
  if (partition.anneal()) {
    setDbInstLocations(partition);

    double curWwl = getWeightedWL();
    logger_->info(MPL, 71, "Placed weighted wire length {:g}", curWwl);
  }
  else
    logger_->warn(MPL, 72, "Partitioning failed.");
}

void MacroPlacer::setDbInstLocations(Partition &partition)
{
  double width = ux_ - lx_;
  double height = uy_ - ly_;
  double x_scale = width / partition.solution_width;
  double y_scale = height / partition.solution_height;

  odb::dbTech* tech = db_->getTech();
  const int dbu = tech->getDbUnitsPerMicron();
  const float pitch_x = static_cast<float>(snap_layer_->getPitchX()) / dbu;
  const float pitch_y = static_cast<float>(snap_layer_->getPitchY()) / dbu;

  int macro_idx = 0;
  for (Macro& pmacro : partition.macros_) {
    // partition macros are 1:1 with macros_.
    Macro& macro = macros_[macro_idx];

    //double x = lx_ + (pmacro.lx - lx_) * x_scale;
    //double y = ly_ + (pmacro.ly - ly_) * y_scale;

    // Translate macro cluster to die center and then spread to fill.
    // This works better than spreading origins because that leaves the
    // macros biased toward the lower left corner.
    double x = lx_ + (pmacro.lx - lx_ + macro.w / 2) * x_scale - macro.w / 2;
    double y = ly_ + (pmacro.ly - ly_ + macro.h / 2) * y_scale - macro.h / 2;

    // Snap to routing grid.
    x = round(x / pitch_x) * pitch_x;
    y = round(y / pitch_y) * pitch_y;

    // Snap macro location to grid.
    macro.lx = x;
    macro.ly = y;

    // Update db inst location.
    dbInst *db_inst = macro.dbInstPtr;
    db_inst->setLocation(round(x * dbu), round(y * dbu));
    db_inst->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
    macro_idx++;
  }
}

////////////////////////////////////////////////////////////////

// Use some undocumented method with cut lines to break the design
// into regions and try all combinations. Pick the one that maximizes (yes, really)
// wire lengths of connections between the macros to force them to the corners.
void MacroPlacer::placeMacrosCornerMaxWl()
{
  init();

  double wl = getWeightedWL();
  logger_->info(MPL, 71, "Initial weighted wire length {:g}", wl);

  Layout layout(lx_, ly_, ux_, uy_);
  bool horizontal = true;
  Partition top_partition(PartClass::ALL, lx_, ly_, ux_ - lx_, uy_ - ly_, this, logger_);
  top_partition.macros_ = macros_;

  logger_->report("Begin One Level Partition");

  TwoPartitions oneLevelPart = getPartitions(layout, top_partition, horizontal);

  logger_->report("End One Level Partition");
  TwoPartitions east_partitions, west_partitions;

  vector<vector<Partition>> allSets;

  MacroPartMap globalMacroPartMap;
  updateMacroPartMap(top_partition, globalMacroPartMap);

  if (connection_driven_) {
    top_partition.fillNetlistTable(globalMacroPartMap);
  }

  // push to the outer vector
  vector<Partition> layoutSet;
  layoutSet.push_back(top_partition);

  allSets.push_back(layoutSet);
  for (auto& partition_set : oneLevelPart) {
    if (horizontal) {
      logger_->report("Begin Horizontal Partition");
      Layout eastInfo(layout, partition_set.first);
      Layout westInfo(layout, partition_set.second);

      logger_->report("Begin East Partition");
      TwoPartitions east_partitions = getPartitions(eastInfo, partition_set.first, !horizontal);
      logger_->report("End East Partition");

      logger_->report("Begin West Partition");
      TwoPartitions west_partitions = getPartitions(westInfo, partition_set.second, !horizontal);
      logger_->report("End West Partition");

      // Zero case handling when east_partitions = 0
      if (east_partitions.empty() && !west_partitions.empty()) {
        for (size_t i = 0; i < west_partitions.size(); i++) {
          vector<Partition> partition_set;

          // one set is composed of two subblocks
          partition_set.push_back(west_partitions[i].first);
          partition_set.push_back(west_partitions[i].second);

          // Fill Macro Netlist
          // update macroPartMap
          MacroPartMap macroPartMap;
          for (auto& partition_set : partition_set) {
            updateMacroPartMap(partition_set, macroPartMap);
          }

          if (connection_driven_) {
            for (auto& partition_set : partition_set) {
              partition_set.fillNetlistTable(macroPartMap);
            }
          }

          allSets.push_back(partition_set);
        }
      }
      // Zero case handling when west_partitions = 0
      else if (!east_partitions.empty() && west_partitions.empty()) {
        for (size_t i = 0; i < east_partitions.size(); i++) {
          vector<Partition> partition_set;

          // one set is composed of two subblocks
          partition_set.push_back(east_partitions[i].first);
          partition_set.push_back(east_partitions[i].second);

          // Fill Macro Netlist
          // update macroPartMap
          MacroPartMap macroPartMap;
          for (auto& partition_set : partition_set) {
            updateMacroPartMap(partition_set, macroPartMap);
          }

          if (connection_driven_) {
            for (auto& partition_set : partition_set) {
              partition_set.fillNetlistTable(macroPartMap);
            }
          }

          allSets.push_back(partition_set);
        }
      } else {
        // for all possible partition combinations
        for (size_t i = 0; i < east_partitions.size(); i++) {
          for (size_t j = 0; j < west_partitions.size(); j++) {
            vector<Partition> partition_set;

            // one set is composed of four subblocks
            partition_set.push_back(east_partitions[i].first);
            partition_set.push_back(east_partitions[i].second);
            partition_set.push_back(west_partitions[j].first);
            partition_set.push_back(west_partitions[j].second);

            MacroPartMap macroPartMap;
            for (auto& partition_set : partition_set) {
              updateMacroPartMap(partition_set, macroPartMap);
            }

            if (connection_driven_) {
              for (auto& partition_set : partition_set) {
                partition_set.fillNetlistTable(macroPartMap);
              }
            }
            allSets.push_back(partition_set);
          }
        }
      }
      logger_->report("End Horizontal Partition");
    } else {
      // Vertical partition support MIA
    }
  }
  logger_->info(MPL, 70, "Using {} partition sets", allSets.size() - 1);

  std::unique_ptr<Graphics> graphics;
  if (gui_debug_ && Graphics::guiActive()) {
    graphics = std::make_unique<Graphics>(db_, logger_);
  }

  solution_count_ = 0;
  bool found_best = false;
  int best_setIdx = 0;
  double bestWwl = -DBL_MAX;
  for (auto& partition_set : allSets) {
    // skip for top partition
    if (partition_set.size() == 1) {
      continue;
    }

    if (gui_debug_) {
      graphics->status("Pre-anneal");
      graphics->set_partitions(partition_set, true);
    }

    // For each of the 4 partitions
    bool isFailed = false;
    for (auto& curPart : partition_set) {
      // Annealing based on ParquetFP Engine
      bool success = curPart.anneal();
      if (!success) {
        logger_->warn(MPL, 61, "Parquet area {:g} x {:g} exceeds the partition area {:g} x {:g}.",
                      curPart.solution_width,
                      curPart.solution_height,
                      curPart.width,
                      curPart.height);
        isFailed = true;
        break;
      }
      // Update mckt frequently
      updateMacroLocations(curPart);
    }

    if (isFailed) {
      continue;
    }

    double curWwl = getWeightedWL();
    logger_->info(MPL, 71, "Solution {} weighted wire length {:g}",
                  solution_count_ + 1,
                  curWwl);
    bool is_best = false;
    if (!found_best
        // Note that this MAXIMIZES wirelength.
        // That is they way mingyu wrote it.
        // This is the only thing that keeps all the macros from ending
        // up in one clump. -cherry
        || curWwl > bestWwl) {
      bestWwl = curWwl;
      best_setIdx = &partition_set - &allSets[0];
      found_best = true;
      is_best = true;
    }
    solution_count_++;

    if (gui_debug_) {
      auto msg("Post-anneal WL: " + std::to_string(curWwl));
      if (is_best) {
        msg += " [BEST]";
      }
      graphics->status(msg);
      graphics->set_partitions(partition_set, false);
    }
  }

  if (found_best) {
    logger_->info(MPL, 73, "Best weighted wire length {:g}", bestWwl);
    std::vector<Partition> best_set = allSets[best_setIdx];
    for (auto& best_partition : best_set) {
      updateMacroLocations(best_partition);
    }
    updateDbInstLocations();
  }
  else
    logger_->warn(MPL, 72, "No partition solutions found.");
}

int MacroPlacer::weight(int idx1, int idx2)
{
  return macro_weights_[idx1][idx2];
}

// Update opendb instance locations from macros.
void MacroPlacer::updateDbInstLocations()
{
  odb::dbTech* tech = db_->getTech();
  const int dbu = tech->getDbUnitsPerMicron();

  for (auto& macro : macros_) {
    macro.dbInstPtr->setLocation(round(macro.lx * dbu),
                                 round(macro.ly * dbu));
    macro.dbInstPtr->setPlacementStatus(odb::dbPlacementStatus::LOCKED);
  }
}

void MacroPlacer::cutRoundUp(const Layout& layout,
                             double& cutLine,
                             bool horizontal)
{
  dbBlock* block = db_->getChip()->getBlock();
  dbSet<dbRow> rows = block->getRows();
  const double dbu = db_->getTech()->getDbUnitsPerMicron();
  dbSite* site = rows.begin()->getSite();
  if (horizontal) {
    double siteSizeX = site->getWidth() / dbu;
    cutLine = round(cutLine / siteSizeX) * siteSizeX;
    cutLine = min(cutLine, layout.ux());
    cutLine = max(cutLine, layout.lx());
  } else {
    double siteSizeY = site->getHeight() / dbu;
    cutLine = round(cutLine / siteSizeY) * siteSizeY;
    cutLine = min(cutLine, layout.uy());
    cutLine = max(cutLine, layout.ly());
  }
}

// using partition,
// fill in macroPartMap
//
// macroPartMap will contain
//
// first: macro partition class info
// second: macro candidates.
void MacroPlacer::updateMacroPartMap(Partition& part,
                                     MacroPartMap& macroPartMap)
{
  // This does not look like it actually does anything -cherry
  vector<int> macros = macroPartMap[part.partClass];
  // convert macro Information into macroIdx
  for (auto& macro : part.macros_) {
    int macro_index = macro_inst_map_[macro.dbInstPtr];
    macros.push_back(macro_index);
  }
  macroPartMap[part.partClass] = macros;
}

// only considers lx or ly coordinates for sorting
static bool segLxLyLess(const std::pair<int, double>& p1,
                        const std::pair<int, double>& p2)
{
  return p1.second < p2.second;
}

// Two partitioning functions:
// first : lower part
// second : upper part
//
// cutLine is sweeping from lower to upper coordinates in x / y
vector<pair<Partition, Partition>>
MacroPlacer::getPartitions(const Layout& layout,
                           const Partition& partition,
                           bool horizontal)
{
  logger_->info(MPL, 76, "Partition {} macros", partition.macros_.size());

  vector<pair<Partition, Partition>> partitions;

  double maxWidth = 0.0;
  double maxHeight = 0.0;

  // segments
  // In a sane implementation this would be a vector of Macro*
  // and use a function to return the origin x/y. -cherry
  vector<std::pair<int, double>> segments;

  // in parent partition, traverse macros
  for (const Macro& macro : partition.macros_) {
    segments.push_back(
        std::make_pair(&macro - &partition.macros_[0],
                       (horizontal) ? macro.lx : macro.ly));

    maxWidth = std::max(maxWidth, macro.w);
    maxHeight = std::max(maxHeight, macro.h);
  }

  double cutLineLimit = (horizontal) ? maxWidth * 0.25 : maxHeight * 0.25;
  vector<double> cutlines;

  // less than 4
  if (partition.macros_.size() <= 4) {
    sort(segments.begin(), segments.end(), segLxLyLess);

    double prevPushLimit = -1e30;
    bool isFirst = true;
    // first : macros_ index
    // second : macro lower coordinates
    for (auto& segPair : segments) {
      if (isFirst) {
        cutlines.push_back(segPair.second);
        prevPushLimit = segPair.second;
        isFirst = false;
      } else if (std::abs(segPair.second - prevPushLimit) > cutLineLimit) {
        cutlines.push_back(segPair.second);
        prevPushLimit = segPair.second;
      }
    }
  }
  // more than 4
  else {
    int hardLimit = round(std::sqrt(partition.macros_.size() / 3.0));
    for (int i = 0; i <= hardLimit; i++) {
      cutlines.push_back(
          (horizontal)
              ? layout.lx() + (layout.ux() - layout.lx()) / hardLimit * i
              : layout.ly() + (layout.uy() - layout.ly()) / hardLimit * i);
    }
  }
  logger_->info(MPL, 77, "Using {} cut lines", cutlines.size());

  // Macro checker array
  // 0 for uninitialize
  // 1 for lower
  // 2 for upper
  // 3 for both
  vector<int> chkArr(partition.macros_.size());

  for (auto& cutLine : cutlines) {
    cutRoundUp(layout, cutLine, horizontal);

    logger_->info(MPL, 79, "Cut line {:.2f}", cutLine);

    // chkArr initialize
    for (size_t i = 0; i < partition.macros_.size(); i++) {
      chkArr[i] = 0;
    }

    bool isImpossible = false;
    for (auto& macro : partition.macros_) {
      int i = &macro - &partition.macros_[0];
      if (horizontal) {
        // lower is possible
        if (macro.w <= cutLine) {
          chkArr[i] += 1;
        }
        // upper is possible
        if (macro.w <= partition.lx + partition.width - cutLine) {
          chkArr[i] += 2;
        }
        // none of them
        if (chkArr[i] == 0) {
          isImpossible = true;
          break;
        }
      } else {
        // lower is possible
        if (macro.h <= cutLine) {
          chkArr[i] += 1;
        }
        // upper is possible
        if (macro.h <= partition.ly + partition.height - cutLine) {
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
    switch (partition.partClass) {
    case PartClass::ALL:
      lClass = (horizontal) ? W : S;
      uClass = (horizontal) ? E : N;
      break;
    case PartClass::W:
      lClass = SW;
      uClass = NW;
      break;
    case PartClass::E:
      lClass = SE;
      uClass = NE;
      break;
    case PartClass::N:
      lClass = NW;
      uClass = NE;
      break;
    case PartClass::S:
      lClass = SW;
      uClass = SE;
      break;
    default:
      logger_->error(MPL, 12, "unhandled partition class");
      lClass = W;
      uClass = E;
      break;
    }

    Partition lowerPart(
        lClass,
        partition.lx,
        partition.ly,
        (horizontal) ? cutLine - partition.lx : partition.width,
        (horizontal) ? partition.height : cutLine - partition.ly,
        this,
        logger_);

    Partition upperPart(
        uClass,
        (horizontal) ? cutLine : partition.lx,
        (horizontal) ? partition.ly : cutLine,
        (horizontal) ? partition.lx + partition.width - cutLine
                       : partition.width,
        (horizontal) ? partition.height
                       : partition.ly + partition.height - cutLine,
        this,
        logger_);

    // Fill in child partitons' macros_
    for (const Macro& macro : partition.macros_) {
      int i = &macro - &partition.macros_[0];
      if (chkArr[i] == 1) {
        lowerPart.macros_.push_back(macro);
      } else if (chkArr[i] == 2) {
        upperPart.macros_.push_back(
            Macro((horizontal) ? macro.lx - cutLine : macro.lx,
                  (horizontal) ? macro.ly : macro.ly - cutLine,
                  macro));
      } else if (chkArr[i] == 3) {
        double centerPoint = (horizontal) ? macro.lx + macro.w / 2.0
                                            : macro.ly + macro.h / 2.0;

        if (centerPoint < cutLine) {
          lowerPart.macros_.push_back(macro);

        } else {
          upperPart.macros_.push_back(
              Macro((horizontal) ? macro.lx - cutLine : macro.lx,
                    (horizontal) ? macro.ly : macro.ly - cutLine,
                    macro));
        }
      }
    }

    double lowerArea = lowerPart.width * lowerPart.height;
    double upperArea = upperPart.width * upperPart.height;

    double upperMacroArea = 0.0f;
    double lowerMacroArea = 0.0f;

    for (auto& macro : upperPart.macros_) {
      upperMacroArea += macro.w * macro.h;
    }
    for (auto& macro : lowerPart.macros_) {
      lowerMacroArea += macro.w * macro.h;
    }

    // impossible partitioning
    if (upperMacroArea > upperArea || lowerMacroArea > lowerArea) {
      logger_->info(MPL, 80, "Impossible partiton found.");
      continue;
    }

    pair<Partition, Partition> curPart(lowerPart, upperPart);
    partitions.push_back(curPart);
  }
  return partitions;
}

// ParqueFP Node putHaloX/Y, putChannelX/Y are non-functional.
// Simulate them by expanding the macro size.
// Halo and 1/2 channel on both left/right top/bottom.
double MacroPlacer::paddedOriginX(const Macro &macro)
{
  MacroSpacings &spacings = getSpacings(macro);
  return macro.lx - spacings.getSpacingX();
}

double MacroPlacer::paddedOriginY(const Macro &macro)
{
  MacroSpacings &spacings = getSpacings(macro);
  return macro.ly - spacings.getSpacingY();
}

double MacroPlacer::paddedWidth(const Macro &macro)
{
  MacroSpacings &spacings = getSpacings(macro);
  return macro.w + spacings.getSpacingX() * 2;
}

double MacroPlacer::paddedHeight(const Macro &macro)
{
  MacroSpacings &spacings = getSpacings(macro);
  return macro.h + spacings.getSpacingY() * 2;
}

void MacroPlacer::findMacros()
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

      int placeX, placeY;
      inst->getLocation(placeX, placeY);

      macro_inst_map_[inst] = macros_.size();
      Macro macro(1.0 * placeX / dbu,
                  1.0 * placeY / dbu,
                  1.0 * inst->getBBox()->getDX() / dbu,
                  1.0 * inst->getBBox()->getDY() / dbu,
                  inst);
      macros_.push_back(macro);
    }
  }

  if (macros_.empty()) {
    logger_->error(MPL, 4, "No macros found.");
  }

  logger_->info(MPL, 5, "Found {} macros", macros_.size());
}

static bool isWithIn(int val, int min, int max)
{
  return ((min <= val) && (val <= max));
}

static float getRoundUpFloat(float x, float unit)
{
  return round(x / unit) * unit;
}

void MacroPlacer::updateMacroLocations(Partition& part)
{
  dbTech* tech = db_->getTech();
  const float pitchX = static_cast<float>(snap_layer_->getPitchX())
    / tech->getDbUnitsPerMicron();
  const float pitchY = static_cast<float>(snap_layer_->getPitchY())
    / tech->getDbUnitsPerMicron();

  for (auto& macro : part.macros_) {
    // snap location to routing layer grid
    float macroX = getRoundUpFloat(macro.lx, pitchX);
    float macroY = getRoundUpFloat(macro.ly, pitchY);
    macro.lx = macroX;
    macro.ly = macroY;
    // Update Macro Location
    int macroIdx = macro_inst_map_[macro.dbInstPtr];
    macros_[macroIdx].lx = macroX;
    macros_[macroIdx].ly = macroY;
  }
}

#define EAST_IDX (macros_.size() + coreEdgeIndex(CoreEdge::East))
#define WEST_IDX (macros_.size() + coreEdgeIndex(CoreEdge::West))
#define NORTH_IDX (macros_.size() + coreEdgeIndex(CoreEdge::North))
#define SOUTH_IDX (macros_.size() + coreEdgeIndex(CoreEdge::South))

double MacroPlacer::getWeightedWL()
{
  double wwl = 0.0f;

  double width = ux_ - lx_;
  double height = uy_ - ly_;

  for (size_t i = 0; i < macros_.size() + core_edge_count; i++) {
    for (size_t j = i + 1; j < macros_.size() + core_edge_count; j++) {
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
        pointX1 = macros_[i].lx + macros_[i].w / 2;
        pointY1 = macros_[i].ly + macros_[i].h / 2;
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
        pointX2 = macros_[j].lx + macros_[j].w / 2;
        pointY2 = macros_[j].ly + macros_[j].h / 2;
      }

      float edgeWeight = 0.0f;
      if (connection_driven_) {
        edgeWeight = macro_weights_[i][j];
      } else {
        edgeWeight = 1;
      }
      double wl = std::sqrt((pointX1 - pointX2) * (pointX1 - pointX2)
                              + (pointY1 - pointY2) * (pointY1 - pointY2));
      double weighted_wl = edgeWeight * wl;
      if (edgeWeight > 0)
        debugPrint(logger_, MPL, "weighted_wl", 1,
                   "{} -> {} wl {:.2f} * weight {:.2f} = {:.2f}",
                   macroIndexName(i),
                   macroIndexName(j),
                   wl,
                   edgeWeight,
                   weighted_wl);
      wwl += weighted_wl;
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
  for (int i = 0; i < reg_adjacency_depth_; i++) {
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
  for (Macro& macro : macros_) {
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
  for (Macro& macro : macros_) {
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

// Fill macro_weights_ array.
void MacroPlacer::fillMacroWeights(AdjWeightMap &adj_map)
{
  size_t weight_size = macros_.size() + core_edge_count;
  macro_weights_.resize(weight_size);
  for (size_t i = 0; i < weight_size; i++) {
    macro_weights_[i].resize(weight_size);
    macro_weights_[i] = {0};
  }

  for (auto pair_weight : adj_map) {
    const MacroPair &from_to = pair_weight.first;
    Macro *from = from_to.first;
    Macro *to = from_to.second;
    float weight = pair_weight.second;
    if (!(macroIndexIsEdge(from) && macroIndexIsEdge(to))) {
      int idx1 = macroIndex(from);
      int idx2 = macroIndex(to);
      // Note macro_weights only has entries for idx1 < idx2.
      macro_weights_[min(idx1, idx2)][max(idx1, idx2)] = weight;
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

// This has to be consistent with the accessors in EAST_IDX
int MacroPlacer::macroIndex(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  if (edge_index < core_edge_count)
    return macros_.size() + edge_index;
  else
    return macro - &macros_[0];
}

string MacroPlacer::macroIndexName(int index)
{
  if (index < macros_.size())
    return macros_[index].name();
  else
    return coreEdgeString(static_cast<CoreEdge>(index - macros_.size()));
}

int MacroPlacer::macroIndex(dbInst *inst)
{
  return macro_inst_map_[inst];
}

bool MacroPlacer::macroIndexIsEdge(Macro *macro)
{
  intptr_t edge_index = reinterpret_cast<intptr_t>(macro);
  return edge_index < core_edge_count;
}

// This assumes the pins straddle the die/fence boundary.
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

////////////////////////////////////////////////////////////////

MacroSpacings &
MacroPlacer::getSpacings(const Macro &macro)
{
  auto itr = macro_spacings_.find(macro.dbInstPtr);
  if (itr == macro_spacings_.end())
    return default_macro_spacings_;
  else
    return itr->second;
}

////////////////////////////////////////////////////////////////

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
             odb::dbInst* _dbInstPtr)
    : lx(_lx),
      ly(_ly),
      w(_w),
      h(_h),
      dbInstPtr(_dbInstPtr)
{
}

Macro::Macro(double _lx,
             double _ly,
             const Macro &copy_from)
  : lx(_lx),
    ly(_ly),
      w(copy_from.w),
      h(copy_from.h),
      dbInstPtr(copy_from.dbInstPtr)
{
}

std::string Macro::name()
{
  return dbInstPtr->getName();
}

MacroSpacings::MacroSpacings()
    : halo_x_(0), halo_y_(0), channel_x_(0), channel_y_(0)
{
}

MacroSpacings::MacroSpacings(double halo_x,
                             double halo_y,
                             double channel_x,
                             double channel_y) :
      halo_x_(halo_x),
      halo_y_(halo_y),
      channel_x_(channel_x),
      channel_y_(channel_y)
{
}

void MacroSpacings::setHalo(double halo_x,
                            double halo_y)
{
  halo_x_ = halo_x;
  halo_y_ = halo_y;
}

void MacroSpacings::setChannel(double channel_x,
                               double channel_y)
{
  channel_x_ = channel_x;
  channel_y_ = channel_y;
}

void MacroSpacings::setChannelX(double channel_x)
{
  channel_x_ = channel_x;
}

void MacroSpacings::setChannelY(double channel_y)
{
  channel_y_ = channel_y;
}

double MacroSpacings::getSpacingX() const
{
  return max(halo_x_, channel_x_ / 2);
}

double MacroSpacings::getSpacingY() const
{
  return max(halo_y_, channel_y_ / 2);
}

}  // namespace mpl
