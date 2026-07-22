// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "ScanPin.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace dft {
class ClockDomain;

// A Scan Cell is a cell that contains a scan enable, scan in and scan out
// It also has the number of bits that can be shifted in this cell and a
// clock domain for architecting.
//
// This is an abstraction since there are several types of scan cells that can
// be use for DFT. The simplest one is just one scan cell but we could support
// in the future black boxes or CTLs (Core Test Language)
class ScanCell
{
 public:
  ScanCell(const std::string& name,
           std::unique_ptr<ClockDomain> clock_domain,
           utl::Logger* logger);
  virtual ~ScanCell() = default;
  // Not copyable or movable
  ScanCell(const ScanCell&) = delete;
  ScanCell& operator=(const ScanCell&) = delete;

  virtual uint64_t getBits() const = 0;
  virtual void connectScanEnable(const ScanDriver& driver) const = 0;
  virtual void connectScanIn(const ScanDriver& driver) const = 0;
  virtual void connectScanOut(const ScanLoad& load) const = 0;
  virtual ScanLoad getScanEnable() const = 0;
  virtual ScanLoad getScanIn() const = 0;
  virtual ScanDriver getScanOut() const = 0;

  const ClockDomain& getClockDomain() const;
  std::string_view getName() const;

  virtual odb::Point getOrigin() const = 0;
  virtual bool isPlaced() const = 0;

 private:
  std::string name_;
  std::unique_ptr<ClockDomain> clock_domain_;

 protected:
  // Top function to connect either dbBTerms or dbITerms
  void Connect(const ScanLoad& load,
               const ScanDriver& driver,
               bool preserve) const;

  // Generic function to connect dbBTerms and dbITerms together
  // Use: set_debug_level DFT scan_cell_connect 1 for debugging messages
  template <typename Load, typename Driver>
  void Connect(const Load& load, const Driver& driver, bool preserve) const
  {
    debugPrint(logger_,
               utl::DFT,
               "scan_cell_connect",
               1,
               "Cell \"{}\", driver: \"{}\" -> load: \"{}\"",
               name_,
               GetTermName(driver),
               GetTermName(load));
    // Reuse any existing net if we are asked to preserve connections
    if (preserve) {
      odb::dbNet* driver_net = driver->getNet();
      if (driver_net) {
        debugPrint(logger_,
                   utl::DFT,
                   "scan_cell_connect",
                   1,
                   "    Preserving connections 1: Connecting driver net "
                   "\"{}\" -> load pin \"{}\"",
                   driver_net->getName(),
                   GetTermName(load));
        load->connect(driver_net);
        return;
      }
      odb::dbNet* load_net = load->getNet();
      if (load_net) {
        debugPrint(logger_,
                   utl::DFT,
                   "scan_cell_connect",
                   1,
                   "    Preserving connections 2: Connecting driver pin "
                   "\"{}\" -> load net \"{}\"",
                   GetTermName(driver),
                   load_net->getName());
        driver->connect(load_net);
        return;
      }
    }

    odb::dbNet* driver_net = driver->getNet();
    if (!driver_net) {
      driver_net
          = odb::dbNet::create(driver->getBlock(), driver->getName().c_str());
      if (!driver_net) {
        logger_->error(utl::DFT,
                       30,
                       "Failed to create driver net named '{}'",
                       driver->getName());
      }
      driver_net->setSigType(odb::dbSigType::SCAN);
      driver->connect(driver_net);
    }
    debugPrint(logger_,
               utl::DFT,
               "scan_cell_connect",
               1,
               "    New connection: Connecting driver net \"{}\" -> load pin "
               "\"{}\"",
               driver_net->getName(),
               GetTermName(load));
    load->connect(driver_net);
  }

  // Helpers to get the name in a generic way from a BTerm and ITerm
  static const char* GetTermName(odb::dbBTerm* term);
  static const char* GetTermName(odb::dbITerm* term);

  utl::Logger* logger_;
};

}  // namespace dft
