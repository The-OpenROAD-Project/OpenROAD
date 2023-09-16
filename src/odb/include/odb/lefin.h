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

#pragma once

#include <list>
#include <string>
#include <vector>

#include "db.h"
#include "dbTypes.h"
#include "odb.h"
#include "utl/Logger.h"

namespace LefDefParser {
class lefiArray;
struct lefiNoiseMargin;
class lefiNoiseTable;
struct lefiPoints;
using lefiNum = lefiPoints;
class lefiCorrectionTable;
class lefiIRDrop;
class lefiLayer;
class lefiMacro;
class lefiMinFeature;
class lefiNonDefault;
class lefiPin;
class lefiProp;
class lefiSite;
class lefiSpacing;
class lefiTiming;
class lefiUnits;
class lefiUseMinSpacing;
class lefiVia;
class lefiViaRule;
class lefiMaxStackVia;
class lefiObstruction;
class lefiGeometries;
struct lefiGeomPolygon;
}  // namespace LefDefParser

namespace odb {

class dbObject;
class dbTech;
class dbTechNonDefaultRule;
class dbLib;
class dbMaster;
class dbDatabase;
class dbTechLayer;
class dbOrientType;
class _dbSite;
class dbSite;

using namespace LefDefParser;

class lefin
{
  dbDatabase* _db;
  dbTech* _tech;
  dbLib* _lib;
  dbMaster* _master;
  utl::Logger* _logger;
  bool _create_tech;
  bool _create_lib;
  bool _skip_obstructions;
  char _left_bus_delimeter;
  char _right_bus_delimeter;
  char _hier_delimeter;
  int _layer_cnt;
  int _master_cnt;
  int _via_cnt;
  int _errors;
  int _lef_units;
  const char* _lib_name;
  double _dist_factor;
  double _area_factor;
  int _dbu_per_micron;
  bool _override_lef_dbu;
  bool _master_modified;
  bool _ignore_non_routing_layers;
  std::vector<std::pair<odb::dbObject*, std::string>> _incomplete_props;

  void init();
  void setDBUPerMicron(int dbu);

  // convert area value to db-units (1nm = 1db unit)
  int dbarea(double value)
  {
    if (value < 0.0)
      return (int) (value * _area_factor - 0.5);
    else
      return (int) (value * _area_factor + 0.5);
  }

  int round(double value)
  {
    if (value < 0.0)
      return (int) (value - 0.5);
    else
      return (int) (value + 0.5);
  }

  bool readLef(const char* lef_file);
  bool addGeoms(dbObject* object, bool is_pin, lefiGeometries* geometry);
  void createLibrary();
  void createPolygon(dbObject* object,
                     bool is_pin,
                     dbTechLayer* layer,
                     lefiGeomPolygon* p,
                     int design_rule_width,
                     double offset_x = 0.0,
                     double offset_y = 0.0);

 public:
  // convert distance value to db-units (1nm = 1db unit)
  int dbdist(double value)
  {
    if (value < 0.0)
      return (int) (value * _dist_factor - 0.5);
    else
      return (int) (value * _dist_factor + 0.5);
  }

  enum AntennaType
  {
    ANTENNA_INPUT_GATE_AREA,
    ANTENNA_INOUT_DIFF_AREA,
    ANTENNA_OUTPUT_DIFF_AREA,
    ANTENNA_INPUT_SIZE,
    ANTENNA_OUTPUT_SIZE,
    ANTENNA_INOUT_SIZE
  };

  void antenna(AntennaType type, double value);
  void arrayBegin(const char* name);
  void array(lefiArray* a);
  void arrayEnd(const char* name);
  int busBitChars(const char* busBit);
  void caseSense(int caseSense);
  void clearance(const char* name);
  void divider(const char* name);
  void noWireExt(const char* name);
  void noiseMargin(lefiNoiseMargin* noise);
  void edge1(double value);
  void edge2(double value);
  void edgeScale(double value);
  void noiseTable(lefiNoiseTable* noise);
  void correction(lefiCorrectionTable* corr);
  void dielectric(double dielectric);
  void irdropBegin(void* ptr);
  void irdrop(lefiIRDrop* irdrop);
  void irdropEnd(void* ptr);
  void layer(lefiLayer* layer);
  void macroBegin(const char* macroName);
  void macro(lefiMacro* macro);
  void macroEnd(const char* macroName);
  void manufacturing(double num);
  void maxStackVia(lefiMaxStackVia* maxStack);
  void minFeature(lefiMinFeature* min);
  void nonDefault(lefiNonDefault* def);
  void obstruction(lefiObstruction* obs);
  void pin(lefiPin* pin);
  void propDefBegin(void* ptr);
  void propDef(lefiProp* prop);
  void propDefEnd(void* ptr);
  void site(lefiSite* site);
  void spacingBegin(void* ptr);
  void spacing(lefiSpacing* spacing);
  void spacingEnd(void* ptr);
  void timing(lefiTiming* timing);
  void units(lefiUnits* unit);
  void useMinSpacing(lefiUseMinSpacing* spacing);
  void version(double num);
  void via(lefiVia* via, dbTechNonDefaultRule* rule = nullptr);
  void viaRule(lefiViaRule* viaRule);
  void viaGenerateRule(lefiViaRule* viaRule);
  void done(void* ptr);
  template <typename... Args>
  inline void warning(int id, std::string msg, const Args&... args)
  {
    _logger->warn(utl::ODB, id, msg, args...);
  }
  template <typename... Args>
  inline void errorTolerant(int id, std::string msg, const Args&... args)
  {
    _logger->warn(utl::ODB, id, msg, args...);
    ++_errors;
  }
  void lineNumber(int lineNo);
  const std::unordered_map<std::string, dbOrientType::Value> orientationMap
      = {{"N", dbOrientType::R0},
         {"W", dbOrientType::R270},
         {"S", dbOrientType::R180},
         {"E", dbOrientType::R90},
         {"FN", dbOrientType::MYR90},
         {"FW", dbOrientType::MY},
         {"FS", dbOrientType::MXR90},
         {"FE", dbOrientType::MX}};

  lefin(dbDatabase* db, utl::Logger* logger, bool ignore_non_routing_layers);
  ~lefin();

  // Skip macro-obstructions in the lef file.
  void skipObstructions() { _skip_obstructions = true; }

  //
  // Override the LEF DBU-PER-MICRON unit.
  // This function only is only effective when creating a technolgy, because the
  // DBU-PER-MICRON is stored in the tech-db (dbTech).
  //
  // When setting this value use a setting such that the dbu-per-micron >=
  // All(LEF/DEF(S) DBU-PER-MICRON)
  //
  // For example, it is an error to the set dbu-per-micron = 1000 and read a DEF
  // with a DBU-PER-MICRON
  /// of 2000 because the DEF reader would truncate values.
  //
  void dbu_per_micron(int dbu)
  {
    _override_lef_dbu = true;
    setDBUPerMicron(dbu);
  }

  // Create a technology from the tech-data of this LEF file.
  dbTech* createTech(const char* name, const char* lef_file);

  // Create a library from the library-data of this LEF file.
  dbLib* createLib(dbTech* tech, const char* name, const char* lef_file);

  // Create a technology and library from the MACRO's in this LEF file.
  dbLib* createTechAndLib(const char* tech_name,
                          const char* lib_name,
                          const char* lef_file);

  // Add macros to this library
  bool updateLib(dbLib* lib, const char* lef_file);

  // Update a technology from the tech-data of this LEF file.
  bool updateTech(dbTech* tech, const char* lef_file);

  // Add macros to this library and the technology of this library
  bool updateTechAndLib(dbLib* lib, const char* lef_file);
};

}  // namespace odb
