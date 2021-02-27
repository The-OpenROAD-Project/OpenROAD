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

#include "odb.h"
#include "definBase.h"
#include "defrReader.hpp"

namespace utl
{
  class Logger;
}

namespace odb {

class definBlockage;
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
class definNonDefaultRule;
class definPropDefs;
class definPinProps;

class definReader : public definBase
{
  dbDatabase*             _db;
  definBlockage*          _blockageR;
  definComponent*         _componentR;
  definFill*              _fillR;
  definGCell*             _gcellR;
  definNet*               _netR;
  definPin*               _pinR;
  definRow*               _rowR;
  definSNet*              _snetR;
  definTracks*            _tracksR;
  definVia*               _viaR;
  definRegion*            _regionR;
  definNonDefaultRule*    _non_default_ruleR;
  definPropDefs*          _prop_defsR;
  definPinProps*          _pin_propsR;
  std::vector<definBase*> _interfaces;
  bool                    _update;
  bool                    _continue_on_errors;
  const char*             _block_name;

  void init();
  void setLibs(std::vector<dbLib*>& lib_names);

  virtual void error(const char* msg);
  virtual void line(int line_num);

  void setTech(dbTech* tech);
  void setBlock(dbBlock* block);
  void setLogger(utl::Logger* logger);

  bool createBlock(const char* file);
  bool replaceWires(const char* file);
  void replaceWires();
  int  errors();

  // Parser callbacks
  static int blockageCallback(defrCallbackType_e type,
                              defiBlockage*      blockage,
                              defiUserData       data);

  static int componentsCallback(defrCallbackType_e type,
                                defiComponent*     comp,
                                defiUserData       data);

  static int componentMaskShiftCallback(
      defrCallbackType_e           type,
      defiComponentMaskShiftLayer* shiftLayers,
      defiUserData                 data);

  static int dieAreaCallback(defrCallbackType_e type,
                             defiBox*           box,
                             defiUserData       data);

  static int extensionCallback(defrCallbackType_e type,
                               const char*        extension,
                               defiUserData       data);

  static int fillsCallback(defrCallbackType_e type,
                           int                count,
                           defiUserData       data);

  static int fillCallback(defrCallbackType_e type,
                          defiFill*          fill,
                          defiUserData       data);

  static int gcellGridCallback(defrCallbackType_e type,
                               defiGcellGrid*     grid,
                               defiUserData       data);

  static int groupNameCallback(defrCallbackType_e type,
                               const char*        name,
                               defiUserData       data);
  static int groupMemberCallback(defrCallbackType_e type,
                                 const char*        member,
                                 defiUserData       data);

  static int groupCallback(defrCallbackType_e type,
                           defiGroup*         group,
                           defiUserData       data);

  static int historyCallback(defrCallbackType_e type,
                             const char*        extension,
                             defiUserData       data);

  static int netCallback(defrCallbackType_e type,
                         defiNet*           net,
                         defiUserData       data);

  static int nonDefaultRuleCallback(defrCallbackType_e type,
                                    defiNonDefault*    rule,
                                    defiUserData       data);

  static int pinCallback(defrCallbackType_e type,
                         defiPin*           pin,
                         defiUserData       data);

  static int pinsEndCallback(defrCallbackType_e type,
                             void*              v,
                             defiUserData       data);

  static int pinPropCallback(defrCallbackType_e type,
                             defiPinProp*       prop,
                             defiUserData       data);

  static int pinsStartCallback(defrCallbackType_e type,
                               int                number,
                               defiUserData       data);

  static int propCallback(defrCallbackType_e type,
                          defiProp*          prop,
                          defiUserData       data);
  static int propEndCallback(defrCallbackType_e type,
                             void*              v,
                             defiUserData       data);
  static int propStartCallback(defrCallbackType_e type,
                               void*              v,
                               defiUserData       data);

  static int regionCallback(defrCallbackType_e type,
                            defiRegion*        region,
                            defiUserData       data);

  static int rowCallback(defrCallbackType_e type,
                         defiRow*           row,
                         defiUserData       data);

  static int scanchainsCallback(defrCallbackType_e type,
                                int                count,
                                defiUserData       data);

  static int slotsCallback(defrCallbackType_e type,
                           int                count,
                           defiUserData       data);

  static int specialNetCallback(defrCallbackType_e type,
                                defiNet*           net,
                                defiUserData       data);

  static int stylesCallback(defrCallbackType_e type,
                            int                count,
                            defiUserData       data);

  static int technologyCallback(defrCallbackType_e type,
                                const char*        name,
                                defiUserData       data);

  static int trackCallback(defrCallbackType_e type,
                           defiTrack*         track,
                           defiUserData       data);

  static int unitsCallback(defrCallbackType_e type,
                           double             number,
                           defiUserData       data);

  static int viaCallback(defrCallbackType_e type,
                         defiVia*           via,
                         defiUserData       data);

 public:
  definReader(dbDatabase* db, utl::Logger* logger, defin::MODE mode = defin::DEFAULT);
  virtual ~definReader();

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

  dbChip*  createChip(std::vector<dbLib*>& search_libs, const char* def_file);
  dbBlock* createBlock(dbBlock*             parent,
                       std::vector<dbLib*>& search_libs,
                       const char*          def_file);
  bool     replaceWires(dbBlock* block, const char* def_file);
};

}  // namespace odb
