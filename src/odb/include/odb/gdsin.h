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
    HEADER = 0, BGNLIB, LIBNAME, UNITS, ENDLIB, BGNSTR, STRNAME, 
    ENDSTR, BOUNDARY, PATH, SREF, AREF, TEXT, LAYER, DATATYPE, WIDTH,    
    XY, ENDEL, SNAME, COLROW, NODE, TEXTTYPE, PRESENTATION, STRING,
    STRANS, MAG, ANGLE, REFLIBS, FONTS, PATHTYPE, GENERATIONS, ATTRTABLE,
    ELFLAGS, NODETYPE, PROPATTR, PROPVALUE, BOX, BOXTYPE, PLEX, TAPENUM, 
    TAPECODE, FORMAT, MASK, ENDMASKS, INVALID_RT
  };

  inline RecordType toRecordType(uint8_t recordType)
  {
    if (recordType >= RecordType::INVALID_RT) {
      throw std::runtime_error("Corrupted GDS, Invalid record type!");
    }
    return static_cast<RecordType>(recordType);
  }

  const char* recordNames[RecordType::INVALID_RT] = {
      "HEADER",       "BGNLIB",     "LIBNAME",     "UNITS",     "ENDLIB",
      "BGNSTR",       "STRNAME",    "ENDSTR",      "BOUNDARY",  "PATH",
      "SREF",         "AREF",       "TEXT",        "LAYER",     "DATATYPE",
      "WIDTH",        "XY",         "ENDEL",       "SNAME",     "COLROW",
      "NODE",         "TEXTTYPE",   "PRESENTATION", "STRING",   "STRANS",
      "MAG",          "ANGLE",      "REFLIBS",     "FONTS",     "PATHTYPE",
      "GENERATIONS",  "ATTRTABLE",  "ELFLAGS",     "NODETYPE",  "PROPATTR",
      "PROPVALUE",    "BOX",        "BOXTYPE",     "PLEX",      "TAPENUM",
      "TAPECODE",     "FORMAT",     "MASK",        "ENDMASKS"};

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

  double readDouble()
  {
    double value;
    file.read(reinterpret_cast<char*>(&value), 8);
    return htobe64(value);
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
    else if(dataType == DataType::ASCII_STRING){
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
};

}  // namespace odb