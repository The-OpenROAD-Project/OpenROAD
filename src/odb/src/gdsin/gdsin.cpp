// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "odb/gdsin.h"

#if defined(__APPLE__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/gdsUtil.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb ::gds {

using utl::ODB;

GDSReader::GDSReader(utl::Logger* logger) : logger_(logger)
{
}

dbGDSLib* GDSReader::read_gds(const std::string& filename, dbDatabase* db)
{
  db_ = db;
  file_.open(filename, std::ios::binary);
  if (!file_) {
    logger_->error(ODB, 450, "Could not open file {}", filename);
  }
  readRecord();
  checkRType(RecordType::HEADER);

  processLib();
  if (file_.is_open()) {
    file_.close();
  }
  db_ = nullptr;

  return lib_;
}

void GDSReader::checkRType(RecordType expect)
{
  if (r_.type != expect) {
    logger_->error(ODB,
                   451,
                   "Corrupted GDS, Expected: {} Got: {}",
                   recordNames[static_cast<int>(expect)],
                   recordNames[static_cast<int>(r_.type)]);
  }
}

void GDSReader::checkRData(DataType eType, size_t eSize)
{
  if (r_.dataType != eType) {
    logger_->error(ODB,
                   452,
                   "Corrupted GDS, Expected data type: {} Got: {}",
                   std::to_string((int) eType),
                   std::to_string((int) r_.dataType));
  }
}

double GDSReader::readReal8()
{
  uint64_t value;
  file_.read(reinterpret_cast<char*>(&value), 8);
  return real8_to_double(htobe64(value));
}

int32_t GDSReader::readInt32()
{
  int32_t value;
  file_.read(reinterpret_cast<char*>(&value), 4);
  return htobe32(value);
}

int16_t GDSReader::readInt16()
{
  int16_t value;
  file_.read(reinterpret_cast<char*>(&value), 2);
  return htobe16(value);
}

int8_t GDSReader::readInt8()
{
  int8_t value;
  file_.read(reinterpret_cast<char*>(&value), 1);
  return value;
}

bool GDSReader::readRecord()
{
  const uint16_t recordLength = readInt16();
  const uint8_t recordType = readInt8();
  const DataType dataType = toDataType(readInt8());
  r_.type = toRecordType(recordType);
  r_.dataType = dataType;
  if ((recordLength - 4) % dataTypeSize[(int) dataType] != 0) {
    throw std::runtime_error(
        "Corrupted GDS, Data size is not a multiple of data type size!");
  }
  r_.length = recordLength;
  const int length = recordLength - 4;
  if (dataType == DataType::kInt2) {
    r_.data16.clear();
    for (int i = 0; i < length; i += 2) {
      r_.data16.push_back(readInt16());
    }
  } else if (dataType == DataType::kInt4) {
    r_.data32.clear();
    for (int i = 0; i < length; i += 4) {
      r_.data32.push_back(readInt32());
    }
  } else if (dataType == DataType::kReal8) {
    r_.data64.clear();
    for (int i = 0; i < length; i += 8) {
      r_.data64.push_back(readReal8());
    }
  } else if (dataType == DataType::kAsciiString
             || dataType == DataType::kBitArray) {
    r_.data8.clear();
    for (int i = 0; i < length; i++) {
      r_.data8.push_back(readInt8());
    }
    if (dataType == DataType::kAsciiString && !r_.data8.empty()
        && r_.data8.back() == 0) {
      r_.data8.pop_back();  // Discard null terminator
    }
  }

  return static_cast<bool>(file_);
}

bool GDSReader::processLib()
{
  readRecord();
  checkRType(RecordType::BGNLIB);

  lib_ = dbGDSLib::create(db_, "TEST");  // FIXME

  if (r_.length != 28) {
    throw std::runtime_error(
        "Corrupted GDS, BGNLIB record length is not 28 bytes");
  }

  readRecord();
  checkRType(RecordType::LIBNAME);
  lib_->setLibname(r_.data8);

  readRecord();
  checkRType(RecordType::UNITS);

  lib_->setUnits(r_.data64[0], r_.data64[1]);

  while (readRecord()) {
    if (r_.type == RecordType::ENDLIB) {
      return true;
    }
    if (r_.type == RecordType::BGNSTR) {
      if (!processStruct()) {
        break;
      }
    }
  }

  lib_ = nullptr;
  return false;
}

bool GDSReader::processStruct()
{
  readRecord();
  checkRType(RecordType::STRNAME);

  const std::string name(r_.data8.begin(), r_.data8.end());

  dbGDSStructure* structure = lib_->findGDSStructure(name.c_str());
  if (structure) {
    if (defined_.find(structure) != defined_.end()) {
      throw std::runtime_error("Corrupted GDS, Duplicate structure name");
    }
  } else {
    structure = dbGDSStructure::create(lib_, name.c_str());
  }
  defined_.insert(structure);

  while (readRecord()) {
    if (r_.type == RecordType::ENDSTR) {
      return true;
    }
    if (!processElement(structure)) {
      break;
    }
  }

  return false;
}

std::vector<Point> GDSReader::processXY()
{
  checkRType(RecordType::XY);
  if (r_.data32.size() % 2 != 0) {
    throw std::runtime_error(
        "Corrupted GDS, XY data size is not a multiple of 2");
  }
  std::vector<Point> xy;
  for (int i = 0; i < r_.data32.size(); i += 2) {
    xy.emplace_back(r_.data32[i], r_.data32[i + 1]);
  }
  return xy;
}

template <typename T>
void GDSReader::processPropAttr(T* elem)
{
  while (readRecord()) {
    if (r_.type == RecordType::ENDEL) {
      return;
    }

    checkRType(RecordType::PROPATTR);
    const int16_t attr = r_.data16[0];

    readRecord();
    checkRType(RecordType::PROPVALUE);
    const std::string value = r_.data8;

    if (elem) {
      elem->getPropattr().emplace_back(attr, value);
    }
  }
}

bool GDSReader::processElement(dbGDSStructure* structure)
{
  switch (r_.type) {
    case RecordType::BOUNDARY: {
      auto el = processBoundary(structure);
      processPropAttr(el);
      break;
    }
    case RecordType::SREF: {
      auto el = processSRef(structure);
      processPropAttr(el);
      break;
    }
    case RecordType::AREF: {
      auto el = processARef(structure);
      processPropAttr(el);
      break;
    }
    case RecordType::PATH: {
      auto el = processPath(structure);
      processPropAttr(el);
      break;
    }
    case RecordType::TEXT: {
      auto el = processText(structure);
      processPropAttr(el);
      break;
    }
    case RecordType::BOX: {
      auto el = processBox(structure);
      processPropAttr(el);
      break;
    }
    case RecordType::NODE: {
      // Ignored - parse and discard
      processNode();
      processPropAttr<dbGDSBox>(nullptr);
      break;
    }
    default: {
      throw std::runtime_error("Unimplemented GDS Record Type");
      break;
    }
  }

  checkRType(RecordType::ENDEL);

  return true;
}

dbGDSPath* GDSReader::processPath(dbGDSStructure* structure)
{
  auto* path = dbGDSPath::create(structure);

  readRecord();
  checkRType(RecordType::LAYER);

  path->setLayer(r_.data16[0]);

  readRecord();
  checkRType(RecordType::DATATYPE);

  path->setDatatype(r_.data16[0]);

  readRecord();
  if (r_.type == RecordType::PATHTYPE) {
    path->setPathType(r_.data16[0]);
    readRecord();
  }

  if (r_.type == RecordType::WIDTH) {
    path->setWidth(r_.data32[0]);
    readRecord();
  }

  path->setXy(processXY());

  return path;
}

dbGDSBoundary* GDSReader::processBoundary(dbGDSStructure* structure)
{
  auto* bdy = dbGDSBoundary::create(structure);

  readRecord();
  checkRType(RecordType::LAYER);
  bdy->setLayer(r_.data16[0]);

  readRecord();
  checkRType(RecordType::DATATYPE);
  bdy->setDatatype(r_.data16[0]);

  readRecord();
  bdy->setXy(processXY());

  return bdy;
}

dbGDSSRef* GDSReader::processSRef(dbGDSStructure* structure)
{
  readRecord();
  checkRType(RecordType::SNAME);

  const std::string name(r_.data8.begin(), r_.data8.end());

  dbGDSStructure* referenced = lib_->findGDSStructure(name.c_str());
  if (!referenced) {
    // Empty structure just to reference not yet defined.
    referenced = dbGDSStructure::create(lib_, name.c_str());
  }

  auto* sref = dbGDSSRef::create(structure, referenced);

  readRecord();
  if (r_.type == RecordType::STRANS) {
    sref->setTransform(processSTrans());
  }

  sref->setOrigin(processXY().at(0));

  return sref;
}

dbGDSARef* GDSReader::processARef(dbGDSStructure* structure)
{
  readRecord();
  checkRType(RecordType::SNAME);

  const std::string name(r_.data8.begin(), r_.data8.end());

  dbGDSStructure* referenced = lib_->findGDSStructure(name.c_str());
  if (!referenced) {
    // Empty structure just to reference not yet defined.
    referenced = dbGDSStructure::create(lib_, name.c_str());
  }

  auto* aref = dbGDSARef::create(structure, referenced);

  readRecord();
  if (r_.type == RecordType::STRANS) {
    aref->setTransform(processSTrans());
  }

  if (r_.type == RecordType::COLROW) {
    aref->setNumColumns(r_.data16[0]);
    aref->setNumRows(r_.data16[1]);
    readRecord();
  }

  std::vector<Point> points = processXY();
  aref->setOrigin(points.at(0));
  aref->setLr(points.at(1));
  aref->setUl(points.at(2));

  return aref;
}

dbGDSText* GDSReader::processText(dbGDSStructure* structure)
{
  auto* text = dbGDSText::create(structure);

  readRecord();
  checkRType(RecordType::LAYER);
  text->setLayer(r_.data16[0]);

  readRecord();
  checkRType(RecordType::TEXTTYPE);
  text->setDatatype(r_.data16[0]);

  readRecord();
  if (r_.type == RecordType::PRESENTATION) {
    text->setPresentation(processTextPres());
    readRecord();
  }

  if (r_.type == RecordType::PATHTYPE) {
    readRecord();  // Ignore PATHTYPE
  }

  if (r_.type == RecordType::WIDTH) {
    // ignored
    readRecord();
  }

  if (r_.type == RecordType::STRANS) {
    text->setTransform(processSTrans());
  }

  text->setOrigin(processXY().at(0));

  readRecord();
  checkRType(RecordType::STRING);
  text->setText(std::string(r_.data8.begin(), r_.data8.end()));

  return text;
}

dbGDSBox* GDSReader::processBox(dbGDSStructure* structure)
{
  auto* box = dbGDSBox::create(structure);

  readRecord();
  checkRType(RecordType::LAYER);
  box->setLayer(r_.data16[0]);

  readRecord();
  checkRType(RecordType::BOXTYPE);
  box->setDatatype(r_.data16[0]);

  readRecord();
  std::vector<Point> points = processXY();
  Rect bounds;
  bounds.mergeInit();
  for (const Point& point : points) {
    bounds.merge(point);
  }
  box->setBounds(bounds);

  return box;
}

void GDSReader::processNode()
{
  readRecord();
  checkRType(RecordType::LAYER);

  readRecord();
  checkRType(RecordType::NODETYPE);

  readRecord();
  processXY();
}

dbGDSSTrans GDSReader::processSTrans()
{
  checkRType(RecordType::STRANS);

  const bool flipX = r_.data8[0] & 0x80;
  // absolute magnification and angle are obsolete and ignored

  readRecord();

  double mag = 1.0;
  if (r_.type == RecordType::MAG) {
    mag = r_.data64[0];
    readRecord();
  }
  double angle = 0.0;
  if (r_.type == RecordType::ANGLE) {
    angle = r_.data64[0];
    readRecord();
  }

  return dbGDSSTrans(flipX, mag, angle);
}

dbGDSTextPres GDSReader::processTextPres()
{
  checkRType(RecordType::PRESENTATION);
  const uint8_t hpres = r_.data8[1] & 0x3;
  const uint8_t vpres = (r_.data8[1] & 0xC) >> 2;

  return dbGDSTextPres((dbGDSTextPres::VPres) vpres,
                       (dbGDSTextPres::HPres) hpres);
}

}  // namespace odb::gds
