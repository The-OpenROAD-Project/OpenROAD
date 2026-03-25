// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#include "OdbScanCellAdapter.hh"

#include <memory>
#include <variant>

#include "ClockDomain.hh"
#include "odb/db.h"
#include "odb/geom.h"

namespace dft {

namespace {

odb::Point itermLocation(odb::dbITerm* iterm, odb::dbInst* inst)
{
  if (iterm != nullptr) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return odb::Point(x, y);
    }
  }
  return inst->getOrigin();
}

// Resolve a variant<dbBTerm*, dbITerm*> to a dbITerm* (returns nullptr for
// top-level BTerm pins).
odb::dbITerm* toITerm(const std::variant<odb::dbBTerm*, odb::dbITerm*>& pin)
{
  if (std::holds_alternative<odb::dbITerm*>(pin)) {
    return std::get<odb::dbITerm*>(pin);
  }
  return nullptr;
}

}  // namespace

OdbScanCellAdapter::OdbScanCellAdapter(odb::dbScanInst* scan_inst,
                                       utl::Logger* logger)
    : ScanCell(scan_inst->getInst()->getName(),
               // Dummy clock domain — the optimizer does not use it.
               std::make_unique<ClockDomain>("__opt__", ClockEdge::Rising),
               logger),
      inst_(scan_inst->getInst()),
      bits_(scan_inst->getBits()),
      scan_in_iterm_(toITerm(scan_inst->getAccessPins().scan_in)),
      scan_out_iterm_(toITerm(scan_inst->getAccessPins().scan_out))
{
}

uint64_t OdbScanCellAdapter::getBits() const
{
  return bits_;
}

ScanLoad OdbScanCellAdapter::getScanEnable() const
{
  return ScanLoad(static_cast<odb::dbITerm*>(nullptr));
}

ScanLoad OdbScanCellAdapter::getScanIn() const
{
  return ScanLoad(scan_in_iterm_);
}

ScanDriver OdbScanCellAdapter::getScanOut() const
{
  return ScanDriver(scan_out_iterm_);
}

odb::Point OdbScanCellAdapter::getOrigin() const
{
  return inst_->getOrigin();
}

bool OdbScanCellAdapter::isPlaced() const
{
  return inst_->isPlaced();
}

odb::Point OdbScanCellAdapter::getScanInLocation() const
{
  return itermLocation(scan_in_iterm_, inst_);
}

odb::Point OdbScanCellAdapter::getScanOutLocation() const
{
  return itermLocation(scan_out_iterm_, inst_);
}

odb::dbITerm* OdbScanCellAdapter::getScanInITerm() const
{
  return scan_in_iterm_;
}

odb::dbITerm* OdbScanCellAdapter::getScanOutITerm() const
{
  return scan_out_iterm_;
}

}  // namespace dft
