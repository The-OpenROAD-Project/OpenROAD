// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2014, Cadence Design Systems
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
//  $Author: arakhman $
//  $Revision: #6 $
//  $Date: 2013/08/09 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************
#include "defrSettings.hpp"
#include "def.tab.h"

using namespace std;

BEGIN_LEFDEF_PARSER_NAMESPACE

defrSettings *defSettings = NULL;

defrSettings::defrSettings()
: defiDeltaNumberLines(10000),
  AssertionWarnings(999),
  BlockageWarnings(999),
  CaseSensitiveWarnings(999),
  ComponentWarnings(999),
  ConstraintWarnings(999),
  DefaultCapWarnings(999),
  FillWarnings(999),
  GcellGridWarnings(999),
  IOTimingWarnings(999),
  LogFileAppend(0),
  NetWarnings(999),
  NonDefaultWarnings(999),
  PinExtWarnings(999),
  PinWarnings(999),
  RegionWarnings(999),
  RowWarnings(999),
  TrackWarnings(999),
  ScanchainWarnings(999),
  SNetWarnings(999),
  StylesWarnings(999),
  UnitsWarnings(999),
  VersionWarnings(999),
  ViaWarnings(999),
  nDDMsgs(0),
  disableDMsgs(NULL),
  totalDefMsgLimit(0),
  AddPathToNet(0),
  AllowComponentNets(0),
  CommentChar('#'),
  DisPropStrProcess(0),
  reader_case_sensitive_set(0),
  ReadFunction(NULL),
  ErrorLogFunction(NULL),
  WarningLogFunction(NULL),
  ContextErrorLogFunction(NULL),
  ContextWarningLogFunction(NULL),
  MagicCommentFoundFunction(NULL),
  MallocFunction(NULL),
  ReallocFunction(NULL),
  FreeFunction(NULL),
  LineNumberFunction(NULL),
  LongLineNumberFunction(NULL),
  ContextLineNumberFunction(NULL),
  ContextLongLineNumberFunction(NULL)
{
    memset(MsgLimit, 0, DEF_MSGS * sizeof(int));
    memset(UnusedCallbacks, 0, CBMAX * sizeof(int));

    init_symbol_table();
}


void 
defrSettings::init_symbol_table()
{
    Keyword_set["ALIGN"] = K_ALIGN;
    Keyword_set["ANALOG"] = K_ANALOG;
    Keyword_set["ANTENNAMODEL"] = K_ANTENNAMODEL;
    Keyword_set["ANTENNAPINGATEAREA"] = K_ANTENNAPINGATEAREA;
    Keyword_set["ANTENNAPINDIFFAREA"] = K_ANTENNAPINDIFFAREA;
    Keyword_set["ANTENNAPINMAXAREACAR"] = K_ANTENNAPINMAXAREACAR;
    Keyword_set["ANTENNAPINMAXCUTCAR"] = K_ANTENNAPINMAXCUTCAR;
    Keyword_set["ANTENNAPINMAXSIDEAREACAR"] = K_ANTENNAPINMAXSIDEAREACAR;
    Keyword_set["ANTENNAPINPARTIALCUTAREA"] = K_ANTENNAPINPARTIALCUTAREA;
    Keyword_set["ANTENNAPINPARTIALMETALAREA"] = K_ANTENNAPINPARTIALMETALAREA;
    Keyword_set["ANTENNAPINPARTIALMETALSIDEAREA"] = K_ANTENNAPINPARTIALMETALSIDEAREA;
    Keyword_set["ARRAY"] = K_ARRAY;
    Keyword_set["ASSERTIONS"] = K_ASSERTIONS;
    Keyword_set["BALANCED"] = K_BALANCED;
    Keyword_set["BEGINEXT"] = K_BEGINEXT;
    Keyword_set["BLOCKAGES"] = K_BLOCKAGES;
    Keyword_set["BLOCKAGEWIRE"] = K_BLOCKAGEWIRE;
    Keyword_set["BLOCKRING"] = K_BLOCKRING;
    Keyword_set["BLOCKWIRE"] = K_BLOCKWIRE;
    Keyword_set["BOTTOMLEFT"] = K_BOTTOMLEFT;
    Keyword_set["BUSBITCHARS"] = K_BUSBITCHARS;
    Keyword_set["BY"] = K_BY;
    Keyword_set["CANNOTOCCUPY"] = K_CANNOTOCCUPY;
    Keyword_set["CANPLACE"] = K_CANPLACE;
    Keyword_set["CAPACITANCE"] = K_CAPACITANCE;
    Keyword_set["CLOCK"] = K_CLOCK;
    Keyword_set["COMMONSCANPINS"] = K_COMMONSCANPINS;
    Keyword_set["COMPONENT"] = K_COMPONENT;
    Keyword_set["COMPONENTPIN"] = K_COMPONENTPIN;
    Keyword_set["COMPONENTS"] = K_COMPS;
    Keyword_set["COMPONENTMASKSHIFT"] = K_COMPSMASKSHIFT;
    Keyword_set["CONSTRAINTS"] = K_CONSTRAINTS;
    Keyword_set["COREWIRE"] = K_COREWIRE;
    Keyword_set["COVER"] = K_COVER;
    Keyword_set["CUTSIZE"] = K_CUTSIZE;
    Keyword_set["CUTSPACING"] = K_CUTSPACING;
    Keyword_set["DEFAULTCAP"] = K_DEFAULTCAP;
    Keyword_set["DESIGN"] = K_DESIGN;
    Keyword_set["DESIGNRULEWIDTH"] = K_DESIGNRULEWIDTH;
    Keyword_set["DIAGWIDTH"] = K_DIAGWIDTH;
    Keyword_set["DIEAREA"] = K_DIEAREA;
    Keyword_set["DIFF"] = K_DIFF;
    Keyword_set["DIRECTION"] = K_DIRECTION;
    Keyword_set["DIST"] = K_DIST;
    Keyword_set["DISTANCE"] = K_DISTANCE;
    Keyword_set["DIVIDERCHAR"] = K_DIVIDERCHAR;
    Keyword_set["DO"] = K_DO;
    Keyword_set["DRCFILL"] = K_DRCFILL;
    Keyword_set["DRIVECELL"] = K_DRIVECELL;
    Keyword_set["E"] = K_E;
    Keyword_set["EEQMASTER"] = K_EEQMASTER;
    Keyword_set["ENCLOSURE"] = K_ENCLOSURE;
    Keyword_set["END"] = K_END;
    Keyword_set["ENDEXT"] = K_ENDEXT;
    Keyword_set["EQUAL"] = K_EQUAL;
    Keyword_set["EXCEPTPGNET"] = K_EXCEPTPGNET;
    Keyword_set["ESTCAP"] = K_ESTCAP;
    Keyword_set["FALL"] = K_FALL;
    Keyword_set["FALLMAX"] = K_FALLMAX;
    Keyword_set["FALLMIN"] = K_FALLMIN;
    Keyword_set["FE"] = K_FE;
    Keyword_set["FENCE"] = K_FENCE;
    Keyword_set["FILLS"] = K_FILLS;
    Keyword_set["FILLWIRE"] = K_FILLWIRE;
    Keyword_set["FILLWIREOPC"] = K_FILLWIREOPC;
    Keyword_set["FIXED"] = K_FIXED;
    Keyword_set["FIXEDBUMP"] = K_FIXEDBUMP;
    Keyword_set["FLOATING"] = K_FLOATING;
    Keyword_set["FLOORPLANCONSTRAINTS"] = K_FPC;
    Keyword_set["FN"] = K_FN;
    Keyword_set["FOLLOWPIN"] = K_FOLLOWPIN;
    Keyword_set["FOREIGN"] = K_FOREIGN;
    Keyword_set["FREQUENCY"] = K_FREQUENCY;
    Keyword_set["FROMCLOCKPIN"] = K_FROMCLOCKPIN;
    Keyword_set["FROMCOMPPIN"] = K_FROMCOMPPIN;
    Keyword_set["FROMIOPIN"] = K_FROMIOPIN;
    Keyword_set["FROMPIN"] = K_FROMPIN;
    Keyword_set["FS"] = K_FS;
    Keyword_set["FW"] = K_FW;
    Keyword_set["GCELLGRID"] = K_GCELLGRID;
    Keyword_set["GENERATE"] = K_COMP_GEN;
    Keyword_set["GUIDE"] = K_GUIDE;
    Keyword_set["GROUND"] = K_GROUND;
    Keyword_set["GROUNDSENSITIVITY"] = K_GROUNDSENSITIVITY;
    Keyword_set["GROUP"] = K_GROUP;
    Keyword_set["GROUPS"] = K_GROUPS;
    Keyword_set["FLOORPLAN"] = K_FLOORPLAN;
    Keyword_set["HALO"] = K_HALO;
    Keyword_set["HARDSPACING"] = K_HARDSPACING;
    Keyword_set["HISTORY"] = K_HISTORY;
    Keyword_set["HOLDRISE"] = K_HOLDRISE;
    Keyword_set["HOLDFALL"] = K_HOLDFALL;
    Keyword_set["HORIZONTAL"] = K_HORIZONTAL;
    Keyword_set["IN"] = K_IN;
    Keyword_set["INTEGER"] = K_INTEGER;
    Keyword_set["IOTIMINGS"] = K_IOTIMINGS;
    Keyword_set["IOWIRE"] = K_IOWIRE;
    Keyword_set["LAYER"] = K_LAYER;
    Keyword_set["LAYERS"] = K_LAYERS;
    Keyword_set["MASK"] = K_MASK;
    Keyword_set["MASKSHIFT"] = K_MASKSHIFT;
    Keyword_set["MAX"] = K_MAX;
    Keyword_set["MAXBITS"] = K_MAXBITS;
    Keyword_set["MAXDIST"] = K_MAXDIST;
    Keyword_set["MAXHALFPERIMETER"] = K_MAXHALFPERIMETER;
    Keyword_set["MAXX"] = K_MAXX;
    Keyword_set["MAXY"] = K_MAXY;
    Keyword_set["MICRONS"] = K_MICRONS;
    Keyword_set["MIN"] = K_MIN;
    Keyword_set["MINCUTS"] = K_MINCUTS;
    Keyword_set["MINPINS"] = K_MINPINS;
    Keyword_set["MUSTJOIN"] = K_MUSTJOIN;
    Keyword_set["N"] = K_N;
    Keyword_set["NAMESCASESENSITIVE"] = K_NAMESCASESENSITIVE;
    Keyword_set["NAMEMAPSTRING"] = K_NAMEMAPSTRING;
    Keyword_set["NET"] = K_NET;
    Keyword_set["NETEXPR"] = K_NETEXPR;
    Keyword_set["NETS"] = K_NETS;
    Keyword_set["NETLIST"] = K_NETLIST;
    Keyword_set["NEW"] = K_NEW;
    Keyword_set["NONDEFAULTRULE"] = K_NONDEFAULTRULE;
    Keyword_set["NONDEFAULTRULES"] = K_NONDEFAULTRULES;
    Keyword_set["NOSHIELD"] = K_NOSHIELD;
    Keyword_set["ON"] = K_ON;
    Keyword_set["OFF"] = K_OFF;
    Keyword_set["OFFSET"] = K_OFFSET;
    Keyword_set["OPC"] = K_OPC;
    Keyword_set["ORDERED"] = K_ORDERED;
    Keyword_set["ORIGIN"] = K_ORIGIN;
    Keyword_set["ORIGINAL"] = K_ORIGINAL;
    Keyword_set["OUT"] = K_OUT;
    Keyword_set["OXIDE1"] = K_OXIDE1;
    Keyword_set["OXIDE2"] = K_OXIDE2;
    Keyword_set["OXIDE3"] = K_OXIDE3;
    Keyword_set["OXIDE4"] = K_OXIDE4;
    Keyword_set["PADRING"] = K_PADRING;
    Keyword_set["PARTIAL"] = K_PARTIAL;
    Keyword_set["PARTITION"] = K_PARTITION;
    Keyword_set["PARALLEL"] = K_PARALLEL;
    Keyword_set["PARTITIONS"] = K_PARTITIONS;
    Keyword_set["PATH"] = K_PATH;
    Keyword_set["PATTERN"] = K_PATTERN;
    Keyword_set["PATTERNNAME"] = K_PATTERNNAME;
    Keyword_set["PIN"] = K_PIN;
    Keyword_set["PINPROPERTIES"] = K_PINPROPERTIES;
    Keyword_set["PINS"] = K_PINS;
    Keyword_set["PLACED"] = K_PLACED;
    Keyword_set["PLACEMENT"] = K_PLACEMENT;
    Keyword_set["POLYGON"] = K_POLYGON;
    Keyword_set["PORT"] = K_PORT;
    Keyword_set["POWER"] = K_POWER;
    Keyword_set["PROPERTY"] = K_PROPERTY;
    Keyword_set["PROPERTYDEFINITIONS"] = K_PROPERTYDEFINITIONS;
    Keyword_set["PUSHDOWN"] = K_PUSHDOWN;
    Keyword_set["RANGE"] = K_RANGE;
    Keyword_set["REAL"] = K_REAL;
    Keyword_set["RECT"] = K_RECT;
    Keyword_set["REENTRANTPATHS"] = K_REENTRANTPATHS;
    Keyword_set["REGION"] = K_REGION;
    Keyword_set["REGIONS"] = K_REGIONS;
    Keyword_set["RESET"] = K_RESET;
    Keyword_set["RING"] = K_RING;
    Keyword_set["RISE"] = K_RISE;
    Keyword_set["RISEMAX"] = K_RISEMAX;
    Keyword_set["RISEMIN"] = K_RISEMIN;
    Keyword_set["ROUTED"] = K_ROUTED;
    Keyword_set["ROUTEHALO"] = K_ROUTEHALO;
    Keyword_set["ROW"] = K_ROW;
    Keyword_set["ROWCOL"] = K_ROWCOL;
    Keyword_set["ROWS"] = K_ROWS;
    Keyword_set["S"] = K_S;
    Keyword_set["SAMEMASK"] = K_SAMEMASK;
    Keyword_set["SCAN"] = K_SCAN;
    Keyword_set["SCANCHAINS"] = K_SCANCHAINS;
    Keyword_set["SETUPFALL"] = K_SETUPFALL;
    Keyword_set["SETUPRISE"] = K_SETUPRISE;
    Keyword_set["SHAPE"] = K_SHAPE;
    Keyword_set["SHIELD"] = K_SHIELD;
    Keyword_set["SHIELDNET"] = K_SHIELDNET;
    Keyword_set["SIGNAL"] = K_SIGNAL;
    Keyword_set["SITE"] = K_SITE;
    Keyword_set["SLEWRATE"] = K_SLEWRATE;
    Keyword_set["SLOTS"] = K_SLOTS;
    Keyword_set["SOFT"] = K_SOFT;
    Keyword_set["SOURCE"] = K_SOURCE;
    Keyword_set["SPACING"] = K_SPACING;
    Keyword_set["SPECIAL"] = K_SPECIAL;
    Keyword_set["SPECIALNET"] = K_SNET;
    Keyword_set["SPECIALNETS"] = K_SNETS;
    Keyword_set["START"] = K_START;
    Keyword_set["STEINER"] = K_STEINER;
    Keyword_set["STEP"] = K_STEP;
    Keyword_set["STOP"] = K_STOP;
    Keyword_set["STRING"] = K_STRING;
    Keyword_set["STRIPE"] = K_STRIPE;
    Keyword_set["STYLE"] = K_STYLE;
    Keyword_set["STYLES"] = K_STYLES;
    Keyword_set["SUBNET"] = K_SUBNET;
    Keyword_set["SUM"] = K_SUM;
    Keyword_set["SUPPLYSENSITIVITY"] = K_SUPPLYSENSITIVITY;
    Keyword_set["SYNTHESIZED"] = K_SYNTHESIZED;
    Keyword_set["TAPER"] = K_TAPER;
    Keyword_set["TAPERRULE"] = K_TAPERRULE;
    Keyword_set["TECHNOLOGY"] = K_TECH;
    Keyword_set["TEST"] = K_TEST;
    Keyword_set["TIEOFF"] = K_TIEOFF;
    Keyword_set["TIMING"] = K_TIMING;
    Keyword_set["TIMINGDISABLES"] = K_TIMINGDISABLES;
    Keyword_set["TOCLOCKPIN"] = K_TOCLOCKPIN;
    Keyword_set["TOCOMPPIN"] = K_TOCOMPPIN;
    Keyword_set["TOIOPIN"] = K_TOIOPIN;
    Keyword_set["TOPIN"] = K_TOPIN;
    Keyword_set["TOPRIGHT"] = K_TOPRIGHT;
    Keyword_set["TRACKS"] = K_TRACKS;
    Keyword_set["TRUNK"] = K_TRUNK;
    Keyword_set["TURNOFF"] = K_TURNOFF;
    Keyword_set["TYPE"] = K_TYPE;
    Keyword_set["UNITS"] = K_UNITS;
    Keyword_set["UNPLACED"] = K_UNPLACED;
    Keyword_set["USE"] = K_USE;
    Keyword_set["USER"] = K_USER;
    Keyword_set["VARIABLE"] = K_VARIABLE;
    Keyword_set["VERSION"] = K_VERSION;
    Keyword_set["VERTICAL"] = K_VERTICAL;
    Keyword_set["VIA"] = K_VIA;
    Keyword_set["VIARULE"] = K_VIARULE;
    Keyword_set["VIAS"] = K_VIAS;
    Keyword_set["VIRTUAL"] = K_VIRTUAL;
    Keyword_set["VOLTAGE"] = K_VOLTAGE;
    Keyword_set["VPIN"] = K_VPIN;
    Keyword_set["W"] = K_W;
    Keyword_set["WEIGHT"] = K_WEIGHT;
    Keyword_set["WIDTH"] = K_WIDTH;
    Keyword_set["WIRECAP"] = K_WIRECAP;
    Keyword_set["WIREEXT"] = K_WIREEXT;
    Keyword_set["WIREDLOGIC"] = K_WIREDLOGIC;
    Keyword_set["X"] = K_X;
    Keyword_set["XTALK"] = K_XTALK;
    Keyword_set["Y"] = K_Y;
}

defrSession::defrSession() 
: FileName(0),
  reader_case_sensitive(0),
  UserData(NULL)
{
}

END_LEFDEF_PARSER_NAMESPACE
