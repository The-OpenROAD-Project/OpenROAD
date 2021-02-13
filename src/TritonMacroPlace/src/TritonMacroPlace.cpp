///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019-2020, The Regents of the University of California
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

#include "mpl/TritonMacroPlace.h"

#include "MacroPlace.h"

namespace mpl {

TritonMacroPlace::TritonMacroPlace() : solCount_(0)
{
  std::unique_ptr<MacroPlacer> mckt(new MacroPlacer());
  mckt_ = std::move(mckt);
}

TritonMacroPlace::~TritonMacroPlace()
{
}

void TritonMacroPlace::init(odb::dbDatabase* db,
                            sta::dbSta* sta,
                            utl::Logger* log)
{
  mckt_->init(db, sta, log);
}

void TritonMacroPlace::setGlobalConfig(const char* globalConfig)
{
  mckt_->setGlobalConfig(globalConfig);
}

void TritonMacroPlace::setLocalConfig(const char* localConfig)
{
  mckt_->setLocalConfig(localConfig);
}

void TritonMacroPlace::setFenceRegion(double lx,
                                      double ly,
                                      double ux,
                                      double uy)
{
  mckt_->setFenceRegion(lx, ly, ux, uy);
}

bool TritonMacroPlace::placeMacros()
{
  mckt_->PlaceMacros(solCount_);
  return true;
}

int TritonMacroPlace::getSolutionCount()
{
  return solCount_;
}

}  // namespace mpl
