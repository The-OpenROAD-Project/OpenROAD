///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// Copyright (c) 2024, Antmicro
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
//
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <string>
#include <utility>

#include "routeBase.h"
#include "grt/GlobalRouter.h"
#include "odb/db.h"
#include "placerBase.h"
#include "utl/Logger.h"

using std::string;
using std::vector;

namespace gpl2 {

/////////////////////////////////////////////
// RouteBaseVars

RouteBaseVars::RouteBaseVars()
{
  reset();
}

void RouteBaseVars::reset()
{
  inflationRatioCoef = 2.5;
  maxInflationRatio = 2.5;
  maxDensity = 0.90;
  targetRC = 1.25;
  ignoreEdgeRatio = 0.8;
  minInflationRatio = 1.01;
  rcK1 = rcK2 = 1.0;
  rcK3 = rcK4 = 0.0;
  maxBloatIter = 1;
  maxInflationIter = 4;
}

/////////////////////////////////////////////
// RouteBase

RouteBase::RouteBase()
    : db_(nullptr), grouter_(nullptr), nbc_(nullptr), log_(nullptr)
{
}

RouteBase::RouteBase(RouteBaseVars rbVars,
                           odb::dbDatabase* db,
                           grt::GlobalRouter* grouter,
                           const std::shared_ptr<PlacerBaseCommon>& nbc,
                           std::vector<std::shared_ptr<PlacerBase>> nbVec,
                           utl::Logger* log)
    : RouteBase()
{
  rbVars_ = rbVars;
  db_ = db;
  grouter_ = grouter;
  nbc_ = std::move(nbc);
  log_ = log;
  nbVec_ = std::move(nbVec);
}

RouteBase::~RouteBase() = default;

}  // namespace gpl2
