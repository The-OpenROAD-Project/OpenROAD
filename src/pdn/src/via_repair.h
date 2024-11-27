///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include <map>
#include <set>

#include "odb/db.h"
#include "shape.h"

namespace utl {
class Logger;
}

namespace pdn {

class ViaRepair
{
  using ViaValue = std::pair<odb::Rect, odb::dbSBox*>;
  using ViaTree = bgi::rtree<ViaValue, bgi::quadratic<16>>;
  using LayerViaTree = std::map<odb::dbTechLayer*, ViaTree>;

 public:
  ViaRepair(utl::Logger* logger, const std::set<odb::dbNet*>& nets);

  void repair();

  void report() const;

 private:
  utl::Logger* logger_;
  std::set<odb::dbNet*> nets_;

  bool use_obs_ = true;
  bool use_nets_ = true;
  bool use_inst_ = true;

  std::map<odb::dbTechLayer*, int> via_count_;
  std::map<odb::dbTechLayer*, int> removal_count_;

  LayerViaTree collectVias();

  using ObsRect = std::map<odb::dbTechLayer*, std::set<odb::Rect>>;

  ObsRect collectBlockObstructions(odb::dbBlock* block);
  ObsRect collectInstanceObstructions(odb::dbBlock* block);
  ObsRect collectNetObstructions(odb::dbBlock* block);
};

}  // namespace pdn
