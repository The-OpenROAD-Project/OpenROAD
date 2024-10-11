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

#include "odb/gdsout.h"

#include <iostream>

namespace odb::gds {

using utl::ODB;

GDSWriter::GDSWriter(utl::Logger* logger) : _logger(logger)
{
}

void GDSWriter::write_gds(dbGDSLib* lib, const std::string& filename)
{
  _lib = lib;
  _file.open(filename, std::ios::binary);
  if (!_file) {
    _logger->error(ODB, 447, "Could not open file {}", filename);
  }
  writeLib();
  if (_file.is_open()) {
    _file.close();
  }
  _lib = nullptr;
}

void GDSWriter::calcRecSize(record_t& r)
{
  r.length = 4;
  switch (r.dataType) {
    case DataType::REAL_8:
      r.length += r.data64.size() * 8;
      break;
    case DataType::INT_4:
      r.length += r.data32.size() * 4;
      break;
    case DataType::INT_2:
      r.length += r.data16.size() * 2;
      break;
    case DataType::ASCII_STRING:
    case DataType::BIT_ARRAY:
      r.length += r.data8.size();
      break;
    case DataType::NO_DATA:
      break;
    case DataType::REAL_4:
    case DataType::INVALID_DT:
      _logger->error(ODB, 448, "Invalid data type");
      break;
  }
}

void GDSWriter::writeReal8(double real)
{
  const uint64_t value = htobe64(double_to_real8(real));
  _file.write(reinterpret_cast<const char*>(&value), sizeof(uint64_t));
}

void GDSWriter::writeInt32(int32_t i)
{
  const int32_t value = htobe32(i);
  _file.write(reinterpret_cast<const char*>(&value), sizeof(int32_t));
}

void GDSWriter::writeInt16(int16_t i)
{
  const int16_t value = htobe16(i);
  _file.write(reinterpret_cast<const char*>(&value), sizeof(int16_t));
}

void GDSWriter::writeInt8(int8_t i)
{
  _file.write(reinterpret_cast<const char*>(&i), sizeof(int8_t));
}

void GDSWriter::writeRecord(record_t& r)
{
  calcRecSize(r);
  writeInt16(r.length);
  writeInt8(fromRecordType(r.type));
  writeInt8(fromDataType(r.dataType));

  switch (r.dataType) {
    case DataType::REAL_8: {
      for (const auto& d : r.data64) {
        writeReal8(d);
      }
      break;
    }
    case DataType::INT_4: {
      for (const auto& d : r.data32) {
        writeInt32(d);
      }
      break;
    }
    case DataType::INT_2: {
      for (const auto& d : r.data16) {
        writeInt16(d);
      }
      break;
    }
    case DataType::ASCII_STRING:
    case DataType::BIT_ARRAY: {
      _file.write(r.data8.c_str(), r.data8.size());
      break;
    }
    case DataType::NO_DATA: {
      break;
    }
    case DataType::REAL_4:
    case DataType::INVALID_DT:
      _logger->error(ODB, 449, "Invalid data type");
      break;
  }
}

void GDSWriter::writeLib()
{
  record_t rh;
  rh.type = RecordType::HEADER;
  rh.dataType = DataType::INT_2;
  rh.data16 = {600};
  writeRecord(rh);

  record_t r;
  r.type = RecordType::BGNLIB;
  r.dataType = DataType::INT_2;

  const std::time_t now = std::time(nullptr);
  const std::tm* lt = std::localtime(&now);
  r.data16 = {(int16_t) lt->tm_year,
              (int16_t) lt->tm_mon,
              (int16_t) lt->tm_mday,
              (int16_t) lt->tm_hour,
              (int16_t) lt->tm_min,
              (int16_t) lt->tm_sec,
              (int16_t) lt->tm_year,
              (int16_t) lt->tm_mon,
              (int16_t) lt->tm_mday,
              (int16_t) lt->tm_hour,
              (int16_t) lt->tm_min,
              (int16_t) lt->tm_sec};
  writeRecord(r);

  record_t r2;
  r2.type = RecordType::LIBNAME;
  r2.dataType = DataType::ASCII_STRING;
  r2.data8 = _lib->getLibname();
  writeRecord(r2);

  record_t r3;

  r3.type = RecordType::UNITS;
  r3.dataType = DataType::REAL_8;
  auto units = _lib->getUnits();
  r3.data64 = {units.first, units.second};
  writeRecord(r3);

  auto structures = _lib->getGDSStructures();
  for (auto s : structures) {
    writeStruct(s);
  }

  record_t r4;
  r4.type = RecordType::ENDLIB;
  r4.dataType = DataType::NO_DATA;
  writeRecord(r4);
}

void GDSWriter::writeStruct(dbGDSStructure* str)
{
  record_t r;
  r.type = RecordType::BGNSTR;
  r.dataType = DataType::INT_2;

  std::time_t now = std::time(nullptr);
  std::tm* lt = std::localtime(&now);
  r.data16 = {(int16_t) lt->tm_year,
              (int16_t) lt->tm_mon,
              (int16_t) lt->tm_mday,
              (int16_t) lt->tm_hour,
              (int16_t) lt->tm_min,
              (int16_t) lt->tm_sec,
              (int16_t) lt->tm_year,
              (int16_t) lt->tm_mon,
              (int16_t) lt->tm_mday,
              (int16_t) lt->tm_hour,
              (int16_t) lt->tm_min,
              (int16_t) lt->tm_sec};
  writeRecord(r);

  record_t r2;
  r2.type = RecordType::STRNAME;
  r2.dataType = DataType::ASCII_STRING;
  r2.data8 = str->getName();
  writeRecord(r2);

  for (auto boundary : str->getGDSBoundarys()) {
    writeBoundary(boundary);
  }

  for (auto box : str->getGDSBoxs()) {
    writeBox(box);
  }

  for (auto node : str->getGDSNodes()) {
    writeNode(node);
  }

  for (auto path : str->getGDSPaths()) {
    writePath(path);
  }

  for (auto sref : str->getGDSSRefs()) {
    writeSRef(sref);
  }

  for (auto text : str->getGDSTexts()) {
    writeText(text);
  }

  record_t r3;
  r3.type = RecordType::ENDSTR;
  r3.dataType = DataType::NO_DATA;
  writeRecord(r3);
}

template <typename T>
void GDSWriter::writePropAttr(T* el)
{
  auto& props = el->getPropattr();
  for (const auto& pair : props) {
    record_t r;
    r.type = RecordType::PROPATTR;
    r.dataType = DataType::INT_2;
    r.data16 = {pair.first};
    writeRecord(r);

    record_t r2;
    r2.type = RecordType::PROPVALUE;
    r2.dataType = DataType::ASCII_STRING;
    r2.data8 = pair.second;
    writeRecord(r2);
  }
}

void GDSWriter::writeLayer(const int16_t layer)
{
  record_t r;
  r.type = RecordType::LAYER;
  r.dataType = DataType::INT_2;
  r.data16 = {layer};
  writeRecord(r);
}

void GDSWriter::writeXY(const std::vector<Point>& points)
{
  record_t r;
  r.type = RecordType::XY;
  r.dataType = DataType::INT_4;
  for (auto pt : points) {
    r.data32.push_back(pt.x());
    r.data32.push_back(pt.y());
  }
  writeRecord(r);
}

void GDSWriter::writeDataType(const int16_t data_type)
{
  record_t r;
  r.type = RecordType::DATATYPE;
  r.dataType = DataType::INT_2;
  r.data16 = {data_type};
  writeRecord(r);
}

void GDSWriter::writeEndel()
{
  record_t r;
  r.type = RecordType::ENDEL;
  r.dataType = DataType::NO_DATA;
  writeRecord(r);
}

void GDSWriter::writeBoundary(dbGDSBoundary* bnd)
{
  record_t r;
  r.type = RecordType::BOUNDARY;
  r.dataType = DataType::NO_DATA;
  writeRecord(r);

  writeLayer(bnd->getLayer());
  writeDataType(bnd->getDatatype());
  writeXY(bnd->getXY());

  writePropAttr(bnd);
  writeEndel();
}

void GDSWriter::writePath(dbGDSPath* path)
{
  record_t r;
  r.type = RecordType::PATH;
  r.dataType = DataType::NO_DATA;
  writeRecord(r);

  writeLayer(path->getLayer());
  writeDataType(path->getDatatype());

  if (path->get_pathType() != 0) {
    record_t r2;
    r2.type = RecordType::PATHTYPE;
    r2.dataType = DataType::INT_2;
    r2.data16 = {path->get_pathType()};
    writeRecord(r2);
  }

  if (path->getWidth() != 0) {
    record_t r3;
    r3.type = RecordType::WIDTH;
    r3.dataType = DataType::INT_4;
    r3.data32 = {path->getWidth()};
    writeRecord(r3);
  }

  writeXY(path->getXY());

  writePropAttr(path);
  writeEndel();
}

void GDSWriter::writeSRef(dbGDSSRef* sref)
{
  record_t r;
  auto colrow = sref->get_colRow();
  if (colrow.first == 1 && colrow.second == 1) {
    r.type = RecordType::SREF;
  } else {
    r.type = RecordType::AREF;
  }
  r.dataType = DataType::NO_DATA;
  writeRecord(r);

  record_t r2;
  r2.type = RecordType::SNAME;
  r2.dataType = DataType::ASCII_STRING;
  r2.data8 = sref->get_sName();
  writeRecord(r2);

  if (!sref->getTransform().identity()) {
    writeSTrans(sref->getTransform());
  }

  if (colrow.first != 1 || colrow.second != 1) {
    record_t r4;
    r4.type = RecordType::COLROW;
    r4.dataType = DataType::INT_2;
    r4.data16 = {colrow.first, colrow.second};
    writeRecord(r4);
  }

  writeXY(sref->getXY());

  writePropAttr(sref);
  writeEndel();
}

void GDSWriter::writeText(dbGDSText* text)
{
  record_t r;
  r.type = RecordType::TEXT;
  r.dataType = DataType::NO_DATA;
  writeRecord(r);

  writeLayer(text->getLayer());

  record_t r2;
  r2.type = RecordType::TEXTTYPE;
  r2.dataType = DataType::INT_2;
  r2.data16 = {text->getDatatype()};
  writeRecord(r2);

  writeTextPres(text->getPresentation());

  if (text->getWidth() != 0) {
    record_t r4;
    r4.type = RecordType::WIDTH;
    r4.dataType = DataType::INT_4;
    r4.data32 = {text->getWidth()};
    writeRecord(r4);
  }

  if (!text->getTransform().identity()) {
    writeSTrans(text->getTransform());
  }

  writeXY(text->getXY());

  record_t r5;
  r5.type = RecordType::STRING;
  r5.dataType = DataType::ASCII_STRING;
  r5.data8 = text->getText();
  writeRecord(r5);

  writePropAttr(text);
  writeEndel();
}

void GDSWriter::writeBox(dbGDSBox* box)
{
  record_t r;
  r.type = RecordType::BOX;
  r.dataType = DataType::NO_DATA;
  writeRecord(r);

  writeLayer(box->getLayer());

  record_t r2;
  r2.type = RecordType::BOXTYPE;
  r2.dataType = DataType::INT_2;
  r2.data16 = {box->getDatatype()};
  writeRecord(r2);

  writeXY(box->getXY());

  writePropAttr(box);
  writeEndel();
}

void GDSWriter::writeNode(dbGDSNode* node)
{
  record_t r;
  r.type = RecordType::NODE;
  r.dataType = DataType::NO_DATA;
  writeRecord(r);

  writeLayer(node->getLayer());

  record_t r2;
  r2.type = RecordType::NODETYPE;
  r2.dataType = DataType::INT_2;
  r2.data16 = {node->getDatatype()};
  writeRecord(r2);

  writeXY(node->getXY());

  writePropAttr(node);
  writeEndel();
}

void GDSWriter::writeSTrans(const dbGDSSTrans& strans)
{
  record_t r;
  r.type = RecordType::STRANS;
  r.dataType = DataType::BIT_ARRAY;

  char data0 = strans._flipX << 7;
  char data1 = strans._absAngle << 2 | strans._absMag << 1;
  r.data8 = {data0, data1};
  writeRecord(r);

  if (strans._mag != 1.0) {
    record_t r2;
    r2.type = RecordType::MAG;
    r2.dataType = DataType::REAL_8;
    r2.data64 = {strans._mag};
    writeRecord(r2);
  }

  if (strans._angle != 0.0) {
    record_t r3;
    r3.type = RecordType::ANGLE;
    r3.dataType = DataType::REAL_8;
    r3.data64 = {strans._angle};
    writeRecord(r3);
  }
}

void GDSWriter::writeTextPres(const dbGDSTextPres& pres)
{
  record_t r;
  r.type = RecordType::PRESENTATION;
  r.dataType = DataType::BIT_ARRAY;
  r.data8 = {0, 0};
  r.data8[1] |= pres._vPres << 2;
  r.data8[1] |= pres._hPres;
  writeRecord(r);
}

}  // namespace odb::gds
