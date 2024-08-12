#include "odb/db.h"
#include "odb/gdsUtil.h"
#include "odb/xml.h"

namespace odb {

// char* recordNames[RecordType::INVALID_RT] = {
//   "HEADER",       "BGNLIB",     "LIBNAME",     "UNITS",  
//   "ENDLIB",       "BGNSTR",     "STRNAME",     "ENDSTR",
//   "BOUNDARY",     "PATH",       "SREF",        "AREF",
//   "TEXT",         "LAYER",      "DATATYPE",    "WIDTH",
//   "XY",           "ENDEL",      "SNAME",       "COLROW",
//   "TEXTNODE",     "NODE",       "TEXTTYPE",    "PRESENTATION",
//   "SPACING",      "STRING",     "STRANS",      "MAG",
//   "ANGLE",        "UINTEGER",   "USTRING",     "REFLIBS",
//   "FONTS",        "PATHTYPE",   "GENERATIONS", "ATTRTABLE",
//   "STYPTABLE",    "STRTYPE",    "ELFLAGS",     "ELKEY",
//   "LINKTYPE",     "LINKKEYS",   "NODETYPE",    "PROPATTR",
//   "PROPVALUE",    "BOX",        "BOXTYPE",     "PLEX",
//   "BGNEXTN",      "ENDEXTN",    "TAPENUM",     "TAPECODE",
//   "STRCLASS",     "RESERVED",   "FORMAT",      "MASK",
//   "ENDMASKS",     "LIBDIRSIZE", "SRFNAME",     "LIBSECUR"
// };

// const size_t dataTypeSize[DataType::INVALID_DT] = { 1, 1, 2, 4, 4, 8, 1 };

RecordType toRecordType(uint8_t recordType){
  if (recordType >= RecordType::INVALID_RT) {
    throw std::runtime_error("Corrupted GDS, Invalid record type!");
  }
  return static_cast<RecordType>(recordType);
}

uint8_t fromRecordType(RecordType recordType){
  if (recordType >= RecordType::INVALID_RT) {
    throw std::runtime_error("Corrupted GDS, Invalid record type!");
  }
  return static_cast<uint8_t>(recordType);
}

DataType toDataType(uint8_t dataType){
  if (dataType >= DataType::INVALID_DT) {
    throw std::runtime_error("Corrupted GDS, Invalid data type!");
  }
  return static_cast<DataType>(dataType);
}

uint8_t fromDataType(DataType dataType){
  if (dataType >= DataType::INVALID_DT) {
    throw std::runtime_error("Corrupted GDS, Invalid data type!");
  }
  return static_cast<uint8_t>(dataType);
}

double real8_to_double(uint64_t real) {
  int64_t exponent = ((real & 0x7F00000000000000) >> 54) - 256;
  double mantissa = ((double)(real & 0x00FFFFFFFFFFFFFF)) / 72057594037927936.0;
  double result = mantissa * exp2((double)exponent);
  return result;
}

uint64_t double_to_real8(double value) {
  if (value == 0) return 0;
  uint8_t u8_1 = 0;
  if (value < 0) {
      u8_1 = 0x80;
      value = -value;
  }
  const double fexp = 0.25 * log2(value);
  double exponent = ceil(fexp);
  if (exponent == fexp) exponent++;
  const uint64_t mantissa = (uint64_t)(value * pow(16, 14 - exponent));
  u8_1 += (uint8_t)(64 + exponent);
  const uint64_t result = ((uint64_t)u8_1 << 56) | (mantissa & 0x00FFFFFFFFFFFFFF);
  return result;
}

std::map<std::pair<int16_t, int16_t>, std::string> getLayerMap(std::string filename){
  std::map<std::pair<int16_t, int16_t>, std::string> layerMap;
  XML xml;
  xml.parseXML(filename);
  XML* layerList = xml.findChild("layer-properties");
  if (layerList == nullptr) {
    throw std::runtime_error("Invalid .lyp file");
  }

  for (auto& layer : layerList->getChildren()) {
    if(layer.getName() != "properties"){
      continue;
    }
    std::string name = layer.findChild("name")->getValue();
    std::string source = layer.findChild("source")->getValue();
    size_t at_pos = source.find("@");
    size_t slash_pos = source.find("/");
    if (at_pos == std::string::npos || slash_pos == std::string::npos) {
      throw std::runtime_error("Invalid .lyp file");
    }
    int16_t layerNum = std::stoi(source.substr(0, slash_pos));
    int16_t dataType = std::stoi(source.substr(slash_pos + 1, at_pos));
    layerMap[std::make_pair(layerNum, dataType)] = name;
  }

  return layerMap;
}

} // namespace odb