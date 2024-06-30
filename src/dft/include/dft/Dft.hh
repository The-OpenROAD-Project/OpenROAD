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
class DftConfig;
class ScanChain;

// The main DFT implementation.
//
// We can split the DFT process in 3 main steps:
//
// 1) Scan Replace: Where non scan sequential cells are replaced by the scan
// equivalent.
//
// 2) Scan Architect: We create the scan chains and decide what scan cells are
// going to be in them. This is an instance of the Bin-Packing problem where we
// have elements of different size (cells of different bits) and bins (scan
// chains) where to put them.
//
// 3 Scan Stitching: Where we take the scan chains from Scan Architect and
// connect the cells together to form the scan chains in the design. This is
// done by connecting the output of each scan scell to the scan input of the
// next scan cell, based on the order defined in Scan Architect.
//
// See:
//  VLSI Test Principles and Architectures, 2006, Chapter 2.7: Scan Design Flow
//
class Dft
{
 public:
  Dft();

  void init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

  // Pre-work for insert_dft. We collect the cells that need to be
  // scan replaced. This function doesn't mutate the design.
  void pre_dft();

  // Calls pre_dft performing scan replace and scan architect.
  // A report is going to be generated and printed followed by a rollback to
  // undo the work of scan replace. Use this command/function to iterate
  // different options like max_length
  //
  // Here we do:
  //  - Scan Replace
  //  - Scan Architect
  //  - Rollback
  //  - Shows Scan Architect report
  //
  // If verbose is true, then we show all the cells that are inside the scan
  // chains
  void previewDft(bool verbose);

  // Inserts the scan chains into the design. For now this just replace the
  // cells in the design with scan equivalent. This functions mutates the
  // design.
  //
  // Here we do:
  //  - Scan Replace
  //
  void scanReplace();

  void insertDft();

  // Returns a mutable version of DftConfig
  DftConfig* getMutableDftConfig();

  // Returns a const version of DftConfig
  const DftConfig& getDftConfig() const;

  // Prints to stdout
  void reportDftConfig() const;

 private:
  // If we need to run pre_dft to create the internal state
  bool need_to_run_pre_dft_;

  // Resets the internal state
  void reset();

  // Common function to perform scan replace and scan architect. Shared between
  // preview_dft and insert_dft
  std::vector<std::unique_ptr<ScanChain>> scanArchitect();

  // Global state
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;

  // Internal state
  std::unique_ptr<ScanReplace> scan_replace_;
  std::unique_ptr<DftConfig> dft_config_;
};

}  // namespace dft
