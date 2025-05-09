// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbScanInst.h"

#include <string>
#include <utility>

#include "dbDatabase.h"
#include "dbDft.h"
#include "dbScanChain.h"
#include "dbScanList.h"
#include "dbScanPartition.h"
#include "dbScanPin.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"
namespace odb {
template class dbTable<_dbScanInst>;

bool _dbScanInst::operator==(const _dbScanInst& rhs) const
{
  if (bits_ != rhs.bits_) {
    return false;
  }
  if (scan_enable_ != rhs.scan_enable_) {
    return false;
  }
  if (inst_ != rhs.inst_) {
    return false;
  }
  if (scan_clock_ != rhs.scan_clock_) {
    return false;
  }
  if (clock_edge_ != rhs.clock_edge_) {
    return false;
  }

  return true;
}

bool _dbScanInst::operator<(const _dbScanInst& rhs) const
{
  return true;
}

_dbScanInst::_dbScanInst(_dbDatabase* db)
{
  bits_ = 0;
  clock_edge_ = 0;
}

dbIStream& operator>>(dbIStream& stream, _dbScanInst& obj)
{
  stream >> obj.bits_;
  stream >> obj.access_pins_;
  stream >> obj.scan_enable_;
  stream >> obj.inst_;
  stream >> obj.scan_clock_;
  stream >> obj.clock_edge_;
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbScanInst& obj)
{
  stream << obj.bits_;
  stream << obj.access_pins_;
  stream << obj.scan_enable_;
  stream << obj.inst_;
  stream << obj.scan_clock_;
  stream << obj.clock_edge_;
  return stream;
}

void _dbScanInst::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);
}

////////////////////////////////////////////////////////////////////
//
// dbScanInst - Methods
//
////////////////////////////////////////////////////////////////////

// User Code Begin dbScanInstPublicMethods
void dbScanInst::setScanClock(std::string_view scan_clock)
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  scan_inst->scan_clock_ = scan_clock;
}

const std::string& dbScanInst::getScanClock() const
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  return scan_inst->scan_clock_;
}

void dbScanInst::setClockEdge(ClockEdge clock_edge)
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  scan_inst->clock_edge_ = static_cast<uint>(clock_edge);
}

dbScanInst::ClockEdge dbScanInst::getClockEdge() const
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  return static_cast<ClockEdge>(scan_inst->clock_edge_);
}

void dbScanInst::setBits(uint bits)
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  scan_inst->bits_ = bits;
}

uint dbScanInst::getBits() const
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  return scan_inst->bits_;
}

void dbScanInst::setScanEnable(dbBTerm* scan_enable)
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  _dbScanChain* scan_chain = (_dbScanChain*) scan_inst->getOwner();
  dbDft* dft = (dbDft*) scan_chain->getOwner();
  scan_inst->scan_enable_ = dbScanPin::create(dft, scan_enable);
}

void dbScanInst::setScanEnable(dbITerm* scan_enable)
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  _dbScanList* scan_list = (_dbScanList*) scan_inst->getOwner();
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  dbDft* dft = (dbDft*) scan_chain->getOwner();
  scan_inst->scan_enable_ = dbScanPin::create(dft, scan_enable);
}

std::variant<dbBTerm*, dbITerm*> dbScanInst::getScanEnable() const
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  _dbScanList* scan_list = (_dbScanList*) scan_inst->getOwner();
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  _dbDft* dft = (_dbDft*) scan_chain->getOwner();
  const dbScanPin* scan_enable = (dbScanPin*) dft->scan_pins_->getPtr(
      (dbId<_dbScanPin>) scan_inst->scan_enable_);
  return scan_enable->getPin();
}

std::string_view getName(odb::dbBTerm* bterm)
{
  return bterm->getConstName();
}

std::string_view getName(odb::dbITerm* iterm)
{
  return iterm->getMTerm()->getConstName();
}

void dbScanInst::setAccessPins(const AccessPins& access_pins)
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  _dbScanList* scan_list = (_dbScanList*) scan_inst->getOwner();
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  dbDft* dft = (dbDft*) scan_chain->getOwner();

  std::visit(
      [&access_pins, scan_inst, dft](auto&& scan_in_pin) {
        const dbId<dbScanPin> scan_in = dbScanPin::create(dft, scan_in_pin);
        std::visit(
            [scan_inst, dft, &scan_in](auto&& scan_out_pin) {
              const dbId<dbScanPin> scan_out
                  = dbScanPin::create(dft, scan_out_pin);
              scan_inst->access_pins_ = std::make_pair(scan_in, scan_out);
            },
            access_pins.scan_out);
      },
      access_pins.scan_in);
}

dbScanInst::AccessPins dbScanInst::getAccessPins() const
{
  AccessPins access_pins;
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  _dbScanList* scan_list = (_dbScanList*) scan_inst->getOwner();
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  _dbDft* dft = (_dbDft*) scan_chain->getOwner();

  const auto& [scan_in_id, scan_out_id] = scan_inst->access_pins_;

  dbScanPin* scan_in
      = (dbScanPin*) dft->scan_pins_->getPtr((dbId<_dbScanPin>) scan_in_id);
  dbScanPin* scan_out
      = (dbScanPin*) dft->scan_pins_->getPtr((dbId<_dbScanPin>) scan_out_id);

  access_pins.scan_in = scan_in->getPin();
  access_pins.scan_out = scan_out->getPin();

  return access_pins;
}

dbInst* dbScanInst::getInst() const
{
  _dbScanInst* scan_inst = (_dbScanInst*) this;
  _dbScanList* scan_list = (_dbScanList*) scan_inst->getOwner();
  _dbScanPartition* scan_partition = (_dbScanPartition*) scan_list->getOwner();
  _dbScanChain* scan_chain = (_dbScanChain*) scan_partition->getOwner();
  _dbDft* dft = (_dbDft*) scan_chain->getOwner();
  _dbBlock* block = (_dbBlock*) dft->getOwner();

  return (dbInst*) block->_inst_tbl->getPtr((dbId<_dbInst>) scan_inst->inst_);
}

dbScanInst* dbScanInst::create(dbScanList* scan_list, dbInst* inst)
{
  _dbScanList* obj = (_dbScanList*) scan_list;
  _dbScanInst* scan_inst = (_dbScanInst*) obj->scan_insts_->create();
  scan_inst->inst_ = ((_dbInst*) inst)->getId();

  return (dbScanInst*) scan_inst;
}

// User Code End dbScanInstPublicMethods
}  // namespace odb
   // Generator Code End Cpp
