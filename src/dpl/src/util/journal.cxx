// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "journal.h"

#include "infrastructure/Grid.h"
#include "optimization/detailed_manager.h"

namespace dpl {
void Journal::clear()
{
  actions_.clear();
  affected_nodes_.clear();
  affected_edges_.clear();
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
void Journal::undo(const JournalAction* action, const bool positions_only) const
{
  switch (action->typeId()) {
    case JournalActionTypeEnum::MOVE_CELL: {
      auto move_action = static_cast<const MoveCellAction*>(action);
      auto node = move_action->getNode();
      if (!positions_only) {
        grid_->erasePixel(node);
        for (auto seg : move_action->getNewSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->removeCellFromSegment(node, seg);
        }
      }
      node->setLeft(move_action->getOrigLeft());
      node->setBottom(move_action->getOrigBottom());
      if (!move_action->wasPlaced()) {
        node->setPlaced(false);
        return;
      }
      if (!positions_only) {
        paintInGrid(grid_, node);
        for (auto seg : move_action->getOrigSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->addCellToSegment(node, seg);
        }
      }
      break;
    }
    case JournalActionTypeEnum::UNPLACE_CELL: {
      auto unplace_action = static_cast<const UnplaceCellAction*>(action);
      auto node = unplace_action->getNode();
      grid_->paintPixel(node);
      node->setPlaced(true);
      node->setHold(unplace_action->wasHold());
      break;
    }
  }
}
////////////////////////////////////////////////////////////////////////////////
void Journal::undo(bool positions_only) const
{
  for (auto it = actions_.rbegin(); it != actions_.rend(); ++it) {
    auto action = (*it).get();
    undo(action, positions_only);
  }
}
////////////////////////////////////////////////////////////////////////////////
void Journal::redo(const JournalAction* action, const bool positions_only) const
{
  switch (action->typeId()) {
    case JournalActionTypeEnum::MOVE_CELL: {
      auto move_action = static_cast<const MoveCellAction*>(action);
      auto node = move_action->getNode();
      if (!positions_only) {
        grid_->erasePixel(node);
        for (auto seg : move_action->getOrigSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->removeCellFromSegment(node, seg);
        }
      }
      node->setLeft(move_action->getNewLeft());
      node->setBottom(move_action->getNewBottom());
      if (!positions_only) {
        paintInGrid(grid_, node);
        for (auto seg : move_action->getNewSegs()) {
          if (seg < 0) {
            continue;
          }
          mgr_->addCellToSegment(node, seg);
        }
      }
      break;
    }
    case JournalActionTypeEnum::UNPLACE_CELL: {
      auto unplace_action = static_cast<const UnplaceCellAction*>(action);
      auto node = unplace_action->getNode();
      grid_->erasePixel(node);
      node->setPlaced(false);
      node->setHold(false);
      break;
    }
    default:
      break;
  }
}
////////////////////////////////////////////////////////////////////////////////
void Journal::redo(bool positions_only) const
{
  for (const auto& action : actions_) {
    redo(action.get(), positions_only);
  }
}
////////////////////////////////////////////////////////////////////////////////

}  // namespace dpl
