// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <list>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "odb.h"
#include "utl/Logger.h"

namespace LefParser {
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
}  // namespace LefParser
namespace odb {

class dbObject;
class dbTech;
class dbTechNonDefaultRule;
class dbLib;
class dbMaster;
class dbDatabase;
class dbTechLayer;
class dbSite;

class lefinReader
{
  dbDatabase* _db;
  dbTech* _tech;
  dbLib* _lib;
  dbMaster* _master;
  utl::Logger* _logger;
  bool _create_tech;
  bool _create_lib;
  bool _skip_obstructions;
  char _left_bus_delimiter;
  char _right_bus_delimiter;
  char _hier_delimiter;
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

  // convert area value to squared db-units
  int dbarea(const double value) { return lround(value * _area_factor); }

  bool readLefInner(const char* lef_file);
  bool readLef(const char* lef_file);
  bool addGeoms(dbObject* object,
                bool is_pin,
                LefParser::lefiGeometries* geometry);
  void createLibrary();
  void createPolygon(dbObject* object,
                     bool is_pin,
                     dbTechLayer* layer,
                     LefParser::lefiGeomPolygon* p,
                     int design_rule_width,
                     double offset_x = 0.0,
                     double offset_y = 0.0);
  dbSite* findSite(const char* name);

 public:
  // convert distance value to db-units
  int dbdist(double value) { return lround(value * _dist_factor); }

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
  void array(LefParser::lefiArray* a);
  void arrayEnd(const char* name);
  int busBitChars(const char* busBit);
  void caseSense(int caseSense);
  void clearance(const char* name);
  void divider(const char* name);
  void noWireExt(const char* name);
  void noiseMargin(LefParser::lefiNoiseMargin* noise);
  void edge1(double value);
  void edge2(double value);
  void edgeScale(double value);
  void noiseTable(LefParser::lefiNoiseTable* noise);
  void correction(LefParser::lefiCorrectionTable* corr);
  void dielectric(double dielectric);
  void irdropBegin(void* ptr);
  void irdrop(LefParser::lefiIRDrop* irdrop);
  void irdropEnd(void* ptr);
  void layer(LefParser::lefiLayer* layer);
  void macroBegin(const char* macroName);
  void macro(LefParser::lefiMacro* macro);
  void macroEnd(const char* macroName);
  void manufacturing(double num);
  void maxStackVia(LefParser::lefiMaxStackVia* maxStack);
  void minFeature(LefParser::lefiMinFeature* min);
  void nonDefault(LefParser::lefiNonDefault* def);
  void obstruction(LefParser::lefiObstruction* obs);
  void pin(LefParser::lefiPin* pin);
  void propDefBegin(void* ptr);
  void propDef(LefParser::lefiProp* prop);
  void propDefEnd(void* ptr);
  void site(LefParser::lefiSite* site);
  void spacingBegin(void* ptr);
  void spacing(LefParser::lefiSpacing* spacing);
  void spacingEnd(void* ptr);
  void timing(LefParser::lefiTiming* timing);
  void units(LefParser::lefiUnits* unit);
  void useMinSpacing(LefParser::lefiUseMinSpacing* spacing);
  void version(double num);
  void via(LefParser::lefiVia* via, dbTechNonDefaultRule* rule = nullptr);
  void viaRule(LefParser::lefiViaRule* viaRule);
  void viaGenerateRule(LefParser::lefiViaRule* viaRule);
  void done(void* ptr);
  template <typename... Args>
  void warning(int id, std::string msg, const Args&... args)
  {
    _logger->warn(utl::ODB, id, msg, args...);
  }
  template <typename... Args>
  void errorTolerant(int id, std::string msg, const Args&... args)
  {
    _logger->warn(utl::ODB, id, msg, args...);
    ++_errors;
  }
  void lineNumber(int lineNo);

  lefinReader(dbDatabase* db,
              utl::Logger* logger,
              bool ignore_non_routing_layers);
  ~lefinReader() = default;

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

class lefin
{
 public:
  lefin(dbDatabase* db, utl::Logger* logger, bool ignore_non_routing_layers);
  ~lefin();

  // convert distance value to db-units
  int dbdist(double value);

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

 private:
  lefinReader* _reader;

  // Protects the LefParser namespace that has static variables
  static std::mutex _lef_mutex;
};

}  // namespace odb
