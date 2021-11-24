// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2017, Cadence Design Systems
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

char defaultOut[128];

// Global variables
FILE* fout;

#define CHECK_STATUS(status) \
  if (status) {              \
     lefwPrintError(status); \
     return(status);         \
  }

int main(int argc, char** argv) {
  char* outfile;
  int   status;    // return code, if none 0 means error
  int   lineNum = 0;

  // assign the default
  strcpy(defaultOut, "lef.in");
  outfile = defaultOut;
  fout = stdout;

  double *xpath;
  double *ypath;
  double *xl;
  double *yl;
  double *wthn, *spng;
  int    *aspc;
  int    encrypt = 0;

#ifdef WIN32
    // Enable two-digit exponent format
    _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

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
        fprintf(stderr, "Usage: lefwrite [-o <filename>] [-e] [-help]\n");
        return 1;
     } else if (strncmp(*argv,  "-e", 2) == 0) {  // compare with -e[ncrypt]
        encrypt = 1; 
     } else {
        fprintf(stderr, "ERROR: Illegal command line option: '%s'\n", *argv);
        return 2;
     }
     argv++;
  }

  // initalize
  status = lefwInit(fout);
  CHECK_STATUS(status);

  // set to write an encrypted file
  if (encrypt) {
     status = lefwEncrypt();
     CHECK_STATUS(status);
  }

  status = lefwVersion(5, 8);
  CHECK_STATUS(status);
//  status = lefwCaseSensitive("ON");   5.6
//  CHECK_STATUS(status);
//  status = lefwNoWireExtensionAtPin("ON");  5.6
//  CHECK_STATUS(status);
  status = lefwBusBitChars("<>");
  CHECK_STATUS(status);
  status = lefwDividerChar(":");
  CHECK_STATUS(status);
  status = lefwManufacturingGrid(3.5);
  CHECK_STATUS(status);
  status = lefwFixedMask();
  CHECK_STATUS(status);
  status = lefwUseMinSpacing("OBS", "OFF");
  CHECK_STATUS(status);
//  status = lefwUseMinSpacing("PIN", "ON");  5.6
//  CHECK_STATUS(status);
  status = lefwClearanceMeasure("EUCLIDEAN");
  CHECK_STATUS(status);
  // status = lefwClearanceMeasure("MAXXY");  5.6
  // CHECK_STATUS(status);
  status = lefwNewLine();
  CHECK_STATUS(status);

  // 5.4 ANTENNA
  status = lefwAntennaInputGateArea(45);
  CHECK_STATUS(status);
  status = lefwAntennaInOutDiffArea(65);
  CHECK_STATUS(status);
  status = lefwAntennaOutputDiffArea(55);
  CHECK_STATUS(status);
  status = lefwNewLine();
  CHECK_STATUS(status);

  // UNITS
  status = lefwStartUnits();
  CHECK_STATUS(status);
  status = lefwUnits(100, 10, 10000, 10000, 10000, 1000, 20000);
  CHECK_STATUS(status);
  status = lefwUnitsFrequency(10);
  CHECK_STATUS(status);
  status = lefwEndUnits();
  CHECK_STATUS(status);

  // PROPERTYDEFINITIONS
  status = lefwStartPropDef();
  CHECK_STATUS(status);
  status = lefwStringPropDef("LIBRARY", "NAME", 0, 0, "Cadence96");
  CHECK_STATUS(status);
  status = lefwIntPropDef("LIBRARY", "intNum", 0, 0, 20);
  CHECK_STATUS(status);
  status = lefwRealPropDef("LIBRARY", "realNum", 0, 0, 21.22);
  CHECK_STATUS(status);
  status = lefwStringPropDef("PIN", "TYPE", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwIntPropDef("PIN", "intProp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwRealPropDef("PIN", "realProp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwStringPropDef("MACRO", "stringProp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwIntPropDef("MACRO", "integerProp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwRealPropDef("MACRO", "WEIGHT", 1.0, 100.0, 0);
  CHECK_STATUS(status);
  status = lefwStringPropDef("VIA", "stringProperty", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwRealPropDef("VIA", "realProp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwIntPropDef("VIA", "COUNT", 1, 100, 0);
  CHECK_STATUS(status);
  status = lefwStringPropDef("LAYER", "lsp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwIntPropDef("LAYER", "lip", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwRealPropDef("LAYER", "lrp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwStringPropDef("VIARULE", "vrsp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwRealPropDef("VIARULE", "vrip", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwIntPropDef("VIARULE", "vrrp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwStringPropDef("NONDEFAULTRULE", "ndrsp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwIntPropDef("NONDEFAULTRULE", "ndrip", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwRealPropDef("NONDEFAULTRULE", "ndrrp", 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndPropDef();
  CHECK_STATUS(status);

  // LAYERS
  double *current;
  double *diffs;
  double *ratios;
  double *area;
  double *width;

  current = (double*)malloc(sizeof(double)*15);
  diffs = (double*)malloc(sizeof(double)*15);
  ratios = (double*)malloc(sizeof(double)*15);

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
  status = lefwLayerDCCurrentDensity("AVERAGE", 0);
  CHECK_STATUS(status);
  current[0] = 2.0;
  current[1] = 5.0;
  current[2] = 10.0;
  status = lefwLayerDCCutarea(3, current);
  CHECK_STATUS(status);
  current[0] = 0.6E-6;
  current[1] = 0.5E-6;
  current[2] = 0.4E-6;
  status = lefwLayerDCTableEntries(3, current);
  CHECK_STATUS(status);
  status = lefwEndLayer("CUT01");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("RX");
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingDiagPitch(1.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingDiagWidth(1.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingDiagSpacing(0.05);
  CHECK_STATUS(status);
  status = lefwLayerRoutingDiagMinEdgeLength(0.07);
  CHECK_STATUS(status);
  status = lefwLayerRoutingArea(34.1);
  CHECK_STATUS(status);
  xl = (double*)malloc(sizeof(double)*2);
  yl = (double*)malloc(sizeof(double)*2);
  xl[0] = 0.14;
  yl[0] = 0.30;
  xl[1] = 0.08;
  yl[1] = 0.33;
  status = lefwLayerRoutingMinsize(2, xl, yl);
  CHECK_STATUS(status);
  free((char*)xl);
  free((char*)yl);
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
  status = lefwLayerRoutingCapMultiplier(1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinwidth(0.15); 
  CHECK_STATUS(status);
  status = lefwLayerRoutingAntennaArea(1);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumAreaRatio(6.7);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumRoutingPlusCut();            // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaAreaMinusDiff(100.0);           // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaGatePlusDiff(2.0);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumDiffAreaRatio(1000);         // 5.7
  CHECK_STATUS(status);
  xl = (double*)malloc(sizeof(double)*5);
  yl = (double*)malloc(sizeof(double)*5);
  xl[0] = 0.0;
  yl[0] = 1.0;
  xl[1] = 0.09999;
  yl[1] = 1.0;
  xl[2] = 0.1;
  yl[2] = 0.2;
  xl[3] = 1.0;
  yl[3] = 0.1;
  xl[4] = 100;
  yl[4] = 0.1;
  status = lefwLayerAntennaAreaDiffReducePwl(5, xl, yl);   // 5.7
  CHECK_STATUS(status);
  free((char*)xl);
  free((char*)yl);
  status = lefwLayerAntennaCumDiffAreaRatio(1000);         // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingAntennaLength(1);
  CHECK_STATUS(status);
  status = lefwLayerDCCurrentDensity("AVERAGE", 10.0);
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("PEAK", 0);
  CHECK_STATUS(status);
  current[0] = 1E6;
  current[1] = 100E6;
  current[2] = 400E6;
  status = lefwLayerACFrequency(3, current);
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
  status = lefwLayerCutSpacing(0.7);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingLayer("RX", 0);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();
  CHECK_STATUS(status);
  status = lefwLayerResistancePerCut(8.0);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(0.22);                // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingAdjacent(3, 0.25, 0);  // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(1.5);                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingParallel();            // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(1.2);                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingAdjacent(2, 1.5, 0);   // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaModel("OXIDE1");
  CHECK_STATUS(status);
  status = lefwLayerAntennaAreaRatio(5.6);
  CHECK_STATUS(status);
  status = lefwLayerAntennaDiffAreaRatio(6.5);
  CHECK_STATUS(status);
  status = lefwLayerAntennaAreaFactor(5.4, 0);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumDiffAreaRatio(4.5);
  CHECK_STATUS(status);
  diffs[0] = 5.4;
  ratios[0] = 5.4;
  diffs[1] = 6.5;
  ratios[1] = 6.5;
  diffs[2] = 7.5;
  ratios[2] = 7.5;
  status = lefwLayerAntennaCumDiffAreaRatioPwl(3, diffs, ratios);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumAreaRatio(6.7);
  CHECK_STATUS(status);
  status = lefwLayerAntennaModel("OXIDE2");
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumAreaRatio(300);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumRoutingPlusCut();            // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaAreaMinusDiff(100.0);           // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaGatePlusDiff(2.0);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaDiffAreaRatio(1000);            // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumDiffAreaRatio(5000);         // 5.7
  CHECK_STATUS(status);
  xl = (double*)malloc(sizeof(double)*5);
  yl = (double*)malloc(sizeof(double)*5);
  xl[0] = 0.0;
  yl[0] = 1.0;
  xl[1] = 0.09999;
  yl[1] = 1.0;
  xl[2] = 0.1;
  yl[2] = 0.2;
  xl[3] = 1.0;
  yl[3] = 0.1;
  xl[4] = 100;
  yl[4] = 0.1;
  status = lefwLayerAntennaAreaDiffReducePwl(5, xl, yl);   // 5.7
  CHECK_STATUS(status);
  free((char*)xl);
  free((char*)yl);
  diffs[0] = 1;
  ratios[0] = 4;
  diffs[1] = 2;
  ratios[1] = 5;
  status = lefwLayerAntennaCumDiffAreaRatioPwl(2, diffs, ratios);
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
  status = lefwLayerRouting("DIAG45", 1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(0.4);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(1.2);                    // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingEndOfLine(1.3, 0.6);      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(1.3);                    // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingEndOfLine(1.4, 0.7);      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingEOLParallel(1.1, 0.5, 1); // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(1.4);                    // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingEndOfLine(1.5, 0.8);      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingEOLParallel(1.2, 0.6, 0); // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingOffsetXYDistance(0.9, 0.7);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("PWL ( ( 1 0.103 ) )");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("PWL ( ( 1 0.000156 ) ( 10 0.001 ) )");
  CHECK_STATUS(status);
  status = lefwLayerAntennaAreaRatio(5.4);
  CHECK_STATUS(status);
  status = lefwLayerAntennaDiffAreaRatio(6.5);
  CHECK_STATUS(status);
  diffs[0] = 4.0;
  ratios[0] = 4.1;
  diffs[1] = 4.2;
  ratios[1] = 4.3;
  status = lefwLayerAntennaDiffAreaRatioPwl(2, diffs, ratios);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumAreaRatio(7.5);
  CHECK_STATUS(status);
  diffs[0] = 5.0;
  ratios[0] = 5.1;
  diffs[1] = 6.0;
  ratios[1] = 6.1;
  status = lefwLayerAntennaCumDiffAreaRatioPwl(2, diffs, ratios);
  CHECK_STATUS(status);
  status = lefwLayerAntennaAreaFactor(4.5, 0);
  CHECK_STATUS(status);
  status = lefwLayerAntennaSideAreaRatio(6.5);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumDiffSideAreaRatio(4.6);
  CHECK_STATUS(status);
  diffs[0] = 8.0;
  ratios[0] = 8.1;
  diffs[1] = 8.2;
  ratios[1] = 8.3;
  diffs[2] = 8.4;
  ratios[2] = 8.5;
  diffs[3] = 8.6;
  ratios[3] = 8.7;
  status = lefwLayerAntennaCumDiffSideAreaRatioPwl(4, diffs, ratios);
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumSideAreaRatio(7.4);
  CHECK_STATUS(status);
  diffs[0] = 7.0;
  ratios[0] = 7.1;
  diffs[1] = 7.2;
  ratios[1] = 7.3;
  status = lefwLayerAntennaDiffSideAreaRatioPwl(2, diffs, ratios);
  CHECK_STATUS(status);
  status = lefwLayerAntennaSideAreaFactor(9.0, "DIFFUSEONLY");
  CHECK_STATUS(status);
  status = lefwLayerACCurrentDensity("PEAK", 10.0);
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
  status = lefwLayerCutSpacing(0.15);                // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingCenterToCenter();      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("BELOW", 0.3, 0.01, 0);
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("ABOVE", 0.5, 0.01, 0);
  CHECK_STATUS(status);
  status = lefwLayerPreferEnclosure("BELOW", 0.06, 0.01, 0);
  CHECK_STATUS(status);
  status = lefwLayerPreferEnclosure("ABOVE", 0.08, 0.02, 0);
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("", 0.02, 0.02, 1.0);
  CHECK_STATUS(status);
  status = lefwLayerEnclosure(NULL, 0.05, 0.05, 2.0);
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("BELOW", 0.07, 0.07, 1.0);
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("ABOVE", 0.09, 0.09, 1.0);
  CHECK_STATUS(status);
  status = lefwLayerResistancePerCut(10.0);
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

  status = lefwStartLayerRouting("M1");
  CHECK_STATUS(status);
  status = lefwLayerRouting("DIAG135", 1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.8);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRange(1.1, 100.1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRangeUseLengthThreshold();
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.61);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRange(1.1, 100.1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRangeInfluence(2.01, 2.0, 1000.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.62);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRange(1.1, 100.1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingRangeRange(4.1, 6.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.63);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingLengthThreshold(1.34, 4.5, 6.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(7);
  CHECK_STATUS(status);
  status = lefwLayerRoutingResistance("0.103");
  CHECK_STATUS(status);
  status = lefwLayerRoutingCapacitance("0.000156");
  CHECK_STATUS(status);
  current[0] = 0.00;
  current[1] = 0.50;
  current[2] = 3.00;
  current[3] = 5.00;
  status = lefwLayerRoutingStartSpacingtableParallel(4, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.15;
  current[2] = 0.15;
  current[3] = 0.15;
  status = lefwLayerRoutingSpacingtableParallelWidth(0.00, 4, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.20;
  current[2] = 0.20;
  current[3] = 0.20;
  status = lefwLayerRoutingSpacingtableParallelWidth(0.25, 4, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.50;
  current[2] = 0.50;
  current[3] = 0.50;
  status = lefwLayerRoutingSpacingtableParallelWidth(1.50, 4, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.50;
  current[2] = 1.00;
  current[3] = 1.00;
  status = lefwLayerRoutingSpacingtableParallelWidth(3.00, 4, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.50;
  current[2] = 1.00;
  current[3] = 2.00;
  status = lefwLayerRoutingSpacingtableParallelWidth(5.00, 4, current);
  CHECK_STATUS(status);
  status = lefwLayerRoutineEndSpacingtable();
  CHECK_STATUS(status);
  status = lefwLayerRoutingStartSpacingtableInfluence();
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingInfluenceWidth(1.5, 0.5, 0.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingInfluenceWidth(3.0, 1.0, 1.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingInfluenceWidth(5.0, 2.0, 2.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutineEndSpacingtable();
  CHECK_STATUS(status);
  status = lefwLayerRoutingStartSpacingtableInfluence();
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingInfluenceWidth(1.5, 0.5, 0.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingInfluenceWidth(5.0, 2.0, 2.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutineEndSpacingtable();
  CHECK_STATUS(status);
  current[0] = 0.00;
  current[1] = 0.50;
  current[2] = 5.00;
  status = lefwLayerRoutingStartSpacingtableParallel(3, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.15;
  current[2] = 0.15;
  status = lefwLayerRoutingSpacingtableParallelWidth(0.00, 3, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.20;
  current[2] = 0.20;
  status = lefwLayerRoutingSpacingtableParallelWidth(0.25, 3, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.50;
  current[2] = 1.00;
  status = lefwLayerRoutingSpacingtableParallelWidth(3.00, 3, current);
  CHECK_STATUS(status);
  current[0] = 0.15;
  current[1] = 0.50;
  current[2] = 2.00;
  status = lefwLayerRoutingSpacingtableParallelWidth(5.00, 3, current);
  CHECK_STATUS(status);
  status = lefwLayerRoutineEndSpacingtable();
  CHECK_STATUS(status);
  free((char*)current);
  free((char*)diffs);
  free((char*)ratios);
  status = lefwLayerAntennaGatePlusDiff(2.0);      // 5.7 
  CHECK_STATUS(status);
  status = lefwLayerAntennaDiffAreaRatio(1000);    // 5.7
  CHECK_STATUS(status);
  status = lefwLayerAntennaCumDiffAreaRatio(5000); // 5.7
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("M1");
  CHECK_STATUS(status);

  status = lefwStartLayer("V1", "CUT");
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingLayer("CA", 0);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();
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
  status = lefwLayerRoutingSpacingLengthThreshold(100.9, 0, 0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingLengthThreshold(0.9, 0, 0.1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.6);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingLengthThreshold(1.9, 0, 0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(1.0);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingSameNet(1);         // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(1.1);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingSameNet(0);         // 5.7
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
  status = lefwLayerRoutingPitchXYDistance(1.8, 1.5);
  CHECK_STATUS(status);
  status = lefwLayerRoutingDiagPitchXYDistance(1.5, 1.8);
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

  area = (double*)malloc(sizeof(double)*3);
  width = (double*)malloc(sizeof(double)*3);

  status = lefwStartLayerRouting("M4");
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 0.9);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcut(2, 0.50);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcut(2, 0.70);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcutConnections("FROMBELOW");
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcut(4, 1.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcutConnections("FROMABOVE");
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcut(2, 1.1);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinimumcutLengthWithin(20.0, 5.0);
  CHECK_STATUS(status);
  area[0]  = 0.40;
  width[0] = 0;
  area[1]  = 0.40;
  width[1] = 0.15;
  area[2]  = 0.80;
  width[2] = 0.50;
  status = lefwLayerRoutingMinenclosedarea(3, area, width);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMaxwidth(10.0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingProtrusion(0.30, 0.60, 1.20);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstep(0.20);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstep(0.05);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.05, NULL, 0.08);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.05, NULL, 0.16);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.05, "INSDECORNER", 0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.05, "INSIDECORNER", 0.15);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.05, "STEP", 0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.05, "STEP", 0.08);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepWithOptions(0.04, "STEP", 0);
  CHECK_STATUS(status);
  status = lefwLayerRoutingMinstepMaxEdges(1.0, 2);    // 5.7
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("M4");
  CHECK_STATUS(status);
  free((char*)area);
  free((char*)width);

  status = lefwStartLayer("implant1", "IMPLANT");
  CHECK_STATUS(status);
  status = lefwLayerWidth(0.50);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(0.50);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();
  CHECK_STATUS(status);
  status = lefwEndLayer("implant1");
  CHECK_STATUS(status);

  status = lefwStartLayer("implant2", "IMPLANT");
  CHECK_STATUS(status);
  status = lefwLayerWidth(0.50);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(0.50);
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();
  CHECK_STATUS(status);
  status = lefwEndLayer("implant2");
  CHECK_STATUS(status);

  status = lefwStartLayer("V3", "CUT");
  CHECK_STATUS(status);
  status = lefwLayerMask(2);
  CHECK_STATUS(status);
  status = lefwLayerWidth(0.60);
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

  status = lefwStartLayerRouting("MET2");
  CHECK_STATUS(status);
  status = lefwLayerRouting("VERTICAL", 0.9);
  CHECK_STATUS(status);
  status = lefwLayerMask(2);
  CHECK_STATUS(status);
  status = lefwMinimumDensity(20.2);
  CHECK_STATUS(status);
  status = lefwMaximumDensity(80.0);
  CHECK_STATUS(status);
  status = lefwDensityCheckWindow(200.0, 200.0);
  CHECK_STATUS(status);
  status = lefwDensityCheckStep(100.0);
  CHECK_STATUS(status);
  status = lefwFillActiveSpacing(3.0);
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("MET2");
  CHECK_STATUS(status);

  status = lefwStartLayer("via34", "CUT");                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerWidth(0.25);                           // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(0.1);                       // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingCenterToCenter();            // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                       // 5.7
  CHECK_STATUS(status);
  status = lefwLayerEnclosure(0, .05, .01, 0);             // 5.7
  CHECK_STATUS(status);
  status = lefwLayerEnclosureLength(0, .05, 0, 0.7);       // 5.7
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("BELOW", .07, .07, 1.0);     // 5.7
  CHECK_STATUS(status);
  status = lefwLayerEnclosure("ABOVE", .09, .09, 1.0);     // 5.7
  CHECK_STATUS(status);
  status = lefwLayerEnclosureWidth(0, .03, .03, 1.0, 0.2); // 5.7
  CHECK_STATUS(status);
  status = lefwEndLayer("via34");                          // 5.7
  CHECK_STATUS(status);

  status = lefwStartLayer("cut23", "CUT");                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacing(0.20);                      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingSameNet();                   // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingLayer("cut12", 1);           // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                       // 5.7
  CHECK_STATUS(status);

  status = lefwLayerCutSpacing(0.30);                      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingCenterToCenter();            // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingSameNet();                   // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingArea(0.02);                  // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                       // 5.7
  CHECK_STATUS(status);

  status = lefwLayerCutSpacing(0.40);                      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingArea(0.5);                   // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                       // 5.7
  CHECK_STATUS(status);

  status = lefwLayerCutSpacing(0.10);                      // 5.7
  CHECK_STATUS(status);
  status = lefwLayerCutSpacingEnd();                       // 5.7
  CHECK_STATUS(status);

  wthn = (double*)malloc(sizeof(double)*3);                // 5.7
  spng = (double*)malloc(sizeof(double)*3);
  aspc = (int*)malloc(sizeof(int)*3);
  wthn[0] = 0.15;
  spng[0] = 0.11;
  wthn[1] = 0.13;
  spng[1] = 0.13;
  wthn[2] = 0.11;
  spng[2] = 0.15;
  status = lefwLayerCutSpacingTableOrtho(3, wthn, spng);
  CHECK_STATUS(status);

  aspc[0] = 3;
  spng[0] = 1;
  status = lefwLayerArraySpacing(0, 2.0, 0.2, 1, aspc, spng);
  CHECK_STATUS(status);
  aspc[0] = 3;
  spng[0] = 1;
  aspc[1] = 4;
  spng[1] = 1.5;
  aspc[2] = 5;
  spng[2] = 2.0;
  status = lefwLayerArraySpacing(1, 2.0, 0.2, 3, aspc, spng);
  CHECK_STATUS(status);
  free((char*)wthn);
  free((char*)spng);
  free((char*)aspc);
  status = lefwEndLayer("cut23");
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("cut24");                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 1);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.2);                     // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.10);                  // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.12);                  // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingNotchLength(0.15);       // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacing(0.14);                  // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingSpacingEndOfNotchWidth(0.15, 0.16, 0.08); // 5.7
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("cut24");                   // 5.7
  CHECK_STATUS(status);

  status = lefwStartLayerRouting("cut25");                 // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingPitch(1.2);                     // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRouting("HORIZONTAL", 1);              // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingWireExtension(7);               // 5.7
  CHECK_STATUS(status);
  status = lefwLayerRoutingStartSpacingtableTwoWidths();   // 5.7
  CHECK_STATUS(status);
  wthn = (double*)malloc(sizeof(double)*4);                // 5.7
  wthn[0] = 0.15;
  wthn[1] = 0.20;
  wthn[2] = 0.50;
  wthn[3] = 1.00;
  status = lefwLayerRoutingSpacingtableTwoWidthsWidth(0.0, 0, 4, wthn);  // 5.7
  CHECK_STATUS(status);
  wthn[0] = 0.20;
  wthn[1] = 0.25;
  wthn[2] = 0.50;
  wthn[3] = 1.00;
  status = lefwLayerRoutingSpacingtableTwoWidthsWidth(0.25, 0.1, 4, wthn);// 5.7
  CHECK_STATUS(status);
  wthn[0] = 0.50;
  wthn[1] = 0.50;
  wthn[2] = 0.60;
  wthn[3] = 1.00;
  status = lefwLayerRoutingSpacingtableTwoWidthsWidth(1.5, 1.5, 4, wthn);// 5.7
  CHECK_STATUS(status);
  wthn[0] = 1.00;
  wthn[1] = 1.00;
  wthn[2] = 1.00;
  wthn[3] = 1.20;
  status = lefwLayerRoutingSpacingtableTwoWidthsWidth(3.0, 3.0, 4, wthn);// 5.7
  CHECK_STATUS(status);
  free(wthn);
  status = lefwLayerRoutineEndSpacingtable();
  CHECK_STATUS(status);
  status = lefwEndLayerRouting("cut25");                   // 5.7
  CHECK_STATUS(status);

  // MAXVIASTACK
  status = lefwMaxviastack(4, "m1", "m7");
  CHECK_STATUS(status);

  // VIA
  status = lefwStartVia("RX_PC", "DEFAULT");
  CHECK_STATUS(status);
  status = lefwViaResistance(2);
  CHECK_STATUS(status);
  status = lefwViaLayer("RX");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.7, -0.7, 0.7, 0.7, 3);
  CHECK_STATUS(status);
  status = lefwViaLayer("CUT12");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.25, -0.25, 0.25, 0.25);
  CHECK_STATUS(status);
  status = lefwViaLayer("PC");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.6, -0.6, 0.6, 0.6, 2);
  CHECK_STATUS(status);
  status = lefwStringProperty("stringProperty", "DEFAULT");
  CHECK_STATUS(status);
  status = lefwRealProperty("realProperty", 32.33);
  CHECK_STATUS(status);
  status = lefwIntProperty("COUNT", 34);
  CHECK_STATUS(status);
  status = lefwEndVia("PC");
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

  xl = (double*)malloc(sizeof(double)*6);
  yl = (double*)malloc(sizeof(double)*6);
  status = lefwStartVia("IN1X", 0);
  CHECK_STATUS(status);
//  status = lefwViaTopofstackonly();  5.6
//  CHECK_STATUS(status);
//  status = lefwViaForeign("IN1X", 0, 0, -1);  5.6
//  CHECK_STATUS(status);
  status = lefwViaLayer("metal2");
  CHECK_STATUS(status);
  xl[0] = -2.1;
  yl[0] = -1.0;
  xl[1] = -0.2;
  yl[1] = 1.0;
  xl[2] = 2.1;
  yl[2] = 1.0;
  xl[3] = 0.2;
  yl[3] = -1.0;
  xl[4] = 0.2;
  yl[4] = -1.0;
  xl[5] = 0.2;
  yl[5] = -1.0;
  status = lefwViaLayerPolygon(6, xl, yl, 2);
  CHECK_STATUS(status);
  xl[0] = -1.1;
  yl[0] = -2.0;
  xl[1] = -0.1;
  yl[1] = 2.0;
  xl[2] = 1.1;
  yl[2] = 2.0;
  xl[3] = 0.1;
  yl[3] = -2.0;
  status = lefwViaLayerPolygon(4, xl, yl, 1);
  CHECK_STATUS(status);
  xl[0] = -3.1;
  yl[0] = -2.0;
  xl[1] = -0.3;
  yl[1] = 2.0;
  xl[2] = 3.1;
  yl[2] = 2.0;
  xl[3] = 0.3;
  yl[3] = -2.0;
  status = lefwViaLayerPolygon(4, xl, yl);
  CHECK_STATUS(status);
  xl[0] = -4.1;
  yl[0] = -2.0;
  xl[1] = -0.4;
  yl[1] = 2.0;
  xl[2] = 4.1;
  yl[2] = 2.0;
  xl[3] = 0.4;
  yl[3] = -2.0;
  status = lefwViaLayerPolygon(4, xl, yl);
  CHECK_STATUS(status);
  status = lefwViaLayer("cut23");
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.4, -0.4, 0.4, 0.4);
  CHECK_STATUS(status);
  xl[0] = -2.1;
  yl[0] = -1.0;
  xl[1] = -0.2;
  yl[1] = 1.0;
  xl[2] = 2.1;
  yl[2] = 1.0;
  xl[3] = 0.2;
  yl[3] = -1.0;
  status = lefwViaLayerPolygon(4, xl, yl);
  CHECK_STATUS(status);
  status = lefwEndVia("IN1X");
  CHECK_STATUS(status);

  status = lefwStartVia("myBlockVia", NULL);
  CHECK_STATUS(status);
  status = lefwViaViarule("DEFAULT", 0.1, 0.1, "metal1", "via12", "metal2",
                          0.1, 0.1, 0.05, 0.01, 0.01, 0.05);
  CHECK_STATUS(status);
  status = lefwViaViaruleRowCol(1, 2);
  CHECK_STATUS(status);
  status = lefwViaViaruleOrigin(1.5, 2.5);
  CHECK_STATUS(status);
  status = lefwViaViaruleOffset(1.5, 2.5, 3.5, 4.5);
  CHECK_STATUS(status);
  status = lefwViaViarulePattern("2_1RF1RF1R71R0_3_R1FFFF");
  CHECK_STATUS(status);
  status = lefwEndVia("myBlockVia");
  CHECK_STATUS(status);

  status = lefwStartVia("myVia23", NULL);
  CHECK_STATUS(status);
  status = lefwViaLayer("metal2"); 
  CHECK_STATUS(status);
  status = lefwViaLayerPolygon(6, xl, yl);
  CHECK_STATUS(status);
  status = lefwViaLayer("cut23"); 
  CHECK_STATUS(status);
  status = lefwViaLayerRect(-0.4, -0.4, 0.4, 0.4);
  CHECK_STATUS(status);
  status = lefwViaLayer("metal3"); 
  CHECK_STATUS(status);
  status = lefwViaLayerPolygon(5, xl, yl);
  CHECK_STATUS(status);
  status = lefwEndVia("myVia23");
  CHECK_STATUS(status);

  free((char*)xl);
  free((char*)yl);

  // VIARULE
  status = lefwStartViaRule("VIALIST12");
  CHECK_STATUS(status);
  lefwAddComment("Break up the old lefwViaRule into 2 routines");
  lefwAddComment("lefwViaRuleLayer and lefwViaRuleVia");
//  status = lefwViaRuleLayer("M1", "VERTICAL", 9.0, 9.6, 4.5, 0);  5.6
//  CHECK_STATUS(status);
//  status = lefwViaRuleLayer("M2", "HORIZONTAL", 3.0, 3.0, 0, 0);  5.6
//  CHECK_STATUS(status);
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
//  status = lefwViaRuleGenLayer("M1", "VERTICAL", 0.1, 19, 1.4, 0);  5.6
//  CHECK_STATUS(status);
//  status = lefwViaRuleGenLayer("M2", "HORIZONTAL", 0, 0, 1.4, 0);  5.6
//  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer("M1", NULL, 0.1, 19, 0, 0);
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer("M2", NULL, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer3("V1", -0.8, -0.8, 0.8, 0.8, 5.6, 6.0, 0.2);
  CHECK_STATUS(status);
  status = lefwEndViaRuleGen("VIAGEN12");
  CHECK_STATUS(status);

  // VIARULE with GENERATE & ENCLOSURE & DEFAULT	
  status = lefwStartViaRuleGen("via12");
  CHECK_STATUS(status);
  status = lefwViaRuleGenDefault();
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayerEnclosure("m1", 0.05, 0.005, 1.0, 100.0);
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayerEnclosure("m2", 0.05, 0.005, 1.0, 100.0);
  CHECK_STATUS(status);
  status = lefwViaRuleGenLayer3("cut12", -0.07, -0.07, 0.07, 0.07, 0.16, 0.16, 0);
  CHECK_STATUS(status);
  status = lefwEndViaRuleGen("via12");
  CHECK_STATUS(status);

  // NONDEFAULTRULE
  status = lefwStartNonDefaultRule("RULE1"); 
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleHardspacing(); 
  CHECK_STATUS(status);
//  status = lefwNonDefaultRuleLayer("RX", 10.0, 2.2, 6, 6.5, 6.5, 6.5);  5.6
//  CHECK_STATUS(status);
//  status = lefwNonDefaultRuleLayer("PC", 10.0, 2.2, 0, 0, 6.5, 0);  5.6
//  CHECK_STATUS(status);
//  status = lefwNonDefaultRuleLayer("M1", 10.0, 2.2, 0, 6.5, 0, 0);  5.6
//  CHECK_STATUS(status);
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
//  status = lefwViaForeignStr("IN1X", 0, 0, "N");  5.6
// CHECK_STATUS(status);
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
  status = lefwStartSpacing();
  CHECK_STATUS(status);
  status = lefwSpacing("CUT01", "RX", 0.1, "STACK");
  CHECK_STATUS(status);
  status = lefwEndSpacing();
  CHECK_STATUS(status);
  status = lefwEndNonDefaultRule("RULE1"); 
  CHECK_STATUS(status);
  status = lefwStartNonDefaultRule("wide1_5x"); 
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleLayer("fw", 4.8, 4.8, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleStartVia("nd1VIARX0", "DEFAULT");
  CHECK_STATUS(status);
//  status = lefwViaTopofstackonly();  5.6
//  CHECK_STATUS(status);
//  status = lefwViaForeign("IN1X", 0, 0, -1);  5.6
//  CHECK_STATUS(status);
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
  status = lefwNonDefaultRuleEndVia("nd1VIARX0");
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleUseVia("via12_fixed_analog_via");
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleMinCuts("cut12", 2);
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleUseVia("via23_fixed_analog_via");
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleMinCuts("cut23", 2);
  CHECK_STATUS(status);
  status = lefwNonDefaultRuleUseViaRule("viaRule23_fixed_analog_via");
  CHECK_STATUS(status);
  status = lefwEndNonDefaultRule("wide1_5x"); 
  CHECK_STATUS(status);

  // UNIVERSALNOISEMARGIN
  /* obsolete in 5.4
  status = lefwUniversalNoiseMargin(0.1, 20);
  CHECK_STATUS(status);
  status = lefwEdgeRateThreshold1(0.1);
  CHECK_STATUS(status);
  status = lefwEdgeRateThreshold2(0.9);
  CHECK_STATUS(status);
  status = lefwEdgeRateScaleFactor(1.0);
  CHECK_STATUS(status);
  */

  // NOISETABLE
  /* obsolete in 5.4
  double *holder;
  holder = (double*)malloc(sizeof(double)*1);
  status = lefwStartNoiseTable(1);
  CHECK_STATUS(status);
  status = lefwEdgeRate(20);
  CHECK_STATUS(status);
  holder[0] = 3;
  status = lefwOutputResistance(1, holder);
  CHECK_STATUS(status);
  holder[0] = 10; 
  status = lefwVictims(25, 1, holder);
  CHECK_STATUS(status);
  status = lefwEndNoiseTable();
  CHECK_STATUS(status);
  */

  // CORRECTIONTABLE
  /* obsolete in 5.4
  status = lefwStartCorrectTable(1);
  CHECK_STATUS(status);
  status = lefwEdgeRate(20);
  CHECK_STATUS(status);
  holder[0] = 3;
  status = lefwOutputResistance(1, holder); // Share the same functions with
  CHECK_STATUS(status);                     // noisetable
  holder[0] = 10.5; 
  status = lefwVictims(25, 1, holder);
  CHECK_STATUS(status);
  status = lefwEndCorrectTable();
  CHECK_STATUS(status);
  free((char*)holder);
  */

  // SPACING
  status = lefwStartSpacing();
  CHECK_STATUS(status);
  status = lefwSpacing("CUT01", "CA", 1.5, NULL);
  CHECK_STATUS(status);
  status = lefwSpacing("CA", "V1", 1.5, "STACK");
  CHECK_STATUS(status);
  status = lefwSpacing("M1", "M1", 3.5, "STACK");
  CHECK_STATUS(status);
  status = lefwSpacing("V1", "V2", 1.5, "STACK");
  CHECK_STATUS(status);
  status = lefwSpacing("M2", "M2", 3.5, "STACK");
  CHECK_STATUS(status);
  status = lefwSpacing("V2", "V3", 1.5, "STACK");
  CHECK_STATUS(status);
  status = lefwEndSpacing();
  CHECK_STATUS(status);

  // MINFEATURE & DIELECTRIC
  status = lefwMinFeature(0.1, 0.1);
  CHECK_STATUS(status);
  /* obsolete in 5.4
  status = lefwDielectric(0.000345);
  CHECK_STATUS(status);
  */
  status = lefwNewLine();
  CHECK_STATUS(status);

  // IRDROP
  /* obsolete in 5.4
  status = lefwStartIrdrop();
  CHECK_STATUS(status);
  status = lefwIrdropTable("DRESHI", "0.0001 -0.7 0.001 -0.8 0.01 -0.9 0.1 -1.0");
  CHECK_STATUS(status);
  status = lefwIrdropTable("DRESLO", "0.0001 -1.7 0.001 -1.6 0.01 -1.5 0.1 -1.3");
  CHECK_STATUS(status);
  status = lefwIrdropTable("DNORESHI", "0.0001 -0.6 0.001 -0.7 0.01 -0.9 0.1 -1.1");
  CHECK_STATUS(status);
  status = lefwIrdropTable("DNORESLO", "0.0001 -1.5 0.001 -1.5 0.01 -1.4 0.1 -1.4");
  CHECK_STATUS(status);
  status = lefwEndIrdrop();
  CHECK_STATUS(status);
  */

  // SITE
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

  // ARRAY
  status = lefwStartArray("M7E4XXX");
  CHECK_STATUS(status);
  status = lefwArraySite("CORE", -5021.450, -4998.000, 0, 14346, 595, 0.700,
                         16.800);
  CHECK_STATUS(status);
  status = lefwArraySiteStr("CORE", -5021.450, -4998.600, "FS", 14346, 595,
                            0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwArraySite("IO", 6148.800, 5800.000, 3, 1, 1, 0.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArraySiteStr("IO", 6148.800, 5240.000, "E", 1, 1, 0.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArraySite("COVER", -7315.0, -7315.000, 1, 1, 1, 0.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArraySiteStr("COVER", 7315.0, 7315.000, "FN", 1, 1, 0.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArrayCanplace("COVER", -7315.000, -7315.000, 0, 1, 1, 0.000,
                             0.000);
  CHECK_STATUS(status);
  status = lefwArrayCanplaceStr("COVER", -7250.000, -7250.000, "N", 5, 1,
                                40.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArrayCannotoccupy("CORE", -5021.450, -4989.600, 6, 100, 595,
                                 0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwArrayCannotoccupyStr("CORE", -5021.450, -4989.600, "N", 100, 595,
                                 0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwArrayTracks("X", -6148.800, 17569, 0.700, "RX");
  CHECK_STATUS(status);
  status = lefwArrayTracks("Y", -6148.800, 20497, 0.600, "RX");
  CHECK_STATUS(status);
  status = lefwStartArrayFloorplan("100%");
  CHECK_STATUS(status);
  status = lefwArrayFloorplan("CANPLACE", "COVER", -7315.000, -7315.000, 1, 1,
                              1, 0.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArrayFloorplanStr("CANPLACE", "COVER", -7250.000, -7250.000,
                                 "N", 5, 1, 40.000, 0.000);
  CHECK_STATUS(status);
  status = lefwArrayFloorplan("CANPLACE", "CORE", -5021.000, -4998.000, 1,
                              14346, 595, 0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwArrayFloorplanStr("CANPLACE", "CORE", -5021.000, -4998.000, "FS",
                                 100, 595, 0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwArrayFloorplan("CANNOTOCCUPY", "CORE", -5021.000, -4998.000, 7,
                              14346, 595, 0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwArrayFloorplanStr("CANNOTOCCUPY", "CORE", -5021.000, -4998.000,
                                 "E", 100, 595, 0.700, 16.800);
  CHECK_STATUS(status);
  status = lefwEndArrayFloorplan("100%");
  CHECK_STATUS(status);
  status = lefwArrayGcellgrid("X", -6157.200, 1467, 8.400);
  CHECK_STATUS(status);
  status = lefwArrayGcellgrid("Y", -6157.200, 1467, 8.400);
  CHECK_STATUS(status);
  status = lefwEndArray("M7E4XXX");
  CHECK_STATUS(status);

  // MACRO
  status = lefwStartMacro("CHK3A");
  CHECK_STATUS(status);
  status = lefwMacroClass("RING", NULL);
  CHECK_STATUS(status);
  status = lefwMacroFixedMask();
  CHECK_STATUS(status);
//  status = lefwMacroSource("USER");  5.6
//  CHECK_STATUS(status);
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
  status = lefwMacroPinMustjoin("PA3");
  CHECK_STATUS(status);
  status = lefwMacroPinTaperRule("RULE1");
  CHECK_STATUS(status);
  status = lefwMacroPinUse("GROUND");
  CHECK_STATUS(status);
  status = lefwMacroPinShape("ABUTMENT");
  CHECK_STATUS(status);
//  status = lefwMacroPinLEQ("A");  5.6
//  CHECK_STATUS(status);
  status = lefwMacroPinSupplySensitivity("vddpin1");
  CHECK_STATUS(status);
  status = lefwMacroPinNetExpr("power1 VDD1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalArea(3, "M1");
  CHECK_STATUS(status);
  // MACRO - PIN
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
//  status = lefwMacroPinForeign("GROUND", 4, 7, 6);  5.6
//  CHECK_STATUS(status);
//  status = lefwMacroPinForeignStr("VSS", 4, 7, "W");  5.6
//  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M1", 0.05);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(-0.9, 3, 9.9, 6, 0, 0, 0, 0, 3); 
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwStringProperty("TYPE", "special");
  CHECK_STATUS(status);
  status = lefwIntProperty("intProp", 23);
  CHECK_STATUS(status);
  status = lefwRealProperty("realProp", 24.25);
  CHECK_STATUS(status);
/* WMD - Comment them out due to mix 5.3 & 5.4 syntax
  status = lefwMacroPinAntennasize(1, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennasize(2, "M2");
*/
  status = lefwMacroPinAntennaModel("OXIDE1");
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
  status = lefwMacroPinNetExpr("power2 VDD2");
  CHECK_STATUS(status);
  // MACRO - PIN - PORT
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M1", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerRect(-0.9, 21, 9.9, 24, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortVia(100, 300, "nd1VIA12", 1, 2, 1, 2, 123);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort("BUMP");
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M2", 0.06);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  xl = (double*)malloc(sizeof(double)*5);
  yl = (double*)malloc(sizeof(double)*5);
  xl[0] = 30.8;
  yl[0] = 30.5;
  xl[1] = 42;
  yl[1] = 53.5;
  xl[2] = 60.8;
  yl[2] = 25.5;
  xl[3] = 47;
  yl[3] = 15.5;
  xl[4] = 20.8;
  yl[4] = 0.5;
  status = lefwStartMacroPinPort("CORE");
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("P1", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerPolygon(5, xl, yl, 5, 6, 454.6, 345.6, 2);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerPolygon(5, xl, yl, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  free((char*)xl);
  free((char*)yl);
/* WMD - Comment them out due to mix 5.3 & 5.4 syntax
  status = lefwMacroPinAntennaMetalArea(3, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalArea(4, "M2");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalLength(5, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMetalLength(6, "M2");
  CHECK_STATUS(status);
*/
  status = lefwEndMacroPin("VDD");
  CHECK_STATUS(status);
  status = lefwStartMacroPin("PA3"); 
  CHECK_STATUS(status);
  status = lefwMacroPinDirection("INPUT");
  CHECK_STATUS(status);
  status = lefwMacroPinNetExpr("gnd1 GND");
  CHECK_STATUS(status);
  // 5.4
  status = lefwMacroPinAntennaPartialMetalArea(4, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaPartialMetalArea(5, "M2");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaPartialMetalSideArea(5, "M2");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaGateArea(1, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaGateArea(2, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaGateArea(3, "M3");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaDiffArea(1, "M1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMaxAreaCar(1, "L1");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMaxSideAreaCar(1, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaPartialCutArea(1, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaPartialCutArea(2, "M2");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaPartialCutArea(3, 0);
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaPartialCutArea(4, "M4");
  CHECK_STATUS(status);
  status = lefwMacroPinAntennaMaxCutCar(1, 0);
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort("CORE");
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M1", 0.02);
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
  status = lefwMacroPinPortDesignRuleWidth("PC", 2);
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
  status = lefwMacroObsLayer("M1", 5.6);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerWidth(5.4);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(6.6, -0.6, 9.6, 0.6, 0, 0, 0, 0, 2);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(4.8, 12.9, 9.6, 13.2, 0, 0, 0, 0, 3);
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
  status = lefwMacroEEQ("CHK1");
  CHECK_STATUS(status);
//  status = lefwMacroLEQ("CHK2");  5.6
//  CHECK_STATUS(status);
  status = lefwMacroClass("CORE", "SPACER");
  CHECK_STATUS(status);
  status = lefwMacroForeign("INVS", 0, 0, -1);
  CHECK_STATUS(status);
  /* obsolete in 5.4
  status = lefwMacroPower(1.0);
  CHECK_STATUS(status);
  */
  status = lefwMacroSize(67.2, 24);
  CHECK_STATUS(status);
  status = lefwMacroSymmetry("X Y R90");
  CHECK_STATUS(status);
  status = lefwMacroSite("CORE1");
  CHECK_STATUS(status);
  status = lefwStartMacroDensity("metal1");
  CHECK_STATUS(status);
  status = lefwMacroDensityLayerRect(0, 0, 100, 100, 45.5);
  CHECK_STATUS(status);
  status = lefwMacroDensityLayerRect(100, 0, 200, 100, 42.2);
  CHECK_STATUS(status);
  status = lefwEndMacroDensity();
  CHECK_STATUS(status);
  status = lefwStartMacroDensity("metal2");
  CHECK_STATUS(status);
  status = lefwMacroDensityLayerRect(200, 1, 300, 200, 43.3);
  CHECK_STATUS(status);
  status = lefwEndMacroDensity();
  CHECK_STATUS(status);
  status = lefwStartMacroPin("Z");
  CHECK_STATUS(status);
  status = lefwMacroPinDirection("OUTPUT");
  CHECK_STATUS(status);
  status = lefwMacroPinUse("SIGNAL");
  CHECK_STATUS(status);
  status = lefwMacroPinShape("ABUTMENT");
  CHECK_STATUS(status);
  /* obsolete in 5.4
  status = lefwMacroPinRisethresh(22);
  CHECK_STATUS(status);
  status = lefwMacroPinFallthresh(100);
  CHECK_STATUS(status);
  status = lefwMacroPinRisesatcur(4);
  CHECK_STATUS(status);
  status = lefwMacroPinFallsatcur(.5);
  CHECK_STATUS(status);
  status = lefwMacroPinVLO(0);
  CHECK_STATUS(status);
  status = lefwMacroPinVHI(5);
  CHECK_STATUS(status);
  status = lefwMacroPinCapacitance(0.08);
  CHECK_STATUS(status);
  status = lefwMacroPinPower(0.1);
  CHECK_STATUS(status);
  */
  status = lefwMacroPinAntennaModel("OXIDE1");
  CHECK_STATUS(status);
  status = lefwStartMacroPinPort(NULL);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayer("M2", 0);
  CHECK_STATUS(status);
  status = lefwMacroPinPortLayerWidth(5.6);
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
  status = lefwMacroPinPortLayerPath(7, xpath, ypath, 0, 0, 0, 0, 2);
  CHECK_STATUS(status);
  status = lefwEndMacroPinPort();
  CHECK_STATUS(status);
  status = lefwEndMacroPin("Z"); 
  free((char*)xpath);
  free((char*)ypath);
  // MACRO - TIMING
  /* obsolete in 5.4
  status = lefwStartMacroTiming();
  CHECK_STATUS(status);
  status = lefwMacroTimingPin("A", "Z");
  CHECK_STATUS(status);
  status = lefwMacroTimingIntrinsic("RISE", .39, .41, 1.2, .25, .29, 1.8,
                                    .67, .87, 2.2, 0.12, 0.13);
  CHECK_STATUS(status);
  status = lefwMacroTimingIntrinsic("FALL", .25, .29, 1.3, .26, .31, 1.7,
                                    .6, .8, 2.1, 0.11, 0.14);
  CHECK_STATUS(status);
  status = lefwMacroTimingRisers(83.178, 90.109);
  CHECK_STATUS(status);
  status = lefwMacroTimingFallrs(76.246, 97.041);
  CHECK_STATUS(status);
  status = lefwMacroTimingRisecs(0.751, 0.751);
  CHECK_STATUS(status);
  status = lefwMacroTimingFallcs(0.751, 0.751);
  CHECK_STATUS(status);
  status = lefwMacroTimingRiset0(0.65453, 0.65453);
  CHECK_STATUS(status);
  status = lefwMacroTimingFallt0(0.38, 0.38);
  CHECK_STATUS(status);
  status = lefwMacroTimingRisesatt1(0, 0);
  CHECK_STATUS(status);
  status = lefwMacroTimingFallsatt1(0.15, 0.15);
  CHECK_STATUS(status);
  status = lefwMacroTimingUnateness("INVERT");
  CHECK_STATUS(status);
  status = lefwEndMacroTiming();
  CHECK_STATUS(status);
  */
  // MACRO - OBS
  status = lefwStartMacroObs();
  CHECK_STATUS(status);
  status = lefwMacroObsDesignRuleWidth("M1", 2);
  CHECK_STATUS(status);
  status = lefwMacroObsLayerRect(24.1, 1.5, 43.5, 208.5, 0, 0, 0, 0);
  CHECK_STATUS(status);
  xpath = (double*)malloc(sizeof(double)*2);
  ypath = (double*)malloc(sizeof(double)*2);
  xpath[0] = 8.4;
  ypath[0] = 3;
  xpath[1] = 8.4;
  ypath[1] = 124;
  status = lefwMacroObsLayerPath(2, xpath, ypath, 0, 0, 0, 0, 2);
  CHECK_STATUS(status);
  xpath[0] = 58.8;
  ypath[0] = 3;
  xpath[1] = 58.8;
  ypath[1] = 123;
  status = lefwMacroObsLayerPath(2, xpath, ypath, 0, 0, 0, 0, 3);
  CHECK_STATUS(status);
  xpath[0] = 64.4;
  ypath[0] = 3;
  xpath[1] = 64.4;
  ypath[1] = 123;
  status = lefwMacroObsLayerPath(2, xpath, ypath, 0, 0, 0, 0);
  CHECK_STATUS(status);
  free((char*)xpath);
  free((char*)ypath);
  xl = (double*)malloc(sizeof(double)*5);
  yl = (double*)malloc(sizeof(double)*5);
  xl[0] = 6.4;
  xl[1] = 3.4;
  xl[2] = 5.4;
  xl[3] = 8.4;
  xl[4] = 9.4;
  yl[0] = 9.2;
  yl[1] = 0.2;
  yl[2] = 7.2;
  yl[3] = 8.2;
  yl[4] = 1.2;
  status = lefwMacroObsLayerPolygon(5, xl, yl, 0, 0, 0, 0, 3);
  CHECK_STATUS(status);
  free((char*)xl);
  free((char*)yl);
  status = lefwEndMacroObs();
  CHECK_STATUS(status);
  status = lefwEndMacro("INV");
  CHECK_STATUS(status);

  // 3rd MACRO
  status = lefwStartMacro("DFF3");
  CHECK_STATUS(status);
  status = lefwMacroClass("CORE", "ANTENNACELL");
  CHECK_STATUS(status);
  status = lefwMacroForeignStr("DFF3S", 0, 0, "N");
  CHECK_STATUS(status);
  /* obsolete in 5.4
  status = lefwMacroPower(4.0);
  CHECK_STATUS(status);
  */
  status = lefwMacroSize(67.2, 210);
  CHECK_STATUS(status);
  status = lefwMacroSymmetry("X Y R90");
  CHECK_STATUS(status);
  status = lefwMacroSitePattern("CORE", 34, 54, 7, 30, 3, 1, 1);
  CHECK_STATUS(status);
  status = lefwMacroSitePatternStr("CORE1", 21, 68, "S", 30, 3, 2, 2);
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF3");
  CHECK_STATUS(status);

  status = lefwStartMacro("DFF4");
  CHECK_STATUS(status);
  status = lefwMacroClass("COVER", "BUMP");
  CHECK_STATUS(status);
  status = lefwMacroForeignStr("DFF3S", 0, 0, "");
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF4");
  CHECK_STATUS(status);

  status = lefwStartMacro("DFF5");
  CHECK_STATUS(status);
  status = lefwMacroClass("COVER", NULL);
  CHECK_STATUS(status);
  status = lefwMacroForeignStr("DFF3S", 0, 0, "");
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF5");
  CHECK_STATUS(status);

  status = lefwStartMacro("DFF6");
  CHECK_STATUS(status);
  status = lefwMacroClass("BLOCK", "BLACKBOX");
  CHECK_STATUS(status);
  status = lefwMacroForeignStr("DFF3S", 0, 0, "");
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF6");
  CHECK_STATUS(status);

  status = lefwStartMacro("DFF7");
  CHECK_STATUS(status);
  status = lefwMacroClass("PAD", "AREAIO");
  CHECK_STATUS(status);
  status = lefwMacroForeignStr("DFF3S", 0, 0, "");
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF7");
  CHECK_STATUS(status);

  status = lefwStartMacro("DFF8");
  CHECK_STATUS(status);
  status = lefwMacroClass("BLOCK", "SOFT");
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF8");
  CHECK_STATUS(status);

  status = lefwStartMacro("DFF9");
  CHECK_STATUS(status);
  status = lefwMacroClass("CORE", "WELLTAP");
  CHECK_STATUS(status);
  status = lefwEndMacro("DFF9");
  CHECK_STATUS(status);

  status = lefwStartMacro("myTest");
  CHECK_STATUS(status);
  status = lefwMacroClass("CORE", NULL);
  CHECK_STATUS(status);
  status = lefwMacroSize(10.0, 14.0);
  CHECK_STATUS(status);
  status = lefwMacroSymmetry("X");
  CHECK_STATUS(status);
  status = lefwMacroSitePatternStr("Fsite", 0, 0, "N", 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwMacroSitePatternStr("Fsite", 0, 7.0, "FS", 30, 3, 2, 2);
  CHECK_STATUS(status);
  status = lefwMacroSitePatternStr("Fsite", 4.0, 0, "N", 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = lefwEndMacro("myTest");
  CHECK_STATUS(status);

  // ANTENNA, this will generate error for 5.4 since I already have ANTENNA
  // somewhere
  status = lefwAntenna("INPUTPINANTENNASIZE", 1);
  CHECK_STATUS(status);
  status = lefwAntenna("OUTPUTPINANTENNASIZE", -1);
  CHECK_STATUS(status);
  status = lefwAntenna("INOUTPINANTENNASIZE", -1);
  CHECK_STATUS(status);
  status = lefwNewLine();
  CHECK_STATUS(status);

  // BEGINEXT
  status = lefwStartBeginext("SIGNATURE");
  CHECK_STATUS(status);
  lefwAddIndent();
  status = lefwBeginextCreator("CADENCE");
  CHECK_STATUS(status);
  // since the date is different each run,
  // comment it out, so the quick test will not fail
  // status = lefwBeginextDate(); 
  // CHECK_STATUS(status);
  status = lefwEndBeginext();
  CHECK_STATUS(status);

  status = lefwEnd();
  CHECK_STATUS(status);

  lineNum = lefwCurrentLineNumber();
  if (lineNum == 0)
     fprintf(stderr, "ERROR: Nothing has been written!!!\n");

  fclose(fout);

  return 0;
}


