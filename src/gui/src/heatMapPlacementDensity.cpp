//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "heatMapPlacementDensity.h"

#include "db_sta/dbNetwork.hh"

namespace gui {

PlacementDensityDataSource::PlacementDensityDataSource(utl::Logger* logger)
    : HeatMapDataSource(logger,
                        "Placement Density",
                        "Placement",
                        "PlacementDensity"),
      include_taps_(true),
      include_filler_(false),
      include_io_(false)
{
  addBooleanSetting(
      "Taps",
      "Include taps and endcaps:",
      [this]() { return include_taps_; },
      [this](bool new_value) { include_taps_ = new_value; });
  addBooleanSetting(
      "Filler",
      "Include fillers:",
      [this]() { return include_filler_; },
      [this](bool new_value) { include_filler_ = new_value; });
  addBooleanSetting(
      "IO",
      "Include IO:",
      [this]() { return include_io_; },
      [this](bool new_value) { include_io_ = new_value; });
}

bool PlacementDensityDataSource::populateMap()
{
  if (getBlock() == nullptr) {
    return false;
  }

  for (auto* inst : getBlock()->getInsts()) {
    if (!inst->getPlacementStatus().isPlaced()) {
      continue;
    }
    if (!include_filler_ && inst->getMaster()->isFiller()) {
      continue;
    }
    if (!include_taps_
        && (inst->getMaster()->getType() == odb::dbMasterType::CORE_WELLTAP
            || inst->getMaster()->isEndCap())) {
      continue;
    }
    if (!include_io_
        && (inst->getMaster()->isPad() || inst->getMaster()->isCover())) {
      continue;
    }
    odb::Rect inst_box = inst->getBBox()->getBox();

    addToMap(inst_box, 100.0);
  }

  return true;
}

void PlacementDensityDataSource::combineMapData(bool base_has_value,
                                                double& base,
                                                const double new_data,
                                                const double data_area,
                                                const double intersection_area,
                                                const double rect_area)
{
  base += new_data * intersection_area / rect_area;
}

void PlacementDensityDataSource::onShow()
{
  HeatMapDataSource::onShow();

  addOwner(getBlock());
}

void PlacementDensityDataSource::onHide()
{
  HeatMapDataSource::onHide();

  removeOwner();
}

void PlacementDensityDataSource::inDbInstCreate(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstCreate(odb::dbInst*, odb::dbRegion*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstDestroy(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstPlacementStatusBefore(
    odb::dbInst*,
    const odb::dbPlacementStatus&)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstSwapMasterBefore(odb::dbInst*,
                                                          odb::dbMaster*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbInstSwapMasterAfter(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbPreMoveInst(odb::dbInst*)
{
  destroyMap();
}

void PlacementDensityDataSource::inDbPostMoveInst(odb::dbInst*)
{
  destroyMap();
}

}  // namespace gui
