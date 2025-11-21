/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         lefyyparse
#define yylex           lefyylex
#define yyerror         lefyyerror
#define yydebug         lefyydebug
#define yynerrs         lefyynerrs
#define yylval          lefyylval
#define yychar          lefyychar

/* First part of user prologue.  */
#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"

#include <cstring>
#include <cstdlib>
#include <cmath>

#include "lex.h"
#include "lefiDefs.hpp"
#include "lefiUser.hpp"
#include "lefiUtil.hpp"

#include "lefrData.hpp"
#include "lefrCallBacks.hpp"
#include "lefrSettings.hpp"

#ifndef WIN32
  // Only include this on non-Windows platforms
  #include "lef_parser.hpp"
#endif

BEGIN_LEF_PARSER_NAMESPACE

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


#line 225 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "lef_parser.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_K_HISTORY = 3,                  /* K_HISTORY  */
  YYSYMBOL_K_ABUT = 4,                     /* K_ABUT  */
  YYSYMBOL_K_ABUTMENT = 5,                 /* K_ABUTMENT  */
  YYSYMBOL_K_ACTIVE = 6,                   /* K_ACTIVE  */
  YYSYMBOL_K_ANALOG = 7,                   /* K_ANALOG  */
  YYSYMBOL_K_ARRAY = 8,                    /* K_ARRAY  */
  YYSYMBOL_K_AREA = 9,                     /* K_AREA  */
  YYSYMBOL_K_BLOCK = 10,                   /* K_BLOCK  */
  YYSYMBOL_K_BOTTOMLEFT = 11,              /* K_BOTTOMLEFT  */
  YYSYMBOL_K_BOTTOMRIGHT = 12,             /* K_BOTTOMRIGHT  */
  YYSYMBOL_K_BY = 13,                      /* K_BY  */
  YYSYMBOL_K_CAPACITANCE = 14,             /* K_CAPACITANCE  */
  YYSYMBOL_K_CAPMULTIPLIER = 15,           /* K_CAPMULTIPLIER  */
  YYSYMBOL_K_CLASS = 16,                   /* K_CLASS  */
  YYSYMBOL_K_CLOCK = 17,                   /* K_CLOCK  */
  YYSYMBOL_K_CLOCKTYPE = 18,               /* K_CLOCKTYPE  */
  YYSYMBOL_K_COLUMNMAJOR = 19,             /* K_COLUMNMAJOR  */
  YYSYMBOL_K_DESIGNRULEWIDTH = 20,         /* K_DESIGNRULEWIDTH  */
  YYSYMBOL_K_INFLUENCE = 21,               /* K_INFLUENCE  */
  YYSYMBOL_K_CORE = 22,                    /* K_CORE  */
  YYSYMBOL_K_CORNER = 23,                  /* K_CORNER  */
  YYSYMBOL_K_COVER = 24,                   /* K_COVER  */
  YYSYMBOL_K_CPERSQDIST = 25,              /* K_CPERSQDIST  */
  YYSYMBOL_K_CURRENT = 26,                 /* K_CURRENT  */
  YYSYMBOL_K_CURRENTSOURCE = 27,           /* K_CURRENTSOURCE  */
  YYSYMBOL_K_CUT = 28,                     /* K_CUT  */
  YYSYMBOL_K_DEFAULT = 29,                 /* K_DEFAULT  */
  YYSYMBOL_K_DATABASE = 30,                /* K_DATABASE  */
  YYSYMBOL_K_DATA = 31,                    /* K_DATA  */
  YYSYMBOL_K_DIELECTRIC = 32,              /* K_DIELECTRIC  */
  YYSYMBOL_K_DIRECTION = 33,               /* K_DIRECTION  */
  YYSYMBOL_K_DO = 34,                      /* K_DO  */
  YYSYMBOL_K_EDGECAPACITANCE = 35,         /* K_EDGECAPACITANCE  */
  YYSYMBOL_K_EEQ = 36,                     /* K_EEQ  */
  YYSYMBOL_K_END = 37,                     /* K_END  */
  YYSYMBOL_K_ENDCAP = 38,                  /* K_ENDCAP  */
  YYSYMBOL_K_FALL = 39,                    /* K_FALL  */
  YYSYMBOL_K_FALLCS = 40,                  /* K_FALLCS  */
  YYSYMBOL_K_FALLT0 = 41,                  /* K_FALLT0  */
  YYSYMBOL_K_FALLSATT1 = 42,               /* K_FALLSATT1  */
  YYSYMBOL_K_FALLRS = 43,                  /* K_FALLRS  */
  YYSYMBOL_K_FALLSATCUR = 44,              /* K_FALLSATCUR  */
  YYSYMBOL_K_FALLTHRESH = 45,              /* K_FALLTHRESH  */
  YYSYMBOL_K_FEEDTHRU = 46,                /* K_FEEDTHRU  */
  YYSYMBOL_K_FIXED = 47,                   /* K_FIXED  */
  YYSYMBOL_K_FOREIGN = 48,                 /* K_FOREIGN  */
  YYSYMBOL_K_FROMPIN = 49,                 /* K_FROMPIN  */
  YYSYMBOL_K_GENERATE = 50,                /* K_GENERATE  */
  YYSYMBOL_K_GENERATOR = 51,               /* K_GENERATOR  */
  YYSYMBOL_K_GROUND = 52,                  /* K_GROUND  */
  YYSYMBOL_K_HEIGHT = 53,                  /* K_HEIGHT  */
  YYSYMBOL_K_HORIZONTAL = 54,              /* K_HORIZONTAL  */
  YYSYMBOL_K_INOUT = 55,                   /* K_INOUT  */
  YYSYMBOL_K_INPUT = 56,                   /* K_INPUT  */
  YYSYMBOL_K_INPUTNOISEMARGIN = 57,        /* K_INPUTNOISEMARGIN  */
  YYSYMBOL_K_COMPONENTPIN = 58,            /* K_COMPONENTPIN  */
  YYSYMBOL_K_INTRINSIC = 59,               /* K_INTRINSIC  */
  YYSYMBOL_K_INVERT = 60,                  /* K_INVERT  */
  YYSYMBOL_K_IRDROP = 61,                  /* K_IRDROP  */
  YYSYMBOL_K_ITERATE = 62,                 /* K_ITERATE  */
  YYSYMBOL_K_IV_TABLES = 63,               /* K_IV_TABLES  */
  YYSYMBOL_K_LAYER = 64,                   /* K_LAYER  */
  YYSYMBOL_K_LEAKAGE = 65,                 /* K_LEAKAGE  */
  YYSYMBOL_K_LEQ = 66,                     /* K_LEQ  */
  YYSYMBOL_K_LIBRARY = 67,                 /* K_LIBRARY  */
  YYSYMBOL_K_MACRO = 68,                   /* K_MACRO  */
  YYSYMBOL_K_MATCH = 69,                   /* K_MATCH  */
  YYSYMBOL_K_MAXDELAY = 70,                /* K_MAXDELAY  */
  YYSYMBOL_K_MAXLOAD = 71,                 /* K_MAXLOAD  */
  YYSYMBOL_K_METALOVERHANG = 72,           /* K_METALOVERHANG  */
  YYSYMBOL_K_MILLIAMPS = 73,               /* K_MILLIAMPS  */
  YYSYMBOL_K_MILLIWATTS = 74,              /* K_MILLIWATTS  */
  YYSYMBOL_K_MINFEATURE = 75,              /* K_MINFEATURE  */
  YYSYMBOL_K_MUSTJOIN = 76,                /* K_MUSTJOIN  */
  YYSYMBOL_K_NAMESCASESENSITIVE = 77,      /* K_NAMESCASESENSITIVE  */
  YYSYMBOL_K_NANOSECONDS = 78,             /* K_NANOSECONDS  */
  YYSYMBOL_K_NETS = 79,                    /* K_NETS  */
  YYSYMBOL_K_NEW = 80,                     /* K_NEW  */
  YYSYMBOL_K_NONDEFAULTRULE = 81,          /* K_NONDEFAULTRULE  */
  YYSYMBOL_K_NONINVERT = 82,               /* K_NONINVERT  */
  YYSYMBOL_K_NONUNATE = 83,                /* K_NONUNATE  */
  YYSYMBOL_K_OBS = 84,                     /* K_OBS  */
  YYSYMBOL_K_OHMS = 85,                    /* K_OHMS  */
  YYSYMBOL_K_OFFSET = 86,                  /* K_OFFSET  */
  YYSYMBOL_K_ORIENTATION = 87,             /* K_ORIENTATION  */
  YYSYMBOL_K_ORIGIN = 88,                  /* K_ORIGIN  */
  YYSYMBOL_K_OUTPUT = 89,                  /* K_OUTPUT  */
  YYSYMBOL_K_OUTPUTNOISEMARGIN = 90,       /* K_OUTPUTNOISEMARGIN  */
  YYSYMBOL_K_OVERHANG = 91,                /* K_OVERHANG  */
  YYSYMBOL_K_OVERLAP = 92,                 /* K_OVERLAP  */
  YYSYMBOL_K_OFF = 93,                     /* K_OFF  */
  YYSYMBOL_K_ON = 94,                      /* K_ON  */
  YYSYMBOL_K_OVERLAPS = 95,                /* K_OVERLAPS  */
  YYSYMBOL_K_PAD = 96,                     /* K_PAD  */
  YYSYMBOL_K_PATH = 97,                    /* K_PATH  */
  YYSYMBOL_K_PATTERN = 98,                 /* K_PATTERN  */
  YYSYMBOL_K_PICOFARADS = 99,              /* K_PICOFARADS  */
  YYSYMBOL_K_PIN = 100,                    /* K_PIN  */
  YYSYMBOL_K_PITCH = 101,                  /* K_PITCH  */
  YYSYMBOL_K_PLACED = 102,                 /* K_PLACED  */
  YYSYMBOL_K_POLYGON = 103,                /* K_POLYGON  */
  YYSYMBOL_K_PORT = 104,                   /* K_PORT  */
  YYSYMBOL_K_POST = 105,                   /* K_POST  */
  YYSYMBOL_K_POWER = 106,                  /* K_POWER  */
  YYSYMBOL_K_PRE = 107,                    /* K_PRE  */
  YYSYMBOL_K_PULLDOWNRES = 108,            /* K_PULLDOWNRES  */
  YYSYMBOL_K_RECT = 109,                   /* K_RECT  */
  YYSYMBOL_K_RESISTANCE = 110,             /* K_RESISTANCE  */
  YYSYMBOL_K_RESISTIVE = 111,              /* K_RESISTIVE  */
  YYSYMBOL_K_RING = 112,                   /* K_RING  */
  YYSYMBOL_K_RISE = 113,                   /* K_RISE  */
  YYSYMBOL_K_RISECS = 114,                 /* K_RISECS  */
  YYSYMBOL_K_RISERS = 115,                 /* K_RISERS  */
  YYSYMBOL_K_RISESATCUR = 116,             /* K_RISESATCUR  */
  YYSYMBOL_K_RISETHRESH = 117,             /* K_RISETHRESH  */
  YYSYMBOL_K_RISESATT1 = 118,              /* K_RISESATT1  */
  YYSYMBOL_K_RISET0 = 119,                 /* K_RISET0  */
  YYSYMBOL_K_RISEVOLTAGETHRESHOLD = 120,   /* K_RISEVOLTAGETHRESHOLD  */
  YYSYMBOL_K_FALLVOLTAGETHRESHOLD = 121,   /* K_FALLVOLTAGETHRESHOLD  */
  YYSYMBOL_K_ROUTING = 122,                /* K_ROUTING  */
  YYSYMBOL_K_ROWMAJOR = 123,               /* K_ROWMAJOR  */
  YYSYMBOL_K_RPERSQ = 124,                 /* K_RPERSQ  */
  YYSYMBOL_K_SAMENET = 125,                /* K_SAMENET  */
  YYSYMBOL_K_SCANUSE = 126,                /* K_SCANUSE  */
  YYSYMBOL_K_SHAPE = 127,                  /* K_SHAPE  */
  YYSYMBOL_K_SHRINKAGE = 128,              /* K_SHRINKAGE  */
  YYSYMBOL_K_SIGNAL = 129,                 /* K_SIGNAL  */
  YYSYMBOL_K_SITE = 130,                   /* K_SITE  */
  YYSYMBOL_K_SIZE = 131,                   /* K_SIZE  */
  YYSYMBOL_K_SOURCE = 132,                 /* K_SOURCE  */
  YYSYMBOL_K_SPACER = 133,                 /* K_SPACER  */
  YYSYMBOL_K_SPACING = 134,                /* K_SPACING  */
  YYSYMBOL_K_SPECIALNETS = 135,            /* K_SPECIALNETS  */
  YYSYMBOL_K_STACK = 136,                  /* K_STACK  */
  YYSYMBOL_K_START = 137,                  /* K_START  */
  YYSYMBOL_K_STEP = 138,                   /* K_STEP  */
  YYSYMBOL_K_STOP = 139,                   /* K_STOP  */
  YYSYMBOL_K_STRUCTURE = 140,              /* K_STRUCTURE  */
  YYSYMBOL_K_SYMMETRY = 141,               /* K_SYMMETRY  */
  YYSYMBOL_K_TABLE = 142,                  /* K_TABLE  */
  YYSYMBOL_K_THICKNESS = 143,              /* K_THICKNESS  */
  YYSYMBOL_K_TIEHIGH = 144,                /* K_TIEHIGH  */
  YYSYMBOL_K_TIELOW = 145,                 /* K_TIELOW  */
  YYSYMBOL_K_TIEOFFR = 146,                /* K_TIEOFFR  */
  YYSYMBOL_K_TIME = 147,                   /* K_TIME  */
  YYSYMBOL_K_TIMING = 148,                 /* K_TIMING  */
  YYSYMBOL_K_TO = 149,                     /* K_TO  */
  YYSYMBOL_K_TOPIN = 150,                  /* K_TOPIN  */
  YYSYMBOL_K_TOPLEFT = 151,                /* K_TOPLEFT  */
  YYSYMBOL_K_TOPRIGHT = 152,               /* K_TOPRIGHT  */
  YYSYMBOL_K_TOPOFSTACKONLY = 153,         /* K_TOPOFSTACKONLY  */
  YYSYMBOL_K_TRISTATE = 154,               /* K_TRISTATE  */
  YYSYMBOL_K_TYPE = 155,                   /* K_TYPE  */
  YYSYMBOL_K_UNATENESS = 156,              /* K_UNATENESS  */
  YYSYMBOL_K_UNITS = 157,                  /* K_UNITS  */
  YYSYMBOL_K_USE = 158,                    /* K_USE  */
  YYSYMBOL_K_VARIABLE = 159,               /* K_VARIABLE  */
  YYSYMBOL_K_VERTICAL = 160,               /* K_VERTICAL  */
  YYSYMBOL_K_VHI = 161,                    /* K_VHI  */
  YYSYMBOL_K_VIA = 162,                    /* K_VIA  */
  YYSYMBOL_K_VIARULE = 163,                /* K_VIARULE  */
  YYSYMBOL_K_VLO = 164,                    /* K_VLO  */
  YYSYMBOL_K_VOLTAGE = 165,                /* K_VOLTAGE  */
  YYSYMBOL_K_VOLTS = 166,                  /* K_VOLTS  */
  YYSYMBOL_K_WIDTH = 167,                  /* K_WIDTH  */
  YYSYMBOL_K_X = 168,                      /* K_X  */
  YYSYMBOL_K_Y = 169,                      /* K_Y  */
  YYSYMBOL_T_STRING = 170,                 /* T_STRING  */
  YYSYMBOL_QSTRING = 171,                  /* QSTRING  */
  YYSYMBOL_NUMBER = 172,                   /* NUMBER  */
  YYSYMBOL_K_N = 173,                      /* K_N  */
  YYSYMBOL_K_S = 174,                      /* K_S  */
  YYSYMBOL_K_E = 175,                      /* K_E  */
  YYSYMBOL_K_W = 176,                      /* K_W  */
  YYSYMBOL_K_FN = 177,                     /* K_FN  */
  YYSYMBOL_K_FS = 178,                     /* K_FS  */
  YYSYMBOL_K_FE = 179,                     /* K_FE  */
  YYSYMBOL_K_FW = 180,                     /* K_FW  */
  YYSYMBOL_K_R0 = 181,                     /* K_R0  */
  YYSYMBOL_K_R90 = 182,                    /* K_R90  */
  YYSYMBOL_K_R180 = 183,                   /* K_R180  */
  YYSYMBOL_K_R270 = 184,                   /* K_R270  */
  YYSYMBOL_K_MX = 185,                     /* K_MX  */
  YYSYMBOL_K_MY = 186,                     /* K_MY  */
  YYSYMBOL_K_MXR90 = 187,                  /* K_MXR90  */
  YYSYMBOL_K_MYR90 = 188,                  /* K_MYR90  */
  YYSYMBOL_K_USER = 189,                   /* K_USER  */
  YYSYMBOL_K_MASTERSLICE = 190,            /* K_MASTERSLICE  */
  YYSYMBOL_K_ENDMACRO = 191,               /* K_ENDMACRO  */
  YYSYMBOL_K_ENDMACROPIN = 192,            /* K_ENDMACROPIN  */
  YYSYMBOL_K_ENDVIARULE = 193,             /* K_ENDVIARULE  */
  YYSYMBOL_K_ENDVIA = 194,                 /* K_ENDVIA  */
  YYSYMBOL_K_ENDLAYER = 195,               /* K_ENDLAYER  */
  YYSYMBOL_K_ENDSITE = 196,                /* K_ENDSITE  */
  YYSYMBOL_K_CANPLACE = 197,               /* K_CANPLACE  */
  YYSYMBOL_K_CANNOTOCCUPY = 198,           /* K_CANNOTOCCUPY  */
  YYSYMBOL_K_TRACKS = 199,                 /* K_TRACKS  */
  YYSYMBOL_K_FLOORPLAN = 200,              /* K_FLOORPLAN  */
  YYSYMBOL_K_GCELLGRID = 201,              /* K_GCELLGRID  */
  YYSYMBOL_K_DEFAULTCAP = 202,             /* K_DEFAULTCAP  */
  YYSYMBOL_K_MINPINS = 203,                /* K_MINPINS  */
  YYSYMBOL_K_WIRECAP = 204,                /* K_WIRECAP  */
  YYSYMBOL_K_STABLE = 205,                 /* K_STABLE  */
  YYSYMBOL_K_SETUP = 206,                  /* K_SETUP  */
  YYSYMBOL_K_HOLD = 207,                   /* K_HOLD  */
  YYSYMBOL_K_DEFINE = 208,                 /* K_DEFINE  */
  YYSYMBOL_K_DEFINES = 209,                /* K_DEFINES  */
  YYSYMBOL_K_DEFINEB = 210,                /* K_DEFINEB  */
  YYSYMBOL_K_IF = 211,                     /* K_IF  */
  YYSYMBOL_K_THEN = 212,                   /* K_THEN  */
  YYSYMBOL_K_ELSE = 213,                   /* K_ELSE  */
  YYSYMBOL_K_FALSE = 214,                  /* K_FALSE  */
  YYSYMBOL_K_TRUE = 215,                   /* K_TRUE  */
  YYSYMBOL_K_EQ = 216,                     /* K_EQ  */
  YYSYMBOL_K_NE = 217,                     /* K_NE  */
  YYSYMBOL_K_LE = 218,                     /* K_LE  */
  YYSYMBOL_K_LT = 219,                     /* K_LT  */
  YYSYMBOL_K_GE = 220,                     /* K_GE  */
  YYSYMBOL_K_GT = 221,                     /* K_GT  */
  YYSYMBOL_K_OR = 222,                     /* K_OR  */
  YYSYMBOL_K_AND = 223,                    /* K_AND  */
  YYSYMBOL_K_NOT = 224,                    /* K_NOT  */
  YYSYMBOL_K_DELAY = 225,                  /* K_DELAY  */
  YYSYMBOL_K_TABLEDIMENSION = 226,         /* K_TABLEDIMENSION  */
  YYSYMBOL_K_TABLEAXIS = 227,              /* K_TABLEAXIS  */
  YYSYMBOL_K_TABLEENTRIES = 228,           /* K_TABLEENTRIES  */
  YYSYMBOL_K_TRANSITIONTIME = 229,         /* K_TRANSITIONTIME  */
  YYSYMBOL_K_EXTENSION = 230,              /* K_EXTENSION  */
  YYSYMBOL_K_PROPDEF = 231,                /* K_PROPDEF  */
  YYSYMBOL_K_STRING = 232,                 /* K_STRING  */
  YYSYMBOL_K_INTEGER = 233,                /* K_INTEGER  */
  YYSYMBOL_K_REAL = 234,                   /* K_REAL  */
  YYSYMBOL_K_RANGE = 235,                  /* K_RANGE  */
  YYSYMBOL_K_PROPERTY = 236,               /* K_PROPERTY  */
  YYSYMBOL_K_VIRTUAL = 237,                /* K_VIRTUAL  */
  YYSYMBOL_K_BUSBITCHARS = 238,            /* K_BUSBITCHARS  */
  YYSYMBOL_K_VERSION = 239,                /* K_VERSION  */
  YYSYMBOL_K_BEGINEXT = 240,               /* K_BEGINEXT  */
  YYSYMBOL_K_ENDEXT = 241,                 /* K_ENDEXT  */
  YYSYMBOL_K_UNIVERSALNOISEMARGIN = 242,   /* K_UNIVERSALNOISEMARGIN  */
  YYSYMBOL_K_EDGERATETHRESHOLD1 = 243,     /* K_EDGERATETHRESHOLD1  */
  YYSYMBOL_K_CORRECTIONTABLE = 244,        /* K_CORRECTIONTABLE  */
  YYSYMBOL_K_EDGERATESCALEFACTOR = 245,    /* K_EDGERATESCALEFACTOR  */
  YYSYMBOL_K_EDGERATETHRESHOLD2 = 246,     /* K_EDGERATETHRESHOLD2  */
  YYSYMBOL_K_VICTIMNOISE = 247,            /* K_VICTIMNOISE  */
  YYSYMBOL_K_NOISETABLE = 248,             /* K_NOISETABLE  */
  YYSYMBOL_K_EDGERATE = 249,               /* K_EDGERATE  */
  YYSYMBOL_K_OUTPUTRESISTANCE = 250,       /* K_OUTPUTRESISTANCE  */
  YYSYMBOL_K_VICTIMLENGTH = 251,           /* K_VICTIMLENGTH  */
  YYSYMBOL_K_CORRECTIONFACTOR = 252,       /* K_CORRECTIONFACTOR  */
  YYSYMBOL_K_OUTPUTPINANTENNASIZE = 253,   /* K_OUTPUTPINANTENNASIZE  */
  YYSYMBOL_K_INPUTPINANTENNASIZE = 254,    /* K_INPUTPINANTENNASIZE  */
  YYSYMBOL_K_INOUTPINANTENNASIZE = 255,    /* K_INOUTPINANTENNASIZE  */
  YYSYMBOL_K_CURRENTDEN = 256,             /* K_CURRENTDEN  */
  YYSYMBOL_K_PWL = 257,                    /* K_PWL  */
  YYSYMBOL_K_ANTENNALENGTHFACTOR = 258,    /* K_ANTENNALENGTHFACTOR  */
  YYSYMBOL_K_TAPERRULE = 259,              /* K_TAPERRULE  */
  YYSYMBOL_K_DIVIDERCHAR = 260,            /* K_DIVIDERCHAR  */
  YYSYMBOL_K_ANTENNASIZE = 261,            /* K_ANTENNASIZE  */
  YYSYMBOL_K_ANTENNAMETALLENGTH = 262,     /* K_ANTENNAMETALLENGTH  */
  YYSYMBOL_K_ANTENNAMETALAREA = 263,       /* K_ANTENNAMETALAREA  */
  YYSYMBOL_K_RISESLEWLIMIT = 264,          /* K_RISESLEWLIMIT  */
  YYSYMBOL_K_FALLSLEWLIMIT = 265,          /* K_FALLSLEWLIMIT  */
  YYSYMBOL_K_FUNCTION = 266,               /* K_FUNCTION  */
  YYSYMBOL_K_BUFFER = 267,                 /* K_BUFFER  */
  YYSYMBOL_K_INVERTER = 268,               /* K_INVERTER  */
  YYSYMBOL_K_NAMEMAPSTRING = 269,          /* K_NAMEMAPSTRING  */
  YYSYMBOL_K_NOWIREEXTENSIONATPIN = 270,   /* K_NOWIREEXTENSIONATPIN  */
  YYSYMBOL_K_WIREEXTENSION = 271,          /* K_WIREEXTENSION  */
  YYSYMBOL_K_MESSAGE = 272,                /* K_MESSAGE  */
  YYSYMBOL_K_CREATEFILE = 273,             /* K_CREATEFILE  */
  YYSYMBOL_K_OPENFILE = 274,               /* K_OPENFILE  */
  YYSYMBOL_K_CLOSEFILE = 275,              /* K_CLOSEFILE  */
  YYSYMBOL_K_WARNING = 276,                /* K_WARNING  */
  YYSYMBOL_K_ERROR = 277,                  /* K_ERROR  */
  YYSYMBOL_K_FATALERROR = 278,             /* K_FATALERROR  */
  YYSYMBOL_K_RECOVERY = 279,               /* K_RECOVERY  */
  YYSYMBOL_K_SKEW = 280,                   /* K_SKEW  */
  YYSYMBOL_K_ANYEDGE = 281,                /* K_ANYEDGE  */
  YYSYMBOL_K_POSEDGE = 282,                /* K_POSEDGE  */
  YYSYMBOL_K_NEGEDGE = 283,                /* K_NEGEDGE  */
  YYSYMBOL_K_SDFCONDSTART = 284,           /* K_SDFCONDSTART  */
  YYSYMBOL_K_SDFCONDEND = 285,             /* K_SDFCONDEND  */
  YYSYMBOL_K_SDFCOND = 286,                /* K_SDFCOND  */
  YYSYMBOL_K_MPWH = 287,                   /* K_MPWH  */
  YYSYMBOL_K_MPWL = 288,                   /* K_MPWL  */
  YYSYMBOL_K_PERIOD = 289,                 /* K_PERIOD  */
  YYSYMBOL_K_ACCURRENTDENSITY = 290,       /* K_ACCURRENTDENSITY  */
  YYSYMBOL_K_DCCURRENTDENSITY = 291,       /* K_DCCURRENTDENSITY  */
  YYSYMBOL_K_AVERAGE = 292,                /* K_AVERAGE  */
  YYSYMBOL_K_PEAK = 293,                   /* K_PEAK  */
  YYSYMBOL_K_RMS = 294,                    /* K_RMS  */
  YYSYMBOL_K_FREQUENCY = 295,              /* K_FREQUENCY  */
  YYSYMBOL_K_CUTAREA = 296,                /* K_CUTAREA  */
  YYSYMBOL_K_MEGAHERTZ = 297,              /* K_MEGAHERTZ  */
  YYSYMBOL_K_USELENGTHTHRESHOLD = 298,     /* K_USELENGTHTHRESHOLD  */
  YYSYMBOL_K_LENGTHTHRESHOLD = 299,        /* K_LENGTHTHRESHOLD  */
  YYSYMBOL_K_ANTENNAINPUTGATEAREA = 300,   /* K_ANTENNAINPUTGATEAREA  */
  YYSYMBOL_K_ANTENNAINOUTDIFFAREA = 301,   /* K_ANTENNAINOUTDIFFAREA  */
  YYSYMBOL_K_ANTENNAOUTPUTDIFFAREA = 302,  /* K_ANTENNAOUTPUTDIFFAREA  */
  YYSYMBOL_K_ANTENNAAREARATIO = 303,       /* K_ANTENNAAREARATIO  */
  YYSYMBOL_K_ANTENNADIFFAREARATIO = 304,   /* K_ANTENNADIFFAREARATIO  */
  YYSYMBOL_K_ANTENNACUMAREARATIO = 305,    /* K_ANTENNACUMAREARATIO  */
  YYSYMBOL_K_ANTENNACUMDIFFAREARATIO = 306, /* K_ANTENNACUMDIFFAREARATIO  */
  YYSYMBOL_K_ANTENNAAREAFACTOR = 307,      /* K_ANTENNAAREAFACTOR  */
  YYSYMBOL_K_ANTENNASIDEAREARATIO = 308,   /* K_ANTENNASIDEAREARATIO  */
  YYSYMBOL_K_ANTENNADIFFSIDEAREARATIO = 309, /* K_ANTENNADIFFSIDEAREARATIO  */
  YYSYMBOL_K_ANTENNACUMSIDEAREARATIO = 310, /* K_ANTENNACUMSIDEAREARATIO  */
  YYSYMBOL_K_ANTENNACUMDIFFSIDEAREARATIO = 311, /* K_ANTENNACUMDIFFSIDEAREARATIO  */
  YYSYMBOL_K_ANTENNASIDEAREAFACTOR = 312,  /* K_ANTENNASIDEAREAFACTOR  */
  YYSYMBOL_K_DIFFUSEONLY = 313,            /* K_DIFFUSEONLY  */
  YYSYMBOL_K_MANUFACTURINGGRID = 314,      /* K_MANUFACTURINGGRID  */
  YYSYMBOL_K_FIXEDMASK = 315,              /* K_FIXEDMASK  */
  YYSYMBOL_K_ANTENNACELL = 316,            /* K_ANTENNACELL  */
  YYSYMBOL_K_CLEARANCEMEASURE = 317,       /* K_CLEARANCEMEASURE  */
  YYSYMBOL_K_EUCLIDEAN = 318,              /* K_EUCLIDEAN  */
  YYSYMBOL_K_MAXXY = 319,                  /* K_MAXXY  */
  YYSYMBOL_K_USEMINSPACING = 320,          /* K_USEMINSPACING  */
  YYSYMBOL_K_ROWMINSPACING = 321,          /* K_ROWMINSPACING  */
  YYSYMBOL_K_ROWABUTSPACING = 322,         /* K_ROWABUTSPACING  */
  YYSYMBOL_K_FLIP = 323,                   /* K_FLIP  */
  YYSYMBOL_K_NONE = 324,                   /* K_NONE  */
  YYSYMBOL_K_ANTENNAPARTIALMETALAREA = 325, /* K_ANTENNAPARTIALMETALAREA  */
  YYSYMBOL_K_ANTENNAPARTIALMETALSIDEAREA = 326, /* K_ANTENNAPARTIALMETALSIDEAREA  */
  YYSYMBOL_K_ANTENNAGATEAREA = 327,        /* K_ANTENNAGATEAREA  */
  YYSYMBOL_K_ANTENNADIFFAREA = 328,        /* K_ANTENNADIFFAREA  */
  YYSYMBOL_K_ANTENNAMAXAREACAR = 329,      /* K_ANTENNAMAXAREACAR  */
  YYSYMBOL_K_ANTENNAMAXSIDEAREACAR = 330,  /* K_ANTENNAMAXSIDEAREACAR  */
  YYSYMBOL_K_ANTENNAPARTIALCUTAREA = 331,  /* K_ANTENNAPARTIALCUTAREA  */
  YYSYMBOL_K_ANTENNAMAXCUTCAR = 332,       /* K_ANTENNAMAXCUTCAR  */
  YYSYMBOL_K_SLOTWIREWIDTH = 333,          /* K_SLOTWIREWIDTH  */
  YYSYMBOL_K_SLOTWIRELENGTH = 334,         /* K_SLOTWIRELENGTH  */
  YYSYMBOL_K_SLOTWIDTH = 335,              /* K_SLOTWIDTH  */
  YYSYMBOL_K_SLOTLENGTH = 336,             /* K_SLOTLENGTH  */
  YYSYMBOL_K_MAXADJACENTSLOTSPACING = 337, /* K_MAXADJACENTSLOTSPACING  */
  YYSYMBOL_K_MAXCOAXIALSLOTSPACING = 338,  /* K_MAXCOAXIALSLOTSPACING  */
  YYSYMBOL_K_MAXEDGESLOTSPACING = 339,     /* K_MAXEDGESLOTSPACING  */
  YYSYMBOL_K_SPLITWIREWIDTH = 340,         /* K_SPLITWIREWIDTH  */
  YYSYMBOL_K_MINIMUMDENSITY = 341,         /* K_MINIMUMDENSITY  */
  YYSYMBOL_K_MAXIMUMDENSITY = 342,         /* K_MAXIMUMDENSITY  */
  YYSYMBOL_K_DENSITYCHECKWINDOW = 343,     /* K_DENSITYCHECKWINDOW  */
  YYSYMBOL_K_DENSITYCHECKSTEP = 344,       /* K_DENSITYCHECKSTEP  */
  YYSYMBOL_K_FILLACTIVESPACING = 345,      /* K_FILLACTIVESPACING  */
  YYSYMBOL_K_MINIMUMCUT = 346,             /* K_MINIMUMCUT  */
  YYSYMBOL_K_ADJACENTCUTS = 347,           /* K_ADJACENTCUTS  */
  YYSYMBOL_K_ANTENNAMODEL = 348,           /* K_ANTENNAMODEL  */
  YYSYMBOL_K_BUMP = 349,                   /* K_BUMP  */
  YYSYMBOL_K_ENCLOSURE = 350,              /* K_ENCLOSURE  */
  YYSYMBOL_K_FROMABOVE = 351,              /* K_FROMABOVE  */
  YYSYMBOL_K_FROMBELOW = 352,              /* K_FROMBELOW  */
  YYSYMBOL_K_IMPLANT = 353,                /* K_IMPLANT  */
  YYSYMBOL_K_LENGTH = 354,                 /* K_LENGTH  */
  YYSYMBOL_K_MAXVIASTACK = 355,            /* K_MAXVIASTACK  */
  YYSYMBOL_K_AREAIO = 356,                 /* K_AREAIO  */
  YYSYMBOL_K_BLACKBOX = 357,               /* K_BLACKBOX  */
  YYSYMBOL_K_MAXWIDTH = 358,               /* K_MAXWIDTH  */
  YYSYMBOL_K_MINENCLOSEDAREA = 359,        /* K_MINENCLOSEDAREA  */
  YYSYMBOL_K_MINSTEP = 360,                /* K_MINSTEP  */
  YYSYMBOL_K_ORIENT = 361,                 /* K_ORIENT  */
  YYSYMBOL_K_OXIDE1 = 362,                 /* K_OXIDE1  */
  YYSYMBOL_K_OXIDE2 = 363,                 /* K_OXIDE2  */
  YYSYMBOL_K_OXIDE3 = 364,                 /* K_OXIDE3  */
  YYSYMBOL_K_OXIDE4 = 365,                 /* K_OXIDE4  */
  YYSYMBOL_K_OXIDE5 = 366,                 /* K_OXIDE5  */
  YYSYMBOL_K_OXIDE6 = 367,                 /* K_OXIDE6  */
  YYSYMBOL_K_OXIDE7 = 368,                 /* K_OXIDE7  */
  YYSYMBOL_K_OXIDE8 = 369,                 /* K_OXIDE8  */
  YYSYMBOL_K_OXIDE9 = 370,                 /* K_OXIDE9  */
  YYSYMBOL_K_OXIDE10 = 371,                /* K_OXIDE10  */
  YYSYMBOL_K_OXIDE11 = 372,                /* K_OXIDE11  */
  YYSYMBOL_K_OXIDE12 = 373,                /* K_OXIDE12  */
  YYSYMBOL_K_OXIDE13 = 374,                /* K_OXIDE13  */
  YYSYMBOL_K_OXIDE14 = 375,                /* K_OXIDE14  */
  YYSYMBOL_K_OXIDE15 = 376,                /* K_OXIDE15  */
  YYSYMBOL_K_OXIDE16 = 377,                /* K_OXIDE16  */
  YYSYMBOL_K_OXIDE17 = 378,                /* K_OXIDE17  */
  YYSYMBOL_K_OXIDE18 = 379,                /* K_OXIDE18  */
  YYSYMBOL_K_OXIDE19 = 380,                /* K_OXIDE19  */
  YYSYMBOL_K_OXIDE20 = 381,                /* K_OXIDE20  */
  YYSYMBOL_K_OXIDE21 = 382,                /* K_OXIDE21  */
  YYSYMBOL_K_OXIDE22 = 383,                /* K_OXIDE22  */
  YYSYMBOL_K_OXIDE23 = 384,                /* K_OXIDE23  */
  YYSYMBOL_K_OXIDE24 = 385,                /* K_OXIDE24  */
  YYSYMBOL_K_OXIDE25 = 386,                /* K_OXIDE25  */
  YYSYMBOL_K_OXIDE26 = 387,                /* K_OXIDE26  */
  YYSYMBOL_K_OXIDE27 = 388,                /* K_OXIDE27  */
  YYSYMBOL_K_OXIDE28 = 389,                /* K_OXIDE28  */
  YYSYMBOL_K_OXIDE29 = 390,                /* K_OXIDE29  */
  YYSYMBOL_K_OXIDE30 = 391,                /* K_OXIDE30  */
  YYSYMBOL_K_OXIDE31 = 392,                /* K_OXIDE31  */
  YYSYMBOL_K_OXIDE32 = 393,                /* K_OXIDE32  */
  YYSYMBOL_K_PARALLELRUNLENGTH = 394,      /* K_PARALLELRUNLENGTH  */
  YYSYMBOL_K_MINWIDTH = 395,               /* K_MINWIDTH  */
  YYSYMBOL_K_PROTRUSIONWIDTH = 396,        /* K_PROTRUSIONWIDTH  */
  YYSYMBOL_K_SPACINGTABLE = 397,           /* K_SPACINGTABLE  */
  YYSYMBOL_K_WITHIN = 398,                 /* K_WITHIN  */
  YYSYMBOL_K_ABOVE = 399,                  /* K_ABOVE  */
  YYSYMBOL_K_BELOW = 400,                  /* K_BELOW  */
  YYSYMBOL_K_CENTERTOCENTER = 401,         /* K_CENTERTOCENTER  */
  YYSYMBOL_K_CUTSIZE = 402,                /* K_CUTSIZE  */
  YYSYMBOL_K_CUTSPACING = 403,             /* K_CUTSPACING  */
  YYSYMBOL_K_DENSITY = 404,                /* K_DENSITY  */
  YYSYMBOL_K_DIAG45 = 405,                 /* K_DIAG45  */
  YYSYMBOL_K_DIAG135 = 406,                /* K_DIAG135  */
  YYSYMBOL_K_MASK = 407,                   /* K_MASK  */
  YYSYMBOL_K_DIAGMINEDGELENGTH = 408,      /* K_DIAGMINEDGELENGTH  */
  YYSYMBOL_K_DIAGSPACING = 409,            /* K_DIAGSPACING  */
  YYSYMBOL_K_DIAGPITCH = 410,              /* K_DIAGPITCH  */
  YYSYMBOL_K_DIAGWIDTH = 411,              /* K_DIAGWIDTH  */
  YYSYMBOL_K_GENERATED = 412,              /* K_GENERATED  */
  YYSYMBOL_K_GROUNDSENSITIVITY = 413,      /* K_GROUNDSENSITIVITY  */
  YYSYMBOL_K_HARDSPACING = 414,            /* K_HARDSPACING  */
  YYSYMBOL_K_INSIDECORNER = 415,           /* K_INSIDECORNER  */
  YYSYMBOL_K_LAYERS = 416,                 /* K_LAYERS  */
  YYSYMBOL_K_LENGTHSUM = 417,              /* K_LENGTHSUM  */
  YYSYMBOL_K_MICRONS = 418,                /* K_MICRONS  */
  YYSYMBOL_K_MINCUTS = 419,                /* K_MINCUTS  */
  YYSYMBOL_K_MINSIZE = 420,                /* K_MINSIZE  */
  YYSYMBOL_K_NETEXPR = 421,                /* K_NETEXPR  */
  YYSYMBOL_K_OUTSIDECORNER = 422,          /* K_OUTSIDECORNER  */
  YYSYMBOL_K_PREFERENCLOSURE = 423,        /* K_PREFERENCLOSURE  */
  YYSYMBOL_K_ROWCOL = 424,                 /* K_ROWCOL  */
  YYSYMBOL_K_ROWPATTERN = 425,             /* K_ROWPATTERN  */
  YYSYMBOL_K_SOFT = 426,                   /* K_SOFT  */
  YYSYMBOL_K_SUPPLYSENSITIVITY = 427,      /* K_SUPPLYSENSITIVITY  */
  YYSYMBOL_K_USEVIA = 428,                 /* K_USEVIA  */
  YYSYMBOL_K_USEVIARULE = 429,             /* K_USEVIARULE  */
  YYSYMBOL_K_WELLTAP = 430,                /* K_WELLTAP  */
  YYSYMBOL_K_ARRAYCUTS = 431,              /* K_ARRAYCUTS  */
  YYSYMBOL_K_ARRAYSPACING = 432,           /* K_ARRAYSPACING  */
  YYSYMBOL_K_ANTENNAAREADIFFREDUCEPWL = 433, /* K_ANTENNAAREADIFFREDUCEPWL  */
  YYSYMBOL_K_ANTENNAAREAMINUSDIFF = 434,   /* K_ANTENNAAREAMINUSDIFF  */
  YYSYMBOL_K_ANTENNACUMROUTINGPLUSCUT = 435, /* K_ANTENNACUMROUTINGPLUSCUT  */
  YYSYMBOL_K_ANTENNAGATEPLUSDIFF = 436,    /* K_ANTENNAGATEPLUSDIFF  */
  YYSYMBOL_K_ENDOFLINE = 437,              /* K_ENDOFLINE  */
  YYSYMBOL_K_ENDOFNOTCHWIDTH = 438,        /* K_ENDOFNOTCHWIDTH  */
  YYSYMBOL_K_EXCEPTEXTRACUT = 439,         /* K_EXCEPTEXTRACUT  */
  YYSYMBOL_K_EXCEPTSAMEPGNET = 440,        /* K_EXCEPTSAMEPGNET  */
  YYSYMBOL_K_EXCEPTPGNET = 441,            /* K_EXCEPTPGNET  */
  YYSYMBOL_K_LONGARRAY = 442,              /* K_LONGARRAY  */
  YYSYMBOL_K_MAXEDGES = 443,               /* K_MAXEDGES  */
  YYSYMBOL_K_NOTCHLENGTH = 444,            /* K_NOTCHLENGTH  */
  YYSYMBOL_K_NOTCHSPACING = 445,           /* K_NOTCHSPACING  */
  YYSYMBOL_K_ORTHOGONAL = 446,             /* K_ORTHOGONAL  */
  YYSYMBOL_K_PARALLELEDGE = 447,           /* K_PARALLELEDGE  */
  YYSYMBOL_K_PARALLELOVERLAP = 448,        /* K_PARALLELOVERLAP  */
  YYSYMBOL_K_PGONLY = 449,                 /* K_PGONLY  */
  YYSYMBOL_K_PRL = 450,                    /* K_PRL  */
  YYSYMBOL_K_TWOEDGES = 451,               /* K_TWOEDGES  */
  YYSYMBOL_K_TWOWIDTHS = 452,              /* K_TWOWIDTHS  */
  YYSYMBOL_IF = 453,                       /* IF  */
  YYSYMBOL_LNOT = 454,                     /* LNOT  */
  YYSYMBOL_455_ = 455,                     /* '-'  */
  YYSYMBOL_456_ = 456,                     /* '+'  */
  YYSYMBOL_457_ = 457,                     /* '*'  */
  YYSYMBOL_458_ = 458,                     /* '/'  */
  YYSYMBOL_UMINUS = 459,                   /* UMINUS  */
  YYSYMBOL_460_ = 460,                     /* ';'  */
  YYSYMBOL_461_ = 461,                     /* '('  */
  YYSYMBOL_462_ = 462,                     /* ')'  */
  YYSYMBOL_463_ = 463,                     /* '='  */
  YYSYMBOL_464_n_ = 464,                   /* '\n'  */
  YYSYMBOL_465_ = 465,                     /* '<'  */
  YYSYMBOL_466_ = 466,                     /* '>'  */
  YYSYMBOL_YYACCEPT = 467,                 /* $accept  */
  YYSYMBOL_lef_file = 468,                 /* lef_file  */
  YYSYMBOL_version = 469,                  /* version  */
  YYSYMBOL_470_1 = 470,                    /* $@1  */
  YYSYMBOL_int_number = 471,               /* int_number  */
  YYSYMBOL_dividerchar = 472,              /* dividerchar  */
  YYSYMBOL_busbitchars = 473,              /* busbitchars  */
  YYSYMBOL_rules = 474,                    /* rules  */
  YYSYMBOL_end_library = 475,              /* end_library  */
  YYSYMBOL_rule = 476,                     /* rule  */
  YYSYMBOL_case_sensitivity = 477,         /* case_sensitivity  */
  YYSYMBOL_wireextension = 478,            /* wireextension  */
  YYSYMBOL_fixedmask = 479,                /* fixedmask  */
  YYSYMBOL_manufacturing = 480,            /* manufacturing  */
  YYSYMBOL_useminspacing = 481,            /* useminspacing  */
  YYSYMBOL_clearancemeasure = 482,         /* clearancemeasure  */
  YYSYMBOL_clearance_type = 483,           /* clearance_type  */
  YYSYMBOL_spacing_type = 484,             /* spacing_type  */
  YYSYMBOL_spacing_value = 485,            /* spacing_value  */
  YYSYMBOL_units_section = 486,            /* units_section  */
  YYSYMBOL_start_units = 487,              /* start_units  */
  YYSYMBOL_units_rules = 488,              /* units_rules  */
  YYSYMBOL_units_rule = 489,               /* units_rule  */
  YYSYMBOL_layer_rule = 490,               /* layer_rule  */
  YYSYMBOL_start_layer = 491,              /* start_layer  */
  YYSYMBOL_492_2 = 492,                    /* $@2  */
  YYSYMBOL_end_layer = 493,                /* end_layer  */
  YYSYMBOL_494_3 = 494,                    /* $@3  */
  YYSYMBOL_layer_options = 495,            /* layer_options  */
  YYSYMBOL_layer_option = 496,             /* layer_option  */
  YYSYMBOL_497_4 = 497,                    /* $@4  */
  YYSYMBOL_498_5 = 498,                    /* $@5  */
  YYSYMBOL_499_6 = 499,                    /* $@6  */
  YYSYMBOL_500_7 = 500,                    /* $@7  */
  YYSYMBOL_501_8 = 501,                    /* $@8  */
  YYSYMBOL_502_9 = 502,                    /* $@9  */
  YYSYMBOL_503_10 = 503,                   /* $@10  */
  YYSYMBOL_504_11 = 504,                   /* $@11  */
  YYSYMBOL_505_12 = 505,                   /* $@12  */
  YYSYMBOL_506_13 = 506,                   /* $@13  */
  YYSYMBOL_507_14 = 507,                   /* $@14  */
  YYSYMBOL_508_15 = 508,                   /* $@15  */
  YYSYMBOL_509_16 = 509,                   /* $@16  */
  YYSYMBOL_510_17 = 510,                   /* $@17  */
  YYSYMBOL_511_18 = 511,                   /* $@18  */
  YYSYMBOL_512_19 = 512,                   /* $@19  */
  YYSYMBOL_513_20 = 513,                   /* $@20  */
  YYSYMBOL_514_21 = 514,                   /* $@21  */
  YYSYMBOL_515_22 = 515,                   /* $@22  */
  YYSYMBOL_516_23 = 516,                   /* $@23  */
  YYSYMBOL_517_24 = 517,                   /* $@24  */
  YYSYMBOL_518_25 = 518,                   /* $@25  */
  YYSYMBOL_519_26 = 519,                   /* $@26  */
  YYSYMBOL_520_27 = 520,                   /* $@27  */
  YYSYMBOL_521_28 = 521,                   /* $@28  */
  YYSYMBOL_522_29 = 522,                   /* $@29  */
  YYSYMBOL_layer_arraySpacing_long = 523,  /* layer_arraySpacing_long  */
  YYSYMBOL_layer_arraySpacing_width = 524, /* layer_arraySpacing_width  */
  YYSYMBOL_layer_arraySpacing_arraycuts = 525, /* layer_arraySpacing_arraycuts  */
  YYSYMBOL_layer_arraySpacing_arraycut = 526, /* layer_arraySpacing_arraycut  */
  YYSYMBOL_sp_options = 527,               /* sp_options  */
  YYSYMBOL_528_30 = 528,                   /* $@30  */
  YYSYMBOL_529_31 = 529,                   /* $@31  */
  YYSYMBOL_530_32 = 530,                   /* $@32  */
  YYSYMBOL_531_33 = 531,                   /* $@33  */
  YYSYMBOL_532_34 = 532,                   /* $@34  */
  YYSYMBOL_533_35 = 533,                   /* $@35  */
  YYSYMBOL_534_36 = 534,                   /* $@36  */
  YYSYMBOL_layer_spacingtable_opts = 535,  /* layer_spacingtable_opts  */
  YYSYMBOL_layer_spacingtable_opt = 536,   /* layer_spacingtable_opt  */
  YYSYMBOL_layer_enclosure_type_opt = 537, /* layer_enclosure_type_opt  */
  YYSYMBOL_layer_enclosure_width_opt = 538, /* layer_enclosure_width_opt  */
  YYSYMBOL_539_37 = 539,                   /* $@37  */
  YYSYMBOL_layer_enclosure_width_except_opt = 540, /* layer_enclosure_width_except_opt  */
  YYSYMBOL_layer_preferenclosure_width_opt = 541, /* layer_preferenclosure_width_opt  */
  YYSYMBOL_layer_minimumcut_within = 542,  /* layer_minimumcut_within  */
  YYSYMBOL_layer_minimumcut_from = 543,    /* layer_minimumcut_from  */
  YYSYMBOL_layer_minimumcut_length = 544,  /* layer_minimumcut_length  */
  YYSYMBOL_layer_minstep_options = 545,    /* layer_minstep_options  */
  YYSYMBOL_layer_minstep_option = 546,     /* layer_minstep_option  */
  YYSYMBOL_layer_minstep_type = 547,       /* layer_minstep_type  */
  YYSYMBOL_layer_antenna_pwl = 548,        /* layer_antenna_pwl  */
  YYSYMBOL_549_38 = 549,                   /* $@38  */
  YYSYMBOL_layer_diffusion_ratios = 550,   /* layer_diffusion_ratios  */
  YYSYMBOL_layer_diffusion_ratio = 551,    /* layer_diffusion_ratio  */
  YYSYMBOL_layer_antenna_duo = 552,        /* layer_antenna_duo  */
  YYSYMBOL_layer_table_type = 553,         /* layer_table_type  */
  YYSYMBOL_layer_frequency = 554,          /* layer_frequency  */
  YYSYMBOL_555_39 = 555,                   /* $@39  */
  YYSYMBOL_556_40 = 556,                   /* $@40  */
  YYSYMBOL_557_41 = 557,                   /* $@41  */
  YYSYMBOL_ac_layer_table_opt = 558,       /* ac_layer_table_opt  */
  YYSYMBOL_559_42 = 559,                   /* $@42  */
  YYSYMBOL_560_43 = 560,                   /* $@43  */
  YYSYMBOL_dc_layer_table = 561,           /* dc_layer_table  */
  YYSYMBOL_562_44 = 562,                   /* $@44  */
  YYSYMBOL_int_number_list = 563,          /* int_number_list  */
  YYSYMBOL_number_list = 564,              /* number_list  */
  YYSYMBOL_layer_prop_list = 565,          /* layer_prop_list  */
  YYSYMBOL_layer_prop = 566,               /* layer_prop  */
  YYSYMBOL_current_density_pwl_list = 567, /* current_density_pwl_list  */
  YYSYMBOL_current_density_pwl = 568,      /* current_density_pwl  */
  YYSYMBOL_cap_points = 569,               /* cap_points  */
  YYSYMBOL_cap_point = 570,                /* cap_point  */
  YYSYMBOL_res_points = 571,               /* res_points  */
  YYSYMBOL_res_point = 572,                /* res_point  */
  YYSYMBOL_layer_type = 573,               /* layer_type  */
  YYSYMBOL_layer_direction = 574,          /* layer_direction  */
  YYSYMBOL_layer_minen_width = 575,        /* layer_minen_width  */
  YYSYMBOL_layer_oxide = 576,              /* layer_oxide  */
  YYSYMBOL_layer_sp_parallel_widths = 577, /* layer_sp_parallel_widths  */
  YYSYMBOL_layer_sp_parallel_width = 578,  /* layer_sp_parallel_width  */
  YYSYMBOL_579_45 = 579,                   /* $@45  */
  YYSYMBOL_layer_sp_TwoWidths = 580,       /* layer_sp_TwoWidths  */
  YYSYMBOL_layer_sp_TwoWidth = 581,        /* layer_sp_TwoWidth  */
  YYSYMBOL_582_46 = 582,                   /* $@46  */
  YYSYMBOL_layer_sp_TwoWidthsPRL = 583,    /* layer_sp_TwoWidthsPRL  */
  YYSYMBOL_layer_sp_influence_widths = 584, /* layer_sp_influence_widths  */
  YYSYMBOL_layer_sp_influence_width = 585, /* layer_sp_influence_width  */
  YYSYMBOL_maxstack_via = 586,             /* maxstack_via  */
  YYSYMBOL_587_47 = 587,                   /* $@47  */
  YYSYMBOL_via = 588,                      /* via  */
  YYSYMBOL_589_48 = 589,                   /* $@48  */
  YYSYMBOL_via_keyword = 590,              /* via_keyword  */
  YYSYMBOL_start_via = 591,                /* start_via  */
  YYSYMBOL_via_viarule = 592,              /* via_viarule  */
  YYSYMBOL_593_49 = 593,                   /* $@49  */
  YYSYMBOL_594_50 = 594,                   /* $@50  */
  YYSYMBOL_595_51 = 595,                   /* $@51  */
  YYSYMBOL_via_viarule_options = 596,      /* via_viarule_options  */
  YYSYMBOL_via_viarule_option = 597,       /* via_viarule_option  */
  YYSYMBOL_598_52 = 598,                   /* $@52  */
  YYSYMBOL_via_option = 599,               /* via_option  */
  YYSYMBOL_via_other_options = 600,        /* via_other_options  */
  YYSYMBOL_via_more_options = 601,         /* via_more_options  */
  YYSYMBOL_via_other_option = 602,         /* via_other_option  */
  YYSYMBOL_603_53 = 603,                   /* $@53  */
  YYSYMBOL_via_prop_list = 604,            /* via_prop_list  */
  YYSYMBOL_via_name_value_pair = 605,      /* via_name_value_pair  */
  YYSYMBOL_via_foreign = 606,              /* via_foreign  */
  YYSYMBOL_start_foreign = 607,            /* start_foreign  */
  YYSYMBOL_608_54 = 608,                   /* $@54  */
  YYSYMBOL_orientation = 609,              /* orientation  */
  YYSYMBOL_via_layer_rule = 610,           /* via_layer_rule  */
  YYSYMBOL_via_layer = 611,                /* via_layer  */
  YYSYMBOL_612_55 = 612,                   /* $@55  */
  YYSYMBOL_via_geometries = 613,           /* via_geometries  */
  YYSYMBOL_via_geometry = 614,             /* via_geometry  */
  YYSYMBOL_615_56 = 615,                   /* $@56  */
  YYSYMBOL_end_via = 616,                  /* end_via  */
  YYSYMBOL_617_57 = 617,                   /* $@57  */
  YYSYMBOL_viarule_keyword = 618,          /* viarule_keyword  */
  YYSYMBOL_619_58 = 619,                   /* $@58  */
  YYSYMBOL_viarule = 620,                  /* viarule  */
  YYSYMBOL_viarule_generate = 621,         /* viarule_generate  */
  YYSYMBOL_622_59 = 622,                   /* $@59  */
  YYSYMBOL_viarule_generate_default = 623, /* viarule_generate_default  */
  YYSYMBOL_viarule_layer_list = 624,       /* viarule_layer_list  */
  YYSYMBOL_opt_viarule_props = 625,        /* opt_viarule_props  */
  YYSYMBOL_viarule_props = 626,            /* viarule_props  */
  YYSYMBOL_viarule_prop = 627,             /* viarule_prop  */
  YYSYMBOL_628_60 = 628,                   /* $@60  */
  YYSYMBOL_viarule_prop_list = 629,        /* viarule_prop_list  */
  YYSYMBOL_viarule_layer = 630,            /* viarule_layer  */
  YYSYMBOL_via_names = 631,                /* via_names  */
  YYSYMBOL_via_name = 632,                 /* via_name  */
  YYSYMBOL_viarule_layer_name = 633,       /* viarule_layer_name  */
  YYSYMBOL_634_61 = 634,                   /* $@61  */
  YYSYMBOL_viarule_layer_options = 635,    /* viarule_layer_options  */
  YYSYMBOL_viarule_layer_option = 636,     /* viarule_layer_option  */
  YYSYMBOL_end_viarule = 637,              /* end_viarule  */
  YYSYMBOL_638_62 = 638,                   /* $@62  */
  YYSYMBOL_spacing_rule = 639,             /* spacing_rule  */
  YYSYMBOL_start_spacing = 640,            /* start_spacing  */
  YYSYMBOL_end_spacing = 641,              /* end_spacing  */
  YYSYMBOL_spacings = 642,                 /* spacings  */
  YYSYMBOL_spacing = 643,                  /* spacing  */
  YYSYMBOL_samenet_keyword = 644,          /* samenet_keyword  */
  YYSYMBOL_maskColor = 645,                /* maskColor  */
  YYSYMBOL_irdrop = 646,                   /* irdrop  */
  YYSYMBOL_start_irdrop = 647,             /* start_irdrop  */
  YYSYMBOL_end_irdrop = 648,               /* end_irdrop  */
  YYSYMBOL_ir_tables = 649,                /* ir_tables  */
  YYSYMBOL_ir_table = 650,                 /* ir_table  */
  YYSYMBOL_ir_table_values = 651,          /* ir_table_values  */
  YYSYMBOL_ir_table_value = 652,           /* ir_table_value  */
  YYSYMBOL_ir_tablename = 653,             /* ir_tablename  */
  YYSYMBOL_minfeature = 654,               /* minfeature  */
  YYSYMBOL_dielectric = 655,               /* dielectric  */
  YYSYMBOL_nondefault_rule = 656,          /* nondefault_rule  */
  YYSYMBOL_657_63 = 657,                   /* $@63  */
  YYSYMBOL_658_64 = 658,                   /* $@64  */
  YYSYMBOL_659_65 = 659,                   /* $@65  */
  YYSYMBOL_end_nd_rule = 660,              /* end_nd_rule  */
  YYSYMBOL_nd_hardspacing = 661,           /* nd_hardspacing  */
  YYSYMBOL_nd_rules = 662,                 /* nd_rules  */
  YYSYMBOL_nd_rule = 663,                  /* nd_rule  */
  YYSYMBOL_usevia = 664,                   /* usevia  */
  YYSYMBOL_useviarule = 665,               /* useviarule  */
  YYSYMBOL_mincuts = 666,                  /* mincuts  */
  YYSYMBOL_nd_prop = 667,                  /* nd_prop  */
  YYSYMBOL_668_66 = 668,                   /* $@66  */
  YYSYMBOL_nd_prop_list = 669,             /* nd_prop_list  */
  YYSYMBOL_nd_layer = 670,                 /* nd_layer  */
  YYSYMBOL_671_67 = 671,                   /* $@67  */
  YYSYMBOL_672_68 = 672,                   /* $@68  */
  YYSYMBOL_673_69 = 673,                   /* $@69  */
  YYSYMBOL_674_70 = 674,                   /* $@70  */
  YYSYMBOL_nd_layer_stmts = 675,           /* nd_layer_stmts  */
  YYSYMBOL_nd_layer_stmt = 676,            /* nd_layer_stmt  */
  YYSYMBOL_site = 677,                     /* site  */
  YYSYMBOL_start_site = 678,               /* start_site  */
  YYSYMBOL_679_71 = 679,                   /* $@71  */
  YYSYMBOL_end_site = 680,                 /* end_site  */
  YYSYMBOL_681_72 = 681,                   /* $@72  */
  YYSYMBOL_site_options = 682,             /* site_options  */
  YYSYMBOL_site_option = 683,              /* site_option  */
  YYSYMBOL_site_class = 684,               /* site_class  */
  YYSYMBOL_site_symmetry_statement = 685,  /* site_symmetry_statement  */
  YYSYMBOL_site_symmetries = 686,          /* site_symmetries  */
  YYSYMBOL_site_symmetry = 687,            /* site_symmetry  */
  YYSYMBOL_site_rowpattern_statement = 688, /* site_rowpattern_statement  */
  YYSYMBOL_689_73 = 689,                   /* $@73  */
  YYSYMBOL_site_rowpatterns = 690,         /* site_rowpatterns  */
  YYSYMBOL_site_rowpattern = 691,          /* site_rowpattern  */
  YYSYMBOL_692_74 = 692,                   /* $@74  */
  YYSYMBOL_pt = 693,                       /* pt  */
  YYSYMBOL_macro = 694,                    /* macro  */
  YYSYMBOL_695_75 = 695,                   /* $@75  */
  YYSYMBOL_start_macro = 696,              /* start_macro  */
  YYSYMBOL_697_76 = 697,                   /* $@76  */
  YYSYMBOL_end_macro = 698,                /* end_macro  */
  YYSYMBOL_699_77 = 699,                   /* $@77  */
  YYSYMBOL_macro_options = 700,            /* macro_options  */
  YYSYMBOL_macro_option = 701,             /* macro_option  */
  YYSYMBOL_702_78 = 702,                   /* $@78  */
  YYSYMBOL_macro_prop_list = 703,          /* macro_prop_list  */
  YYSYMBOL_macro_symmetry_statement = 704, /* macro_symmetry_statement  */
  YYSYMBOL_macro_symmetries = 705,         /* macro_symmetries  */
  YYSYMBOL_macro_symmetry = 706,           /* macro_symmetry  */
  YYSYMBOL_macro_name_value_pair = 707,    /* macro_name_value_pair  */
  YYSYMBOL_macro_class = 708,              /* macro_class  */
  YYSYMBOL_class_type = 709,               /* class_type  */
  YYSYMBOL_pad_type = 710,                 /* pad_type  */
  YYSYMBOL_core_type = 711,                /* core_type  */
  YYSYMBOL_endcap_type = 712,              /* endcap_type  */
  YYSYMBOL_macro_generator = 713,          /* macro_generator  */
  YYSYMBOL_macro_generate = 714,           /* macro_generate  */
  YYSYMBOL_macro_source = 715,             /* macro_source  */
  YYSYMBOL_macro_power = 716,              /* macro_power  */
  YYSYMBOL_macro_origin = 717,             /* macro_origin  */
  YYSYMBOL_macro_foreign = 718,            /* macro_foreign  */
  YYSYMBOL_macro_fixedMask = 719,          /* macro_fixedMask  */
  YYSYMBOL_macro_eeq = 720,                /* macro_eeq  */
  YYSYMBOL_721_79 = 721,                   /* $@79  */
  YYSYMBOL_macro_leq = 722,                /* macro_leq  */
  YYSYMBOL_723_80 = 723,                   /* $@80  */
  YYSYMBOL_macro_site = 724,               /* macro_site  */
  YYSYMBOL_macro_site_word = 725,          /* macro_site_word  */
  YYSYMBOL_site_word = 726,                /* site_word  */
  YYSYMBOL_macro_size = 727,               /* macro_size  */
  YYSYMBOL_macro_pin = 728,                /* macro_pin  */
  YYSYMBOL_start_macro_pin = 729,          /* start_macro_pin  */
  YYSYMBOL_730_81 = 730,                   /* $@81  */
  YYSYMBOL_end_macro_pin = 731,            /* end_macro_pin  */
  YYSYMBOL_732_82 = 732,                   /* $@82  */
  YYSYMBOL_macro_pin_options = 733,        /* macro_pin_options  */
  YYSYMBOL_macro_pin_option = 734,         /* macro_pin_option  */
  YYSYMBOL_735_83 = 735,                   /* $@83  */
  YYSYMBOL_736_84 = 736,                   /* $@84  */
  YYSYMBOL_737_85 = 737,                   /* $@85  */
  YYSYMBOL_738_86 = 738,                   /* $@86  */
  YYSYMBOL_739_87 = 739,                   /* $@87  */
  YYSYMBOL_740_88 = 740,                   /* $@88  */
  YYSYMBOL_741_89 = 741,                   /* $@89  */
  YYSYMBOL_742_90 = 742,                   /* $@90  */
  YYSYMBOL_743_91 = 743,                   /* $@91  */
  YYSYMBOL_744_92 = 744,                   /* $@92  */
  YYSYMBOL_pin_layer_oxide = 745,          /* pin_layer_oxide  */
  YYSYMBOL_pin_prop_list = 746,            /* pin_prop_list  */
  YYSYMBOL_pin_name_value_pair = 747,      /* pin_name_value_pair  */
  YYSYMBOL_electrical_direction = 748,     /* electrical_direction  */
  YYSYMBOL_start_macro_port = 749,         /* start_macro_port  */
  YYSYMBOL_macro_port_class_option = 750,  /* macro_port_class_option  */
  YYSYMBOL_macro_pin_use = 751,            /* macro_pin_use  */
  YYSYMBOL_macro_scan_use = 752,           /* macro_scan_use  */
  YYSYMBOL_pin_shape = 753,                /* pin_shape  */
  YYSYMBOL_geometries = 754,               /* geometries  */
  YYSYMBOL_geometry = 755,                 /* geometry  */
  YYSYMBOL_756_93 = 756,                   /* $@93  */
  YYSYMBOL_757_94 = 757,                   /* $@94  */
  YYSYMBOL_geometry_options = 758,         /* geometry_options  */
  YYSYMBOL_layer_exceptpgnet = 759,        /* layer_exceptpgnet  */
  YYSYMBOL_layer_spacing = 760,            /* layer_spacing  */
  YYSYMBOL_firstPt = 761,                  /* firstPt  */
  YYSYMBOL_nextPt = 762,                   /* nextPt  */
  YYSYMBOL_otherPts = 763,                 /* otherPts  */
  YYSYMBOL_via_placement = 764,            /* via_placement  */
  YYSYMBOL_765_95 = 765,                   /* $@95  */
  YYSYMBOL_766_96 = 766,                   /* $@96  */
  YYSYMBOL_stepPattern = 767,              /* stepPattern  */
  YYSYMBOL_sitePattern = 768,              /* sitePattern  */
  YYSYMBOL_trackPattern = 769,             /* trackPattern  */
  YYSYMBOL_770_97 = 770,                   /* $@97  */
  YYSYMBOL_771_98 = 771,                   /* $@98  */
  YYSYMBOL_772_99 = 772,                   /* $@99  */
  YYSYMBOL_773_100 = 773,                  /* $@100  */
  YYSYMBOL_trackLayers = 774,              /* trackLayers  */
  YYSYMBOL_layer_name = 775,               /* layer_name  */
  YYSYMBOL_gcellPattern = 776,             /* gcellPattern  */
  YYSYMBOL_macro_obs = 777,                /* macro_obs  */
  YYSYMBOL_start_macro_obs = 778,          /* start_macro_obs  */
  YYSYMBOL_macro_density = 779,            /* macro_density  */
  YYSYMBOL_density_layers = 780,           /* density_layers  */
  YYSYMBOL_density_layer = 781,            /* density_layer  */
  YYSYMBOL_782_101 = 782,                  /* $@101  */
  YYSYMBOL_783_102 = 783,                  /* $@102  */
  YYSYMBOL_density_layer_rects = 784,      /* density_layer_rects  */
  YYSYMBOL_density_layer_rect = 785,       /* density_layer_rect  */
  YYSYMBOL_macro_clocktype = 786,          /* macro_clocktype  */
  YYSYMBOL_787_103 = 787,                  /* $@103  */
  YYSYMBOL_timing = 788,                   /* timing  */
  YYSYMBOL_start_timing = 789,             /* start_timing  */
  YYSYMBOL_end_timing = 790,               /* end_timing  */
  YYSYMBOL_timing_options = 791,           /* timing_options  */
  YYSYMBOL_timing_option = 792,            /* timing_option  */
  YYSYMBOL_793_104 = 793,                  /* $@104  */
  YYSYMBOL_794_105 = 794,                  /* $@105  */
  YYSYMBOL_795_106 = 795,                  /* $@106  */
  YYSYMBOL_one_pin_trigger = 796,          /* one_pin_trigger  */
  YYSYMBOL_two_pin_trigger = 797,          /* two_pin_trigger  */
  YYSYMBOL_from_pin_trigger = 798,         /* from_pin_trigger  */
  YYSYMBOL_to_pin_trigger = 799,           /* to_pin_trigger  */
  YYSYMBOL_delay_or_transition = 800,      /* delay_or_transition  */
  YYSYMBOL_list_of_table_entries = 801,    /* list_of_table_entries  */
  YYSYMBOL_table_entry = 802,              /* table_entry  */
  YYSYMBOL_list_of_table_axis_dnumbers = 803, /* list_of_table_axis_dnumbers  */
  YYSYMBOL_slew_spec = 804,                /* slew_spec  */
  YYSYMBOL_risefall = 805,                 /* risefall  */
  YYSYMBOL_unateness = 806,                /* unateness  */
  YYSYMBOL_list_of_from_strings = 807,     /* list_of_from_strings  */
  YYSYMBOL_list_of_to_strings = 808,       /* list_of_to_strings  */
  YYSYMBOL_array = 809,                    /* array  */
  YYSYMBOL_810_107 = 810,                  /* $@107  */
  YYSYMBOL_start_array = 811,              /* start_array  */
  YYSYMBOL_812_108 = 812,                  /* $@108  */
  YYSYMBOL_end_array = 813,                /* end_array  */
  YYSYMBOL_814_109 = 814,                  /* $@109  */
  YYSYMBOL_array_rules = 815,              /* array_rules  */
  YYSYMBOL_array_rule = 816,               /* array_rule  */
  YYSYMBOL_817_110 = 817,                  /* $@110  */
  YYSYMBOL_818_111 = 818,                  /* $@111  */
  YYSYMBOL_819_112 = 819,                  /* $@112  */
  YYSYMBOL_820_113 = 820,                  /* $@113  */
  YYSYMBOL_821_114 = 821,                  /* $@114  */
  YYSYMBOL_floorplan_start = 822,          /* floorplan_start  */
  YYSYMBOL_floorplan_list = 823,           /* floorplan_list  */
  YYSYMBOL_floorplan_element = 824,        /* floorplan_element  */
  YYSYMBOL_825_115 = 825,                  /* $@115  */
  YYSYMBOL_826_116 = 826,                  /* $@116  */
  YYSYMBOL_cap_list = 827,                 /* cap_list  */
  YYSYMBOL_one_cap = 828,                  /* one_cap  */
  YYSYMBOL_msg_statement = 829,            /* msg_statement  */
  YYSYMBOL_830_117 = 830,                  /* $@117  */
  YYSYMBOL_create_file_statement = 831,    /* create_file_statement  */
  YYSYMBOL_832_118 = 832,                  /* $@118  */
  YYSYMBOL_def_statement = 833,            /* def_statement  */
  YYSYMBOL_834_119 = 834,                  /* $@119  */
  YYSYMBOL_835_120 = 835,                  /* $@120  */
  YYSYMBOL_836_121 = 836,                  /* $@121  */
  YYSYMBOL_dtrm = 837,                     /* dtrm  */
  YYSYMBOL_then = 838,                     /* then  */
  YYSYMBOL_else = 839,                     /* else  */
  YYSYMBOL_expression = 840,               /* expression  */
  YYSYMBOL_b_expr = 841,                   /* b_expr  */
  YYSYMBOL_s_expr = 842,                   /* s_expr  */
  YYSYMBOL_relop = 843,                    /* relop  */
  YYSYMBOL_prop_def_section = 844,         /* prop_def_section  */
  YYSYMBOL_845_122 = 845,                  /* $@122  */
  YYSYMBOL_prop_stmts = 846,               /* prop_stmts  */
  YYSYMBOL_prop_stmt = 847,                /* prop_stmt  */
  YYSYMBOL_848_123 = 848,                  /* $@123  */
  YYSYMBOL_849_124 = 849,                  /* $@124  */
  YYSYMBOL_850_125 = 850,                  /* $@125  */
  YYSYMBOL_851_126 = 851,                  /* $@126  */
  YYSYMBOL_852_127 = 852,                  /* $@127  */
  YYSYMBOL_853_128 = 853,                  /* $@128  */
  YYSYMBOL_854_129 = 854,                  /* $@129  */
  YYSYMBOL_855_130 = 855,                  /* $@130  */
  YYSYMBOL_prop_define = 856,              /* prop_define  */
  YYSYMBOL_opt_range_second = 857,         /* opt_range_second  */
  YYSYMBOL_opt_endofline = 858,            /* opt_endofline  */
  YYSYMBOL_859_131 = 859,                  /* $@131  */
  YYSYMBOL_opt_endofline_twoedges = 860,   /* opt_endofline_twoedges  */
  YYSYMBOL_opt_samenetPGonly = 861,        /* opt_samenetPGonly  */
  YYSYMBOL_opt_def_range = 862,            /* opt_def_range  */
  YYSYMBOL_opt_def_value = 863,            /* opt_def_value  */
  YYSYMBOL_opt_def_dvalue = 864,           /* opt_def_dvalue  */
  YYSYMBOL_layer_spacing_opts = 865,       /* layer_spacing_opts  */
  YYSYMBOL_layer_spacing_opt = 866,        /* layer_spacing_opt  */
  YYSYMBOL_867_132 = 867,                  /* $@132  */
  YYSYMBOL_layer_spacing_cut_routing = 868, /* layer_spacing_cut_routing  */
  YYSYMBOL_869_133 = 869,                  /* $@133  */
  YYSYMBOL_870_134 = 870,                  /* $@134  */
  YYSYMBOL_871_135 = 871,                  /* $@135  */
  YYSYMBOL_872_136 = 872,                  /* $@136  */
  YYSYMBOL_873_137 = 873,                  /* $@137  */
  YYSYMBOL_spacing_cut_layer_opt = 874,    /* spacing_cut_layer_opt  */
  YYSYMBOL_opt_adjacentcuts_exceptsame = 875, /* opt_adjacentcuts_exceptsame  */
  YYSYMBOL_opt_layer_name = 876,           /* opt_layer_name  */
  YYSYMBOL_877_138 = 877,                  /* $@138  */
  YYSYMBOL_req_layer_name = 878,           /* req_layer_name  */
  YYSYMBOL_879_139 = 879,                  /* $@139  */
  YYSYMBOL_universalnoisemargin = 880,     /* universalnoisemargin  */
  YYSYMBOL_edgeratethreshold1 = 881,       /* edgeratethreshold1  */
  YYSYMBOL_edgeratethreshold2 = 882,       /* edgeratethreshold2  */
  YYSYMBOL_edgeratescalefactor = 883,      /* edgeratescalefactor  */
  YYSYMBOL_noisetable = 884,               /* noisetable  */
  YYSYMBOL_885_140 = 885,                  /* $@140  */
  YYSYMBOL_end_noisetable = 886,           /* end_noisetable  */
  YYSYMBOL_noise_table_list = 887,         /* noise_table_list  */
  YYSYMBOL_noise_table_entry = 888,        /* noise_table_entry  */
  YYSYMBOL_output_resistance_entry = 889,  /* output_resistance_entry  */
  YYSYMBOL_890_141 = 890,                  /* $@141  */
  YYSYMBOL_num_list = 891,                 /* num_list  */
  YYSYMBOL_victim_list = 892,              /* victim_list  */
  YYSYMBOL_victim = 893,                   /* victim  */
  YYSYMBOL_894_142 = 894,                  /* $@142  */
  YYSYMBOL_vnoiselist = 895,               /* vnoiselist  */
  YYSYMBOL_correctiontable = 896,          /* correctiontable  */
  YYSYMBOL_897_143 = 897,                  /* $@143  */
  YYSYMBOL_end_correctiontable = 898,      /* end_correctiontable  */
  YYSYMBOL_correction_table_list = 899,    /* correction_table_list  */
  YYSYMBOL_correction_table_item = 900,    /* correction_table_item  */
  YYSYMBOL_output_list = 901,              /* output_list  */
  YYSYMBOL_902_144 = 902,                  /* $@144  */
  YYSYMBOL_numo_list = 903,                /* numo_list  */
  YYSYMBOL_corr_victim_list = 904,         /* corr_victim_list  */
  YYSYMBOL_corr_victim = 905,              /* corr_victim  */
  YYSYMBOL_906_145 = 906,                  /* $@145  */
  YYSYMBOL_corr_list = 907,                /* corr_list  */
  YYSYMBOL_input_antenna = 908,            /* input_antenna  */
  YYSYMBOL_output_antenna = 909,           /* output_antenna  */
  YYSYMBOL_inout_antenna = 910,            /* inout_antenna  */
  YYSYMBOL_antenna_input = 911,            /* antenna_input  */
  YYSYMBOL_antenna_inout = 912,            /* antenna_inout  */
  YYSYMBOL_antenna_output = 913,           /* antenna_output  */
  YYSYMBOL_extension_opt = 914,            /* extension_opt  */
  YYSYMBOL_extension = 915                 /* extension  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if !defined yyoverflow

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* !defined yyoverflow */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  4
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2723

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  467
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  449
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1076
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  2100

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   710


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     464,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     461,   462,   457,   456,     2,   455,     2,   458,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   460,
     465,   463,   466,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166,   167,   168,   169,   170,   171,   172,   173,   174,
     175,   176,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   187,   188,   189,   190,   191,   192,   193,   194,
     195,   196,   197,   198,   199,   200,   201,   202,   203,   204,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     215,   216,   217,   218,   219,   220,   221,   222,   223,   224,
     225,   226,   227,   228,   229,   230,   231,   232,   233,   234,
     235,   236,   237,   238,   239,   240,   241,   242,   243,   244,
     245,   246,   247,   248,   249,   250,   251,   252,   253,   254,
     255,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,   321,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   347,   348,   349,   350,   351,   352,   353,   354,
     355,   356,   357,   358,   359,   360,   361,   362,   363,   364,
     365,   366,   367,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   392,   393,   394,
     395,   396,   397,   398,   399,   400,   401,   402,   403,   404,
     405,   406,   407,   408,   409,   410,   411,   412,   413,   414,
     415,   416,   417,   418,   419,   420,   421,   422,   423,   424,
     425,   426,   427,   428,   429,   430,   431,   432,   433,   434,
     435,   436,   437,   438,   439,   440,   441,   442,   443,   444,
     445,   446,   447,   448,   449,   450,   451,   452,   453,   454,
     459
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   347,   347,   368,   368,   407,   424,   437,   450,   451,
     452,   456,   462,   472,   472,   472,   472,   473,   473,   473,
     473,   473,   474,   474,   475,   475,   475,   475,   475,   475,
     475,   476,   476,   476,   477,   477,   478,   478,   479,   479,
     479,   480,   480,   481,   481,   481,   481,   481,   482,   482,
     482,   483,   486,   500,   518,   528,   539,   552,   559,   574,
     578,   579,   582,   583,   586,   587,   589,   595,   618,   619,
     622,   624,   626,   628,   630,   632,   634,   641,   644,   651,
     651,   683,   683,   741,   742,   747,   753,   746,   770,   776,
     791,   796,   801,   805,   809,   813,   817,   821,   825,   830,
     847,   846,   883,   882,   900,   914,   926,   937,   949,   960,
     972,   984,   996,  1008,  1020,  1033,  1060,  1079,  1097,  1116,
    1116,  1121,  1120,  1135,  1150,  1166,  1189,  1165,  1192,  1215,
    1191,  1219,  1258,  1257,  1296,  1335,  1334,  1374,  1373,  1388,
    1427,  1426,  1465,  1504,  1503,  1543,  1542,  1583,  1582,  1621,
    1642,  1663,  1685,  1706,  1684,  1722,  1746,  1770,  1794,  1818,
    1842,  1866,  1890,  1913,  1931,  1949,  1967,  1985,  2003,  2028,
    2054,  2053,  2071,  2070,  2086,  2085,  2092,  2109,  2108,  2135,
    2134,  2154,  2153,  2171,  2189,  2215,  2214,  2232,  2234,  2240,
    2242,  2248,  2250,  2253,  2271,  2292,  2297,  2303,  2270,  2317,
    2321,  2316,  2345,  2344,  2369,  2371,  2374,  2381,  2382,  2383,
    2385,  2387,  2386,  2393,  2409,  2410,  2426,  2427,  2434,  2435,
    2451,  2452,  2471,  2490,  2491,  2509,  2510,  2513,  2517,  2521,
    2535,  2536,  2537,  2540,  2544,  2543,  2563,  2564,  2568,  2573,
    2574,  2608,  2609,  2610,  2614,  2616,  2619,  2613,  2623,  2625,
    2624,  2639,  2638,  2655,  2654,  2659,  2660,  2663,  2664,  2668,
    2669,  2673,  2681,  2689,  2701,  2703,  2706,  2710,  2711,  2714,
    2718,  2719,  2722,  2726,  2727,  2728,  2729,  2730,  2731,  2734,
    2735,  2736,  2737,  2739,  2740,  2747,  2752,  2757,  2762,  2767,
    2772,  2777,  2782,  2787,  2792,  2797,  2802,  2807,  2812,  2817,
    2822,  2827,  2832,  2837,  2842,  2847,  2852,  2857,  2862,  2867,
    2872,  2877,  2882,  2887,  2892,  2897,  2902,  2909,  2910,  2914,
    2913,  2923,  2924,  2928,  2927,  2938,  2942,  2949,  2950,  2953,
    2956,  2992,  2992,  3019,  3019,  3029,  3032,  3041,  3049,  3058,
    3060,  3063,  3058,  3084,  3085,  3088,  3092,  3096,  3100,  3100,
    3106,  3107,  3109,  3112,  3113,  3117,  3119,  3121,  3123,  3123,
    3126,  3137,  3138,  3142,  3152,  3160,  3170,  3179,  3188,  3197,
    3207,  3207,  3211,  3212,  3213,  3214,  3215,  3216,  3217,  3218,
    3219,  3220,  3221,  3222,  3223,  3224,  3225,  3226,  3228,  3231,
    3231,  3238,  3240,  3244,  3258,  3257,  3281,  3281,  3315,  3315,
    3325,  3344,  3343,  3371,  3372,  3390,  3391,  3394,  3396,  3400,
    3401,  3404,  3404,  3409,  3410,  3414,  3422,  3430,  3441,  3472,
    3474,  3477,  3480,  3480,  3486,  3488,  3492,  3511,  3530,  3566,
    3568,  3571,  3573,  3575,  3605,  3633,  3633,  3667,  3670,  3692,
    3705,  3707,  3710,  3724,  3739,  3745,  3746,  3749,  3752,  3763,
    3772,  3774,  3777,  3785,  3787,  3790,  3793,  3796,  3810,  3821,
    3822,  3832,  3821,  3862,  3867,  3888,  3890,  3909,  3910,  3914,
    3915,  3916,  3917,  3918,  3919,  3920,  3923,  3940,  3959,  3978,
    3978,  3983,  3984,  3988,  3996,  4004,  4015,  4016,  4025,  4029,
    4015,  4071,  4073,  4077,  4082,  4085,  4109,  4132,  4155,  4174,
    4180,  4180,  4190,  4190,  4221,  4223,  4227,  4233,  4235,  4240,
    4244,  4245,  4246,  4248,  4251,  4253,  4257,  4259,  4261,  4264,
    4264,  4268,  4270,  4273,  4273,  4277,  4279,  4283,  4282,  4290,
    4290,  4308,  4308,  4330,  4332,  4336,  4337,  4338,  4339,  4340,
    4341,  4343,  4345,  4347,  4349,  4350,  4351,  4353,  4355,  4357,
    4359,  4361,  4363,  4365,  4367,  4369,  4369,  4374,  4375,  4378,
    4389,  4391,  4395,  4397,  4399,  4403,  4413,  4421,  4430,  4438,
    4439,  4458,  4459,  4460,  4479,  4498,  4499,  4512,  4513,  4514,
    4537,  4538,  4544,  4547,  4552,  4553,  4554,  4555,  4556,  4557,
    4560,  4561,  4562,  4563,  4580,  4597,  4616,  4617,  4618,  4619,
    4620,  4621,  4623,  4626,  4630,  4639,  4648,  4658,  4668,  4708,
    4719,  4730,  4741,  4754,  4764,  4764,  4767,  4767,  4778,  4789,
    4805,  4809,  4812,  4828,  4835,  4835,  4841,  4841,  4863,  4864,
    4868,  4877,  4886,  4895,  4904,  4913,  4922,  4922,  4931,  4940,
    4942,  4944,  4946,  4955,  4964,  4973,  4982,  4991,  5000,  5009,
    5018,  5020,  5020,  5022,  5022,  5031,  5031,  5040,  5040,  5049,
    5058,  5060,  5062,  5071,  5080,  5089,  5098,  5107,  5116,  5125,
    5127,  5127,  5130,  5144,  5156,  5177,  5198,  5219,  5221,  5223,
    5253,  5283,  5313,  5343,  5373,  5403,  5433,  5464,  5463,  5493,
    5493,  5509,  5509,  5525,  5525,  5543,  5548,  5553,  5558,  5563,
    5568,  5573,  5578,  5583,  5588,  5593,  5598,  5603,  5608,  5613,
    5618,  5623,  5628,  5633,  5638,  5643,  5648,  5653,  5658,  5663,
    5668,  5673,  5678,  5683,  5688,  5693,  5698,  5705,  5706,  5710,
    5720,  5728,  5738,  5739,  5740,  5741,  5742,  5744,  5756,  5757,
    5762,  5763,  5764,  5765,  5766,  5767,  5770,  5771,  5772,  5773,
    5776,  5777,  5778,  5779,  5781,  5784,  5785,  5784,  5801,  5813,
    5834,  5855,  5875,  5895,  5917,  5938,  5941,  5942,  5944,  5945,
    5960,  5961,  5975,  5990,  5994,  5998,  6000,  6004,  6004,  6017,
    6017,  6032,  6036,  6047,  6060,  6068,  6059,  6071,  6079,  6070,
    6081,  6090,  6100,  6102,  6105,  6108,  6117,  6127,  6138,  6150,
    6162,  6183,  6184,  6187,  6188,  6187,  6194,  6195,  6198,  6204,
    6204,  6207,  6210,  6213,  6227,  6229,  6234,  6233,  6244,  6244,
    6247,  6246,  6250,  6259,  6261,  6263,  6265,  6267,  6269,  6271,
    6273,  6275,  6277,  6279,  6281,  6283,  6285,  6287,  6289,  6291,
    6293,  6297,  6299,  6301,  6305,  6307,  6309,  6311,  6315,  6317,
    6319,  6323,  6325,  6327,  6331,  6333,  6337,  6339,  6342,  6346,
    6348,  6353,  6354,  6356,  6361,  6363,  6367,  6369,  6371,  6375,
    6377,  6381,  6383,  6387,  6386,  6396,  6396,  6406,  6406,  6430,
    6431,  6435,  6435,  6442,  6442,  6449,  6449,  6456,  6456,  6462,
    6465,  6465,  6471,  6477,  6480,  6485,  6486,  6490,  6490,  6497,
    6497,  6507,  6508,  6511,  6515,  6515,  6519,  6519,  6523,  6523,
    6532,  6532,  6541,  6541,  6552,  6553,  6554,  6557,  6558,  6562,
    6563,  6567,  6568,  6569,  6570,  6571,  6572,  6573,  6575,  6578,
    6579,  6580,  6581,  6582,  6583,  6584,  6585,  6586,  6587,  6588,
    6589,  6590,  6592,  6593,  6596,  6602,  6604,  6613,  6617,  6618,
    6619,  6620,  6621,  6622,  6623,  6624,  6625,  6629,  6628,  6641,
    6642,  6646,  6646,  6655,  6655,  6664,  6664,  6674,  6674,  6683,
    6683,  6692,  6692,  6701,  6701,  6710,  6710,  6721,  6726,  6731,
    6736,  6741,  6749,  6750,  6755,  6762,  6769,  6777,  6779,  6778,
    6787,  6788,  6796,  6797,  6805,  6806,  6811,  6812,  6817,  6818,
    6821,  6823,  6825,  6850,  6849,  6874,  6898,  6900,  6901,  6900,
    6917,  6916,  6941,  6965,  6964,  6970,  6976,  6984,  6983,  6999,
    7013,  7030,  7031,  7039,  7040,  7057,  7058,  7058,  7063,  7063,
    7067,  7081,  7094,  7107,  7121,  7120,  7126,  7139,  7140,  7144,
    7151,  7155,  7154,  7160,  7163,  7168,  7169,  7173,  7172,  7179,
    7182,  7187,  7186,  7193,  7206,  7207,  7211,  7218,  7222,  7221,
    7228,  7231,  7236,  7237,  7242,  7241,  7248,  7251,  7257,  7281,
    7305,  7329,  7363,  7397,  7431,  7432,  7434
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "K_HISTORY", "K_ABUT",
  "K_ABUTMENT", "K_ACTIVE", "K_ANALOG", "K_ARRAY", "K_AREA", "K_BLOCK",
  "K_BOTTOMLEFT", "K_BOTTOMRIGHT", "K_BY", "K_CAPACITANCE",
  "K_CAPMULTIPLIER", "K_CLASS", "K_CLOCK", "K_CLOCKTYPE", "K_COLUMNMAJOR",
  "K_DESIGNRULEWIDTH", "K_INFLUENCE", "K_CORE", "K_CORNER", "K_COVER",
  "K_CPERSQDIST", "K_CURRENT", "K_CURRENTSOURCE", "K_CUT", "K_DEFAULT",
  "K_DATABASE", "K_DATA", "K_DIELECTRIC", "K_DIRECTION", "K_DO",
  "K_EDGECAPACITANCE", "K_EEQ", "K_END", "K_ENDCAP", "K_FALL", "K_FALLCS",
  "K_FALLT0", "K_FALLSATT1", "K_FALLRS", "K_FALLSATCUR", "K_FALLTHRESH",
  "K_FEEDTHRU", "K_FIXED", "K_FOREIGN", "K_FROMPIN", "K_GENERATE",
  "K_GENERATOR", "K_GROUND", "K_HEIGHT", "K_HORIZONTAL", "K_INOUT",
  "K_INPUT", "K_INPUTNOISEMARGIN", "K_COMPONENTPIN", "K_INTRINSIC",
  "K_INVERT", "K_IRDROP", "K_ITERATE", "K_IV_TABLES", "K_LAYER",
  "K_LEAKAGE", "K_LEQ", "K_LIBRARY", "K_MACRO", "K_MATCH", "K_MAXDELAY",
  "K_MAXLOAD", "K_METALOVERHANG", "K_MILLIAMPS", "K_MILLIWATTS",
  "K_MINFEATURE", "K_MUSTJOIN", "K_NAMESCASESENSITIVE", "K_NANOSECONDS",
  "K_NETS", "K_NEW", "K_NONDEFAULTRULE", "K_NONINVERT", "K_NONUNATE",
  "K_OBS", "K_OHMS", "K_OFFSET", "K_ORIENTATION", "K_ORIGIN", "K_OUTPUT",
  "K_OUTPUTNOISEMARGIN", "K_OVERHANG", "K_OVERLAP", "K_OFF", "K_ON",
  "K_OVERLAPS", "K_PAD", "K_PATH", "K_PATTERN", "K_PICOFARADS", "K_PIN",
  "K_PITCH", "K_PLACED", "K_POLYGON", "K_PORT", "K_POST", "K_POWER",
  "K_PRE", "K_PULLDOWNRES", "K_RECT", "K_RESISTANCE", "K_RESISTIVE",
  "K_RING", "K_RISE", "K_RISECS", "K_RISERS", "K_RISESATCUR",
  "K_RISETHRESH", "K_RISESATT1", "K_RISET0", "K_RISEVOLTAGETHRESHOLD",
  "K_FALLVOLTAGETHRESHOLD", "K_ROUTING", "K_ROWMAJOR", "K_RPERSQ",
  "K_SAMENET", "K_SCANUSE", "K_SHAPE", "K_SHRINKAGE", "K_SIGNAL", "K_SITE",
  "K_SIZE", "K_SOURCE", "K_SPACER", "K_SPACING", "K_SPECIALNETS",
  "K_STACK", "K_START", "K_STEP", "K_STOP", "K_STRUCTURE", "K_SYMMETRY",
  "K_TABLE", "K_THICKNESS", "K_TIEHIGH", "K_TIELOW", "K_TIEOFFR", "K_TIME",
  "K_TIMING", "K_TO", "K_TOPIN", "K_TOPLEFT", "K_TOPRIGHT",
  "K_TOPOFSTACKONLY", "K_TRISTATE", "K_TYPE", "K_UNATENESS", "K_UNITS",
  "K_USE", "K_VARIABLE", "K_VERTICAL", "K_VHI", "K_VIA", "K_VIARULE",
  "K_VLO", "K_VOLTAGE", "K_VOLTS", "K_WIDTH", "K_X", "K_Y", "T_STRING",
  "QSTRING", "NUMBER", "K_N", "K_S", "K_E", "K_W", "K_FN", "K_FS", "K_FE",
  "K_FW", "K_R0", "K_R90", "K_R180", "K_R270", "K_MX", "K_MY", "K_MXR90",
  "K_MYR90", "K_USER", "K_MASTERSLICE", "K_ENDMACRO", "K_ENDMACROPIN",
  "K_ENDVIARULE", "K_ENDVIA", "K_ENDLAYER", "K_ENDSITE", "K_CANPLACE",
  "K_CANNOTOCCUPY", "K_TRACKS", "K_FLOORPLAN", "K_GCELLGRID",
  "K_DEFAULTCAP", "K_MINPINS", "K_WIRECAP", "K_STABLE", "K_SETUP",
  "K_HOLD", "K_DEFINE", "K_DEFINES", "K_DEFINEB", "K_IF", "K_THEN",
  "K_ELSE", "K_FALSE", "K_TRUE", "K_EQ", "K_NE", "K_LE", "K_LT", "K_GE",
  "K_GT", "K_OR", "K_AND", "K_NOT", "K_DELAY", "K_TABLEDIMENSION",
  "K_TABLEAXIS", "K_TABLEENTRIES", "K_TRANSITIONTIME", "K_EXTENSION",
  "K_PROPDEF", "K_STRING", "K_INTEGER", "K_REAL", "K_RANGE", "K_PROPERTY",
  "K_VIRTUAL", "K_BUSBITCHARS", "K_VERSION", "K_BEGINEXT", "K_ENDEXT",
  "K_UNIVERSALNOISEMARGIN", "K_EDGERATETHRESHOLD1", "K_CORRECTIONTABLE",
  "K_EDGERATESCALEFACTOR", "K_EDGERATETHRESHOLD2", "K_VICTIMNOISE",
  "K_NOISETABLE", "K_EDGERATE", "K_OUTPUTRESISTANCE", "K_VICTIMLENGTH",
  "K_CORRECTIONFACTOR", "K_OUTPUTPINANTENNASIZE", "K_INPUTPINANTENNASIZE",
  "K_INOUTPINANTENNASIZE", "K_CURRENTDEN", "K_PWL",
  "K_ANTENNALENGTHFACTOR", "K_TAPERRULE", "K_DIVIDERCHAR", "K_ANTENNASIZE",
  "K_ANTENNAMETALLENGTH", "K_ANTENNAMETALAREA", "K_RISESLEWLIMIT",
  "K_FALLSLEWLIMIT", "K_FUNCTION", "K_BUFFER", "K_INVERTER",
  "K_NAMEMAPSTRING", "K_NOWIREEXTENSIONATPIN", "K_WIREEXTENSION",
  "K_MESSAGE", "K_CREATEFILE", "K_OPENFILE", "K_CLOSEFILE", "K_WARNING",
  "K_ERROR", "K_FATALERROR", "K_RECOVERY", "K_SKEW", "K_ANYEDGE",
  "K_POSEDGE", "K_NEGEDGE", "K_SDFCONDSTART", "K_SDFCONDEND", "K_SDFCOND",
  "K_MPWH", "K_MPWL", "K_PERIOD", "K_ACCURRENTDENSITY",
  "K_DCCURRENTDENSITY", "K_AVERAGE", "K_PEAK", "K_RMS", "K_FREQUENCY",
  "K_CUTAREA", "K_MEGAHERTZ", "K_USELENGTHTHRESHOLD", "K_LENGTHTHRESHOLD",
  "K_ANTENNAINPUTGATEAREA", "K_ANTENNAINOUTDIFFAREA",
  "K_ANTENNAOUTPUTDIFFAREA", "K_ANTENNAAREARATIO",
  "K_ANTENNADIFFAREARATIO", "K_ANTENNACUMAREARATIO",
  "K_ANTENNACUMDIFFAREARATIO", "K_ANTENNAAREAFACTOR",
  "K_ANTENNASIDEAREARATIO", "K_ANTENNADIFFSIDEAREARATIO",
  "K_ANTENNACUMSIDEAREARATIO", "K_ANTENNACUMDIFFSIDEAREARATIO",
  "K_ANTENNASIDEAREAFACTOR", "K_DIFFUSEONLY", "K_MANUFACTURINGGRID",
  "K_FIXEDMASK", "K_ANTENNACELL", "K_CLEARANCEMEASURE", "K_EUCLIDEAN",
  "K_MAXXY", "K_USEMINSPACING", "K_ROWMINSPACING", "K_ROWABUTSPACING",
  "K_FLIP", "K_NONE", "K_ANTENNAPARTIALMETALAREA",
  "K_ANTENNAPARTIALMETALSIDEAREA", "K_ANTENNAGATEAREA",
  "K_ANTENNADIFFAREA", "K_ANTENNAMAXAREACAR", "K_ANTENNAMAXSIDEAREACAR",
  "K_ANTENNAPARTIALCUTAREA", "K_ANTENNAMAXCUTCAR", "K_SLOTWIREWIDTH",
  "K_SLOTWIRELENGTH", "K_SLOTWIDTH", "K_SLOTLENGTH",
  "K_MAXADJACENTSLOTSPACING", "K_MAXCOAXIALSLOTSPACING",
  "K_MAXEDGESLOTSPACING", "K_SPLITWIREWIDTH", "K_MINIMUMDENSITY",
  "K_MAXIMUMDENSITY", "K_DENSITYCHECKWINDOW", "K_DENSITYCHECKSTEP",
  "K_FILLACTIVESPACING", "K_MINIMUMCUT", "K_ADJACENTCUTS",
  "K_ANTENNAMODEL", "K_BUMP", "K_ENCLOSURE", "K_FROMABOVE", "K_FROMBELOW",
  "K_IMPLANT", "K_LENGTH", "K_MAXVIASTACK", "K_AREAIO", "K_BLACKBOX",
  "K_MAXWIDTH", "K_MINENCLOSEDAREA", "K_MINSTEP", "K_ORIENT", "K_OXIDE1",
  "K_OXIDE2", "K_OXIDE3", "K_OXIDE4", "K_OXIDE5", "K_OXIDE6", "K_OXIDE7",
  "K_OXIDE8", "K_OXIDE9", "K_OXIDE10", "K_OXIDE11", "K_OXIDE12",
  "K_OXIDE13", "K_OXIDE14", "K_OXIDE15", "K_OXIDE16", "K_OXIDE17",
  "K_OXIDE18", "K_OXIDE19", "K_OXIDE20", "K_OXIDE21", "K_OXIDE22",
  "K_OXIDE23", "K_OXIDE24", "K_OXIDE25", "K_OXIDE26", "K_OXIDE27",
  "K_OXIDE28", "K_OXIDE29", "K_OXIDE30", "K_OXIDE31", "K_OXIDE32",
  "K_PARALLELRUNLENGTH", "K_MINWIDTH", "K_PROTRUSIONWIDTH",
  "K_SPACINGTABLE", "K_WITHIN", "K_ABOVE", "K_BELOW", "K_CENTERTOCENTER",
  "K_CUTSIZE", "K_CUTSPACING", "K_DENSITY", "K_DIAG45", "K_DIAG135",
  "K_MASK", "K_DIAGMINEDGELENGTH", "K_DIAGSPACING", "K_DIAGPITCH",
  "K_DIAGWIDTH", "K_GENERATED", "K_GROUNDSENSITIVITY", "K_HARDSPACING",
  "K_INSIDECORNER", "K_LAYERS", "K_LENGTHSUM", "K_MICRONS", "K_MINCUTS",
  "K_MINSIZE", "K_NETEXPR", "K_OUTSIDECORNER", "K_PREFERENCLOSURE",
  "K_ROWCOL", "K_ROWPATTERN", "K_SOFT", "K_SUPPLYSENSITIVITY", "K_USEVIA",
  "K_USEVIARULE", "K_WELLTAP", "K_ARRAYCUTS", "K_ARRAYSPACING",
  "K_ANTENNAAREADIFFREDUCEPWL", "K_ANTENNAAREAMINUSDIFF",
  "K_ANTENNACUMROUTINGPLUSCUT", "K_ANTENNAGATEPLUSDIFF", "K_ENDOFLINE",
  "K_ENDOFNOTCHWIDTH", "K_EXCEPTEXTRACUT", "K_EXCEPTSAMEPGNET",
  "K_EXCEPTPGNET", "K_LONGARRAY", "K_MAXEDGES", "K_NOTCHLENGTH",
  "K_NOTCHSPACING", "K_ORTHOGONAL", "K_PARALLELEDGE", "K_PARALLELOVERLAP",
  "K_PGONLY", "K_PRL", "K_TWOEDGES", "K_TWOWIDTHS", "IF", "LNOT", "'-'",
  "'+'", "'*'", "'/'", "UMINUS", "';'", "'('", "')'", "'='", "'\\n'",
  "'<'", "'>'", "$accept", "lef_file", "version", "$@1", "int_number",
  "dividerchar", "busbitchars", "rules", "end_library", "rule",
  "case_sensitivity", "wireextension", "fixedmask", "manufacturing",
  "useminspacing", "clearancemeasure", "clearance_type", "spacing_type",
  "spacing_value", "units_section", "start_units", "units_rules",
  "units_rule", "layer_rule", "start_layer", "$@2", "end_layer", "$@3",
  "layer_options", "layer_option", "$@4", "$@5", "$@6", "$@7", "$@8",
  "$@9", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15", "$@16", "$@17",
  "$@18", "$@19", "$@20", "$@21", "$@22", "$@23", "$@24", "$@25", "$@26",
  "$@27", "$@28", "$@29", "layer_arraySpacing_long",
  "layer_arraySpacing_width", "layer_arraySpacing_arraycuts",
  "layer_arraySpacing_arraycut", "sp_options", "$@30", "$@31", "$@32",
  "$@33", "$@34", "$@35", "$@36", "layer_spacingtable_opts",
  "layer_spacingtable_opt", "layer_enclosure_type_opt",
  "layer_enclosure_width_opt", "$@37", "layer_enclosure_width_except_opt",
  "layer_preferenclosure_width_opt", "layer_minimumcut_within",
  "layer_minimumcut_from", "layer_minimumcut_length",
  "layer_minstep_options", "layer_minstep_option", "layer_minstep_type",
  "layer_antenna_pwl", "$@38", "layer_diffusion_ratios",
  "layer_diffusion_ratio", "layer_antenna_duo", "layer_table_type",
  "layer_frequency", "$@39", "$@40", "$@41", "ac_layer_table_opt", "$@42",
  "$@43", "dc_layer_table", "$@44", "int_number_list", "number_list",
  "layer_prop_list", "layer_prop", "current_density_pwl_list",
  "current_density_pwl", "cap_points", "cap_point", "res_points",
  "res_point", "layer_type", "layer_direction", "layer_minen_width",
  "layer_oxide", "layer_sp_parallel_widths", "layer_sp_parallel_width",
  "$@45", "layer_sp_TwoWidths", "layer_sp_TwoWidth", "$@46",
  "layer_sp_TwoWidthsPRL", "layer_sp_influence_widths",
  "layer_sp_influence_width", "maxstack_via", "$@47", "via", "$@48",
  "via_keyword", "start_via", "via_viarule", "$@49", "$@50", "$@51",
  "via_viarule_options", "via_viarule_option", "$@52", "via_option",
  "via_other_options", "via_more_options", "via_other_option", "$@53",
  "via_prop_list", "via_name_value_pair", "via_foreign", "start_foreign",
  "$@54", "orientation", "via_layer_rule", "via_layer", "$@55",
  "via_geometries", "via_geometry", "$@56", "end_via", "$@57",
  "viarule_keyword", "$@58", "viarule", "viarule_generate", "$@59",
  "viarule_generate_default", "viarule_layer_list", "opt_viarule_props",
  "viarule_props", "viarule_prop", "$@60", "viarule_prop_list",
  "viarule_layer", "via_names", "via_name", "viarule_layer_name", "$@61",
  "viarule_layer_options", "viarule_layer_option", "end_viarule", "$@62",
  "spacing_rule", "start_spacing", "end_spacing", "spacings", "spacing",
  "samenet_keyword", "maskColor", "irdrop", "start_irdrop", "end_irdrop",
  "ir_tables", "ir_table", "ir_table_values", "ir_table_value",
  "ir_tablename", "minfeature", "dielectric", "nondefault_rule", "$@63",
  "$@64", "$@65", "end_nd_rule", "nd_hardspacing", "nd_rules", "nd_rule",
  "usevia", "useviarule", "mincuts", "nd_prop", "$@66", "nd_prop_list",
  "nd_layer", "$@67", "$@68", "$@69", "$@70", "nd_layer_stmts",
  "nd_layer_stmt", "site", "start_site", "$@71", "end_site", "$@72",
  "site_options", "site_option", "site_class", "site_symmetry_statement",
  "site_symmetries", "site_symmetry", "site_rowpattern_statement", "$@73",
  "site_rowpatterns", "site_rowpattern", "$@74", "pt", "macro", "$@75",
  "start_macro", "$@76", "end_macro", "$@77", "macro_options",
  "macro_option", "$@78", "macro_prop_list", "macro_symmetry_statement",
  "macro_symmetries", "macro_symmetry", "macro_name_value_pair",
  "macro_class", "class_type", "pad_type", "core_type", "endcap_type",
  "macro_generator", "macro_generate", "macro_source", "macro_power",
  "macro_origin", "macro_foreign", "macro_fixedMask", "macro_eeq", "$@79",
  "macro_leq", "$@80", "macro_site", "macro_site_word", "site_word",
  "macro_size", "macro_pin", "start_macro_pin", "$@81", "end_macro_pin",
  "$@82", "macro_pin_options", "macro_pin_option", "$@83", "$@84", "$@85",
  "$@86", "$@87", "$@88", "$@89", "$@90", "$@91", "$@92",
  "pin_layer_oxide", "pin_prop_list", "pin_name_value_pair",
  "electrical_direction", "start_macro_port", "macro_port_class_option",
  "macro_pin_use", "macro_scan_use", "pin_shape", "geometries", "geometry",
  "$@93", "$@94", "geometry_options", "layer_exceptpgnet", "layer_spacing",
  "firstPt", "nextPt", "otherPts", "via_placement", "$@95", "$@96",
  "stepPattern", "sitePattern", "trackPattern", "$@97", "$@98", "$@99",
  "$@100", "trackLayers", "layer_name", "gcellPattern", "macro_obs",
  "start_macro_obs", "macro_density", "density_layers", "density_layer",
  "$@101", "$@102", "density_layer_rects", "density_layer_rect",
  "macro_clocktype", "$@103", "timing", "start_timing", "end_timing",
  "timing_options", "timing_option", "$@104", "$@105", "$@106",
  "one_pin_trigger", "two_pin_trigger", "from_pin_trigger",
  "to_pin_trigger", "delay_or_transition", "list_of_table_entries",
  "table_entry", "list_of_table_axis_dnumbers", "slew_spec", "risefall",
  "unateness", "list_of_from_strings", "list_of_to_strings", "array",
  "$@107", "start_array", "$@108", "end_array", "$@109", "array_rules",
  "array_rule", "$@110", "$@111", "$@112", "$@113", "$@114",
  "floorplan_start", "floorplan_list", "floorplan_element", "$@115",
  "$@116", "cap_list", "one_cap", "msg_statement", "$@117",
  "create_file_statement", "$@118", "def_statement", "$@119", "$@120",
  "$@121", "dtrm", "then", "else", "expression", "b_expr", "s_expr",
  "relop", "prop_def_section", "$@122", "prop_stmts", "prop_stmt", "$@123",
  "$@124", "$@125", "$@126", "$@127", "$@128", "$@129", "$@130",
  "prop_define", "opt_range_second", "opt_endofline", "$@131",
  "opt_endofline_twoedges", "opt_samenetPGonly", "opt_def_range",
  "opt_def_value", "opt_def_dvalue", "layer_spacing_opts",
  "layer_spacing_opt", "$@132", "layer_spacing_cut_routing", "$@133",
  "$@134", "$@135", "$@136", "$@137", "spacing_cut_layer_opt",
  "opt_adjacentcuts_exceptsame", "opt_layer_name", "$@138",
  "req_layer_name", "$@139", "universalnoisemargin", "edgeratethreshold1",
  "edgeratethreshold2", "edgeratescalefactor", "noisetable", "$@140",
  "end_noisetable", "noise_table_list", "noise_table_entry",
  "output_resistance_entry", "$@141", "num_list", "victim_list", "victim",
  "$@142", "vnoiselist", "correctiontable", "$@143", "end_correctiontable",
  "correction_table_list", "correction_table_item", "output_list", "$@144",
  "numo_list", "corr_victim_list", "corr_victim", "$@145", "corr_list",
  "input_antenna", "output_antenna", "inout_antenna", "antenna_input",
  "antenna_inout", "antenna_output", "extension_opt", "extension", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1586)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-792)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
    2165, -1586,   141,  2368, -1586, -1586,    -4, -1586, -1586, -1586,
      -4,   515, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586,   -18, -1586,    -4,    -4,    -4,    -4,    -4,    -4,
      -4,    -4,    -4,    30,   530, -1586, -1586,     9,   134,   193,
      -4,  -127,   335,   224,    -4, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586,   205, -1586,   110, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586,    26,   217, -1586,    39,   313,
     345,    -4,    45,    59,   352,   355,   362,   374,   378,   381,
   -1586,   108,   407,    -4,   125,   128,   130,   132, -1586,   137,
     152,   166,   169,   202,   204,   500,   504,   216,   220,   226,
     228, -1586, -1586, -1586,   250, -1586, -1586,   566,  -132,   577,
    1178,     4,   436,   447, -1586,   664, -1586, -1586,   165,   122,
      23,   470,   555,   677, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586,   290, -1586, -1586, -1586, -1586, -1586,   298,   342,   367,
    1060, -1586,   391,   424, -1586, -1586, -1586, -1586,   437, -1586,
   -1586, -1586, -1586, -1586, -1586,   442,   445, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586,   440, -1586, -1586,   816,   844,   505,
     765,   852,   842,   850,   764,   634, -1586,   772,   920,    -4,
      16,    -4, -1586,    -4,    -4,    -4,   365,    -4,    -4,    -4,
      40,    -4, -1586,   -58,    -4,    -4,    91,   658,    -4, -1586,
      -4, -1586,    -4,    -4, -1586,    -4, -1586,    -4,    -4,    -4,
      -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,    -4,
      -4,    -4, -1586,   308,    -4,   779,    -4,    -4,    -4,   506,
      -4,    -4,    -4,    -4,    -4, -1586,   308, -1586,   492,    -4,
     494,    -4, -1586, -1586, -1586, -1586, -1586, -1586,    -4, -1586,
   -1586, -1586, -1586,   919, -1586, -1586, -1586,   464, -1586, -1586,
   -1586, -1586,   787, -1586,   -47,    29,   825, -1586, -1586, -1586,
     790,   900,   792, -1586, -1586, -1586,    70, -1586,    -4, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586,    75, -1586, -1586,   793,
     794, -1586, -1586,   -88, -1586,    -4, -1586,    -4,   263, -1586,
   -1586, -1586,   467,   507,   901,   614,   929, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
     798, -1586, -1586, -1586, -1586,   522, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586,   799, -1586,    -4, -1586,   935, -1586,
   -1586, -1586, -1586, -1586,   559,     5,  -117,   -89,   745, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
     519,   525,  -117,  -117, -1586,   810,    -4,    -4,    -4, -1586,
      -4,    -4,    -4,    -4,   809,   524,   -63,   527, -1586, -1586,
   -1586, -1586,   528,   529,   812,   531,   -82,   -45,   171,   536,
     553, -1586,   556, -1586, -1586, -1586, -1586, -1586, -1586,   558,
     564,   815,   565,    -4,   568,   572,   573, -1586, -1586, -1586,
      -4,   -36,   574,   230,   575,   230, -1586,   576,   230,   579,
     230, -1586,   580,   581,   583,   584,   585,   586,   589,   598,
     599,   600,    -4,   601,   602,   823,  1301, -1586, -1586,    -4,
     603, -1586, -1586,   612,   684,   650,     7,   616,   617,   626,
     -35,   627,   -88,    -4,   646,   -88,   629, -1586,   630,   921,
     922,   633,   924,   926, -1586, -1586,   443, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586,    -4,    -4,   638,   641,    83,   664,
     639,   533, -1586,   933,  1067,   -41, -1586, -1586,   119,    -4,
      -4,   -88,    -4,    -4,    -4,    -4, -1586, -1586,   947, -1586,
   -1586,   -30,   645,   660,   662,   953,  1112,  -118, -1586,  -179,
       1, -1586,   777,   560,    11, -1586, -1586, -1586, -1586,   669,
     961,   962,   965,   676,   967,   678,   969,   682,  1130,   685,
     686,   687,   -80,   974,   688,   689, -1586, -1586, -1586, -1586,
     690,   659, -1586, -1586,   -22,   692,  2029, -1586, -1586,   746,
     746,   746,   -19,    -4,  1117, -1586, -1586,  1613,   985,   985,
     578, -1586,   642, -1586,   985, -1586, -1586,   161,   696, -1586,
     -89,     5,     5, -1586,   259, -1586,   -89,  -117,  -281,   -89,
   -1586, -1586,   -89,   -89,   862,   281,   892, -1586,   987,   988,
     989,   991,   992,   993,   994,   995,    -4, -1586,    73, -1586,
   -1586,    -4, -1586,   293, -1586, -1586,  -281,  -281,   996,   707,
     709,   720,   723,   725,   726,   728,   729, -1586,   715,   730,
   -1586, -1586, -1586, -1586, -1586, -1586,   731, -1586,   734,   735,
     737, -1586, -1586,   -72, -1586, -1586, -1586,   550,   -83, -1586,
     740,    -4, -1586, -1586, -1586,   738,   882,    -4,  1023,   742,
   -1586,   743, -1586,   747, -1586,   748,   893, -1586,   749, -1586,
     750,   893, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586,   752, -1586, -1586,    -4, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,   754,    -4,
   -1586,  1049, -1586, -1586,    -4,    -4,  1050,    -4,  1051,   759,
   -1586, -1586, -1586, -1586,   760, -1586, -1586, -1586,    -4, -1586,
    1054,   -88, -1586, -1586, -1586,   766, -1586,   767,   588,   -74,
   -1586,  1055, -1586,    -4, -1586, -1586, -1586,   769,   746,   746,
   -1586,   239, -1586, -1586, -1586, -1586,   -41,   770, -1586, -1586,
   -1586,   773,   774,   775,   776,   -88,   778,  1211,  1083,    -4,
      -4, -1586,    -4, -1586, -1586, -1586, -1586, -1586,    -4, -1586,
   -1586, -1586, -1586, -1586,   -62, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
     780,   785,   791, -1586,   796, -1586, -1586, -1586,    -4, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586,   637,   -54, -1586,
   -1586, -1586,  1069,    93, -1586, -1586,   800,  1080, -1586,    -4,
   -1586,    -4,   184,   644, -1586,    -4,    -4, -1586,  1082,    -4,
   -1586,    -4,    -4, -1586, -1586, -1586,    -4,    -4,    -4,    -4,
      -4,    -4,    -4,   456,   509,    -4,  1099,    -4,    -4, -1586,
   -1586,  1084,    -4,    -4,  1065,    -4,    -4,  1081,  1087,  1089,
    1090,  1091,  1094,  1095,  1096, -1586, -1586, -1586, -1586,   -79,
   -1586, -1586, -1586,   112,  1101,    -4,   -21,   -20,   -16,   746,
     -88,   813, -1586,   453,  1121, -1586,    -4,    -4,    -4,    -4,
   -1586, -1586,    -4,    -4,    -4,    -4, -1586,   497,  1066, -1586,
   -1586,    -4,   814,   817, -1586, -1586,  1103,  1105,  1107, -1586,
   -1586, -1586, -1586, -1586,  1056,   449,   225,    -4,   820,   821,
      -4,    -4,   826,    -4,    -4,   827,    67,   833,  1115,  1124,
   -1586, -1586, -1586, -1586,   -15,    66, -1586,   236,     5,     5,
       5,     5, -1586, -1586, -1586,    66,  -110,  -117, -1586,    66,
   -1586,   716,    69,   848, -1586, -1586, -1586, -1586, -1586, -1586,
       5,     5, -1586, -1586, -1586,     5,   -89,   -89,   -89,   -89,
   -1586,  -117,  -117,  -117,   306,   306,   306,   306,   306,   306,
     306,   306,   835,    -4,  1052,  -299, -1586,   841,    -4,  1057,
    -299, -1586, -1586, -1586,   843, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586,   847, -1586, -1586, -1586,   861, -1586, -1586,
   -1586, -1586,    27,   -72, -1586, -1586, -1586, -1586, -1586,    -4,
     415, -1586,   840, -1586,  1137, -1586, -1586, -1586, -1586,   -88,
   -1586, -1586, -1586,   863, -1586, -1586,   864, -1586, -1586, -1586,
   -1586,    -4,   866,   -90,  1162,  1196,    -4, -1586,    -4, -1586,
   -1586,     8, -1586,    -4,   928, -1586, -1586,   930, -1586, -1586,
   -1586, -1586, -1586, -1586,   872, -1586, -1586,   -88,  1067, -1586,
    -115, -1586,  1165, -1586, -1586, -1586, -1586,   876, -1586,    -4,
      -4,   877,   -92, -1586,   878,  1318, -1586, -1586, -1586, -1586,
   -1586, -1586,   879, -1586, -1586, -1586, -1586, -1586,   880, -1586,
   -1586, -1586, -1586,  1318,   881,   883,   884,   886,   889,   890,
    -109,  1182,   896,   899,    -4,  1183,   906,  1197,   909,   910,
    1201,    -4,   912,   913,   914,   915,   916,   917,   918, -1586,
   -1586, -1586, -1586,   923, -1586, -1586, -1586,   927,   932, -1586,
   -1586, -1586, -1586, -1586, -1586,   934,   936,   937,  1209,    -4,
     938,  1316,  1316,  1316,   939,   941,  1316,  1316,  1316,  1316,
    1317,  1317,  1316,  1317,  2170,  1212,  1217,  1219,    12, -1586,
     680,    75, -1586,   453, -1586, -1586,   -88, -1586,   -88,   -88,
     -88,   -88,   -88, -1586, -1586, -1586, -1586,    -4,    -4,    -4,
      -4,  1225,    -4,    -4,    -4,    -4,  1233, -1586, -1586, -1586,
     944,    -4, -1586,   -14,    -4,   421, -1586, -1586,   945,   946,
     948,    -4, -1586, -1586, -1586,   567,    -4, -1586, -1586,  1251,
   -1586, -1586,  1375,  1376, -1586,  1377,  1378, -1586,  1213,    -4,
   -1586, -1586, -1586, -1586,   985,   985, -1586,   699, -1586,  1243,
    1246,  1247, -1586, -1586,  1381, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586,  1207,     5, -1586,   446,   446, -1586, -1586,  -117,
   -1586, -1586,   -89, -1586,   282,   282,   282, -1586, -1586,   695,
     279,   964,   964,   964,  1250,  1187,  1187,  1253,   966,   968,
     970,   971,   973,   975,   977,   978, -1586, -1586,    -5, -1586,
   -1586, -1586, -1586,    10, -1586, -1586, -1586,    -4,   452, -1586,
      -4,   457, -1586,   976,  1252, -1586,    -4,    -4,    -4,    -4,
      -4,    -4,   980, -1586,    -4,   981, -1586,   982, -1586, -1586,
   -1586,   -88, -1586, -1586,  1029,   -87, -1586, -1586, -1586, -1586,
      -4, -1586,    -4, -1586, -1586, -1586,    -4,    -4,  1045, -1586,
     997, -1586, -1586, -1586,  1277, -1586,    -4, -1586,    -4, -1586,
     -88,   -88, -1586, -1586, -1586, -1586, -1586,   986,   990, -1586,
     998, -1586, -1586, -1586, -1586, -1586,  1411, -1586, -1586, -1586,
   -1586, -1586, -1586,   999, -1586, -1586, -1586, -1586,    -4,  1000,
   -1586,  1001, -1586, -1586,  1002,    -4, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,   717,
     -52, -1586,    -4, -1586, -1586,  1003,  1004,  1005, -1586, -1586,
    1006,  1007,  1010,  1011, -1586,  1012,  1013,  1014,  1015, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586,  1016,  1017,  1018,  1019, -1586,   824, -1586,  1020,  1047,
    1414,  1068, -1586,    19,   -88,   -88,   -88,  1048, -1586,  1278,
    1070,  1071,  1072,  1073, -1586,   -37,  1074,  1075,  1079,  1085,
   -1586,   -27, -1586,  1245, -1586, -1586,    -4, -1586, -1586, -1586,
   -1586, -1586,    -4, -1586, -1586, -1586,  1227,    -4,   497,    -4,
      -4,    -4,    -4, -1586,  1306,  1088,  1092,  1284, -1586, -1586,
   -1586,   297,    -4,  1097,  1098,  1285, -1586, -1586,  -108,   -13,
     834,    88,   118, -1586,    -4,    -4,  1353, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586,  1206, -1586,  1276, -1586,
      -4,  1100, -1586,    -4,  1102, -1586, -1586, -1586, -1586,  1359,
      -4,  1307,  1143,  1145,  1104, -1586, -1586,  1106, -1586, -1586,
   -1586,    25,    34, -1586,    -4,   569,    -4,    -4,  1109, -1586,
   -1586,  1111, -1586,    -4,    -4,    -4,    -4,    -4,  1116, -1586,
    -107,    -4,   -88,  1120, -1586, -1586, -1586, -1586,  1435,    -4,
   -1586,  1122, -1586, -1586, -1586,  1123, -1586, -1586, -1586, -1586,
   -1586,  1131,  1380, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
    1389, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
    1132, -1586, -1586, -1586, -1586,    86,    -3, -1586,   -88, -1586,
    1513, -1586,  1391,  1133, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586,    -4,    -4,    -4,    -4,
   -1586,  1337,  1426,  1427,  1429,  1434,    -4, -1586, -1586, -1586,
   -1586,   -96,  1135, -1586, -1586, -1586, -1586,  1364,     5,  -117,
     -89,    -4, -1586, -1586, -1586, -1586,    -4,  1206, -1586,    -4,
    1276, -1586,  1119, -1586,  1128, -1586, -1586, -1586,    -4,    -4,
      -4,    -4, -1586,    42, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586,  1230, -1586, -1586, -1586, -1586,  1198,  1465,  1433,
   -1586, -1586, -1586, -1586,  1171,  1146, -1586, -1586,  1147,   -88,
   -1586,   -88, -1586,  1590, -1586, -1586, -1586, -1586, -1586, -1586,
      -4,    -4,  1148,    -4,  1149, -1586,    21,  1156,  1513, -1586,
     397,  1155,  1159,    -4,    -4,    -4,    -4,    -4,    -4,    -4,
    1163,  1438, -1586, -1586, -1586, -1586,   282,   964,   411, -1586,
    1166, -1586,  1172, -1586, -1586, -1586,  1486,    65,    -4, -1586,
   -1586,  1181, -1586,  1405,  1405,  -100,    -4,  1174,  1199,    -4,
    1176,  1198,    -4,    -4, -1586,    -4,  1177,  1171, -1586,  1223,
   -1586,   -88,  1435,    -4, -1586, -1586, -1586,  1628, -1586,    -3,
   -1586, -1586,  1184,  1185, -1586, -1586,    -4,    -4,  1483,    -4,
    1186,  1188, -1586, -1586, -1586,    -4, -1586, -1586, -1586, -1586,
      -4,    -4, -1586, -1586, -1586,  1218,  1200,    -4,     3,    -4,
   -1586, -1586, -1586,  1259, -1586,    -4, -1586,  1515, -1586, -1586,
   -1586, -1586,    -4,  1517, -1586, -1586, -1586, -1586,    32,    -4,
   -1586,  1521,    -4,  1235, -1586, -1586,  1236,    -4,    -4,    -4,
    1596,  1597,  1237,  1446,  1452,  1469,    -4, -1586, -1586,    -4,
   -1586, -1586,    -4,  1533,  1478, -1586,    -4, -1586,    -4, -1586,
   -1586,  1540,    -4,  1538, -1586,  1249,    -4,  1573, -1586, -1586,
      -4,    -4,  1254, -1586, -1586, -1586,    -4,    -4,    -4, -1586,
    1314, -1586, -1586,  1541, -1586, -1586, -1586,  1548,    -4,    -4,
   -1586,  1540, -1586,  1546, -1586,    -4,    -4,    -4,  1257, -1586,
   -1586, -1586, -1586, -1586,    51, -1586,    53,    -4,    -4, -1586,
   -1586, -1586,    56,    -4, -1586, -1586,   997, -1586,  1549, -1586,
      -4,    -4, -1586,  1550,  1550,    38, -1586, -1586, -1586, -1586,
   -1586, -1586,    74,    81, -1586, -1586,  1320,  1554,    -4,  1263,
   -1586,    -4, -1586, -1586,  1704,    -4, -1586,  1610,    -4,    -4,
      -4, -1586,  1286, -1586, -1586,    87,    -4,    -4, -1586, -1586,
    1332, -1586,    -4,  1279,  1566,    -4,  1280,  1281,  1287, -1586,
   -1586, -1586,  1604, -1586, -1586,    -4,  1288, -1586, -1586,  1289,
   -1586, -1586, -1586,    -4, -1586,    -4,    -4, -1586, -1586, -1586,
      -4,  1290,  1396,    -4,    -4,    -4,    -4,  1291, -1586, -1586,
     -17,    -4,    -4, -1586,    -4, -1586,    -4,    -4,  1582,    -4,
      -4,  1293,  1294,  1295,    -4, -1586, -1586, -1586,  1296, -1586
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       0,    10,     0,  1074,     1,   875,     0,   448,    79,   529,
       0,     0,   459,   500,   438,    67,   335,   398,   908,   910,
     912,   957,     0,     3,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   904,   906,     0,     0,     0,
       0,     0,     0,     0,     0,    13,    21,    14,     9,    15,
      22,    47,    46,    48,    49,    16,    68,    17,    83,    50,
      18,     0,   333,     0,    19,    20,    24,   440,    27,   450,
      26,    25,    32,    28,   504,    29,   533,    30,   879,    23,
      51,    31,    33,    34,    35,    37,    36,    38,    39,    40,
      41,    42,    43,    44,    45,    11,     0,     5,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     959,     0,     0,     0,     0,     0,     0,     0,  1034,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    56,    61,    60,     0,    62,    63,     0,     0,     0,
       0,   336,     0,   403,   422,   419,   405,   424,     0,     0,
       0,   527,   873,     0,  1076,     2,  1075,   876,   458,    80,
     530,     0,    53,    52,   460,   501,   399,     0,     0,     0,
       0,     7,     0,     0,  1031,  1051,  1033,  1032,     0,  1069,
    1068,  1070,     6,    55,    54,     0,     0,  1071,  1072,  1073,
      57,    59,    65,    64,     0,   331,   330,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    69,     0,     0,     0,
       0,     0,    81,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   119,     0,     0,     0,     0,     0,     0,   132,
       0,   135,     0,     0,   140,     0,   143,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   147,   207,     0,     0,     0,     0,     0,   177,
       0,     0,     0,     0,     0,   185,   207,    85,     0,     0,
       0,     0,    78,    84,   337,   338,   370,   389,     0,   360,
     339,   358,   350,     0,   351,   353,   355,     0,   356,   391,
     404,   401,     0,   406,   407,   418,     0,   444,   437,   441,
       0,     0,     0,   447,   451,   453,     0,   502,     0,   514,
     519,   499,   505,   508,   507,   509,     0,   809,   614,     0,
       0,   616,   799,     0,   624,     0,   620,     0,     0,   560,
     812,   555,     0,     0,     0,     0,     0,   534,   539,   535,
     536,   537,   538,   542,   541,   543,   540,   544,   545,   547,
       0,   546,   548,   628,   551,     0,   552,   553,   554,   814,
     621,   883,   885,   887,     0,   890,     0,   881,     0,   880,
     895,   893,    12,   457,   465,     0,     0,     0,     0,   963,
     973,   961,   967,   975,   965,   969,   971,   960,     4,  1030,
       0,     0,     0,     0,    58,     0,     0,     0,     0,    66,
       0,     0,     0,     0,     0,     0,     0,     0,   279,   280,
     281,   282,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   100,     0,   274,   275,   273,   276,   277,   278,     0,
       0,     0,     0,     0,     0,     0,     0,   242,   241,   243,
     121,     0,     0,     0,     0,     0,   137,     0,     0,     0,
       0,   145,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   208,   209,     0,
       0,   170,   174,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   187,     0,     0,   149,     0,     0,
       0,     0,     0,     0,   396,   334,   352,   372,   374,   375,
     373,   376,   378,   379,   377,   380,   381,   382,   383,   386,
     384,   387,   385,   366,     0,     0,     0,     0,   388,     0,
       0,     0,   411,     0,     0,   408,   409,   420,     0,     0,
       0,     0,     0,     0,     0,     0,   425,   439,     0,   449,
     456,     0,     0,     0,     0,     0,     0,     0,   521,   572,
     580,   581,   569,     0,   577,   571,   578,   575,   576,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   613,   803,   801,   609,
       0,     0,   531,   528,     0,     0,     0,   798,   755,   445,
     445,   445,   445,     0,     0,   766,   765,     0,     0,     0,
       0,   894,     0,   901,     0,   877,   874,     0,     0,   467,
       0,     0,     0,   928,   914,   947,     0,     0,   914,     0,
     943,   942,     0,     0,     0,   914,     0,   958,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1058,     0,  1054,
    1057,     0,  1041,     0,  1037,  1040,   914,   914,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    99,     0,     0,
     113,   104,   114,    82,   109,    94,     0,    90,     0,     0,
       0,   183,   112,  1000,   111,    88,    98,     0,     0,   259,
       0,     0,   116,   115,   110,     0,     0,     0,     0,     0,
     131,     0,   233,     0,   134,     0,   239,   139,     0,   142,
       0,   239,   155,   156,   157,   158,   159,   160,   161,   162,
     163,   164,     0,   166,   167,     0,   285,   286,   287,   288,
     289,   290,   291,   292,   293,   294,   295,   296,   297,   298,
     299,   300,   301,   302,   303,   304,   305,   306,   307,   308,
     309,   310,   311,   312,   313,   314,   315,   316,     0,     0,
     168,   283,   225,   169,     0,     0,     0,     0,     0,     0,
      89,   184,    97,    92,     0,    96,   773,   775,     0,   188,
     189,     0,   151,   150,   371,     0,   357,     0,     0,     0,
     361,     0,   354,     0,   525,   369,   367,     0,   445,   445,
     392,   407,   423,   415,   416,   417,     0,     0,   435,   400,
     410,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   452,     0,   454,   511,   510,   512,   503,     0,   516,
     517,   518,   513,   515,     0,   573,   574,   590,   593,   591,
     592,   594,   595,   582,   570,   600,   601,   597,   596,   598,
     599,   583,   586,   584,   585,   587,   588,   589,   579,   568,
       0,     0,     0,   602,     0,   608,   625,   607,     0,   606,
     605,   604,   562,   563,   564,   559,   561,     0,     0,   557,
     549,   550,     0,     0,   612,   610,     0,     0,   618,     0,
     619,     0,     0,     0,   626,     0,     0,   657,     0,     0,
     636,     0,     0,   651,   653,   737,     0,     0,     0,     0,
       0,     0,     0,     0,   750,     0,     0,     0,     0,   670,
     655,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   687,   693,   689,   691,     0,
     623,   629,   639,   738,     0,     0,     0,     0,     0,   445,
       0,     0,   797,   754,     0,   865,     0,     0,     0,     0,
     816,   864,     0,     0,     0,     0,   818,     0,     0,   844,
     845,     0,     0,     0,   846,   847,     0,     0,     0,   841,
     842,   843,   811,   815,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     897,   899,   896,   466,   461,     0,   925,     0,     0,     0,
       0,     0,   915,   916,   909,     0,     0,     0,   911,     0,
     939,     0,     0,     0,   952,   953,   948,   949,   950,   951,
       0,     0,   954,   955,   956,     0,     0,     0,     0,     0,
     913,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   914,  1055,     0,     0,     0,
     914,  1038,   905,   907,     0,    71,    74,    76,    73,    72,
      70,    75,    77,     0,   107,    95,    91,     0,   105,  1003,
    1002,  1005,  1006,  1000,   261,   262,   263,   120,   260,     0,
       0,   264,     0,   123,     0,   122,   128,   125,   124,     0,
     133,   136,   240,     0,   141,   144,     0,   165,   172,   148,
     179,     0,     0,     0,     0,     0,     0,   194,     0,   178,
      93,     0,   181,     0,     0,   152,   390,     0,   365,   364,
     363,   359,   362,   397,     0,   368,   394,     0,     0,   413,
       0,   421,     0,   426,   427,   434,   433,     0,   432,     0,
       0,     0,     0,   455,     0,     0,   520,   522,   810,   615,
     603,   617,     0,   567,   566,   565,   556,   558,     0,   800,
     802,   611,   532,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   746,
     747,   748,   749,     0,   751,   753,   752,     0,     0,   741,
     744,   745,   743,   742,   740,     0,     0,     0,     0,     0,
       0,  1025,  1025,  1025,     0,     0,  1025,  1025,  1025,  1025,
       0,     0,  1025,     0,     0,     0,     0,     0,     0,   630,
       0,     0,   673,     0,   756,   446,     0,   775,     0,     0,
       0,     0,     0,   777,   758,   767,   813,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   866,   867,   868,
       0,     0,   859,     0,     0,     0,   856,   840,     0,     0,
       0,     0,   848,   849,   850,     0,     0,   854,   855,     0,
     884,   886,     0,     0,   888,     0,     0,   891,     0,     0,
     902,   882,   878,   889,     0,     0,   486,     0,   479,     0,
       0,     0,   470,   471,     0,   468,   473,   474,   475,   472,
     469,   917,     0,     0,   926,   922,   921,   923,   924,     0,
     945,   944,     0,   940,   931,   930,   929,   935,   936,   938,
     937,   934,   933,   932,   979,   994,   994,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1056,  1060,     0,  1053,
    1052,  1039,  1043,     0,  1036,  1035,   332,     0,     0,   267,
       0,     0,   270,   992,     0,  1007,     0,     0,     0,     0,
       0,     0,     0,  1001,     0,     0,   265,     0,   244,   255,
     257,     0,   138,   146,   218,   210,   284,   171,   232,   230,
       0,   231,     0,   175,   226,   227,     0,     0,     0,   255,
     325,   186,   774,   776,   216,   190,     0,   236,     0,   526,
       0,     0,   402,   412,   414,   436,   430,     0,     0,   428,
       0,   442,   506,   523,   622,   804,   783,   659,   664,   665,
     736,   735,   732,     0,   733,   627,   646,   644,     0,     0,
     642,     0,   660,   661,     0,     0,   638,   663,   662,   645,
     643,   666,   667,   641,   650,   649,   640,   648,   647,     0,
       0,   727,     0,   669,  1026,     0,     0,     0,   677,   678,
       0,     0,     0,     0,  1028,     0,     0,     0,     0,   695,
     696,   697,   698,   699,   700,   701,   702,   703,   704,   705,
     706,   707,   708,   709,   710,   711,   712,   713,   714,   715,
     716,   717,   718,   719,   720,   721,   722,   723,   724,   725,
     726,     0,     0,     0,     0,   633,     0,   631,     0,     0,
       0,   768,   775,     0,     0,     0,     0,     0,   779,     0,
       0,     0,     0,     0,   869,     0,     0,     0,     0,     0,
     871,     0,   833,     0,   823,   860,     0,   824,   857,   837,
     838,   839,     0,   851,   852,   853,     0,     0,     0,     0,
       0,     0,     0,   892,     0,     0,     0,     0,   483,   484,
     485,     0,     0,     0,     0,   463,   462,   918,     0,     0,
       0,     0,     0,   980,     0,   998,   996,   981,   964,   974,
     962,   968,   976,   966,   970,   972,     0,  1061,     0,  1044,
       0,     0,   268,     0,     0,   271,   993,  1004,  1012,     0,
       0,  1015,     0,     0,     0,  1019,   101,     0,   117,   118,
     257,     0,     0,   234,     0,   220,     0,     0,     0,   228,
     229,     0,   102,     0,   195,     0,     0,     0,     0,    86,
       0,     0,     0,     0,   431,   429,   443,   524,     0,     0,
     734,     0,   668,   637,   652,     0,   731,   730,   729,   671,
     728,     0,     0,   674,   676,   675,   679,   680,   683,   682,
       0,   684,   685,   681,   686,   688,   694,   690,   692,   634,
       0,   632,   739,   672,   769,   770,     0,   759,     0,   775,
       0,   761,     0,     0,   828,   832,   830,   826,   870,   817,
     827,   825,   829,   831,   872,   819,     0,     0,     0,     0,
     820,     0,     0,     0,     0,     0,     0,   898,   900,   487,
     481,     0,     0,   476,   477,   464,   919,     0,     0,     0,
       0,     0,   999,   977,   997,   978,     0,  1059,  1062,     0,
    1042,  1045,     0,   108,     0,   106,  1008,  1013,     0,     0,
       0,     0,   266,     0,   129,   256,   258,   126,   236,   219,
     221,   222,   223,   211,   213,   180,   176,   204,     0,     0,
     326,   199,   217,   182,   191,     0,   237,   238,     0,     0,
     393,     0,   806,     0,   658,   654,   656,  1027,  1029,   635,
       0,     0,     0,     0,     0,   775,     0,     0,     0,   778,
       0,     0,     0,     0,   861,     0,     0,     0,     0,     0,
       0,     0,   480,   482,   478,   920,   927,   946,   941,   995,
       0,  1063,     0,  1046,   269,   272,  1021,   982,     0,  1010,
    1017,     0,   245,     0,     0,     0,     0,     0,   214,     0,
       0,   204,     0,     0,   255,     0,     0,   191,   153,     0,
     775,     0,   805,     0,   772,   771,   757,     0,   760,     0,
     763,   762,     0,     0,   858,   836,     0,     0,     0,     0,
     784,   787,   795,   796,   903,     0,  1064,  1047,  1022,  1009,
       0,     0,   983,  1014,  1016,  1023,   987,     0,   248,     0,
     130,   127,   235,     0,   173,     0,   212,     0,   103,   205,
     202,   196,   200,     0,    87,   192,   154,   340,     0,     0,
     807,     0,     0,     0,   780,   834,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   984,     0,  1024,  1011,     0,
    1018,  1020,     0,     0,     0,   253,     0,   215,     0,   327,
     255,   321,     0,     0,   395,     0,     0,     0,   764,   835,
       0,     0,     0,   785,   788,   488,     0,     0,     0,   986,
       0,   251,   249,     0,   255,   224,   206,   203,   197,     0,
     201,   321,   193,     0,   808,     0,     0,   862,     0,   822,
     792,   792,   491,  1066,     0,  1049,     0,     0,     0,   255,
     257,   246,     0,     0,   328,   317,   325,   322,     0,   782,
       0,     0,   821,   786,   789,     0,  1065,  1067,  1048,  1050,
     985,   988,     0,     0,   257,   254,     0,   198,     0,     0,
     781,     0,   794,   793,     0,     0,   489,     0,     0,     0,
       0,   492,   990,   252,   250,     0,     0,     0,   318,   323,
       0,   863,     0,     0,     0,     0,     0,     0,     0,   991,
     989,   247,     0,   319,   255,     0,     0,   497,   490,     0,
     493,   494,   498,     0,   255,   324,     0,   496,   495,   329,
     320,     0,     0,     0,     0,     0,     0,     0,   341,   343,
     342,     0,     0,   348,     0,   344,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   346,   349,   345,     0,   347
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1586, -1586, -1586, -1586,    -6, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,   -86, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,   -84, -1586,
    1492, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586,   116, -1586,     2, -1586,  1058, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586,   -70, -1586, -1391, -1585, -1586,  1093,
   -1586,   693, -1586,   404, -1586,   405, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586,  -206, -1586, -1586,  -229, -1586, -1586, -1586,
   -1586,   781, -1586,  1474, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586,  1274, -1586, -1586,  1009, -1586,
    -139, -1586,  -332, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586,  1255,  1021, -1586,
    -498, -1586, -1586,  -130, -1586, -1586, -1586, -1586, -1586, -1586,
     648, -1586,   783, -1586, -1586, -1586, -1586, -1586,  -513, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1460, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586,  -211, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
     904, -1586,   547, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
     319, -1586, -1586, -1586, -1586, -1586, -1586,   548,   837, -1586,
   -1586, -1586, -1586, -1586,  -920, -1210, -1228, -1586, -1586, -1586,
   -1502,  -593, -1586, -1586, -1586, -1586, -1586,  -199, -1586, -1586,
   -1586, -1586, -1586, -1586,   931, -1586, -1586, -1586,   -69, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586,   521, -1586, -1586,   -11,   232,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586,  1639, -1586, -1586, -1586,
    -588,  -712,  -913,  -343,  -609,  -336,  -624, -1586, -1586, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,  -257,
   -1586, -1586, -1586, -1586, -1586,   458, -1586, -1586,   719, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,  -446,
   -1586,  -658, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586,
   -1586,  1150, -1586, -1586, -1586, -1586,    55, -1586, -1586, -1586,
   -1586, -1586, -1586,  1158, -1586, -1586, -1586, -1586,    68, -1586,
   -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586, -1586
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,    45,   112,   515,    46,    47,     3,   155,    48,
      49,    50,    51,    52,    53,    54,   134,   137,   194,    55,
      56,   139,   206,    57,    58,    99,   272,   414,   140,   273,
     484,  1774,   673,  1767,   431,   686,  1380,  1834,  1379,  1833,
     443,   445,   696,   448,   450,   701,   466,  1407,  1906,   751,
    1384,   752,   476,  1385,  1404,   482,   770,  1114,  1846,  1847,
     759,  1399,  1769,  1940,  1995,  1844,  1941,  1939,  1840,  1841,
     469,  1628,  1838,  1896,  1638,  1625,  1762,  1837,  1103,  1394,
    1395,   693,  1758,  1640,  1776,  1093,   440,  1085,  1620,  1888,
    2014,  1934,  1990,  1989,  1890,  1964,  1621,  1622,   678,   679,
    1080,  1081,  1358,  1359,  1361,  1362,   429,   412,  1102,   748,
    2017,  2038,  2064,  1970,  1971,  2054,  1636,  1967,  1994,    59,
     395,    60,   142,    61,    62,   282,   492,  1943,  2079,  2080,
    2085,  2088,   283,   284,   496,   285,   493,   779,   780,   286,
     287,   489,   516,   288,   289,   490,   518,   790,  1410,   495,
     781,    63,   106,    64,    65,   519,   291,   145,   524,   525,
     526,   796,  1130,   146,   294,   527,   147,   292,   295,   536,
     799,  1132,    66,    67,   298,   148,   299,   300,   936,    68,
      69,   303,   149,   304,   541,   813,   305,    70,    71,    72,
     104,   374,  1304,  1576,   609,   994,  1305,  1306,  1307,  1308,
    1309,  1571,  1721,  1310,  1567,  1811,  1982,  2044,  2005,  2031,
      73,    74,   105,   311,   545,   150,   312,   313,   314,   547,
     823,   315,   548,   824,  1147,  1647,  1402,    75,   336,    76,
     100,   583,   877,   151,   337,   573,   868,   338,   572,   866,
     869,   339,   559,   848,   833,   841,   340,   341,   342,   343,
     344,   345,   346,   347,   561,   348,   564,   349,   350,   367,
     351,   352,   353,   566,   930,  1171,   586,   931,  1177,  1180,
    1181,  1209,  1174,  1208,  1224,  1226,  1227,  1225,  1511,  1460,
    1461,   932,   933,  1233,  1205,  1193,  1197,   594,   595,   934,
    1521,   943,  1685,  1792,   767,  1403,  1111,   596,  1529,  1692,
    1794,   585,   982,  1920,  1980,  1921,  1981,  2003,  2023,   985,
     354,   355,   356,   873,   578,   872,  1648,  1852,  1782,   357,
     560,   358,   359,   972,   597,   973,  1251,  1256,  1804,   974,
     975,  1275,  1556,  1279,  1265,  1266,  1263,  1868,   976,  1260,
    1535,  1541,    77,   368,    78,    96,   606,   988,   152,   369,
     604,   598,   599,   600,   602,   370,   607,   992,  1294,  1295,
     986,  1290,    79,   125,    80,   126,    81,   107,   108,   109,
    1004,  1313,  1728,   624,   625,   626,  1025,    82,   110,   170,
     387,   630,   628,   633,   631,   634,   635,   629,   632,  1338,
    1883,  1930,  2032,  2050,  1607,  1585,  1735,  1733,  1072,  1073,
    1363,  1372,  1609,  1826,  1885,  1827,  1886,  1879,  1928,  1465,
    1662,  1475,  1670,    83,    84,    85,    86,    87,   178,  1050,
     643,   644,   645,  1048,  1353,  1740,  1741,  1924,  1986,    88,
     390,  1045,   638,   639,   640,  1043,  1348,  1737,  1738,  1923,
    1984,    89,    90,    91,    92,    93,    94,    95,   156
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      98,   995,  1033,   580,   101,   978,   979,  1005,  1634,  1523,
    1009,   987,   335,  1010,  1012,   293,  1237,  1239,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   800,   756,  1525,
    1008,  1793,   614,   274,   130,  1753,  1364,  1030,   138,   306,
     618,  1236,  1238,   939,  1420,  1433,  1240,   827,  1388,  1296,
     819,   820,  2024,  1069,   615,   521,   646,   647,  1052,  1053,
     307,  1228,   528,   153,   821,    97,   842,   843,   423,  2081,
     408,  2082,    97,  2025,  1297,  2026,   517,   937,   938,   940,
    1626,  2083,   615,    97,    97,   549,  1880,   677,   862,   863,
      97,  1365,   542,    97,   616,   161,   778,   550,   551,   552,
     844,   529,   864,   195,  1288,  1726,  1790,   173,  1145,    97,
    1044,  1720,   565,   553,    97,    16,   867,   845,  1459,    14,
     530,   522,   619,   521,   581,   620,   621,    97,  1231,   521,
    1159,   687,   424,  1698,   828,   622,    97,    97,   531,   532,
    1298,     4,    97,  1704,   846,   829,   830,    16,  2027,  1232,
      97,    97,    97,   111,   308,  1297,    97,   577,    97,   301,
     143,  1002,   425,   533,   309,  1003,   543,    97,    97,    97,
    1932,   554,  2028,   801,   144,  1007,   409,    97,   825,  1002,
      97,   127,    97,  1003,    97,   787,   788,   555,  1797,   522,
    1165,    97,   789,    97,   658,   522,   534,    97,   989,   432,
    1726,   122,   296,   407,    97,   413,  1756,   415,   416,   417,
     419,   420,   421,   422,  1756,   430,   610,   434,   435,   436,
    1791,  1298,   442,    97,   444,    97,   446,   447,    97,   449,
     426,   451,   452,   453,   454,   455,   456,   457,   458,   459,
     460,   461,   462,   463,   464,   465,    97,   826,   470,   876,
     472,   473,   474,  1756,   477,   478,   479,   480,   481,  1756,
     688,  1813,  1366,   486,   302,   488,   154,  1627,   996,   997,
    1289,   766,   491,   569,   771,  1126,  1127,   427,  1311,   802,
    1011,  1006,  1026,  1027,  1276,  1026,  1027,  1013,  1028,  1029,
     297,  1028,  1029,  1319,  1686,  1166,  1862,  1322,  1129,  1933,
    1881,  1726,   546,   144,  1026,  1027,   128,   544,   135,  2029,
    1028,  1029,   556,   570,  1688,  1689,  1522,   831,  1524,   567,
     805,   568,   636,   637,   136,  1389,  1367,  1390,   196,  1070,
    1049,  1726,  1391,   131,  1014,  1015,  1016,  1017,  1018,  1019,
    1031,  1032,   822,    97,   617,  1413,  1007,   998,   999,  1000,
    1001,  1434,  1320,  1392,   514,  1775,  1727,  1913,   990,   991,
     603,   514,  1892,  1882,  1812,   129,   611,   847,  1421,   613,
    1393,   613,   623,   514,  1368,   141,  1071,  1077,   665,   535,
     865,  1229,   514,   437,   438,   439,  1121,   157,   935,  1033,
     649,   650,   651,   428,   652,   653,   654,   655,  1146,   557,
     659,   757,    97,   433,  1299,  2013,  1156,  2084,  1659,   521,
     666,   668,   670,  1300,  1301,   667,   275,  1327,  1328,  1329,
    1330,   410,   411,  1699,   558,   763,  1242,   681,   669,  2035,
     811,   832,  1779,  1705,   685,   689,   945,   692,   878,   692,
     514,   514,   692,  1007,   692,   514,  1544,   929,   310,  2030,
    1277,  1727,   571,  1902,  1278,  1596,   712,  1350,   514,   758,
     611,  1796,  1355,   749,  1369,  1370,   612,  1297,  1401,   514,
    1598,  1371,  1515,   514,   764,   522,   290,   768,  1795,  1687,
     514,  1860,   514,   159,   276,  1754,   316,   691,   317,   418,
    1642,   276,  1944,   514,  1757,  1026,  1027,  1026,  1027,   158,
     277,  1028,  1832,  1028,  1029,   162,   318,   277,   783,   784,
     951,  2006,  1189,  2008,  1194,   160,  2015,   588,   276,   163,
     319,   320,   164,   803,   804,   165,   806,   807,   808,   809,
    1312,  1323,   166,  1298,  2033,   812,   321,    97,  1334,  1335,
    1336,  2034,   641,   642,   167,  1190,   278,  2051,   168,  1968,
     589,   169,  1727,   278,   322,  1195,   590,  1257,   323,   587,
    1115,   695,   591,  1476,   698,  1478,   700,  1859,   171,  1850,
     324,   835,   836,  1992,  1007,  1337,   325,   172,   879,  1258,
    1259,  1022,  1727,  1023,  1024,   174,   588,   941,   175,   279,
     176,   197,   177,  1191,  1137,  1192,   279,   179,  2012,   280,
     326,   327,   328,   198,   613,   613,   613,   199,   102,   103,
     613,   329,   180,   613,   200,   592,   613,   613,   330,   589,
     593,  1196,  1908,   123,   124,   590,   181,  1026,  1027,   182,
    1042,   591,  1414,  1028,  1029,  1047,    97,   497,   498,   499,
     500,   501,   502,   503,   504,   505,   506,   507,   508,   509,
     510,   511,   512,   132,   133,  1315,  1316,  1317,  1318,   192,
     193,   293,   183,  2065,   184,   837,  1729,   838,  1730,  1729,
     185,  1321,   281,  2070,   186,  1082,   187,  1324,  1325,   281,
     188,  1086,  1326,   201,   592,   360,   189,   202,   190,   593,
    1167,   998,   999,  1000,  1001,  1331,  1332,  1333,  1314,  1168,
    1169,  1565,  1566,   793,   794,   795,   331,   467,   468,  1098,
     191,   839,   840,  1581,   998,   999,  1000,  1001,  1230,  1002,
    1074,  1075,  1076,  1003,   203,   766,   766,  1241,   144,  1243,
    1272,  1273,  1274,  1170,   574,   575,   332,   998,   999,  1000,
    1001,  1002,   204,  1100,   372,  1003,   980,   981,  1104,  1105,
     373,  1107,   361,   362,   363,   364,   365,   366,  1118,  1119,
    1120,   375,  1112,    18,    19,    20,  1466,  1467,   636,   637,
    1470,  1471,  1472,  1473,   641,   642,  1477,  1124,  1339,  1340,
    1341,  1342,  1343,  1344,  1345,   333,    97,   497,   498,   499,
     500,   501,   502,   503,   504,   505,   506,   507,   508,   509,
     510,   511,   512,  1141,  1142,   376,  1143,  1153,  1154,  1155,
     983,   984,  1144,  1423,   497,   498,   499,   500,   501,   502,
     503,   504,   505,   506,   507,   508,   509,   510,   511,   512,
     377,  1426,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,  1553,  1554,
    1555,   388,  1152,   497,   498,   499,   500,   501,   502,   503,
     504,   505,   506,   507,   508,   509,   510,   511,   512,  1568,
    1569,  1570,   205,  1163,   334,  1164,  1079,  1375,  1381,  1172,
    1173,  1547,  1264,  1176,   389,  1178,  1179,  1656,  1657,  1658,
    1182,  1183,  1184,  1185,  1186,  1187,  1188,   391,  1518,  1198,
     394,  1206,  1207,  1000,  1001,   392,  1211,  1212,   393,  1214,
    1215,  1026,  1027,  1357,  1601,   396,  1411,   397,  1360,  1604,
    1760,  1761,   399,   398,   513,   514,   400,   401,   402,  1235,
     403,   404,  1014,  1015,  1016,  1017,  1018,  1019,  1020,  1021,
    1247,  1248,  1249,  1250,   405,   406,  1252,  1253,  1254,  1255,
     441,   471,   475,   485,   487,  1262,   494,   520,  1033,   537,
     538,   539,   540,   562,   563,   577,   582,   576,   584,   601,
    1578,   879,   605,   608,  1282,  1283,   627,  1285,  1286,  1580,
     648,   656,   663,  1579,   657,   677,  1582,   660,   661,   662,
     715,   664,   613,   613,   613,   613,   671,   497,   498,   499,
     500,   501,   502,   503,   504,   505,   506,   507,   508,   509,
     510,   511,   512,   672,   613,   613,   674,  1516,   675,   613,
     613,   613,   613,   613,   676,   766,   680,   766,   682,  1526,
    1527,  1528,   683,   684,   690,   694,   697,  1347,   754,   699,
     702,   703,  1352,   704,   705,   706,   707,  1726,   755,   708,
    1014,  1015,  1016,  1017,  1018,  1019,  1020,  1021,   709,   710,
     711,   713,   714,   750,  1014,  1015,  1016,  1017,  1018,  1019,
    1031,  1032,   753,  1374,   579,   514,   760,   761,  1014,  1015,
    1016,  1017,  1018,  1019,  1020,  1021,   762,   765,   769,   772,
     773,   774,   775,   776,   777,  1386,   778,   378,   785,   792,
    1398,   786,  1400,   797,   798,   814,  1199,  1405,  1014,  1015,
    1016,  1017,  1018,  1019,  1031,  1032,  1200,   810,   379,   875,
     815,  1818,   816,   817,   380,   818,   834,   381,   382,   849,
    1201,   850,   851,  1417,  1418,   852,   853,   854,   855,   856,
    1517,   383,   857,   858,   867,   859,   860,   861,   870,   871,
     874,  1202,   880,   935,   942,   977,   993,  1034,  1035,  1036,
     384,  1037,  1038,  1039,  1040,  1041,  1054,  1055,  1438,  1056,
    1623,   998,   999,  1000,  1001,  1445,  1063,  1084,  1314,  1022,
    1057,  1023,  1024,  1058,  1680,  1059,  1060,   207,  1061,  1062,
    1064,  1065,   208,   209,  1066,  1087,  1067,  1068,  1083,   766,
    1643,  1079,  1088,  1462,  1089,  1203,  1092,  1090,  1091,  1094,
    1095,   210,  1097,   211,  1099,   212,  1101,  1106,  1108,  1109,
    1110,  1113,   385,   386,  1139,  1123,  1116,  1117,  1204,  1125,
    1131,   213,  1140,  1133,  1134,  1135,  1136,  1213,  1138,  1158,
    1148,  1530,  1531,  1532,  1533,  1149,  1536,  1537,  1538,  1539,
    1162,  1150,  1175,  1216,  1210,  1543,  1151,  1545,  1546,  1217,
    1161,  1218,  1219,  1220,   214,  1552,  1221,  1222,  1223,  1246,
    1557,  1234,  1261,  1244,  1268,  1264,  1269,  1267,  1270,   215,
    1280,  1281,  1271,  1564,  1679,  1292,  1284,  1287,   216,   998,
     999,  1000,  1001,  1291,  1293,  1346,  1349,  1022,  1727,  1023,
    1024,  1351,  1377,  1356,  1007,  1354,   217,   613,  1357,  1378,
    1320,  1022,   218,  1023,  1024,  1690,   613,   998,   999,  1000,
    1001,   219,  1360,  1382,  1383,  1022,  1387,  1023,  1024,  1396,
    1397,  1406,  1408,   220,  1409,  1415,  1416,  1419,  1422,  1424,
    1425,  1427,  1597,  1428,  1429,   221,  1430,  1599,  1007,  1431,
    1432,  1600,  1435,  1439,  1603,  1022,  1436,  1023,  1024,  1437,
    1610,  1611,  1612,  1613,  1614,  1615,  1440,  1441,  1617,  1442,
    1443,  1444,  1446,  1447,  1448,  1449,  1450,  1451,  1452,  1459,
    1464,  1474,  1512,  1453,  1629,  1816,  1630,  1454,  1513,  1514,
    1631,  1632,  1455,  1817,  1456,  1534,  1457,  1458,  1463,  1468,
    1639,  1469,  1641,  1540,  1542,  1549,  1550,  1558,  1551,  1559,
    1560,  1561,  1562,  1572,   222,  1563,  1573,  1574,  1575,  1577,
    1007,  1583,  1584,  1587,  1608,  1606,  1588,  1624,  1589,  1777,
    1590,  1591,  1651,  1592,   223,  1593,   224,  1594,  1595,  1655,
    1616,  1618,  1619,  1633,  1637,  1649,  1644,  1635,  1693,   225,
    1645,  1683,  1706,  1709,  1719,  1725,  1661,  1736,  1646,  1650,
    1652,  1653,  1654,  1663,  1664,  1665,  1666,  1667,   226,   227,
    1668,  1669,  1671,  1672,  1673,  1674,  1675,  1676,  1677,  1678,
    1681,   228,   229,   230,   231,   232,   233,   234,   235,   236,
     237,   497,   498,   499,   500,   501,   502,   503,   504,   505,
     506,   507,   508,   509,   510,   511,   512,  1682,  1691,  1684,
    1716,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,  1734,   252,  1739,   253,  1746,
    1694,  1695,  1696,  1697,  1700,  1701,   254,   255,   256,  1702,
    1707,  1749,  1748,  1750,  1781,  1703,  1708,  1793,  1717,  1751,
    1787,  1710,  1718,  1712,  1713,  1714,  1715,  1723,  1724,  1788,
    1743,  1798,  1745,  1805,  1806,  1807,  1722,  1808,  1752,  1765,
    1851,  1766,  1809,   257,   258,   259,  1773,  1815,  1731,  1732,
    1780,  1824,  1784,  1785,  1836,   260,   261,   262,   263,   264,
    1825,  1786,  1789,  1799,  1742,  1814,  1839,  1744,   265,  1842,
    1843,   266,  1845,  1853,  1747,  1875,  1848,  1849,  1856,  1858,
     267,   268,   269,   270,   271,  1755,  1861,  1864,  1759,  1865,
    1763,  1764,  1878,  1874,  1777,  1887,  1876,  1768,  1755,  1770,
    1771,  1772,  1877,  1889,  1894,  1778,  1898,  1904,  1895,  1907,
    1909,  1912,  1918,  1783,  1914,  1915,  -790,  1929,  -791,  1938,
     944,  1942,   945,   946,   947,   948,   949,  1936,  1927,  1946,
    1953,  1954,   950,   716,   717,   718,   719,   720,   721,   722,
     723,   724,   725,   726,   727,   728,   729,   730,   731,   732,
     733,   734,   735,   736,   737,   738,   739,   740,   741,   742,
     743,   744,   745,   746,   747,  1948,  1949,  1955,  1956,  1957,
    1800,  1801,  1802,  1803,  1958,  1962,  1963,  1969,  1973,  1974,
    1810,  1976,  1988,  1991,  1979,  1993,  1998,  2002,  2036,  2019,
    2022,  2037,   613,  2040,   613,  1819,   951,   952,   953,  2042,
    1820,   954,   955,  1822,  2045,  2055,  2058,  2049,  2063,  2057,
    2060,  2061,  1828,  1829,  1830,  1831,  2073,  2062,  2067,  2068,
    2072,  2078,  2092,  2095,  2096,  2097,  2099,  1899,   483,  1096,
    1835,  1905,  1602,   956,  1891,  1997,  1605,  2018,   523,   957,
     782,  1078,  1157,  1376,   791,  1302,  1412,  1303,  1519,  1660,
    1245,  1520,  2004,  1910,  1854,  1855,  1548,  1857,  1122,  1863,
    1711,   371,  1373,  1051,  1586,  1823,  1046,  1866,  1867,  1869,
    1870,  1871,  1872,  1873,  1160,  1821,     0,     0,     0,     0,
       0,     0,  1128,     0,     0,     0,     0,     0,   958,   959,
     960,     0,  1884,     0,     0,     0,     0,     0,     0,     0,
    1893,     0,     0,  1897,     0,     0,  1900,  1901,     0,  1903,
     961,   962,     0,   963,     0,     0,     0,  1911,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1916,  1917,     0,  1919,     0,     0,     0,     0,     0,  1922,
       0,     0,     0,     0,  1925,  1926,     0,     0,     0,     0,
       0,  1931,     0,  1935,     0,     0,     0,     0,     0,  1937,
       0,     0,   964,   965,     0,     0,  1755,   966,   967,   968,
     969,   970,   971,  1945,     0,     0,  1947,     0,     0,     0,
       0,  1950,  1951,  1952,     0,     0,     0,     0,     0,     0,
    1959,     0,     0,  1960,     0,     0,  1961,     0,     0,     0,
    1965,     0,  1966,     0,     0,     0,  1972,     0,     0,     0,
    1975,     0,     0,     0,  1977,  1978,     0,     0,     0,     0,
    1983,  1985,  1987,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1755,  1996,     0,     0,     0,     0,     0,  1999,
    2000,  2001,     0,     0,     0,     0,     0,     0,  2007,     0,
    2009,  2010,  2011,     0,     0,     0,  1755,  2016,     0,     0,
       0,     0,     0,     0,  2020,  2021,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1755,     0,     0,     0,
       0,     0,  2039,     0,     0,  2041,     0,     0,     0,  2043,
       0,     0,  2046,  2047,  2048,     0,     0,     0,     0,     0,
    2052,  2053,     0,     0,     0,     0,  2056,     0,     0,  2059,
       0,     0,     0,   881,     0,     0,     0,     0,     0,  2066,
       0,     0,     0,     0,     0,     0,   882,  2069,     0,  1755,
    2071,     0,   883,     0,  1755,     0,   884,  2074,  2075,  2076,
    2077,     0,     0,   885,   886,  2086,  2087,   276,  2089,     0,
    2090,  2091,     0,  2093,  2094,     0,   887,     0,  2098,     0,
       0,     0,   888,     0,   889,   890,     0,     0,     0,   891,
     892,     0,     0,     0,     0,   893,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   894,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   895,     0,   896,     0,   897,     0,   898,
       0,     0,     0,     0,     0,   899,   900,     0,     0,   901,
     902,     0,     0,     0,     0,   903,   904,     0,     0,     0,
       0,     0,     0,     0,     0,    -8,     1,     0,     0,     0,
       0,     0,     0,    -8,     0,   905,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   906,     0,     0,
     907,     0,     0,   908,     0,     0,     0,    -8,     0,     0,
       0,     0,    -8,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    -8,     0,     0,    -8,
       0,     0,     0,    -8,     0,     0,     0,     0,     0,     0,
      -8,     0,    -8,     0,     0,     0,    -8,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   909,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   910,
       0,     0,     0,     0,     0,     0,     0,     0,   911,     0,
     912,   913,   914,   915,   916,    -8,     0,     0,     0,    -8,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    -8,     0,     0,     0,     0,    -8,    -8,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   917,   918,   919,   920,   921,   922,
     923,   924,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    -8,    -8,    -8,     5,   925,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    -8,     0,     0,     0,
       6,     0,     0,    -8,    -8,    -8,     0,    -8,    -8,    -8,
      -8,    -8,     0,    -8,     0,     0,     0,     0,    -8,    -8,
      -8,     0,     0,     0,     0,    -8,     0,     0,     0,     7,
       0,     0,     8,     0,     0,    -8,     9,    -8,    -8,     0,
       0,     0,   926,    10,     0,    11,     0,     0,     0,    12,
     927,     0,     0,     0,     0,     0,   928,     0,     0,     0,
       0,     0,     0,     0,     0,    -8,    -8,    -8,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    -8,
      -8,     0,    -8,     0,     0,    -8,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    13,     0,
       0,     0,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      -8,     0,     0,     0,     0,    15,     0,     0,     0,     0,
      16,    17,  1479,  1480,  1481,  1482,  1483,  1484,  1485,  1486,
    1487,  1488,  1489,  1490,  1491,  1492,  1493,  1494,  1495,  1496,
    1497,  1498,  1499,  1500,  1501,  1502,  1503,  1504,  1505,  1506,
    1507,  1508,  1509,  1510,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    18,    19,    20,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    21,
       0,     0,     0,     0,     0,     0,    22,    23,     0,     0,
      24,    25,    26,    27,    28,     0,    29,     0,     0,     0,
       0,    30,    31,    32,     0,     0,     0,     0,    33,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    34,     0,
      35,    36,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    37,    38,
      39,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    40,    41,     0,    42,     0,     0,    43,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    44
};

static const yytype_int16 yycheck[] =
{
       6,   610,   626,   335,    10,   598,   599,   616,  1399,  1237,
     619,   604,   151,   622,   623,   145,   936,   937,    24,    25,
      26,    27,    28,    29,    30,    31,    32,   525,    21,  1239,
     618,    34,   375,    29,    40,  1620,     9,   625,    44,    16,
     376,    62,    62,    62,   136,   154,    62,    46,   138,    64,
     168,   169,    14,   125,   171,   170,   392,   393,   646,   647,
      37,   140,    33,    37,   182,   172,    55,    56,    28,    86,
      54,    88,   172,    35,   170,    37,   287,   590,   591,   592,
     167,    98,   171,   172,   172,    10,    21,   170,   168,   169,
     172,    64,    22,   172,   211,   101,   170,    22,    23,    24,
      89,    72,   182,   235,    37,   213,    20,   113,   170,   172,
      37,  1571,   323,    38,   172,   162,   170,   106,   170,   134,
      91,   236,   211,   170,   335,   214,   215,   172,    16,   170,
      37,   167,    92,   170,   133,   224,   172,   172,   109,   110,
     236,     0,   172,   170,   133,   144,   145,   162,   110,    37,
     172,   172,   172,   171,   131,   170,   172,    64,   172,    37,
      50,   460,   122,   134,   141,   464,    96,   172,   172,   172,
     167,    96,   134,    54,    64,   456,   160,   172,   357,   460,
     172,   172,   172,   464,   172,   517,   103,   112,  1690,   236,
       6,   172,   109,   172,   257,   236,   167,   172,    37,   257,
     213,   171,    37,   209,   172,   211,   172,   213,   214,   215,
     216,   217,   218,   219,   172,   221,   211,   223,   224,   225,
     134,   236,   228,   172,   230,   172,   232,   233,   172,   235,
     190,   237,   238,   239,   240,   241,   242,   243,   244,   245,
     246,   247,   248,   249,   250,   251,   172,   426,   254,   581,
     256,   257,   258,   172,   260,   261,   262,   263,   264,   172,
     296,  1721,   235,   269,   142,   271,   240,   354,   611,   612,
     203,   482,   278,    10,   485,   788,   789,   237,   212,   160,
     623,   617,   216,   217,    59,   216,   217,   623,   222,   223,
     125,   222,   223,  1005,  1522,   111,  1798,  1009,   796,   296,
     235,   213,   308,    64,   216,   217,   172,   237,    84,   271,
     222,   223,   237,    50,  1524,  1525,  1236,   316,  1238,   325,
     531,   327,   249,   250,   100,   415,   299,   417,   460,   401,
      37,   213,   422,   460,   216,   217,   218,   219,   220,   221,
     222,   223,   460,   172,   461,   460,   456,   455,   456,   457,
     458,   460,   462,   443,   461,   462,   464,  1859,   197,   198,
     366,   461,   462,   298,   460,   172,   455,   356,   460,   375,
     460,   377,   461,   461,   347,   170,   448,   460,   460,   350,
     460,   460,   461,   292,   293,   294,   460,   170,   407,  1013,
     396,   397,   398,   353,   400,   401,   402,   403,   460,   324,
     406,   394,   172,   461,   419,  1990,   460,   424,   460,   170,
     416,   417,   418,   428,   429,   460,   412,  1026,  1027,  1028,
    1029,   405,   406,   460,   349,   460,   939,   433,   257,  2014,
     460,   430,  1642,   460,   440,   441,    39,   443,   460,   445,
     461,   461,   448,   456,   450,   461,   460,   586,   425,   411,
     225,   464,   189,  1844,   229,   460,   462,  1045,   461,   452,
     455,  1689,  1050,   469,   437,   438,   461,   170,   460,   461,
     460,   444,   460,   461,   480,   236,    29,   483,  1688,   460,
     461,   460,   461,   170,    48,   460,    16,   257,    18,   124,
    1410,    48,   460,   461,   460,   216,   217,   216,   217,   460,
      64,   222,   460,   222,   223,   460,    36,    64,   514,   515,
     113,   460,    56,   460,     5,   170,   460,    64,    48,   460,
      50,    51,   170,   529,   530,   170,   532,   533,   534,   535,
     464,   462,   170,   236,   460,   541,    66,   172,   232,   233,
     234,   460,   249,   250,   170,    89,   110,   460,   170,  1940,
      97,   170,   464,   110,    84,    46,   103,    60,    88,    37,
     771,   445,   109,  1221,   448,  1223,   450,  1795,   460,  1779,
     100,    11,    12,  1964,   456,   269,   106,   170,   584,    82,
      83,   463,   464,   465,   466,   460,    64,   593,   460,   153,
     460,    14,   460,   137,   805,   139,   153,   460,  1989,   163,
     130,   131,   132,    26,   610,   611,   612,    30,    93,    94,
     616,   141,   460,   619,    37,   162,   622,   623,   148,    97,
     167,   112,  1850,    93,    94,   103,   460,   216,   217,   460,
     636,   109,  1130,   222,   223,   641,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   318,   319,   998,   999,  1000,  1001,    93,
      94,   791,   460,  2054,   460,   105,  1579,   107,  1581,  1582,
     170,  1007,   236,  2064,   170,   681,   460,  1020,  1021,   236,
     460,   687,  1025,   106,   162,   130,   460,   110,   460,   167,
      46,   455,   456,   457,   458,  1031,  1032,  1033,   462,    55,
      56,  1294,  1295,   170,   171,   172,   236,   399,   400,   715,
     460,   151,   152,  1322,   455,   456,   457,   458,   929,   460,
     170,   171,   172,   464,   147,   936,   937,   938,    64,   940,
     281,   282,   283,    89,   267,   268,   266,   455,   456,   457,
     458,   460,   165,   749,    67,   464,   168,   169,   754,   755,
     460,   757,   197,   198,   199,   200,   201,   202,   170,   171,
     172,   463,   768,   208,   209,   210,  1212,  1213,   249,   250,
    1216,  1217,  1218,  1219,   249,   250,  1222,   783,  1035,  1036,
    1037,  1038,  1039,  1040,  1041,   315,   172,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   809,   810,   463,   812,   170,   171,   172,
     168,   169,   818,  1145,   173,   174,   175,   176,   177,   178,
     179,   180,   181,   182,   183,   184,   185,   186,   187,   188,
     463,  1163,   173,   174,   175,   176,   177,   178,   179,   180,
     181,   182,   183,   184,   185,   186,   187,   188,   281,   282,
     283,   460,   858,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   170,
     171,   172,   295,   879,   404,   881,   461,   462,  1089,   885,
     886,   460,   461,   889,   460,   891,   892,   170,   171,   172,
     896,   897,   898,   899,   900,   901,   902,   460,  1230,   905,
     460,   907,   908,   457,   458,   463,   912,   913,   463,   915,
     916,   216,   217,   461,   462,    99,  1127,    73,   461,   462,
     351,   352,   157,   418,   460,   461,    74,    85,    78,   935,
     166,   297,   216,   217,   218,   219,   220,   221,   222,   223,
     946,   947,   948,   949,   172,    25,   952,   953,   954,   955,
     292,   172,   446,   461,   460,   961,    37,   170,  1582,   134,
     170,    61,   170,   170,   170,    64,    37,   460,   170,   170,
    1313,   977,    37,   414,   980,   981,   231,   983,   984,  1322,
     170,   172,   170,  1319,   460,   170,  1322,   460,   460,   460,
     167,   460,   998,   999,  1000,  1001,   460,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   460,  1020,  1021,   460,  1228,   460,  1025,
    1026,  1027,  1028,  1029,   460,  1236,   461,  1238,   460,  1240,
    1241,  1242,   460,   460,   460,   460,   460,  1043,   354,   460,
     460,   460,  1048,   460,   460,   460,   460,   213,   398,   460,
     216,   217,   218,   219,   220,   221,   222,   223,   460,   460,
     460,   460,   460,   460,   216,   217,   218,   219,   220,   221,
     222,   223,   460,  1079,   460,   461,   460,   460,   216,   217,
     218,   219,   220,   221,   222,   223,   460,   460,   442,   460,
     460,   170,   170,   460,   170,  1101,   170,    37,   460,   460,
    1106,   460,  1108,   170,    37,   460,     7,  1113,   216,   217,
     218,   219,   220,   221,   222,   223,    17,   170,    58,   460,
     460,  1730,   460,   170,    64,    13,   349,    67,    68,   460,
      31,   170,   170,  1139,  1140,   170,   460,   170,   460,   170,
     460,    81,   460,    13,   170,   460,   460,   460,   460,   460,
     460,    52,   460,   407,    37,   170,   460,   170,   170,   170,
     100,   170,   170,   170,   170,   170,   170,   460,  1174,   460,
    1381,   455,   456,   457,   458,  1181,   461,   295,   462,   463,
     460,   465,   466,   460,  1516,   460,   460,     9,   460,   460,
     460,   460,    14,    15,   460,   172,   461,   460,   460,  1410,
    1411,   461,   460,  1209,   461,   106,   313,   460,   460,   460,
     460,    33,   460,    35,   460,    37,   167,   167,   167,   460,
     460,   167,   162,   163,    13,   170,   460,   460,   129,   460,
     460,    53,   149,   460,   460,   460,   460,   172,   460,   170,
     460,  1247,  1248,  1249,  1250,   460,  1252,  1253,  1254,  1255,
     170,   460,   170,   172,   170,  1261,   460,  1263,  1264,   172,
     460,   172,   172,   172,    86,  1271,   172,   172,   172,   148,
    1276,   170,   206,   460,   171,   461,   171,   460,   171,   101,
     460,   460,   226,  1289,   460,   170,   460,   460,   110,   455,
     456,   457,   458,   460,   170,   460,   244,   463,   464,   465,
     466,   460,   462,   460,   456,   248,   128,  1313,   461,   172,
     462,   463,   134,   465,   466,  1526,  1322,   455,   456,   457,
     458,   143,   461,   460,   460,   463,   460,   465,   466,   167,
     134,   403,   402,   155,   462,   170,   460,   460,   460,   460,
     460,   460,  1348,   460,   460,   167,   460,  1353,   456,   460,
     460,  1357,   170,   170,  1360,   463,   460,   465,   466,   460,
    1366,  1367,  1368,  1369,  1370,  1371,   460,   170,  1374,   460,
     460,   170,   460,   460,   460,   460,   460,   460,   460,   170,
      64,    64,   170,   460,  1390,  1728,  1392,   460,   171,   170,
    1396,  1397,   460,  1729,   460,   170,   460,   460,   460,   460,
    1406,   460,  1408,   170,   460,   460,   460,   156,   460,    34,
      34,    34,    34,   170,   236,   202,   170,   170,    37,   212,
     456,   171,   235,   170,   172,   449,   460,   398,   460,  1640,
     460,   460,  1438,   460,   256,   460,   258,   460,   460,  1445,
     460,   460,   460,   398,   167,    34,   460,   450,   170,   271,
     460,    37,   207,   226,   170,   170,  1462,   251,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   460,   290,   291,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     460,   303,   304,   305,   306,   307,   308,   309,   310,   311,
     312,   173,   174,   175,   176,   177,   178,   179,   180,   181,
     182,   183,   184,   185,   186,   187,   188,   460,   460,   441,
     204,   333,   334,   335,   336,   337,   338,   339,   340,   341,
     342,   343,   344,   345,   346,   172,   348,   251,   350,   170,
     460,   460,   460,   460,   460,   460,   358,   359,   360,   460,
    1546,   398,   235,   398,   109,   460,  1552,    34,   460,   445,
     170,  1557,   460,  1559,  1560,  1561,  1562,   460,   460,   170,
     460,   170,   460,   226,   138,   138,  1572,   138,   462,   460,
    1781,   460,   138,   395,   396,   397,   460,   213,  1584,  1585,
     460,   462,   460,   460,   354,   407,   408,   409,   410,   411,
     462,   460,   460,   460,  1600,   460,   398,  1603,   420,   134,
     167,   423,   431,    13,  1610,   167,   460,   460,   460,   460,
     432,   433,   434,   435,   436,  1621,   460,   462,  1624,   460,
    1626,  1627,   136,   460,  1835,   444,   460,  1633,  1634,  1635,
    1636,  1637,   460,   228,   460,  1641,   460,   460,   439,   416,
    1851,    13,   159,  1649,   460,   460,   460,   447,   460,   134,
      37,   134,    39,    40,    41,    42,    43,   398,   440,   138,
      64,    64,    49,   362,   363,   364,   365,   366,   367,   368,
     369,   370,   371,   372,   373,   374,   375,   376,   377,   378,
     379,   380,   381,   382,   383,   384,   385,   386,   387,   388,
     389,   390,   391,   392,   393,   460,   460,   460,   252,   247,
    1706,  1707,  1708,  1709,   235,   172,   228,   167,   170,   460,
    1716,   138,   398,   172,   460,   167,   170,   460,   398,   170,
     170,   167,  1728,   460,  1730,  1731,   113,   114,   115,    25,
    1736,   118,   119,  1739,   124,   403,   170,   451,   134,   460,
     460,   460,  1748,  1749,  1750,  1751,   350,   460,   460,   460,
     460,   460,   170,   460,   460,   460,   460,  1841,   266,   701,
    1758,  1847,  1358,   150,  1834,  1971,  1361,  1996,   294,   156,
     496,   678,   868,  1080,   519,   994,  1128,   994,  1231,  1460,
     943,  1233,  1981,  1852,  1790,  1791,  1265,  1793,   779,  1800,
    1558,   152,  1073,   643,  1336,  1740,   638,  1803,  1804,  1805,
    1806,  1807,  1808,  1809,   873,  1737,    -1,    -1,    -1,    -1,
      -1,    -1,   791,    -1,    -1,    -1,    -1,    -1,   205,   206,
     207,    -1,  1828,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1836,    -1,    -1,  1839,    -1,    -1,  1842,  1843,    -1,  1845,
     227,   228,    -1,   230,    -1,    -1,    -1,  1853,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1866,  1867,    -1,  1869,    -1,    -1,    -1,    -1,    -1,  1875,
      -1,    -1,    -1,    -1,  1880,  1881,    -1,    -1,    -1,    -1,
      -1,  1887,    -1,  1889,    -1,    -1,    -1,    -1,    -1,  1895,
      -1,    -1,   279,   280,    -1,    -1,  1902,   284,   285,   286,
     287,   288,   289,  1909,    -1,    -1,  1912,    -1,    -1,    -1,
      -1,  1917,  1918,  1919,    -1,    -1,    -1,    -1,    -1,    -1,
    1926,    -1,    -1,  1929,    -1,    -1,  1932,    -1,    -1,    -1,
    1936,    -1,  1938,    -1,    -1,    -1,  1942,    -1,    -1,    -1,
    1946,    -1,    -1,    -1,  1950,  1951,    -1,    -1,    -1,    -1,
    1956,  1957,  1958,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1968,  1969,    -1,    -1,    -1,    -1,    -1,  1975,
    1976,  1977,    -1,    -1,    -1,    -1,    -1,    -1,  1984,    -1,
    1986,  1987,  1988,    -1,    -1,    -1,  1992,  1993,    -1,    -1,
      -1,    -1,    -1,    -1,  2000,  2001,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  2012,    -1,    -1,    -1,
      -1,    -1,  2018,    -1,    -1,  2021,    -1,    -1,    -1,  2025,
      -1,    -1,  2028,  2029,  2030,    -1,    -1,    -1,    -1,    -1,
    2036,  2037,    -1,    -1,    -1,    -1,  2042,    -1,    -1,  2045,
      -1,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,  2055,
      -1,    -1,    -1,    -1,    -1,    -1,    27,  2063,    -1,  2065,
    2066,    -1,    33,    -1,  2070,    -1,    37,  2073,  2074,  2075,
    2076,    -1,    -1,    44,    45,  2081,  2082,    48,  2084,    -1,
    2086,  2087,    -1,  2089,  2090,    -1,    57,    -1,  2094,    -1,
      -1,    -1,    63,    -1,    65,    66,    -1,    -1,    -1,    70,
      71,    -1,    -1,    -1,    -1,    76,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    90,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   104,    -1,   106,    -1,   108,    -1,   110,
      -1,    -1,    -1,    -1,    -1,   116,   117,    -1,    -1,   120,
     121,    -1,    -1,    -1,    -1,   126,   127,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     0,     1,    -1,    -1,    -1,
      -1,    -1,    -1,     8,    -1,   146,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   158,    -1,    -1,
     161,    -1,    -1,   164,    -1,    -1,    -1,    32,    -1,    -1,
      -1,    -1,    37,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    61,    -1,    -1,    64,
      -1,    -1,    -1,    68,    -1,    -1,    -1,    -1,    -1,    -1,
      75,    -1,    77,    -1,    -1,    -1,    81,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   236,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   250,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   259,    -1,
     261,   262,   263,   264,   265,   130,    -1,    -1,    -1,   134,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   157,    -1,    -1,    -1,    -1,   162,   163,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   325,   326,   327,   328,   329,   330,
     331,   332,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   208,   209,   210,     8,   348,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   231,    -1,    -1,    -1,
      32,    -1,    -1,   238,   239,   240,    -1,   242,   243,   244,
     245,   246,    -1,   248,    -1,    -1,    -1,    -1,   253,   254,
     255,    -1,    -1,    -1,    -1,   260,    -1,    -1,    -1,    61,
      -1,    -1,    64,    -1,    -1,   270,    68,   272,   273,    -1,
      -1,    -1,   413,    75,    -1,    77,    -1,    -1,    -1,    81,
     421,    -1,    -1,    -1,    -1,    -1,   427,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   300,   301,   302,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   314,
     315,    -1,   317,    -1,    -1,   320,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   130,    -1,
      -1,    -1,   134,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     355,    -1,    -1,    -1,    -1,   157,    -1,    -1,    -1,    -1,
     162,   163,   362,   363,   364,   365,   366,   367,   368,   369,
     370,   371,   372,   373,   374,   375,   376,   377,   378,   379,
     380,   381,   382,   383,   384,   385,   386,   387,   388,   389,
     390,   391,   392,   393,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   208,   209,   210,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   231,
      -1,    -1,    -1,    -1,    -1,    -1,   238,   239,    -1,    -1,
     242,   243,   244,   245,   246,    -1,   248,    -1,    -1,    -1,
      -1,   253,   254,   255,    -1,    -1,    -1,    -1,   260,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   270,    -1,
     272,   273,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   300,   301,
     302,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   314,   315,    -1,   317,    -1,    -1,   320,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   355
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,     1,   468,   474,     0,     8,    32,    61,    64,    68,
      75,    77,    81,   130,   134,   157,   162,   163,   208,   209,
     210,   231,   238,   239,   242,   243,   244,   245,   246,   248,
     253,   254,   255,   260,   270,   272,   273,   300,   301,   302,
     314,   315,   317,   320,   355,   469,   472,   473,   476,   477,
     478,   479,   480,   481,   482,   486,   487,   490,   491,   586,
     588,   590,   591,   618,   620,   621,   639,   640,   646,   647,
     654,   655,   656,   677,   678,   694,   696,   809,   811,   829,
     831,   833,   844,   880,   881,   882,   883,   884,   896,   908,
     909,   910,   911,   912,   913,   914,   812,   172,   471,   492,
     697,   471,    93,    94,   657,   679,   619,   834,   835,   836,
     845,   171,   470,   471,   471,   471,   471,   471,   471,   471,
     471,   471,   171,    93,    94,   830,   832,   172,   172,   172,
     471,   460,   318,   319,   483,    84,   100,   484,   471,   488,
     495,   170,   589,    50,    64,   624,   630,   633,   642,   649,
     682,   700,   815,    37,   240,   475,   915,   170,   460,   170,
     170,   471,   460,   460,   170,   170,   170,   170,   170,   170,
     846,   460,   170,   471,   460,   460,   460,   460,   885,   460,
     460,   460,   460,   460,   460,   170,   170,   460,   460,   460,
     460,   460,    93,    94,   485,   235,   460,    14,    26,    30,
      37,   106,   110,   147,   165,   295,   489,     9,    14,    15,
      33,    35,    37,    53,    86,   101,   110,   128,   134,   143,
     155,   167,   236,   256,   258,   271,   290,   291,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   333,   334,
     335,   336,   337,   338,   339,   340,   341,   342,   343,   344,
     345,   346,   348,   350,   358,   359,   360,   395,   396,   397,
     407,   408,   409,   410,   411,   420,   423,   432,   433,   434,
     435,   436,   493,   496,    29,   412,    48,    64,   110,   153,
     163,   236,   592,   599,   600,   602,   606,   607,   610,   611,
      29,   623,   634,   630,   631,   635,    37,   125,   641,   643,
     644,    37,   142,   648,   650,   653,    16,    37,   131,   141,
     425,   680,   683,   684,   685,   688,    16,    18,    36,    50,
      51,    66,    84,    88,   100,   106,   130,   131,   132,   141,
     148,   236,   266,   315,   404,   607,   695,   701,   704,   708,
     713,   714,   715,   716,   717,   718,   719,   720,   722,   724,
     725,   727,   728,   729,   777,   778,   779,   786,   788,   789,
     130,   197,   198,   199,   200,   201,   202,   726,   810,   816,
     822,   833,    67,   460,   658,   463,   463,   463,    37,    58,
      64,    67,    68,    81,   100,   162,   163,   847,   460,   460,
     897,   460,   463,   463,   460,   587,    99,    73,   418,   157,
      74,    85,    78,   166,   297,   172,    25,   471,    54,   160,
     405,   406,   574,   471,   494,   471,   471,   471,   124,   471,
     471,   471,   471,    28,    92,   122,   190,   237,   353,   573,
     471,   501,   257,   461,   471,   471,   471,   292,   293,   294,
     553,   292,   471,   507,   471,   508,   471,   471,   510,   471,
     511,   471,   471,   471,   471,   471,   471,   471,   471,   471,
     471,   471,   471,   471,   471,   471,   513,   399,   400,   537,
     471,   172,   471,   471,   471,   446,   519,   471,   471,   471,
     471,   471,   522,   537,   497,   461,   471,   460,   471,   608,
     612,   471,   593,   603,    37,   616,   601,   173,   174,   175,
     176,   177,   178,   179,   180,   181,   182,   183,   184,   185,
     186,   187,   188,   460,   461,   471,   609,   693,   613,   622,
     170,   170,   236,   590,   625,   626,   627,   632,    33,    72,
      91,   109,   110,   134,   167,   350,   636,   134,   170,    61,
     170,   651,    22,    96,   237,   681,   471,   686,   689,    10,
      22,    23,    24,    38,    96,   112,   237,   324,   349,   709,
     787,   721,   170,   170,   723,   693,   730,   471,   471,    10,
      50,   189,   705,   702,   267,   268,   460,    64,   781,   460,
     609,   693,    37,   698,   170,   768,   733,    37,    64,    97,
     103,   109,   162,   167,   754,   755,   764,   791,   818,   819,
     820,   170,   821,   471,   817,    37,   813,   823,   414,   661,
     211,   455,   461,   471,   840,   171,   211,   461,   842,   211,
     214,   215,   224,   461,   840,   841,   842,   231,   849,   854,
     848,   851,   855,   850,   852,   853,   249,   250,   899,   900,
     901,   249,   250,   887,   888,   889,   842,   842,   170,   471,
     471,   471,   471,   471,   471,   471,   172,   460,   257,   471,
     460,   460,   460,   170,   460,   460,   471,   460,   471,   257,
     471,   460,   460,   499,   460,   460,   460,   170,   565,   566,
     461,   471,   460,   460,   460,   471,   502,   167,   296,   471,
     460,   257,   471,   548,   460,   548,   509,   460,   548,   460,
     548,   512,   460,   460,   460,   460,   460,   460,   460,   460,
     460,   460,   471,   460,   460,   167,   362,   363,   364,   365,
     366,   367,   368,   369,   370,   371,   372,   373,   374,   375,
     376,   377,   378,   379,   380,   381,   382,   383,   384,   385,
     386,   387,   388,   389,   390,   391,   392,   393,   576,   471,
     460,   516,   518,   460,   354,   398,    21,   394,   452,   527,
     460,   460,   460,   460,   471,   460,   693,   761,   471,   442,
     523,   693,   460,   460,   170,   170,   460,   170,   170,   604,
     605,   617,   602,   471,   471,   460,   460,   609,   103,   109,
     614,   624,   460,   170,   171,   172,   628,   170,    37,   637,
     627,    54,   160,   471,   471,   693,   471,   471,   471,   471,
     170,   460,   471,   652,   460,   460,   460,   170,    13,   168,
     169,   182,   460,   687,   690,   357,   426,    46,   133,   144,
     145,   316,   430,   711,   349,    11,    12,   105,   107,   151,
     152,   712,    55,    56,    89,   106,   133,   356,   710,   460,
     170,   170,   170,   460,   170,   460,   170,   460,    13,   460,
     460,   460,   168,   169,   182,   460,   706,   170,   703,   707,
     460,   460,   782,   780,   460,   460,   609,   699,   460,   471,
     460,    14,    27,    33,    37,    44,    45,    57,    63,    65,
      66,    70,    71,    76,    90,   104,   106,   108,   110,   116,
     117,   120,   121,   126,   127,   146,   158,   161,   164,   236,
     250,   259,   261,   262,   263,   264,   265,   325,   326,   327,
     328,   329,   330,   331,   332,   348,   413,   421,   427,   607,
     731,   734,   748,   749,   756,   407,   645,   645,   645,    62,
     645,   471,    37,   758,    37,    39,    40,    41,    42,    43,
      49,   113,   114,   115,   118,   119,   150,   156,   205,   206,
     207,   227,   228,   230,   279,   280,   284,   285,   286,   287,
     288,   289,   790,   792,   796,   797,   805,   170,   768,   768,
     168,   169,   769,   168,   169,   776,   827,   768,   814,    37,
     197,   198,   824,   460,   662,   841,   840,   840,   455,   456,
     457,   458,   460,   464,   837,   841,   842,   456,   837,   841,
     841,   840,   841,   842,   216,   217,   218,   219,   220,   221,
     222,   223,   463,   465,   466,   843,   216,   217,   222,   223,
     837,   222,   223,   843,   170,   170,   170,   170,   170,   170,
     170,   170,   471,   902,    37,   898,   900,   471,   890,    37,
     886,   888,   837,   837,   170,   460,   460,   460,   460,   460,
     460,   460,   460,   461,   460,   460,   460,   461,   460,   125,
     401,   448,   865,   866,   170,   171,   172,   460,   566,   461,
     567,   568,   471,   460,   295,   554,   471,   172,   460,   461,
     460,   460,   313,   552,   460,   460,   552,   460,   471,   460,
     471,   167,   575,   545,   471,   471,   167,   471,   167,   460,
     460,   763,   471,   167,   524,   693,   460,   460,   170,   171,
     172,   460,   605,   170,   471,   460,   645,   645,   625,   627,
     629,   460,   638,   460,   460,   460,   460,   693,   460,    13,
     149,   471,   471,   471,   471,   170,   460,   691,   460,   460,
     460,   460,   471,   170,   171,   172,   460,   707,   170,    37,
     781,   460,   170,   471,   471,     6,   111,    46,    55,    56,
      89,   732,   471,   471,   739,   170,   471,   735,   471,   471,
     736,   737,   471,   471,   471,   471,   471,   471,   471,    56,
      89,   137,   139,   752,     5,    46,   112,   753,   471,     7,
      17,    31,    52,   106,   129,   751,   471,   471,   740,   738,
     170,   471,   471,   172,   471,   471,   172,   172,   172,   172,
     172,   172,   172,   172,   741,   744,   742,   743,   140,   460,
     693,    16,    37,   750,   170,   471,    62,   761,    62,   761,
      62,   693,   645,   693,   460,   755,   148,   471,   471,   471,
     471,   793,   471,   471,   471,   471,   794,    60,    82,    83,
     806,   206,   471,   803,   461,   801,   802,   460,   171,   171,
     171,   226,   281,   282,   283,   798,    59,   225,   229,   800,
     460,   460,   471,   471,   460,   471,   471,   460,    37,   203,
     828,   460,   170,   170,   825,   826,    64,   170,   236,   419,
     428,   429,   588,   639,   659,   663,   664,   665,   666,   667,
     670,   212,   464,   838,   462,   840,   840,   840,   840,   838,
     462,   842,   838,   462,   840,   840,   840,   841,   841,   841,
     841,   842,   842,   842,   232,   233,   234,   269,   856,   856,
     856,   856,   856,   856,   856,   856,   460,   471,   903,   244,
     837,   460,   471,   891,   248,   837,   460,   461,   569,   570,
     461,   571,   572,   867,     9,    64,   235,   299,   347,   437,
     438,   444,   868,   865,   471,   462,   568,   462,   172,   505,
     503,   693,   460,   460,   517,   520,   471,   460,   138,   415,
     417,   422,   443,   460,   546,   547,   167,   134,   471,   528,
     471,   460,   693,   762,   521,   471,   403,   514,   402,   462,
     615,   693,   637,   460,   627,   170,   460,   471,   471,   460,
     136,   460,   460,   609,   460,   460,   609,   460,   460,   460,
     460,   460,   460,   154,   460,   170,   460,   460,   471,   170,
     460,   170,   460,   460,   170,   471,   460,   460,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   460,   460,   170,
     746,   747,   471,   460,    64,   876,   876,   876,   460,   460,
     876,   876,   876,   876,    64,   878,   878,   876,   878,   362,
     363,   364,   365,   366,   367,   368,   369,   370,   371,   372,
     373,   374,   375,   376,   377,   378,   379,   380,   381,   382,
     383,   384,   385,   386,   387,   388,   389,   390,   391,   392,
     393,   745,   170,   171,   170,   460,   693,   460,   609,   709,
     754,   757,   761,   763,   761,   762,   693,   693,   693,   765,
     471,   471,   471,   471,   170,   807,   471,   471,   471,   471,
     170,   808,   460,   471,   460,   471,   471,   460,   802,   460,
     460,   460,   471,   281,   282,   283,   799,   471,   156,    34,
      34,    34,    34,   202,   471,   768,   768,   671,   170,   171,
     172,   668,   170,   170,   170,    37,   660,   212,   840,   842,
     840,   841,   842,   171,   235,   862,   862,   170,   460,   460,
     460,   460,   460,   460,   460,   460,   460,   471,   460,   471,
     471,   462,   570,   471,   462,   572,   449,   861,   172,   869,
     471,   471,   471,   471,   471,   471,   460,   471,   460,   460,
     555,   563,   564,   693,   398,   542,   167,   354,   538,   471,
     471,   471,   471,   398,   563,   450,   583,   167,   541,   471,
     550,   471,   761,   693,   460,   460,   460,   692,   783,    34,
     460,   471,   460,   460,   460,   471,   170,   171,   172,   460,
     747,   471,   877,   460,   460,   460,   460,   460,   460,   460,
     879,   460,   460,   460,   460,   460,   460,   460,   460,   460,
     609,   460,   460,    37,   441,   759,   763,   460,   762,   762,
     693,   460,   766,   170,   460,   460,   460,   460,   170,   460,
     460,   460,   460,   460,   170,   460,   207,   471,   471,   226,
     471,   806,   471,   471,   471,   471,   204,   460,   460,   170,
     667,   669,   471,   460,   460,   170,   213,   464,   839,   839,
     839,   471,   471,   864,   172,   863,   251,   904,   905,   251,
     892,   893,   471,   460,   471,   460,   170,   471,   235,   398,
     398,   445,   462,   564,   460,   471,   172,   460,   549,   471,
     351,   352,   543,   471,   471,   460,   460,   500,   471,   529,
     471,   471,   471,   460,   498,   462,   551,   693,   471,   762,
     460,   109,   785,   471,   460,   460,   460,   170,   170,   460,
      20,   134,   760,    34,   767,   762,   763,   767,   170,   460,
     471,   471,   471,   471,   795,   226,   138,   138,   138,   138,
     471,   672,   460,   667,   460,   213,   840,   842,   841,   471,
     471,   905,   471,   893,   462,   462,   870,   872,   471,   471,
     471,   471,   460,   506,   504,   550,   354,   544,   539,   398,
     535,   536,   134,   167,   532,   431,   525,   526,   460,   460,
     762,   693,   784,    13,   471,   471,   460,   471,   460,   763,
     460,   460,   767,   805,   462,   460,   471,   471,   804,   471,
     471,   471,   471,   471,   460,   167,   460,   460,   136,   874,
      21,   235,   298,   857,   471,   871,   873,   444,   556,   228,
     561,   561,   462,   471,   460,   439,   540,   471,   460,   535,
     471,   471,   563,   471,   460,   525,   515,   416,   763,   693,
     785,   471,    13,   767,   460,   460,   471,   471,   159,   471,
     770,   772,   471,   906,   894,   471,   471,   440,   875,   447,
     858,   471,   167,   296,   558,   471,   398,   471,   134,   534,
     530,   533,   134,   594,   460,   471,   138,   471,   460,   460,
     471,   471,   471,    64,    64,   460,   252,   247,   235,   471,
     471,   471,   172,   228,   562,   471,   471,   584,   563,   167,
     580,   581,   471,   170,   460,   471,   138,   471,   471,   460,
     771,   773,   673,   471,   907,   471,   895,   471,   398,   560,
     559,   172,   563,   167,   585,   531,   471,   580,   170,   471,
     471,   471,   460,   774,   774,   675,   460,   471,   460,   471,
     471,   471,   563,   564,   557,   460,   471,   577,   583,   170,
     471,   471,   170,   775,    14,    35,    37,   110,   134,   271,
     411,   676,   859,   460,   460,   564,   398,   167,   578,   471,
     460,   471,    25,   471,   674,   124,   471,   471,   471,   451,
     860,   460,   471,   471,   582,   403,   471,   460,   170,   471,
     460,   460,   460,   134,   579,   563,   471,   460,   460,   471,
     563,   471,   460,   350,   471,   471,   471,   471,   460,   595,
     596,    86,    88,    98,   424,   597,   471,   471,   598,   471,
     471,   471,   170,   471,   471,   460,   460,   460,   471,   460
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   467,   468,   470,   469,   471,   472,   473,   474,   474,
     474,   475,   475,   476,   476,   476,   476,   476,   476,   476,
     476,   476,   476,   476,   476,   476,   476,   476,   476,   476,
     476,   476,   476,   476,   476,   476,   476,   476,   476,   476,
     476,   476,   476,   476,   476,   476,   476,   476,   476,   476,
     476,   476,   477,   477,   478,   478,   479,   480,   481,   482,
     483,   483,   484,   484,   485,   485,   486,   487,   488,   488,
     489,   489,   489,   489,   489,   489,   489,   489,   490,   492,
     491,   494,   493,   495,   495,   497,   498,   496,   496,   496,
     496,   496,   496,   496,   496,   496,   496,   496,   496,   496,
     499,   496,   500,   496,   496,   496,   496,   496,   496,   496,
     496,   496,   496,   496,   496,   496,   496,   496,   496,   501,
     496,   502,   496,   496,   496,   503,   504,   496,   505,   506,
     496,   496,   507,   496,   496,   508,   496,   509,   496,   496,
     510,   496,   496,   511,   496,   512,   496,   513,   496,   496,
     496,   496,   514,   515,   496,   496,   496,   496,   496,   496,
     496,   496,   496,   496,   496,   496,   496,   496,   496,   496,
     516,   496,   517,   496,   518,   496,   496,   519,   496,   520,
     496,   521,   496,   496,   496,   522,   496,   523,   523,   524,
     524,   525,   525,   526,   528,   529,   530,   531,   527,   532,
     533,   527,   534,   527,   535,   535,   536,   537,   537,   537,
     538,   539,   538,   538,   540,   540,   541,   541,   542,   542,
     543,   543,   543,   544,   544,   545,   545,   546,   546,   546,
     547,   547,   547,   548,   549,   548,   550,   550,   551,   552,
     552,   553,   553,   553,   555,   556,   557,   554,   558,   559,
     558,   560,   558,   562,   561,   563,   563,   564,   564,   565,
     565,   566,   566,   566,   567,   567,   568,   569,   569,   570,
     571,   571,   572,   573,   573,   573,   573,   573,   573,   574,
     574,   574,   574,   575,   575,   576,   576,   576,   576,   576,
     576,   576,   576,   576,   576,   576,   576,   576,   576,   576,
     576,   576,   576,   576,   576,   576,   576,   576,   576,   576,
     576,   576,   576,   576,   576,   576,   576,   577,   577,   579,
     578,   580,   580,   582,   581,   583,   583,   584,   584,   585,
     586,   587,   586,   589,   588,   590,   591,   591,   591,   593,
     594,   595,   592,   596,   596,   597,   597,   597,   598,   597,
     599,   599,   600,   601,   601,   602,   602,   602,   603,   602,
     602,   604,   604,   605,   605,   605,   606,   606,   606,   606,
     608,   607,   609,   609,   609,   609,   609,   609,   609,   609,
     609,   609,   609,   609,   609,   609,   609,   609,   610,   612,
     611,   613,   613,   614,   615,   614,   617,   616,   619,   618,
     620,   622,   621,   623,   623,   624,   624,   625,   625,   626,
     626,   628,   627,   629,   629,   627,   627,   627,   630,   631,
     631,   632,   634,   633,   635,   635,   636,   636,   636,   636,
     636,   636,   636,   636,   636,   638,   637,   639,   640,   641,
     642,   642,   643,   643,   644,   645,   645,   646,   647,   648,
     649,   649,   650,   651,   651,   652,   653,   654,   655,   657,
     658,   659,   656,   660,   660,   661,   661,   662,   662,   663,
     663,   663,   663,   663,   663,   663,   664,   665,   666,   668,
     667,   669,   669,   667,   667,   667,   671,   672,   673,   674,
     670,   675,   675,   676,   676,   676,   676,   676,   676,   677,
     679,   678,   681,   680,   682,   682,   683,   683,   683,   683,
     684,   684,   684,   685,   686,   686,   687,   687,   687,   689,
     688,   690,   690,   692,   691,   693,   693,   695,   694,   697,
     696,   699,   698,   700,   700,   701,   701,   701,   701,   701,
     701,   701,   701,   701,   701,   701,   701,   701,   701,   701,
     701,   701,   701,   701,   701,   702,   701,   703,   703,   704,
     705,   705,   706,   706,   706,   707,   707,   707,   708,   709,
     709,   709,   709,   709,   709,   709,   709,   709,   709,   709,
     709,   709,   709,   709,   710,   710,   710,   710,   710,   710,
     711,   711,   711,   711,   711,   711,   712,   712,   712,   712,
     712,   712,   713,   714,   715,   715,   715,   716,   717,   718,
     718,   718,   718,   719,   721,   720,   723,   722,   724,   724,
     725,   726,   727,   728,   730,   729,   732,   731,   733,   733,
     734,   734,   734,   734,   734,   734,   735,   734,   734,   734,
     734,   734,   734,   734,   734,   734,   734,   734,   734,   734,
     734,   736,   734,   737,   734,   738,   734,   739,   734,   734,
     734,   734,   734,   734,   734,   734,   734,   734,   734,   734,
     740,   734,   734,   734,   734,   734,   734,   734,   734,   734,
     734,   734,   734,   734,   734,   734,   734,   741,   734,   742,
     734,   743,   734,   744,   734,   745,   745,   745,   745,   745,
     745,   745,   745,   745,   745,   745,   745,   745,   745,   745,
     745,   745,   745,   745,   745,   745,   745,   745,   745,   745,
     745,   745,   745,   745,   745,   745,   745,   746,   746,   747,
     747,   747,   748,   748,   748,   748,   748,   749,   750,   750,
     751,   751,   751,   751,   751,   751,   752,   752,   752,   752,
     753,   753,   753,   753,   754,   756,   757,   755,   755,   755,
     755,   755,   755,   755,   755,   755,   758,   758,   759,   759,
     760,   760,   760,   761,   762,   763,   763,   765,   764,   766,
     764,   767,   768,   768,   770,   771,   769,   772,   773,   769,
     769,   769,   774,   774,   775,   776,   776,   777,   777,   778,
     779,   780,   780,   782,   783,   781,   784,   784,   785,   787,
     786,   788,   789,   790,   791,   791,   793,   792,   794,   792,
     795,   792,   792,   792,   792,   792,   792,   792,   792,   792,
     792,   792,   792,   792,   792,   792,   792,   792,   792,   792,
     792,   796,   796,   796,   797,   797,   797,   797,   798,   798,
     798,   799,   799,   799,   800,   800,   801,   801,   802,   803,
     803,   804,   804,   804,   805,   805,   806,   806,   806,   807,
     807,   808,   808,   810,   809,   812,   811,   814,   813,   815,
     815,   817,   816,   818,   816,   819,   816,   820,   816,   816,
     821,   816,   816,   816,   822,   823,   823,   825,   824,   826,
     824,   827,   827,   828,   830,   829,   832,   831,   834,   833,
     835,   833,   836,   833,   837,   837,   837,   838,   838,   839,
     839,   840,   840,   840,   840,   840,   840,   840,   840,   841,
     841,   841,   841,   841,   841,   841,   841,   841,   841,   841,
     841,   841,   841,   841,   842,   842,   842,   842,   843,   843,
     843,   843,   843,   843,   843,   843,   843,   845,   844,   846,
     846,   848,   847,   849,   847,   850,   847,   851,   847,   852,
     847,   853,   847,   854,   847,   855,   847,   856,   856,   856,
     856,   856,   857,   857,   857,   857,   857,   858,   859,   858,
     860,   860,   861,   861,   862,   862,   863,   863,   864,   864,
     865,   865,   866,   867,   866,   866,   868,   869,   870,   868,
     871,   868,   868,   872,   868,   868,   868,   873,   868,   868,
     868,   874,   874,   875,   875,   876,   877,   876,   879,   878,
     880,   881,   882,   883,   885,   884,   886,   887,   887,   888,
     888,   890,   889,   891,   891,   892,   892,   894,   893,   895,
     895,   897,   896,   898,   899,   899,   900,   900,   902,   901,
     903,   903,   904,   904,   906,   905,   907,   907,   908,   909,
     910,   911,   912,   913,   914,   914,   915
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     3,     0,     4,     1,     3,     3,     0,     2,
       1,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     3,     3,     3,     2,     3,     4,     3,
       1,     1,     1,     1,     1,     1,     4,     1,     0,     2,
       4,     4,     4,     4,     4,     4,     4,     4,     3,     0,
       3,     0,     3,     0,     2,     0,     0,     9,     3,     3,
       3,     4,     3,     4,     3,     4,     3,     3,     3,     3,
       0,     6,     0,     9,     3,     4,     7,     4,     7,     3,
       3,     3,     3,     3,     3,     3,     3,     6,     6,     0,
       4,     0,     4,     4,     4,     0,     0,     9,     0,     0,
       9,     3,     0,     4,     3,     0,     4,     0,     5,     3,
       0,     4,     3,     0,     4,     0,     5,     0,     4,     2,
       3,     3,     0,     0,     9,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     4,     3,     3,     3,     3,
       0,     5,     0,     9,     0,     5,     7,     0,     4,     0,
       7,     0,     7,     3,     3,     0,     5,     0,     1,     0,
       2,     0,     2,     4,     0,     0,     0,     0,    11,     0,
       0,     9,     0,     9,     0,     2,     4,     0,     1,     1,
       0,     0,     4,     2,     0,     2,     0,     2,     0,     2,
       0,     1,     1,     0,     4,     0,     2,     1,     2,     2,
       1,     1,     1,     1,     0,     7,     0,     2,     1,     0,
       1,     1,     1,     1,     0,     0,     0,    12,     0,     0,
       5,     0,     5,     0,     5,     0,     2,     0,     2,     1,
       2,     2,     2,     2,     1,     2,     4,     1,     2,     4,
       1,     2,     4,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     2,     0,
       4,     0,     2,     0,     6,     0,     2,     0,     2,     6,
       3,     0,     7,     0,     4,     1,     2,     3,     3,     0,
       0,     0,    26,     0,     2,     4,     4,     6,     0,     4,
       1,     1,     2,     0,     2,     1,     1,     3,     0,     4,
       1,     1,     2,     2,     2,     2,     2,     3,     4,     3,
       0,     3,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     0,
       4,     0,     2,     5,     0,     8,     0,     3,     0,     3,
       5,     0,     7,     0,     1,     1,     2,     0,     1,     1,
       2,     0,     4,     1,     2,     2,     2,     2,     2,     0,
       2,     3,     0,     4,     0,     2,     3,     3,     4,     5,
       4,     5,     3,     3,     3,     0,     3,     3,     1,     2,
       0,     2,     5,     6,     1,     0,     2,     3,     1,     2,
       0,     2,     3,     0,     2,     2,     2,     4,     3,     0,
       0,     0,     8,     1,     2,     0,     2,     0,     2,     1,
       1,     1,     1,     1,     1,     1,     3,     3,     4,     0,
       4,     1,     2,     2,     2,     2,     0,     0,     0,     0,
      12,     0,     2,     3,     3,     4,     4,     3,     3,     3,
       0,     3,     0,     3,     0,     2,     5,     1,     1,     1,
       3,     3,     3,     3,     0,     2,     1,     1,     1,     0,
       4,     0,     2,     0,     3,     2,     4,     0,     4,     0,
       3,     0,     3,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     1,     1,     1,     1,     0,     4,     1,     2,     3,
       0,     2,     1,     1,     1,     2,     2,     2,     3,     1,
       2,     1,     1,     2,     2,     1,     1,     1,     1,     2,
       1,     1,     2,     2,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     4,     3,     3,     3,     3,     3,     2,
       3,     4,     3,     2,     0,     4,     0,     4,     3,     3,
       1,     1,     5,     3,     0,     3,     0,     3,     0,     2,
       2,     3,     4,     3,     4,     5,     0,     4,     3,     1,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     0,     4,     0,     5,     0,     5,     0,     5,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     4,     3,
       0,     4,     4,     2,     4,     4,     4,     3,     3,     4,
       4,     4,     4,     4,     4,     4,     4,     0,     4,     0,
       4,     0,     4,     0,     4,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     2,
       2,     2,     3,     3,     4,     3,     3,     1,     0,     3,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     1,     1,     1,     2,     0,     0,     7,     3,     5,
       7,     5,     7,     7,     9,     1,     0,     2,     0,     1,
       0,     2,     2,     1,     1,     0,     2,     0,     6,     0,
       8,     7,    11,     4,     0,     0,    10,     0,     0,    10,
       6,     6,     0,     2,     1,     6,     6,     3,     2,     1,
       4,     0,     2,     0,     0,     7,     0,     2,     5,     0,
       4,     3,     1,     2,     0,     2,     0,     4,     0,     4,
       0,    10,     9,     3,     3,     4,     4,     4,     4,     4,
       4,     4,     4,     3,     7,     8,     6,     3,     3,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     2,     5,     1,
       2,     0,     4,     7,     1,     1,     1,     1,     1,     1,
       2,     1,     2,     0,     4,     0,     3,     0,     3,     0,
       2,     0,     4,     0,     4,     0,     4,     0,     4,     4,
       0,     4,     5,     1,     2,     0,     2,     0,     4,     0,
       4,     0,     2,     5,     0,     6,     0,     6,     0,     6,
       0,     6,     0,     6,     0,     1,     1,     1,     2,     1,
       2,     3,     3,     3,     3,     2,     3,     6,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     2,
       3,     6,     1,     1,     3,     3,     6,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     0,     5,     0,
       2,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     3,     3,     1,
       2,     2,     0,     1,     2,     5,     3,     0,     0,     6,
       0,     1,     0,     1,     0,     3,     0,     1,     0,     1,
       0,     2,     1,     0,     3,     1,     0,     0,     0,     5,
       0,     6,     2,     0,     5,     2,     5,     0,     6,     2,
       6,     0,     1,     0,     1,     0,     0,     3,     0,     3,
       4,     3,     3,     3,     0,     7,     2,     1,     2,     3,
       1,     0,     5,     1,     2,     1,     2,     0,     7,     1,
       2,     0,     7,     2,     1,     2,     3,     1,     0,     5,
       1,     2,     1,     2,     0,     7,     1,     2,     3,     3,
       3,     3,     3,     3,     0,     2,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)




# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* lef_file: rules extension_opt end_library  */
#line 348 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 3957 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 3: /* $@1: %empty  */
#line 368 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   { lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 3963 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 4: /* version: K_VERSION $@1 T_STRING ';'  */
#line 369 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { 
		 // More than 1 VERSION in lef file within the open file - It's wrong syntax, 
		 // but copy old behavior - initialize lef reading.
         if (lefData->hasVer)     
         {
			lefData->initRead();
		 }

         lefData->versionNum = convert_name2num((yyvsp[-1].string));
         if (lefData->versionNum > CURRENT_VERSION) {
            char temp[120];
            sprintf(temp,
               "Lef parser %.1f does not support lef file with version %s. Parser will stop processing.", CURRENT_VERSION, (yyvsp[-1].string));
            lefError(1503, temp);
            return 1;
         }

         if (lefCallbacks->VersionStrCbk) {
            CALLBACK(lefCallbacks->VersionStrCbk, lefrVersionStrCbkType, (yyvsp[-1].string));
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
#line 4005 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 5: /* int_number: NUMBER  */
#line 408 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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

        (yyval.dval) = yylval.dval ;
      }
#line 4025 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 6: /* dividerchar: K_DIVIDERCHAR QSTRING ';'  */
#line 425 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {
        if (lefCallbacks->DividerCharCbk) {
          if (strcmp((yyvsp[-1].string), "") != 0) {
             CALLBACK(lefCallbacks->DividerCharCbk, lefrDividerCharCbkType, (yyvsp[-1].string));
          } else {
             CALLBACK(lefCallbacks->DividerCharCbk, lefrDividerCharCbkType, "/");
             lefWarning(2005, "DIVIDERCHAR has an invalid null value. Value is set to default /");
          }
        }
        lefData->hasDivChar = 1;
      }
#line 4041 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 7: /* busbitchars: K_BUSBITCHARS QSTRING ';'  */
#line 438 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {
        if (lefCallbacks->BusBitCharsCbk) {
          if (strcmp((yyvsp[-1].string), "") != 0) {
             CALLBACK(lefCallbacks->BusBitCharsCbk, lefrBusBitCharsCbkType, (yyvsp[-1].string)); 
          } else {
             CALLBACK(lefCallbacks->BusBitCharsCbk, lefrBusBitCharsCbkType, "[]"); 
             lefWarning(2006, "BUSBITCHAR has an invalid null value. Value is set to default []");
          }
        }
        lefData->hasBusBit = 1;
      }
#line 4057 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 10: /* rules: error  */
#line 453 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            { }
#line 4063 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 11: /* end_library: %empty  */
#line 456 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {
        if (lefData->versionNum >= 5.6) {
           lefData->doneLib = 1;
           lefData->ge56done = 1;
        }
      }
#line 4074 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 12: /* end_library: K_END K_LIBRARY  */
#line 463 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {
        lefData->doneLib = 1;
        lefData->ge56done = 1;
        if (lefCallbacks->LibraryEndCbk)
          CALLBACK(lefCallbacks->LibraryEndCbk, lefrLibraryEndCbkType, 0);
        // 11/16/2001 - Wanda da Rosa - pcr 408334
        // Return 1 if there are errors
      }
#line 4087 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 52: /* case_sensitivity: K_NAMESCASESENSITIVE K_ON ';'  */
#line 487 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {
            if (lefData->versionNum < 5.6) {
              lefData->namesCaseSensitive = true;
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
#line 4105 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 53: /* case_sensitivity: K_NAMESCASESENSITIVE K_OFF ';'  */
#line 501 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {
            if (lefData->versionNum < 5.6) {
              lefData->namesCaseSensitive = false;
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
#line 4126 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 54: /* wireextension: K_NOWIREEXTENSIONATPIN K_ON ';'  */
#line 519 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->NoWireExtensionCbk)
          CALLBACK(lefCallbacks->NoWireExtensionCbk, lefrNoWireExtensionCbkType, "ON");
      } else
        if (lefCallbacks->NoWireExtensionCbk) // write warning only if cbk is set 
           if (lefData->noWireExtensionWarnings++ < lefSettings->NoWireExtensionWarnings)
             lefWarning(2008, "NOWIREEXTENSIONATPIN statement is obsolete in version 5.6 or later.\nThe NOWIREEXTENSIONATPIN statement will be ignored.");
    }
#line 4140 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 55: /* wireextension: K_NOWIREEXTENSIONATPIN K_OFF ';'  */
#line 529 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->NoWireExtensionCbk)
          CALLBACK(lefCallbacks->NoWireExtensionCbk, lefrNoWireExtensionCbkType, "OFF");
      } else
        if (lefCallbacks->NoWireExtensionCbk) // write warning only if cbk is set 
           if (lefData->noWireExtensionWarnings++ < lefSettings->NoWireExtensionWarnings)
             lefWarning(2008, "NOWIREEXTENSIONATPIN statement is obsolete in version 5.6 or later.\nThe NOWIREEXTENSIONATPIN statement will be ignored.");
    }
#line 4154 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 56: /* fixedmask: K_FIXEDMASK ';'  */
#line 540 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
       if (lefData->versionNum >= 5.8) {
       
          if (lefCallbacks->FixedMaskCbk) {
            lefData->lefFixedMask = 1;
            CALLBACK(lefCallbacks->FixedMaskCbk, lefrFixedMaskCbkType, lefData->lefFixedMask);
          }
          
          lefData->hasFixedMask = 1;
       }
    }
#line 4170 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 57: /* manufacturing: K_MANUFACTURINGGRID int_number ';'  */
#line 553 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ManufacturingCbk)
        CALLBACK(lefCallbacks->ManufacturingCbk, lefrManufacturingCbkType, (yyvsp[-1].dval));
      lefData->hasManufactur = 1;
    }
#line 4180 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 58: /* useminspacing: K_USEMINSPACING spacing_type spacing_value ';'  */
#line 560 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if ((strcmp((yyvsp[-2].string), "PIN") == 0) && (lefData->versionNum >= 5.6)) {
      if (lefCallbacks->UseMinSpacingCbk) // write warning only if cbk is set 
         if (lefData->useMinSpacingWarnings++ < lefSettings->UseMinSpacingWarnings)
            lefWarning(2009, "USEMINSPACING PIN statement is obsolete in version 5.6 or later.\n The USEMINSPACING PIN statement will be ignored.");
    } else {
        if (lefCallbacks->UseMinSpacingCbk) {
          lefData->lefrUseMinSpacing.set((yyvsp[-2].string), (yyvsp[-1].integer));
          CALLBACK(lefCallbacks->UseMinSpacingCbk, lefrUseMinSpacingCbkType,
                   &lefData->lefrUseMinSpacing);
      }
    }
  }
#line 4198 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 59: /* clearancemeasure: K_CLEARANCEMEASURE clearance_type ';'  */
#line 575 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { CALLBACK(lefCallbacks->ClearanceMeasureCbk, lefrClearanceMeasureCbkType, (yyvsp[-1].string)); }
#line 4204 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 60: /* clearance_type: K_MAXXY  */
#line 578 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {(yyval.string) = (char*)"MAXXY";}
#line 4210 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 61: /* clearance_type: K_EUCLIDEAN  */
#line 579 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"EUCLIDEAN";}
#line 4216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 62: /* spacing_type: K_OBS  */
#line 582 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {(yyval.string) = (char*)"OBS";}
#line 4222 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 63: /* spacing_type: K_PIN  */
#line 583 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {(yyval.string) = (char*)"PIN";}
#line 4228 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 64: /* spacing_value: K_ON  */
#line 586 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {(yyval.integer) = 1;}
#line 4234 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 65: /* spacing_value: K_OFF  */
#line 587 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {(yyval.integer) = 0;}
#line 4240 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 66: /* units_section: start_units units_rules K_END K_UNITS  */
#line 590 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->UnitsCbk)
        CALLBACK(lefCallbacks->UnitsCbk, lefrUnitsCbkType, &lefData->lefrUnits);
    }
#line 4249 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 67: /* start_units: K_UNITS  */
#line 596 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 4275 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 70: /* units_rule: K_TIME K_NANOSECONDS int_number ';'  */
#line 623 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setTime((yyvsp[-1].dval)); }
#line 4281 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 71: /* units_rule: K_CAPACITANCE K_PICOFARADS int_number ';'  */
#line 625 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setCapacitance((yyvsp[-1].dval)); }
#line 4287 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 72: /* units_rule: K_RESISTANCE K_OHMS int_number ';'  */
#line 627 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setResistance((yyvsp[-1].dval)); }
#line 4293 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 73: /* units_rule: K_POWER K_MILLIWATTS int_number ';'  */
#line 629 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setPower((yyvsp[-1].dval)); }
#line 4299 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 74: /* units_rule: K_CURRENT K_MILLIAMPS int_number ';'  */
#line 631 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setCurrent((yyvsp[-1].dval)); }
#line 4305 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 75: /* units_rule: K_VOLTAGE K_VOLTS int_number ';'  */
#line 633 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setVoltage((yyvsp[-1].dval)); }
#line 4311 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 76: /* units_rule: K_DATABASE K_MICRONS int_number ';'  */
#line 635 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if(validNum((int)(yyvsp[-1].dval))) {
         if (lefCallbacks->UnitsCbk)
            lefData->lefrUnits.setDatabase("MICRONS", (yyvsp[-1].dval));
      }
    }
#line 4322 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 77: /* units_rule: K_FREQUENCY K_MEGAHERTZ NUMBER ';'  */
#line 642 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->UnitsCbk) lefData->lefrUnits.setFrequency((yyvsp[-1].dval)); }
#line 4328 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 78: /* layer_rule: start_layer layer_options end_layer  */
#line 646 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk)
        CALLBACK(lefCallbacks->LayerCbk, lefrLayerCbkType, &lefData->lefrLayer);
    }
#line 4337 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 79: /* $@2: %empty  */
#line 651 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                     {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 4343 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 80: /* start_layer: K_LAYER $@2 T_STRING  */
#line 652 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        lefData->lefrLayer.setName((yyvsp[0].string));
      lefData->useLenThr = 0;
      lefData->layerCut = 0;
      lefData->layerMastOver = 0;
      lefData->layerRout = 0;
      lefData->layerDir = 0;
      lefData->lefrHasLayer = 1;
      //strcpy(lefData->layerName, $3);
      lefData->layerName = strdup((yyvsp[0].string));
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
#line 4378 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 81: /* $@3: %empty  */
#line 683 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 4384 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 82: /* end_layer: K_END $@3 T_STRING  */
#line 684 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (strcmp(lefData->layerName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
          if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
             lefData->outMsg = (char*)lefMalloc(10000);
             sprintf (lefData->outMsg,
                "END LAYER name %s is different from the LAYER name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->layerName);
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
              sprintf (lefData->outMsg, "The DIRECTION statement which is required in a LAYER with TYPE ROUTING is not defined in LAYER %s.\nUpdate your lef file and add the DIRECTION statement for layer %s.", (yyvsp[0].string), (yyvsp[0].string));
              lefError(1511, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR(); 
            }
          }
        }
      }
    }
#line 4444 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 83: /* layer_options: %empty  */
#line 741 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 4450 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 84: /* layer_options: layer_options layer_option  */
#line 743 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 4456 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 85: /* $@4: %empty  */
#line 747 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       // let setArraySpacingCutSpacing to set the data 
    }
#line 4464 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 86: /* $@5: %empty  */
#line 753 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.setArraySpacingCut((yyvsp[0].dval));
         lefData->arrayCutsVal = 0;
      }
    }
#line 4475 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 87: /* layer_option: K_ARRAYSPACING $@4 layer_arraySpacing_long layer_arraySpacing_width K_CUTSPACING int_number $@5 layer_arraySpacing_arraycuts ';'  */
#line 760 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 4490 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 88: /* layer_option: K_TYPE layer_type ';'  */
#line 771 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.setType((yyvsp[-1].string));
      lefData->hasType = 1;
    }
#line 4500 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 89: /* layer_option: K_MASK int_number ';'  */
#line 777 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.8) {
          if (lefData->layerWarnings++ < lefSettings->ViaWarnings) {
              lefError(2081, "MASK information can only be defined with version 5.8");
              CHKERR(); 
          }           
      } else {
          if (lefCallbacks->LayerCbk) {
            lefData->lefrLayer.setMask((int)(yyvsp[-1].dval));
          }
          
          lefData->hasMask = 1;
      }
    }
#line 4519 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 90: /* layer_option: K_PITCH int_number ';'  */
#line 792 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setPitch((yyvsp[-1].dval));
      lefData->hasPitch = 1;  
    }
#line 4528 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 91: /* layer_option: K_PITCH int_number int_number ';'  */
#line 797 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setPitchXY((yyvsp[-2].dval), (yyvsp[-1].dval));
      lefData->hasPitch = 1;  
    }
#line 4537 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 92: /* layer_option: K_DIAGPITCH int_number ';'  */
#line 802 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagPitch((yyvsp[-1].dval));
    }
#line 4545 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 93: /* layer_option: K_DIAGPITCH int_number int_number ';'  */
#line 806 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagPitchXY((yyvsp[-2].dval), (yyvsp[-1].dval));
    }
#line 4553 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 94: /* layer_option: K_OFFSET int_number ';'  */
#line 810 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setOffset((yyvsp[-1].dval));
    }
#line 4561 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 95: /* layer_option: K_OFFSET int_number int_number ';'  */
#line 814 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setOffsetXY((yyvsp[-2].dval), (yyvsp[-1].dval));
    }
#line 4569 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 96: /* layer_option: K_DIAGWIDTH int_number ';'  */
#line 818 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagWidth((yyvsp[-1].dval));
    }
#line 4577 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 97: /* layer_option: K_DIAGSPACING int_number ';'  */
#line 822 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDiagSpacing((yyvsp[-1].dval));
    }
#line 4585 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 98: /* layer_option: K_WIDTH int_number ';'  */
#line 826 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setWidth((yyvsp[-1].dval));
      lefData->hasWidth = 1;  
    }
#line 4594 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 99: /* layer_option: K_AREA NUMBER ';'  */
#line 831 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.setArea((yyvsp[-1].dval));
      }
    }
#line 4614 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 100: /* $@6: %empty  */
#line 847 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->hasSpCenter = 0;       // reset to 0, only once per spacing is allowed 
      lefData->hasSpSamenet = 0;
      lefData->hasSpParallel = 0;
      lefData->hasSpLayer = 0;
      lefData->layerCutSpacing = (yyvsp[0].dval);  // for error message purpose
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
           lefData->lefrLayer.setSpacingMin((yyvsp[0].dval));
        lefData->lefrHasSpacing = 1;
      } else { 
        if (lefCallbacks->LayerCbk)
           lefData->lefrLayer.setSpacingMin((yyvsp[0].dval));
      }
    }
#line 4652 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 101: /* layer_option: K_SPACING int_number $@6 layer_spacing_opts layer_spacing_cut_routing ';'  */
#line 881 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                  {}
#line 4658 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 102: /* $@7: %empty  */
#line 883 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.setSpacingTableOrtho();
      if (lefCallbacks->LayerCbk) // due to converting to C, else, convertor produce 
         lefData->lefrLayer.addSpacingTableOrthoWithin((yyvsp[-2].dval), (yyvsp[0].dval));//bad code
    }
#line 4669 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 103: /* layer_option: K_SPACINGTABLE K_ORTHOGONAL K_WITHIN int_number K_SPACING int_number $@7 layer_spacingtable_opts ';'  */
#line 890 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 4684 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 104: /* layer_option: K_DIRECTION layer_direction ';'  */
#line 901 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDirection((yyvsp[-1].string));
      lefData->hasDirection = 1;  
    }
#line 4702 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 105: /* layer_option: K_RESISTANCE K_RPERSQ int_number ';'  */
#line 915 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1514, "RESISTANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setResistance((yyvsp[-1].dval));
    }
#line 4718 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 106: /* layer_option: K_RESISTANCE K_RPERSQ K_PWL '(' res_points ')' ';'  */
#line 927 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 4733 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 107: /* layer_option: K_CAPACITANCE K_CPERSQDIST int_number ';'  */
#line 938 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1516, "CAPACITANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCapacitance((yyvsp[-1].dval));
    }
#line 4749 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 108: /* layer_option: K_CAPACITANCE K_CPERSQDIST K_PWL '(' cap_points ')' ';'  */
#line 950 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 4764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 109: /* layer_option: K_HEIGHT int_number ';'  */
#line 961 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1518, "HEIGHT statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setHeight((yyvsp[-1].dval));
    }
#line 4780 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 110: /* layer_option: K_WIREEXTENSION int_number ';'  */
#line 973 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1519, "WIREEXTENSION statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setWireExtension((yyvsp[-1].dval));
    }
#line 4796 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 111: /* layer_option: K_THICKNESS int_number ';'  */
#line 985 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout && (lefData->layerCut || lefData->layerMastOver)) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1520, "THICKNESS statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setThickness((yyvsp[-1].dval));
    }
#line 4812 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 112: /* layer_option: K_SHRINKAGE int_number ';'  */
#line 997 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1521, "SHRINKAGE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setShrinkage((yyvsp[-1].dval));
    }
#line 4828 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 113: /* layer_option: K_CAPMULTIPLIER int_number ';'  */
#line 1009 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1522, "CAPMULTIPLIER statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCapMultiplier((yyvsp[-1].dval));
    }
#line 4844 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 114: /* layer_option: K_EDGECAPACITANCE int_number ';'  */
#line 1021 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1523, "EDGECAPACITANCE statement can only be defined in LAYER with TYPE ROUTING. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setEdgeCap((yyvsp[-1].dval));
    }
#line 4860 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 115: /* layer_option: K_ANTENNALENGTHFACTOR int_number ';'  */
#line 1034 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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

      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaLength((yyvsp[-1].dval));
    }
#line 4891 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 116: /* layer_option: K_CURRENTDEN int_number ';'  */
#line 1061 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCurrentDensity((yyvsp[-1].dval));
      } else {
         if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
            lefWarning(2079, "CURRENTDEN statement is obsolete in version 5.2 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.2 or later.");
            CHKERR();
         }
      }
    }
#line 4914 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 117: /* layer_option: K_CURRENTDEN K_PWL '(' current_density_pwl_list ')' ';'  */
#line 1080 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 4936 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 118: /* layer_option: K_CURRENTDEN '(' int_number int_number ')' ';'  */
#line 1098 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCurrentPoint((yyvsp[-3].dval), (yyvsp[-2].dval));
      } else {
         if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
            lefWarning(2079, "CURRENTDEN statement is obsolete in version 5.2 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.2 or later.");
            CHKERR();
         }
      }
    }
#line 4959 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 119: /* $@8: %empty  */
#line 1116 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               { lefData->lefDumbMode = 10000000;}
#line 4965 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 120: /* layer_option: K_PROPERTY $@8 layer_prop_list ';'  */
#line 1117 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->lefDumbMode = 0;
    }
#line 4973 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 121: /* $@9: %empty  */
#line 1121 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->layerMastOver) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1527, "ACCURRENTDENSITY statement can't be defined in LAYER with TYPE MASTERSLICE or OVERLAP. Parser will stop processing.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAccurrentDensity((yyvsp[0].string));      
    }
#line 4989 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 122: /* layer_option: K_ACCURRENTDENSITY layer_table_type $@9 layer_frequency  */
#line 1132 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {

    }
#line 4997 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 123: /* layer_option: K_ACCURRENTDENSITY layer_table_type int_number ';'  */
#line 1136 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
           lefData->lefrLayer.addAccurrentDensity((yyvsp[-2].string));
           lefData->lefrLayer.setAcOneEntry((yyvsp[-1].dval));
      }
    }
#line 5016 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 124: /* layer_option: K_DCCURRENTDENSITY K_AVERAGE int_number ';'  */
#line 1151 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.setDcOneEntry((yyvsp[-1].dval));
      }
    }
#line 5035 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 125: /* $@10: %empty  */
#line 1166 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.addNumber((yyvsp[0].dval));
      }
    }
#line 5062 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 126: /* $@11: %empty  */
#line 1189 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addDcCutarea(); }
#line 5068 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 127: /* layer_option: K_DCCURRENTDENSITY K_AVERAGE K_CUTAREA NUMBER $@10 number_list ';' $@11 dc_layer_table  */
#line 1190 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {}
#line 5074 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 128: /* $@12: %empty  */
#line 1192 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.addNumber((yyvsp[0].dval));
      }
    }
#line 5101 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 129: /* $@13: %empty  */
#line 1215 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addDcWidth(); }
#line 5107 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 130: /* layer_option: K_DCCURRENTDENSITY K_AVERAGE K_WIDTH int_number $@12 int_number_list ';' $@13 dc_layer_table  */
#line 1216 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {}
#line 5113 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 131: /* layer_option: K_ANTENNAAREARATIO int_number ';'  */
#line 1220 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaAreaRatio((yyvsp[-1].dval));
    }
#line 5155 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 132: /* $@14: %empty  */
#line 1258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5197 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 133: /* layer_option: K_ANTENNADIFFAREARATIO $@14 layer_antenna_pwl ';'  */
#line 1295 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 5203 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 134: /* layer_option: K_ANTENNACUMAREARATIO int_number ';'  */
#line 1297 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaCumAreaRatio((yyvsp[-1].dval));
    }
#line 5245 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 135: /* $@15: %empty  */
#line 1335 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5287 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 136: /* layer_option: K_ANTENNACUMDIFFAREARATIO $@15 layer_antenna_pwl ';'  */
#line 1372 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 5293 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 137: /* $@16: %empty  */
#line 1374 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaAreaFactor((yyvsp[0].dval));
      lefData->antennaType = lefiAntennaAF;
    }
#line 5311 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 138: /* layer_option: K_ANTENNAAREAFACTOR int_number $@16 layer_antenna_duo ';'  */
#line 1387 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 5317 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 139: /* layer_option: K_ANTENNASIDEAREARATIO int_number ';'  */
#line 1389 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaSideAreaRatio((yyvsp[-1].dval));
    }
#line 5359 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 140: /* $@17: %empty  */
#line 1427 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5401 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 141: /* layer_option: K_ANTENNADIFFSIDEAREARATIO $@17 layer_antenna_pwl ';'  */
#line 1464 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 5407 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 142: /* layer_option: K_ANTENNACUMSIDEAREARATIO int_number ';'  */
#line 1466 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaCumSideAreaRatio((yyvsp[-1].dval));
    }
#line 5449 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 143: /* $@18: %empty  */
#line 1504 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5491 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 144: /* layer_option: K_ANTENNACUMDIFFSIDEAREARATIO $@18 layer_antenna_pwl ';'  */
#line 1541 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 5497 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 145: /* $@19: %empty  */
#line 1543 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaSideAreaFactor((yyvsp[0].dval));
      lefData->antennaType = lefiAntennaSAF;
    }
#line 5540 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 146: /* layer_option: K_ANTENNASIDEAREAFACTOR int_number $@19 layer_antenna_duo ';'  */
#line 1581 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 5546 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 147: /* $@20: %empty  */
#line 1583 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5588 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 148: /* layer_option: K_ANTENNAMODEL $@20 layer_oxide ';'  */
#line 1620 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {}
#line 5594 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 149: /* layer_option: K_ANTENNACUMROUTINGPLUSCUT ';'  */
#line 1622 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5619 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 150: /* layer_option: K_ANTENNAGATEPLUSDIFF int_number ';'  */
#line 1643 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaGatePlusDiff((yyvsp[-1].dval));
      }
    }
#line 5644 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 151: /* layer_option: K_ANTENNAAREAMINUSDIFF int_number ';'  */
#line 1664 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setAntennaAreaMinusDiff((yyvsp[-1].dval));
      }
    }
#line 5669 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 152: /* $@21: %empty  */
#line 1685 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrAntennaPWLPtr->addAntennaPWL((yyvsp[-1].pt).x, (yyvsp[-1].pt).y);
         lefData->lefrAntennaPWLPtr->addAntennaPWL((yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
    }
#line 5694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 153: /* $@22: %empty  */
#line 1706 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setAntennaPWL(lefiAntennaADR, lefData->lefrAntennaPWLPtr);
        lefData->lefrAntennaPWLPtr = NULL;
      }
    }
#line 5705 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 154: /* layer_option: K_ANTENNAAREADIFFREDUCEPWL '(' pt pt $@21 layer_diffusion_ratios ')' ';' $@22  */
#line 1712 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 5720 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 155: /* layer_option: K_SLOTWIREWIDTH int_number ';'  */
#line 1723 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireWidth((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireWidth((yyvsp[-1].dval));
    }
#line 5748 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 156: /* layer_option: K_SLOTWIRELENGTH int_number ';'  */
#line 1747 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireLength((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWireLength((yyvsp[-1].dval));
    }
#line 5776 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 157: /* layer_option: K_SLOTWIDTH int_number ';'  */
#line 1771 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWidth((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotWidth((yyvsp[-1].dval));
    }
#line 5804 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 158: /* layer_option: K_SLOTLENGTH int_number ';'  */
#line 1795 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotLength((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSlotLength((yyvsp[-1].dval));
    }
#line 5832 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 159: /* layer_option: K_MAXADJACENTSLOTSPACING int_number ';'  */
#line 1819 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxAdjacentSlotSpacing((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxAdjacentSlotSpacing((yyvsp[-1].dval));
    }
#line 5860 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 160: /* layer_option: K_MAXCOAXIALSLOTSPACING int_number ';'  */
#line 1843 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxCoaxialSlotSpacing((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxCoaxialSlotSpacing((yyvsp[-1].dval));
    }
#line 5888 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 161: /* layer_option: K_MAXEDGESLOTSPACING int_number ';'  */
#line 1867 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // 5.4 syntax 
      if (lefData->ignoreVersion) {
         // do nothing 
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxEdgeSlotSpacing((yyvsp[-1].dval));
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
         if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxEdgeSlotSpacing((yyvsp[-1].dval));
    }
#line 5916 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 162: /* layer_option: K_SPLITWIREWIDTH int_number ';'  */
#line 1891 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setSplitWireWidth((yyvsp[-1].dval));
    }
#line 5943 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 163: /* layer_option: K_MINIMUMDENSITY int_number ';'  */
#line 1914 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMinimumDensity((yyvsp[-1].dval));
    }
#line 5965 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 164: /* layer_option: K_MAXIMUMDENSITY int_number ';'  */
#line 1932 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaximumDensity((yyvsp[-1].dval));
    }
#line 5987 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 165: /* layer_option: K_DENSITYCHECKWINDOW int_number int_number ';'  */
#line 1950 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDensityCheckWindow((yyvsp[-2].dval), (yyvsp[-1].dval));
    }
#line 6009 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 166: /* layer_option: K_DENSITYCHECKSTEP int_number ';'  */
#line 1968 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setDensityCheckStep((yyvsp[-1].dval));
    }
#line 6031 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 167: /* layer_option: K_FILLACTIVESPACING int_number ';'  */
#line 1986 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setFillActiveSpacing((yyvsp[-1].dval));
    }
#line 6053 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 168: /* layer_option: K_MAXWIDTH int_number ';'  */
#line 2004 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMaxwidth((yyvsp[-1].dval));
    }
#line 6082 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 169: /* layer_option: K_MINWIDTH int_number ';'  */
#line 2029 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setMinwidth((yyvsp[-1].dval));
    }
#line 6111 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 170: /* $@23: %empty  */
#line 2054 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinenclosedarea((yyvsp[0].dval));
    }
#line 6131 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 171: /* layer_option: K_MINENCLOSEDAREA NUMBER $@23 layer_minen_width ';'  */
#line 2069 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                          {}
#line 6137 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 172: /* $@24: %empty  */
#line 2071 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { // pcr 409334 
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addMinimumcut((int)(yyvsp[-2].dval), (yyvsp[0].dval)); 
      lefData->hasLayerMincut = 0;
    }
#line 6147 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 173: /* layer_option: K_MINIMUMCUT int_number K_WIDTH int_number $@24 layer_minimumcut_within layer_minimumcut_from layer_minimumcut_length ';'  */
#line 2079 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->hasLayerMincut) {   // FROMABOVE nor FROMBELOW is set 
         if (lefCallbacks->LayerCbk)
             lefData->lefrLayer.addMinimumcutConnect((char*)"");
      }
    }
#line 6158 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 174: /* $@25: %empty  */
#line 2086 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstep((yyvsp[0].dval));
    }
#line 6166 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 175: /* layer_option: K_MINSTEP int_number $@25 layer_minstep_options ';'  */
#line 2090 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    }
#line 6173 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 176: /* layer_option: K_PROTRUSIONWIDTH int_number K_LENGTH int_number K_WIDTH int_number ';'  */
#line 2093 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.setProtrusion((yyvsp[-5].dval), (yyvsp[-3].dval), (yyvsp[-1].dval));
    }
#line 6193 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 177: /* $@26: %empty  */
#line 2109 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 6221 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 178: /* layer_option: K_SPACINGTABLE $@26 sp_options ';'  */
#line 2132 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {}
#line 6227 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 179: /* $@27: %empty  */
#line 2135 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.addEnclosure((yyvsp[-2].string), (yyvsp[-1].dval), (yyvsp[0].dval));
    }
#line 6248 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 180: /* layer_option: K_ENCLOSURE layer_enclosure_type_opt int_number int_number $@27 layer_enclosure_width_opt ';'  */
#line 2151 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                  {}
#line 6254 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 181: /* $@28: %empty  */
#line 2154 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.addPreferEnclosure((yyvsp[-2].string), (yyvsp[-1].dval), (yyvsp[0].dval));
    }
#line 6275 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 182: /* layer_option: K_PREFERENCLOSURE layer_enclosure_type_opt int_number int_number $@28 layer_preferenclosure_width_opt ';'  */
#line 2170 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                        {}
#line 6281 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 183: /* layer_option: K_RESISTANCE int_number ';'  */
#line 2172 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
            lefData->lefrLayer.setResPerCut((yyvsp[-1].dval));
      }
    }
#line 6303 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 184: /* layer_option: K_DIAGMINEDGELENGTH int_number ';'  */
#line 2190 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
            lefData->lefrLayer.setDiagMinEdgeLength((yyvsp[-1].dval));
      }
    }
#line 6332 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 185: /* $@29: %empty  */
#line 2215 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      // Use the polygon code to retrieve the points for MINSIZE
      lefData->lefrGeometriesPtr = (lefiGeometries*)lefMalloc(sizeof(lefiGeometries));
      lefData->lefrGeometriesPtr->Init();
      lefData->lefrDoGeometries = 1;
    }
#line 6343 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 186: /* layer_option: K_MINSIZE $@29 firstPt otherPts ';'  */
#line 2222 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrGeometriesPtr->addPolygon();
         lefData->lefrLayer.setMinSize(lefData->lefrGeometriesPtr);
      }
     lefData->lefrDoGeometries = 0;
      lefData->lefrGeometriesPtr->Destroy();
      lefFree(lefData->lefrGeometriesPtr);
    }
#line 6357 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 188: /* layer_arraySpacing_long: K_LONGARRAY  */
#line 2235 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
        if (lefCallbacks->LayerCbk)
           lefData->lefrLayer.setArraySpacingLongArray();
    }
#line 6366 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 190: /* layer_arraySpacing_width: K_WIDTH int_number  */
#line 2243 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.setArraySpacingWidth((yyvsp[0].dval));
    }
#line 6375 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 193: /* layer_arraySpacing_arraycut: K_ARRAYCUTS int_number K_SPACING int_number  */
#line 2254 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addArraySpacingArray((int)(yyvsp[-2].dval), (yyvsp[0].dval));
         if (lefData->arrayCutsVal > (int)(yyvsp[-2].dval)) {
            // Mulitiple ARRAYCUTS value needs to me in ascending order 
            if (!lefData->arrayCutsWar) {
               if (lefData->layerWarnings++ < lefSettings->LayerWarnings)
                  lefWarning(2080, "The number of cut values in multiple ARRAYSPACING ARRAYCUTS are not in increasing order.\nTo be consistent with the documentation, update the cut values to increasing order.");
               lefData->arrayCutsWar = 1;
            }
         }
         lefData->arrayCutsVal = (int)(yyvsp[-2].dval);
      }
    }
#line 6394 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 194: /* $@30: %empty  */
#line 2271 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval));
      lefData->hasParallel = 1;
    }
#line 6419 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 195: /* $@31: %empty  */
#line 2292 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->spParallelLength = lefData->lefrLayer.getNumber();
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpParallelLength();
    }
#line 6428 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 196: /* $@32: %empty  */
#line 2297 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addSpParallelWidth((yyvsp[0].dval));
      }
    }
#line 6438 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 197: /* $@33: %empty  */
#line 2303 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 6454 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 199: /* $@34: %empty  */
#line 2317 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval));
    }
#line 6462 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 200: /* $@35: %empty  */
#line 2321 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->hasParallel) { // 5.7 - Either PARALLEL OR TWOWIDTHS per layer
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1592, "A PARALLELRUNLENGTH statement was already defined in the layer.\nIt is PARALLELRUNLENGTH or TWOWIDTHS is allowed per layer.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpTwoWidths((yyvsp[-4].dval), (yyvsp[-3].dval));
      lefData->hasTwoWidths = 1;
    }
#line 6479 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 201: /* sp_options: K_TWOWIDTHS K_WIDTH int_number layer_sp_TwoWidthsPRL int_number $@34 int_number_list $@35 layer_sp_TwoWidths  */
#line 2334 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 6494 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 202: /* $@36: %empty  */
#line 2345 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.addSpInfluence((yyvsp[-4].dval), (yyvsp[-2].dval), (yyvsp[0].dval));
      }
    }
#line 6521 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 206: /* layer_spacingtable_opt: K_WITHIN int_number K_SPACING int_number  */
#line 2375 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addSpacingTableOrthoWithin((yyvsp[-2].dval), (yyvsp[0].dval));
  }
#line 6530 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 207: /* layer_enclosure_type_opt: %empty  */
#line 2381 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {(yyval.string) = (char*)"NULL";}
#line 6536 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 208: /* layer_enclosure_type_opt: K_ABOVE  */
#line 2382 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
             {(yyval.string) = (char*)"ABOVE";}
#line 6542 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 209: /* layer_enclosure_type_opt: K_BELOW  */
#line 2383 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
             {(yyval.string) = (char*)"BELOW";}
#line 6548 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 211: /* $@37: %empty  */
#line 2387 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addEnclosureWidth((yyvsp[0].dval));
      }
    }
#line 6558 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 213: /* layer_enclosure_width_opt: K_LENGTH int_number  */
#line 2394 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
            lefData->lefrLayer.addEnclosureLength((yyvsp[0].dval));
         }
      }
    }
#line 6577 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 215: /* layer_enclosure_width_except_opt: K_EXCEPTEXTRACUT int_number  */
#line 2411 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
            lefData->lefrLayer.addEnclosureExceptEC((yyvsp[0].dval));
         }
      }
    }
#line 6596 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 217: /* layer_preferenclosure_width_opt: K_WIDTH int_number  */
#line 2428 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addPreferEnclosureWidth((yyvsp[0].dval));
      }
    }
#line 6606 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 219: /* layer_minimumcut_within: K_WITHIN int_number  */
#line 2436 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
            lefData->lefrLayer.addMinimumcutWithin((yyvsp[0].dval));
         }
      }
    }
#line 6625 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 221: /* layer_minimumcut_from: K_FROMABOVE  */
#line 2453 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 6648 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 222: /* layer_minimumcut_from: K_FROMBELOW  */
#line 2472 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 6670 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 224: /* layer_minimumcut_length: K_LENGTH int_number K_WITHIN int_number  */
#line 2492 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         lefData->lefrLayer.addMinimumcutLengDis((yyvsp[-2].dval), (yyvsp[0].dval));
    }
#line 6691 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 227: /* layer_minstep_option: layer_minstep_type  */
#line 2514 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstepType((yyvsp[0].string));
  }
#line 6699 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 228: /* layer_minstep_option: K_LENGTHSUM int_number  */
#line 2518 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstepLengthsum((yyvsp[0].dval));
  }
#line 6707 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 229: /* layer_minstep_option: K_MAXEDGES int_number  */
#line 2522 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefData->versionNum < 5.7) {
      lefData->outMsg = (char*)lefMalloc(10000);
      sprintf(lefData->outMsg,
        "MAXEDGES is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
      lefError(1710, lefData->outMsg);
      lefFree(lefData->outMsg);
      CHKERR();
    } else
       if (lefCallbacks->LayerCbk) lefData->lefrLayer.addMinstepMaxedges((int)(yyvsp[0].dval));
  }
#line 6723 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 230: /* layer_minstep_type: K_INSIDECORNER  */
#line 2535 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 {(yyval.string) = (char*)"INSIDECORNER";}
#line 6729 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 231: /* layer_minstep_type: K_OUTSIDECORNER  */
#line 2536 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"OUTSIDECORNER";}
#line 6735 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 232: /* layer_minstep_type: K_STEP  */
#line 2537 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
           {(yyval.string) = (char*)"STEP";}
#line 6741 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 233: /* layer_antenna_pwl: int_number  */
#line 2541 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { if (lefCallbacks->LayerCbk)
          lefData->lefrLayer.setAntennaValue(lefData->antennaType, (yyvsp[0].dval)); }
#line 6748 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 234: /* $@38: %empty  */
#line 2544 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { if (lefCallbacks->LayerCbk) { // require min 2 points, set the 1st 2 
          if (lefData->lefrAntennaPWLPtr) {
            lefData->lefrAntennaPWLPtr->Destroy();
            lefFree(lefData->lefrAntennaPWLPtr);
          }

          lefData->lefrAntennaPWLPtr = lefiAntennaPWL::create();
          lefData->lefrAntennaPWLPtr->addAntennaPWL((yyvsp[-1].pt).x, (yyvsp[-1].pt).y);
          lefData->lefrAntennaPWLPtr->addAntennaPWL((yyvsp[0].pt).x, (yyvsp[0].pt).y);
        }
      }
#line 6764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 235: /* layer_antenna_pwl: K_PWL '(' pt pt $@38 layer_diffusion_ratios ')'  */
#line 2556 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { 
        if (lefCallbacks->LayerCbk) {
          lefData->lefrLayer.setAntennaPWL(lefData->antennaType, lefData->lefrAntennaPWLPtr);
          lefData->lefrAntennaPWLPtr = NULL;
        }
      }
#line 6775 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 238: /* layer_diffusion_ratio: pt  */
#line 2569 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  { if (lefCallbacks->LayerCbk)
      lefData->lefrAntennaPWLPtr->addAntennaPWL((yyvsp[0].pt).x, (yyvsp[0].pt).y);
  }
#line 6783 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 240: /* layer_antenna_duo: K_DIFFUSEONLY  */
#line 2575 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 6819 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 241: /* layer_table_type: K_PEAK  */
#line 2608 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {(yyval.string) = (char*)"PEAK";}
#line 6825 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 242: /* layer_table_type: K_AVERAGE  */
#line 2609 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {(yyval.string) = (char*)"AVERAGE";}
#line 6831 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 243: /* layer_table_type: K_RMS  */
#line 2610 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {(yyval.string) = (char*)"RMS";}
#line 6837 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 244: /* $@39: %empty  */
#line 2614 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval)); }
#line 6843 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 245: /* $@40: %empty  */
#line 2616 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcFrequency(); }
#line 6849 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 246: /* $@41: %empty  */
#line 2619 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval)); }
#line 6855 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 247: /* layer_frequency: K_FREQUENCY NUMBER $@39 number_list ';' $@40 ac_layer_table_opt K_TABLEENTRIES NUMBER $@41 number_list ';'  */
#line 2621 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcTableEntry(); }
#line 6861 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 249: /* $@42: %empty  */
#line 2625 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerCut) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1600, "CUTAREA statement can only be defined in LAYER with TYPE CUT.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval));
    }
#line 6877 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 250: /* ac_layer_table_opt: K_CUTAREA NUMBER $@42 number_list ';'  */
#line 2637 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcCutarea(); }
#line 6883 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 251: /* $@43: %empty  */
#line 2639 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (!lefData->layerRout) {
         if (lefCallbacks->LayerCbk) { // write error only if cbk is set 
            if (lefData->layerWarnings++ < lefSettings->LayerWarnings) {
              lefError(1601, "WIDTH can only be defined in LAYER with TYPE ROUTING.");
              CHKERR();
            }
         }
      }
      if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval));
    }
#line 6899 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 252: /* ac_layer_table_opt: K_WIDTH int_number $@43 int_number_list ';'  */
#line 2651 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addAcWidth(); }
#line 6905 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 253: /* $@44: %empty  */
#line 2655 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval)); }
#line 6911 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 254: /* dc_layer_table: K_TABLEENTRIES int_number $@44 int_number_list ';'  */
#line 2657 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addDcTableEntry(); }
#line 6917 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 256: /* int_number_list: int_number_list int_number  */
#line 2661 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval)); }
#line 6923 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 258: /* number_list: number_list NUMBER  */
#line 2665 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval)); }
#line 6929 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 261: /* layer_prop: T_STRING T_STRING  */
#line 2674 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        char propTp;
        propTp = lefSettings->lefProps.lefrLayerProp.propType((yyvsp[-1].string));
        lefData->lefrLayer.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 6941 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 262: /* layer_prop: T_STRING QSTRING  */
#line 2682 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        char propTp;
        propTp = lefSettings->lefProps.lefrLayerProp.propType((yyvsp[-1].string));
        lefData->lefrLayer.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 6953 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 263: /* layer_prop: T_STRING NUMBER  */
#line 2690 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      char temp[32];
      sprintf(temp, "%.11g", (yyvsp[0].dval));
      if (lefCallbacks->LayerCbk) {
        char propTp;
        propTp = lefSettings->lefProps.lefrLayerProp.propType((yyvsp[-1].string));
        lefData->lefrLayer.addNumProp((yyvsp[-1].string), (yyvsp[0].dval), temp, propTp);
      }
    }
#line 6967 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 264: /* current_density_pwl_list: current_density_pwl  */
#line 2702 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 6973 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 265: /* current_density_pwl_list: current_density_pwl_list current_density_pwl  */
#line 2704 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 6979 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 266: /* current_density_pwl: '(' int_number int_number ')'  */
#line 2707 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCurrentPoint((yyvsp[-2].dval), (yyvsp[-1].dval)); }
#line 6985 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 269: /* cap_point: '(' int_number int_number ')'  */
#line 2715 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.setCapacitancePoint((yyvsp[-2].dval), (yyvsp[-1].dval)); }
#line 6991 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 271: /* res_points: res_points res_point  */
#line 2720 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 6997 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 272: /* res_point: '(' int_number int_number ')'  */
#line 2723 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.setResistancePoint((yyvsp[-2].dval), (yyvsp[-1].dval)); }
#line 7003 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 273: /* layer_type: K_ROUTING  */
#line 2726 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"ROUTING"; lefData->layerRout = 1;}
#line 7009 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 274: /* layer_type: K_CUT  */
#line 2727 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"CUT"; lefData->layerCut = 1;}
#line 7015 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 275: /* layer_type: K_OVERLAP  */
#line 2728 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"OVERLAP"; lefData->layerMastOver = 1;}
#line 7021 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 276: /* layer_type: K_MASTERSLICE  */
#line 2729 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"MASTERSLICE"; lefData->layerMastOver = 1;}
#line 7027 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 277: /* layer_type: K_VIRTUAL  */
#line 2730 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"VIRTUAL";}
#line 7033 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 278: /* layer_type: K_IMPLANT  */
#line 2731 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"IMPLANT";}
#line 7039 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 279: /* layer_direction: K_HORIZONTAL  */
#line 2734 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"HORIZONTAL";}
#line 7045 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 280: /* layer_direction: K_VERTICAL  */
#line 2735 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"VERTICAL";}
#line 7051 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 281: /* layer_direction: K_DIAG45  */
#line 2736 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"DIAG45";}
#line 7057 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 282: /* layer_direction: K_DIAG135  */
#line 2737 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"DIAG135";}
#line 7063 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 284: /* layer_minen_width: K_WIDTH int_number  */
#line 2741 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addMinenclosedareaWidth((yyvsp[0].dval));
    }
#line 7072 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 285: /* layer_oxide: K_OXIDE1  */
#line 2748 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(1);
    }
#line 7081 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 286: /* layer_oxide: K_OXIDE2  */
#line 2753 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(2);
    }
#line 7090 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 287: /* layer_oxide: K_OXIDE3  */
#line 2758 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(3);
    }
#line 7099 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 288: /* layer_oxide: K_OXIDE4  */
#line 2763 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(4);
    }
#line 7108 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 289: /* layer_oxide: K_OXIDE5  */
#line 2768 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(5);
    }
#line 7117 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 290: /* layer_oxide: K_OXIDE6  */
#line 2773 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(6);
    }
#line 7126 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 291: /* layer_oxide: K_OXIDE7  */
#line 2778 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(7);
    }
#line 7135 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 292: /* layer_oxide: K_OXIDE8  */
#line 2783 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(8);
    }
#line 7144 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 293: /* layer_oxide: K_OXIDE9  */
#line 2788 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(9);
    }
#line 7153 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 294: /* layer_oxide: K_OXIDE10  */
#line 2793 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(10);
    }
#line 7162 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 295: /* layer_oxide: K_OXIDE11  */
#line 2798 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(11);
    }
#line 7171 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 296: /* layer_oxide: K_OXIDE12  */
#line 2803 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(12);
    }
#line 7180 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 297: /* layer_oxide: K_OXIDE13  */
#line 2808 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(13);
    }
#line 7189 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 298: /* layer_oxide: K_OXIDE14  */
#line 2813 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(14);
    }
#line 7198 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 299: /* layer_oxide: K_OXIDE15  */
#line 2818 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(15);
    }
#line 7207 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 300: /* layer_oxide: K_OXIDE16  */
#line 2823 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(16);
    }
#line 7216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 301: /* layer_oxide: K_OXIDE17  */
#line 2828 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(17);
    }
#line 7225 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 302: /* layer_oxide: K_OXIDE18  */
#line 2833 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(18);
    }
#line 7234 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 303: /* layer_oxide: K_OXIDE19  */
#line 2838 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(19);
    }
#line 7243 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 304: /* layer_oxide: K_OXIDE20  */
#line 2843 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(20);
    }
#line 7252 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 305: /* layer_oxide: K_OXIDE21  */
#line 2848 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(21);
    }
#line 7261 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 306: /* layer_oxide: K_OXIDE22  */
#line 2853 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(22);
    }
#line 7270 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 307: /* layer_oxide: K_OXIDE23  */
#line 2858 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(23);
    }
#line 7279 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 308: /* layer_oxide: K_OXIDE24  */
#line 2863 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(24);
    }
#line 7288 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 309: /* layer_oxide: K_OXIDE25  */
#line 2868 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(25);
    }
#line 7297 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 310: /* layer_oxide: K_OXIDE26  */
#line 2873 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(26);
    }
#line 7306 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 311: /* layer_oxide: K_OXIDE27  */
#line 2878 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(27);
    }
#line 7315 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 312: /* layer_oxide: K_OXIDE28  */
#line 2883 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(28);
    }
#line 7324 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 313: /* layer_oxide: K_OXIDE29  */
#line 2888 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(29);
    }
#line 7333 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 314: /* layer_oxide: K_OXIDE30  */
#line 2893 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(30);
    }
#line 7342 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 315: /* layer_oxide: K_OXIDE31  */
#line 2898 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(31);
    }
#line 7351 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 316: /* layer_oxide: K_OXIDE32  */
#line 2903 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->LayerCbk)
       lefData->lefrLayer.addAntennaModel(32);
    }
#line 7360 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 317: /* layer_sp_parallel_widths: %empty  */
#line 2909 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7366 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 318: /* layer_sp_parallel_widths: layer_sp_parallel_widths layer_sp_parallel_width  */
#line 2911 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7372 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 319: /* $@45: %empty  */
#line 2914 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->LayerCbk) {
         lefData->lefrLayer.addSpParallelWidth((yyvsp[0].dval));
      }
    }
#line 7382 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 320: /* layer_sp_parallel_width: K_WIDTH int_number $@45 int_number_list  */
#line 2920 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpParallelWidthSpacing(); }
#line 7388 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 321: /* layer_sp_TwoWidths: %empty  */
#line 2923 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7394 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 322: /* layer_sp_TwoWidths: layer_sp_TwoWidth layer_sp_TwoWidths  */
#line 2925 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7400 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 323: /* $@46: %empty  */
#line 2928 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       if (lefCallbacks->LayerCbk) lefData->lefrLayer.addNumber((yyvsp[0].dval));
    }
#line 7408 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 324: /* layer_sp_TwoWidth: K_WIDTH int_number layer_sp_TwoWidthsPRL int_number $@46 int_number_list  */
#line 2932 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
         lefData->lefrLayer.addSpTwoWidths((yyvsp[-4].dval), (yyvsp[-3].dval));
    }
#line 7417 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 325: /* layer_sp_TwoWidthsPRL: %empty  */
#line 2938 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
        (yyval.dval) = -1; // cannot use 0, since PRL number can be 0 
        lefData->lefrLayer.setSpTwoWidthsHasPRL(0);
    }
#line 7426 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 326: /* layer_sp_TwoWidthsPRL: K_PRL int_number  */
#line 2943 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
        (yyval.dval) = (yyvsp[0].dval); 
        lefData->lefrLayer.setSpTwoWidthsHasPRL(1);
    }
#line 7435 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 327: /* layer_sp_influence_widths: %empty  */
#line 2949 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7441 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 328: /* layer_sp_influence_widths: layer_sp_influence_widths layer_sp_influence_width  */
#line 2951 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7447 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 329: /* layer_sp_influence_width: K_WIDTH int_number K_WITHIN int_number K_SPACING int_number  */
#line 2954 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->LayerCbk) lefData->lefrLayer.addSpInfluence((yyvsp[-4].dval), (yyvsp[-2].dval), (yyvsp[0].dval)); }
#line 7453 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 330: /* maxstack_via: K_MAXVIASTACK int_number ';'  */
#line 2957 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
           lefData->lefrMaxStackVia.setMaxStackVia((int)(yyvsp[-1].dval));
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
#line 7493 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 331: /* $@47: %empty  */
#line 2992 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                     {lefData->lefDumbMode = 2; lefData->lefNoNum= 2;}
#line 7499 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 332: /* maxstack_via: K_MAXVIASTACK int_number K_RANGE $@47 T_STRING T_STRING ';'  */
#line 2994 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
           lefData->lefrMaxStackVia.setMaxStackVia((int)(yyvsp[-5].dval));
           lefData->lefrMaxStackVia.setMaxStackViaRange((yyvsp[-2].string), (yyvsp[-1].string));
           CALLBACK(lefCallbacks->MaxStackViaCbk, lefrMaxStackViaCbkType, &lefData->lefrMaxStackVia);
        }
      }
      lefData->lefrHasMaxVS = 1;
    }
#line 7528 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 333: /* $@48: %empty  */
#line 3019 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                { lefData->hasViaRule_layer = 0; }
#line 7534 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 334: /* via: start_via $@48 via_option end_via  */
#line 3020 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->ViaCbk) {
        if (lefData->ndRule) 
            lefData->nd->addViaRule(&lefData->lefrVia);
         else 
            CALLBACK(lefCallbacks->ViaCbk, lefrViaCbkType, &lefData->lefrVia);
       }
    }
#line 7547 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 335: /* via_keyword: K_VIA  */
#line 3030 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
     { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 7553 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 336: /* start_via: via_keyword T_STRING  */
#line 3033 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      // 0 is nodefault 
      if (lefCallbacks->ViaCbk) lefData->lefrVia.setName((yyvsp[0].string), 0);
      lefData->viaLayer = 0;
      lefData->numVia++;
      //strcpy(lefData->viaName, $2);
      lefData->viaName = strdup((yyvsp[0].string));
    }
#line 7566 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 337: /* start_via: via_keyword T_STRING K_DEFAULT  */
#line 3042 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      // 1 is default 
      if (lefCallbacks->ViaCbk) lefData->lefrVia.setName((yyvsp[-1].string), 1);
      lefData->viaLayer = 0;
      //strcpy(lefData->viaName, $2);
      lefData->viaName = strdup((yyvsp[-1].string));
    }
#line 7578 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 338: /* start_via: via_keyword T_STRING K_GENERATED  */
#line 3050 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      // 2 is generated 
      if (lefCallbacks->ViaCbk) lefData->lefrVia.setName((yyvsp[-1].string), 2);
      lefData->viaLayer = 0;
      //strcpy(lefData->viaName, $2);
      lefData->viaName = strdup((yyvsp[-1].string));
    }
#line 7590 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 339: /* $@49: %empty  */
#line 3058 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 7596 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 340: /* $@50: %empty  */
#line 3060 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
           {lefData->lefDumbMode = 3; lefData->lefNoNum = 1; }
#line 7602 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 341: /* $@51: %empty  */
#line 3063 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          if (lefCallbacks->ViaCbk) lefData->lefrVia.setViaRule((yyvsp[-21].string), (yyvsp[-18].dval), (yyvsp[-17].dval), (yyvsp[-13].string), (yyvsp[-12].string), (yyvsp[-11].string),
                          (yyvsp[-8].dval), (yyvsp[-7].dval), (yyvsp[-4].dval), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval));
       lefData->viaLayer++;
       lefData->hasViaRule_layer = 1;
    }
#line 7625 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 345: /* via_viarule_option: K_ROWCOL int_number int_number ';'  */
#line 3089 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setRowCol((int)(yyvsp[-2].dval), (int)(yyvsp[-1].dval));
    }
#line 7633 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 346: /* via_viarule_option: K_ORIGIN int_number int_number ';'  */
#line 3093 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setOrigin((yyvsp[-2].dval), (yyvsp[-1].dval));
    }
#line 7641 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 347: /* via_viarule_option: K_OFFSET int_number int_number int_number int_number ';'  */
#line 3097 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setOffset((yyvsp[-4].dval), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval));
    }
#line 7649 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 348: /* $@52: %empty  */
#line 3100 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 7655 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 349: /* via_viarule_option: K_PATTERN $@52 T_STRING ';'  */
#line 3101 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       if (lefCallbacks->ViaCbk) lefData->lefrVia.setPattern((yyvsp[-1].string));
    }
#line 7663 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 355: /* via_other_option: via_foreign  */
#line 3118 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7669 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 356: /* via_other_option: via_layer_rule  */
#line 3120 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7675 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 357: /* via_other_option: K_RESISTANCE int_number ';'  */
#line 3122 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaCbk) lefData->lefrVia.setResistance((yyvsp[-1].dval)); }
#line 7681 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 358: /* $@53: %empty  */
#line 3123 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               { lefData->lefDumbMode = 1000000; }
#line 7687 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 359: /* via_other_option: K_PROPERTY $@53 via_prop_list ';'  */
#line 3124 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;
    }
#line 7694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 360: /* via_other_option: K_TOPOFSTACKONLY  */
#line 3127 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setTopOfStack();
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
              lefWarning(2019, "TOPOFSTACKONLY statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later");
    }
#line 7707 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 363: /* via_name_value_pair: T_STRING NUMBER  */
#line 3143 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      char temp[32];
      sprintf(temp, "%.11g", (yyvsp[0].dval));
      if (lefCallbacks->ViaCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaProp.propType((yyvsp[-1].string));
         lefData->lefrVia.addNumProp((yyvsp[-1].string), (yyvsp[0].dval), temp, propTp);
      }
    }
#line 7721 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 364: /* via_name_value_pair: T_STRING QSTRING  */
#line 3153 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ViaCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaProp.propType((yyvsp[-1].string));
         lefData->lefrVia.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 7733 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 365: /* via_name_value_pair: T_STRING T_STRING  */
#line 3161 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ViaCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaProp.propType((yyvsp[-1].string));
         lefData->lefrVia.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 7745 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 366: /* via_foreign: start_foreign ';'  */
#line 3171 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign((yyvsp[-1].string), 0, 0.0, 0.0, -1);
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 7758 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 367: /* via_foreign: start_foreign pt ';'  */
#line 3180 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign((yyvsp[-2].string), 1, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, -1);
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 7771 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 368: /* via_foreign: start_foreign pt orientation ';'  */
#line 3189 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign((yyvsp[-3].string), 1, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].integer));
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 7784 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 369: /* via_foreign: start_foreign orientation ';'  */
#line 3198 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->ViaCbk) lefData->lefrVia.setForeign((yyvsp[-2].string), 0, 0.0, 0.0, (yyvsp[-1].integer));
      } else
        if (lefCallbacks->ViaCbk)  // write warning only if cbk is set 
           if (lefData->viaWarnings++ < lefSettings->ViaWarnings)
             lefWarning(2020, "FOREIGN statement in VIA is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 7797 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 370: /* $@54: %empty  */
#line 3207 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {lefData->lefDumbMode = 1; lefData->lefNoNum= 1;}
#line 7803 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 371: /* start_foreign: K_FOREIGN $@54 T_STRING  */
#line 3208 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 7809 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 372: /* orientation: K_N  */
#line 3211 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 0;}
#line 7815 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 373: /* orientation: K_W  */
#line 3212 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 1;}
#line 7821 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 374: /* orientation: K_S  */
#line 3213 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 2;}
#line 7827 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 375: /* orientation: K_E  */
#line 3214 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 3;}
#line 7833 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 376: /* orientation: K_FN  */
#line 3215 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 4;}
#line 7839 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 377: /* orientation: K_FW  */
#line 3216 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 5;}
#line 7845 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 378: /* orientation: K_FS  */
#line 3217 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 6;}
#line 7851 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 379: /* orientation: K_FE  */
#line 3218 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 7;}
#line 7857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 380: /* orientation: K_R0  */
#line 3219 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 0;}
#line 7863 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 381: /* orientation: K_R90  */
#line 3220 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 1;}
#line 7869 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 382: /* orientation: K_R180  */
#line 3221 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 2;}
#line 7875 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 383: /* orientation: K_R270  */
#line 3222 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 3;}
#line 7881 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 384: /* orientation: K_MY  */
#line 3223 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 4;}
#line 7887 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 385: /* orientation: K_MYR90  */
#line 3224 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 5;}
#line 7893 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 386: /* orientation: K_MX  */
#line 3225 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 6;}
#line 7899 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 387: /* orientation: K_MXR90  */
#line 3226 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.integer) = 7;}
#line 7905 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 388: /* via_layer_rule: via_layer via_geometries  */
#line 3229 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 7911 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 389: /* $@55: %empty  */
#line 3231 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 7917 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 390: /* via_layer: K_LAYER $@55 T_STRING ';'  */
#line 3232 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ViaCbk) lefData->lefrVia.addLayer((yyvsp[-1].string));
      lefData->viaLayer++;
      lefData->hasViaRule_layer = 1;
    }
#line 7927 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 393: /* via_geometry: K_RECT maskColor pt pt ';'  */
#line 3245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->ViaCbk) {
        if (lefData->versionNum < 5.8 && (int)(yyvsp[-3].integer) > 0) {
          if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefError(2081, "MASK information can only be defined with version 5.8");
              CHKERR(); 
            }           
        } else {
          lefData->lefrVia.addRectToLayer((int)(yyvsp[-3].integer), (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y);
        }
      }
    }
#line 7944 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 394: /* $@56: %empty  */
#line 3258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->lefrGeometriesPtr = (lefiGeometries*)lefMalloc(sizeof(lefiGeometries));
      lefData->lefrGeometriesPtr->Init();
      lefData->lefrDoGeometries = 1;
    }
#line 7954 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 395: /* via_geometry: K_POLYGON maskColor $@56 firstPt nextPt nextPt otherPts ';'  */
#line 3264 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->ViaCbk) {
        if (lefData->versionNum < 5.8 && (yyvsp[-6].integer) > 0) {
          if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefError(2083, "Color mask information can only be defined with version 5.8.");
              CHKERR(); 
            }           
        } else {
            lefData->lefrGeometriesPtr->addPolygon((int)(yyvsp[-6].integer));
            lefData->lefrVia.addPolyToLayer((int)(yyvsp[-6].integer), lefData->lefrGeometriesPtr);   // 5.6
        }
      }
      lefData->lefrGeometriesPtr->clearPolyItems(); // free items fields
      lefFree(lefData->lefrGeometriesPtr); // Don't need anymore, poly data has
      lefData->lefrDoGeometries = 0;       // copied
    }
#line 7975 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 396: /* $@57: %empty  */
#line 3281 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 7981 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 397: /* end_via: K_END $@57 T_STRING  */
#line 3282 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      // 10/17/2001 - Wanda da Rosa, PCR 404149
      //              Error if no layer in via
      if (!lefData->viaLayer) {
         if (lefCallbacks->ViaCbk) {  // write error only if cbk is set 
            if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "A LAYER statement is missing in the VIA %s.\nAt least one LAYERis required per VIA statement.", (yyvsp[0].string));
              lefError(1606, lefData->outMsg);
              lefFree(lefData->outMsg);
              CHKERR();
            }
         }
      }
      if (strcmp(lefData->viaName, (yyvsp[0].string)) != 0) {
         if (lefCallbacks->ViaCbk) { // write error only if cbk is set 
            if (lefData->viaWarnings++ < lefSettings->ViaWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                "END VIA name %s is different from the VIA name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->viaName);
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
#line 8018 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 398: /* $@58: %empty  */
#line 3315 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                            { lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 8024 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 399: /* viarule_keyword: K_VIARULE $@58 T_STRING  */
#line 3316 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setName((yyvsp[0].string));
      lefData->viaRuleLayer = 0;
      //strcpy(lefData->viaRuleName, $3);
      lefData->viaRuleName = strdup((yyvsp[0].string));
      lefData->isGenerate = 0;
    }
#line 8036 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 400: /* viarule: viarule_keyword viarule_layer_list via_names opt_viarule_props end_viarule  */
#line 3326 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8056 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 401: /* $@59: %empty  */
#line 3344 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->isGenerate = 1;
    }
#line 8064 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 402: /* viarule_generate: viarule_keyword K_GENERATE viarule_generate_default $@59 viarule_layer_list opt_viarule_props end_viarule  */
#line 3348 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8091 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 404: /* viarule_generate_default: K_DEFAULT  */
#line 3373 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8111 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 411: /* $@60: %empty  */
#line 3404 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                         { lefData->lefDumbMode = 10000000;}
#line 8117 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 412: /* viarule_prop: K_PROPERTY $@60 viarule_prop_list ';'  */
#line 3405 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;
    }
#line 8124 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 415: /* viarule_prop: T_STRING T_STRING  */
#line 3415 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ViaRuleCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaRuleProp.propType((yyvsp[-1].string));
         lefData->lefrViaRule.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 8136 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 416: /* viarule_prop: T_STRING QSTRING  */
#line 3423 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ViaRuleCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaRuleProp.propType((yyvsp[-1].string));
         lefData->lefrViaRule.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 8148 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 417: /* viarule_prop: T_STRING NUMBER  */
#line 3431 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      char temp[32];
      sprintf(temp, "%.11g", (yyvsp[0].dval));
      if (lefCallbacks->ViaRuleCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrViaRuleProp.propType((yyvsp[-1].string));
         lefData->lefrViaRule.addNumProp((yyvsp[-1].string), (yyvsp[0].dval), temp, propTp);
      }
    }
#line 8162 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 418: /* viarule_layer: viarule_layer_name viarule_layer_options  */
#line 3442 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8195 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 421: /* via_name: via_keyword T_STRING ';'  */
#line 3478 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.addViaName((yyvsp[-1].string)); }
#line 8201 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 422: /* $@61: %empty  */
#line 3480 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                            {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 8207 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 423: /* viarule_layer_name: K_LAYER $@61 T_STRING ';'  */
#line 3481 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setLayer((yyvsp[-1].string));
      lefData->viaRuleHasDir = 0;
      lefData->viaRuleHasEnc = 0;
    }
#line 8216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 426: /* viarule_layer_option: K_DIRECTION K_HORIZONTAL ';'  */
#line 3493 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8239 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 427: /* viarule_layer_option: K_DIRECTION K_VERTICAL ';'  */
#line 3512 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8262 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 428: /* viarule_layer_option: K_ENCLOSURE int_number int_number ';'  */
#line 3531 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
         if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setEnclosure((yyvsp[-2].dval), (yyvsp[-1].dval));
      }
      lefData->viaRuleHasEnc = 1;
    }
#line 8302 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 429: /* viarule_layer_option: K_WIDTH int_number K_TO int_number ';'  */
#line 3567 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setWidth((yyvsp[-3].dval),(yyvsp[-1].dval)); }
#line 8308 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 430: /* viarule_layer_option: K_RECT pt pt ';'  */
#line 3569 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaRuleCbk)
        lefData->lefrViaRule.setRect((yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y); }
#line 8315 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 431: /* viarule_layer_option: K_SPACING int_number K_BY int_number ';'  */
#line 3572 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setSpacing((yyvsp[-3].dval),(yyvsp[-1].dval)); }
#line 8321 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 432: /* viarule_layer_option: K_RESISTANCE int_number ';'  */
#line 3574 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setResistance((yyvsp[-1].dval)); }
#line 8327 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 433: /* viarule_layer_option: K_OVERHANG int_number ';'  */
#line 3576 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setOverhang((yyvsp[-1].dval));
      } else {
        if (lefCallbacks->ViaRuleCbk)  // write warning only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
              lefWarning(2023, "OVERHANG statement will be translated into similar ENCLOSURE rule");
        // In 5.6 & later, set it to either ENCLOSURE overhang1 or overhang2
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setOverhangToEnclosure((yyvsp[-1].dval));
      }
    }
#line 8361 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 434: /* viarule_layer_option: K_METALOVERHANG int_number ';'  */
#line 3606 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        if (lefCallbacks->ViaRuleCbk) lefData->lefrViaRule.setMetalOverhang((yyvsp[-1].dval));
      } else
        if (lefCallbacks->ViaRuleCbk)  // write warning only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings)
             lefWarning(2024, "METALOVERHANG statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 8392 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 435: /* $@62: %empty  */
#line 3633 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 8398 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 436: /* end_viarule: K_END $@62 T_STRING  */
#line 3634 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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

      if (strcmp(lefData->viaRuleName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->ViaRuleCbk) {  // write error only if cbk is set 
           if (lefData->viaRuleWarnings++ < lefSettings->ViaRuleWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END VIARULE name %s is different from the VIARULE name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->viaRuleName);
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
#line 8435 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 437: /* spacing_rule: start_spacing spacings end_spacing  */
#line 3668 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 8441 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 438: /* start_spacing: K_SPACING  */
#line 3671 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8466 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 439: /* end_spacing: K_END K_SPACING  */
#line 3693 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8482 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 442: /* spacing: samenet_keyword T_STRING T_STRING int_number ';'  */
#line 3711 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if ((lefData->versionNum < 5.6) || (!lefData->ndRule)) {
        if (lefData->versionNum < 5.7) {
          if (lefCallbacks->SpacingCbk) {
            lefData->lefrSpacing.set((yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].dval), 0);
            if (lefData->ndRule)
                lefData->nd->addSpacingRule(&lefData->lefrSpacing);
            else 
                CALLBACK(lefCallbacks->SpacingCbk, lefrSpacingCbkType, &lefData->lefrSpacing);            
          }
        }
      }
    }
#line 8500 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 443: /* spacing: samenet_keyword T_STRING T_STRING int_number K_STACK ';'  */
#line 3725 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if ((lefData->versionNum < 5.6) || (!lefData->ndRule)) {
        if (lefData->versionNum < 5.7) {
          if (lefCallbacks->SpacingCbk) {
            lefData->lefrSpacing.set((yyvsp[-4].string), (yyvsp[-3].string), (yyvsp[-2].dval), 1);
            if (lefData->ndRule)
                lefData->nd->addSpacingRule(&lefData->lefrSpacing);
            else 
                CALLBACK(lefCallbacks->SpacingCbk, lefrSpacingCbkType, &lefData->lefrSpacing);    
          }
        }
      }
    }
#line 8518 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 444: /* samenet_keyword: K_SAMENET  */
#line 3741 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 2; lefData->lefNoNum = 2; lefData->hasSamenet = 1; }
#line 8524 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 445: /* maskColor: %empty  */
#line 3745 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.integer) = 0; }
#line 8530 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 446: /* maskColor: K_MASK int_number  */
#line 3747 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.integer) = (int)(yyvsp[0].dval); }
#line 8536 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 447: /* irdrop: start_irdrop ir_tables end_irdrop  */
#line 3750 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 8542 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 448: /* start_irdrop: K_IRDROP  */
#line 3753 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->IRDropBeginCbk) 
          CALLBACK(lefCallbacks->IRDropBeginCbk, lefrIRDropBeginCbkType, 0);
      } else
        if (lefCallbacks->IRDropBeginCbk) // write warning only if cbk is set 
          if (lefData->iRDropWarnings++ < lefSettings->IRDropWarnings)
            lefWarning(2026, "IRDROP statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 8556 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 449: /* end_irdrop: K_END K_IRDROP  */
#line 3764 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->IRDropEndCbk)
          CALLBACK(lefCallbacks->IRDropEndCbk, lefrIRDropEndCbkType, 0);
      }
    }
#line 8567 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 452: /* ir_table: ir_tablename ir_table_values ';'  */
#line 3778 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->IRDropCbk)
          CALLBACK(lefCallbacks->IRDropCbk, lefrIRDropCbkType, &lefData->lefrIRDrop);
      }
    }
#line 8578 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 455: /* ir_table_value: int_number int_number  */
#line 3791 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  { if (lefCallbacks->IRDropCbk) lefData->lefrIRDrop.setValues((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 8584 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 456: /* ir_tablename: K_TABLE T_STRING  */
#line 3794 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  { if (lefCallbacks->IRDropCbk) lefData->lefrIRDrop.setTableName((yyvsp[0].string)); }
#line 8590 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 457: /* minfeature: K_MINFEATURE int_number int_number ';'  */
#line 3797 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    lefData->hasMinfeature = 1;
    if (lefData->versionNum < 5.4) {
       if (lefCallbacks->MinFeatureCbk) {
         lefData->lefrMinFeature.set((yyvsp[-2].dval), (yyvsp[-1].dval));
         CALLBACK(lefCallbacks->MinFeatureCbk, lefrMinFeatureCbkType, &lefData->lefrMinFeature);
       }
    } else
       if (lefCallbacks->MinFeatureCbk) // write warning only if cbk is set 
          if (lefData->minFeatureWarnings++ < lefSettings->MinFeatureWarnings)
            lefWarning(2027, "MINFEATURE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }
#line 8607 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 458: /* dielectric: K_DIELECTRIC int_number ';'  */
#line 3811 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefData->versionNum < 5.4) {
       if (lefCallbacks->DielectricCbk)
         CALLBACK(lefCallbacks->DielectricCbk, lefrDielectricCbkType, (yyvsp[-1].dval));
    } else
       if (lefCallbacks->DielectricCbk) // write warning only if cbk is set 
         if (lefData->dielectricWarnings++ < lefSettings->DielectricWarnings)
           lefWarning(2028, "DIELECTRIC statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }
#line 8621 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 459: /* $@63: %empty  */
#line 3821 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                  {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 8627 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 460: /* $@64: %empty  */
#line 3822 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    (void)lefSetNonDefault((yyvsp[0].string));
    if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.setName((yyvsp[0].string));
    lefData->ndLayer = 0;
    lefData->ndRule = 1;
    lefData->numVia = 0;
    //strcpy(lefData->nonDefaultRuleName, $3);
    lefData->nonDefaultRuleName = strdup((yyvsp[0].string));
  }
#line 8641 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 461: /* $@65: %empty  */
#line 3832 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
           {lefData->lefNdRule = 1;}
#line 8647 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 462: /* nondefault_rule: K_NONDEFAULTRULE $@63 T_STRING $@64 nd_hardspacing nd_rules $@65 end_nd_rule  */
#line 3833 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8680 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 463: /* end_nd_rule: K_END  */
#line 3863 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if ((lefData->nonDefaultRuleName) && (*lefData->nonDefaultRuleName != '\0'))
        lefFree(lefData->nonDefaultRuleName);
    }
#line 8689 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 464: /* end_nd_rule: K_END T_STRING  */
#line 3868 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (strcmp(lefData->nonDefaultRuleName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
          if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
             lefData->outMsg = (char*)lefMalloc(10000);
             sprintf (lefData->outMsg,
                "END NONDEFAULTRULE name %s is different from the NONDEFAULTRULE name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->nonDefaultRuleName);
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
#line 8712 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 466: /* nd_hardspacing: K_HARDSPACING ';'  */
#line 3891 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 8733 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 476: /* usevia: K_USEVIA T_STRING ';'  */
#line 3924 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
             lefData->lefrNonDefault.addUseVia((yyvsp[-1].string));
       }
    }
#line 8753 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 477: /* useviarule: K_USEVIARULE T_STRING ';'  */
#line 3941 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
             lefData->lefrNonDefault.addUseViaRule((yyvsp[-1].string));
       }
    }
#line 8775 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 478: /* mincuts: K_MINCUTS T_STRING int_number ';'  */
#line 3960 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
             lefData->lefrNonDefault.addMinCuts((yyvsp[-2].string), (int)(yyvsp[-1].dval));
       }
    }
#line 8797 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 479: /* $@66: %empty  */
#line 3978 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    { lefData->lefDumbMode = 10000000;}
#line 8803 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 480: /* nd_prop: K_PROPERTY $@66 nd_prop_list ';'  */
#line 3979 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;
    }
#line 8810 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 483: /* nd_prop: T_STRING T_STRING  */
#line 3989 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->NonDefaultCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrNondefProp.propType((yyvsp[-1].string));
         lefData->lefrNonDefault.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 8822 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 484: /* nd_prop: T_STRING QSTRING  */
#line 3997 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->NonDefaultCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrNondefProp.propType((yyvsp[-1].string));
         lefData->lefrNonDefault.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 8834 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 485: /* nd_prop: T_STRING NUMBER  */
#line 4005 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->NonDefaultCbk) {
         char temp[32];
         char propTp;
         sprintf(temp, "%.11g", (yyvsp[0].dval));
         propTp = lefSettings->lefProps.lefrNondefProp.propType((yyvsp[-1].string));
         lefData->lefrNonDefault.addNumProp((yyvsp[-1].string), (yyvsp[0].dval), temp, propTp);
      }
    }
#line 8848 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 486: /* $@67: %empty  */
#line 4015 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 8854 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 487: /* $@68: %empty  */
#line 4016 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.addLayer((yyvsp[0].string));
    lefData->ndLayer++;
    //strcpy(lefData->layerName, $3);
    lefData->layerName = strdup((yyvsp[0].string));
    lefData->ndLayerWidth = 0;
    lefData->ndLayerSpace = 0;
  }
#line 8867 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 488: /* $@69: %empty  */
#line 4025 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  { 
    lefData->ndLayerWidth = 1;
    if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.addWidth((yyvsp[-1].dval));
  }
#line 8876 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 489: /* $@70: %empty  */
#line 4029 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 8882 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 490: /* nd_layer: K_LAYER $@67 T_STRING $@68 K_WIDTH int_number ';' $@69 nd_layer_stmts K_END $@70 T_STRING  */
#line 4030 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (strcmp(lefData->layerName, (yyvsp[0].string)) != 0) {
      if (lefCallbacks->NonDefaultCbk) { // write error only if cbk is set 
         if (lefData->nonDefaultWarnings++ < lefSettings->NonDefaultWarnings) {
            lefData->outMsg = (char*)lefMalloc(10000);
            sprintf (lefData->outMsg,
               "END LAYER name %s is different from the LAYER name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[-9].string), lefData->layerName);
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
#line 8926 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 493: /* nd_layer_stmt: K_SPACING int_number ';'  */
#line 4078 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->ndLayerSpace = 1;
      if (lefCallbacks->NonDefaultCbk) lefData->lefrNonDefault.addSpacing((yyvsp[-1].dval));
    }
#line 8935 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 494: /* nd_layer_stmt: K_WIREEXTENSION int_number ';'  */
#line 4083 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NonDefaultCbk)
         lefData->lefrNonDefault.addWireExtension((yyvsp[-1].dval)); }
#line 8942 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 495: /* nd_layer_stmt: K_RESISTANCE K_RPERSQ int_number ';'  */
#line 4086 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->ignoreVersion) {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addResistance((yyvsp[-1].dval));
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
         lefData->lefrNonDefault.addResistance((yyvsp[-1].dval));
    }
#line 8969 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 496: /* nd_layer_stmt: K_CAPACITANCE K_CPERSQDIST int_number ';'  */
#line 4110 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->ignoreVersion) {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addCapacitance((yyvsp[-1].dval));
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
         lefData->lefrNonDefault.addCapacitance((yyvsp[-1].dval));
    }
#line 8996 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 497: /* nd_layer_stmt: K_EDGECAPACITANCE int_number ';'  */
#line 4133 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->ignoreVersion) {
         if (lefCallbacks->NonDefaultCbk)
            lefData->lefrNonDefault.addEdgeCap((yyvsp[-1].dval));
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
         lefData->lefrNonDefault.addEdgeCap((yyvsp[-1].dval));
    }
#line 9023 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 498: /* nd_layer_stmt: K_DIAGWIDTH int_number ';'  */
#line 4156 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
            lefData->lefrNonDefault.addDiagWidth((yyvsp[-1].dval));
      }
    }
#line 9045 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 499: /* site: start_site site_options end_site  */
#line 4175 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->SiteCbk)
        CALLBACK(lefCallbacks->SiteCbk, lefrSiteCbkType, &lefData->lefrSite);
    }
#line 9054 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 500: /* $@71: %empty  */
#line 4180 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 9060 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 501: /* start_site: K_SITE $@71 T_STRING  */
#line 4181 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->SiteCbk) lefData->lefrSite.setName((yyvsp[0].string));
      //strcpy(lefData->siteName, $3);
      lefData->siteName = strdup((yyvsp[0].string));
      lefData->hasSiteClass = 0;
      lefData->hasSiteSize = 0;
      lefData->hasSite = 1;
    }
#line 9073 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 502: /* $@72: %empty  */
#line 4190 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 9079 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 503: /* end_site: K_END $@72 T_STRING  */
#line 4191 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (strcmp(lefData->siteName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->SiteCbk) { // write error only if cbk is set 
           if (lefData->siteWarnings++ < lefSettings->SiteWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END SITE name %s is different from the SITE name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->siteName);
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
#line 9113 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 506: /* site_option: K_SIZE int_number K_BY int_number ';'  */
#line 4228 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {

      if (lefCallbacks->SiteCbk) lefData->lefrSite.setSize((yyvsp[-3].dval),(yyvsp[-1].dval));
      lefData->hasSiteSize = 1;
    }
#line 9123 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 507: /* site_option: site_symmetry_statement  */
#line 4234 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 9129 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 508: /* site_option: site_class  */
#line 4236 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->SiteCbk) lefData->lefrSite.setClass((yyvsp[0].string));
      lefData->hasSiteClass = 1;
    }
#line 9138 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 509: /* site_option: site_rowpattern_statement  */
#line 4241 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 9144 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 510: /* site_class: K_CLASS K_PAD ';'  */
#line 4244 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"PAD"; }
#line 9150 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 511: /* site_class: K_CLASS K_CORE ';'  */
#line 4245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                        {(yyval.string) = (char*)"CORE"; }
#line 9156 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 512: /* site_class: K_CLASS K_VIRTUAL ';'  */
#line 4246 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                           {(yyval.string) = (char*)"VIRTUAL"; }
#line 9162 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 513: /* site_symmetry_statement: K_SYMMETRY site_symmetries ';'  */
#line 4249 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 9168 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 516: /* site_symmetry: K_X  */
#line 4258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.setXSymmetry(); }
#line 9174 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 517: /* site_symmetry: K_Y  */
#line 4260 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.setYSymmetry(); }
#line 9180 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 518: /* site_symmetry: K_R90  */
#line 4262 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.set90Symmetry(); }
#line 9186 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 519: /* $@73: %empty  */
#line 4264 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                        {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 9192 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 520: /* site_rowpattern_statement: K_ROWPATTERN $@73 site_rowpatterns ';'  */
#line 4266 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 9198 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 523: /* $@74: %empty  */
#line 4273 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                      {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 9204 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 524: /* site_rowpattern: T_STRING orientation $@74  */
#line 4274 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->SiteCbk) lefData->lefrSite.addRowPattern((yyvsp[-2].string), (yyvsp[-1].integer)); }
#line 9210 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 525: /* pt: int_number int_number  */
#line 4278 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.pt).x = (yyvsp[-1].dval); (yyval.pt).y = (yyvsp[0].dval); }
#line 9216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 526: /* pt: '(' int_number int_number ')'  */
#line 4280 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.pt).x = (yyvsp[-2].dval); (yyval.pt).y = (yyvsp[-1].dval); }
#line 9222 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 527: /* $@75: %empty  */
#line 4283 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->MacroCbk)
        CALLBACK(lefCallbacks->MacroCbk, lefrMacroCbkType, &lefData->lefrMacro);
      lefData->lefrDoSite = 0;
    }
#line 9232 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 529: /* $@76: %empty  */
#line 4290 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                     {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 9238 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 530: /* start_macro: K_MACRO $@76 T_STRING  */
#line 4291 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        lefData->lefrMacro.setName((yyvsp[0].string));
        CALLBACK(lefCallbacks->MacroBeginCbk, lefrMacroBeginCbkType, (yyvsp[0].string));
      }
      //strcpy(lefData->macroName, $3);
      lefData->macroName = strdup((yyvsp[0].string));
    }
#line 9259 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 531: /* $@77: %empty  */
#line 4308 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 9265 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 532: /* end_macro: K_END $@77 T_STRING  */
#line 4309 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (strcmp(lefData->macroName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->MacroEndCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END MACRO name %s is different from the MACRO name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->macroName);
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
        CALLBACK(lefCallbacks->MacroEndCbk, lefrMacroEndCbkType, (yyvsp[0].string));
    }
#line 9290 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 540: /* macro_option: macro_fixedMask  */
#line 4342 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9296 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 541: /* macro_option: macro_origin  */
#line 4344 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9302 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 542: /* macro_option: macro_power  */
#line 4346 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9308 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 543: /* macro_option: macro_foreign  */
#line 4348 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9314 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 546: /* macro_option: macro_size  */
#line 4352 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9320 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 547: /* macro_option: macro_site  */
#line 4354 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9326 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 548: /* macro_option: macro_pin  */
#line 4356 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9332 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 549: /* macro_option: K_FUNCTION K_BUFFER ';'  */
#line 4358 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setBuffer(); }
#line 9338 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 550: /* macro_option: K_FUNCTION K_INVERTER ';'  */
#line 4360 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setInverter(); }
#line 9344 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 551: /* macro_option: macro_obs  */
#line 4362 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9350 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 552: /* macro_option: macro_density  */
#line 4364 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9356 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 553: /* macro_option: macro_clocktype  */
#line 4366 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9362 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 554: /* macro_option: timing  */
#line 4368 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { }
#line 9368 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 555: /* $@78: %empty  */
#line 4369 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode = 1000000; }
#line 9374 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 556: /* macro_option: K_PROPERTY $@78 macro_prop_list ';'  */
#line 4370 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      { lefData->lefDumbMode = 0;
      }
#line 9381 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 559: /* macro_symmetry_statement: K_SYMMETRY macro_symmetries ';'  */
#line 4379 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->siteDef) { // SITE is defined before SYMMETRY 
          // pcr 283846 suppress warning 
          if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
             if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
               lefWarning(2032, "A SITE statement is defined before SYMMETRY statement.\nTo avoid this warning in the future, define SITE after SYMMETRY");
      }
      lefData->symDef = 1;
    }
#line 9395 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 562: /* macro_symmetry: K_X  */
#line 4396 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setXSymmetry(); }
#line 9401 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 563: /* macro_symmetry: K_Y  */
#line 4398 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setYSymmetry(); }
#line 9407 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 564: /* macro_symmetry: K_R90  */
#line 4400 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.set90Symmetry(); }
#line 9413 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 565: /* macro_name_value_pair: T_STRING NUMBER  */
#line 4404 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      char temp[32];
      sprintf(temp, "%.11g", (yyvsp[0].dval));
      if (lefCallbacks->MacroCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrMacroProp.propType((yyvsp[-1].string));
         lefData->lefrMacro.setNumProperty((yyvsp[-1].string), (yyvsp[0].dval), temp,  propTp);
      }
    }
#line 9427 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 566: /* macro_name_value_pair: T_STRING QSTRING  */
#line 4414 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->MacroCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrMacroProp.propType((yyvsp[-1].string));
         lefData->lefrMacro.setProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 9439 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 567: /* macro_name_value_pair: T_STRING T_STRING  */
#line 4422 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->MacroCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrMacroProp.propType((yyvsp[-1].string));
         lefData->lefrMacro.setProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 9451 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 568: /* macro_class: K_CLASS class_type ';'  */
#line 4431 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
       if (lefCallbacks->MacroCbk) lefData->lefrMacro.setClass((yyvsp[-1].string));
       if (lefCallbacks->MacroClassTypeCbk)
          CALLBACK(lefCallbacks->MacroClassTypeCbk, lefrMacroClassTypeCbkType, (yyvsp[-1].string));
    }
#line 9461 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 569: /* class_type: K_COVER  */
#line 4438 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {(yyval.string) = (char*)"COVER"; }
#line 9467 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 570: /* class_type: K_COVER K_BUMP  */
#line 4440 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"COVER BUMP";
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
#line 9490 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 571: /* class_type: K_RING  */
#line 4458 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.string) = (char*)"RING"; }
#line 9496 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 572: /* class_type: K_BLOCK  */
#line 4459 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.string) = (char*)"BLOCK"; }
#line 9502 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 573: /* class_type: K_BLOCK K_BLACKBOX  */
#line 4461 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"BLOCK BLACKBOX";
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
#line 9525 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 574: /* class_type: K_BLOCK K_SOFT  */
#line 4480 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->ignoreVersion) {
        (yyval.string) = (char*)"BLOCK SOFT";
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
        (yyval.string) = (char*)"BLOCK SOFT";
    }
#line 9548 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 575: /* class_type: K_NONE  */
#line 4498 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.string) = (char*)"NONE"; }
#line 9554 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 576: /* class_type: K_BUMP  */
#line 4500 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {
        if (lefData->versionNum < 5.7) {
          lefData->outMsg = (char*)lefMalloc(10000);
          sprintf(lefData->outMsg,
            "BUMP is a version 5.7 or later syntax.\nYour lef file is defined with version %g.", lefData->versionNum);
          lefError(1698, lefData->outMsg);
          lefFree(lefData->outMsg);
          CHKERR();
        }
       
        (yyval.string) = (char*)"BUMP";
     }
#line 9571 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 577: /* class_type: K_PAD  */
#line 4512 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.string) = (char*)"PAD"; }
#line 9577 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 578: /* class_type: K_VIRTUAL  */
#line 4513 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.string) = (char*)"VIRTUAL"; }
#line 9583 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 579: /* class_type: K_PAD pad_type  */
#line 4515 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {  sprintf(lefData->temp_name, "PAD %s", (yyvsp[0].string));
        (yyval.string) = lefData->temp_name; 
        if (lefData->versionNum < 5.5) {
           if (strcmp("AREAIO", (yyvsp[0].string)) != 0) {
             sprintf(lefData->temp_name, "PAD %s", (yyvsp[0].string));
             (yyval.string) = lefData->temp_name; 
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
#line 9610 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 580: /* class_type: K_CORE  */
#line 4537 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {(yyval.string) = (char*)"CORE"; }
#line 9616 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 581: /* class_type: K_CORNER  */
#line 4539 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {(yyval.string) = (char*)"CORNER";
      // This token is NOT in the spec but has shown up in 
      // some lef files.  This exception came from LEFOUT
      // in 'frameworks'
      }
#line 9626 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 582: /* class_type: K_CORE core_type  */
#line 4545 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {sprintf(lefData->temp_name, "CORE %s", (yyvsp[0].string));
      (yyval.string) = lefData->temp_name;}
#line 9633 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 583: /* class_type: K_ENDCAP endcap_type  */
#line 4548 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
      {sprintf(lefData->temp_name, "ENDCAP %s", (yyvsp[0].string));
      (yyval.string) = lefData->temp_name;}
#line 9640 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 584: /* pad_type: K_INPUT  */
#line 4552 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                  {(yyval.string) = (char*)"INPUT";}
#line 9646 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 585: /* pad_type: K_OUTPUT  */
#line 4553 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"OUTPUT";}
#line 9652 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 586: /* pad_type: K_INOUT  */
#line 4554 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"INOUT";}
#line 9658 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 587: /* pad_type: K_POWER  */
#line 4555 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"POWER";}
#line 9664 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 588: /* pad_type: K_SPACER  */
#line 4556 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"SPACER";}
#line 9670 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 589: /* pad_type: K_AREAIO  */
#line 4557 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"AREAIO";}
#line 9676 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 590: /* core_type: K_FEEDTHRU  */
#line 4560 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"FEEDTHRU";}
#line 9682 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 591: /* core_type: K_TIEHIGH  */
#line 4561 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"TIEHIGH";}
#line 9688 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 592: /* core_type: K_TIELOW  */
#line 4562 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                    {(yyval.string) = (char*)"TIELOW";}
#line 9694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 593: /* core_type: K_SPACER  */
#line 4564 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      (yyval.string) = (char*)"SPACER";

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
#line 9715 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 594: /* core_type: K_ANTENNACELL  */
#line 4581 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      (yyval.string) = (char*)"ANTENNACELL";

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
#line 9736 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 595: /* core_type: K_WELLTAP  */
#line 4598 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      (yyval.string) = (char*)"WELLTAP";

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
#line 9757 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 596: /* endcap_type: K_PRE  */
#line 4616 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"PRE";}
#line 9763 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 597: /* endcap_type: K_POST  */
#line 4617 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {(yyval.string) = (char*)"POST";}
#line 9769 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 598: /* endcap_type: K_TOPLEFT  */
#line 4618 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                      {(yyval.string) = (char*)"TOPLEFT";}
#line 9775 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 599: /* endcap_type: K_TOPRIGHT  */
#line 4619 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       {(yyval.string) = (char*)"TOPRIGHT";}
#line 9781 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 600: /* endcap_type: K_BOTTOMLEFT  */
#line 4620 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                         {(yyval.string) = (char*)"BOTTOMLEFT";}
#line 9787 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 601: /* endcap_type: K_BOTTOMRIGHT  */
#line 4621 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                         {(yyval.string) = (char*)"BOTTOMRIGHT";}
#line 9793 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 602: /* macro_generator: K_GENERATOR T_STRING ';'  */
#line 4624 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setGenerator((yyvsp[-1].string)); }
#line 9799 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 603: /* macro_generate: K_GENERATE T_STRING T_STRING ';'  */
#line 4627 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setGenerate((yyvsp[-2].string), (yyvsp[-1].string)); }
#line 9805 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 604: /* macro_source: K_SOURCE K_USER ';'  */
#line 4631 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSource("USER");
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2036, "SOURCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 9818 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 605: /* macro_source: K_SOURCE K_GENERATE ';'  */
#line 4640 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSource("GENERATE");
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2037, "SOURCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 9831 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 606: /* macro_source: K_SOURCE K_BLOCK ';'  */
#line 4649 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSource("BLOCK");
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2037, "SOURCE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 9844 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 607: /* macro_power: K_POWER int_number ';'  */
#line 4659 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setPower((yyvsp[-1].dval));
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2038, "MACRO POWER statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 9857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 608: /* macro_origin: K_ORIGIN pt ';'  */
#line 4669 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
       if (lefCallbacks->MacroCbk) lefData->lefrMacro.setOrigin((yyvsp[-1].pt).x, (yyvsp[-1].pt).y);
       if (lefCallbacks->MacroOriginCbk) {
          lefData->macroNum.x = (yyvsp[-1].pt).x; 
          lefData->macroNum.y = (yyvsp[-1].pt).y; 
          CALLBACK(lefCallbacks->MacroOriginCbk, lefrMacroOriginCbkType, lefData->macroNum);
       }
    }
#line 9899 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 609: /* macro_foreign: start_foreign ';'  */
#line 4709 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign((yyvsp[-1].string), 0, 0.0, 0.0, -1);
      }
      
      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign((yyvsp[-1].string), 0, 0.0, 0.0, 0, 0);
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      }  
    }
#line 9914 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 610: /* macro_foreign: start_foreign pt ';'  */
#line 4720 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign((yyvsp[-2].string), 1, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, -1);
      }
      
      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign((yyvsp[-2].string), 1, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, 0, 0);
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      }  
    }
#line 9929 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 611: /* macro_foreign: start_foreign pt orientation ';'  */
#line 4731 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign((yyvsp[-3].string), 1, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].integer));
      }
      
      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign((yyvsp[-3].string), 1, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, 1, (yyvsp[-1].integer));
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      } 
    }
#line 9944 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 612: /* macro_foreign: start_foreign orientation ';'  */
#line 4742 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.addForeign((yyvsp[-2].string), 0, 0.0, 0.0, (yyvsp[-1].integer));
      }

      if (lefCallbacks->MacroForeignCbk) {
        lefiMacroForeign foreign((yyvsp[-2].string), 0, 0.0, 0.0, 1, (yyvsp[-1].integer));
        CALLBACK(lefCallbacks->MacroForeignCbk, lefrMacroForeignCbkType, &foreign);
      } 
    }
#line 9959 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 613: /* macro_fixedMask: K_FIXEDMASK ';'  */
#line 4755 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
   {   
       if (lefCallbacks->MacroCbk && lefData->versionNum >= 5.8) {
          lefData->lefrMacro.setFixedMask(1);
       }
       if (lefCallbacks->MacroFixedMaskCbk) {
          CALLBACK(lefCallbacks->MacroFixedMaskCbk, lefrMacroFixedMaskCbkType, 1);
       }        
    }
#line 9972 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 614: /* $@79: %empty  */
#line 4764 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 9978 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 615: /* macro_eeq: K_EEQ $@79 T_STRING ';'  */
#line 4765 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setEEQ((yyvsp[-1].string)); }
#line 9984 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 616: /* $@80: %empty  */
#line 4767 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 9990 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 617: /* macro_leq: K_LEQ $@80 T_STRING ';'  */
#line 4768 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->MacroCbk) lefData->lefrMacro.setLEQ((yyvsp[-1].string));
      } else
        if (lefCallbacks->MacroCbk) // write warning only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings)
             lefWarning(2042, "LEQ statement in MACRO is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10003 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 618: /* macro_site: macro_site_word T_STRING ';'  */
#line 4779 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->MacroCbk) {
        lefData->lefrMacro.setSiteName((yyvsp[-1].string));
      }

      if (lefCallbacks->MacroSiteCbk) {
        lefiMacroSite site((yyvsp[-1].string), 0);
        CALLBACK(lefCallbacks->MacroSiteCbk, lefrMacroSiteCbkType, &site);
      }
    }
#line 10018 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 619: /* macro_site: macro_site_word sitePattern ';'  */
#line 4790 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 10037 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 620: /* macro_site_word: K_SITE  */
#line 4806 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; lefData->siteDef = 1;
        if (lefCallbacks->MacroCbk) lefData->lefrDoSite = 1; }
#line 10044 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 621: /* site_word: K_SITE  */
#line 4810 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 10050 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 622: /* macro_size: K_SIZE int_number K_BY int_number ';'  */
#line 4813 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->siteDef) { // SITE is defined before SIZE 
      }
      lefData->sizeDef = 1;
      if (lefCallbacks->MacroCbk) lefData->lefrMacro.setSize((yyvsp[-3].dval), (yyvsp[-1].dval));
      if (lefCallbacks->MacroSizeCbk) {
         lefData->macroNum.x = (yyvsp[-3].dval); 
         lefData->macroNum.y = (yyvsp[-1].dval); 
         CALLBACK(lefCallbacks->MacroSizeCbk, lefrMacroSizeCbkType, lefData->macroNum);
      }
    }
#line 10066 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 623: /* macro_pin: start_macro_pin macro_pin_options end_macro_pin  */
#line 4829 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PinCbk)
        CALLBACK(lefCallbacks->PinCbk, lefrPinCbkType, &lefData->lefrPin);
      lefData->lefrPin.clear();
    }
#line 10076 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 624: /* $@81: %empty  */
#line 4835 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; lefData->pinDef = 1;}
#line 10082 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 625: /* start_macro_pin: K_PIN $@81 T_STRING  */
#line 4836 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setName((yyvsp[0].string));
      //strcpy(lefData->pinName, $3);
      lefData->pinName = strdup((yyvsp[0].string));
    }
#line 10091 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 626: /* $@82: %empty  */
#line 4841 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                     {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 10097 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 627: /* end_macro_pin: K_END $@82 T_STRING  */
#line 4842 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (strcmp(lefData->pinName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->MacroCbk) { // write error only if cbk is set 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END PIN name %s is different from the PIN name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->pinName);
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
#line 10120 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 628: /* macro_pin_options: %empty  */
#line 4863 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 10126 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 629: /* macro_pin_options: macro_pin_options macro_pin_option  */
#line 4865 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 10132 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 630: /* macro_pin_option: start_foreign ';'  */
#line 4869 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign((yyvsp[-1].string), 0, 0.0, 0.0, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10145 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 631: /* macro_pin_option: start_foreign pt ';'  */
#line 4878 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign((yyvsp[-2].string), 1, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10158 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 632: /* macro_pin_option: start_foreign pt orientation ';'  */
#line 4887 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign((yyvsp[-3].string), 1, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].integer));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10171 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 633: /* macro_pin_option: start_foreign K_STRUCTURE ';'  */
#line 4896 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign((yyvsp[-2].string), 0, 0.0, 0.0, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10184 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 634: /* macro_pin_option: start_foreign K_STRUCTURE pt ';'  */
#line 4905 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign((yyvsp[-3].string), 1, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, -1);
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10197 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 635: /* macro_pin_option: start_foreign K_STRUCTURE pt orientation ';'  */
#line 4914 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.addForeign((yyvsp[-4].string), 1, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].integer));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2043, "FOREIGN statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 10210 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 636: /* $@83: %empty  */
#line 4922 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 10216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 637: /* macro_pin_option: K_LEQ $@83 T_STRING ';'  */
#line 4923 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setLEQ((yyvsp[-1].string));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2044, "LEQ statement in MACRO PIN is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
   }
#line 10229 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 638: /* macro_pin_option: K_POWER int_number ';'  */
#line 4932 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setPower((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2045, "MACRO POWER statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10242 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 639: /* macro_pin_option: electrical_direction  */
#line 4941 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setDirection((yyvsp[0].string)); }
#line 10248 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 640: /* macro_pin_option: K_USE macro_pin_use ';'  */
#line 4943 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setUse((yyvsp[-1].string)); }
#line 10254 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 641: /* macro_pin_option: K_SCANUSE macro_scan_use ';'  */
#line 4945 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 10260 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 642: /* macro_pin_option: K_LEAKAGE int_number ';'  */
#line 4947 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setLeakage((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2046, "MACRO LEAKAGE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, r emove this statement from the LEF file with version 5.4 or later.");
    }
#line 10273 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 643: /* macro_pin_option: K_RISETHRESH int_number ';'  */
#line 4956 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseThresh((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2047, "MACRO RISETHRESH statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10286 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 644: /* macro_pin_option: K_FALLTHRESH int_number ';'  */
#line 4965 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setFallThresh((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2048, "MACRO FALLTHRESH statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10299 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 645: /* macro_pin_option: K_RISESATCUR int_number ';'  */
#line 4974 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseSatcur((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2049, "MACRO RISESATCUR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10312 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 646: /* macro_pin_option: K_FALLSATCUR int_number ';'  */
#line 4983 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setFallSatcur((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2050, "MACRO FALLSATCUR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10325 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 647: /* macro_pin_option: K_VLO int_number ';'  */
#line 4992 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setVLO((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2051, "MACRO VLO statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10338 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 648: /* macro_pin_option: K_VHI int_number ';'  */
#line 5001 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setVHI((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2052, "MACRO VHI statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10351 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 649: /* macro_pin_option: K_TIEOFFR int_number ';'  */
#line 5010 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setTieoffr((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2053, "MACRO TIEOFFR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10364 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 650: /* macro_pin_option: K_SHAPE pin_shape ';'  */
#line 5019 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setShape((yyvsp[-1].string)); }
#line 10370 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 651: /* $@84: %empty  */
#line 5020 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 10376 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 652: /* macro_pin_option: K_MUSTJOIN $@84 T_STRING ';'  */
#line 5021 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setMustjoin((yyvsp[-1].string)); }
#line 10382 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 653: /* $@85: %empty  */
#line 5022 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                        {lefData->lefDumbMode = 1;}
#line 10388 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 654: /* macro_pin_option: K_OUTPUTNOISEMARGIN $@85 int_number int_number ';'  */
#line 5023 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setOutMargin((yyvsp[-2].dval), (yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2054, "MACRO OUTPUTNOISEMARGIN statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10401 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 655: /* $@86: %empty  */
#line 5031 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       {lefData->lefDumbMode = 1;}
#line 10407 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 656: /* macro_pin_option: K_OUTPUTRESISTANCE $@86 int_number int_number ';'  */
#line 5032 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setOutResistance((yyvsp[-2].dval), (yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2055, "MACRO OUTPUTRESISTANCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10420 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 657: /* $@87: %empty  */
#line 5040 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       {lefData->lefDumbMode = 1;}
#line 10426 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 658: /* macro_pin_option: K_INPUTNOISEMARGIN $@87 int_number int_number ';'  */
#line 5041 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setInMargin((yyvsp[-2].dval), (yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2056, "MACRO INPUTNOISEMARGIN statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10439 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 659: /* macro_pin_option: K_CAPACITANCE int_number ';'  */
#line 5050 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setCapacitance((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2057, "MACRO CAPACITANCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10452 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 660: /* macro_pin_option: K_MAXDELAY int_number ';'  */
#line 5059 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setMaxdelay((yyvsp[-1].dval)); }
#line 10458 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 661: /* macro_pin_option: K_MAXLOAD int_number ';'  */
#line 5061 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setMaxload((yyvsp[-1].dval)); }
#line 10464 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 662: /* macro_pin_option: K_RESISTANCE int_number ';'  */
#line 5063 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setResistance((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2058, "MACRO RESISTANCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10477 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 663: /* macro_pin_option: K_PULLDOWNRES int_number ';'  */
#line 5072 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setPulldownres((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2059, "MACRO PULLDOWNRES statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10490 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 664: /* macro_pin_option: K_CURRENTSOURCE K_ACTIVE ';'  */
#line 5081 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setCurrentSource("ACTIVE");
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2060, "MACRO CURRENTSOURCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10503 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 665: /* macro_pin_option: K_CURRENTSOURCE K_RESISTIVE ';'  */
#line 5090 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setCurrentSource("RESISTIVE");
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2061, "MACRO CURRENTSOURCE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10516 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 666: /* macro_pin_option: K_RISEVOLTAGETHRESHOLD int_number ';'  */
#line 5099 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseVoltage((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2062, "MACRO RISEVOLTAGETHRESHOLD statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10529 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 667: /* macro_pin_option: K_FALLVOLTAGETHRESHOLD int_number ';'  */
#line 5108 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setFallVoltage((yyvsp[-1].dval));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2063, "MACRO FALLVOLTAGETHRESHOLD statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10542 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 668: /* macro_pin_option: K_IV_TABLES T_STRING T_STRING ';'  */
#line 5117 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->PinCbk) lefData->lefrPin.setTables((yyvsp[-2].string), (yyvsp[-1].string));
      } else
        if (lefCallbacks->PinCbk) // write warning only if cbk is set 
           if (lefData->pinWarnings++ < lefSettings->PinWarnings)
             lefWarning(2064, "MACRO IV_TABLES statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 10555 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 669: /* macro_pin_option: K_TAPERRULE T_STRING ';'  */
#line 5126 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setTaperRule((yyvsp[-1].string)); }
#line 10561 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 670: /* $@88: %empty  */
#line 5127 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode = 1000000; }
#line 10567 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 671: /* macro_pin_option: K_PROPERTY $@88 pin_prop_list ';'  */
#line 5128 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;
    }
#line 10574 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 672: /* macro_pin_option: start_macro_port macro_port_class_option geometries K_END  */
#line 5131 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 10592 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 673: /* macro_pin_option: start_macro_port K_END  */
#line 5145 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 10608 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 674: /* macro_pin_option: K_ANTENNASIZE int_number opt_layer_name ';'  */
#line 5157 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaSize((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10633 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 675: /* macro_pin_option: K_ANTENNAMETALAREA NUMBER opt_layer_name ';'  */
#line 5178 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMetalArea((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10658 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 676: /* macro_pin_option: K_ANTENNAMETALLENGTH int_number opt_layer_name ';'  */
#line 5199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMetalLength((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10683 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 677: /* macro_pin_option: K_RISESLEWLIMIT int_number ';'  */
#line 5220 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setRiseSlewLimit((yyvsp[-1].dval)); }
#line 10689 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 678: /* macro_pin_option: K_FALLSLEWLIMIT int_number ';'  */
#line 5222 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PinCbk) lefData->lefrPin.setFallSlewLimit((yyvsp[-1].dval)); }
#line 10695 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 679: /* macro_pin_option: K_ANTENNAPARTIALMETALAREA NUMBER opt_layer_name ';'  */
#line 5224 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaPartialMetalArea((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10729 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 680: /* macro_pin_option: K_ANTENNAPARTIALMETALSIDEAREA NUMBER opt_layer_name ';'  */
#line 5254 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaPartialMetalSideArea((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10763 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 681: /* macro_pin_option: K_ANTENNAPARTIALCUTAREA NUMBER opt_layer_name ';'  */
#line 5284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaPartialCutArea((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10797 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 682: /* macro_pin_option: K_ANTENNADIFFAREA NUMBER opt_layer_name ';'  */
#line 5314 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaDiffArea((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10831 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 683: /* macro_pin_option: K_ANTENNAGATEAREA NUMBER opt_layer_name ';'  */
#line 5344 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaGateArea((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10865 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 684: /* macro_pin_option: K_ANTENNAMAXAREACAR NUMBER req_layer_name ';'  */
#line 5374 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMaxAreaCar((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10899 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 685: /* macro_pin_option: K_ANTENNAMAXSIDEAREACAR NUMBER req_layer_name ';'  */
#line 5404 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMaxSideAreaCar((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10933 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 686: /* macro_pin_option: K_ANTENNAMAXCUTCAR NUMBER req_layer_name ';'  */
#line 5434 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
      if (lefCallbacks->PinCbk) lefData->lefrPin.addAntennaMaxCutCar((yyvsp[-2].dval), (yyvsp[-1].string));
    }
#line 10967 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 687: /* $@89: %empty  */
#line 5464 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 11000 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 689: /* $@90: %empty  */
#line 5493 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {lefData->lefDumbMode = 2; lefData->lefNoNum = 2; }
#line 11006 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 690: /* macro_pin_option: K_NETEXPR $@90 QSTRING ';'  */
#line 5494 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        if (lefCallbacks->PinCbk) lefData->lefrPin.setNetExpr((yyvsp[-1].string));
    }
#line 11026 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 691: /* $@91: %empty  */
#line 5509 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                        {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 11032 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 692: /* macro_pin_option: K_SUPPLYSENSITIVITY $@91 T_STRING ';'  */
#line 5510 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        if (lefCallbacks->PinCbk) lefData->lefrPin.setSupplySensitivity((yyvsp[-1].string));
    }
#line 11052 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 693: /* $@92: %empty  */
#line 5525 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                        {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 11058 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 694: /* macro_pin_option: K_GROUNDSENSITIVITY $@92 T_STRING ';'  */
#line 5526 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        if (lefCallbacks->PinCbk) lefData->lefrPin.setGroundSensitivity((yyvsp[-1].string));
    }
#line 11078 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 695: /* pin_layer_oxide: K_OXIDE1  */
#line 5544 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(1);
    }
#line 11087 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 696: /* pin_layer_oxide: K_OXIDE2  */
#line 5549 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(2);
    }
#line 11096 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 697: /* pin_layer_oxide: K_OXIDE3  */
#line 5554 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(3);
    }
#line 11105 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 698: /* pin_layer_oxide: K_OXIDE4  */
#line 5559 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(4);
    }
#line 11114 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 699: /* pin_layer_oxide: K_OXIDE5  */
#line 5564 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(5);
    }
#line 11123 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 700: /* pin_layer_oxide: K_OXIDE6  */
#line 5569 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(6);
    }
#line 11132 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 701: /* pin_layer_oxide: K_OXIDE7  */
#line 5574 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(7);
    }
#line 11141 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 702: /* pin_layer_oxide: K_OXIDE8  */
#line 5579 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(8);
    }
#line 11150 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 703: /* pin_layer_oxide: K_OXIDE9  */
#line 5584 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(9);
    }
#line 11159 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 704: /* pin_layer_oxide: K_OXIDE10  */
#line 5589 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(10);
    }
#line 11168 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 705: /* pin_layer_oxide: K_OXIDE11  */
#line 5594 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(11);
    }
#line 11177 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 706: /* pin_layer_oxide: K_OXIDE12  */
#line 5599 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(12);
    }
#line 11186 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 707: /* pin_layer_oxide: K_OXIDE13  */
#line 5604 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(13);
    }
#line 11195 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 708: /* pin_layer_oxide: K_OXIDE14  */
#line 5609 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(14);
    }
#line 11204 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 709: /* pin_layer_oxide: K_OXIDE15  */
#line 5614 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(15);
    }
#line 11213 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 710: /* pin_layer_oxide: K_OXIDE16  */
#line 5619 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(16);
    }
#line 11222 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 711: /* pin_layer_oxide: K_OXIDE17  */
#line 5624 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(17);
    }
#line 11231 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 712: /* pin_layer_oxide: K_OXIDE18  */
#line 5629 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(18);
    }
#line 11240 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 713: /* pin_layer_oxide: K_OXIDE19  */
#line 5634 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(19);
    }
#line 11249 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 714: /* pin_layer_oxide: K_OXIDE20  */
#line 5639 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(20);
    }
#line 11258 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 715: /* pin_layer_oxide: K_OXIDE21  */
#line 5644 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(21);
    }
#line 11267 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 716: /* pin_layer_oxide: K_OXIDE22  */
#line 5649 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(22);
    }
#line 11276 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 717: /* pin_layer_oxide: K_OXIDE23  */
#line 5654 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(23);
    }
#line 11285 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 718: /* pin_layer_oxide: K_OXIDE24  */
#line 5659 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(24);
    }
#line 11294 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 719: /* pin_layer_oxide: K_OXIDE25  */
#line 5664 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(25);
    }
#line 11303 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 720: /* pin_layer_oxide: K_OXIDE26  */
#line 5669 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(26);
    }
#line 11312 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 721: /* pin_layer_oxide: K_OXIDE27  */
#line 5674 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(27);
    }
#line 11321 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 722: /* pin_layer_oxide: K_OXIDE28  */
#line 5679 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(28);
    }
#line 11330 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 723: /* pin_layer_oxide: K_OXIDE29  */
#line 5684 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(29);
    }
#line 11339 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 724: /* pin_layer_oxide: K_OXIDE30  */
#line 5689 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(30);
    }
#line 11348 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 725: /* pin_layer_oxide: K_OXIDE31  */
#line 5694 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(31);
    }
#line 11357 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 726: /* pin_layer_oxide: K_OXIDE32  */
#line 5699 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefCallbacks->PinCbk)
       lefData->lefrPin.addAntennaModel(32);
    }
#line 11366 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 729: /* pin_name_value_pair: T_STRING NUMBER  */
#line 5711 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      char temp[32];
      sprintf(temp, "%.11g", (yyvsp[0].dval));
      if (lefCallbacks->PinCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrPinProp.propType((yyvsp[-1].string));
         lefData->lefrPin.setNumProperty((yyvsp[-1].string), (yyvsp[0].dval), temp, propTp);
      }
    }
#line 11380 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 730: /* pin_name_value_pair: T_STRING QSTRING  */
#line 5721 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->PinCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrPinProp.propType((yyvsp[-1].string));
         lefData->lefrPin.setProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 11392 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 731: /* pin_name_value_pair: T_STRING T_STRING  */
#line 5729 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->PinCbk) {
         char propTp;
         propTp = lefSettings->lefProps.lefrPinProp.propType((yyvsp[-1].string));
         lefData->lefrPin.setProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
      }
    }
#line 11404 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 732: /* electrical_direction: K_DIRECTION K_INPUT ';'  */
#line 5738 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                       {(yyval.string) = (char*)"INPUT";}
#line 11410 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 733: /* electrical_direction: K_DIRECTION K_OUTPUT ';'  */
#line 5739 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                        {(yyval.string) = (char*)"OUTPUT";}
#line 11416 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 734: /* electrical_direction: K_DIRECTION K_OUTPUT K_TRISTATE ';'  */
#line 5740 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                        {(yyval.string) = (char*)"OUTPUT TRISTATE";}
#line 11422 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 735: /* electrical_direction: K_DIRECTION K_INOUT ';'  */
#line 5741 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                        {(yyval.string) = (char*)"INOUT";}
#line 11428 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 736: /* electrical_direction: K_DIRECTION K_FEEDTHRU ';'  */
#line 5742 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                        {(yyval.string) = (char*)"FEEDTHRU";}
#line 11434 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 737: /* start_macro_port: K_PORT  */
#line 5745 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 11449 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 739: /* macro_port_class_option: K_CLASS class_type ';'  */
#line 5758 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->addClass((yyvsp[-1].string)); }
#line 11456 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 740: /* macro_pin_use: K_SIGNAL  */
#line 5762 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"SIGNAL";}
#line 11462 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 741: /* macro_pin_use: K_ANALOG  */
#line 5763 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"ANALOG";}
#line 11468 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 742: /* macro_pin_use: K_POWER  */
#line 5764 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"POWER";}
#line 11474 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 743: /* macro_pin_use: K_GROUND  */
#line 5765 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"GROUND";}
#line 11480 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 744: /* macro_pin_use: K_CLOCK  */
#line 5766 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"CLOCK";}
#line 11486 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 745: /* macro_pin_use: K_DATA  */
#line 5767 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"DATA";}
#line 11492 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 746: /* macro_scan_use: K_INPUT  */
#line 5770 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {(yyval.string) = (char*)"INPUT";}
#line 11498 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 747: /* macro_scan_use: K_OUTPUT  */
#line 5771 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"OUTPUT";}
#line 11504 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 748: /* macro_scan_use: K_START  */
#line 5772 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"START";}
#line 11510 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 749: /* macro_scan_use: K_STOP  */
#line 5773 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"STOP";}
#line 11516 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 750: /* pin_shape: %empty  */
#line 5776 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {(yyval.string) = (char*)""; }
#line 11522 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 751: /* pin_shape: K_ABUTMENT  */
#line 5777 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"ABUTMENT";}
#line 11528 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 752: /* pin_shape: K_RING  */
#line 5778 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"RING";}
#line 11534 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 753: /* pin_shape: K_FEEDTHRU  */
#line 5779 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.string) = (char*)"FEEDTHRU";}
#line 11540 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 755: /* $@93: %empty  */
#line 5784 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 11546 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 756: /* $@94: %empty  */
#line 5785 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        lefData->lefrGeometriesPtr->addLayer((yyvsp[0].string));
      lefData->needGeometry = 1;    // within LAYER it requires either path, rect, poly
      lefData->hasGeoLayer = 1;
    }
#line 11565 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 758: /* geometry: K_WIDTH int_number ';'  */
#line 5802 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else
           lefData->lefrGeometriesPtr->addWidth((yyvsp[-1].dval)); 
      } 
    }
#line 11581 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 759: /* geometry: K_PATH maskColor firstPt otherPts ';'  */
#line 5814 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)(yyvsp[-3].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
                lefData->lefrGeometriesPtr->addPath((int)(yyvsp[-3].integer));
           }
        }
      }
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
#line 11606 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 760: /* geometry: K_PATH maskColor K_ITERATE firstPt otherPts stepPattern ';'  */
#line 5835 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)(yyvsp[-5].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addPathIter((int)(yyvsp[-5].integer));
            }
         }
      } 
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
#line 11631 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 761: /* geometry: K_RECT maskColor pt pt ';'  */
#line 5856 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)(yyvsp[-3].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addRect((int)(yyvsp[-3].integer), (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].pt).x, (yyvsp[-1].pt).y);
           }
        }
      }
      lefData->needGeometry = 2;
    }
#line 11655 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 762: /* geometry: K_RECT maskColor K_ITERATE pt pt stepPattern ';'  */
#line 5876 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)(yyvsp[-5].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addRectIter((int)(yyvsp[-5].integer), (yyvsp[-3].pt).x, (yyvsp[-3].pt).y, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y);
           }
        }
      }
      lefData->needGeometry = 2;
    }
#line 11679 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 763: /* geometry: K_POLYGON maskColor firstPt nextPt nextPt otherPts ';'  */
#line 5896 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)(yyvsp[-5].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addPolygon((int)(yyvsp[-5].integer));
            }
           }
      }
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
#line 11705 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 764: /* geometry: K_POLYGON maskColor K_ITERATE firstPt nextPt nextPt otherPts stepPattern ';'  */
#line 5918 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (lefData->hasGeoLayer == 0) {   // LAYER statement is missing 
           if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
              lefError(1701, "A LAYER statement is missing in Geometry.\nLAYER is a required statement before any geometry can be defined.");
              CHKERR();
           }
        } else {
           if (lefData->versionNum < 5.8 && (int)(yyvsp[-7].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
           } else {
              lefData->lefrGeometriesPtr->addPolygonIter((int)(yyvsp[-7].integer));
           }
         }
      }
      lefData->hasPRP = 1;
      lefData->needGeometry = 2;
    }
#line 11730 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 765: /* geometry: via_placement  */
#line 5939 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 11736 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 769: /* layer_exceptpgnet: K_EXCEPTPGNET  */
#line 5946 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 11754 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 771: /* layer_spacing: K_SPACING int_number  */
#line 5962 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (zeroOrGt((yyvsp[0].dval)))
           lefData->lefrGeometriesPtr->addLayerMinSpacing((yyvsp[0].dval));
        else {
           lefData->outMsg = (char*)lefMalloc(10000);
           sprintf (lefData->outMsg,
              "THE SPACING statement has the value %g in MACRO OBS.\nValue has to be 0 or greater.", (yyvsp[0].dval));
           lefError(1659, lefData->outMsg);
           lefFree(lefData->outMsg);
           CHKERR();
        }
      }
    }
#line 11772 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 772: /* layer_spacing: K_DESIGNRULEWIDTH int_number  */
#line 5976 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries) {
        if (zeroOrGt((yyvsp[0].dval)))
           lefData->lefrGeometriesPtr->addLayerRuleWidth((yyvsp[0].dval));
        else {
           lefData->outMsg = (char*)lefMalloc(10000);
           sprintf (lefData->outMsg,
              "THE DESIGNRULEWIDTH statement has the value %g in MACRO OBS.\nValue has to be 0 or greater.", (yyvsp[0].dval));
           lefError(1660, lefData->outMsg);
           lefFree(lefData->outMsg);
           CHKERR();
        }
      }
    }
#line 11790 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 773: /* firstPt: pt  */
#line 5991 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->startList((yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 11797 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 774: /* nextPt: pt  */
#line 5995 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoGeometries)
        lefData->lefrGeometriesPtr->addToList((yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 11804 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 777: /* $@95: %empty  */
#line 6004 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                     {lefData->lefDumbMode = 1;}
#line 11810 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 778: /* via_placement: K_VIA maskColor pt $@95 T_STRING ';'  */
#line 6005 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
        if (lefData->lefrDoGeometries){
            if (lefData->versionNum < 5.8 && (int)(yyvsp[-4].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
            } else {
                lefData->lefrGeometriesPtr->addVia((int)(yyvsp[-4].integer), (yyvsp[-3].pt).x, (yyvsp[-3].pt).y, (yyvsp[-1].string));
            }
        }
    }
#line 11827 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 779: /* $@96: %empty  */
#line 6017 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                 {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 11833 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 780: /* via_placement: K_VIA K_ITERATE maskColor pt $@96 T_STRING stepPattern ';'  */
#line 6019 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
        if (lefData->lefrDoGeometries) {
            if (lefData->versionNum < 5.8 && (int)(yyvsp[-5].integer) > 0) {
              if (lefData->macroWarnings++ < lefSettings->MacroWarnings) {
                 lefError(2083, "Color mask information can only be defined with version 5.8.");
                 CHKERR(); 
              }           
            } else {
              lefData->lefrGeometriesPtr->addViaIter((int)(yyvsp[-5].integer), (yyvsp[-4].pt).x, (yyvsp[-4].pt).y, (yyvsp[-2].string)); 
            }
        }
    }
#line 11850 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 781: /* stepPattern: K_DO int_number K_BY int_number K_STEP int_number int_number  */
#line 6033 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
     { if (lefData->lefrDoGeometries)
         lefData->lefrGeometriesPtr->addStepPattern((yyvsp[-5].dval), (yyvsp[-3].dval), (yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 11857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 782: /* sitePattern: T_STRING int_number int_number orientation K_DO int_number K_BY int_number K_STEP int_number int_number  */
#line 6038 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->lefrDoSite) {
        lefData->lefrSitePatternPtr = (lefiSitePattern*)lefMalloc(
                                   sizeof(lefiSitePattern));
        lefData->lefrSitePatternPtr->Init();
        lefData->lefrSitePatternPtr->set((yyvsp[-10].string), (yyvsp[-9].dval), (yyvsp[-8].dval), (yyvsp[-7].integer), (yyvsp[-5].dval), (yyvsp[-3].dval),
          (yyvsp[-1].dval), (yyvsp[0].dval));
        }
    }
#line 11871 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 783: /* sitePattern: T_STRING int_number int_number orientation  */
#line 6048 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->lefrDoSite) {
        lefData->lefrSitePatternPtr = (lefiSitePattern*)lefMalloc(
                                   sizeof(lefiSitePattern));
        lefData->lefrSitePatternPtr->Init();
        lefData->lefrSitePatternPtr->set((yyvsp[-3].string), (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].integer), -1, -1,
          -1, -1);
        }
    }
#line 11885 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 784: /* $@97: %empty  */
#line 6060 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("X", (yyvsp[-4].dval), (int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }    
    }
#line 11898 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 785: /* $@98: %empty  */
#line 6068 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1000000000;}
#line 11904 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 786: /* trackPattern: K_X int_number K_DO int_number K_STEP int_number $@97 K_LAYER $@98 trackLayers  */
#line 6069 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;}
#line 11910 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 787: /* $@99: %empty  */
#line 6071 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                    sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("Y", (yyvsp[-4].dval), (int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }    
    }
#line 11923 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 788: /* $@100: %empty  */
#line 6079 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1000000000;}
#line 11929 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 789: /* trackPattern: K_Y int_number K_DO int_number K_STEP int_number $@99 K_LAYER $@100 trackLayers  */
#line 6080 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;}
#line 11935 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 790: /* trackPattern: K_X int_number K_DO int_number K_STEP int_number  */
#line 6082 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                    sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("X", (yyvsp[-4].dval), (int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }    
    }
#line 11948 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 791: /* trackPattern: K_Y int_number K_DO int_number K_STEP int_number  */
#line 6091 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefData->lefrDoTrack) {
        lefData->lefrTrackPatternPtr = (lefiTrackPattern*)lefMalloc(
                                    sizeof(lefiTrackPattern));
        lefData->lefrTrackPatternPtr->Init();
        lefData->lefrTrackPatternPtr->set("Y", (yyvsp[-4].dval), (int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }    
    }
#line 11961 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 794: /* layer_name: T_STRING  */
#line 6106 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefData->lefrDoTrack) lefData->lefrTrackPatternPtr->addLayer((yyvsp[0].string)); }
#line 11967 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 795: /* gcellPattern: K_X int_number K_DO int_number K_STEP int_number  */
#line 6109 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->lefrDoGcell) {
        lefData->lefrGcellPatternPtr = (lefiGcellPattern*)lefMalloc(
                                    sizeof(lefiGcellPattern));
        lefData->lefrGcellPatternPtr->Init();
        lefData->lefrGcellPatternPtr->set("X", (yyvsp[-4].dval), (int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }    
    }
#line 11980 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 796: /* gcellPattern: K_Y int_number K_DO int_number K_STEP int_number  */
#line 6118 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->lefrDoGcell) {
        lefData->lefrGcellPatternPtr = (lefiGcellPattern*)lefMalloc(
                                    sizeof(lefiGcellPattern));
        lefData->lefrGcellPatternPtr->Init();
        lefData->lefrGcellPatternPtr->set("Y", (yyvsp[-4].dval), (int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }    
    }
#line 11993 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 797: /* macro_obs: start_macro_obs geometries K_END  */
#line 6128 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 12008 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 798: /* macro_obs: start_macro_obs K_END  */
#line 6139 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 12023 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 799: /* start_macro_obs: K_OBS  */
#line 6151 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 12038 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 800: /* macro_density: K_DENSITY density_layer density_layers K_END  */
#line 6163 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 12062 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 803: /* $@101: %empty  */
#line 6187 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                       { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 12068 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 804: /* $@102: %empty  */
#line 6188 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->DensityCbk)
        lefData->lefrDensity.addLayer((yyvsp[-1].string));
    }
#line 12077 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 808: /* density_layer_rect: K_RECT pt pt int_number ';'  */
#line 6199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->DensityCbk)
        lefData->lefrDensity.addRect((yyvsp[-3].pt).x, (yyvsp[-3].pt).y, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y, (yyvsp[-1].dval)); 
    }
#line 12086 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 809: /* $@103: %empty  */
#line 6204 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                             { lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 12092 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 810: /* macro_clocktype: K_CLOCKTYPE $@103 T_STRING ';'  */
#line 6205 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->MacroCbk) lefData->lefrMacro.setClockType((yyvsp[-1].string)); }
#line 12098 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 811: /* timing: start_timing timing_options end_timing  */
#line 6208 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12104 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 812: /* start_timing: K_TIMING  */
#line 6211 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12110 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 813: /* end_timing: K_END K_TIMING  */
#line 6214 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 12127 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 816: /* $@104: %empty  */
#line 6234 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    if (lefData->versionNum < 5.4) {
      if (lefCallbacks->TimingCbk && lefData->lefrTiming.hasData())
        CALLBACK(lefCallbacks->TimingCbk, lefrTimingCbkType, &lefData->lefrTiming);
    }
    lefData->lefDumbMode = 1000000000;
    lefData->lefrTiming.clear();
    }
#line 12140 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 817: /* timing_option: K_FROMPIN $@104 list_of_from_strings ';'  */
#line 6243 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;}
#line 12146 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 818: /* $@105: %empty  */
#line 6244 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1000000000;}
#line 12152 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 819: /* timing_option: K_TOPIN $@105 list_of_to_strings ';'  */
#line 6245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { lefData->lefDumbMode = 0;}
#line 12158 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 820: /* $@106: %empty  */
#line 6247 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFall((yyvsp[-3].string),(yyvsp[-1].dval),(yyvsp[0].dval)); }
#line 12164 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 821: /* timing_option: risefall K_INTRINSIC int_number int_number $@106 slew_spec K_VARIABLE int_number int_number ';'  */
#line 6249 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallVariable((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12170 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 822: /* timing_option: risefall delay_or_transition K_UNATENESS unateness K_TABLEDIMENSION int_number int_number int_number ';'  */
#line 6252 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) {
        if ((yyvsp[-7].string)[0] == 'D' || (yyvsp[-7].string)[0] == 'd') // delay 
          lefData->lefrTiming.addDelay((yyvsp[-8].string), (yyvsp[-5].string), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval));
        else
          lefData->lefrTiming.addTransition((yyvsp[-8].string), (yyvsp[-5].string), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval));
      }
    }
#line 12182 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 823: /* timing_option: K_TABLEAXIS list_of_table_axis_dnumbers ';'  */
#line 6260 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12188 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 824: /* timing_option: K_TABLEENTRIES list_of_table_entries ';'  */
#line 6262 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12194 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 825: /* timing_option: K_RISERS int_number int_number ';'  */
#line 6264 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseRS((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12200 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 826: /* timing_option: K_FALLRS int_number int_number ';'  */
#line 6266 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallRS((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12206 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 827: /* timing_option: K_RISECS int_number int_number ';'  */
#line 6268 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseCS((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12212 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 828: /* timing_option: K_FALLCS int_number int_number ';'  */
#line 6270 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallCS((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12218 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 829: /* timing_option: K_RISESATT1 int_number int_number ';'  */
#line 6272 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseAtt1((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12224 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 830: /* timing_option: K_FALLSATT1 int_number int_number ';'  */
#line 6274 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallAtt1((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12230 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 831: /* timing_option: K_RISET0 int_number int_number ';'  */
#line 6276 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setRiseTo((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12236 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 832: /* timing_option: K_FALLT0 int_number int_number ';'  */
#line 6278 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setFallTo((yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12242 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 833: /* timing_option: K_UNATENESS unateness ';'  */
#line 6280 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addUnateness((yyvsp[-1].string)); }
#line 12248 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 834: /* timing_option: K_STABLE K_SETUP int_number K_HOLD int_number risefall ';'  */
#line 6282 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setStable((yyvsp[-4].dval),(yyvsp[-2].dval),(yyvsp[-1].string)); }
#line 12254 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 835: /* timing_option: two_pin_trigger from_pin_trigger to_pin_trigger K_TABLEDIMENSION int_number int_number int_number ';'  */
#line 6284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addSDF2Pins((yyvsp[-7].string),(yyvsp[-6].string),(yyvsp[-5].string),(yyvsp[-3].dval),(yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12260 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 836: /* timing_option: one_pin_trigger K_TABLEDIMENSION int_number int_number int_number ';'  */
#line 6286 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addSDF1Pin((yyvsp[-5].string),(yyvsp[-3].dval),(yyvsp[-2].dval),(yyvsp[-2].dval)); }
#line 12266 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 837: /* timing_option: K_SDFCONDSTART QSTRING ';'  */
#line 6288 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setSDFcondStart((yyvsp[-1].string)); }
#line 12272 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 838: /* timing_option: K_SDFCONDEND QSTRING ';'  */
#line 6290 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setSDFcondEnd((yyvsp[-1].string)); }
#line 12278 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 839: /* timing_option: K_SDFCOND QSTRING ';'  */
#line 6292 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.setSDFcond((yyvsp[-1].string)); }
#line 12284 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 840: /* timing_option: K_EXTENSION ';'  */
#line 6294 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12290 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 841: /* one_pin_trigger: K_MPWH  */
#line 6298 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"MPWH";}
#line 12296 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 842: /* one_pin_trigger: K_MPWL  */
#line 6300 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"MPWL";}
#line 12302 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 843: /* one_pin_trigger: K_PERIOD  */
#line 6302 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"PERIOD";}
#line 12308 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 844: /* two_pin_trigger: K_SETUP  */
#line 6306 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"SETUP";}
#line 12314 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 845: /* two_pin_trigger: K_HOLD  */
#line 6308 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"HOLD";}
#line 12320 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 846: /* two_pin_trigger: K_RECOVERY  */
#line 6310 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"RECOVERY";}
#line 12326 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 847: /* two_pin_trigger: K_SKEW  */
#line 6312 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"SKEW";}
#line 12332 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 848: /* from_pin_trigger: K_ANYEDGE  */
#line 6316 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"ANYEDGE";}
#line 12338 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 849: /* from_pin_trigger: K_POSEDGE  */
#line 6318 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"POSEDGE";}
#line 12344 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 850: /* from_pin_trigger: K_NEGEDGE  */
#line 6320 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"NEGEDGE";}
#line 12350 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 851: /* to_pin_trigger: K_ANYEDGE  */
#line 6324 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"ANYEDGE";}
#line 12356 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 852: /* to_pin_trigger: K_POSEDGE  */
#line 6326 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"POSEDGE";}
#line 12362 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 853: /* to_pin_trigger: K_NEGEDGE  */
#line 6328 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"NEGEDGE";}
#line 12368 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 854: /* delay_or_transition: K_DELAY  */
#line 6332 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"DELAY"; }
#line 12374 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 855: /* delay_or_transition: K_TRANSITIONTIME  */
#line 6334 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"TRANSITION"; }
#line 12380 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 856: /* list_of_table_entries: table_entry  */
#line 6338 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12386 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 857: /* list_of_table_entries: list_of_table_entries table_entry  */
#line 6340 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12392 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 858: /* table_entry: '(' int_number int_number int_number ')'  */
#line 6343 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addTableEntry((yyvsp[-3].dval),(yyvsp[-2].dval),(yyvsp[-1].dval)); }
#line 12398 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 859: /* list_of_table_axis_dnumbers: int_number  */
#line 6347 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addTableAxisNumber((yyvsp[0].dval)); }
#line 12404 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 860: /* list_of_table_axis_dnumbers: list_of_table_axis_dnumbers int_number  */
#line 6349 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addTableAxisNumber((yyvsp[0].dval)); }
#line 12410 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 861: /* slew_spec: %empty  */
#line 6353 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12416 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 862: /* slew_spec: int_number int_number int_number int_number  */
#line 6355 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallSlew((yyvsp[-3].dval),(yyvsp[-2].dval),(yyvsp[-1].dval),(yyvsp[0].dval)); }
#line 12422 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 863: /* slew_spec: int_number int_number int_number int_number int_number int_number int_number  */
#line 6357 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallSlew((yyvsp[-6].dval),(yyvsp[-5].dval),(yyvsp[-4].dval),(yyvsp[-3].dval));
      if (lefCallbacks->TimingCbk) lefData->lefrTiming.addRiseFallSlew2((yyvsp[-2].dval),(yyvsp[-1].dval),(yyvsp[0].dval)); }
#line 12429 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 864: /* risefall: K_RISE  */
#line 6362 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"RISE"; }
#line 12435 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 865: /* risefall: K_FALL  */
#line 6364 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"FALL"; }
#line 12441 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 866: /* unateness: K_INVERT  */
#line 6368 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"INVERT"; }
#line 12447 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 867: /* unateness: K_NONINVERT  */
#line 6370 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"NONINVERT"; }
#line 12453 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 868: /* unateness: K_NONUNATE  */
#line 6372 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (char*)"NONUNATE"; }
#line 12459 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 869: /* list_of_from_strings: T_STRING  */
#line 6376 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addFromPin((yyvsp[0].string)); }
#line 12465 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 870: /* list_of_from_strings: list_of_from_strings T_STRING  */
#line 6378 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addFromPin((yyvsp[0].string)); }
#line 12471 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 871: /* list_of_to_strings: T_STRING  */
#line 6382 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addToPin((yyvsp[0].string)); }
#line 12477 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 872: /* list_of_to_strings: list_of_to_strings T_STRING  */
#line 6384 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->TimingCbk) lefData->lefrTiming.addToPin((yyvsp[0].string)); }
#line 12483 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 873: /* $@107: %empty  */
#line 6387 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk)
        CALLBACK(lefCallbacks->ArrayCbk, lefrArrayCbkType, &lefData->lefrArray);
      lefData->lefrArray.clear();
      lefData->lefrSitePatternPtr = 0;
      lefData->lefrDoSite = 0;
   }
#line 12495 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 875: /* $@108: %empty  */
#line 6396 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                     {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 12501 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 876: /* start_array: K_ARRAY $@108 T_STRING  */
#line 6397 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.setName((yyvsp[0].string));
        CALLBACK(lefCallbacks->ArrayBeginCbk, lefrArrayBeginCbkType, (yyvsp[0].string));
      }
      //strcpy(lefData->arrayName, $3);
      lefData->arrayName = strdup((yyvsp[0].string));
    }
#line 12514 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 877: /* $@109: %empty  */
#line 6406 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 {lefData->lefDumbMode = 1; lefData->lefNoNum = 1;}
#line 12520 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 878: /* end_array: K_END $@109 T_STRING  */
#line 6407 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk && lefCallbacks->ArrayEndCbk)
        CALLBACK(lefCallbacks->ArrayEndCbk, lefrArrayEndCbkType, (yyvsp[0].string));
      if (strcmp(lefData->arrayName, (yyvsp[0].string)) != 0) {
        if (lefCallbacks->ArrayCbk) { // write error only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings) {
              lefData->outMsg = (char*)lefMalloc(10000);
              sprintf (lefData->outMsg,
                 "END ARRAY name %s is different from the ARRAY name %s.\nCorrect the LEF file before rerunning it through the LEF parser.", (yyvsp[0].string), lefData->arrayName);
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
#line 12545 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 879: /* array_rules: %empty  */
#line 6430 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12551 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 880: /* array_rules: array_rules array_rule  */
#line 6432 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12557 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 881: /* $@110: %empty  */
#line 6435 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            { if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; lefData->lefDumbMode = 1; }
#line 12563 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 882: /* array_rule: site_word $@110 sitePattern ';'  */
#line 6437 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addSitePattern(lefData->lefrSitePatternPtr);
      }
    }
#line 12573 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 883: /* $@111: %empty  */
#line 6442 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode = 1; if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; }
#line 12579 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 884: /* array_rule: K_CANPLACE $@111 sitePattern ';'  */
#line 6444 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addCanPlace(lefData->lefrSitePatternPtr);
      }
    }
#line 12589 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 885: /* $@112: %empty  */
#line 6449 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {lefData->lefDumbMode = 1; if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; }
#line 12595 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 886: /* array_rule: K_CANNOTOCCUPY $@112 sitePattern ';'  */
#line 6451 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addCannotOccupy(lefData->lefrSitePatternPtr);
      }
    }
#line 12605 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 887: /* $@113: %empty  */
#line 6456 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
             { if (lefCallbacks->ArrayCbk) lefData->lefrDoTrack = 1; }
#line 12611 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 888: /* array_rule: K_TRACKS $@113 trackPattern ';'  */
#line 6457 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addTrack(lefData->lefrTrackPatternPtr);
      }
    }
#line 12621 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 889: /* array_rule: floorplan_start floorplan_list K_END T_STRING  */
#line 6463 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
    }
#line 12628 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 890: /* $@114: %empty  */
#line 6465 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                { if (lefCallbacks->ArrayCbk) lefData->lefrDoGcell = 1; }
#line 12634 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 891: /* array_rule: K_GCELLGRID $@114 gcellPattern ';'  */
#line 6466 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.addGcell(lefData->lefrGcellPatternPtr);
      }
    }
#line 12644 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 892: /* array_rule: K_DEFAULTCAP int_number cap_list K_END K_DEFAULTCAP  */
#line 6472 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk) {
        lefData->lefrArray.setTableSize((int)(yyvsp[-3].dval));
      }
    }
#line 12654 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 893: /* array_rule: def_statement  */
#line 6478 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12660 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 894: /* floorplan_start: K_FLOORPLAN T_STRING  */
#line 6481 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ArrayCbk) lefData->lefrArray.addFloorPlan((yyvsp[0].string)); }
#line 12666 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 895: /* floorplan_list: %empty  */
#line 6485 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12672 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 896: /* floorplan_list: floorplan_list floorplan_element  */
#line 6487 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12678 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 897: /* $@115: %empty  */
#line 6490 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
             { lefData->lefDumbMode = 1; if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; }
#line 12684 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 898: /* floorplan_element: K_CANPLACE $@115 sitePattern ';'  */
#line 6492 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk)
        lefData->lefrArray.addSiteToFloorPlan("CANPLACE",
        lefData->lefrSitePatternPtr);
    }
#line 12694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 899: /* $@116: %empty  */
#line 6497 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   { if (lefCallbacks->ArrayCbk) lefData->lefrDoSite = 1; lefData->lefDumbMode = 1; }
#line 12700 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 900: /* floorplan_element: K_CANNOTOCCUPY $@116 sitePattern ';'  */
#line 6499 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->ArrayCbk)
        lefData->lefrArray.addSiteToFloorPlan("CANNOTOCCUPY",
        lefData->lefrSitePatternPtr);
     }
#line 12710 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 901: /* cap_list: %empty  */
#line 6507 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12716 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 902: /* cap_list: cap_list one_cap  */
#line 6509 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12722 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 903: /* one_cap: K_MINPINS int_number K_WIRECAP int_number ';'  */
#line 6512 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->ArrayCbk) lefData->lefrArray.addDefaultCap((int)(yyvsp[-3].dval), (yyvsp[-1].dval)); }
#line 12728 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 904: /* $@117: %empty  */
#line 6515 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode=1;lefData->lefNlToken=true;}
#line 12734 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 905: /* msg_statement: K_MESSAGE $@117 T_STRING '=' s_expr dtrm  */
#line 6516 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {  }
#line 12740 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 906: /* $@118: %empty  */
#line 6519 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode=1;lefData->lefNlToken=true;}
#line 12746 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 907: /* create_file_statement: K_CREATEFILE $@118 T_STRING '=' s_expr dtrm  */
#line 6520 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 12752 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 908: /* $@119: %empty  */
#line 6523 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
           {lefData->lefDumbMode=1;lefData->lefNlToken=true;}
#line 12758 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 909: /* def_statement: K_DEFINE $@119 T_STRING '=' expression dtrm  */
#line 6524 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6)
        lefAddNumDefine((yyvsp[-3].string), (yyvsp[-1].dval));
      else
        if (lefCallbacks->ArrayCbk) // write warning only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings)
             lefWarning(2067, "DEFINE statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 12771 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 910: /* $@120: %empty  */
#line 6532 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode=1;lefData->lefNlToken=true;}
#line 12777 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 911: /* def_statement: K_DEFINES $@120 T_STRING '=' s_expr dtrm  */
#line 6533 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6)
        lefAddStringDefine((yyvsp[-3].string), (yyvsp[-1].string));
      else
        if (lefCallbacks->ArrayCbk) // write warning only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings)
             lefWarning(2068, "DEFINES statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 12790 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 912: /* $@121: %empty  */
#line 6541 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
               {lefData->lefDumbMode=1;lefData->lefNlToken=true;}
#line 12796 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 913: /* def_statement: K_DEFINEB $@121 T_STRING '=' b_expr dtrm  */
#line 6542 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.6)
        lefAddBooleanDefine((yyvsp[-3].string), (yyvsp[-1].integer));
      else
        if (lefCallbacks->ArrayCbk) // write warning only if cbk is set 
           if (lefData->arrayWarnings++ < lefSettings->ArrayWarnings)
             lefWarning(2069, "DEFINEB statement is obsolete in version 5.6 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.6 or later.");
    }
#line 12809 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 915: /* dtrm: ';'  */
#line 6553 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {lefData->lefNlToken = false;}
#line 12815 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 916: /* dtrm: '\n'  */
#line 6554 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                 {lefData->lefNlToken = false;}
#line 12821 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 921: /* expression: expression '+' expression  */
#line 6567 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.dval) = (yyvsp[-2].dval) + (yyvsp[0].dval); }
#line 12827 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 922: /* expression: expression '-' expression  */
#line 6568 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.dval) = (yyvsp[-2].dval) - (yyvsp[0].dval); }
#line 12833 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 923: /* expression: expression '*' expression  */
#line 6569 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.dval) = (yyvsp[-2].dval) * (yyvsp[0].dval); }
#line 12839 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 924: /* expression: expression '/' expression  */
#line 6570 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.dval) = (yyvsp[-2].dval) / (yyvsp[0].dval); }
#line 12845 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 925: /* expression: '-' expression  */
#line 6571 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.dval) = -(yyvsp[0].dval);}
#line 12851 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 926: /* expression: '(' expression ')'  */
#line 6572 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.dval) = (yyvsp[-1].dval);}
#line 12857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 927: /* expression: K_IF b_expr then expression else expression  */
#line 6574 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                {(yyval.dval) = ((yyvsp[-4].integer) != 0) ? (yyvsp[-2].dval) : (yyvsp[0].dval);}
#line 12863 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 928: /* expression: int_number  */
#line 6575 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                     {(yyval.dval) = (yyvsp[0].dval);}
#line 12869 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 929: /* b_expr: expression relop expression  */
#line 6578 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = comp_num((yyvsp[-2].dval),(yyvsp[-1].integer),(yyvsp[0].dval));}
#line 12875 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 930: /* b_expr: expression K_AND expression  */
#line 6579 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.integer) = (yyvsp[-2].dval) != 0 && (yyvsp[0].dval) != 0;}
#line 12881 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 931: /* b_expr: expression K_OR expression  */
#line 6580 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                {(yyval.integer) = (yyvsp[-2].dval) != 0 || (yyvsp[0].dval) != 0;}
#line 12887 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 932: /* b_expr: s_expr relop s_expr  */
#line 6581 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = comp_str((yyvsp[-2].string),(yyvsp[-1].integer),(yyvsp[0].string));}
#line 12893 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 933: /* b_expr: s_expr K_AND s_expr  */
#line 6582 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-2].string)[0] != 0 && (yyvsp[0].string)[0] != 0;}
#line 12899 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 934: /* b_expr: s_expr K_OR s_expr  */
#line 6583 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-2].string)[0] != 0 || (yyvsp[0].string)[0] != 0;}
#line 12905 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 935: /* b_expr: b_expr K_EQ b_expr  */
#line 6584 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-2].integer) == (yyvsp[0].integer);}
#line 12911 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 936: /* b_expr: b_expr K_NE b_expr  */
#line 6585 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-2].integer) != (yyvsp[0].integer);}
#line 12917 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 937: /* b_expr: b_expr K_AND b_expr  */
#line 6586 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-2].integer) && (yyvsp[0].integer);}
#line 12923 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 938: /* b_expr: b_expr K_OR b_expr  */
#line 6587 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-2].integer) || (yyvsp[0].integer);}
#line 12929 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 939: /* b_expr: K_NOT b_expr  */
#line 6588 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                                               {(yyval.integer) = !(yyval.integer);}
#line 12935 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 940: /* b_expr: '(' b_expr ')'  */
#line 6589 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = (yyvsp[-1].integer);}
#line 12941 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 941: /* b_expr: K_IF b_expr then b_expr else b_expr  */
#line 6591 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
        {(yyval.integer) = ((yyvsp[-4].integer) != 0) ? (yyvsp[-2].integer) : (yyvsp[0].integer);}
#line 12947 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 942: /* b_expr: K_TRUE  */
#line 6592 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = 1;}
#line 12953 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 943: /* b_expr: K_FALSE  */
#line 6593 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                              {(yyval.integer) = 0;}
#line 12959 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 944: /* s_expr: s_expr '+' s_expr  */
#line 6597 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      (yyval.string) = (char*)lefMalloc(strlen((yyvsp[-2].string))+strlen((yyvsp[0].string))+1);
      strcpy((yyval.string),(yyvsp[-2].string));
      strcat((yyval.string),(yyvsp[0].string));
    }
#line 12969 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 945: /* s_expr: '(' s_expr ')'  */
#line 6603 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (yyvsp[-1].string); }
#line 12975 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 946: /* s_expr: K_IF b_expr then s_expr else s_expr  */
#line 6605 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      lefData->lefDefIf = true;
      if ((yyvsp[-4].integer) != 0) {
        (yyval.string) = (yyvsp[-2].string);        
      } else {
        (yyval.string) = (yyvsp[0].string);
      }
    }
#line 12988 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 947: /* s_expr: QSTRING  */
#line 6614 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 12994 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 948: /* relop: K_LE  */
#line 6617 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
       {(yyval.integer) = C_LE;}
#line 13000 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 949: /* relop: K_LT  */
#line 6618 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_LT;}
#line 13006 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 950: /* relop: K_GE  */
#line 6619 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_GE;}
#line 13012 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 951: /* relop: K_GT  */
#line 6620 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_GT;}
#line 13018 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 952: /* relop: K_EQ  */
#line 6621 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_EQ;}
#line 13024 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 953: /* relop: K_NE  */
#line 6622 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_NE;}
#line 13030 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 954: /* relop: '='  */
#line 6623 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_EQ;}
#line 13036 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 955: /* relop: '<'  */
#line 6624 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_LT;}
#line 13042 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 956: /* relop: '>'  */
#line 6625 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
         {(yyval.integer) = C_GT;}
#line 13048 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 957: /* $@122: %empty  */
#line 6629 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropBeginCbk)
        CALLBACK(lefCallbacks->PropBeginCbk, lefrPropBeginCbkType, 0);
    }
#line 13057 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 958: /* prop_def_section: K_PROPDEF $@122 prop_stmts K_END K_PROPDEF  */
#line 6634 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropEndCbk)
        CALLBACK(lefCallbacks->PropEndCbk, lefrPropEndCbkType, 0);
    }
#line 13066 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 959: /* prop_stmts: %empty  */
#line 6641 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13072 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 960: /* prop_stmts: prop_stmts prop_stmt  */
#line 6643 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13078 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 961: /* $@123: %empty  */
#line 6646 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13084 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 962: /* prop_stmt: K_LIBRARY $@123 T_STRING prop_define ';'  */
#line 6648 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("library", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrLibProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13096 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 963: /* $@124: %empty  */
#line 6655 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                   {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13102 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 964: /* prop_stmt: K_COMPONENTPIN $@124 T_STRING prop_define ';'  */
#line 6657 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("componentpin", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrCompProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13114 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 965: /* $@125: %empty  */
#line 6664 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13120 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 966: /* prop_stmt: K_PIN $@125 T_STRING prop_define ';'  */
#line 6666 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("pin", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrPinProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
      
    }
#line 13133 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 967: /* $@126: %empty  */
#line 6674 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13139 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 968: /* prop_stmt: K_MACRO $@126 T_STRING prop_define ';'  */
#line 6676 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("macro", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrMacroProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13151 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 969: /* $@127: %empty  */
#line 6683 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
          {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13157 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 970: /* prop_stmt: K_VIA $@127 T_STRING prop_define ';'  */
#line 6685 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("via", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrViaProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13169 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 971: /* $@128: %empty  */
#line 6692 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
              {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13175 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 972: /* prop_stmt: K_VIARULE $@128 T_STRING prop_define ';'  */
#line 6694 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("viarule", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrViaRuleProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13187 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 973: /* $@129: %empty  */
#line 6701 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13193 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 974: /* prop_stmt: K_LAYER $@129 T_STRING prop_define ';'  */
#line 6703 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("layer", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrLayerProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13205 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 975: /* $@130: %empty  */
#line 6710 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
                     {lefData->lefDumbMode = 1; lefData->lefrProp.clear(); }
#line 13211 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 976: /* prop_stmt: K_NONDEFAULTRULE $@130 T_STRING prop_define ';'  */
#line 6712 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) {
        lefData->lefrProp.setPropType("nondefaultrule", (yyvsp[-2].string));
        CALLBACK(lefCallbacks->PropCbk, lefrPropCbkType, &lefData->lefrProp);
      }
      lefSettings->lefProps.lefrNondefProp.setPropType((yyvsp[-2].string), lefData->lefPropDefType);
    }
#line 13223 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 977: /* prop_define: K_INTEGER opt_def_range opt_def_dvalue  */
#line 6722 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropInteger();
      lefData->lefPropDefType = 'I';
    }
#line 13232 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 978: /* prop_define: K_REAL opt_def_range opt_def_value  */
#line 6727 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropReal();
      lefData->lefPropDefType = 'R';
    }
#line 13241 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 979: /* prop_define: K_STRING  */
#line 6732 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropString();
      lefData->lefPropDefType = 'S';
    }
#line 13250 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 980: /* prop_define: K_STRING QSTRING  */
#line 6737 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropQString((yyvsp[0].string));
      lefData->lefPropDefType = 'Q';
    }
#line 13259 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 981: /* prop_define: K_NAMEMAPSTRING T_STRING  */
#line 6742 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->PropCbk) lefData->lefrProp.setPropNameMapString((yyvsp[0].string));
      lefData->lefPropDefType = 'S';
    }
#line 13268 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 982: /* opt_range_second: %empty  */
#line 6749 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13274 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 983: /* opt_range_second: K_USELENGTHTHRESHOLD  */
#line 6751 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingRangeUseLength();
    }
#line 13283 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 984: /* opt_range_second: K_INFLUENCE int_number  */
#line 6756 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingRangeInfluence((yyvsp[0].dval));
        lefData->lefrLayer.setSpacingRangeInfluenceRange(-1, -1);
      }
    }
#line 13294 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 985: /* opt_range_second: K_INFLUENCE int_number K_RANGE int_number int_number  */
#line 6763 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingRangeInfluence((yyvsp[-3].dval));
        lefData->lefrLayer.setSpacingRangeInfluenceRange((yyvsp[-1].dval), (yyvsp[0].dval));
      }
    }
#line 13305 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 986: /* opt_range_second: K_RANGE int_number int_number  */
#line 6770 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingRangeRange((yyvsp[-1].dval), (yyvsp[0].dval));
    }
#line 13314 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 987: /* opt_endofline: %empty  */
#line 6777 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13320 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 988: /* $@131: %empty  */
#line 6779 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingParSW((yyvsp[-2].dval), (yyvsp[0].dval));
    }
#line 13329 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 990: /* opt_endofline_twoedges: %empty  */
#line 6787 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13335 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 991: /* opt_endofline_twoedges: K_TWOEDGES  */
#line 6789 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingParTwoEdges();
    }
#line 13344 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 992: /* opt_samenetPGonly: %empty  */
#line 6796 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13350 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 993: /* opt_samenetPGonly: K_PGONLY  */
#line 6798 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingSamenetPGonly();
    }
#line 13359 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 994: /* opt_def_range: %empty  */
#line 6805 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13365 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 995: /* opt_def_range: K_RANGE int_number int_number  */
#line 6807 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {  if (lefCallbacks->PropCbk) lefData->lefrProp.setRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 13371 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 996: /* opt_def_value: %empty  */
#line 6811 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13377 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 997: /* opt_def_value: NUMBER  */
#line 6813 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PropCbk) lefData->lefrProp.setNumber((yyvsp[0].dval)); }
#line 13383 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 998: /* opt_def_dvalue: %empty  */
#line 6817 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13389 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 999: /* opt_def_dvalue: int_number  */
#line 6819 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->PropCbk) lefData->lefrProp.setNumber((yyvsp[0].dval)); }
#line 13395 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1002: /* layer_spacing_opt: K_CENTERTOCENTER  */
#line 6826 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13423 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1003: /* $@132: %empty  */
#line 6850 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13441 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1004: /* layer_spacing_opt: K_SAMENET $@132 opt_samenetPGonly  */
#line 6864 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13456 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1005: /* layer_spacing_opt: K_PARALLELOVERLAP  */
#line 6875 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13483 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1007: /* $@133: %empty  */
#line 6900 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 13489 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1008: /* $@134: %empty  */
#line 6901 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        lefData->lefrLayer.setSpacingName((yyvsp[0].string));
      }
    }
#line 13508 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1010: /* $@135: %empty  */
#line 6917 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
        lefData->lefrLayer.setSpacingAdjacent((int)(yyvsp[-2].dval), (yyvsp[0].dval));
      }
    }
#line 13536 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1012: /* layer_spacing_cut_routing: K_AREA NUMBER  */
#line 6942 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          lefData->lefrLayer.setSpacingArea((yyvsp[0].dval));
        }
      }
    }
#line 13563 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1013: /* $@136: %empty  */
#line 6965 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingRange((yyvsp[-1].dval), (yyvsp[0].dval));
    }
#line 13572 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1015: /* layer_spacing_cut_routing: K_LENGTHTHRESHOLD int_number  */
#line 6971 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingLength((yyvsp[0].dval));
      }
    }
#line 13582 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1016: /* layer_spacing_cut_routing: K_LENGTHTHRESHOLD int_number K_RANGE int_number int_number  */
#line 6977 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk) {
        lefData->lefrLayer.setSpacingLength((yyvsp[-3].dval));
        lefData->lefrLayer.setSpacingLengthRange((yyvsp[-1].dval), (yyvsp[0].dval));
      }
    }
#line 13593 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1017: /* $@137: %empty  */
#line 6984 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingEol((yyvsp[-2].dval), (yyvsp[0].dval));
    }
#line 13602 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1018: /* layer_spacing_cut_routing: K_ENDOFLINE int_number K_WITHIN int_number $@137 opt_endofline  */
#line 6989 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13617 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1019: /* layer_spacing_cut_routing: K_NOTCHLENGTH int_number  */
#line 7000 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          lefData->lefrLayer.setSpacingNotchLength((yyvsp[0].dval));
      }
    }
#line 13635 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1020: /* layer_spacing_cut_routing: K_ENDOFNOTCHWIDTH int_number K_NOTCHSPACING int_number K_NOTCHLENGTH int_number  */
#line 7014 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          lefData->lefrLayer.setSpacingEndOfNotchWidth((yyvsp[-4].dval), (yyvsp[-2].dval), (yyvsp[0].dval));
      }
    }
#line 13653 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1021: /* spacing_cut_layer_opt: %empty  */
#line 7030 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {}
#line 13659 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1022: /* spacing_cut_layer_opt: K_STACK  */
#line 7032 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefCallbacks->LayerCbk)
        lefData->lefrLayer.setSpacingLayerStack();
    }
#line 13668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1023: /* opt_adjacentcuts_exceptsame: %empty  */
#line 7039 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {}
#line 13674 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1024: /* opt_adjacentcuts_exceptsame: K_EXCEPTSAMEPGNET  */
#line 7041 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13692 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1025: /* opt_layer_name: %empty  */
#line 7057 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = 0; }
#line 13698 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1026: /* $@138: %empty  */
#line 7058 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
            {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 13704 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1027: /* opt_layer_name: K_LAYER $@138 T_STRING  */
#line 7059 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 13710 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1028: /* $@139: %empty  */
#line 7063 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
           {lefData->lefDumbMode = 1; lefData->lefNoNum = 1; }
#line 13716 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1029: /* req_layer_name: K_LAYER $@139 T_STRING  */
#line 7064 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { (yyval.string) = (yyvsp[0].string); }
#line 13722 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1030: /* universalnoisemargin: K_UNIVERSALNOISEMARGIN int_number int_number ';'  */
#line 7068 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->NoiseMarginCbk) {
          lefData->lefrNoiseMargin.low = (yyvsp[-2].dval);
          lefData->lefrNoiseMargin.high = (yyvsp[-1].dval);
          CALLBACK(lefCallbacks->NoiseMarginCbk, lefrNoiseMarginCbkType, &lefData->lefrNoiseMargin);
        }
      } else
        if (lefCallbacks->NoiseMarginCbk) // write warning only if cbk is set 
          if (lefData->noiseMarginWarnings++ < lefSettings->NoiseMarginWarnings)
            lefWarning(2070, "UNIVERSALNOISEMARGIN statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 13739 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1031: /* edgeratethreshold1: K_EDGERATETHRESHOLD1 int_number ';'  */
#line 7082 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->EdgeRateThreshold1Cbk) {
          CALLBACK(lefCallbacks->EdgeRateThreshold1Cbk,
          lefrEdgeRateThreshold1CbkType, (yyvsp[-1].dval));
        }
      } else
        if (lefCallbacks->EdgeRateThreshold1Cbk) // write warning only if cbk is set 
          if (lefData->edgeRateThreshold1Warnings++ < lefSettings->EdgeRateThreshold1Warnings)
            lefWarning(2071, "EDGERATETHRESHOLD1 statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 13755 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1032: /* edgeratethreshold2: K_EDGERATETHRESHOLD2 int_number ';'  */
#line 7095 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->EdgeRateThreshold2Cbk) {
          CALLBACK(lefCallbacks->EdgeRateThreshold2Cbk,
          lefrEdgeRateThreshold2CbkType, (yyvsp[-1].dval));
        }
      } else
        if (lefCallbacks->EdgeRateThreshold2Cbk) // write warning only if cbk is set 
          if (lefData->edgeRateThreshold2Warnings++ < lefSettings->EdgeRateThreshold2Warnings)
            lefWarning(2072, "EDGERATETHRESHOLD2 statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 13771 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1033: /* edgeratescalefactor: K_EDGERATESCALEFACTOR int_number ';'  */
#line 7108 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    {
      if (lefData->versionNum < 5.4) {
        if (lefCallbacks->EdgeRateScaleFactorCbk) {
          CALLBACK(lefCallbacks->EdgeRateScaleFactorCbk,
          lefrEdgeRateScaleFactorCbkType, (yyvsp[-1].dval));
        }
      } else
        if (lefCallbacks->EdgeRateScaleFactorCbk) // write warning only if cbk is set 
          if (lefData->edgeRateScaleFactorWarnings++ < lefSettings->EdgeRateScaleFactorWarnings)
            lefWarning(2073, "EDGERATESCALEFACTOR statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
    }
#line 13787 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1034: /* $@140: %empty  */
#line 7121 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk) lefData->lefrNoiseTable.setup((int)(yyvsp[0].dval)); }
#line 13793 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1035: /* noisetable: K_NOISETABLE int_number $@140 ';' noise_table_list end_noisetable dtrm  */
#line 7123 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13799 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1036: /* end_noisetable: K_END K_NOISETABLE  */
#line 7127 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  {
    if (lefData->versionNum < 5.4) {
      if (lefCallbacks->NoiseTableCbk)
        CALLBACK(lefCallbacks->NoiseTableCbk, lefrNoiseTableCbkType, &lefData->lefrNoiseTable);
    } else
      if (lefCallbacks->NoiseTableCbk) // write warning only if cbk is set 
        if (lefData->noiseTableWarnings++ < lefSettings->NoiseTableWarnings)
          lefWarning(2074, "NOISETABLE statement is obsolete in version 5.4 and later.\nThe LEF parser will ignore this statement.\nTo avoid this warning in the future, remove this statement from the LEF file with version 5.4 or later.");
  }
#line 13813 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1039: /* noise_table_entry: K_EDGERATE int_number ';'  */
#line 7145 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk)
         {
            lefData->lefrNoiseTable.newEdge();
            lefData->lefrNoiseTable.addEdge((yyvsp[-1].dval));
         }
    }
#line 13824 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1040: /* noise_table_entry: output_resistance_entry  */
#line 7152 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13830 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1041: /* $@141: %empty  */
#line 7155 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk) lefData->lefrNoiseTable.addResistance(); }
#line 13836 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1043: /* num_list: int_number  */
#line 7161 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addResistanceNumber((yyvsp[0].dval)); }
#line 13843 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1044: /* num_list: num_list int_number  */
#line 7164 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addResistanceNumber((yyvsp[0].dval)); }
#line 13850 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1047: /* $@142: %empty  */
#line 7173 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
        { if (lefCallbacks->NoiseTableCbk)
        lefData->lefrNoiseTable.addVictimLength((yyvsp[-1].dval)); }
#line 13857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1048: /* victim: K_VICTIMLENGTH int_number ';' $@142 K_VICTIMNOISE vnoiselist ';'  */
#line 7176 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
        { }
#line 13863 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1049: /* vnoiselist: int_number  */
#line 7180 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addVictimNoise((yyvsp[0].dval)); }
#line 13870 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1050: /* vnoiselist: vnoiselist int_number  */
#line 7183 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->NoiseTableCbk)
    lefData->lefrNoiseTable.addVictimNoise((yyvsp[0].dval)); }
#line 13877 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1051: /* $@143: %empty  */
#line 7187 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->CorrectionTableCbk)
    lefData->lefrCorrectionTable.setup((int)(yyvsp[-1].dval)); }
#line 13884 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1052: /* correctiontable: K_CORRECTIONTABLE int_number ';' $@143 correction_table_list end_correctiontable dtrm  */
#line 7190 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13890 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1053: /* end_correctiontable: K_END K_CORRECTIONTABLE  */
#line 7194 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
#line 13905 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1056: /* correction_table_item: K_EDGERATE int_number ';'  */
#line 7212 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->CorrectionTableCbk)
         {
            lefData->lefrCorrectionTable.newEdge();
            lefData->lefrCorrectionTable.addEdge((yyvsp[-1].dval));
         }
    }
#line 13916 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1057: /* correction_table_item: output_list  */
#line 7219 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { }
#line 13922 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1058: /* $@144: %empty  */
#line 7222 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  { if (lefCallbacks->CorrectionTableCbk)
  lefData->lefrCorrectionTable.addResistance(); }
#line 13929 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1059: /* output_list: K_OUTPUTRESISTANCE $@144 numo_list ';' corr_victim_list  */
#line 7225 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
  { }
#line 13935 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1060: /* numo_list: int_number  */
#line 7229 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->CorrectionTableCbk)
    lefData->lefrCorrectionTable.addResistanceNumber((yyvsp[0].dval)); }
#line 13942 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1061: /* numo_list: numo_list int_number  */
#line 7232 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->CorrectionTableCbk)
    lefData->lefrCorrectionTable.addResistanceNumber((yyvsp[0].dval)); }
#line 13949 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1064: /* $@145: %empty  */
#line 7242 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
     { if (lefCallbacks->CorrectionTableCbk)
     lefData->lefrCorrectionTable.addVictimLength((yyvsp[-1].dval)); }
#line 13956 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1065: /* corr_victim: K_VICTIMLENGTH int_number ';' $@145 K_CORRECTIONFACTOR corr_list ';'  */
#line 7245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
     { }
#line 13962 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1066: /* corr_list: int_number  */
#line 7249 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->CorrectionTableCbk)
        lefData->lefrCorrectionTable.addVictimCorrection((yyvsp[0].dval)); }
#line 13969 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1067: /* corr_list: corr_list int_number  */
#line 7252 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { if (lefCallbacks->CorrectionTableCbk)
        lefData->lefrCorrectionTable.addVictimCorrection((yyvsp[0].dval)); }
#line 13976 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1068: /* input_antenna: K_INPUTPINANTENNASIZE int_number ';'  */
#line 7258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          CALLBACK(lefCallbacks->InputAntennaCbk, lefrInputAntennaCbkType, (yyvsp[-1].dval));
    }
#line 14003 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1069: /* output_antenna: K_OUTPUTPINANTENNASIZE int_number ';'  */
#line 7282 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          CALLBACK(lefCallbacks->OutputAntennaCbk, lefrOutputAntennaCbkType, (yyvsp[-1].dval));
    }
#line 14030 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1070: /* inout_antenna: K_INOUTPINANTENNASIZE int_number ';'  */
#line 7306 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          CALLBACK(lefCallbacks->InoutAntennaCbk, lefrInoutAntennaCbkType, (yyvsp[-1].dval));
    }
#line 14057 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1071: /* antenna_input: K_ANTENNAINPUTGATEAREA NUMBER ';'  */
#line 7330 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          CALLBACK(lefCallbacks->AntennaInputCbk, lefrAntennaInputCbkType, (yyvsp[-1].dval));
    }
#line 14094 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1072: /* antenna_inout: K_ANTENNAINOUTDIFFAREA NUMBER ';'  */
#line 7364 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          CALLBACK(lefCallbacks->AntennaInoutCbk, lefrAntennaInoutCbkType, (yyvsp[-1].dval));
    }
#line 14131 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1073: /* antenna_output: K_ANTENNAOUTPUTDIFFAREA NUMBER ';'  */
#line 7398 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
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
          CALLBACK(lefCallbacks->AntennaOutputCbk, lefrAntennaOutputCbkType, (yyvsp[-1].dval));
    }
#line 14168 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;

  case 1076: /* extension: K_BEGINEXT  */
#line 7435 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"
    { 
        if (lefCallbacks->ExtensionCbk)
          CALLBACK(lefCallbacks->ExtensionCbk, lefrExtensionCbkType, &lefData->Hist_text[0]);
        if (lefData->versionNum >= 5.6)
           lefData->ge56almostDone = 1;
    }
#line 14179 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"
    break;


#line 14183 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
    }

  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 7442 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"


END_LEF_PARSER_NAMESPACE
