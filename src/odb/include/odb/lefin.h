// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cmath>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "absl/synchronization/mutex.h"
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
 public:
  // convert distance value to db-units
  int dbdist(double value) { return lround(value * dist_factor_); }

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
    logger_->warn(utl::ODB, id, msg, args...);
  }
  template <typename... Args>
  void errorTolerant(int id, std::string msg, const Args&... args)
  {
    logger_->warn(utl::ODB, id, msg, args...);
    ++errors_;
  }
  void lineNumber(int lineNo);

  lefinReader(dbDatabase* db,
              utl::Logger* logger,
              bool ignore_non_routing_layers);
  ~lefinReader() = default;

  // Skip macro-obstructions in the lef file.
  void skipObstructions() { skip_obstructions_ = true; }

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
    override_lef_dbu_ = true;
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

 private:
  void init();
  void setDBUPerMicron(int dbu);

  // convert area value to squared db-units
  int dbarea(const double value) { return lround(value * area_factor_); }

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

  dbDatabase* db_;
  dbTech* tech_;
  dbLib* lib_;
  dbMaster* master_;
  utl::Logger* logger_;
  bool create_tech_;
  bool create_lib_;
  bool skip_obstructions_;
  char left_bus_delimiter_;
  char right_bus_delimiter_;
  char hier_delimiter_;
  int layer_cnt_;
  int master_cnt_;
  int via_cnt_;
  int errors_;
  int lef_units_;
  const char* lib_name_;
  double dist_factor_;
  double area_factor_;
  int dbu_per_micron_;
  bool override_lef_dbu_;
  bool master_modified_;
  bool ignore_non_routing_layers_;
  std::vector<std::pair<odb::dbObject*, std::string>> incomplete_props_;
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
  lefinReader* reader_;

  // Protects the LefParser namespace that has static variables
  static absl::Mutex lef_mutex_;
};

}  // namespace odb
