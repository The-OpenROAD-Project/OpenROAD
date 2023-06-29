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

#include <optional>
#include <type_traits>
#include <vector>

#include "ScanChain.hh"
#include "odb/db.h"

namespace dft {

// Helper for CreateNewPort
template <class>
inline constexpr bool always_false_v = false;

// Performs Scan Stitch over the given set of scan chains. All the scan chains
// are going to share the same scan_enable
class ScanStitch
{
 public:
  explicit ScanStitch(odb::dbDatabase* db);

  // Stitch all the cells inside each one of the scan chains together.
  void Stitch(const std::vector<std::unique_ptr<ScanChain>>& scan_chains);
  void Stitch(odb::dbBlock* block,
              const ScanChain& scan_chain,
              const ScanDriver& scan_enable);

 private:
  ScanDriver FindOrCreateScanEnable(odb::dbBlock* block);
  ScanDriver FindOrCreateScanIn(odb::dbBlock* block);
  ScanLoad FindOrCreateScanOut(odb::dbBlock* block,
                               const ScanDriver& cell_scan_out);

  // Typesafe function to create Ports for the scan chains.
  template <typename Port>
  Port CreateNewPort(odb::dbBlock* block, std::string_view name_pattern)
  {
    for (int port_number = 1;; ++port_number) {
      std::string port_name
          = fmt::format(FMT_RUNTIME(name_pattern), port_number);
      odb::dbBTerm* port = block->findBTerm(port_name.c_str());
      if (!port) {
        odb::dbNet* net = odb::dbNet::create(block, port_name.c_str());
        net->setSigType(odb::dbSigType::SCAN);
        port = odb::dbBTerm::create(net, port_name.c_str());
        port->setSigType(odb::dbSigType::SCAN);

        if constexpr (std::is_same_v<Port, ScanLoad>) {
          port->setIoType(odb::dbIoType::OUTPUT);
        } else if constexpr (std::is_same_v<Port, ScanDriver>) {
          port->setIoType(odb::dbIoType::INPUT);
        } else {
          static_assert(always_false_v<Port>,
                        "Non-exhaustive cases for Port Type");
        }

        return Port(port);
      }
    }
  }

  odb::dbDatabase* db_;
  odb::dbBlock* top_block_;
};

}  // namespace dft
