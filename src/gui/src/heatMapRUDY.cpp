//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "heatMapRUDY.h"

#include "odb/db.h"

namespace gui {

RUDYDataSource::RUDYDataSource(utl::Logger* logger, odb::dbDatabase* db)
    : gui::HeatMapDataSource(logger,
                             "Estimated Congestion (RUDY)",
                             "RUDY",
                             "RUDY")
{
  logger_ = logger;
  db_ = db;
}

double RUDYDataSource::getGridXSize() const
{
  if (getBlock() == nullptr) {
    return default_grid_;
  }

  auto* gCellGrid = getBlock()->getGCellGrid();
  if (gCellGrid == nullptr) {
    return default_grid_;
  }

  std::vector<int> grid;
  gCellGrid->getGridX(grid);

  if (grid.size() < 2) {
    return default_grid_;
  }
  const double delta = grid[1] - grid[0];
  return delta / getBlock()->getDbUnitsPerMicron();
}

double RUDYDataSource::getGridYSize() const
{
  if (getBlock() == nullptr) {
    return default_grid_;
  }

  auto* gCellGrid = getBlock()->getGCellGrid();
  if (gCellGrid == nullptr) {
    return default_grid_;
  }

  std::vector<int> grid;
  gCellGrid->getGridY(grid);

  if (grid.size() < 2) {
    return default_grid_;
  }
  const double delta = grid[1] - grid[0];
  return delta / getBlock()->getDbUnitsPerMicron();
}

void RUDYDataSource::combineMapData(bool base_has_value,
                                    double& base,
                                    const double new_data,
                                    const double data_area,
                                    const double intersection_area,
                                    const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}
bool RUDYDataSource::populateMap()
{
  if (!getBlock()) {
    return false;
  }

  auto gridSize = rudyInfo_->getGridSize();
  if (gridSize.first == 0 || gridSize.second == 0) {
    return false;
  }

  rudyInfo_->calculateRUDY();

  for (int x = 0; x < gridSize.first; ++x) {
    for (int y = 0; y < gridSize.second; ++y) {
      auto tile = rudyInfo_->getTile(x, y);
      auto box = tile.getRect();
      const double value = tile.getRUDY();
      addToMap(box, value);
    }
  }
  return true;
}
void RUDYDataSource::setBlock(odb::dbBlock* block)
{
  HeatMapDataSource::setBlock(block);
  rudyInfo_ = std::make_unique<odb::RUDYCalculator>(getBlock());
}
}  // namespace gui
