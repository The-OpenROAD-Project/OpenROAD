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

#include <vector>

#include "definBase.h"
#include "defrReader.hpp"
#include "odb/odb.h"

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
  dbDatabase* _db;
  dbBlock* parent_;  // For Hierarchal implementation if exits
  definBlockage* _blockageR;
  definComponentMaskShift* _componentMaskShift;
  definComponent* _componentR;
  definFill* _fillR;
  definGCell* _gcellR;
  definNet* _netR;
  definPin* _pinR;
  definRow* _rowR;
  definSNet* _snetR;
  definTracks* _tracksR;
  definVia* _viaR;
  definRegion* _regionR;
  definGroup* _groupR;
  definNonDefaultRule* _non_default_ruleR;
  definPropDefs* _prop_defsR;
  definPinProps* _pin_propsR;
  std::vector<definBase*> _interfaces;
  bool _update;
  bool _continue_on_errors;
  std::string _block_name;
  std::string version_;
  char hier_delimeter_;
  char left_bus_delimeter_;
  char right_bus_delimeter_;

  void init() override;
  void setLibs(std::vector<dbLib*>& lib_names);

  void line(int line_num);

  void setTech(dbTech* tech);
  void setBlock(dbBlock* block);
  void setLogger(utl::Logger* logger);

  bool createBlock(const char* file);
  bool replaceWires(const char* file);
  void replaceWires();
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
  void namesAreDBIDs();
  void setAssemblyMode();
  void error(std::string_view msg);

  dbChip* createChip(std::vector<dbLib*>& search_libs,
                     const char* def_file,
                     dbTech* tech);
  dbBlock* createBlock(dbBlock* parent,
                       std::vector<dbLib*>& search_libs,
                       const char* def_file,
                       dbTech* tech);
  bool replaceWires(dbBlock* block, const char* def_file);
};

}  // namespace odb
