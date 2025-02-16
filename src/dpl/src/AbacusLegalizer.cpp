/////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2024, The Regents of the University of California
// Copyright (c) 2024, Minjae Kim (kmj0824@postech.ac.kr)
// All rights reserved.
//
// BSD 3-Clause License
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
//
// This code refers to the below contents.
// 1. [Abacus] PAPER:
//    Fast Legalization of Standard Cell Circuits with Minimal Movement
// 2. [DREAMPlace] LINK:
//    https://github.com/limbo018/DREAMPlace/tree/master/dreamplace/ops/abacus_legalize
///////////////////////////////////////////////////////////////////////////////

#include "AbacusLegalizer.h"

#include <algorithm>
#include <limits>
#include <map>

#include "dpl/Opendp.h"

namespace dpl {

void Opendp::runAbacus()
{
  AbacusLegalizer abacusLegalizer;

  abacusLegalizer.runAbacus(db_->getChip()->getBlock());
}

// clang-format off
/**
 * @brief Algorithm 1: Modified Abacus Legalization Approach
 *
 * This algorithm legalizes cells in a VLSI design by minimizing their movement.
 * It optimizes the cell placement by considering the nearest rows first and
 * limits vertical movements to reduce total displacement and computational
 * effort.
 *
 * @details The cells are assumed to be pre-sorted according to their
 * x-position. The algorithm tries to find the best row for each cell such that
 * the overall movement is minimized.
 *
 * @note The cells are initially sorted either in ascending or descending order
 * of x-position.
 *
 * @pseudo
 * 1. Sort cells according to x-position; // Ascending or descending
 * 2. foreach cell i do
 * 3.     c_best ← ∞; // Initialize best cost as infinity
 * 4.     r_best ← null; // Initialize best row as null
 * 5.     r_closest ← find the nearest row to cell i according to the global position;
 * 6.     foreach direction in [above, below] do
 * 7.         foreach row r in direction from r_closest do
 * 8.             Insert cell i into row r;
 * 9.             PlaceRow r (trial);
 * 10.            Determine cost c_i; // Determine the cost for cell i at row r
 * 11.            if c_i < c_best then
 * 12.                c_best = c_i; // Update best cost
 * 13.                r_best = r; // Update best row
 * 14.            else if cost exceeds lower bound then
 * 15.                break; // Stop if the cost exceeds the lower bound
 * 16.            end if
 * 17.            Remove cell i from row r; // Remove cell i from row r as it's a trial placement
 * 18.        end foreach
 * 19.    end foreach
 * 20.    if r_best is not null then
 * 21.        Insert Cell i to row r_best; // Insert cell i into the best row
 * 22.        PlaceRow r_best (final); // Finalize the placement
 * 23.    end if
 * 24. end foreach
 * @endpseudo
 * Here, we will set the lower bound of cost
 * as only one movement from closest row
 */
// clang-format on
void AbacusLegalizer::runAbacus(odb::dbBlock* block)
{
  // Initialization
  targetBlock_ = block;
  initRow();
  std::vector<odb::dbInst*> cells;
  cells.reserve(targetBlock_->getInsts().size());
  for (auto inst : targetBlock_->getInsts()) {
    if (inst->isFixed()) {
      continue;
    }
    cells.push_back(inst);
  }

  // for common vars
  auto rowSample = *targetBlock_->getRows().begin();
  auto rowHeight = rowSample->getBBox().dy();
  auto yMin = rowSample->getBBox().yMin();

  // Sort cells according to x-position
  std::sort(cells.begin(),
            cells.end(),
            [](const odb::dbInst* a, const odb::dbInst* b) {
              return a->getLocation().x() < b->getLocation().x();
            });

  // for each cell_{i} do
  for (auto cell : cells) {
    // c_{best} ← ∞
    uint costBest = std::numeric_limits<uint>::max();
    InstsInRow* bestRow = nullptr;
    auto closestRow = cellToRowMap_[cell];

    vector<InstsInRow*> rowsToCheck{
        getBelowRow(closestRow), closestRow, getAboveRow(closestRow)};

    closestRow->erase(std::remove(closestRow->begin(), closestRow->end(), cell),
                      closestRow->end());
    for (auto row : rowsToCheck) {
      if (row == nullptr) {
        continue;
      }
      // insert cell i into rwo r
      row->push_back(cell);
      auto rowIdx = rowToRowIdxMap_[row];
      cell->setLocation(cell->getLocation().x(), rowIdx * rowHeight + yMin);
      // PlaceRow r (trial)
      auto cost = placeRow(row, true);
      if (cost < costBest) {
        costBest = cost;
        bestRow = row;
      }
      // Remove the cell after trial
      row->erase(std::remove(row->begin(), row->end(), cell), row->end());
    }
    if (bestRow != nullptr) {
      bestRow->push_back(cell);
      placeRow(bestRow, false);
      cellToRowMap_[cell] = bestRow;
    }
  }
}

uint AbacusLegalizer::placeRow(InstsInRow* instsInRow, bool trial)
{
  std::map<dbInst*, pair<int, int>> coordinateStore;
  if (trial == true) {
    for (auto inst : *instsInRow) {
      coordinateStore[inst]
          = {inst->getLocation().x(), inst->getLocation().y()};
    }
  }

  // Sort cells according to x-position
  std::sort(instsInRow->begin(),
            instsInRow->end(),
            [](const odb::dbInst* a, const odb::dbInst* b) {
              return a->getLocation().x() < b->getLocation().x();
            });

  // Init cluster
  std::vector<AbacusCluster> abacusClusters;
  auto numInstsInRow = instsInRow->size();
  abacusClusters.reserve(numInstsInRow);
  // kernel algorithm for placeRow
  for (int i = 0; i < numInstsInRow; ++i) {
    auto inst = instsInRow->at(i);
    auto instX = static_cast<double>(inst->getLocation().x());

    if (abacusClusters.empty()
        || abacusClusters.back().x + abacusClusters.back().w <= instX) {
      AbacusCluster cluster;  // create new cluster
      cluster.e = 0;
      cluster.w = 0;
      cluster.q = 0;
      cluster.x = instX;
      addCell(&cluster, inst);
      abacusClusters.push_back(cluster);
    } else {
      auto& lastCluster = abacusClusters.back();
      addCell(&lastCluster, inst);
      collapse(lastCluster, abacusClusters);
    }
  }
  // transform cluster positions x_c(c) to cell positions x(i)
  for (const auto& cluster : abacusClusters) {
    int x = static_cast<int>(cluster.x);
    for (auto inst : cluster.instSet) {
      if (inst->getPlacementStatus().isFixed()) {
        continue;
      }
      inst->setLocation(x, inst->getLocation().y());
      inst->setPlacementStatus(odb::dbPlacementStatus::PLACED);
      x += static_cast<int>(inst->getMaster()->getWidth());
    }
  }

  auto cost = getCost();
  // If this is for the trial, then redo the coordinate
  if (trial == true) {
    // redo the coordinate
    for (auto inst : *instsInRow) {
      auto coordinate = coordinateStore[inst];
      inst->setLocation(coordinate.first, coordinate.second);
    }
  }

  return cost;
}

void AbacusLegalizer::addCell(AbacusLegalizer::AbacusCluster* cluster,
                              odb::dbInst* inst)
{
  auto instX = static_cast<double>(inst->getLocation().x());
  auto instWidth = static_cast<double>(inst->getMaster()->getWidth());
  double instE;
  if (inst->getPlacementStatus().isFixed()) {
    instE = std::numeric_limits<double>::max();
  } else {
    instE = 1;
  }

  cluster->instSet.push_back(inst);
  cluster->e += instE;
  cluster->q += instE * (instX - cluster->w);
  cluster->w += instWidth;
}

void AbacusLegalizer::addCluster(AbacusLegalizer::AbacusCluster* predecessor,
                                 AbacusLegalizer::AbacusCluster* cluster)
{
  predecessor->instSet.insert(predecessor->instSet.end(),
                              cluster->instSet.begin(),
                              cluster->instSet.end());
  predecessor->e += cluster->e;
  predecessor->q += cluster->q - cluster->e * predecessor->w;
  predecessor->w += cluster->w;
}

void AbacusLegalizer::collapse(AbacusLegalizer::AbacusCluster& cluster,
                               vector<AbacusCluster>& abacusClusters)
{
  auto xMin = (*targetBlock_->getRows().begin())->getBBox().xMin();
  auto xMax = (*targetBlock_->getRows().begin())->getBBox().xMax();

  cluster.x = cluster.q / cluster.e;  // place cluster c
  // limit position between x min and x max - w_c(c)
  if (cluster.x < xMin) {
    cluster.x = xMin;
  } else if (cluster.x > xMax - cluster.w) {
    cluster.x = xMax - cluster.w;
  }

  if (abacusClusters.size() > 1) {
    // if predecessor exists,
    auto& predecessor = *(abacusClusters.end() - 2);
    if (predecessor.x + predecessor.w > cluster.x) {
      // merge cluster c to c'
      addCluster(&predecessor, &cluster);
      abacusClusters.erase(abacusClusters.end() - 1);  // remove cluster c
      collapse(predecessor, abacusClusters);
    }
  }
}

void AbacusLegalizer::initRow()
{
  auto numRows = targetBlock_->getRows().size();
  rows_.resize(numRows);
  for (auto inst : targetBlock_->getInsts()) {
    auto rowSample = (*targetBlock_->getRows().begin());
    auto rowIdx = getRowIdx(inst);
    auto rowHeight = rowSample->getBBox().dy();
    auto yMin = rowSample->getBBox().yMin();
    inst->setLocation(inst->getLocation().x(), rowIdx * rowHeight + yMin);
    if (rowIdx >= 0 && rowIdx < numRows) {
      if (inst->isFixed()) {
        continue;
      }
      rows_.at(rowIdx).push_back(inst);
      cellToRowMap_[inst] = &rows_.at(rowIdx);
    }
  }
  for (int i = 0; i < rows_.size(); ++i) {
    auto& row = rows_.at(i);
    rowToRowIdxMap_[&row] = i;
  }
  // sort the instances in the row according to the x-coordinates for each row
  for (auto& row : rows_) {
    std::sort(
        row.begin(), row.end(), [](const odb::dbInst* a, const odb::dbInst* b) {
          return a->getLocation().x() < b->getLocation().x();
        });
  }
}
uint AbacusLegalizer::getCost()
{
  int64_t hpwl_sum = 0;
  for (dbNet* net : targetBlock_->getNets()) {
    if (net->getSigType().isSupply()) {
      continue;
    }
    auto bbox = net->getTermBBox();
    hpwl_sum += bbox.dx() + bbox.dy();
  }
  return hpwl_sum;
}

int AbacusLegalizer::getRowIdx(dbInst* cell)
{
  auto rowHeight = (*targetBlock_->getRows().begin())->getBBox().dy();
  auto yMin = (*targetBlock_->getRows().begin())->getBBox().yMin();
  auto cellY = cell->getLocation().y() + cell->getMaster()->getHeight() / 2;
  int rowIdx = (cellY - yMin) / rowHeight;
  return rowIdx;
}
AbacusLegalizer::InstsInRow* AbacusLegalizer::getAboveRow(InstsInRow* row)
{
  if (!row) {
    return nullptr;
  }
  // Finding the index of the current row in rows_
  auto it = std::find_if(
      rows_.begin(), rows_.end(), [row](const InstsInRow& rowTmp) {
        return &rowTmp == row;  // Compare addresses
      });

  // If the current row is found, and it is not the first row
  if (it != rows_.end() && it != rows_.end() - 1) {
    // Get the row above the current row
    auto aboveIt = std::next(it);
    return &(*aboveIt);
  }

  // If the current row is the first row or not found
  return nullptr;
}
AbacusLegalizer::InstsInRow* AbacusLegalizer::getBelowRow(InstsInRow* row)
{
  if (!row) {
    return nullptr;
  }
  // Finding the index of the current row in rows_
  auto it = std::find_if(
      rows_.begin(), rows_.end(), [row](const InstsInRow& rowTmp) {
        return &rowTmp == row;  // Compare addresses
      });

  // If the current row is found, and it is not the first row
  if (it != rows_.end() && it != rows_.begin()) {
    // Get the row below the current row
    auto belowIt = std::prev(it);
    return &(*belowIt);
  }

  // If the current row is the first row or not found
  return nullptr;
}

}  // namespace dpl
