// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"

namespace odb {
class Rect;
class dbInst;
class dbModule;
class dbDatabase;
class dbITerm;
class dbTechLayer;
class dbBox;
class dbTrackGrid;
}  // namespace odb

namespace utl {
class Logger;
}

namespace par {

class Cluster;
using UniqueClusterVector = std::vector<std::unique_ptr<Cluster>>;

class Module;
using SharedModuleVector = std::vector<std::shared_ptr<Module>>;

class Module
{
 public:
  // constructors
  Module(int i) : id_(i) {}

  // modifiers
  void setId(int i) { id_ = i; }
  void setAvgK(double a_k) { avg_k_ = a_k; }
  void setAvgInsts(double a_in) { avg_insts_ = a_in; }
  void setAvgT(double a_t) { avg_t_ = a_t; }
  void setInT(double in_t) { avg_in_t_ = in_t; }
  void setOutT(double out_t) { avg_out_t_ = out_t; }
  void setSigmaT(double sigma_t) { sigma_t_ = sigma_t; }

  // accossers
  int getId() { return id_; }
  double getAvgK() { return avg_k_; }
  double getAvgInsts() { return avg_insts_; }
  double getAvgT() { return avg_t_; }
  double getInT() { return avg_in_t_; }
  double getOutT() { return avg_out_t_; }
  double getSigmaT() { return sigma_t_; }

 private:
  int id_;
  double avg_k_ = 0.0;
  double avg_insts_ = 0.0;
  double avg_t_ = 0.0;
  double avg_in_t_ = 0.0;
  double avg_out_t_ = 0.0;
  double sigma_t_ = 0.0;
};

struct ModuleMgr
{
  std::vector<std::shared_ptr<Module>> modules;

  void addModule(std::shared_ptr<Module> module)
  {
    modules.push_back(std::move(module));
  }

  int getNumModules() const { return modules.size(); }

  SharedModuleVector getModules() const { return modules; }
};

class Cluster
{
 public:
  Cluster(int i) : id_(i) {}
  ~Cluster() { insts_.clear(); }

  int getId() const { return id_; }

  const std::vector<odb::dbInst*>& getInsts() const { return insts_; }

  odb::dbInst* getInst(const int i)
  {
    if (i < insts_.size()) {
      return insts_[i];
    }
    return nullptr;
  }

  void addInst(odb::dbInst* inst) { insts_.push_back(inst); }

  int getNumInsts() const { return insts_.size(); }

  void clearInsts() { insts_.clear(); }

 private:
  int id_ = -1;
  std::vector<odb::dbInst*> insts_;
  static int next_id_;
};

}  // namespace par
