// SPDX-FileCopyrightText: 2023 The Naja authors
// <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include <fcntl.h>

#include <iostream>
#include <sstream>

#include "NajaIF.h"

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include <cassert>

#include "db.h"
#include "dbTypes.h"
#include "naja_nl_implementation.capnp.h"

using namespace odb;

dbSigType::Value CapnPtoODBType(
    DBImplementation::LibraryImplementation::SNLDesignImplementation::NetType
        type)
{
  switch (type) {
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::
        NetType::STANDARD:
      return dbSigType::Value::SIGNAL;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::
        NetType::ASSIGN0:
      return dbSigType::Value::GROUND;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::
        NetType::ASSIGN1:
      return dbSigType::Value::POWER;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::
        NetType::SUPPLY0:
      return dbSigType::Value::GROUND;
    case DBImplementation::LibraryImplementation::SNLDesignImplementation::
        NetType::SUPPLY1:
      return dbSigType::Value::POWER;
  }
  return dbSigType::Value::SIGNAL;  // LCOV_EXCL_LINE
}

void loadInstance(
    dbModule* design,
    const DBImplementation::LibraryImplementation::SNLDesignImplementation::
        Instance::Reader& instance,
    std::map<size_t, odb::dbModInst*>& instance_map,
    std::map<size_t, std::pair<odb::dbInst*, dbModule*>>& leaf_map)
{
  auto instanceID = instance.getId();
  auto modelReference = instance.getModelReference();
  dbModule* model
      = NajaIF::module_map_[std::make_tuple(modelReference.getDbID(),
                                            modelReference.getLibraryID(),
                                            modelReference.getDesignID())]
            .first;
  if (not model) {
    std::ostringstream reason;
    reason << "cannot deserialize instance: no model found with provided "
              "reference";
    assert(false);
  }
  if (NajaIF::module_map_[std::make_tuple(modelReference.getDbID(),
                                          modelReference.getLibraryID(),
                                          modelReference.getDesignID())]
          .second) {
    dbMaster* master = NajaIF::db_->findMaster(model->getName());
    assert(master != nullptr);
    auto db_inst = dbInst::create(
        NajaIF::block_, master, instance.getName().cStr(), false, design);
    leaf_map[instanceID] = std::pair<dbInst*, dbModule*>(db_inst, model);
  } else {
    dbModInst* modinst
        = dbModInst::create(design, model, instance.getName().cStr());
    // dbModule::copy(model, uniquefied_model, modinst);
    instance_map[instanceID] = modinst;
  }
}

void loadTermReference(
    dbModNet* net,
    const DBImplementation::TermReference::Reader& termReference)
{
  dbModule* design = net->getParent();
  printf("module name: %s\n", design->getName());
  printf("Loading term reference with termID %d and bit %d (%lu)\n",
         termReference.getTermID(),
         termReference.getBit(),
         NajaIF::module2terms_[design->getId()].size());
  dbModBTerm* term = design->findModBTerm(
      NajaIF::module2terms_[design->getId()][termReference.getTermID()]
          .first.c_str());
  // auto term =
  // design->getTerm(NLID::DesignObjectID(termReference.getTermID()));
  // LCOV_EXCL_START
  if (not term) {
    std::ostringstream reason;
    reason << "cannot deserialize term reference: no term found with provided "
              "reference";
    assert(false);
    // throw NLException(reason.str());
  }
  // LCOV_EXCL_STOP
  if (!term->isBusPort()) {
    term->setModNet(net);
  } else {
    dbBusPort* busTerm = term->getBusPort();
    assert(busTerm);
    dbModBTerm* busTermBit
        = busTerm->getBusIndexedElement(termReference.getBit());
    // LCOV_EXCL_START
    if (not busTermBit) {
      std::ostringstream reason;
      reason << "cannot deserialize term reference: no bus term bit found with "
                "provided reference";
      assert(false);
      // throw NLException(reason.str());
    }
    // LCOV_EXCL_STOP
    busTermBit->setModNet(net);
  }
}

void loadInstTermReference(
    dbModNet* net,
    const DBImplementation::InstTermReference::Reader& instTermReference,
    std::map<size_t, odb::dbModInst*>& instance_map,
    std::map<size_t, std::pair<odb::dbInst*, dbModule*>>& leaf_map)
{
  auto instanceID = instTermReference.getInstanceID();
  dbModule* design = net->getParent();
  dbModInst* instance = instance_map[instanceID];
  if (nullptr != instance) {
    // LCOV_EXCL_START
    if (not instance) {
      std::ostringstream reason;
      reason << "cannot deserialize instance term reference, no instance found "
                "with ID ";
      reason << instanceID << " in design " << design->getName();
      assert(false);
      // throw NLException(reason.str());
    }
    // LCOV_EXCL_STOP
    dbModule* model = instance->getMaster();
    auto termID = instTermReference.getTermID();
    dbModBTerm* term
        = model->getModBTerm(termID);  // TODO : check if the ids are the same
    // LCOV_EXCL_START
    if (not term) {
      std::ostringstream reason;
      reason << "cannot deserialize instance " << instance->getName();
      reason << " term reference: no term found with ID ";
      reason << termID << " in model " << model->getName();
      assert(false);
      // throw NLException(reason.str());
    }
    dbModITerm* instTerm = dbModITerm::create(instance, term->getName(), term);
    assert(instTerm);
    instTerm->connect(net);
    return;
  }
  printf("looking for inst id %u\n", instanceID);
  dbInst* leaf = leaf_map[instanceID].first;

  if (nullptr == leaf) {
    assert(false);
  }
}

void loadBusNet(dbModule* module,
                const DBImplementation::LibraryImplementation::
                    SNLDesignImplementation::BusNet::Reader& net,
                std::map<size_t, odb::dbModInst*>& instance_map,
                std::map<size_t, std::pair<odb::dbInst*, dbModule*>>& leaf_map)
{
  if (net.hasBits()) {
    for (auto bitNet : net.getBits()) {
      auto bit = bitNet.getBit();
      std::string bitName
          = std::string(net.getName().cStr()) + "[" + std::to_string(bit) + "]";
      dbModNet* scalarNet = dbModNet::create(
          module, bitName.c_str());  // TOTO - what about type?
      // auto scalarNet = SNLScalarNet::create(design,
      // NLID::DesignObjectID(net.getId()), snlName);
      // scalarNet->setType(CapnPtoSNLNetType(net.getType())); TODO:: make sure
      // to set on term
      size_t topPorts = 0;
      size_t leafPorts = 0;
      if (bitNet.hasComponents()) {
        for (auto componentReference : bitNet.getComponents()) {
          if (componentReference.isInstTermReference()) {
            if (leaf_map.find(
                    componentReference.getInstTermReference().getInstanceID())
                != leaf_map.end()) {
              leafPorts++;
            }
            loadInstTermReference(scalarNet,
                                  componentReference.getInstTermReference(),
                                  instance_map,
                                  leaf_map);
          } else if (componentReference.isTermReference()) {
            if (NajaIF::top_ == module) {
              topPorts++;
              continue;
            }
            loadTermReference(scalarNet, componentReference.getTermReference());
          }
        }
      }
      // assert that all terms connected to scalarNet are at the same hierachy
      for (auto term : scalarNet->getModITerms()) {
        assert(term->getParent()->getParent() == module);
      }
      for (auto term : scalarNet->getModBTerms()) {
        assert(term->getParent() == module);
      }
      for (auto term : scalarNet->getITerms()) {
        assert(term->getInst()->getModule() == module);
      }
      // Second pass for dbNet
      if (topPorts > 0 || leafPorts > 0) {
        dbNet* db_net = dbNet::create(NajaIF::block_, bitName.c_str());
        // if (network_->isPower(net)) {
        //   db_net->setSigType(odb::dbSigType::POWER);
        // }
        // if (network_->isGround(net)) {
        //   db_net->setSigType(odb::dbSigType::GROUND);
        // }
        for (auto componentReference : bitNet.getComponents()) {
          if (componentReference.isInstTermReference()) {
            auto instanceID
                = componentReference.getInstTermReference().getInstanceID();
            if (leaf_map.find(instanceID) == leaf_map.end()) {
              continue;
            }
            dbInst* leaf = leaf_map[instanceID].first;
            dbMaster* master = leaf->getMaster();
            dbMTerm* mterm = master->findMTerm(
                NajaIF::block_,
                NajaIF::module2terms_[leaf_map[instanceID].second->getId()]
                                     [componentReference.getInstTermReference()
                                          .getTermID()]
                                         .first.c_str());
            leaf->getITerm(mterm)->connect(db_net);
          } else if (componentReference.isTermReference()) {
            if (NajaIF::top_ == module) {
              std::string termname;
              if (NajaIF::module2terms_[module->getId()]
                                       [componentReference.getTermReference()
                                            .getTermID()]
                                           .second) {
                termname
                    = std::string(NajaIF::module2terms_[module->getId()]
                                                       [componentReference
                                                            .getTermReference()
                                                            .getTermID()].first
                                                           .c_str())
                      + "["
                      + std::to_string(
                          componentReference.getTermReference().getBit())
                      + "]";
              } else {
                termname = std::string(
                    NajaIF::module2terms_[module->getId()]
                                         [componentReference.getTermReference()
                                              .getTermID()].first
                                             .c_str());
              }
              dbBTerm* bterm = dbBTerm::create(db_net, termname.c_str());
              // const dbIoType io_type = CapnPtoODBDirection(termDirection);
              // bterm->setIoType(io_type);
              //  debugPrint(logger_,
              //             utl::ODB,
              //             "dbReadVerilog",
              //             2,
              //             "makeDbNets created bterm {}",
              //             bterm->getName());
              // dbIoType io_type = staToDb(network_->direction(pin));
              // bterm->setIoType(io_type);
            }
          }
        }
      }
    }
  }
}

void loadScalarNet(
    dbModule* module,
    const DBImplementation::LibraryImplementation::SNLDesignImplementation::
        ScalarNet::Reader& net,
    std::map<size_t, odb::dbModInst*>& instance_map,
    std::map<size_t, std::pair<odb::dbInst*, dbModule*>>& leaf_map)
{
  dbModNet* scalarNet = dbModNet::create(
      module, net.getName().cStr());
  size_t topPorts = 0;
  size_t leafPorts = 0;
  if (net.hasComponents()) {
    for (auto componentReference : net.getComponents()) {
      if (componentReference.isInstTermReference()) {
        if (leaf_map.find(
                componentReference.getInstTermReference().getInstanceID())
            != leaf_map.end()) {
          leafPorts++;
        }
        loadInstTermReference(scalarNet,
                              componentReference.getInstTermReference(),
                              instance_map,
                              leaf_map);
      } else if (componentReference.isTermReference()) {
        if (NajaIF::top_ == module) {
          topPorts++;
          continue;
        }
        loadTermReference(scalarNet, componentReference.getTermReference());
      }
    }
  }
  // assert that all terms connected to scalarNet are at the same hierachy
  for (auto term : scalarNet->getModITerms()) {
    assert(term->getParent()->getParent() == module);
  }
  for (auto term : scalarNet->getModBTerms()) {
    assert(term->getParent() == module);
  }
  for (auto term : scalarNet->getITerms()) {
    assert(term->getInst()->getModule() == module);
  }
  // Second pass for dbNet
  if (topPorts > 0 || leafPorts > 0) {
    dbNet* db_net = dbNet::create(NajaIF::block_, net.getName().cStr());
    for (auto componentReference : net.getComponents()) {
      if (componentReference.isInstTermReference()) {
        auto instanceID
            = componentReference.getInstTermReference().getInstanceID();
        if (leaf_map.find(instanceID) == leaf_map.end()) {
          continue;
        }
        dbInst* leaf = leaf_map[instanceID].first;
        dbMaster* master = leaf->getMaster();
        dbMTerm* mterm = master->findMTerm(
            NajaIF::block_,
            NajaIF::module2terms_[leaf_map[instanceID].second->getId()]
                                 [componentReference.getInstTermReference()
                                      .getTermID()].first
                                     .c_str());
        leaf->getITerm(mterm)->connect(db_net);
      } else if (componentReference.isTermReference()) {
        if (NajaIF::top_ == module) {
          std::string termname;
          if (NajaIF::module2terms_[module->getId()]
                                   [componentReference.getTermReference()
                                        .getTermID()]
                                       .second) {
            termname
                = std::string(
                      NajaIF::module2terms_
                          [module->getId()]
                          [componentReference.getTermReference().getTermID()].first
                              .c_str())
                  + "["
                  + std::to_string(
                      componentReference.getTermReference().getBit())
                  + "]";
          } else {
            termname = std::string(
                NajaIF::module2terms_[module->getId()]
                                     [componentReference.getTermReference()
                                          .getTermID()].first
                                         .c_str());
          }
          dbBTerm* bterm = dbBTerm::create(db_net, termname.c_str());
        }
      }
    }
  }
}

void loadDesignImplementation(
    DBImplementation::Reader db,
    const DBImplementation::LibraryImplementation::Reader&
        libraryImplementation,
    const DBImplementation::LibraryImplementation::SNLDesignImplementation::
        Reader& designImplementation)
{
  std::map<size_t, odb::dbModInst*> instance_map;
  std::map<size_t, std::pair<odb::dbInst*, dbModule*>> leaf_map;
  auto designID = designImplementation.getId();
  dbModule* design
      = NajaIF::module_map_[std::make_tuple(db.getId(),
                                            libraryImplementation.getId(),
                                            designImplementation.getId())]
            .first;
  if (designImplementation.hasInstances()) {
    for (auto instance : designImplementation.getInstances()) {
      loadInstance(design, instance, instance_map, leaf_map);
    }
  }
  if (designImplementation.hasNets()) {
    for (auto net : designImplementation.getNets()) {
      if (net.isScalarNet()) {
        auto scalarNet = net.getScalarNet();
        loadScalarNet(design, scalarNet, instance_map, leaf_map);
      } else if (net.isBusNet()) {
        auto busNet = net.getBusNet();
        loadBusNet(design, busNet, instance_map, leaf_map);
      }
    }
  }
}

void loadLibraryImplementation(
    DBImplementation::Reader db,
    const DBImplementation::LibraryImplementation::Reader&
        libraryImplementation)
{
  auto libraryID = libraryImplementation.getId();
  if (libraryImplementation.hasSnlDesignImplementations()) {
    for (auto designImplementation :
         libraryImplementation.getSnlDesignImplementations()) {
      loadDesignImplementation(
          db, libraryImplementation, designImplementation);
    }
  }
}

void NajaIF::loadImplementation(int fileDescriptor)
{
  ::capnp::ReaderOptions options;
  options.traversalLimitInWords = std::numeric_limits<uint64_t>::max();
  ::capnp::PackedFdMessageReader message(fileDescriptor, options);

  DBImplementation::Reader dbImplementation
      = message.getRoot<DBImplementation>();
  auto dbID = dbImplementation.getId();
  if (dbImplementation.hasLibraryImplementations()) {
    for (auto libraryImplementation :
         dbImplementation.getLibraryImplementations()) {
      loadLibraryImplementation(dbImplementation, libraryImplementation);
    }
  }
}

void NajaIF::loadImplementation(const std::filesystem::path& implementationPath)
{
  // FIXME: verify if file can be opened
  int fd = open(implementationPath.c_str(), O_RDONLY);
  // return
  loadImplementation(fd);
  // Full unquification all modinst to match behavior of OR parser
  for (auto& [key, value] : NajaIF::module_map_) {
    dbModule* model = value.first;
    for (dbModInst* inst : model->getModInsts()) {
      std::string name
          = std::string(model->getName()) + "_" + std::string(inst->getName());
      dbModule* uniqueModel = dbModule::create(NajaIF::block_, name.c_str());
      dbModule::copy(model, uniqueModel, inst);
    }
  }
}