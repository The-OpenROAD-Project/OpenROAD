// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <map>
#include <set>
#include <utility>

#include "odb/db.h"
#include "shape.h"

namespace utl {
class Logger;
}

namespace pdn {

class ViaRepair
{
  using ViaValue = std::pair<odb::Rect, odb::dbSBox*>;
  using ViaTree = bgi::rtree<ViaValue, bgi::quadratic<16>>;
  using LayerViaTree = std::map<odb::dbTechLayer*, ViaTree>;

 public:
  ViaRepair(utl::Logger* logger, const std::set<odb::dbNet*>& nets);

  void repair();

  void report() const;

 private:
  utl::Logger* logger_;
  std::set<odb::dbNet*> nets_;

  bool use_obs_ = true;
  bool use_nets_ = true;
  bool use_inst_ = true;

  std::map<odb::dbTechLayer*, int> via_count_;
  std::map<odb::dbTechLayer*, int> removal_count_;

  LayerViaTree collectVias();

  using ObsRect = std::map<odb::dbTechLayer*, std::set<odb::Rect>>;

  ObsRect collectBlockObstructions(odb::dbBlock* block);
  ObsRect collectInstanceObstructions(odb::dbBlock* block);
  ObsRect collectNetObstructions(odb::dbBlock* block);
};

}  // namespace pdn
