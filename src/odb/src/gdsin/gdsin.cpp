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

  bindAllSRefs();
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

  std::tm lastMT;
  lastMT.tm_year = _r.data16[0];
  lastMT.tm_mon = _r.data16[1];
  lastMT.tm_mday = _r.data16[2];
  lastMT.tm_hour = _r.data16[3];
  lastMT.tm_min = _r.data16[4];
  lastMT.tm_sec = _r.data16[5];

  _lib->set_lastModified(lastMT);

  std::tm lastAT;
  lastAT.tm_year = _r.data16[6];
  lastAT.tm_mon = _r.data16[7];
  lastAT.tm_mday = _r.data16[8];
  lastAT.tm_hour = _r.data16[9];
  lastAT.tm_min = _r.data16[10];
  lastAT.tm_sec = _r.data16[11];

  _lib->set_lastAccessed(lastAT);

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

  if (_lib->findGDSStructure(name.c_str()) != nullptr) {
    throw std::runtime_error("Corrupted GDS, Duplicate structure name");
  }

  dbGDSStructure* str = dbGDSStructure::create(_lib, name.c_str());

  while (readRecord()) {
    if (_r.type == RecordType::ENDSTR) {
      return true;
    }
    if (!processElement(str)) {
      break;
    }
  }

  return false;
}

template <typename T>
bool GDSReader::processXY(T* elem)
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
  elem->setXy(xy);
  return true;
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

    elem->getPropattr().emplace_back(attr, value);
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
    case RecordType::SREF:
    case RecordType::AREF: {
      auto el = processSRef(structure);
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
      auto el = processNode(structure);
      processPropAttr(el);
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
    path->set_pathType(_r.data16[0]);
    readRecord();
  } else {
    path->set_pathType(0);
  }

  if (_r.type == RecordType::WIDTH) {
    path->setWidth(_r.data32[0]);
    readRecord();
  } else {
    path->setWidth(0);
  }

  processXY(path);

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
  processXY(bdy);

  return bdy;
}

dbGDSSRef* GDSReader::processSRef(dbGDSStructure* structure)
{
  auto* sref = dbGDSSRef::create(structure);

  readRecord();
  checkRType(RecordType::SNAME);
  sref->set_sName(std::string(_r.data8.begin(), _r.data8.end()));

  readRecord();
  if (_r.type == RecordType::STRANS) {
    sref->setTransform(processSTrans());
  }

  if (_r.type == RecordType::COLROW) {
    sref->set_colRow({_r.data16[0], _r.data16[1]});
    readRecord();
  } else {
    sref->set_colRow({1, 1});
  }

  processXY(sref);

  return sref;
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
    text->setWidth(_r.data32[0]);
    readRecord();
  }

  if (_r.type == RecordType::STRANS) {
    text->setTransform(processSTrans());
  }

  processXY(text);

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
  processXY(box);

  return box;
}

dbGDSNode* GDSReader::processNode(dbGDSStructure* structure)
{
  auto* elem = dbGDSNode::create(structure);

  readRecord();
  checkRType(RecordType::LAYER);
  elem->setLayer(_r.data16[0]);

  readRecord();
  checkRType(RecordType::NODETYPE);
  elem->setDatatype(_r.data16[0]);

  readRecord();
  processXY(elem);

  return elem;
}

dbGDSSTrans GDSReader::processSTrans()
{
  checkRType(RecordType::STRANS);

  const bool flipX = _r.data8[0] & 0x80;
  const bool absMag = _r.data8[1] & 0x04;
  const bool absAngle = _r.data8[1] & 0x02;

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

  return dbGDSSTrans(flipX, absMag, absAngle, mag, angle);
}

dbGDSTextPres GDSReader::processTextPres()
{
  checkRType(RecordType::PRESENTATION);
  const uint8_t hpres = _r.data8[1] & 0x3;
  const uint8_t vpres = (_r.data8[1] & 0xC) >> 2;

  return dbGDSTextPres((dbGDSTextPres::VPres) vpres,
                       (dbGDSTextPres::HPres) hpres);
}

void GDSReader::bindAllSRefs()
{
  for (auto str : _lib->getGDSStructures()) {
    for (auto sref : str->getGDSSRefs()) {
      dbGDSStructure* ref = _lib->findGDSStructure(sref->get_sName().c_str());
      if (ref == nullptr) {
        throw std::runtime_error("Corrupted GDS, SRef to non-existent struct");
      }
      sref->setStructure(ref);
    }
  }
}

}  // namespace odb::gds
