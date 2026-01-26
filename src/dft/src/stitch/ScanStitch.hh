// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "ScanChain.hh"
#include "ScanPin.hh"
#include "ScanStitchConfig.hh"
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
                      utl::Logger* logger,
                      const ScanStitchConfig& config);

  // Stitch one or more scan chains.
  void Stitch(const std::vector<std::unique_ptr<ScanChain>>& scan_chains);

  // Stitch all the cells inside each one of the scan chains together.
  // - Ordinals are used with scan in/out/enable name patterns to produce the
  // - final name for the signal(s) in question. Enable ordinal is different
  // - to account for whether you're using global or per-chain enable.
  void Stitch(odb::dbBlock* block, ScanChain& scan_chain, size_t ordinal = 0);

 private:
  ScanDriver FindOrCreateDriver(std::string_view kind,
                                odb::dbBlock* block,
                                const std::string& with_name);
  ScanDriver FindOrCreateScanEnable(odb::dbBlock* block,
                                    const std::string& with_name);
  ScanDriver FindOrCreateScanIn(odb::dbBlock* block,
                                const std::string& with_name);
  ScanLoad FindOrCreateScanOut(odb::dbBlock* block,
                               const ScanDriver& cell_scan_out,
                               const std::string& with_name);

  // Typesafe function to create Ports for the scan chains.
  template <typename Port>
  Port CreateNewPort(odb::dbBlock* block,
                     const std::string& port_name,
                     odb::dbNet* net = nullptr)
  {
    auto port = dft::utils::CreateNewPort(block, port_name, logger_, net);

    if constexpr (std::is_same_v<Port, ScanLoad>) {
      port->setIoType(odb::dbIoType::OUTPUT);
    } else if constexpr (std::is_same_v<Port, ScanDriver>) {
      port->setIoType(odb::dbIoType::INPUT);
    } else {
      static_assert(always_false_v<Port>, "Non-exhaustive cases for Port Type");
    }

    return Port(port);
  }

  const ScanStitchConfig& config_;
  odb::dbDatabase* db_;
  utl::Logger* logger_;
  odb::dbBlock* top_block_;
};

}  // namespace dft
