// *****************************************************************************
// *****************************************************************************
// Copyright 2013, Cadence Design Systems
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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************


// This file contains code for implementing the defwriter 5.3
// It has all the functions user can call in their callbacks or
// just their writer to write out the correct lef syntax.
//
// Author: Wanda da Rosa
// Date:   Summer, 1998
//
// Revisions: 11/25/2002 - bug fix: submitted by Craig Files
//                         (cfiles@ftc.agilent.com)
//                         Changed all (!name && !*name) to
//                         (!name || !*name)

#include "defwWriter.hpp"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "defiUtil.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

// States of the writer.
#define DEFW_UNINIT            0
#define DEFW_INIT              1
#define DEFW_VERSION           2
#define DEFW_CASESENSITIVE     3
#define DEFW_DIVIDER           4
#define DEFW_BUSBIT            5
#define DEFW_DESIGN            6
#define DEFW_TECHNOLOGY        7
#define DEFW_ARRAY             8
#define DEFW_FLOORPLAN         9
#define DEFW_UNITS            10
#define DEFW_HISTORY          11
#define DEFW_PROP_START       12
#define DEFW_PROP             13
#define DEFW_PROP_END         14
#define DEFW_DIE_AREA         15
#define DEFW_ROW              16
#define DEFW_TRACKS           17
#define DEFW_GCELL_GRID       18
#define DEFW_DEFAULTCAP_START 19
#define DEFW_DEFAULTCAP       20
#define DEFW_DEFAULTCAP_END   21
#define DEFW_CANPLACE         22
#define DEFW_CANNOTOCCUPY     23
#define DEFW_VIA_START        24
#define DEFW_VIA              25
#define DEFW_VIAVIARULE       26
#define DEFW_VIAONE_END       27
#define DEFW_VIA_END          28
#define DEFW_REGION_START     29
#define DEFW_REGION           30
#define DEFW_REGION_END       31
#define DEFW_COMPONENT_MASKSHIFTLAYERS 32
#define DEFW_COMPONENT_START  33
#define DEFW_COMPONENT        34
#define DEFW_COMPONENT_END    35
#define DEFW_PIN_START        36
#define DEFW_PIN              37
#define DEFW_PIN_PORT         38
#define DEFW_PIN_END          39
#define DEFW_PINPROP_START    40
#define DEFW_PINPROP          41
#define DEFW_PINPROP_END      42
#define DEFW_BLOCKAGE_START   43
#define DEFW_BLOCKAGE_LAYER   44
#define DEFW_BLOCKAGE_PLACE   45
#define DEFW_BLOCKAGE_RECT    46
#define DEFW_BLOCKAGE_POLYGON 47
#define DEFW_BLOCKAGE_END     48
#define DEFW_SNET_START       49
#define DEFW_SNET             50
#define DEFW_SNET_OPTIONS     51
#define DEFW_SNET_ENDNET      52
#define DEFW_SNET_END         53
#define DEFW_PATH_START       54
#define DEFW_PATH             55
#define DEFW_SHIELD           56
#define DEFW_NET_START        57
#define DEFW_NET              58
#define DEFW_NET_OPTIONS      59
#define DEFW_NET_ENDNET       60
#define DEFW_NET_END          61
#define DEFW_SUBNET           62
#define DEFW_NOSHIELD         63
#define DEFW_IOTIMING_START   64
#define DEFW_IOTIMING         65
#define DEFW_IOTIMING_END     66
#define DEFW_SCANCHAIN_START  67
#define DEFW_SCANCHAIN        68
#define DEFW_SCAN_FLOATING    69
#define DEFW_SCAN_ORDERED     70
#define DEFW_SCANCHAIN_END    71
#define DEFW_FPC_START        72
#define DEFW_FPC              73
#define DEFW_FPC_OPER         74
#define DEFW_FPC_OPER_SUM     75
#define DEFW_FPC_END          76
#define DEFW_GROUP_START      77
#define DEFW_GROUP            78
#define DEFW_GROUP_END        79
#define DEFW_SLOT_START       80
#define DEFW_SLOT_LAYER       81
#define DEFW_SLOT_RECT        82
#define DEFW_SLOT_END         83
#define DEFW_FILL_START       84
#define DEFW_FILL_LAYER       85
#define DEFW_FILL_VIA         86
#define DEFW_FILL_OPC         87
#define DEFW_FILL_RECT        88
#define DEFW_FILL_END         89
#define DEFW_NDR_START        90
#define DEFW_NDR              91
#define DEFW_NDR_END          92
#define DEFW_STYLES_START     93
#define DEFW_STYLES           94
#define DEFW_STYLES_END       95
#define DEFW_BEGINEXT_START   96
#define DEFW_BEGINEXT         97
#define DEFW_BEGINEXT_END     98
#define DEFW_FILL_LAYERMASK   99
#define DEFW_FILL_VIAMASK    100
#define DEFW_BLOCKAGE_MASK   101

#define DEFW_END             102

#define DEFW_DONE            999

#define MAXSYN               103


// *****************************************************************************
//        Global Variables
// *****************************************************************************
FILE *defwFile = 0;   // File to write to.
int defwLines = 0;    // number of lines written
int defwState = DEFW_UNINIT;  // Current state of writer
int defwFunc = DEFW_UNINIT;   // Current function of writer
int defwDidNets = 0;  // required section
int defwDidComponents = 0;  // required section
int defwDidInit = 0;  // required section
int defwCounter = 0;  // number of nets, components in section
int defwLineItemCounter = 0; // number of items on current line
int defwFPC = 0;  // Current number of items in constraints/operand/sum
int defwHasInit = 0;    // for defwInit has called
int defwHasInitCbk = 0; // for defwInitCbk has called
int defwSpNetShield = 0; // for special net shieldNetName
static double defVersionNum = 5.7;  // default to 5.7
static int defwObsoleteNum = -1; // keep track the obsolete syntax for error
static int defwViaHasVal = 0;    // keep track only ViaRule|Pattern
static int defwBlockageHasSD = 0;// keep track only Spacing|Designrulewidth
static int defwBlockageHasSF = 0;// keep track only SLOTS|FILLS
static int defwBlockageHasSP = 0;// keep track only SOFT|PARTIAL

char defwStateStr[MAXSYN] [80] = {
    "UNINITIALIZE",         //  0
    "INITIALIZE",           //  1
    "VERSION",              //  2
    "CASESENSITIVE",        //  3
    "BUSBIT",               //  4
    "DIVIDER",              //  5
    "DESIGN",               //  6
    "TECHNOLOGY",           //  7
    "ARRAY",                //  8
    "FLOORPLAN",            //  9
    "UNITS",                // 10
    "HISTORY",              // 11
    "PROPERTYDEFINITIONS",  // 12
    "PROPERTYDEFINITIONS",  // 13
    "PROPERTYDEFINITIONS",  // 14
    "DIEAREA",              // 15
    "ROW",                  // 16
    "TRACKS",               // 17
    "GCELLGRID",            // 18
    "DEFAULTCAP",           // 19
    "DEFAULTCAP",           // 20
    "DEFAULTCAP",           // 21
    "CANPLACE",             // 22
    "CANNOTOCCUPY",         // 23
    "VIA",                  // 24
    "VIA",                  // 25
    "VIA",                  // 26
    "VIA",                  // 27
    "VIA",                  // 28
    "REGION",               // 29
    "REGION",               // 30
    "REGION",               // 31
    "COMPONENT",            // 32
    "COMPONENT",            // 33
    "COMPONENT",            // 34
    "COMPONENT",            // 35
    "PIN",                  // 36
    "PIN",                  // 37
    "PIN",                  // 38
    "PIN",                  // 39
    "PINPROPERTY",          // 40 
    "PINPROPERTY",          // 41
    "PINPROPERTY",          // 42
    "SNET",                 // 43
    "SNET",                 // 44
    "SNET",                 // 45
    "SNET",                 // 46
    "SNET",                 // 47
    "PATH",                 // 48
    "PATH",                 // 49
    "SHIELD",               // 50 
    "NET",                  // 51
    "NET",                  // 52
    "NET",                  // 53
    "NET",                  // 54
    "NET",                  // 55
    "SUBNET",               // 56
    "NOSHIELD",             // 57
    "IOTIMING",             // 58
    "IOTIMING",             // 59
    "IOTIMING",             // 60 
    "SCANCHAIN",            // 61
    "SCANCHAIN",            // 62
    "SCAN FLOATING",        // 63
    "SCAN ORDERED",         // 64
    "SCANCHAIN",            // 65
    "CONSTRAINTS",          // 66
    "CONSTRAINTS",          // 67
    "CONSTRAINTS",          // 68
    "CONSTRAINTS",          // 69
    "CONSTRAINTS",          // 70 
    "GROUP",                // 71
    "GROUP",                // 71
    "GROUP",                // 72
    "BLOCKAGE",             // 73
    "BLOCKAGE LAYER",       // 74
    "BLOCKAGE PLACEMENT",   // 75
    "BLOCKAGE RECT",        // 76
    "BLOCKAGE POLYGON",     // 77
    "BLOCKAGE",             // 78
    "SLOT",                 // 79
    "SLOT",                 // 80
    "SLOT",                 // 81
    "SLOT",                 // 82
    "FILL",                 // 83
    "FILL",                 // 84
    "FILL",                 // 85
    "FILL",                 // 86
    "FILL",                 // 87
    "FILL",                 // 88
    "NDR",                  // 89
    "NDR",                  // 90
    "NDR",                  // 91
    "STYLES",               // 92
    "STYLES",               // 93
    "STYLES",               // 94
    "BEGINEXT",             // 95
    "BEGINEXT",             // 96
    "BEGINEXT",             // 97
    "DESIGN END",           // 98
    "FILL_LAYERMASK",       // 99
    "FILL_VIAMASK",         // 100 
    "BLOCKAGE_MASK"         // 101

};


static int printPointsNum = 0;
static void printPoints(FILE *file, double x, double y, 
                        const char* prefix, const char* suffix)
{
    static double x_old = 0;
    static double y_old = 0;

    fprintf(file, "%s", prefix);

    if (printPointsNum++ == 0) {
        fprintf(file, "( %.11g %.11g )", x, y);
    } else if (x_old == x) {
        if (y_old == y) {
            fprintf(file, "( * * )");
        } else {
            fprintf(file, "( * %.11g )", y);
        }
    } else if (y_old == y) {
        fprintf(file, "( %.11g * )", x);
    } else {
        fprintf(file, "( %.11g %.11g )", x, y);
    }

    fprintf(file, "%s", suffix);

    x_old = x;
    y_old = y;
}



int
defwNewLine()
{
    if (!defwFile)
        return DEFW_BAD_ORDER;
    fprintf(defwFile, "\n");
    return DEFW_OK;
}


// this function is required to be called first to initialize the required
// sections.
// Either this function or defwInitCbk can be called, cannot be both
int
defwInit(FILE       *f,
         int        vers1,
         int        vers2,
         const char *caseSensitive,
         const char *dividerChar,
         const char *busBitChars,
         const char *designName,
         const char *technology,  // optional 
         const char *array,       // optional 
         const char *floorplan,   // optional 
         double     units             // optional  (set to -1 to ignore) 
         )
{

    //if (defwFile) return DEFW_BAD_ORDER;
    defwFile = f;
    if (defwHasInitCbk == 1) {  // defwInitCbk has already called, issue an error
        fprintf(stderr,
                "ERROR (DEFWRIT-9000): The DEF writer has detected that the function defwInitCbk has already been called and you are trying to call defwInit.\nOnly defwInitCbk or defwInit can be called but not both.\nUpdate your program and then try again.\n");
        fprintf(stderr, "Writer Exit.\n");
        exit(DEFW_BAD_ORDER);
    }

    defwState = DEFW_UNINIT;  // Current state of writer
    defwFunc = DEFW_UNINIT;   // Current function of writer
    defwDidNets = 0;  // required section
    defwDidComponents = 0;  // required section
    defwDidInit = 0;  // required section

    if (vers1) {  // optional in 5.6 on
        fprintf(defwFile, "VERSION %d.%d ;\n", vers1, vers2);
        defwLines++;
    }

    if ((vers1 == 5) && (vers2 < 6)) {  // For version before 5.6
        if (caseSensitive == 0 || *caseSensitive == 0)
            return DEFW_BAD_DATA;
        fprintf(defwFile, "NAMESCASESENSITIVE %s ;\n", caseSensitive);
    }

    if (dividerChar) {  // optional in 5.6 on
        fprintf(defwFile, "DIVIDERCHAR \"%s\" ;\n", dividerChar);
        defwLines++;
    }

    if (busBitChars) {  // optional in 5.6 on
        fprintf(defwFile, "BUSBITCHARS \"%s\" ;\n", busBitChars);
        defwLines++;
    }

    if (designName == 0 || *designName == 0)
        return DEFW_BAD_DATA;
    fprintf(defwFile, "DESIGN %s ;\n", designName);
    defwLines++;

    if (technology) {
        fprintf(defwFile, "TECHNOLOGY %s ;\n", technology);
        defwLines++;
    }

    if (array) {
        fprintf(defwFile, "ARRAY %s ;\n", array);
        defwLines++;
    }

    if (floorplan) {
        fprintf(defwFile, "FLOORPLAN %s ;\n", floorplan);
        defwLines++;
    }

    if (units != -1.0) {
        int unitsVal = (int) units;
        switch (unitsVal) {
        case 100:
        case 200:
        case 1000:
        case 2000:
        case 4000:
        case 8000:
        case 10000:
        case 16000:
        case 20000:
            fprintf(defwFile, "UNITS DISTANCE MICRONS %d ;\n", ROUND(units));
            defwLines++;
            break;
        default:
            return DEFW_BAD_DATA;
        }
    }

    defwDidInit = 1;
    defwState = DEFW_DESIGN;
    defwHasInit = 1;
    return DEFW_OK;
}


// this function is required to be called first to initialize the variables
// Either this function or defwInit can be called, cannot be both
int
defwInitCbk(FILE *f)
{

    defwFile = f;
    if (defwHasInit == 1) {  // defwInit has already called, issue an error
        fprintf(stderr,
                "ERROR (DEFWRIT-9001): The DEF writer has detected that the function defwInit has already been called and you are trying to call defwInitCbk.\nOnly defwInitCbk or defwInit can be called but not both.\nUpdate your program and then try again.\n");
        fprintf(stderr, "Writer Exit.\n");
        exit(DEFW_BAD_ORDER);
    }

    defwState = DEFW_UNINIT;  // Current state of writer
    defwFunc = DEFW_UNINIT;   // Current function of writer
    defwDidNets = 0;  // required section
    defwDidComponents = 0;  // required section
    defwDidInit = 0;  // required section

    defwDidInit = 1;
    defwState = DEFW_INIT;
    defwHasInitCbk = 1;
    return DEFW_OK;
}

int
defwVersion(int vers1,
            int vers2)
{
    defwFunc = DEFW_VERSION;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState != DEFW_INIT)  // version follows init 
        return DEFW_BAD_ORDER;
    fprintf(defwFile, "VERSION %d.%d ;\n", vers1, vers2);
    if (vers2 >= 10)
        defVersionNum = vers1 + (vers2 / 100.0);
    else
        defVersionNum = vers1 + (vers2 / 10.0);
    defwLines++;

    defwState = DEFW_VERSION;
    return DEFW_OK;
}

int
defwCaseSensitive(const char *caseSensitive)
{
    defwObsoleteNum = DEFW_CASESENSITIVE;
    defwFunc = DEFW_CASESENSITIVE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defVersionNum >= 5.6)
        return DEFW_OBSOLETE;
    // Check for repeated casesensitive
    if (defwState == DEFW_CASESENSITIVE)
        return DEFW_BAD_ORDER;
    if (strcmp(caseSensitive, "ON") && strcmp(caseSensitive, "OFF"))
        return DEFW_BAD_DATA;     // has to be either ON or OFF
    fprintf(defwFile, "NAMESCASESENSITIVE %s ;\n", caseSensitive);
    defwLines++;

    defwState = DEFW_CASESENSITIVE;
    return DEFW_OK;
}

int
defwBusBitChars(const char *busBitChars)
{
    defwFunc = DEFW_BUSBIT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    // Check for repeated casesensitive
    if (defwState == DEFW_BUSBIT)
        return DEFW_BAD_ORDER;
    if (busBitChars && busBitChars != 0 && *busBitChars != 0) {
        fprintf(defwFile, "BUSBITCHARS \"%s\" ;\n", busBitChars);
        defwLines++;
    }

    defwState = DEFW_BUSBIT;
    return DEFW_OK;
}

int
defwDividerChar(const char *dividerChar)
{
    defwFunc = DEFW_DIVIDER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    // Check for repeated busbit
    if (defwState == DEFW_DIVIDER)
        return DEFW_BAD_ORDER;
    if (dividerChar && dividerChar != 0 && *dividerChar != 0) {
        fprintf(defwFile, "DIVIDERCHAR \"%s\" ;\n", dividerChar);
        defwLines++;
    }

    defwState = DEFW_DIVIDER;
    return DEFW_OK;
}

int
defwDesignName(const char *name)
{
    defwFunc = DEFW_DESIGN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    // Check for repeated design
    if (defwState == DEFW_DESIGN)
        return DEFW_BAD_ORDER;
    if (name && name != 0 && *name != 0) {
        fprintf(defwFile, "DESIGN %s ;\n", name);
        defwLines++;
    }

    defwState = DEFW_DESIGN;
    return DEFW_OK;
}

int
defwTechnology(const char *technology)
{
    defwFunc = DEFW_TECHNOLOGY;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (technology && technology != 0 && *technology != 0) {
        fprintf(defwFile, "TECHNOLOGY %s ;\n", technology);
        defwLines++;
    }

    defwState = DEFW_TECHNOLOGY;
    return DEFW_OK;
}

int
defwArray(const char *array)
{
    defwFunc = DEFW_ARRAY;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState == DEFW_ARRAY)
        return DEFW_BAD_ORDER;     // check for repeated array
    if (array && array != 0 && *array != 0) {
        fprintf(defwFile, "ARRAY %s ;\n", array);
        defwLines++;
    }

    defwState = DEFW_ARRAY;
    return DEFW_OK;
}

int
defwFloorplan(const char *floorplan)
{
    defwFunc = DEFW_FLOORPLAN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState == DEFW_FLOORPLAN)
        return DEFW_BAD_ORDER;     // Check for repeated floorplan
    if (floorplan && floorplan != 0 && *floorplan != 0) {
        fprintf(defwFile, "FLOORPLAN %s ;\n", floorplan);
        defwLines++;
    }

    defwState = DEFW_FLOORPLAN;
    return DEFW_OK;
}

int
defwUnits(int units)
{
    defwFunc = DEFW_UNITS;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState == DEFW_UNITS)
        return DEFW_BAD_ORDER;     // Check for repeated units
    if (units != 0) {
        switch (units) {
        case 100:
        case 200:
        case 1000:
        case 2000:
        case 10000:
        case 20000:
            fprintf(defwFile, "UNITS DISTANCE MICRONS %d ;\n", units);
            defwLines++;
            break;
        default:
            return DEFW_BAD_DATA;
        }
    }

    defwState = DEFW_UNITS;
    return DEFW_OK;
}

int
defwHistory(const char *string)
{
    char *c;
    defwFunc = DEFW_HISTORY;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (string == 0 || *string == 0)
        return DEFW_BAD_DATA;

    for (c = (char*) string; *c; c++)
        if (*c == '\n')
            defwLines++;

    fprintf(defwFile, "HISTORY %s ;\n", string);
    defwLines++;

    defwState = DEFW_HISTORY;
    return DEFW_OK;
}

int
defwStartPropDef()
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_PROP_START) && (defwState <= DEFW_PROP_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "PROPERTYDEFINITIONS\n");
    defwLines++;

    defwState = DEFW_PROP_START;
    return DEFW_OK;
}


int
defwIsPropObjType(const char *objType)
{
    if (strcmp(objType, "DESIGN") && strcmp(objType, "COMPONENT") &&
        strcmp(objType, "NET") && strcmp(objType, "SPECIALNET") &&
        strcmp(objType, "GROUP") && strcmp(objType, "ROW") &&
        strcmp(objType, "COMPONENTPIN") && strcmp(objType, "REGION") &&
        strcmp(objType, "NONDEFAULTRULE"))
        return 0;
    return 1;
}

int
defwIntPropDef(const char   *objType,
               const char   *propName,
               double       leftRange,
               double       rightRange,    // optional 
               int          propValue                        // optional 
               )
{

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PROP_START &&
        defwState != DEFW_PROP)
        return DEFW_BAD_ORDER;
    if ((!objType || !*objType) || (!propName || !*propName)) // require
        return DEFW_BAD_DATA;

    if (!defwIsPropObjType(objType))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "   %s %s INTEGER ", objType, propName);
    if (leftRange || rightRange)
        fprintf(defwFile, "RANGE %.11g %.11g ", leftRange, rightRange);

    if (propValue)
        fprintf(defwFile, "%d ", propValue);

    fprintf(defwFile, ";\n");

    defwLines++;
    defwState = DEFW_PROP;
    return DEFW_OK;
}


int
defwRealPropDef(const char  *objType,
                const char  *propName,
                double      leftRange,
                double      rightRange,    // optional 
                double      propValue                        // optional 
                )
{

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PROP_START &&
        defwState != DEFW_PROP)
        return DEFW_BAD_ORDER;
    if ((!objType || !*objType) || (!propName || !*propName)) // require
        return DEFW_BAD_DATA;

    if (!defwIsPropObjType(objType))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "   %s %s REAL ", objType, propName);
    if (leftRange || rightRange)
        fprintf(defwFile, "RANGE %.11g %.11g ", leftRange, rightRange);

    if (propValue)
        fprintf(defwFile, "%.11g ", propValue);

    fprintf(defwFile, ";\n");

    defwLines++;
    defwState = DEFW_PROP;
    return DEFW_OK;
}


int
defwStringPropDef(const char    *objType,
                  const char    *propName,
                  double        leftRange,
                  double        rightRange,    // optional 
                  const char    *propValue                   // optional 
                  )
{

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PROP_START &&
        defwState != DEFW_PROP)
        return DEFW_BAD_ORDER;
    if ((!objType || !*objType) || (!propName || !*propName))
        return DEFW_BAD_DATA;

    if (!defwIsPropObjType(objType))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "   %s %s STRING ", objType, propName);
    if (leftRange || rightRange)
        fprintf(defwFile, "RANGE %.11g %.11g ", leftRange, rightRange);

    if (propValue)
        fprintf(defwFile, "\"%s\" ", propValue);  // string, set quotes

    fprintf(defwFile, ";\n");

    defwLines++;
    defwState = DEFW_PROP;
    return DEFW_OK;
}


int
defwEndPropDef()
{
    defwFunc = DEFW_PROP_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PROP_START &&
        defwState != DEFW_PROP)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "END PROPERTYDEFINITIONS\n\n");
    defwLines++;

    defwState = DEFW_PROP_END;
    return DEFW_OK;
}

int
defwIsPropState()
{
    if ((defwState != DEFW_ROW) && (defwState != DEFW_REGION) &&
        (defwState != DEFW_COMPONENT) && (defwState != DEFW_PIN) &&
        (defwState != DEFW_SNET) && (defwState != DEFW_NET) &&
        (defwState != DEFW_GROUP) && (defwState != DEFW_PINPROP) &&
        (defwState != DEFW_SNET_OPTIONS) && (defwState != DEFW_NET_OPTIONS) &&
        (defwState != DEFW_NDR) && (defwState != DEFW_BEGINEXT))
        return 0;
    return 1;
}

int
defwStringProperty(const char   *propName,
                   const char   *propValue)
{
    if (!defwIsPropState())
        return DEFW_BAD_ORDER;

    // new line for the defwRow of the previous line
    // do not end with newline, may have more than on properties
    fprintf(defwFile, "\n      + PROPERTY %s \"%s\" ", propName, propValue);
    defwLines++;
    return DEFW_OK;
}


int
defwRealProperty(const char *propName,
                 double     propValue)
{
    if (!defwIsPropState())
        return DEFW_BAD_ORDER;

    // new line for the defwRow of the previous line
    // do not end with newline, may have more than on properties
    fprintf(defwFile, "\n      + PROPERTY %s %.11g ", propName, propValue);
    defwLines++;
    return DEFW_OK;
}


int
defwIntProperty(const char  *propName,
                int         propValue)
{
    if (!defwIsPropState())
        return DEFW_BAD_ORDER;

    // new line for the defwRow of the previous line
    // do not end with newline, may have more than on properties
    fprintf(defwFile, "\n      + PROPERTY %s %d ", propName, propValue);
    defwLines++;
    return DEFW_OK;
}


int
defwDieArea(int xl,
            int yl,
            int xh,
            int yh)
{
    defwFunc = DEFW_DIE_AREA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState == DEFW_DIE_AREA)
        return DEFW_BAD_ORDER;
    if (xl > xh || yl > yh)
        return DEFW_BAD_DATA;

    fprintf(defwFile, "DIEAREA ( %d %d ) ( %d %d ) ;\n", xl, yl, xh, yh);
    defwLines++;

    defwState = DEFW_DIE_AREA;
    return DEFW_OK;
}


int
defwDieAreaList(int num_points,
                int *xl,
                int *yl)
{
    int i;

    defwFunc = DEFW_DIE_AREA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState == DEFW_DIE_AREA)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (num_points < 4)
        return DEFW_BAD_DATA;

    fprintf(defwFile, "DIEAREA ");
    for (i = 0; i < num_points; i++) {
        if (i == 0)
            fprintf(defwFile, "( %d %d ) ", *xl++, *yl++);
        else {
            if ((i % 5) == 0) {
                fprintf(defwFile, "\n        ( %d %d ) ", *xl++, *yl++);
                defwLines++;
            } else
                fprintf(defwFile, "( %d %d ) ", *xl++, *yl++);
        }
    }
    fprintf(defwFile, ";\n");
    defwLines++;

    defwState = DEFW_DIE_AREA;
    return DEFW_OK;
}


char *
defwAddr(const char *x)
{
    return (char*) x;
}


char *
defwOrient(int num)
{
    switch (num) {
    case 0:
        return defwAddr("N");
    case 1:
        return defwAddr("W");
    case 2:
        return defwAddr("S");
    case 3:
        return defwAddr("E");
    case 4:
        return defwAddr("FN");
    case 5:
        return defwAddr("FW");
    case 6:
        return defwAddr("FS");
    case 7:
        return defwAddr("FE");
    };
    return defwAddr("BOGUS ");
}


int
defwRow(const char  *rowName,
        const char  *rowType,
        int         x_orig,
        int         y_orig,
        int         orient,
        int         do_count,
        int         do_increment,
        int         do_x,
        int         do_y)
{
    defwFunc = DEFW_ROW;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n");// add the ; and newline for the previous row.

    // required
    if ((rowName == 0) || (*rowName == 0) || (rowType == 0) || (*rowType == 0))
        return DEFW_BAD_DATA;

    // do not have ; because the row may have properties
    // do not end with newline, if there is no property, ; need to be concat.
    fprintf(defwFile, "ROW %s %s %d %d %s ", rowName, rowType, x_orig, y_orig,
            defwOrient(orient));
    if ((do_count != 0) || (do_increment != 0)) {
        fprintf(defwFile, "DO %d BY %d ", do_count, do_increment);
        if ((do_x != 0) || (do_y != 0))
            fprintf(defwFile, "STEP %d %d ", do_x, do_y);
    }
    defwLines++;

    defwState = DEFW_ROW;
    return DEFW_OK;
}


int
defwRowStr(const char   *rowName,
           const char   *rowType,
           int          x_orig,
           int          y_orig,
           const char   *orient,
           int          do_count,
           int          do_increment,
           int          do_x,
           int          do_y)
{
    defwFunc = DEFW_ROW;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n");// add the ; and newline for the previous row.

    if ((!rowName || !*rowName) || (!rowType || !*rowType)) // required
        return DEFW_BAD_DATA;

    // do not have ; because the row may have properties
    // do not end with newline, if there is no property, ; need to be concat.
    fprintf(defwFile, "ROW %s %s %d %d %s ", rowName, rowType, x_orig, y_orig,
            orient);
    if ((do_count != 0) || (do_increment != 0)) {
        fprintf(defwFile, "DO %d BY %d ", do_count, do_increment);
        if ((do_x != 0) || (do_y != 0))
            fprintf(defwFile, "STEP %d %d ", do_x, do_y);
    }
    defwLines++;

    defwState = DEFW_ROW;
    return DEFW_OK;
}


int
defwTracks(const char   *master,
           int          do_start,
           int          do_cnt,
           int          do_step,
           int          num_layers,
           const char   **layers,
           int          mask,
           int          sameMask)
{
    int i;

    defwFunc = DEFW_TRACKS;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row

    if (!master || !*master) // required
        return DEFW_BAD_DATA;
    if (strcmp(master, "X") && strcmp(master, "Y"))
        return DEFW_BAD_DATA;


    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        if (sameMask) {
            fprintf(defwFile, "TRACKS %s %d DO %d STEP %d MASK %d SAMEMASK",
                    master, do_start, do_cnt, do_step, mask);
        } else {
            fprintf(defwFile, "TRACKS %s %d DO %d STEP %d MASK %d",
                    master, do_start, do_cnt, do_step, mask);
        }
    } else {
        fprintf(defwFile, "TRACKS %s %d DO %d STEP %d",
                master, do_start, do_cnt, do_step);
    }

    if (num_layers > 0) {
        fprintf(defwFile, " LAYER");
        for (i = 0; i < num_layers; i++) {
            fprintf(defwFile, " %s", layers[i]);
        }
    }
    fprintf(defwFile, " ;\n");
    defwLines++;

    defwState = DEFW_TRACKS;
    return DEFW_OK;
}


int
defwGcellGrid(const char    *master,
              int           do_start,
              int           do_cnt,
              int           do_step)
{
    defwFunc = DEFW_GCELL_GRID;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    if (!master || !*master) // required
        return DEFW_BAD_DATA;
    if (strcmp(master, "X") && strcmp(master, "Y"))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "GCELLGRID %s %d DO %d STEP %d ;\n", master, do_start,
            do_cnt, do_step);
    defwLines++;

    defwState = DEFW_GCELL_GRID;
    return DEFW_OK;
}


int
defwStartDefaultCap(int count)
{
    defwObsoleteNum = DEFW_DEFAULTCAP_START;
    defwFunc = DEFW_DEFAULTCAP_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_DEFAULTCAP_START) &&
        (defwState <= DEFW_DEFAULTCAP_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum >= 5.4)
        return DEFW_OBSOLETE;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    fprintf(defwFile, "DEFAULTCAP %d\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_DEFAULTCAP_START;
    return DEFW_OK;
}


int
defwDefaultCap(int      pins,
               double   cap)
{
    defwFunc = DEFW_DEFAULTCAP;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_DEFAULTCAP_START &&
        defwState != DEFW_DEFAULTCAP)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "   MINPINS %d WIRECAP %f ;\n", pins, cap);
    defwLines++;
    defwCounter--;

    defwState = DEFW_DEFAULTCAP;
    return DEFW_OK;
}


int
defwEndDefaultCap()
{
    defwFunc = DEFW_DEFAULTCAP_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_DEFAULTCAP_START &&
        defwState != DEFW_DEFAULTCAP)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END DEFAULTCAP\n\n");
    defwLines++;

    defwState = DEFW_DEFAULTCAP_END;
    return DEFW_OK;
}


int
defwCanPlace(const char *master,
             int        xOrig,
             int        yOrig,
             int        orient,
             int        doCnt,
             int        doInc,
             int        xStep,
             int        yStep)
{
    defwFunc = DEFW_CANPLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    if ((master == 0) || (*master == 0)) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, "CANPLACE %s %d %d %s DO %d BY %d STEP %d %d ;\n",
            master, xOrig, yOrig, defwOrient(orient),
            doCnt, doInc, xStep, yStep);
    defwLines++;

    defwState = DEFW_CANPLACE;
    return DEFW_OK;
}


int
defwCanPlaceStr(const char  *master,
                int         xOrig,
                int         yOrig,
                const char  *orient,
                int         doCnt,
                int         doInc,
                int         xStep,
                int         yStep)
{
    defwFunc = DEFW_CANPLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    if (!master || !*master) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, "CANPLACE %s %d %d %s DO %d BY %d STEP %d %d ;\n",
            master, xOrig, yOrig, orient,
            doCnt, doInc, xStep, yStep);
    defwLines++;

    defwState = DEFW_CANPLACE;
    return DEFW_OK;
}


int
defwCannotOccupy(const char *master,
                 int        xOrig,
                 int        yOrig,
                 int        orient,
                 int        doCnt,
                 int        doInc,
                 int        xStep,
                 int        yStep)
{
    defwFunc = DEFW_CANNOTOCCUPY;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if ((master == 0) || (*master == 0)) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, "CANNOTOCCUPY %s %d %d %s DO %d BY %d STEP %d %d ;\n",
            master, xOrig, yOrig, defwOrient(orient),
            doCnt, doInc, xStep, yStep);
    defwLines++;

    defwState = DEFW_CANNOTOCCUPY;
    return DEFW_OK;
}


int
defwCannotOccupyStr(const char  *master,
                    int         xOrig,
                    int         yOrig,
                    const char  *orient,
                    int         doCnt,
                    int         doInc,
                    int         xStep,
                    int         yStep)
{
    defwFunc = DEFW_CANNOTOCCUPY;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;

    if (!master || !*master) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, "CANNOTOCCUPY %s %d %d %s DO %d BY %d STEP %d %d ;\n",
            master, xOrig, yOrig, orient,
            doCnt, doInc, xStep, yStep);
    defwLines++;

    defwState = DEFW_CANNOTOCCUPY;
    return DEFW_OK;
}


int
defwStartVias(int count)
{
    defwFunc = DEFW_VIA_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_VIA_START) && (defwState <= DEFW_VIA_END))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    fprintf(defwFile, "VIAS %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_VIA_START;
    return DEFW_OK;
}


int
defwViaName(const char *name)
{
    defwFunc = DEFW_VIA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIA_START &&
        defwState != DEFW_VIAONE_END)
        return DEFW_BAD_ORDER;
    defwCounter--;

    if (!name || !*name) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, "   - %s", name);

    defwState = DEFW_VIA;
    defwViaHasVal = 0;
    return DEFW_OK;
}


int
defwViaPattern(const char *pattern)
{
    defwFunc = DEFW_VIA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIA)
        return DEFW_BAD_ORDER;  // after defwViaName

    if (defwViaHasVal)
        return DEFW_ALREADY_DEFINED;  // either PatternName or
    // ViaRule has defined
    if (!pattern || !*pattern) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, " + PATTERNNAME %s", pattern);

    defwState = DEFW_VIA;
    defwViaHasVal = 1;
    return DEFW_OK;
}


int
defwViaRect(const char  *layerNames,
            int         xl,
            int         yl,
            int         xh,
            int         yh,
            int         mask)
{
    defwFunc = DEFW_VIA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIA)
        return DEFW_BAD_ORDER;

    if (!layerNames || !*layerNames) // required
        return DEFW_BAD_DATA;
    if (!mask) {
        fprintf(defwFile, "\n      + RECT %s ( %d %d ) ( %d %d )", layerNames,
                xl, yl, xh, yh);
    } else {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n      + RECT %s + MASK %d ( %d %d ) ( %d %d )",
                layerNames, mask, xl, yl, xh, yh);
    }
    defwLines++;


    defwState = DEFW_VIA;
    return DEFW_OK;
}

int
defwViaPolygon(const char   *layerName,
               int          num_polys,
               double       *xl,
               double       *yl,
               int          mask)
{
    int i;

    defwFunc = DEFW_VIA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIA)
        return DEFW_BAD_ORDER;

    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;

    if (!mask) {
        fprintf(defwFile, "\n      + POLYGON %s ", layerName);
    } else {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n      + POLYGON %s + MASK %d ", layerName, mask);
    }

    printPointsNum = 0;
    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) != 0))
            printPoints(defwFile, *xl++, *yl++, "", " ");
        else {
            printPoints(defwFile, *xl++, *yl++, "\n                ", " ");
            defwLines++;
        }
    }
    defwLines++;
    return DEFW_OK;
}

int
defwViaViarule(const char   *viaRuleName,
               double       xCutSize,
               double       yCutSize,
               const char   *botMetalLayer,
               const char   *cutLayer,
               const char   *topMetalLayer,
               double       xCutSpacing,
               double       yCutSpacing,
               double       xBotEnc,
               double       yBotEnc,
               double       xTopEnc,
               double       yTopEnc)
{
    defwFunc = DEFW_VIA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIA)
        return DEFW_BAD_ORDER;
    if (defwViaHasVal)
        return DEFW_ALREADY_DEFINED;  // either PatternName or
    // ViaRule has defined
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, " + VIARULE %s\n", viaRuleName);
    fprintf(defwFile, "      + CUTSIZE %.11g %.11g\n", xCutSize, yCutSize);
    fprintf(defwFile, "      + LAYERS %s %s %s\n", botMetalLayer,
            cutLayer, topMetalLayer);
    fprintf(defwFile, "      + CUTSPACING %.11g %.11g\n",
            xCutSpacing, yCutSpacing);
    fprintf(defwFile, "      + ENCLOSURE %.11g %.11g %.11g %.11g",
            xBotEnc, yBotEnc, xTopEnc, yTopEnc);
    defwLines += 5;
    defwState = DEFW_VIAVIARULE;
    defwViaHasVal = 1;
    return DEFW_OK;
}

int
defwViaViaruleRowCol(int    numCutRows,
                     int    numCutCols)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIAVIARULE)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ROWCOL %d %d", numCutRows, numCutCols);
    defwLines++;
    return DEFW_OK;
}

int
defwViaViaruleOrigin(int    xOffset,
                     int    yOffset)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIAVIARULE)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ORIGIN %d %d", xOffset, yOffset);
    defwLines++;
    return DEFW_OK;
}

int
defwViaViaruleOffset(int    xBotOffset,
                     int    yBotOffset,
                     int    xTopOffset,
                     int    yTopOffset)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIAVIARULE)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + OFFSET %d %d %d %d",
            xBotOffset, yBotOffset, xTopOffset, yTopOffset);
    defwLines++;
    return DEFW_OK;
}

int
defwViaViarulePattern(const char *cutPattern)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIAVIARULE)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + PATTERN %s", cutPattern);
    defwLines++;
    return DEFW_OK;
}

int
defwOneViaEnd()
{
    defwFunc = DEFW_VIA;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState != DEFW_VIA) && (defwState != DEFW_VIAVIARULE))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, " ;\n");
    defwLines++;

    defwState = DEFW_VIAONE_END;
    return DEFW_OK;
}


int
defwEndVias()
{
    defwFunc = DEFW_VIA_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_VIA_START &&
        defwState != DEFW_VIAONE_END)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END VIAS\n\n");
    defwLines++;

    defwState = DEFW_VIA_END;
    return DEFW_OK;
}


int
defwStartRegions(int count)
{
    defwFunc = DEFW_REGION_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_REGION_START) && (defwState <= DEFW_REGION_END))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    fprintf(defwFile, "REGIONS %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_REGION_START;
    return DEFW_OK;
}


int
defwRegionName(const char *name)
{
    defwFunc = DEFW_REGION;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_REGION_START &&
        defwState != DEFW_REGION)
        return DEFW_BAD_ORDER;
    defwCounter--;

    if (defwState == DEFW_REGION)
        fprintf(defwFile, ";\n");  // add the ; and \n for the previous row.

    if (!name || !*name) // required
        return DEFW_BAD_DATA;
    fprintf(defwFile, "   - %s ", name);
    defwState = DEFW_REGION;
    return DEFW_OK;
}


int
defwRegionPoints(int    xl,
                 int    yl,
                 int    xh,
                 int    yh)
{
    defwFunc = DEFW_REGION;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_REGION)
        return DEFW_BAD_ORDER;  // after RegionName

    fprintf(defwFile, "      ( %d %d ) ( %d %d ) ", xl, yl, xh, yh);

    defwState = DEFW_REGION;
    return DEFW_OK;
}


int
defwRegionType(const char *type)
{
    defwFunc = DEFW_REGION;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_REGION)
        return DEFW_BAD_ORDER;  // after RegionName

    if (!type || !*type) // required
        return DEFW_BAD_DATA;
    if (strcmp(type, "FENCE") && strcmp(type, "GUIDE"))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "         + TYPE %s ", type);

    defwState = DEFW_REGION;
    return DEFW_OK;
}


int
defwEndRegions()
{
    defwFunc = DEFW_REGION_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_REGION_START &&
        defwState != DEFW_REGION)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    if (defwState == DEFW_REGION)
        fprintf(defwFile, ";\nEND REGIONS\n\n");  // ; for the previous statement
    else
        fprintf(defwFile, "END REGIONS\n\n");  // ; for the previous statement
    defwLines++;

    defwState = DEFW_REGION_END;
    return DEFW_OK;
}


int
defwComponentMaskShiftLayers(const char **layerNames,
                             int        numLayerName)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_COMPONENT_MASKSHIFTLAYERS;

    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if (defwState == DEFW_COMPONENT_MASKSHIFTLAYERS)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "COMPONENTMASKSHIFT ");

    if (numLayerName) {
        for (int i = 0; i < numLayerName; i++)
            fprintf(defwFile, "%s ", layerNames[i]);
    }

    fprintf(defwFile, ";\n\n");

    defwLines++;

    defwState = DEFW_COMPONENT_MASKSHIFTLAYERS;
    return DEFW_OK;
}


int
defwStartComponents(int count)
{
    defwFunc = DEFW_COMPONENT_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_COMPONENT_START) && (defwState <= DEFW_COMPONENT_END))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    fprintf(defwFile, "COMPONENTS %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_COMPONENT_START;
    return DEFW_OK;
}


int
defwComponent(const char    *instance,
              const char    *master,
              int           numNetName,
              const char    **netNames,
              const char    *eeq,
              const char    *genName,
              const char    *genParemeters,
              const char    *source,
              int           numForeign,
              const char    **foreigns,
              int           *foreignX,
              int           *foreignY,
              int           *foreignOrients,
              const char    *status,
              int           statusX,
              int           statusY,
              int           statusOrient,
              double        weight,
              const char    *region,
              int           xl,
              int           yl,
              int           xh,
              int           yh)
{

    int i;
    int uplace = 0;

    defwFunc = DEFW_COMPONENT;   // Current function of writer

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_COMPONENT_START &&
        defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;

    defwCounter--;

    // required
    if ((instance == 0) || (*instance == 0) || (master == 0) || (*master == 0))
        return DEFW_BAD_DATA;

    if (source && strcmp(source, "NETLIST") && strcmp(source, "DIST") &&
        strcmp(source, "USER") && strcmp(source, "TIMING"))
        return DEFW_BAD_DATA;

    if (status) {
        if (strcmp(status, "UNPLACED") == 0) {
            uplace = 1;
        } else if (strcmp(status, "COVER") && strcmp(status, "FIXED") &&
                   strcmp(status, "PLACED"))
            return DEFW_BAD_DATA;
    }

    // only either region or xl, yl, xh, yh
    if (region && (xl || yl || xh || yh))
        return DEFW_BAD_DATA;

    if (defwState == DEFW_COMPONENT)
        fprintf(defwFile, ";\n");       // newline for the previous component

    fprintf(defwFile, "   - %s %s ", instance, master);
    if (numNetName) {
        for (i = 0; i < numNetName; i++)
            fprintf(defwFile, "%s ", netNames[i]);
    }
    defwLines++;
    // since the rest is optionals, new line is placed before the options
    if (eeq) {
        fprintf(defwFile, "\n      + EEQMASTER %s ", eeq);
        defwLines++;
    }
    if (genName) {
        fprintf(defwFile, "\n      + GENERATE %s ", genName);
        if (genParemeters)
            fprintf(defwFile, " %s ", genParemeters);
        defwLines++;
    }
    if (source) {
        fprintf(defwFile, "\n      + SOURCE %s ", source);
        defwLines++;
    }
    if (numForeign) {
        for (i = 0; i < numForeign; i++) {
            fprintf(defwFile, "\n      + FOREIGN %s ( %d %d ) %s ", foreigns[i],
                    foreignX[i], foreignY[i], defwOrient(foreignOrients[i]));
            defwLines++;
        }
    }
    if (status && (uplace == 0)) {
        fprintf(defwFile, "\n      + %s ( %d %d ) %s ", status, statusX, statusY,
                defwOrient(statusOrient));
    } else if (uplace) {
        fprintf(defwFile, "\n      + %s ", status);
    }
    defwLines++;
    if (weight) {
        fprintf(defwFile, "\n      + WEIGHT %.11g ", weight);
        defwLines++;
    }
    if (region) {
        fprintf(defwFile, "\n      + REGION %s ", region);
        defwLines++;
    } else if (xl || yl || xh || yh) {
        fprintf(defwFile, "\n      + REGION ( %d %d ) ( %d %d ) ",
                xl, yl, xh, yh);
        defwLines++;
    }

    defwState = DEFW_COMPONENT;
    return DEFW_OK;
}


int
defwComponentStr(const char *instance,
                 const char *master,
                 int        numNetName,
                 const char **netNames,
                 const char *eeq,
                 const char *genName,
                 const char *genParemeters,
                 const char *source,
                 int        numForeign,
                 const char **foreigns,
                 int        *foreignX,
                 int        *foreignY,
                 const char **foreignOrients,
                 const char *status,
                 int        statusX,
                 int        statusY,
                 const char *statusOrient,
                 double     weight,
                 const char *region,
                 int        xl,
                 int        yl,
                 int        xh,
                 int        yh)
{

    int i;
    int uplace = 0;

    defwFunc = DEFW_COMPONENT;   // Current function of writer

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_COMPONENT_START &&
        defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;

    defwCounter--;

    if ((!instance || !*instance) || (!master || !*master)) // required
        return DEFW_BAD_DATA;

    if (source && strcmp(source, "NETLIST") && strcmp(source, "DIST") &&
        strcmp(source, "USER") && strcmp(source, "TIMING"))
        return DEFW_BAD_DATA;

    if (status) {
        if (strcmp(status, "UNPLACED") == 0) {
            uplace = 1;
        } else if (strcmp(status, "COVER") && strcmp(status, "FIXED") &&
                   strcmp(status, "PLACED"))
            return DEFW_BAD_DATA;
    }

    // only either region or xl, yl, xh, yh
    if (region && (xl || yl || xh || yh))
        return DEFW_BAD_DATA;

    if (defwState == DEFW_COMPONENT)
        fprintf(defwFile, ";\n");       // newline for the previous component

    fprintf(defwFile, "   - %s %s ", instance, master);
    if (numNetName) {
        for (i = 0; i < numNetName; i++)
            fprintf(defwFile, "%s ", netNames[i]);
    }
    defwLines++;
    // since the rest is optionals, new line is placed before the options
    if (eeq) {
        fprintf(defwFile, "\n      + EEQMASTER %s ", eeq);
        defwLines++;
    }
    if (genName) {
        fprintf(defwFile, "\n      + GENERATE %s ", genName);
        if (genParemeters)
            fprintf(defwFile, " %s ", genParemeters);
        defwLines++;
    }
    if (source) {
        fprintf(defwFile, "\n      + SOURCE %s ", source);
        defwLines++;
    }
    if (numForeign) {
        for (i = 0; i < numForeign; i++) {
            fprintf(defwFile, "\n      + FOREIGN %s ( %d %d ) %s ", foreigns[i],
                    foreignX[i], foreignY[i], foreignOrients[i]);
            defwLines++;
        }
    }
    if (status && (uplace == 0)) {
        fprintf(defwFile, "\n      + %s ( %d %d ) %s ", status, statusX, statusY,
                statusOrient);
    } else if (uplace) {
        fprintf(defwFile, "\n      + %s ", status);
    }
    defwLines++;
    if (weight) {
        fprintf(defwFile, "\n      + WEIGHT %.11g ", weight);
        defwLines++;
    }
    if (region) {
        fprintf(defwFile, "\n      + REGION %s ", region);
        defwLines++;
    } else if (xl || yl || xh || yh) {
        fprintf(defwFile, "\n      + REGION ( %d %d ) ( %d %d ) ",
                xl, yl, xh, yh);
        defwLines++;
    }

    defwState = DEFW_COMPONENT;
    return DEFW_OK;
}

int
defwComponentMaskShift(int shiftLayerMasks)
{
    defwFunc = DEFW_COMPONENT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    if (defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + MASKSHIFT %d ", shiftLayerMasks);
    defwLines++;
    return DEFW_OK;
}

int
defwComponentHalo(int   left,
                  int   bottom,
                  int   right,
                  int   top)
{
    defwFunc = DEFW_COMPONENT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + HALO %d %d %d %d ", left, bottom, right, top);
    defwLines++;
    return DEFW_OK;
}

// 5.7
int
defwComponentHaloSoft(int   left,
                      int   bottom,
                      int   right,
                      int   top)
{
    defwFunc = DEFW_COMPONENT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;
    if (defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + HALO SOFT %d %d %d %d ", left, bottom,
            right, top);
    defwLines++;
    return DEFW_OK;
}

// 5.7
int
defwComponentRouteHalo(int          haloDist,
                       const char   *minLayer,
                       const char   *maxLayer)
{
    defwFunc = DEFW_COMPONENT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;
    if (defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ROUTEHALO %d %s %s ", haloDist, minLayer,
            maxLayer);
    defwLines++;
    return DEFW_OK;
}


int
defwEndComponents()
{
    defwFunc = DEFW_COMPONENT_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_COMPONENT_START &&
        defwState != DEFW_COMPONENT)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    defwDidComponents = 1;

    if (defwState == DEFW_COMPONENT)
        fprintf(defwFile, ";\nEND COMPONENTS\n\n");
    else
        fprintf(defwFile, "END COMPONENTS\n\n");
    defwLines++;

    defwState = DEFW_COMPONENT_END;
    return DEFW_OK;
}


int
defwStartPins(int count)
{
    defwFunc = DEFW_PIN_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidComponents)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_PIN_START) && (defwState <= DEFW_PIN_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "PINS %d", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_PIN_START;
    return DEFW_OK;
}


int
defwPin(const char  *name,
        const char  *net,
        int         special,       // optional 0-ignore 1-special 
        const char  *direction,                            // optional 
        const char  *use,                                  // optional 
        const char  *status,
        int         xo,
        int         yo,
        int         orient,   // optional 
        const char  *layer,
        int         xl,
        int         yl,
        int         xh,
        int         yh // optional 
        )
{

    defwFunc = DEFW_PIN;   // Current function of writer

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN_START && defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;

    defwCounter--;

    fprintf(defwFile, " ;\n   - %s + NET %s", name, net);

    if (special)
        fprintf(defwFile, "\n      + SPECIAL");
    if (direction) {
        if (strcmp(direction, "INPUT") && strcmp(direction, "OUTPUT") &&
            strcmp(direction, "INOUT") && strcmp(direction, "FEEDTHRU"))
            return DEFW_BAD_DATA;
        fprintf(defwFile, "\n      + DIRECTION %s", direction);
    }
    if (use) {
        if (strcmp(use, "SIGNAL") && strcmp(use, "POWER") &&
            strcmp(use, "GROUND") && strcmp(use, "CLOCK") &&
            strcmp(use, "TIEOFF") && strcmp(use, "ANALOG") &&
            strcmp(use, "SCAN") && strcmp(use, "RESET"))
            return DEFW_BAD_DATA;
        fprintf(defwFile, "\n      + USE %s", use);
    }
    if (status) {
        if (strcmp(status, "FIXED") && strcmp(status, "PLACED") &&
            strcmp(status, "COVER"))
            return DEFW_BAD_DATA;

        fprintf(defwFile, "\n      + %s ( %d %d ) %s", status, xo, yo,
                defwOrient(orient));
    }
    // In 5.6, user should use defPinLayer to write out layer construct 
    if (layer) {
        fprintf(defwFile, "\n      + LAYER %s ( %d %d ) ( %d %d )",
                layer, xl, yl, xh, yh);
    }

    defwLines++;

    defwState = DEFW_PIN;
    return DEFW_OK;
}


int
defwPinStr(const char   *name,
           const char   *net,
           int          special,       // optional 0-ignore 1-special 
           const char   *direction,                                    // optional 
           const char   *use,                                          // optional 
           const char   *status,
           int          xo,
           int          yo,
           const char   *orient,   // optional 
           const char   *layer,
           int          xl,
           int          yl,
           int          xh,
           int          yh // optional 
           )
{

    defwFunc = DEFW_PIN;   // Current function of writer

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN_START && defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;

    defwCounter--;

    fprintf(defwFile, " ;\n   - %s + NET %s", name, net);

    if (special)
        fprintf(defwFile, "\n      + SPECIAL");
    if (direction) {
        if (strcmp(direction, "INPUT") && strcmp(direction, "OUTPUT") &&
            strcmp(direction, "INOUT") && strcmp(direction, "FEEDTHRU"))
            return DEFW_BAD_DATA;
        fprintf(defwFile, "\n      + DIRECTION %s", direction);
    }
    if (use) {
        if (strcmp(use, "SIGNAL") && strcmp(use, "POWER") &&
            strcmp(use, "GROUND") && strcmp(use, "CLOCK") &&
            strcmp(use, "TIEOFF") && strcmp(use, "ANALOG") &&
            strcmp(use, "SCAN") && strcmp(use, "RESET"))
            return DEFW_BAD_DATA;
        fprintf(defwFile, "\n      + USE %s", use);
    }
    if (status) {
        if (strcmp(status, "FIXED") && strcmp(status, "PLACED") &&
            strcmp(status, "COVER"))
            return DEFW_BAD_DATA;

        fprintf(defwFile, "\n      + %s ( %d %d ) %s", status, xo, yo,
                orient);
    }
    // In 5.6, user should use defPinLayer to write out layer construct 
    if (layer) {
        fprintf(defwFile, "\n      + LAYER %s ( %d %d ) ( %d %d )",
                layer, xl, yl, xh, yh);
    }

    defwLines++;

    defwState = DEFW_PIN;
    return DEFW_OK;
}

int
defwPinLayer(const char *layerName,
             int        spacing,
             int        designRuleWidth,
             int        xl,
             int        yl,
             int        xh,
             int        yh,
             int        mask)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (spacing && designRuleWidth)
        return DEFW_BAD_DATA;  // only one, spacing
    // or designRuleWidth can be defined, not both

    fprintf(defwFile, "\n      + LAYER %s ", layerName);

    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n        MASK %d", mask);
    }

    if (spacing)
        fprintf(defwFile, "\n        SPACING %d", spacing);
    else if (designRuleWidth)        // can be both 0
        fprintf(defwFile, "\n        DESIGNRULEWIDTH  %d", designRuleWidth);
    fprintf(defwFile, "\n        ( %d %d ) ( %d %d )", xl, yl, xh, yh);

    defwState = DEFW_PIN;
    defwLines++;
    return DEFW_OK;
}

int
defwPinPolygon(const char   *layerName,
               int          spacing,
               int          designRuleWidth,
               int          num_polys,
               double       *xl,
               double       *yl,
               int          mask)
{
    int i;

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (spacing && designRuleWidth)
        return DEFW_BAD_DATA; // only one, spacing
    // or designRuleWidth can be defined, not both

    fprintf(defwFile, "\n      + POLYGON %s ", layerName);

    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n        MASK %d", mask);
    }

    if (spacing)
        fprintf(defwFile, "\n        SPACING %d", spacing);
    else if (designRuleWidth)        // can be both 0
        fprintf(defwFile, "\n        DESIGNRULEWIDTH  %d", designRuleWidth);

    printPointsNum = 0;

    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) == 0)) {
            printPoints(defwFile, *xl++, *yl++, "\n        ", " ");
            defwLines++; 
        } else
            printPoints(defwFile, *xl++, *yl++, "", " ");
    }
   
    defwState = DEFW_PIN;
    defwLines++;
    return DEFW_OK;
}

// 5.7
int
defwPinVia(const char   *viaName,
           int          xl,
           int          yl,
           int          mask)
{

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;

    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n      + VIA %s MASK %d ( %d %d ) ", viaName, mask, xl, yl);
    } else {
        fprintf(defwFile, "\n      + VIA %s ( %d %d ) ", viaName, xl, yl);
    }

    defwLines++;
    defwState = DEFW_PIN;
    return DEFW_OK;
}


// 5.7
int
defwPinPort()
{

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + PORT");

    defwLines++;
    defwState = DEFW_PIN_PORT;
    return DEFW_OK;
}

// 5.7
int
defwPinPortLayer(const char *layerName,
                 int        spacing,
                 int        designRuleWidth,
                 int        xl,
                 int        yl,
                 int        xh,
                 int        yh,
                 int        mask)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN && defwState != DEFW_PIN_PORT)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;
    if (spacing && designRuleWidth)
        return DEFW_BAD_DATA;  // only one, spacing
    // or designRuleWidth can be defined, not both

    fprintf(defwFile, "\n        + LAYER %s ", layerName);

    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n          MASK %d", mask);
    }

    if (spacing)
        fprintf(defwFile, "\n          SPACING %d", spacing);
    else if (designRuleWidth)        // can be both 0
        fprintf(defwFile, "\n          DESIGNRULEWIDTH  %d", designRuleWidth);

    fprintf(defwFile, "\n        ( %d %d ) ( %d %d )", xl, yl, xh, yh);

    defwState = DEFW_PIN;
    defwLines++;
    return DEFW_OK;
}

// 5.7
int
defwPinPortPolygon(const char   *layerName,
                   int          spacing,
                   int          designRuleWidth,
                   int          num_polys,
                   double       *xl,
                   double       *yl,
                   int          mask)
{
    int i;

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN && defwState != DEFW_PIN_PORT)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;
    if (spacing && designRuleWidth)
        return DEFW_BAD_DATA; // only one, spacing
    // or designRuleWidth can be defined, not both

    fprintf(defwFile, "\n        + POLYGON %s ", layerName);

    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n          MASK %d", mask);
    }

    if (spacing)
        fprintf(defwFile, "\n          SPACING %d", spacing);
    else if (designRuleWidth)        // can be both 0
        fprintf(defwFile, "\n          DESIGNRULEWIDTH  %d", designRuleWidth);

    printPointsNum = 0;
    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) == 0)) {
            printPoints(defwFile, *xl++, *yl++, "\n          ", " ");
            defwLines++; 
        } else
            printPoints(defwFile, *xl++, *yl++, "", " ");
    }

    defwState = DEFW_PIN;
    defwLines++;
    return DEFW_OK;
}

// 5.7
int
defwPinPortVia(const char   *viaName,
               int          xl,
               int          yl,
               int          mask)
{

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN && defwState != DEFW_PIN_PORT)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;

    if (mask) {
        if (defVersionNum < 5.8) {
            return DEFW_WRONG_VERSION;
        }

        fprintf(defwFile, "\n        + VIA %s MASK %d ( %d %d ) ", viaName, mask, xl, yl);
    } else {
        fprintf(defwFile, "\n        + VIA %s ( %d %d ) ", viaName, xl, yl);
    }

    defwLines++;
    defwState = DEFW_PIN;
    return DEFW_OK;
}

// 5.7
int
defwPinPortLocation(const char  *status,
                    int         statusX,
                    int         statusY,
                    const char  *orient)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN && defwState != DEFW_PIN_PORT)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.7)
        return DEFW_WRONG_VERSION;

    if (strcmp(status, "FIXED") && strcmp(status, "PLACED") &&
        strcmp(status, "COVER"))
        return DEFW_BAD_DATA;
    fprintf(defwFile, "\n        + %s ( %d %d ) %s ", status, statusX, statusY,
            orient);
    defwState = DEFW_PIN;
    defwLines++;
    return DEFW_OK;
}

int
defwPinNetExpr(const char *pinExpr)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (pinExpr && pinExpr != 0 && *pinExpr != 0)
        fprintf(defwFile, "\n      + NETEXPR \"%s\"", pinExpr);

    defwLines++;
    return DEFW_OK;
}


int
defwPinSupplySensitivity(const char *pinName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (pinName && pinName != 0 && *pinName != 0)
        fprintf(defwFile, "\n      + SUPPLYSENSITIVITY %s", pinName);

    defwLines++;
    return DEFW_OK;
}


int
defwPinGroundSensitivity(const char *pinName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;
    if (pinName && pinName != 0 && *pinName != 0)
        fprintf(defwFile, "\n      + GROUNDSENSITIVITY %s", pinName);

    defwLines++;
    return DEFW_OK;
}


int
defwPinAntennaPinPartialMetalArea(int           value,
                                  const char    *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINPARTIALMETALAREA %d", value);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinPartialMetalSideArea(int           value,
                                      const char    *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINPARTIALMETALSIDEAREA %d", value);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinPartialCutArea(int         value,
                                const char  *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINPARTIALCUTAREA %d", value);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaModel(const char *oxide)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAMODEL %s", oxide);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinDiffArea(int           value,
                          const char    *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINDIFFAREA %d", value);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinGateArea(int           value,
                          const char    *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINGATEAREA %d", value);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinMaxAreaCar(int         value,
                            const char  *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINMAXAREACAR %d", value);
    if (!layerName)
        return DEFW_BAD_DATA;  // layerName is required 

    fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinMaxSideAreaCar(int         value,
                                const char  *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINMAXSIDEAREACAR %d", value);
    if (!layerName)
        return DEFW_BAD_DATA;  // layerName is required 

    fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwPinAntennaPinMaxCutCar(int          value,
                           const char   *layerName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + ANTENNAPINMAXCUTCAR %d", value);
    if (!layerName)
        return DEFW_BAD_DATA;

    fprintf(defwFile, " LAYER %s", layerName);
    defwLines++;

    return DEFW_OK;
}


int
defwEndPins()
{
    defwFunc = DEFW_PIN_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PIN_START && defwState != DEFW_PIN)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, " ;\nEND PINS\n\n");
    defwLines++;

    defwState = DEFW_PIN_END;
    return DEFW_OK;
}


int
defwStartPinProperties(int count)
{
    defwFunc = DEFW_PINPROP_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_PINPROP_START) && (defwState <= DEFW_PINPROP_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "PINPROPERTIES %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_PINPROP_START;
    return DEFW_OK;
}


int
defwPinProperty(const char  *name,
                const char  *pinName)
{

    defwFunc = DEFW_PINPROP;   // Current function of writer

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PINPROP_START &&
        defwState != DEFW_PINPROP)
        return DEFW_BAD_ORDER;

    defwCounter--;
    if ((!name || !*name) || (!pinName || !*pinName)) // required
        return DEFW_BAD_DATA;

    if (defwState == DEFW_PINPROP)
        fprintf(defwFile, ";\n");

    fprintf(defwFile, "   - %s %s ", name, pinName);
    defwLines++;

    defwState = DEFW_PINPROP;
    return DEFW_OK;
}


int
defwEndPinProperties()
{
    defwFunc = DEFW_PIN_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PINPROP_START &&
        defwState != DEFW_PINPROP)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    if (defwState == DEFW_PINPROP_START)
        fprintf(defwFile, "END PINPROPERTIES\n\n");
    else
        fprintf(defwFile, ";\nEND PINPROPERTIES\n\n");
    defwLines++;

    defwState = DEFW_PINPROP_END;
    return DEFW_OK;
}


int
defwStartSpecialNets(int count)
{
    defwFunc = DEFW_SNET_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_SNET_START) && (defwState <= DEFW_SNET_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "SPECIALNETS %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_SNET_START;
    return DEFW_OK;
}


int
defwSpecialNetOptions()
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (defwState == DEFW_SNET) {
        defwState = DEFW_SNET_OPTIONS;
        return 1;
    }
    if (defwState == DEFW_SNET_OPTIONS)
        return 1;
    return 0;
}


int
defwSpecialNet(const char *name)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SNET_START &&
        defwState != DEFW_SNET_ENDNET)
        return DEFW_BAD_ORDER;
    defwState = DEFW_SNET;

    fprintf(defwFile, "   - %s", name);
    defwLineItemCounter = 0;
    defwCounter--;

    return DEFW_OK;
}


int
defwSpecialNetConnection(const char *inst,
                         const char *pin,
                         int        synthesized)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SNET)
        return DEFW_BAD_ORDER;

    if ((++defwLineItemCounter & 3) == 0) { // since a net can have more than
        fprintf(defwFile, "\n     ");  // one inst pin connection, don't print
        defwLines++;             // newline until the line is certain length
    }
    fprintf(defwFile, " ( %s %s ", inst, pin);
    if (synthesized)
        fprintf(defwFile, " + SYNTHESIZED ");
    fprintf(defwFile, ") ");
    return DEFW_OK;
}


int
defwSpecialNetFixedbump()
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + FIXEDBUMP");
    defwLines++;
    return DEFW_OK;
}

int
defwSpecialNetVoltage(double d)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    int v = (int)(d * 1000);

    fprintf(defwFile, "\n      + VOLTAGE %d", v);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetSpacing(const char    *layer,
                      int           spacing,
                      double        minwidth,
                      double        maxwidth)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + SPACING %s %d", layer, spacing);
    if (minwidth || maxwidth)
        fprintf(defwFile, " RANGE %.11g %.11g", minwidth, maxwidth);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetWidth(const char  *layer,
                    int         w)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + WIDTH %s %d", layer, w);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetSource(const char *name)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + SOURCE %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetOriginal(const char *name)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ORIGINAL %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetPattern(const char *name)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + PATTERN %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetUse(const char *name)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    if (strcmp(name, "SIGNAL") && strcmp(name, "POWER") &&
        strcmp(name, "GROUND") && strcmp(name, "CLOCK") &&
        strcmp(name, "TIEOFF") && strcmp(name, "ANALOG") &&
        strcmp(name, "SCAN") && strcmp(name, "RESET"))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + USE %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetWeight(double d)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + WEIGHT %.11g", d);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetEstCap(double d)
{
    defwFunc = DEFW_SNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ESTCAP %.11g", d);
    defwLines++;
    return DEFW_OK;
}

int
defwSpecialNetPathStart(const char *typ)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions() &&
        (defwState != DEFW_SUBNET) && // path in subnet 
        (defwState != DEFW_PATH))   // NEW in the path, path hasn't end yet 
        return DEFW_BAD_ORDER;

    if (strcmp(typ, "NEW") && strcmp(typ, "FIXED") && strcmp(typ, "COVER") &&
        strcmp(typ, "ROUTED") && strcmp(typ, "SHIELD"))
        return DEFW_BAD_DATA;

    defwSpNetShield = 0;

    // The second time around for a path on this net, we
    // must start it with a new instead of a fixed...
    if (strcmp(typ, "NEW") == 0) {
        if (defwState != DEFW_PATH)
            return DEFW_BAD_DATA;
        fprintf(defwFile, " NEW");
    } else if (strcmp(typ, "SHIELD") == 0) {
        fprintf(defwFile, "\n      + %s", typ);
        defwSpNetShield = 1;
    } else
        fprintf(defwFile, "\n      + %s", typ);

    defwState = DEFW_PATH_START;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwSpecialNetShieldNetName(const char *name)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH_START)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    if (defwSpNetShield)
        fprintf(defwFile, " %s", name);
    else
        return DEFW_BAD_ORDER;
    return DEFW_OK;
}


int
defwSpecialNetPathWidth(int w)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %d", w);
    return DEFW_OK;
}


int
defwSpecialNetPathLayer(const char *name)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH_START)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %s", name);
    defwState = DEFW_PATH;
    return DEFW_OK;
}


int
defwSpecialNetPathStyle(int styleNum)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;

    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, "\n      + STYLE %d", styleNum);
    defwState = DEFW_PATH;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwSpecialNetPathShape(const char *typ)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;

    if (strcmp(typ, "RING") && strcmp(typ, "STRIPE") && strcmp(typ, "FOLLOWPIN") &&
        strcmp(typ, "IOWIRE") && strcmp(typ, "COREWIRE") &&
        strcmp(typ, "BLOCKWIRE") && strcmp(typ, "FILLWIRE") &&
        strcmp(typ, "BLOCKAGEWIRE") && strcmp(typ, "PADRING") &&
        strcmp(typ, "BLOCKRING") && strcmp(typ, "DRCFILL") &&
        strcmp(typ, "FILLWIREOPC"))      // 5.7
        return DEFW_BAD_DATA;

    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, "\n      + SHAPE %s", typ);

    defwState = DEFW_PATH;
    defwLineItemCounter = 0;
    return DEFW_OK;
}

int
defwSpecialNetPathMask(int colorMask)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_PATH;   // Current function of writer

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " MASK %d", colorMask);
    return DEFW_OK;
}

int
defwSpecialNetPathPoint(int     numPts,
                        double  *pointx,
                        double  *pointy)
{
    int i;

    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;

    printPointsNum = 0;
    for (i = 0; i < numPts; i++) {
          if ((++defwLineItemCounter & 3) == 0) {
              fprintf(defwFile, "\n     ");
              defwLines++;
          }

          printPoints(defwFile, pointx[i], pointy[i], " ", "");
      }
    return DEFW_OK;
}


int
defwSpecialNetPathVia(const char *name)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %s", name);
    return DEFW_OK;
}


int
defwSpecialNetPathViaData(int   numX,
                          int   numY,
                          int   stepX,
                          int   stepY)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " DO %d BY %d STEP %d %d", numX, numY, stepX, stepY);
    return DEFW_OK;
}


int
defwSpecialNetPathPointWithWireExt(int      numPts,
                                   double   *pointx,
                                   double   *pointy,
                                   double   *optValue)
{
    int i;
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    for (i = 0; i < numPts; i++) {
        if ((++defwLineItemCounter & 3) == 0) {
            fprintf(defwFile, "\n        ");
            defwLines++;
        }
        fprintf(defwFile, " ( %.11g %.11g ", pointx[i], pointy[i]);
        if (optValue[i])
            fprintf(defwFile, "%.11g ", optValue[i]);
        fprintf(defwFile, ")");
    }
    return DEFW_OK;
}


int
defwSpecialNetPathEnd()
{
    defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    defwState = DEFW_SNET_OPTIONS;
    return DEFW_OK;
}


int 
defwSpecialNetPolygon(const char* layerName,
                     int num_polys, 
                     double* xl, double* yl) {
                              int i;

                              defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
                              if (! defwSpecialNetOptions() &&
                                  (defwState != DEFW_PATH))  // not inside a path
                                  return DEFW_BAD_ORDER;

                              if (defVersionNum < 5.6)
                                  return DEFW_WRONG_VERSION;

                              fprintf(defwFile, "\n      + POLYGON %s ", layerName);

                              printPointsNum = 0;
                              for (i = 0; i < num_polys; i++) {
                                  if ((i == 0) || ((i % 5) != 0))
                                      printPoints(defwFile, *xl++, *yl++, "", " ");
                                  else {
                                      printPoints(defwFile,  *xl++, *yl++, "\n                ", " ");
                                      defwLines++; 
                                  }
                              }
                              defwLines++; 
                              return DEFW_OK;
}


int
defwSpecialNetRect(const char   *layerName,
                   int          xl,
                   int          yl,
                   int          xh,
                   int          yh)
{
    defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
    if (!defwSpecialNetOptions() &&
        (defwState != DEFW_PATH))  // not inside a path
        return DEFW_BAD_ORDER;

    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + RECT %s ( %d %d ) ( %d %d ) ", layerName,
            xl, yl, xh, yh);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetVia(const char *layerName)
{
    defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
    if (!defwSpecialNetOptions() &&
        (defwState != DEFW_PATH))  // not inside a path
        return DEFW_BAD_ORDER;

    if (defVersionNum < 5.8)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + VIA %s ", layerName);
    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetViaWithOrient(const char  *layerName,
                            int         orient)
{
    defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
    if (!defwSpecialNetOptions() &&
        (defwState != DEFW_PATH))  // not inside a path
        return DEFW_BAD_ORDER;

    if (defVersionNum < 5.8)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "\n      + VIA %s %s", layerName, defwOrient(orient));

    defwLines++;
    return DEFW_OK;
}


int
defwSpecialNetViaPoints(int     num_points,
                        double  *xl,
                        double  *yl)
{
    defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
    if (!defwSpecialNetOptions() &&
        (defwState != DEFW_PATH))  // not inside a path
        return DEFW_BAD_ORDER;

    if (defVersionNum < 5.8)
        return DEFW_WRONG_VERSION;


    printPointsNum = 0;
    for (int i = 0; i < num_points; i++) {
        if ((i == 0) || ((i % 5) != 0))
            printPoints(defwFile, *xl++, *yl++, "", " ");
        else {
            printPoints(defwFile, *xl++, *yl++, "\n             ", " ");
            defwLines++;
        }
    }

    defwLines++;
    return DEFW_OK;

}


int
defwSpecialNetShieldStart(const char *name)
{
    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    // The second time around for a shield on this net, we
    // must start it with a new instead of the name ...
    if (strcmp(name, "NEW") == 0) {
        if (defwState != DEFW_SHIELD)
            return DEFW_BAD_DATA;
        fprintf(defwFile, " NEW");
    } else
        fprintf(defwFile, "\n      + SHIELD %s", name);

    defwState = DEFW_SHIELD;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwSpecialNetShieldWidth(int w)
{
    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %d", w);
    return DEFW_OK;
}


int
defwSpecialNetShieldLayer(const char *name)
{
    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %s", name);
    return DEFW_OK;
}


int
defwSpecialNetShieldShape(const char *typ)
{
    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;

    if (strcmp(typ, "RING") && strcmp(typ, "STRIPE") && strcmp(typ, "FOLLOWPIN") &&
        strcmp(typ, "IOWIRE") && strcmp(typ, "COREWIRE") &&
        strcmp(typ, "BLOCKWIRE") && strcmp(typ, "FILLWIRE") &&
        strcmp(typ, "BLOCKAGEWIRE") && strcmp(typ, "DRCFILL"))
        return DEFW_BAD_DATA;

    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, "\n      + SHAPE %s", typ);

    defwState = DEFW_SHIELD;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwSpecialNetShieldPoint(int       numPts,
                          double    *pointx,
                          double    *pointy)
{
    int i;

    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;

    printPointsNum = 0;
    for (i = 0; i < numPts; i++) {
        if ((++defwLineItemCounter & 3) == 0) {
            fprintf(defwFile, "\n     ");
            defwLines++;
        }
     printPoints(defwFile, pointx[i], pointy[i], " ", "");
    }
    return DEFW_OK;
}


int
defwSpecialNetShieldVia(const char *name)
{
    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %s", name);
    return DEFW_OK;
}


int
defwSpecialNetShieldViaData(int numX,
                            int numY,
                            int stepX,
                            int stepY)
{
    defwFunc = DEFW_SHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " DO %d BY %d STEP %d %d", numX, numY, stepX, stepY);
    return DEFW_OK;
}

int
defwSpecialNetShieldEnd()
{
    defwFunc = DEFW_SNET_OPTIONS;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SHIELD)
        return DEFW_BAD_ORDER;
    defwState = DEFW_SNET_OPTIONS;
    return DEFW_OK;
}


int
defwSpecialNetEndOneNet()
{
    defwFunc = DEFW_SNET_ENDNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwSpecialNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, " ;\n");
    defwLines++;
    defwState = DEFW_SNET_ENDNET;

    return DEFW_OK;
}


int
defwEndSpecialNets()
{
    defwFunc = DEFW_SNET_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SNET_START && defwState != DEFW_SNET_OPTIONS &&
        defwState != DEFW_SNET_ENDNET &&   // last state is special net
        defwState != DEFW_SNET)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END SPECIALNETS\n\n");
    defwLines++;

    defwState = DEFW_SNET_END;
    return DEFW_OK;
}


int
defwStartNets(int count)
{
    defwFunc = DEFW_NET_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_NET_START) && (defwState <= DEFW_NET_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "NETS %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_NET_START;
    return DEFW_OK;
}


int
defwNetOptions()
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (defwState == DEFW_NET) {
        defwState = DEFW_NET_OPTIONS;
        return 1;
    }
    if (defwState == DEFW_NET_OPTIONS)
        return 1;
    return 0;
}


int
defwNet(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NET_START &&
        defwState != DEFW_NET_ENDNET)
        return DEFW_BAD_ORDER;
    defwState = DEFW_NET;

    fprintf(defwFile, "   - %s", name);
    defwLineItemCounter = 0;
    defwCounter--;

    return DEFW_OK;
}


int
defwNetConnection(const char    *inst,
                  const char    *pin,
                  int           synthesized)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NET)
        return DEFW_BAD_ORDER;

    if ((++defwLineItemCounter & 3) == 0) {  // since there is more than one
        fprintf(defwFile, "\n"); // inst & pin connection, don't print newline
        defwLines++;             // until the line is certain length long
    }
    fprintf(defwFile, " ( %s %s", inst, pin);
    if (synthesized)
        fprintf(defwFile, " + SYNTHESIZED ) ");
    else
        fprintf(defwFile, " ) ");
    return DEFW_OK;
}


int
defwNetMustjoinConnection(const char    *inst,
                          const char    *pin)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NET_ENDNET)
        return DEFW_BAD_ORDER;

    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " - MUSTJOIN ( %s %s )", inst, pin);

  defwState = DEFW_NET;
  
  defwCounter--;

    return DEFW_OK;
}


int
defwNetFixedbump()
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + FIXEDBUMP");
    defwLines++;
    return DEFW_OK;
}


int
defwNetFrequency(double frequency)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + FREQUENCY %.11g", frequency);
    defwLines++;
    return DEFW_OK;
}


int
defwNetSource(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + SOURCE %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwNetXtalk(int xtalk)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + XTALK %d", xtalk);
    defwLines++;
    return DEFW_OK;
}


int
defwNetVpin(const char  *vpinName,
            const char  *layerName,
            int         layerXl,
            int         layerYl,
            int         layerXh,
            int         layerYh,
            const char  *status,
            int         statusX,
            int         statusY,
            int         orient)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;
    if ((vpinName == 0) || (*vpinName == 0)) // required
        return DEFW_BAD_DATA;

    if (status && strcmp(status, "PLACED") && strcmp(status, "FIXED") &&
        strcmp(status, "COVER"))
        return DEFW_BAD_DATA;
    if (status && (orient == 1))  // require if status is set
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + VPIN %s", vpinName);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    fprintf(defwFile, " ( %d %d ) ( %d %d )\n", layerXl, layerYl, layerXh,
            layerYh);
    defwLines++;

    if (status)
        fprintf(defwFile, "         %s ( %d %d ) %s", status, statusX, statusY,
                defwOrient(orient));
    defwLines++;
    return DEFW_OK;
}


int
defwNetVpinStr(const char   *vpinName,
               const char   *layerName,
               int          layerXl,
               int          layerYl,
               int          layerXh,
               int          layerYh,
               const char   *status,
               int          statusX,
               int          statusY,
               const char   *orient)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;
    if (!vpinName || !*vpinName) // required
        return DEFW_BAD_DATA;

    if (status && strcmp(status, "PLACED") && strcmp(status, "FIXED") &&
        strcmp(status, "COVER"))
        return DEFW_BAD_DATA;
    if (status && orient && *orient == '\0')  // require if status is set
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + VPIN %s", vpinName);
    if (layerName)
        fprintf(defwFile, " LAYER %s", layerName);
    fprintf(defwFile, " ( %d %d ) ( %d %d )\n", layerXl, layerYl, layerXh,
            layerYh);
    defwLines++;

    if (status)
        fprintf(defwFile, "         %s ( %d %d ) %s", status, statusX, statusY,
                orient);
    defwLines++;
    return DEFW_OK;
}


int
defwNetOriginal(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ORIGINAL %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwNetPattern(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + PATTERN %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwNetUse(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    if (strcmp(name, "SIGNAL") && strcmp(name, "POWER") &&
        strcmp(name, "GROUND") && strcmp(name, "CLOCK") &&
        strcmp(name, "TIEOFF") && strcmp(name, "ANALOG") &&
        strcmp(name, "SCAN") && strcmp(name, "RESET"))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + USE %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwNetNondefaultRule(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState != DEFW_NET) && (defwState != DEFW_NET_OPTIONS) &&
        (defwState != DEFW_SUBNET))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_SUBNET)
        fprintf(defwFile, "\n         NONDEFAULTRULE %s", name);
    else
        fprintf(defwFile, "\n      + NONDEFAULTRULE %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwNetWeight(double d)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + WEIGHT %.11g", d);
    defwLines++;
    return DEFW_OK;
}


int
defwNetEstCap(double d)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + ESTCAP %.11g", d);
    defwLines++;
    return DEFW_OK;
}


int
defwNetShieldnet(const char *name)
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "\n      + SHIELDNET %s", name);
    defwLines++;
    return DEFW_OK;
}


int
defwNetNoshieldStart(const char *name)
{
    defwFunc = DEFW_NOSHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;
    fprintf(defwFile, "\n      + NOSHIELD %s", name);

    defwState = DEFW_NOSHIELD;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwNetNoshieldPoint(int        numPts,
                     const char **pointx,
                     const char **pointy)
{
    int i;

    defwFunc = DEFW_NOSHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NOSHIELD)
        return DEFW_BAD_ORDER;
    for (i = 0; i < numPts; i++) {
        if ((++defwLineItemCounter & 3) == 0) {
            fprintf(defwFile, "\n     ");
            defwLines++;
        }
        fprintf(defwFile, " ( %s %s )", pointx[i], pointy[i]);
    }
    return DEFW_OK;
}


int
defwNetNoshieldVia(const char *name)
{
    defwFunc = DEFW_NOSHIELD;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NOSHIELD)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " %s", name);
    return DEFW_OK;
}


int
defwNetNoshieldEnd()
{
    defwFunc = DEFW_NET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NOSHIELD)
        return DEFW_BAD_ORDER;
    defwState = DEFW_NET;
    return DEFW_OK;
}


int
defwNetSubnetStart(const char *name)
{
    defwFunc = DEFW_SUBNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;
    if (!name || !*name) // required
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + SUBNET %s", name);
    defwLines++;
    defwState = DEFW_SUBNET;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwNetSubnetPin(const char *compName,
                 const char *pinName)
{
    defwFunc = DEFW_SUBNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SUBNET)
        return DEFW_BAD_ORDER;
    if ((!compName || !*compName) || (!pinName || !*pinName)) // required
        return DEFW_BAD_DATA;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n        ");
        defwLines++;
    }
    fprintf(defwFile, " ( %s %s )", compName, pinName);
    defwLines++;
    return DEFW_OK;
}


int
defwNetSubnetEnd()
{
    defwFunc = DEFW_SUBNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState != DEFW_SUBNET) &&    // subnet does not have path 
        (defwState != DEFW_NET_OPTIONS)) // subnet has path and path just ended 
        return DEFW_BAD_ORDER;
    defwState = DEFW_NET_OPTIONS;
    return DEFW_OK;
}


int
defwNetPathStart(const char *typ)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions() && (defwState != DEFW_SUBNET) && // path in subnet 
        (defwState != DEFW_PATH))      // NEW in the path, path hasn't end yet 
        return DEFW_BAD_ORDER;

    if (strcmp(typ, "NEW") && strcmp(typ, "FIXED") && strcmp(typ, "COVER") &&
        strcmp(typ, "ROUTED") && strcmp(typ, "NOSHIELD"))
        return DEFW_BAD_DATA;

    // The second time around for a path on this net, we
    // must start it with a new instead of a fixed...
    if (strcmp(typ, "NEW") == 0) {
        if (defwState != DEFW_PATH)
            return DEFW_BAD_DATA;
        fprintf(defwFile, "\n         NEW");
    } else {
        if (defwState == DEFW_SUBNET)
            fprintf(defwFile, "\n      %s", typ);
        else
            fprintf(defwFile, "\n      + %s", typ);
    }

    defwState = DEFW_PATH_START;
    defwLineItemCounter = 0;
    return DEFW_OK;
}


int
defwNetPathWidth(int w)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n         ");
        defwLines++;
    }
    fprintf(defwFile, " %d", w);
    return DEFW_OK;
}


int
defwNetPathLayer(const char *name,
                 int        isTaper,
                 const char *ruleName)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH_START)
        return DEFW_BAD_ORDER;

    // only one, either isTaper or ruleName can be set
    if (isTaper && ruleName)
        return DEFW_BAD_DATA;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n        ");
        defwLines++;
    }
    fprintf(defwFile, " %s", name);
    if (isTaper)
        fprintf(defwFile, " TAPER");
    else if (ruleName)
        fprintf(defwFile, " TAPERRULE %s", ruleName);
    defwState = DEFW_PATH;
    return DEFW_OK;
}


int
defwNetPathStyle(int styleNum)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, " STYLE %d", styleNum);
    return DEFW_OK;
}


int
defwNetPathPoint(int    numPts,
                 double *pointx,
                 double *pointy)
{
    int i;
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;

    printPointsNum = 0;
    for (i = 0; i < numPts; i++) {
        if ((++defwLineItemCounter & 3) == 0) {
            fprintf(defwFile, "\n        ");
            defwLines++;
        }
        printPoints(defwFile, pointx[i], pointy[i], " ", "");
    }
    return DEFW_OK;
}

int
defwNetPathPointWithExt(int     numPts,
                        double  *pointx,
                        double  *pointy,
                        double  *optValue)
{
    int i;
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    for (i = 0; i < numPts; i++) {
        if ((++defwLineItemCounter & 3) == 0) {
            fprintf(defwFile, "\n        ");
            defwLines++;
        }
        fprintf(defwFile, " ( %.11g %.11g %.11g )", pointx[i], pointy[i], optValue[i]);
    }
    return DEFW_OK;
}

int
defwNetPathVia(const char *name)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n        ");
        defwLines++;
    }
    if (!name || !*name) // required
        return DEFW_BAD_DATA;

    fprintf(defwFile, " %s", name);
    return DEFW_OK;
}


int
defwNetPathViaWithOrient(const char *name,
                         int        orient)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n        ");
        defwLines++;
    }

    if (!name || !*name) // required
        return DEFW_BAD_DATA;

    if (orient == -1)
        fprintf(defwFile, " %s", name);
    else if (orient >= 0 && orient <= 7)
        fprintf(defwFile, " %s %s", name, defwOrient(orient));
    else
        return DEFW_BAD_DATA;
    return DEFW_OK;
}


int
defwNetPathViaWithOrientStr(const char  *name,
                            const char  *orient)
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n        ");
        defwLines++;
    }

    if (!name || !*name) // required
        return DEFW_BAD_DATA;

    if (!orient || !*orient)
        fprintf(defwFile, " %s", name);
    else
        fprintf(defwFile, " %s %s", name, orient);
    return DEFW_OK;
}

int
defwNetPathMask(int colorMask)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " MASK %d", colorMask);
    return DEFW_OK;
}

int
defwNetPathRect(int deltaX1,
                int deltaY1,
                int deltaX2,
                int deltaY2)
{
    if (defVersionNum < 5.8)
        return DEFW_WRONG_VERSION;

    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;

    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }

    fprintf(defwFile, " RECT ( %d %d %d %d )", deltaX1, deltaY1, deltaX2, deltaY2);

    return DEFW_OK;
}

int
defwNetPathVirtual(int  x,
                   int  y)
{
    if (defVersionNum < 5.8)
        return DEFW_WRONG_VERSION;

    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    if ((++defwLineItemCounter & 3) == 0) {
        fprintf(defwFile, "\n     ");
        defwLines++;
    }
    fprintf(defwFile, " VIRTUAL ( %d %d )", x, y);
    return DEFW_OK;
}

int
defwNetPathEnd()
{
    defwFunc = DEFW_PATH;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_PATH)
        return DEFW_BAD_ORDER;
    defwState = DEFW_NET_OPTIONS;
    return DEFW_OK;
}


int
defwNetEndOneNet()
{
    defwFunc = DEFW_NET_ENDNET;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwNetOptions())
        return DEFW_BAD_ORDER;

    fprintf(defwFile, " ;\n");
    defwLines++;
    defwState = DEFW_NET_ENDNET;

    return DEFW_OK;
}


int
defwEndNets()
{
    defwFunc = DEFW_NET_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NET_START && defwState != DEFW_NET_OPTIONS &&
        defwState != DEFW_NET &&
        defwState != DEFW_NET_ENDNET) // last state is a net 
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END NETS\n\n");
    defwLines++;

    defwState = DEFW_NET_END;
    defwDidNets = 1;
    return DEFW_OK;
}


int
defwStartIOTimings(int count)
{
    defwObsoleteNum = DEFW_IOTIMING_START;
    defwFunc = DEFW_IOTIMING_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidNets)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_IOTIMING_START) &&
        (defwState >= DEFW_IOTIMING_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum >= 5.4)
        return DEFW_OBSOLETE;

    fprintf(defwFile, "IOTIMINGS %d ;\n", count);
    defwLines++;

    defwCounter = count;
    defwState = DEFW_IOTIMING_START;
    return DEFW_OK;
}


int
defwIOTiming(const char *instance,
             const char *pin)
{
    defwFunc = DEFW_IOTIMING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_IOTIMING_START &&
        defwState != DEFW_IOTIMING)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_IOTIMING)
        fprintf(defwFile, " ;\n");   // from previous statement
    fprintf(defwFile, "   - ( %s %s )\n", instance, pin);
    defwLines++;

    defwCounter--;
    defwState = DEFW_IOTIMING;
    return DEFW_OK;
}

int
defwIOTimingVariable(const char *riseFall,
                     int        num1,
                     int        num2)
{
    defwFunc = DEFW_IOTIMING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_IOTIMING)
        return DEFW_BAD_ORDER;

    if (strcmp(riseFall, "RISE") &&
        strcmp(riseFall, "FALL"))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "      + %s VARIABLE %d %d\n", riseFall,
            num1, num2);
    defwLines++;

    return DEFW_OK;
}

int
defwIOTimingSlewrate(const char *riseFall,
                     int        num1,
                     int        num2)
{
    defwFunc = DEFW_IOTIMING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_IOTIMING)
        return DEFW_BAD_ORDER;

    if (strcmp(riseFall, "RISE") &&
        strcmp(riseFall, "FALL"))
        return DEFW_BAD_DATA;

    fprintf(defwFile, "      + %s SLEWRATE %d %d\n", riseFall,
            num1, num2);
    defwLines++;

    return DEFW_OK;
}

int
defwIOTimingDrivecell(const char    *name,
                      const char    *fromPin,
                      const char    *toPin,
                      int           numDrivers)
{
    defwFunc = DEFW_IOTIMING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_IOTIMING)
        return DEFW_BAD_ORDER;
    if (!name || !*name) // required
        return DEFW_BAD_DATA;

    fprintf(defwFile, "      + DRIVECELL %s ", name);
    if (fromPin && (!toPin || !*toPin)) // if have fromPin, toPin is required
        return DEFW_BAD_DATA;
    if (fromPin)
        fprintf(defwFile, "FROMPIN %s ", fromPin);
    if (toPin)
        fprintf(defwFile, "TOPIN %s ", toPin);
    if (numDrivers)
        fprintf(defwFile, "PARALLEL %d ", numDrivers);
    defwLines++;

    return DEFW_OK;
}


int
defwIOTimingCapacitance(double num)
{
    defwFunc = DEFW_IOTIMING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_IOTIMING)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "      + CAPACITANCE %.11g", num);
    defwLines++;

    return DEFW_OK;
}

int
defwEndIOTimings()
{
    defwFunc = DEFW_IOTIMING_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_IOTIMING_START && defwState != DEFW_IOTIMING)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    if (defwState == DEFW_IOTIMING)
        fprintf(defwFile, " ;\n");   // from previous statement
    fprintf(defwFile, "END IOTIMINGS\n\n");
    defwLines++;

    defwState = DEFW_IOTIMING_END;
    return DEFW_OK;
}


int
defwStartScanchains(int count)
{
    defwFunc = DEFW_SCANCHAIN_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidNets)
        return DEFW_BAD_ORDER;
    if ((defwState >= DEFW_SCANCHAIN_START) &&
        (defwState <= DEFW_SCANCHAIN_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "SCANCHAINS %d ;\n", count);
    defwLines++;

    defwState = DEFW_SCANCHAIN_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwScanchain(const char *name)
{
    defwFunc = DEFW_SCANCHAIN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_SCANCHAIN || defwState == DEFW_SCAN_FLOATING ||
        defwState == DEFW_SCAN_ORDERED) // put a ; for the previous scanchain
        fprintf(defwFile, " ;\n");

    fprintf(defwFile, "   - %s", name);
    defwLines++;

    defwCounter--;
    defwState = DEFW_SCANCHAIN;
    return DEFW_OK;
}

int
defwScanchainCommonscanpins(const char  *inst1,
                            const char  *pin1,
                            const char  *inst2,
                            const char  *pin2)
{
    defwFunc = DEFW_SCANCHAIN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!inst1) {     // if inst1 is null, nothing will be written
        defwState = DEFW_SCANCHAIN;
        return DEFW_OK;
    }

    if (inst1 && strcmp(inst1, "IN") && strcmp(inst1, "OUT")) // IN | OUT
        return DEFW_BAD_DATA;

    if (inst1 && !pin1)        // pin1 can't be NULL if inst1 is not
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + COMMONSCANPINS ( %s %s )", inst1, pin1);

    if (inst2 && !pin2)        // pin2 can't be NULL if inst2 is not
        return DEFW_BAD_DATA;

    if (inst2 && strcmp(inst2, "IN") && strcmp(inst2, "OUT")) // IN | OUT
        return DEFW_BAD_DATA;

    if (inst2)
        fprintf(defwFile, " ( %s %s )", inst2, pin2);

    defwLines++;

    defwState = DEFW_SCANCHAIN;
    return DEFW_OK;
}

int
defwScanchainPartition(const char   *name,
                       int          maxBits)
{
    defwFunc = DEFW_SCANCHAIN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!name || !*name)        // require
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + PARTITION %s", name);
    if (maxBits != -1)
        fprintf(defwFile, " MAXBITS %d", maxBits);
    defwLines++;

    defwState = DEFW_SCANCHAIN;
    return DEFW_OK;
}

int
defwScanchainStart(const char   *inst,
                   const char   *pin)
{
    defwFunc = DEFW_SCANCHAIN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!inst || !*inst)        // require
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + START %s", inst);
    if (pin)
        fprintf(defwFile, " %s", pin);
    defwLines++;

    defwState = DEFW_SCANCHAIN;
    return DEFW_OK;
}


int
defwScanchainStop(const char    *inst,
                  const char    *pin)
{
    defwFunc = DEFW_SCANCHAIN;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!inst || !*inst)        // require
        return DEFW_BAD_DATA;

    fprintf(defwFile, "\n      + STOP %s", inst);
    if (pin)
        fprintf(defwFile, " %s", pin);
    defwLines++;

    defwState = DEFW_SCANCHAIN;
    return DEFW_OK;
}

int
defwScanchainFloating(const char    *name,
                      const char    *inst1,
                      const char    *pin1,
                      const char    *inst2,
                      const char    *pin2)
{
    defwFunc = DEFW_SCAN_FLOATING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!name || !*name)        // require
        return DEFW_BAD_DATA;
    if (inst1 && strcmp(inst1, "IN") && strcmp(inst1, "OUT"))
        return DEFW_BAD_DATA;
    if (inst2 && strcmp(inst2, "IN") && strcmp(inst2, "OUT"))
        return DEFW_BAD_DATA;
    if (inst1 && !pin1)
        return DEFW_BAD_DATA;
    if (inst2 && !pin2)
        return DEFW_BAD_DATA;

    if (defwState != DEFW_SCAN_FLOATING)
        fprintf(defwFile, "\n      + FLOATING");
    else
        fprintf(defwFile, "\n         ");

    fprintf(defwFile, " %s", name);
    if (inst1)
        fprintf(defwFile, " ( %s %s )", inst1, pin1);
    if (inst2)
        fprintf(defwFile, " ( %s %s )", inst2, pin2);

    defwState = DEFW_SCAN_FLOATING;
    defwLines++;

    return DEFW_OK;
}

int
defwScanchainFloatingBits(const char    *name,
                          const char    *inst1,
                          const char    *pin1,
                          const char    *inst2,
                          const char    *pin2,
                          int           bits)
{
    defwFunc = DEFW_SCAN_FLOATING;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!name || !*name)        // require
        return DEFW_BAD_DATA;
    if (inst1 && strcmp(inst1, "IN") && strcmp(inst1, "OUT"))
        return DEFW_BAD_DATA;
    if (inst2 && strcmp(inst2, "IN") && strcmp(inst2, "OUT"))
        return DEFW_BAD_DATA;
    if (inst1 && !pin1)
        return DEFW_BAD_DATA;
    if (inst2 && !pin2)
        return DEFW_BAD_DATA;

    if (defwState != DEFW_SCAN_FLOATING)
        fprintf(defwFile, "\n      + FLOATING");
    else
        fprintf(defwFile, "\n         ");

    fprintf(defwFile, " %s", name);
    if (inst1)
        fprintf(defwFile, " ( %s %s )", inst1, pin1);
    if (inst2)
        fprintf(defwFile, " ( %s %s )", inst2, pin2);
    if (bits != -1)
        fprintf(defwFile, " ( BITS %d )", bits);

    defwState = DEFW_SCAN_FLOATING;
    defwLines++;

    return DEFW_OK;
}
int
defwScanchainOrdered(const char *name1,
                     const char *inst1,
                     const char *pin1,
                     const char *inst2,
                     const char *pin2,
                     const char *name2,
                     const char *inst3,
                     const char *pin3,
                     const char *inst4,
                     const char *pin4)
{
    defwFunc = DEFW_SCAN_ORDERED;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!name1 || !*name1)        // require
        return DEFW_BAD_DATA;
    if (inst1 && strcmp(inst1, "IN") && strcmp(inst1, "OUT"))
        return DEFW_BAD_DATA;
    if (inst2 && strcmp(inst2, "IN") && strcmp(inst2, "OUT"))
        return DEFW_BAD_DATA;
    if (inst1 && !pin1)
        return DEFW_BAD_DATA;
    if (inst2 && !pin2)
        return DEFW_BAD_DATA;
    if (defwState != DEFW_SCAN_ORDERED) {  // 1st time require both name1 & name2
        if (!name2 || !*name2)        // require
            return DEFW_BAD_DATA;
        if (inst3 && strcmp(inst3, "IN") && strcmp(inst3, "OUT"))
            return DEFW_BAD_DATA;
        if (inst4 && strcmp(inst4, "IN") && strcmp(inst4, "OUT"))
            return DEFW_BAD_DATA;
        if (inst3 && !pin3)
            return DEFW_BAD_DATA;
        if (inst4 && !pin4)
            return DEFW_BAD_DATA;
    }

    if (defwState != DEFW_SCAN_ORDERED)
        fprintf(defwFile, "\n      + ORDERED");
    else
        fprintf(defwFile, "\n         ");

    fprintf(defwFile, " %s", name1);
    if (inst1)
        fprintf(defwFile, " ( %s %s )", inst1, pin1);
    if (inst2)
        fprintf(defwFile, " ( %s %s )", inst2, pin2);
    defwLines++;

    if (name2) {
        fprintf(defwFile, "\n          %s", name2);
        if (inst3)
            fprintf(defwFile, " ( %s %s )", inst3, pin3);
        if (inst4)
            fprintf(defwFile, " ( %s %s )", inst4, pin4);
        defwLines++;
    }

    defwState = DEFW_SCAN_ORDERED;

    return DEFW_OK;
}

int
defwScanchainOrderedBits(const char *name1,
                         const char *inst1,
                         const char *pin1,
                         const char *inst2,
                         const char *pin2,
                         int        bits1,
                         const char *name2,
                         const char *inst3,
                         const char *pin3,
                         const char *inst4,
                         const char *pin4,
                         int        bits2)
{
    defwFunc = DEFW_SCAN_ORDERED;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCANCHAIN &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCAN_ORDERED)
        return DEFW_BAD_ORDER;

    if (!name1 || !*name1)        // require
        return DEFW_BAD_DATA;
    if (inst1 && strcmp(inst1, "IN") && strcmp(inst1, "OUT"))
        return DEFW_BAD_DATA;
    if (inst2 && strcmp(inst2, "IN") && strcmp(inst2, "OUT"))
        return DEFW_BAD_DATA;
    if (inst1 && !pin1)
        return DEFW_BAD_DATA;
    if (inst2 && !pin2)
        return DEFW_BAD_DATA;
    if (defwState != DEFW_SCAN_ORDERED) {  // 1st time require both name1 & name2
        if (!name2 || !*name2)        // require
            return DEFW_BAD_DATA;
        if (inst3 && strcmp(inst3, "IN") && strcmp(inst3, "OUT"))
            return DEFW_BAD_DATA;
        if (inst4 && strcmp(inst4, "IN") && strcmp(inst4, "OUT"))
            return DEFW_BAD_DATA;
        if (inst3 && !pin3)
            return DEFW_BAD_DATA;
        if (inst4 && !pin4)
            return DEFW_BAD_DATA;
    }

    if (defwState != DEFW_SCAN_ORDERED)
        fprintf(defwFile, "\n      + ORDERED");
    else
        fprintf(defwFile, "\n         ");

    fprintf(defwFile, " %s", name1);
    if (inst1)
        fprintf(defwFile, " ( %s %s )", inst1, pin1);
    if (inst2)
        fprintf(defwFile, " ( %s %s )", inst2, pin2);
    if (bits1 != -1)
        fprintf(defwFile, " ( BITS %d )", bits1);
    defwLines++;

    if (name2) {
        fprintf(defwFile, "\n          %s", name2);
        if (inst3)
            fprintf(defwFile, " ( %s %s )", inst3, pin3);
        if (inst4)
            fprintf(defwFile, " ( %s %s )", inst4, pin4);
        if (bits2 != -1)
            fprintf(defwFile, " ( BITS %d )", bits2);
        defwLines++;
    }

    defwState = DEFW_SCAN_ORDERED;

    return DEFW_OK;
}

int
defwEndScanchain()
{
    defwFunc = DEFW_SCANCHAIN_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SCANCHAIN_START && defwState != DEFW_SCAN_ORDERED &&
        defwState != DEFW_SCAN_FLOATING && defwState != DEFW_SCANCHAIN)
        return DEFW_BAD_ORDER;

    if (defwState != DEFW_SCANCHAIN_START)  // from previous statement
        fprintf(defwFile, " ;\n");

    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END SCANCHAINS\n\n");
    defwLines++;

    defwState = DEFW_SCANCHAIN_END;
    return DEFW_OK;
}

int
defwStartConstraints(int count)
{
    defwObsoleteNum = DEFW_FPC_START;
    defwFunc = DEFW_FPC_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_FPC_START) && (defwState <= DEFW_FPC_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum >= 5.4)
        return DEFW_OBSOLETE;

    fprintf(defwFile, "CONSTRAINTS %d ;\n", count);
    defwLines++;

    defwState = DEFW_FPC_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwConstraintOperand()
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_START && defwState != DEFW_FPC)
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "   -");
    defwCounter--;
    defwFPC = 0;
    defwState = DEFW_FPC_OPER;
    return DEFW_OK;
}

int
defwConstraintOperandNet(const char *netName)
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_OPER && defwState != DEFW_FPC_OPER_SUM)
        return DEFW_BAD_ORDER;          // net can be within SUM

    if (!netName || !*netName)        // require
        return DEFW_BAD_DATA;
    if (defwFPC > 0)
        fprintf(defwFile, " ,");
    if (defwState == DEFW_FPC_OPER_SUM)
        defwFPC++;
    fprintf(defwFile, " NET %s", netName);
    return DEFW_OK;
}

int
defwConstraintOperandPath(const char    *comp1,
                          const char    *fromPin,
                          const char    *comp2,
                          const char    *toPin)
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_OPER && defwState != DEFW_FPC_OPER_SUM)
        return DEFW_BAD_ORDER;          // path can be within SUM

    if ((comp1 == 0) || (*comp1 == 0) || (fromPin == 0) || (*fromPin == 0) ||
        (comp2 == 0) || (*comp2 == 0) || (toPin == 0) || (*toPin == 0)) // require
        return DEFW_BAD_DATA;
    if (defwFPC > 0)
        fprintf(defwFile, " ,");
    if (defwState == DEFW_FPC_OPER_SUM)
        defwFPC++;
    fprintf(defwFile, " PATH %s %s %s %s", comp1, fromPin, comp2, toPin);
    return DEFW_OK;
}

int
defwConstraintOperandSum()
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_OPER && defwState != DEFW_FPC_OPER_SUM)
        return DEFW_BAD_ORDER;          // sum can be within SUM

    fprintf(defwFile, " SUM (");
    defwState = DEFW_FPC_OPER_SUM;
    defwFPC = 0;
    return DEFW_OK;
}

int
defwConstraintOperandSumEnd()
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_OPER_SUM)
        return DEFW_BAD_ORDER;
    fprintf(defwFile, " )");
    defwState = DEFW_FPC_OPER;
    defwFPC = 0;
    return DEFW_OK;
}

int
defwConstraintOperandTime(const char    *timeType,
                          int           time)
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_OPER)
        return DEFW_BAD_ORDER;
    if (timeType && strcmp(timeType, "RISEMAX") && strcmp(timeType, "FALLMAX") &&
        strcmp(timeType, "RISEMIN") && strcmp(timeType, "FALLMIN"))
        return DEFW_BAD_DATA;
    fprintf(defwFile, " + %s %d", timeType, time);
    return DEFW_OK;
}

int
defwConstraintOperandEnd()
{
    defwFunc = DEFW_FPC_OPER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_OPER)
        return DEFW_BAD_ORDER;
    fprintf(defwFile, " ;\n");
    defwState = DEFW_FPC;
    return DEFW_OK;
}

int
defwConstraintWiredlogic(const char *netName,
                         int        distance)
{
    defwFunc = DEFW_FPC;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_START && defwState != DEFW_FPC)
        return DEFW_BAD_ORDER;

    if (!netName || !*netName)        // require
        return DEFW_BAD_DATA;
    fprintf(defwFile, "   - WIREDLOGIC %s MAXDIST %d ;\n", netName, distance);
    defwCounter--;
    defwState = DEFW_FPC;
    defwLines++;
    return DEFW_OK;
}

int
defwEndConstraints()
{
    defwFunc = DEFW_FPC_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FPC_START && defwState != DEFW_FPC)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END CONSTRAINTS\n\n");
    defwLines++;

    defwState = DEFW_FPC_END;
    return DEFW_OK;
}

int
defwStartGroups(int count)
{
    defwFunc = DEFW_GROUP_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_GROUP_START) && (defwState <= DEFW_GROUP_END))
        return DEFW_BAD_ORDER;

    fprintf(defwFile, "GROUPS %d ;\n", count);
    defwLines++;

    defwState = DEFW_GROUP_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwGroup(const char    *groupName,
          int           numExpr,
          const char    **groupExpr)
{
    int i;

    defwFunc = DEFW_GROUP;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_GROUP_START && defwState != DEFW_GROUP)
        return DEFW_BAD_ORDER;

    if ((groupName == 0) || (*groupName == 0) || (groupExpr == 0) ||
        (*groupExpr == 0))  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_GROUP)
        fprintf(defwFile, " ;\n");          // add ; for the previous group
    fprintf(defwFile, "   - %s", groupName);
    if (numExpr) {
        for (i = 0; i < numExpr; i++)
            fprintf(defwFile, " %s", groupExpr[i]);
    }
    defwCounter--;
    defwLines++;
    defwState = DEFW_GROUP;
    return DEFW_OK;
}


int
defwGroupSoft(const char    *type1,
              double        value1,
              const char    *type2,
              double        value2,
              const char    *type3,
              double        value3)
{
    defwFunc = DEFW_GROUP;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_GROUP)
        return DEFW_BAD_ORDER;

    if (type1 && strcmp(type1, "MAXHALFPERIMETER") && strcmp(type1, "MAXX") &&
        strcmp(type1, "MAXY"))
        return DEFW_BAD_DATA;
    if (type2 && strcmp(type2, "MAXHALFPERIMETER") && strcmp(type2, "MAXX") &&
        strcmp(type2, "MAXY"))
        return DEFW_BAD_DATA;
    if (type3 && strcmp(type3, "MAXHALFPERIMETER") && strcmp(type3, "MAXX") &&
        strcmp(type3, "MAXY"))
        return DEFW_BAD_DATA;
    if (type1)
        fprintf(defwFile, "\n     + SOFT %s %.11g", type1, value1);
    if (type2)
        fprintf(defwFile, " %s %.11g", type2, value2);
    if (type3)
        fprintf(defwFile, " %s %.11g", type3, value3);
    defwLines++;
    return DEFW_OK;
}

int
defwGroupRegion(int         xl,
                int         yl,
                int         xh,
                int         yh,
                const char  *regionName)
{
    defwFunc = DEFW_GROUP;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_GROUP)
        return DEFW_BAD_ORDER;

    if ((xl || yl || xh || yh) && (regionName))  // ether pts or regionName
        return DEFW_BAD_DATA;

    if (regionName)
        fprintf(defwFile, "\n      + REGION %s", regionName);
    else
        fprintf(defwFile, "\n      + REGION ( %d %d ) ( %d %d )",
                xl, yl, xh, yh);
    defwLines++;
    return DEFW_OK;
}

int
defwEndGroups()
{
    defwFunc = DEFW_GROUP_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_GROUP_START && defwState != DEFW_GROUP)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    if (defwState != DEFW_GROUP_START)
        fprintf(defwFile, " ;\n");

    fprintf(defwFile, "END GROUPS\n\n");
    defwLines++;

    defwState = DEFW_GROUP_END;
    return DEFW_OK;
}


int
defwStartBlockages(int count)
{
    defwFunc = DEFW_BLOCKAGE_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_BLOCKAGE_START) && (defwState <= DEFW_BLOCKAGE_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "BLOCKAGES %d ;\n", count);
    defwLines++;

    defwState = DEFW_BLOCKAGE_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwBlockagesLayer(const char *layerName)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_PLACE)
                                             || (defwState == DEFW_BLOCKAGE_LAYER)))
        return DEFW_BAD_ORDER;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle

    fprintf(defwFile, "   - LAYER %s", layerName);
    fprintf(defwFile, "\n");
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSD = 0;
    defwBlockageHasSF = 0;
    return DEFW_OK;
}

int
defwBlockagesLayerSlots()
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwBlockageHasSF)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "      + SLOTS\n");
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSF = 1;
    return DEFW_OK;
}

int
defwBlockagesLayerFills()
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwBlockageHasSF)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle

    fprintf(defwFile, "     + FILLS\n");
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSF = 1;
    return DEFW_OK;
}

int
defwBlockagesLayerComponent(const char *compName)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if ((compName == 0) || (*compName == 0))  // require
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "     + COMPONENT %s\n", compName);
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

int
defwBlockagesLayerPushdown()
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle

    fprintf(defwFile, "     + PUSHDOWN\n");
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

// 5.7
int
defwBlockagesLayerExceptpgnet()
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle

    fprintf(defwFile, "     + EXCEPTPGNET\n");
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

int
defwBlockagesLayerSpacing(int minSpacing)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;
    if (defwBlockageHasSD)    // Either spacing or designrulewidth has defined
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + SPACING %d\n", minSpacing);
    defwLines++;
    defwBlockageHasSD = 1;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

int
defwBlockagesLayerDesignRuleWidth(int effectiveWidth)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;
    if (defwBlockageHasSD)    // Either spacing or designrulewidth has defined
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + DESIGNRULEWIDTH %d\n", effectiveWidth);
    defwLines++;
    defwBlockageHasSD = 1;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

int
defwBlockagesLayerMask(int colorMask)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_BLOCKAGE_MASK;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     + MASK %d", colorMask);
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayer
int
defwBlockageLayer(const char    *layerName,
                  const char    *compName)
{      // optional(NULL) 
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define component or layer slots or fills
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_PLACE)
                                             || (defwState == DEFW_BLOCKAGE_LAYER)))
        return DEFW_BAD_DATA;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s ", layerName);
    if (compName && *compName != 0)  // optional
        fprintf(defwFile, "+ COMPONENT %s ", compName);
    fprintf(defwFile, "\n");
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSD = 0;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayerSlots
int
defwBlockageLayerSlots(const char *layerName)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define component or layer or layer fills
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_PLACE)
                                             || (defwState == DEFW_BLOCKAGE_LAYER)))
        return DEFW_BAD_DATA;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s + SLOTS\n", layerName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSD = 0;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayerFills
int
defwBlockageLayerFills(const char *layerName)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define component or layer or layer slots
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_PLACE)
                                             || (defwState == DEFW_BLOCKAGE_LAYER)))
        return DEFW_BAD_DATA;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s + FILLS\n", layerName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSD = 0;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayerPushdown
int
defwBlockageLayerPushdown(const char *layerName)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define component or layer or layer slots
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_PLACE)
                                             || (defwState == DEFW_BLOCKAGE_LAYER)))
        return DEFW_BAD_DATA;

    if ((layerName == 0) || (*layerName == 0))  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s + PUSHDOWN\n", layerName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSD = 0;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayerExceptpgnet
int
defwBlockageLayerExceptpgnet(const char *layerName)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define component or layer or layer slots
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_PLACE)
                                             || (defwState == DEFW_BLOCKAGE_LAYER)))
        return DEFW_BAD_DATA;

    if ((layerName == 0) || (*layerName == 0))  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s + EXCEPTPGNET\n", layerName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    defwBlockageHasSD = 0;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayerSpacing
int
defwBlockageSpacing(int minSpacing)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // Checked if defwBlockageDesignRuleWidth has already called
    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_DATA;
    if (defwBlockageHasSD)    // Either spacing or designrulewidth has defined
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + SPACING %d\n", minSpacing);
    defwLines++;
    defwBlockageHasSD = 1;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

//To be removed, replaced by defwBlockagesLayerDesignRuleWidth
int
defwBlockageDesignRuleWidth(int effectiveWidth)
{
    defwFunc = DEFW_BLOCKAGE_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // Checked if defwBlockageDesignRuleWidth has already called
    if ((defwState != DEFW_BLOCKAGE_LAYER) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_DATA;
    if (defwBlockageHasSD)    // Either spacing or designrulewidth has defined
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + DESIGNRULEWIDTH %d\n", effectiveWidth);
    defwLines++;
    defwBlockageHasSD = 1;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

int
defwBlockagesPlacement()
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_LAYER)
                                             || (defwState == DEFW_BLOCKAGE_PLACE)))
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle

    fprintf(defwFile, "   - PLACEMENT\n");
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    defwBlockageHasSP = 0;
    return DEFW_OK;
}


int
defwBlockagesPlacementComponent(const char *compName)
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_PLACE) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if ((compName == 0) || (*compName == 0))  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "     + COMPONENT %s\n", compName);
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}


int
defwBlockagesPlacementPushdown()
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_PLACE) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "     + PUSHDOWN\n");
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}

// 5.7
int
defwBlockagesPlacementSoft()
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_PLACE) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwBlockageHasSP)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "     + SOFT\n");
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    defwBlockageHasSP = 1;
    return DEFW_OK;
}

// 5.7
int
defwBlockagesPlacementPartial(double maxDensity)
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;

    if ((defwState != DEFW_BLOCKAGE_PLACE) && (defwState != DEFW_BLOCKAGE_RECT))
        return DEFW_BAD_ORDER;

    if (defwBlockageHasSP)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "     + PARTIAL %.11g\n", maxDensity);
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    defwBlockageHasSP = 1;
    return DEFW_OK;
}

int
defwBlockagesRect(int   xl,
                  int   yl,
                  int   xh,
                  int   yh)
{
    defwFunc = DEFW_BLOCKAGE_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_PLACE && defwState != DEFW_BLOCKAGE_LAYER &&
        defwState != DEFW_BLOCKAGE_RECT)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     RECT ( %d %d ) ( %d %d )", xl, yl, xh, yh);
    defwLines++;
    defwState = DEFW_BLOCKAGE_RECT;
    return DEFW_OK;
}


int
defwBlockagesPolygon(int    num_polys,
                     int    *xl,
                     int    *yl)
{
    int i;

    defwFunc = DEFW_BLOCKAGE_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_LAYER && defwState != DEFW_BLOCKAGE_RECT)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     POLYGON ");
    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) != 0))
            fprintf(defwFile, "( %d %d ) ", *xl++, *yl++);
        else {
            fprintf(defwFile, "\n             ( %d %d ) ", *xl++, *yl++);
            defwLines++;
        }
    }
    defwLines++;
    defwState = DEFW_BLOCKAGE_RECT;  // use rect flag.  It works the same for poly
    return DEFW_OK;
}

// To be removed. Will replace by defwBlcokagesPlacement
// bug fix: submitted by Craig Files (cfiles@ftc.agilent.com)
int
defwBlockagePlacement()
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_LAYER)
                                             || (defwState == DEFW_BLOCKAGE_PLACE)))
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - PLACEMENT\n");
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}

// To be removed. Will replace by defwBlcokagesPlacementComponent
int
defwBlockagePlacementComponent(const char *compName)
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_LAYER)
                                             || (defwState == DEFW_BLOCKAGE_PLACE)))
        return DEFW_BAD_DATA;

    if ((compName == 0) || (*compName == 0))  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - PLACEMENT + COMPONENT %s\n", compName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}

// To be removed. Will replace by defwBlcokagesPlacementPushdown
int
defwBlockagePlacementPushdown()
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_LAYER)
                                             || (defwState == DEFW_BLOCKAGE_PLACE)))
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - PLACEMENT + PUSHDOWN\n");
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}

// To be removed. Will replace by defwBlcokagesPlacementSoft
int
defwBlockagePlacementSoft()
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_LAYER)
                                             || (defwState == DEFW_BLOCKAGE_PLACE)))
        return DEFW_BAD_DATA;
    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - PLACEMENT + SOFT\n");
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}

// To be removed. Will replace by defwBlcokagesPlacementPartial
int
defwBlockagePlacementPartial(double maxDensity)
{
    defwFunc = DEFW_BLOCKAGE_PLACE;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_START && ((defwState == DEFW_BLOCKAGE_LAYER)
                                             || (defwState == DEFW_BLOCKAGE_PLACE)))
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - PLACEMENT + PARTIAL %.11g\n", maxDensity);
    defwCounter--;
    defwLines++;
    defwState = DEFW_BLOCKAGE_PLACE;
    return DEFW_OK;
}

// To be removed. Will replace by defwBlockagesLayerMask
int
defwBlockageMask(int colorMask)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_BLOCKAGE_MASK;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_PLACE && defwState != DEFW_BLOCKAGE_LAYER &&
        defwState != DEFW_BLOCKAGE_RECT)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     + MASK %d", colorMask);
    defwLines++;
    defwState = DEFW_BLOCKAGE_LAYER;
    return DEFW_OK;
}

// Tobe removed. Will be replaced by defwBlockagesRect.
int
defwBlockageRect(int    xl,
                 int    yl,
                 int    xh,
                 int    yh)
{
    defwFunc = DEFW_BLOCKAGE_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_PLACE && defwState != DEFW_BLOCKAGE_LAYER &&
        defwState != DEFW_BLOCKAGE_RECT && defwState != DEFW_BLOCKAGE_MASK)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     RECT ( %d %d ) ( %d %d )", xl, yl, xh, yh);
    defwLines++;
    defwState = DEFW_BLOCKAGE_RECT;
    return DEFW_OK;
}

// Tobe removed. Will be replaced by defwBlockagesPolygon.
int
defwBlockagePolygon(int num_polys,
                    int *xl,
                    int *yl)
{
    int i;

    defwFunc = DEFW_BLOCKAGE_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_BLOCKAGE_LAYER && defwState != DEFW_BLOCKAGE_RECT
        && defwState != DEFW_BLOCKAGE_MASK)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_BLOCKAGE_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     POLYGON ");
    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) != 0))
            fprintf(defwFile, "( %d %d ) ", *xl++, *yl++);
        else {
            fprintf(defwFile, "\n             ( %d %d ) ", *xl++, *yl++);
            defwLines++;
        }
    }
    defwLines++;
    defwState = DEFW_BLOCKAGE_RECT;  // use rect flag.  It works the same for poly
    return DEFW_OK;
}


int
defwEndBlockages()
{
    defwFunc = DEFW_BLOCKAGE_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_BLOCKAGE_RECT)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, " ;\n");

    fprintf(defwFile, "END BLOCKAGES\n\n");
    defwLines++;

    defwState = DEFW_BLOCKAGE_END;
    return DEFW_OK;
}


int
defwStartSlots(int count)
{
    defwFunc = DEFW_SLOT_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_SLOT_START) && (defwState <= DEFW_SLOT_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "SLOTS %d ;\n", count);
    defwLines++;

    defwState = DEFW_SLOT_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwSlotLayer(const char *layerName)
{
    defwFunc = DEFW_SLOT_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_SLOT_START && defwState == DEFW_SLOT_LAYER)
        return DEFW_BAD_DATA;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_SLOT_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s \n", layerName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_SLOT_LAYER;
    return DEFW_OK;
}


int
defwSlotRect(int    xl,
             int    yl,
             int    xh,
             int    yh)
{
    defwFunc = DEFW_SLOT_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_SLOT_LAYER && defwState != DEFW_SLOT_RECT)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_SLOT_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     RECT ( %d %d ) ( %d %d )", xl, yl, xh, yh);
    defwLines++;
    defwState = DEFW_SLOT_RECT;
    return DEFW_OK;
}


int
defwSlotPolygon(int     num_polys,
                double  *xl,
                double  *yl)
{
    int i;

    defwFunc = DEFW_SLOT_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_SLOT_LAYER && defwState != DEFW_SLOT_RECT)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_SLOT_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     POLYGON ");

    printPointsNum = 0;
    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) != 0))
            printPoints(defwFile, *xl++, *yl++, "", " ");
        else {
            printPoints(defwFile, *xl++, *yl++, "\n             ", " ");
            defwLines++;
        }
    }
    defwLines++;
    defwState = DEFW_SLOT_RECT;
    return DEFW_OK;
}


int
defwEndSlots()
{
    defwFunc = DEFW_SLOT_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_SLOT_RECT)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, " ;\n");

    fprintf(defwFile, "END SLOTS\n\n");
    defwLines++;

    defwState = DEFW_SLOT_END;
    return DEFW_OK;
}


int
defwStartFills(int count)
{
    defwFunc = DEFW_FILL_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_FILL_START) && (defwState <= DEFW_FILL_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.4)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "FILLS %d ;\n", count);
    defwLines++;

    defwState = DEFW_FILL_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwFillLayer(const char *layerName)
{
    defwFunc = DEFW_FILL_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_START && defwState == DEFW_FILL_LAYER)
        return DEFW_BAD_DATA;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_FILL_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - LAYER %s \n", layerName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_FILL_LAYER;
    return DEFW_OK;
}

int
defwFillLayerMask(int colorMask)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_FILL_LAYERMASK;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_LAYER)
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + MASK %d", colorMask);
    defwLines++;
    defwState = DEFW_FILL_LAYERMASK;
    return DEFW_OK;
}


// 5.71
int
defwFillLayerOPC()
{
    defwFunc = DEFW_FILL_OPC;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_LAYER && defwState != DEFW_FILL_LAYERMASK)
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + OPC");
    defwLines++;
    defwState = DEFW_FILL_OPC;
    return DEFW_OK;
}

int
defwFillRect(int    xl,
             int    yl,
             int    xh,
             int    yh)
{
    defwFunc = DEFW_FILL_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_LAYER && defwState != DEFW_FILL_RECT &&
        defwState != DEFW_FILL_OPC && defwState != DEFW_FILL_LAYERMASK)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_FILL_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     RECT ( %d %d ) ( %d %d )", xl, yl, xh, yh);
    defwLines++;
    defwState = DEFW_FILL_RECT;
    return DEFW_OK;
}


int
defwFillPolygon(int     num_polys,
                double  *xl,
                double  *yl)
{
    int i;

    defwFunc = DEFW_FILL_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_LAYER && defwState != DEFW_FILL_RECT &&
        defwState != DEFW_FILL_OPC && defwState != DEFW_FILL_LAYERMASK)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_FILL_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     POLYGON ");
    printPointsNum = 0;
    for (i = 0; i < num_polys; i++) {
        if ((i == 0) || ((i % 5) != 0))
            printPoints(defwFile, *xl++, *yl++, "", " ");
        else {
            printPoints(defwFile, *xl++, *yl++,  "\n             ", " ");
            defwLines++;
        }
    }
    defwLines++;
    defwState = DEFW_FILL_RECT;
    return DEFW_OK;
}

// 5.7
int
defwFillVia(const char *viaName)
{
    defwFunc = DEFW_FILL_LAYER;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_START && defwState == DEFW_FILL_LAYER)
        return DEFW_BAD_DATA;

    if (!viaName || !*viaName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_FILL_RECT)
        fprintf(defwFile, " ;\n");   // end the previous rectangle
    fprintf(defwFile, "   - VIA %s \n", viaName);
    defwCounter--;
    defwLines++;
    defwState = DEFW_FILL_VIA;
    return DEFW_OK;
}

int
defwFillViaMask(int maskColor)
{
    if (defVersionNum < 5.8) {
        return DEFW_WRONG_VERSION;
    }

    defwFunc = DEFW_FILL_VIAMASK;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_VIA)
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + MASK %d", maskColor);
    defwLines++;
    defwState = DEFW_FILL_VIAMASK;
    return DEFW_OK;
}


// 5.71
int
defwFillViaOPC()
{
    defwFunc = DEFW_FILL_OPC;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_VIA && defwState != DEFW_FILL_VIAMASK)
        return DEFW_BAD_DATA;

    fprintf(defwFile, "     + OPC");
    defwLines++;
    defwState = DEFW_FILL_OPC;
    return DEFW_OK;
}


int
defwFillPoints(int      num_points,
               double   *xl,
               double   *yl)
{
    int i;

    defwFunc = DEFW_FILL_RECT;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_FILL_VIA && defwState != DEFW_FILL_RECT &&
        defwState != DEFW_FILL_OPC && defwState != DEFW_FILL_VIAMASK)
        return DEFW_BAD_DATA;

    if (defwState == DEFW_FILL_RECT)
        fprintf(defwFile, "\n");   // set a newline for the previous rectangle

    fprintf(defwFile, "     ");
    printPointsNum = 0;

    for (i = 0; i < num_points; i++) {
        if ((i == 0) || ((i % 5) != 0))
            printPoints(defwFile, *xl++, *yl++, "", " ");
        else {
            printPoints(defwFile, *xl++, *yl++, "\n             ", " ");
            defwLines++;
        }
    }

    defwLines++;
    defwState = DEFW_FILL_RECT;
    return DEFW_OK;
}


int
defwEndFills()
{
    defwFunc = DEFW_FILL_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_FILL_RECT && defwState != DEFW_FILL_OPC)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, " ;\n");

    fprintf(defwFile, "END FILLS\n\n");
    defwLines++;

    defwState = DEFW_FILL_END;
    return DEFW_OK;
}


int
defwStartNonDefaultRules(int count)
{
    defwFunc = DEFW_NDR_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_NDR_START) && (defwState <= DEFW_NDR_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "NONDEFAULTRULES %d ;\n", count);
    defwLines++;

    defwState = DEFW_NDR_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwNonDefaultRule(const char   *ruleName,
                   int          hardSpacing)
{
    defwFunc = DEFW_NDR;
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_NDR_START && defwState != DEFW_NDR)
        return DEFW_BAD_ORDER;

    if (!ruleName || !*ruleName)  // require
        return DEFW_BAD_DATA;
    if (defwState == DEFW_NDR)
        fprintf(defwFile, ";\n");
    fprintf(defwFile, "   - %s", ruleName);
    if (hardSpacing)
        fprintf(defwFile, "\n      + HARDSPACING");
    defwCounter--;
    defwLines++;
    defwState = DEFW_NDR;
    return DEFW_OK;
}


int
defwNonDefaultRuleLayer(const char  *layerName,
                        int         width,
                        int         diagWidth,
                        int         spacing,
                        int         wireExt)
{
    defwFunc = DEFW_NDR;
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_NDR)
        return DEFW_BAD_ORDER;

    if (!layerName || !*layerName)  // require
        return DEFW_BAD_DATA;
    fprintf(defwFile, "\n      + LAYER %s ", layerName);
    fprintf(defwFile, " WIDTH %d ", width);
    if (diagWidth)
        fprintf(defwFile, " DIAGWIDTH %d ", diagWidth);
    if (spacing)
        fprintf(defwFile, " SPACING %d ", spacing);
    if (wireExt)
        fprintf(defwFile, " WIREEXT %d ", wireExt);
    defwLines++;
    defwState = DEFW_NDR;
    return DEFW_OK;
}

int
defwNonDefaultRuleVia(const char *viaName)
{
    defwFunc = DEFW_NDR;
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_NDR)
        return DEFW_BAD_ORDER;

    if (!viaName || !*viaName)  // require
        return DEFW_BAD_DATA;
    fprintf(defwFile, "\n      + VIA %s ", viaName);
    defwLines++;
    defwState = DEFW_NDR;
    return DEFW_OK;
}

int
defwNonDefaultRuleViaRule(const char *viaRuleName)
{
    defwFunc = DEFW_NDR;
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_NDR)
        return DEFW_BAD_ORDER;

    if ((viaRuleName == 0) || (*viaRuleName == 0))  // require
        return DEFW_BAD_DATA;
    fprintf(defwFile, "\n      + VIARULE %s ", viaRuleName);
    defwLines++;
    defwState = DEFW_NDR;
    return DEFW_OK;
}

int
defwNonDefaultRuleMinCuts(const char    *cutLayerName,
                          int           numCuts)
{
    defwFunc = DEFW_NDR;
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_NDR)
        return DEFW_BAD_ORDER;

    if ((cutLayerName == 0) || (*cutLayerName == 0))  // require
        return DEFW_BAD_DATA;
    fprintf(defwFile, "\n      + MINCUTS %s %d ", cutLayerName, numCuts);
    defwLines++;
    defwState = DEFW_NDR;
    return DEFW_OK;
}

int
defwEndNonDefaultRules()
{
    defwFunc = DEFW_NDR_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_NDR)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, ";\nEND NONDEFAULTRULES\n\n");
    defwLines++;

    defwState = DEFW_NDR_END;
    return DEFW_OK;
}


int
defwStartStyles(int count)
{
    defwFunc = DEFW_STYLES_START;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if ((defwState >= DEFW_STYLES_START) && (defwState <= DEFW_STYLES_END))
        return DEFW_BAD_ORDER;
    if (defVersionNum < 5.6)
        return DEFW_WRONG_VERSION;

    fprintf(defwFile, "STYLES %d ;\n", count);
    defwLines++;

    defwState = DEFW_STYLES_START;
    defwCounter = count;
    return DEFW_OK;
}

int
defwStyles(int      styleNums,
           int      num_points,
           double   *xp,
           double   *yp)
{
    int i;

    defwFunc = DEFW_STYLES;
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    // May be user already define layer
    if (defwState != DEFW_STYLES_START && defwState != DEFW_STYLES)
        return DEFW_BAD_ORDER;

    if (styleNums < 0)  // require
        return DEFW_BAD_DATA;
    fprintf(defwFile, "   - STYLE %d ", styleNums);

    printPointsNum = 0;
    for (i = 0; i < num_points; i++) {
        if ((i == 0) || ((i % 5) != 0))
            printPoints(defwFile, *xp++, *yp++, "", " ");
        else {
            printPoints(defwFile, *xp++, *yp++, "\n       ", " ");
            defwLines++;
        }
    }

    defwCounter--;
    defwLines++;
    fprintf(defwFile, ";\n");
    defwState = DEFW_STYLES;
    return DEFW_OK;
}

int
defwEndStyles()
{
    defwFunc = DEFW_STYLES_END;   // Current function of writer
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (defwState != DEFW_STYLES)
        return DEFW_BAD_ORDER;
    if (defwCounter > 0)
        return DEFW_BAD_DATA;
    else if (defwCounter < 0)
        return DEFW_TOO_MANY_STMS;

    fprintf(defwFile, "END STYLES\n\n");
    defwLines++;

    defwState = DEFW_STYLES_END;
    return DEFW_OK;
}


int
defwStartBeginext(const char *name)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState == DEFW_BEGINEXT_START ||
        defwState == DEFW_BEGINEXT)
        return DEFW_BAD_ORDER;
    if (!name || name == 0 || *name == 0)
        return DEFW_BAD_DATA;
    fprintf(defwFile, "BEGINEXT \"%s\"\n", name);

    defwState = DEFW_BEGINEXT_START;
    defwLines++;
    return DEFW_OK;
}

int
defwBeginextCreator(const char *creatorName)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState != DEFW_BEGINEXT_START &&
        defwState != DEFW_BEGINEXT)
        return DEFW_BAD_ORDER;
    if (!creatorName || creatorName == 0 || *creatorName == 0)
        return DEFW_BAD_DATA;
    fprintf(defwFile, "   CREATOR \"%s\"\n", creatorName);

    defwState = DEFW_BEGINEXT;
    defwLines++;
    return DEFW_OK;
}


int
defwBeginextDate()
{
    time_t  todayTime;
    char    *rettime;

    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState != DEFW_BEGINEXT_START &&
        defwState != DEFW_BEGINEXT)
        return DEFW_BAD_ORDER;

    todayTime = time(NULL);             // time in UTC 
    rettime = ctime(&todayTime);        // convert to string
    rettime[strlen(rettime) - 1] = '\0';  // replace \n with \0
    fprintf(defwFile, "   DATE \"%s\"", rettime);

    defwState = DEFW_BEGINEXT;
    defwLines++;
    return DEFW_OK;
}


int
defwBeginextRevision(int    vers1,
                     int    vers2)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState != DEFW_BEGINEXT_START &&
        defwState != DEFW_BEGINEXT)
        return DEFW_BAD_ORDER;
    fprintf(defwFile, "\n   REVISION %d.%d", vers1, vers2);

    defwState = DEFW_BEGINEXT;
    defwLines++;
    return DEFW_OK;
}


int
defwBeginextSyntax(const char   *title,
                   const char   *string)
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState != DEFW_BEGINEXT_START &&
        defwState != DEFW_BEGINEXT)
        return DEFW_BAD_ORDER;
    fprintf(defwFile, "\n   - %s %s", title, string);

    defwState = DEFW_BEGINEXT;
    defwLines++;
    return DEFW_OK;
}


int
defwEndBeginext()
{
    if (!defwFile)
        return DEFW_UNINITIALIZED;
    if (!defwDidInit)
        return DEFW_BAD_ORDER;
    if (defwState != DEFW_BEGINEXT_START &&
        defwState != DEFW_BEGINEXT)
        return DEFW_BAD_ORDER;
    fprintf(defwFile, ";\nENDEXT\n\n");

    defwState = DEFW_BEGINEXT_END;
    defwLines++;
    return DEFW_OK;
}


int
defwEnd()
{
    defwFunc = DEFW_END;   // Current function of writer
    if (!defwFile)
        return 1;

    if (defwState == DEFW_ROW)
        fprintf(defwFile, ";\n\n");  // add the ; and \n for the previous row.

    fprintf(defwFile, "END DESIGN\n\n");
    defwLines++;
    //defwFile = 0;
    defwState = DEFW_DONE;
    return DEFW_OK;
}


int
defwCurrentLineNumber()
{
    return defwLines;
}


void
defwPrintError(int status)
{
    switch (status) {
    case DEFW_OK:
        fprintf(defwFile, "No Error.\n");
        break;
    case DEFW_UNINITIALIZED:
        printf("Need to call defwInit first.\n");
        break;
    case DEFW_BAD_ORDER:
        fprintf(defwFile, "%s - Incorrect order of data.\n",
                defwStateStr[defwFunc]);
        break;
    case DEFW_BAD_DATA:
        fprintf(defwFile, "%s - Invalid data.\n",
                defwStateStr[defwFunc]);
        break;
    case DEFW_ALREADY_DEFINED:
        fprintf(defwFile, "%s - Section is allowed to define only once.\n",
                defwStateStr[defwFunc]);
        break;
    case DEFW_WRONG_VERSION:
        fprintf(defwFile, "%s - Version number is set before 5.6, but 5.6 API is used.\n",
                defwStateStr[defwFunc]);
        break;
    case DEFW_OBSOLETE:
        fprintf(defwFile, "%s - is no longer valid in 5.6.\n",
                defwStateStr[defwObsoleteNum]);
        break;
    }
    return;
}


void
defwAddComment(const char *comment)
{
    if (comment)
        fprintf(defwFile, "# %s\n", comment);
    return;
}


void
defwAddIndent()
{
    fprintf(defwFile, "   ");
    return;
}


//***************************
//   Questions:
// - Is only one row rule allowed
// - Is only one tracks rule allowed
// - In the die area is a zero area allowed? overlaps?
// - What type of checking is needed for the rows and tracks do loop?
// - Can you have a default prop with a number AND a range?
// - What is the pin properties section mentioned in the 5.1 spec?
// *****************************

END_LEFDEF_PARSER_NAMESPACE

