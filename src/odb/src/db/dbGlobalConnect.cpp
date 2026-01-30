// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

// Generator Code Begin Cpp
#include "dbGlobalConnect.h"

#include <regex>
#include <string>

#include "dbCore.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "odb/db.h"
// User Code Begin Includes
#include <utility>

#include "dbInst.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "utl/Logger.h"
// User Code End Includes
namespace odb {
template class dbTable<_dbGlobalConnect>;

bool _dbGlobalConnect::operator==(const _dbGlobalConnect& rhs) const
{
  if (region_ != rhs.region_) {
    return false;
  }
  if (net_ != rhs.net_) {
    return false;
  }
  if (inst_pattern_ != rhs.inst_pattern_) {
    return false;
  }
  if (pin_pattern_ != rhs.pin_pattern_) {
    return false;
  }

  return true;
}

bool _dbGlobalConnect::operator<(const _dbGlobalConnect& rhs) const
{
  if (region_ >= rhs.region_) {
    return false;
  }
  if (net_ >= rhs.net_) {
    return false;
  }
  if (inst_pattern_ >= rhs.inst_pattern_) {
    return false;
  }
  if (pin_pattern_ >= rhs.pin_pattern_) {
    return false;
  }

  return true;
}

_dbGlobalConnect::_dbGlobalConnect(_dbDatabase* db)
{
}

dbIStream& operator>>(dbIStream& stream, _dbGlobalConnect& obj)
{
  stream >> obj.region_;
  stream >> obj.net_;
  stream >> obj.inst_pattern_;
  stream >> obj.pin_pattern_;
  // User Code Begin >>
  obj.setupRegex();
  // User Code End >>
  return stream;
}

dbOStream& operator<<(dbOStream& stream, const _dbGlobalConnect& obj)
{
  stream << obj.region_;
  stream << obj.net_;
  stream << obj.inst_pattern_;
  stream << obj.pin_pattern_;
  return stream;
}

void _dbGlobalConnect::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  // User Code Begin collectMemInfo
  info.children["inst_pattern"].add(inst_pattern_);
  info.children["pin_pattern"].add(pin_pattern_);
  // User Code End collectMemInfo
}

////////////////////////////////////////////////////////////////////
//
// dbGlobalConnect - Methods
//
////////////////////////////////////////////////////////////////////

dbRegion* dbGlobalConnect::getRegion() const
{
  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  if (obj->region_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbRegion*) par->region_tbl_->getPtr(obj->region_);
}

dbNet* dbGlobalConnect::getNet() const
{
  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  if (obj->net_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbNet*) par->net_tbl_->getPtr(obj->net_);
}

std::string dbGlobalConnect::getInstPattern() const
{
  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  return obj->inst_pattern_;
}

std::string dbGlobalConnect::getPinPattern() const
{
  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  return obj->pin_pattern_;
}

// User Code Begin dbGlobalConnectPublicMethods

std::vector<dbInst*> dbGlobalConnect::getInsts() const
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;

  const auto mterm_mapping = obj->getMTermMapping();

  std::vector<dbInst*> insts;
  for (dbInst* inst : block->getInsts()) {
    if (obj->appliesTo(inst)) {
      insts.push_back(inst);
    }
  }

  return insts;
}

int dbGlobalConnect::connect(dbInst* inst, bool force)
{
  if (inst->isDoNotTouch()) {
    return 0;
  }

  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  if (!obj->appliesTo(inst)) {
    return 0;
  }

  const auto& [connections, skipped] = obj->connect({inst}, force);
  return connections.size();
}

dbGlobalConnect* dbGlobalConnect::create(dbNet* net,
                                         dbRegion* region,
                                         const std::string& inst_pattern,
                                         const std::string& pin_pattern)
{
  _dbNet* dbnet = (_dbNet*) net;
  utl::Logger* logger = dbnet->getImpl()->getLogger();
  _dbGlobalConnect::testRegex(logger, inst_pattern, "instance");
  _dbGlobalConnect::testRegex(logger, pin_pattern, "pin");

  _dbBlock* block = (_dbBlock*) dbnet->getImpl()->getOwner();
  _dbGlobalConnect* gc = block->global_connect_tbl_->create();

  gc->net_ = dbnet->getOID();
  if (region == nullptr) {
    gc->region_ = 0;
  } else {
    _dbRegion* dbregion = (_dbRegion*) region;
    gc->region_ = dbregion->getOID();
  }

  gc->inst_pattern_ = inst_pattern;
  gc->pin_pattern_ = pin_pattern;

  gc->setupRegex();

  // check if global connect is identical to another
  for (const dbGlobalConnect* check_gc :
       ((dbBlock*) block)->getGlobalConnects()) {
    const _dbGlobalConnect* db_check_gc = (const _dbGlobalConnect*) check_gc;

    if (db_check_gc->getOID() == gc->getOID()) {
      // dont compare with self
      continue;
    }

    if (*db_check_gc == *gc) {
      destroy((dbGlobalConnect*) gc);
      gc = nullptr;
      break;
    }
  }

  return ((dbGlobalConnect*) gc);
}

void dbGlobalConnect::destroy(dbGlobalConnect* gc)
{
  _dbBlock* block = (_dbBlock*) gc->getImpl()->getOwner();
  _dbGlobalConnect* dbgc = (_dbGlobalConnect*) gc;
  dbProperty::destroyProperties(gc);
  block->global_connect_tbl_->destroy(dbgc);
}

dbSet<dbGlobalConnect>::iterator dbGlobalConnect::destroy(
    dbSet<dbGlobalConnect>::iterator& itr)
{
  dbGlobalConnect* g = *itr;
  dbSet<dbGlobalConnect>::iterator next = ++itr;
  destroy(g);
  return next;
}

void _dbGlobalConnect::setupRegex()
{
  inst_regex_
      = std::regex(inst_pattern_, std::regex::optimize | std::regex::nosubs);
}

void _dbGlobalConnect::testRegex(utl::Logger* logger,
                                 const std::string& pattern,
                                 const std::string& type)
{
  try {
    std::regex test(pattern);  // NOLINT(*-unused-local-non-trivial-variable)
  } catch (const std::regex_error&) {
    logger->error(utl::ODB,
                  384,
                  "Invalid regular expression specified the {} pattern: {}",
                  type,
                  pattern);
  }
}

std::map<dbMaster*, std::set<dbMTerm*>> _dbGlobalConnect::getMTermMapping()
{
  const std::regex pin_regex = std::regex(pin_pattern_);

  std::map<dbMaster*, std::set<dbMTerm*>> mapping;

  dbDatabase* db = (dbDatabase*) getImpl()->getDatabase();
  for (dbLib* lib : db->getLibs()) {
    for (dbMaster* master : lib->getMasters()) {
      std::set<dbMTerm*> mterms = getMTermMapping(master, pin_regex);

      if (!mterms.empty()) {
        mapping[master] = mterms;
      }
    }
  }

  return mapping;
}

std::set<dbMTerm*> _dbGlobalConnect::getMTermMapping(
    dbMaster* master,
    const std::regex& pin_regex) const
{
  std::set<dbMTerm*> mterms;
  for (dbMTerm* mterm : master->getMTerms()) {
    if (std::regex_match(mterm->getConstName(), pin_regex)) {
      mterms.insert(mterm);
    }
  }

  return mterms;
}

std::pair<std::set<dbITerm*>, std::set<dbITerm*>> _dbGlobalConnect::connect(
    const std::vector<dbInst*>& insts,
    bool force)
{
  utl::Logger* logger = getImpl()->getLogger();
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbNet* net = odb::dbNet::getNet(block, net_);

  std::set<dbITerm*> iterms;
  std::set<dbITerm*> iterms_skipped;

  if (net->isDoNotTouch()) {
    logger->warn(
        utl::ODB,
        379,
        "{} is marked do not touch, will be skipped for global connections",
        net->getName());
    return {iterms, iterms_skipped};
  }

  const auto mterm_map = getMTermMapping();

  for (dbInst* inst : insts) {
    _dbInst* dbinst = (_dbInst*) inst;
    if (region_ != 0 && region_ != dbinst->region_) {
      continue;
    }

    dbMaster* master = inst->getMaster();

    auto findmaster = mterm_map.find(master);
    if (findmaster == mterm_map.end()) {
      continue;
    }

    for (dbMTerm* mterm : findmaster->second) {
      auto* iterm = inst->getITerm(mterm);

      auto* current_net = iterm->getNet();
      if (current_net == net) {
        // Already connected, nothing to do
        continue;
      }
      if (current_net != nullptr && current_net->isDoNotTouch()) {
        // Connected, but current net is marked do not touch, nothing to do
        if (force) {
          logger->warn(utl::ODB,
                       380,
                       "{} is connected to {} which is marked do not touch, "
                       "this connection will not be modified.",
                       iterm->getName(),
                       current_net->getName());
        }
        continue;
      }
      if (current_net != nullptr && !force) {
        // Already connected, so dont update unless force
        iterms_skipped.insert(iterm);
        continue;
      }
      iterm->connect(net);
      if (net->isSpecial()) {
        iterm->setSpecial();
      }
      iterms.insert(iterm);
    }
  }
  return {iterms, iterms_skipped};
}

bool _dbGlobalConnect::appliesTo(dbInst* inst) const
{
  return std::regex_match(inst->getConstName(), inst_regex_);
}

bool _dbGlobalConnect::needsModification(dbInst* inst) const
{
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbNet* net = odb::dbNet::getNet(block, net_);

  for (dbMTerm* mterm :
       getMTermMapping(inst->getMaster(), std::regex(pin_pattern_))) {
    auto* iterm = inst->getITerm(mterm);

    if (iterm->getNet() != net) {
      return true;
    }
  }
  return false;
}

// User Code End dbGlobalConnectPublicMethods
}  // namespace odb
   // Generator Code End Cpp
