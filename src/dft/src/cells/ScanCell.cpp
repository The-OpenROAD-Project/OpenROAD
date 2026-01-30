// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "ScanCell.hh"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "ClockDomain.hh"
#include "ScanPin.hh"
#include "odb/db.h"
#include "utl/Logger.h"

namespace dft {

ScanCell::ScanCell(const std::string& name,
                   std::unique_ptr<ClockDomain> clock_domain,
                   utl::Logger* logger)
    : name_(name), clock_domain_(std::move(clock_domain)), logger_(logger)
{
}

std::string_view ScanCell::getName() const
{
  return name_;
}

const ClockDomain& ScanCell::getClockDomain() const
{
  return *clock_domain_;
}

void ScanCell::Connect(const ScanLoad& load,
                       const ScanDriver& driver,
                       bool preserve) const
{
  std::visit(
      [this, &driver, preserve](auto&& load_term) {
        std::visit(
            [this, &load_term, preserve](auto&& driver_term) {
              this->Connect(load_term, driver_term, preserve);
            },
            driver.getValue());
      },
      load.getValue());
}

const char* ScanCell::GetTermName(odb::dbBTerm* term)
{
  return term->getConstName();
}

const char* ScanCell::GetTermName(odb::dbITerm* term)
{
  return term->getMTerm()->getConstName();
}

}  // namespace dft
