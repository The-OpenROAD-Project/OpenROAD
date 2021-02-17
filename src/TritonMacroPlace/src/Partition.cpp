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

#include "Partition.h"

#include "btreeanneal.h"
#include "mpl/MacroPlacer.h"
#include "mixedpackingfromdb.h"
#include "utility/Logger.h"

namespace mpl {

using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using utl::MPL;

namespace pfp = parquetfp;

Partition::Partition(PartClass _partClass,
                     double _lx,
                     double _ly,
                     double _width,
                     double _height,
                     utl::Logger* log)
    : partClass(_partClass),
      lx(_lx),
      ly(_ly),
      width(_width),
      height(_height),
      netTable(nullptr),
      tableCnt(0),
      logger_(log)
{
}

Partition::Partition(const Partition& prev)
    : partClass(prev.partClass),
      lx(prev.lx),
      ly(prev.ly),
      width(prev.width),
      height(prev.height),
      macroStor(prev.macroStor),
      tableCnt(prev.tableCnt),
      logger_(prev.logger_)
{
  if (prev.netTable) {
    netTable = new double[tableCnt];
    for (int i = 0; i < tableCnt; i++) {
      netTable[i] = prev.netTable[i];
    }
  } else {
    netTable = nullptr;
  }
}

Partition::~Partition()
{
  delete[] netTable;
}

#define USE_MAGIC_INDICIES 1

#if USE_MAGIC_INDICIES
#define EAST_IDX (macroStor.size())
#define WEST_IDX (macroStor.size() + 1)
#define NORTH_IDX (macroStor.size() + 2)
#define SOUTH_IDX (macroStor.size() + 3)

#define GLOBAL_EAST_IDX (placer->macroStor.size())
#define GLOBAL_WEST_IDX (placer->macroStor.size() + 1)
#define GLOBAL_NORTH_IDX (placer->macroStor.size() + 2)
#define GLOBAL_SOUTH_IDX (placer->macroStor.size() + 3)

#else
// This "should" work but does not. -cherry
#define EAST_IDX (macroStor.size() + coreEdgeIndex(CoreEdge::East))
#define WEST_IDX (macroStor.size() + coreEdgeIndex(CoreEdge::West))
#define NORTH_IDX (macroStor.size() + coreEdgeIndex(CoreEdge::North))
#define SOUTH_IDX (macroStor.size() + coreEdgeIndex(CoreEdge::South))

#define GLOBAL_EAST_IDX (placer->macroStor.size() + coreEdgeIndex(CoreEdge::East))
#define GLOBAL_WEST_IDX (placer->macroStor.size() + coreEdgeIndex(CoreEdge::West))
#define GLOBAL_NORTH_IDX (placer->macroStor.size() + coreEdgeIndex(CoreEdge::North))
#define GLOBAL_SOUTH_IDX (placer->macroStor.size() + coreEdgeIndex(CoreEdge::South))
#endif

string Partition::GetName(int macroIdx)
{
  if (macroIdx < macroStor.size()) {
    return macroStor[macroIdx].name();
  } else {
#if USE_MAGIC_INDICIES
    if (macroIdx == EAST_IDX) {
      return "East";
    } else if (macroIdx == WEST_IDX) {
      return "West";
    } else if (macroIdx == NORTH_IDX) {
      return "North";
    } else if (macroIdx == SOUTH_IDX) {
      return "South";
    } else {
      return "None";
    }
#else
    return coreEdgeString(coreEdgeFromIndex(macroIdx - macroStor.size()));
#endif
  }
}

void Partition::FillNetlistTable(
    MacroPlacer *placer,
    unordered_map<PartClass, vector<int>, PartClassHash, PartClassEqual>&
        macroPartMap)
{
  tableCnt = (macroStor.size() + core_edge_count) * (macroStor.size() + core_edge_count);
  netTable = new double[tableCnt];
  for (int i = 0; i < tableCnt; i++) {
    netTable[i] = 0.0;
  }

  auto mpPtr = macroPartMap.find(partClass);
  if (mpPtr == macroPartMap.end()) {
    logger_->error(MPL,
                54,
                "Partition: {} missing from macroPartMap",
                partClass);
  }

  // Just Copy to the netlistTable.
  if (partClass == ALL) {
    for (size_t i = 0; i < (macroStor.size() + core_edge_count); i++) {
      for (size_t j = 0; j < macroStor.size() + core_edge_count; j++) {
        netTable[i * (macroStor.size() + core_edge_count) + j]
            = (double) placer->macroWeight[i][j];
      }
    }
  }
  else {
    // row
    for (size_t i = 0; i < macroStor.size() + core_edge_count; i++) {
      // column
      for (size_t j = 0; j < macroStor.size() + core_edge_count; j++) {
        if (i == j) {
          continue;
        }

        // from: macro case
        if (i < macroStor.size()) {
          int globalIdx1 = placer->macroInstMap[macroStor[i].dbInstPtr];
          // to macro case
          if (j < macroStor.size()) {
            int globalIdx2 = placer->macroInstMap[macroStor[j].dbInstPtr];
            netTable[i * (macroStor.size() + core_edge_count) + j]
              = placer->macroWeight[globalIdx1][globalIdx2];
          }
          // to IO-west case
          else if (j == WEST_IDX) {
            int westSum = placer->macroWeight[globalIdx1][GLOBAL_WEST_IDX];

            if (partClass == PartClass::NE) {
              auto mpPtr = macroPartMap.find(PartClass::NW);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  westSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            if (partClass == PartClass::SE) {
              auto mpPtr = macroPartMap.find(PartClass::SW);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  westSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            netTable[i * (macroStor.size() + core_edge_count) + j] = westSum;
          } else if (j == EAST_IDX) {
            int eastSum = placer->macroWeight[globalIdx1][GLOBAL_EAST_IDX];

            if (partClass == PartClass::NW) {
              auto mpPtr = macroPartMap.find(PartClass::NE);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  eastSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            if (partClass == PartClass::SW) {
              auto mpPtr = macroPartMap.find(PartClass::SE);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  eastSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            netTable[i * (macroStor.size() + core_edge_count) + j] = eastSum;
          } else if (j == NORTH_IDX) {
            int northSum = placer->macroWeight[globalIdx1][GLOBAL_NORTH_IDX];

            if (partClass == PartClass::SE) {
              auto mpPtr = macroPartMap.find(PartClass::NE);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  northSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            if (partClass == PartClass::SW) {
              auto mpPtr = macroPartMap.find(PartClass::NW);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  northSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            netTable[i * (macroStor.size() + core_edge_count) + j] = northSum;
          } else if (j == SOUTH_IDX) {
            int southSum = placer->macroWeight[globalIdx1][GLOBAL_SOUTH_IDX];

            if (partClass == PartClass::NE) {
              auto mpPtr = macroPartMap.find(PartClass::SE);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  southSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            if (partClass == PartClass::NW) {
              auto mpPtr = macroPartMap.find(PartClass::SW);
              if (mpPtr != macroPartMap.end()) {
                for (auto& curMacroIdx : mpPtr->second) {
                  int curGlobalIdx
                    = placer->macroInstMap[macroStor[curMacroIdx].dbInstPtr];
                  southSum += placer->macroWeight[globalIdx1][curGlobalIdx];
                }
              }
            }
            netTable[i * (macroStor.size() + core_edge_count) + j] = southSum;
          }
        }
        // from IO
        else if (i == WEST_IDX) {
          // to Macro
          if (j < macroStor.size()) {
            int globalIdx2 = placer->macroInstMap[macroStor[j].dbInstPtr];
            netTable[i * (macroStor.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_WEST_IDX][globalIdx2];
          }
        } else if (i == EAST_IDX) {
          // to Macro
          if (j < macroStor.size()) {
            int globalIdx2 = placer->macroInstMap[macroStor[j].dbInstPtr];
            netTable[i * (macroStor.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_EAST_IDX][globalIdx2];
          }
        } else if (i == NORTH_IDX) {
          // to Macro
          if (j < macroStor.size()) {
            int globalIdx2 = placer->macroInstMap[macroStor[j].dbInstPtr];
            netTable[i * (macroStor.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_NORTH_IDX][globalIdx2];
          }
        } else if (i == SOUTH_IDX) {
          // to Macro
          if (j < macroStor.size()) {
            int globalIdx2 = placer->macroInstMap[macroStor[j].dbInstPtr];
            netTable[i * (macroStor.size() + core_edge_count) + j]
              = placer->macroWeight[GLOBAL_SOUTH_IDX][globalIdx2];
          }
        }
      }
    }
  }
}

void Partition::UpdateMacroCoordi(MacroPlacer* placer)
{
  for (auto& curPartMacro : macroStor) {
    int macroIdx = placer->macroInstMap[curPartMacro.dbInstPtr];
    curPartMacro.lx = placer->macroStor[macroIdx].lx;
    curPartMacro.ly = placer->macroStor[macroIdx].ly;
  }
}

// Call ParquetFP
bool Partition::DoAnneal()
{
  // No macro, no need to execute
  if (macroStor.size() == 0) {
    return true;
  }

  logger_->report("Begin Parquet");

  // Preprocessing in macroPlacer side
  // For nets and wts
  vector<pair<int, int>> netStor;
  vector<int> costStor;

  netStor.reserve((macroStor.size() + core_edge_count) * (macroStor.size() + 3) / 2);
  costStor.reserve((macroStor.size() + core_edge_count) * (macroStor.size() + 3) / 2);

  for (size_t i = 0; i < macroStor.size() + core_edge_count; i++) {
    for (size_t j = i + 1; j < macroStor.size() + core_edge_count; j++) {
      int cost = 0;
      if (netTable) {
        cost = netTable[i * (macroStor.size() + core_edge_count) + j]
               + netTable[j * (macroStor.size() + core_edge_count) + i];
      }
      if (cost != 0) {
        netStor.push_back(std::make_pair(std::min(i, j), std::max(i, j)));
        costStor.push_back(cost);
      }
    }
  }

  if (netStor.size() == 0) {
    for (size_t i = 0; i < core_edge_count; i++) {
      for (size_t j = i + 1; j < core_edge_count; j++) {
        netStor.push_back(std::make_pair(i, j));
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
  for (auto& curMacro : macroStor) {
    double padMacroWidth
        = curMacro.w + 2 * (curMacro.haloX + curMacro.channelX);
    double padMacroHeight
        = curMacro.h + 2 * (curMacro.haloY + curMacro.channelY);

    pfp::Node tmpMacro(curMacro.name(),
                       padMacroWidth * padMacroHeight,
                       padMacroWidth / padMacroHeight,
                       padMacroWidth / padMacroHeight,
                       &curMacro - &macroStor[0],
                       false);

    tmpMacro.addSubBlockIndex(&curMacro - &macroStor[0]);

    // TODO
    // tmpMacro.putSnapX();
    // tmpMacro.putHaloX();
    // tmpMacro.putChannelX();

    nodes->putNewNode(tmpMacro);
  }

  // Feed node structure: terminal Info
  int indexTerm = 0;
  double posX[core_edge_count] = {0.0, width, width / 2.0, width / 2.0};
  double posY[core_edge_count] = {height / 2.0, height / 2.0, height, 0.0f};
  for (int i = 0; i < core_edge_count; i++) {
    pfp::Node tmpPin(coreEdgeString(coreEdgeFromIndex(i)), 0, 1, 1, indexTerm++, true);
    tmpPin.putX(posX[i]);
    tmpPin.putY(posY[i]);
    nodes->putNewTerm(tmpPin);
  }

  //////////////////////////////////////////////////////
  // Feed net / weight structure
  for (auto& curNet : netStor) {
    int idx = &curNet - &netStor[0];
    pfp::Net pnet;

    parquetfp::pin pin1(GetName(curNet.first).c_str(), true, 0, 0, idx);
    parquetfp::pin pin2(GetName(curNet.second).c_str(), true, 0, 0, idx);

    pnet.addNode(pin1);
    pnet.addNode(pin2);
    pnet.putIndex(idx);
    pnet.putName(std::string("n" + std::to_string(idx)).c_str());
    pnet.putWeight(costStor[idx]);

    nets->putNewNet(pnet);
  }

  nets->updateNodeInfo(*nodes);
  nodes->updatePinsInfo(*nets);

  // Populate MixedBlockInfoType object
  // It is from DB object
  MixedBlockInfoTypeFromDB dbBlockInfo(db);
  MixedBlockInfoType* blockInfo
      = reinterpret_cast<MixedBlockInfoType*>(&dbBlockInfo);

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

    macroStor[i].lx = (isFlipX)
                          ? width - curNode.getX() - curNode.getWidth() + lx
                          : curNode.getX() + lx;
    macroStor[i].ly = (isFlipY)
                          ? height - curNode.getY() - curNode.getHeight() + ly
                          : curNode.getY() + ly;

    macroStor[i].lx += (macroStor[i].haloX + macroStor[i].channelX);
    macroStor[i].ly += (macroStor[i].haloY + macroStor[i].channelY);
  }

  logger_->report("End Parquet");
  return true;
}

}  // namespace mpl
