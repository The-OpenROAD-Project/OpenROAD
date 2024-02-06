///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include "gui/heatMap.h"
#include "odb/dbBlockCallBackObj.h"
#include "odb/util.h"

namespace odb {
class dbDatabase;
}

namespace gui {

class RUDYDataSource : public GlobalRoutingDataSource,
                       public odb::dbBlockCallBackObj
{
 public:
  RUDYDataSource(utl::Logger* logger);

  void onShow() override;
  void onHide() override;

  // from dbBlockCallBackObj API
  void inDbInstCreate(odb::dbInst*) override;
  void inDbInstCreate(odb::dbInst*, odb::dbRegion*) override;
  void inDbInstDestroy(odb::dbInst*) override;
  void inDbInstPlacementStatusBefore(odb::dbInst*,
                                     const odb::dbPlacementStatus&) override;
  void inDbInstSwapMasterAfter(odb::dbInst*) override;
  void inDbPostMoveInst(odb::dbInst*) override;

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      const double new_data,
                      const double data_area,
                      const double intersection_area,
                      const double rect_area) override;
  void setBlock(odb::dbBlock* block) override;

 private:
  std::unique_ptr<odb::RUDYCalculator> rudyInfo_;
};

}  // namespace gui
