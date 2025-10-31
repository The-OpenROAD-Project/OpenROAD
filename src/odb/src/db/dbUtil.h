// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/odb.h"
#include "utl/Logger.h"

namespace odb {

namespace dbUtil {

template <typename NetType>
void findBTermDrivers(const NetType* net,
                      std::vector<std::string>& drvr_info_list)
{
  // Find BTerm drivers
  for (dbBTerm* bterm : net->getBTerms()) {
    if (bterm->getSigType().isSupply()) {
      continue;
    }

    if (bterm->getIoType() == dbIoType::INPUT
        || bterm->getIoType() == dbIoType::INOUT) {
      dbBlock* block = bterm->getBlock();
      dbModule* parent_module = block->getTopModule();
      drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
          "\n  - bterm: '{}' (block: '{}', parent_module: '{}')",
          bterm->getName(),
          (block) ? block->getConstName() : "null",
          (parent_module) ? parent_module->getName() : "null"));
    }
  }
}

template <typename NetType>
void findITermDrivers(const NetType* net,
                      std::vector<std::string>& drvr_info_list)
{
  // Find ITerm drivers
  for (dbITerm* iterm : net->getITerms()) {
    if (iterm->getSigType().isSupply()) {
      continue;
    }

    if (iterm->getIoType() == dbIoType::OUTPUT
        || iterm->getIoType() == dbIoType::INOUT) {
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
}

inline void findModBTermDrivers(const dbModNet* net,
                                std::vector<std::string>& drvr_info_list)
{
  // Find ModBTerm drivers
  for (dbModBTerm* modbterm : net->getModBTerms()) {
    if (modbterm->getSigType().isSupply()) {
      continue;
    }

    if (modbterm->getIoType() == dbIoType::INPUT
        || modbterm->getIoType() == dbIoType::INOUT) {
      drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
          "\n  - modbterm: '{}'",
          modbterm->getHierarchicalName()));
    }
  }
}

inline void findModITermDrivers(const dbModNet* net,
                                std::vector<std::string>& drvr_info_list)
{
  // Find ModITerm drivers
  for (dbModITerm* moditerm : net->getModITerms()) {
    if (dbModBTerm* child_bterm = moditerm->getChildModBTerm()) {
      if (child_bterm->getSigType().isSupply()) {
        continue;
      }

      if (child_bterm->getIoType() == dbIoType::OUTPUT
          || child_bterm->getIoType() == dbIoType::INOUT) {
        drvr_info_list.push_back(fmt::format(  // NOLINT(misc-include-cleaner)
            "\n  - moditerm: '{}'",
            moditerm->getHierarchicalName()));
      }
    }
  }
}

template <typename NetType>
void checkNetSanity(const NetType* net,
                    const std::vector<std::string>& drvr_info_list)
{
  utl::Logger* logger = net->getImpl()->getLogger();
  const size_t drvr_count = drvr_info_list.size();

  //
  // 1. Check multiple drivers
  //
  if (drvr_count > 1) {
    if constexpr (std::is_same_v<NetType, dbNet>) {
      logger->warn(
          utl::ODB,
          49,
          "SanityCheck: dbNet '{}' has multiple drivers: {}",
          net->getName(),
          fmt::join(drvr_info_list, ""));  // NOLINT(misc-include-cleaner)
    } else {
      logger->warn(
          utl::ODB,
          481,  // Reusing error code from dbNet
          "SanityCheck: dbModNet '{}' has multiple drivers: {}",
          net->getHierarchicalName(),
          fmt::join(drvr_info_list, ""));  // NOLINT(misc-include-cleaner)
    }
  }

  const uint iterm_count = net->getITerms().size();
  const uint bterm_count = net->getBTerms().size();
  uint term_count;

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
      const uint moditerm_count = net->getModITerms().size();
      if (moditerm_count == 1) {
        dbModITerm* moditerm = *(net->getModITerms().begin());
        if (dbModBTerm* child_bterm = moditerm->getChildModBTerm()) {
          if (child_bterm->getIoType() == dbIoType::OUTPUT) {
            return;  // OK: Unconnected output pin on module instance
          }
        }
      }
      const uint modbterm_count = net->getModBTerms().size();
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

}  // namespace dbUtil

}  // namespace odb
