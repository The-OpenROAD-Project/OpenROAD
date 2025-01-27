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

#include "odb/gdsin.h"

#include <iostream>
#include <vector>

namespace odb ::gds {

using utl::ODB;

GDSReader::GDSReader(utl::Logger* logger) : _logger(logger)
{
}

dbGDSLib* GDSReader::read_gds(const std::string& filename, dbDatabase* db)
{
  _db = db;
  _file.open(filename, std::ios::binary);
  if (!_file) {
    _logger->error(ODB, 450, "Could not open file {}", filename);
  }
  readRecord();
  checkRType(RecordType::HEADER);

  processLib();
  if (_file.is_open()) {
    _file.close();
  }
  _db = nullptr;

  return _lib;
}

void GDSReader::checkRType(RecordType expect)
{
  if (_r.type != expect) {
    _logger->error(ODB,
                   451,
                   "Corrupted GDS, Expected: {} Got: {}",
                   recordNames[static_cast<int>(expect)],
                   recordNames[static_cast<int>(_r.type)]);
  }
}

void GDSReader::checkRData(DataType eType, size_t eSize)
{
  if (_r.dataType != eType) {
    _logger->error(ODB,
                   452,
                   "Corrupted GDS, Expected data type: {} Got: {}",
                   std::to_string((int) eType),
                   std::to_string((int) _r.dataType));
  }
}

double GDSReader::readReal8()
{
  uint64_t value;
  _file.read(reinterpret_cast<char*>(&value), 8);
  return real8_to_double(htobe64(value));
}

int32_t GDSReader::readInt32()
{
  int32_t value;
  _file.read(reinterpret_cast<char*>(&value), 4);
  return htobe32(value);
}

int16_t GDSReader::readInt16()
{
  int16_t value;
  _file.read(reinterpret_cast<char*>(&value), 2);
  return htobe16(value);
}

int8_t GDSReader::readInt8()
{
  int8_t value;
  _file.read(reinterpret_cast<char*>(&value), 1);
  return value;
}

bool GDSReader::readRecord()
{
  const uint16_t recordLength = readInt16();
  const uint8_t recordType = readInt8();
  const DataType dataType = toDataType(readInt8());
  _r.type = toRecordType(recordType);
  _r.dataType = dataType;
  if ((recordLength - 4) % dataTypeSize[(int) dataType] != 0) {
    throw std::runtime_error(
        "Corrupted GDS, Data size is not a multiple of data type size!");
  }
  _r.length = recordLength;
  const int length = recordLength - 4;
  if (dataType == DataType::INT_2) {
    _r.data16.clear();
    for (int i = 0; i < length; i += 2) {
      _r.data16.push_back(readInt16());
    }
  } else if (dataType == DataType::INT_4) {
    _r.data32.clear();
    for (int i = 0; i < length; i += 4) {
      _r.data32.push_back(readInt32());
    }
  } else if (dataType == DataType::REAL_8) {
    _r.data64.clear();
    for (int i = 0; i < length; i += 8) {
      _r.data64.push_back(readReal8());
    }
  } else if (dataType == DataType::ASCII_STRING
             || dataType == DataType::BIT_ARRAY) {
    _r.data8.clear();
    for (int i = 0; i < length; i++) {
      _r.data8.push_back(readInt8());
    }
  }

  return static_cast<bool>(_file);
}

bool GDSReader::processLib()
{
  readRecord();
  checkRType(RecordType::BGNLIB);

  _lib = dbGDSLib::create(_db, "TEST");  // FIXME

  if (_r.length != 28) {
    throw std::runtime_error(
        "Corrupted GDS, BGNLIB record length is not 28 bytes");
  }

  readRecord();
  checkRType(RecordType::LIBNAME);
  _lib->setLibname(_r.data8);

  readRecord();
  checkRType(RecordType::UNITS);

  _lib->setUnits(_r.data64[0], _r.data64[1]);

  while (readRecord()) {
    if (_r.type == RecordType::ENDLIB) {
      return true;
    }
    if (_r.type == RecordType::BGNSTR) {
      if (!processStruct()) {
        break;
      }
    }
  }

  _lib = nullptr;
  return false;
}

bool GDSReader::processStruct()
{
  readRecord();
  checkRType(RecordType::STRNAME);

  const std::string name(_r.data8.begin(), _r.data8.end());

  dbGDSStructure* structure = _lib->findGDSStructure(name.c_str());
  if (structure) {
    if (_defined.find(structure) != _defined.end()) {
      throw std::runtime_error("Corrupted GDS, Duplicate structure name");
    }
  } else {
    structure = dbGDSStructure::create(_lib, name.c_str());
  }
  _defined.insert(structure);

  while (readRecord()) {
    if (_r.type == RecordType::ENDSTR) {
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
  if (_r.data32.size() % 2 != 0) {
    throw std::runtime_error(
        "Corrupted GDS, XY data size is not a multiple of 2");
  }
  std::vector<Point> xy;
  for (int i = 0; i < _r.data32.size(); i += 2) {
    xy.emplace_back(_r.data32[i], _r.data32[i + 1]);
  }
  return xy;
}

template <typename T>
void GDSReader::processPropAttr(T* elem)
{
  while (readRecord()) {
    if (_r.type == RecordType::ENDEL) {
      return;
    }

    checkRType(RecordType::PROPATTR);
    const int16_t attr = _r.data16[0];

    readRecord();
    checkRType(RecordType::PROPVALUE);
    const std::string value = _r.data8;

    if (elem) {
      elem->getPropattr().emplace_back(attr, value);
    }
  }
}

bool GDSReader::processElement(dbGDSStructure* structure)
{
  switch (_r.type) {
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

  path->setLayer(_r.data16[0]);

  readRecord();
  checkRType(RecordType::DATATYPE);

  path->setDatatype(_r.data16[0]);

  readRecord();
  if (_r.type == RecordType::PATHTYPE) {
    path->setPathType(_r.data16[0]);
    readRecord();
  }

  if (_r.type == RecordType::WIDTH) {
    path->setWidth(_r.data32[0]);
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
  bdy->setLayer(_r.data16[0]);

  readRecord();
  checkRType(RecordType::DATATYPE);
  bdy->setDatatype(_r.data16[0]);

  readRecord();
  bdy->setXy(processXY());

  return bdy;
}

dbGDSSRef* GDSReader::processSRef(dbGDSStructure* structure)
{
  readRecord();
  checkRType(RecordType::SNAME);

  const std::string name(_r.data8.begin(), _r.data8.end());

  dbGDSStructure* referenced = _lib->findGDSStructure(name.c_str());
  if (!referenced) {
    // Empty structure just to reference not yet defined.
    referenced = dbGDSStructure::create(_lib, name.c_str());
  }

  auto* sref = dbGDSSRef::create(structure, referenced);

  readRecord();
  if (_r.type == RecordType::STRANS) {
    sref->setTransform(processSTrans());
  }

  sref->setOrigin(processXY().at(0));

  return sref;
}

dbGDSARef* GDSReader::processARef(dbGDSStructure* structure)
{
  readRecord();
  checkRType(RecordType::SNAME);

  const std::string name(_r.data8.begin(), _r.data8.end());

  dbGDSStructure* referenced = _lib->findGDSStructure(name.c_str());
  if (!referenced) {
    // Empty structure just to reference not yet defined.
    referenced = dbGDSStructure::create(_lib, name.c_str());
  }

  auto* aref = dbGDSARef::create(structure, referenced);

  readRecord();
  if (_r.type == RecordType::STRANS) {
    aref->setTransform(processSTrans());
  }

  if (_r.type == RecordType::COLROW) {
    aref->setNumColumns(_r.data16[0]);
    aref->setNumRows(_r.data16[1]);
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
  text->setLayer(_r.data16[0]);

  readRecord();
  checkRType(RecordType::TEXTTYPE);
  text->setDatatype(_r.data16[0]);

  readRecord();
  if (_r.type == RecordType::PRESENTATION) {
    text->setPresentation(processTextPres());
    readRecord();
  }

  if (_r.type == RecordType::PATHTYPE) {
    readRecord();  // Ignore PATHTYPE
  }

  if (_r.type == RecordType::WIDTH) {
    // ignored
    readRecord();
  }

  if (_r.type == RecordType::STRANS) {
    text->setTransform(processSTrans());
  }

  text->setOrigin(processXY().at(0));

  readRecord();
  checkRType(RecordType::STRING);
  text->setText(std::string(_r.data8.begin(), _r.data8.end()));

  return text;
}

dbGDSBox* GDSReader::processBox(dbGDSStructure* structure)
{
  auto* box = dbGDSBox::create(structure);

  readRecord();
  checkRType(RecordType::LAYER);
  box->setLayer(_r.data16[0]);

  readRecord();
  checkRType(RecordType::BOXTYPE);
  box->setDatatype(_r.data16[0]);

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

  const bool flipX = _r.data8[0] & 0x80;
  // absolute magnification and angle are obsolete and ignored

  readRecord();

  double mag = 1.0;
  if (_r.type == RecordType::MAG) {
    mag = _r.data64[0];
    readRecord();
  }
  double angle = 0.0;
  if (_r.type == RecordType::ANGLE) {
    angle = _r.data64[0];
    readRecord();
  }

  return dbGDSSTrans(flipX, mag, angle);
}

dbGDSTextPres GDSReader::processTextPres()
{
  checkRType(RecordType::PRESENTATION);
  const uint8_t hpres = _r.data8[1] & 0x3;
  const uint8_t vpres = (_r.data8[1] & 0xC) >> 2;

  return dbGDSTextPres((dbGDSTextPres::VPres) vpres,
                       (dbGDSTextPres::HPres) hpres);
}

}  // namespace odb::gds
