/////////////////////////////////////////////////////////////////////////////
// James Cherry, Parallax Software, Inc.
//
// BSD 3-Clause License
//
// Copyright (c) 2020, James Cherry, Parallax Software, Inc.
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
///////////////////////////////////////////////////////////////////////////////

#include "opendp/Opendp.h"

namespace opendp {

using std::vector;

using odb::dbMaster;
using odb::dbLib;

void
Opendp::placeFillers(StringSeq *filler_master_names)
{
  findFillerMasters(filler_master_names);
  for (dbMaster *filler_master : filler_masters_) {
    printf("%s\n", filler_master->getConstName());
  }
  gap_fillers_.clear();
  dbMasterSeq &fillers = gapFillers(5);
  for (dbMaster *master : fillers) {
    printf("+ %s\n", master->getConstName());
  }
  fillers = gapFillers(66);
  for (dbMaster *master : fillers) {
    printf("+ %s\n", master->getConstName());
  }
}

void
Opendp::findFillerMasters(StringSeq *filler_master_names)
{
  filler_masters_.clear();
  for(string &master_name : *filler_master_names) {
    for (dbLib *lib : db_->getLibs()) {
      dbMaster *master = lib->findMaster(master_name.c_str());
      if (master) {
	filler_masters_.push_back(master);
	break;
      }
    }
  }
  sort(filler_masters_.begin(), filler_masters_.end(),
       [](dbMaster *master1,
	  dbMaster *master2) {
	 return master1->getWidth() > master2->getWidth();
       });
}

// return list of masters to fill gap (in site width units)
dbMasterSeq &
Opendp::gapFillers(int gap)
{
  if (gap_fillers_.size() < gap + 1)
    gap_fillers_.resize(gap + 1);
  dbMasterSeq &fillers = gap_fillers_[gap];
  if (fillers.empty()) {
    int width = 0;
    for (dbMaster *filler_master : filler_masters_) {
      int filler_width = filler_master->getWidth() / site_width_;
      while ((width + filler_width) <= gap) {
	fillers.push_back(filler_master);
	width += filler_width;
	if (width == gap)
	  return fillers;
      }
    }
  }
  error("could not fill gap");
  return fillers;
}

}  // namespace opendp
