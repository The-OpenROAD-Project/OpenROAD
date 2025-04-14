// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

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
        paintInGrid(grid_, node);
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
        paintInGrid(grid_, node);
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
