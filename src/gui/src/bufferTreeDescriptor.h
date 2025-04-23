// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <set>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "odb/db.h"

namespace utl {
class Logger;
}

namespace sta {
class dbSta;
}

namespace gui {

class BufferTree
{
 public:
  explicit BufferTree(odb::dbNet* net);

  static void setSTA(sta::dbSta* sta) { sta_ = sta; }
  static bool isAggregate(odb::dbNet* net);
  static bool isAggregate(odb::dbInst* inst);

  const std::string& getName() const { return name_; }
  const std::vector<odb::dbNet*>& getNets() const { return nets_; }
  const std::set<odb::dbInst*>& getInsts() const { return insts_; }
  const std::set<odb::dbITerm*>& getITerms() const { return iterm_terms_; }
  const std::set<odb::dbBTerm*>& getBTerms() const { return bterm_terms_; }

 private:
  // Use vector for nets since painting for order matters
  std::vector<odb::dbNet*> nets_;
  std::set<odb::dbInst*> insts_;
  std::set<odb::dbITerm*> iterm_terms_;
  std::set<odb::dbBTerm*> bterm_terms_;
  std::string name_;

  static sta::dbSta* sta_;

  void populate(odb::dbNet* net);
};

class BufferTreeDescriptor : public Descriptor
{
 public:
  BufferTreeDescriptor(odb::dbDatabase* db,
                       sta::dbSta* sta,
                       const std::set<odb::dbNet*>& focus_nets,
                       const std::set<odb::dbNet*>& guide_nets,
                       const std::set<odb::dbNet*>& tracks_nets);

  std::string getName(std::any object) const override;
  std::string getTypeName() const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object, Painter& painter) const override;

  Properties getProperties(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object) const override;
  bool lessThan(std::any l, std::any r) const override;

  bool getAllObjects(SelectionSet& objects) const override;

 private:
  odb::dbDatabase* db_;
  const Descriptor* net_descriptor_;
  const std::set<odb::dbNet*>& focus_nets_;
  const std::set<odb::dbNet*>& guide_nets_;
  const std::set<odb::dbNet*>& tracks_nets_;
};

}  // namespace gui
