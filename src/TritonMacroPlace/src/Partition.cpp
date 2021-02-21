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

#include "mpl/Partition.h"

#include "btreeanneal.h"
#include "mpl/MacroPlacer.h"
#include "mixedpackingfromdb.h"
#include "utility/Logger.h"

namespace mpl {

using std::min;
using std::max;
using std::make_pair;
using std::to_string;

using utl::MPL;

Partition::Partition(PartClass _partClass,
                     double _lx,
                     double _ly,
                     double _width,
                     double _height,
                     MacroPlacer *macro_placer,
                     utl::Logger* log)
    : partClass(_partClass),
      lx(_lx),
      ly(_ly),
      width(_width),
      height(_height),
      macro_placer_(macro_placer),
      logger_(log)
{
}

Partition::Partition(const Partition& part)
    : partClass(part.partClass),
      lx(part.lx),
      ly(part.ly),
      width(part.width),
      height(part.height),
      macros_(part.macros_),
      net_tbl_(part.net_tbl_),
      macro_placer_(part.macro_placer_),
      logger_(part.logger_)
{
}

#define EAST_IDX (macros_.size() + coreEdgeIndex(CoreEdge::East))
#define WEST_IDX (macros_.size() + coreEdgeIndex(CoreEdge::West))
#define NORTH_IDX (macros_.size() + coreEdgeIndex(CoreEdge::North))
#define SOUTH_IDX (macros_.size() + coreEdgeIndex(CoreEdge::South))

#define GLOBAL_EAST_IDX (macro_placer_->macroCount() + coreEdgeIndex(CoreEdge::East))
#define GLOBAL_WEST_IDX (macro_placer_->macroCount() + coreEdgeIndex(CoreEdge::West))
#define GLOBAL_NORTH_IDX (macro_placer_->macroCount() + coreEdgeIndex(CoreEdge::North))
#define GLOBAL_SOUTH_IDX (macro_placer_->macroCount() + coreEdgeIndex(CoreEdge::South))

string Partition::getName(int macroIdx)
{
  if (macroIdx < macros_.size()) {
    return macros_[macroIdx].name();
  } else {
    return coreEdgeString(coreEdgeFromIndex(macroIdx - macros_.size()));
  }
}

void Partition::fillNetlistTable(MacroPartMap &macroPartMap)
{
  int macro_edge_count = macros_.size() + core_edge_count;
  net_tbl_.resize(macro_edge_count * macro_edge_count);

  if (partClass == ALL) {
    for (size_t i = 0; i < macro_edge_count; i++) {
      for (size_t j = 0; j < macro_edge_count; j++) {
        // Note that net_tbl only entries for i < j.
        net_tbl_[i * macro_edge_count + j] = macro_placer_->weight(i, j);
      }
    }
  }
  else {
    for (size_t i = 0; i < net_tbl_.size(); i++) {
      net_tbl_[i] = 0.0;
    }

    // row
    for (size_t i = 0; i < macro_edge_count; i++) {
      // column
      for (size_t j = 0; j < macro_edge_count; j++) {
        if (i == j)
          continue;
        // from: macro case
        if (i < macros_.size()) {
          int global_idx1 = globalIndex(i);
          // to macro case
          if (j < macros_.size()) {
            int global_idx2 = globalIndex(j);
            net_tbl_[i * macro_edge_count + j]
              = macro_placer_->weight(global_idx1, global_idx2);
          }
          // to IO-west case
          else if (j == WEST_IDX) {
            int weight = macro_placer_->weight(global_idx1, GLOBAL_WEST_IDX);
            if (partClass == PartClass::NE) {
              for (auto& macro_idx : macroPartMap[PartClass::NW]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            if (partClass == PartClass::SE) {
              for (auto& macro_idx : macroPartMap[PartClass::SW]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            net_tbl_[i * macro_edge_count + j] = weight;
          } else if (j == EAST_IDX) {
            int weight = macro_placer_->weight(global_idx1, GLOBAL_EAST_IDX);
            if (partClass == PartClass::NW) {
              for (auto& macro_idx : macroPartMap[PartClass::NE]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            if (partClass == PartClass::SW) {
              for (auto& macro_idx : macroPartMap[PartClass::SE]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            net_tbl_[i * macro_edge_count + j] = weight;
          } else if (j == NORTH_IDX) {
            int weight = macro_placer_->weight(global_idx1, GLOBAL_NORTH_IDX);
            if (partClass == PartClass::SE) {
              for (auto& macro_idx : macroPartMap[PartClass::SE]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            if (partClass == PartClass::SW) {
              for (auto& macro_idx : macroPartMap[PartClass::NW]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            net_tbl_[i * macro_edge_count + j] = weight;
          } else if (j == SOUTH_IDX) {
            int weight = macro_placer_->weight(global_idx1, GLOBAL_SOUTH_IDX);

            if (partClass == PartClass::NE) {
              for (auto& macro_idx : macroPartMap[PartClass::SE]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            if (partClass == PartClass::NW) {
              for (auto& macro_idx : macroPartMap[PartClass::SW]) {
                int global_idx = globalIndex(macro_idx);
                weight += macro_placer_->weight(global_idx1, global_idx);
              }
            }
            net_tbl_[i * macro_edge_count + j] = weight;
          }
        }
        // from IO
        else if (i == WEST_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int global_idx2 = globalIndex(j);
            net_tbl_[i * macro_edge_count + j]
              = macro_placer_->weight(GLOBAL_WEST_IDX, global_idx2);
          }
        } else if (i == EAST_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int global_idx2 = globalIndex(j);
            net_tbl_[i * macro_edge_count + j]
              = macro_placer_->weight(GLOBAL_EAST_IDX, global_idx2);
          }
        } else if (i == NORTH_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int global_idx2 = globalIndex(j);
            net_tbl_[i * macro_edge_count + j]
              = macro_placer_->weight(GLOBAL_NORTH_IDX, global_idx2);
          }
        } else if (i == SOUTH_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int global_idx2 = globalIndex(j);
            net_tbl_[i * macro_edge_count + j]
              = macro_placer_->weight(GLOBAL_SOUTH_IDX, global_idx2);
          }
        }
      }
    }
  }
}

int Partition::globalIndex(int macro_idx)
{
  return macro_placer_->macroIndex(macros_[macro_idx].dbInstPtr);
}

void Partition::updateMacroCoordi()
{
  for (auto& pmacro : macros_) {
    int macro_idx = macro_placer_->macroIndex(pmacro.dbInstPtr);
    Macro &macro = macro_placer_->macro(macro_idx);
    macro.lx = macro.lx;
    macro.ly = macro.ly;
  }
}

// Call ParquetFP
bool Partition::anneal()
{
  // No macro, no need to execute
  if (!macros_.empty()) {
    // Populating DB structure
    // Instantiate Parquet DB structure
    DB db;
    pfp::Nodes* pfp_nodes = db.getNodes();
    pfp::Nets* pfp_nets = db.getNets();

    //////////////////////////////////////////////////////
    // Make node structures for macros.
    for (auto& macro : macros_) {
      MacroSpacings &spacings = macro_placer_->getSpacings(macro);
      // ParqueFP Node putHaloX/Y, putChannelX/Y are non-functional.
      // Simulate them by expanding the macro size.
      // Halo and 1/2 channel on both left/right top/bottom.
      double padded_width = macro.w + max(spacings.getHaloX() * 2, spacings.getChannelX());
      double padded_height = macro.h + max(spacings.getHaloY() * 2, spacings.getChannelY());
      double aspect_ratio = padded_width / padded_height;
      pfp::Node node(macro.name(),
                     padded_width * padded_height,
                     aspect_ratio,
                     aspect_ratio,
                     &macro - &macros_[0],
                     false);
      // These do absolutely nothing.
      node.putHaloX(spacings.getHaloX());
      node.putHaloY(spacings.getHaloY());
      node.putChannelX(spacings.getChannelX());
      node.putChannelY(spacings.getChannelY());

      node.addSubBlockIndex(&macro - &macros_[0]);
      pfp_nodes->putNewNode(node);
    }

    // Make node structures for pin edges.
    int indexTerm = 0;
    for (int i = 0; i < core_edge_count; i++) {
      CoreEdge core_edge = coreEdgeFromIndex(i);
      pfp::Node pin(coreEdgeString(core_edge), 0, 1, 1, indexTerm++, true);
      double x, y;
      switch (core_edge) {
      case CoreEdge::West:
        x = 0.0;
        y = height / 2.0;
        break;
      case CoreEdge::East:
        x = width;
        y = height / 2.0;
        break;
      case CoreEdge::North:
        x = width / 2.0;
        y = height;
        break;
      case CoreEdge::South:
        x = width / 2.0;
        y = 0.0;
        break;
      }
      pin.putX(x);
      pin.putY(y);
      pfp_nodes->putNewTerm(pin);
    }

    //////////////////////////////////////////////////////
    // Feed net / weight structure

    // Preprocessing in macro placer side
    // For nets and wts
    int macro_edge_count = macros_.size() + core_edge_count;
    int pnet_idx = 0;
    for (size_t i = 0; i < macro_edge_count; i++) {
      for (size_t j = i + 1; j < macro_edge_count; j++) {
        int cost = 0;
        if (!net_tbl_.empty()) {
          // Note that net_tbl only entries for i < j.
          cost = net_tbl_[i * macro_edge_count + j]
            + net_tbl_[j * macro_edge_count + i];
        }
        if (cost != 0) {
          makePins(min(i, j), max(i, j), cost, pnet_idx, pfp_nets);
          pnet_idx++;
        }
      }
    }

    if (pnet_idx == 0) {
      for (size_t i = 0; i < core_edge_count; i++) {
        for (size_t j = i + 1; j < core_edge_count; j++) {
          makePins(i, j, 1, pnet_idx, pfp_nets);
          pnet_idx++;
        }
      }
    }

    pfp_nets->updateNodeInfo(*pfp_nodes);
    pfp_nodes->updatePinsInfo(*pfp_nets);

    // Populate MixedBlockInfoType object
    // It is from DB object
    MixedBlockInfoTypeFromDB dbBlockInfo(db);
    MixedBlockInfoType* blockInfo = &dbBlockInfo;

    // Populate Command_Line options.
    pfp::Command_Line param;
    param.minWL = true;
    param.noRotation = true;
    param.FPrep = "BTree";
    param.seed = 100;
    param.scaleTerms = false;

    // Fixed-outline mode in Parquet
    param.nonTrivialOutline = pfp::BBox(0, 0, width, height);
    param.reqdAR = width / height;
    param.maxWS = 0;
    param.verb = 0;

    // Instantiate BTreeAnnealer Object
    pfp::BTreeAreaWireAnnealer* annealer =
      new pfp::BTreeAreaWireAnnealer(*blockInfo, &param, &db);
    annealer->go();

    const pfp::BTree& sol = annealer->currSolution();
    // Failed annealing
    if (sol.totalWidth() > width || sol.totalHeight() > height) {
      logger_->info(MPL, 61, "Parquet BBOX exceed the given area");
      logger_->info(MPL,
                    62,
                    "ParquetSolLayout {:g} {:g}",
                    sol.totalWidth(),
                    sol.totalHeight());
      logger_->info(MPL, 63, "TargetLayout {:g} {:g}", width, height);
      return false;
    }
    delete annealer;

    // flip info initialization for each partition
    bool isFlipX = false, isFlipY = false;
    switch (partClass) {
      // y flip
    case NW:
      isFlipY = true;
      break;
      // x, y flip
    case NE:
      isFlipX = isFlipY = true;
      break;
      // NonFlip
    case SW:
      break;
      // x flip
    case SE:
      isFlipX = true;
      break;
      // very weird
    default:
      break;
    }

    // update back into macro placer
    for (size_t i = 0; i < pfp_nodes->getNumNodes(); i++) {
      pfp::Node& node = pfp_nodes->getNode(i);
      Macro &macro = macros_[i];
      macro.lx = (isFlipX)
        ? width - node.getX() - node.getWidth() + lx
        : node.getX() + lx;
      macro.ly = (isFlipY)
        ? height - node.getY() - node.getHeight() + ly
        : node.getY() + ly;
      MacroSpacings &spacings = macro_placer_->getSpacings(macro);
      macro.lx += spacings.getHaloX() + spacings.getChannelX();
      macro.ly += spacings.getHaloY() + spacings.getChannelY();
    }
  }
  return true;
}

void Partition::makePins(int macro_idx1,
                         int macro_idx2,
                         int cost,
                         int pnet_idx,
                         pfp::Nets* pfp_nets)
{
  pfp::Net pnet;

  parquetfp::pin pin1(getName(macro_idx1).c_str(), true, 0, 0, pnet_idx);
  parquetfp::pin pin2(getName(macro_idx2).c_str(), true, 0, 0, pnet_idx);

  pnet.addNode(pin1);
  pnet.addNode(pin2);
  pnet.putIndex(pnet_idx);
  pnet.putName(string("n" + to_string(pnet_idx)).c_str());
  pnet.putWeight(cost);

  pfp_nets->putNewNet(pnet);
}

}  // namespace mpl
