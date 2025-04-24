// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <set>
#include <vector>

#include "infrastructure/network.h"
namespace dpl {
class Grid;

class DetailedMgr;

class JournalAction
{
 public:
  enum TYPE
  {
    MOVE_CELL
  };
  JournalAction() = default;
  void setOrigLocation(const DbuX x, const DbuY y)
  {
    orig_x_ = x;
    orig_y_ = y;
  }
  void setNewLocation(const DbuX x, const DbuY y)
  {
    new_x_ = x;
    new_y_ = y;
  }
  void setOrigSegs(const std::vector<int>& segs) { orig_segs_ = segs; }
  void setNewSegs(const std::vector<int>& segs) { new_segs_ = segs; }
  void setNode(Node* node) { node_ = node; }
  void setType(TYPE type) { type_ = type; }
  // getters
  Node* getNode() const { return node_; }
  DbuX getOrigLeft() const { return orig_x_; }
  DbuY getOrigBottom() const { return orig_y_; }
  DbuX getNewLeft() const { return new_x_; }
  DbuY getNewBottom() const { return new_y_; }
  const std::vector<int>& getOrigSegs() const { return orig_segs_; }
  const std::vector<int>& getNewSegs() const { return new_segs_; }
  TYPE getType() const { return type_; }

 private:
  TYPE type_;
  Node* node_{nullptr};
  DbuX orig_x_{0};
  DbuY orig_y_{0};
  DbuX new_x_{0};
  DbuY new_y_{0};
  std::vector<int> orig_segs_;
  std::vector<int> new_segs_;
};
class Journal
{
 public:
  Journal(Grid* grid, DetailedMgr* mgr) : grid_(grid), mgr_(mgr) {}
  // setters
  void addAction(const JournalAction& action)
  {
    actions_.push_back(action);
    affected_nodes_.insert(action.getNode());
  }
  // getters
  const JournalAction& getLastAction() const { return actions_.back(); }
  bool isEmpty() const { return actions_.empty(); }
  size_t size() const { return actions_.size(); }
  const std::set<Node*>& getAffectedNodes() const { return affected_nodes_; }
  const std::vector<JournalAction>& getActions() const { return actions_; }
  // other
  void removeLastAction() { actions_.pop_back(); }
  void clearJournal();
  void undo(const JournalAction& action, bool positions_only = false) const;
  void redo(const JournalAction& action, bool positions_only = false) const;

 private:
  Grid* grid_{nullptr};
  DetailedMgr* mgr_{nullptr};
  std::vector<JournalAction> actions_;
  std::set<Node*> affected_nodes_;
};

}  // namespace dpl
