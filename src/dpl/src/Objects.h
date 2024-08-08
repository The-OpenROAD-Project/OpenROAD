/////////////////////////////////////////////////////////////////////////////
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
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Grid.h"
#include "dpl/Opendp.h"

namespace dpl {

using odb::dbOrientType;
using odb::dbSite;

struct Master
{
  bool is_multi_row = false;
};

struct Cell
{
  const char* name() const;
  bool inGroup() const { return group_ != nullptr; }
  int64_t area() const;
  bool isStdCell() const;
  DbuX siteWidth() const;
  bool isFixed() const;
  bool isHybrid() const;
  bool isHybridParent() const;
  dbSite* getSite() const;
  DbuX xMax() const { return x_ + width_; }
  bool isBlock() const;

  dbInst* db_inst_ = nullptr;
  DbuX x_{0};  // lower left wrt core DBU
  DbuY y_{0};
  dbOrientType orient_;
  DbuX width_{0};
  DbuY height_{0};
  bool is_placed_ = false;
  bool hold_ = false;
  Group* group_ = nullptr;
  Rect* region_ = nullptr;  // group rect
};

struct Group
{
  string name;
  vector<Rect> region_boundaries;
  vector<Cell*> cells_;
  Rect boundary;
  double util = 0.0;
};

}  // namespace dpl
