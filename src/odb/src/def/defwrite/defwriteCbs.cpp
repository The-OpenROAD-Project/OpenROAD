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

#include <string.h>  // NOLINT(modernize-deprecated-headers): for strdup()

#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef WIN32
#include <unistd.h>
#endif /* not WIN32 */
#include "defiDefs.hpp"
#include "defwWriter.hpp"
#include "defwWriterCalls.hpp"

// Global variables
char defaultOut[128];
FILE* fout;
int userData;

#define CHECK_STATUS(status) \
  if (status) {              \
    defwPrintError(status);  \
    return (status);         \
  }

void dataError()
{
  fprintf(fout, "ERROR: returned user data is not correct!\n");
}

void checkType(defwCallbackType_e c)
{
  if (c >= 0 && c <= defwDesignEndCbkType) {
    // OK
  } else {
    fprintf(fout, "ERROR: callback type is out of bounds!\n");
  }
}

int versionCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwVersion(5, 6);
  CHECK_STATUS(status);
  return 0;
}

int dividerCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwDividerChar("/");
  CHECK_STATUS(status);
  return 0;
}

int busbitCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwBusBitChars("[]");
  CHECK_STATUS(status);
  return 0;
}

int designCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwDesignName("muk");
  CHECK_STATUS(status);
  return 0;
}

int technologyCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwTechnology("muk");
  CHECK_STATUS(status);
  return 0;
}

int arrayCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwArray("core_array");
  CHECK_STATUS(status);
  return 0;
}

int floorplanCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwFloorplan("DEFAULT");
  CHECK_STATUS(status);
  return 0;
}

int unitsCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwUnits(100);
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// history
int historyCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwHistory(
      "Corrected STEP for ROW_9 and added ROW_10 of SITE CORE1 (def)");
  CHECK_STATUS(status);
  status = defwHistory("Removed NONDEFAULTRULE from the net XX100 (def)");
  CHECK_STATUS(status);
  status = defwHistory("Changed some cell orientations (def)");
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// PROPERTYDEFINITIONS
int propdefCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartPropDef();
  CHECK_STATUS(status);
  defwAddComment("defwPropDef is broken into 3 routines, defwStringPropDef");
  defwAddComment("defwIntPropDef, and defwRealPropDef");
  status = defwStringPropDef("REGION", "scum", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("REGION", "center", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("REGION", "area", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("GROUP", "ggrp", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("GROUP", "site", 0, 25, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("GROUP", "maxarea", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("COMPONENT", "cc", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("COMPONENT", "index", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("COMPONENT", "size", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("NET", "alt", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("NET", "lastName", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("NET", "length", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("SPECIALNET", "contype", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("SPECIALNET", "ind", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("SPECIALNET", "maxlength", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("DESIGN", "title", 0, 0, "Buffer");
  CHECK_STATUS(status);
  status = defwIntPropDef("DESIGN", "priority", 0, 0, 14);
  CHECK_STATUS(status);
  status = defwRealPropDef("DESIGN", "howbig", 0, 0, 15.16);
  CHECK_STATUS(status);
  status = defwRealPropDef("ROW", "minlength", 1.0, 100.0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("ROW", "firstName", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("ROW", "idx", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwIntPropDef("COMPONENTPIN", "dpIgnoreTerm", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("COMPONENTPIN", "dpBit", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("COMPONENTPIN", "realProperty", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("NET", "IGNOREOPTIMIZATION", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwStringPropDef("SPECIALNET", "IGNOREOPTIMIZATION", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("NET", "FREQUENCY", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwRealPropDef("SPECIALNET", "FREQUENCY", 0, 0, nullptr);
  CHECK_STATUS(status);
  status = defwEndPropDef();
  CHECK_STATUS(status);
  return 0;
}

// DIEAREA
int dieareaCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwDieArea(-190000, -120000, 190000, 70000);
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// ROW
int rowCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwRow("ROW_9", "CORE", -177320, -111250, 5, 911, 1, 360, 0);
  CHECK_STATUS(status);
  status = defwRealProperty("minlength", 50.5);
  CHECK_STATUS(status);
  status = defwStringProperty("firstName", "Only");
  CHECK_STATUS(status);
  status = defwIntProperty("idx", 1);
  CHECK_STATUS(status);
  status = defwRow("ROW_10", "CORE1", -19000, -11000, 6, 1, 100, 0, 600);
  CHECK_STATUS(status);
  return 0;
}

// TRACKS
int trackCB(defwCallbackType_e c, defiUserData ud)
{
  int status;
  const char** layers;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  layers = (const char**) malloc(sizeof(char*) * 1);
  layers[0] = strdup("M1");
  status = defwTracks("X", 3000, 40, 120, 1, layers);
  CHECK_STATUS(status);
  free((char*) layers[0]);
  layers[0] = strdup("M2");
  status = defwTracks("Y", 5000, 10, 20, 1, layers);
  CHECK_STATUS(status);
  free((char*) layers[0]);
  free((char*) layers);
  status = defwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// GCELLGRID
int gcellgridCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwGcellGrid("X", 0, 100, 600);
  CHECK_STATUS(status);
  status = defwGcellGrid("Y", 10, 120, 400);
  CHECK_STATUS(status);
  status = defwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// VIAS
int viaCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartVias(2);
  CHECK_STATUS(status);
  status = defwViaName("VIA_ARRAY");
  CHECK_STATUS(status);
  status = defwViaRect("M1", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwViaRect("V1", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwViaRect("M2", -50, -50, 50, 50);
  CHECK_STATUS(status);
  status = defwOneViaEnd();
  CHECK_STATUS(status);
  status = defwViaName("VIA_ARRAY1");
  CHECK_STATUS(status);
  status = defwViaRect("M1", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwViaRect("V1", -40, -40, 40, 40);
  CHECK_STATUS(status);
  status = defwViaRect("M2", -50, -50, 50, 50);
  CHECK_STATUS(status);
  status = defwOneViaEnd();

  status = defwEndVias();
  CHECK_STATUS(status);
  return 0;
}

// REGIONS
int regionCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartRegions(2);
  CHECK_STATUS(status);
  status = defwRegionName("region1");
  CHECK_STATUS(status);
  status = defwRegionPoints(-500, -500, 300, 100);
  CHECK_STATUS(status);
  status = defwRegionPoints(500, 500, 1000, 1000);
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
  return 0;
}

// COMPONENTS
int componentCB(defwCallbackType_e c, defiUserData ud)
{
  int status;
  const char** foreigns;
  int *foreignX, *foreignY, *foreignOrient;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  foreigns = (const char**) malloc(sizeof(char*) * 1);
  foreignX = (int*) malloc(sizeof(int) * 1);
  foreignY = (int*) malloc(sizeof(int) * 1);
  foreignOrient = (int*) malloc(sizeof(int) * 1);
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
  status = defwComponent("Z38A03",
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
                         6,
                         0,
                         nullptr,
                         0,
                         0,
                         0,
                         0);
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
                         100,
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
  status = defwComponent("cell4",
                         "CHM3A",
                         0,
                         nullptr,
                         "CHM6A",
                         nullptr,
                         nullptr,
                         "DIST",
                         0,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr,
                         "PLACED",
                         360,
                         10,
                         1,
                         0,
                         "region2",
                         0,
                         0,
                         0,
                         0);
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
  free((char*) foreigns);
  free((char*) foreignX);
  free((char*) foreignY);
  free((char*) foreignOrient);
  return 0;
}

// PINS
int pinCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartPins(6);
  CHECK_STATUS(status);
  status = defwPin("scanpin",
                   "SCAN",
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
  status = defwEndPins();
  CHECK_STATUS(status);
  return 0;
}

// PINPROPERTIES
int pinpropCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
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
  return 0;
}

// SPECIALNETS
int snetCB(defwCallbackType_e c, defiUserData ud)
{
  int status;
  const char **coorX, **coorY;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartSpecialNets(2);
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
  status = defwSpecialNetConnection("cell1", "GND", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell2", "GND", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell3", "GND", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetConnection("cell4", "GND", 0);
  CHECK_STATUS(status);
  status = defwSpecialNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwSpecialNetPathLayer("M1");
  CHECK_STATUS(status);
  status = defwSpecialNetPathWidth(250);
  CHECK_STATUS(status);
  status = defwSpecialNetPathShape("IOWIRE");
  CHECK_STATUS(status);
  coorX = (const char**) malloc(sizeof(char*) * 3);
  coorY = (const char**) malloc(sizeof(char*) * 3);
  coorX[0] = strdup("5");
  coorY[0] = strdup("15");
  coorX[1] = strdup("125");
  coorY[1] = strdup("*");
  coorX[2] = strdup("245");
  coorY[2] = strdup("*");
  status = defwSpecialNetPathPoint(3, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetPathEnd();
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldStart("my_net");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldLayer("M2");
  CHECK_STATUS(status);
  status = defwSpecialNetShieldWidth(90);
  CHECK_STATUS(status);
  coorX[0] = strdup("14100");
  coorY[0] = strdup("342440");
  coorX[1] = strdup("13920");
  coorY[1] = strdup("*");
  status = defwSpecialNetShieldPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M2_TURN");
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  coorX[0] = strdup("*");
  coorY[0] = strdup("263200");
  status = defwSpecialNetShieldPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M1_M2");
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  coorX[0] = strdup("2400");
  coorY[0] = strdup("*");
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
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  coorX[0] = strdup("14100");
  coorY[0] = strdup("342440");
  coorX[1] = strdup("13920");
  coorY[1] = strdup("*");
  status = defwSpecialNetShieldPoint(2, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M2_TURN");
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  coorX[0] = strdup("*");
  coorY[0] = strdup("263200");
  status = defwSpecialNetShieldPoint(1, coorX, coorY);
  CHECK_STATUS(status);
  status = defwSpecialNetShieldVia("M1_M2");
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  coorX[0] = strdup("2400");
  coorY[0] = strdup("*");
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
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorX[2]);
  free((char*) coorY[2]);
  free((char*) coorX);
  free((char*) coorY);
  status = defwEndSpecialNets();
  CHECK_STATUS(status);
  return 0;
}

// NETS
int netCB(defwCallbackType_e c, defiUserData ud)
{
  int status;
  const char **coorX, **coorY;
  const char** coorValue;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartNets(11);
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
  status = defwIntProperty("alt", 37);
  CHECK_STATUS(status);
  status = defwStringProperty("lastName", "Unknown");
  CHECK_STATUS(status);
  status = defwRealProperty("length", 10.11);
  CHECK_STATUS(status);
  status = defwNetPattern("BALANCED");
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);

  coorX = (const char**) malloc(sizeof(char*) * 5);
  coorY = (const char**) malloc(sizeof(char*) * 5);
  coorValue = (const char**) malloc(sizeof(char*) * 5);
  status = defwNet("my_net");
  CHECK_STATUS(status);
  status = defwNetConnection("I1", "A", 0);
  CHECK_STATUS(status);
  status = defwNetConnection("BUF", "Z", 0);
  CHECK_STATUS(status);
  status = defwNetNondefaultRule("RULE1");
  CHECK_STATUS(status);
  status = defwNetShieldnet("VSS");
  CHECK_STATUS(status);
  status = defwNetShieldnet("VDD");
  CHECK_STATUS(status);
  status = defwNetPathStart("ROUTED");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M2", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("14000");
  coorY[0] = strdup("341440");
  coorValue[0] = nullptr;
  coorX[1] = strdup("9600");
  coorY[1] = strdup("*");
  coorValue[1] = nullptr;
  coorX[2] = strdup("*");
  coorY[2] = strdup("282400");
  coorValue[2] = nullptr;
  status = defwNetPathPoint(3, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("nd1VIA12");
  CHECK_STATUS(status);
  coorX[0] = strdup("2400");
  coorY[0] = strdup("*");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 1, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("2400");
  coorY[0] = strdup("282400");
  coorValue[0] = nullptr;
  coorX[1] = strdup("240");
  coorY[1] = strdup("*");
  coorValue[1] = nullptr;
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetNoshieldStart("M2");
  CHECK_STATUS(status);
  coorX[0] = strdup("14100");
  coorY[0] = strdup("341440");
  coorX[1] = strdup("14000");
  coorY[1] = strdup("*");
  status = defwNetNoshieldPoint(2, coorX, coorY);
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
  coorX[0] = strdup("54040");
  coorY[0] = strdup("30300");
  coorValue[0] = strdup("0");
  coorX[1] = strdup("*");
  coorY[1] = strdup("30900");
  coorValue[1] = nullptr;
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  status = defwNetPathVia("nd1VIA12");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("0");
  coorX[1] = strdup("56280");
  coorY[1] = strdup("*");
  coorValue[1] = nullptr;
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("31500");
  coorValue[0] = nullptr;
  coorX[1] = strdup("55160");
  coorY[1] = strdup("*");
  coorValue[1] = nullptr;
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
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
  coorX[0] = strdup("168280");
  coorY[0] = strdup("63300");
  coorValue[0] = strdup("7");
  coorX[1] = strdup("*");
  coorY[1] = strdup("64500");
  coorValue[1] = nullptr;
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  status = defwNetPathVia("M1_M2");
  CHECK_STATUS(status);
  coorX[0] = strdup("169400");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("8");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
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
  coorX[0] = strdup("188400");
  coorY[0] = strdup("26100");
  coorValue[0] = strdup("0");
  coorX[1] = strdup("*");
  coorY[1] = strdup("27300");
  coorValue[1] = strdup("0");
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("M1_M2");
  CHECK_STATUS(status);
  coorX[0] = strdup("189560");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("0");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
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
  coorX[0] = strdup("269400");
  coorY[0] = strdup("64500");
  coorValue[0] = strdup("0");
  coorX[1] = strdup("*");
  coorY[1] = strdup("54900");
  coorValue[1] = nullptr;
  coorX[2] = strdup("170520");
  coorY[2] = strdup("*");
  coorValue[2] = nullptr;
  coorX[3] = strdup("*");
  coorY[3] = strdup("37500");
  coorValue[3] = nullptr;
  coorX[4] = strdup("*");
  coorY[4] = strdup("30300");
  coorValue[4] = nullptr;
  status = defwNetPathPoint(5, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorX[2]);
  free((char*) coorY[2]);
  free((char*) coorX[3]);
  free((char*) coorY[3]);
  free((char*) coorX[4]);
  free((char*) coorY[4]);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = strdup("171080");
  coorY[0] = strdup("*");
  coorValue[0] = nullptr;
  coorX[1] = strdup("17440");
  coorY[1] = strdup("0");
  coorValue[1] = strdup("0");
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("*");
  coorValue[0] = nullptr;
  coorX[1] = strdup("*");
  coorY[1] = strdup("26700");
  coorValue[1] = strdup("8");
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = strdup("177800");
  coorY[0] = strdup("*");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("8");
  coorX[1] = strdup("*");
  coorY[1] = strdup("30300");
  coorValue[1] = strdup("8");
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("nd1VIA23");
  CHECK_STATUS(status);
  coorX[0] = strdup("189560");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("8");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("nd1VIA12");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("27300");
  coorValue[0] = strdup("0");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M3", 1, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("55160");
  coorY[0] = strdup("31500");
  coorValue[0] = strdup("8");
  coorX[1] = strdup("*");
  coorY[1] = strdup("34500");
  coorValue[1] = strdup("0");
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorValue[1]);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  coorX[0] = strdup("149800");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("8");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("35700");
  coorValue[0] = nullptr;
  coorX[1] = strdup("*");
  coorY[1] = strdup("37500");
  coorValue[1] = nullptr;
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  status = defwNetPathVia("M2_M3");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("*");
  coorValue[0] = strdup("8");
  ;
  coorX[1] = strdup("170520");
  coorY[1] = strdup("*");
  coorValue[1] = strdup("0");
  status = defwNetPathPoint(2, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  free((char*) coorValue[0]);
  free((char*) coorX[1]);
  free((char*) coorY[1]);
  free((char*) coorValue[1]);
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
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("1288210");
  coorY[0] = strdup("580930");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH1W1W1");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("582820");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH2W1W1");
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M3", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("1141350");
  coorY[0] = strdup("582820");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH2W1W1");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("580930");
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH1W1W1");
  CHECK_STATUS(status);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("1278410");
  coorY[0] = strdup("275170");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathStart("NEW");
  CHECK_STATUS(status);
  status = defwNetPathLayer("M1", 0, nullptr);
  CHECK_STATUS(status);
  coorX[0] = strdup("1141210");
  coorY[0] = strdup("271250");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH1W1W1");
  CHECK_STATUS(status);
  coorX[0] = strdup("*");
  coorY[0] = strdup("271460");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH2W1W1");
  CHECK_STATUS(status);
  coorX[0] = strdup("1142820");
  coorY[0] = strdup("*");
  coorValue[0] = nullptr;
  status = defwNetPathPoint(1, coorX, coorY, coorValue);
  CHECK_STATUS(status);
  free((char*) coorX[0]);
  free((char*) coorY[0]);
  status = defwNetPathVia("GETH3W1W1");
  CHECK_STATUS(status);
  status = defwNetPathEnd();
  CHECK_STATUS(status);
  status = defwNetEndOneNet();
  CHECK_STATUS(status);
  free((char*) coorX);
  free((char*) coorY);
  free((char*) coorValue);
  status = defwEndNets();
  CHECK_STATUS(status);
  return 0;
}

// GROUPS
int groupCB(defwCallbackType_e c, defiUserData ud)
{
  int status;
  const char** groupExpr;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
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
  status = defwGroupSoft(
      "MAXHALFPERIMETER", 4000, "MAXX", 10000, nullptr, nullptr);
  CHECK_STATUS(status);
  status = defwEndGroups();
  CHECK_STATUS(status);
  free((char*) groupExpr);
  status = defwNewLine();
  CHECK_STATUS(status);
  return 0;
}

// BEGINEXT
int extensionCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwStartBeginext("tag");
  CHECK_STATUS(status);
  status = defwBeginextCreator("CADENCE");
  CHECK_STATUS(status);
  status = defwBeginextDate();
  CHECK_STATUS(status);
  status = defwBeginextSyntax("OTTER", "furry");
  CHECK_STATUS(status);
  status = defwStringProperty("arrg", "later");
  CHECK_STATUS(status);
  status = defwBeginextSyntax("SEAL", "cousin to WALRUS");
  CHECK_STATUS(status);
  status = defwEndBeginext();
  CHECK_STATUS(status);
  return 0;
}

static int designendCB(defwCallbackType_e c, defiUserData ud)
{
  int status;

  checkType(c);
  if ((int) ud != userData) {
    dataError();
  }
  status = defwEnd();
  CHECK_STATUS(status);
  return 0;
}

main(int argc, char** argv)
{
  char* outfile;
  int status;  // return code, if none 0 means error
  int res;

  // assign the default
  strcpy(defaultOut, "def.in");
  outfile = defaultOut;
  fout = stdout;
  userData = 0x01020304;

  double* axis;
  double* num1;
  double* num2;
  double* num3;

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
    } else {
      fprintf(stderr, "ERROR: Illegal command line option: '%s'\n", *argv);
      return 2;
    }
    argv++;
  }

  // initalize
  status = defwInitCbk(fout);
  CHECK_STATUS(status);

  // set the callback functions
  defwSetArrayCbk(arrayCB);
  defwSetBusBitCbk(busbitCB);
  defwSetDividerCbk(dividerCB);
  defwSetComponentCbk(componentCB);
  defwSetDesignCbk(designCB);
  defwSetDesignEndCbk((defwVoidCbkFnType) designendCB);
  defwSetDieAreaCbk(dieareaCB);
  defwSetExtCbk(extensionCB);
  defwSetFloorPlanCbk(floorplanCB);
  defwSetGcellGridCbk(gcellgridCB);
  defwSetGroupCbk(groupCB);
  defwSetHistoryCbk(historyCB);
  defwSetNetCbk(netCB);
  defwSetPinCbk(pinCB);
  defwSetPinPropCbk(pinpropCB);
  defwSetPropDefCbk(propdefCB);
  defwSetRegionCbk(regionCB);
  defwSetRowCbk(rowCB);
  defwSetSNetCbk(snetCB);
  defwSetTechnologyCbk(technologyCB);
  defwSetTrackCbk(trackCB);
  defwSetUnitsCbk(unitsCB);
  defwSetViaCbk(viaCB);

  res = defwWrite(fout, outfile, userData);

  fclose(fout);
  return 0;
}
