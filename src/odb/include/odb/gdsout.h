// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "odb/gdsUtil.h"
#include "odb/gdsin.h"
#include "odb/geom.h"

namespace odb::gds {

class GDSWriter
{
 public:
  GDSWriter(utl::Logger* logger);

  /**
   * Writes a dbGDSLib object to a GDS file
   *
   * @param filename The path to the output file
   * @param lib The dbGDSLib object to write
   * @throws std::runtime_error if the file cannot be opened, or if the GDS is
   * corrupted
   */
  void write_gds(dbGDSLib* lib, const std::string& filename);

 private:
  /**
   * Calculates and sets the size of a record
   *
   * @param r The record to evaluate
   */
  void calcRecSize(record_t& r);

  /**
   * Writes a record to the output file stream
   *
   * @param r The record to write
   */
  void writeRecord(record_t& r);

  /**
   * Writes a real8 to _file
   *
   * NOTE: real8 is not the same as double. This conversion is not lossless.
   */
  void writeReal8(double real);

  /** Writes an int32 to _file */
  void writeInt32(int32_t i);

  /** Writes an int16 to _file */
  void writeInt16(int16_t i);

  /** Writes an int8 to _file */
  void writeInt8(int8_t i);

  /** Helper function to write layer record of a dbGDSElement to _file */
  void writeLayer(int16_t layer);
  /** Helper function to write XY record  of a dbGDSElement to _file */
  void writeXY(const std::vector<Point>& points);
  /** Helper function to write the datatype record of a dbGDSElement to _file */
  void writeDataType(int16_t data_type);
  /** Helper function to end an element in _file */
  void writeEndel();

  /** Helper function a property attribute to _file */
  template <typename T>
  void writePropAttr(T* el);

  /** Writes _lib to the _file */
  void writeLib();

  /** Writes a dbGDSStructure to _file */
  void writeStruct(dbGDSStructure* str);

  /** Writes different variants of dbGDSElement to _file */
  void writeBoundary(dbGDSBoundary* bnd);
  void writePath(dbGDSPath* path);
  void writeSRef(dbGDSSRef* sref);
  void writeARef(dbGDSARef* aref);
  void writeText(dbGDSText* text);
  void writeBox(dbGDSBox* box);

  /** Writes a Transform to _file */
  void writeSTrans(const dbGDSSTrans& strans);

  /** Writes a Text Presentation to _file */
  void writeTextPres(const dbGDSTextPres& pres);

  /** Output filestream */
  std::ofstream file_;
  /** Current dbGDSLib object */
  dbGDSLib* lib_{nullptr};

  utl::Logger* logger_{nullptr};
};

}  // namespace odb::gds
