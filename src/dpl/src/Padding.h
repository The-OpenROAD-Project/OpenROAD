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

#include "Coordinates.h"
#include "dpl/Opendp.h"

namespace dpl {

class Padding
{
 public:
  int padGlobalLeft() const { return pad_left_; }
  int padGlobalRight() const { return pad_right_; }

  void setPaddingGlobal(int left, int right);
  void setPadding(dbInst* inst, int left, int right);
  void setPadding(dbMaster* master, int left, int right);
  bool havePadding() const;

  // Find instance/master/global padding value for an instance.
  int padLeft(dbInst* inst) const;
  int padLeft(const Cell* cell) const;
  int padRight(dbInst* inst) const;
  int padRight(const Cell* cell) const;
  bool isPaddedType(dbInst* inst) const;
  DbuX paddedWidth(const Cell* cell) const;

 private:
  using InstPaddingMap = map<dbInst*, pair<int, int>>;
  using MasterPaddingMap = map<dbMaster*, pair<int, int>>;

  int pad_left_ = 0;
  int pad_right_ = 0;
  InstPaddingMap inst_padding_map_;
  MasterPaddingMap master_padding_map_;
};

}  // namespace dpl
