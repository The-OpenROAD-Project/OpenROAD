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

#include <cfloat>
#include <climits>
#include <fstream>
#include <iostream>

#include "Parquet.h"
#include "btreeanneal.h"
#include "circuit.h"
#include "mixedpackingfromdb.h"
#include "utility/Logger.h"

namespace mpl {

using std::endl;
using std::pair;
using std::string;
using std::unordered_map;
using std::vector;

using utl::MPL;

Partition::Partition(utl::Logger* log)
    : partClass(PartClass::None),
      lx(FLT_MAX),
      ly(FLT_MAX),
      width(FLT_MAX),
      height(FLT_MAX),
      netTable(0),
      tableCnt(0),
      log_(log)
{
}

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
      netTable(0),
      tableCnt(0),
      log_(log)
{
}

Partition::~Partition()
{
  if (netTable) {
    delete[] netTable;
    netTable = 0;
    tableCnt = 0;
  }
  log_ = nullptr;
}

Partition::Partition(const Partition& prev)
    : partClass(prev.partClass),
      lx(prev.lx),
      ly(prev.ly),
      width(prev.width),
      height(prev.height),
      macroStor(prev.macroStor),
      tableCnt(prev.tableCnt),
      macroMap(prev.macroMap),
      log_(prev.log_)
{
  if (prev.netTable) {
    netTable = new double[tableCnt];
    for (int i = 0; i < tableCnt; i++) {
      netTable[i] = prev.netTable[i];
    }
  } else {
    netTable = 0;
  }
}

Partition& Partition::operator=(const Partition& prev)
{
  partClass = prev.partClass;
  lx = prev.lx;
  ly = prev.ly;
  width = prev.width;
  height = prev.height;
  macroStor = prev.macroStor;
  tableCnt = prev.tableCnt;
  macroMap = prev.macroMap;
  log_ = prev.log_;

  if (prev.netTable) {
    netTable = new double[tableCnt];
    for (int i = 0; i < tableCnt; i++) {
      netTable[i] = prev.netTable[i];
    }
  } else {
    netTable = 0;
  }
  return *this;
}

void Partition::Dump()
{
  log_->report("partClass: {}", partClass);
  log_->report("{} {} {} {}", lx, ly, width, height);
  for (auto& curMacro : macroStor) {
    curMacro.Dump();
  }
}

void Partition::PrintParquetFormat(string origName)
{
  string blkName = origName + ".blocks";
  string netName = origName + ".nets";
  string wtsName = origName + ".wts";
  string plName = origName + ".pl";

  // For *.nets and *.wts writing
  vector<pair<int, int>> netStor;
  vector<int> costStor;

  netStor.reserve((macroStor.size() + 4) * (macroStor.size() + 3) / 2);
  costStor.reserve((macroStor.size() + 4) * (macroStor.size() + 3) / 2);
  for (size_t i = 0; i < macroStor.size() + 4; i++) {
    for (size_t j = i + 1; j < macroStor.size() + 4; j++) {
      int cost = 0;
      if (netTable) {
        cost = netTable[i * (macroStor.size() + 4) + j]
               + netTable[j * (macroStor.size() + 4) + i];
      }
      if (cost != 0) {
        netStor.push_back(std::make_pair(std::min(i, j), std::max(i, j)));
        costStor.push_back(cost);
      }
    }
  }

  WriteBlkFile(blkName);
  WriteNetFile(netStor, netName);
  WriteWtsFile(costStor, wtsName);
  WritePlFile(plName);
}

void Partition::WriteBlkFile(string blkName)
{
  std::ofstream blkFile(blkName);
  if (!blkFile.good()) {
    log_->error(MPL, 50, "Cannot Open BlkFile to write : {}", blkName);
  }

  std::stringstream feed;
  feed << "UCSC blocks 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;

  feed << "NumSoftRectangularBlocks : 0" << endl;
  feed << "NumHardRectilinearBlocks : " << macroStor.size() << endl;
  feed << "NumTerminals : 4" << endl << endl;

  for (auto& curMacro : macroStor) {
    feed << curMacro.name() << " hardrectilinear 4 ";
    feed << "(" << curMacro.lx << ", " << curMacro.ly << ") ";
    feed << "(" << curMacro.lx << ", " << curMacro.ly + curMacro.h << ") ";
    feed << "(" << curMacro.lx + curMacro.w << ", " << curMacro.ly + curMacro.h
         << ") ";
    feed << "(" << curMacro.lx + curMacro.w << ", " << curMacro.ly << ") "
         << endl;
  }

  feed << endl;

  feed << "West terminal" << endl;
  feed << "East terminal" << endl;
  feed << "North terminal" << endl;
  feed << "South terminal" << endl;

  blkFile << feed.str();
  blkFile.close();
  feed.clear();
}

#define EAST_IDX (macroStor.size())
#define WEST_IDX (macroStor.size() + 1)
#define NORTH_IDX (macroStor.size() + 2)
#define SOUTH_IDX (macroStor.size() + 3)

#define GLOBAL_EAST_IDX (mckt.macroStor.size())
#define GLOBAL_WEST_IDX (mckt.macroStor.size() + 1)
#define GLOBAL_NORTH_IDX (mckt.macroStor.size() + 2)
#define GLOBAL_SOUTH_IDX (mckt.macroStor.size() + 3)

string Partition::GetName(int macroIdx)
{
  if (macroIdx < macroStor.size()) {
    return macroStor[macroIdx].name();
  } else {
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
  }
}

void Partition::WriteNetFile(vector<pair<int, int>>& netStor, string netName)
{
  std::ofstream netFile(netName);
  if (!netFile.good()) {
    log_->error(MPL, 51, "Cannot Open NetFile to write : {}", netName);
  }

  std::stringstream feed;
  feed << "UCLA nets 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;

  feed << "NumNets : " << netStor.size() << endl;
  feed << "NumPins : " << 2 * netStor.size() << endl;

  for (auto& curNet : netStor) {
    int idx = &curNet - &netStor[0];
    feed << "NetDegree : 2  n" << std::to_string(idx) << endl;
    feed << GetName(curNet.first) << " B : %0.0 %0.0" << endl;
    feed << GetName(curNet.second) << " B : %0.0 %0.0" << endl;
  }

  feed << endl;

  netFile << feed.str();
  netFile.close();
  feed.clear();
}

void Partition::WriteWtsFile(vector<int>& costStor, string wtsName)
{
  std::ofstream wtsFile(wtsName);
  if (!wtsFile.good()) {
    log_->error(MPL, 52, "Cannot Open WtsFile to write : {}", wtsName);
  }

  std::stringstream feed;
  feed << "UCLA wts 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;

  for (auto& curWts : costStor) {
    int idx = &curWts - &costStor[0];
    feed << "n" << std::to_string(idx) << " " << curWts << endl;
  }

  feed << endl;

  wtsFile << feed.str();
  wtsFile.close();
  feed.clear();
}

void Partition::WritePlFile(string plName)
{
  std::ofstream plFile(plName);
  if (!plFile.good()) {
    log_->error(MPL, 53, "Cannot Open PlFIle to write : {}", plName);
  }

  std::stringstream feed;
  feed << "UCLA pl 1.0" << endl;
  feed << "# Created" << endl;
  feed << "# User" << endl;
  feed << "# Platform" << endl << endl;

  for (auto& curMacro : macroStor) {
    feed << curMacro.name() << " 0 0" << endl;
  }

  feed << "East " << lx + width << " " << ly + height / 2.0 << endl;
  feed << "West " << lx << " " << ly + height / 2.0 << endl;
  feed << "North " << lx + width / 2.0 << " " << ly + height << endl;
  feed << "South " << lx + width / 2.0 << " " << ly << endl;

  feed << endl;

  plFile << feed.str();
  plFile.close();
  feed.clear();
}

void Partition::FillNetlistTable(
    MacroCircuit& mckt,
    unordered_map<PartClass, vector<int>, PartClassHash, PartClassEqual>&
        macroPartMap)
{
  tableCnt = (macroStor.size() + 4) * (macroStor.size() + 4);
  netTable = new double[tableCnt];
  for (int i = 0; i < tableCnt; i++) {
    netTable[i] = 0.0f;
  }
  // FillNetlistTableIncr();
  // FillNetlistTableDesc();

  auto mpPtr = macroPartMap.find(partClass);
  if (mpPtr == macroPartMap.end()) {
    log_->error(MPL,
                54,
                "Partition: {} not exists MacroCell (macroPartMap)",
                partClass);
  }

  // Just Copy to the netlistTable.
  if (partClass == ALL) {
    for (size_t i = 0; i < (macroStor.size() + 4); i++) {
      for (size_t j = 0; j < macroStor.size() + 4; j++) {
        netTable[i * (macroStor.size() + 4) + j]
            = (double) mckt.macroWeight[i][j];
      }
    }
    return;
  }

  // row
  for (size_t i = 0; i < macroStor.size() + 4; i++) {
    // column
    for (size_t j = 0; j < macroStor.size() + 4; j++) {
      if (i == j) {
        continue;
      }

      // from: macro case
      if (i < macroStor.size()) {
        auto mPtr = mckt.macroNameMap.find(macroStor[i].name());
        if (mPtr == mckt.macroNameMap.end()) {
          log_->error(MPL,
                      55,
                      "Cannot find macros {} in macroNameMap",
                      macroStor[i].name());
        }
        int globalIdx1 = mPtr->second;

        // to macro case
        if (j < macroStor.size()) {
          auto mPtr = mckt.macroNameMap.find(macroStor[j].name());
          if (mPtr == mckt.macroNameMap.end()) {
            log_->error(MPL,
                        56,
                        "Cannot find macros {} in macroNameMap",
                        macroStor[j].name());
          }
          int globalIdx2 = mPtr->second;
          netTable[i * (macroStor.size() + 4) + j]
              = mckt.macroWeight[globalIdx1][globalIdx2];
        }
        // to IO-west case
        else if (j == WEST_IDX) {
          int westSum = mckt.macroWeight[globalIdx1][GLOBAL_WEST_IDX];

          if (partClass == PartClass::NE) {
            auto mpPtr = macroPartMap.find(PartClass::NW);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                westSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          if (partClass == PartClass::SE) {
            auto mpPtr = macroPartMap.find(PartClass::SW);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                westSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          netTable[i * (macroStor.size() + 4) + j] = westSum;
        } else if (j == EAST_IDX) {
          int eastSum = mckt.macroWeight[globalIdx1][GLOBAL_EAST_IDX];

          if (partClass == PartClass::NW) {
            auto mpPtr = macroPartMap.find(PartClass::NE);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                eastSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          if (partClass == PartClass::SW) {
            auto mpPtr = macroPartMap.find(PartClass::SE);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                eastSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          netTable[i * (macroStor.size() + 4) + j] = eastSum;
        } else if (j == NORTH_IDX) {
          int northSum = mckt.macroWeight[globalIdx1][GLOBAL_NORTH_IDX];

          if (partClass == PartClass::SE) {
            auto mpPtr = macroPartMap.find(PartClass::NE);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                northSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          if (partClass == PartClass::SW) {
            auto mpPtr = macroPartMap.find(PartClass::NW);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                northSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          netTable[i * (macroStor.size() + 4) + j] = northSum;
        } else if (j == SOUTH_IDX) {
          int southSum = mckt.macroWeight[globalIdx1][GLOBAL_SOUTH_IDX];

          if (partClass == PartClass::NE) {
            auto mpPtr = macroPartMap.find(PartClass::SE);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                southSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          if (partClass == PartClass::NW) {
            auto mpPtr = macroPartMap.find(PartClass::SW);
            if (mpPtr != macroPartMap.end()) {
              for (auto& curMacroIdx : mpPtr->second) {
                int curGlobalIdx
                    = mckt.macroNameMap[mckt.macroStor[curMacroIdx].name()];
                southSum += mckt.macroWeight[globalIdx1][curGlobalIdx];
              }
            }
          }
          netTable[i * (macroStor.size() + 4) + j] = southSum;
        }
      }
      // from IO
      else if (i == WEST_IDX) {
        // to Macro
        if (j < macroStor.size()) {
          auto mPtr = mckt.macroNameMap.find(macroStor[j].name());
          if (mPtr == mckt.macroNameMap.end()) {
            log_->error(MPL,
                        57,
                        "Cannot find macros {} in macroNameMap",
                        macroStor[j].name());
          }
          int globalIdx2 = mPtr->second;
          netTable[i * (macroStor.size() + 4) + j]
              = mckt.macroWeight[GLOBAL_WEST_IDX][globalIdx2];
        }
      } else if (i == EAST_IDX) {
        // to Macro
        if (j < macroStor.size()) {
          auto mPtr = mckt.macroNameMap.find(macroStor[j].name());
          if (mPtr == mckt.macroNameMap.end()) {
            log_->error(MPL,
                        58,
                        "Cannot find macros {} in macroNameMap",
                        macroStor[j].name());
          }
          int globalIdx2 = mPtr->second;
          netTable[i * (macroStor.size() + 4) + j]
              = mckt.macroWeight[GLOBAL_EAST_IDX][globalIdx2];
        }
      } else if (i == NORTH_IDX) {
        // to Macro
        if (j < macroStor.size()) {
          auto mPtr = mckt.macroNameMap.find(macroStor[j].name());
          if (mPtr == mckt.macroNameMap.end()) {
            log_->error(MPL,
                        59,
                        "Cannot find macros {} in macroNameMap",
                        macroStor[j].name());
          }
          int globalIdx2 = mPtr->second;
          netTable[i * (macroStor.size() + 4) + j]
              = mckt.macroWeight[GLOBAL_NORTH_IDX][globalIdx2];
        }
      } else if (i == SOUTH_IDX) {
        // to Macro
        if (j < macroStor.size()) {
          auto mPtr = mckt.macroNameMap.find(macroStor[j].name());
          if (mPtr == mckt.macroNameMap.end()) {
            log_->error(MPL,
                        60,
                        "Cannot find macros {} in macroNameMap",
                        macroStor[j].name());
          }
          int globalIdx2 = mPtr->second;
          netTable[i * (macroStor.size() + 4) + j]
              = mckt.macroWeight[GLOBAL_SOUTH_IDX][globalIdx2];
        }
      }
    }
  }
}

void Partition::FillNetlistTableIncr()
{
  //      if( macroStor.size() <= 1 ) {
  //        return;
  //      }

  for (size_t i = 0; i < macroStor.size() + 4; i++) {
    for (size_t j = 0; j < macroStor.size() + 4; j++) {
      double val = (i + j + 1) * 100;
      if (i == j || (i >= macroStor.size() && j >= macroStor.size())) {
        val = 0;
      }
      netTable[i * (macroStor.size() + 4) + j] = val;
    }
  }
}

void Partition::FillNetlistTableDesc()
{
  //      if( macroStor.size() <= 1 ) {
  //        return;
  //      }

  for (size_t i = 0; i < macroStor.size() + 4; i++) {
    for (size_t j = 0; j < macroStor.size() + 4; j++) {
      double val = (2 * macroStor.size() + 8 - (i + j)) * 100;
      if (i == j || (i >= macroStor.size() && j >= macroStor.size())) {
        val = 0;
      }
      netTable[i * (macroStor.size() + 4) + j] = val;
    }
  }
}

void Partition::UpdateMacroCoordi(MacroCircuit& mckt)
{
  for (auto& curPartMacro : macroStor) {
    auto mPtr = mckt.macroNameMap.find(curPartMacro.name());
    int macroIdx = mPtr->second;
    curPartMacro.lx = mckt.macroStor[macroIdx].lx;
    curPartMacro.ly = mckt.macroStor[macroIdx].ly;
  }
}

// Call ParquetFP
bool Partition::DoAnneal()
{
  // No macro, no need to execute
  if (macroStor.size() == 0) {
    return true;
  }

  log_->report("Begin Parquet");

  // Preprocessing in macroPlacer side
  // For nets and wts
  vector<pair<int, int>> netStor;
  vector<int> costStor;

  netStor.reserve((macroStor.size() + 4) * (macroStor.size() + 3) / 2);
  costStor.reserve((macroStor.size() + 4) * (macroStor.size() + 3) / 2);

  for (size_t i = 0; i < macroStor.size() + 4; i++) {
    for (size_t j = i + 1; j < macroStor.size() + 4; j++) {
      int cost = 0;
      if (netTable) {
        cost = netTable[i * (macroStor.size() + 4) + j]
               + netTable[j * (macroStor.size() + 4) + i];
      }
      if (cost != 0) {
        netStor.push_back(std::make_pair(std::min(i, j), std::max(i, j)));
        costStor.push_back(cost);
      }
    }
  }

  if (netStor.size() == 0) {
    for (size_t i = 0; i < 4; i++) {
      for (size_t j = i + 1; j < 4; j++) {
        netStor.push_back(std::make_pair(i, j));
        costStor.push_back(1);
      }
    }
  }

  using namespace parquetfp;

  // Populating DB structure
  // Instantiate Parquet DB structure
  DB db;
  Nodes* nodes = db.getNodes();
  Nets* nets = db.getNets();

  //////////////////////////////////////////////////////
  // Feed node structure: macro Info
  for (auto& curMacro : macroStor) {
    double padMacroWidth
        = curMacro.w + 2 * (curMacro.haloX + curMacro.channelX);
    double padMacroHeight
        = curMacro.h + 2 * (curMacro.haloY + curMacro.channelY);

    Node tmpMacro(std::string(curMacro.name().c_str()),
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
  std::string pinNames[4] = {"West", "East", "North", "South"};
  double posX[4] = {0.0, width, width / 2.0, width / 2.0};
  double posY[4] = {height / 2.0, height / 2.0, height, 0.0f};
  for (int i = 0; i < 4; i++) {
    Node tmpPin(pinNames[i], 0, 1, 1, indexTerm++, true);
    tmpPin.putX(posX[i]);
    tmpPin.putY(posY[i]);
    nodes->putNewTerm(tmpPin);
  }

  //////////////////////////////////////////////////////
  // Feed net / weight structure
  for (auto& curNet : netStor) {
    int idx = &curNet - &netStor[0];
    Net tmpEdge;

    parquetfp::pin tempPin1(GetName(curNet.first).c_str(), true, 0, 0, idx);
    parquetfp::pin tempPin2(GetName(curNet.second).c_str(), true, 0, 0, idx);

    tmpEdge.addNode(tempPin1);
    tmpEdge.addNode(tempPin2);
    tmpEdge.putIndex(idx);
    tmpEdge.putName(std::string("n" + std::to_string(idx)).c_str());
    tmpEdge.putWeight(costStor[idx]);

    nets->putNewNet(tmpEdge);
  }

  nets->updateNodeInfo(*nodes);
  nodes->updatePinsInfo(*nets);

  // Populate MixedBlockInfoType object
  // It is from DB object
  MixedBlockInfoTypeFromDB dbBlockInfo(db);
  MixedBlockInfoType* blockInfo
      = reinterpret_cast<MixedBlockInfoType*>(&dbBlockInfo);

  // Command_Line object populate
  Command_Line param;
  param.minWL = true;
  param.noRotation = true;
  param.FPrep = "BTree";
  param.seed = 100;
  param.scaleTerms = false;

  // Fixed-outline mode in Parquet
  param.nonTrivialOutline = parquetfp::BBox(0, 0, width, height);
  param.reqdAR = width / height;
  param.maxWS = 0;
  param.verb = "0 0 0";

  // Instantiate BTreeAnnealer Object
  BTreeAreaWireAnnealer* annealer = new BTreeAreaWireAnnealer(
      *blockInfo, const_cast<Command_Line*>(&param), &db);

  annealer->go();

  const BTree& sol = annealer->currSolution();
  // Failed annealing
  if (sol.totalWidth() > width || sol.totalHeight() > height) {
    log_->info(MPL, 61, "Parquet BBOX exceed the given area");
    log_->info(MPL,
               62,
               "ParquetSolLayout {:g} {:g}",
               sol.totalWidth(),
               sol.totalHeight());
    log_->info(MPL, 63, "TargetLayout {:g} {:g}", width, height);
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
    Node& curNode = nodes->getNode(i);

    macroStor[i].lx = (isFlipX)
                          ? width - curNode.getX() - curNode.getWidth() + lx
                          : curNode.getX() + lx;
    macroStor[i].ly = (isFlipY)
                          ? height - curNode.getY() - curNode.getHeight() + ly
                          : curNode.getY() + ly;

    macroStor[i].lx += (macroStor[i].haloX + macroStor[i].channelX);
    macroStor[i].ly += (macroStor[i].haloY + macroStor[i].channelY);
  }

  //  db.plot( "out.plt", 0, 0, 0, 0, 0,
  //      0, 1, 1,  // slack, net, name
  //      true,
  //      0, 0, width, height);

  log_->report("End Parquet");
  return true;
}

void Partition::PrintSetFormat(FILE* fp)
{
  string sliceStr = "";
  if (partClass == ALL) {
    sliceStr = "ALL";
  } else if (partClass == NW) {
    sliceStr = "NW";
  } else if (partClass == NE) {
    sliceStr = "NE";
  } else if (partClass == SW) {
    sliceStr = "SW";
  } else if (partClass == SE) {
    sliceStr = "SE";
  } else if (partClass == E) {
    sliceStr = "E";
  } else if (partClass == W) {
    sliceStr = "W";
  } else if (partClass == S) {
    sliceStr = "S";
  } else if (partClass == N) {
    sliceStr = "N";
  }

  fprintf(fp, "  BEGIN SLICE %s %ld ;\n", sliceStr.c_str(), macroStor.size());
  fprintf(fp, "    LX %f ;\n", lx);
  fprintf(fp, "    LY %f ;\n", ly);
  fprintf(fp, "    WIDTH %f ;\n", width);
  fprintf(fp, "    HEIGHT %f ;\n", height);
  for (auto& curMacro : macroStor) {
    fprintf(fp,
            "    MACRO %s %s %f %f %f %f ;\n",
            curMacro.name().c_str(),
            curMacro.type().c_str(),
            curMacro.lx,
            curMacro.ly,
            curMacro.w,
            curMacro.h);
  }

  if (netTable) {
    fprintf(fp, "    NETLISTTABLE \n    ");
    for (size_t i = 0; i < macroStor.size() + 4; i++) {
      for (size_t j = 0; j < macroStor.size() + 4; j++) {
        fprintf(fp, "%.3f ", netTable[(macroStor.size() + 4) * i + j]);
      }
      if (i == macroStor.size() + 3) {
        fprintf(fp, "; \n");
      } else {
        fprintf(fp, "\n    ");
      }
    }
  } else {
    fprintf(fp, "    NETLISTTABLE ;\n");
  }

  fprintf(fp, "  END SLICE ;\n");
  fflush(fp);
}

}  // namespace mpl
