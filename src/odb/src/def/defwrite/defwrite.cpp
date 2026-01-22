// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2016, Cadence Design Systems
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

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef WIN32
#include <unistd.h>
#endif /* not WIN32 */
#include "defwWriter.hpp"

char defaultOut[128];

// Global variables
FILE* fout;

#define CHECK_STATUS(status) \
  if (status) {              \
    defwPrintError(status);  \
    return (status);         \
  }

int main(int argc, char** argv)
{
  char* outfile;
  int status;  // return code, if none 0 means error
  int lineNumber = 0;

  const char** layers;
  const char** foreigns;
  const char** shiftLayers;
  int *foreignX, *foreignY, *foreignOrient;
  const char** foreignOrientStr;
  double *coorX, *coorY;
  double* coorValue;
  const char** groupExpr;
  int *xPoints, *yPoints;
  double *xP, *yP;
  const char **coorXSN, **coorYSN;
  bool groupInit = false;

#if (defined WIN32 && _MSC_VER < 1800)
  // Enable two-digit exponent format
  _set_output_format(_TWO_DIGIT_EXPONENT);
#endif

  // assign the default
  strcpy(defaultOut, "def.in");
  outfile = defaultOut;
  fout = stdout;

  argc--;
  argv++;
  while (argc--) {
    if (strcmp(*argv, "-o") == 0) {  // output filename
      argv++;
      argc--;
      outfile = *argv;
      fout = fopen(outfile, "w");
      if (fout == nullptr) {
        fprintf(stderr, "ERROR: could not open output file\n");
        return 2;
      }
    } else if (strncmp(*argv, "-h", 2) == 0) {  // compare with -h[elp]
      fprintf(stderr, "Usage: defwrite [-o <filename>] [-help]\n");
      return 1;
    } else if (strncmp(*argv, "-g", 2) == 0) {  // test of group init function.
      groupInit = true;
    } else {
      fprintf(stderr, "ERROR: Illegal command line option: '%s'\n", *argv);
      return 2;
    }
    argv++;
  }

  if (!groupInit) {
    status = defwInitCbk(fout);
    CHECK_STATUS(status);
    status = defwVersion(5, 8);
    CHECK_STATUS(status);
    status = defwDividerChar(":");
    CHECK_STATUS(status);
    status = defwBusBitChars("[]");
    CHECK_STATUS(status);
    status = defwDesignName("muk");
    CHECK_STATUS(status);
    status = defwTechnology("muk");
    CHECK_STATUS(status);
    status = defwArray("core_array");
    CHECK_STATUS(status);
    status = defwFloorplan("DEFAULT");
    CHECK_STATUS(status);
    status = defwUnits(100);
    CHECK_STATUS(status);
  } else {
    // initalize
    status = defwInit(fout,
                      5,
                      8,
                      "ON",
                      ":",
                      "[]",
                      "muk",
                      "muk",
                      "core_array",
                      "DEFAULT",
                      100);
    CHECK_STATUS(status);
  }

  status = defwNewLine();
  CHECK_STATUS(status);

  // history
  status = defwHistory(
      "Corrected STEP for ROW_9 and added ROW_10 of SITE CORE1 (def)");
  CHECK_STATUS(status);
  status = defwHistory("Removed NONDEFAULTRULE from the net XX100 (def)");
  CHECK_STATUS(status);
  status = defwHistory("Changed some cell orientations (def)");
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);

  // FILLS (add another FILLS is here for CCR 746462
  xP = (double*) malloc(sizeof(double) * 7);
  yP = (double*) malloc(sizeof(double) * 7);
  xP[0] = 2.1;
  yP[0] = 2.1;
  xP[1] = 3.1;
  yP[1] = 3.1;
  xP[2] = 4.1;
  yP[2] = 4.1;
  xP[3] = 5.1;
  yP[3] = 5.1;
  xP[4] = 6.1;
  yP[4] = 6.1;
  xP[5] = 7.1;
  yP[5] = 7.1;
  xP[6] = 8.1;
  yP[6] = 8.1;
  status = defwStartFills(5);
  CHECK_STATUS(status);
  status = defwFillLayer("MET1");
  CHECK_STATUS(status);
  status = defwFillRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwFillPolygon(5, xP, yP);
  CHECK_STATUS(status);
  status = defwFillRect(2000, 2000, 2500, 4000);
  CHECK_STATUS(status);
  status = defwFillPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwFillRect(3000, 2000, 3500, 4000);
  CHECK_STATUS(status);
  status = defwFillLayer("MET2");
  CHECK_STATUS(status);
  status = defwFillRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwFillRect(1000, 4500, 1500, 6500);
  CHECK_STATUS(status);
  status = defwFillRect(1000, 7000, 1500, 9000);
  CHECK_STATUS(status);
  status = defwFillRect(1000, 9500, 1500, 11500);
  CHECK_STATUS(status);
  status = defwFillPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwFillPolygon(6, xP, yP);
  CHECK_STATUS(status);
  status = defwFillLayer("metal1");
  CHECK_STATUS(status);
  status = defwFillLayerMask(1);
  CHECK_STATUS(status);
  status = defwFillLayerOPC();
  CHECK_STATUS(status);
  status = defwFillRect(100, 200, 150, 400);
  CHECK_STATUS(status);
  status = defwFillRect(300, 200, 350, 400);
  CHECK_STATUS(status);
  status = defwFillVia("via28");
  CHECK_STATUS(status);
  status = defwFillViaMask(2);
  CHECK_STATUS(status);
  status = defwFillViaOPC();
  CHECK_STATUS(status);
  status = defwFillPoints(1, xP, yP);
  CHECK_STATUS(status);
  status = defwFillVia("via26");
  CHECK_STATUS(status);
  status = defwFillPoints(3, xP, yP);
  CHECK_STATUS(status);
  status = defwEndFills();
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);

  // PROPERTYDEFINITIONS
  status = defwStartPropDef();
  CHECK_STATUS(status);
  defwAddComment("defwPropDef is broken into 3 routines, defwStringPropDef");
  defwAddComment("defwIntPropDef, and defwRealPropDef");
  status = defwStringPropDef("REGION", "scum", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("REGION", "center", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwRealPropDef("REGION", "area", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("GROUP", "ggrp", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("GROUP", "site", 0, 25, 0);
  CHECK_STATUS(status);
  status = defwRealPropDef("GROUP", "maxarea", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("COMPONENT", "cc", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("COMPONENT", "index", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwRealPropDef("COMPONENT", "size", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwIntPropDef("NET", "alt", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("NET", "lastName", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("NET", "length", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("SPECIALNET", "contype", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("SPECIALNET", "ind", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwRealPropDef("SPECIALNET", "maxlength", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("DESIGN", "title", 0, 0, "Buffer");
  CHECK_STATUS(status);
  status = defwIntPropDef("DESIGN", "priority", 0, 0, 14);
  CHECK_STATUS(status);
  status = defwRealPropDef("DESIGN", "howbig", 0, 0, 15.16);
  CHECK_STATUS(status);
  status = defwRealPropDef("ROW", "minlength", 1.0, 100.0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("ROW", "firstName", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("ROW", "idx", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwIntPropDef("COMPONENTPIN", "dpIgnoreTerm", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("COMPONENTPIN", "dpBit", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("COMPONENTPIN", "realProperty", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("NET", "IGNOREOPTIMIZATION", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("SPECIALNET", "IGNOREOPTIMIZATION", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("NET", "FREQUENCY", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwRealPropDef("SPECIALNET", "FREQUENCY", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwStringPropDef("NONDEFAULTRULE", "ndprop1", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("NONDEFAULTRULE", "ndprop2", 0, 0, 0);
  CHECK_STATUS(status);
  status = defwRealPropDef("NONDEFAULTRULE", "ndprop3", 0, 0, 0.009);
  CHECK_STATUS(status);
  status = defwRealPropDef("NONDEFAULTRULE", "ndprop4", .1, 1.0, 0);
  CHECK_STATUS(status);
  status = defwEndPropDef();
  CHECK_STATUS(status);

  // DIEAREA
  /*
    status = defwDieArea(-190000, -120000, 190000, 70000);
    CHECK_STATUS(status);
  */
  xPoints = (int*) malloc(sizeof(int) * 6);
  yPoints = (int*) malloc(sizeof(int) * 6);
  xPoints[0] = 2000;
  yPoints[0] = 2000;
  xPoints[1] = 3000;
  yPoints[1] = 3000;
  xPoints[2] = 4000;
  yPoints[2] = 4000;
  xPoints[3] = 5000;
  yPoints[3] = 5000;
  xPoints[4] = 6000;
  yPoints[4] = 6000;
  xPoints[5] = 7000;
  yPoints[5] = 7000;
  status = defwDieAreaList(6, xPoints, yPoints);
  CHECK_STATUS(status);
  free((char*) xPoints);
  free((char*) yPoints);

  status = defwNewLine();
  CHECK_STATUS(status);

  // ROW
  status = defwRow("ROW_9", "CORE", -177320, -111250, 6, 911, 1, 360, 0);
  CHECK_STATUS(status);
  status = defwRealProperty("minlength", 50.5);
  CHECK_STATUS(status);
  status = defwStringProperty("firstName", "Only");
  CHECK_STATUS(status);
  status = defwIntProperty("idx", 1);
  CHECK_STATUS(status);
  status = defwRowStr("ROW_10", "CORE1", -19000, -11000, "FN", 1, 100, 0, 600);
  CHECK_STATUS(status);
  status = defwRowStr("ROW_11", "CORE1", -19000, -11000, "FN", 1, 100, 0, 0);
  CHECK_STATUS(status);
  status = defwRow("ROW_12", "CORE1", -19000, -11000, 3, 0, 0, 0, 0);
  CHECK_STATUS(status);
  status = defwRowStr("ROW_13", "CORE1", -19000, -11000, "FN", 0, 0, 0, 0);
  CHECK_STATUS(status);

  // TRACKS
  layers = (const char**) malloc(sizeof(char*) * 1);
  layers[0] = strdup("M1");
  status = defwTracks("X", 3000, 40, 120, 1, layers, 2, 1);
  CHECK_STATUS(status);
  free((char*) layers[0]);
  layers[0] = strdup("M2");
  status = defwTracks("Y", 5000, 10, 20, 1, layers);
  CHECK_STATUS(status);
  free((char*) layers[0]);
  free((char*) layers);
  status = defwNewLine();
  CHECK_STATUS(status);

  // GCELLGRID
  status = defwGcellGrid("X", 0, 100, 600);
  CHECK_STATUS(status);
  status = defwGcellGrid("Y", 10, 120, 400);
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);

  // DEFAULTCAP
  /* obsolete in 5.4
  status = defwStartDefaultCap(4);
  CHECK_STATUS(status);
  status = defwDefaultCap(2, 3);
  CHECK_STATUS(status);
  status = defwDefaultCap(4, 6);
  CHECK_STATUS(status);
  status = defwDefaultCap(8, 9);
  CHECK_STATUS(status);
  status = defwDefaultCap(10, 12);
  CHECK_STATUS(status);
  status = defwEndDefaultCap();
  CHECK_STATUS(status);
  */

  // CANPLACE
  status = defwCanPlaceStr("dp", 45, 64, "N", 35, 1, 39, 1);
  CHECK_STATUS(status);

  status = defwCanPlace("dp", 45, 64, 1, 35, 1, 39, 1);
  CHECK_STATUS(status);

  // CANNOTOCCUPY
  status = defwCannotOccupyStr("dp", 54, 44, "S", 55, 2, 45, 3);
  CHECK_STATUS(status);

  // VIAS
  status = defwStartVias(7);
  CHECK_STATUS(status);
  status = defwViaName("VIA_ARRAY");
  CHECK_STATUS(status);
  status = defwViaPattern("P1-435-543-IJ1FS");
  CHECK_STATUS(status);
  status = defwViaRect("M1", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwViaRect("V1", -40, -40, 40, 40, 3);
  CHECK_STATUS(status);
  status = defwViaRect("M2", -50, -50, 50, 50);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);
  status = defwViaName("VIA_ARRAY1");
  CHECK_STATUS(status);
  status = defwViaRect("M1", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwViaRect("V1", -40, -40, 40, 40, 2);
  CHECK_STATUS(status);
  status = defwViaRect("M2", -50, -50, 50, 50);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);
  status = defwViaName("myUnshiftedVia");
  CHECK_STATUS(status);
  status = defwViaViarule(
      "myViaRule", 20, 20, "metal1", "cut12", "metal2", 5, 5, 0, 4, 0, 1);
  CHECK_STATUS(status);
  status = defwViaViaruleRowCol(2, 3);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);
  status = defwViaName("via2");
  CHECK_STATUS(status);
  status = defwViaViarule("viaRule2",
                          5,
                          6,
                          "botLayer2",
                          "cutLayer2",
                          "topLayer2",
                          6,
                          6,
                          1,
                          4,
                          1,
                          4);
  CHECK_STATUS(status);
  status = defwViaViaruleOrigin(10, -10);
  CHECK_STATUS(status);
  status = defwViaViaruleOffset(0, 0, 20, -20);
  CHECK_STATUS(status);
  status = defwViaViarulePattern("2_F0_2_F8_1_78");
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);

  status = defwViaName("via3");
  CHECK_STATUS(status);
  status = defwViaPattern("P2-435-543-IJ1FS");
  CHECK_STATUS(status);
  status = defwViaRect("M2", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);

  xP = (double*) malloc(sizeof(double) * 6);
  yP = (double*) malloc(sizeof(double) * 6);
  xP[0] = -2.1;
  yP[0] = -1.0;
  xP[1] = -2;
  yP[1] = 1;
  xP[2] = 2.1;
  yP[2] = 1.0;
  xP[3] = 2.0;
  yP[3] = -1.0;
  status = defwViaName("via4");
  CHECK_STATUS(status);
  status = defwViaPolygon("M3", 4, xP, yP, 2);
  CHECK_STATUS(status);
  status = defwViaRect("M4", -40, -40, 40, 40);
  CHECK_STATUS(status);
  xP[0] = 100;
  yP[0] = 100;
  xP[1] = 200;
  yP[1] = 200;
  xP[2] = 300;
  yP[2] = 300;
  xP[3] = 400;
  yP[3] = 400;
  xP[4] = 500;
  yP[4] = 500;
  xP[5] = 600;
  yP[5] = 600;
  status = defwViaPolygon("M5", 6, xP, yP, 3);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);

  xP[0] = 200;
  yP[0] = 200;
  xP[1] = 300;
  yP[1] = 300;
  xP[2] = 400;
  yP[2] = 500;
  xP[3] = 100;
  yP[3] = 300;
  xP[4] = 300;
  yP[4] = 200;
  status = defwViaName("via5");
  CHECK_STATUS(status);
  status = defwViaPolygon("M6", 5, xP, yP);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);
  status = defwEndVias();
  CHECK_STATUS(status);

  // REGIONS
  status = defwStartRegions(2);
  CHECK_STATUS(status);
  status = defwRegionName("region1");
  CHECK_STATUS(status);
  status = defwRegionPoints(-500, -500, 300, 100);
  CHECK_STATUS(status);
  status = defwRegionPoints(500, 500, 1000, 1000);
  CHECK_STATUS(status);
  status = defwRegionType("FENCE");
  CHECK_STATUS(status);
  status = defwStringProperty("scum", "on top");
  CHECK_STATUS(status);
  status = defwIntProperty("center", 250);
  CHECK_STATUS(status);
  status = defwIntProperty("area", 730000);
  CHECK_STATUS(status);
  status = defwRegionName("region2");
  CHECK_STATUS(status);
  status = defwRegionPoints(4000, 0, 5000, 1000);
  CHECK_STATUS(status);
  status = defwStringProperty("scum", "on bottom");
  CHECK_STATUS(status);
  status = defwEndRegions();
  CHECK_STATUS(status);

  // COMPONENTMASKSHIFTLAYER
  shiftLayers = (const char**) malloc(sizeof(char*) * 2);
  shiftLayers[0] = strdup("M3");
  shiftLayers[1] = strdup("M2");

  status = defwComponentMaskShiftLayers(shiftLayers, 2);

  free((char*) shiftLayers[0]);
  free((char*) shiftLayers[1]);
  free((char*) shiftLayers);

  // COMPONENTS
  foreigns = (const char**) malloc(sizeof(char*) * 2);
  foreignX = (int*) malloc(sizeof(int) * 2);
  foreignY = (int*) malloc(sizeof(int) * 2);
  foreignOrient = (int*) malloc(sizeof(int) * 2);
  foreignOrientStr = (const char**) malloc(sizeof(char*) * 2);
  status = defwStartComponents(11);
  CHECK_STATUS(status);
  status = defwComponent("Z38A01",
                         "DFF3",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "PLACED",
                         18592,
                         5400,
                         6,
                         0,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponentMaskShift(123);
  CHECK_STATUS(status);
  status = defwComponentHalo(100, 0, 50, 200);
  CHECK_STATUS(status);
  status = defwComponentStr("Z38A03",
                            "DFF3",
                            0,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            0,
                            nullptr,
                            nullptr,
                            nullptr,
                            nullptr,
                            "PLACED",
                            16576,
                            45600,
                            "FS",
                            0,
                            nullptr,
                            0,
                            0,
                            0,
                            0);
  CHECK_STATUS(status);
  status = defwComponentHalo(200, 2, 60, 300);
  CHECK_STATUS(status);
  status = defwComponent("Z38A05",
                         "DFF3",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "PLACED",
                         51520,
                         9600,
                         6,
                         0,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponent("|i0",
                         "INV_B",
                         0,
                         nullptr,
                         "INV",
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         0,
                         -1,
                         0,
                         "region1",
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponentHaloSoft(100, 0, 50, 200);
  CHECK_STATUS(status);
  status = defwComponent("|i1",
                         "INV_B",
                         0,
                         nullptr,
                         "INV",
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "UNPLACED",
                         1000,
                         1000,
                         0,
                         0,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponent("cell1",
                         "CHM6A",
                         0,
                         nullptr,
                         nullptr,
                         "generator",
                         nullptr,
                         "USER",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "FIXED",
                         0,
                         10,
                         0,
                         100.4534535,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponent("cell2",
                         "CHM6A",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "NETLIST",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "COVER",
                         120,
                         10,
                         4,
                         2,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  foreigns[0] = strdup("gds2name");
  foreignX[0] = -500;
  foreignY[0] = -500;
  foreignOrient[0] = 3;
  status = defwComponent("cell3",
                         "CHM6A",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "TIMING",
                         1,
                         foreigns,
                         foreignX,
                         foreignY,
                         foreignOrient,
                         "PLACED",
                         240,
                         10,
                         0,
                         0,
                         "region1",
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponentRouteHalo(100, "metal1", "metal3");
  CHECK_STATUS(status);
  free((char*) foreigns[0]);
  foreigns[0] = strdup("gds3name");
  foreignX[0] = -500;
  foreignY[0] = -500;
  foreignOrientStr[0] = strdup("FW");
  foreigns[1] = strdup("gds4name");
  foreignX[1] = -300;
  foreignY[1] = -300;
  foreignOrientStr[1] = strdup("FS");
  status = defwComponentStr("cell4",
                            "CHM3A",
                            0,
                            nullptr,
                            "CHM6A",
                            nullptr,
                            nullptr,
                            "DIST",
                            2,
                            foreigns,
                            foreignX,
                            foreignY,
                            foreignOrientStr,
                            "PLACED",
                            360,
                            10,
                            "W",
                            0,
                            "region2",
                            0,
                            0,
                            0,
                            0);
  CHECK_STATUS(status);
  status = defwComponentHaloSoft(100, 0, 50, 200);
  CHECK_STATUS(status);
  status = defwStringProperty("cc", "This is the copy list");
  CHECK_STATUS(status);
  status = defwIntProperty("index", 9);
  CHECK_STATUS(status);
  status = defwRealProperty("size", 7.8);
  CHECK_STATUS(status);
  status = defwComponent("scancell1",
                         "CHK3A",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "PLACED",
                         500,
                         10,
                         7,
                         0,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwComponent("scancell2",
                         "CHK3A",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "PLACED",
                         700,
                         10,
                         6,
                         0,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
  CHECK_STATUS(status);
  status = defwEndComponents();
  CHECK_STATUS(status);
  free((char*) foreigns[0]);
  free((char*) foreigns[1]);
  free((char*) foreigns);
  free((char*) foreignX);
  free((char*) foreignY);
  free((char*) foreignOrient);
  free((char*) foreignOrientStr[0]);
  free((char*) foreignOrientStr[1]);
  free((char*) foreignOrientStr);

  xP = (double*) malloc(sizeof(double) * 6);
  yP = (double*) malloc(sizeof(double) * 6);
  xP[0] = 2.1;
  yP[0] = 2.1;
  xP[1] = 3.1;
  yP[1] = 3.1;
  xP[2] = 4.1;
  yP[2] = 4.1;
  xP[3] = 5.1;
  yP[3] = 5.1;
  xP[4] = 6.1;
  yP[4] = 6.1;
  xP[5] = 7.1;
  yP[5] = 7.1;

  // PINS
  status = defwStartPins(11);
  CHECK_STATUS(status);
  status = defwPin("scanpin",
                   "net1",
                   0,
                   "INPUT",
                   nullptr,
                   nullptr,
                   0,
                   0,
                   -1,
                   nullptr,
                   0,
                   0,
                   0,
                   0);
  CHECK_STATUS(status);
  status = defwPinPolygon("metal1", 0, 1000, 6, xP, yP);
  CHECK_STATUS(status);
  status = defwPinNetExpr("power1 VDD1");
  CHECK_STATUS(status);
  status = defwPin("pin0",
                   "net1",
                   0,
                   "INPUT",
                   "SCAN",
                   nullptr,
                   0,
                   0,
                   -1,
                   nullptr,
                   0,
                   0,
                   0,
                   0);
  CHECK_STATUS(status);
  status = defwPinStr("pin0.5",
                      "net1",
                      0,
                      "INPUT",
                      "RESET",
                      "FIXED",
                      0,
                      0,
                      "S",
                      nullptr,
                      0,
                      0,
                      0,
                      0);
  CHECK_STATUS(status);
  status = defwPinPolygon("metal2", 0, 0, 4, xP, yP);
  CHECK_STATUS(status);
  status = defwPinLayer("metal3", 500, 0, -5000, -100, -4950, -90);
  CHECK_STATUS(status);
  status = defwPin("pin1",
                   "net1",
                   1,
                   nullptr,
                   "POWER",
                   nullptr,
                   0,
                   0,
                   -1,
                   "M1",
                   -5000,
                   -100,
                   -4950,
                   -90);
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalArea(4580, "M1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalArea(4580, "M11");
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalArea(4580, "M12");
  CHECK_STATUS(status);
  status = defwPinAntennaPinGateArea(4580, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinDiffArea(4580, "M3");
  CHECK_STATUS(status);
  status = defwPinAntennaPinDiffArea(4580, "M31");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxAreaCar(5000, "L1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxSideAreaCar(5000, "M4");
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialCutArea(4580, "M4");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxCutCar(5000, "L1");
  CHECK_STATUS(status);
  status = defwPin("pin2",
                   "net2",
                   0,
                   "INPUT",
                   "SIGNAL",
                   nullptr,
                   0,
                   0,
                   -1,
                   "M1",
                   -5000,
                   0,
                   -4950,
                   10);
  CHECK_STATUS(status);
  status = defwPinLayer("M1", 500, 0, -5000, 0, -4950, 10);
  CHECK_STATUS(status);
  status = defwPinPolygon("M2", 0, 0, 4, xP, yP);
  CHECK_STATUS(status);
  status = defwPinPolygon("M3", 0, 0, 3, xP, yP);
  CHECK_STATUS(status);
  status = defwPinLayer("M4", 0, 500, 0, 100, -400, 100);
  CHECK_STATUS(status);
  status = defwPinSupplySensitivity("vddpin1");
  CHECK_STATUS(status);
  status = defwPinGroundSensitivity("gndpin1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalArea(5000, nullptr);
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalSideArea(4580, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinGateArea(5000, nullptr);
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialCutArea(5000, nullptr);
  CHECK_STATUS(status);
  status = defwPin("INBUS[1]",
                   "|INBUS[1]",
                   0,
                   "INPUT",
                   "SIGNAL",
                   "FIXED",
                   45,
                   -2160,
                   0,
                   "M2",
                   0,
                   0,
                   30,
                   135);
  CHECK_STATUS(status);
  status = defwPinLayer("M2", 0, 0, 0, 0, 30, 135);
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalArea(1, "M1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialMetalSideArea(2, "M1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinDiffArea(4, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinPartialCutArea(5, "V1");
  CHECK_STATUS(status);
  status = defwPinAntennaModel("OXIDE1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinGateArea(3, "M1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxAreaCar(6, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxSideAreaCar(7, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxCutCar(8, "V1");
  CHECK_STATUS(status);
  status = defwPinAntennaModel("OXIDE2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinGateArea(30, "M1");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxAreaCar(60, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxSideAreaCar(70, "M2");
  CHECK_STATUS(status);
  status = defwPinAntennaPinMaxCutCar(80, "V1");
  CHECK_STATUS(status);
  status = defwPin("INBUS<0>",
                   "|INBUS<0>",
                   0,
                   "INPUT",
                   "SIGNAL",
                   "PLACED",
                   -45,
                   2160,
                   1,
                   "M2",
                   0,
                   0,
                   30,
                   134);
  CHECK_STATUS(status);
  status = defwPinLayer("M2", 0, 1000, 0, 0, 30, 134);
  CHECK_STATUS(status);
  status = defwPin("OUTBUS<1>",
                   "|OUTBUS<1>",
                   0,
                   "OUTPUT",
                   "SIGNAL",
                   "COVER",
                   2160,
                   645,
                   2,
                   "M1",
                   0,
                   0,
                   30,
                   135);
  CHECK_STATUS(status);
  status = defwPinLayer("M1", 0, 0, 0, 0, 30, 134);
  CHECK_STATUS(status);
  status = defwPinNetExpr("gnd1 GND");
  CHECK_STATUS(status);
  status = defwPin("VDD",
                   "VDD",
                   1,
                   "INOUT",
                   "POWER",
                   nullptr,
                   0,
                   0,
                   -1,
                   nullptr,
                   0,
                   0,
                   0,
                   0);
  CHECK_STATUS(status);
  status = defwPin("BUSA[0]",
                   "BUSA[0]",
                   0,
                   "INPUT",
                   "SIGNAL",
                   "PLACED",
                   0,
                   2500,
                   1,
                   nullptr,
                   0,
                   0,
                   0,
                   0);
  CHECK_STATUS(status);
  status = defwPinLayer("M1", 0, 0, -25, 0, 25, 50);
  CHECK_STATUS(status);
  status = defwPinLayer("M2", 0, 0, -10, 0, 10, 75);
  CHECK_STATUS(status);
  status = defwPinVia("via12", 0, 25);
  CHECK_STATUS(status);
  status = defwPin("VDD",
                   "VDD",
                   1,
                   "INOUT",
                   "POWER",
                   nullptr,
                   0,
                   0,
                   -1,
                   nullptr,
                   0,
                   0,
                   0,
                   0);
  CHECK_STATUS(status);
  status = defwPinPort();
  CHECK_STATUS(status);
  status = defwPinPortLayer("M2", 0, 0, -25, 0, 25, 50);
  CHECK_STATUS(status);
  status = defwPinPortLocation("PLACED", 0, 2500, "S");
  CHECK_STATUS(status);
  status = defwPinPort();
  CHECK_STATUS(status);
  status = defwPinPortLayer("M1", 0, 0, -25, 0, 25, 50);
  CHECK_STATUS(status);
  status = defwPinPortLocation("COVER", 0, 2500, "S");
  CHECK_STATUS(status);
  status = defwPinPort();
  CHECK_STATUS(status);
  status = defwPinPortLayer("M1", 0, 0, -25, 0, 25, 50, 2);
  CHECK_STATUS(status);
  status = defwPinPortLocation("FIXED", 0, 2500, "S");
  CHECK_STATUS(status);
  status = defwPinPortPolygon("M1", 0, 2, 6, xP, yP);
  CHECK_STATUS(status);
  status = defwPinPortPolygon("M2", 5, 0, 6, xP, yP, 1);
  CHECK_STATUS(status);
  status = defwPinPortVia("M2", 5, 4, 112);
  CHECK_STATUS(status);

  status = defwEndPins();
  CHECK_STATUS(status);

  free((char*) xP);
  free((char*) yP);

  // PINPROPERTIES
  status = defwStartPinProperties(2);
  CHECK_STATUS(status);
  status = defwPinProperty("cell1", "PB1");
  CHECK_STATUS(status);
  status = defwStringProperty("dpBit", "1");
  CHECK_STATUS(status);
  status = defwRealProperty("realProperty", 3.4);
  CHECK_STATUS(status);
  status = defwPinProperty("cell2", "vdd");
  CHECK_STATUS(status);
  status = defwIntProperty("dpIgnoreTerm", 2);
  CHECK_STATUS(status);
  status = defwEndPinProperties();
  CHECK_STATUS(status);

  // SPECIALNETS
  status = defwStartSpecialNets(7);
  CHECK_STATUS(status);
  status = defwSpecialNet("net1");
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell1", "VDD", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell2", "VDD", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell3", "VDD", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell4", "VDD", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetWidth("M1", 200);
  CHECK_STATUS(status);
  status = defwSpecialNetWidth("M2", 300);
  CHECK_STATUS(status);
  status = defwSpecialNetVoltage(3.2);
  CHECK_STATUS(status);
  status = defwSpecialNetSpacing("M1", 200, 190, 210);
  CHECK_STATUS(status);
  status = defwSpecialNetSource("TIMING");
  CHECK_STATUS(status);
  status = defwSpecialNetOriginal("VDD");
  CHECK_STATUS(status);
  status = defwSpecialNetUse("POWER");
  CHECK_STATUS(status);
  status = defwSpecialNetWeight(30);
  CHECK_STATUS(status);
  status = defwStringProperty("contype", "star");
  CHECK_STATUS(status);
  status = defwIntProperty("ind", 1);
  CHECK_STATUS(status);
  status = defwRealProperty("maxlength", 12.13);
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  status = defwSpecialNet("VSS");
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell1", "GND", 1);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell2", "GND", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell3", "GND", 1);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell4", "GND", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetUse("SCAN");
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M1");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(250);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("IOWIRE");
  CHECK_STATUS(status);
  coorX = (double*) malloc(sizeof(double) * 6);
  coorY = (double*) malloc(sizeof(double) * 6);
  coorValue = (double*) malloc(sizeof(double) * 6);
  coorX[0] = 5.0;
  coorY[0] = 15.0;
  coorValue[0] = 0;
  coorX[1] = 125.0;
  coorY[1] = 15.0;
  coorValue[1] = 235.0;
  coorX[2] = 245.0;
  coorY[2] = 15.0;
  coorValue[2] = 255.0;
  status = defwSpecialNetPathPointWithWireExt(3, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwSpecialNetPathEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetShieldStart("my_net");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldWidth(90);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldShape("STRIPE");
  CHECK_STATUS(status);
  coorX[0] = 14100.0;
  coorY[0] = 342440.0;
  coorX[1] = 13920.0;
  coorY[1] = 342440.0;
  status = defwSpecialNetShieldPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M2_TURN");
  CHECK_STATUS(status);
  coorX[0] = 14100.0;
  coorY[0] = 263200.0;
  status = defwSpecialNetShieldPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M1_M2");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldViaData(10, 20, 1000, 2000);
  CHECK_STATUS(status);
  coorX[0] = 2400.0;
  coorY[0] = 263200.0;
  status = defwSpecialNetShieldPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetShieldStart("my_net1");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldWidth(90);
  CHECK_STATUS(status);
  coorX[0] = 14100.0;
  coorY[0] = 342440.0;
  coorX[1] = 13920.0;
  coorY[1] = 342440.0;
  status = defwSpecialNetShieldPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M2_TURN");
  CHECK_STATUS(status);
  coorX[0] = 13920.0;
  coorY[0] = 263200.0;
  status = defwSpecialNetShieldPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M1_M2");
  CHECK_STATUS(status);
  coorX[0] = 2400.0;
  coorY[0] = 263200.0;
  status = defwSpecialNetShieldPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetPattern("STEINER");
  CHECK_STATUS(status);
  status = defwSpecialNetEstCap(100);
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  status = defwSpecialNet("VDD");
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("*", "VDD", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("metal2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(100);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("RING");
  CHECK_STATUS(status);
  status = defwSpecialNetPathStyle(1);
  CHECK_STATUS(status);
  coorX[0] = 0.0;
  coorY[0] = 0.0;
  coorX[1] = 100.0;
  coorY[1] = 100.0;
  coorX[2] = 200.0;
  coorY[2] = 100.0;
  status = defwSpecialNetPathPoint(3, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(270);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("PADRING");
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(270);
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  status = defwSpecialNet("CLOCK");
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(200);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("BLOCKRING");
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(270);
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  status = defwSpecialNet("VCC");
  CHECK_STATUS(status);
  /*
    status = defwSpecialNetShieldNetName("ShieldName");
  */
  status = defwSpecialNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(200);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("DRCFILL");
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(270);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("STRIPE");
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathMask(31);
  CHECK_STATUS(status);
  status = defwSpecialNetPathVia("VIAGEN21_2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathViaData(10, 20, 10000, 20000);
  CHECK_STATUS(status);
  status = defwSpecialNetPathEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  status = defwSpecialNet("n1");
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("PIN", "n1", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("driver1", "in", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("bumpa1", "bumppin", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetFixedbump();
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(200);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("FILLWIREOPC");
  CHECK_STATUS(status);
  coorX[0] = -45.0;
  coorY[0] = 1350.0;
  coorX[1] = 44865.0;
  coorY[1] = 1350.0;
  status = defwSpecialNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathEnd();
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  free((char*) coorX);
  free((char*) coorY);
  free((char*) coorValue);

  status = defwSpecialNet("VSS1");
  CHECK_STATUS(status);
  status = defwSpecialNetUse("POWER");
  CHECK_STATUS(status);
  xP = (double*) malloc(sizeof(double) * 6);
  yP = (double*) malloc(sizeof(double) * 6);
  xP[0] = 2.1;
  yP[0] = 2.1;
  xP[1] = 3.1;
  yP[1] = 3.1;
  xP[2] = 4.1;
  yP[2] = 4.1;
  xP[3] = 5.1;
  yP[3] = 5.1;
  xP[4] = 6.1;
  yP[4] = 6.1;
  xP[5] = 7.1;
  yP[5] = 7.1;
  status = defwSpecialNetPolygon("metal1", 4, xP, yP);
  CHECK_STATUS(status);
  status = defwSpecialNetPolygon("metal1", 6, xP, yP);
  CHECK_STATUS(status);
  status = defwSpecialNetRect("metal1", 0, 0, 100, 200);
  CHECK_STATUS(status);
  status = defwSpecialNetMask(2);
  CHECK_STATUS(status);
  status = defwSpecialNetRect("metal2", 1, 1, 100, 200);
  CHECK_STATUS(status);
  status = defwSpecialNetVia("metal2");
  CHECK_STATUS(status);
  status = defwSpecialNetViaPoints(4, xP, yP);
  CHECK_STATUS(status);
  status = defwSpecialNetEndOneNet();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);
  status = defwEndSpecialNets();
  CHECK_STATUS(status);

  // NETS
  status = defwStartNets(13);
  CHECK_STATUS(status);
  status = defwNet("net1");
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A01", "Q", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A03", "Q", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A05", "Q", 0);
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("net2");
  CHECK_STATUS(status);
  status = defwNetConnection("cell1", "PB1", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("cell2", "PB1", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("cell3", "PB1", 0);
  CHECK_STATUS(status);
  status = defwNetEstCap(200);
  CHECK_STATUS(status);
  status = defwNetWeight(2);
  CHECK_STATUS(status);
  status = defwNetVpin("P1", nullptr, 0, 0, 0, 0, "PLACED", 54, 64, 3);
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("net3");
  CHECK_STATUS(status);
  status = defwNetConnection("cell4", "PA3", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("cell2", "P10", 0);
  CHECK_STATUS(status);
  status = defwNetXtalk(30);
  CHECK_STATUS(status);
  status = defwNetOriginal("extra_crispy");
  CHECK_STATUS(status);
  status = defwNetSource("USER");
  CHECK_STATUS(status);
  status = defwNetUse("SIGNAL");
  CHECK_STATUS(status);
  status = defwNetFrequency(100);
  CHECK_STATUS(status);
  status = defwIntProperty("alt", 37);
  CHECK_STATUS(status);
  status = defwStringProperty("lastName", "Unknown");
  CHECK_STATUS(status);
  status = defwRealProperty("length", 10.11);
  CHECK_STATUS(status);
  status = defwNetPattern("BALANCED");
  CHECK_STATUS(status);
  status = defwNetVpinStr("P2", "L1", 45, 54, 3, 46, "FIXED", 23, 12, "FN");
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  coorX = (double*) malloc(sizeof(double) * 3);
  coorY = (double*) malloc(sizeof(double) * 3);
  status = defwNet("my_net");
  CHECK_STATUS(status);
  status = defwNetConnection("I1", "A", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("BUF", "Z", 0);
  CHECK_STATUS(status);
  status = defwNetNondefaultRule("RULE1");
  CHECK_STATUS(status);
  status = defwNetUse("RESET");
  CHECK_STATUS(status);
  status = defwNetShieldnet("VSS");
  CHECK_STATUS(status);
  status = defwNetShieldnet("VDD");
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M2", 0, nullptr);
  CHECK_STATUS(status);
  status = defwNetPathStyle(2);
  CHECK_STATUS(status);
  coorX[0] = 14000.0;
  coorY[0] = 341440.0;
  coorX[1] = 9600.0;
  coorY[1] = 341440.0;
  coorX[2] = 9600.0;
  coorY[2] = 282400.0;
  status = defwNetPathPoint(3, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA12");
  CHECK_STATUS(status);
  coorX[0] = 2400;
  coorY[0] = 2400;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 1, nullptr);
  CHECK_STATUS(status);
  status = defwNetPathStyle(4);
  CHECK_STATUS(status);
  coorX[0] = 2400.0;
  coorY[0] = 282400.0;
  coorX[1] = 240.0;
  coorY[1] = 282400.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  free((char*) coorX);
  free((char*) coorY);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetNoshieldStart("M2");
  CHECK_STATUS(status);
  coorXSN = (const char**) malloc(sizeof(char*) * 2);
  coorYSN = (const char**) malloc(sizeof(char*) * 2);
  coorXSN[0] = strdup("14100");
  coorYSN[0] = strdup("341440");
  coorXSN[1] = strdup("14000");
  coorYSN[1] = strdup("341440");
  status = defwNetNoshieldPoint(2, coorXSN, coorYSN);
  CHECK_STATUS(status);
  status = defwNetNoshieldVia("VIA4");
  CHECK_STATUS(status);
  status = defwNetNoshieldEnd();
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("|INBUS[1]");
  CHECK_STATUS(status);
  status = defwNetConnection("|i1", "A", 0);
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("|INBUS<0>");
  CHECK_STATUS(status);
  status = defwNetConnection("|i0", "A", 0);
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("|OUTBUS<1>");
  CHECK_STATUS(status);
  status = defwNetConnection("|i0", "Z", 0);
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("MUSTJOIN");
  CHECK_STATUS(status);
  status = defwNetConnection("cell4", "PA1", 0);
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("XX100");
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A05", "G", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A03", "G", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A01", "G", 0);
  CHECK_STATUS(status);
  status = defwNetVpin("V_SUB3_XX100",
                       nullptr,
                       -333,
                       -333,
                       333,
                       333,
                       "PLACED",
                       189560,
                       27300,
                       0);
  CHECK_STATUS(status);
  status = defwNetVpin("V_SUB2_XX100",
                       nullptr,
                       -333,
                       -333,
                       333,
                       333,
                       "PLACED",
                       169400,
                       64500,
                       0);
  CHECK_STATUS(status);
  status = defwNetVpin(
      "V_SUB1_XX100", nullptr, -333, -333, 333, 333, "PLACED", 55160, 31500, 0);
  CHECK_STATUS(status);
  status = defwNetSubnetStart("SUB1_XX100");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("Z38A05", "G");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("VPIN", "V_SUB1_XX100");
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, "RULE1");
  CHECK_STATUS(status);
  free((char*) coorXSN[0]);
  free((char*) coorYSN[0]);
  free((char*) coorXSN[1]);
  free((char*) coorYSN[1]);
  free((char*) coorXSN);
  free((char*) coorYSN);
  coorX = (double*) malloc(sizeof(double) * 5);
  coorY = (double*) malloc(sizeof(double) * 5);
  coorValue = (double*) malloc(sizeof(double) * 5);
  coorX[0] = 54040.0;
  coorY[0] = 30300.0;
  coorX[1] = 54040.0;
  coorY[1] = 30900.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA12");
  CHECK_STATUS(status);
  coorX[0] = 54040.0;
  coorY[0] = 30900.0;
  coorX[1] = 56280.0;
  coorY[1] = 30900.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathViaWithOrient("nd1VIA23", 6);
  CHECK_STATUS(status);
  coorX[0] = 56280.0;
  coorY[0] = 31500.0;
  coorX[1] = 55160.0;
  coorY[1] = 31500.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetSubnetEnd();
  CHECK_STATUS(status);
  status = defwNetSubnetStart("SUB2_XX100");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("Z38A03", "G");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("VPIN", "V_SUB2_XX100");
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 168280.0;
  coorY[0] = 63300.0;
  coorValue[0] = 7.0;
  coorX[1] = 168280.0;
  coorY[1] = 64500.0;
  coorValue[1] = 0;
  status = defwNetPathPointWithExt(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("M1_M2");
  CHECK_STATUS(status);
  coorX[0] = 169400.0;
  coorY[0] = 64500.0;
  coorValue[0] = 8.0;
  status = defwNetPathPointWithExt(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathViaWithOrientStr("M2_M3", "SE");
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetSubnetEnd();
  CHECK_STATUS(status);
  status = defwNetSubnetStart("SUB3_XX100");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("Z38A01", "G");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("VPIN", "V_SUB3_XX100");
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 188400.0;
  coorY[0] = 26100.0;
  coorX[1] = 188400.0;
  coorY[1] = 27300.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("M1_M2");
  CHECK_STATUS(status);
  coorX[0] = 189560.0;
  coorY[0] = 27300.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("M1_M2");
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetSubnetEnd();
  CHECK_STATUS(status);
  status = defwNetSubnetStart("SUB0_XX100");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("VPIN", "V_SUB1_XX100");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("VPIN", "V_SUB2_XX100");
  CHECK_STATUS(status);
  status = defwNetSubnetPin("VPIN", "V_SUB3_XX100");
  CHECK_STATUS(status);
  status = defwNetNondefaultRule("RULE1");
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M3", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 269400.0;
  coorY[0] = 64500.0;
  coorX[1] = 269400.0;
  coorY[1] = 54900.0;
  coorX[2] = 170520.0;
  coorY[2] = 54900.0;
  coorX[3] = 170520.0;
  coorY[3] = 37500.0;
  coorX[4] = 170520.0;
  coorY[4] = 30300.0;
  status = defwNetPathPoint(5, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = 171080.0;
  coorY[0] = 30300.0;
  coorX[1] = 17440.0;
  coorY[1] = 0.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = 17440.0;
  coorY[0] = 0.0;
  coorValue[0] = 0;
  coorX[1] = 17440.0;
  coorY[1] = 26700.0;
  coorValue[1] = 8.0;
  status = defwNetPathPointWithExt(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = 177800.0;
  coorY[0] = 26700.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = 177800.0;
  coorY[0] = 26700.0;
  coorValue[0] = 8.0;
  coorX[1] = 177800.0;
  coorY[1] = 30300.0;
  coorValue[1] = 8.0;
  status = defwNetPathPointWithExt(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = 189560.0;
  coorY[0] = 30300.0;
  ;
  coorValue[0] = 8.0;
  status = defwNetPathPointWithExt(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA12");
  CHECK_STATUS(status);
  coorX[0] = 189560.0;
  coorY[0] = 27300.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M3", 1, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 55160.0;
  coorY[0] = 31500.0;
  coorValue[0] = 8.0;
  coorX[1] = 55160.0;
  coorY[1] = 34500.0;
  coorValue[1] = 0.0;
  status = defwNetPathPointWithExt(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  coorX[0] = 149800.0;
  coorY[0] = 34500.0;
  coorValue[0] = 8.0;
  status = defwNetPathPointWithExt(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  coorX[0] = 149800.0;
  coorY[0] = 35700.0;
  coorX[1] = 149800.0;
  coorY[1] = 35700.0;
  status = defwNetPathPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  coorX[0] = 149800.0;
  coorY[0] = 37500.0;
  coorValue[0] = 8.0;
  ;
  coorX[1] = 170520.0;
  coorY[1] = 37500.0;
  coorValue[1] = 0.0;
  status = defwNetPathPointWithExt(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("SCAN");
  CHECK_STATUS(status);
  status = defwNetConnection("scancell1", "P10", 1);
  CHECK_STATUS(status);
  status = defwNetConnection("scancell2", "PA0", 1);
  CHECK_STATUS(status);
  status = defwNetSource("TEST");
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNet("testBug");
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A05", "G", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A03", "G", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("Z38A01", "G", 0);
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M2", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 100.0;
  coorY[0] = 100.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  coorX[0] = 500.0;
  coorY[0] = 100.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVirtual(700, 100);
  CHECK_STATUS(status);
  status = defwNetPathMask(2);
  CHECK_STATUS(status);
  status = defwNetPathRect(-300, 100, -100, 300);
  CHECK_STATUS(status);
  coorX[0] = 700.0;
  coorY[0] = 700.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 1288210.0;
  coorY[0] = 580930.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathMask(31);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH1W1W1");
  CHECK_STATUS(status);
  coorX[0] = 1288210.0;
  coorY[0] = 582820.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH2W1W1");
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M3", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 1141350.0;
  coorY[0] = 582820.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathMask(3);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH2W1W1");
  CHECK_STATUS(status);
  coorX[0] = 1141350.0;
  coorY[0] = 580930.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH1W1W1");
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 1278410.0;
  coorY[0] = 275170.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = 1141210.0;
  coorY[0] = 271250.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH1W1W1");
  CHECK_STATUS(status);
  coorX[0] = 1141210.0;
  coorY[0] = 271460.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH2W1W1");
  CHECK_STATUS(status);
  coorX[0] = 1142820.0;
  coorY[0] = 271460.0;
  status = defwNetPathPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwNetPathVia("GETH3W1W1");
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);
  free((char*) coorX);
  free((char*) coorY);
  free((char*) coorValue);

  status = defwNet("n1");
  CHECK_STATUS(status);
  status = defwNetConnection("PIN", "n1", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("driver1", "in", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("bumpa1", "bumppin", 0);
  CHECK_STATUS(status);
  status = defwNetFixedbump();
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwNetMustjoinConnection("PIN2", "n2");
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  status = defwEndNets();
  CHECK_STATUS(status);

  // IOTIMINGS
  /* obsolete in 5.4
  status = defwStartIOTimings(3);
  CHECK_STATUS(status);
  status = defwIOTiming("PIN", "INBUS<0>");
  CHECK_STATUS(status);
  status = defwIOTimingVariable("RISE", 6100000, 7100000);
  CHECK_STATUS(status);
  status = defwIOTimingVariable("FALL", 3100000, 3100000);
  CHECK_STATUS(status);
  status = defwIOTimingSlewrate("RISE", 110, 110);
  CHECK_STATUS(status);
  status = defwIOTimingSlewrate("FALL", 290, 290);
  CHECK_STATUS(status);
  status = defwIOTimingCapacitance(0);
  CHECK_STATUS(status);
  status = defwIOTiming("PIN", "INBUS[1]");
  CHECK_STATUS(status);
  status = defwIOTimingDrivecell("INV", "A", "Z", 2);
  CHECK_STATUS(status);
  status = defwIOTimingSlewrate("RISE", 110, 110);
  CHECK_STATUS(status);
  status = defwIOTimingSlewrate("FALL", 290, 290);
  CHECK_STATUS(status);
  status = defwIOTimingCapacitance(0);
  CHECK_STATUS(status);
  status = defwIOTiming("PIN", "OUTPUS<1>");
  CHECK_STATUS(status);
  status = defwIOTimingCapacitance(120000);
  CHECK_STATUS(status);
  status = defwEndIOTimings();
  CHECK_STATUS(status);
  */

  // SCANCHAIN
  status = defwStartScanchains(4);
  CHECK_STATUS(status);
  status = defwScanchain("the_chain");
  CHECK_STATUS(status);
  status = defwScanchainCommonscanpins("IN", "PA1", "OUT", "PA2");
  CHECK_STATUS(status);
  status = defwScanchainStart("PIN", "scanpin");
  CHECK_STATUS(status);
  status = defwScanchainStop("cell4", "PA2");
  CHECK_STATUS(status);
  status = defwScanchainOrdered("cell2",
                                "IN",
                                "PA0",
                                nullptr,
                                nullptr,
                                "cell1",
                                "OUT",
                                "P10",
                                nullptr,
                                nullptr);
  CHECK_STATUS(status);
  status = defwScanchainFloating("scancell1", "IN", "PA0", nullptr, nullptr);
  CHECK_STATUS(status);
  status = defwScanchainFloating("scancell2", "OUT", "P10", nullptr, nullptr);
  CHECK_STATUS(status);
  status = defwScanchain("chain1_clock1");
  CHECK_STATUS(status);
  status = defwScanchainPartition("clock1", -1);
  CHECK_STATUS(status);
  status = defwScanchainStart("block1/current_state_reg_0_QZ", nullptr);
  CHECK_STATUS(status);
  status
      = defwScanchainFloating("block1/pgm_cgm_en_reg", "IN", "SD", "OUT", "QZ");
  CHECK_STATUS(status);
  status = defwScanchainFloating(
      "block1/start_reset_dd_reg", "IN", "SD", "OUT", "QZ");
  CHECK_STATUS(status);
  status = defwScanchainStop("block1/start_reset_d_reg", nullptr);
  CHECK_STATUS(status);
  status = defwScanchain("chain2_clock2");
  CHECK_STATUS(status);
  status = defwScanchainPartition("clock2", 1000);
  CHECK_STATUS(status);
  status = defwScanchainStart("block1/current_state_reg_0_QZ", nullptr);
  CHECK_STATUS(status);
  status = defwScanchainFloating(
      "block1/port2_phy_addr_reg_0_", "IN", "SD", "OUT", "QZ ");
  CHECK_STATUS(status);
  status = defwScanchainFloating(
      "block1/port2_phy_addr_reg_4_", "IN", "SD", "OUT", "QZ");
  CHECK_STATUS(status);
  status = defwScanchainFloatingBits(
      "block1/port3_intfc", "IN", "SD", "OUT", "QZ", 4);
  CHECK_STATUS(status);
  status = defwScanchainOrderedBits("block1/mux1",
                                    "IN",
                                    "A",
                                    "OUT",
                                    "X",
                                    0,
                                    "block1/ff2",
                                    "IN",
                                    "SD",
                                    "OUT",
                                    "Q",
                                    -1);
  CHECK_STATUS(status);
  status = defwScanchain("chain4_clock3");
  CHECK_STATUS(status);
  status = defwScanchainPartition("clock3", -1);
  CHECK_STATUS(status);
  status = defwScanchainStart("block1/prescaler_IO/lfsr_reg1", nullptr);
  CHECK_STATUS(status);
  status = defwScanchainFloating(
      "block1/dp1_timers", nullptr, nullptr, nullptr, nullptr);
  CHECK_STATUS(status);
  status = defwScanchainFloatingBits(
      "block1/bus8", nullptr, nullptr, nullptr, nullptr, 8);
  CHECK_STATUS(status);
  status = defwScanchainOrderedBits("block1/dsl/ffl",
                                    "IN",
                                    "SD",
                                    "OUT",
                                    "Q",
                                    -1,
                                    "block1/dsl/mux1",
                                    "IN",
                                    "B",
                                    "OUT",
                                    "Y",
                                    0);
  CHECK_STATUS(status);
  status = defwScanchainOrderedBits("block1/dsl/ff2",
                                    "IN",
                                    "SD",
                                    "OUT",
                                    "Q",
                                    -1,
                                    "block1/dsl/mux2",
                                    "IN",
                                    "B",
                                    "OUT",
                                    "Y",
                                    0);
  CHECK_STATUS(status);
  status = defwScanchainStop("block1/start_reset_d_reg", nullptr);
  CHECK_STATUS(status);

  status = defwEndScanchain();
  CHECK_STATUS(status);

  // CONSTRAINTS
  /* obsolete in 5.4
  status = defwStartConstraints(3);
  CHECK_STATUS(status);
  status = defwConstraintOperand();  // the following are operand
  CHECK_STATUS(status);
  status = defwConstraintOperandPath("cell1", "VDD", "cell2", "VDD");
  CHECK_STATUS(status);
  status = defwConstraintOperandTime("RISEMAX", 6000);
  CHECK_STATUS(status);
  status = defwConstraintOperandTime("FALLMIN", 9000);
  CHECK_STATUS(status);
  status = defwConstraintOperandEnd();
  CHECK_STATUS(status);
  status = defwConstraintOperand();
  CHECK_STATUS(status);
  status = defwConstraintOperandSum();
  CHECK_STATUS(status);
  status = defwConstraintOperandNet("net2");
  CHECK_STATUS(status);
  status = defwConstraintOperandNet("net3");
  CHECK_STATUS(status);
  status = defwConstraintOperandSumEnd();
  CHECK_STATUS(status);
  status = defwConstraintOperandTime("RISEMAX", 2000);
  CHECK_STATUS(status);
  status = defwConstraintOperandTime("FALLMIN", 5000);
  CHECK_STATUS(status);
  status = defwConstraintOperandEnd();
  CHECK_STATUS(status);
  status = defwConstraintWiredlogic("net1", 1000);
  CHECK_STATUS(status);
  status = defwEndConstraints();
  CHECK_STATUS(status);
  */

  // GROUPS
  groupExpr = (const char**) malloc(sizeof(char*) * 2);
  status = defwStartGroups(2);
  CHECK_STATUS(status);
  groupExpr[0] = strdup("cell2");
  groupExpr[1] = strdup("cell3");
  status = defwGroup("group1", 2, groupExpr);
  CHECK_STATUS(status);
  free((char*) groupExpr[0]);
  free((char*) groupExpr[1]);
  status = defwGroupRegion(0, 0, 0, 0, "region1");
  CHECK_STATUS(status);
  status = defwStringProperty("ggrp", "xx");
  CHECK_STATUS(status);
  status = defwIntProperty("side", 2);
  CHECK_STATUS(status);
  status = defwRealProperty("maxarea", 5.6);
  CHECK_STATUS(status);
  groupExpr[0] = strdup("cell1");
  status = defwGroup("group2", 1, groupExpr);
  CHECK_STATUS(status);
  free((char*) groupExpr[0]);
  status = defwGroupRegion(0, 10, 1000, 1010, nullptr);
  CHECK_STATUS(status);
  status = defwStringProperty("ggrp", "after the fall");
  CHECK_STATUS(status);
  status = defwGroupSoft("MAXHALFPERIMETER", 4000, "MAXX", 10000, nullptr, 0);
  CHECK_STATUS(status);
  status = defwEndGroups();
  CHECK_STATUS(status);
  free((char*) groupExpr);
  status = defwNewLine();
  CHECK_STATUS(status);

  // BLOCKAGES
  int *xPB, *yPB;
  xPB = (int*) malloc(sizeof(int) * 7);
  yPB = (int*) malloc(sizeof(int) * 7);
  xPB[0] = 2;
  yPB[0] = 2;
  xPB[1] = 3;
  yPB[1] = 3;
  xPB[2] = 4;
  yPB[2] = 4;
  xPB[3] = 5;
  yPB[3] = 5;
  xPB[4] = 6;
  yPB[4] = 6;
  xPB[5] = 7;
  yPB[5] = 7;
  xPB[6] = 8;
  yPB[6] = 8;

  status = defwStartBlockages(13);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("m1");
  CHECK_STATUS(status);
  status = defwBlockagesLayerComponent("comp1");
  CHECK_STATUS(status);
  status = defwBlockagesRect(3456, 4535, 3000, 4000);
  CHECK_STATUS(status);
  status = defwBlockagesRect(4500, 6500, 5500, 6000);
  CHECK_STATUS(status);
  status = defwBlockagesPolygon(7, xPB, yPB);
  CHECK_STATUS(status);
  status = defwBlockagesPolygon(6, xPB, yPB);
  CHECK_STATUS(status);
  status = defwBlockagesRect(5000, 6000, 4000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesPlacement();
  CHECK_STATUS(status);
  status = defwBlockagesPlacementComponent("m2");
  CHECK_STATUS(status);
  status = defwBlockagesRect(4000, 6000, 8000, 4000);
  CHECK_STATUS(status);
  status = defwBlockagesRect(8000, 400, 600, 800);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("m3");
  CHECK_STATUS(status);
  status = defwBlockagesLayerSpacing(1000);
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("m4");
  CHECK_STATUS(status);
  status = defwBlockagesLayerSpacing(100);
  CHECK_STATUS(status);
  status = defwBlockagesLayerExceptpgnet();
  CHECK_STATUS(status);
  status = defwBlockagesLayerComponent("U2726");
  CHECK_STATUS(status);
  status = defwBlockagesLayerPushdown();
  CHECK_STATUS(status);
  status = defwBlockagesLayerMask(3);
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("m4");
  CHECK_STATUS(status);
  status = defwBlockagesLayerComponent("U2726");
  CHECK_STATUS(status);
  status = defwBlockagesLayerPushdown();
  CHECK_STATUS(status);
  status = defwBlockagesLayerDesignRuleWidth(1000);
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("m5");
  CHECK_STATUS(status);
  status = defwBlockagesLayerFills();
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("m6");
  CHECK_STATUS(status);
  status = defwBlockagesLayerPushdown();
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesPolygon(7, xPB, yPB);
  CHECK_STATUS(status);
  status = defwBlockagesPlacement();
  CHECK_STATUS(status);
  status = defwBlockagesPlacementComponent("m7");
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesPlacement();
  CHECK_STATUS(status);
  status = defwBlockagesPlacementPushdown();
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesPlacement();
  CHECK_STATUS(status);
  status = defwBlockagesRect(3000, 4000, 6000, 5000);
  CHECK_STATUS(status);
  status = defwBlockagesPlacement();
  CHECK_STATUS(status);
  status = defwBlockagesPlacementSoft();
  CHECK_STATUS(status);
  status = defwBlockagesPlacementComponent("U2729");
  CHECK_STATUS(status);
  status = defwBlockagesPlacementPushdown();
  CHECK_STATUS(status);
  status = defwBlockagesRect(4000, 6000, 8000, 4000);
  CHECK_STATUS(status);
  status = defwBlockagesPlacement();
  CHECK_STATUS(status);
  status = defwBlockagesPlacementPartial(1.1);
  CHECK_STATUS(status);
  status = defwBlockagesRect(4000, 6000, 8000, 4000);
  CHECK_STATUS(status);
  status = defwBlockagesLayer("metal1");
  CHECK_STATUS(status);
  status = defwBlockagesLayerExceptpgnet();
  CHECK_STATUS(status);
  status = defwBlockagesLayerSpacing(4);
  CHECK_STATUS(status);
  status = defwBlockagesPolygon(3, xPB, yPB);
  CHECK_STATUS(status);
  status = defwEndBlockages();
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  free((char*) xPB);
  free((char*) yPB);

  // SLOTS
  xP = (double*) malloc(sizeof(double) * 7);
  yP = (double*) malloc(sizeof(double) * 7);
  xP[0] = 2.1;
  yP[0] = 2.1;
  xP[1] = 3.1;
  yP[1] = 3.1;
  xP[2] = 4.1;
  yP[2] = 4.1;
  xP[3] = 5.1;
  yP[3] = 5.1;
  xP[4] = 6.1;
  yP[4] = 6.1;
  xP[5] = 7.1;
  yP[5] = 7.1;
  xP[6] = 8.1;
  yP[6] = 8.1;
  status = defwStartSlots(2);
  CHECK_STATUS(status);
  status = defwSlotLayer("MET1");
  CHECK_STATUS(status);
  status = defwSlotPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwSlotPolygon(3, xP, yP);
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwSlotRect(2000, 2000, 2500, 4000);
  CHECK_STATUS(status);
  status = defwSlotRect(3000, 2000, 3500, 4000);
  CHECK_STATUS(status);
  status = defwSlotLayer("MET2");
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwSlotPolygon(6, xP, yP);
  CHECK_STATUS(status);
  status = defwEndSlots();
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);

  // FILLS
  xP = (double*) malloc(sizeof(double) * 7);
  yP = (double*) malloc(sizeof(double) * 7);
  xP[0] = 2.1;
  yP[0] = 2.1;
  xP[1] = 3.1;
  yP[1] = 3.1;
  xP[2] = 4.1;
  yP[2] = 4.1;
  xP[3] = 5.1;
  yP[3] = 5.1;
  xP[4] = 6.1;
  yP[4] = 6.1;
  xP[5] = 7.1;
  yP[5] = 7.1;
  xP[6] = 8.1;
  yP[6] = 8.1;
  status = defwStartFills(5);
  CHECK_STATUS(status);
  status = defwFillLayer("MET1");
  CHECK_STATUS(status);
  status = defwFillRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwFillPolygon(5, xP, yP);
  CHECK_STATUS(status);
  status = defwFillRect(2000, 2000, 2500, 4000);
  CHECK_STATUS(status);
  status = defwFillPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwFillRect(3000, 2000, 3500, 4000);
  CHECK_STATUS(status);
  status = defwFillLayer("MET2");
  CHECK_STATUS(status);
  status = defwFillRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwFillRect(1000, 4500, 1500, 6500);
  CHECK_STATUS(status);
  status = defwFillRect(1000, 7000, 1500, 9000);
  CHECK_STATUS(status);
  status = defwFillRect(1000, 9500, 1500, 11500);
  CHECK_STATUS(status);
  status = defwFillPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwFillPolygon(6, xP, yP);
  CHECK_STATUS(status);
  status = defwFillLayer("metal1");
  CHECK_STATUS(status);
  status = defwFillLayerOPC();
  CHECK_STATUS(status);
  status = defwFillRect(100, 200, 150, 400);
  CHECK_STATUS(status);
  status = defwFillRect(300, 200, 350, 400);
  CHECK_STATUS(status);
  status = defwFillVia("via28");
  CHECK_STATUS(status);
  status = defwFillViaOPC();
  CHECK_STATUS(status);
  status = defwFillPoints(1, xP, yP);
  CHECK_STATUS(status);
  status = defwFillVia("via26");
  CHECK_STATUS(status);
  status = defwFillPoints(3, xP, yP);
  CHECK_STATUS(status);
  status = defwEndFills();
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);

  // SLOTS
  xP = (double*) malloc(sizeof(double) * 7);
  yP = (double*) malloc(sizeof(double) * 7);
  xP[0] = 2.1;
  yP[0] = 2.1;
  xP[1] = 3.1;
  yP[1] = 3.1;
  xP[2] = 4.1;
  yP[2] = 4.1;
  xP[3] = 5.1;
  yP[3] = 5.1;
  xP[4] = 6.1;
  yP[4] = 6.1;
  xP[5] = 7.1;
  yP[5] = 7.1;
  xP[6] = 8.1;
  yP[6] = 8.1;
  status = defwStartSlots(2);
  CHECK_STATUS(status);
  status = defwSlotLayer("MET1");
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwSlotPolygon(5, xP, yP);
  CHECK_STATUS(status);
  status = defwSlotRect(2000, 2000, 2500, 4000);
  CHECK_STATUS(status);
  status = defwSlotPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwSlotRect(3000, 2000, 3500, 4000);
  CHECK_STATUS(status);
  status = defwSlotLayer("MET2");
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 2000, 1500, 4000);
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 4500, 1500, 6500);
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 7000, 1500, 9000);
  CHECK_STATUS(status);
  status = defwSlotRect(1000, 9500, 1500, 11500);
  CHECK_STATUS(status);
  status = defwSlotPolygon(7, xP, yP);
  CHECK_STATUS(status);
  status = defwSlotPolygon(6, xP, yP);
  CHECK_STATUS(status);
  status = defwEndSlots();
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);

  // NONDEFAULTRULES
  status = defwStartNonDefaultRules(4);
  CHECK_STATUS(status);
  status = defwNonDefaultRule("doubleSpaceRule", 1);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal1", 2, 0, 1, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal2", 2, 0, 1, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal3", 2, 0, 1, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRule("lowerResistance", 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal1", 6, 0, 0, 5);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal2", 5, 1, 6, 4);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal3", 5, 0, 0, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleMinCuts("cut12", 2);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleMinCuts("cut23", 2);
  CHECK_STATUS(status);
  status = defwNonDefaultRule("myRule", 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal1", 2, 0, 0, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal2", 2, 0, 0, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal3", 2, 0, 0, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleViaRule("myvia12rule");
  CHECK_STATUS(status);
  status = defwNonDefaultRuleViaRule("myvia23rule");
  CHECK_STATUS(status);
  status = defwRealProperty("minlength", 50.5);
  CHECK_STATUS(status);
  status = defwStringProperty("firstName", "Only");
  CHECK_STATUS(status);
  status = defwIntProperty("idx", 1);
  CHECK_STATUS(status);
  status = defwNonDefaultRule("myCustomRule", 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal1", 5, 0, 1, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal2", 5, 0, 1, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleLayer("metal3", 5, 0, 1, 0);
  CHECK_STATUS(status);
  status = defwNonDefaultRuleVia("myvia12_custom1");
  CHECK_STATUS(status);
  status = defwNonDefaultRuleVia("myvia12_custom2");
  CHECK_STATUS(status);
  status = defwNonDefaultRuleVia("myvia23_custom1");
  CHECK_STATUS(status);
  status = defwNonDefaultRuleVia("myvia23_custom2");
  CHECK_STATUS(status);
  status = defwEndNonDefaultRules();
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);

  // STYLES
  status = defwStartStyles(3);
  CHECK_STATUS(status);
  xP = (double*) malloc(sizeof(double) * 6);
  yP = (double*) malloc(sizeof(double) * 6);
  xP[0] = 30;
  yP[0] = 10;
  xP[1] = 10;
  yP[1] = 30;
  xP[2] = -10;
  yP[2] = 30;
  xP[3] = -30;
  yP[3] = 10;
  xP[4] = -30;
  yP[4] = -10;
  xP[5] = -10;
  yP[5] = -30;
  status = defwStyles(1, 6, xP, yP);
  CHECK_STATUS(status);
  status = defwStyles(2, 5, xP, yP);
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);
  xP = (double*) malloc(sizeof(double) * 8);
  yP = (double*) malloc(sizeof(double) * 8);
  xP[0] = 30;
  yP[0] = 10;
  xP[1] = 10;
  yP[1] = 30;
  xP[2] = -10;
  yP[2] = 30;
  xP[3] = -30;
  yP[3] = 10;
  xP[4] = -30;
  yP[4] = -10;
  xP[5] = -10;
  yP[5] = -30;
  xP[6] = 10;
  yP[6] = -30;
  xP[7] = 30;
  yP[7] = -10;
  status = defwStyles(3, 8, xP, yP);
  CHECK_STATUS(status);
  status = defwEndStyles();
  CHECK_STATUS(status);
  free((char*) xP);
  free((char*) yP);
  status = defwNewLine();
  CHECK_STATUS(status);

  // BEGINEXT
  status = defwStartBeginext("tag");
  CHECK_STATUS(status);
  defwAddIndent();
  status = defwBeginextCreator("CADENCE");
  CHECK_STATUS(status);
  // since the date is different each time,
  // it will cause the quick test to fail
  // status = defwBeginextDate();
  // CHECK_STATUS(status);
  status = defwBeginextRevision(5, 7);
  CHECK_STATUS(status);
  status = defwBeginextSyntax("OTTER", "furry");
  CHECK_STATUS(status);
  status = defwStringProperty("arrg", "later");
  CHECK_STATUS(status);
  status = defwBeginextSyntax("SEAL", "cousin to WALRUS");
  CHECK_STATUS(status);
  status = defwEndBeginext();
  CHECK_STATUS(status);

  status = defwEnd();
  CHECK_STATUS(status);

  lineNumber = defwCurrentLineNumber();
  if (lineNumber == 0) {
    fprintf(stderr, "ERROR: nothing has been read.\n");
  }

  fclose(fout);

  return 0;
}
