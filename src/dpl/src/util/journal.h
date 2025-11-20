// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <set>
#include <vector>

#include "dpl/Opendp.h"
#include "infrastructure/network.h"
namespace dpl {
class Grid;

class DetailedMgr;
enum class JournalActionTypeEnum
{
  MOVE_CELL,
  UNPLACE_CELL
};
class JournalAction
{
 public:
  virtual ~JournalAction() = default;
  virtual JournalActionTypeEnum typeId() const = 0;
};
class MoveCellAction : public JournalAction
{
 public:
  MoveCellAction(Node* node,
                 const DbuX orig_x,
                 const DbuY orig_y,
                 const DbuX new_x,
                 const DbuY new_y,
                 const bool was_placed,
                 const std::vector<int>& orig_segs = {},
                 const std::vector<int>& new_segs = {})
      : node_(node),
        orig_x_(orig_x),
        orig_y_(orig_y),
        new_x_(new_x),
        new_y_(new_y),
        was_placed_(was_placed),
        orig_segs_(orig_segs),
        new_segs_(new_segs)
  {
  }
  // getters
  Node* getNode() const { return node_; }
  DbuX getOrigLeft() const { return orig_x_; }
  DbuY getOrigBottom() const { return orig_y_; }
  DbuX getNewLeft() const { return new_x_; }
  DbuY getNewBottom() const { return new_y_; }
  const std::vector<int>& getOrigSegs() const { return orig_segs_; }
  const std::vector<int>& getNewSegs() const { return new_segs_; }
  bool wasPlaced() const { return was_placed_; }
  JournalActionTypeEnum typeId() const override
  {
    return JournalActionTypeEnum::MOVE_CELL;
  }

 private:
  Node* node_;
  const DbuX orig_x_;
  const DbuY orig_y_;
  const DbuX new_x_;
  const DbuY new_y_;
  const bool was_placed_;
  const std::vector<int> orig_segs_;
  const std::vector<int> new_segs_;
};
class UnplaceCellAction : public JournalAction
{
 public:
  UnplaceCellAction(Node* node, const bool was_hold)
      : node_(node), was_hold_(was_hold)
  {
  }
  Node* getNode() const { return node_; }
  bool wasHold() const { return was_hold_; }
  JournalActionTypeEnum typeId() const override
  {
    return JournalActionTypeEnum::UNPLACE_CELL;
  }

 private:
  Node* node_;
  const bool was_hold_;
};
class Journal
{
 public:
  Journal(Grid* grid, DetailedMgr* mgr) : grid_(grid), mgr_(mgr) {}
  // setters
  void addAction(const MoveCellAction& action)
  {
    affected_nodes_.insert(action.getNode());
    for (auto pin : action.getNode()->getPins()) {
      affected_edges_.insert(pin->getEdge());
    }
    actions_.push_back(std::make_unique<MoveCellAction>(action));
  }
  void addAction(const UnplaceCellAction& action)
  {
    affected_nodes_.insert(action.getNode());
    for (auto pin : action.getNode()->getPins()) {
      affected_edges_.insert(pin->getEdge());
    }
    actions_.push_back(std::make_unique<UnplaceCellAction>(action));
  }
  // getters
  bool empty() const { return actions_.empty(); }
  size_t size() const { return actions_.size(); }
  const std::set<Node*>& getAffectedNodes() const { return affected_nodes_; }
  const std::set<Edge*>& getAffectedEdges() const { return affected_edges_; }
  // iterator support for range-based for loops
  auto begin() const { return actions_.begin(); }
  auto end() const { return actions_.end(); }
  // other
  void clear();
  void undo(bool positions_only = false) const;
  void redo(bool positions_only = false) const;

 private:
  JournalAction* getLastAction() const { return actions_.back().get(); }
  const std::vector<std::unique_ptr<JournalAction>>& getActions() const
  {
    return actions_;
  }
  void removeLastAction() { actions_.pop_back(); }
  void undo(const JournalAction* action, bool positions_only = false) const;
  void redo(const JournalAction* action, bool positions_only = false) const;

  Grid* grid_{nullptr};
  DetailedMgr* mgr_{nullptr};
  std::vector<std::unique_ptr<JournalAction>> actions_;
  std::set<Node*> affected_nodes_;
  std::set<Edge*> affected_edges_;
};

}  // namespace dpl
