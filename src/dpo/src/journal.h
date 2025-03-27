//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2025, Precision Innovations Inc.
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
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <set>

#include "network.h"
#pragma once
namespace dpo {
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
  Journal() = default;
  void addAction(const JournalAction& action)
  {
    actions_.push_back(action);
    affected_nodes_.insert(action.getNode());
  }
  const JournalAction& getLastAction() const { return actions_.back(); }
  void removeLastAction() { actions_.pop_back(); }
  bool isEmpty() const { return actions_.empty(); }
  void clearJournal();
  const std::set<Node*>& getAffectedNodes() const { return affected_nodes_; }
  const std::vector<JournalAction>& getActions() const { return actions_; }
  size_t size() const { return actions_.size(); }

 private:
  std::vector<JournalAction> actions_;
  std::set<Node*> affected_nodes_;
};

}  // namespace dpo