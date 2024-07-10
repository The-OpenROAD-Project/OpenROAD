#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <endian.h>

#include "odb/db.h"
#include "odb/gdsUtil.h"

namespace odb {

class dbGDSLib;
class dbGDSElement;
class dbGDSBoundary;
class dbGDSPath;
class dbGDSSRef;
class dbGDSStructure;

class GDSReader
{
 public: 
  GDSReader(const std::string& filename);

  ~GDSReader();

  dbGDSLib* read_gds();

 private:
  std::ifstream file;
  record_t r;
  dbGDSLib* lib;
  dbDatabase* db;

  bool checkRType(RecordType expect);

  // expected size is the number of elements of the given type, not the size in bytes
  bool checkRData(DataType eType, size_t eSize);

  double readReal8();

  int32_t readInt32();

  int16_t readInt16();

  int8_t readInt8();

  bool readRecord();

  bool processLib();
  bool processStruct();
  bool processElement(dbGDSStructure& str);
  bool processBoundary(dbGDSStructure& str);
  bool processPath(dbGDSStructure& str);
  bool processSRef(dbGDSStructure& str);

  dbGDSSTrans processSTrans();
};

}  // namespace odb