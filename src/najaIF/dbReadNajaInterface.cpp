// SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
//
// SPDX-License-Identifier: Apache-2.0

#include "SNLCapnP.h"

#include <fcntl.h>
#include <sstream>
#include <list>
//#include <boost/asio.hpp>

#include <capnp/message.h>
#include <capnp/serialize-packed.h>

#include "snl_interface.capnp.h"

#include "NajaDumpableProperty.h"

#include "NLUniverse.h"
#include "NLDB.h"
#include "NLException.h"

#include "SNLDesign.h"
#include "SNLScalarTerm.h"
#include "SNLBusTerm.h"
#include "NLLibraryTruthTables.h"

//using boost::asio::ip::tcp;

namespace {

using namespace naja;
using namespace naja::NL;

DBInterface::LibraryType SNLtoCapnPLibraryType(NLLibrary::Type type) {
  switch (type) {
    case NLLibrary::Type::Standard:
      return DBInterface::LibraryType::STANDARD;
    case NLLibrary::Type::Primitives:
      return DBInterface::LibraryType::PRIMITIVES;
    //LCOV_EXCL_START
    case NLLibrary::Type::InDB0:
      throw NLException("Unexpected InDB0 Library type while loading Library");
    //LCOV_EXCL_STOP
  }
  return DBInterface::LibraryType::STANDARD; //LCOV_EXCL_LINE
}

NLLibrary::Type CapnPtoNLLibraryType(DBInterface::LibraryType type) {
  switch (type) {
    case DBInterface::LibraryType::STANDARD:
      return NLLibrary::Type::Standard;
    case DBInterface::LibraryType::PRIMITIVES:
      return  NLLibrary::Type::Primitives;
  }
  return NLLibrary::Type::Standard; //LCOV_EXCL_LINE
}

SNLDesign::Type CapnPtoSNLDesignType(DBInterface::LibraryInterface::DesignType type) {
  switch (type) {
    case DBInterface::LibraryInterface::DesignType::STANDARD:
      return SNLDesign::Type::Standard;
    case DBInterface::LibraryInterface::DesignType::BLACKBOX: 
      return SNLDesign::Type::Blackbox;
    case DBInterface::LibraryInterface::DesignType::PRIMITIVE: 
      return SNLDesign::Type::Primitive;
  }
  return SNLDesign::Type::Standard; //LCOV_EXCL_LINE
}

SNLTerm::Direction CapnPtoSNLDirection(DBInterface::LibraryInterface::DesignInterface::Direction direction) {
  switch (direction) {
    case DBInterface::LibraryInterface::DesignInterface::Direction::INPUT:
      return SNLTerm::Direction::Input;
    case DBInterface::LibraryInterface::DesignInterface::Direction::OUTPUT:
      return SNLTerm::Direction::Output;
    case DBInterface::LibraryInterface::DesignInterface::Direction::INOUT:
      return SNLTerm::Direction::InOut;
  }
  return SNLTerm::Direction::Input; //LCOV_EXCL_LINE
}

SNLParameter::Type CapnPtoSNLParameterType(DBInterface::LibraryInterface::DesignInterface::ParameterType type) {
  switch (type) {
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::DECIMAL:
      return SNLParameter::Type::Decimal;
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::BINARY:
      return SNLParameter::Type::Binary;
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::BOOLEAN:
      return SNLParameter::Type::Boolean;
    case DBInterface::LibraryInterface::DesignInterface::ParameterType::STRING:
      return SNLParameter::Type::String;
  }
  return SNLParameter::Type::Decimal; //LCOV_EXCL_LINE
}

template<typename T> void loadProperties(
  const T& dumpObjectReader,
  NajaObject* object,
  auto& propertiesGetter) {
  for (auto property: propertiesGetter(dumpObjectReader)) {
    auto najaProperty = NajaDumpableProperty::create(object, property.getName());
    for (auto value: property.getValues()) {
      if (value.isText()) {
        najaProperty->addStringValue(value.getText());
      } else if (value.isUint64()) {
        najaProperty->addUInt64Value(value.getUint64());
      }
    }
  }
}

void loadScalarTerm(
    SNLDesign* design,
    const DBInterface::LibraryInterface::DesignInterface::ScalarTerm::Reader& term) {
  auto termID = term.getId();
  auto termDirection = term.getDirection();

  dbModBTerm* modbterm;
  std::string port_name_str = pin_name_string;  // intentionally make copy
  const size_t last_idx = port_name_str.find_last_of('/');
  if (last_idx != std::string::npos) {
    port_name_str = port_name_str.substr(last_idx + 1);
  }
  dbModule* module = modinst->getMaster();
  modbterm = module->findModBTerm(port_name_str.c_str());
  // pass the modbterm into the moditerm creator
  // so that during journalling we keep the moditerm/modbterm correlation
  dbModITerm* moditerm
      = dbModITerm::create(modinst, pin_name_string.c_str(), modbterm);
  //   NLName snlName;
  //   if (term.hasName()) {
  //     snlName = NLName(term.getName());
  //   }
  //   SNLScalarTerm::create(design, NLID::DesignObjectID(termID),
  //   CapnPtoSNLDirection(termDirection), snlName);
}

void loadBusTerm(
  SNLDesign* design,
  const DBInterface::LibraryInterface::DesignInterface::BusTerm::Reader& term) {
  auto termID = term.getId();
  auto termDirection = term.getDirection();
  auto msb = term.getMsb();
  auto lsb = term.getLsb();
  NLName snlName;
  if (term.hasName()) {
    snlName = NLName(term.getName());
  }
  SNLBusTerm::create(design, NLID::DesignObjectID(termID), CapnPtoSNLDirection(termDirection), msb, lsb, snlName);
}

void loadDesignParameter(
  SNLDesign* design,
  const DBInterface::LibraryInterface::DesignInterface::Parameter::Reader& parameter) {
  auto name = parameter.getName();
  auto type = parameter.getType();
  auto value = parameter.getValue();
  SNLParameter::create(design, NLName(name), CapnPtoSNLParameterType(type), value);
}

void loadDesignInterface(
  NLLibrary* library,
  const DBInterface::LibraryInterface::DesignInterface::Reader& designInterface) {
  auto designID = designInterface.getId();
  auto designType = designInterface.getType();
  NLName snlName;
  if (designInterface.hasName()) {
    snlName = NLName(designInterface.getName());
  }
  SNLDesign* snlDesign = SNLDesign::create(library, NLID::DesignID(designID), CapnPtoSNLDesignType(designType), snlName);
   if (designInterface.hasProperties()) {
    auto lambda = [](const DBInterface::LibraryInterface::DesignInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(designInterface, snlDesign, lambda);
  }
  if (designInterface.hasParameters()) {
    for (auto parameter: designInterface.getParameters()) {
      loadDesignParameter(snlDesign, parameter);
    }
  }
  if (designInterface.hasTerms()) {
    for (auto term: designInterface.getTerms()) {
      if (term.isScalarTerm()) {
        auto scalarTerm = term.getScalarTerm();
        loadScalarTerm(snlDesign, scalarTerm);
      } else if (term.isBusTerm()) {
        auto busTerm = term.getBusTerm();
        loadBusTerm(snlDesign, busTerm);
      }
    }
  }
}

void loadLibraryInterface(NajaObject* parent, const DBInterface::LibraryInterface::Reader& libraryInterface) {
  NLLibrary* parentLibrary = nullptr;
  NLDB* parentDB = dynamic_cast<NLDB*>(parent);
  if (not parentDB) {
    parentLibrary = static_cast<NLLibrary*>(parent);
  }
  auto libraryID = libraryInterface.getId();
  auto libraryType = libraryInterface.getType();
  NLName snlName;
  if (libraryInterface.hasName()) {
    snlName = NLName(libraryInterface.getName());
  }
  NLLibrary* snlLibrary = nullptr;
  if (parentDB) {
    snlLibrary = NLLibrary::create(parentDB, NLID::LibraryID(libraryID), CapnPtoNLLibraryType(libraryType), snlName);
  } else {
    snlLibrary = NLLibrary::create(parentLibrary, NLID::LibraryID(libraryID), CapnPtoNLLibraryType(libraryType), snlName);
  }
  if (libraryInterface.hasProperties()) {
    auto lambda = [](const DBInterface::LibraryInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(libraryInterface, snlLibrary, lambda);
  }
  if (libraryInterface.hasDesignInterfaces()) {
    for (auto designInterface: libraryInterface.getDesignInterfaces()) {
      loadDesignInterface(snlLibrary, designInterface);
    } 
  }
  if (libraryInterface.hasLibraryInterfaces()) {
    for (auto subLibraryInterface: libraryInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snlLibrary, subLibraryInterface);
    }
  }
  if (snlLibrary->isPrimitives()) {
    NLLibraryTruthTables::construct(snlLibrary);
  }
}

}

namespace naja { namespace NL {

void SNLCapnP::dumpInterface(const NLDB* snlDB, int fileDescriptor) {
  dumpInterface(snlDB, fileDescriptor, snlDB->getID());
}

void SNLCapnP::dumpInterface(const NLDB* snlDB, int fileDescriptor, NLID::DBID forceDBID) {
  ::capnp::MallocMessageBuilder message;

  DBInterface::Builder db = message.initRoot<DBInterface>();
  db.setId(forceDBID);
  auto lambda = [](DBInterface::Builder& builder, size_t nbProperties) {
    return builder.initProperties(nbProperties);
  };
  dumpProperties(db, snlDB, lambda);

  auto libraries = db.initLibraryInterfaces(snlDB->getLibraries().size());
  size_t id = 0;
  for (auto snlLibrary: snlDB->getLibraries()) {
    auto libraryInterfaceBuilder = libraries[id++];
    dumpLibraryInterface(libraryInterfaceBuilder, snlLibrary);
  }

  if (auto topDesign = snlDB->getTopDesign()) {
    auto designReference = topDesign->getReference();
    auto designReferenceBuilder = db.initTopDesignReference();
    designReferenceBuilder.setDbID(designReference.dbID_);
    designReferenceBuilder.setLibraryID(designReference.getDBDesignReference().libraryID_);
    designReferenceBuilder.setDesignID(designReference.getDBDesignReference().designID_);
  }

  writePackedMessageToFd(fileDescriptor, message);
}

void SNLCapnP::dumpInterface(const NLDB* snlDB, const std::filesystem::path& interfacePath) {
  int fd = open(
    interfacePath.c_str(),
    O_CREAT | O_WRONLY,
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  
  dumpInterface(snlDB, fd);
  close(fd);
}

//Need to find a proper way to test serialization on the wire
//LCOV_EXCL_START
//void SNLCapnP::sendInterface(const NLDB* db, tcp::socket& socket, NLID::DBID forceDBID) {
//  dumpInterface(db, socket.native_handle(), forceDBID);
//}
//
//void SNLCapnP::sendInterface(const NLDB* db, tcp::socket& socket) {
//  sendInterface(db, socket, db->getID());
//}
//
//void SNLCapnP::sendInterface(
//  const NLDB* db,
//  const std::string& ipAddress,
//  uint16_t port) {
//  boost::asio::io_context ioContext;
//  //socket creation
//  tcp::socket socket(ioContext);
//  socket.connect(tcp::endpoint(boost::asio::ip::make_address(ipAddress), port));
//  sendInterface(db, socket);
//}
//LCOV_EXCL_STOP

NLDB* SNLCapnP::loadInterface(int fileDescriptor) {
  ::capnp::PackedFdMessageReader message(fileDescriptor);

  DBInterface::Reader dbInterface = message.getRoot<DBInterface>();
  auto dbID = dbInterface.getId();
  auto universe = NLUniverse::get();
  if (not universe) {
    universe = NLUniverse::create();
  }
  auto snldb = NLDB::create(universe, dbID);
  if (dbInterface.hasProperties()) {
    auto lambda = [](const DBInterface::Reader& reader) {
      return reader.getProperties();
    };
    loadProperties(dbInterface, snldb, lambda);
  }
  
  if (dbInterface.hasLibraryInterfaces()) {
    for (auto libraryInterface: dbInterface.getLibraryInterfaces()) {
      loadLibraryInterface(snldb, libraryInterface);
    }
  }
  if (dbInterface.hasTopDesignReference()) {
    auto designReference = dbInterface.getTopDesignReference();
    auto snlDesignReference =
      NLID::DesignReference(
        designReference.getDbID(),
        designReference.getLibraryID(),
        designReference.getDesignID());
    auto topDesign = NLUniverse::get()->getSNLDesign(snlDesignReference);
    if (not topDesign) {
      //LCOV_EXCL_START
      std::ostringstream reason;
      reason << "cannot deserialize top design: no design found with provided reference";
      throw NLException(reason.str());
      //LCOV_EXCL_STOP
    }
    snldb->setTopDesign(topDesign);
  }
  return snldb;
}

NLDB* SNLCapnP::loadInterface(const std::filesystem::path& interfacePath) {
  //FIXME: verify if file can be opened
  int fd = open(interfacePath.c_str(), O_RDONLY);
  return loadInterface(fd);
}

//LCOV_EXCL_START
//NLDB* SNLCapnP::receiveInterface(tcp::socket& socket) {
//  return loadInterface(socket.native_handle());
//}
//LCOV_EXCL_STOP

//LCOV_EXCL_START
//NLDB* SNLCapnP::receiveInterface(uint16_t port) {
//  boost::asio::io_context ioContext;
//  //listen for new connection
//  tcp::acceptor acceptor_(ioContext, tcp::endpoint(tcp::v4(), port));
//  //socket creation 
//  tcp::socket socket(ioContext);
//  //waiting for connection
//  acceptor_.accept(socket);
//  NLDB* db = receiveInterface(socket);
//  return db;
//}
//LCOV_EXCL_STOP

}} // namespace NL // namespace naja
