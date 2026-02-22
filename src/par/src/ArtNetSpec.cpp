// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "ArtNetSpec.h"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "TritonPart.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbSet.h"
#include "odb/dbTypes.h"
#include "par/PartitionMgr.h"
#include "sta/Bfs.hh"
#include "sta/ConcreteNetwork.hh"
#include "sta/ExceptionPath.hh"
#include "sta/FuncExpr.hh"
#include "sta/Graph.hh"
#include "sta/GraphDelayCalc.hh"
#include "sta/Liberty.hh"
#include "sta/MakeConcreteNetwork.hh"
#include "sta/MinMax.hh"
#include "sta/Mode.hh"
#include "sta/Network.hh"
#include "sta/NetworkClass.hh"
#include "sta/ParseBus.hh"
#include "sta/Path.hh"
#include "sta/PathEnd.hh"
#include "sta/PathExpanded.hh"
#include "sta/PatternMatch.hh"
#include "sta/PortDirection.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/Sta.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

using odb::dbBTerm;
using odb::dbInst;
using odb::dbIoType;
using odb::dbITerm;
using odb::dbMaster;
using odb::dbMasterType;

using sta::CellPortBitIterator;
using sta::InstancePinIterator;
using sta::NetPinIterator;
using sta::NetTermIterator;
using utl::PAR;

namespace par {

int Cluster::next_id_ = 0;

void PartitionMgr::writeArtNetSpec(const char* fileName)
{
  if (!getDbBlock()) {
    logger_->error(PAR, 53, "Design not loaded.");
  }
  if (db_->getLibs().empty()) {
    logger_->error(PAR, 55, "Libs not loaded.");
  }

  std::map<std::string, MasterInfo> onlyUseMasters;
  std::string top_name;
  int numInsts = 0;
  int numMacros = 0;
  int numPIs = 0;
  int numPOs = 0;
  int numSeq = 0;
  int Dmax = -1;
  int MDmax = -1;
  float Rratio = 0.0;
  float p = 0.0;
  float q = 0.0;
  float avgK = 0.0;

  getFromODB(
      onlyUseMasters, top_name, numInsts, numMacros, numPIs, numPOs, numSeq);
  logger_->report("getFromODB done");
  getFromSTA(Dmax, MDmax);
  logger_->report("getFromSTA done");
  getFromPAR(Rratio, p, q, avgK);
  logger_->report("getFromPAR done");
  writeFile(onlyUseMasters,
            top_name,
            numInsts,
            numMacros,
            numPIs,
            numPOs,
            numSeq,
            Dmax,
            MDmax,
            Rratio,
            p,
            q,
            avgK,
            fileName);
}

void PartitionMgr::getFromODB(std::map<std::string, MasterInfo>& onlyUseMasters,
                              std::string& top_name,
                              int& numInsts,
                              int& numMacros,
                              int& numPIs,
                              int& numPOs,
                              int& numSeq)
{
  auto block = getDbBlock();
  odb::dbSet<dbInst> insts = block->getInsts();
  odb::dbSet<dbBTerm> bterms = block->getBTerms();
  numInsts = insts.size();

  for (auto bterm : bterms) {
    if (bterm->getIoType() == odb::dbIoType::INPUT) {
      numPIs++;
    }
    if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
      numPOs++;
    }
  }

  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    bool isMacro = (master->getType() == dbMasterType::BLOCK ? 1 : 0);
    const sta::LibertyCell* lib_cell = db_network_->libertyCell(inst);
    if (!lib_cell) {
      logger_->error(PAR, 56, "Liberty cell not found: {}", inst->getName());
    }
    if (lib_cell->hasSequentials()) {
      numSeq++;
    }
    auto [it, inserted]
        = onlyUseMasters.try_emplace(master->getName(), MasterInfo{});
    MasterInfo& info = it->second;
    ++info.count;
    if (inserted) {
      info.isMacro = isMacro;
    }
    if (isMacro) {
      numMacros++;
    }
  }
}

void PartitionMgr::getFromSTA(int& Dmax, int& MDmax)
{
  BuildTimingPath(Dmax, MDmax);
}

void PartitionMgr::BuildTimingPath(int& Dmax, int& MDmax)
{
  sta_->ensureGraph();     // Ensure that the timing graph has been built
  sta_->searchPreamble();  // Make graph and find delays
  sta_->ensureLevelized();
  // Step 1:  find the top_n critical timing paths
  sta::ExceptionFrom* e_from = nullptr;
  sta::ExceptionThruSeq* e_thrus = nullptr;
  sta::ExceptionTo* e_to = nullptr;
  bool include_unconstrained = false;
  bool get_max = true;  // max for setup check, min for hold check
  // Timing paths are grouped into path groups according to the clock
  // associated with the endpoint of the path, for example, path group for clk
  // int group_count = top_n_;
  int group_count = INT_MAX;
  int endpoint_count = 1;  // The number of paths to report for each endpoint.
  // Definition for findPathEnds function in Search.hh
  // PathEndSeq *findPathEnds(ExceptionFrom *from,
  //              ExceptionThruSeq *thrus,
  //              ExceptionTo *to,
  //              bool unconstrained,
  //              const Scene *corner,
  //              const MinMaxAll *min_max,
  //              int group_count,
  //              int endpoint_count,
  //              bool unique_pins,
  //              float slack_min,
  //              float slack_max,
  //              bool sort_by_slack,
  //              PathGroupNameSet *group_names,
  //              bool setup,
  //              bool hold,
  //              bool recovery,
  //              bool removal,
  //              bool clk_gating_setup,
  //              bool clk_gating_hold);
  // PathEnds represent search endpoints that are either unconstrained or
  // constrained by a timing check, output delay, data check, or path delay.
  sta::StdStringSeq group_names_empty;
  sta::PathEndSeq path_ends = sta_->search()->findPathEnds(  // from, thrus, to,
                                                             // unconstrained
      e_from,   // return paths from a list of clocks/instances/ports/register
                // clock pins or latch data pins
      e_thrus,  // return paths through a list of instances/ports/nets
      e_to,     // return paths to a list of clocks/instances/ports or pins
      include_unconstrained,  // return unconstrained paths
      // corner, min_max,
      sta_->cmdMode()->scenes(),  // return paths for a process corner
      get_max ? sta::MinMaxAll::max()
              : sta::MinMaxAll::min(),  // return max/min paths checks
      // group_count, endpoint_count, unique_pins
      group_count,     // number of paths in total
      endpoint_count,  // number of paths for each endpoint
      true,            // unique pins
      true,            // unique edges
      -sta::INF,
      sta::INF,           // slack_min, slack_max,
      true,               // sort_by_slack
      group_names_empty,  // group_names
      // setup, hold, recovery, removal,
      get_max,
      !get_max,
      false,
      false,
      // clk_gating_setup, clk_gating_hold
      false,
      false);

  auto block = getDbBlock();
  std::map<std::string, int> pathDepthMap;
  // check all the timing paths
  for (auto& path_end : path_ends) {
    // Printing timing paths to logger
    // sta_->reportPathEnd(path_end);
    auto* path = path_end->path();

    int depth = 0;
    std::string endPointName;
    std::unordered_set<std::string> visitedInstances;
    std::unordered_set<std::string> visitedBterms;

    sta::PathExpanded expand(path, sta_);
    for (size_t i = 0; i < expand.size(); i++) {
      const sta::Path* ref = expand.path(i);
      sta::Pin* pin = ref->vertex(sta_)->pin();
      // Nets connect pins at a level of the hierarchy
      auto net = db_network_->net(pin);  // sta::Net*
      // Check if the pin is connected to a net
      if (net == nullptr) {
        continue;  // check if the net exists
      }
      std::string name;

      if (db_network_->isTopLevelPort(pin) == true) {
        auto bterm = block->findBTerm(db_network_->pathName(pin));
        name = bterm->getName();
        if (visitedBterms.insert(name).second) {
          depth++;
        }
      } else {
        auto inst = db_network_->instance(pin);
        auto db_inst = block->findInst(db_network_->pathName(inst));
        name = db_inst->getName();
        if (visitedInstances.insert(name).second) {
          depth++;
        }
      }

      if (i == expand.size() - 1) {
        endPointName = name;
        pathDepthMap[endPointName] = depth;
      }
    }
  }  // path_end

  int ff_max = 0;
  int mac_max = 0;
  for (const auto& path : pathDepthMap) {
    auto inst = block->findInst((path.first).c_str());
    if (inst) {
      if (inst->getMaster()->isBlock()) {
        mac_max = std::max(path.second, mac_max);
      } else {
        ff_max = std::max(path.second, ff_max);
      }
    }
  }
  Dmax = ff_max;
  MDmax = mac_max;
}

void PartitionMgr::getFromPAR(float& Rratio, float& p, float& q, float& avgK)
{
  getRents(Rratio, p, q, avgK);
}

void PartitionMgr::getRents(float& Rratio, float& p, float& q, float& avgK)
{
  auto block = getDbBlock();
  ModuleMgr modMgr;
  SharedClusterVector cv;
  auto c = std::make_shared<Cluster>(0);
  cv.push_back(c);
  auto triton_part
      = std::make_shared<TritonPart>(db_network_, db_, sta_, logger_);
  double totPins = 0;
  int id = 0;
  for (dbInst* inst : block->getInsts()) {
    for (dbITerm* inst_iterm : inst->getITerms()) {
      if (inst_iterm->getIoType() == dbIoType::INPUT
          || inst_iterm->getIoType() == dbIoType::OUTPUT) {
        totPins++;
      }
    }
    c->addInst(inst);
    odb::dbIntProperty::create(inst, "inst_id", id);
    ++id;
  }
  if (id != 0) {
    avgK = totPins / block->getInsts().size();
  }
  bool flag = true;
  while (flag) {
    flag = partitionCluster(triton_part, modMgr, cv);
  }

  if (!block->getInsts().empty() && !block->getBTerms().empty()) {
    auto m = std::make_shared<Module>(modMgr.getNumModules());
    m->setAvgK(avgK);
    m->setAvgInsts(block->getInsts().size());
    m->setAvgT(block->getBTerms().size());
    m->setSigmaT(0.0);
    modMgr.addModule(m);
    linCurvFit(modMgr, Rratio, p, q);
  }
}

bool PartitionMgr::partitionCluster(
    const std::shared_ptr<TritonPart>& triton_part,
    ModuleMgr& modMgr,
    SharedClusterVector& cv)
{
  int MIN_GATE_NUM_PER_CLUSTER = 100;
  bool flag = true;
  SharedClusterVector resultCV;
  int clusterNum = cv.size();
  double sampleNum = (double) clusterNum * 2.0;
  double expo = 1.0 / sampleNum;
  double avgInsts = 0;
  double avgT = 0;
  int count = 0;
  std::vector<double> Tvect;
  std::vector<bool> inside;
  int numInsts = getDbBlock()->getInsts().size();

  for (int i = 0; i < clusterNum; ++i) {
    auto c = cv[i];
    resultCV.clear();
    Partitioning(triton_part, c, resultCV);
    cv.push_back(resultCV[0]);
    cv.push_back(resultCV[1]);

    for (int j = 0; j < 2; ++j) {
      const auto& newC = resultCV[j];
      int newGateNum = newC->getNumInsts();
      if (newGateNum < MIN_GATE_NUM_PER_CLUSTER) {
        flag = false;
      }

      inside.assign(numInsts, false);
      for (int k = 0; k < newGateNum; ++k) {
        auto inst = newC->getInst(k);
        auto inst_prop = odb::dbIntProperty::find(inst, "inst_id");
        const int inst_id = inst_prop->getValue();
        inside[inst_id] = true;
      }

      double sumT = getClusterIONum(inside, newC);
      double numInsts = newC->getNumInsts();
      if (numInsts >= 1.0 && sumT >= 1.0) {
        avgInsts += numInsts * expo;
        avgT += sumT * expo;
        Tvect.push_back(sumT);
        count++;
      }
    }
  }

  double stdevT = 0.0;

  if (Tvect.size() > 1) {
    double sumSqrtT = 0.0;
    for (const double t : Tvect) {
      sumSqrtT += pow((t - avgT), 2);
    }
    stdevT = sqrt(sumSqrtT / count);
  }

  if (avgInsts >= 1 && avgT >= 1) {
    auto m = std::make_shared<Module>(modMgr.getNumModules());
    m->setAvgInsts(avgInsts);
    m->setAvgT(avgT);
    m->setSigmaT(stdevT);
    modMgr.addModule(m);
  }

  // erase the first clusterNum elements
  cv.erase(cv.begin(), cv.begin() + clusterNum);

  return flag;
}

void PartitionMgr::Partitioning(const std::shared_ptr<TritonPart>& triton_part,
                                const std::shared_ptr<Cluster>& cluster,
                                SharedClusterVector& resultCV)
{
  std::vector<odb::dbInst*> insts;
  insts.reserve(cluster->getNumInsts());
  std::map<odb::dbInst*, int> inst_vertex_id_map;
  std::vector<float> vertex_weight;
  int vertex_id = 0;
  int large_net_threshold = 50;
  std::vector<bool> inside;
  inside.assign(getDbBlock()->getInsts().size(), false);
  std::unordered_set<odb::dbNet*> cluster_nets;

  const int num_insts = cluster->getNumInsts();
  insts.reserve(num_insts);
  vertex_weight.reserve(num_insts);
  cluster_nets.reserve(num_insts);

  for (odb::dbInst* inst : cluster->getInsts()) {
    inst_vertex_id_map[inst] = vertex_id++;
    vertex_weight.push_back(1.0f);
    auto inst_prop = odb::dbIntProperty::find(inst, "inst_id");
    const int inst_id = inst_prop->getValue();
    inside[inst_id] = true;
    insts.push_back(inst);

    for (odb::dbITerm* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (net != nullptr && !net->getSigType().isSupply()) {
        cluster_nets.insert(net);
      }
    }
  }

  std::vector<std::vector<int>> hyperedges;
  hyperedges.reserve(cluster_nets.size());
  // Make the iteration order stable
  std::vector<odb::dbNet*> cluster_nets_sorted(cluster_nets.begin(),
                                               cluster_nets.end());
  std::ranges::sort(cluster_nets_sorted);
  /*
  std::sort(
      cluster_nets_sorted.begin(),
      cluster_nets_sorted.end(),
      [](odb::dbNet* a, odb::dbNet* b) { return a->getName() < b->getName(); });
  */
  for (odb::dbNet* net : cluster_nets_sorted) {
    int driver_id = -1;
    std::set<int> loads_id;
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      auto inst_prop = odb::dbIntProperty::find(inst, "inst_id");
      const int inst_id = inst_prop->getValue();
      if (inside[inst_id]) {
        int vertex_id = inst_vertex_id_map[inst];
        if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
          driver_id = vertex_id;
        } else {
          loads_id.insert(vertex_id);
        }
      }
    }
    loads_id.insert(driver_id);
    if (driver_id != -1 && loads_id.size() > 1
        && loads_id.size() < large_net_threshold) {
      std::vector<int> hyperedge;
      hyperedge.insert(hyperedge.end(), loads_id.begin(), loads_id.end());
      hyperedges.push_back(hyperedge);
    }
  }

  const int seed = 0;
  constexpr float default_balance_constraint = 0.5f;
  float balance_constraint = default_balance_constraint;
  const int num_parts = 2;  // We use two-way partitioning here
  const int num_vertices = static_cast<int>(vertex_weight.size());
  std::vector<float> hyperedge_weights(hyperedges.size(), 1.0f);

  std::vector<int> solution;
  triton_part->SetFineTuneParams(  // coarsening related parameters
      200,     // thr_coarsen_hyperedge_size_skip (default: 200)
      10,      // thr_coarsen_vertices (default: 10)
      50,      // thr_coarsen_hyperedges (default: 50)
      1.6,     // coarsening_ratio (default: 1.6)
      30,      // max_coarsen_iters (default: 30)
      0.0001,  // adj_diff_ratio (default: 0.0001)
      4,       // min_num_vertices_each_part (default: 4)
      // initial partitioning related parameters
      15,  // num_initial_solutions(default: 50)
      3,   // num_best_initial_solutions (default: 10)
      // refinement related parameters
      7,    // refiner_iters (default: 10)
      50,   // max_moves (default: 60)
      0.5,  // early_stop_ratio (default: 0.5)
      25,   // total_corking_passes (default: 25)
      // vcycle related parameters
      true,   // v_cycle_flag (default: true)
      1,      // max_num_vcycle (default: 1)
      3,      // num_coarsen_solutions (default: 3)
      0,      // num_vertices_threshold_ilp (default: 50)
      1000);  // global_net_threshold (default: 1000)

  solution = triton_part->PartitionKWaySimpleMode(num_parts,
                                                  balance_constraint,
                                                  seed,
                                                  hyperedges,
                                                  vertex_weight,
                                                  hyperedge_weights);

  cluster->clearInsts();
  auto cluster_part_1 = std::make_shared<Cluster>(0);

  for (int i = 0; i < num_vertices; i++) {
    odb::dbInst* inst = insts[i];
    if (solution[i] == 0) {
      cluster->addInst(inst);
    } else {
      cluster_part_1->addInst(inst);
    }
  }

  resultCV.push_back(cluster_part_1);
  resultCV.push_back(cluster);
}

int PartitionMgr::getClusterIONum(std::vector<bool>& inside,
                                  const std::shared_ptr<Cluster>& cluster)
{
  std::vector<odb::dbInst*> cInsts = cluster->getInsts();
  std::unordered_set<odb::dbNet*> cNets;

  for (odb::dbInst* inst : cInsts) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (!net || net->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      cNets.insert(net);
    }
  }

  int terms = 0;
  for (odb::dbNet* net : cNets) {
    if (!net) {
      continue;
    }

    if (!net->getBTerms().empty()) {
      terms++;
      continue;
    }

    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      if (!inst) {
        continue;
      }

      auto* prop = odb::dbIntProperty::find(inst, "inst_id");
      int inst_id = prop->getValue();
      if (!inside[inst_id]) {
        terms++;
        break;
      }
    }
  }

  return terms;
}

// from RentCon
void PartitionMgr::linCurvFit(ModuleMgr& modMgr,
                              float& Rratio,
                              float& p,
                              float& q)
{
  const int n = modMgr.getNumModules();
  double* x = new double[n];
  double* y = new double[n];

  auto modules = modMgr.getModules();
  std::ranges::sort(
      modules,

      [](const std::shared_ptr<Module>& m1, const std::shared_ptr<Module>& m2) {
        return std::make_tuple(m1->getAvgInsts(), m1->getId())
               < std::make_tuple(m2->getAvgInsts(), m2->getId());
      });

  const double b = log(modules[n - 1]->getAvgK());
  for (int i = 0; i < n; i++) {
    const auto& m = modules[i];
    x[i] = log(m->getAvgInsts());
    y[i] = log(m->getAvgT()) - b;
  }

  auto [ratio, rentP, std_dev] = fitRent(x, y, n);
  delete[] x;
  delete[] y;
  Rratio = ratio;
  p = rentP;
  q = std_dev;
}

// from RentCon
std::tuple<double, double, double> PartitionMgr::fitRent(const double* x,
                                                         const double* y,
                                                         int n)
{
  const int minPntNum = (int) (n * 0.75);
  const int totPoints = n;
  int bestN = n;
  double rentP;
  double cov11;
  double sumsq;

  fit_mul(x, 1, y, 1, n, &rentP, &cov11, &sumsq);
  double bestRent = rentP;

  double oldDev = sqrt(sumsq / n);

  while (n > minPntNum) {
    n--;
    fit_mul(x, 1, y, 1, n, &rentP, &cov11, &sumsq);
    // compute the standard deviation of the residuals
    const double newDev = sqrt(sumsq / n);
    if (newDev > oldDev) {
      break;
    }
    oldDev = newDev;
    bestN = n;
    bestRent = rentP;
  }
  double Rratio;
  if (bestN == totPoints) {
    Rratio = 0.9;
  } else {
    Rratio = pow(2, bestN - totPoints);
  }
  return std::make_tuple(Rratio, bestRent, oldDev);
}

// from gsl library
void PartitionMgr::fit_mul(const double* x,
                           const size_t xstride,
                           const double* y,
                           const size_t ystride,
                           const size_t n,
                           double* c1,
                           double* cov_11,
                           double* sumsq)
{
  double m_x = 0;
  double m_y = 0;
  double m_dx2 = 0;
  double m_dxdy = 0;
  for (size_t i = 0; i < n; i++) {
    m_x += (x[i * xstride] - m_x) / (i + 1.0);
    m_y += (y[i * ystride] - m_y) / (i + 1.0);
  }
  for (size_t i = 0; i < n; i++) {
    const double dx = x[i * xstride] - m_x;
    const double dy = y[i * ystride] - m_y;
    m_dx2 += (dx * dx - m_dx2) / (i + 1.0);
    m_dxdy += (dx * dy - m_dxdy) / (i + 1.0);
  }
  /* In terms of y =  b x */
  {
    double d2 = 0;
    const double b = (m_x * m_y + m_dxdy) / (m_x * m_x + m_dx2);
    *c1 = b;
    /* Compute chi^2 = \sum (y_i -  b * x_i)^2 */
    for (size_t i = 0; i < n; i++) {
      const double dx = x[i * xstride] - m_x;
      const double dy = y[i * ystride] - m_y;
      const double d = (m_y - b * m_x) + dy - b * dx;
      d2 += d * d;
    }
    const double s2 = d2 / (n - 1.0); /* chisq per degree of freedom */
    *cov_11 = s2 * 1.0 / (n * (m_x * m_x + m_dx2));
    *sumsq = d2;
  }
}

void PartitionMgr::writeFile(
    const std::map<std::string, MasterInfo>& onlyUseMasters,
    const std::string& top_name,
    const int numInsts,
    const int numMacros,
    const int numPIs,
    const int numPOs,
    const int numSeq,
    const int Dmax,
    const int MDmax,
    const float Rratio,
    const float p,
    const float q,
    const float avgK,
    const char* fileName)
{
  std::ofstream outFile(fileName);
  if (!outFile.good()) {
    logger_->error(PAR, 54, "Cannot open file");
    exit(0);
  }

  outFile << "LIBRARY\n";
  outFile << "NAME lib\n";

  // map<string, MasterInfo> --> cellName / cellCount, isMacro
  for (const auto& [name, info] : onlyUseMasters) {
    outFile << "ONLY_USE " << name << '\n';
  }
  outFile << '\n';

  outFile << "CIRCUIT\n";
  outFile << "NAME " << top_name << '\n';
  outFile << "LIBRARIES lib" << '\n';
  outFile << "DISTRIBUTION ";
  for (const auto& [name, info] : onlyUseMasters) {
    outFile << info.count << " ";
  }
  outFile << '\n';
  outFile << "SIZE " << int(numInsts * Rratio) << '\n';
  outFile << "p " << p << '\n';
  outFile << "q " << q << '\n';
  outFile << "END\n";
  outFile << "SIZE " << numInsts << '\n';
  outFile << "I " << numPIs << '\n';
  outFile << "O " << numPOs << '\n';
  outFile << "END\n";
  outFile.close();
  float Sratio = 0.0;
  if (numInsts > 0) {
    Sratio = float(numSeq) / numInsts;
  }
  logger_->report("#instances: {}", numInsts);
  logger_->report("#macros: {}", numMacros);
  logger_->report("#primary inputs: {}", numPIs);
  logger_->report("#primary outputs: {}", numPOs);
  logger_->report("Ratio of Region I: {}", Rratio);
  logger_->report("Rent's exponent: {}", p);
  logger_->report("Average #pins per instances: {}", avgK);
  logger_->report("Ratio of #sequential instances to the #instances: {}",
                  Sratio);
  logger_->report("Maximum depth of any timing path: {}", Dmax);
  logger_->report("Maximum depth of macro path: {}", MDmax);
}

}  // namespace par
