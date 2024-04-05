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

#include "heatMapRudy.h"

#include "odb/db.h"

namespace grt {

RUDYDataSource::RUDYDataSource(utl::Logger* logger, grt::GlobalRouter* grouter, odb::dbDatabase* db)
    : GlobalRoutingDataSource(logger,
                              "Estimated Congestion (RUDY inside GRT)",
                              "RUDY",
                              "RUDY")
{
  grouter_ = grouter;
  db_ = db;
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
  rudy_ = std::make_unique<grt::Rudy>(db_->getChip()->getBlock(), grouter_);

  const auto& [x_grid_size, y_grid_size] = rudy_->getGridSize();
  if (x_grid_size == 0 || y_grid_size == 0) {
    return false;
  }

  rudy_->calculateRudy();

  for (int x = 0; x < x_grid_size; ++x) {
    for (int y = 0; y < y_grid_size; ++y) {
      auto tile = rudy_->getTile(x, y);
      auto box = tile.getRect();
      const double value = tile.getRudy();
      addToMap(box, value);
    }
  }
  return true;
}

void RUDYDataSource::onShow()
{
  HeatMapDataSource::onShow();

  addOwner(getBlock());
}

void RUDYDataSource::onHide()
{
  HeatMapDataSource::onHide();

  removeOwner();
}

void RUDYDataSource::inDbInstCreate(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  destroyMap();
}

void RUDYDataSource::inDbInstDestroy(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbInstPlacementStatusBefore(
    odb::dbInst*,
    const odb::dbPlacementStatus&)
{
  destroyMap();
}

void RUDYDataSource::inDbInstSwapMasterAfter(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbPostMoveInst(odb::dbInst*)
{
  destroyMap();
}

void RUDYDataSource::inDbITermPostDisconnect(odb::dbITerm*, odb::dbNet*)
{
  destroyMap();
}

void RUDYDataSource::inDbITermPostConnect(odb::dbITerm*)
{
  destroyMap();
}

void RUDYDataSource::inDbBTermPostConnect(odb::dbBTerm*)
{
  destroyMap();
}

void RUDYDataSource::inDbBTermPostDisConnect(odb::dbBTerm*, odb::dbNet*)
{
  destroyMap();
}

}  // namespace grt
