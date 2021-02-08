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

#include "macro.h"
#include "openroad/OpenRoad.hh"
#include "opendb/db.h"

namespace mpl {

Macro::Macro(double _lx, double _ly, 
        double _w, double _h,
        double _haloX, double _haloY, 
        double _channelX, double _channelY,
        Vertex* _ptr, sta::Instance* _staInstPtr,
        odb::dbInst* _dbInstPtr) 
      : lx(_lx), ly(_ly), 
      w(_w), h(_h),
      haloX(_haloX), haloY(_haloY),
      channelX(_channelX), channelY(_channelY), 
      ptr(_ptr), staInstPtr(_staInstPtr),
      dbInstPtr(_dbInstPtr) {}

std::string Macro::name() {
  return dbInstPtr->getName();
}

std::string Macro::type() {
  return dbInstPtr->getMaster()->getName();
}

void Macro::Dump() {
  // debugPrint(log_, MPL, "tritonmp", 5, "MACRO {} {} {} {} {} {}",
  //     name, type, lx, ly, w, h);
  // debugPrint(log_, MPL, "tritonmp", 5, "{} {} {} {}", 
  //     haloX, haloY, 
  //    channelX, channelY); 
}

}

MacroLocalInfo::MacroLocalInfo() : 
    haloX_(0), haloY_(0), 
    channelX_(0), channelY_(0) {}
