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

#include "journal.h"

#include "detailed_manager.h"
#include "dpl/Grid.h"

namespace dpo {
void Journal::clearJournal()
{
  actions_.clear();
  affected_nodes_.clear();
}
namespace {
void paintInGrid(Grid* grid, Node* node)
{
  const auto grid_x = grid->gridX(DbuX(node->getLeft()));
  const auto grid_y = grid->gridRoundY(DbuY(node->getBottom()));
  auto pixel = grid->gridPixel(grid_x, grid_y);
  grid->paintPixel(node, grid_x, grid_y);
  node->adjustCurrOrient(
      pixel->sites.at(node->getDbInst()->getMaster()->getSite()));
}

};  // namespace
void Journal::undo(const JournalAction& action, const bool positions_only) const
{
  auto node = action.getNode();
  switch (action.getType()) {
    case JournalAction::MOVE_CELL:
      if (!positions_only) {
        grid_->erasePixel(node);
        for (auto seg : action.getNewSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->removeCellFromSegment(node, seg);
        }
      }
      node->setLeft(DbuX{action.getOrigLeft()});
      node->setBottom(action.getOrigBottom());
      if (!positions_only) {
        mgr_->paintInGrid(node);
        for (auto seg : action.getOrigSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->addCellToSegment(node, seg);
        }
      }
      break;
  }
}
////////////////////////////////////////////////////////////////////////////////
void Journal::redo(const JournalAction& action, const bool positions_only) const
{
  auto node = action.getNode();
  switch (action.getType()) {
    case JournalAction::MOVE_CELL:
      if (!positions_only) {
        grid_->erasePixel(node);
        for (auto seg : action.getOrigSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->removeCellFromSegment(node, seg);
        }
      }
      node->setLeft(DbuX{action.getNewLeft()});
      node->setBottom(action.getNewBottom());
      if (!positions_only) {
        mgr_->paintInGrid(node);
        for (auto seg : action.getNewSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->addCellToSegment(node, seg);
        }
      }
      break;
  }
}

}  // namespace dpo