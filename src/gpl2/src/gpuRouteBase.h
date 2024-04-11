///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018-2023, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <memory>
#include <vector>

namespace odb {
class dbDatabase;
}

namespace grt {
class GlobalRouter;
}

namespace utl {
class Logger;
}

namespace gpl2 {

class PlacerBaseCommon;
class PlacerBase;

class RouteBaseVars
{
 public:
  float inflationRatioCoef;
  float maxInflationRatio;
  float maxDensity;
  float targetRC;
  float ignoreEdgeRatio;
  float minInflationRatio;

  // targetRC metric coefficients.
  float rcK1, rcK2, rcK3, rcK4;

  int maxBloatIter;
  int maxInflationIter;

  RouteBaseVars();
  void reset();
};


class GpuRouteBase
{
  public:
    GpuRouteBase();
    GpuRouteBase(RouteBaseVars rbVars,
                 odb::dbDatabase* db,
                 grt::GlobalRouter* grouter,
                 std::shared_ptr<PlacerBaseCommon> nbc,
                 std::vector<std::shared_ptr<PlacerBase> > nbVec,
                 utl::Logger* log);
    ~GpuRouteBase();

  private:
    RouteBaseVars rbVars_;
    odb::dbDatabase* db_;
    grt::GlobalRouter* grouter_;

    std::shared_ptr<PlacerBaseCommon> nbc_;
    std::vector<std::shared_ptr<PlacerBase> > nbVec_;
    utl::Logger* log_;

    // TODO: dummy function now
    void init();
};
}  // namespace gpl2
