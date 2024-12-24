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
#include "Utils.hh"
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
  explicit ScanStitch(odb::dbDatabase* db,
                      bool per_chain_enable,
                      std::string scan_enable_name_pattern = "scan_enable_{}",
                      std::string scan_in_name_pattern = "scan_in_{}",
                      std::string scan_out_name_pattern = "scan_out_{}");

  // Stitch all the cells inside each one of the scan chains together.
  void Stitch(const std::vector<std::unique_ptr<ScanChain>>& scan_chains,
              utl::Logger* logger);
  void Stitch(odb::dbBlock* block,
              const ScanChain& scan_chain,
              utl::Logger* logger,
              size_t ordinal = 1,
              size_t enable_ordinal = 1);

 private:
  ScanDriver FindOrCreateDriver(const char* kind,
                                odb::dbBlock* block,
                                const std::string& with_name,
                                utl::Logger* logger);
  ScanDriver FindOrCreateScanEnable(odb::dbBlock* block,
                                    const std::string& with_name,
                                    utl::Logger* logger);
  ScanDriver FindOrCreateScanIn(odb::dbBlock* block,
                                const std::string& with_name,
                                utl::Logger* logger);
  ScanLoad FindOrCreateScanOut(odb::dbBlock* block,
                               const ScanDriver& cell_scan_out,
                               const std::string& with_name,
                               utl::Logger* logger);

  // Typesafe function to create Ports for the scan chains.
  template <typename Port>
  inline Port CreateNewPort(odb::dbBlock* block,
                            const std::string& port_name,
                            utl::Logger* logger,
                            odb::dbNet* net = nullptr)
  {
    auto port = dft::utils::CreateNewPort(block, port_name, logger, net);

    if constexpr (std::is_same_v<Port, ScanLoad>) {
      port->setIoType(odb::dbIoType::OUTPUT);
    } else if constexpr (std::is_same_v<Port, ScanDriver>) {
      port->setIoType(odb::dbIoType::INPUT);
    } else {
      static_assert(always_false_v<Port>, "Non-exhaustive cases for Port Type");
    }

    return Port(port);
  }

  odb::dbDatabase* db_;
  odb::dbBlock* top_block_;
  bool per_chain_enable_;
  std::string scan_enable_name_pattern_;
  std::string scan_in_name_pattern_;
  std::string scan_out_name_pattern_;
};

}  // namespace dft
