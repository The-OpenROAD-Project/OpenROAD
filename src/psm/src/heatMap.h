///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

namespace sta {
class Sta;
class Corner;
}  // namespace sta

namespace psm {
class PDNSim;

class IRDropDataSource : public gui::RealValueHeatMapDataSource
{
 public:
  IRDropDataSource(PDNSim* psm, sta::Sta* sta, utl::Logger* logger);

  void setBlock(odb::dbBlock* block) override;

  void setNet(odb::dbNet* net) { net_ = net; }
  void setCorner(sta::Corner* corner) { corner_ = corner; }

 protected:
  bool populateMap() override;
  void combineMapData(bool base_has_value,
                      double& base,
                      double new_data,
                      double data_area,
                      double intersection_area,
                      double rect_area) override;

  void determineMinMax(const gui::HeatMapDataSource::Map& map) override;

 private:
  void ensureLayer();
  void ensureCorner();
  void ensureNet();
  void setLayer(const std::string& name);
  void setCorner(const std::string& name);
  void setNet(const std::string& name);

  psm::PDNSim* psm_;
  sta::Sta* sta_;
  odb::dbTech* tech_ = nullptr;
  odb::dbTechLayer* layer_ = nullptr;
  odb::dbNet* net_ = nullptr;
  sta::Corner* corner_ = nullptr;
};

}  // namespace psm
