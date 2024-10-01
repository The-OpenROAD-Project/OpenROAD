///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "odb/gdsUtil.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <ctime>
#include <map>
#include <utility>

#include "../db/dbGDSBoundary.h"
#include "../db/dbGDSBox.h"
#include "../db/dbGDSElement.h"
#include "../db/dbGDSLib.h"
#include "../db/dbGDSNode.h"
#include "../db/dbGDSPath.h"
#include "../db/dbGDSSRef.h"
#include "../db/dbGDSStructure.h"
#include "../db/dbGDSText.h"
#include "odb/db.h"

namespace odb {
namespace gds {

const char* recordNames[(int) RecordType::INVALID_RT]
    = {"HEADER",    "BGNLIB",    "LIBNAME",    "UNITS",        "ENDLIB",
       "BGNSTR",    "STRNAME",   "ENDSTR",     "BOUNDARY",     "PATH",
       "SREF",      "AREF",      "TEXT",       "LAYER",        "DATATYPE",
       "WIDTH",     "XY",        "ENDEL",      "SNAME",        "COLROW",
       "TEXTNODE",  "NODE",      "TEXTTYPE",   "PRESENTATION", "SPACING",
       "STRING",    "STRANS",    "MAG",        "ANGLE",        "UINTEGER",
       "USTRING",   "REFLIBS",   "FONTS",      "PATHTYPE",     "GENERATIONS",
       "ATTRTABLE", "STYPTABLE", "STRTYPE",    "ELFLAGS",      "ELKEY",
       "LINKTYPE",  "LINKKEYS",  "NODETYPE",   "PROPATTR",     "PROPVALUE",
       "BOX",       "BOXTYPE",   "PLEX",       "BGNEXTN",      "ENDEXTN",
       "TAPENUM",   "TAPECODE",  "STRCLASS",   "RESERVED",     "FORMAT",
       "MASK",      "ENDMASKS",  "LIBDIRSIZE", "SRFNAME",      "LIBSECUR"};

// const size_t dataTypeSize[DataType::INVALID_DT] = { 1, 1, 2, 4, 4, 8, 1 };

RecordType toRecordType(uint8_t recordType)
{
  if (recordType >= (uint8_t) RecordType::INVALID_RT) {
    throw std::runtime_error("Corrupted GDS, Invalid record type!");
  }
  return static_cast<RecordType>(recordType);
}

std::string recordTypeToString(RecordType recordType)
{
  if (recordType >= RecordType::INVALID_RT) {
    throw std::runtime_error("Corrupted GDS, Invalid record type!");
  }
  return recordNames[static_cast<uint8_t>(recordType)];
}

uint8_t fromRecordType(RecordType recordType)
{
  if (recordType >= RecordType::INVALID_RT) {
    throw std::runtime_error("Corrupted GDS, Invalid record type!");
  }
  return static_cast<uint8_t>(recordType);
}

DataType toDataType(uint8_t dataType)
{
  if (dataType >= (uint8_t) DataType::INVALID_DT) {
    throw std::runtime_error("Corrupted GDS, Invalid data type!");
  }
  return static_cast<DataType>(dataType);
}

uint8_t fromDataType(DataType dataType)
{
  if (dataType >= DataType::INVALID_DT) {
    throw std::runtime_error("Corrupted GDS, Invalid data type!");
  }
  return static_cast<uint8_t>(dataType);
}

double real8_to_double(uint64_t real)
{
  int64_t exponent = ((real & 0x7F00000000000000) >> 54) - 256;
  double mantissa
      = ((double) (real & 0x00FFFFFFFFFFFFFF)) / 72057594037927936.0;
  double result = mantissa * exp2((double) exponent);
  return result;
}

uint64_t double_to_real8(double value)
{
  if (value == 0) {
    return 0;
  }
  uint8_t u8_1 = 0;
  if (value < 0) {
    u8_1 = 0x80;
    value = -value;
  }
  const double fexp = 0.25 * log2(value);
  double exponent = ceil(fexp);
  if (exponent == fexp) {
    exponent++;
  }
  const uint64_t mantissa = (uint64_t) (value * pow(16, 14 - exponent));
  u8_1 += (uint8_t) (64 + exponent);
  const uint64_t result
      = ((uint64_t) u8_1 << 56) | (mantissa & 0x00FFFFFFFFFFFFFF);
  return result;
}

std::map<std::pair<int16_t, int16_t>, std::string> getLayerMap(
    const std::string& filename)
{
  std::map<std::pair<int16_t, int16_t>, std::string> layerMap;
  boost::property_tree::ptree xml;
  boost::property_tree::read_xml(filename, xml);
  boost::property_tree::ptree layerList;

  layerList = xml.get_child("layer-properties", layerList);
  if (layerList.empty()) {
    throw std::runtime_error("Invalid .lyp file");
  }

  for (auto& pairs : layerList) {
    if (pairs.first != "properties") {
      continue;
    }
    boost::property_tree::ptree& layer = pairs.second;
    std::string name = layer.get<std::string>("name", "");
    std::string source = layer.get<std::string>("source", "");
    size_t at_pos = source.find('@');
    size_t slash_pos = source.find('/');
    if (at_pos == std::string::npos || slash_pos == std::string::npos) {
      throw std::runtime_error("Invalid .lyp file");
    }
    int16_t layerNum = std::stoi(source.substr(0, slash_pos));
    int16_t dataType = std::stoi(source.substr(slash_pos + 1, at_pos));
    layerMap[std::make_pair(layerNum, dataType)] = std::move(name);
  }

  return layerMap;
}

dbGDSLib* createEmptyGDSLib(dbDatabase* db, const std::string& libname)
{
  dbGDSLib* lib = (dbGDSLib*) (new _dbGDSLib((_dbDatabase*) db));
  lib->setLibname(libname);
  stampGDSLib(lib, true);
  return lib;
}

void stampGDSLib(dbGDSLib* lib, bool modified)
{
  time_t now = std::time(nullptr);
  std::tm now_tm = *std::localtime(&now);
  lib->set_lastAccessed(now_tm);
  if (modified) {
    lib->set_lastModified(now_tm);
  }
}

dbGDSStructure* createEmptyGDSStructure(dbGDSLib* lib, const std::string& name)
{
  return dbGDSStructure::create(lib, name.c_str());
}

dbGDSBoundary* createEmptyGDSBoundary(dbDatabase* db)
{
  return (dbGDSBoundary*) (new _dbGDSBoundary((_dbDatabase*) db));
}

dbGDSBox* createEmptyGDSBox(dbDatabase* db)
{
  return (dbGDSBox*) (new _dbGDSBox((_dbDatabase*) db));
}

dbGDSText* createEmptyGDSText(dbDatabase* db)
{
  return (dbGDSText*) (new _dbGDSText((_dbDatabase*) db));
}

dbGDSPath* createEmptyGDSPath(dbDatabase* db)
{
  return (dbGDSPath*) (new _dbGDSPath((_dbDatabase*) db));
}

dbGDSSRef* createEmptyGDSSRef(dbDatabase* db)
{
  return (dbGDSSRef*) (new _dbGDSSRef((_dbDatabase*) db));
}

dbGDSNode* createEmptyGDSNode(dbDatabase* db)
{
  return (dbGDSNode*) (new _dbGDSNode((_dbDatabase*) db));
}

}  // namespace gds
}  // namespace odb
