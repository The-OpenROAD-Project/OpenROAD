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

#include <vector>
#include <memory>
#include "odb/db.h"
#include "utl/Logger.h"
#include "db_sta/dbSta.hh"
#include "db_sta/dbNetwork.hh"
#include "sta/Liberty.hh"

namespace dft {
namespace stitch {

struct State {
  odb::dbDatabase *db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  sta::dbNetwork db_network_;
};

// Actions
class Action {
 public:
  virtual bool Do(State &state) = 0;
  bool IsDone() const;

 protected:
  bool done_;
};

// Replacement of a sequential cell with a ScanCell
class ScanReplace: public Action {
 public:
  ScanReplace(sta::LibertyCell* original_cell_);
  virtual bool Do(State &state) override;

 private:
  sta::LibertyCell *original_cell_;
  sta::LibertyCell *new_cell_;
};

// A holder to create or use an existing port
class CreateOrUsePort: public Action {
  public:
   virtual bool Do(State &state) override;
};


// Connects the Scan In of a cell to the Scan Out of another
class ScanConnectCells: public Action {
  public:
   virtual bool Do(State &state) override;
  private:
   Action *from_cell_;
   Action *to_cell_;
};

// Connnects a Scan In Port with a Scan In of a Cell
// Dependencies:
//  - CreateOrUsePort
//  - ScanReplace
//
class ScanConnectScanInPort: public Action {
  public:
   virtual bool Do(State &state) override;
};

// Connnects a scan cell scan out with a scan out port
// Dependencies:
//  - CreateOrUsePort
//  - ScanReplace
//
class ScanConnectScanOutPort: public Action {
  public:
   virtual bool Do(State &state) override;
};

// Connects scan data lines to create a scan chain's shift register.
//
// Dependencies:
//  - ScanConnectCells
//  - ScanConnectScanInPort
//  - ScanConnectScanOutPort
//
class ScanConnectScanData: public Action {
  public:
   virtual bool Do(State &state) override;
};

// Scan connect one cell's SE to a port.
//
// Dependencies
// - CreateOrUsePort
// - ScanReplace
//
class ScanConnectScanEnable: public Action {
 public:
  virtual bool Do(State &state) override;
};

// Connects all the scan cells to a scan enable port
//
// Dependencies:
//  - ScanConnectScanEnable
//
class ScanConnectScanEnables: public Action {
 public:
  virtual bool Do(State &state) override;
};

// Performs a scan stiching of only one scan chain
//
// Dependencies:
//  - ScanConnectScanEnables
//  - [ScanConnectScanData]
//
class ScanStitchScanChain: public Action {
  public:
   virtual bool Do(State &state) override;
};

class Stitch {
 public:
  Stitch();
  void run(State &state);
  std::string GetDot() const;

 private:
};

} // namespace stitch
} // namespace dft
