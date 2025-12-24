// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "defiBlockage.hpp"
#include "defiComponent.hpp"
#include "defiDefs.hpp"
#include "defiFill.hpp"
#include "defiGroup.hpp"
#include "defiNet.hpp"
#include "defiNonDefault.hpp"
#include "defiPinCap.hpp"
#include "defiPinProp.hpp"
#include "defiProp.hpp"
#include "defiRegion.hpp"
#include "defiRowTrack.hpp"
#include "defiScanchain.hpp"
#include "defiSite.hpp"
#include "defiVia.hpp"
#include "definBase.h"
#include "defrReader.hpp"
#include "odb/db.h"
#include "odb/defin.h"

namespace utl {
class Logger;
}

namespace odb {

class definBlockage;
class definComponentMaskShift;
class definComponent;
class definFill;
class definGCell;
class definNet;
class definPin;
class definRow;
class definSNet;
class definTracks;
class definVia;
class definRegion;
class definGroup;
class definNonDefaultRule;
class definPropDefs;
class definPinProps;

class definReader : public definBase
{
 public:
  definReader(dbDatabase* db,
              utl::Logger* logger,
              defin::MODE mode = defin::DEFAULT);
  ~definReader() override;

  void skipConnections();
  void skipWires();
  void skipSpecialWires();
  void skipShields();
  void skipBlockWires();
  void skipFillWires();
  void continueOnErrors();
  void useBlockName(const char* name);
  void error(std::string_view msg);

  void readChip(std::vector<dbLib*>& search_libs,
                const char* def_file,
                dbChip* chip,
                bool issue_callback);

 private:
  void init();
  void setLibs(std::vector<dbLib*>& lib_names);

  void line(int line_num);

  void setTech(dbTech* tech);
  void setBlock(dbBlock* block);
  void setLogger(utl::Logger* logger);

  bool createBlock(const char* file);
  int errors();

  // Parser callbacks
  static int blockageCallback(DefParser::defrCallbackType_e type,
                              DefParser::defiBlockage* blockage,
                              DefParser::defiUserData data);

  static int componentsCallback(DefParser::defrCallbackType_e type,
                                DefParser::defiComponent* comp,
                                DefParser::defiUserData data);

  static int componentMaskShiftCallback(
      DefParser::defrCallbackType_e type,
      DefParser::defiComponentMaskShiftLayer* shiftLayers,
      DefParser::defiUserData data);

  static int dieAreaCallback(DefParser::defrCallbackType_e type,
                             DefParser::defiBox* box,
                             DefParser::defiUserData data);

  static int extensionCallback(DefParser::defrCallbackType_e type,
                               const char* extension,
                               DefParser::defiUserData data);

  static int fillsCallback(DefParser::defrCallbackType_e type,
                           int count,
                           DefParser::defiUserData data);

  static int fillCallback(DefParser::defrCallbackType_e type,
                          DefParser::defiFill* fill,
                          DefParser::defiUserData data);

  static int gcellGridCallback(DefParser::defrCallbackType_e type,
                               DefParser::defiGcellGrid* grid,
                               DefParser::defiUserData data);

  static int groupNameCallback(DefParser::defrCallbackType_e type,
                               const char* name,
                               DefParser::defiUserData data);
  static int groupMemberCallback(DefParser::defrCallbackType_e type,
                                 const char* member,
                                 DefParser::defiUserData data);

  static int groupCallback(DefParser::defrCallbackType_e type,
                           DefParser::defiGroup* group,
                           DefParser::defiUserData data);

  static int historyCallback(DefParser::defrCallbackType_e type,
                             const char* extension,
                             DefParser::defiUserData data);

  static int netCallback(DefParser::defrCallbackType_e type,
                         DefParser::defiNet* net,
                         DefParser::defiUserData data);

  static int nonDefaultRuleCallback(DefParser::defrCallbackType_e type,
                                    DefParser::defiNonDefault* rule,
                                    DefParser::defiUserData data);

  static int pinCallback(DefParser::defrCallbackType_e type,
                         DefParser::defiPin* pin,
                         DefParser::defiUserData data);

  static int pinsEndCallback(DefParser::defrCallbackType_e type,
                             void* v,
                             DefParser::defiUserData data);

  static int pinPropCallback(DefParser::defrCallbackType_e type,
                             DefParser::defiPinProp* prop,
                             DefParser::defiUserData data);

  static int pinsStartCallback(DefParser::defrCallbackType_e type,
                               int number,
                               DefParser::defiUserData data);

  static int propCallback(DefParser::defrCallbackType_e type,
                          DefParser::defiProp* prop,
                          DefParser::defiUserData data);
  static int propEndCallback(DefParser::defrCallbackType_e type,
                             void* v,
                             DefParser::defiUserData data);
  static int propStartCallback(DefParser::defrCallbackType_e type,
                               void* v,
                               DefParser::defiUserData data);

  static int regionCallback(DefParser::defrCallbackType_e type,
                            DefParser::defiRegion* region,
                            DefParser::defiUserData data);

  static int rowCallback(DefParser::defrCallbackType_e type,
                         DefParser::defiRow* row,
                         DefParser::defiUserData data);

  static int scanchainsStartCallback(DefParser::defrCallbackType_e type,
                                     int count,
                                     DefParser::defiUserData data);

  static int scanchainsCallback(DefParser::defrCallbackType_e,
                                DefParser::defiScanchain* scan_chain,
                                DefParser::defiUserData data);

  static int slotsCallback(DefParser::defrCallbackType_e type,
                           int count,
                           DefParser::defiUserData data);

  static int specialNetCallback(DefParser::defrCallbackType_e type,
                                DefParser::defiNet* net,
                                DefParser::defiUserData data);

  static int stylesCallback(DefParser::defrCallbackType_e type,
                            int count,
                            DefParser::defiUserData data);

  static int technologyCallback(DefParser::defrCallbackType_e type,
                                const char* name,
                                DefParser::defiUserData data);

  static int trackCallback(DefParser::defrCallbackType_e type,
                           DefParser::defiTrack* track,
                           DefParser::defiUserData data);

  static int versionCallback(DefParser::defrCallbackType_e type,
                             const char* version_str,
                             DefParser::defiUserData data);

  static int divideCharCallback(DefParser::defrCallbackType_e type,
                                const char* value,
                                DefParser::defiUserData data);

  static int busBitCallback(DefParser::defrCallbackType_e type,
                            const char* busbit,
                            DefParser::defiUserData data);

  static int designCallback(DefParser::defrCallbackType_e type,
                            const char* design,
                            DefParser::defiUserData data);

  static int unitsCallback(DefParser::defrCallbackType_e type,
                           double number,
                           DefParser::defiUserData data);

  static int viaCallback(DefParser::defrCallbackType_e type,
                         DefParser::defiVia* via,
                         DefParser::defiUserData data);

  static void contextLogFunctionCallback(DefParser::defiUserData data,
                                         const char* msg);

  static void contextWarningLogFunctionCallback(DefParser::defiUserData data,
                                                const char* msg);

  dbDatabase* _db;
  dbChip* chip_{nullptr};
  std::unique_ptr<definBlockage> _blockageR;
  std::unique_ptr<definComponentMaskShift> _componentMaskShift;
  std::unique_ptr<definComponent> _componentR;
  std::unique_ptr<definFill> _fillR;
  std::unique_ptr<definGCell> _gcellR;
  std::unique_ptr<definNet> _netR;
  std::unique_ptr<definPin> _pinR;
  std::unique_ptr<definRow> _rowR;
  std::unique_ptr<definSNet> _snetR;
  std::unique_ptr<definTracks> _tracksR;
  std::unique_ptr<definVia> _viaR;
  std::unique_ptr<definRegion> _regionR;
  std::unique_ptr<definGroup> _groupR;
  std::unique_ptr<definNonDefaultRule> _non_default_ruleR;
  std::unique_ptr<definPropDefs> _prop_defsR;
  std::unique_ptr<definPinProps> _pin_propsR;
  std::vector<definBase*> _interfaces;
  bool _continue_on_errors{false};
  std::string _block_name;
  std::string version_;
  char hier_delimiter_{0};
  char left_bus_delimiter_{0};
  char right_bus_delimiter_{0};
};

}  // namespace odb
