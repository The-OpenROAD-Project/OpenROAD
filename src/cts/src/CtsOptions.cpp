/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2025, The Regents of the University of California
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

#include "CtsOptions.h"

namespace cts {

CtsOptions::MasterType CtsOptions::getType(odb::dbInst* inst) const
{
  if (inst->getName().substr(0, dummyload_prefix_.size())
      == dummyload_prefix_) {
    return MasterType::DUMMY;
  }

  return MasterType::TREE;
}

void CtsOptions::inDbInstCreate(odb::dbInst* inst)
{
  recordBuffer(inst->getMaster(), getType(inst));
}

void CtsOptions::inDbInstCreate(odb::dbInst* inst, odb::dbRegion* region)
{
  recordBuffer(inst->getMaster(), getType(inst));
}

void CtsOptions::recordBuffer(odb::dbMaster* master, MasterType type)
{
  switch (type) {
    case MasterType::DUMMY:
      dummy_count_[master]++;
      break;
    case MasterType::TREE:
      buffer_count_[master]++;
      break;
  }
}

}  // namespace cts
