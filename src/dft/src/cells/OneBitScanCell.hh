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

#include "ScanCell.hh"
#include "ScanPin.hh"
#include "db_sta/dbNetwork.hh"
#include "odb/db.h"
#include "sta/Liberty.hh"

namespace dft {

// A simple single cell with just one bit. Usually one scan FF
class OneBitScanCell : public ScanCell
{
 public:
  OneBitScanCell(const std::string& name,
                 std::unique_ptr<ClockDomain> clock_domain,
                 odb::dbInst* inst,
                 sta::TestCell* test_cell,
                 sta::dbNetwork* db_network,
                 utl::Logger* logger);
  // Not copyable or movable
  OneBitScanCell(const OneBitScanCell&) = delete;
  OneBitScanCell& operator=(const OneBitScanCell&) = delete;

  uint64_t getBits() const override;
  void connectScanEnable(const ScanDriver& driver) const override;
  void connectScanIn(const ScanDriver& driver) const override;
  void connectScanOut(const ScanLoad& load) const override;
  ScanDriver getScanOut() const override;

  odb::Point getOrigin() const override;
  bool isPlaced() const override;

 private:
  odb::dbITerm* findITerm(sta::LibertyPort* liberty_port) const;

  odb::dbInst* inst_;
  sta::TestCell* test_cell_;
  sta::dbNetwork* db_network_;
};

}  // namespace dft
