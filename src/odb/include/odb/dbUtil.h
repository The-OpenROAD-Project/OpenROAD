// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "utl/Logger.h"

namespace odb::dbUtil {

// Find BTerm drivers
template <typename NetType>
void findBTermDrivers(const NetType* net, std::vector<dbObject*>& drvr_vec)
{
  // Find BTerm drivers
  for (dbBTerm* bterm : net->getBTerms()) {
    if (bterm->getSigType().isSupply()) {
      continue;
    }

    if (bterm->getIoType() == dbIoType::INPUT
        || bterm->getIoType() == dbIoType::INOUT) {
      drvr_vec.push_back(bterm);
    }
  }
}

// Find BTerm drivers and format them into strings.
template <typename NetType>
void findBTermDrivers(const NetType* net,
                      std::vector<std::string>& drvr_info_list)
{
  std::vector<dbObject*> drivers;
  findBTermDrivers(net, drivers);
  for (dbObject* driver : drivers) {
    dbBTerm* bterm = static_cast<dbBTerm*>(driver);
    dbBlock* block = bterm->getBlock();
    dbModule* parent_module = block->getTopModule();
    drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
        "\n  - bterm: '{}' (block: '{}', parent_module: '{}')",
        bterm->getName(),
        (block) ? block->getConstName() : "null",
        (parent_module) ? parent_module->getName() : "null"));
  }
}

// Find ITerm drivers
template <typename NetType>
void findITermDrivers(const NetType* net, std::vector<dbObject*>& drvr_vec)
{
  for (dbITerm* iterm : net->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }

    if (iterm->getIoType() == dbIoType::OUTPUT
        || iterm->getIoType() == dbIoType::INOUT) {
      drvr_vec.push_back(iterm);
    }
  }
}

// Find ITerm drivers and format them into strings.
template <typename NetType>
void findITermDrivers(const NetType* net,
                      std::vector<std::string>& drvr_info_list)
{
  std::vector<dbObject*> drivers;
  findITermDrivers(net, drivers);
  for (dbObject* driver : drivers) {
    dbITerm* iterm = static_cast<dbITerm*>(driver);
    dbInst* inst = iterm->getInst();
    dbMaster* master = inst->getMaster();
    dbModule* parent_module = inst->getModule();
    dbBlock* block = inst->getBlock();
    drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
        "\n  - iterm: '{}' (block: '{}', parent_module: '{}', master: '{}')",
        iterm->getName(),
        (block) ? block->getConstName() : "null",
        (parent_module) ? parent_module->getName() : "null",
        (master) ? master->getConstName() : "null"));
  }
}

// Find ModBTerm drivers
inline void findModBTermDrivers(const dbModNet* net,
                                std::vector<dbObject*>& drvr_vec)
{
  for (dbModBTerm* modbterm : net->getModBTerms()) {
    if (modbterm->getSigType().isSupply()) {
      continue;
    }

    if (modbterm->getIoType() == dbIoType::INPUT
        || modbterm->getIoType() == dbIoType::INOUT) {
      drvr_vec.push_back(modbterm);
    }
  }
}

// Find ModBTerm drivers and format them into strings.
inline void findModBTermDrivers(const dbModNet* net,
                                std::vector<std::string>& drvr_info_list)
{
  std::vector<dbObject*> drivers;
  findModBTermDrivers(net, drivers);
  for (dbObject* driver : drivers) {
    dbModBTerm* modbterm = static_cast<dbModBTerm*>(driver);
    drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
        "\n  - modbterm: '{}'",
        modbterm->getHierarchicalName()));
  }
}

// Find ModITerm drivers
inline void findModITermDrivers(const dbModNet* net,
                                std::vector<dbObject*>& drvr_vec)
{
  for (dbModITerm* moditerm : net->getModITerms()) {
    if (dbModBTerm* child_bterm = moditerm->getChildModBTerm()) {
      if (child_bterm->getSigType().isSupply()) {
        continue;
      }

      if (child_bterm->getIoType() == dbIoType::OUTPUT
          || child_bterm->getIoType() == dbIoType::INOUT) {
        drvr_vec.push_back(moditerm);
      }
    }
  }
}

// Find ModITerm drivers and format them into strings.
inline void findModITermDrivers(const dbModNet* net,
                                std::vector<std::string>& drvr_info_list)
{
  std::vector<dbObject*> drivers;
  findModITermDrivers(net, drivers);
  for (dbObject* driver : drivers) {
    dbModITerm* moditerm = static_cast<dbModITerm*>(driver);
    drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
        "\n  - moditerm: '{}'",
        moditerm->getHierarchicalName()));
  }
}

// Check sanity for dbNet or dbModNet
template <typename NetType>
void checkNetSanity(const NetType* net,
                    const std::vector<std::string>& drvr_info_list)
{
  utl::Logger* logger = net->getImpl()->getLogger();
  const size_t drvr_count = drvr_info_list.size();

  std::string net_name = net->getName();
  if (net_name.find("VDD") != std::string::npos
      || net_name.find("VSS") != std::string::npos) {
    return;
  }

  //
  // 1. Check multiple drivers
  //
  if (drvr_count > 1) {
    if constexpr (std::is_same_v<NetType, dbNet>) {
      logger->warn(utl::ODB,
                   49,
                   "SanityCheck: dbNet '{}' has multiple drivers: {}",
                   net->getName(),
                   drvr_info_list.size());  // NOLINT(misc-include-cleaner)
    } else {
      logger->warn(utl::ODB,
                   481,  // Reusing error code from dbNet
                   "SanityCheck: dbModNet '{}' has multiple drivers: {}",
                   net->getHierarchicalName(),
                   drvr_info_list.size());  // NOLINT(misc-include-cleaner)
    }
  }

  const uint32_t iterm_count = net->getITerms().size();
  const uint32_t bterm_count = net->getBTerms().size();
  uint32_t term_count;

  if constexpr (std::is_same_v<NetType, dbNet>) {
    term_count = iterm_count + bterm_count;
  } else {
    term_count = net->connectionCount();
  }

  //
  // 2. Check no driver
  //
  if (drvr_count == 0 && term_count > 0) {
    if constexpr (std::is_same_v<NetType, dbNet>) {
      logger->warn(utl::ODB,
                   50,
                   "SanityCheck: dbNet '{}' has no driver.",
                   net->getName());
    } else {
      logger->warn(utl::ODB,
                   482,
                   "SanityCheck: dbModNet '{}' has no driver.",
                   net->getHierarchicalName());
    }
  }

  //
  // 3. Check dangling nets
  //
  if (term_count < 2) {
    if constexpr (std::is_same_v<NetType, dbNet>) {
      // Skip power/ground net
      if (net->getSigType().isSupply()) {
        return;  // OK: Unconnected power/ground net
      }
    }

    // A net connected to 1 terminal
    if (iterm_count == 1
        && (*(net->getITerms().begin()))->getIoType() == dbIoType::OUTPUT) {
      return;  // OK: Unconnected output pin
    }
    if (bterm_count == 1
        && (*(net->getBTerms().begin()))->getIoType() == dbIoType::INPUT) {
      return;  // OK: Unconnected input port
    }

    if constexpr (std::is_same_v<NetType, dbNet>) {
      logger->warn(utl::ODB,
                   51,
                   "SanityCheck: dbNet '{}' is dangling. It has less than 2 "
                   "connections (# of ITerms = {}, # of BTerms = {}).",
                   net->getName(),
                   iterm_count,
                   bterm_count);
    } else {
      // A net connected to 1 terminal
      const uint32_t moditerm_count = net->getModITerms().size();
      if (moditerm_count == 1) {
        dbModITerm* moditerm = *(net->getModITerms().begin());
        if (dbModBTerm* child_bterm = moditerm->getChildModBTerm()) {
          if (child_bterm->getIoType() == dbIoType::OUTPUT) {
            return;  // OK: Unconnected output pin on module instance
          }
        }
      }
      const uint32_t modbterm_count = net->getModBTerms().size();
      if (modbterm_count == 1) {
        dbModBTerm* modbterm = *(net->getModBTerms().begin());
        if (modbterm->getIoType() == dbIoType::INPUT) {
          return;  // OK: Unconnected input port on module
        }
      }

      logger->warn(utl::ODB,
                   483,
                   "SanityCheck: dbModNet '{}' is dangling. It has less than 2 "
                   "connections (# of ITerms = {}, # of BTerms = {}, # of "
                   "ModITerms = {}, # of ModBTerms = {}).",
                   net->getHierarchicalName(),
                   iterm_count,
                   bterm_count,
                   moditerm_count,
                   modbterm_count);
    }
  }
}

}  // namespace odb::dbUtil
