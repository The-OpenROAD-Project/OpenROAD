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

// init the static variables to nullptr

// NLLibrary::Type CapnPtoNLLibraryType(DBInterface::LibraryType type)
// {
//   switch (type) {
//     case DBInterface::LibraryType::STANDARD:
//       return NLLibrary::Type::Standard;
//     case DBInterface::LibraryType::PRIMITIVES:
//       return NLLibrary::Type::Primitives;
//   }
//   return NLLibrary::Type::Standard;  // LCOV_EXCL_LINE
// }

// SNLDesign::Type CapnPtoODBDesignType(DesignType type)
// {
//   switch (type) {
//     case DesignType::STANDARD:
//       return SNLDesign::Type::Standard;
//     case DesignType::BLACKBOX:
//       return SNLDesign::Type::Blackbox;
//     case DesignType::PRIMITIVE:
//       return SNLDesign::Type::Primitive;
//   }
//   return SNLDesign::Type::Standard;  // LCOV_EXCL_LINE
// }

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

// SNLParameter::Type CapnPtoSNLParameterType(ParameterType type) {
//   switch (type) {
//     case ParameterType::DECIMAL:
//       return SNLParameter::Type::Decimal;
//     case ParameterType::BINARY:
//       return SNLParameter::Type::Binary;
//     case ParameterType::BOOLEAN:
//       return SNLParameter::Type::Boolean;
//     case ParameterType::STRING:
//       return SNLParameter::Type::String;
//   }
//   return SNLParameter::Type::Decimal; //LCOV_EXCL_LINE
// }

// template <typename T>
// void loadProperties(const T& dumpObjectReader,
//                     NajaObject* object,
//                     auto& propertiesGetter)
// {
//   for (auto property : propertiesGetter(dumpObjectReader)) {
//     auto najaProperty
//         = NajaDumpableProperty::create(object, property.getName());
//     for (auto value : property.getValues()) {
//       if (value.isText()) {
//         najaProperty->addStringValue(value.getText());
//       } else if (value.isUint64()) {
//         najaProperty->addUInt64Value(value.getUint64());
//       }
//     }
//   }
// }

void loadScalarTerm(dbModule* mod,
                    const SNLDesignInterface::ScalarTerm::Reader& term)
{
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  dbModBTerm* modbterm = dbModBTerm::create(mod, term.getName().cStr());
  NajaIF::module2terms_[mod->getId()].push_back(term.getName());
}

void loadBusTerm(
    dbModule* mod,
    const SNLDesignInterface::BusTerm::Reader& term)
{
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
  NajaIF::module2terms_[mod->getId()].push_back(term.getName());
  // NLName snlName;
  // if (term.hasName()) {
  //   snlName = NLName(term.getName());
  // }
  // SNLBusTerm::create(design,
  //                    NLID::DesignObjectID(termID),
  //                    CapnPtoODBDirection(termDirection),
  //                    msb,
  //                    lsb,
  //                    snlName);
}

// void loadDesignParameter(
//   SNLDesign* design,
//   const DBInterface::LibraryInterface::DesignInterface::Parameter::Reader&
//   parameter) { auto name = parameter.getName(); auto type =
//   parameter.getType(); auto value = parameter.getValue();
//   SNLParameter::create(design, NLName(name), CapnPtoSNLParameterType(type),
//   value);
// }

void loadDesignInterface(
    const DBInterface::Reader& dbInterface,
    const DBInterface::LibraryInterface::Reader& libraryInterface,
    const SNLDesignInterface::Reader& designInterface)
{
  auto designID = designInterface.getId();
  auto designType = designInterface.getType();
  // NLName snlName;
  // if (designInterface.hasName()) {
  //   snlName = NLName(designInterface.getName());
  // }
  dbModule* mod = dbModule::create(NajaIF::block_, designInterface.getName().cStr());
  NajaIF::module_map_[std::make_tuple(
      dbInterface.getId(), libraryInterface.getId(), designID)] = std::pair<dbModule*, bool>(mod, 
        designInterface.getType() == DesignType::PRIMITIVE ? true : false);
  // dbModule* module = dbModule::makeUniqueDbModule(
  //       designInterface.getName().cStr(), network_->name(inst), block_);
  /*SNLDesign* snlDesign = SNLDesign::create(library,
                                           NLID::DesignID(designID),
                                           CapnPtoSNLDesignType(designType),
                                           snlName);*/
  // if (designInterface.hasProperties()) {
  //   auto lambda
  //       = [](const SNLDesignInterface::Reader&
  //                reader) { return reader.getProperties(); };
  //   loadProperties(designInterface, snlDesign, lambda);
  // }
  // if (designInterface.hasParameters()) {
  //   for (auto parameter: designInterface.getParameters()) {
  //     loadDesignParameter(snlDesign, parameter);
  //   }
  // }
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
}

void loadLibraryInterface(
    //NajaObject* parent,
    const DBInterface::Reader& dbInterface,
    const DBInterface::LibraryInterface::Reader& libraryInterface)
{
  // NLLibrary* parentLibrary = nullptr;
  // NLDB* parentDB = dynamic_cast<NLDB*>(parent);
  // if (not parentDB) {
  //   parentLibrary = static_cast<NLLibrary*>(parent);
  // }
  // auto libraryID = libraryInterface.getId();
  // auto libraryType = libraryInterface.getType();
  // NLName snlName;
  // if (libraryInterface.hasName()) {
  //   snlName = NLName(libraryInterface.getName());
  // }
  // NLLibrary* snlLibrary = nullptr;
  // if (parentDB) {
  //   snlLibrary = NLLibrary::create(parentDB,
  //                                  NLID::LibraryID(libraryID),
  //                                  CapnPtoNLLibraryType(libraryType),
  //                                  snlName);
  // } else {
  //   snlLibrary = NLLibrary::create(parentLibrary,
  //                                  NLID::LibraryID(libraryID),
  //                                  CapnPtoNLLibraryType(libraryType),
  //                                  snlName);
  // }
  // if (libraryInterface.hasProperties()) {
  //   auto lambda = [](const DBInterface::LibraryInterface::Reader& reader) {
  //     return reader.getProperties();
  //   };
  //   loadProperties(libraryInterface, snlLibrary, lambda);
  // }
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
  // if (snlLibrary->isPrimitives()) {
  //   NLLibraryTruthTables::construct(snlLibrary);
  // }
}

//}  // namespace


// void NajaIF::dumpInterface(const NLDB* snlDB, int fileDescriptor)
// {
//   dumpInterface(snlDB, fileDescriptor, snlDB->getID());
// }

// void NajaIF::dumpInterface(const NLDB* snlDB,
//                            int fileDescriptor,
//                            uint8_t forceDBID)
// {
//   ::capnp::MallocMessageBuilder message;

//   DBInterface::Builder db = message.initRoot<DBInterface>();
//   db.setId(forceDBID);
//   auto lambda = [](DBInterface::Builder& builder, size_t nbProperties) {
//     return builder.initProperties(nbProperties);
//   };
//   dumpProperties(db, snlDB, lambda);

//   auto libraries = db.initLibraryInterfaces(snlDB->getLibraries().size());
//   size_t id = 0;
//   for (auto snlLibrary : snlDB->getLibraries()) {
//     auto libraryInterfaceBuilder = libraries[id++];
//     dumpLibraryInterface(libraryInterfaceBuilder, snlLibrary);
//   }

//   if (auto topDesign = snlDB->getTopDesign()) {
//     auto designReference = topDesign->getReference();
//     auto designReferenceBuilder = db.initTopDesignReference();
//     designReferenceBuilder.setDbID(designReference.dbID_);
//     designReferenceBuilder.setLibraryID(
//         designReference.getDBDesignReference().libraryID_);
//     designReferenceBuilder.setDesignID(
//         designReference.getDBDesignReference().designID_);
//   }

//   writePackedMessageToFd(fileDescriptor, message);
// }

// void NajaIF::dumpInterface(const NLDB* snlDB,
//                            const std::filesystem::path& interfacePath)
// {
//   int fd = open(interfacePath.c_str(),
//                 O_CREAT | O_WRONLY,
//                 S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

//   dumpInterface(snlDB, fd);
//   close(fd);
// }

// Need to find a proper way to test serialization on the wire
// LCOV_EXCL_START
// void NajaIF::sendInterface(const NLDB* db, tcp::socket& socket, uint8_t
// forceDBID) {
//   dumpInterface(db, socket.native_handle(), forceDBID);
// }
//
// void NajaIF::sendInterface(const NLDB* db, tcp::socket& socket) {
//   sendInterface(db, socket, db->getID());
// }
//
// void NajaIF::sendInterface(
//   const NLDB* db,
//   const std::string& ipAddress,
//   uint16_t port) {
//   boost::asio::io_context ioContext;
//   //socket creation
//   tcp::socket socket(ioContext);
//   socket.connect(tcp::endpoint(boost::asio::ip::make_address(ipAddress),
//   port)); sendInterface(db, socket);
// }
// LCOV_EXCL_STOP

void NajaIF::loadInterface(int fileDescriptor)
{
  makeBlock();
  ::capnp::PackedFdMessageReader message(fileDescriptor);
  DBInterface::Reader dbInterface = message.getRoot<DBInterface>();
  // auto dbID = dbInterface.getId();
  // auto universe = NLUniverse::get();
  // if (not universe) {
  //   universe = NLUniverse::create();
  // }
  // auto snldb = NLDB::create(universe, dbID);
  // if (dbInterface.hasProperties()) {
  //   auto lambda = [](const DBInterface::Reader& reader) {
  //     return reader.getProperties();
  //   };
  //   loadProperties(dbInterface, snldb, lambda);
  // }

  if (dbInterface.hasLibraryInterfaces()) {
    for (auto libraryInterface : dbInterface.getLibraryInterfaces()) {
      loadLibraryInterface(dbInterface,/*snldb,*/ libraryInterface);
    }
  }
  if (dbInterface.hasTopDesignReference()) {
    auto top = dbInterface.getTopDesignReference();
    dbModule* topModule = module_map_[std::make_tuple(
        dbInterface.getId(),
        top.getLibraryID(),
        top.getDesignID())].first;
    block_->getName() = topModule->getName();
    // auto snlDesignReference
    //     = NLID::DesignReference(designReference.getDbID(),
    //                             designReference.getLibraryID(),
    //                             designReference.getDesignID());
    // auto topDesign = NLUniverse::get()->getSNLDesign(snlDesignReference);
    // if (not topDesign) {
    //   // LCOV_EXCL_START
    //   std::ostringstream reason;
    //   reason << "cannot deserialize top design: no design found with provided "
    //             "reference";
    //   throw NLException(reason.str());
    //   // LCOV_EXCL_STOP
    // }
    // snldb->setTopDesign(topDesign);
  }
  //return db_;
}

void NajaIF::loadInterface(const std::filesystem::path& interfacePath)
{
  // FIXME: verify if file can be opened
  int fd = open(interfacePath.c_str(), O_RDONLY);
  //return loadInterface(fd);
}

void NajaIF::makeBlock()
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
    //const char* design
    //    = network_->name(network_->cell(network_->topInstance()));
    block_ = dbBlock::create(
        chip, "", db_->getTech(), '/');
  }
  dbTech* tech = db_->getTech();
  block_->setDefUnits(tech->getLefUnits());
  block_->setBusDelimiters('[', ']');
}

// LCOV_EXCL_START
// NLDB* NajaIF::receiveInterface(tcp::socket& socket) {
//   return loadInterface(socket.native_handle());
// }
// LCOV_EXCL_STOP

// LCOV_EXCL_START
// NLDB* NajaIF::receiveInterface(uint16_t port) {
//   boost::asio::io_context ioContext;
//   //listen for new connection
//   tcp::acceptor acceptor_(ioContext, tcp::endpoint(tcp::v4(), port));
//   //socket creation
//   tcp::socket socket(ioContext);
//   //waiting for connection
//   acceptor_.accept(socket);
//   NLDB* db = receiveInterface(socket);
//   return db;
// }
// LCOV_EXCL_STOP
