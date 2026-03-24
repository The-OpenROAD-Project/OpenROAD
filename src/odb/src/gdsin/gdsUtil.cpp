// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "odb/gdsUtil.h"

#include <cmath>
#include <cstdint>
#include <ctime>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "odb/db.h"

namespace odb::gds {

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
  if (dataType >= (uint8_t) DataType::kInvalid) {
    throw std::runtime_error("Corrupted GDS, Invalid data type!");
  }
  return static_cast<DataType>(dataType);
}

uint8_t fromDataType(DataType dataType)
{
  if (dataType >= DataType::kInvalid) {
    throw std::runtime_error("Corrupted GDS, Invalid data type!");
  }
  return static_cast<uint8_t>(dataType);
}

double real8_to_double(uint64_t real)
{
  const int64_t exponent = ((real & 0x7F00000000000000) >> 54) - 256;
  const double mantissa
      = ((double) (real & 0x00FFFFFFFFFFFFFF)) / 72057594037927936.0;
  return mantissa * exp2((double) exponent);
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
    const boost::property_tree::ptree& layer = pairs.second;
    std::string name = layer.get<std::string>("name", "");
    const std::string source = layer.get<std::string>("source", "");
    const size_t at_pos = source.find('@');
    const size_t slash_pos = source.find('/');
    if (at_pos == std::string::npos || slash_pos == std::string::npos) {
      throw std::runtime_error("Invalid .lyp file");
    }
    const int16_t layerNum = std::stoi(source.substr(0, slash_pos));
    const int16_t dataType = std::stoi(source.substr(slash_pos + 1, at_pos));
    layerMap[std::make_pair(layerNum, dataType)] = std::move(name);
  }

  return layerMap;
}

dbGDSLib* createEmptyGDSLib(dbDatabase* db, const std::string& libname)
{
  dbGDSLib* lib = dbGDSLib::create(db, libname);
  return lib;
}

}  // namespace odb::gds
