///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

// Generator Code Begin Cpp
#include "dbGlobalConnect.h"

#include "dbDatabase.h"
#include "dbDiff.hpp"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"

// User Code Begin Includes
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

void _dbGlobalConnect::differences(dbDiff& diff,
                                   const char* field,
                                   const _dbGlobalConnect& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(region_);
  DIFF_FIELD(net_);
  DIFF_FIELD(inst_pattern_);
  DIFF_FIELD(pin_pattern_);
  DIFF_END
}

void _dbGlobalConnect::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(region_);
  DIFF_OUT_FIELD(net_);
  DIFF_OUT_FIELD(inst_pattern_);
  DIFF_OUT_FIELD(pin_pattern_);

  DIFF_END
}

_dbGlobalConnect::_dbGlobalConnect(_dbDatabase* db)
{
}

_dbGlobalConnect::_dbGlobalConnect(_dbDatabase* db, const _dbGlobalConnect& r)
{
  region_ = r.region_;
  net_ = r.net_;
  inst_pattern_ = r.inst_pattern_;
  pin_pattern_ = r.pin_pattern_;
  // User Code Begin CopyConstructor
  setupRegex();
  // User Code End CopyConstructor
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
  return (dbRegion*) par->_region_tbl->getPtr(obj->region_);
}

dbNet* dbGlobalConnect::getNet() const
{
  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  if (obj->net_ == 0) {
    return nullptr;
  }
  _dbBlock* par = (_dbBlock*) obj->getOwner();
  return (dbNet*) par->_net_tbl->getPtr(obj->net_);
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

  std::vector<dbInst*> insts;
  for (dbInst* inst : block->getInsts()) {
    if (obj->appliesTo(inst)) {
      insts.push_back(inst);
    }
  }

  return insts;
}

int dbGlobalConnect::connect(dbInst* inst)
{
  if (inst->isDoNotTouch()) {
    return 0;
  }

  _dbGlobalConnect* obj = (_dbGlobalConnect*) this;
  if (!obj->appliesTo(inst)) {
    return 0;
  }

  const auto connections = obj->connect({inst});
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
    auto test = std::regex(pattern);
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
      std::set<dbMTerm*> mterms;
      for (dbMTerm* mterm : master->getMTerms()) {
        if (std::regex_match(mterm->getConstName(), pin_regex)) {
          mterms.insert(mterm);
        }
      }

      if (!mterms.empty()) {
        mapping[master] = mterms;
      }
    }
  }

  return mapping;
}

std::set<dbITerm*> _dbGlobalConnect::connect(const std::vector<dbInst*>& insts)
{
  utl::Logger* logger = getImpl()->getLogger();
  dbBlock* block = (dbBlock*) getImpl()->getOwner();
  dbNet* net = odb::dbNet::getNet(block, net_);

  if (net->isDoNotTouch()) {
    logger->warn(
        utl::ODB,
        379,
        "{} is marked do not touch, will be skipped for global conenctions",
        net->getName());
    return {};
  }

  const auto mterm_map = getMTermMapping();

  std::set<dbITerm*> iterms;
  for (dbInst* inst : insts) {
    _dbInst* dbinst = (_dbInst*) inst;
    if (region_ != 0 && region_ != dbinst->_region) {
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
      if (current_net != nullptr && current_net->isDoNotTouch()) {
        logger->warn(utl::ODB,
                     380,
                     "{}/{} is connected to {} which is marked do not touch, "
                     "this connection will not be modified.",
                     inst->getName(),
                     mterm->getName(),
                     current_net->getName());
        continue;
      }
      if (current_net == net) {
        continue;
      }
      iterm->connect(net);
      if (net->isSpecial()) {
        iterm->setSpecial();
      }
      iterms.insert(iterm);
    }
  }
  return iterms;
}

bool _dbGlobalConnect::appliesTo(dbInst* inst) const
{
  return std::regex_match(inst->getConstName(), inst_regex_);
}

// User Code End dbGlobalConnectPublicMethods
}  // namespace odb
   // Generator Code End Cpp