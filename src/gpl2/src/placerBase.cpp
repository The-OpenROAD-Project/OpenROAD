///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
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

#include "placerBase.h"
#include "placerObjects.h"
#include "db_sta/dbNetwork.hh"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include <odb/db.h>
#include "util.h"
#include "poissonSolver.h"


#include <stdio.h>
#include <memory>
#include <iostream>
#include <sstream>
#include <cmath>
#include <memory>
#include <numeric>
#include <unordered_set>
#include <chrono>
#include <cuda.h>
#include <cuda_runtime.h>
#include <cufft.h>

// basic vectors
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#include <thrust/device_malloc.h>
#include <thrust/device_free.h>
#include <thrust/sequence.h>
#include <thrust/reduce.h>
// memory related
#include <thrust/copy.h>
#include <thrust/fill.h>
// algorithm related
#include <thrust/transform.h>
#include <thrust/replace.h>
#include <thrust/functional.h>
#include <thrust/execution_policy.h>

namespace gpl2 {

using namespace std;
using utl::GPL2;

#define REPLACE_SQRT2 1.414213562373095048801L

///////////////////////////////////////////////////////////////////////////////////
// PlacerBaseVars
///////////////////////////////////////////////////////////////////////////////////

PlacerBaseVars::PlacerBaseVars()
{
  reset();
}

void PlacerBaseVars::reset()
{
  padLeft = padRight = 0;
  skipIoMode = false;
  row_limit = 6;
}

/////////////////////////////////////////////////////////////////////////////////////
// NesterovBaseVars
/////////////////////////////////////////////////////////////////////////////////////

NesterovBaseVars::NesterovBaseVars()
{
  reset();
}

void NesterovBaseVars::reset()
{
  targetDensity = 1.0;
  binCntX = binCntY = 0;
  minWireLengthForceBar = -300;
  isSetBinCnt = false;
  useUniformTargetDensity = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// NesterovPlaceVars
/////////////////////////////////////////////////////////////////////////////////////


NesterovPlaceVars::NesterovPlaceVars()
{
  reset();
}

// default variables for NesterovPlaceVars
void NesterovPlaceVars::reset()
{
  maxNesterovIter = 5000;
  maxBackTrack = 10;
  initDensityPenalty = 0.0008;
  initWireLengthCoef = 0.25;
  targetOverflow = 0.1;
  minPhiCoef = 0.95;
  maxPhiCoef = 1.03;
  minPreconditioner = 1.0;
  initialPrevCoordiUpdateCoef = 100;
  referenceHpwl = 446000000;
  routabilityCheckOverflow = 0.20;
  timingDrivenMode = true;
  routabilityDrivenMode = true;
}


/////////////////////////////////////////////////////////////////////////////////////
// PlacerBaseCommon
/////////////////////////////////////////////////////////////////////////////////////

PlacerBaseCommon::PlacerBaseCommon()
  : network_(nullptr),
    db_(nullptr),
    log_(nullptr),
    pbVars_(),
    wlGradOp_(nullptr),
    die_(),
    siteSizeX_(0),
    siteSizeY_(0),
    numInsts_(0),
    haloWidth_(0),
    virtualIter_(0),
    clusterFlag_(false),
    numPlaceInsts_(0),
    numFixedInsts_(0),
    numDummyInsts_(0),
    placeInstsArea_(0),
    nonPlaceInstsArea_(0),
    macroInstsArea_(0),
    stdCellInstsArea_(0),
    virtualWeightFactor_(0.0),
    dInstDCxPtr_(nullptr),
    dInstDCyPtr_(nullptr),
    dWLGradXPtr_(nullptr),
    dWLGradYPtr_(nullptr)
{
}


PlacerBaseCommon::PlacerBaseCommon(sta::dbNetwork* network,
                                   odb::dbDatabase* db,
                                   PlacerBaseVars pbVars,
                                   utl::Logger* log,
                                   float haloWidth,
                                   int virtualIter,
                                   int numHops,
                                   float bloatFactor,
                                   bool clusterFlag,
                                   bool dataflowFlag,
                                   bool datapathFlag,
                                   bool clusterConstraintFlag)
  : PlacerBaseCommon()
{
  network_ = network;
  db_ = db;
  log_ = log;
  pbVars_ = pbVars;
  block_ = db_->getChip()->getBlock();

  clusterFlag_ = clusterFlag;
  dataflowFlag_ = dataflowFlag;
  datapathFlag_ = datapathFlag;
  clusterConstraintFlag_ = clusterConstraintFlag;
  
  virtualIter_ = virtualIter;
  numHops_ = numHops;
  bloatFactor_ = bloatFactor;


  std::cout << "[INFO] clusterFlag = " << clusterFlag_ << std::endl;
  std::cout << "[INFO] dataflowFlag = " << dataflowFlag_ << std::endl;
  std::cout << "[INFO] datapathFlag = " << datapathFlag_ << std::endl;
  std::cout << "[INFO] clusterConstraintFlag = " << clusterConstraintFlag_ << std::endl;
  std::cout << "[INFO] virtualIter = " << virtualIter_ << std::endl;
  std::cout << "[INFO] numHops = " << numHops_ << std::endl;
  std::cout << "[INFO] haloWidth = " << haloWidth << std::endl;
  std::cout << "[INFO] bloatFactor = " << bloatFactor << std::endl;

  float dbuPerMicron = static_cast<float>(db_->getTech()->getDbUnitsPerMicron());
  haloWidth_ = haloWidth * dbuPerMicron;  
 
  createDataFlow();

  if (clusterFlag_ == true) {
    initClusterNetlist(); // for cluster placement
  } else {
    init(); // for stamdard cell placement
  }
}


PlacerBaseCommon::~PlacerBaseCommon()
{
  reset();
}


void PlacerBaseCommon::evaluateHPWL()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  odb::dbSet<odb::dbNet> nets = block->getNets();
  unsigned long int hpwl = 0;
  for (odb::dbNet* net : nets) {
    odb::dbSigType netType = net->getSigType();
    // escape nets with VDD/VSS/reset nets
    if (netType == odb::dbSigType::SIGNAL || netType == odb::dbSigType::CLOCK) { 
      int minX = std::numeric_limits<int>::max();
      int minY = std::numeric_limits<int>::max();
      int maxX = std::numeric_limits<int>::min();
      int maxY = std::numeric_limits<int>::min();
     
      for (odb::dbITerm* iTerm : net->getITerms()) {
        int offsetLx = std::numeric_limits<int>::max();
        int offsetLy = std::numeric_limits<int>::max();
        int offsetUx = std::numeric_limits<int>::min();
        int offsetUy = std::numeric_limits<int>::min();

        int offsetCx_ = 0;
        int offsetCy_ = 0;

        for (odb::dbMPin* mPin : iTerm->getMTerm()->getMPins()) {
          for (odb::dbBox* box : mPin->getGeometry()) {
            offsetLx = std::min(box->xMin(), offsetLx);
            offsetLy = std::min(box->yMin(), offsetLy);
            offsetUx = std::max(box->xMax(), offsetUx);
            offsetUy = std::max(box->yMax(), offsetUy);
          }
        }

        int lx = iTerm->getInst()->getBBox()->xMin();
        int ly = iTerm->getInst()->getBBox()->yMin();

        int instCenterX = iTerm->getInst()->getMaster()->getWidth() / 2;
        int instCenterY = iTerm->getInst()->getMaster()->getHeight() / 2;

        // Pin SHAPE is NOT FOUND;
        // (may happen on OpenDB bug case)
        if (offsetLx == INT_MAX || offsetLy == INT_MAX || offsetUx == INT_MIN
          || offsetUy == INT_MIN) {
          // offset is center of instances
          offsetCx_ = offsetCy_ = 0;
        } else {
          // offset is Pin BBoxs' center, so
          // subtract the Origin coordinates (e.g. instCenterX, instCenterY)
          //
          // Transform coordinates
          // from (origin: 0,0)
          // to (origin: instCenterX, instCenterY)
          //
          offsetCx_ = (offsetLx + offsetUx) / 2 - instCenterX;
          offsetCy_ = (offsetLy + offsetUy) / 2 - instCenterY;
        }

        int cx = lx + instCenterX + offsetCx_;
        int cy = ly + instCenterY + offsetCy_;

        minX = std::min(minX, cx);
        minY = std::min(minY, cy);
        maxX = std::max(maxX, cx);
        maxY = std::max(maxY, cy);
      }

      for (auto bTerm : net->getBTerms()) {
        int lx = std::numeric_limits<int>::max();
        int ly = std::numeric_limits<int>::max();
        int ux = std::numeric_limits<int>::min();
        int uy = std::numeric_limits<int>::min();

        for (odb::dbBPin* bPin : bTerm->getBPins()) {
          odb::Rect bbox = bPin->getBBox();
          lx = std::min(bbox.xMin(), lx);
          ly = std::min(bbox.yMin(), ly);
          ux = std::max(bbox.xMax(), ux);
          uy = std::max(bbox.yMax(), uy);
        }

        int cx = (lx + ux) / 2;
        int cy = (ly + uy) / 2;

        minX = std::min(minX, cx);
        minY = std::min(minY, cy);
        maxX = std::max(maxX, cx);
        maxY = std::max(maxY, cy);
      }
      hpwl += (maxX - minX) + (maxY - minY);
    }     
  }  
  std::cout << "[Total HPWL] HPWL = " << hpwl << std::endl;
}


// In this mode, we assume there is no fixed instances
// This is for mixed-size placement
void PlacerBaseCommon::initClusterNetlist()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  // get the site information
  odb::dbSite* site = nullptr;
  for (auto* row : block->getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      site = row->getSite();
      break;
    }
  }

  // siteSize update
  siteSizeX_ = site->getWidth();
  siteSizeY_ = site->getHeight();

  // get the die information
  odb::Rect coreRect = block->getCoreArea();
  odb::Rect dieRect = block->getDieArea();
  die_ = Die(dieRect, coreRect);
  
  // Reorder the cluster id
  int numClusters = 0;
  std::map<int, int> clusterIdReorderMap;

  // insts fill with real instances
  // update the clusters
  odb::dbSet<odb::dbInst> insts = block->getInsts();
  for (odb::dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    // check if the instance has a cluster id attribute    
    auto clusterIdProp = odb::dbIntProperty::find(inst, "cluster_id");
    if (clusterIdProp != nullptr) {
      const int clusterId = clusterIdProp->getValue();
      if (clusterIdReorderMap.find(clusterId) == clusterIdReorderMap.end()) {
        clusterIdReorderMap[clusterId] = numClusters++; 
      }
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(clusterIdReorderMap[clusterId]);
    }
  }

 
  std::cout << "[INFO] Number of clusters = " << numClusters << std::endl;
  std::vector<int64_t> clustersArea(numClusters, 0);  
  for (odb::dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    odb::dbBox* bbox = inst->getBBox();
    int haloWidth = 0;
    if (inst->getMaster()->isBlock()) {
      haloWidth = haloWidth_;
    }
    const int64_t instArea = static_cast<int64_t>(bbox->getDX() + haloWidth * 2) * 
                             static_cast<int64_t>(bbox->getDY() + haloWidth * 2);
    // check if the instance has a cluster id attribute    
    auto clusterIdProp = odb::dbIntProperty::find(inst, "cluster_id");
    if (clusterIdProp != nullptr) {
      const int clusterId = clusterIdProp->getValue();
      clustersArea[clusterId] += instArea; 
    }
  }


  // We need to bloat the area of clusters to speed the clusters
  int64_t coreArea = die_.coreArea();
  int64_t sumArea = std::accumulate(clustersArea.begin(),
                                    clustersArea.end(), static_cast<int64_t>(0));
  
  // In the default node (bloatFactor_ == 1.0), we bloat the clusters to fill the entire core area
  std::cout << "[INFO] bloatFactor = " << bloatFactor_ << std::endl;
  float bloatFactor = bloatFactor_;
  if (bloatFactor >= 0.99) {
    bloatFactor = static_cast<float>(coreArea) / static_cast<float>(sumArea);
    std::cout << "[INFO] Reset the bloatFactor to " << bloatFactor << std::endl;
  }

  for (auto& area : clustersArea) {
    area = area * bloatFactor;
  }
    
  // create fake instances
  numInsts_ = numClusters;
  numPlaceInsts_ = numClusters;
  numFixedInsts_ = numInsts_ - numPlaceInsts_;
  // allocate the objects on host side
  instStor_.resize(numInsts_);
  // different to original RePlAce codes,
  // we assign inst_id property to each inst
  // So we do not use inst_map_
  int placeInstId = -1;
  int fixedInstId = -1;
  const float cluster_ratio = 1.0; // We assume the aspect ratio of the cluster is 1.0
  for (int clusterId = 0; clusterId < numClusters; clusterId++) {    
    // create fake instances
    placeInstId++;
    const int64_t height = std::sqrt(clustersArea[clusterId] * cluster_ratio);
    const int64_t width = clustersArea[clusterId] / height;
    Instance myInst(0, 0, width, height, false); // just a small instance
    myInst.setMacro();
    myInst.setInstId(placeInstId);
    instStor_[placeInstId] = myInst;
    const int64_t instArea = myInst.area();
    if (myInst.isMacro()) {
      macroInstsArea_ += instArea;
    } else {
      stdCellInstsArea_ += instArea;
    }
    placeInstsArea_ += instArea;
  }

  // create pointers on for the host objects
  for (auto& inst : instStor_) {
    insts_.push_back(&inst);
    if (!inst.isFixed()) {
      placeInsts_.push_back(&inst);
    }
  }

  // store the connections between clusters
  std::map<int, std::map<int, int> > adjMatrix; 
  // nets fill
  odb::dbSet<odb::dbNet> nets = block->getNets();
  // TODO: I obersve that if the reserve size is not large enough, the program will crash
  netStor_.reserve(nets.size() * 10);   // Here we do not use resize
  pinStor_.reserve(nets.size() * 10); // average degree is around 3
  int netId = -1;
  int pinId = -1;
  for (odb::dbNet* net : nets) {
    odb::dbSigType netType = net->getSigType();
    odb::dbIntProperty::create(net, "netId", -1);    
    
    // escape nets with VDD/VSS/reset nets
    if (netType == odb::dbSigType::SIGNAL || netType == odb::dbSigType::CLOCK) {
      // check number of clusters connected to this net
      std::set<int> iTermClusters;
      // check all the instance pins connected to this net
      for (odb::dbITerm* iTerm : net->getITerms()) {
        // map the pin to its inst
        const int clusterId = odb::dbIntProperty::find(iTerm->getInst(), "cluster_id")->getValue();
        iTermClusters.insert(clusterId);        
      }

      if (iTermClusters.size() + net->getBTerms().size() <= 1) {
        continue;
      }

      // ignore high-fanout nets to avoid divergence
      if (iTermClusters.size() > 100) {
        continue;
      }

      // create fake pins for clusters
      netId++;
      odb::dbIntProperty::find(net, "netId")->setValue(netId);
      Net myNet(netId);
      myNet.setWeight(1.0);
      myNet.setVirtualWeight(0.0);
      netStor_.push_back(myNet);

      // create fake pins for clusters
      // check all the instance pins connected to this net
      for (auto& clusterId : iTermClusters) {
        pinId++;
        Pin myPin(pinId);
        myPin.setNet(&netStor_.back());
        myPin.setInstance(&instStor_[clusterId]);
        pinStor_.push_back(myPin);
      }
      
      for (odb::dbBTerm* bTerm : net->getBTerms()) {
        pinId++;
        odb::dbIntProperty::create(bTerm, "pinId", pinId);    
        Pin myPin(bTerm, log_);
        // link the pin with the net
        myPin.setNet(&netStor_.back());
        pinStor_.push_back(myPin);
      }
    }
  }

  std::map<int, odb::dbBTerm*> btermClusterMap;
  int btermClusterId = numClusters;
  for (odb::dbBTerm* bterm : block->getBTerms()) {
    btermClusterMap[btermClusterId++] = bterm;
  }


  for (int clusterId = 0; clusterId < adjMatrix_.size(); clusterId++) {
    for (auto& adj : adjMatrix_[clusterId]) {
      if (clusterId >= numClusters) {
        // check if pinId exists
        if (odb::dbIntProperty::find(btermClusterMap[clusterId], "pinId") == nullptr) {
          continue;
        }
      }

      if (adj.first >= numClusters) {
        // check if pinId exists
        if (odb::dbIntProperty::find(btermClusterMap[adj.first], "pinId") == nullptr) {
          continue;
        }
      }
      
      netId++;
      Net myNet(netId);
      myNet.setWeight(adj.second);
      myNet.setVirtualWeight(0.0);
      netStor_.push_back(myNet);

      if (clusterId >= numClusters) {
        pinId++;
        Pin myPin(btermClusterMap[clusterId], log_);
        // link the pin with the net
        myPin.setNet(&netStor_.back());
        pinStor_.push_back(myPin);
      } else {
        pinId++;
        Pin myPin(pinId);
        myPin.setNet(&netStor_.back());
        myPin.setInstance(&instStor_[clusterId]);
        pinStor_.push_back(myPin);
      }

      if (adj.first >= numClusters) {
        pinId++;
        Pin myPin(btermClusterMap[adj.first], log_);
        // link the pin with the net
        myPin.setNet(&netStor_.back());
        pinStor_.push_back(myPin);
      } else {
        pinId++;
        Pin myPin(pinId);
        myPin.setNet(&netStor_.back());
        myPin.setInstance(&instStor_[adj.first]);
        pinStor_.push_back(myPin);
      }
    }
  }

  for (auto& net : netStor_) {
    nets_.push_back(&net);
  }

  for (auto& pin : pinStor_) {
    pins_.push_back(&pin);
    if (pin.net() != nullptr) {
      pin.net()->addPin(&pin);
    }
    if (pin.isITerm()) {
      pin.instance()->addPin(&pin);
    }
  }


  // Initialize the virtual weight related variables
  initVirtualWeightFactor_ = 0.00;
  virtualWeightFactor_ = 0.00;

  // print the statistics
  printInfo(); 
}




void splitString(std::string& inputString) 
{
  if (inputString.back() != '_') {
    return;
  }  
  
  // Create a stringstream from the input string
  std::istringstream ss(inputString);
  // Create a vector to store the split parts
  std::vector<std::string> parts;
  std::string part;
  while (std::getline(ss, part, '_')) {
    // Add each part to the vector
    parts.push_back(part);
  }

  if (parts.size() == 1) {
    return;
  }

  inputString.erase(inputString.length() - parts.back().length() - 2);
}


size_t customHash(const std::string& input) {
  size_t hash = 0;
  for (char ch : input) {
      // Multiply the current hash value by a prime number 
      // and add the character's ASCII value
      hash = hash * 11 + static_cast<size_t>(ch);
  }
  return hash;
}


bool checkSDFF(std::string input) {
  if (input.at(0) == 'S') {
    return true;
  }      
  
  return false; 
}

void addVirtualConnection(
  std::map<int, float>& adjList,
  const int clusterId,
  const int seqVertexId,
  const int level,
  const int numHops,
  const int bitWidth,
  std::map<int, odb::dbBTerm*>& ioPinVertex,
  std::map<int, odb::dbInst*>&  instVertex,
  const std::vector<Vertex>& seqVertices)
{
  std::set<int> sinkClusters;
  std::set<int> sinkSeqVertexIds; // We need to further traverse the sequential graph
  for (auto& sink : seqVertices[seqVertexId].sinks) {
    if (sink.first >= ioPinVertex.size()) {
      // check if we should stop here
      if (odb::dbIntProperty::find(instVertex[sink.first], "cluster_id") == nullptr) {
        continue;
      }

      const int sinkClusterId = odb::dbIntProperty::find(instVertex[sink.first], "cluster_id")->getValue();    
      auto prop = odb::dbIntProperty::find(instVertex[sink.first], "dVertexId");
      if (sinkClusterId != clusterId && prop != nullptr) {
        sinkClusters.insert(sinkClusterId);
      } else {
        auto seqProp = odb::dbIntProperty::find(instVertex[sink.first], "seqVertexId");
        if (seqProp != nullptr) {
          const int sinkSeqVertexId = seqProp->getValue();
          sinkSeqVertexIds.insert(sinkSeqVertexId);
        } else {
          std::cout << "instName = " << instVertex[sink.first]->getName() << std::endl;
        }
      }
    } else {
      auto prop = odb::dbIntProperty::find(ioPinVertex[sink.first], "dVertexId");
      if (prop != nullptr) {
        const int sinkClusterId = odb::dbIntProperty::find(ioPinVertex[sink.first], "cluster_id")->getValue();   
        sinkClusters.insert(sinkClusterId);
      } 
    }
  }  

  for (auto& sinkClusterId : sinkClusters) {
    if (adjList.find(sinkClusterId) == adjList.end()) {
      adjList[sinkClusterId] = bitWidth / std::pow(2.0, level);
    } else {
      adjList[sinkClusterId] += bitWidth / std::pow(2.0, level);
    }
  }

  if (level < numHops) {
    for (auto& sink : sinkSeqVertexIds) {
      addVirtualConnection(adjList, clusterId, sink, level + 1, numHops, bitWidth, ioPinVertex, instVertex, seqVertices);
    }
  }
}


void addDataflowEdge(
  DVertex& dVertex,
  const int seqVertexId,
  const int maxDist,
  std::map<int, odb::dbBTerm*>& ioPinVertex,
  std::map<int, odb::dbInst*>&  instVertex,
  const std::vector<Vertex>& seqVertices) 
{
  for (auto& sink : seqVertices[seqVertexId].sinks) {
    const float weight = static_cast<float>(sink.second) / static_cast<float>(maxDist);  
    int sinkDVertexId = -1;
    if (sink.first >= ioPinVertex.size()) {
      auto prop = odb::dbIntProperty::find(instVertex[sink.first], "dVertexId");
      if (prop == nullptr) {
        std::cout << "instName = " << instVertex[sink.first]->getName() << std::endl;
        continue;
      } else {
        sinkDVertexId = prop->getValue();
      }
    } else {
      auto prop = odb::dbIntProperty::find(ioPinVertex[sink.first], "dVertexId");
      if (prop == nullptr) {
        std::cout << "ioName = " << ioPinVertex[sink.first]->getName() << std::endl;
        continue;
      } else {
        sinkDVertexId = prop->getValue();
      } 
    }
    dVertex.addSink(sinkDVertexId, weight);      
  }
}


// Create Dataflow Information
// model each std cell instance, IO pin and macro pin as vertices
void PlacerBaseCommon::createDataFlow()
{
  //if (datapathFlag_ == false && dataflowFlag_ == false) {
  //  return;
  //}
  
  std::map<int64_t, int> instMap;
  std::map<int64_t, int> multiFFMap;
  int numMacros = 0;

  // assign vertex_id property of each instance
  for (auto inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a Pad, Cover or a block
    // We ignore nets connecting Pads, Covers
    // for blocks, we iterate over the block pins
    if (master->isPad() || master->isCover()) {
      continue;
    }
    
    if (master->isBlock()) {
      numMacros++;
      continue;
    }
    
    const sta::LibertyCell* libertyCell = network_->libertyCell(inst);
    if (libertyCell == nullptr) {
      continue;
    }

    if (!libertyCell->hasSequentials()) {
      continue;  // ignore combinational cell
    }

    const std::string masterName = master->getName();
    if (checkSDFF(masterName)) {
      continue; // ignore scan FF
    }

    std::string instName = inst->getName();
    splitString(instName);

    const size_t instHash = customHash(instName);
    if (instMap.find(instHash) == instMap.end()) {
      instMap[instHash] = 1;
    } else {
      instMap[instHash] += 1;
    }
  }

  int maxFFBits = 0;
  int64_t hashId = 0;
  for (auto& inst : instMap) {
    if (inst.second >= busLimit_) {
      multiFFMap[inst.first] = inst.second;
      if (maxFFBits < inst.second) {
        hashId = inst.first;
        maxFFBits = inst.second;
      }
    }
  }

  std::cout << "Number of multiFF instances = " << multiFFMap.size() << std::endl;
  std::cout << "maxFFBits = " << maxFFBits << std::endl;

  if (maxFFBits == 0) {
    std::cout << "[INFO] This is no multi-bit Flip Flops detected" << std::endl;
    dataflowVertices_.clear();
    return;
  }

  // create the data flow vertices
  dataflowVertices_.resize(multiFFMap.size() + numMacros + block_->getBTerms().size());
  int dVertexId = 0;
  std::map<int64_t, int> multiFFDVertexIdMap;
  for (auto& inst : multiFFMap) {
    dataflowVertices_[dVertexId] = DVertex(dVertexId);
    multiFFDVertexIdMap[inst.first] = dVertexId; // map from hash id to dVertexId
    dVertexId++;
  }

  for (auto inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a Pad, Cover or a block
    // We ignore nets connecting Pads, Covers
    // for blocks, we iterate over the block pins
    if (master->isPad() || master->isCover()) {
      continue;
    }
    
    if (master->isBlock()) {
      dataflowVertices_[dVertexId] = DVertex(dVertexId, inst);
      auto prop = odb::dbIntProperty::find(inst, "dVertexId");
      if (prop == nullptr) {
        odb::dbIntProperty::create(inst, "dVertexId", dVertexId);
      }
      dVertexId++;
      continue;
    }
    
    const sta::LibertyCell* libertyCell = network_->libertyCell(inst);
    if (libertyCell == nullptr) {
      continue;
    }

    if (!libertyCell->hasSequentials()) {
      continue;  // ignore combinational cell
    }

    const std::string masterName = master->getName();
    if (checkSDFF(masterName)) {
      continue; // ignore scan FF
    }

    std::string instName = inst->getName();
    splitString(instName);

    const size_t instHash = customHash(instName);
    if (multiFFMap.find(instHash) == multiFFMap.end()) {
      continue;
    }

    const int instDVertexId = multiFFDVertexIdMap[instHash];
    dataflowVertices_[instDVertexId].addInst(inst);
    auto prop = odb::dbIntProperty::find(inst, "dVertexId");
    if (prop == nullptr) {
      odb::dbIntProperty::create(inst, "dVertexId", instDVertexId);
    }
  }

  for (auto bterm : block_->getBTerms()) {
    dataflowVertices_[dVertexId] = DVertex(dVertexId, bterm);
    auto prop = odb::dbIntProperty::find(bterm, "dVertexId");
    if (prop == nullptr) {
      odb::dbIntProperty::create(bterm, "dVertexId", dVertexId);
    }
    dVertexId++;
  }

  std::cout << "Finish here (a)" << std::endl;

  // create sequential graph
  std::map<int, odb::dbBTerm*> ioPinVertex;
  std::map<int, odb::dbInst*>  instVertex;
  std::vector<Vertex> seqVertices;
  // create the original netlist
  std::vector<std::vector<int> > vertices;
  std::vector<std::vector<int> > sinkHyperedges;  // dircted hypergraph


  std::cout << "Before createSeqGraph" << std::endl;
  createSeqGraph(ioPinVertex, instVertex, seqVertices, vertices, sinkHyperedges);
  std::cout << "After createSeqGraph" << std::endl;

  std::cout << "finish creating the sequential graph" << std::endl;


  // for debug, print the statistics
  // print the average fannots
  int64_t totalFanouts = 0;
  int maxDist = 0;
  for (auto& vertex : seqVertices) {
    totalFanouts += vertex.sinks.size();
    for (auto& sink : vertex.sinks) {
      maxDist = max(maxDist, sink.second);
    }
  }
  
  std::cout << "Average fanouts = " << static_cast<double>(totalFanouts) / static_cast<double>(seqVertices.size()) << std::endl;
  std::cout << "maxDist = " << maxDist << std::endl;

  // Reorder the cluster id
  int numClusters = 0;
  std::map<int, int> clusterIdReorderMap;
 
  // insts fill with real instances
  // update the clusters
  odb::dbSet<odb::dbInst> insts = block_->getInsts();
  for (odb::dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    // check if the instance has a cluster id attribute    
    auto clusterIdProp = odb::dbIntProperty::find(inst, "cluster_id");
    if (clusterIdProp != nullptr) {
      const int clusterId = clusterIdProp->getValue();
      if (clusterIdReorderMap.find(clusterId) == clusterIdReorderMap.end()) {
        clusterIdReorderMap[clusterId] = numClusters++; 
      }
      odb::dbIntProperty::find(inst, "cluster_id")->setValue(clusterIdReorderMap[clusterId]);
    }
  }

  std::cout << "finish remaping the cluster id" << std::endl;

  // create clusterId on each IO pin
  // clear the property
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    odb::dbIntProperty *prop = odb::dbIntProperty::find(bterm, "cluster_id");
    if (prop != nullptr) {  // Ensure the property exists before trying to delete it
      odb::dbIntProperty::find(bterm, "cluster_id")->setValue(numClusters++);
    } else {
      odb::dbIntProperty::create(bterm, "cluster_id", numClusters++);
    }
  }

  std::cout << "finish remaping the IO cluster id" << std::endl;

  if (numClusters > 0 && dataflowFlag_ == true) {
   
    // We need to determine the connections between clusters
    // We have number of clusters from 0, ..., numClusters_ - 1 (really clusters + IOs)
    adjMatrix_.resize(numClusters);   
    // A breath-first search at G_{seq} starts simultaneously from all components
    // of block i traversing only outgoing edges through glue logic.
    // When a component of block j is reached, the bitwidth of its predecessor
    // in the path is added to the bin corresponding to the number of flops stage
    // create the virtaul nodes for each multiFF instance
    for (auto& dVertex : dataflowVertices_) {
      if (dVertex.isBTerm()) {
        const int seqVertexId = odb::dbIntProperty::find(dVertex.getBTerm(), "seqVertexId")->getValue();
        const int clusterId = odb::dbIntProperty::find(dVertex.getBTerm(), "cluster_id")->getValue();
        addVirtualConnection(adjMatrix_[clusterId],
                             clusterId,
                             seqVertexId,
                             0,
                             numHops_,
                             1,
                             ioPinVertex,
                             instVertex,
                             seqVertices);
      } else if (dVertex.isMacro()) {
        const int seqVertexId = odb::dbIntProperty::find(dVertex.getMacro(), "seqVertexId")->getValue();
        const int clusterId = odb::dbIntProperty::find(dVertex.getMacro(), "cluster_id")->getValue();
        int bitWidth = 0;
        for (odb::dbITerm* pin : dVertex.getMacro()->getITerms()) {
          if (pin->getSigType() != odb::dbSigType::SIGNAL || pin->getIoType() != odb::dbIoType::OUTPUT) {
            continue;
          }
          bitWidth++;
        }
        addVirtualConnection(adjMatrix_[clusterId],
          clusterId,
          seqVertexId,
          0,
          numHops_,
          bitWidth,
          ioPinVertex,
          instVertex,
          seqVertices);
      } else {
        for (auto& inst : dVertex.getFFs()) {
          const int seqVertexId = odb::dbIntProperty::find(inst, "seqVertexId")->getValue();
          const int clusterId = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
          addVirtualConnection(adjMatrix_[clusterId],
            clusterId,
            seqVertexId,
            0,
            numHops_,
            1,
            ioPinVertex,
            instVertex,
            seqVertices);
        }
      }
    }  
  }

  std::cout << "finish adding virtual connection" << std::endl;
  if (debugFlag_) {
    for (int i = 0; i < adjMatrix_.size(); i++) {
      std::cout << "clusterId = " << i << std::endl;
      for (auto& adj : adjMatrix_[i]) {
        std::cout << "adj = " << adj.first << " " << adj.second << std::endl;
      }
    }
  }

  if (datapathFlag_ == true) {
  // create the virtaul nodes for each multiFF instance
  for (auto& dVertex : dataflowVertices_) {
    if (dVertex.isBTerm()) {
      const int seqVertexId = odb::dbIntProperty::find(dVertex.getBTerm(), "seqVertexId")->getValue();
      addDataflowEdge(dVertex, seqVertexId, maxDist, ioPinVertex, instVertex, seqVertices);
    } else if (dVertex.isMacro()) {
      const int seqVertexId = odb::dbIntProperty::find(dVertex.getMacro(), "seqVertexId")->getValue();
      addDataflowEdge(dVertex, seqVertexId, maxDist, ioPinVertex, instVertex, seqVertices);
    } else {
      for (auto& inst : dVertex.getFFs()) {
        const int seqVertexId = odb::dbIntProperty::find(inst, "seqVertexId")->getValue();
        addDataflowEdge(dVertex, seqVertexId, maxDist, ioPinVertex, instVertex, seqVertices);
      }
    }
  }
 }
}


// Extract the sequence graph from the original netlist
// sequence graph consists of FFs, macros and IOs, we do not consider SDFFs
// Edges between sequential components are inferred by
// analyzing their transitive fanin/fanout in the original netlist

void buildSeqGraphFromVertex(
  Vertex& vertex,
  const int vertexId,
  int step, 
  const std::vector<std::vector<int> >& vertices,
  const std::vector<std::vector<int> >& sinkHyperedges,
  const std::vector<bool>& stopFlagVec,
  std::unordered_set<int>& visited)
{
  if (stopFlagVec[vertexId] == true && vertex.src != vertexId) {
    vertex.addSink(vertexId, step);
    return;
  }

  visited.insert(vertexId);
  for (auto hyperedgeId : vertices[vertexId]) {
    for (auto& sink : sinkHyperedges[hyperedgeId]) {
      if (visited.find(sink) != visited.end()) {
        continue; // This sink has been visited
      }
      buildSeqGraphFromVertex(vertex, sink, step + 1, vertices, sinkHyperedges, stopFlagVec, visited);
    }
  }
}               


void PlacerBaseCommon::clearPinProperty()
{
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    odb::dbProperty::destroyProperties(bterm);
  }
}

void PlacerBaseCommon::clearInstProperty()
{
  for (auto inst : block_->getInsts()) {
    odb::dbProperty::destroyProperties(inst);
  }
}

void PlacerBaseCommon::createSeqGraph(
  std::map<int, odb::dbBTerm*>& ioPinVertex,
  std::map<int, odb::dbInst*>&  instVertex,
  std::vector<Vertex>& seqVertices,
  // create the original netlist, directed hypergraph
  std::vector<std::vector<int> >& vertices,
  std::vector<std::vector<int> >& sinkHyperedges)
{
  std::vector<bool> stopFlagVec; // stop at IO pins, FFs and macros
  int vertexId = 0;
  // assign vertexId property of each Bterm
  // All boundary terms are marked as sequential stopping points
  for (odb::dbBTerm* term : block_->getBTerms()) {
    auto prop1 = odb::dbIntProperty::find(term, "vertexId");
    if (prop1 == nullptr) {
      odb::dbIntProperty::create(term, "vertexId",  vertexId);
    }
    ioPinVertex[vertexId] = term;
    vertexId++;
    stopFlagVec.push_back(true);
    auto prop2 = odb::dbIntProperty::find(term, "seqVertexId");
    if (prop2 == nullptr) {
      odb::dbIntProperty::create(term, "seqVertexId", seqVertices.size());
    }
    seqVertices.emplace_back(vertexId, true); // true means it is an IO pin
  }

  std::cout << "[Test] Finish adding IO pins" << std::endl;
    
  // assign vertexId property of each instance
  for (auto inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    // check if the instance is a Pad, Cover or a block
    // We ignore nets connecting Pads, Covers
    // for blocks, we iterate over the block pins
    if (master->isPad() || master->isCover()) {
      continue;
    }

    const sta::LibertyCell* libertyCell = network_->libertyCell(inst);
    if (libertyCell == nullptr) {
      continue;
    }

    // mark sequential instances
    if (master->isBlock() || (libertyCell->hasSequentials() && !checkSDFF(master->getName()))) {
      auto prop1 = odb::dbIntProperty::find(inst, "vertexId");
      if (prop1 == nullptr) {
        odb::dbIntProperty::create(inst, "vertexId",  vertexId);
      }
      auto prop2 = odb::dbIntProperty::find(inst, "seqVertexId");
      if (prop2 == nullptr) {
        odb::dbIntProperty::create(inst, "seqVertexId", seqVertices.size());
      }
      instVertex[vertexId] = inst;
      stopFlagVec.push_back(true); // Sequential cells 
      seqVertices.emplace_back(vertexId, false); // false means it is not an IO pin
      vertexId++;
    } else if (!libertyCell->hasSequentials()) { 
      auto prop1 = odb::dbIntProperty::find(inst, "vertexId");
      if (prop1 == nullptr) {
        odb::dbIntProperty::create(inst, "vertexId",  vertexId);
      }
      instVertex[vertexId] = inst;
      stopFlagVec.push_back(false); // Comb cells
      vertexId++;
    } else {
      auto prop1 = odb::dbIntProperty::find(inst, "vertexId");
      if (prop1 == nullptr) {
        odb::dbIntProperty::create(inst, "vertexId",  -1);
      }
    }
  }

  std::cout << "[Test] Finish adding instances" << std::endl;

  // create the original netlist
  vertices.resize(stopFlagVec.size());
  // traverse the netlist
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply() || net->getITerms().size() >= largeNetThreshold_) {
      continue;
    }
  
    int driverId = -1;      // driver vertex id
    std::set<int> loadsId;  // load vertex id
    bool padSDFFFlag = false;
    
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();
      // check if the instance is a Pad, Cover or empty block (such as marker)
      // We ignore nets connecting Pads, Covers, or markers
      if (master->isPad() || master->isCover()) {
        padSDFFFlag = true;
        break;
      }
      
      const int vertexId = odb::dbIntProperty::find(inst, "vertexId")->getValue();
      if (vertexId == -1) {
        continue;
      }      
      
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driverId = vertexId;
      } else {
        loadsId.insert(vertexId);
      }
    }
    
    if (padSDFFFlag) {
      continue;  // the nets with Pads should be ignored
    }

    // check the connected IO pins  of the net
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertexId
          = odb::dbIntProperty::find(bterm, "vertexId")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driverId = vertexId;
      } else {
        loadsId.insert(vertexId);
      }
    }

    //
    // Skip high fanout nets or nets that do not have valid driver or loads
    //
    if (driverId < 0 || loadsId.size() < 1 || loadsId.size() > largeNetThreshold_) {
      continue;
    }

    // Create the hyperedge
    std::vector<int> hyperedge;
    for (auto& load : loadsId) {
      if (load != driverId) {
        hyperedge.push_back(load);
      }
    }
    
    vertices[driverId].push_back(sinkHyperedges.size());  
    sinkHyperedges.push_back(hyperedge);
  }  // end net traversal

  std::cout << "[Test] Finish adding nets" << std::endl;

  std::unordered_set<int> visited;
  // Build the sequence graph
  for (auto& vertex : seqVertices) {
    visited.clear();
    buildSeqGraphFromVertex(vertex, vertex.src, 0, vertices, sinkHyperedges, stopFlagVec, visited);
  }
  visited.clear();
  std::cout << "[Test] Finish building the sequence graph" << std::endl;
}




// ---------------------------------------------------------------------------------------------------------------------
// for standard cell placement
// ---------------------------------------------------------------------------------------------------------------------

void PlacerBaseCommon::init()
{
  odb::dbBlock* block = db_->getChip()->getBlock();
  // get the site information
  odb::dbSite* site = nullptr;
  for (auto* row : block->getRows()) {
    if (row->getSite()->getClass() != odb::dbSiteClass::PAD) {
      site = row->getSite();
      break;
    }
  }

  // siteSize update
  siteSizeX_ = site->getWidth();
  siteSizeY_ = site->getHeight();

  // get the die information
  odb::Rect coreRect = block->getCoreArea();
  odb::Rect dieRect = block->getDieArea();
  die_ = Die(dieRect, coreRect);

  // cluster constraint 
  // Reorder the cluster id
  int numClusters = 0;
  numPlaceInsts_ = 0;
  numInsts_ = 0;
  // insts fill with real instances
  odb::dbSet<odb::dbInst> insts = block->getInsts();
  for (odb::dbInst* inst : insts) {
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    numInsts_++;
    if (!isFixedOdbInst(inst)) { 
      // call the utility function to check if the instance is a fixed instance
      numPlaceInsts_++;
    }

    // check if the instance has a cluster id attribute    
    auto clusterIdProp = odb::dbIntProperty::find(inst, "cluster_id");
    if (clusterIdProp != nullptr) {
      const int clusterId = clusterIdProp->getValue();
      numClusters = max(numClusters, clusterId + 1);
    }
  }
  
  if (clusterConstraintFlag_ == false) {
    numClusters = 0;
  }

  // datapath constraint
  // Number of multi-bit FFs
  int numMultiFFs = 0;
  for (auto& vertex : dataflowVertices_) {
    if (vertex.getFFs().size() > 1) {
      numMultiFFs++;
    }
  }

  if (datapathFlag_ == false) {
    numMultiFFs = 0;
    dataflowVertices_.clear();
  }

  std::map<int, std::pair<int, int> > clusterLoc;
  const int numRealPlaceInsts = numPlaceInsts_;
  numInsts_ += numClusters + numMultiFFs;
  numPlaceInsts_ += numClusters + numMultiFFs;
  numFixedInsts_ = numInsts_ - numPlaceInsts_;

  // allocate the objects on host side
  instStor_.resize(numInsts_);
  dbInstStor_.resize(numInsts_);
  // different to original RePlAce codes,
  // we assign inst_id property to each inst
  // So we do not use inst_map_
  int placeInstId = -1;
  int fixedInstId = -1;
  for (odb::dbInst* inst : insts) {  
    // check the instance id
    odb::dbIntProperty::create(inst, "instId", -1);    
    auto type = inst->getMaster()->getType();
    if (!type.isCore() && !type.isBlock()) {
      continue;
    }
    int instId = -1;
    // check if the instance is a fixed instance
    if (isFixedOdbInst(inst)) {
      fixedInstId++;
      instId = numPlaceInsts_ + fixedInstId; 
    } else {
      placeInstId++;
      instId = placeInstId;
    } 
    odb::dbIntProperty::find(inst, "instId")->setValue(instId);
    // create the GpuInstance
    Instance myInst(inst, 
                    pbVars_.padLeft * siteSizeX_,
                    pbVars_.padRight * siteSizeX_,
                    siteSizeY_,
                    pbVars_.row_limit,
                    log_);    
    if (type.isBlock()) {
      myInst.setHaloWidth(haloWidth_);
    }
    // Fixed instaces need to be snapped outwards to the nearest site
    // boundary.  A partially overlapped site is unusable and this
    // is the simplest way to ensure it is counted as fully used.
    if (myInst.isFixed()) {
      myInst.snapOutward(coreRect.ll(), siteSizeX_, siteSizeY_);
    } 
    dbInstStor_[instId] = inst;
    instStor_[instId] = myInst;
    
    // for clustered netlist
    auto prop = odb::dbIntProperty::find(inst, "cluster_id");
    if (prop != nullptr) {
      const int clusterId = prop->getValue();
      if (clusterLoc.find(clusterId) == clusterLoc.end()) {
        clusterLoc[clusterId] = std::make_pair(myInst.cx(), myInst.cy());
      }
    }

    const int64_t instArea = myInst.area();
    if (myInst.isMacro()) {
      macroInstsArea_ += instArea;
    } else {
      stdCellInstsArea_ += instArea;
    }

    if (myInst.isFixed()) {
      nonPlaceInstsArea_ += instArea;
    } else {
      placeInstsArea_ += instArea;
    }
  }


  // for clustered netlist
  // create virtual instances for each cluster
  // 2729, 2800 is the size of a dummy instance
  for (int clusterId = 0; clusterId < numClusters; clusterId++) {
    placeInstId++;
    auto& loc = clusterLoc[clusterId];
    Instance myInst(loc.first, loc.second, 2729, 2800, false); // just a small instance
    dbInstStor_[placeInstId] = nullptr;
    instStor_[placeInstId] = myInst;
  } 

  
  // for multi-bit FFs
  for (auto& vertex : dataflowVertices_) {
    if (vertex.getFFs().size() > 1) {
      placeInstId++;
      int64_t cx = 0;
      int64_t cy = 0;
      for (auto& inst : vertex.getFFs()) {
        int lx = 0;
        int ly = 0;
        inst->getLocation(lx, ly);
        lx += floor(inst->getBBox()->getDX() / 2);
        ly += floor(inst->getBBox()->getDY() / 2);
        cx += lx;
        cy += ly;
      }
      cx = static_cast<float>(cx) / vertex.getFFs().size();
      cy = static_cast<float>(cy) / vertex.getFFs().size(); 
      Instance myInst(cx, cy, 2729, 2800, false); // just a small instance
      vertex.instId = placeInstId;
      dbInstStor_[placeInstId] = nullptr;
      instStor_[placeInstId] = myInst;
    }
  }

  // create pointers on for the host objects
  for (auto& inst : instStor_) {
    insts_.push_back(&inst);
    if (!inst.isFixed()) {
      placeInsts_.push_back(&inst);
    }
  }

  // nets fill
  odb::dbSet<odb::dbNet> nets = block->getNets();
  // TODO: I obersve that if the reserve size is not large enough, the program will crash
  netStor_.reserve(nets.size() * 100);   // Here we do not use resize
  dbNetStor_.reserve(nets.size() * 100);  
  pinStor_.reserve(nets.size() * 100); // average degree is around 3
  dbPinStor_.reserve(nets.size() * 100);
  int netId = -1;
  int pinId = -1;
  int ignoreNet = 0;
  for (odb::dbNet* net : nets) {
    odb::dbSigType netType = net->getSigType();
    odb::dbIntProperty *prop = odb::dbIntProperty::find(net, "netId");
    if (prop != nullptr) {  // Ensure the property exists before trying to delete it
      odb::dbProperty::destroy(prop); 
    }
    odb::dbIntProperty::create(net, "netId", -1);    
    
    // escape nets with VDD/VSS/reset nets
    if (netType == odb::dbSigType::SIGNAL || netType == odb::dbSigType::CLOCK) {
      const int num_fanouts = net->getITerms().size() + net->getBTerms().size();
      // We can enable this to further improve the runtime
      if (num_fanouts <= 1 || num_fanouts > 10000000) {
        ignoreNet++;
        continue;
      }
         
      netId++;
      dbNetStor_.push_back(net);
      odb::dbIntProperty::find(net, "netId")->setValue(netId);
      
      Net myNet(net); 
      netStor_.push_back(myNet);
      
      // check all the instance pins connected to this net
      for (odb::dbITerm* iTerm : net->getITerms()) {
        pinId++;
        odb::dbIntProperty::create(iTerm, "pinId", pinId);    
        Pin myPin(iTerm, log_);   
        // link the pin with the net
        myPin.setNet(&netStor_.back());
        // map the pin to its inst
        int instId = odb::dbIntProperty::find(iTerm->getInst(), "instId")->getValue();
        myPin.setInstance(&instStor_[instId]);
        pinStor_.push_back(myPin);
      }

      for (odb::dbBTerm* bTerm : net->getBTerms()) {
        pinId++;
        odb::dbIntProperty *prop = odb::dbIntProperty::find(bTerm, "pinId");
        if (prop != nullptr) {  // Ensure the property exists before trying to delete it
          odb::dbProperty::destroy(prop); 
        }
        odb::dbIntProperty::create(bTerm, "pinId", pinId);    
        Pin myPin(bTerm, log_);
        // link the pin with the net
        myPin.setNet(&netStor_.back());
        pinStor_.push_back(myPin);
      }
    }
  }


  // create cluster constraints
  if (numClusters > 0) {
    // create virtual nets within each cluster
    for (odb::dbInst* inst : insts) {  
      const int instId = odb::dbIntProperty::find(inst, "instId")->getValue();
      if (instId == -1) {
        continue;
      }
      const int clusterId = odb::dbIntProperty::find(inst, "cluster_id")->getValue();
      // create a virtual net
      // create a virtual pin for each instance and a cluster
      netId++;
      Net myNet(netId);
      myNet.setVirtualWeight(1.0);
      netStor_.push_back(myNet);

      // create a virtual pin for each instance
      pinId++;
      Pin myPin(pinId);
      myPin.setNet(&netStor_.back());
      myPin.setInstance(&instStor_[instId]);
      pinStor_.push_back(myPin);

      // create a virtual pin for each cluster
      pinId++;
      Pin myPin2(pinId);
      myPin2.setNet(&netStor_.back());
      myPin2.setInstance(&instStor_[numRealPlaceInsts + clusterId]);
      pinStor_.push_back(myPin2);
    }
  }


  // create the datapath constraints
  int FF_limit = 2;
  float FF_weight = 1.0 / 2980; // default value exp(-8)

  if (numMultiFFs > 0) {
    for (auto& vertex : dataflowVertices_) {
      if (vertex.getFFs().size() >= FF_limit) {
        int numSinks = 0;
        for (auto& sink : vertex.sinks) {
          for (auto& inst : dataflowVertices_[sink.first].getFFs()) {
            const int instId = odb::dbIntProperty::find(inst, "instId")->getValue();
            if (instId == -1) {
              continue;
            }
            numSinks++;
          }

          if (dataflowVertices_[sink.first].isBTerm()) {
            auto prop1 = odb::dbIntProperty::find(dataflowVertices_[sink.first].getBTerm(), "pinId");
            if (prop1 == nullptr) {  // Ensure the property exists before trying to delete it
              continue;
            }
            numSinks++;
          }
        }

        if (numSinks == 0) {
          continue;
        }

        int numFFs = vertex.getFFs().size();
        // create a virtual net
        // create a virtual pin for each instance and a cluster
        netId++;
        Net myNet(netId);
        myNet.setVirtualWeight(1.0 / (numFFs - 1) * FF_weight);
        netStor_.push_back(myNet);
    
        // create a virtual pin for each instance
        pinId++;
        Pin myPin(pinId);
        myPin.setNet(&netStor_.back());
        myPin.setInstance(&instStor_[vertex.instId]);
        pinStor_.push_back(myPin);
        
        // create a virtual pin for each cluster
        for (auto& inst : vertex.getFFs()) {
          const int instId = odb::dbIntProperty::find(inst, "instId")->getValue();
          if (instId == -1) {
            continue;
          }
          
          pinId++;
          Pin myPin2(pinId);
          myPin2.setNet(&netStor_.back());
          myPin2.setInstance(&instStor_[instId]);
          pinStor_.push_back(myPin2);
        }
      }
    }
  }

  for (auto& net : netStor_) {
    nets_.push_back(&net);
  }

  for (auto& pin : pinStor_) {
    pins_.push_back(&pin);
    if (pin.net() != nullptr) {
      pin.net()->addPin(&pin);
    }
    if (pin.isITerm()) {
      pin.instance()->addPin(&pin);
    }
  }

  initVirtualWeightFactor_ = std::exp(virtualIter_);
  virtualWeightFactor_ = std::exp(virtualIter_);
  std::cout << "[INFO] initVirtualWeightFactor = " << initVirtualWeightFactor_ << std::endl;
  std::cout << "[INFO] virtualWeightFactor = " << virtualWeightFactor_ << std::endl;

  // print the statistics
  printInfo(); 
}

void PlacerBaseCommon::updateVirtualWeightFactor(int iter) {
  virtualWeightFactor_ = initVirtualWeightFactor_ / std::exp(iter);
}

void PlacerBaseCommon::reset()
{
  db_ = nullptr;
  pbVars_.reset();

  // we need to free the memory on host and device
  // clear the vectors
  instStor_.clear();
  pinStor_.clear();
  netStor_.clear();

  dbInstStor_.clear();
  dbPinStor_.clear();
  dbNetStor_.clear();

  placeInsts_.clear();
  nets_.clear();
  insts_.clear();
  pins_.clear();

  numInsts_ = 0;
  numPlaceInsts_ = 0;
  numFixedInsts_ = 0;
  numDummyInsts_ = 0;

  placeInstsArea_ = 0;
  nonPlaceInstsArea_ = 0;
  macroInstsArea_ = 0;
  stdCellInstsArea_ = 0;

  clearPinProperty();

  if (clusterFlag_ == false) {
    clearInstProperty();
  }

  freeCUDAKernel();
}

// basic information
void PlacerBaseCommon::printInfo() const
{
  std::string msg;
  float dbuPerMicron = static_cast<float>(db_->getTech()->getDbUnitsPerMicron());

  msg = "NumInstances: " + std::to_string(numInsts_);
  log_->report(msg);

  msg = "NumPlaceInstances: " + std::to_string(numPlaceInsts_);
  log_->report(msg);

  msg = "NumFixedInstances: " + std::to_string(numFixedInsts_);
  log_->report(msg);

  msg = "NumDummyInstances: " + std::to_string(numDummyInsts_);
  log_->report(msg);

  msg = "DieAreaLxLy: " + floatToStringWithPrecision(static_cast<float>(die_.dieLx()) / dbuPerMicron, 2);
  msg += " " + floatToStringWithPrecision(static_cast<float>(die_.dieLy()) / dbuPerMicron, 2);
  log_->report(msg);

  msg = "DieAreaUxUy: " + floatToStringWithPrecision(static_cast<float>(die_.dieUx()) / dbuPerMicron, 2);
  msg += " " + floatToStringWithPrecision(static_cast<float>(die_.dieUy()) / dbuPerMicron, 2);
  log_->report(msg);

  msg = "CoreAreaLxLy: " + floatToStringWithPrecision(static_cast<float>(die_.coreLx()) / dbuPerMicron, 2);
  msg += " " + floatToStringWithPrecision(static_cast<float>(die_.coreLy()) / dbuPerMicron, 2);
  log_->report(msg);

  msg = "CoreAreaUxUy: " + floatToStringWithPrecision(static_cast<float>(die_.coreUx()) / dbuPerMicron, 2);
  msg += " " + floatToStringWithPrecision(static_cast<float>(die_.coreUy()) / dbuPerMicron, 2);
  log_->report(msg);
  

  const int64_t coreArea = die_.coreArea();
  float util = static_cast<double>(stdCellInstsArea_ + macroInstsArea_)
             / static_cast<double>(coreArea) * 100;
  msg = "CoreArea: " + floatToStringWithPrecision(static_cast<float>(coreArea) / dbuPerMicron / dbuPerMicron, 2);
  log_->report(msg);

  msg = "NonPlaceInstsArea: " + floatToStringWithPrecision(static_cast<float>(nonPlaceInstsArea_) / dbuPerMicron / dbuPerMicron, 2);
  log_->report(msg);

  msg = "PlaceInstsArea: " + floatToStringWithPrecision(static_cast<float>(placeInstsArea_) / dbuPerMicron / dbuPerMicron, 2);
  log_->report(msg);

  msg = "StdInstsArea: " + floatToStringWithPrecision(static_cast<float>(stdCellInstsArea_) / dbuPerMicron / dbuPerMicron, 2);
  log_->report(msg);

  msg = "MacroInstsArea: " + floatToStringWithPrecision(static_cast<float>(macroInstsArea_) / dbuPerMicron / dbuPerMicron, 2);
  log_->report(msg);

  msg = "Util(%): " + floatToStringWithPrecision(util, 2);
  log_->report(msg);
  
  if (util >= 100.1) {
    log_->error(GPL2, 301, "Utilization exceeds 100%.");
  }
}


int64_t PlacerBaseCommon::hpwl() const
{
  if (wlGradOp_ != nullptr) {
    //return wlGradOp_->computeHPWL();
    return wlGradOp_->computeWeightedHPWL(virtualWeightFactor_);
  } else {
    return 0;
  }
}

void PlacerBaseCommon::updatePinLocation()
{
  if (wlGradOp_ != nullptr) {
    wlGradOp_->updatePinLocation(dInstDCxPtr_, dInstDCyPtr_);
  }
}

// calculate the wirelength gradient
// we encapsulate the GPU acceleration into the WireLengthGradientOp class
void PlacerBaseCommon::updateWireLengthForce(const float wlCoeffX, const float wlCoeffY)
{
  if (wlGradOp_ != nullptr) {
    wlGradOp_->computeWireLengthForce(wlCoeffX, 
                                      wlCoeffY, 
                                      virtualWeightFactor_,
                                      dWLGradXPtr_,
                                      dWLGradYPtr_);
  }
}

double PlacerBaseCommon::wireLengthRuntime() const
{
  if (wlGradOp_ != nullptr) {
    return wlGradOp_->runtime();
  } else {
    return 0.0;
  }
}


////////////////////////////////////////////////////////////////////////////////
// Class PlacerBase
////////////////////////////////////////////////////////////////////////////////

PlacerBase::PlacerBase() 
  : db_(nullptr),
    log_(nullptr),
    pbCommon_(nullptr),
    group_(nullptr),
    densityOp_(nullptr),
    bg_(),
    die_(),
    siteSizeX_(0),
    siteSizeY_(0),
    nbVars_(),
    npVars_(),
    fillerDx_(0),
    fillerDy_(0),
    whiteSpaceArea_(0),
    movableArea_(0),
    totalFillerArea_(0),
    placeInstsArea_(0),
    nonPlaceInstsArea_(0),
    macroInstsArea_(0),
    stdInstsArea_(0),
    numInsts_(0),
    numNonPlaceInsts_(0),
    numPlaceInsts_(0),
    numFixedInsts_(0),
    numDummyInsts_(0),
    numFillerInsts_(0),
    sumPhi_(0.0),
    targetDensity_(0.0),
    uniformTargetDensity_(0.0),
    densityPenalty_(0.0),
    baseWireLengthCoef_(0.0),
    sumOverflow_(0.0),
    sumOverflowUnscaled_(0.0),
    prevHpwl_(0),
    isDiverged_(false),
    isMaxPhiCoefChanged_(false),
    minSumOverflow_(1e30),
    hpwlWithMinSumOverflow_(1e30),
    iter_(0),
    isConverged_(false),
    stepLength_(0.0),
    wireLengthGradSum_(0.0),
    densityGradSum_(0.0),
    dInstDDxPtr_(nullptr),
    dInstDDyPtr_(nullptr),
    dInstDCxPtr_(nullptr),
    dInstDCyPtr_(nullptr),
    dDensityGradXPtr_(nullptr),
    dDensityGradYPtr_(nullptr),
    dWireLengthGradXPtr_(nullptr),
    dWireLengthGradYPtr_(nullptr),
    dWireLengthPrecondiPtr_(nullptr),
    dDensityPrecondiPtr_(nullptr),
    dCurSLPCoordiPtr_(nullptr),
    dCurSLPSumGradsPtr_(nullptr),
    dCurSLPWireLengthGradXPtr_(nullptr),
    dCurSLPWireLengthGradYPtr_(nullptr),
    dCurSLPDensityGradXPtr_(nullptr),
    dCurSLPDensityGradYPtr_(nullptr),
    dPrevSLPCoordiPtr_(nullptr),
    dPrevSLPSumGradsPtr_(nullptr),
    dPrevSLPWireLengthGradXPtr_(nullptr),
    dPrevSLPWireLengthGradYPtr_(nullptr),
    dPrevSLPDensityGradXPtr_(nullptr),
    dPrevSLPDensityGradYPtr_(nullptr),
    dNextSLPCoordiPtr_(nullptr),
    dNextSLPSumGradsPtr_(nullptr),
    dNextSLPWireLengthGradXPtr_(nullptr),
    dNextSLPWireLengthGradYPtr_(nullptr),
    dNextSLPDensityGradXPtr_(nullptr),
    dNextSLPDensityGradYPtr_(nullptr),
    dCurCoordiPtr_(nullptr),
    dNextCoordiPtr_(nullptr)
  { }



// Constructor
PlacerBase::PlacerBase(NesterovBaseVars nbVars,
                       odb::dbDatabase* db,
                       std::shared_ptr<PlacerBaseCommon> pbCommon,
                       utl::Logger* log,
                       odb::dbGroup* group)
  : PlacerBase()
{
  nbVars_ = nbVars;
  db_ = db;
  log_ = log;
  pbCommon_ = std::move(pbCommon);
  group_ = group;
  init();
}


PlacerBase::~PlacerBase()
{
  reset();
}


void PlacerBase::reset()
{
  freeCUDAKernel();
}

void PlacerBase::init()
{
  // set a fixed seed 
  srand(42);
  // get the die information
  die_ = pbCommon_->die();
  // siteSize update
  siteSizeX_ = pbCommon_->siteSizeX();
  siteSizeY_ = pbCommon_->siteSizeY();
  // Here we call the objects on the host side
  for (auto& inst : pbCommon_->insts()) {
    if (!inst->isInstance()) {
      continue;
    }

    // check whether the instance is in the group
    if (inst->dbInst() && inst->dbInst()->getGroup() != group_) {
      continue;
    }
    if (inst->isFixed() && isCoreAreaOverlap(die_, inst)) {
      // Check whether fixed instance is
      // within the corearea
      // outside of corearea is none of RePlAce's business
      nonPlaceInsts_.push_back(inst);
      nonPlaceInstsArea_ += getOverlapWithCoreArea(die_, inst);
    } else {
      placeInsts_.push_back(inst);
      insts_.push_back(inst);
      int64_t instArea = inst->area();
      placeInstsArea_ += instArea;
      // macro cells should be macroInstsArea_
      if (inst->isMacro()) {
        macroInstsArea_ += instArea;
      } else {
        stdInstsArea_ += instArea;
      }
    }
  }

  // create the dummy instances
  // insts fill with fake instances (fragmented row or blockage)
  initInstsForUnusableSites();
  for (auto& inst : dummyInsts_) {
    nonPlaceInstsArea_ += inst.area();
    nonPlaceInsts_.push_back(&inst);
  }

  // update gFillerCells
  initFillerGCells();
  for (auto& inst : fillerInsts_) {
    insts_.push_back(&inst);
  }

  // get the statistics
  numPlaceInsts_ = placeInsts_.size();
  numDummyInsts_ = dummyInsts_.size();
  numFillerInsts_ = fillerInsts_.size();
  // Place insts can be moved, we identify them as insts
  numInsts_ = numPlaceInsts_ + numFillerInsts_;
  // fixed instances and dummy instances cannot be moved, we identify
  // them as nonPlaceInsts
  numNonPlaceInsts_ = nonPlaceInsts_.size();

    
  std::cout << "placeInstsArea_ = " << placeInstsArea_ << std::endl;
  std::cout << "numInsts_ " << numInsts_ << std::endl;


  // initialize bin grid structure
  if (nbVars_.isSetBinCnt) {
    bg_.setBinCnt(nbVars_.binCntX, nbVars_.binCntY);
  }
  bg_.setPlacerBase(this);
  bg_.setLogger(log_);
  bg_.setCorePoints(&die());
  bg_.setTargetDensity(targetDensity_);
  bg_.initBins();
 
  // update densitySize and densityScale in each gCell
  updateDensitySize();
}


std::pair<int, int> getMinMaxIdx(int ll,
                                 int uu,
                                 int coreLL,
                                 int siteSize,
                                 int minIdx,
                                 int maxIdx)
{
  int lowerIdx = (ll - coreLL) / siteSize;
  int upperIdx = (fastModulo((uu - coreLL), siteSize) == 0)
               ? (uu - coreLL) / siteSize
               : (uu - coreLL) / siteSize + 1;
  return std::make_pair(std::max(minIdx, lowerIdx), std::min(maxIdx, upperIdx));
}




// Use dummy instance to fill unusable sites.  Sites are unusable
// due to fragmented rows or placement blockages.
void PlacerBase::initInstsForUnusableSites()
{
  odb::dbSet<odb::dbRow> rows = db_->getChip()->getBlock()->getRows();
  odb::dbSet<odb::dbPowerDomain> pds = db_->getChip()->getBlock()->getPowerDomains();

  int64_t siteCountX = (die_.coreUx() - die_.coreLx()) / siteSizeX_;
  int64_t siteCountY = (die_.coreUy() - die_.coreLy()) / siteSizeY_;

  enum PlaceInfo
  {
    Empty, // This site cannot be used
    Row, // This site can be used
    FixedInst // This site is blocked by the fixed instance
  };

  //
  // Initialize siteGrid as empty
  //
  std::vector<PlaceInfo> siteGrid(siteCountX * siteCountY, PlaceInfo::Empty);
  // check if this belongs to a group
  // if there is a group, only mark the sites that belong to the group as Row
  // if there is no group, then mark all as Row, and then for each power domain,
  // mark the sites that belong to the power domain as Empty
  if (group_ != nullptr) {
    for (auto boundary : group_->getRegion()->getBoundaries()) {
      odb::Rect rect = boundary->getBox();
      std::pair<int, int> pairX = getMinMaxIdx(
          rect.xMin(), rect.xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);
      std::pair<int, int> pairY = getMinMaxIdx(
          rect.yMin(), rect.yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);
      for (int i = pairX.first; i < pairX.second; i++) {
        for (int j = pairY.first; j < pairY.second; j++) {
          siteGrid[j * siteCountX + i] = Row;
        }
      }
    }
  } else {
    // fill in rows' bbox
    for (odb::dbRow* row : rows) {
      odb::Rect rect = row->getBBox();
      std::pair<int, int> pairX = getMinMaxIdx(
          rect.xMin(), rect.xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);
      std::pair<int, int> pairY = getMinMaxIdx(
          rect.yMin(), rect.yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);
      for (int i = pairX.first; i < pairX.second; i++) {
        for (int j = pairY.first; j < pairY.second; j++) {
          siteGrid[j * siteCountX + i] = Row;
        }
      }
    }
  }

  // Mark blockage areas as empty so that their sites will be blocked.
  for (odb::dbBlockage* blockage : db_->getChip()->getBlock()->getBlockages()) {
    odb::dbInst* inst = blockage->getInstance();
    if (inst && !inst->isFixed()) {
      continue;
    }
    odb::dbBox* bbox = blockage->getBBox();
    std::pair<int, int> pairX = getMinMaxIdx(
        bbox->xMin(), bbox->xMax(), die_.coreLx(), siteSizeX_, 0, siteCountX);
    std::pair<int, int> pairY = getMinMaxIdx(
        bbox->yMin(), bbox->yMax(), die_.coreLy(), siteSizeY_, 0, siteCountY);

    // The placement density may be partial blockage
    // TODO : handle the partial blockage
    for (int j = pairY.first; j < pairY.second; j++) {
      for (int i = pairX.first; i < pairX.second; i++) {
        siteGrid[j * siteCountX + i] = Empty;
      }
    }
  }

  // fill fixed instances' bbox
  for (auto& inst : pbCommon_->insts()) {
    if (!inst->isFixed()) {
      continue;
    }
    std::pair<int, int> pairX = getMinMaxIdx(
        inst->lx(), inst->ux(), die_.coreLx(), siteSizeX_, 0, siteCountX);
    std::pair<int, int> pairY = getMinMaxIdx(
        inst->ly(), inst->uy(), die_.coreLy(), siteSizeY_, 0, siteCountY);

    for (int i = pairX.first; i < pairX.second; i++) {
      for (int j = pairY.first; j < pairY.second; j++) {
        siteGrid[j * siteCountX + i] = FixedInst;
      }
    }
  }

  // In the case of top level power domain i.e no group,
  // mark all other power domains as empty
  if (group_ == nullptr) {
    for (odb::dbPowerDomain* pd : pds) {
      if (pd->getGroup() != nullptr) {
        for (auto boundary : pd->getGroup()->getRegion()->getBoundaries()) {
          odb::Rect rect = boundary->getBox();
          std::pair<int, int> pairX = getMinMaxIdx(rect.xMin(),
                                                   rect.xMax(),
                                                   die_.coreLx(),
                                                   siteSizeX_,
                                                   0,
                                                   siteCountX);

          std::pair<int, int> pairY = getMinMaxIdx(rect.yMin(),
                                                   rect.yMax(),
                                                   die_.coreLy(),
                                                   siteSizeY_,
                                                   0,
                                                   siteCountY);

          for (int i = pairX.first; i < pairX.second; i++) {
            for (int j = pairY.first; j < pairY.second; j++) {
              siteGrid[j * siteCountX + i] = Empty;
            }
          }
        }
      }
    }
  }

  //
  // Search the "Empty" coordinates on site-grid
  // --> These sites need to be dummyInstance
  //
  for (int j = 0; j < siteCountY; j++) {
    for (int i = 0; i < siteCountX; i++) {
      // if empty spot found
      if (siteGrid[j * siteCountX + i] == Empty) {
        int startX = i;
        // find end points
        while (i < siteCountX && siteGrid[j * siteCountX + i] == Empty) {
          i++;
        }
        int endX = i;
        Instance myInst(die_.coreLx() + siteSizeX_ * startX,
                           die_.coreLy() + siteSizeY_ * j,
                           siteSizeX_ * (endX - startX),
                           siteSizeY_, 
                           true); // dummy instances
        dummyInsts_.push_back(myInst);
      }
    }
  }
}


// Note that filler cells can be moved around
// create the filler cells
void PlacerBase::initFillerGCells()
{
  // extract average dx/dy in range (10%, 90%)
  // Following codes are operated on the host side
  std::vector<int> dxStor;
  std::vector<int> dyStor;

  dxStor.reserve(pbCommon_->numPlaceInsts());
  dyStor.reserve(pbCommon_->numPlaceInsts());
  for (auto& placeInst : pbCommon_->placeInsts()) {
    dxStor.push_back(placeInst->dx());
    dyStor.push_back(placeInst->dy());
  }

  // sort
  std::sort(dxStor.begin(), dxStor.end());
  std::sort(dyStor.begin(), dyStor.end());

  // average from (10 - 90%)
  int64_t dxSum = 0;
  int64_t dySum = 0;

  int minIdx = dxStor.size() * 0.05;
  int maxIdx = dyStor.size() * 0.95;

  // when #instances are too small,
  // extracts average values in whole ranges
  if (minIdx == maxIdx) {
    minIdx = 0;
    maxIdx = dxStor.size();
  }

  for (int i = minIdx; i < maxIdx; i++) {
    dxSum += dxStor[i];
    dySum += dyStor[i];
  }

  // the avgDx and avgDy will be used as filler cells'
  // width and height
  fillerDx_ = static_cast<int>(dxSum / (maxIdx - minIdx));
  fillerDy_ = static_cast<int>(dySum / (maxIdx - minIdx));

  int64_t coreArea = die_.coreArea();
  whiteSpaceArea_ = coreArea - static_cast<int64_t>(nonPlaceInstsArea_);

  // targetDensity initialize
  if (nbVars_.useUniformTargetDensity) {
    // calculate the default uniform target density
    targetDensity_ = static_cast<float>(stdInstsArea_) /
                     static_cast<float>(whiteSpaceArea_ - macroInstsArea_)
                     + 0.01;
  } else {
    targetDensity_ = nbVars_.targetDensity;
  }

  // TODO density screening
  movableArea_ = whiteSpaceArea_ * targetDensity_;
  totalFillerArea_ = movableArea_ - nesterovInstsArea();
  uniformTargetDensity_ = static_cast<float>(nesterovInstsArea())
                          / static_cast<float>(whiteSpaceArea_);

  if (totalFillerArea_ < 0) {
    uniformTargetDensity_ = ceilf(uniformTargetDensity_ * 100) / 100;
    log_->error(GPL2,
      302,
      "Use a higher -density or "
      "re-floorplan with a larger core area.\n"
      "Given target density: {:.2f}\n"
      "Suggested target density: {:.2f}",
      targetDensity_,
      uniformTargetDensity_);
  }

  // calculate the number of filler cells
  // This may have some overflow issue
  numFillerInsts_ = static_cast<int>(
      totalFillerArea_ / (static_cast<int64_t>(fillerDx_ * fillerDy_)));
  if (numFillerInsts_ < 0) {
    numFillerInsts_ = 0;
  }
  
  if (pbCommon_->numPlaceInsts() <= 10000) {
    std::cout << "[DEBUG ]Manually set the numFillerInst_ to 0" << std::endl;
    numFillerInsts_ = 0;
  }

  // only for debug
  std::cout << "[GpuPlacerBase] totalFillerArea = " << totalFillerArea_ << std::endl;
  std::cout << "[GpuPlacerBase] FillerInit : NumFillerCells = " << numFillerInsts_ << std::endl;
  std::cout << "[GpuPlacerBase] FillerInit : FillerCellSize = " << fillerDx_ << " , " << fillerDy_ << std::endl;

  //
  // mt19937 supports huge range of random values.
  // rand()'s RAND_MAX is only 32767.
  //
  mt19937 randVal(0);
  for (int i = 0; i < numFillerInsts_; i++) {
    // instability problem between g++ and clang++!
    auto randX = randVal();
    auto randY = randVal();
    int cx = randX % pbCommon_->die().coreDx() + pbCommon_->die().coreLx();
    int cy = randY % pbCommon_->die().coreDy() + pbCommon_->die().coreLy();
    // place filler cells on random coordi and
    // set size as avgDx and avgDy
    Instance myGCell(cx, cy, fillerDx_, fillerDy_, false); // filler instance
    fillerInsts_.push_back(myGCell);
  }
}

// update densitySize and densityScale in each gCell
// We do not need to convert this into GPU kernel
// This is not the bottleneck currently
void PlacerBase::updateDensitySize()
{
  int instId = 0;
  for (auto& inst : insts_) {
    float scaleX = 0, scaleY = 0;
    float densitySizeX = 0, densitySizeY = 0;
    if (inst->dx() < REPLACE_SQRT2 * bg_.binSizeX()) {
      scaleX = static_cast<float>(inst->dx())
               / static_cast<float>(REPLACE_SQRT2 * bg_.binSizeX());
      densitySizeX = REPLACE_SQRT2 * static_cast<float>(bg_.binSizeX());
    } else {
      scaleX = 1.0;
      densitySizeX = inst->dx();
    }

    if (inst->dy() < REPLACE_SQRT2 * bg_.binSizeY()) {
      scaleY = static_cast<float>(inst->dy())
               / static_cast<float>(REPLACE_SQRT2 * bg_.binSizeY());
      densitySizeY = REPLACE_SQRT2 * static_cast<float>(bg_.binSizeY());
    } else {
      scaleY = 1.0;
      densitySizeY = inst->dy();
    }
    inst->setDensitySize(densitySizeX, densitySizeY, scaleX * scaleY);
  }
}


// update the step length
bool PlacerBase::nesterovUpdateStepLength()
{
  if (isConverged_) {
    return true;
  }

  float newStepLength = getStepLength(
      dCurSLPCoordiPtr_,
      dCurSLPSumGradsPtr_,
      dNextSLPCoordiPtr_,
      dNextSLPSumGradsPtr_);


  if (isnan(newStepLength) || isinf(newStepLength)) {
    isDiverged_ = true;
    divergeMsg_ = "RePlAce diverged at newStepLength.";
    divergeCode_ = 305;
    return false;
  }

  if (newStepLength > stepLength_ * 0.95) {
    stepLength_ = newStepLength;
    return false;
  }

  if (newStepLength < 0.01) {
    stepLength_ = 0.01;
    return false;
  }

  stepLength_ = newStepLength;

  return true;
}

// NestrovePlace related functions
void PlacerBase::updateDensityCenterCur()
{
  updateGCellDensityCenterLocation(dCurCoordiPtr_);
}

void PlacerBase::updateDensityCenterCurSLP()
{
  updateGCellDensityCenterLocation(dCurSLPCoordiPtr_);
}

void PlacerBase::updateDensityCenterPrevSLP()
{
  updateGCellDensityCenterLocation(dPrevSLPCoordiPtr_);
}

void PlacerBase::updateDensityCenterNextSLP()
{
  updateGCellDensityCenterLocation(dNextSLPCoordiPtr_);
}


void PlacerBase::updateDensityForceBin()
{
  densityOp_->updateDensityForceBin();
}

// dynamic adjustment for better convergence with large designs
void PlacerBase::nesterovAdjustPhi()
{
  if (isConverged_) {
    return;
  }

  // dynamic adjustment for
  // better convergence with
  // large designs
  if (!isMaxPhiCoefChanged_ && sumOverflowUnscaled_ < 0.35f) {
    isMaxPhiCoefChanged_ = true;
    npVars_.maxPhiCoef *= 0.99;
  }
}


float PlacerBase::getPhiCoef(float scaledDiffHpwl) const
{
  float retCoef = (scaledDiffHpwl < 0)
                ? npVars_.maxPhiCoef
                : npVars_.maxPhiCoef
                * pow(npVars_.maxPhiCoef, scaledDiffHpwl * -1.0);
  retCoef = std::max(npVars_.minPhiCoef, retCoef);
  return retCoef;
}

bool PlacerBase::checkConvergence()
{
  if (isConverged_) {
    return true;
  }

  //std::cout << "npVars_.targetOverflow = " << npVars_.targetOverflow << std::endl;
  if (numPlaceInsts_ <= 10000) {
    npVars_.targetOverflow = 0.2; 
    //std::cout << "[Update] npVars_.targetOverflow = " << npVars_.targetOverflow << std::endl;
  }
  
  if (sumOverflowUnscaled_ <= npVars_.targetOverflow) {
    if (group_) {
      std::string msg = "[NesterovSolve] PowerDomain ";
      msg += group_->getName();
      msg += " finished with Overflow: ";
      msg += std::to_string(sumOverflowUnscaled_);
      log_->report(msg);
    } else {
      std::string msg = "[NesterovSolve] Finished with Overflow: ";
      msg += std::to_string(sumOverflowUnscaled_);
      log_->report(msg);
    }

    isConverged_ = true;
    return true;
  }
  return false;
}


float PlacerBase::overflowArea() const {
  return densityOp_->sumOverflow();
}

// exchange the states:  prev -> current, current -> next
// update the parameters
void PlacerBase::updateNextIter(int iter)
{
  if (isConverged_) {
    return;
  }

  // Previous <= Current
  std::swap(dCurSLPCoordiPtr_, dPrevSLPCoordiPtr_);
  std::swap(dCurSLPSumGradsPtr_, dPrevSLPSumGradsPtr_);
  std::swap(dCurSLPWireLengthGradXPtr_, dPrevSLPWireLengthGradXPtr_);
  std::swap(dCurSLPWireLengthGradYPtr_, dPrevSLPWireLengthGradYPtr_);
  std::swap(dCurSLPDensityGradXPtr_, dPrevSLPDensityGradXPtr_);
  std::swap(dCurSLPDensityGradYPtr_, dPrevSLPDensityGradYPtr_);

  // Current <= Next
  std::swap(dCurSLPCoordiPtr_, dNextSLPCoordiPtr_);
  std::swap(dCurSLPSumGradsPtr_, dNextSLPSumGradsPtr_);
  std::swap(dCurSLPWireLengthGradXPtr_, dNextSLPWireLengthGradXPtr_);
  std::swap(dCurSLPWireLengthGradYPtr_, dNextSLPWireLengthGradYPtr_);
  std::swap(dCurSLPDensityGradXPtr_, dNextSLPDensityGradXPtr_);
  std::swap(dCurSLPDensityGradYPtr_, dNextSLPDensityGradYPtr_);
  
  std::swap(dCurCoordiPtr_, dNextCoordiPtr_);
  
  // In a macro dominated design like mock-array-big you may be placing
  // very few std cells in a sea of fixed macros. The overflow denominator
  // may be quite small and prevent convergence. This is mostly due to 
  // our limited ability to move instances off macros cleanly.
  // As that improves this should no longer be needed.
  const float fractionOfMaxIters
      = static_cast<float>(iter) / npVars_.maxNesterovIter;  
  const float overflowDenominator
      = std::max(static_cast<float>(nesterovInstsArea()),
                 fractionOfMaxIters * nonPlaceInstsArea() * 0.05f); 
      
  sumOverflow_ = overflowArea() / overflowDenominator;
  sumOverflowUnscaled_ = overflowAreaUnscaled() / overflowDenominator;

  int64_t hpwl = pbCommon_->hpwl();
  float phiCoef = getPhiCoef(static_cast<float>(hpwl - prevHpwl_)
                             / npVars_.referenceHpwl);

  prevHpwl_ = hpwl;
  // TODO:  use autotuner to autotune this parameter for better tradeoff between overflow and wirelength
  //densityPenalty_ *= phiCoef * 1.01;
  densityPenalty_ *= phiCoef * 0.99;

  if (iter == 0 || (iter + 1) % 10 == 0) {
    std::string msg = "[NesterovSolve] Iter: "+ std::to_string(iter + 1) + " ";
    msg += "overflow: " + std::to_string(sumOverflowUnscaled_) + " ";
    msg += "HPWL: " + std::to_string(prevHpwl_) + " ";
    msg += "densityPenalty: " + std::to_string(double(densityPenalty_));
    log_->report(msg);
  }

  if (iter > 50 && minSumOverflow_ > sumOverflowUnscaled_) {
    minSumOverflow_ = sumOverflowUnscaled_;
    hpwlWithMinSumOverflow_ = prevHpwl_;
  }
}

////////////////////////////////////////////////////////////////////////////
// class BinGrid
////////////////////////////////////////////////////////////////////////////
BinGrid::BinGrid()
  : log_(nullptr),
    pb_(nullptr),
    numBins_(0),
    lx_(0),
    ly_(0),
    ux_(0),
    uy_(0),
    binCntX_(0),
    binCntY_(0),
    binSizeX_(0),
    binSizeY_(0),
    targetDensity_(0),
    isSetBinCnt_(0)
{
}

BinGrid::BinGrid(Die* die) : BinGrid()
{
  setCorePoints(die);
}

BinGrid::~BinGrid()
{
  log_ = nullptr;
  pb_ = nullptr;
  binStor_.clear();
  bins_.clear();
  numBins_ = 0;
  binCntX_ = binCntY_ = 0;
  binSizeX_ = binSizeY_ = 0;
  isSetBinCnt_ = 0;
}


void BinGrid::setCorePoints(const Die* die)
{
  lx_ = die->coreLx();
  ly_ = die->coreLy();
  ux_ = die->coreUx();
  uy_ = die->coreUy();
}

void BinGrid::setBinCnt(int binCntX, int binCntY)
{
  isSetBinCnt_ = true;
  binCntX_ = binCntX;
  binCntY_ = binCntY;
}

static unsigned int roundDownToPowerOfTwo(unsigned int x)
{
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return x ^ (x >> 1);
}

void BinGrid::initBins()
{
  int64_t totalBinArea
    = static_cast<int64_t>(ux_ - lx_) 
      * static_cast<int64_t>(uy_ - ly_);
  int64_t averagePlaceInstArea
      = pb_->placeInstsArea() / pb_->numPlaceInsts();

  int64_t idealBinArea
      = std::round(
        static_cast<float>(averagePlaceInstArea) 
                           / targetDensity_);

  int idealBinCnt = totalBinArea / idealBinArea;
  if (idealBinCnt < 4) {  // the smallest we allow is 2x2 bins
    idealBinCnt = 4;
  }

  if (!isSetBinCnt_) {
    // Consider the apect ratio of the block when computing the number
    // of bins so that the bins remain relatively square.
    const unsigned int width = ux_ - lx_;
    const unsigned int height = uy_ - ly_;
    const int ratio = roundDownToPowerOfTwo(std::max(width, height)
                                            / std::min(width, height));
    int foundBinCnt = 2;
    // find binCnt: 2, 4, 8, 16, 32, 64, ...
    // s.t. #bins(binCnt) <= idealBinCnt <= #bins(binCnt*2).
    for (foundBinCnt = 2; foundBinCnt <= 1024; foundBinCnt *= 2) {
      if ((foundBinCnt == 2
           || foundBinCnt * (foundBinCnt * ratio) <= idealBinCnt)
           && 4 * foundBinCnt * (foundBinCnt * ratio) > idealBinCnt) {
        break;
      }
    }

    if (width > height) {
      binCntX_ = foundBinCnt * ratio;
      binCntY_ = foundBinCnt;
    } else {
      binCntX_ = foundBinCnt;
      binCntY_ = foundBinCnt * ratio;
    }
  }


  if (binCntX_ >= 512) {
    binCntX_ = 512;
    binCntY_ = 512;
  }

  binSizeX_ = ceil(static_cast<float>((ux_ - lx_)) / binCntX_);
  binSizeY_ = ceil(static_cast<float>((uy_ - ly_)) / binCntY_);
  // create bins_ on host and device
  numBins_ = binCntX_ * binCntY_;
  
  std::cout << "isSetBinCnt_ =  " << isSetBinCnt_ << std::endl;
  std::cout << "binSizeX_ = " << binSizeX_ << std::endl;
  std::cout << "binSizeY_ = " << binSizeY_ << std::endl;
  std::cout << "binCntX_ = " << binCntX_ << std::endl;
  std::cout << "binCntY_ = " << binCntY_ << std::endl;
  std::cout << "numBins = " << numBins_ << std::endl;
  std::cout << "targetDensity_ = " << targetDensity_ << std::endl;

  // create bins_ on host
  binStor_.reserve(numBins_);
  for (int idxY = 0; idxY < binCntY_; ++idxY) {
    for (int idxX = 0; idxX < binCntX_; ++idxX) {
      const int x = lx_ + idxX * binSizeX_;
      const int y = ly_ + idxY * binSizeY_;
      const int sizeX = std::min(ux_ - x, binSizeX_);
      const int sizeY = std::min(uy_ - y, binSizeY_);
      binStor_.emplace_back(
          idxX, idxY, x, y, x + sizeX, y + sizeY, targetDensity_);
    }
  }

  for (auto& bin : binStor_) {
    bins_.push_back(&bin);
  }

  updateBinsNonPlaceArea();
}



static int64_t getOverlapArea(const Bin* bin,
                              const Instance* inst,
                              int dbu_per_micron)
{
  int rectLx = max(bin->lx(), inst->lx()), rectLy = max(bin->ly(), inst->ly()),
      rectUx = min(bin->ux(), inst->ux()), rectUy = min(bin->uy(), inst->uy());

  if (rectLx >= rectUx || rectLy >= rectUy) {
    return 0;
  }

  if (inst->isMacro()) {
    const float meanX = (inst->cx() - inst->lx()) / (float) dbu_per_micron;
    const float meanY = (inst->cy() - inst->ly()) / (float) dbu_per_micron;

    // For the bivariate normal distribution, we are using
    // the shifted means of X and Y.
    // Sigma is used as the mean/4 for both dimensions
    const biNormalParameters i
        = {meanX,
           meanY,
           meanX / 4,
           meanY / 4,
           (rectLx - inst->lx()) / (float) dbu_per_micron,
           (rectLy - inst->ly()) / (float) dbu_per_micron,
           (rectUx - inst->lx()) / (float) dbu_per_micron,
           (rectUy - inst->ly()) / (float) dbu_per_micron};

    const float original = static_cast<float>(rectUx - rectLx)
                           * static_cast<float>(rectUy - rectLy);
    const float scaled = calculateBiVariateNormalCDF(i)
                         * static_cast<float>(inst->ux() - inst->lx())
                         * static_cast<float>(inst->uy() - inst->ly());

    // For heavily dense regions towards the center of the macro,
    // we are using an upper limit of 1.15*(overlap) between the macro
    // and the bin.
    if (scaled >= original) {
      return min<float>(scaled, original * 1.15);
    }
    // If the scaled value is smaller than the actual overlap
    // then use the original overlap value instead.
    // This is implemented to prevent cells from being placed
    // at the outer sides of the macro.
    else {
      return original;
    }
  } else {
    return static_cast<float>(rectUx - rectLx)
           * static_cast<float>(rectUy - rectLy);
  }
}


void BinGrid::updateBinsNonPlaceArea()
{
  for (auto bin : bins_) {
    bin->setNonPlaceArea(0);
  }

  for (auto& inst : pb_->nonPlaceInsts()) {
    std::pair<int, int> pairX = getMinMaxIdxX(inst);
    std::pair<int, int> pairY = getMinMaxIdxY(inst);
    for (int i = pairX.first; i < pairX.second; i++) {
      for (int j = pairY.first; j < pairY.second; j++) {
        Bin* bin = bins_[j * binCntX_ + i];
        // Note that nonPlaceArea should have scale-down with
        // target density.
        // See MS-replace paper
        //
        bin->addNonPlaceArea(
            getOverlapArea(
                bin,
                inst,
                pb_->db()->getChip()->getBlock()->getDbUnitsPerMicron())
            * bin->targetDensity());
      }
    }
  }
}

std::pair<int, int> BinGrid::getMinMaxIdxX(const Instance* inst) const
{
  int lowerIdx = (inst->lx() - lx()) / binSizeX_;
  int upperIdx = (fastModulo((inst->ux() - lx()), binSizeX_) == 0)
                     ? (inst->ux() - lx()) / binSizeX_
                     : (inst->ux() - lx()) / binSizeX_ + 1;

  return std::make_pair(std::max(lowerIdx, 0), std::min(upperIdx, binCntX_));
}

std::pair<int, int> BinGrid::getMinMaxIdxY(const Instance* inst) const
{
  int lowerIdx = (inst->ly() - ly()) / binSizeY_;
  int upperIdx = (fastModulo((inst->uy() - ly()), binSizeY_) == 0)
                     ? (inst->uy() - ly()) / binSizeY_
                     : (inst->uy() - ly()) / binSizeY_ + 1;

  return std::make_pair(std::max(lowerIdx, 0), std::min(upperIdx, binCntY_));
}

} // namespace gpl2
