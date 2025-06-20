// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>

#include <list>
#include <sstream>

#include "NajaIF.h"
// #include <boost/asio.hpp>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "db.h"
#include "dbTypes.h"
#include "naja_nl_interface.capnp.h"

// #include "NajaDumpableProperty.h"

// using boost::asio::ip::tcp;

//namespace {

using namespace odb;

odb::dbDatabase* NajaIF::db_ = nullptr;
odb::dbBlock* NajaIF::block_ = nullptr;
odb::dbBlock* NajaIF::top_block_ = nullptr;
std::map<std::tuple<size_t, size_t, size_t>, std::pair<odb::dbModule*, bool>> NajaIF::module_map_;
std::map<size_t, std::vector<std::pair<std::string, bool>>> NajaIF::module2terms_;
odb::dbModule* NajaIF::top_ = nullptr;

dbIoType::Value CapnPtoODBDirection(Direction direction)
{
  switch (direction) {
    case Direction::INPUT:
      return dbIoType::Value::INPUT;
    case Direction::OUTPUT:
      return dbIoType::Value::OUTPUT;
    case Direction::INOUT:
      return dbIoType::Value::INOUT;
  }
  return dbIoType::Value::INPUT;  // LCOV_EXCL_LINE
}


void loadScalarTerm(dbModule* mod,
                    const SNLDesignInterface::ScalarTerm::Reader& term)
{
  NajaIF::module2terms_[mod->getId()].push_back(std::pair<std::string, bool>(term.getName(), false));
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  dbModBTerm* modbterm = dbModBTerm::create(mod, term.getName().cStr());
}

void loadBusTerm(
    dbModule* mod,
    const SNLDesignInterface::BusTerm::Reader& term)
{
  NajaIF::module2terms_[mod->getId()].push_back(std::pair<std::string, bool>(term.getName(), true));
  dbBusPort* dbbusport = nullptr;
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  auto msb = term.getMsb();
  auto lsb = term.getLsb();

  dbModBTerm* bmodterm = dbModBTerm::create(mod, term.getName().cStr());
  dbbusport = dbBusPort::create(mod,
                                    bmodterm,  // the root of the bus port
                                    lsb,
                                    msb);
      bmodterm->setBusPort(dbbusport);
      const dbIoType io_type = CapnPtoODBDirection(termDirection);
      bmodterm->setIoType(io_type);
}

void loadDesignInterface(
    const DBInterface::Reader& dbInterface,
    const DBInterface::LibraryInterface::Reader& libraryInterface,
    const SNLDesignInterface::Reader& designInterface)
{
  auto designID = designInterface.getId();
  auto designType = designInterface.getType();
  dbModule* mod = nullptr;
  if (dbInterface.getTopDesignReference().getLibraryID() == libraryInterface.getId() &&
      dbInterface.getTopDesignReference().getDesignID() == designID) {
    mod = NajaIF::block_->getTopModule();
    NajaIF::top_ = mod;
    // In case of top, we do not create the terms on modules but only as flat dbTerms
    // when iterating on nets. We do still cache their names in module2terms_.
    if (designInterface.hasTerms()) {
      for (auto term : designInterface.getTerms()) {
        if (term.isScalarTerm()) {
          auto scalarTerm = term.getScalarTerm();
          NajaIF::module2terms_[mod->getId()].push_back(std::pair<std::string, bool>(scalarTerm.getName(), false));
        } else if (term.isBusTerm()) {
          NajaIF::module2terms_[mod->getId()].push_back(std::pair<std::string, bool>(term.getBusTerm().getName(), true));
        }
      }
    }
  } else {
    mod = dbModule::create(NajaIF::block_, designInterface.getName().cStr());
    if (designInterface.hasTerms()) {
      for (auto term : designInterface.getTerms()) {
        if (term.isScalarTerm()) {
          auto scalarTerm = term.getScalarTerm();
          loadScalarTerm(mod, scalarTerm);
        } else if (term.isBusTerm()) {
          auto busTerm = term.getBusTerm();
          loadBusTerm(mod, busTerm);
        }
      }
    }
    mod->getModBTerms().reverse();
  }
  NajaIF::module_map_[std::make_tuple(
      dbInterface.getId(), libraryInterface.getId(), designID)] = std::pair<dbModule*, bool>(mod, 
        designInterface.getType() == DesignType::PRIMITIVE ? true : false);
}

void loadLibraryInterface(
    //NajaObject* parent,
    const DBInterface::Reader& dbInterface,
    const DBInterface::LibraryInterface::Reader& libraryInterface)
{
  if (libraryInterface.hasSnlDesignInterfaces()) {
    for (auto designInterface : libraryInterface.getSnlDesignInterfaces()) {
      loadDesignInterface(dbInterface, libraryInterface, designInterface);
    }
  }
  if (libraryInterface.hasLibraryInterfaces()) {
    for (auto subLibraryInterface : libraryInterface.getLibraryInterfaces()) {
      loadLibraryInterface(dbInterface,/*snlLibrary,*/ subLibraryInterface);
    }
  }
}

std::string getTopName(DBInterface::Reader dbInterface) {
  if (dbInterface.hasTopDesignReference()) {
    auto top = dbInterface.getTopDesignReference();
    for (auto libraryInterface : dbInterface.getLibraryInterfaces()) {
      if (libraryInterface.hasSnlDesignInterfaces()) {
        for (auto designInterface : libraryInterface.getSnlDesignInterfaces()) {
          if (designInterface.getId() == top.getDesignID() &&
              libraryInterface.getId() == top.getLibraryID()) {
            // Found the top design reference in the DBInterface
            // Return the name of the design
            return designInterface.getName().cStr();
          }
        }
      }
    }
  }
  assert(false && "Top design reference not found in DBInterface");
  return "error"; 
}

void NajaIF::loadInterface(int fileDescriptor)
{
  ::capnp::PackedFdMessageReader message(fileDescriptor);
  DBInterface::Reader dbInterface = message.getRoot<DBInterface>();
  makeBlock(getTopName(dbInterface));
  if (dbInterface.hasLibraryInterfaces()) {
    for (auto libraryInterface : dbInterface.getLibraryInterfaces()) {
      loadLibraryInterface(dbInterface,/*snldb,*/ libraryInterface);
    }
  }
}

void NajaIF::loadInterface(const std::filesystem::path& interfacePath)
{
  // FIXME: verify if file can be opened
  int fd = open(interfacePath.c_str(), O_RDONLY);
  loadInterface(fd);
}

void NajaIF::makeBlock(const std::string& topName)
{
  dbChip* chip = db_->getChip();
  if (chip == nullptr) {
    chip = dbChip::create(db_);
  }
  block_ = chip->getBlock();
  if (block_) {
    // Delete existing db network objects.
    auto insts = block_->getInsts();
    for (auto iter = insts.begin(); iter != insts.end();) {
      iter = dbInst::destroy(iter);
    }
    auto nets = block_->getNets();
    for (auto iter = nets.begin(); iter != nets.end();) {
      iter = dbNet::destroy(iter);
    }
    auto bterms = block_->getBTerms();
    for (auto iter = bterms.begin(); iter != bterms.end();) {
      iter = dbBTerm::destroy(iter);
    }
    auto mod_insts = block_->getTopModule()->getChildren();
    for (auto iter = mod_insts.begin(); iter != mod_insts.end();) {
      iter = dbModInst::destroy(iter);
    }
  } else {
    block_ = dbBlock::create(
        chip, topName.c_str(), db_->getTech(), '/');
  }
  dbTech* tech = db_->getTech();
  block_->setDefUnits(tech->getLefUnits());
  block_->setBusDelimiters('[', ']');
}
