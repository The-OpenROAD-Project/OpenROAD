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
  Module(int i) : id(i) {}

  // modifiers
  void setId(int i) { id = i; }
  void setAvgK(double aK) { avgK = aK; }
  void setAvgInsts(double aIn) { avgInsts = aIn; }
  void setAvgT(double aT) { avgT = aT; }
  void setInT(double inT) { avgInT = inT; }
  void setOutT(double outT) { avgOutT = outT; }
  void setSigmaT(double sigmaT) { SigmaT = sigmaT; }

  // accossers
  int getId() { return id; }
  double getAvgK() { return avgK; }
  double getAvgInsts() { return avgInsts; }
  double getAvgT() { return avgT; }
  double getInT() { return avgInT; }
  double getOutT() { return avgOutT; }
  double getSigmaT() { return SigmaT; }

 private:
  int id;
  double avgK = 0.0;
  double avgInsts = 0.0;
  double avgT = 0.0;
  double avgInT = 0.0;
  double avgOutT = 0.0;
  double SigmaT = 0.0;
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
