/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2024, Parallax Software, Inc.
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <vector>

namespace utl {
class Logger;
}

#include "odb/db.h"

namespace dpl {

using odb::dbInst;
using odb::dbNet;
using odb::Rect;

using std::unordered_map;
using std::vector;

using utl::Logger;

class NetBox
{
 public:
  NetBox() = default;
  NetBox(dbNet* net, Rect box, bool ignore);
  int64_t hpwl();
  void saveBox();
  void restoreBox();

  dbNet* net_ = nullptr;
  Rect box_;
  Rect box_saved_;
  bool ignore_ = false;
};

using NetBoxMap = unordered_map<dbNet*, NetBox>;
using NetBoxes = vector<NetBox*>;

class OptimizeMirroring
{
 public:
  OptimizeMirroring(Logger* logger, odb::dbDatabase* db);

  void run();

 private:
  int mirrorCandidates(vector<dbInst*>& mirror_candidates);
  void findNetBoxes();
  std::vector<dbInst*> findMirrorCandidates(NetBoxes& net_boxes);

  void updateNetBoxes(dbInst* inst);
  void saveNetBoxes(dbInst* inst);
  void restoreNetBoxes(dbInst* inst);

  int64_t hpwl(dbInst* inst);  // Sum of ITerm hpwl's.

  Logger* logger_ = nullptr;
  odb::dbDatabase* db_ = nullptr;
  odb::dbBlock* block_ = nullptr;

  NetBoxMap net_box_map_;

  // Net bounding box size on nets with more instance terminals
  // than this are ignored.
  static constexpr int mirror_max_iterm_count_ = 100;
};

}  // namespace dpl
