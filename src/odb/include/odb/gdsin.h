#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstddef>

#include "odb/db.h"
#include "odb/dbMap.h"
#include "odb/odb.h"

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
    TAPECODE, FORMAT, MASK, ENDMASKS, INVALID
  };

  inline RecordType toRecordType(uint8_t recordType)
  {
    if (recordType >= RecordType::INVALID) {
      throw std::runtime_error("Corrupted GDS, Invalid record type!");
    }
    return static_cast<RecordType>(recordType);
  }

  const char* recordNames[INVALID] = {
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
    NO_DATA = 0, BIT_ARRAY, INT_2, INT_4, REAL_4, REAL_8, ASCII_STRING, INVALID
  };

  inline DataType toDataType(uint8_t dataType)
  {
    if (dataType >= DataType::INVALID) {
      throw std::runtime_error("Corrupted GDS, Invalid data type!");
    }
    return static_cast<DataType>(dataType);
  }


  typedef struct
  {
    RecordType type;
    DataType dataType;
    uint16_t length;
    std::vector<std::byte> data;
  } record_t;

 public: 
  GDSReader(const std::string& filename) : lib(nullptr)
  {
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
    if(next_record.type != RecordType::HEADER)
      throw std::runtime_error("Corrupted GDS, missing HEADER !");
  
    processLib();
    return lib;
  }

 private:
  std::ifstream file;
  record_t next_record;
  dbGDSLib* lib;

  bool readRecord(){
    uint32_t recordHeader;
    file.read(reinterpret_cast<char*>(&recordHeader), 4);
    if (!file)
      return false;

    uint16_t recordLength = recordHeader >> 16;
    uint8_t recordType = recordHeader & 0xFF;
    uint8_t dataType = recordHeader & 0xFF00;

    std::vector<char> recordData(recordLength - 2);
    file.read(recordData.data(), recordData.size());
    if (!file)
      throw std::runtime_error("Corrupted GDS, Unexpected end of file!");

    next_record.type = toRecordType(recordType);
    next_record.dataType = toDataType(dataType);
    next_record.length = recordLength;
    next_record.data = std::vector<std::byte>(recordData.begin(), recordData.end());

    return true;
  }

  bool processLib();
};

int main()
{
  try {
    GDSReader reader("example.gds");
    reader.read_gds();
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}

}  // namespace odb