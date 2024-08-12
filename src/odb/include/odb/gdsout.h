#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <endian.h>

#include "odb/db.h"
#include "gdsin.h"

namespace odb {

class dbGDSLib;
class dbGDSElement;
class dbGDSBoundary;
class dbGDSPath;
class dbGDSSRef;
class dbGDSStructure;

class GDSWriter
{
 public:
  GDSWriter();

  ~GDSWriter();

  void write_gds(dbGDSLib* lib, const std::string& filename);

 private:
  std::ofstream _file;
  dbGDSLib* _lib;
  dbDatabase* _db;

  void calcRecSize(record_t& r);
  void writeRecord(record_t& r);

  void writeReal8(double real);
  void writeInt32(int32_t i);
  void writeInt16(int16_t i);
  void writeInt8(int8_t i);

  void writeLayer(dbGDSElement* el);
  void writeXY(dbGDSElement* el);
  void writeDataType(dbGDSElement* el);
  void writeEndel();
  void writePropAttr(dbGDSElement* el);

  void writeLib();
  void writeStruct(dbGDSStructure* str);
  void writeElement(dbGDSElement* el);
  void writeBoundary(dbGDSBoundary* bnd);
  void writePath(dbGDSPath* path);
  void writeSRef(dbGDSSRef* sref);
  void writeText(dbGDSText* text);

  void writeSTrans(const dbGDSSTrans& strans);
  void writeTextPres(const dbGDSTextPres& pres);
};
  
} // namespace odb