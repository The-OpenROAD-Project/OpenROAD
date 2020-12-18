// *****************************************************************************
// *****************************************************************************
// Copyright 2012, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#   include <unistd.h>
#endif /* not WIN32 */
#include "lefwWriter.hpp"
#include "lefwWriterCalls.hpp"

// Global variables
char  defaultOut[128];
FILE* fout;
int   userData;

#define CHECK_STATUS(status) \
  if (status) {              \
     lefwPrintError(status); \
     return(status);         \
  }

void dataError() {
  fprintf(fout, "ERROR: returned user data is not correct!\n");
}

void checkType(lefwCallbackType_e c) {
  if (c >= 0 && c <= lefwEndLibCbkType) {
    // OK
  } else {
    fprintf(fout, "ERROR: callback type is out of bounds!\n");
  }
}

int versionCB(lefwCallbackType_e c, lefiUserData ud) {
  int status;

  checkType(c);
  if ((int)ud != userData) dataError();
  status = lefwVersion(5, 6);
  CHECK_STATUS(status);
  return 0;
}

int busBitCharsCB(lefwCallbackType_e c, lefiUserData ud) {
  int status;

  checkType(c);
  if ((int)ud != userData) dataError();
  status = lefwBusBitChars("<>");
  CHECK_STATUS(status);
  return 0;
}

int dividerCB(lefwCallbackType_e c, lefiUserData ud) {
  int status;

  checkType(c);
  if ((int)ud != userData) dataError();
  status = lefwDividerChar(":");
  CHECK_STATUS(status);
  status = lefwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// UNITS
int unitsCB(lefwCallbackType_e c, lefiUserData ud) {
  int status;

  checkType(c);
  if ((int)ud != userData) dataError();
  status = lefwStartUnits();
  CHECK_STATUS(status);
  status = lefwUnits(100, 10, 10000, 10000, 10000, 1000, 0);
  CHECK_STATUS(status);
  status = lefwEndUnits();
  CHECK_STATUS(status);
  return 0;
}

// PROPERTYDEFINITIONS
int propDefCB(lefwCallbackType_e c, lefiUserData ud) {
  int status;

  checkType(c);
  if ((int)ud != userData) dataError();
  status = lefwStartPropDef();
  CHECK_STATUS(status);
  status = lefwStringPropDef("LIBRARY", "NAME", 0, 0, "Cadence96");
  CHECK_STATUS(status);
  status = lefwIntPropDef("LIBRARY", "intNum", 0, 0, 20);
  CHECK_STATUS(status);
  status = lefwRealPropDef("LIBRARY", "realNum", 0, 0, 21.22);
  CHECK_STATUS(status);
  status = lefwStringPropDef("PIN", "TYPE", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwIntPropDef("PIN", "intProp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwRealPropDef("PIN", "realProp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwStringPropDef("MACRO", "stringProp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwIntPropDef("MACRO", "integerProp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwRealPropDef("MACRO", "WEIGHT", 1.0, 100.0, NULL);
  CHECK_STATUS(status);
  status = lefwStringPropDef("VIA", "stringProperty", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwRealPropDef("VIA", "realProp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwIntPropDef("VIA", "COUNT", 1, 100, NULL);
  CHECK_STATUS(status);
  status = lefwStringPropDef("LAYER", "lsp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwIntPropDef("LAYER", "lip", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwRealPropDef("LAYER", "lrp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwStringPropDef("VIARULE", "vrsp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwRealPropDef("VIARULE", "vrip", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwIntPropDef("VIARULE", "vrrp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwStringPropDef("NONDEFAULTRULE", "ndrsp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwIntPropDef("NONDEFAULTRULE", "ndrip", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwRealPropDef("NONDEFAULTRULE", "ndrrp", 0, 0, NULL);
  CHECK_STATUS(status);
  status = lefwEndPropDef();
  return 0;
}

// LAYERS
int layerCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;
  double *current;

  checkType(c);
  if ((int)ud != userData) dataError();
  current = (double*)malloc(sizeof(double)*15);

  status = lefwStartLayer("POLYS", "MASTERSLICE");
  CHECK_STATUS(status);
  status = lefwStringProperty("lsp", "top");
  CHECK_STATUS(status);
  status = lefwIntProperty("lip", 1);
  CHECK_STATUS(status);
  status = lefwRealProperty("lrp", 2.3);
  CHECK_STATUS(status);
  status = lefwEndLayer("POLYS");
  CHECK_STATUS(status);

  status = lefwStartLayer("CUT01", "CUT");
  CHECK_STATUS(status);
  status = lefwEndLayer("CUT01");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("RX");
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(0.75);
  CHECK_STATUS(status);
  status = lefwLayerRoutingOffset(0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRange(0.1, 9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("0.103");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("0.000156");
  CHECK_STATUS(status);
  status = lefwLayerRoutingHeight(9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingThickness(1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingShrinkage(0.1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingEdgeCap(0.00005);
  CHECK_STATUS(status);
  status = lefwLayerRoutingAntennaArea(1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingAntennaLength(1);
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("PEAK", 0);
  CHECK_STATUS(status);
  current[0] = 1E6;
  current[1] = 100E6;
  current[2] = 400E6;
  status = lefwLayerACFrequency(3, current);
  CHECK_STATUS(status);
  current[0] = 0.4;
  current[1] = 0.8;
  current[2] = 10.0;
  current[3] = 50.0;
  current[4] = 100.0;
  status = lefwLayerACWidth(5, current);
  CHECK_STATUS(status);
  current[0] = 2.0E-6;
  current[1] = 1.9E-6;
  current[2] = 1.8E-6;
  current[3] = 1.7E-6;
  current[4] = 1.5E-6;
  current[5] = 1.4E-6;
  current[6] = 1.3E-6;
  current[7] = 1.2E-6;
  current[8] = 1.1E-6;
  current[9] = 1.0E-6;
  current[10] = 0.9E-6;
  current[11] = 0.8E-6;
  current[12] = 0.7E-6;
  current[13] = 0.6E-6;
  current[14] = 0.4E-6;
  status = lefwLayerACTableEntries(15, current);
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("AVERAGE", 0);
  CHECK_STATUS(status);
  current[0] = 1E6;
  current[1] = 100E6;
  current[2] = 400E6;
  status = lefwLayerACFrequency(3, current);
  CHECK_STATUS(status);
  current[0] = 0.6E-6;
  current[1] = 0.5E-6;
  current[2] = 0.4E-6;
  status = lefwLayerACTableEntries(3, current);
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("RMS", 0);
  CHECK_STATUS(status);
  current[0] = 1E6;
  current[1] = 400E6;
  current[2] = 800E6;
  status = lefwLayerACFrequency(3, current);
  CHECK_STATUS(status);
  current[0] = 0.4;
  current[1] = 0.8;
  current[2] = 10.0;
  current[3] = 50.0;
  current[4] = 100.0;
  status = lefwLayerACWidth(5, current);
  CHECK_STATUS(status);
  current[0] = 2.0E-6;
  current[1] = 1.9E-6;
  current[2] = 1.8E-6;
  current[3] = 1.7E-6;
  current[4] = 1.5E-6;
  current[5] = 1.4E-6;
  current[6] = 1.3E-6;
  current[7] = 1.2E-6;
  current[8] = 1.1E-6;
  current[9] = 1.0E-6;
  current[10] = 0.9E-6;
  current[11] = 0.8E-6;
  current[12] = 0.7E-6;
  current[13] = 0.6E-6;
  current[14] = 0.4E-6;
  status = lefwLayerACTableEntries(15, current);
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("RX");
  CHECK_STATUS(status);

  status = lefwStartLayer("CUT12", "CUT");
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("PEAK", 0);
  CHECK_STATUS(status);
  current[0] = 1E6;
  current[1] = 100E6;
  status = lefwLayerACFrequency(2, current);
  CHECK_STATUS(status);
  current[0] = 0.5E-6;
  current[1] = 0.4E-6;
  status = lefwLayerACTableEntries(2, current);
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("AVERAGE", 0);
  CHECK_STATUS(status);
  current[0] = 1E6;
  current[1] = 100E6;
  status = lefwLayerACFrequency(2, current);
  CHECK_STATUS(status);
  current[0] = 0.6E-6;
  current[1] = 0.5E-6;
  status = lefwLayerACTableEntries(2, current);
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("RMS", 0);
  CHECK_STATUS(status);
  current[0] = 100E6;
  current[1] = 800E6;
  status = lefwLayerACFrequency(2, current);
  CHECK_STATUS(status);
  current[0] = 0.5E-6;
  current[1] = 0.4E-6;
  status = lefwLayerACTableEntries(2, current);
  CHECK_STATUS(status);
  status = lefwEndLayer("CUT12");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("PC");
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(0.4);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("PWL ( ( 1 0.103 ) )");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("PWL ( ( 1 0.000156 ) ( 10 0.001 ) )");
  CHECK_STATUS(status);
  status = lefwLayerDCCurrentDensity("AVERAGE", 0);
  CHECK_STATUS(status);
  current[0] = 20.0;
  current[1] = 50.0;
  current[2] = 100.0;
  status = lefwLayerDCWidth(3, current);
  CHECK_STATUS(status);
  current[0] = 1.0E-6;
  current[1] = 0.7E-6;
  current[2] = 0.5E-6;
  status = lefwLayerDCTableEntries(3, current);
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("PC");
  CHECK_STATUS(status);

  status = lefwStartLayer("CA", "CUT");
  CHECK_STATUS(status);
  status = lefwLayerDCCurrentDensity("AVERAGE", 0);
  CHECK_STATUS(status);
  current[0] = 2.0;
  current[1] = 5.0;
  current[2] = 10.0;
  status = lefwLayerDCWidth(3, current);
  CHECK_STATUS(status);
  current[0] = 0.6E-6;
  current[1] = 0.5E-6;
  current[2] = 0.4E-6;
  status = lefwLayerDCTableEntries(3, current);
  CHECK_STATUS(status);
  status = lefwEndLayer("CA");
  CHECK_STATUS(status);
  free((char*)current);

  status = lefwStartLayerRouting("M1");
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(7);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("0.103");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("0.000156");
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("M1");
  CHECK_STATUS(status);

  status = lefwStartLayer("V1", "CUT");
  CHECK_STATUS(status);
  status = lefwLayer(0.6, "CA");
  CHECK_STATUS(status);
  status = lefwEndLayer("V1");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("M2");
  CHECK_STATUS(status);
  status = lefwLayerRouting("VERTICAL", 0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("0.0608");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("0.000184");
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("M2");
  CHECK_STATUS(status);

  status = lefwStartLayer("V2", "CUT");
  CHECK_STATUS(status);
  status = lefwEndLayer("V2");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("M3");
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("0.0608");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("0.000184");
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("M3");
  CHECK_STATUS(status);

  status = lefwStartLayer("V3", "CUT");
  CHECK_STATUS(status);
  status = lefwEndLayer("V3");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("MT");
  CHECK_STATUS(status);
  status = lefwLayerRouting("VERTICAL", 0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("0.0608");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("0.000184");
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("MT");
  CHECK_STATUS(status);

  status = lefwStartLayer("OVERLAP", "OVERLAP");
  CHECK_STATUS(status);
  status = lefwEndLayer("OVERLAP");
  CHECK_STATUS(status);
  return 0;
}

// VIA
int viaCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwStartVia("RX_PC", "DEFAULT");
  CHECK_STATUS(status);
  status = lefwViaResistance(2);
  CHECK_STATUS(status);
  status = lefwViaLayer("RX");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.7, -0.7, 0.7, 0.7);
  CHECK_STATUS(status);
  status = lefwViaLayer("CUT12");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.25, -0.25, 0.25, 0.25);
  CHECK_STATUS(status);
  status = lefwViaLayer("PC");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.6, -0.6, 0.6, 0.6);
  CHECK_STATUS(status);
  status = lefwStringProperty("stringProperty", "DEFAULT");
  CHECK_STATUS(status);
  status = lefwRealProperty("realProperty", 32.33);
  CHECK_STATUS(status);
  status = lefwIntProperty("COUNT", 34);
  CHECK_STATUS(status);
  status = lefwEndVia("RX_PC");
  CHECK_STATUS(status);

  status = lefwStartVia("M2_M3_PWR", NULL);
  CHECK_STATUS(status);
  status = lefwViaResistance(0.4);
  CHECK_STATUS(status);
  status = lefwViaLayer("M2");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-1.35, -1.35, 1.35, 1.35);
  CHECK_STATUS(status);
  status = lefwViaLayer("V2");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-1.35, -1.35, -0.45, 1.35);
  CHECK_STATUS(status);
  status = lefwViaLayerRect(0.45, -1.35, 1.35, -0.45);
  CHECK_STATUS(status);
  status = lefwViaLayerRect(0.45, 0.45, 1.35, 1.35);
  CHECK_STATUS(status);
  status = lefwViaLayer("M3");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-1.35, -1.35, 1.35, 1.35);
  CHECK_STATUS(status);
  status = lefwEndVia("M2_M3_PWR");
  CHECK_STATUS(status);

  status = lefwStartVia("IN1X", NULL);
  CHECK_STATUS(status);
  status = lefwViaLayer("CUT01");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.45, -0.45, 0.45, 0.45);
  CHECK_STATUS(status);
  status = lefwIntProperty("COUNT", 1);
  CHECK_STATUS(status);
  status = lefwEndVia("IN1X");
  CHECK_STATUS(status);
  return 0;
}

// VIARULE
int viaRuleCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwStartViaRule("VIALIST12");
  CHECK_STATUS(status);
  lefwAddComment("Break up the old lefwViaRule into 2 routines");
  lefwAddComment("lefwViaRuleLayer and lefwViaRuleVia");
  status = lefwViaRuleLayer("M1", NULL, 9.0, 9.6, 0, 0);
  CHECK_STATUS(status);
  status = lefwViaRuleLayer("M2", NULL, 3.0, 3.0, 0, 0);
  CHECK_STATUS(status);
  status = lefwViaRuleVia("VIACENTER12");
  CHECK_STATUS(status);
  status = lefwStringProperty("vrsp", "new");
  CHECK_STATUS(status);
  status = lefwIntProperty("vrip", 1);
  CHECK_STATUS(status);
  status = lefwRealProperty("vrrp", 4.5);
  CHECK_STATUS(status);
  status = lefwEndViaRule("VIALIST12");
  CHECK_STATUS(status);

  // VIARULE with GENERATE
  lefwAddComment("Break up the old lefwViaRuleGenearte into 4 routines");
  lefwAddComment("lefwStartViaRuleGen, lefwViaRuleGenLayer,");
  lefwAddComment("lefwViaRuleGenLayer3, and lefwEndViaRuleGen");
  status = lefwStartViaRuleGen("VIAGEN12");
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer("M1", NULL, 0.1, 19, 0, 0);
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer("M2", NULL, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer3("V1", -0.8, -0.8, 0.8, 0.8, 5.6, 6.0, 0.2);
  CHECK_STATUS(status);
  status = lefwEndViaRuleGen("VIAGEN12");
  CHECK_STATUS(status);
  return 0;
}

  // NONDEFAULTRULE
int nonDefaultCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwStartNonDefaultRule("RULE1"); 
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleLayer("RX", 10.0, 2.2, 6, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleLayer("PC", 10.0, 2.2, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleLayer("M1", 10.0, 2.2, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwStartVia("nd1VARX0", NULL);
  CHECK_STATUS(status);
  status = lefwViaResistance(0.2);
  CHECK_STATUS(status);
  status = lefwViaLayer("RX");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-3, -3, 3, 3);
  CHECK_STATUS(status);
  status = lefwViaLayer("CUT12");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-1.0, -1.0, 1.0, 1.0);
  CHECK_STATUS(status);
  status = lefwViaLayer("PC");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-3, -3, 3, 3);
  CHECK_STATUS(status);
  status = lefwEndVia("nd1VARX0");
  CHECK_STATUS(status);
  status = lefwEndNonDefaultRule("RULE1"); 
  CHECK_STATUS(status);
  return 0;
}

// MINFEATURE & DIELECTRIC
int minFeatureCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwMinFeature(0.1, 0.1);
  CHECK_STATUS(status);
  return 0;
}

// SITE
int siteCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwSite("CORE1", "CORE", "X", 67.2, 6);
  CHECK_STATUS(status);
  status = lefwSiteRowPattern("Fsite", 0);
  CHECK_STATUS(status);
  status = lefwSiteRowPatternStr("Lsite", "N");
  CHECK_STATUS(status);
  status = lefwSiteRowPatternStr("Lsite", "FS");
  CHECK_STATUS(status);
  lefwEndSite("CORE1");
  CHECK_STATUS(status);
  status = lefwSite("CORE", "CORE", "Y", 3.6, 28.8);
  CHECK_STATUS(status);
  lefwEndSite("CORE");
  CHECK_STATUS(status);
  status = lefwSite("MRCORE", "CORE", "Y", 3.6, 28.8);
  CHECK_STATUS(status);
  lefwEndSite("MRCORE");
  CHECK_STATUS(status);
  status = lefwSite("IOWIRED", "PAD", NULL, 57.6, 432);
  CHECK_STATUS(status);
  lefwEndSite("IOWIRED");
  CHECK_STATUS(status);
  return 0;
}

// MACRO
int macroCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;
  double *xpath;
  double *ypath;

  checkType(c);
  status = lefwStartMacro("CHK3A");
  CHECK_STATUS(status);
  status = lefwMacroClass("RING", NULL);
  CHECK_STATUS(status);
  status = lefwMacroOrigin(0.9, 0.9);
  CHECK_STATUS(status);
  status = lefwMacroSize(10.8, 28.8);
  CHECK_STATUS(status);
  status = lefwMacroSymmetry("X Y R90");
  CHECK_STATUS(status);
  status = lefwMacroSite("CORE");
  CHECK_STATUS(status);
  status = lefwStartMacroPin("GND");
  CHECK_STATUS(status);
  status = lefwMacroPinDirection("INOUT");
  CHECK_STATUS(status);
  status = lefwMacroPinTaperRule("RULE1");
  CHECK_STATUS(status);
  status = lefwMacroPinUse("GROUND");
  CHECK_STATUS(status);
  status = lefwMacroPinShape("ABUTMENT");
  CHECK_STATUS(status);
  // MACRO - PIN
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M1", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(-0.9, 3, 9.9, 6, 0, 0, 0, 0); 
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwStringProperty("TYPE", "special");
  CHECK_STATUS(status);
  status = lefwIntProperty("intProp", 23);
  CHECK_STATUS(status);
  status = lefwRealProperty("realProp", 24.25);
  CHECK_STATUS(status);
  status = lefwMacroPinAntennasize(1, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennasize(2, "M2");
  CHECK_STATUS(status);
  status = lefwEndMacroPin("GND");
  CHECK_STATUS(status);
  status = lefwStartMacroPin("VDD");
  CHECK_STATUS(status);
  status = lefwMacroPinDirection("INOUT");
  CHECK_STATUS(status);
  status = lefwMacroPinUse("POWER");
  CHECK_STATUS(status);
  status = lefwMacroPinShape("ABUTMENT"); 
  CHECK_STATUS(status);
  // MACRO - PIN - PORT
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M1", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(-0.9, 21, 9.9, 24, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalArea(3, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalArea(4, "M2");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalLength(5, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalLength(6, "M2");
  CHECK_STATUS(status);
  status = lefwEndMacroPin("VDD");
  CHECK_STATUS(status);
  status = lefwStartMacroPin("PA3"); 
  CHECK_STATUS(status);
  status = lefwMacroPinDirection("INPUT");
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M1", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(1.35, -0.45, 2.25, 0.45, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(-0.45, -0.45, 0.45, 0.45, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("PC", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(-0.45, 12.15, 0.45, 13.05, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort(); 
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("PC", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(8.55, 8.55, 9.45, 9.45, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(6.75, 6.75, 7.65, 7.65, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(6.75, 8.75, 7.65, 9.65, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(6.75, 10.35, 7.65, 11.25, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwEndMacroPin("PA3"); 
  CHECK_STATUS(status);
  // MACRO - OBS
  status = lefwStartMacroObs();
  CHECK_STATUS(status);
  status = lefwMacroObsLayer("M1", 0);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(6.6, -0.6, 9.6, 0.6, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(4.8, 12.9, 9.6, 13.2, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(3, 13.8, 7.8, 16.8, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(3, -0.6, 6, 0.6, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroObs();
  CHECK_STATUS(status);
  status = lefwStringProperty("stringProp", "first");
  CHECK_STATUS(status);
  status = lefwIntProperty("integerProp", 1);
  CHECK_STATUS(status);
  status = lefwRealProperty("WEIGHT", 30.31);
  CHECK_STATUS(status);
  status = lefwEndMacro("CHK3A");
  CHECK_STATUS(status);

  // 2nd MACRO
  status = lefwStartMacro("INV");
  CHECK_STATUS(status);
  status = lefwMacroClass("CORE", NULL);
  CHECK_STATUS(status);
  status = lefwMacroForeign("INVS", 0, 0, -1);
  CHECK_STATUS(status);
  status = lefwMacroSize(67.2, 24);
  CHECK_STATUS(status);
  status = lefwMacroSymmetry("X Y R90");
  CHECK_STATUS(status);
  status = lefwMacroSite("CORE1");
  CHECK_STATUS(status);
  status = lefwStartMacroPin("Z");
  CHECK_STATUS(status);
  status = lefwMacroPinDirection("OUTPUT");
  CHECK_STATUS(status);
  status = lefwMacroPinUse("SIGNAL");
  CHECK_STATUS(status);
  status = lefwMacroPinShape("ABUTMENT");
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M2", 5.6);
  CHECK_STATUS(status);
  xpath = (double*)malloc(sizeof(double)*7);
  ypath = (double*)malloc(sizeof(double)*7);
  xpath[0] = 30.8;
  ypath[0] = 9;
  xpath[1] = 42;
  ypath[1] = 9;
  xpath[2] = 30.8;
  ypath[2] = 9;
  xpath[3] = 42;
  ypath[3] = 9;
  xpath[4] = 30.8;
  ypath[4] = 9;
  xpath[5] = 42;
  ypath[5] = 9;
  xpath[6] = 30.8;
  ypath[6] = 9;
  status = lefwMacroPinPortLayerPath(7, xpath, ypath, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwEndMacroPin("Z"); 
  free((char*)xpath);
  free((char*)ypath);
  // MACRO - OBS
  status = lefwStartMacroObs();
  CHECK_STATUS(status);
  status = lefwMacroObsLayer("M1", 0);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(24.1, 1.5, 43.5, 208.5, 0, 0, 0, 0);
  CHECK_STATUS(status);
  xpath = (double*)malloc(sizeof(double)*2);
  ypath = (double*)malloc(sizeof(double)*2);
  xpath[0] = 8.4;
  ypath[0] = 3;
  xpath[1] = 8.4;
  ypath[1] = 124;
  status = lefwMacroObsLayerPath(2, xpath, ypath, 0, 0, 0, 0);
  CHECK_STATUS(status);
  xpath[0] = 58.8;
  ypath[0] = 3;
  xpath[1] = 58.8;
  ypath[1] = 123;
  status = lefwMacroObsLayerPath(2, xpath, ypath, 0, 0, 0, 0);
  CHECK_STATUS(status);
  xpath[0] = 64.4;
  ypath[0] = 3;
  xpath[1] = 64.4;
  ypath[1] = 123;
  status = lefwMacroObsLayerPath(2, xpath, ypath, 0, 0, 0, 0);
  CHECK_STATUS(status);
  free((char*)xpath);
  free((char*)ypath);
  status = lefwEndMacroObs();
  CHECK_STATUS(status);
  status = lefwEndMacro("INV");
  CHECK_STATUS(status);
  return 0;
}

// ANTENNA
int antennaCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwAntenna("INPUTPINANTENNASIZE", 1);
  CHECK_STATUS(status);
  status = lefwAntenna("OUTPUTPINANTENNASIZE", -1);
  CHECK_STATUS(status);
  status = lefwAntenna("INOUTPINANTENNASIZE", -1);
  CHECK_STATUS(status);
  status = lefwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// BEGINEXT
int extCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwStartBeginext("SIGNATURE");
  CHECK_STATUS(status);
  status = lefwBeginextCreator("CADENCE");
  CHECK_STATUS(status);
  status = lefwBeginextDate(); 
  CHECK_STATUS(status);
  status = lefwEndBeginext();
  CHECK_STATUS(status);
  return 0;
}

int endLibCB(lefwCallbackType_e c, lefiUserData ud) {
  int    status;

  checkType(c);
  status = lefwEnd();
  CHECK_STATUS(status);
  return 0;
}

main(int argc, char** argv) {
  char* outfile;
  int   status;    // return code, if none 0 means error
  int   res;

  // assign the default
  strcpy(defaultOut, "lef.in");
  outfile  = defaultOut;
  fout     = stdout;
  userData = 0x01020304;

  double* axis;
  double* num1;
  double* num2;
  double* num3;

  int     encrypt = 0; // if user wants encrypted output

  argc--;
  argv++;
  while (argc--) {
     if (strcmp(*argv, "-o") == 0) {   // output filename
        argv++;
        argc--;
        outfile = *argv;
        if ((fout = fopen(outfile, "w")) == 0) {
           fprintf(stderr, "ERROR: could not open output file\n");
           return 2;
        }
     } else if (strncmp(*argv,  "-h", 2) == 0) {  // compare with -h[elp]
        fprintf(stderr, "Usage: lefwrite [-o <filename>] [-help] [-e]\n");
        return 1;
     } else if (strcmp(*argv, "-e") == 0) { // user wants to write out encrpyted
        encrypt = 1;
     } else {
        fprintf(stderr, "ERROR: Illegal command line option: '%s'\n", *argv);
        return 2;
     }
     argv++;
  }

  // initalize
  // status = lefwInit(fout);
  // CHECK_STATUS(status);
  status = lefwInitCbk(fout);
  CHECK_STATUS(status);

  if (encrypt) {
     // user wants encrypted output, make sure to call lefwCloseEncrypt()
     // before calling fclose();
     status = lefwEncrypt();
     CHECK_STATUS(status);
  }

  // set the callback functions
  lefwSetAntennaCbk(antennaCB);
  lefwSetBusBitCharsCbk(busBitCharsCB);
  lefwSetDividerCharCbk(dividerCB);
  lefwSetEndLibCbk(endLibCB);
  lefwSetExtCbk(extCB);
  lefwSetLayerCbk(layerCB);
  lefwSetMacroCbk(macroCB);
  lefwSetMinFeatureCbk(minFeatureCB);
  lefwSetNonDefaultCbk(nonDefaultCB);
  lefwSetPropDefCbk(propDefCB);
  lefwSetSiteCbk(siteCB);
  lefwSetUnitsCbk(unitsCB);
  lefwSetUserData((void*)3);
  lefwSetVersionCbk(versionCB);
  lefwSetViaCbk(viaCB);
  lefwSetViaRuleCbk(viaRuleCB);

  res = lefwWrite(fout, outfile, (void*)userData);

  if (encrypt) {
     // output has been written in encrypted, need to close the encrypted
     // buffer
     status = lefwCloseEncrypt();
     CHECK_STATUS(status);
  }

  fclose(fout);
  return 0;
}
