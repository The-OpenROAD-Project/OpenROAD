// ************************************************************************** 
// ************************************************************************** 
// ATTENTION: THIS IS AN AUTO-GENERATED FILE. DO NOT CHANGE IT!               
// ************************************************************************** 
// ************************************************************************** 
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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $                            
// ************************************************************************** 
// ************************************************************************** 

//  Error message number:
//   100 -  lef reader, lefrReader.c
//   1000 - lef parser, error, lex.cpph, lef.y (CALLBACK & CHKERR)
//   1300 - from lefiError, lefiLayer.cpp
//   1350 - from lefiError, lefiMacro.cpp
//   1360 - from lefiError, lefiMisc.cpp
//   1400 - from lefiError, lefiNonDefault.cpp
//   1420 - from lefiError, lefiVia.cpp
//   1430 - from lefiError, lefiViaRule.cpp
//   1500 - lef parser, error, lef.y
//   2000 - lef parser, warning, lex.cpph
//   2500 - lef parser, warning, lef.y
//   3000 - lef parser, info, lex.cpph
//   4000 - lef writer, error, lefwWrtier.cpp & lefwWriterCalls.cpp
//   4500 - lef writer, warning, lefwWrtier.cpp & lefwWriterCalls.cpp
//   4700 - lef writer, info, lefwWrtier.cpp & lefwWriterCalls.cpp
// 
//   Highest message number = 4700

%{
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "lex.h"
#include "lefiDefs.hpp"
#include "lefiUser.hpp"
#include "lefiUtil.hpp"

#include "lefrData.hpp"
#include "lefrCallBacks.hpp"
#include "lefrSettings.hpp"

#if defined(__clang__)
#pragma(clang, ignored -Wdeprecated-register)
#endif

BEGIN_LEFDEF_PARSER_NAMESPACE

#define LYPROP_ECAP "EDGE_CAPACITANCE"

#define YYINITDEPTH 10000  // pcr 640902 - initialize the yystacksize to 300 
                           // this may need to increase in a design gets 
                           // larger and a polygon has around 300 sizes 
                           // 11/21/2003 - incrreased to 500, design from 
                           // Artisan is greater than 300, need to find a 
                           // way to dynamically increase the size 
                           // 2/10/2004 - increased to 1000 for pcr 686073 
                           // 3/22/2004 - increased to 2000 for pcr 695879 
                           // 9/29/2004 - double the size for pcr 746865 
                           // tried to overwrite the yyoverflow definition 
                           // it is impossible due to the union structure 
                           // 10/03/2006 - increased to 10000 for pcr 913695 

#define YYMAXDEPTH 300000  // 1/24/2008 - increased from 150000 
                           // This value has to be greater than YYINITDEPTH 


// Macro to describe how we handle a callback.
// If the function was set then call it.
// If the function returns non zero then there was an error
// so call the error routine and exit.
#define CALLBACK(func, typ, data) \
    if (!lefData->lef_errors) { \
      if (func) { \
        if ((lefData->lefRetVal = (*func)(typ, data, lefSettings->UserData)) == 0) { \
        } else { \
          return lefData->lefRetVal; \
        } \
      } \
    }

#define CHKERR() \
    if (lefData->lef_errors > 20) { \
      lefError(1020, "Too many syntax errors."); \
      lefData->lef_errors = 0; \
      return 1; \
    }

// **********************************************************************
// **********************************************************************

#define C_EQ 0
#define C_NE 1
#define C_LT 2
#define C_LE 3
#define C_GT 4
#define C_GE 5


int comp_str(char *s1, int op, char *s2)
{
    int k = strcmp(s1, s2);
    switch (op) {
        case C_EQ: return k == 0;
        case C_NE: return k != 0;
        case C_GT: return k >  0;
        case C_GE: return k >= 0;
        case C_LT: return k <  0;
        case C_LE: return k <= 0;
        }
    return 0;
}
int comp_num(double s1, int op, double s2)
{
    double k = s1 - s2;
    switch (op) {
        case C_EQ: return k == 0;
        case C_NE: return k != 0;
        case C_GT: return k >  0;
        case C_GE: return k >= 0;
        case C_LT: return k <  0;
        case C_LE: return k <= 0;
        }
    return 0;
}

int validNum(int values) {
    switch (values) {
        case 100:
        case 200:
        case 1000:
        case 2000:
             return 1;
        case 400:
        case 800:
        case 4000:
        case 8000:
        case 10000:
        case 20000:
             if (lefData->versionNum < 5.6) {
                if (lefCallbacks->UnitsCbk) {
                  if (lefData->unitsWarnings++ < lefSettings->UnitsWarnings) {
                    lefData->outMsg = (char*)lefMalloc(10000);
                    sprintf (lefData->outMsg,
                       "Error found when processing LEF file '%s'\nUnit %d is a version 5.6 or later syntax\nYour lef file is defined with version %g.",
                    lefData->lefrFileName, values, lefData->versionNum);
                    lefError(1501, lefData->outMsg);
                    lefFree(lefData->outMsg);
                  }
                }
                return 0;
             } else {
                return 1;
             }        
    }
    if (lefData->unitsWarnings++ < lefSettings->UnitsWarnings) {
       lefData->outMsg = (char*)lefMalloc(10000);
       sprintf (lefData->outMsg,
          "The value %d defined for LEF UNITS DATABASE MICRONS is invalid\n. Correct value is 100, 200, 400, 800, 1000, 2000, 4000, 8000, 10000, or 20000", values);
       lefError(1502, lefData->outMsg);
       lefFree(lefData->outMsg);
    }
    CHKERR();
    return 0;
}

int zeroOrGt(double values) {
    if (values < 0)
      return 0;
    return 1;
}

%}

%union {
        double    dval ;
        int       integer ;
        char *    string ;
        LefDefParser::lefPOINT  pt;
}

%token <string> K_HISTORY
%token K_ABUT K_ABUTMENT K_ACTIVE K_ANALOG K_ARRAY K_AREA
%token K_BLOCK K_BOTTOMLEFT K_BOTTOMRIGHT
%token K_BY K_CAPACITANCE K_CAPMULTIPLIER K_CLASS K_CLOCK K_CLOCKTYPE
%token K_COLUMNMAJOR K_DESIGNRULEWIDTH K_INFLUENCE
%token K_CORE K_CORNER K_COVER K_CPERSQDIST K_CURRENT 
%token K_CURRENTSOURCE K_CUT K_DEFAULT K_DATABASE K_DATA
%token K_DIELECTRIC K_DIRECTION K_DO K_EDGECAPACITANCE
%token K_EEQ K_END K_ENDCAP K_FALL K_FALLCS K_FALLT0 K_FALLSATT1
%token K_FALLRS K_FALLSATCUR K_FALLTHRESH K_FEEDTHRU K_FIXED K_FOREIGN K_FROMPIN
%token K_GENERATE K_GENERATOR K_GROUND K_HEIGHT K_HORIZONTAL K_INOUT K_INPUT
%token K_INPUTNOISEMARGIN K_COMPONENTPIN
%token K_INTRINSIC K_INVERT K_IRDROP K_ITERATE K_IV_TABLES K_LAYER K_LEAKAGE
%token K_LEQ K_LIBRARY K_MACRO K_MATCH K_MAXDELAY K_MAXLOAD K_METALOVERHANG K_MILLIAMPS
%token K_MILLIWATTS K_MINFEATURE K_MUSTJOIN K_NAMESCASESENSITIVE K_NANOSECONDS
%token K_NETS K_NEW K_NONDEFAULTRULE
%token K_NONINVERT K_NONUNATE K_OBS K_OHMS K_OFFSET K_ORIENTATION K_ORIGIN K_OUTPUT
%token K_OUTPUTNOISEMARGIN
%token K_OVERHANG K_OVERLAP K_OFF K_ON K_OVERLAPS K_PAD K_PATH K_PATTERN K_PICOFARADS
%token K_PIN K_PITCH
%token K_PLACED K_POLYGON K_PORT K_POST K_POWER K_PRE K_PULLDOWNRES K_RECT
%token K_RESISTANCE K_RESISTIVE K_RING K_RISE K_RISECS K_RISERS K_RISESATCUR K_RISETHRESH
%token K_RISESATT1 K_RISET0 K_RISEVOLTAGETHRESHOLD K_FALLVOLTAGETHRESHOLD
%token K_ROUTING K_ROWMAJOR K_RPERSQ K_SAMENET K_SCANUSE K_SHAPE K_SHRINKAGE
%token K_SIGNAL K_SITE K_SIZE K_SOURCE K_SPACER K_SPACING K_SPECIALNETS K_STACK
%token K_START K_STEP K_STOP K_STRUCTURE K_SYMMETRY K_TABLE K_THICKNESS K_TIEHIGH
%token K_TIELOW K_TIEOFFR K_TIME K_TIMING K_TO K_TOPIN K_TOPLEFT K_TOPRIGHT
%token K_TOPOFSTACKONLY
%token K_TRISTATE K_TYPE K_UNATENESS K_UNITS K_USE K_VARIABLE K_VERTICAL K_VHI
%token K_VIA K_VIARULE K_VLO K_VOLTAGE K_VOLTS K_WIDTH K_X K_Y
%token <string> T_STRING QSTRING
%token <dval> NUMBER
%token K_N K_S K_E K_W K_FN K_FS K_FE K_FW
%token K_R0 K_R90 K_R180 K_R270 K_MX K_MY K_MXR90 K_MYR90
%token K_USER K_MASTERSLICE
%token K_ENDMACRO K_ENDMACROPIN K_ENDVIARULE K_ENDVIA K_ENDLAYER K_ENDSITE
%token K_CANPLACE K_CANNOTOCCUPY K_TRACKS K_FLOORPLAN K_GCELLGRID K_DEFAULTCAP
%token K_MINPINS K_WIRECAP
%token K_STABLE K_SETUP K_HOLD
%token K_DEFINE K_DEFINES K_DEFINEB K_IF K_THEN K_ELSE K_FALSE K_TRUE 
%token K_EQ K_NE K_LE K_LT K_GE K_GT K_OR K_AND K_NOT
%token K_DELAY K_TABLEDIMENSION K_TABLEAXIS K_TABLEENTRIES K_TRANSITIONTIME
%token K_EXTENSION
%token K_PROPDEF K_STRING K_INTEGER K_REAL K_RANGE K_PROPERTY
%token K_VIRTUAL K_BUSBITCHARS K_VERSION
%token K_BEGINEXT K_ENDEXT
%token K_UNIVERSALNOISEMARGIN K_EDGERATETHRESHOLD1 K_CORRECTIONTABLE
%token K_EDGERATESCALEFACTOR K_EDGERATETHRESHOLD2 K_VICTIMNOISE
%token K_NOISETABLE K_EDGERATE K_OUTPUTRESISTANCE K_VICTIMLENGTH
%token K_CORRECTIONFACTOR K_OUTPUTPINANTENNASIZE
%token K_INPUTPINANTENNASIZE K_INOUTPINANTENNASIZE
%token K_CURRENTDEN K_PWL K_ANTENNALENGTHFACTOR K_TAPERRULE
%token K_DIVIDERCHAR K_ANTENNASIZE K_ANTENNAMETALLENGTH K_ANTENNAMETALAREA
%token K_RISESLEWLIMIT K_FALLSLEWLIMIT K_FUNCTION K_BUFFER K_INVERTER
%token K_NAMEMAPSTRING K_NOWIREEXTENSIONATPIN K_WIREEXTENSION
%token K_MESSAGE K_CREATEFILE K_OPENFILE K_CLOSEFILE K_WARNING
%token K_ERROR K_FATALERROR
%token K_RECOVERY K_SKEW K_ANYEDGE K_POSEDGE K_NEGEDGE
%token K_SDFCONDSTART K_SDFCONDEND K_SDFCOND
%token K_MPWH K_MPWL K_PERIOD
%token K_ACCURRENTDENSITY K_DCCURRENTDENSITY K_AVERAGE K_PEAK K_RMS K_FREQUENCY
%token K_CUTAREA K_MEGAHERTZ K_USELENGTHTHRESHOLD K_LENGTHTHRESHOLD
%token K_ANTENNAINPUTGATEAREA K_ANTENNAINOUTDIFFAREA K_ANTENNAOUTPUTDIFFAREA
%token K_ANTENNAAREARATIO K_ANTENNADIFFAREARATIO K_ANTENNACUMAREARATIO
%token K_ANTENNACUMDIFFAREARATIO K_ANTENNAAREAFACTOR K_ANTENNASIDEAREARATIO
%token K_ANTENNADIFFSIDEAREARATIO K_ANTENNACUMSIDEAREARATIO
%token K_ANTENNACUMDIFFSIDEAREARATIO K_ANTENNASIDEAREAFACTOR
%token K_DIFFUSEONLY K_MANUFACTURINGGRID K_FIXEDMASK
%token K_ANTENNACELL K_CLEARANCEMEASURE K_EUCLIDEAN K_MAXXY
%token K_USEMINSPACING K_ROWMINSPACING K_ROWABUTSPACING K_FLIP K_NONE
%token K_ANTENNAPARTIALMETALAREA K_ANTENNAPARTIALMETALSIDEAREA
%token K_ANTENNAGATEAREA K_ANTENNADIFFAREA K_ANTENNAMAXAREACAR
%token K_ANTENNAMAXSIDEAREACAR K_ANTENNAPARTIALCUTAREA K_ANTENNAMAXCUTCAR
%token K_SLOTWIREWIDTH K_SLOTWIRELENGTH K_SLOTWIDTH K_SLOTLENGTH
%token K_MAXADJACENTSLOTSPACING K_MAXCOAXIALSLOTSPACING K_MAXEDGESLOTSPACING
%token K_SPLITWIREWIDTH K_MINIMUMDENSITY K_MAXIMUMDENSITY K_DENSITYCHECKWINDOW
%token K_DENSITYCHECKSTEP K_FILLACTIVESPACING K_MINIMUMCUT K_ADJACENTCUTS
%token K_ANTENNAMODEL K_BUMP K_ENCLOSURE K_FROMABOVE K_FROMBELOW
%token K_IMPLANT K_LENGTH K_MAXVIASTACK K_AREAIO K_BLACKBOX
%token K_MAXWIDTH K_MINENCLOSEDAREA K_MINSTEP K_ORIENT K_OXIDE1 K_OXIDE2
%token K_OXIDE3 K_OXIDE4 K_PARALLELRUNLENGTH K_MINWIDTH
%token K_PROTRUSIONWIDTH K_SPACINGTABLE K_WITHIN
%token K_ABOVE K_BELOW K_CENTERTOCENTER K_CUTSIZE K_CUTSPACING K_DENSITY
%token K_DIAG45 K_DIAG135 K_MASK
%token K_DIAGMINEDGELENGTH K_DIAGSPACING K_DIAGPITCH K_DIAGWIDTH
%token K_GENERATED K_GROUNDSENSITIVITY K_HARDSPACING K_INSIDECORNER
%token K_LAYERS K_LENGTHSUM K_MICRONS K_MINCUTS
%token K_MINSIZE K_NETEXPR K_OUTSIDECORNER
%token K_PREFERENCLOSURE K_ROWCOL K_ROWPATTERN K_SOFT
%token K_SUPPLYSENSITIVITY K_USEVIA
%token K_USEVIARULE K_WELLTAP
%token K_ARRAYCUTS K_ARRAYSPACING K_ANTENNAAREADIFFREDUCEPWL
%token K_ANTENNAAREAMINUSDIFF
%token K_ANTENNACUMROUTINGPLUSCUT K_ANTENNAGATEPLUSDIFF
%token K_ENDOFLINE K_ENDOFNOTCHWIDTH K_EXCEPTEXTRACUT K_EXCEPTSAMEPGNET
%token K_EXCEPTPGNET
%token K_LONGARRAY K_MAXEDGES K_NOTCHLENGTH K_NOTCHSPACING K_ORTHOGONAL
%token K_PARALLELEDGE K_PARALLELOVERLAP K_PGONLY K_PRL K_TWOEDGES K_TWOWIDTHS

%type <string> start_macro end_macro
%type <string> start_layer
%type <string> macro_pin_use
%type <string> macro_scan_use
%type <string> pin_shape
%type <string> pad_type core_type endcap_type class_type site_class
%type <string> start_foreign spacing_type clearance_type
%type <pt> pt 
%type <pt> macro_origin
%type <string> layer_option layer_options layer_type layer_direction
%type <string> electrical_direction
%type <integer> orientation maskColor
%type <dval> expression
%type <integer> b_expr
%type <string>  s_expr
%type <integer> relop spacing_value
%type <string> opt_layer_name risefall unateness delay_or_transition
%type <string> two_pin_trigger from_pin_trigger to_pin_trigger
%type <string> one_pin_trigger req_layer_name 
%type <string> layer_table_type layer_enclosure_type_opt layer_minstep_type
%type <dval>   layer_sp_TwoWidthsPRL
%type <dval> int_number

%nonassoc IF
%left K_AND
%left K_OR
%left K_LE K_EQ K_LT K_NE K_GE K_GT
%nonassoc LNOT
%left '-' '+'
%left '*' '/'
%nonassoc UMINUS

%%

lef_file: rules extension_opt  end_library
      {
        // 11/16/2001 - Wanda da Rosa - pcr 408334
        // Return 1 if there are errors
        if (lefData->lef_errors)
           return 1;
        if (!lefData->hasVer) {
              char temp[300];
              sprintf(temp, "No VERSION statement found, using the default value %2g.", lefData->versionNum);
              lefWarning(2001, temp);            
        }        
        //only pre 5.6, 5.6 it is obsolete
        if (!lefData->hasNameCase && lefData->versionNum < 5.6)
           lefWarning(2002, "NAMESCASESENSITIVE is a required statement on LEF file with version 5.5 and earlier.\nWithout NAMESCASESENSITIVE defined, the LEF file is technically incorrect.\nRefer the LEF/DEF 5.5 or earlier Language Referece manual on how to define this statement.");
        if (!lefData->hasBusBit && lefData->versionNum < 5.6)
           lefWarning(2003, "BUSBITCHARS is a required statement on LEF file with version 5.5 and earlier.\nWithout BUSBITCHARS defined, the LEF file is technically incorrect.\nRefer the LEF/DEF 5.5 or earlier Language Referece manual on how to define this statement.");
        if (!lefData->hasDivChar && lefData->versionNum < 5.6)
           lefWarning(2004, "DIVIDERCHAR is a required statementon LEF file with version 5.5 and earlier.\nWithout DIVIDECHAR defined, the LEF file is technically incorrect.\nRefer the LEF/DEF 5.5 or earlier Language Referece manual on how to define this statement.");

      }

version: K_VERSION { lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING ';'
      { 
		 // More than 1 VERSION in lef file within the open file - It's wrong syntax, 
		 // but copy old behavior - initialize lef reading.
         if (lefData->hasVer)     
         {
			lefData->initRead();
		 }

         lefData->versionNum = convert_name2num($3);
         if (lefData->versionNum > CURRENT_VERSION) {
            char temp[120];
            sprintf(temp,
               "Lef parser %.1f does not support lef file with version %s. Parser will stop processing.", CURRENT_VERSION, $3);
            lefError(1503, temp);
            return 1;
         }

         if (lefCallbacks->VersionStrCbk) {
            CALLBACK(lefCallbacks->VersionStrCbk, lefrVersionStrCbkType, $3);
         } else {
            if (lefCallbacks->VersionCbk)
               CALLBACK(lefCallbacks->VersionCbk, lefrVersionCbkType, lefData->versionNum);
         }
         if (lefData->versionNum > 5.3 && lefData->versionNum < 5.4) {
            lefData->ignoreVersion = 1;
         }
         lefData->use5_3 = lefData->use5_4 = 0;
         lefData->lef_errors = 0;
         lefData->hasVer = 1;
         if (lefData->versionNum < 5.6) {
            lefData->doneLib = 0;
            lefData->namesCaseSensitive = lefSettings->CaseSensitive;
         } else {
            lefData->doneLib = 1;
            lefData->namesCaseSensitive = 1;
         }
      }

int_number : NUMBER 
      {
         // int_number represent 'integer-like' type. It can have fraction and exponent part 
         // but the value shouldn't exceed the 64-bit integer limit. 
         if (!(( yylval.dval >= lefData->leflVal) && ( yylval.dval <= lefData->lefrVal))) { // YES, it isn't really a number 
            char *str = (char*) lefMalloc(strlen(lefData->current_token) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-203) Number has exceeded the limit for an integer. See file %s at line %d.\n",
                    lefData->lefrFileName, lefData->lef_nlines);
            fflush(stdout);
            lefiError(0, 203, str);
            free(str);
            lefData->lef_errors++;
        }

        $$ = yylval.dval ;
      }

dividerchar: K_DIVIDERCHAR QSTRING ';'
      {
        if (lefCallbacks->DividerCharCbk) {
          if (strcmp($2, "") != 0) {
             CALLBACK(lefCallbacks->DividerCharCbk, lefrDividerCharCbkType, $2);
          } else {
             CALLBACK(lefCallbacks->DividerCharCbk, lefrDividerCharCbkType, "/");
             lefWarning(2005, "DIVIDERCHAR has an invalid null value. Value is set to default /");
          }
        }
        lefData->hasDivChar = 1;
      }

busbitchars: K_BUSBITCHARS QSTRING ';'
      {
        if (lefCallbacks->BusBitCharsCbk) {
          if (strcmp($2, "") != 0) {
             CALLBACK(lefCallbacks->BusBitCharsCbk, lefrBusBitCharsCbkType, $2); 
          } else {
             CALLBACK(lefCallbacks->BusBitCharsCbk, lefrBusBitCharsCbkType, "[]"); 
             lefWarning(2006, "BUSBITCHAR has an invalid null value. Value is set to default []");
          }
        }
        lefData->hasBusBit = 1;
      }

rules:
        | rules rule
        | error 
            { }

end_library:
      {
        if (lefData->versionNum >= 5.6) {
           lefData->doneLib = 1;
           lefData->ge56done = 1;
        }
      }
      | K_END K_LIBRARY
      {
        lefData->doneLib = 1;
        lefData->ge56done = 1;
        if (lefCallbacks->LibraryEndCbk)
          CALLBACK(lefCallbacks->LibraryEndCbk, lefrLibraryEndCbkType, 0);
        // 11/16/2001 - Wanda da Rosa - pcr 408334
        // Return 1 if there are errors
      }

rule:  version | busbitchars | case_sensitivity | units_section
    | layer_rule | via | viarule | viarule_generate | dividerchar
    | wireextension | msg_statement
    | spacing_rule | dielectric | minfeature | irdrop | site | macro | array
    | def_statement | nondefault_rule | prop_def_section
    | universalnoisemargin | edgeratethreshold1
    | edgeratescalefactor | edgeratethreshold2
    | noisetable | correctiontable | input_antenna
    | output_antenna | inout_antenna
    | antenna_input | antenna_inout | antenna_output | manufacturing  | fixedmask 
    | useminspacing | clearancemeasure | maxstack_via
    | create_file_statement
    ;

case_sensitivity: K_NAMESCASESENSITIVE K_ON ';'
          {
            if (lefData->versionNum < 5.6) {
              lefData->namesCaseSensitive = TRUE;
              if (lefCallbacks->CaseSensitiveCbk)
                CALLBACK(lefCallbacks->CaseSensitiveCbk, 
                         lefrCaseSensitiveCbkType,
                         lefData->namesCaseSensitive);
              lefData->hasNameCase = 1;
            } else
              if (lefCallbacks->CaseSensitiveCbk) // write warning only if cbk is set 
                 if (lefData->caseSensitiveWarnings++ < lefSettings->CaseSensitiveWarnings)
                   lefWarning(2007, "NAMESCASESENSITIVE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
          }
      | K_NAMESCASESENSITIVE K_OFF ';'
          {
            if (lefData->versionNum < 5.6) {
              lefData->namesCaseSensitive = FALSE;
              if (lefCallbacks->CaseSensitiveCbk)
                CALLBACK(lefCallbacks->CaseSensitiveCbk, lefrCaseSensitiveCbkType,
                               lefData->namesCaseSensitive);
              lefData->hasNameCase = 1;
            } else {
              if (lefCallbacks->CaseSensitiveCbk) { // write error only if cbk is set 
                if (lefData->caseSensitiveWarnings++ < lefSettings->CaseSensitiveWarnings) {
                  lefError(1504, "NAMESCASESENSITIVE statement is set with OFF.\nStarting version 5.6, NAMESCASENSITIVE is obsolete,\nif it is defined, it has to have the ON value.\nParser will stop processing.");
                  CHKERR();
                }
              }
            }
          }

wireextension: K_NOWIREEXTENSIONATPIN K_ON ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->NoWireExtensionCbk)
          CALLBACK(lefCallbacks->NoWireExtensionCbk, lefrNoWireExtensionCbkType, "ON");
      } else
        if (lefCallbacks->NoWireExtensionCbk) // write warning only if cbk is set 
           if (lefData->noWireExtensionWarnings++ < lefSettings->NoWireExtensionWarnings)
             lefWarning(2008, "NOWIREEXTENSIONATPIN statement is obsolete in version 5.6 or later.\nThe NOWIREEXTENSIONATPIN statement will be ignored.");
    }
  | K_NOWIREEXTENSIONATPIN K_OFF ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->NoWireExtensionCbk)
          CALLBACK(lefCallbacks->NoWireExtensionCbk, lefrNoWireExtensionCbkType, "OFF");
      } else
        if (lefCallbacks->NoWireExtensionCbk) // write warning only if cbk is set 
           if (lefData->noWireExtensionWarnings++ < lefSettings->NoWireExtensionWarnings)
             lefWarning(2008, "NOWIREEXTENSIONATPIN statement is obsolete in version 5.6 or later.\nThe NOWIREEXTENSIONATPIN statement will be ignored.");
    }

fixedmask: K_FIXEDMASK ';'
    { 
       if (lefData->versionNum >= 5.8) {
       
          if (lefCallbacks->FixedMaskCbk) {
            lefData->lefFixedMask = 1;
            CALLBACK(lefCallbacks->FixedMaskCbk, lefrFixedMaskCbkType, lefData->lefFixedMask);
          }
          
          lefData->hasFixedMask = 1;
       }
    }
    
manufacturing: K_MANUFACTURINGGRID int_number ';'
    {
      if (lefCallbacks->ManufacturingCbk)
        CALLBACK(lefCallbacks->ManufacturingCbk, lefrManufacturingCbkType, $2);
      lefData->hasManufactur = 1;
    }

useminspacing: K_USEMINSPACING spacing_type spacing_value ';'
  {
    if ((strcmp($2, "PIN") == 0) && (lefData->versionNum >= 5.6)) {
      if (lefCallbacks->UseMinSpacingCbk) // write warning only if cbk is set 
         if (lefData->useMinSpacingWarnings++ < lefSettings->UseMinSpacingWarnings)
            lefWarning(2009, "USEMINSPACING PIN statement is obsolete in version 5.6 or later.\n The USEMINSPACING PIN statement will be ignored.");
    } else {
        if (lefCallbacks->UseMinSpacingCbk) {
          lefData->lefrUseMinSpacing.set($2, $3);
          CALLBACK(lefCallbacks->UseMinSpacingCbk, lefrUseMinSpacingCbkType,
                   &lefData->lefrUseMinSpacing);
      }
    }
  }

clearancemeasure: K_CLEARANCEMEASURE clearance_type ';'
    { CALLBACK(lefCallbacks->ClearanceMeasureCbk, lefrClearanceMeasureCbkType, $2); }

clearance_type:
  K_MAXXY   {$$ = (char*)"MAXXY";}
  | K_EUCLIDEAN   {$$ = (char*)"EUCLIDEAN";}

spacing_type:
  K_OBS     {$$ = (char*)"OBS";}
  | K_PIN   {$$ = (char*)"PIN";}

spacing_value:
  K_ON      {$$ = 1;}
  | K_OFF   {$$ = 0;}

units_section: start_units units_rules K_END K_UNITS
    { 
      if (lefCallbacks->UnitsCbk)
        CALLBACK(lefCallbacks->UnitsCbk, lefrUnitsCbkType, &lefData->lefrUnits);
    }

start_units: K_UNITS
    {
      lefData->lefrUnits.clear();
      if (lefData->hasManufactur) {
        if (lefData->unitsWarnings++ < lefSettings->UnitsWarnings) {
          lefError(1505, "MANUFACTURINGGRID statement was defined before UNITS.\nRefer the LEF Language Reference manual for the order of LEF statements.");
          CHKERR();
        }
      }
      if (lefData->hasMinfeature) {
        if (lefData->unitsWarnings++ < lefSettings->UnitsWarnings) {
          lefError(1712, "MINFEATURE statement was defined before UNITS.\nRefer the LEF Language Reference manual for the order of LEF statements.");
          CHKERR();
        }
      }
      if (lefData->versionNum < 5.6) {
        if (lefData->hasSite) {//SITE is defined before UNIT and is illegal in pre 5.6
          lefError(1713, "SITE statement was defined before UNITS.\nRefer the LEF Language Reference manual for the order of LEF statements.");
          CHKERR();
        }
      }
    }

units_rules: 
  | units_rules units_rule
  ;

units_rule: K_TIME K_NANOSECONDS int_number ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setTime($3); }
  | K_CAPACITANCE K_PICOFARADS int_number ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setCapacitance($3); }
  | K_RESISTANCE K_OHMS int_number ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setResistance($3); }
  | K_POWER K_MILLIWATTS int_number ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setPower($3); }
  | K_CURRENT K_MILLIAMPS int_number ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setCurrent($3); }
  | K_VOLTAGE K_VOLTS int_number ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setVoltage($3); }
  | K_DATABASE K_MICRONS int_number ';'
    { 
      if(validNum((int)$3)) {
         if (lefCallbacks->UnitsCbk)
            lefData->lefrUnits.setDatabase("MICRONS", $3);
      }
    }
  | K_FREQUENCY K_MEGAHERTZ NUMBER ';'
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setFrequency($3); }

layer_rule: start_layer 
    layer_options end_layer
    { 
      if (lefCallbacks->LayerCbk)
        CALLBACK(lefCallbacks->LayerCbk, lefrLayerCbkType, &lefData->lefrLayer);
    }

start_layer: K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING
    { 
      if (lefData->lefrHasMaxVS) {   // 5.5 
        if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
          if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
            lefError(1506, "A MAXVIASTACK statement is defined before the LAYER statement.\nRefer to the LEF Language Reference manual for the order of LEF statements.");
            CHKERR();
          }
        }
      }
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setName($3);
      lefData->useLenThr = 0;
      lefData->layerCut = 0;
      lefData->layerMastOver = 0;
      lefData->layerRout = 0;
      lefData->layerDir = 0;
      lefData->lefrHasLayer = 1;
      //strcpy(lefData->layerName, $3);
      lefData->layerName = strdup($3);
      lefData->hasType = 0;
      lefData->hasMask = 0;
      lefData->hasPitch = 0;
      lefData->hasWidth = 0;
      lefData->hasDirection = 0;
      lefData->hasParallel = 0;
      lefData->hasInfluence = 0;
      lefData->hasTwoWidths = 0;
      lefData->lefrHasSpacingTbl = 0;
      lefData->lefrHasSpacing = 0;
    }

end_layer: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING
    { 
      if (strcmp(lefData->layerName, $3) != 0) {
        if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
          if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
             lefData->outMsg = (char*)lefMalloc(10000);
             sprintf (lefData->outMsg,
                "END LAYER name %s is different from the LAYER name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->layerName);
             lefError(1507, lefData->outMsg);
             lefFree(lefData->outMsg);
             lefFree(lefData->layerName);
             CHKERR(); 
          } else
             lefFree(lefData->layerName);
        } else
          lefFree(lefData->layerName);
      } else
        lefFree(lefData->layerName);
      if (!lefSettings->RelaxMode) {
        if (lefData->hasType == 0) {
          if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1508, "TYPE statement is a required statement in a LAYER and it is not defined.");
               CHKERR(); 
            }
          }
        }
        if ((lefData->layerRout == 1) && (lefData->hasPitch == 0)) {
          if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1509, "PITCH statement is a required statement in a LAYER with type ROUTING and it is not defined.");
              CHKERR(); 
            }
          }
        }
        if ((lefData->layerRout == 1) && (lefData->hasWidth == 0)) {
          if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1510, "WIDTH statement is a required statement in a LAYER with type ROUTING and it is not defined.");
              CHKERR(); 
            }
          }
        }
        if ((lefData->layerRout == 1) && (lefData->hasDirection == 0)) {
          if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg, "The DIRECTION statement which is required in a LAYER with TYPE ROUTING is not defined in LAYER %s.\nUpdate your lef file and add the DIRECTION statement for layer %s.", $3, $3);
              lefError(1511, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR(); 
            }
          }
        }
      }
    }

layer_options:
    { }
  | layer_options layer_option    // Use left recursions 
    { }

layer_option:
  K_ARRAYSPACING                   // 5.7 
    {
       // let setArraySpacingCutSpacing to set the data 
    }
    layer_arraySpacing_long
    layer_arraySpacing_width
    K_CUTSPACING int_number
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.setArraySpacingCut($6);
         lefData->arrayCutsVal = 0;
      }
    }
    layer_arraySpacing_arraycuts ';'
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "ARRAYSPACING is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1685, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      }
    }
  | K_TYPE layer_type ';'
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.setType($2);
      lefData->hasType = 1;
    }
  | K_MASK int_number ';'
    {
      if (lefData->versionNum < 5.8) {
          if (lefData->layerWarnings++ < lefSettings->ViaWarnings) {
              lefError(2081, "MASK information can only be defined with version 5.8");
              CHKERR(); 
          }           
      } else {
          if (lefCallbacks->LayerCbk) {
            lefData->lefrLayer.setMask((int)$2);
          }
          
          lefData->hasMask = 1;
      }
    }
  | K_PITCH int_number ';'
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setPitch($2);
      lefData->hasPitch = 1;  
    }
  | K_PITCH int_number int_number ';'
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setPitchXY($2, $3);
      lefData->hasPitch = 1;  
    }
  | K_DIAGPITCH int_number ';'
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagPitch($2);
    }
  | K_DIAGPITCH int_number int_number ';'
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagPitchXY($2, $3);
    }
  | K_OFFSET int_number ';'
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setOffset($2);
    }
  | K_OFFSET int_number int_number ';'
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setOffsetXY($2, $3);
    }
  | K_DIAGWIDTH int_number ';'
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagWidth($2);
    }
  | K_DIAGSPACING int_number ';'
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagSpacing($2);
    }
  | K_WIDTH int_number ';'    // CUT & ROUTING
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setWidth($2);
      lefData->hasWidth = 1;  
    }
  | K_AREA NUMBER ';'
    {
      // Issue an error is this is defined in masterslice
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1715, "It is incorrect to define an AREA statement in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
               CHKERR();
            }
         }
      }

      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.setArea($2);
      }
    }
  | K_SPACING int_number
    {
      lefData->hasSpCenter = 0;       // reset to 0, only once per spacing is allowed 
      lefData->hasSpSamenet = 0;
      lefData->hasSpParallel = 0;
      lefData->hasSpLayer = 0;
      lefData->layerCutSpacing = $2;  // for error message purpose
      // 11/22/99 - Wanda da Rosa, PCR 283762
      //            Issue an error is this is defined in masterslice
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1512, "It is incorrect to define a SPACING statement in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      // 5.5 either SPACING or SPACINGTABLE, not both for routing layer only
      if (lefData->layerRout) {
        if (lefData->lefrHasSpacingTbl && lefData->versionNum < 5.7) {
           if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
              if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                lefWarning(2010, "It is incorrect to have both SPACING rules & SPACINGTABLE rules within a ROUTING layer");
              }
           }
        }
        if (lefCallbacks->LayerCbk)
           lefData->lefrLayer.setSpacingMin($2);
        lefData->lefrHasSpacing = 1;
      } else { 
        if (lefCallbacks->LayerCbk)
           lefData->lefrLayer.setSpacingMin($2);
      }
    }
    layer_spacing_opts
    layer_spacing_cut_routing ';' {}
  | K_SPACINGTABLE K_ORTHOGONAL K_WITHIN int_number K_SPACING int_number   // 5.7 
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.setSpacingTableOrtho();
      if (lefCallbacks->LayerCbk) // due to converting to C, else, convertor produce 
         lefData->lefrLayer.addSpacingTableOrthoWithin($4, $6);//bad code
    }
    layer_spacingtable_opts ';'
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "SPACINGTABLE ORTHOGONAL is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1694, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      }
    }
  | K_DIRECTION layer_direction ';'
    {
      lefData->layerDir = 1;
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1513, "DIRECTION statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDirection($2);
      lefData->hasDirection = 1;  
    }
  | K_RESISTANCE K_RPERSQ int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1514, "RESISTANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setResistance($3);
    }
  | K_RESISTANCE K_RPERSQ K_PWL '(' res_points ')' ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1515, "RESISTANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
    }
  | K_CAPACITANCE K_CPERSQDIST int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1516, "CAPACITANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCapacitance($3);
    }
  | K_CAPACITANCE K_CPERSQDIST K_PWL '(' cap_points ')' ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1517, "CAPACITANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
    }
  | K_HEIGHT int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1518, "HEIGHT statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setHeight($2);
    }
  | K_WIREEXTENSION int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1519, "WIREEXTENSION statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setWireExtension($2);
    }
  | K_THICKNESS int_number ';'
    {
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1520, "THICKNESS statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setThickness($2);
    }
  | K_SHRINKAGE int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1521, "SHRINKAGE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setShrinkage($2);
    }
  | K_CAPMULTIPLIER int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1522, "CAPMULTIPLIER statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCapMultiplier($2);
    }
  | K_EDGECAPACITANCE int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1523, "EDGECAPACITANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setEdgeCap($2);
    }

  | K_ANTENNALENGTHFACTOR int_number ';'
    { // 5.3 syntax 
      lefData->use5_3 = 1;
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1525, "ANTENNALENGTHFACTOR statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      } else if (lefData->versionNum >= 5.4) {
         if (lefData->use5_4) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                  lefData->outMsg = (char*)lefMalloc(10000);
                  sprintf (lefData->outMsg,
                    "ANTENNALENGTHFACTOR statement is a version 5.3 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNALENGTHFACTOR syntax, which is incorrect.", lefData->versionNum);
                  lefError(1526, lefData->outMsg);
                  lefFree(lefData->outMsg);
                  CHKERR();
               }
            }
         }
      }

      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaLength($2);
    }
  | K_CURRENTDEN int_number ';'
    {
      if (lefData->versionNum < 5.2) {
         if (!lefData->layerRout) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefError(1702, "CURRENTDEN statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
                 CHKERR();
               }
            }
         }
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCurrentDensity($2);
      } else {
         if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
            lefWarning(2079, "CURRENTDEN statement is obsolete in version 5.2 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.2 or later.");
            CHKERR();
         }
      }
    }
  | K_CURRENTDEN K_PWL '(' current_density_pwl_list ')' ';'
    { 
      if (lefData->versionNum < 5.2) {
         if (!lefData->layerRout) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefError(1702, "CURRENTDEN statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
                 CHKERR();
               }
            }
         }
      } else {
         if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
            lefWarning(2079, "CURRENTDEN statement is obsolete in version 5.2 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.2 or later.");
            CHKERR();
         }
      }
    }
  | K_CURRENTDEN '(' int_number int_number ')' ';'
    {
      if (lefData->versionNum < 5.2) {
         if (!lefData->layerRout) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefError(1702, "CURRENTDEN statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
                 CHKERR();
               }
            }
         }
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCurrentPoint($3, $4);
      } else {
         if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
            lefWarning(2079, "CURRENTDEN statement is obsolete in version 5.2 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.2 or later.");
            CHKERR();
         }
      }
    }
  | K_PROPERTY { lefData->lefDumbMode = 10000000;} layer_prop_list ';'
    {
      lefData->lefDumbMode = 0;
    }
  | K_ACCURRENTDENSITY layer_table_type
    {
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1527, "ACCURRENTDENSITY statement can't be defined in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAccurrentDensity($2);      
    }
    layer_frequency {

    }
  | K_ACCURRENTDENSITY layer_table_type int_number ';'
    {
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1527, "ACCURRENTDENSITY statement can't be defined in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) {
           lefData->lefrLayer.addAccurrentDensity($2);
           lefData->lefrLayer.setAcOneEntry($3);
      }
    }
  | K_DCCURRENTDENSITY K_AVERAGE int_number ';'
    {
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1528, "DCCURRENTDENSITY statement can't be defined in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addDccurrentDensity("AVERAGE");
         lefData->lefrLayer.setDcOneEntry($3);
      }
    }
  | K_DCCURRENTDENSITY K_AVERAGE K_CUTAREA NUMBER
    {
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1528, "DCCURRENTDENSITY statement can't be defined in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (!lefData->layerCut) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1529, "CUTAREA statement can only be defined in LAYER with type CUT. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addDccurrentDensity("AVERAGE");
         lefData->lefrLayer.addNumber($4);
      }
    }
    number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addDcCutarea(); }
    dc_layer_table {}
  | K_DCCURRENTDENSITY K_AVERAGE K_WIDTH int_number
    {
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1528, "DCCURRENTDENSITY can't be defined in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1530, "WIDTH statement can only be defined in LAYER with type ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addDccurrentDensity("AVERAGE");
         lefData->lefrLayer.addNumber($4);
      }
    }
    int_number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addDcWidth(); }
    dc_layer_table {}

// 3/23/2000 - 5.4 syntax.  Wanda da Rosa 
  | K_ANTENNAAREARATIO int_number ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNAAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1531, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNADIFFAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNAAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1704, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (!lefData->layerRout && !lefData->layerCut && lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1533, "ANTENNAAREARATIO statement can only be defined in LAYER with TYPE ROUTING or CUT. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaAreaRatio($2);
    }
  | K_ANTENNADIFFAREARATIO
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNADIFFAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1532, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNADIFFAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNADIFFAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1704, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (!lefData->layerRout && !lefData->layerCut && lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1534, "ANTENNADIFFAREARATIO statement can only be defined in LAYER with TYPE ROUTING or CUT. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      lefData->antennaType = lefiAntennaDAR; 
    }
    layer_antenna_pwl ';' {}
  | K_ANTENNACUMAREARATIO int_number ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNACUMAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1535, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNACUMAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNACUMAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1536, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (!lefData->layerRout && !lefData->layerCut && lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1537, "ANTENNACUMAREARATIO statement can only be defined in LAYER with TYPE ROUTING or CUT. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaCumAreaRatio($2);
    }
  | K_ANTENNACUMDIFFAREARATIO
    {  // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNACUMDIFFAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1538, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNACUMDIFFAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNACUMDIFFAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1539, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (!lefData->layerRout && !lefData->layerCut && lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1540, "ANTENNACUMDIFFAREARATIO statement can only be defined in LAYER with TYPE ROUTING or CUT. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      lefData->antennaType = lefiAntennaCDAR;
    }
    layer_antenna_pwl ';' {} 
  | K_ANTENNAAREAFACTOR int_number
    { // both 5.3  & 5.4 syntax 
      if (!lefData->layerRout && !lefData->layerCut && lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1541, "ANTENNAAREAFACTOR can only be defined in LAYER with TYPE ROUTING or CUT. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      // this does not need to check, since syntax is in both 5.3 & 5.4 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaAreaFactor($2);
      lefData->antennaType = lefiAntennaAF;
    }
    layer_antenna_duo ';' {}
  | K_ANTENNASIDEAREARATIO int_number ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1542, "ANTENNASIDEAREARATIO can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNASIDEAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1543, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNASIDEAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNASIDEAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1544, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaSideAreaRatio($2);
    }
  | K_ANTENNADIFFSIDEAREARATIO
    {  // 5.4 syntax 
      lefData->use5_4 = 1;
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1545, "ANTENNADIFFSIDEAREARATIO can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNADIFFSIDEAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1546, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNADIFFSIDEAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNADIFFSIDEAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1547, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      lefData->antennaType = lefiAntennaDSAR;
    }
    layer_antenna_pwl ';' {}
  | K_ANTENNACUMSIDEAREARATIO int_number ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1548, "ANTENNACUMSIDEAREARATIO can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNACUMSIDEAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1549, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNACUMSIDEAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNACUMSIDEAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1550, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaCumSideAreaRatio($2);
    }
  | K_ANTENNACUMDIFFSIDEAREARATIO
    {  // 5.4 syntax 
      lefData->use5_4 = 1;
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1551, "ANTENNACUMDIFFSIDEAREARATIO can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNACUMDIFFSIDEAREARATIO statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1552, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNACUMDIFFSIDEAREARATIO statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNACUMDIFFSIDEAREARATIO syntax, which is incorrect.", lefData->versionNum);
               lefError(1553, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      lefData->antennaType = lefiAntennaCDSAR;
    }
    layer_antenna_pwl ';' {}
  | K_ANTENNASIDEAREAFACTOR int_number
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1554, "ANTENNASIDEAREAFACTOR can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNASIDEAREAFACTOR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1555, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNASIDEAREAFACTOR statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNASIDEAREAFACTOR syntax, which is incorrect.", lefData->versionNum);
               lefError(1556, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaSideAreaFactor($2);
      lefData->antennaType = lefiAntennaSAF;
    }
    layer_antenna_duo ';' {}
  | K_ANTENNAMODEL // 5.5 
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (!lefData->layerRout && !lefData->layerCut && lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1557, "ANTENNAMODEL can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNAMODEL statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1558, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->use5_3) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "ANTENNAMODEL statement is a version 5.4 or earlier syntax.\nYour lef file with version %g, has both old and new ANTENNAMODEL syntax, which is incorrect.", lefData->versionNum);
               lefError(1559, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      lefData->antennaType = lefiAntennaO;
    }
    layer_oxide ';' {}
  | K_ANTENNACUMROUTINGPLUSCUT ';'        // 5.7 
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "ANTENNACUMROUTINGPLUSCUT is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1686, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      } else {
         if (!lefData->layerRout && !lefData->layerCut) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                  lefError(1560, "ANTENNACUMROUTINGPLUSCUT can only be defined in LAYER with type ROUTING or CUT. Parser will stop processing.");
                  CHKERR();
               }
            }
         }
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaCumRoutingPlusCut();
      }
    }
  | K_ANTENNAGATEPLUSDIFF int_number ';'      // 5.7 
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "ANTENNAGATEPLUSDIFF is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1687, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      } else {
         if (!lefData->layerRout && !lefData->layerCut) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                  lefError(1561, "ANTENNAGATEPLUSDIFF can only be defined in LAYER with type ROUTING or CUT. Parser will stop processing.");
                  CHKERR();
               }
            }
         }
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaGatePlusDiff($2);
      }
    }
  | K_ANTENNAAREAMINUSDIFF int_number ';'     // 5.7 
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "ANTENNAAREAMINUSDIFF is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1688, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      } else {
         if (!lefData->layerRout && !lefData->layerCut) {
            if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                  lefError(1562, "ANTENNAAREAMINUSDIFF can only be defined in LAYER with type ROUTING or CUT. Parser will stop processing.");
                  CHKERR();
               }
            }
         }
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaAreaMinusDiff($2);
      }
    }
  | K_ANTENNAAREADIFFREDUCEPWL '(' pt pt            // 5.7 
    {
      if (!lefData->layerRout && !lefData->layerCut) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1563, "ANTENNAAREADIFFREDUCEPWL can only be defined in LAYER with type ROUTING or CUT. Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) { // require min 2 points, set the 1st 2 
         if (lefData->lefrAntennaPWLPtr) {
            lefData->lefrAntennaPWLPtr->Destroy();
            lefFree(lefData->lefrAntennaPWLPtr);
         }

         lefData->lefrAntennaPWLPtr = lefiAntennaPWL::create();
         lefData->lefrAntennaPWLPtr->addAntennaPWL($3.x, $3.y);
         lefData->lefrAntennaPWLPtr->addAntennaPWL($4.x, $4.y);
      }
    } 
    layer_diffusion_ratios ')' ';'
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setAntennaPWL(lefiAntennaADR, lefData->lefrAntennaPWLPtr);
        lefData->lefrAntennaPWLPtr = NULL;
      }
    }
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "ANTENNAAREADIFFREDUCEPWL is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1689, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      }
    }
  | K_SLOTWIREWIDTH int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireWidth($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2011, "SLOTWIREWIDTH statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "SLOTWIREWIDTH statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1564, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireWidth($2);
    }
  | K_SLOTWIRELENGTH int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireLength($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2012, "SLOTWIRELENGTH statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "SLOTWIRELENGTH statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1565, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireLength($2);
    }
  | K_SLOTWIDTH int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWidth($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2013, "SLOTWIDTH statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "SLOTWIDTH statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1566, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWidth($2);
    }
  | K_SLOTLENGTH int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotLength($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2014, "SLOTLENGTH statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "SLOTLENGTH statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1567, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotLength($2);
    }
  | K_MAXADJACENTSLOTSPACING int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxAdjacentSlotSpacing($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2015, "MAXADJACENTSLOTSPACING statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MAXADJACENTSLOTSPACING statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1568, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxAdjacentSlotSpacing($2);
    }
  | K_MAXCOAXIALSLOTSPACING int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxCoaxialSlotSpacing($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
                lefWarning(2016, "MAXCOAXIALSLOTSPACING statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MAXCOAXIALSLOTSPACING statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1569, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxCoaxialSlotSpacing($2);
    }
  | K_MAXEDGESLOTSPACING int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxEdgeSlotSpacing($2);
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2017, "MAXEDGESLOTSPACING statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MAXEDGESLOTSPACING statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1570, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxEdgeSlotSpacing($2);
    }
  | K_SPLITWIREWIDTH int_number ';'
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum >= 5.7) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
               lefWarning(2018, "SPLITWIREWIDTH statement is obsolete in version 5.7 or later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.7 or later.");
         }
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "SPLITWIREWIDTH statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1571, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSplitWireWidth($2);
    }
  | K_MINIMUMDENSITY int_number ';'
    { // 5.4 syntax, pcr 394389 
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MINIMUMDENSITY statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1572, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMinimumDensity($2);
    }
  | K_MAXIMUMDENSITY int_number ';'
    { // 5.4 syntax, pcr 394389 
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MAXIMUMDENSITY statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1573, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaximumDensity($2);
    }
  | K_DENSITYCHECKWINDOW int_number int_number ';'
    { // 5.4 syntax, pcr 394389 
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "DENSITYCHECKWINDOW statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1574, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDensityCheckWindow($2, $3);
    }
  | K_DENSITYCHECKSTEP int_number ';'
    { // 5.4 syntax, pcr 394389 
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "DENSITYCHECKSTEP statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1575, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDensityCheckStep($2);
    }
  | K_FILLACTIVESPACING int_number ';'
    { // 5.4 syntax, pcr 394389 
      if (lefData->ignoreVersion) {
         // do nothing 
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "FILLACTIVESPACING statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1576, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setFillActiveSpacing($2);
    }
  | K_MAXWIDTH int_number ';'              // 5.5 
    {
      // 5.5 MAXWIDTH, is for routing layer only
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefError(1577, "MAXWIDTH statement can only be defined in LAYER with TYPE ROUTING.  Parser will stop processing.");
               CHKERR();
            }
         }
      }
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MAXWIDTH statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1578, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxwidth($2);
    }
  | K_MINWIDTH int_number ';'              // 5.5 
    {
      // 5.5 MINWIDTH, is for routing layer only
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1579, "MINWIDTH statement can only be defined in LAYER with TYPE ROUTING.  Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MINWIDTH statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1580, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMinwidth($2);
    }
  | K_MINENCLOSEDAREA NUMBER           // 5.5 
    {
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "MINENCLOSEDAREA statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1581, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinenclosedarea($2);
    }
    layer_minen_width ';' {}
  | K_MINIMUMCUT int_number K_WIDTH int_number // 5.5 
    { // pcr 409334 
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addMinimumcut((int)$2, $4); 
      lefData->hasLayerMincut = 0;
    }
    layer_minimumcut_within
    layer_minimumcut_from
    layer_minimumcut_length ';'
    {
      if (!lefData->hasLayerMincut) {   // FROMABOVE nor FROMBELOW is set 
         if (lefCallbacks->LayerCbk)
             lefData->lefrLayer.addMinimumcutConnect((char*)"");
      }
    }
  | K_MINSTEP int_number               // 5.5 
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstep($2);
    }
    layer_minstep_options ';'      // 5.6 
    {
    }
  | K_PROTRUSIONWIDTH int_number K_LENGTH int_number K_WIDTH int_number ';'  // 5.5 
    {
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "PROTRUSION RULE statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1582, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setProtrusion($2, $4, $6);
    }
  | K_SPACINGTABLE                    // 5.5 
    {
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "SPACINGTABLE statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1583, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      // 5.5 either SPACING or SPACINGTABLE in a layer, not both
      if (lefData->lefrHasSpacing && lefData->layerRout && lefData->versionNum < 5.7) {
         if (lefCallbacks->LayerCbk)  // write warning only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefWarning(2010, "It is incorrect to have both SPACING rules & SPACINGTABLE rules within a ROUTING layer");
            }
      } 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpacingTable();
      lefData->lefrHasSpacingTbl = 1;
    }
    sp_options ';' {}
  // 10/12/2003 - 5.6 syntax 
  | K_ENCLOSURE layer_enclosure_type_opt int_number int_number
    {
      if (lefData->versionNum < 5.6) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ENCLOSURE statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1584, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addEnclosure($2, $3, $4);
    }
    layer_enclosure_width_opt ';' {}
  // 12/30/2003 - 5.6 syntax 
  | K_PREFERENCLOSURE layer_enclosure_type_opt int_number int_number
    {
      if (lefData->versionNum < 5.6) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "PREFERENCLOSURE statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1585, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addPreferEnclosure($2, $3, $4);
    }
    layer_preferenclosure_width_opt ';' {}
  | K_RESISTANCE int_number ';'
    {
      if (lefData->versionNum < 5.6) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "RESISTANCE statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1586, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else {
         if (lefCallbacks->LayerCbk)
            lefData->lefrLayer.setResPerCut($2);
      }
    }
  | K_DIAGMINEDGELENGTH int_number ';'
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1587, "DIAGMINEDGELENGTH can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      } else if (lefData->versionNum < 5.6) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "DIAGMINEDGELENGTH statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1588, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else {
         if (lefCallbacks->LayerCbk)
            lefData->lefrLayer.setDiagMinEdgeLength($2);
      }
    }
  | K_MINSIZE
    {
      // Use the polygon code to retrieve the points for MINSIZE
      lefData->lefrGeometriesPtr = (lefiGeometries*)lefMalloc(sizeof(lefiGeometries));
      lefData->lefrGeometriesPtr->Init();
      lefData->lefrDoGeometries = 1;
    }
    firstPt otherPts ';' 
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrGeometriesPtr->addPolygon();
         lefData->lefrLayer.setMinSize(lefData->lefrGeometriesPtr);
      }
     lefData->lefrDoGeometries = 0;
      lefData->lefrGeometriesPtr->Destroy();
      lefFree(lefData->lefrGeometriesPtr);
    }

layer_arraySpacing_long:            // 5.7
  // empty 
  | K_LONGARRAY
    {
        if (lefCallbacks->LayerCbk)
           lefData->lefrLayer.setArraySpacingLongArray();
    }

layer_arraySpacing_width:           // 5.7
  // empty 
  | K_WIDTH int_number
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.setArraySpacingWidth($2);
    }

layer_arraySpacing_arraycuts:       // 5.7
  // empty 
  | layer_arraySpacing_arraycut layer_arraySpacing_arraycuts

layer_arraySpacing_arraycut:
  K_ARRAYCUTS int_number K_SPACING int_number
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addArraySpacingArray((int)$2, $4);
         if (lefData->arrayCutsVal > (int)$2) {
            // Mulitiple ARRAYCUTS value needs to me in ascending order 
            if (!lefData->arrayCutsWar) {
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
                  lefWarning(2080, "The number of cut values in multiple ARRAYSPACING ARRAYCUTS are not in increasing order.\nTo be consistent with the documentation, update the cut values to increasing order.");
               lefData->arrayCutsWar = 1;
            }
         }
         lefData->arrayCutsVal = (int)$2;
    }

sp_options:
  K_PARALLELRUNLENGTH int_number
    { 
      if (lefData->hasInfluence) {  // 5.5 - INFLUENCE table must follow a PARALLEL
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1589, "An INFLUENCE table statement was defined before the PARALLELRUNLENGTH table statement.\nINFLUENCE table statement should be defined following the PARALLELRUNLENGTH.\nChange the LEF file and rerun the parser.");
              CHKERR();
            }
         }
      }
      if (lefData->hasParallel) { // 5.5 - Only one PARALLEL table is allowed per layer
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1590, "There is multiple PARALLELRUNLENGTH table statements are defined within a layer.\nAccording to the LEF Reference Manual, only one PARALLELRUNLENGTH table statement is allowed per layer.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2);
      lefData->hasParallel = 1;
    }
    int_number_list
    {
      lefData->spParallelLength = lefData->lefrLayer.getNumber();
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpParallelLength();
    }
    K_WIDTH int_number
    { 
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addSpParallelWidth($7);
      }
    }
    int_number_list
    { 
      if (lefData->lefrLayer.getNumber() != lefData->spParallelLength) {
         if (lefCallbacks->LayerCbk) {
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1591, "The number of length in the PARALLELRUNLENGTH statement is not equal to\nthe total number of spacings defined in the WIDTH statement in the SPACINGTABLE.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpParallelWidthSpacing();
    }
    layer_sp_parallel_widths

  | K_TWOWIDTHS K_WIDTH int_number layer_sp_TwoWidthsPRL int_number
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($5);
    }
    int_number_list
    {
      if (lefData->hasParallel) { // 5.7 - Either PARALLEL OR TWOWIDTHS per layer
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1592, "A PARALLELRUNLENGTH statement was already defined in the layer.\nIt is PARALLELRUNLENGTH or TWOWIDTHS is allowed per layer.");
              CHKERR();
            }
         }
      }
      if (lefData->hasTwoWidths) { // 5.7 - only 1 TWOWIDTHS per layer
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1593, "A TWOWIDTHS table statement was already defined in the layer.\nOnly one TWOWIDTHS statement is allowed per layer.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpTwoWidths($3, $4);
      lefData->hasTwoWidths = 1;
    }
    layer_sp_TwoWidths
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "TWOWIDTHS is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1697, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } 
    }
  | K_INFLUENCE K_WIDTH int_number K_WITHIN int_number K_SPACING int_number
    {
      if (lefData->hasInfluence) {  // 5.5 - INFLUENCE table must follow a PARALLEL
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1594, "A INFLUENCE table statement was already defined in the layer.\nOnly one INFLUENCE statement is allowed per layer.");
              CHKERR();
            }
         }
      }
      if (!lefData->hasParallel) {  // 5.5 - INFLUENCE must follow a PARALLEL
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1595, "An INFLUENCE table statement was already defined before the layer.\nINFLUENCE statement has to be defined after the PARALLELRUNLENGTH table statement in the layer.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.setInfluence();
         lefData->lefrLayer.addSpInfluence($3, $5, $7);
      }
    }
    layer_sp_influence_widths

layer_spacingtable_opts:      // 5.7 
  // empty 
  | layer_spacingtable_opt layer_spacingtable_opts

layer_spacingtable_opt:
  K_WITHIN int_number K_SPACING int_number
  {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addSpacingTableOrthoWithin($2, $4);
  }

layer_enclosure_type_opt: 
    {$$ = (char*)"NULL";}   // empty 
  | K_ABOVE  {$$ = (char*)"ABOVE";}
  | K_BELOW  {$$ = (char*)"BELOW";}

layer_enclosure_width_opt:  // empty 
  | K_WIDTH int_number
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addEnclosureWidth($2);
      }
    }
  layer_enclosure_width_except_opt
  | K_LENGTH int_number              // 5.7 
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "LENGTH is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1691, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      } else {
         if (lefCallbacks->LayerCbk) {
            lefData->lefrLayer.addEnclosureLength($2);
         }
      }
    }
    
layer_enclosure_width_except_opt: // empty 
  | K_EXCEPTEXTRACUT int_number       // 5.7 
    {
      if (lefData->versionNum < 5.7) {
         lefData->outMsg = (char*)lefMalloc(10000);
         sprintf(lefData->outMsg,
           "EXCEPTEXTRACUT is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
         lefError(1690, lefData->outMsg);
         lefFree(lefData->outMsg);
         CHKERR();
      } else {
         if (lefCallbacks->LayerCbk) {
            lefData->lefrLayer.addEnclosureExceptEC($2);
         }
      }
    }

layer_preferenclosure_width_opt:  // empty 
  | K_WIDTH int_number
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addPreferEnclosureWidth($2);
      }
    }
    
layer_minimumcut_within: // empty 
  | K_WITHIN int_number
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "MINIMUMCUT WITHIN is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1700, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } else {
         if (lefCallbacks->LayerCbk) {
            lefData->lefrLayer.addMinimumcutWithin($2);
         }
      }
    }

layer_minimumcut_from: // empty 
  | K_FROMABOVE
    {
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "FROMABOVE statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1596, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      lefData->hasLayerMincut = 1;
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addMinimumcutConnect((char*)"FROMABOVE");

    }
  | K_FROMBELOW
    {
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "FROMBELOW statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1597, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      }
      lefData->hasLayerMincut = 1;
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addMinimumcutConnect((char*)"FROMBELOW");
    }

layer_minimumcut_length: // empty 
  | K_LENGTH int_number K_WITHIN int_number
    {   
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "LENGTH WITHIN statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1598, lefData->outMsg);
               lefFree(lefData->outMsg);
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addMinimumcutLengDis($2, $4);
    }

layer_minstep_options: // empty 
  | layer_minstep_options layer_minstep_option  // Use left recursions 

layer_minstep_option:
  layer_minstep_type
  {
    if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstepType($1);
  }
  | K_LENGTHSUM int_number
  {
    if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstepLengthsum($2);
  }
  | K_MAXEDGES int_number                  // 5.7 
  {
    if (lefData->versionNum < 5.7) {
      lefData->outMsg = (char*)lefMalloc(10000);
      sprintf(lefData->outMsg,
        "MAXEDGES is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
      lefError(1710, lefData->outMsg);
      lefFree(lefData->outMsg);
      CHKERR();
    } else
       if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstepMaxedges((int)$2);
  }

layer_minstep_type:
  K_INSIDECORNER {$$ = (char*)"INSIDECORNER";}
  | K_OUTSIDECORNER {$$ = (char*)"OUTSIDECORNER";}
  | K_STEP {$$ = (char*)"STEP";}

layer_antenna_pwl:
  int_number
      { if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setAntennaValue(lefData->antennaType, $1); }
  | K_PWL '(' pt pt
      { if (lefCallbacks->LayerCbk) { // require min 2 points, set the 1st 2 
          if (lefData->lefrAntennaPWLPtr) {
            lefData->lefrAntennaPWLPtr->Destroy();
            lefFree(lefData->lefrAntennaPWLPtr);
          }

          lefData->lefrAntennaPWLPtr = lefiAntennaPWL::create();
          lefData->lefrAntennaPWLPtr->addAntennaPWL($3.x, $3.y);
          lefData->lefrAntennaPWLPtr->addAntennaPWL($4.x, $4.y);
        }
      }
    layer_diffusion_ratios ')'
      { 
        if (lefCallbacks->LayerCbk) {
          lefData->lefrLayer.setAntennaPWL(lefData->antennaType, lefData->lefrAntennaPWLPtr);
          lefData->lefrAntennaPWLPtr = NULL;
        }
      }

layer_diffusion_ratios: // empty 
  | layer_diffusion_ratios layer_diffusion_ratio  // Use left recursions 
  ;

layer_diffusion_ratio:
  pt
  { if (lefCallbacks->LayerCbk)
      lefData->lefrAntennaPWLPtr->addAntennaPWL($1.x, $1.y);
  }

layer_antenna_duo: // empty 
  | K_DIFFUSEONLY
      { 
        lefData->use5_4 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        }
        else if ((lefData->antennaType == lefiAntennaAF) && (lefData->versionNum <= 5.3)) {
           if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
              if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                   "ANTENNAAREAFACTOR with DIFFUSEONLY statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                 lefError(1599, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        } else if (lefData->use5_3) {
           if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
              if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                   "ANTENNAAREAFACTOR with DIFFUSEONLY statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                 lefError(1599, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
        if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setAntennaDUO(lefData->antennaType);
      }

layer_table_type:
  K_PEAK       {$$ = (char*)"PEAK";}
  | K_AVERAGE  {$$ = (char*)"AVERAGE";}
  | K_RMS      {$$ = (char*)"RMS";}

layer_frequency:
  K_FREQUENCY NUMBER
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2); }
  number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcFrequency(); }
  ac_layer_table_opt
  K_TABLEENTRIES NUMBER
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($9); }
    number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcTableEntry(); }

ac_layer_table_opt:  // empty 
  | K_CUTAREA NUMBER
    {
      if (!lefData->layerCut) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1600, "CUTAREA statement can only be defined in LAYER with TYPE CUT.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2);
    }
    number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcCutarea(); }
  | K_WIDTH int_number
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1601, "WIDTH can only be defined in LAYER with TYPE ROUTING.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2);
    }
    int_number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcWidth(); }

dc_layer_table:
  K_TABLEENTRIES int_number
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2); }
    int_number_list ';'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addDcTableEntry(); }

int_number_list:
  | int_number_list int_number
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2); }

number_list:
  | number_list NUMBER
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($2); }

layer_prop_list:
  layer_prop
  | layer_prop_list layer_prop
  ;

layer_prop:
  T_STRING T_STRING
    {
      if (lefCallbacks->LayerCbk) {
        char propTp;
        propTp = lefSettings->lefProps.lefrLayerProp.propType($1);
        lefData->lefrLayer.addProp($1, $2, propTp);
      }
    }
  | T_STRING QSTRING
    {
      if (lefCallbacks->LayerCbk) {
        char propTp;
        propTp = lefSettings->lefProps.lefrLayerProp.propType($1);
        lefData->lefrLayer.addProp($1, $2, propTp);
      }
    }
  | T_STRING NUMBER
    {
      char temp[32];
      sprintf(temp, "%.11g", $2);
      if (lefCallbacks->LayerCbk) {
        char propTp;
        propTp = lefSettings->lefProps.lefrLayerProp.propType($1);
        lefData->lefrLayer.addNumProp($1, $2, temp, propTp);
      }
    }

current_density_pwl_list :
  current_density_pwl
    { }
  | current_density_pwl_list current_density_pwl
    { }

current_density_pwl: '(' int_number int_number ')'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCurrentPoint($2, $3); }

cap_points :
  cap_point
  | cap_points cap_point
  ;

cap_point: '(' int_number int_number ')'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCapacitancePoint($2, $3); }

res_points :
  res_point
  | res_points res_point
    { }

res_point: '(' int_number int_number ')'
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.setResistancePoint($2, $3); }

layer_type:
  K_ROUTING       {$$ = (char*)"ROUTING"; lefData->layerRout = 1;}
  | K_CUT         {$$ = (char*)"CUT"; lefData->layerCut = 1;}
  | K_OVERLAP     {$$ = (char*)"OVERLAP"; lefData->layerMastOver = 1;}
  | K_MASTERSLICE {$$ = (char*)"MASTERSLICE"; lefData->layerMastOver = 1;}
  | K_VIRTUAL     {$$ = (char*)"VIRTUAL";}
  | K_IMPLANT     {$$ = (char*)"IMPLANT";}

layer_direction:
  K_HORIZONTAL      {$$ = (char*)"HORIZONTAL";}
  |  K_VERTICAL     {$$ = (char*)"VERTICAL";}
  |  K_DIAG45       {$$ = (char*)"DIAG45";}
  |  K_DIAG135      {$$ = (char*)"DIAG135";}

layer_minen_width:
  | K_WIDTH int_number
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addMinenclosedareaWidth($2);
    }

layer_oxide:
  K_OXIDE1
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(1);
    }
  | K_OXIDE2
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(2);
    }
  | K_OXIDE3
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(3);
    }
  | K_OXIDE4
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(4);
    }

layer_sp_parallel_widths: // empty 
    { }
  | layer_sp_parallel_widths layer_sp_parallel_width  // Use left recursions 
    { }

layer_sp_parallel_width: K_WIDTH int_number
    { 
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addSpParallelWidth($2);
      }
    }
    int_number_list
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpParallelWidthSpacing(); }
 
layer_sp_TwoWidths: // empty               // 5.7
    { }
  | layer_sp_TwoWidth layer_sp_TwoWidths
    { }
    
layer_sp_TwoWidth: K_WIDTH int_number layer_sp_TwoWidthsPRL int_number
    {
       if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber($4);
    }
    int_number_list 
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addSpTwoWidths($2, $3);
    }

layer_sp_TwoWidthsPRL:                       // 5.7
    { 
        $$ = -1; // cannot use 0, since PRL number can be 0 
        lefData->lefrLayer.setSpTwoWidthsHasPRL(0);
    }           
  | K_PRL int_number
    { 
        $$ = $2; 
        lefData->lefrLayer.setSpTwoWidthsHasPRL(1);
    }
 
layer_sp_influence_widths: // empty 
    { }
  | layer_sp_influence_widths layer_sp_influence_width
    { }

layer_sp_influence_width: K_WIDTH int_number K_WITHIN int_number K_SPACING int_number
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpInfluence($2, $4, $6); }

maxstack_via: K_MAXVIASTACK int_number ';'
    {
      if (!lefData->lefrHasLayer) {  // 5.5 
        if (lefCallbacks->MaxStackViaCbk) { // write error only if cbk is set 
           if (lefData->maxStackViaWarnings++ < lefSettings->MaxStackViaWarnings) {
             lefError(1602, "MAXVIASTACK statement has to be defined after the LAYER statement.");
             CHKERR();
           }
        }
      } else if (lefData->lefrHasMaxVS) {
        if (lefCallbacks->MaxStackViaCbk) { // write error only if cbk is set 
           if (lefData->maxStackViaWarnings++ < lefSettings->MaxStackViaWarnings) {
             lefError(1603, "A MAXVIASTACK was already defined.\nOnly one MAXVIASTACK is allowed per lef file.");
             CHKERR();
           }
        }
      } else {
        if (lefCallbacks->MaxStackViaCbk) {
           lefData->lefrMaxStackVia.setMaxStackVia((int)$2);
           CALLBACK(lefCallbacks->MaxStackViaCbk, lefrMaxStackViaCbkType, &lefData->lefrMaxStackVia);
        }
      }
      if (lefData->versionNum < 5.5) {
        if (lefCallbacks->MaxStackViaCbk) { // write error only if cbk is set 
           if (lefData->maxStackViaWarnings++ < lefSettings->MaxStackViaWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "MAXVIASTACK statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1604, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      lefData->lefrHasMaxVS = 1;
    }
  | K_MAXVIASTACK int_number K_RANGE {lefData->lefDumbMode = 2; lefData->lefNoNum= 2;}
    T_STRING T_STRING ';'
    {
      if (!lefData->lefrHasLayer) {  // 5.5 
        if (lefCallbacks->MaxStackViaCbk) { // write error only if cbk is set 
           if (lefData->maxStackViaWarnings++ < lefSettings->MaxStackViaWarnings) {
              lefError(1602, "MAXVIASTACK statement has to be defined after the LAYER statement.");
              CHKERR();
           }
        }
      } else if (lefData->lefrHasMaxVS) {
        if (lefCallbacks->MaxStackViaCbk) { // write error only if cbk is set 
           if (lefData->maxStackViaWarnings++ < lefSettings->MaxStackViaWarnings) {
             lefError(1603, "A MAXVIASTACK was already defined.\nOnly one MAXVIASTACK is allowed per lef file.");
             CHKERR();
           }
        }
      } else {
        if (lefCallbacks->MaxStackViaCbk) {
           lefData->lefrMaxStackVia.setMaxStackVia((int)$2);
           lefData->lefrMaxStackVia.setMaxStackViaRange($5, $6);
           CALLBACK(lefCallbacks->MaxStackViaCbk, lefrMaxStackViaCbkType, &lefData->lefrMaxStackVia);
        }
      }
      lefData->lefrHasMaxVS = 1;
    }

via: start_via  { lefData->hasViaRule_layer = 0; } via_option end_via
    { 
      if (lefCallbacks->ViaCbk) {
        if (lefData->ndRule) 
            lefData->nd->addViaRule(&lefData->lefrVia);
         else 
            CALLBACK(lefCallbacks->ViaCbk, lefrViaCbkType, &lefData->lefrVia);
       }
    }

via_keyword : K_VIA                 //needed to have a VIA named via
     { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }

start_via: via_keyword T_STRING 
    {
      // 0 is nodefault 
      if (lefCallbacks->ViaCbk) lefData->lefrVia.setName($2, 0);
      lefData->viaLayer = 0;
      lefData->numVia++;
      //strcpy(lefData->viaName, $2);
      lefData->viaName = strdup($2);
    }
  | via_keyword T_STRING K_DEFAULT
    {
      // 1 is default 
      if (lefCallbacks->ViaCbk) lefData->lefrVia.setName($2, 1);
      lefData->viaLayer = 0;
      //strcpy(lefData->viaName, $2);
      lefData->viaName = strdup($2);
    }
  | via_keyword T_STRING K_GENERATED
    {
      // 2 is generated 
      if (lefCallbacks->ViaCbk) lefData->lefrVia.setName($2, 2);
      lefData->viaLayer = 0;
      //strcpy(lefData->viaName, $2);
      lefData->viaName = strdup($2);
    }

via_viarule: K_VIARULE {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
  K_CUTSIZE int_number int_number ';'
  K_LAYERS {lefData->lefDumbMode = 3; lefData->lefNoNum = 1; } T_STRING T_STRING T_STRING ';'
  K_CUTSPACING int_number int_number ';'
  K_ENCLOSURE int_number int_number int_number int_number ';'
    {
       if (lefData->versionNum < 5.6) {
         if (lefCallbacks->ViaCbk) { // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "VIARULE statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1709, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
            }
         }
       }  else
          if (lefCallbacks->ViaCbk) lefData->lefrVia.setViaRule($3, $6, $7, $11, $12, $13,
                          $16, $17, $20, $21, $22, $23);
       lefData->viaLayer++;
       lefData->hasViaRule_layer = 1;
    }
  via_viarule_options
  ;

via_viarule_options: // empty 
  | via_viarule_options via_viarule_option
  ;

via_viarule_option: K_ROWCOL int_number int_number ';'
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setRowCol((int)$2, (int)$3);
    }
  | K_ORIGIN int_number int_number ';'
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setOrigin($2, $3);
    }
  | K_OFFSET int_number int_number int_number int_number ';'
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setOffset($2, $3, $4, $5);
    }
  | K_PATTERN {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setPattern($3);
    }
  ;

via_option: via_viarule
  | via_other_options

via_other_options: via_other_option
  via_more_options

via_more_options: // empty 
  | via_more_options via_other_option
  ;

via_other_option:
  via_foreign 
    { }
  | via_layer_rule 
    { }
  | K_RESISTANCE int_number ';'
    { if (lefCallbacks->ViaCbk) lefData->lefrVia.setResistance($2); }
  | K_PROPERTY { lefData->lefDumbMode = 1000000; } via_prop_list ';'
    { lefData->lefDumbMode = 0;
    }
  | K_TOPOFSTACKONLY
    { 
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setTopOfStack();
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
              lefWarning(2019, "TOPOFSTACKONLY statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later");
    }

via_prop_list:
  via_name_value_pair
  | via_prop_list via_name_value_pair
  ;

via_name_value_pair:
  T_STRING NUMBER
    { 
      char temp[32];
      sprintf(temp, "%.11g", $2);
      if (lefCallbacks->ViaCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaProp.propType($1);
         lefData->lefrVia.addNumProp($1, $2, temp, propTp);
      }
    }
  | T_STRING QSTRING
    {
      if (lefCallbacks->ViaCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaProp.propType($1);
         lefData->lefrVia.addProp($1, $2, propTp);
      }
    }
  | T_STRING T_STRING
    {
      if (lefCallbacks->ViaCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaProp.propType($1);
         lefData->lefrVia.addProp($1, $2, propTp);
      }
    }

via_foreign:
  start_foreign ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign($1, 0, 0.0, 0.0, -1);
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign pt ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign($1, 1, $2.x, $2.y, -1);
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign pt orientation ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign($1, 1, $2.x, $2.y, $3);
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign orientation ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign($1, 0, 0.0, 0.0, $2);
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }

start_foreign:        K_FOREIGN {lefData->lefDumbMode = 1; lefData->lefNoNum= 1;} T_STRING
    { $$ = $3; }

orientation:
  K_N         {$$ = 0;}
  | K_W       {$$ = 1;}
  | K_S       {$$ = 2;}
  | K_E       {$$ = 3;}
  | K_FN      {$$ = 4;}
  | K_FW      {$$ = 5;}
  | K_FS      {$$ = 6;}
  | K_FE      {$$ = 7;}
  | K_R0      {$$ = 0;}
  | K_R90     {$$ = 1;}
  | K_R180    {$$ = 2;}
  | K_R270    {$$ = 3;}
  | K_MY      {$$ = 4;}
  | K_MYR90   {$$ = 5;}
  | K_MX      {$$ = 6;}
  | K_MXR90   {$$ = 7;}

via_layer_rule: via_layer via_geometries
    { }

via_layer: K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
      if (lefCallbacks->ViaCbk) lefData->lefrVia.addLayer($3);
      lefData->viaLayer++;
      lefData->hasViaRule_layer = 1;
    }

via_geometries:
  // empty 
  | via_geometries via_geometry 
  ;

via_geometry:
  K_RECT maskColor pt pt ';'
    { 
      if (lefCallbacks->ViaCbk) {
        if (lefData->versionNum < 5.8 && (int)$2 > 0) {
          if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefError(2081, "MASK information can only be defined with version 5.8");
              CHKERR(); 
            }           
        } else {
          lefData->lefrVia.addRectToLayer((int)$2, $3.x, $3.y, $4.x, $4.y);
        }
      }
    }
  | K_POLYGON maskColor                                              // 5.6
    {
      lefData->lefrGeometriesPtr = (lefiGeometries*)lefMalloc(sizeof(lefiGeometries));
      lefData->lefrGeometriesPtr->Init();
      lefData->lefrDoGeometries = 1;
    }
    firstPt nextPt nextPt otherPts ';'
    { 
      if (lefCallbacks->ViaCbk) {
        if (lefData->versionNum < 5.8 && $2 > 0) {
          if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefError(2083, "Color mask information can only be defined with version 5.8.");
              CHKERR(); 
            }           
        } else {
            lefData->lefrGeometriesPtr->addPolygon((int)$2);
            lefData->lefrVia.addPolyToLayer((int)$2, lefData->lefrGeometriesPtr);   // 5.6
        }
      }
      lefData->lefrGeometriesPtr->clearPolyItems(); // free items fields
      lefFree((char*)(lefData->lefrGeometriesPtr)); // Don't need anymore, poly data has
      lefData->lefrDoGeometries = 0;                // copied
    }

end_via: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING 
    { 
      // 10/17/2001 - Wanda da Rosa, PCR 404149
      //              Error if no layer in via
      if (!lefData->viaLayer) {
         if (lefCallbacks->ViaCbk) {  // write error only if cbk is set 
            if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "A LAYER statement is missing in the VIA %s.\nAt least one LAYERis required per VIA statement.", $3);
              lefError(1606, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
            }
         }
      }
      if (strcmp(lefData->viaName, $3) != 0) {
         if (lefCallbacks->ViaCbk) { // write error only if cbk is set 
            if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "END VIA name %s is different from the VIA name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->viaName);
              lefError(1607, lefData->outMsg);
              lefFree(lefData->outMsg);
              lefFree(lefData->viaName);
              CHKERR();
            } else
              lefFree(lefData->viaName);
         } else
            lefFree(lefData->viaName);
      } else
         lefFree(lefData->viaName);
    }

viarule_keyword : K_VIARULE { lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    { 
      if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setName($3);
      lefData->viaRuleLayer = 0;
      //strcpy(lefData->viaRuleName, $3);
      lefData->viaRuleName = strdup($3);
      lefData->isGenerate = 0;
    }

viarule:
  viarule_keyword viarule_layer_list via_names opt_viarule_props end_viarule
    {
      if (lefData->viaRuleLayer == 0 || lefData->viaRuleLayer > 2) {
         if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefError(1608, "A VIARULE statement requires two layers.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->ViaRuleCbk)
        CALLBACK(lefCallbacks->ViaRuleCbk, lefrViaRuleCbkType, &lefData->lefrViaRule);
      // 2/19/2004 - reset the ENCLOSURE overhang values which may be
      // set by the old syntax OVERHANG -- Not necessary, but just incase
      if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.clearLayerOverhang();
    }

viarule_generate:
  viarule_keyword K_GENERATE viarule_generate_default
    {
      lefData->isGenerate = 1;
    }
  viarule_layer_list opt_viarule_props end_viarule
    {
      if (lefData->viaRuleLayer == 0) {
         if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefError(1708, "A VIARULE GENERATE requires three layers.");
              CHKERR();
            }
         }
      } else if ((lefData->viaRuleLayer < 3) && (lefData->versionNum >= 5.6)) {
         if (lefCallbacks->ViaRuleCbk)  // write warning only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
              lefWarning(2021, "turn-via is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
      } else {
         if (lefCallbacks->ViaRuleCbk) {
            lefData->lefrViaRule.setGenerate();
            CALLBACK(lefCallbacks->ViaRuleCbk, lefrViaRuleCbkType, &lefData->lefrViaRule);
         }
      }
      // 2/19/2004 - reset the ENCLOSURE overhang values which may be
      // set by the old syntax OVERHANG
      if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.clearLayerOverhang();
    }

viarule_generate_default:  // optional 
  | K_DEFAULT   // 5.6 syntax
    {
      if (lefData->versionNum < 5.6) {
         if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "DEFAULT statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1605, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
            }
         }
      } else
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setDefault();
    }
 
viarule_layer_list :
  viarule_layer
  | viarule_layer_list viarule_layer
  ;

opt_viarule_props:
  // empty 
  | viarule_props
  ;

viarule_props:
  viarule_prop
  | viarule_props viarule_prop
  ;

viarule_prop: K_PROPERTY { lefData->lefDumbMode = 10000000;} viarule_prop_list ';'
    { lefData->lefDumbMode = 0;
    }

viarule_prop_list:
  viarule_prop
  | viarule_prop_list viarule_prop
  ;

viarule_prop:
  T_STRING T_STRING
    {
      if (lefCallbacks->ViaRuleCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaRuleProp.propType($1);
         lefData->lefrViaRule.addProp($1, $2, propTp);
      }
    }
  | T_STRING QSTRING
    {
      if (lefCallbacks->ViaRuleCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaRuleProp.propType($1);
         lefData->lefrViaRule.addProp($1, $2, propTp);
      }
    }
  | T_STRING NUMBER
    {
      char temp[32];
      sprintf(temp, "%.11g", $2);
      if (lefCallbacks->ViaRuleCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaRuleProp.propType($1);
         lefData->lefrViaRule.addNumProp($1, $2, temp, propTp);
      }
    }

viarule_layer: viarule_layer_name viarule_layer_options
    {
      // 10/18/2001 - Wanda da Rosa PCR 404181
      //              Make sure the 1st 2 layers in viarule has direction
      // 04/28/2004 - PCR 704072 - DIRECTION in viarule generate is
      //              obsolete in 5.6
      if (lefData->versionNum >= 5.6) {
         if (lefData->viaRuleLayer < 2 && !lefData->viaRuleHasDir && !lefData->viaRuleHasEnc &&
             !lefData->isGenerate) {
            if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
               if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
                  lefError(1705, "VIARULE statement in a layer, requires a DIRECTION construct statement.");
                  CHKERR(); 
               }
            }
         }
      } else {
         if (lefData->viaRuleLayer < 2 && !lefData->viaRuleHasDir && !lefData->viaRuleHasEnc &&
             lefData->isGenerate) {
            if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
               if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
                  lefError(1705, "VIARULE statement in a layer, requires a DIRECTION construct statement.");
                  CHKERR(); 
               }
            }
         }
      }
      lefData->viaRuleLayer++;
    }
  ;

via_names:
  // empty 
  | via_names via_name
  ;

via_name: via_keyword T_STRING ';'
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.addViaName($2); }

viarule_layer_name: K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setLayer($3);
      lefData->viaRuleHasDir = 0;
      lefData->viaRuleHasEnc = 0;
    }

viarule_layer_options:
  // empty 
  | viarule_layer_options viarule_layer_option
  ;

viarule_layer_option:
  K_DIRECTION K_HORIZONTAL ';'
    {
      if (lefData->viaRuleHasEnc) {
        if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefError(1706, "An ENCLOSRE statement was already defined in the layer.\nIt is DIRECTION or ENCLOSURE can be specified in a layer.");
              CHKERR();
           }
        }
      } else {
        if ((lefData->versionNum < 5.6) || (!lefData->isGenerate)) {
          if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setHorizontal();
        } else
          if (lefCallbacks->ViaRuleCbk)  // write warning only if cbk is set 
             if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
               lefWarning(2022, "DIRECTION statement in VIARULE is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
      }
      lefData->viaRuleHasDir = 1;
    }
  | K_DIRECTION K_VERTICAL ';'
    { 
      if (lefData->viaRuleHasEnc) {
        if (lefCallbacks->ViaRuleCbk) { // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefError(1706, "An ENCLOSRE statement was already defined in the layer.\nIt is DIRECTION or ENCLOSURE can be specified in a layer.");
              CHKERR();
           }
        }
      } else {
        if ((lefData->versionNum < 5.6) || (!lefData->isGenerate)) {
          if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setVertical();
        } else
          if (lefCallbacks->ViaRuleCbk) // write warning only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
              lefWarning(2022, "DIRECTION statement in VIARULE is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
      }
      lefData->viaRuleHasDir = 1;
    }
  | K_ENCLOSURE int_number int_number ';'    // 5.5 
    {
      if (lefData->versionNum < 5.5) {
         if (lefCallbacks->ViaRuleCbk) { // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "ENCLOSURE statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1707, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
         }
      }
      // 2/19/2004 - Enforced the rule that ENCLOSURE can only be defined
      // in VIARULE GENERATE
      if (!lefData->isGenerate) {
         if (lefCallbacks->ViaRuleCbk) { // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefError(1614, "An ENCLOSURE statement is defined in a VIARULE statement only.\nOVERHANG statement can only be defined in VIARULE GENERATE.");
              CHKERR();
           }
         }
      }
      if (lefData->viaRuleHasDir) {
         if (lefCallbacks->ViaRuleCbk) { // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefError(1609, "A DIRECTION statement was already defined in the layer.\nIt is DIRECTION or ENCLOSURE can be specified in a layer.");
              CHKERR();
           }
         }
      } else {
         if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setEnclosure($2, $3);
      }
      lefData->viaRuleHasEnc = 1;
    }
  | K_WIDTH int_number K_TO int_number ';'
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setWidth($2,$4); }
  | K_RECT pt pt ';'
    { if (lefCallbacks->ViaRuleCbk)
        lefData->lefrViaRule.setRect($2.x, $2.y, $3.x, $3.y); } 
  | K_SPACING int_number K_BY int_number ';'
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setSpacing($2,$4); }
  | K_RESISTANCE int_number ';'
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setResistance($2); }
  | K_OVERHANG int_number ';'
    {
      if (!lefData->viaRuleHasDir) {
         if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
               lefError(1610, "An OVERHANG statement is defined, but the required DIRECTION statement is not yet defined.\nUpdate the LEF file to define the DIRECTION statement before the OVERHANG.");
               CHKERR();
            }
         }
      }
      // 2/19/2004 - Enforced the rule that OVERHANG can only be defined
      // in VIARULE GENERATE after 5.3
      if ((lefData->versionNum > 5.3) && (!lefData->isGenerate)) {
         if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
               lefError(1611, "An OVERHANG statement is defined in a VIARULE statement only.\nOVERHANG statement can only be defined in VIARULE GENERATE.");
               CHKERR();
            }
         }
      }
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setOverhang($2);
      } else {
        if (lefCallbacks->ViaRuleCbk)  // write warning only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
              lefWarning(2023, "OVERHANG statement will be translated into similar ENCLOSURE rule");
        // In 5.6 & later, set it to either ENCLOSURE overhang1 or overhang2
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setOverhangToEnclosure($2);
      }
    }
  | K_METALOVERHANG int_number ';'
    {
      // 2/19/2004 - Enforced the rule that METALOVERHANG can only be defined
      // in VIARULE GENERATE
      if ((lefData->versionNum > 5.3) && (!lefData->isGenerate)) {
         if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
            if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
               lefError(1612, "An METALOVERHANG statement is defined in a VIARULE statement only.\nOVERHANG statement can only be defined in VIARULE GENERATE.");
               CHKERR();
            }
         }
      }
      if (lefData->versionNum < 5.6) {
        if (!lefData->viaRuleHasDir) {
           if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
             if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
                lefError(1613, "An METALOVERHANG statement is defined, but the required DIRECTION statement is not yet defined.\nUpdate the LEF file to define the DIRECTION statement before the OVERHANG.");
                CHKERR();
             } 
           }
        }
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setMetalOverhang($2);
      } else
        if (lefCallbacks->ViaRuleCbk)  // write warning only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
             lefWarning(2024, "METALOVERHANG statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }

end_viarule: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}  T_STRING 
    {
      if ((lefData->isGenerate) && (lefCallbacks->ViaRuleCbk) && lefData->lefrViaRule.numLayers() >= 3) {         
        if (!lefData->lefrViaRule.layer(0)->hasRect() &&
            !lefData->lefrViaRule.layer(1)->hasRect() &&
            !lefData->lefrViaRule.layer(2)->hasRect()) {
            lefData->outMsg = (char*)lefMalloc(10000);
            sprintf (lefData->outMsg, 
                     "VIARULE GENERATE '%s' cut layer definition should have RECT statement.\nCorrect the LEF file before rerunning it through the LEF parser.", 
                      lefData->viaRuleName);
            lefWarning(1714, lefData->outMsg); 
            lefFree(lefData->outMsg);            
            CHKERR();                
        }
      }

      if (strcmp(lefData->viaRuleName, $3) != 0) {
        if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END VIARULE name %s is different from the VIARULE name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->viaRuleName);
              lefError(1615, lefData->outMsg);
              lefFree(lefData->outMsg);
              lefFree(lefData->viaRuleName);
              CHKERR();
           } else
              lefFree(lefData->viaRuleName);
        } else
           lefFree(lefData->viaRuleName);
      } else
        lefFree(lefData->viaRuleName);
    }

spacing_rule: start_spacing spacings end_spacing 
    { }

start_spacing: K_SPACING
    {
      lefData->hasSamenet = 0;
      if ((lefData->versionNum < 5.6) || (!lefData->ndRule)) {
        // if 5.6 and in nondefaultrule, it should not get in here, 
        // it should go to the else statement to write out a warning 
        // if 5.6, not in nondefaultrule, it will get in here 
        // if 5.5 and earlier in nondefaultrule is ok to get in here 
        if (lefData->versionNum >= 5.7) { // will get to this if statement if  
                           // lefData->versionNum is 5.6 and higher but lefData->ndRule = 0 
           if (lefData->spacingWarnings == 0) {  // only print once 
              lefWarning(2077, "A SPACING SAMENET section is defined but it is not legal in a LEF 5.7 version file.\nIt will be ignored which will probably cause real DRC violations to be ignored, and may\ncause false DRC violations to occur.\n\nTo avoid this warning, and correctly handle these DRC rules, you should modify your\nLEF to use the appropriate SAMENET keywords as described in the LEF/DEF 5.7\nmanual under the SPACING statements in the LAYER (Routing) and LAYER (Cut)\nsections listed in the LEF Table of Contents.");
              lefData->spacingWarnings++;
           }
        } else if (lefCallbacks->SpacingBeginCbk && !lefData->ndRule)
          CALLBACK(lefCallbacks->SpacingBeginCbk, lefrSpacingBeginCbkType, 0);
      } else
        if (lefCallbacks->SpacingBeginCbk && !lefData->ndRule)  // write warning only if cbk is set 
           if (lefData->spacingWarnings++ < lefSettings->SpacingWarnings)
             lefWarning(2025, "SAMENET statement in NONDEFAULTRULE is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }

end_spacing: K_END K_SPACING
    {
      if ((lefData->versionNum < 5.6) || (!lefData->ndRule)) {
        if ((lefData->versionNum <= 5.4) && (!lefData->hasSamenet)) {
           lefError(1616, "SAMENET statement is required inside SPACING for any lef file with version 5.4 and earlier, but is not defined in the parsed lef file.");
           CHKERR();
        } else if (lefData->versionNum < 5.7) { // obsolete in 5.7 and later 
           if (lefCallbacks->SpacingEndCbk && !lefData->ndRule)
             CALLBACK(lefCallbacks->SpacingEndCbk, lefrSpacingEndCbkType, 0);
        }
      }
    }

spacings:
  // empty 
  | spacings spacing
  ;

spacing:  samenet_keyword T_STRING T_STRING int_number ';'
    {
      if ((lefData->versionNum < 5.6) || (!lefData->ndRule)) {
        if (lefData->versionNum < 5.7) {
          if (lefCallbacks->SpacingCbk) {
            lefData->lefrSpacing.set($2, $3, $4, 0);
            if (lefData->ndRule)
                lefData->nd->addSpacingRule(&lefData->lefrSpacing);
            else 
                CALLBACK(lefCallbacks->SpacingCbk, lefrSpacingCbkType, &lefData->lefrSpacing);            
          }
        }
      }
    }
  | samenet_keyword T_STRING T_STRING int_number K_STACK ';'
    {
      if ((lefData->versionNum < 5.6) || (!lefData->ndRule)) {
        if (lefData->versionNum < 5.7) {
          if (lefCallbacks->SpacingCbk) {
            lefData->lefrSpacing.set($2, $3, $4, 1);
            if (lefData->ndRule)
                lefData->nd->addSpacingRule(&lefData->lefrSpacing);
            else 
                CALLBACK(lefCallbacks->SpacingCbk, lefrSpacingCbkType, &lefData->lefrSpacing);    
          }
        }
      }
    }

samenet_keyword: K_SAMENET
    // must be followed by two names 
    { lefData->lefDumbMode = 2; lefData->lefNoNum = 2; lefData->hasSamenet = 1; }
     
maskColor:
    // empty 
    { $$ = 0; }
    | K_MASK int_number
    { $$ = (int)$2; }
            
irdrop: start_irdrop ir_tables end_irdrop
    { }

start_irdrop: K_IRDROP
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->IRDropBeginCbk) 
          CALLBACK(lefCallbacks->IRDropBeginCbk, lefrIRDropBeginCbkType, 0);
      } else
        if (lefCallbacks->IRDropBeginCbk) // write warning only if cbk is set 
          if (lefData->iRDropWarnings++ < lefSettings->IRDropWarnings)
            lefWarning(2026, "IRDROP statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }

end_irdrop: K_END K_IRDROP
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->IRDropEndCbk)
          CALLBACK(lefCallbacks->IRDropEndCbk, lefrIRDropEndCbkType, 0);
      }
    }
      

ir_tables:
  // empty 
  | ir_tables ir_table
  ;

ir_table: ir_tablename ir_table_values ';'
    { 
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->IRDropCbk)
          CALLBACK(lefCallbacks->IRDropCbk, lefrIRDropCbkType, &lefData->lefrIRDrop);
      }
    }

ir_table_values:
  // empty 
  | ir_table_values ir_table_value 
  ;

ir_table_value: int_number int_number 
  { if (lefCallbacks->IRDropCbk) lefData->lefrIRDrop.setValues($1, $2); }

ir_tablename: K_TABLE T_STRING
  { if (lefCallbacks->IRDropCbk) lefData->lefrIRDrop.setTableName($2); }

minfeature: K_MINFEATURE int_number int_number ';'
  {
    lefData->hasMinfeature = 1;
    if (lefData->versionNum < 5.4) {
       if (lefCallbacks->MinFeatureCbk) {
         lefData->lefrMinFeature.set($2, $3);
         CALLBACK(lefCallbacks->MinFeatureCbk, lefrMinFeatureCbkType, &lefData->lefrMinFeature);
       }
    } else
       if (lefCallbacks->MinFeatureCbk) // write warning only if cbk is set 
          if (lefData->minFeatureWarnings++ < lefSettings->MinFeatureWarnings)
            lefWarning(2027, "MINFEATURE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }

dielectric: K_DIELECTRIC int_number ';'
  {
    if (lefData->versionNum < 5.4) {
       if (lefCallbacks->DielectricCbk)
         CALLBACK(lefCallbacks->DielectricCbk, lefrDielectricCbkType, $2);
    } else
       if (lefCallbacks->DielectricCbk) // write warning only if cbk is set 
         if (lefData->dielectricWarnings++ < lefSettings->DielectricWarnings)
           lefWarning(2028, "DIELECTRIC statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }

nondefault_rule: K_NONDEFAULTRULE {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
  {
    (void)lefSetNonDefault($3);
    if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.setName($3);
    lefData->ndLayer = 0;
    lefData->ndRule = 1;
    lefData->numVia = 0;
    //strcpy(lefData->nonDefaultRuleName, $3);
    lefData->nonDefaultRuleName = strdup($3);
  }
  nd_hardspacing
  nd_rules {lefData->lefNdRule = 1;} end_nd_rule
  {
    // 10/18/2001 - Wanda da Rosa, PCR 404189
    //              At least 1 layer is required
    if ((!lefData->ndLayer) && (!lefSettings->RelaxMode)) {
       if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
         if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
            lefError(1617, "NONDEFAULTRULE statement requires at least one LAYER statement.");
            CHKERR();
         }
       }
    }
    if ((!lefData->numVia) && (!lefSettings->RelaxMode) && (lefData->versionNum < 5.6)) {
       // VIA is no longer a required statement in 5.6
       if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
         if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
            lefError(1618, "NONDEFAULTRULE statement requires at least one VIA statement.");
            CHKERR();
         }
       }
    }
    if (lefCallbacks->NonDefaultCbk) {
      lefData->lefrNonDefault.end();
      CALLBACK(lefCallbacks->NonDefaultCbk, lefrNonDefaultCbkType, &lefData->lefrNonDefault);
    }
    lefData->ndRule = 0;
    lefData->lefDumbMode = 0;
    (void)lefUnsetNonDefault();
  }

end_nd_rule: K_END
    {
      if ((lefData->nonDefaultRuleName) && (*lefData->nonDefaultRuleName != '\0'))
        lefFree(lefData->nonDefaultRuleName);
    }
  | K_END T_STRING
    {
      if (strcmp(lefData->nonDefaultRuleName, $2) != 0) {
        if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
          if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
             lefData->outMsg = (char*)lefMalloc(10000);
             sprintf (lefData->outMsg,
                "END NONDEFAULTRULE name %s is different from the NONDEFAULTRULE name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $2, lefData->nonDefaultRuleName);
             lefError(1619, lefData->outMsg);
             lefFree(lefData->nonDefaultRuleName);
             lefFree(lefData->outMsg);
             CHKERR();
          } else
             lefFree(lefData->nonDefaultRuleName);
        } else
           lefFree(lefData->nonDefaultRuleName);
      } else
        lefFree(lefData->nonDefaultRuleName);
    }
  ;

nd_hardspacing:
  // empty 
  | K_HARDSPACING ';'   // HARDSPACING is optional in 5.6 
    {
       if (lefData->versionNum < 5.6) {
          if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "HARDSPACING statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1620, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
          }
       } else 
          if (lefCallbacks->NonDefaultCbk)
             lefData->lefrNonDefault.setHardspacing();
    }
  ;

nd_rules: // empty 
  | nd_rules nd_rule
  ;

nd_rule:
  nd_layer
  | via
  | spacing_rule
  | nd_prop
  | usevia
  | useviarule
  | mincuts
  ;

usevia: K_USEVIA T_STRING ';'
    {
       if (lefData->versionNum < 5.6) {
          if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
             lefData->outMsg = (char*)lefMalloc(10000);
             sprintf (lefData->outMsg,
               "USEVIA statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
             lefError(1621, lefData->outMsg);
             lefFree(lefData->outMsg);
             CHKERR();
          }
       } else {
          if (lefCallbacks->NonDefaultCbk)
             lefData->lefrNonDefault.addUseVia($2);
       }
    }

useviarule:  K_USEVIARULE T_STRING ';'
    {
       if (lefData->versionNum < 5.6) {
          if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
             if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
                lefData->outMsg = (char*)lefMalloc(10000);
                sprintf (lefData->outMsg,
                  "USEVIARULE statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                lefError(1622, lefData->outMsg);
                lefFree(lefData->outMsg);
                CHKERR();
             }
          }
       } else {
          if (lefCallbacks->NonDefaultCbk)
             lefData->lefrNonDefault.addUseViaRule($2);
       }
    }

mincuts: K_MINCUTS T_STRING int_number ';'
    {
       if (lefData->versionNum < 5.6) {
          if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
             if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
                lefData->outMsg = (char*)lefMalloc(10000);
                sprintf (lefData->outMsg,
                  "MINCUTS statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                lefError(1623, lefData->outMsg);
                lefFree(lefData->outMsg);
                CHKERR();
             }
          }
       } else {
          if (lefCallbacks->NonDefaultCbk)
             lefData->lefrNonDefault.addMinCuts($2, (int)$3);
       }
    }

nd_prop: K_PROPERTY { lefData->lefDumbMode = 10000000;} nd_prop_list ';'
    { lefData->lefDumbMode = 0;
    }

nd_prop_list:
  nd_prop
  | nd_prop_list nd_prop
  ;

nd_prop:
  T_STRING T_STRING
    {
      if (lefCallbacks->NonDefaultCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrNondefProp.propType($1);
         lefData->lefrNonDefault.addProp($1, $2, propTp);
      }
    }
  | T_STRING QSTRING
    {
      if (lefCallbacks->NonDefaultCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrNondefProp.propType($1);
         lefData->lefrNonDefault.addProp($1, $2, propTp);
      }
    }
  | T_STRING NUMBER
    {
      if (lefCallbacks->NonDefaultCbk) {
         char temp[32];
         char propTp;
         sprintf(temp, "%.11g", $2);
         propTp = lefSettings->lefProps.lefrNondefProp.propType($1);
         lefData->lefrNonDefault.addNumProp($1, $2, temp, propTp);
      }
    }

nd_layer: K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
  {
    if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.addLayer($3);
    lefData->ndLayer++;
    //strcpy(lefData->layerName, $3);
    lefData->layerName = strdup($3);
    lefData->ndLayerWidth = 0;
    lefData->ndLayerSpace = 0;
  }
  K_WIDTH int_number ';'
  { 
    lefData->ndLayerWidth = 1;
    if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.addWidth($6);
  }
  nd_layer_stmts K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
  {
    if (strcmp(lefData->layerName, $12) != 0) {
      if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
         if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
            lefData->outMsg = (char*)lefMalloc(10000);
            sprintf (lefData->outMsg,
               "END LAYER name %s is different from the LAYER name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->layerName);
            lefError(1624, lefData->outMsg);
            lefFree(lefData->outMsg);
            lefFree(lefData->layerName);
            CHKERR();
         } else
            lefFree(lefData->layerName);
      } else
         lefFree(lefData->layerName);
    } else
      lefFree(lefData->layerName);
    if (!lefData->ndLayerWidth) {
      if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
         if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
            lefError(1625, "A WIDTH statement is required in the LAYER statement in NONDEFULTRULE.");
            CHKERR();
         }
      }
    }
    if (!lefData->ndLayerSpace && lefData->versionNum < 5.6) {   // 5.6, SPACING is optional
      if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
         if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
            lefData->outMsg = (char*)lefMalloc(10000);
            sprintf (lefData->outMsg,
               "A SPACING statement is required in the LAYER statement in NONDEFAULTRULE for lef file with version 5.5 and earlier.\nYour lef file is defined with version %g. Update your lef to add a LAYER statement and try again.",
                lefData->versionNum);
            lefError(1626, lefData->outMsg);
            lefFree(lefData->outMsg);
            CHKERR();
         }
      }
    }
  }
  ;

nd_layer_stmts:
  // empty 
  | nd_layer_stmts nd_layer_stmt
  ;

nd_layer_stmt:
  K_SPACING int_number ';'
    {
      lefData->ndLayerSpace = 1;
      if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.addSpacing($2);
    }
  | K_WIREEXTENSION int_number ';'
    { if (lefCallbacks->NonDefaultCbk)
         lefData->lefrNonDefault.addWireExtension($2); }
  | K_RESISTANCE K_RPERSQ int_number ';'
    {
      if (lefData->ignoreVersion) {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addResistance($3);
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "RESISTANCE RPERSQ statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1627, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->versionNum > 5.5) {  // obsolete in 5.6
         if (lefCallbacks->NonDefaultCbk) // write warning only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings)
              lefWarning(2029, "RESISTANCE RPERSQ statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
      } else if (lefCallbacks->NonDefaultCbk)
         lefData->lefrNonDefault.addResistance($3);
    } 
 
  | K_CAPACITANCE K_CPERSQDIST int_number ';'
    {
      if (lefData->ignoreVersion) {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addCapacitance($3);
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "CAPACITANCE CPERSQDIST statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1628, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
            }
         }
      } else if (lefData->versionNum > 5.5) { // obsolete in 5.6
         if (lefCallbacks->NonDefaultCbk) // write warning only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings)
              lefWarning(2030, "CAPACITANCE CPERSQDIST statement is obsolete in version 5.6. and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
      } else if (lefCallbacks->NonDefaultCbk)
         lefData->lefrNonDefault.addCapacitance($3);
    }
  | K_EDGECAPACITANCE int_number ';'
    {
      if (lefData->ignoreVersion) {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addEdgeCap($2);
      } else if (lefData->versionNum < 5.4) {
         if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "EDGECAPACITANCE statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1629, lefData->outMsg);
               lefFree(lefData->outMsg);
              CHKERR();
            }
         }
      } else if (lefData->versionNum > 5.5) {  // obsolete in 5.6
         if (lefCallbacks->NonDefaultCbk) // write warning only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings)
              lefWarning(2031, "EDGECAPACITANCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
      } else if (lefCallbacks->NonDefaultCbk)
         lefData->lefrNonDefault.addEdgeCap($2);
    }
  | K_DIAGWIDTH int_number ';'
    {
      if (lefData->versionNum < 5.6) {  // 5.6 syntax
         if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
            if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                 "DIAGWIDTH statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
               lefError(1630, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR(); 
            }
         }
      } else {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addDiagWidth($2);
      }
    }

site: start_site site_options end_site
    { 
      if (lefCallbacks->SiteCbk)
        CALLBACK(lefCallbacks->SiteCbk, lefrSiteCbkType, &lefData->lefrSite);
    }

start_site: K_SITE {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING 
    { 
      if (lefCallbacks->SiteCbk) lefData->lefrSite.setName($3);
      //strcpy(lefData->siteName, $3);
      lefData->siteName = strdup($3);
      lefData->hasSiteClass = 0;
      lefData->hasSiteSize = 0;
      lefData->hasSite = 1;
    }

end_site: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    {
      if (strcmp(lefData->siteName, $3) != 0) {
        if (lefCallbacks->SiteCbk) { // write error only if cbk is set 
           if (lefData->siteWarnings++ < lefSettings->SiteWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END SITE name %s is different from the SITE name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->siteName);
              lefError(1631, lefData->outMsg);
              lefFree(lefData->outMsg);
              lefFree(lefData->siteName);
              CHKERR();
           } else
              lefFree(lefData->siteName);
        } else
           lefFree(lefData->siteName);
      } else {
        lefFree(lefData->siteName);
        if (lefCallbacks->SiteCbk) { // write error only if cbk is set 
          if (lefData->hasSiteClass == 0) {
             lefError(1632, "A CLASS statement is required in the SITE statement.");
             CHKERR();
          }
          if (lefData->hasSiteSize == 0) {
             lefError(1633, "A SIZE  statement is required in the SITE statement.");
             CHKERR();
          }
        }
      }
    }

site_options:
  // empty 
  | site_options site_option
  ;

site_option:
  K_SIZE int_number K_BY int_number ';' 
    {

      if (lefCallbacks->SiteCbk) lefData->lefrSite.setSize($2,$4);
      lefData->hasSiteSize = 1;
    }
  | site_symmetry_statement
    { }
  | site_class 
    { 
      if (lefCallbacks->SiteCbk) lefData->lefrSite.setClass($1);
      lefData->hasSiteClass = 1;
    }
  | site_rowpattern_statement
    { }

site_class:
  K_CLASS K_PAD ';' {$$ = (char*)"PAD"; }
  | K_CLASS K_CORE ';'  {$$ = (char*)"CORE"; }
  | K_CLASS K_VIRTUAL ';'  {$$ = (char*)"VIRTUAL"; }

site_symmetry_statement: K_SYMMETRY site_symmetries ';'
    { }

site_symmetries:
  // empty 
  | site_symmetries site_symmetry
  ;

site_symmetry:
  K_X 
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.setXSymmetry(); }
  | K_Y 
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.setYSymmetry(); }
  | K_R90
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.set90Symmetry(); }

site_rowpattern_statement: K_ROWPATTERN {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
    site_rowpatterns ';'
    { }

site_rowpatterns:
  // empty 
  | site_rowpatterns site_rowpattern
  ;

site_rowpattern: T_STRING orientation {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.addRowPattern($1, $2); }

pt:
  int_number int_number
    { $$.x = $1; $$.y = $2; }
  | '(' int_number int_number ')'
    { $$.x = $2; $$.y = $3; }

macro: start_macro macro_options
    { 
      if (lefCallbacks->MacroCbk)
        CALLBACK(lefCallbacks->MacroCbk, lefrMacroCbkType, &lefData->lefrMacro);
      lefData->lefrDoSite = 0;
    }
    end_macro

start_macro: K_MACRO {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING 
    {
      lefData->siteDef = 0;
      lefData->symDef = 0;
      lefData->sizeDef = 0; 
      lefData->pinDef = 0; 
      lefData->obsDef = 0; 
      lefData->origDef = 0;
      lefData->lefrMacro.clear();      
      if (lefCallbacks->MacroBeginCbk || lefCallbacks->MacroCbk) {
        // some reader may not have MacroBeginCB, but has MacroCB set
        lefData->lefrMacro.setName($3);
        CALLBACK(lefCallbacks->MacroBeginCbk, lefrMacroBeginCbkType, $3);
      }
      //strcpy(lefData->macroName, $3);
      lefData->macroName = strdup($3);
    }

end_macro: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    {
      if (strcmp(lefData->macroName, $3) != 0) {
        if (lefCallbacks->MacroEndCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END MACRO name %s is different from the MACRO name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->macroName);
              lefError(1634, lefData->outMsg);
              lefFree(lefData->outMsg);
              lefFree(lefData->macroName);
              CHKERR();
           } else
              lefFree(lefData->macroName);
        } else
           lefFree(lefData->macroName);
      } else
        lefFree(lefData->macroName);
      if (lefCallbacks->MacroEndCbk)
        CALLBACK(lefCallbacks->MacroEndCbk, lefrMacroEndCbkType, $3);
    }

macro_options:
  // empty 
  | macro_options macro_option   // Use left recursions 
  ;

macro_option:
  macro_class 
  | macro_generator 
  | macro_generate 
  | macro_source
  | macro_symmetry_statement 
  | macro_fixedMask
      { }
  | macro_origin 
      { }
  | macro_power 
      { }
  | macro_foreign
      { }
  | macro_eeq 
  | macro_leq 
  | macro_size 
      { }
  | macro_site 
      { }
  | macro_pin 
      { }
  | K_FUNCTION K_BUFFER ';'
      { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setBuffer(); }
  | K_FUNCTION K_INVERTER ';'
      { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setInverter(); }
  | macro_obs 
      { }
  | macro_density 
      { }
  | macro_clocktype 
      { }
  | timing
      { }
  | K_PROPERTY {lefData->lefDumbMode = 1000000; } macro_prop_list  ';'
      { lefData->lefDumbMode = 0;
      }

macro_prop_list:
  macro_name_value_pair
  | macro_prop_list macro_name_value_pair
  ;

macro_symmetry_statement: K_SYMMETRY macro_symmetries ';'
    {
      if (lefData->siteDef) { // SITE is defined before SYMMETRY 
          // pcr 283846 suppress warning 
          if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
               lefWarning(2032, "A SITE statement is defined before SYMMETRY statement.\nTo avoid this warning in the future, define SITE after SYMMETRY");
      }
      lefData->symDef = 1;
    }

macro_symmetries:
  // empty 
  | macro_symmetries macro_symmetry
  ;

macro_symmetry:
  K_X 
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setXSymmetry(); }
  | K_Y 
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setYSymmetry(); }
  | K_R90
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.set90Symmetry(); }

macro_name_value_pair:
  T_STRING NUMBER
    {
      char temp[32];
      sprintf(temp, "%.11g", $2);
      if (lefCallbacks->MacroCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrMacroProp.propType($1);
         lefData->lefrMacro.setNumProperty($1, $2, temp,  propTp);
      }
    }
  | T_STRING QSTRING
    {
      if (lefCallbacks->MacroCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrMacroProp.propType($1);
         lefData->lefrMacro.setProperty($1, $2, propTp);
      }
    }
  | T_STRING T_STRING
    {
      if (lefCallbacks->MacroCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrMacroProp.propType($1);
         lefData->lefrMacro.setProperty($1, $2, propTp);
      }
    }

macro_class: K_CLASS class_type ';'
    {
       if (lefCallbacks->MacroCbk) lefData->lefrMacro.setClass($2);
       if (lefCallbacks->MacroClassTypeCbk)
          CALLBACK(lefCallbacks->MacroClassTypeCbk, lefrMacroClassTypeCbkType, $2);
    }

class_type:
  K_COVER {$$ = (char*)"COVER"; }
  | K_COVER K_BUMP
    { $$ = (char*)"COVER BUMP";
      if (lefData->versionNum < 5.5) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              if (lefSettings->RelaxMode)
                 lefWarning(2033, "The statement COVER BUMP is a LEF verion 5.5 syntax.\nYour LEF file is version 5.4 or earlier which is incorrect but will be allowed\nbecause this application does not enforce strict version checking.\nOther tools that enforce strict checking will have a syntax error when reading this file.\nYou can change the VERSION statement in this LEF file to 5.5 or higher to stop this warning.");
              else {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "COVER BUMP statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                 lefError(1635, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
      }
    }
  | K_RING    {$$ = (char*)"RING"; }
  | K_BLOCK   {$$ = (char*)"BLOCK"; }
  | K_BLOCK K_BLACKBOX 
    { $$ = (char*)"BLOCK BLACKBOX";
      if (lefData->versionNum < 5.5) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
             if (lefSettings->RelaxMode)
                lefWarning(2034, "The statement BLOCK BLACKBOX is a LEF verion 5.5 syntax.\nYour LEF file is version 5.4 or earlier which is incorrect but will be allowed\nbecause this application does not enforce strict version checking.\nOther tools that enforce strict checking will have a syntax error when reading this file.\nYou can change the VERSION statement in this LEF file to 5.5 or higher to stop this warning.");
              else {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "BLOCK BLACKBOX statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                 lefError(1636, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
      }
    }
  | K_BLOCK K_SOFT
    {
      if (lefData->ignoreVersion) {
        $$ = (char*)"BLOCK SOFT";
      } else if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "BLOCK SOFT statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1637, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      else
        $$ = (char*)"BLOCK SOFT";
    }
  | K_NONE    {$$ = (char*)"NONE"; }
  | K_BUMP                         // 5.7 
      {
        if (lefData->versionNum < 5.7) {
          lefData->outMsg = (char*)lefMalloc(10000);
          sprintf(lefData->outMsg,
            "BUMP is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
          lefError(1698, lefData->outMsg);
          lefFree(lefData->outMsg);
          CHKERR();
        }
       
        $$ = (char*)"BUMP";
     }
  | K_PAD     {$$ = (char*)"PAD"; } 
  | K_VIRTUAL {$$ = (char*)"VIRTUAL"; }
  | K_PAD  pad_type 
      {  sprintf(lefData->temp_name, "PAD %s", $2);
        $$ = lefData->temp_name; 
        if (lefData->versionNum < 5.5) {
           if (strcmp("AREAIO", $2) != 0) {
             sprintf(lefData->temp_name, "PAD %s", $2);
             $$ = lefData->temp_name; 
           } else if (lefCallbacks->MacroCbk) { 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
               if (lefSettings->RelaxMode)
                  lefWarning(2035, "The statement PAD AREAIO is a LEF verion 5.5 syntax.\nYour LEF file is version 5.4 or earlier which is incorrect but will be allowed\nbecause this application does not enforce strict version checking.\nOther tools that enforce strict checking will have a syntax error when reading this file.\nYou can change the VERSION statement in this LEF file to 5.5 or higher to stop this warning.");
               else {
                  lefData->outMsg = (char*)lefMalloc(10000);
                  sprintf (lefData->outMsg,
                     "PAD AREAIO statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                  lefError(1638, lefData->outMsg);
                  lefFree(lefData->outMsg);
                  CHKERR();
               }
            }
          }
        }
      }
  | K_CORE    {$$ = (char*)"CORE"; }
  | K_CORNER 
      {$$ = (char*)"CORNER";
      // This token is NOT in the spec but has shown up in 
      // some lef files.  This exception came from LEFOUT
      // in 'frameworks'
      }
  | K_CORE core_type
      {sprintf(lefData->temp_name, "CORE %s", $2);
      $$ = lefData->temp_name;} 
  | K_ENDCAP endcap_type
      {sprintf(lefData->temp_name, "ENDCAP %s", $2);
      $$ = lefData->temp_name;} 

pad_type: 
  K_INPUT         {$$ = (char*)"INPUT";}
  | K_OUTPUT        {$$ = (char*)"OUTPUT";}
  | K_INOUT         {$$ = (char*)"INOUT";}
  | K_POWER         {$$ = (char*)"POWER";}
  | K_SPACER        {$$ = (char*)"SPACER";}
  | K_AREAIO    {$$ = (char*)"AREAIO";}

core_type:
  K_FEEDTHRU        {$$ = (char*)"FEEDTHRU";}
  | K_TIEHIGH       {$$ = (char*)"TIEHIGH";}
  | K_TIELOW        {$$ = (char*)"TIELOW";}
  | K_SPACER
    { 
      $$ = (char*)"SPACER";

      if (!lefData->ignoreVersion && lefData->versionNum < 5.4) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "SPACER statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1639, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
    }
  | K_ANTENNACELL
    { 
      $$ = (char*)"ANTENNACELL";

      if (!lefData->ignoreVersion && lefData->versionNum < 5.4) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNACELL statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1640, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
    }
  | K_WELLTAP
    { 
      $$ = (char*)"WELLTAP";

      if (!lefData->ignoreVersion && lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "WELLTAP statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1641, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
    }

endcap_type:
  K_PRE         {$$ = (char*)"PRE";}
  | K_POST         {$$ = (char*)"POST";}
  | K_TOPLEFT         {$$ = (char*)"TOPLEFT";}
  | K_TOPRIGHT         {$$ = (char*)"TOPRIGHT";}
  | K_BOTTOMLEFT         {$$ = (char*)"BOTTOMLEFT";}
  | K_BOTTOMRIGHT        {$$ = (char*)"BOTTOMRIGHT";}

macro_generator: K_GENERATOR T_STRING ';'
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setGenerator($2); }

macro_generate: K_GENERATE T_STRING T_STRING ';'
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setGenerate($2, $3); }

macro_source:
  K_SOURCE K_USER ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSource("USER");
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2036, "SOURCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | K_SOURCE K_GENERATE ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSource("GENERATE");
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2037, "SOURCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | K_SOURCE K_BLOCK ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSource("BLOCK");
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2037, "SOURCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }

macro_power: K_POWER int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setPower($2);
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2038, "MACRO POWER statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }

macro_origin: K_ORIGIN pt ';'
    { 
       if (lefData->origDef) { // Has multiple ORIGIN defined in a macro, stop parsing
          if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                lefError(1642, "ORIGIN statement has defined more than once in a MACRO statement.\nOnly one ORIGIN statement can be defined in a Macro.\nParser will stop processing.");
               CHKERR();
             }
          }
       }
       lefData->origDef = 1;
       if (lefData->siteDef) { // SITE is defined before ORIGIN 
          // pcr 283846 suppress warning 
          if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
               lefWarning(2039, "A SITE statement is defined before ORIGIN statement.\nTo avoid this warning in the future, define SITE after ORIGIN");
       }
       if (lefData->pinDef) { // PIN is defined before ORIGIN 
          // pcr 283846 suppress warning 
          if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
               lefWarning(2040, "A PIN statement is defined before ORIGIN statement.\nTo avoid this warning in the future, define PIN after ORIGIN");
       }
       if (lefData->obsDef) { // OBS is defined before ORIGIN 
          // pcr 283846 suppress warning 
          if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
               lefWarning(2041, "A OBS statement is defined before ORIGIN statement.\nTo avoid this warning in the future, define OBS after ORIGIN");
       }
      
       // Workaround for pcr 640902 
       if (lefCallbacks->MacroCbk) lefData->lefrMacro.setOrigin($2.x, $2.y);
       if (lefCallbacks->MacroOriginCbk) {
          lefData->macroNum.x = $2.x; 
          lefData->macroNum.y = $2.y; 
          CALLBACK(lefCallbacks->MacroOriginCbk, lefrMacroOriginCbkType, lefData->macroNum);
       }
    }

macro_foreign:
  start_foreign ';'
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign($1, 0, 0.0, 0.0, -1);
      }
      
      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign($1, 0, 0.0, 0.0, 0, 0);
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      }  
    }
  | start_foreign pt ';'
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign($1, 1, $2.x, $2.y, -1);
      }
      
      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign($1, 1, $2.x, $2.y, 0, 0);
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      }  
    }
  | start_foreign pt orientation ';'
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign($1, 1, $2.x, $2.y, $3);
      }
      
      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign($1, 1, $2.x, $2.y, 1, $3);
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      } 
    }
  | start_foreign orientation ';'
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign($1, 0, 0.0, 0.0, $2);
      }

      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign($1, 0, 0.0, 0.0, 1, $2);
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      } 
    }

macro_fixedMask:
   K_FIXEDMASK ';' 
   {   
       if (lefCallbacks->MacroCbk && lefData->versionNum >= 5.8) {
          lefData->lefrMacro.setFixedMask(1);
       }
       if (lefCallbacks->MacroFixedMaskCbk) {
          CALLBACK(lefCallbacks->MacroFixedMaskCbk, lefrMacroFixedMaskCbkType, 1);
       }        
    }
    
macro_eeq: K_EEQ { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setEEQ($3); }

macro_leq: K_LEQ { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setLEQ($3);
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2042, "LEQ statement in MACRO is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }

macro_site:
  macro_site_word  T_STRING ';'
    {
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.setSiteName($2);
      }

      if (lefCallbacks->MacroSiteCbk) {
        lefiMacroSite site($2, 0);
        CALLBACK(lefCallbacks->MacroSiteCbk, lefrMacroSiteCbkType, &site);
      }
    }
  | macro_site_word sitePattern ';'
    {
      if (lefCallbacks->MacroCbk) {
        // also set site name in the variable siteName_ in lefiMacro 
        // this, if user wants to use method lefData->siteName will get the name also 
        lefData->lefrMacro.setSitePattern(lefData->lefrSitePatternPtr);
      }

      if (lefCallbacks->MacroSiteCbk) {
        lefiMacroSite site(0, lefData->lefrSitePatternPtr);
        CALLBACK(lefCallbacks->MacroSiteCbk, lefrMacroSiteCbkType, &site);
      }
        
      lefData->lefrSitePatternPtr = 0;
    }

macro_site_word: K_SITE
    { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; lefData->siteDef = 1;
        if (lefCallbacks->MacroCbk) lefData->lefrDoSite = 1; }

site_word: K_SITE
    { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }

macro_size: K_SIZE int_number K_BY int_number ';'
    { 
      if (lefData->siteDef) { // SITE is defined before SIZE 
      }
      lefData->sizeDef = 1;
      if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSize($2, $4);
      if (lefCallbacks->MacroSizeCbk) {
         lefData->macroNum.x = $2; 
         lefData->macroNum.y = $4; 
         CALLBACK(lefCallbacks->MacroSizeCbk, lefrMacroSizeCbkType, lefData->macroNum);
      }
    }

// This is confusing, since FEF and LEF have opposite definitions of
// ports and pins 

macro_pin: start_macro_pin macro_pin_options end_macro_pin
    { 
      if (lefCallbacks->PinCbk)
        CALLBACK(lefCallbacks->PinCbk, lefrPinCbkType, &lefData->lefrPin);
      lefData->lefrPin.clear();
    }

start_macro_pin: K_PIN {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; lefData->pinDef = 1;} T_STRING 
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setName($3);
      //strcpy(lefData->pinName, $3);
      lefData->pinName = strdup($3);
    }

end_macro_pin: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    {
      if (strcmp(lefData->pinName, $3) != 0) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END PIN name %s is different from the PIN name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->pinName);
              lefError(1643, lefData->outMsg);
              lefFree(lefData->outMsg);
              lefFree(lefData->pinName);
              CHKERR();
           } else
              lefFree(lefData->pinName);
        } else
           lefFree(lefData->pinName);
      } else
        lefFree(lefData->pinName);
    }

macro_pin_options:
  // empty 
    { }
  | macro_pin_options macro_pin_option 
    { }

macro_pin_option:
  start_foreign ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign($1, 0, 0.0, 0.0, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign pt ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign($1, 1, $2.x, $2.y, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign pt orientation ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign($1, 1, $2.x, $2.y, $3);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign K_STRUCTURE ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign($1, 0, 0.0, 0.0, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign K_STRUCTURE pt ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign($1, 1, $3.x, $3.y, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | start_foreign K_STRUCTURE pt orientation ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign($1, 1, $3.x, $3.y, $4);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  | K_LEQ { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setLEQ($3);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2044, "LEQ statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
   }
  | K_POWER int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setPower($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2045, "MACRO POWER statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | electrical_direction
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setDirection($1); }
  | K_USE macro_pin_use ';'
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setUse($2); }
  | K_SCANUSE macro_scan_use ';'
    { }
  | K_LEAKAGE int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setLeakage($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2046, "MACRO LEAKAGE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, r emove this statement from the LEF file with version 5.4 or later.");
    }
  | K_RISETHRESH int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseThresh($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2047, "MACRO RISETHRESH statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_FALLTHRESH int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setFallThresh($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2048, "MACRO FALLTHRESH statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_RISESATCUR int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseSatcur($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2049, "MACRO RISESATCUR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_FALLSATCUR int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setFallSatcur($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2050, "MACRO FALLSATCUR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_VLO int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setVLO($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2051, "MACRO VLO statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_VHI int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setVHI($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2052, "MACRO VHI statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_TIEOFFR int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setTieoffr($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2053, "MACRO TIEOFFR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_SHAPE pin_shape ';'
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setShape($2); }
  | K_MUSTJOIN {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING ';'
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setMustjoin($3); }
  | K_OUTPUTNOISEMARGIN {lefData->lefDumbMode = 1;} int_number int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setOutMargin($3, $4);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2054, "MACRO OUTPUTNOISEMARGIN statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_OUTPUTRESISTANCE {lefData->lefDumbMode = 1;} int_number int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setOutResistance($3, $4);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2055, "MACRO OUTPUTRESISTANCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_INPUTNOISEMARGIN {lefData->lefDumbMode = 1;} int_number int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setInMargin($3, $4);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2056, "MACRO INPUTNOISEMARGIN statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_CAPACITANCE int_number ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setCapacitance($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2057, "MACRO CAPACITANCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_MAXDELAY int_number ';' 
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setMaxdelay($2); }
  | K_MAXLOAD int_number ';' 
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setMaxload($2); }
  | K_RESISTANCE int_number ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setResistance($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2058, "MACRO RESISTANCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_PULLDOWNRES int_number ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setPulldownres($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2059, "MACRO PULLDOWNRES statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_CURRENTSOURCE K_ACTIVE ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setCurrentSource("ACTIVE");
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2060, "MACRO CURRENTSOURCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_CURRENTSOURCE K_RESISTIVE ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setCurrentSource("RESISTIVE");
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2061, "MACRO CURRENTSOURCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_RISEVOLTAGETHRESHOLD int_number ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseVoltage($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2062, "MACRO RISEVOLTAGETHRESHOLD statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_FALLVOLTAGETHRESHOLD int_number ';' 
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setFallVoltage($2);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2063, "MACRO FALLVOLTAGETHRESHOLD statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_IV_TABLES T_STRING T_STRING ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setTables($2, $3);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2064, "MACRO IV_TABLES statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
  | K_TAPERRULE T_STRING ';'
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setTaperRule($2); }
  | K_PROPERTY {lefData->lefDumbMode = 1000000; } pin_prop_list ';'
    { lefData->lefDumbMode = 0;
    }
  | start_macro_port macro_port_class_option geometries K_END
    {
      lefData->lefDumbMode = 0;
      lefData->hasGeoLayer = 0;
      if (lefCallbacks->PinCbk) {
        lefData->lefrPin.addPort(lefData->lefrGeometriesPtr);
        lefData->lefrGeometriesPtr = 0;
        lefData->lefrDoGeometries = 0;
      }
      if ((lefData->needGeometry) && (lefData->needGeometry != 2))  // if the lefData->last LAYER in PORT
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2065, "Either PATH, RECT or POLYGON statement is a required in MACRO/PIN/PORT.");
    }
  | start_macro_port K_END
    {
      // Since in start_macro_port it has call the Init method, here
      // we need to call the Destroy method.
      // Still add a null pointer to set the number of port
      if (lefCallbacks->PinCbk) {
        lefData->lefrPin.addPort(lefData->lefrGeometriesPtr);
        lefData->lefrGeometriesPtr = 0;
        lefData->lefrDoGeometries = 0;
      }
      lefData->hasGeoLayer = 0;
    }
  | K_ANTENNASIZE int_number opt_layer_name ';'
    {  // a pre 5.4 syntax 
      lefData->use5_3 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum >= 5.4) {
        if (lefData->use5_4) {
           if (lefCallbacks->PinCbk) { // write error only if cbk is set 
             if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
                lefData->outMsg = (char*)lefMalloc(10000);
                sprintf (lefData->outMsg,
                   "ANTENNASIZE statement is a version 5.3 and earlier syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                lefError(1644, lefData->outMsg);
                lefFree(lefData->outMsg);
                CHKERR();
             }
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaSize($2, $3);
    }
  | K_ANTENNAMETALAREA NUMBER opt_layer_name ';'
    {  // a pre 5.4 syntax 
      lefData->use5_3 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum >= 5.4) {
        if (lefData->use5_4) {
           if (lefCallbacks->PinCbk) { // write error only if cbk is set 
              if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "ANTENNAMETALAREA statement is a version 5.3 and earlier syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                 lefError(1645, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMetalArea($2, $3);
    }
  | K_ANTENNAMETALLENGTH int_number opt_layer_name ';'
    { // a pre 5.4 syntax  
      lefData->use5_3 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum >= 5.4) {
        if (lefData->use5_4) {
           if (lefCallbacks->PinCbk) { // write error only if cbk is set 
              if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "ANTENNAMETALLENGTH statement is a version 5.3 and earlier syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
                 lefError(1646, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMetalLength($2, $3);
    }
  | K_RISESLEWLIMIT int_number ';'
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseSlewLimit($2); }
  | K_FALLSLEWLIMIT int_number ';'
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setFallSlewLimit($2); }
  | K_ANTENNAPARTIALMETALAREA NUMBER opt_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAPARTIALMETALAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1647, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAPARTIALMETALAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1647, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaPartialMetalArea($2, $3);
    }
  | K_ANTENNAPARTIALMETALSIDEAREA NUMBER opt_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAPARTIALMETALSIDEAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1648, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAPARTIALMETALSIDEAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1648, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaPartialMetalSideArea($2, $3);
    }
  | K_ANTENNAPARTIALCUTAREA NUMBER opt_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAPARTIALCUTAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1649, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAPARTIALCUTAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1649, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaPartialCutArea($2, $3);
    }
  | K_ANTENNADIFFAREA NUMBER opt_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNADIFFAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1650, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNADIFFAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1650, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaDiffArea($2, $3);
    }
  | K_ANTENNAGATEAREA NUMBER opt_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAGATEAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1651, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAGATEAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1651, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaGateArea($2, $3);
    }
  | K_ANTENNAMAXAREACAR NUMBER req_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMAXAREACAR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1652, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMAXAREACAR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1652, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMaxAreaCar($2, $3);
    }
  | K_ANTENNAMAXSIDEAREACAR NUMBER req_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMAXSIDEAREACAR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1653, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMAXSIDEAREACAR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1653, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMaxSideAreaCar($2, $3);
    }
  | K_ANTENNAMAXCUTCAR NUMBER req_layer_name ';'
    { // 5.4 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMAXCUTCAR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1654, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMAXCUTCAR statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1654, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMaxCutCar($2, $3);
    }
  | K_ANTENNAMODEL
    { // 5.5 syntax 
      lefData->use5_4 = 1;
      if (lefData->ignoreVersion) {
        // do nothing 
      } else if (lefData->versionNum < 5.5) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMODEL statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1655, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else if (lefData->use5_3) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ANTENNAMODEL statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1655, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      }
    }
    pin_layer_oxide ';'
  | K_NETEXPR {lefData->lefDumbMode = 2; lefData->lefNoNum = 2; } QSTRING ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "NETEXPR statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1656, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else
        if (lefCallbacks->PinCbk) lefData->lefrPin.setNetExpr($3);
    }
  | K_SUPPLYSENSITIVITY {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "SUPPLYSENSITIVITY statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1657, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else
        if (lefCallbacks->PinCbk) lefData->lefrPin.setSupplySensitivity($3);
    }
  | K_GROUNDSENSITIVITY {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) { // write error only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "GROUNDSENSITIVITY statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1658, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } else
        if (lefCallbacks->PinCbk) lefData->lefrPin.setGroundSensitivity($3);
    }

pin_layer_oxide:
  K_OXIDE1
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(1);
    }
  | K_OXIDE2
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(2);
    }
  | K_OXIDE3
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(3);
    }
  | K_OXIDE4
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(4);
    }

pin_prop_list:
  pin_name_value_pair
  | pin_prop_list pin_name_value_pair
  ;

pin_name_value_pair:
  T_STRING NUMBER
    { 
      char temp[32];
      sprintf(temp, "%.11g", $2);
      if (lefCallbacks->PinCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrPinProp.propType($1);
         lefData->lefrPin.setNumProperty($1, $2, temp, propTp);
      }
    }
  | T_STRING QSTRING
    {
      if (lefCallbacks->PinCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrPinProp.propType($1);
         lefData->lefrPin.setProperty($1, $2, propTp);
      }
    }
  | T_STRING T_STRING
    {
      if (lefCallbacks->PinCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrPinProp.propType($1);
         lefData->lefrPin.setProperty($1, $2, propTp);
      }
    }

electrical_direction:
  K_DIRECTION K_INPUT ';'              {$$ = (char*)"INPUT";}
  | K_DIRECTION K_OUTPUT ';'            {$$ = (char*)"OUTPUT";}
  | K_DIRECTION K_OUTPUT K_TRISTATE ';' {$$ = (char*)"OUTPUT TRISTATE";}
  | K_DIRECTION K_INOUT  ';'            {$$ = (char*)"INOUT";}
  | K_DIRECTION K_FEEDTHRU ';'          {$$ = (char*)"FEEDTHRU";}

start_macro_port: K_PORT
    {
      if (lefCallbacks->PinCbk) {
        lefData->lefrDoGeometries = 1;
        lefData->hasPRP = 0;
        lefData->lefrGeometriesPtr = (lefiGeometries*)lefMalloc( sizeof(lefiGeometries));
        lefData->lefrGeometriesPtr->Init();
      }
      lefData->needGeometry = 0;  // don't need rect/path/poly define yet
      lefData->hasGeoLayer = 0;   // make sure LAYER is set before geometry
    }

macro_port_class_option: // empty 
  | K_CLASS class_type ';'
    { if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->addClass($2); }

macro_pin_use:
  K_SIGNAL      {$$ = (char*)"SIGNAL";}
  | K_ANALOG    {$$ = (char*)"ANALOG";}
  | K_POWER     {$$ = (char*)"POWER";}
  | K_GROUND    {$$ = (char*)"GROUND";}
  | K_CLOCK     {$$ = (char*)"CLOCK";}
  | K_DATA      {$$ = (char*)"DATA";}

macro_scan_use:
  K_INPUT {$$ = (char*)"INPUT";}
  | K_OUTPUT    {$$ = (char*)"OUTPUT";}
  | K_START     {$$ = (char*)"START";}
  | K_STOP      {$$ = (char*)"STOP";}

pin_shape:
  {$$ = (char*)""; }      // non-lefData->ring shape 
  | K_ABUTMENT  {$$ = (char*)"ABUTMENT";}
  | K_RING      {$$ = (char*)"RING";}
  | K_FEEDTHRU  {$$ = (char*)"FEEDTHRU";}

geometries: geometry geometry_options

geometry:
  K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING
    {
      if ((lefData->needGeometry) && (lefData->needGeometry != 2)) // 1 LAYER follow after another
        if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
          // geometries is called by MACRO/OBS & MACRO/PIN/PORT 
          if (lefData->obsDef)
             lefWarning(2076, "Either PATH, RECT or POLYGON statement is a required in MACRO/OBS.");
          else
             lefWarning(2065, "Either PATH, RECT or POLYGON statement is a required in MACRO/PIN/PORT.");
        }
      if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->addLayer($3);
      lefData->needGeometry = 1;    // within LAYER it requires either path, rect, poly
      lefData->hasGeoLayer = 1;
    }
  layer_exceptpgnet
  layer_spacing ';'
  | K_WIDTH int_number ';'
    { 
      if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else
           lefData->lefrGeometriesPtr->addWidth($2); 
      } 
    }
  | K_PATH maskColor firstPt otherPts ';'
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
                lefData->lefrGeometriesPtr->addPath((int)$2);
           }
        }
      }
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
  | K_PATH maskColor K_ITERATE firstPt otherPts stepPattern ';'
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addPathIter((int)$2);
            }
         }
      } 
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
  | K_RECT maskColor pt pt';'
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addRect((int)$2, $3.x, $3.y, $4.x, $4.y);
           }
        }
      }
      lefData->needGeometry = 2;
    }
  | K_RECT maskColor K_ITERATE pt pt stepPattern ';'
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addRectIter((int)$2, $4.x, $4.y, $5.x, $5.y);
           }
        }
      }
      lefData->needGeometry = 2;
    }
  | K_POLYGON maskColor firstPt nextPt nextPt otherPts ';'
    {
      if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addPolygon((int)$2);
            }
           }
      }
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
  | K_POLYGON maskColor K_ITERATE firstPt nextPt nextPt otherPts stepPattern ';'
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addPolygonIter((int)$2);
           }
         }
      }
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
  | via_placement
    { }

geometry_options: // empty 
  | geometry_options geometry

layer_exceptpgnet: // empty 
  | K_EXCEPTPGNET                   // 5.7 
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "EXCEPTPGNET is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1699, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } else {
       if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->addLayerExceptPgNet();
      }
    }

layer_spacing: // empty 
  | K_SPACING int_number
    { if (lefData->lefrDoGeometries) {
        if (zeroOrGt($2))
           lefData->lefrGeometriesPtr->addLayerMinSpacing($2);
        else {
           lefData->outMsg = (char*)lefMalloc(10000);
           sprintf (lefData->outMsg,
              "THE SPACING statement has the value %g in MACRO OBS.\nValue has to be 0 or greater.", $2);
           lefError(1659, lefData->outMsg);
           lefFree(lefData->outMsg);
           CHKERR();
        }
      }
    }
  | K_DESIGNRULEWIDTH int_number
    { if (lefData->lefrDoGeometries) {
        if (zeroOrGt($2))
           lefData->lefrGeometriesPtr->addLayerRuleWidth($2);
        else {
           lefData->outMsg = (char*)lefMalloc(10000);
           sprintf (lefData->outMsg,
              "THE DESIGNRULEWIDTH statement has the value %g in MACRO OBS.\nValue has to be 0 or greater.", $2);
           lefError(1660, lefData->outMsg);
           lefFree(lefData->outMsg);
           CHKERR();
        }
      }
    }

firstPt: pt  
    { if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->startList($1.x, $1.y); }

nextPt:  pt
    { if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->addToList($1.x, $1.y); }

otherPts:
  // empty 
  | otherPts nextPt
  ;

via_placement:
  K_VIA maskColor pt {lefData->lefDumbMode = 1;} T_STRING ';'
    { 
        if (lefData->lefrDoGeometries){
            if (lefData->versionNum < 5.8 && (int)$2 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
            } else {
                lefData->lefrGeometriesPtr->addVia((int)$2, $3.x, $3.y, $5);
            }
        }
    }
  | K_VIA K_ITERATE maskColor pt {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    stepPattern ';'
    { 
        if (lefData->lefrDoGeometries) {
            if (lefData->versionNum < 5.8 && (int)$3 > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
            } else {
              lefData->lefrGeometriesPtr->addViaIter((int)$3, $4.x, $4.y, $6); 
            }
        }
    }
        
stepPattern: K_DO int_number K_BY int_number K_STEP int_number int_number
     { if (lefData->lefrDoGeometries)
         lefData->lefrGeometriesPtr->addStepPattern($2, $4, $6, $7); }

sitePattern: T_STRING int_number int_number orientation
  K_DO int_number K_BY int_number K_STEP int_number int_number
    {
      if (lefData->lefrDoSite) {
        lefData->lefrSitePatternPtr = (lefiSitePattern*)lefMalloc(
                                   sizeof(lefiSitePattern));
        lefData->lefrSitePatternPtr->Init();
        lefData->lefrSitePatternPtr->set($1, $2, $3, $4, $6, $8,
          $10, $11);
        }
    }
  | T_STRING int_number int_number orientation
    {
      if (lefData->lefrDoSite) {
        lefData->lefrSitePatternPtr = (lefiSitePattern*)lefMalloc(
                                   sizeof(lefiSitePattern));
        lefData->lefrSitePatternPtr->Init();
        lefData->lefrSitePatternPtr->set($1, $2, $3, $4, -1, -1,
          -1, -1);
        }
    }

trackPattern:
  K_X int_number K_DO int_number K_STEP int_number 
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("X", $2, (int)$4, $6);
      }    
    }
    K_LAYER {lefData->lefDumbMode = 1000000000;} trackLayers
    { lefData->lefDumbMode = 0;}
  | K_Y int_number K_DO int_number K_STEP int_number
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                    sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("Y", $2, (int)$4, $6);
      }    
    }
    K_LAYER {lefData->lefDumbMode = 1000000000;} trackLayers
    { lefData->lefDumbMode = 0;}
  | K_X int_number K_DO int_number K_STEP int_number 
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                    sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("X", $2, (int)$4, $6);
      }    
    }
  | K_Y int_number K_DO int_number K_STEP int_number
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                    sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("Y", $2, (int)$4, $6);
      }    
    }

trackLayers:
  // empty 
  | trackLayers layer_name
  ;

layer_name: T_STRING
    { if (lefData->lefrDoTrack) lefData->lefrTrackPatternPtr->addLayer($1); }

gcellPattern: K_X int_number K_DO int_number K_STEP int_number
    {
      if (lefData->lefrDoGcell) {
        lefData->lefrGcellPatternPtr = (lefiGcellPattern*)lefMalloc(
                                    sizeof(lefiGcellPattern));
        lefData->lefrGcellPatternPtr->Init();
        lefData->lefrGcellPatternPtr->set("X", $2, (int)$4, $6);
      }    
    }
  | K_Y int_number K_DO int_number K_STEP int_number
    {
      if (lefData->lefrDoGcell) {
        lefData->lefrGcellPatternPtr = (lefiGcellPattern*)lefMalloc(
                                    sizeof(lefiGcellPattern));
        lefData->lefrGcellPatternPtr->Init();
        lefData->lefrGcellPatternPtr->set("Y", $2, (int)$4, $6);
      }    
    }

macro_obs: start_macro_obs geometries K_END
    { 
      if (lefCallbacks->ObstructionCbk) {
        lefData->lefrObstruction.setGeometries(lefData->lefrGeometriesPtr);
        lefData->lefrGeometriesPtr = 0;
        lefData->lefrDoGeometries = 0;
        CALLBACK(lefCallbacks->ObstructionCbk, lefrObstructionCbkType, &lefData->lefrObstruction);
      }
      lefData->lefDumbMode = 0;
      lefData->hasGeoLayer = 0;       // reset 
    }
  | start_macro_obs K_END
    {
       // The pointer has malloced in start, need to free manually 
       if (lefData->lefrGeometriesPtr) {
          lefData->lefrGeometriesPtr->Destroy();
          lefFree(lefData->lefrGeometriesPtr);
          lefData->lefrGeometriesPtr = 0;
          lefData->lefrDoGeometries = 0;
       }
       lefData->hasGeoLayer = 0;
    }

start_macro_obs: K_OBS
    {
      lefData->obsDef = 1;
      if (lefCallbacks->ObstructionCbk) {
        lefData->lefrDoGeometries = 1;
        lefData->lefrGeometriesPtr = (lefiGeometries*)lefMalloc(
            sizeof(lefiGeometries));
        lefData->lefrGeometriesPtr->Init();
        }
      lefData->hasGeoLayer = 0;
    }

macro_density: K_DENSITY density_layer density_layers K_END
    { 
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->DensityCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "DENSITY statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1661, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
      } 
      if (lefCallbacks->DensityCbk) {
        CALLBACK(lefCallbacks->DensityCbk, lefrDensityCbkType, &lefData->lefrDensity);
        lefData->lefrDensity.clear();
      }
      lefData->lefDumbMode = 0;
    }

density_layers: // empty 
    | density_layers density_layer
    ;

density_layer: K_LAYER { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    {
      if (lefCallbacks->DensityCbk)
        lefData->lefrDensity.addLayer($3);
    }
    density_layer_rect density_layer_rects

density_layer_rects: // empty 
    | density_layer_rects density_layer_rect
    ;

density_layer_rect: K_RECT pt pt int_number ';'
    {
      if (lefCallbacks->DensityCbk)
        lefData->lefrDensity.addRect($2.x, $2.y, $3.x, $3.y, $4); 
    }

macro_clocktype: K_CLOCKTYPE { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING ';'
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setClockType($3); }

timing: start_timing timing_options end_timing
    { }

start_timing: K_TIMING 
    { }

end_timing: K_END K_TIMING 
  {
    if (lefData->versionNum < 5.4) {
      if (lefCallbacks->TimingCbk && lefData->lefrTiming.hasData())
        CALLBACK(lefCallbacks->TimingCbk, lefrTimingCbkType, &lefData->lefrTiming);
      lefData->lefrTiming.clear();
    } else {
      if (lefCallbacks->TimingCbk) // write warning only if cbk is set 
        if (lefData->timingWarnings++ < lefSettings->TimingWarnings)
          lefWarning(2066, "MACRO TIMING statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
      lefData->lefrTiming.clear();
    }
  }

timing_options:
  // empty 
  | timing_options timing_option
  ;

timing_option:
  K_FROMPIN 
    {
    if (lefData->versionNum < 5.4) {
      if (lefCallbacks->TimingCbk && lefData->lefrTiming.hasData())
        CALLBACK(lefCallbacks->TimingCbk, lefrTimingCbkType, &lefData->lefrTiming);
    }
    lefData->lefDumbMode = 1000000000;
    lefData->lefrTiming.clear();
    }
    list_of_from_strings ';'
    { lefData->lefDumbMode = 0;}
  | K_TOPIN {lefData->lefDumbMode = 1000000000;} list_of_to_strings ';'
    { lefData->lefDumbMode = 0;}
  | risefall K_INTRINSIC int_number int_number
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFall($1,$3,$4); }
    slew_spec K_VARIABLE int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallVariable($8,$9); }
  | risefall delay_or_transition K_UNATENESS unateness
    K_TABLEDIMENSION int_number int_number int_number ';' 
    { if (lefCallbacks->TimingCbk) {
        if ($2[0] == 'D' || $2[0] == 'd') // delay 
          lefData->lefrTiming.addDelay($1, $4, $6, $7, $8);
        else
          lefData->lefrTiming.addTransition($1, $4, $6, $7, $8);
      }
    }
  | K_TABLEAXIS list_of_table_axis_dnumbers ';'
    { }
  | K_TABLEENTRIES list_of_table_entries ';'
    { }
  | K_RISERS int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseRS($2,$3); }
  | K_FALLRS int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallRS($2,$3); }
  | K_RISECS int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseCS($2,$3); }
  | K_FALLCS int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallCS($2,$3); }
  | K_RISESATT1 int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseAtt1($2,$3); }
  | K_FALLSATT1 int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallAtt1($2,$3); }
  | K_RISET0 int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseTo($2,$3); }
  | K_FALLT0 int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallTo($2,$3); }
  | K_UNATENESS unateness ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addUnateness($2); }
  | K_STABLE K_SETUP int_number K_HOLD int_number risefall ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setStable($3,$5,$6); }
  | two_pin_trigger from_pin_trigger to_pin_trigger K_TABLEDIMENSION int_number int_number int_number ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addSDF2Pins($1,$2,$3,$5,$6,$7); }
  | one_pin_trigger K_TABLEDIMENSION int_number int_number int_number ';' 
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addSDF1Pin($1,$3,$4,$4); }
  | K_SDFCONDSTART QSTRING ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setSDFcondStart($2); }
  | K_SDFCONDEND QSTRING ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setSDFcondEnd($2); }
  | K_SDFCOND QSTRING ';'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setSDFcond($2); }
  | K_EXTENSION ';'
    { }

one_pin_trigger:
  K_MPWH
    { $$ = (char*)"MPWH";}
  | K_MPWL
    { $$ = (char*)"MPWL";}
  | K_PERIOD
    { $$ = (char*)"PERIOD";}

two_pin_trigger :
  K_SETUP
    { $$ = (char*)"SETUP";}
  | K_HOLD
    { $$ = (char*)"HOLD";}
  | K_RECOVERY
    { $$ = (char*)"RECOVERY";}
  | K_SKEW
    { $$ = (char*)"SKEW";}

from_pin_trigger:
  K_ANYEDGE
    { $$ = (char*)"ANYEDGE";}
  | K_POSEDGE
    { $$ = (char*)"POSEDGE";}
  | K_NEGEDGE 
    { $$ = (char*)"NEGEDGE";}

to_pin_trigger:
  K_ANYEDGE
    { $$ = (char*)"ANYEDGE";}
  | K_POSEDGE
    { $$ = (char*)"POSEDGE";}
  | K_NEGEDGE 
    { $$ = (char*)"NEGEDGE";}

delay_or_transition :
  K_DELAY
    { $$ = (char*)"DELAY"; }
  | K_TRANSITIONTIME
    { $$ = (char*)"TRANSITION"; }

list_of_table_entries:
  table_entry
    { }
  | list_of_table_entries table_entry
    { }

table_entry: '(' int_number int_number int_number ')'
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addTableEntry($2,$3,$4); }

list_of_table_axis_dnumbers:
  int_number
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addTableAxisNumber($1); }
  | list_of_table_axis_dnumbers int_number
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addTableAxisNumber($2); }

slew_spec:
  // empty 
    { }
  | int_number int_number int_number int_number 
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallSlew($1,$2,$3,$4); }
  |  int_number int_number int_number int_number int_number int_number int_number 
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallSlew($1,$2,$3,$4);
      if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallSlew2($5,$6,$7); }

risefall:
  K_RISE
    { $$ = (char*)"RISE"; }
  | K_FALL 
    { $$ = (char*)"FALL"; }

unateness:
  K_INVERT
    { $$ = (char*)"INVERT"; }
  | K_NONINVERT
    { $$ = (char*)"NONINVERT"; }
  | K_NONUNATE 
    { $$ = (char*)"NONUNATE"; }

list_of_from_strings:
  T_STRING
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addFromPin($1); }
  | list_of_from_strings T_STRING 
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addFromPin($2); }

list_of_to_strings:
  T_STRING
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addToPin($1); }
  | list_of_to_strings T_STRING 
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addToPin($2); }

array: start_array array_rules
    {
      if (lefCallbacks->ArrayCbk)
        CALLBACK(lefCallbacks->ArrayCbk, lefrArrayCbkType, &lefData->lefrArray);
      lefData->lefrArray.clear();
      lefData->lefrSitePatternPtr = 0;
      lefData->lefrDoSite = 0;
   }
    end_array

start_array: K_ARRAY {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.setName($3);
        CALLBACK(lefCallbacks->ArrayBeginCbk, lefrArrayBeginCbkType, $3);
      }
      //strcpy(lefData->arrayName, $3);
      lefData->arrayName = strdup($3);
    }

end_array: K_END {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;} T_STRING
    {
      if (lefCallbacks->ArrayCbk && lefCallbacks->ArrayEndCbk)
        CALLBACK(lefCallbacks->ArrayEndCbk, lefrArrayEndCbkType, $3);
      if (strcmp(lefData->arrayName, $3) != 0) {
        if (lefCallbacks->ArrayCbk) { // write error only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END ARRAY name %s is different from the ARRAY name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", $3, lefData->arrayName);
              lefError(1662, lefData->outMsg);
              lefFree(lefData->outMsg);
              lefFree(lefData->arrayName);
              CHKERR();
           } else
              lefFree(lefData->arrayName);
        } else
           lefFree(lefData->arrayName);
      } else
        lefFree(lefData->arrayName);
    }

array_rules:
  // empty 
    { }
  | array_rules array_rule
    { }

array_rule:
  site_word { if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; lefData->lefDumbMode = 1; }
    sitePattern  ';'
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addSitePattern(lefData->lefrSitePatternPtr);
      }
    }
  | K_CANPLACE {lefData->lefDumbMode = 1; if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; }
    sitePattern ';'
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addCanPlace(lefData->lefrSitePatternPtr);
      }
    }
  | K_CANNOTOCCUPY {lefData->lefDumbMode = 1; if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; }
    sitePattern ';'
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addCannotOccupy(lefData->lefrSitePatternPtr);
      }
    }
  | K_TRACKS { if (lefCallbacks->ArrayCbk) lefData->lefrDoTrack = 1; } trackPattern ';'
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addTrack(lefData->lefrTrackPatternPtr);
      }
    }
  | floorplan_start floorplan_list K_END T_STRING 
    {
    }
  | K_GCELLGRID { if (lefCallbacks->ArrayCbk) lefData->lefrDoGcell = 1; } gcellPattern ';'
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addGcell(lefData->lefrGcellPatternPtr);
      }
    }
  | K_DEFAULTCAP int_number cap_list K_END K_DEFAULTCAP
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.setTableSize((int)$2);
      }
    }
  | def_statement
    { }

floorplan_start: K_FLOORPLAN T_STRING
    { if (lefCallbacks->ArrayCbk) lefData->lefrArray.addFloorPlan($2); }
        
floorplan_list:
  // empty 
    { }
  | floorplan_list floorplan_element
    { }

floorplan_element:
  K_CANPLACE { lefData->lefDumbMode = 1; if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; }
    sitePattern ';'
    {
      if (lefCallbacks->ArrayCbk)
        lefData->lefrArray.addSiteToFloorPlan("CANPLACE",
        lefData->lefrSitePatternPtr);
    }
  | K_CANNOTOCCUPY { if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; lefData->lefDumbMode = 1; }
    sitePattern ';'
    {
      if (lefCallbacks->ArrayCbk)
        lefData->lefrArray.addSiteToFloorPlan("CANNOTOCCUPY",
        lefData->lefrSitePatternPtr);
     }

cap_list:
  // empty 
    { }
  | cap_list one_cap
    { }

one_cap: K_MINPINS int_number K_WIRECAP int_number ';'
    { if (lefCallbacks->ArrayCbk) lefData->lefrArray.addDefaultCap((int)$2, $4); }

msg_statement:
  K_MESSAGE {lefData->lefDumbMode=1;lefData->lefNlToken=TRUE;} T_STRING '=' s_expr dtrm
    {  }

create_file_statement:
  K_CREATEFILE {lefData->lefDumbMode=1;lefData->lefNlToken=TRUE;} T_STRING '=' s_expr dtrm
    { }

def_statement:
  K_DEFINE {lefData->lefDumbMode=1;lefData->lefNlToken=TRUE;} T_STRING '=' expression dtrm
    {
      if (lefData->versionNum < 5.6)
        lefAddNumDefine($3, $5);
      else
        if (lefCallbacks->ArrayCbk) // write warning only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings)
             lefWarning(2067, "DEFINE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  |  K_DEFINES {lefData->lefDumbMode=1;lefData->lefNlToken=TRUE;} T_STRING '=' s_expr dtrm
    {
      if (lefData->versionNum < 5.6)
        lefAddStringDefine($3, $5);
      else
        if (lefCallbacks->ArrayCbk) // write warning only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings)
             lefWarning(2068, "DEFINES statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
  |  K_DEFINEB {lefData->lefDumbMode=1;lefData->lefNlToken=TRUE;} T_STRING '=' b_expr dtrm
    {
      if (lefData->versionNum < 5.6)
        lefAddBooleanDefine($3, $5);
      else
        if (lefCallbacks->ArrayCbk) // write warning only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings)
             lefWarning(2069, "DEFINEB statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }

// terminator for &defines.  Can be semicolon or newline 
dtrm:
  |  ';' {lefData->lefNlToken = FALSE;}
  |  '\n'        {lefData->lefNlToken = FALSE;}

then:
  K_THEN
  | '\n' K_THEN
  ;

else:
  K_ELSE
  | '\n' K_ELSE
  ;

expression:
  expression '+' expression     {$$ = $1 + $3; }
  | expression '-' expression   {$$ = $1 - $3; }
  | expression '*' expression   {$$ = $1 * $3; }
  | expression '/' expression   {$$ = $1 / $3; }
  | '-' expression %prec UMINUS {$$ = -$2;}
  | '(' expression ')'          {$$ = $2;}
  | K_IF b_expr then expression else expression %prec IF
                {$$ = ($2 != 0) ? $4 : $6;}
  | int_number                       {$$ = $1;}

b_expr:
  expression relop expression {$$ = comp_num($1,$2,$3);}
  | expression K_AND expression {$$ = $1 != 0 && $3 != 0;}
  | expression K_OR  expression {$$ = $1 != 0 || $3 != 0;}
  | s_expr relop s_expr       {$$ = comp_str($1,$2,$3);}
  | s_expr K_AND s_expr       {$$ = $1[0] != 0 && $3[0] != 0;}
  | s_expr K_OR  s_expr       {$$ = $1[0] != 0 || $3[0] != 0;}
  | b_expr K_EQ b_expr        {$$ = $1 == $3;}
  | b_expr K_NE b_expr        {$$ = $1 != $3;}
  | b_expr K_AND b_expr       {$$ = $1 && $3;}
  | b_expr K_OR  b_expr       {$$ = $1 || $3;}
  | K_NOT b_expr                    %prec LNOT {$$ = !$$;}
  | '(' b_expr ')'            {$$ = $2;}
  | K_IF b_expr then b_expr else b_expr %prec IF
        {$$ = ($2 != 0) ? $4 : $6;}
  | K_TRUE                    {$$ = 1;}
  | K_FALSE                   {$$ = 0;}

s_expr:
  s_expr '+' s_expr
    {
      $$ = (char*)lefMalloc(strlen($1)+strlen($3)+1);
      strcpy($$,$1);
      strcat($$,$3);
    }
  | '(' s_expr ')'
    { $$ = $2; }
  | K_IF b_expr then s_expr else s_expr %prec IF
    {
      lefData->lefDefIf = TRUE;
      if ($2 != 0) {
        $$ = $4;        
      } else {
        $$ = $6;
      }
    }
  | QSTRING
    { $$ = $1; }

relop:
  K_LE {$$ = C_LE;}
  | K_LT {$$ = C_LT;}
  | K_GE {$$ = C_GE;}
  | K_GT {$$ = C_GT;}
  | K_EQ {$$ = C_EQ;}
  | K_NE {$$ = C_NE;}
  | '='  {$$ = C_EQ;}
  | '<'  {$$ = C_LT;}
  | '>'  {$$ = C_GT;}


prop_def_section: K_PROPDEF
    { 
      if (lefCallbacks->PropBeginCbk)
        CALLBACK(lefCallbacks->PropBeginCbk, lefrPropBeginCbkType, 0);
    }
    prop_stmts K_END K_PROPDEF
    { 
      if (lefCallbacks->PropEndCbk)
        CALLBACK(lefCallbacks->PropEndCbk, lefrPropEndCbkType, 0);
    }

prop_stmts:
  // empty 
    { }
  | prop_stmts prop_stmt
    { }

prop_stmt:
  K_LIBRARY {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("library", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrLibProp.setPropType($3, lefData->lefPropDefType);
    }
  | K_COMPONENTPIN {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("componentpin", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrCompProp.setPropType($3, lefData->lefPropDefType);
    }
  | K_PIN {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("pin", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrPinProp.setPropType($3, lefData->lefPropDefType);
      
    }
  | K_MACRO {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("macro", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrMacroProp.setPropType($3, lefData->lefPropDefType);
    }
  | K_VIA {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("via", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrViaProp.setPropType($3, lefData->lefPropDefType);
    }
  | K_VIARULE {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("viarule", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrViaRuleProp.setPropType($3, lefData->lefPropDefType);
    }
  | K_LAYER {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("layer", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrLayerProp.setPropType($3, lefData->lefPropDefType);
    }
  | K_NONDEFAULTRULE {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
    T_STRING prop_define ';'
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("nondefaultrule", $3);
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrNondefProp.setPropType($3, lefData->lefPropDefType);
    }
    
prop_define:
  K_INTEGER  opt_def_range opt_def_dvalue 
    { 
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropInteger();
      lefData->lefPropDefType = 'I';
    }
  | K_REAL opt_def_range opt_def_value
    { 
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropReal();
      lefData->lefPropDefType = 'R';
    }
  | K_STRING
    {
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropString();
      lefData->lefPropDefType = 'S';
    }
  | K_STRING QSTRING
    {
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropQString($2);
      lefData->lefPropDefType = 'Q';
    }
  | K_NAMEMAPSTRING T_STRING
    {
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropNameMapString($2);
      lefData->lefPropDefType = 'S';
    }

opt_range_second:
  // nothing 
    { }
  | K_USELENGTHTHRESHOLD
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingRangeUseLength();
    }
  | K_INFLUENCE int_number
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingRangeInfluence($2);
        lefData->lefrLayer.setSpacingRangeInfluenceRange(-1, -1);
      }
    }
  | K_INFLUENCE int_number K_RANGE int_number int_number
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingRangeInfluence($2);
        lefData->lefrLayer.setSpacingRangeInfluenceRange($4, $5);
      }
    }
  | K_RANGE int_number int_number
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingRangeRange($2, $3);
    }

opt_endofline:                                      // 5.7 
  // nothing 
    { }
  | K_PARALLELEDGE int_number K_WITHIN int_number
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingParSW($2, $4);
    }
    opt_endofline_twoedges

opt_endofline_twoedges:                             // 5.7 
  // nothing 
    { }
  | K_TWOEDGES
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingParTwoEdges();
    }

opt_samenetPGonly:                                  // 5.7 
  // nothing 
    { }
  | K_PGONLY
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingSamenetPGonly();
    }

opt_def_range:
  // nothing 
    { }
  | K_RANGE int_number int_number
    {  if (lefCallbacks->PropCbk) lefData->lefrProp.setRange($2, $3); }

opt_def_value:
  // empty 
    { }
  | NUMBER
    { if (lefCallbacks->PropCbk) lefData->lefrProp.setNumber($1); }

opt_def_dvalue:
  // empty 
    { }
  | int_number
    { if (lefCallbacks->PropCbk) lefData->lefrProp.setNumber($1); }

layer_spacing_opts:
  // empty 
  | layer_spacing_opt layer_spacing_opts

layer_spacing_opt: K_CENTERTOCENTER      // 5.7 
    {
      if (lefCallbacks->LayerCbk) {
         if (lefData->hasSpCenter) {
           if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1663, "A CENTERTOCENTER statement was already defined in SPACING\nCENTERTOCENTER can only be defined once per LAYER CUT SPACING.");
              CHKERR();
           }
        }
        lefData->hasSpCenter = 1;
        if (lefData->versionNum < 5.6) {
           if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "CENTERTOCENTER statement is a version 5.6 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1664, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
        if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setSpacingCenterToCenter();
      }
    }
  | K_SAMENET             // 5.7 
    {
      if (lefCallbacks->LayerCbk) {
        if (lefData->hasSpSamenet) {
           if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1665, "A SAMENET statement was already defined in SPACING\nSAMENET can only be defined once per LAYER CUT SPACING.");
              CHKERR();
           }
        }
        lefData->hasSpSamenet = 1;
        if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setSpacingSamenet();
       }
    }
    opt_samenetPGonly
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "SAMENET is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1684, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      }
    }
  | K_PARALLELOVERLAP    // 5.7 
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "PARALLELOVERLAP is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1680, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR(); 
      } else {
        if (lefCallbacks->LayerCbk) {
          if (lefData->hasSpParallel) {
             if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                lefError(1666, "A PARALLELOVERLAP statement was already defined in SPACING\nPARALLELOVERLAP can only be defined once per LAYER CUT SPACING.");
                CHKERR();
             }
          }
          lefData->hasSpParallel = 1;
          if (lefCallbacks->LayerCbk)
            lefData->lefrLayer.setSpacingParallelOverlap();
        }
      }
    }

layer_spacing_cut_routing:
  // empty 
  | K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING
    {
      if (lefCallbacks->LayerCbk)
{
        if (lefData->versionNum < 5.7) {
           if (lefData->hasSpSamenet) {    // 5.6 and earlier does not allow 
              if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefError(1667, "A SAMENET statement was already defined in SPACING\nEither SAMENET or LAYER can be defined, but not both.");
                 CHKERR();
              }
           }
        }
        lefData->lefrLayer.setSpacingName($3);
      }
    }
    spacing_cut_layer_opt
  | K_ADJACENTCUTS int_number K_WITHIN int_number
    {
      if (lefCallbacks->LayerCbk) {
        if (lefData->versionNum < 5.5) {
           if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "ADJACENTCUTS statement is a version 5.5 and later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
              lefError(1668, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
           }
        }
        if (lefData->versionNum < 5.7) {
           if (lefData->hasSpSamenet) {    // 5.6 and earlier does not allow 
              if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                 lefError(1669, "A SAMENET statement was already defined in SPACING\nEither SAMENET or ADJACENTCUTS can be defined, but not both.");
                 CHKERR();
              }
           }
        }
        lefData->lefrLayer.setSpacingAdjacent((int)$2, $4);
      }
    }
    opt_adjacentcuts_exceptsame
  | K_AREA NUMBER        // 5.7 
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "AREA is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1693, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } else {
        if (lefCallbacks->LayerCbk) {
          if (lefData->versionNum < 5.7) {
             if (lefData->hasSpSamenet) {    // 5.6 and earlier does not allow 
                if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
                   lefError(1670, "A SAMENET statement was already defined in SPACING\nEither SAMENET or AREA can be defined, but not both.");
                   CHKERR();
                }
             }
          }
          lefData->lefrLayer.setSpacingArea($2);
        }
      }
    }
  | K_RANGE int_number int_number
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingRange($2, $3);
    }
    opt_range_second
  | K_LENGTHTHRESHOLD int_number
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingLength($2);
      }
    }
  | K_LENGTHTHRESHOLD int_number K_RANGE int_number int_number
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingLength($2);
        lefData->lefrLayer.setSpacingLengthRange($4, $5);
      }
    }
  | K_ENDOFLINE int_number K_WITHIN int_number    // 5.7 
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingEol($2, $4);
    }
    opt_endofline
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "ENDOFLINE is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1681, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      }
    }
  | K_NOTCHLENGTH int_number      // 5.7 
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "NOTCHLENGTH is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1682, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } else {
        if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setSpacingNotchLength($2);
      }
    }
  | K_ENDOFNOTCHWIDTH int_number K_NOTCHSPACING int_number K_NOTCHLENGTH int_number //5.7
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "ENDOFNOTCHWIDTH is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1696, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } else {
        if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setSpacingEndOfNotchWidth($2, $4, $6);
      }
    }

spacing_cut_layer_opt:                      // 5.7 
  // empty 
    {}
  | K_STACK
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingLayerStack();
    }

opt_adjacentcuts_exceptsame:                // 5.7 
  // empty 
    {}
  | K_EXCEPTSAMEPGNET
    {
      if (lefData->versionNum < 5.7) {
        lefData->outMsg = (char*)lefMalloc(10000);
        sprintf(lefData->outMsg,
          "EXCEPTSAMEPGNET is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
        lefError(1683, lefData->outMsg);
        lefFree(lefData->outMsg);
        CHKERR();
      } else {
        if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setSpacingAdjacentExcept();
      }
    }

opt_layer_name:
  // empty 
    { $$ = 0; }
  | K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING
    { $$ = $3; }

req_layer_name:
  // pcr 355313 
   K_LAYER {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; } T_STRING
    { $$ = $3; }

// 9/11/2001 - Wanda da Rosa.  The following are obsolete in 5.4 
universalnoisemargin: K_UNIVERSALNOISEMARGIN int_number int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->NoiseMarginCbk) {
          lefData->lefrNoiseMargin.low = $2;
          lefData->lefrNoiseMargin.high = $3;
          CALLBACK(lefCallbacks->NoiseMarginCbk, lefrNoiseMarginCbkType, &lefData->lefrNoiseMargin);
        }
      } else
        if (lefCallbacks->NoiseMarginCbk) // write warning only if cbk is set 
          if (lefData->noiseMarginWarnings++ < lefSettings->NoiseMarginWarnings)
            lefWarning(2070, "UNIVERSALNOISEMARGIN statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }

edgeratethreshold1: K_EDGERATETHRESHOLD1 int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->EdgeRateThreshold1Cbk) {
          CALLBACK(lefCallbacks->EdgeRateThreshold1Cbk,
          lefrEdgeRateThreshold1CbkType, $2);
        }
      } else
        if (lefCallbacks->EdgeRateThreshold1Cbk) // write warning only if cbk is set 
          if (lefData->edgeRateThreshold1Warnings++ < lefSettings->EdgeRateThreshold1Warnings)
            lefWarning(2071, "EDGERATETHRESHOLD1 statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }

edgeratethreshold2: K_EDGERATETHRESHOLD2 int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->EdgeRateThreshold2Cbk) {
          CALLBACK(lefCallbacks->EdgeRateThreshold2Cbk,
          lefrEdgeRateThreshold2CbkType, $2);
        }
      } else
        if (lefCallbacks->EdgeRateThreshold2Cbk) // write warning only if cbk is set 
          if (lefData->edgeRateThreshold2Warnings++ < lefSettings->EdgeRateThreshold2Warnings)
            lefWarning(2072, "EDGERATETHRESHOLD2 statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }

edgeratescalefactor: K_EDGERATESCALEFACTOR int_number ';'
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->EdgeRateScaleFactorCbk) {
          CALLBACK(lefCallbacks->EdgeRateScaleFactorCbk,
          lefrEdgeRateScaleFactorCbkType, $2);
        }
      } else
        if (lefCallbacks->EdgeRateScaleFactorCbk) // write warning only if cbk is set 
          if (lefData->edgeRateScaleFactorWarnings++ < lefSettings->EdgeRateScaleFactorWarnings)
            lefWarning(2073, "EDGERATESCALEFACTOR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }

noisetable: K_NOISETABLE int_number
    { if (lefCallbacks->NoiseTableCbk) lefData->lefrNoiseTable.setup((int)$2); }
    ';' noise_table_list end_noisetable dtrm
    { }

end_noisetable:
  K_END K_NOISETABLE
  {
    if (lefData->versionNum < 5.4) {
      if (lefCallbacks->NoiseTableCbk)
        CALLBACK(lefCallbacks->NoiseTableCbk, lefrNoiseTableCbkType, &lefData->lefrNoiseTable);
    } else
      if (lefCallbacks->NoiseTableCbk) // write warning only if cbk is set 
        if (lefData->noiseTableWarnings++ < lefSettings->NoiseTableWarnings)
          lefWarning(2074, "NOISETABLE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }


noise_table_list :
  noise_table_entry
  | noise_table_list noise_table_entry
  ;

noise_table_entry:
  K_EDGERATE int_number ';'
    { if (lefCallbacks->NoiseTableCbk)
         {
            lefData->lefrNoiseTable.newEdge();
            lefData->lefrNoiseTable.addEdge($2);
         }
    }
  | output_resistance_entry
    { }

output_resistance_entry: K_OUTPUTRESISTANCE
    { if (lefCallbacks->NoiseTableCbk) lefData->lefrNoiseTable.addResistance(); }
    num_list ';' victim_list
    ;

num_list:
  int_number
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addResistanceNumber($1); }
   | num_list int_number
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addResistanceNumber($2); }

victim_list:
  victim
  | victim_list victim
  ;

victim: K_VICTIMLENGTH int_number ';'
        { if (lefCallbacks->NoiseTableCbk)
        lefData->lefrNoiseTable.addVictimLength($2); }
      K_VICTIMNOISE vnoiselist ';'
        { }

vnoiselist:
  int_number
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addVictimNoise($1); }
  | vnoiselist int_number
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addVictimNoise($2); }

correctiontable: K_CORRECTIONTABLE int_number ';'
    { if (lefCallbacks->CorrectionTableCbk)
    lefData->lefrCorrectionTable.setup((int)$2); }
    correction_table_list end_correctiontable dtrm
    { }

end_correctiontable:
  K_END K_CORRECTIONTABLE
  {
    if (lefData->versionNum < 5.4) {
      if (lefCallbacks->CorrectionTableCbk)
        CALLBACK(lefCallbacks->CorrectionTableCbk, lefrCorrectionTableCbkType,
               &lefData->lefrCorrectionTable);
    } else
      if (lefCallbacks->CorrectionTableCbk) // write warning only if cbk is set 
        if (lefData->correctionTableWarnings++ < lefSettings->CorrectionTableWarnings)
          lefWarning(2075, "CORRECTIONTABLE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }

correction_table_list:
  correction_table_item
  | correction_table_list correction_table_item
  ;

correction_table_item:
  K_EDGERATE int_number ';'
    { if (lefCallbacks->CorrectionTableCbk)
         {
            lefData->lefrCorrectionTable.newEdge();
            lefData->lefrCorrectionTable.addEdge($2);
         }
    }
  | output_list
    { }

output_list: K_OUTPUTRESISTANCE
  { if (lefCallbacks->CorrectionTableCbk)
  lefData->lefrCorrectionTable.addResistance(); }
  numo_list ';' corr_victim_list
  { }

numo_list:
  int_number
    { if (lefCallbacks->CorrectionTableCbk)
    lefData->lefrCorrectionTable.addResistanceNumber($1); }
  | numo_list int_number
    { if (lefCallbacks->CorrectionTableCbk)
    lefData->lefrCorrectionTable.addResistanceNumber($2); }

corr_victim_list:
   corr_victim
   | corr_victim_list corr_victim
   ;

corr_victim:
  K_VICTIMLENGTH int_number ';'
     { if (lefCallbacks->CorrectionTableCbk)
     lefData->lefrCorrectionTable.addVictimLength($2); }
  K_CORRECTIONFACTOR corr_list ';'
     { }

corr_list:
  int_number
    { if (lefCallbacks->CorrectionTableCbk)
        lefData->lefrCorrectionTable.addVictimCorrection($1); }
  | corr_list int_number
    { if (lefCallbacks->CorrectionTableCbk)
        lefData->lefrCorrectionTable.addVictimCorrection($2); }

// end of 5.4 obsolete syntax 

input_antenna: K_INPUTPINANTENNASIZE int_number ';'
    { // 5.3 syntax 
        lefData->use5_3 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        } else if (lefData->versionNum > 5.3) {
           // A 5.3 syntax in 5.4 
           if (lefData->use5_4) {
              if (lefCallbacks->InputAntennaCbk) { // write warning only if cbk is set 
                if (lefData->inputAntennaWarnings++ < lefSettings->InputAntennaWarnings) {
                   lefData->outMsg = (char*)lefMalloc(10000);
                   sprintf (lefData->outMsg,
                      "INPUTPINANTENNASIZE statement is a version 5.3 or earlier syntax.\nYour lef file with version %g, has both old and new INPUTPINANTENNASIZE syntax, which is incorrect.", lefData->versionNum);
                   lefError(1671, lefData->outMsg);
                   lefFree(lefData->outMsg);
                   CHKERR();
                }
              }
           }
        }
        if (lefCallbacks->InputAntennaCbk)
          CALLBACK(lefCallbacks->InputAntennaCbk, lefrInputAntennaCbkType, $2);
    }

output_antenna: K_OUTPUTPINANTENNASIZE int_number ';'
    { // 5.3 syntax 
        lefData->use5_3 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        } else if (lefData->versionNum > 5.3) {
           // A 5.3 syntax in 5.4 
           if (lefData->use5_4) {
              if (lefCallbacks->OutputAntennaCbk) { // write warning only if cbk is set 
                if (lefData->outputAntennaWarnings++ < lefSettings->OutputAntennaWarnings) {
                   lefData->outMsg = (char*)lefMalloc(10000);
                   sprintf (lefData->outMsg,
                      "OUTPUTPINANTENNASIZE statement is a version 5.3 or earlier syntax.\nYour lef file with version %g, has both old and new OUTPUTPINANTENNASIZE syntax, which is incorrect.", lefData->versionNum);
                   lefError(1672, lefData->outMsg);
                   lefFree(lefData->outMsg);
                   CHKERR();
                }
              }
           }
        }
        if (lefCallbacks->OutputAntennaCbk)
          CALLBACK(lefCallbacks->OutputAntennaCbk, lefrOutputAntennaCbkType, $2);
    }

inout_antenna: K_INOUTPINANTENNASIZE int_number ';'
    { // 5.3 syntax 
        lefData->use5_3 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        } else if (lefData->versionNum > 5.3) {
           // A 5.3 syntax in 5.4 
           if (lefData->use5_4) {
              if (lefCallbacks->InoutAntennaCbk) { // write warning only if cbk is set 
                if (lefData->inoutAntennaWarnings++ < lefSettings->InoutAntennaWarnings) {
                   lefData->outMsg = (char*)lefMalloc(10000);
                   sprintf (lefData->outMsg,
                      "INOUTPINANTENNASIZE statement is a version 5.3 or earlier syntax.\nYour lef file with version %g, has both old and new INOUTPINANTENNASIZE syntax, which is incorrect.", lefData->versionNum);
                   lefError(1673, lefData->outMsg);
                   lefFree(lefData->outMsg);
                   CHKERR();
                }
              }
           }
        }
        if (lefCallbacks->InoutAntennaCbk)
          CALLBACK(lefCallbacks->InoutAntennaCbk, lefrInoutAntennaCbkType, $2);
    }

antenna_input: K_ANTENNAINPUTGATEAREA NUMBER ';'
    { // 5.4 syntax 
        // 11/12/2002 - this is obsolete in 5.5, suppose should be ingored 
        // 12/16/2002 - talked to Dave Noice, leave them in here for debugging
        lefData->use5_4 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        } else if (lefData->versionNum < 5.4) {
           if (lefCallbacks->AntennaInputCbk) { // write warning only if cbk is set 
             if (lefData->antennaInputWarnings++ < lefSettings->AntennaInputWarnings) {
               lefData->outMsg = (char*)lefMalloc(10000);
               sprintf (lefData->outMsg,
                  "ANTENNAINPUTGATEAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.\nEither update your VERSION number or use the 5.3 syntax.", lefData->versionNum);
               lefError(1674, lefData->outMsg);
               lefFree(lefData->outMsg);
               CHKERR();
             }
           }
        } else if (lefData->use5_3) {
           if (lefCallbacks->AntennaInputCbk) { // write warning only if cbk is set 
             if (lefData->antennaInputWarnings++ < lefSettings->AntennaInputWarnings) {
                lefData->outMsg = (char*)lefMalloc(10000);
                sprintf (lefData->outMsg,
                   "ANTENNAINPUTGATEAREA statement is a version 5.4 or later syntax.\nYour lef file with version %g, has both old and new ANTENNAINPUTGATEAREA syntax, which is incorrect.", lefData->versionNum);
                lefError(1675, lefData->outMsg);
                lefFree(lefData->outMsg);
               CHKERR();
             }
           }
        }
        if (lefCallbacks->AntennaInputCbk)
          CALLBACK(lefCallbacks->AntennaInputCbk, lefrAntennaInputCbkType, $2);
    }

antenna_inout: K_ANTENNAINOUTDIFFAREA NUMBER ';'
    { // 5.4 syntax 
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        // 12/16/2002 - talked to Dave Noice, leave them in here for debugging
        lefData->use5_4 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        } else if (lefData->versionNum < 5.4) {
           if (lefCallbacks->AntennaInoutCbk) { // write warning only if cbk is set 
              if (lefData->antennaInoutWarnings++ < lefSettings->AntennaInoutWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "ANTENNAINOUTDIFFAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.\nEither update your VERSION number or use the 5.3 syntax.", lefData->versionNum);
                 lefError(1676, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        } else if (lefData->use5_3) {
           if (lefCallbacks->AntennaInoutCbk) { // write warning only if cbk is set 
              if (lefData->antennaInoutWarnings++ < lefSettings->AntennaInoutWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "ANTENNAINOUTDIFFAREA statement is a version 5.4 or later syntax.\nYour lef file with version %g, has both old and new ANTENNAINOUTDIFFAREA syntax, which is incorrect.", lefData->versionNum);
                 lefError(1677, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
        if (lefCallbacks->AntennaInoutCbk)
          CALLBACK(lefCallbacks->AntennaInoutCbk, lefrAntennaInoutCbkType, $2);
    }

antenna_output: K_ANTENNAOUTPUTDIFFAREA NUMBER ';'
    { // 5.4 syntax 
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        // 12/16/2002 - talked to Dave Noice, leave them in here for debugging
        lefData->use5_4 = 1;
        if (lefData->ignoreVersion) {
           // do nothing 
        } else if (lefData->versionNum < 5.4) {
           if (lefCallbacks->AntennaOutputCbk) { // write warning only if cbk is set 
              if (lefData->antennaOutputWarnings++ < lefSettings->AntennaOutputWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "ANTENNAOUTPUTDIFFAREA statement is a version 5.4 and later syntax.\nYour lef file is defined with version %g.\nEither update your VERSION number or use the 5.3 syntax.", lefData->versionNum);
                 lefError(1678, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        } else if (lefData->use5_3) {
           if (lefCallbacks->AntennaOutputCbk) { // write warning only if cbk is set 
              if (lefData->antennaOutputWarnings++ < lefSettings->AntennaOutputWarnings) {
                 lefData->outMsg = (char*)lefMalloc(10000);
                 sprintf (lefData->outMsg,
                    "ANTENNAOUTPUTDIFFAREA statement is a version 5.4 or later syntax.\nYour lef file with version %g, has both old and new ANTENNAOUTPUTDIFFAREA syntax, which is incorrect.", lefData->versionNum);
                 lefError(1679, lefData->outMsg);
                 lefFree(lefData->outMsg);
                 CHKERR();
              }
           }
        }
        if (lefCallbacks->AntennaOutputCbk)
          CALLBACK(lefCallbacks->AntennaOutputCbk, lefrAntennaOutputCbkType, $2);
    }

extension_opt:  // empty 
    | extension

extension: K_BEGINEXT
    { 
        if (lefCallbacks->ExtensionCbk)
          CALLBACK(lefCallbacks->ExtensionCbk, lefrExtensionCbkType, &lefData->Hist_text[0]);
        if (lefData->versionNum >= 5.6)
           lefData->ge56almostDone = 1;
    }

%%

END_LEFDEF_PARSER_NAMESPACE
