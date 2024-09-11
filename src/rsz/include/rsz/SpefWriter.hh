/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, Precision Innovations Inc.
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <map>

#include "db_sta/dbSta.hh"
#include "utl/Logger.h"

namespace grt {
class GlobalRouter;
class MakeWireParasitics;
}  // namespace grt

namespace rsz {

class Resizer;
using utl::Logger;

using sta::Corner;
using sta::dbNetwork;
using sta::dbSta;
using sta::Net;
using sta::NetSeq;
using sta::Parasitic;
using sta::Parasitics;

class SpefWriter
{
 public:
  SpefWriter(Logger* logger,
             dbSta* sta,
             std::map<Corner*, std::ostream*>& spef_streams);
  void writeHeader();
  void writePorts();
  void writeNet(Corner* corner, const Net* net, Parasitic* parasitic);

 private:
  Logger* logger_;
  dbSta* sta_;
  dbNetwork* network_;
  Parasitics* parasitics_;

  std::map<Corner*, std::ostream*> spef_streams_;
};

}  // namespace rsz
