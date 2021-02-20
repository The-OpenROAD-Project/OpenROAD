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

namespace pfp = parquetfp;

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

Partition::Partition(const Partition& prev)
    : partClass(prev.partClass),
      lx(prev.lx),
      ly(prev.ly),
      width(prev.width),
      height(prev.height),
      macros_(prev.macros_),
      net_tbl_(prev.net_tbl_),
      macro_placer_(prev.macro_placer_),
      logger_(prev.logger_)
{
}

Partition::~Partition()
{
}

#define EAST_IDX (macros_.size() + coreEdgeIndex(CoreEdge::East))
#define WEST_IDX (macros_.size() + coreEdgeIndex(CoreEdge::West))
#define NORTH_IDX (macros_.size() + coreEdgeIndex(CoreEdge::North))
#define SOUTH_IDX (macros_.size() + coreEdgeIndex(CoreEdge::South))

#define GLOBAL_EAST_IDX (placer->macros_.size() + coreEdgeIndex(CoreEdge::East))
#define GLOBAL_WEST_IDX (placer->macros_.size() + coreEdgeIndex(CoreEdge::West))
#define GLOBAL_NORTH_IDX (placer->macros_.size() + coreEdgeIndex(CoreEdge::North))
#define GLOBAL_SOUTH_IDX (placer->macros_.size() + coreEdgeIndex(CoreEdge::South))

string Partition::getName(int macroIdx)
{
  if (macroIdx < macros_.size()) {
    return macros_[macroIdx].name();
  } else {
    return coreEdgeString(coreEdgeFromIndex(macroIdx - macros_.size()));
  }
}

void Partition::FillNetlistTable(MacroPlacer *placer,
                                 MacroPartMap &macroPartMap)
{
  int macro_edge_count = macros_.size() + core_edge_count;
  net_tbl_.resize(macro_edge_count * macro_edge_count);
  for (size_t i = 0; i < net_tbl_.size(); i++) {
    net_tbl_[i] = 0.0;
  }

  // Just Copy to the netlistTable.
  if (partClass == ALL) {
    for (size_t i = 0; i < macro_edge_count; i++) {
      for (size_t j = 0; j < macro_edge_count; j++) {
        net_tbl_[i * macro_edge_count + j] = placer->macroWeight[i][j];
      }
    }
  }
  else {
    // row
    for (size_t i = 0; i < macro_edge_count; i++) {
      // column
      for (size_t j = 0; j < macro_edge_count; j++) {
        if (i == j) {
          continue;
        }

        // from: macro case
        if (i < macros_.size()) {
          int globalIdx1 = placer->macroInstMap[macros_[i].dbInstPtr];
          // to macro case
          if (j < macros_.size()) {
            int globalIdx2 = placer->macroInstMap[macros_[j].dbInstPtr];
            net_tbl_[i * (macros_.size() + core_edge_count) + j]
              = placer->macroWeight[globalIdx1][globalIdx2];
          }
          // to IO-west case
          else if (j == WEST_IDX) {
            int westSum = placer->macroWeight[globalIdx1][GLOBAL_WEST_IDX];

            if (partClass == PartClass::NE) {
              for (auto& curMacroIdx : macroPartMap[PartClass::NW]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                westSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            if (partClass == PartClass::SE) {
              for (auto& curMacroIdx : macroPartMap[PartClass::SW]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                westSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            net_tbl_[i * (macros_.size() + core_edge_count) + j] = westSum;
          } else if (j == EAST_IDX) {
            int eastSum = placer->macroWeight[globalIdx1][GLOBAL_EAST_IDX];

            if (partClass == PartClass::NW) {
              for (auto& curMacroIdx : macroPartMap[PartClass::NE]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                eastSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            if (partClass == PartClass::SW) {
              for (auto& curMacroIdx : macroPartMap[PartClass::SE]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                eastSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            net_tbl_[i * (macros_.size() + core_edge_count) + j] = eastSum;
          } else if (j == NORTH_IDX) {
            int northSum = placer->macroWeight[globalIdx1][GLOBAL_NORTH_IDX];

            if (partClass == PartClass::SE) {
              for (auto& curMacroIdx : macroPartMap[PartClass::SE]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                northSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            if (partClass == PartClass::SW) {
              for (auto& curMacroIdx : macroPartMap[PartClass::NW]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                northSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            net_tbl_[i * (macros_.size() + core_edge_count) + j] = northSum;
          } else if (j == SOUTH_IDX) {
            int southSum = placer->macroWeight[globalIdx1][GLOBAL_SOUTH_IDX];

            if (partClass == PartClass::NE) {
              for (auto& curMacroIdx : macroPartMap[PartClass::SE]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                southSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            if (partClass == PartClass::NW) {
              for (auto& curMacroIdx : macroPartMap[PartClass::SW]) {
                int curGlobalIdx
                  = placer->macroInstMap[macros_[curMacroIdx].dbInstPtr];
                southSum += placer->macroWeight[globalIdx1][curGlobalIdx];
              }
            }
            net_tbl_[i * (macros_.size() + core_edge_count) + j] = southSum;
          }
        }
        // from IO
        else if (i == WEST_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int globalIdx2 = placer->macroInstMap[macros_[j].dbInstPtr];
            net_tbl_[i * (macros_.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_WEST_IDX][globalIdx2];
          }
        } else if (i == EAST_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int globalIdx2 = placer->macroInstMap[macros_[j].dbInstPtr];
            net_tbl_[i * (macros_.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_EAST_IDX][globalIdx2];
          }
        } else if (i == NORTH_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int globalIdx2 = placer->macroInstMap[macros_[j].dbInstPtr];
            net_tbl_[i * (macros_.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_NORTH_IDX][globalIdx2];
          }
        } else if (i == SOUTH_IDX) {
          // to Macro
          if (j < macros_.size()) {
            int globalIdx2 = placer->macroInstMap[macros_[j].dbInstPtr];
            net_tbl_[i * (macros_.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_SOUTH_IDX][globalIdx2];
          }
        }
      }
    }
  }
}

void Partition::UpdateMacroCoordi(MacroPlacer* placer)
{
  for (auto& curPartMacro : macros_) {
    int macroIdx = placer->macroInstMap[curPartMacro.dbInstPtr];
    curPartMacro.lx = placer->macros_[macroIdx].lx;
    curPartMacro.ly = placer->macros_[macroIdx].ly;
  }
}

// Call ParquetFP
bool Partition::DoAnneal()
{
  // No macro, no need to execute
  if (macros_.size() == 0) {
    return true;
  }

  logger_->report("Begin Parquet");

  // Preprocessing in macroPlacer side
  // For nets and wts
  vector<pair<int, int>> netStor;
  vector<int> costStor;

  int macro_edge_count = macros_.size() + core_edge_count;
  netStor.reserve(macro_edge_count * (macro_edge_count - 1) / 2);
  costStor.reserve(macro_edge_count * (macro_edge_count - 1) / 2);

  for (size_t i = 0; i < macro_edge_count; i++) {
    for (size_t j = i + 1; j < macro_edge_count; j++) {
      int cost = 0;
      if (!net_tbl_.empty()) {
        cost = net_tbl_[i * macro_edge_count + j]
               + net_tbl_[j * macro_edge_count + i];
      }
      if (cost != 0) {
        netStor.push_back(make_pair(min(i, j), max(i, j)));
        costStor.push_back(cost);
      }
    }
  }

  if (netStor.size() == 0) {
    for (size_t i = 0; i < core_edge_count; i++) {
      for (size_t j = i + 1; j < core_edge_count; j++) {
        netStor.push_back(make_pair(i, j));
        costStor.push_back(1);
      }
    }
  }

  // Populating DB structure
  // Instantiate Parquet DB structure
  DB db;
  pfp::Nodes* nodes = db.getNodes();
  pfp::Nets* nets = db.getNets();

  //////////////////////////////////////////////////////
  // Feed node structure: macro Info
  for (auto& curMacro : macros_) {
    MacroSpacings &spacings = macro_placer_->getSpacings(curMacro);
    double padded_width
      = curMacro.w + 2 * (spacings.getHaloX() + spacings.getChannelX());
    double padded_height
        = curMacro.h + 2 * (spacings.getHaloY() + spacings.getChannelY());
    double aspect_ratio = padded_width / padded_height;
    pfp::Node node(curMacro.name(),
                   padded_width * padded_height,
                   aspect_ratio,
                   aspect_ratio,
                   &curMacro - &macros_[0],
                   false);
    
    node.addSubBlockIndex(&curMacro - &macros_[0]);

    // TODO
    // tmpMacro.putSnapX();
    // tmpMacro.putHaloX();
    // tmpMacro.putChannelX();

    nodes->putNewNode(node);
  }

  // Feed node structure: terminal Info
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
    nodes->putNewTerm(pin);
  }

  //////////////////////////////////////////////////////
  // Feed net / weight structure
  for (auto& curNet : netStor) {
    int idx = &curNet - &netStor[0];
    pfp::Net pnet;

    parquetfp::pin pin1(getName(curNet.first).c_str(), true, 0, 0, idx);
    parquetfp::pin pin2(getName(curNet.second).c_str(), true, 0, 0, idx);

    pnet.addNode(pin1);
    pnet.addNode(pin2);
    pnet.putIndex(idx);
    pnet.putName(string("n" + to_string(idx)).c_str());
    pnet.putWeight(costStor[idx]);

    nets->putNewNet(pnet);
  }

  nets->updateNodeInfo(*nodes);
  nodes->updatePinsInfo(*nets);

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
  param.nonTrivialOutline = parquetfp::BBox(0, 0, width, height);
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

  //
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

  // update back into macroPlacer
  for (size_t i = 0; i < nodes->getNumNodes(); i++) {
    pfp::Node& curNode = nodes->getNode(i);

    macros_[i].lx = (isFlipX)
                          ? width - curNode.getX() - curNode.getWidth() + lx
                          : curNode.getX() + lx;
    macros_[i].ly = (isFlipY)
                          ? height - curNode.getY() - curNode.getHeight() + ly
                          : curNode.getY() + ly;

    Macro &macro = macros_[i];
    MacroSpacings &spacings = macro_placer_->getSpacings(macro);
    macro.lx += spacings.getHaloX() + spacings.getChannelX();
    macro.ly += spacings.getHaloY() + spacings.getChannelY();
  }

  logger_->report("End Parquet");
  return true;
}

}  // namespace mpl
