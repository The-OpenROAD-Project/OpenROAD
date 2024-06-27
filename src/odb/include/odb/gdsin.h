#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <endian.h>

#include "odb/db.h"

namespace odb {

class dbGDSLib;
class dbGDSElement;
class dbGDSBoundary;
class dbGDSPath;
class dbGDSSRef;
class dbGDSStructure;

class GDSReader
{
 private:
  enum RecordType
  {
    HEADER = 0,     BGNLIB = 1,     LIBNAME = 2,      UNITS = 3, 
    ENDLIB = 4,     BGNSTR = 5,     STRNAME = 6 ,     ENDSTR = 7,
    BOUNDARY = 8,   PATH = 9,       SREF = 10,        AREF = 11,
    TEXT = 12,      LAYER = 13,     DATATYPE = 14,    WIDTH = 15,
    XY = 16,        ENDEL = 17,     SNAME = 18,       COLROW = 19,
    TEXTNODE = 20,  NODE = 21,      TEXTTYPE = 22,    PRESENTATION = 23,
    SPACING = 24,   STRING = 25,    STRANS = 26,      MAG = 27,
    ANGLE = 28,     UINTEGER = 29,  USTRING = 30,     REFLIBS = 31,
    FONTS = 32,     PATHTYPE = 33,  GENERATIONS = 34, ATTRTABLE = 35,
    STYPTABLE = 36, STRTYPE = 37,   ELFLAGS = 38,     ELKEY = 39,
    LINKTYPE = 40,  LINKKEYS = 41,  NODETYPE = 42,    PROPATTR = 43,
    PROPVALUE = 44, BOX = 45,       BOXTYPE = 46,     PLEX = 47,
    BGNEXTN = 48,   ENDEXTN = 49,   TAPENUM = 50,     TAPECODE = 51,
    STRCLASS = 52,  RESERVED = 53,  FORMAT = 54,      MASK = 55,
    ENDMASKS = 56,  LIBDIRSIZE = 57, SRFNAME = 58,    LIBSECUR = 59,
    INVALID_RT = 60
  };

  inline RecordType toRecordType(uint8_t recordType)
  {
    if (recordType >= RecordType::INVALID_RT) {
      throw std::runtime_error("Corrupted GDS, Invalid record type!");
    }
    return static_cast<RecordType>(recordType);
  }

  const char* recordNames[RecordType::INVALID_RT] = {
      "HEADER",       "BGNLIB",     "LIBNAME",     "UNITS",  
      "ENDLIB",       "BGNSTR",     "STRNAME",     "ENDSTR",
      "BOUNDARY",     "PATH",       "SREF",        "AREF",
      "TEXT",         "LAYER",      "DATATYPE",    "WIDTH",
      "XY",           "ENDEL",      "SNAME",       "COLROW",
      "TEXTNODE",     "NODE",       "TEXTTYPE",    "PRESENTATION",
      "SPACING",      "STRING",     "STRANS",      "MAG",
      "ANGLE",        "UINTEGER",   "USTRING",     "REFLIBS",
      "FONTS",        "PATHTYPE",   "GENERATIONS", "ATTRTABLE",
      "STYPTABLE",    "STRTYPE",    "ELFLAGS",     "ELKEY",
      "LINKTYPE",     "LINKKEYS",   "NODETYPE",    "PROPATTR",
      "PROPVALUE",    "BOX",        "BOXTYPE",     "PLEX",
      "BGNEXTN",      "ENDEXTN",    "TAPENUM",     "TAPECODE",
      "STRCLASS",     "RESERVED",   "FORMAT",      "MASK",
      "ENDMASKS",     "LIBDIRSIZE", "SRFNAME",     "LIBSECUR"
  };

  enum DataType
  {
    NO_DATA = 0, BIT_ARRAY, INT_2, INT_4, REAL_4, REAL_8, ASCII_STRING, INVALID_DT
  };

  const size_t dataTypeSize[DataType::INVALID_DT] = { 1, 1, 2, 4, 4, 8, 1 };

  inline DataType toDataType(uint8_t dataType)
  {
    if (dataType >= DataType::INVALID_DT) {
      throw std::runtime_error("Corrupted GDS, Invalid data type!");
    }
    return static_cast<DataType>(dataType);
  }


  typedef struct
  {
    RecordType type;
    DataType dataType;
    uint16_t length;
    std::string data8;
    std::vector<int16_t> data16;
    std::vector<int32_t> data32;
    std::vector<double> data64;
  } record_t;

 public: 
  GDSReader(const std::string& filename) : lib(nullptr)
  {
    db = dbDatabase::create();
    file.open(filename, std::ios::binary);
    if (!file) {
      throw std::runtime_error("Could not open file");
    }
  }

  ~GDSReader()
  {
    if (file.is_open()) {
      file.close();
    }
  }

  dbGDSLib* read_gds()
  {
    readRecord();
    checkRType(RecordType::HEADER);
  
    processLib();
    return lib;
  }

 private:
  std::ifstream file;
  record_t r;
  dbGDSLib* lib;
  dbDatabase* db;

  bool checkRType(RecordType expect)
  {
    if (r.type != expect) {
      std::string error_msg = "Corrupted GDS, Expected: " + std::string(recordNames[static_cast<int>(expect)])
       + " Got: " + std::string(recordNames[static_cast<int>(r.type)]);
      throw std::runtime_error(error_msg);
    }
    return true;
  }

  // expected size is the number of elements of the given type, not the size in bytes
  bool checkRData(DataType eType, size_t eSize)
  {
    if (r.dataType != eType) {
      std::string error_msg = "Corrupted GDS, Expected data type: " + std::to_string(eType)
       + " Got: " + std::to_string(r.dataType);
      throw std::runtime_error("Corrupted GDS, Unexpected data type!");
    }
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


  double readDouble()
  {
    uint64_t value;
    file.read(reinterpret_cast<char*>(&value), 8);
    return real8_to_double(htobe64(value));
  }

  uint32_t readInt32()
  {
    int32_t value;
    file.read(reinterpret_cast<char*>(&value), 4);
    return htobe32(value);
  }

  int16_t readInt16()
  {
    int16_t value;
    file.read(reinterpret_cast<char*>(&value), 2);
    return htobe16(value);
  }

  int8_t readInt8()
  {
    int8_t value;
    file.read(reinterpret_cast<char*>(&value), 1);
    return value;
  }

  bool readRecord(){

    uint16_t recordLength = readInt16();
    uint8_t recordType = readInt8();
    uint8_t dataType = readInt8();

    r.type = toRecordType(recordType);
    r.dataType = toDataType(dataType);

    printf("Record Length: %d Record Type: %s Data Type: %d\n", recordLength, recordNames[recordType], dataType);

    if((recordLength-4) % dataTypeSize[r.dataType] != 0){
      throw std::runtime_error("Corrupted GDS, Data size is not a multiple of data type size!");
    }

    r.length = recordLength;
    int length = recordLength - 4;

    if(dataType == DataType::INT_2){
      r.data16.clear();
      for(int i = 0; i < length; i += 2){
        r.data16.push_back(readInt16());
      }
    }
    else if(dataType == DataType::INT_4 || dataType == DataType::REAL_4){
      r.data32.clear();
      for(int i = 0; i < length; i += 4){
        r.data32.push_back(readInt32());
      }
    }
    else if(dataType == DataType::REAL_8){
      r.data64.clear();
      for(int i = 0; i < length; i += 8){
        r.data64.push_back(readDouble());
      }
    }
    else if(dataType == DataType::ASCII_STRING || dataType == DataType::BIT_ARRAY){
      r.data8.clear();
      for(int i = 0; i < length; i++){
        r.data8.push_back(readInt8());
      }
    }
    
    if(file)
      return true;
    else
      return false;
  }

  bool processLib();
  bool processStruct();
  bool processElement(dbGDSStructure& str);
  bool processBoundary(dbGDSStructure& str);
  bool processSRef(dbGDSStructure& str);

  dbGDSSTrans processSTrans();
};

}  // namespace odb