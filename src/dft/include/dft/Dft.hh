///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
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

#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace dft {
class ScanReplace;

class Dft
{
 public:
  void init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  // Pre-work for insert_dft. We collect the cells that need to be
  // scan replaced. This function doesn't mutate the design.
  //
  // TODO (and not implemented yet)
  //  - scan architect
  void pre_dft();

  // Inserts the scan chains into the design. For now this just replace the
  // cells in the design with scan equivalent. This functions mutates the
  // design.
  //
  // TODO (and not implemented yet)
  // - scan stitching
  void insert_dft();

 private:
  // If we need to run pre_dft to create the internal state
  bool need_to_run_pre_dft_;

  // Resets the internal state
  void reset();

  // Global state
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;

  // Internal state
  std::unique_ptr<ScanReplace> scan_replace_;
};

}  // namespace dft
