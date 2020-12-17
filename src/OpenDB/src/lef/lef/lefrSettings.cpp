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
//  $Author: arakhman $
//  $Revision: #11 $
//  $Date: 2013/04/23 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include <string.h>


#include "lefrSettings.hpp"
#include "lef.tab.h"

BEGIN_LEFDEF_PARSER_NAMESPACE

lefrSettings *lefSettings = NULL;

lefrSettings::lefrSettings()
: CommentChar('#'),
  VersionNum(0.0),
  DisPropStrProcess(0),
  CaseSensitive(FALSE),
  CaseSensitiveSet(FALSE),
  DeltaNumberLines(10000),
  AntennaInoutWarnings(999),
  AntennaInputWarnings(999),
  AntennaOutputWarnings(999),
  ArrayWarnings(999),
  CaseSensitiveWarnings(999),
  CorrectionTableWarnings(999),
  DielectricWarnings(999),
  EdgeRateScaleFactorWarnings(999),
  EdgeRateThreshold1Warnings(999),
  EdgeRateThreshold2Warnings(999),
  IRDropWarnings(999),
  InoutAntennaWarnings(999),
  InputAntennaWarnings(999),
  LineNumberFunction(0),
  LayerWarnings(999),
  MacroWarnings(999),
  MaxStackViaWarnings(999),
  MinFeatureWarnings(999),
  NoWireExtensionWarnings(999),
  NoiseMarginWarnings(999),
  NoiseTableWarnings(999),
  NonDefaultWarnings(999),
  OutputAntennaWarnings(999),
  PinWarnings(999),
  ReadFunction(0),
  ReadEncrypted(0),
  RegisterUnused(0),
  RelaxMode(FALSE),
  ShiftCase(0),
  SiteWarnings(999),
  SpacingWarnings(999),
  TimingWarnings(999),
  UnitsWarnings(999),
  UseMinSpacingWarnings(999),
  ViaRuleWarnings(999),
  ViaWarnings(999),
  LogFileAppend(0),
  TotalMsgLimit(0),
  UserData(NULL),
  MallocFunction(0),
  ReallocFunction(0),
  FreeFunction(0),
  ErrorLogFunction(0),
  SetLogFunction(0),
  WarningLogFunction(0),  
  dAllMsgs(0)
{
    memset(MsgLimit, 0, MAX_LEF_MSGS * sizeof(int));
    init_symbol_table();

    // Define LEF58_TYPE values and dependences here:

    // Popular layer groups.
    const char *polyroutingLayers[] = {"ROUTING", ""};
    const char *mimcapLayers[] = {"ROUTING", "CUT", ""};    
    const char *tsvLayers[] = {"CUT", ""};
    const char *mastersliceOnly[] = {"MASTERSLICE", ""};
    const char *wellLayers[] = {"MASTERSLICE", "OVERLAP", ""};

    // Register LEF58 types and allowed layer types pairs.
    addLef58Type("POLYROUTING", polyroutingLayers);
    addLef58Type("MIMCAP", mimcapLayers);
    addLef58Type("TSV", tsvLayers);
    addLef58Type("PASSIVATION", tsvLayers);
    addLef58Type("TRIMPOLY", mastersliceOnly);
    addLef58Type("NWELL", wellLayers);
    addLef58Type("PWELL", wellLayers);
    addLef58Type("BELOWDIEEDGE", wellLayers);
    addLef58Type("ABOVEDIEEDGE", wellLayers);
    addLef58Type("DIFFUSION", wellLayers);
    addLef58Type("TRIMMETAL", wellLayers);
    addLef58Type("MEOL", mastersliceOnly);
}


void
lefrSettings::reset()
{
    if (lefSettings) {
        delete lefSettings;
    }

    lefSettings = new lefrSettings();
}


void
lefrSettings::init_symbol_table()
{
    Keyword_set["&DEFINE"] = K_DEFINE;
    Keyword_set["&DEFINEB"] = K_DEFINEB;
    Keyword_set["&DEFINES"] = K_DEFINES;
    Keyword_set["&MESSAGE"] = K_MESSAGE;
    Keyword_set["&CREATEFILE"] = K_CREATEFILE;
    Keyword_set["&OPENFILE"] = K_OPENFILE;
    Keyword_set["&CLOSEFILE"] = K_CLOSEFILE;
    Keyword_set["&WARNING"] = K_WARNING;
    Keyword_set["&ERROR"] = K_ERROR;
    Keyword_set["&FATALERROR"] = K_FATALERROR;
    Keyword_set["namescasesensitive"] = K_NAMESCASESENSITIVE;
    Keyword_set["off"] = K_OFF;
    Keyword_set["on"] = K_ON;
    Keyword_set["ABOVE"] = K_ABOVE;
    Keyword_set["ABUT"] = K_ABUT;
    Keyword_set["ABUTMENT"] = K_ABUTMENT;
    Keyword_set["ACCURRENTDENSITY"] = K_ACCURRENTDENSITY;
    Keyword_set["ACTIVE"] = K_ACTIVE;
    Keyword_set["ADJACENTCUTS"] = K_ADJACENTCUTS;
    Keyword_set["ANALOG"] = K_ANALOG;
    Keyword_set["AND"] = K_AND;
    Keyword_set["ANTENNAAREAFACTOR"] = K_ANTENNAAREAFACTOR;
    Keyword_set["ANTENNAAREADIFFREDUCEPWL"] = K_ANTENNAAREADIFFREDUCEPWL;
    Keyword_set["ANTENNAAREAMINUSDIFF"] = K_ANTENNAAREAMINUSDIFF;
    Keyword_set["ANTENNAAREARATIO"] = K_ANTENNAAREARATIO;
    Keyword_set["ANTENNACELL"] = K_ANTENNACELL;
    Keyword_set["ANTENNACUMAREARATIO"] = K_ANTENNACUMAREARATIO;
    Keyword_set["ANTENNACUMDIFFAREARATIO"] = K_ANTENNACUMDIFFAREARATIO;
    Keyword_set["ANTENNACUMDIFFSIDEAREARATIO"] = K_ANTENNACUMDIFFSIDEAREARATIO;
    Keyword_set["ANTENNACUMROUTINGPLUSCUT"] = K_ANTENNACUMROUTINGPLUSCUT;
    Keyword_set["ANTENNACUMSIDEAREARATIO"] = K_ANTENNACUMSIDEAREARATIO;
    Keyword_set["ANTENNADIFFAREA"] = K_ANTENNADIFFAREA;
    Keyword_set["ANTENNADIFFAREARATIO"] = K_ANTENNADIFFAREARATIO;
    Keyword_set["ANTENNADIFFSIDEAREARATIO"] = K_ANTENNADIFFSIDEAREARATIO;
    Keyword_set["ANTENNAGATEAREA"] = K_ANTENNAGATEAREA;
    Keyword_set["ANTENNAGATEPLUSDIFF"] = K_ANTENNAGATEPLUSDIFF;
    Keyword_set["ANTENNAINOUTDIFFAREA"] = K_ANTENNAINOUTDIFFAREA;
    Keyword_set["ANTENNAINPUTGATEAREA"] = K_ANTENNAINPUTGATEAREA;
    Keyword_set["ANTENNALENGTHFACTOR"] = K_ANTENNALENGTHFACTOR;
    Keyword_set["ANTENNAMAXAREACAR"] = K_ANTENNAMAXAREACAR;
    Keyword_set["ANTENNAMAXCUTCAR"] = K_ANTENNAMAXCUTCAR;
    Keyword_set["ANTENNAMAXSIDEAREACAR"] = K_ANTENNAMAXSIDEAREACAR;
    Keyword_set["ANTENNAMETALAREA"] = K_ANTENNAMETALAREA;
    Keyword_set["ANTENNAMETALLENGTH"] = K_ANTENNAMETALLENGTH;
    Keyword_set["ANTENNAMODEL"] = K_ANTENNAMODEL;
    Keyword_set["ANTENNAOUTPUTDIFFAREA"] = K_ANTENNAOUTPUTDIFFAREA;
    Keyword_set["ANTENNAPARTIALCUTAREA"] = K_ANTENNAPARTIALCUTAREA;
    Keyword_set["ANTENNAPARTIALMETALAREA"] = K_ANTENNAPARTIALMETALAREA;
    Keyword_set["ANTENNAPARTIALMETALSIDEAREA"] = K_ANTENNAPARTIALMETALSIDEAREA;
    Keyword_set["ANTENNASIDEAREARATIO"] = K_ANTENNASIDEAREARATIO;
    Keyword_set["ANTENNASIZE"] = K_ANTENNASIZE;
    Keyword_set["ANTENNASIDEAREAFACTOR"] = K_ANTENNASIDEAREAFACTOR;
    Keyword_set["ANYEDGE"] = K_ANYEDGE;
    Keyword_set["AREA"] = K_AREA;
    Keyword_set["AREAIO"] = K_AREAIO;
    Keyword_set["ARRAY"] = K_ARRAY;
    Keyword_set["ARRAYCUTS"] = K_ARRAYCUTS;
    Keyword_set["ARRAYSPACING"] = K_ARRAYSPACING;
    Keyword_set["AVERAGE"] = K_AVERAGE;
    Keyword_set["BELOW"] = K_BELOW;
    Keyword_set["BEGINEXT"] = K_BEGINEXT;
    Keyword_set["BLACKBOX"] = K_BLACKBOX;
    Keyword_set["BLOCK"] = K_BLOCK;
    Keyword_set["BOTTOMLEFT"] = K_BOTTOMLEFT;
    Keyword_set["BOTTOMRIGHT"] = K_BOTTOMRIGHT;
    Keyword_set["BUMP"] = K_BUMP;
    Keyword_set["BUSBITCHARS"] = K_BUSBITCHARS;
    Keyword_set["BUFFER"] = K_BUFFER;
    Keyword_set["BY"] = K_BY;
    Keyword_set["CANNOTOCCUPY"] = K_CANNOTOCCUPY;
    Keyword_set["CANPLACE"] = K_CANPLACE;
    Keyword_set["CAPACITANCE"] = K_CAPACITANCE;
    Keyword_set["CAPMULTIPLIER"] = K_CAPMULTIPLIER;
    Keyword_set["CENTERTOCENTER"] = K_CENTERTOCENTER;
    Keyword_set["CLASS"] = K_CLASS;
    Keyword_set["CLEARANCEMEASURE"] = K_CLEARANCEMEASURE;
    Keyword_set["CLOCK"] = K_CLOCK;
    Keyword_set["CLOCKTYPE"] = K_CLOCKTYPE;
    Keyword_set["COLUMNMAJOR"] = K_COLUMNMAJOR;
    Keyword_set["CURRENTDEN"] = K_CURRENTDEN;
    Keyword_set["COMPONENTPIN"] = K_COMPONENTPIN;
    Keyword_set["CORE"] = K_CORE;
    Keyword_set["CORNER"] = K_CORNER;
    Keyword_set["CORRECTIONFACTOR"] = K_CORRECTIONFACTOR;
    Keyword_set["CORRECTIONTABLE"] = K_CORRECTIONTABLE;
    Keyword_set["COVER"] = K_COVER;
    Keyword_set["CPERSQDIST"] = K_CPERSQDIST;
    Keyword_set["CURRENT"] = K_CURRENT;
    Keyword_set["CURRENTSOURCE"] = K_CURRENTSOURCE;
    Keyword_set["CUT"] = K_CUT;
    Keyword_set["CUTAREA"] = K_CUTAREA;
    Keyword_set["CUTSIZE"] = K_CUTSIZE;
    Keyword_set["CUTSPACING"] = K_CUTSPACING;
    Keyword_set["DATA"] = K_DATA;
    Keyword_set["DATABASE"] = K_DATABASE;
    Keyword_set["DCCURRENTDENSITY"] = K_DCCURRENTDENSITY;
    Keyword_set["DEFAULT"] = K_DEFAULT;
    Keyword_set["DEFAULTCAP"] = K_DEFAULTCAP;
    Keyword_set["DELAY"] = K_DELAY;
    Keyword_set["DENSITY"] = K_DENSITY;
    Keyword_set["DENSITYCHECKSTEP"] = K_DENSITYCHECKSTEP;
    Keyword_set["DENSITYCHECKWINDOW"] = K_DENSITYCHECKWINDOW;
    Keyword_set["DESIGNRULEWIDTH"] = K_DESIGNRULEWIDTH;
    Keyword_set["DIAG45"] = K_DIAG45;
    Keyword_set["DIAG135"] = K_DIAG135;
    Keyword_set["DIAGMINEDGELENGTH"] = K_DIAGMINEDGELENGTH;
    Keyword_set["DIAGSPACING"] = K_DIAGSPACING;
    Keyword_set["DIAGPITCH"] = K_DIAGPITCH;
    Keyword_set["DIAGWIDTH"] = K_DIAGWIDTH;
    Keyword_set["DIELECTRIC"] = K_DIELECTRIC;
    Keyword_set["DIFFUSEONLY"] = K_DIFFUSEONLY;
    Keyword_set["DIRECTION"] = K_DIRECTION;
    Keyword_set["DIVIDERCHAR"] = K_DIVIDERCHAR;
    Keyword_set["DO"] = K_DO;
    Keyword_set["E"] = K_E;
    Keyword_set["EDGECAPACITANCE"] = K_EDGECAPACITANCE;
    Keyword_set["EDGERATE"] = K_EDGERATE;
    Keyword_set["EDGERATESCALEFACTOR"] = K_EDGERATESCALEFACTOR;
    Keyword_set["EDGERATETHRESHOLD1"] = K_EDGERATETHRESHOLD1;
    Keyword_set["EDGERATETHRESHOLD2"] = K_EDGERATETHRESHOLD2;
    Keyword_set["EEQ"] = K_EEQ;
    Keyword_set["ELSE"] = K_ELSE;
    Keyword_set["ENCLOSURE"] = K_ENCLOSURE;
    Keyword_set["END"] = K_END;
    Keyword_set["ENDEXT"] = K_ENDEXT;
    Keyword_set["ENDCAP"] = K_ENDCAP;
    Keyword_set["ENDOFLINE"] = K_ENDOFLINE;
    Keyword_set["ENDOFNOTCHWIDTH"] = K_ENDOFNOTCHWIDTH;
    Keyword_set["EUCLIDEAN"] = K_EUCLIDEAN;
    Keyword_set["EXCEPTEXTRACUT"] = K_EXCEPTEXTRACUT;
    Keyword_set["EXCEPTSAMEPGNET"] = K_EXCEPTSAMEPGNET;
    Keyword_set["EXCEPTPGNET"] = K_EXCEPTPGNET;
    Keyword_set["EXTENSION"] = K_EXTENSION;
    Keyword_set["FALL"] = K_FALL;
    Keyword_set["FALLCS"] = K_FALLCS;
    Keyword_set["FALLRS"] = K_FALLRS;
    Keyword_set["FALLSATCUR"] = K_FALLSATCUR;
    Keyword_set["FALLSATT1"] = K_FALLSATT1;
    Keyword_set["FALLSLEWLIMIT"] = K_FALLSLEWLIMIT;
    Keyword_set["FALLT0"] = K_FALLT0;
    Keyword_set["FALLTHRESH"] = K_FALLTHRESH;
    Keyword_set["FALLVOLTAGETHRESHOLD"] = K_FALLVOLTAGETHRESHOLD;
    Keyword_set["FALSE"] = K_FALSE;
    Keyword_set["FE"] = K_FE;
    Keyword_set["FEEDTHRU"] = K_FEEDTHRU;
    Keyword_set["FILLACTIVESPACING"] = K_FILLACTIVESPACING;
    Keyword_set["FIXED"] = K_FIXED;
    Keyword_set["FIXEDMASK"] = K_FIXEDMASK;
    Keyword_set["FLIP"] = K_FLIP;
    Keyword_set["FLOORPLAN"] = K_FLOORPLAN;
    Keyword_set["FN"] = K_FN;
    Keyword_set["FOREIGN"] = K_FOREIGN;
    Keyword_set["FREQUENCY"] = K_FREQUENCY;
    Keyword_set["FROMABOVE"] = K_FROMABOVE;
    Keyword_set["FROMBELOW"] = K_FROMBELOW;
    Keyword_set["FROMPIN"] = K_FROMPIN;
    Keyword_set["FUNCTION"] = K_FUNCTION;
    Keyword_set["FS"] = K_FS;
    Keyword_set["FW"] = K_FW;
    Keyword_set["GCELLGRID"] = K_GCELLGRID;
    Keyword_set["GENERATE"] = K_GENERATE;
    Keyword_set["GENERATED"] = K_GENERATED;
    Keyword_set["GENERATOR"] = K_GENERATOR;
    Keyword_set["GROUND"] = K_GROUND;
    Keyword_set["GROUNDSENSITIVITY"] = K_GROUNDSENSITIVITY;
    Keyword_set["HARDSPACING"] = K_HARDSPACING;
    Keyword_set["HEIGHT"] = K_HEIGHT;
    Keyword_set["HISTORY"] = K_HISTORY;
    Keyword_set["HOLD"] = K_HOLD;
    Keyword_set["HORIZONTAL"] = K_HORIZONTAL;
    Keyword_set["IF"] = K_IF;
    Keyword_set["IMPLANT"] = K_IMPLANT;
    Keyword_set["INFLUENCE"] = K_INFLUENCE;
    Keyword_set["INOUT"] = K_INOUT;
    Keyword_set["INOUTPINANTENNASIZE"] = K_INOUTPINANTENNASIZE;
    Keyword_set["INPUT"] = K_INPUT;
    Keyword_set["INPUTPINANTENNASIZE"] = K_INPUTPINANTENNASIZE;
    Keyword_set["INPUTNOISEMARGIN"] = K_INPUTNOISEMARGIN;
    Keyword_set["INSIDECORNER"] = K_INSIDECORNER;
    Keyword_set["INTEGER"] = K_INTEGER;
    Keyword_set["INTRINSIC"] = K_INTRINSIC;
    Keyword_set["INVERT"] = K_INVERT;
    Keyword_set["INVERTER"] = K_INVERTER;
    Keyword_set["IRDROP"] = K_IRDROP;
    Keyword_set["ITERATE"] = K_ITERATE;
    Keyword_set["IV_TABLES"] = K_IV_TABLES;
    Keyword_set["LAYER"] = K_LAYER;
    Keyword_set["LAYERS"] = K_LAYERS;
    Keyword_set["LEAKAGE"] = K_LEAKAGE;
    Keyword_set["LENGTH"] = K_LENGTH;
    Keyword_set["LENGTHSUM"] = K_LENGTHSUM;
    Keyword_set["LENGTHTHRESHOLD"] = K_LENGTHTHRESHOLD;
    Keyword_set["LEQ"] = K_LEQ;
    Keyword_set["LIBRARY"] = K_LIBRARY;
    Keyword_set["LONGARRAY"] = K_LONGARRAY;
    Keyword_set["MACRO"] = K_MACRO;
    Keyword_set["MANUFACTURINGGRID"] = K_MANUFACTURINGGRID;
    Keyword_set["MASTERSLICE"] = K_MASTERSLICE;
    Keyword_set["MASK"] = K_MASK;
    Keyword_set["MATCH"] = K_MATCH;
    Keyword_set["MAXADJACENTSLOTSPACING"] = K_MAXADJACENTSLOTSPACING;
    Keyword_set["MAXCOAXIALSLOTSPACING"] = K_MAXCOAXIALSLOTSPACING;
    Keyword_set["MAXDELAY"] = K_MAXDELAY;
    Keyword_set["MAXEDGES"] = K_MAXEDGES;
    Keyword_set["MAXEDGESLOTSPACING"] = K_MAXEDGESLOTSPACING;
    Keyword_set["MAXLOAD"] = K_MAXLOAD;
    Keyword_set["MAXIMUMDENSITY"] = K_MAXIMUMDENSITY;
    Keyword_set["MAXVIASTACK"] = K_MAXVIASTACK;
    Keyword_set["MAXWIDTH"] = K_MAXWIDTH;
    Keyword_set["MAXXY"] = K_MAXXY;
    Keyword_set["MEGAHERTZ"] = K_MEGAHERTZ;
    Keyword_set["METALOVERHANG"] = K_METALOVERHANG;
    Keyword_set["MICRONS"] = K_MICRONS;
    Keyword_set["MILLIAMPS"] = K_MILLIAMPS;
    Keyword_set["MILLIWATTS"] = K_MILLIWATTS;
    Keyword_set["MINCUTS"] = K_MINCUTS;
    Keyword_set["MINENCLOSEDAREA"] = K_MINENCLOSEDAREA;
    Keyword_set["MINFEATURE"] = K_MINFEATURE;
    Keyword_set["MINIMUMCUT"] = K_MINIMUMCUT;
    Keyword_set["MINIMUMDENSITY"] = K_MINIMUMDENSITY;
    Keyword_set["MINPINS"] = K_MINPINS;
    Keyword_set["MINSIZE"] = K_MINSIZE;
    Keyword_set["MINSTEP"] = K_MINSTEP;
    Keyword_set["MINWIDTH"] = K_MINWIDTH;
    Keyword_set["MPWH"] = K_MPWH;
    Keyword_set["MPWL"] = K_MPWL;
    Keyword_set["MUSTJOIN"] = K_MUSTJOIN;
    Keyword_set["MX"] = K_MX;
    Keyword_set["MY"] = K_MY;
    Keyword_set["MXR90"] = K_MXR90;
    Keyword_set["MYR90"] = K_MYR90;
    Keyword_set["N"] = K_N;
    Keyword_set["NAMEMAPSTRING"] = K_NAMEMAPSTRING;
    Keyword_set["NAMESCASESENSITIVE"] = K_NAMESCASESENSITIVE;
    Keyword_set["NANOSECONDS"] = K_NANOSECONDS;
    Keyword_set["NEGEDGE"] = K_NEGEDGE;
    Keyword_set["NETEXPR"] = K_NETEXPR;
    Keyword_set["NETS"] = K_NETS;
    Keyword_set["NEW"] = K_NEW;
    Keyword_set["NONDEFAULTRULE"] = K_NONDEFAULTRULE;
    Keyword_set["NONE"] = K_NONE;
    Keyword_set["NONINVERT"] = K_NONINVERT;
    Keyword_set["NONUNATE"] = K_NONUNATE;
    Keyword_set["NOISETABLE"] = K_NOISETABLE;
    Keyword_set["NOTCHLENGTH"] = K_NOTCHLENGTH;
    Keyword_set["NOTCHSPACING"] = K_NOTCHSPACING;
    Keyword_set["NOWIREEXTENSIONATPIN"] = K_NOWIREEXTENSIONATPIN;
    Keyword_set["OBS"] = K_OBS;
    Keyword_set["OFF"] = K_OFF;
    Keyword_set["OFFSET"] = K_OFFSET;
    Keyword_set["OHMS"] = K_OHMS;
    Keyword_set["ON"] = K_ON;
    Keyword_set["OR"] = K_OR;
    Keyword_set["ORIENT"] = K_ORIENT;
    Keyword_set["ORIENTATION"] = K_ORIENTATION;
    Keyword_set["ORIGIN"] = K_ORIGIN;
    Keyword_set["ORTHOGONAL"] = K_ORTHOGONAL;
    Keyword_set["OUTPUT"] = K_OUTPUT;
    Keyword_set["OUTPUTPINANTENNASIZE"] = K_OUTPUTPINANTENNASIZE;
    Keyword_set["OUTPUTNOISEMARGIN"] = K_OUTPUTNOISEMARGIN;
    Keyword_set["OUTPUTRESISTANCE"] = K_OUTPUTRESISTANCE;
    Keyword_set["OUTSIDECORNER"] = K_OUTSIDECORNER;
    Keyword_set["OVERHANG"] = K_OVERHANG;
    Keyword_set["OVERLAP"] = K_OVERLAP;
    Keyword_set["OVERLAPS"] = K_OVERLAPS;
    Keyword_set["OXIDE1"] = K_OXIDE1;
    Keyword_set["OXIDE2"] = K_OXIDE2;
    Keyword_set["OXIDE3"] = K_OXIDE3;
    Keyword_set["OXIDE4"] = K_OXIDE4;
    Keyword_set["PAD"] = K_PAD;
    Keyword_set["PARALLELEDGE"] = K_PARALLELEDGE;
    Keyword_set["PARALLELOVERLAP"] = K_PARALLELOVERLAP;
    Keyword_set["PARALLELRUNLENGTH"] = K_PARALLELRUNLENGTH;
    Keyword_set["PATH"] = K_PATH;
    Keyword_set["PATTERN"] = K_PATTERN;
    Keyword_set["PEAK"] = K_PEAK;
    Keyword_set["PERIOD"] = K_PERIOD;
    Keyword_set["PGONLY"] = K_PGONLY;
    Keyword_set["PICOFARADS"] = K_PICOFARADS;
    Keyword_set["PIN"] = K_PIN;
    Keyword_set["PITCH"] = K_PITCH;
    Keyword_set["PLACED"] = K_PLACED;
    Keyword_set["POLYGON"] = K_POLYGON;
    Keyword_set["PORT"] = K_PORT;
    Keyword_set["POSEDGE"] = K_POSEDGE;
    Keyword_set["POST"] = K_POST;
    Keyword_set["POWER"] = K_POWER;
    Keyword_set["PRE"] = K_PRE;
    Keyword_set["PREFERENCLOSURE"] = K_PREFERENCLOSURE;
    Keyword_set["PRL"] = K_PRL;
    Keyword_set["PROPERTY"] = K_PROPERTY;
    Keyword_set["PROPERTYDEFINITIONS"] = K_PROPDEF;
    Keyword_set["PROTRUSIONWIDTH"] = K_PROTRUSIONWIDTH;
    Keyword_set["PULLDOWNRES"] = K_PULLDOWNRES;
    Keyword_set["PWL"] = K_PWL;
    Keyword_set["R0"] = K_R0;
    Keyword_set["R90"] = K_R90;
    Keyword_set["R180"] = K_R180;
    Keyword_set["R270"] = K_R270;
    Keyword_set["RANGE"] = K_RANGE;
    Keyword_set["REAL"] = K_REAL;
    Keyword_set["RECOVERY"] = K_RECOVERY;
    Keyword_set["RECT"] = K_RECT;
    Keyword_set["RESISTANCE"] = K_RESISTANCE;
    Keyword_set["RESISTIVE"] = K_RESISTIVE;
    Keyword_set["RING"] = K_RING;
    Keyword_set["RISE"] = K_RISE;
    Keyword_set["RISECS"] = K_RISECS;
    Keyword_set["RISERS"] = K_RISERS;
    Keyword_set["RISESATCUR"] = K_RISESATCUR;
    Keyword_set["RISESATT1"] = K_RISESATT1;
    Keyword_set["RISESLEWLIMIT"] = K_RISESLEWLIMIT;
    Keyword_set["RISET0"] = K_RISET0;
    Keyword_set["RISETHRESH"] = K_RISETHRESH;
    Keyword_set["RISEVOLTAGETHRESHOLD"] = K_RISEVOLTAGETHRESHOLD;
    Keyword_set["RMS"] = K_RMS;
    Keyword_set["ROUTING"] = K_ROUTING;
    Keyword_set["ROWABUTSPACING"] = K_ROWABUTSPACING;
    Keyword_set["ROWCOL"] = K_ROWCOL;
    Keyword_set["ROWMAJOR"] = K_ROWMAJOR;
    Keyword_set["ROWMINSPACING"] = K_ROWMINSPACING;
    Keyword_set["ROWPATTERN"] = K_ROWPATTERN;
    Keyword_set["RPERSQ"] = K_RPERSQ;
    Keyword_set["S"] = K_S;
    Keyword_set["SAMENET"] = K_SAMENET;
    Keyword_set["SCANUSE"] = K_SCANUSE;
    Keyword_set["SDFCOND"] = K_SDFCOND;
    Keyword_set["SDFCONDEND"] = K_SDFCONDEND;
    Keyword_set["SDFCONDSTART"] = K_SDFCONDSTART;
    Keyword_set["SETUP"] = K_SETUP;
    Keyword_set["SHAPE"] = K_SHAPE;
    Keyword_set["SHRINKAGE"] = K_SHRINKAGE;
    Keyword_set["SIGNAL"] = K_SIGNAL;
    Keyword_set["SITE"] = K_SITE;
    Keyword_set["SIZE"] = K_SIZE;
    Keyword_set["SKEW"] = K_SKEW;
    Keyword_set["SLOTLENGTH"] = K_SLOTLENGTH;
    Keyword_set["SLOTWIDTH"] = K_SLOTWIDTH;
    Keyword_set["SLOTWIRELENGTH"] = K_SLOTWIRELENGTH;
    Keyword_set["SLOTWIREWIDTH"] = K_SLOTWIREWIDTH;
    Keyword_set["SPLITWIREWIDTH"] = K_SPLITWIREWIDTH;
    Keyword_set["SOFT"] = K_SOFT;
    Keyword_set["SOURCE"] = K_SOURCE;
    Keyword_set["SPACER"] = K_SPACER;
    Keyword_set["SPACING"] = K_SPACING;
    Keyword_set["SPACINGTABLE"] = K_SPACINGTABLE;
    Keyword_set["SPECIALNETS"] = K_SPECIALNETS;
    Keyword_set["STABLE"] = K_STABLE;
    Keyword_set["STACK"] = K_STACK;
    Keyword_set["START"] = K_START;
    Keyword_set["STEP"] = K_STEP;
    Keyword_set["STOP"] = K_STOP;
    Keyword_set["STRING"] = K_STRING;
    Keyword_set["STRUCTURE"] = K_STRUCTURE;
    Keyword_set["SUPPLYSENSITIVITY"] = K_SUPPLYSENSITIVITY;
    Keyword_set["SYMMETRY"] = K_SYMMETRY;
    Keyword_set["TABLE"] = K_TABLE;
    Keyword_set["TABLEAXIS"] = K_TABLEAXIS;
    Keyword_set["TABLEDIMENSION"] = K_TABLEDIMENSION;
    Keyword_set["TABLEENTRIES"] = K_TABLEENTRIES;
    Keyword_set["TAPERRULE"] = K_TAPERRULE;
    Keyword_set["THEN"] = K_THEN;
    Keyword_set["THICKNESS"] = K_THICKNESS;
    Keyword_set["TIEHIGH"] = K_TIEHIGH;
    Keyword_set["TIELOW"] = K_TIELOW;
    Keyword_set["TIEOFFR"] = K_TIEOFFR;
    Keyword_set["TIME"] = K_TIME;
    Keyword_set["TIMING"] = K_TIMING;
    Keyword_set["TO"] = K_TO;
    Keyword_set["TOPIN"] = K_TOPIN;
    Keyword_set["TOPLEFT"] = K_TOPLEFT;
    Keyword_set["TOPOFSTACKONLY"] = K_TOPOFSTACKONLY;
    Keyword_set["TOPRIGHT"] = K_TOPRIGHT;
    Keyword_set["TRACKS"] = K_TRACKS;
    Keyword_set["TRANSITIONTIME"] = K_TRANSITIONTIME;
    Keyword_set["TRISTATE"] = K_TRISTATE;
    Keyword_set["TRUE"] = K_TRUE;
    Keyword_set["TWOEDGES"] = K_TWOEDGES;
    Keyword_set["TWOWIDTHS"] = K_TWOWIDTHS;
    Keyword_set["TYPE"] = K_TYPE;
    Keyword_set["UNATENESS"] = K_UNATENESS;
    Keyword_set["UNITS"] = K_UNITS;
    Keyword_set["UNIVERSALNOISEMARGIN"] = K_UNIVERSALNOISEMARGIN;
    Keyword_set["USE"] = K_USE;
    Keyword_set["USELENGTHTHRESHOLD"] = K_USELENGTHTHRESHOLD;
    Keyword_set["USEMINSPACING"] = K_USEMINSPACING;
    Keyword_set["USER"] = K_USER;
    Keyword_set["USEVIA"] = K_USEVIA;
    Keyword_set["USEVIARULE"] = K_USEVIARULE;
    Keyword_set["VARIABLE"] = K_VARIABLE;
    Keyword_set["VERSION"] = K_VERSION;
    Keyword_set["VERTICAL"] = K_VERTICAL;
    Keyword_set["VHI"] = K_VHI;
    Keyword_set["VIA"] = K_VIA;
    Keyword_set["VIARULE"] = K_VIARULE;
    Keyword_set["VICTIMLENGTH"] = K_VICTIMLENGTH;
    Keyword_set["VICTIMNOISE"] = K_VICTIMNOISE;
    Keyword_set["VIRTUAL"] = K_VIRTUAL;
    Keyword_set["VLO"] = K_VLO;
    Keyword_set["VOLTAGE"] = K_VOLTAGE;
    Keyword_set["VOLTS"] = K_VOLTS;
    Keyword_set["W"] = K_W;
    Keyword_set["WELLTAP"] = K_WELLTAP;
    Keyword_set["WIDTH"] = K_WIDTH;
    Keyword_set["WITHIN"] = K_WITHIN;
    Keyword_set["WIRECAP"] = K_WIRECAP;
    Keyword_set["WIREEXTENSION"] = K_WIREEXTENSION;
    Keyword_set["X"] = K_X;
    Keyword_set["Y"] = K_Y;
}


void 
lefrSettings::disableMsg(int msgId)
{
    msgsDisableMap[msgId] = 0;
}


void 
lefrSettings::enableMsg(int msgId)
{
    std::map<int, int>::iterator search = msgsDisableMap.find(msgId);

    if (search != msgsDisableMap.end()) {
        msgsDisableMap.erase(search);
    }
}


void 
lefrSettings::enableAllMsgs()
{
    msgsDisableMap.clear();
}


// Check if the message was disabled and returns statuses:
// 0 - enabled, 1 - disabled, need warning print, 2 - disabled no warning.
int 
lefrSettings::suppresMsg(int msgId)
{
    std::map<int, int>::iterator search = msgsDisableMap.find(msgId);

    if (search != msgsDisableMap.end()) {
        int status = msgsDisableMap[msgId];
        if (!status) {
            msgsDisableMap[msgId] = 1;
            return 1;
        } else {
            return 2;
        }
    }

    return 0;
}


// This function will get token from input string. Also sets 
// startIdx on first character after the token.
std::string
lefrSettings::getToken(const std::string &input, int &startIdx)
{
    std::string  divChars = " \n\t\r;";
    int          tokenStart = input.find_first_not_of(divChars, 
                                                      startIdx);
    int          tokenEnd = input.find_first_of(divChars, 
                                                 tokenStart);

    startIdx = tokenEnd;
    return input.substr(tokenStart, tokenEnd - tokenStart);
}


// This function adds new lef58Type-layerType pairs. layerType 
// is reference to string array last element of which should be 
// "". The pairs will be created for each element of the array.
// Duplicated pairs will be ignored.
void 
lefrSettings::addLef58Type(const char *lef58Type, 
                           const char **layerType)
{
    for (;**layerType;     layerType++) {
        std::string typesPair(lef58Type);

        typesPair = typesPair + " " + *layerType;

        Lef58TypePairs.insert(typesPair);
    }
}


std::string
lefrSettings::getLayerLef58Types(const char *type) const
{
    std::string                 result;
    StringSet::const_iterator   pairsIter = Lef58TypePairs.begin();

    for (; pairsIter != Lef58TypePairs.end(); ++pairsIter) {
        const   std::string pair(*pairsIter);
        int     sepIdx = pair.find(' ');

        if (pair.substr(sepIdx + 1) != type) {
            continue;
        }

        if (!result.empty()) {
            result += ", ";
        }

        result += pair.substr(0, sepIdx);
    }

    return result;
}

END_LEFDEF_PARSER_NAMESPACE
