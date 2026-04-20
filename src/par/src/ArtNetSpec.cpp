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
#include "sta/SdcClass.hh"
#include "sta/Search.hh"
#include "sta/SearchClass.hh"
#include "sta/SearchPred.hh"
#include "sta/Sequential.hh"
#include "sta/Sta.hh"
#include "sta/StringUtil.hh"
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

void PartitionMgr::writeArtNetSpec(const char* file_name)
{
  if (!getDbBlock()) {
    logger_->error(PAR, 53, "Design not loaded.");
  }
  if (db_->getLibs().empty()) {
    logger_->error(PAR, 55, "Libs not loaded.");
  }

  std::map<std::string, MasterInfo> only_use_masters;
  std::string top_name;
  int num_insts = 0;
  int num_macros = 0;
  int num_pi = 0;
  int num_po = 0;
  int num_seq = 0;
  int max_flop_depth = -1;
  int max_macro_depth = -1;
  float r_ratio = 0.0;
  float p = 0.0;
  float q = 0.0;
  float avg_k = 0.0;

  getFromODB(only_use_masters,
             top_name,
             num_insts,
             num_macros,
             num_pi,
             num_po,
             num_seq);
  logger_->report("getFromODB done");
  getFromSTA(max_flop_depth, max_macro_depth);
  logger_->report("getFromSTA done");
  getFromPAR(r_ratio, p, q, avg_k);
  logger_->report("getFromPAR done");
  writeFile(only_use_masters,
            top_name,
            num_insts,
            num_macros,
            num_pi,
            num_po,
            num_seq,
            max_flop_depth,
            max_macro_depth,
            r_ratio,
            p,
            q,
            avg_k,
            file_name);
}

void PartitionMgr::getFromODB(
    std::map<std::string, MasterInfo>& only_use_masters,
    std::string& top_name,
    int& num_insts,
    int& num_macros,
    int& num_pi,
    int& num_po,
    int& num_seq)
{
  auto block = getDbBlock();
  odb::dbSet<dbInst> insts = block->getInsts();
  odb::dbSet<dbBTerm> bterms = block->getBTerms();
  num_insts = insts.size();

  for (auto bterm : bterms) {
    if (bterm->getIoType() == odb::dbIoType::INPUT) {
      num_pi++;
    }
    if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
      num_po++;
    }
  }

  for (auto inst : insts) {
    dbMaster* master = inst->getMaster();
    bool is_macro = (master->getType() == dbMasterType::BLOCK ? 1 : 0);
    const sta::LibertyCell* lib_cell = db_network_->libertyCell(inst);
    if (!lib_cell) {
      logger_->error(PAR, 56, "Liberty cell not found: {}", inst->getName());
    }
    if (lib_cell->hasSequentials()) {
      num_seq++;
    }
    auto [it, inserted]
        = only_use_masters.try_emplace(master->getName(), MasterInfo{});
    MasterInfo& info = it->second;
    ++info.count;
    if (inserted) {
      info.is_macro = is_macro;
    }
    if (is_macro) {
      num_macros++;
    }
  }
}

void PartitionMgr::getFromSTA(int& max_flop_depth, int& max_macro_depth)
{
  BuildTimingPath(max_flop_depth, max_macro_depth);
}

void PartitionMgr::BuildTimingPath(int& max_flop_depth, int& max_macro_depth)
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
  sta::StringSeq group_names_empty;
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
  std::map<std::string, int> path_depth_map;
  // check all the timing paths
  for (auto& path_end : path_ends) {
    // Printing timing paths to logger
    // sta_->reportPathEnd(path_end);
    auto* path = path_end->path();

    int depth = 0;
    std::string end_point_name;
    std::unordered_set<std::string> visited_instances;
    std::unordered_set<std::string> visited_bterms;

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

      if (db_network_->isTopLevelPort(pin)) {
        auto bterm = block->findBTerm(db_network_->pathName(pin).c_str());
        name = bterm->getName();
        if (visited_bterms.insert(name).second) {
          depth++;
        }
      } else {
        auto inst = db_network_->instance(pin);
        auto db_inst = block->findInst(db_network_->pathName(inst).c_str());
        name = db_inst->getName();
        if (visited_instances.insert(name).second) {
          depth++;
        }
      }

      if (i == expand.size() - 1) {
        end_point_name = name;
        path_depth_map[end_point_name] = depth;
      }
    }
  }  // path_end

  int ff_max = 0;
  int mac_max = 0;
  for (const auto& path : path_depth_map) {
    auto inst = block->findInst((path.first).c_str());
    if (inst) {
      if (inst->getMaster()->isBlock()) {
        mac_max = std::max(path.second, mac_max);
      } else {
        ff_max = std::max(path.second, ff_max);
      }
    }
  }
  max_flop_depth = ff_max;
  max_macro_depth = mac_max;
}

void PartitionMgr::getFromPAR(float& r_ratio, float& p, float& q, float& avg_k)
{
  getRents(r_ratio, p, q, avg_k);
}

void PartitionMgr::getRents(float& r_ratio, float& p, float& q, float& avg_k)
{
  auto block = getDbBlock();
  ModuleMgr mod_mgr;
  SharedClusterVector cv;
  auto c = std::make_shared<Cluster>(0);
  cv.push_back(c);
  auto triton_part
      = std::make_shared<TritonPart>(db_network_, db_, sta_, logger_);
  double tot_pins = 0;
  int id = 0;
  for (dbInst* inst : block->getInsts()) {
    for (dbITerm* inst_iterm : inst->getITerms()) {
      if (inst_iterm->getIoType() == dbIoType::INPUT
          || inst_iterm->getIoType() == dbIoType::OUTPUT) {
        tot_pins++;
      }
    }
    c->addInst(inst);
    odb::dbIntProperty::create(inst, "inst_id", id);
    ++id;
  }
  if (id != 0) {
    avg_k = tot_pins / block->getInsts().size();
  }
  bool flag = true;
  while (flag) {
    flag = partitionCluster(triton_part, mod_mgr, cv);
  }

  if (!block->getInsts().empty() && !block->getBTerms().empty()) {
    auto m = std::make_shared<Module>(mod_mgr.getNumModules());
    m->setAvgK(avg_k);
    m->setAvgInsts(block->getInsts().size());
    m->setAvgT(block->getBTerms().size());
    m->setSigmaT(0.0);
    mod_mgr.addModule(m);
    linCurvFit(mod_mgr, r_ratio, p, q);
  }
}

bool PartitionMgr::partitionCluster(
    const std::shared_ptr<TritonPart>& triton_part,
    ModuleMgr& mod_mgr,
    SharedClusterVector& cv)
{
  int min_gate_num_per_cluster = 100;
  bool flag = true;
  SharedClusterVector result_cv;
  int cluster_num = cv.size();
  double sample_num = (double) cluster_num * 2.0;
  double expo = 1.0 / sample_num;
  double avg_insts = 0;
  double avg_t = 0;
  int count = 0;
  std::vector<double> tvect;
  std::vector<bool> inside;
  int num_insts = getDbBlock()->getInsts().size();

  for (int i = 0; i < cluster_num; ++i) {
    auto c = cv[i];
    result_cv.clear();
    Partitioning(triton_part, c, result_cv);
    cv.push_back(result_cv[0]);
    cv.push_back(result_cv[1]);

    for (int j = 0; j < 2; ++j) {
      const auto& new_c = result_cv[j];
      int new_gate_num = new_c->getNumInsts();
      if (new_gate_num < min_gate_num_per_cluster) {
        flag = false;
      }

      inside.assign(num_insts, false);
      for (int k = 0; k < new_gate_num; ++k) {
        auto inst = new_c->getInst(k);
        auto inst_prop = odb::dbIntProperty::find(inst, "inst_id");
        const int inst_id = inst_prop->getValue();
        inside[inst_id] = true;
      }

      double sum_t = getClusterIONum(inside, new_c);
      double num_insts = new_c->getNumInsts();
      if (num_insts >= 1.0 && sum_t >= 1.0) {
        avg_insts += num_insts * expo;
        avg_t += sum_t * expo;
        tvect.push_back(sum_t);
        count++;
      }
    }
  }

  double stdev_t = 0.0;

  if (tvect.size() > 1) {
    double sum_sqrt_t = 0.0;
    for (const double t : tvect) {
      sum_sqrt_t += pow((t - avg_t), 2);
    }
    stdev_t = sqrt(sum_sqrt_t / count);
  }

  if (avg_insts >= 1 && avg_t >= 1) {
    auto m = std::make_shared<Module>(mod_mgr.getNumModules());
    m->setAvgInsts(avg_insts);
    m->setAvgT(avg_t);
    m->setSigmaT(stdev_t);
    mod_mgr.addModule(m);
  }

  // erase the first clusterNum elements
  cv.erase(cv.begin(), cv.begin() + cluster_num);

  return flag;
}

void PartitionMgr::Partitioning(const std::shared_ptr<TritonPart>& triton_part,
                                const std::shared_ptr<Cluster>& cluster,
                                SharedClusterVector& result_cv)
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
  constexpr float kDefaultBalanceConstraint = 0.5f;
  float balance_constraint = kDefaultBalanceConstraint;
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

  result_cv.push_back(cluster_part_1);
  result_cv.push_back(cluster);
}

int PartitionMgr::getClusterIONum(std::vector<bool>& inside,
                                  const std::shared_ptr<Cluster>& cluster)
{
  std::vector<odb::dbInst*> c_insts = cluster->getInsts();
  std::unordered_set<odb::dbNet*> c_nets;

  for (odb::dbInst* inst : c_insts) {
    for (odb::dbITerm* iterm : inst->getITerms()) {
      odb::dbNet* net = iterm->getNet();
      if (!net || net->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      c_nets.insert(net);
    }
  }

  int terms = 0;
  for (odb::dbNet* net : c_nets) {
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
void PartitionMgr::linCurvFit(ModuleMgr& mod_mgr,
                              float& r_ratio,
                              float& p,
                              float& q)
{
  const int n = mod_mgr.getNumModules();
  double* x = new double[n];
  double* y = new double[n];

  auto modules = mod_mgr.getModules();
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
  r_ratio = ratio;
  p = rentP;
  q = std_dev;
}

// from RentCon
std::tuple<double, double, double> PartitionMgr::fitRent(const double* x,
                                                         const double* y,
                                                         int n)
{
  const int min_pnt_num = (int) (n * 0.75);
  const int tot_points = n;
  int best_n = n;
  double rent_p;
  double cov11;
  double sumsq;

  fit_mul(x, 1, y, 1, n, &rent_p, &cov11, &sumsq);
  double best_rent = rent_p;

  double old_dev = sqrt(sumsq / n);

  while (n > min_pnt_num) {
    n--;
    fit_mul(x, 1, y, 1, n, &rent_p, &cov11, &sumsq);
    // compute the standard deviation of the residuals
    const double new_dev = sqrt(sumsq / n);
    if (new_dev > old_dev) {
      break;
    }
    old_dev = new_dev;
    best_n = n;
    best_rent = rent_p;
  }
  double r_ratio;
  if (best_n == tot_points) {
    r_ratio = 0.9;
  } else {
    r_ratio = pow(2, best_n - tot_points);
  }
  return std::make_tuple(r_ratio, best_rent, old_dev);
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
    const std::map<std::string, MasterInfo>& only_use_masters,
    const std::string& top_name,
    const int num_insts,
    const int num_macros,
    const int num_pi,
    const int num_po,
    const int num_seq,
    const int max_flop_depth,
    const int max_macro_depth,
    const float r_ratio,
    const float p,
    const float q,
    const float avg_k,
    const char* file_name)
{
  std::ofstream out_file(file_name);
  if (!out_file.good()) {
    logger_->error(PAR, 54, "Cannot open file");
    exit(0);
  }

  out_file << "LIBRARY\n";
  out_file << "NAME lib\n";

  // map<string, MasterInfo> --> cellName / cellCount, isMacro
  for (const auto& [name, info] : only_use_masters) {
    out_file << "ONLY_USE " << name << '\n';
  }
  out_file << '\n';

  out_file << "CIRCUIT\n";
  out_file << "NAME " << top_name << '\n';
  out_file << "LIBRARIES lib" << '\n';
  out_file << "DISTRIBUTION ";
  for (const auto& [name, info] : only_use_masters) {
    out_file << info.count << " ";
  }
  out_file << '\n';
  out_file << "SIZE " << int(num_insts * r_ratio) << '\n';
  out_file << "p " << p << '\n';
  out_file << "q " << q << '\n';
  out_file << "END\n";
  out_file << "SIZE " << num_insts << '\n';
  out_file << "I " << num_pi << '\n';
  out_file << "O " << num_po << '\n';
  out_file << "END\n";
  out_file.close();
  float sratio = 0.0;
  if (num_insts > 0) {
    sratio = float(num_seq) / num_insts;
  }
  logger_->report("#instances: {}", num_insts);
  logger_->report("#macros: {}", num_macros);
  logger_->report("#primary inputs: {}", num_pi);
  logger_->report("#primary outputs: {}", num_po);
  logger_->report("Ratio of Region I: {}", r_ratio);
  logger_->report("Rent's exponent: {}", p);
  logger_->report("Average #pins per instances: {}", avg_k);
  logger_->report("Ratio of #sequential instances to the #instances: {}",
                  sratio);
  logger_->report("Maximum depth of any timing path: {}", max_flop_depth);
  logger_->report("Maximum depth of macro path: {}", max_macro_depth);
}

}  // namespace par
