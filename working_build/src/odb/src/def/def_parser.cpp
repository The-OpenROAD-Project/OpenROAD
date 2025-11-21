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
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         defyyparse
#define yylex           defyylex
#define yyerror         defyyerror
#define yydebug         defyydebug
#define yynerrs         defyynerrs

/* First part of user prologue.  */
#line 58 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include "defrReader.hpp"
#include "defiUser.hpp"
#include "defrCallBacks.hpp"

#define DEF_MAX_INT 2147483647
#define YYDEBUG 1     // this is temp fix for pcr 755132 
// TX_DIR:TRANSLATION ON


#include "defrData.hpp"
#include "defrSettings.hpp"
#include "defrCallBacks.hpp"

BEGIN_DEF_PARSER_NAMESPACE

using std::round;
  
// Macro to describe how we handle a callback.
// If the function was set then call it.
// If the function returns non zero then there was an error
// so call the error routine and exit.
//
#define CALLBACK(func, typ, data) \
    if (!defData->errors) {\
      if (func) { \
        if ((defData->defRetVal = (*func)(typ, data, defData->session->UserData)) == PARSE_OK) { \
        } else if (defData->defRetVal == STOP_PARSE) { \
          return defData->defRetVal; \
        } else { \
          defData->defError(6010, "An error has been reported in callback."); \
          return defData->defRetVal; \
        } \
      } \
    }

#define CHKERR() \
    if (defData->checkErrors()) { \
      return 1; \
    }

#define CHKPROPTYPE(propType, propName, name) \
    if (propType == 'N') { \
       defData->warningMsg = (char*)malloc(strlen(propName)+strlen(name)+40); \
       sprintf(defData->warningMsg, "The PropName %s is not defined for %s.", \
               propName, name); \
       defData->defWarning(7010, defData->warningMsg); \
       free(defData->warningMsg); \
    }

int yylex(YYSTYPE *pYylval, defrData *defData)
{
    return defData->defyylex(pYylval);
}


void yyerror(defrData *defData, const char *s)
{
    return defData->defyyerror(s);
}




#define FIXED 1
#define COVER 2
#define PLACED 3
#define UNPLACED 4

#line 150 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"

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

#include "def_parser.hpp"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_QSTRING = 3,                    /* QSTRING  */
  YYSYMBOL_T_STRING = 4,                   /* T_STRING  */
  YYSYMBOL_SITE_PATTERN = 5,               /* SITE_PATTERN  */
  YYSYMBOL_NUMBER = 6,                     /* NUMBER  */
  YYSYMBOL_K_HISTORY = 7,                  /* K_HISTORY  */
  YYSYMBOL_K_NAMESCASESENSITIVE = 8,       /* K_NAMESCASESENSITIVE  */
  YYSYMBOL_K_DESIGN = 9,                   /* K_DESIGN  */
  YYSYMBOL_K_VIAS = 10,                    /* K_VIAS  */
  YYSYMBOL_K_TECH = 11,                    /* K_TECH  */
  YYSYMBOL_K_UNITS = 12,                   /* K_UNITS  */
  YYSYMBOL_K_ARRAY = 13,                   /* K_ARRAY  */
  YYSYMBOL_K_FLOORPLAN = 14,               /* K_FLOORPLAN  */
  YYSYMBOL_K_SITE = 15,                    /* K_SITE  */
  YYSYMBOL_K_CANPLACE = 16,                /* K_CANPLACE  */
  YYSYMBOL_K_CANNOTOCCUPY = 17,            /* K_CANNOTOCCUPY  */
  YYSYMBOL_K_DIEAREA = 18,                 /* K_DIEAREA  */
  YYSYMBOL_K_PINS = 19,                    /* K_PINS  */
  YYSYMBOL_K_DEFAULTCAP = 20,              /* K_DEFAULTCAP  */
  YYSYMBOL_K_MINPINS = 21,                 /* K_MINPINS  */
  YYSYMBOL_K_WIRECAP = 22,                 /* K_WIRECAP  */
  YYSYMBOL_K_TRACKS = 23,                  /* K_TRACKS  */
  YYSYMBOL_K_GCELLGRID = 24,               /* K_GCELLGRID  */
  YYSYMBOL_K_DO = 25,                      /* K_DO  */
  YYSYMBOL_K_BY = 26,                      /* K_BY  */
  YYSYMBOL_K_STEP = 27,                    /* K_STEP  */
  YYSYMBOL_K_LAYER = 28,                   /* K_LAYER  */
  YYSYMBOL_K_ROW = 29,                     /* K_ROW  */
  YYSYMBOL_K_RECT = 30,                    /* K_RECT  */
  YYSYMBOL_K_COMPS = 31,                   /* K_COMPS  */
  YYSYMBOL_K_COMP_GEN = 32,                /* K_COMP_GEN  */
  YYSYMBOL_K_SOURCE = 33,                  /* K_SOURCE  */
  YYSYMBOL_K_WEIGHT = 34,                  /* K_WEIGHT  */
  YYSYMBOL_K_EEQMASTER = 35,               /* K_EEQMASTER  */
  YYSYMBOL_K_FIXED = 36,                   /* K_FIXED  */
  YYSYMBOL_K_COVER = 37,                   /* K_COVER  */
  YYSYMBOL_K_UNPLACED = 38,                /* K_UNPLACED  */
  YYSYMBOL_K_PLACED = 39,                  /* K_PLACED  */
  YYSYMBOL_K_FOREIGN = 40,                 /* K_FOREIGN  */
  YYSYMBOL_K_REGION = 41,                  /* K_REGION  */
  YYSYMBOL_K_REGIONS = 42,                 /* K_REGIONS  */
  YYSYMBOL_K_NETS = 43,                    /* K_NETS  */
  YYSYMBOL_K_START_NET = 44,               /* K_START_NET  */
  YYSYMBOL_K_MUSTJOIN = 45,                /* K_MUSTJOIN  */
  YYSYMBOL_K_ORIGINAL = 46,                /* K_ORIGINAL  */
  YYSYMBOL_K_USE = 47,                     /* K_USE  */
  YYSYMBOL_K_STYLE = 48,                   /* K_STYLE  */
  YYSYMBOL_K_PATTERN = 49,                 /* K_PATTERN  */
  YYSYMBOL_K_PATTERNNAME = 50,             /* K_PATTERNNAME  */
  YYSYMBOL_K_ESTCAP = 51,                  /* K_ESTCAP  */
  YYSYMBOL_K_ROUTED = 52,                  /* K_ROUTED  */
  YYSYMBOL_K_NEW = 53,                     /* K_NEW  */
  YYSYMBOL_K_SNETS = 54,                   /* K_SNETS  */
  YYSYMBOL_K_SHAPE = 55,                   /* K_SHAPE  */
  YYSYMBOL_K_WIDTH = 56,                   /* K_WIDTH  */
  YYSYMBOL_K_VOLTAGE = 57,                 /* K_VOLTAGE  */
  YYSYMBOL_K_SPACING = 58,                 /* K_SPACING  */
  YYSYMBOL_K_NONDEFAULTRULE = 59,          /* K_NONDEFAULTRULE  */
  YYSYMBOL_K_NONDEFAULTRULES = 60,         /* K_NONDEFAULTRULES  */
  YYSYMBOL_K_N = 61,                       /* K_N  */
  YYSYMBOL_K_S = 62,                       /* K_S  */
  YYSYMBOL_K_E = 63,                       /* K_E  */
  YYSYMBOL_K_W = 64,                       /* K_W  */
  YYSYMBOL_K_FN = 65,                      /* K_FN  */
  YYSYMBOL_K_FE = 66,                      /* K_FE  */
  YYSYMBOL_K_FS = 67,                      /* K_FS  */
  YYSYMBOL_K_FW = 68,                      /* K_FW  */
  YYSYMBOL_K_GROUPS = 69,                  /* K_GROUPS  */
  YYSYMBOL_K_GROUP = 70,                   /* K_GROUP  */
  YYSYMBOL_K_SOFT = 71,                    /* K_SOFT  */
  YYSYMBOL_K_MAXX = 72,                    /* K_MAXX  */
  YYSYMBOL_K_MAXY = 73,                    /* K_MAXY  */
  YYSYMBOL_K_MAXHALFPERIMETER = 74,        /* K_MAXHALFPERIMETER  */
  YYSYMBOL_K_CONSTRAINTS = 75,             /* K_CONSTRAINTS  */
  YYSYMBOL_K_NET = 76,                     /* K_NET  */
  YYSYMBOL_K_PATH = 77,                    /* K_PATH  */
  YYSYMBOL_K_SUM = 78,                     /* K_SUM  */
  YYSYMBOL_K_DIFF = 79,                    /* K_DIFF  */
  YYSYMBOL_K_SCANCHAINS = 80,              /* K_SCANCHAINS  */
  YYSYMBOL_K_START = 81,                   /* K_START  */
  YYSYMBOL_K_FLOATING = 82,                /* K_FLOATING  */
  YYSYMBOL_K_ORDERED = 83,                 /* K_ORDERED  */
  YYSYMBOL_K_STOP = 84,                    /* K_STOP  */
  YYSYMBOL_K_IN = 85,                      /* K_IN  */
  YYSYMBOL_K_OUT = 86,                     /* K_OUT  */
  YYSYMBOL_K_RISEMIN = 87,                 /* K_RISEMIN  */
  YYSYMBOL_K_RISEMAX = 88,                 /* K_RISEMAX  */
  YYSYMBOL_K_FALLMIN = 89,                 /* K_FALLMIN  */
  YYSYMBOL_K_FALLMAX = 90,                 /* K_FALLMAX  */
  YYSYMBOL_K_WIREDLOGIC = 91,              /* K_WIREDLOGIC  */
  YYSYMBOL_K_MAXDIST = 92,                 /* K_MAXDIST  */
  YYSYMBOL_K_ASSERTIONS = 93,              /* K_ASSERTIONS  */
  YYSYMBOL_K_DISTANCE = 94,                /* K_DISTANCE  */
  YYSYMBOL_K_MICRONS = 95,                 /* K_MICRONS  */
  YYSYMBOL_K_END = 96,                     /* K_END  */
  YYSYMBOL_K_IOTIMINGS = 97,               /* K_IOTIMINGS  */
  YYSYMBOL_K_RISE = 98,                    /* K_RISE  */
  YYSYMBOL_K_FALL = 99,                    /* K_FALL  */
  YYSYMBOL_K_VARIABLE = 100,               /* K_VARIABLE  */
  YYSYMBOL_K_SLEWRATE = 101,               /* K_SLEWRATE  */
  YYSYMBOL_K_CAPACITANCE = 102,            /* K_CAPACITANCE  */
  YYSYMBOL_K_DRIVECELL = 103,              /* K_DRIVECELL  */
  YYSYMBOL_K_FROMPIN = 104,                /* K_FROMPIN  */
  YYSYMBOL_K_TOPIN = 105,                  /* K_TOPIN  */
  YYSYMBOL_K_PARALLEL = 106,               /* K_PARALLEL  */
  YYSYMBOL_K_TIMINGDISABLES = 107,         /* K_TIMINGDISABLES  */
  YYSYMBOL_K_THRUPIN = 108,                /* K_THRUPIN  */
  YYSYMBOL_K_MACRO = 109,                  /* K_MACRO  */
  YYSYMBOL_K_PARTITIONS = 110,             /* K_PARTITIONS  */
  YYSYMBOL_K_TURNOFF = 111,                /* K_TURNOFF  */
  YYSYMBOL_K_FROMCLOCKPIN = 112,           /* K_FROMCLOCKPIN  */
  YYSYMBOL_K_FROMCOMPPIN = 113,            /* K_FROMCOMPPIN  */
  YYSYMBOL_K_FROMIOPIN = 114,              /* K_FROMIOPIN  */
  YYSYMBOL_K_TOCLOCKPIN = 115,             /* K_TOCLOCKPIN  */
  YYSYMBOL_K_TOCOMPPIN = 116,              /* K_TOCOMPPIN  */
  YYSYMBOL_K_TOIOPIN = 117,                /* K_TOIOPIN  */
  YYSYMBOL_K_SETUPRISE = 118,              /* K_SETUPRISE  */
  YYSYMBOL_K_SETUPFALL = 119,              /* K_SETUPFALL  */
  YYSYMBOL_K_HOLDRISE = 120,               /* K_HOLDRISE  */
  YYSYMBOL_K_HOLDFALL = 121,               /* K_HOLDFALL  */
  YYSYMBOL_K_VPIN = 122,                   /* K_VPIN  */
  YYSYMBOL_K_SUBNET = 123,                 /* K_SUBNET  */
  YYSYMBOL_K_XTALK = 124,                  /* K_XTALK  */
  YYSYMBOL_K_PIN = 125,                    /* K_PIN  */
  YYSYMBOL_K_SYNTHESIZED = 126,            /* K_SYNTHESIZED  */
  YYSYMBOL_K_DEFINE = 127,                 /* K_DEFINE  */
  YYSYMBOL_K_DEFINES = 128,                /* K_DEFINES  */
  YYSYMBOL_K_DEFINEB = 129,                /* K_DEFINEB  */
  YYSYMBOL_K_IF = 130,                     /* K_IF  */
  YYSYMBOL_K_THEN = 131,                   /* K_THEN  */
  YYSYMBOL_K_ELSE = 132,                   /* K_ELSE  */
  YYSYMBOL_K_FALSE = 133,                  /* K_FALSE  */
  YYSYMBOL_K_TRUE = 134,                   /* K_TRUE  */
  YYSYMBOL_K_EQ = 135,                     /* K_EQ  */
  YYSYMBOL_K_NE = 136,                     /* K_NE  */
  YYSYMBOL_K_LE = 137,                     /* K_LE  */
  YYSYMBOL_K_LT = 138,                     /* K_LT  */
  YYSYMBOL_K_GE = 139,                     /* K_GE  */
  YYSYMBOL_K_GT = 140,                     /* K_GT  */
  YYSYMBOL_K_OR = 141,                     /* K_OR  */
  YYSYMBOL_K_AND = 142,                    /* K_AND  */
  YYSYMBOL_K_NOT = 143,                    /* K_NOT  */
  YYSYMBOL_K_SPECIAL = 144,                /* K_SPECIAL  */
  YYSYMBOL_K_DIRECTION = 145,              /* K_DIRECTION  */
  YYSYMBOL_K_RANGE = 146,                  /* K_RANGE  */
  YYSYMBOL_K_FPC = 147,                    /* K_FPC  */
  YYSYMBOL_K_HORIZONTAL = 148,             /* K_HORIZONTAL  */
  YYSYMBOL_K_VERTICAL = 149,               /* K_VERTICAL  */
  YYSYMBOL_K_ALIGN = 150,                  /* K_ALIGN  */
  YYSYMBOL_K_MIN = 151,                    /* K_MIN  */
  YYSYMBOL_K_MAX = 152,                    /* K_MAX  */
  YYSYMBOL_K_EQUAL = 153,                  /* K_EQUAL  */
  YYSYMBOL_K_BOTTOMLEFT = 154,             /* K_BOTTOMLEFT  */
  YYSYMBOL_K_TOPRIGHT = 155,               /* K_TOPRIGHT  */
  YYSYMBOL_K_ROWS = 156,                   /* K_ROWS  */
  YYSYMBOL_K_TAPER = 157,                  /* K_TAPER  */
  YYSYMBOL_K_TAPERRULE = 158,              /* K_TAPERRULE  */
  YYSYMBOL_K_VERSION = 159,                /* K_VERSION  */
  YYSYMBOL_K_DIVIDERCHAR = 160,            /* K_DIVIDERCHAR  */
  YYSYMBOL_K_BUSBITCHARS = 161,            /* K_BUSBITCHARS  */
  YYSYMBOL_K_PROPERTYDEFINITIONS = 162,    /* K_PROPERTYDEFINITIONS  */
  YYSYMBOL_K_STRING = 163,                 /* K_STRING  */
  YYSYMBOL_K_REAL = 164,                   /* K_REAL  */
  YYSYMBOL_K_INTEGER = 165,                /* K_INTEGER  */
  YYSYMBOL_K_PROPERTY = 166,               /* K_PROPERTY  */
  YYSYMBOL_K_BEGINEXT = 167,               /* K_BEGINEXT  */
  YYSYMBOL_K_ENDEXT = 168,                 /* K_ENDEXT  */
  YYSYMBOL_K_NAMEMAPSTRING = 169,          /* K_NAMEMAPSTRING  */
  YYSYMBOL_K_ON = 170,                     /* K_ON  */
  YYSYMBOL_K_OFF = 171,                    /* K_OFF  */
  YYSYMBOL_K_X = 172,                      /* K_X  */
  YYSYMBOL_K_Y = 173,                      /* K_Y  */
  YYSYMBOL_K_COMPONENT = 174,              /* K_COMPONENT  */
  YYSYMBOL_K_MASK = 175,                   /* K_MASK  */
  YYSYMBOL_K_MASKSHIFT = 176,              /* K_MASKSHIFT  */
  YYSYMBOL_K_COMPSMASKSHIFT = 177,         /* K_COMPSMASKSHIFT  */
  YYSYMBOL_K_SAMEMASK = 178,               /* K_SAMEMASK  */
  YYSYMBOL_K_PINPROPERTIES = 179,          /* K_PINPROPERTIES  */
  YYSYMBOL_K_TEST = 180,                   /* K_TEST  */
  YYSYMBOL_K_COMMONSCANPINS = 181,         /* K_COMMONSCANPINS  */
  YYSYMBOL_K_SNET = 182,                   /* K_SNET  */
  YYSYMBOL_K_COMPONENTPIN = 183,           /* K_COMPONENTPIN  */
  YYSYMBOL_K_REENTRANTPATHS = 184,         /* K_REENTRANTPATHS  */
  YYSYMBOL_K_SHIELD = 185,                 /* K_SHIELD  */
  YYSYMBOL_K_SHIELDNET = 186,              /* K_SHIELDNET  */
  YYSYMBOL_K_NOSHIELD = 187,               /* K_NOSHIELD  */
  YYSYMBOL_K_VIRTUAL = 188,                /* K_VIRTUAL  */
  YYSYMBOL_K_ANTENNAPINPARTIALMETALAREA = 189, /* K_ANTENNAPINPARTIALMETALAREA  */
  YYSYMBOL_K_ANTENNAPINPARTIALMETALSIDEAREA = 190, /* K_ANTENNAPINPARTIALMETALSIDEAREA  */
  YYSYMBOL_K_ANTENNAPINGATEAREA = 191,     /* K_ANTENNAPINGATEAREA  */
  YYSYMBOL_K_ANTENNAPINDIFFAREA = 192,     /* K_ANTENNAPINDIFFAREA  */
  YYSYMBOL_K_ANTENNAPINMAXAREACAR = 193,   /* K_ANTENNAPINMAXAREACAR  */
  YYSYMBOL_K_ANTENNAPINMAXSIDEAREACAR = 194, /* K_ANTENNAPINMAXSIDEAREACAR  */
  YYSYMBOL_K_ANTENNAPINPARTIALCUTAREA = 195, /* K_ANTENNAPINPARTIALCUTAREA  */
  YYSYMBOL_K_ANTENNAPINMAXCUTCAR = 196,    /* K_ANTENNAPINMAXCUTCAR  */
  YYSYMBOL_K_SIGNAL = 197,                 /* K_SIGNAL  */
  YYSYMBOL_K_POWER = 198,                  /* K_POWER  */
  YYSYMBOL_K_GROUND = 199,                 /* K_GROUND  */
  YYSYMBOL_K_CLOCK = 200,                  /* K_CLOCK  */
  YYSYMBOL_K_TIEOFF = 201,                 /* K_TIEOFF  */
  YYSYMBOL_K_ANALOG = 202,                 /* K_ANALOG  */
  YYSYMBOL_K_SCAN = 203,                   /* K_SCAN  */
  YYSYMBOL_K_RESET = 204,                  /* K_RESET  */
  YYSYMBOL_K_RING = 205,                   /* K_RING  */
  YYSYMBOL_K_STRIPE = 206,                 /* K_STRIPE  */
  YYSYMBOL_K_FOLLOWPIN = 207,              /* K_FOLLOWPIN  */
  YYSYMBOL_K_IOWIRE = 208,                 /* K_IOWIRE  */
  YYSYMBOL_K_COREWIRE = 209,               /* K_COREWIRE  */
  YYSYMBOL_K_BLOCKWIRE = 210,              /* K_BLOCKWIRE  */
  YYSYMBOL_K_FILLWIRE = 211,               /* K_FILLWIRE  */
  YYSYMBOL_K_BLOCKAGEWIRE = 212,           /* K_BLOCKAGEWIRE  */
  YYSYMBOL_K_PADRING = 213,                /* K_PADRING  */
  YYSYMBOL_K_BLOCKRING = 214,              /* K_BLOCKRING  */
  YYSYMBOL_K_BLOCKAGES = 215,              /* K_BLOCKAGES  */
  YYSYMBOL_K_PLACEMENT = 216,              /* K_PLACEMENT  */
  YYSYMBOL_K_SLOTS = 217,                  /* K_SLOTS  */
  YYSYMBOL_K_FILLS = 218,                  /* K_FILLS  */
  YYSYMBOL_K_PUSHDOWN = 219,               /* K_PUSHDOWN  */
  YYSYMBOL_K_NETLIST = 220,                /* K_NETLIST  */
  YYSYMBOL_K_DIST = 221,                   /* K_DIST  */
  YYSYMBOL_K_USER = 222,                   /* K_USER  */
  YYSYMBOL_K_TIMING = 223,                 /* K_TIMING  */
  YYSYMBOL_K_BALANCED = 224,               /* K_BALANCED  */
  YYSYMBOL_K_STEINER = 225,                /* K_STEINER  */
  YYSYMBOL_K_TRUNK = 226,                  /* K_TRUNK  */
  YYSYMBOL_K_FIXEDBUMP = 227,              /* K_FIXEDBUMP  */
  YYSYMBOL_K_FENCE = 228,                  /* K_FENCE  */
  YYSYMBOL_K_FREQUENCY = 229,              /* K_FREQUENCY  */
  YYSYMBOL_K_GUIDE = 230,                  /* K_GUIDE  */
  YYSYMBOL_K_MAXBITS = 231,                /* K_MAXBITS  */
  YYSYMBOL_K_PARTITION = 232,              /* K_PARTITION  */
  YYSYMBOL_K_TYPE = 233,                   /* K_TYPE  */
  YYSYMBOL_K_ANTENNAMODEL = 234,           /* K_ANTENNAMODEL  */
  YYSYMBOL_K_DRCFILL = 235,                /* K_DRCFILL  */
  YYSYMBOL_K_OXIDE1 = 236,                 /* K_OXIDE1  */
  YYSYMBOL_K_OXIDE2 = 237,                 /* K_OXIDE2  */
  YYSYMBOL_K_OXIDE3 = 238,                 /* K_OXIDE3  */
  YYSYMBOL_K_OXIDE4 = 239,                 /* K_OXIDE4  */
  YYSYMBOL_K_OXIDE5 = 240,                 /* K_OXIDE5  */
  YYSYMBOL_K_OXIDE6 = 241,                 /* K_OXIDE6  */
  YYSYMBOL_K_OXIDE7 = 242,                 /* K_OXIDE7  */
  YYSYMBOL_K_OXIDE8 = 243,                 /* K_OXIDE8  */
  YYSYMBOL_K_OXIDE9 = 244,                 /* K_OXIDE9  */
  YYSYMBOL_K_OXIDE10 = 245,                /* K_OXIDE10  */
  YYSYMBOL_K_OXIDE11 = 246,                /* K_OXIDE11  */
  YYSYMBOL_K_OXIDE12 = 247,                /* K_OXIDE12  */
  YYSYMBOL_K_OXIDE13 = 248,                /* K_OXIDE13  */
  YYSYMBOL_K_OXIDE14 = 249,                /* K_OXIDE14  */
  YYSYMBOL_K_OXIDE15 = 250,                /* K_OXIDE15  */
  YYSYMBOL_K_OXIDE16 = 251,                /* K_OXIDE16  */
  YYSYMBOL_K_OXIDE17 = 252,                /* K_OXIDE17  */
  YYSYMBOL_K_OXIDE18 = 253,                /* K_OXIDE18  */
  YYSYMBOL_K_OXIDE19 = 254,                /* K_OXIDE19  */
  YYSYMBOL_K_OXIDE20 = 255,                /* K_OXIDE20  */
  YYSYMBOL_K_OXIDE21 = 256,                /* K_OXIDE21  */
  YYSYMBOL_K_OXIDE22 = 257,                /* K_OXIDE22  */
  YYSYMBOL_K_OXIDE23 = 258,                /* K_OXIDE23  */
  YYSYMBOL_K_OXIDE24 = 259,                /* K_OXIDE24  */
  YYSYMBOL_K_OXIDE25 = 260,                /* K_OXIDE25  */
  YYSYMBOL_K_OXIDE26 = 261,                /* K_OXIDE26  */
  YYSYMBOL_K_OXIDE27 = 262,                /* K_OXIDE27  */
  YYSYMBOL_K_OXIDE28 = 263,                /* K_OXIDE28  */
  YYSYMBOL_K_OXIDE29 = 264,                /* K_OXIDE29  */
  YYSYMBOL_K_OXIDE30 = 265,                /* K_OXIDE30  */
  YYSYMBOL_K_OXIDE31 = 266,                /* K_OXIDE31  */
  YYSYMBOL_K_OXIDE32 = 267,                /* K_OXIDE32  */
  YYSYMBOL_K_CUTSIZE = 268,                /* K_CUTSIZE  */
  YYSYMBOL_K_CUTSPACING = 269,             /* K_CUTSPACING  */
  YYSYMBOL_K_DESIGNRULEWIDTH = 270,        /* K_DESIGNRULEWIDTH  */
  YYSYMBOL_K_DIAGWIDTH = 271,              /* K_DIAGWIDTH  */
  YYSYMBOL_K_ENCLOSURE = 272,              /* K_ENCLOSURE  */
  YYSYMBOL_K_HALO = 273,                   /* K_HALO  */
  YYSYMBOL_K_GROUNDSENSITIVITY = 274,      /* K_GROUNDSENSITIVITY  */
  YYSYMBOL_K_HARDSPACING = 275,            /* K_HARDSPACING  */
  YYSYMBOL_K_LAYERS = 276,                 /* K_LAYERS  */
  YYSYMBOL_K_MINCUTS = 277,                /* K_MINCUTS  */
  YYSYMBOL_K_NETEXPR = 278,                /* K_NETEXPR  */
  YYSYMBOL_K_OFFSET = 279,                 /* K_OFFSET  */
  YYSYMBOL_K_ORIGIN = 280,                 /* K_ORIGIN  */
  YYSYMBOL_K_ROWCOL = 281,                 /* K_ROWCOL  */
  YYSYMBOL_K_STYLES = 282,                 /* K_STYLES  */
  YYSYMBOL_K_POLYGON = 283,                /* K_POLYGON  */
  YYSYMBOL_K_PORT = 284,                   /* K_PORT  */
  YYSYMBOL_K_SUPPLYSENSITIVITY = 285,      /* K_SUPPLYSENSITIVITY  */
  YYSYMBOL_K_VIA = 286,                    /* K_VIA  */
  YYSYMBOL_K_VIARULE = 287,                /* K_VIARULE  */
  YYSYMBOL_K_WIREEXT = 288,                /* K_WIREEXT  */
  YYSYMBOL_K_EXCEPTPGNET = 289,            /* K_EXCEPTPGNET  */
  YYSYMBOL_K_FILLWIREOPC = 290,            /* K_FILLWIREOPC  */
  YYSYMBOL_K_OPC = 291,                    /* K_OPC  */
  YYSYMBOL_K_PARTIAL = 292,                /* K_PARTIAL  */
  YYSYMBOL_K_ROUTEHALO = 293,              /* K_ROUTEHALO  */
  YYSYMBOL_294_ = 294,                     /* ';'  */
  YYSYMBOL_295_ = 295,                     /* '-'  */
  YYSYMBOL_296_ = 296,                     /* '+'  */
  YYSYMBOL_297_ = 297,                     /* '('  */
  YYSYMBOL_298_ = 298,                     /* ')'  */
  YYSYMBOL_299_ = 299,                     /* '*'  */
  YYSYMBOL_300_ = 300,                     /* ','  */
  YYSYMBOL_YYACCEPT = 301,                 /* $accept  */
  YYSYMBOL_def_file = 302,                 /* def_file  */
  YYSYMBOL_version_stmt = 303,             /* version_stmt  */
  YYSYMBOL_304_1 = 304,                    /* $@1  */
  YYSYMBOL_case_sens_stmt = 305,           /* case_sens_stmt  */
  YYSYMBOL_rules = 306,                    /* rules  */
  YYSYMBOL_rule = 307,                     /* rule  */
  YYSYMBOL_design_section = 308,           /* design_section  */
  YYSYMBOL_design_name = 309,              /* design_name  */
  YYSYMBOL_310_2 = 310,                    /* $@2  */
  YYSYMBOL_end_design = 311,               /* end_design  */
  YYSYMBOL_tech_name = 312,                /* tech_name  */
  YYSYMBOL_313_3 = 313,                    /* $@3  */
  YYSYMBOL_array_name = 314,               /* array_name  */
  YYSYMBOL_315_4 = 315,                    /* $@4  */
  YYSYMBOL_floorplan_name = 316,           /* floorplan_name  */
  YYSYMBOL_317_5 = 317,                    /* $@5  */
  YYSYMBOL_history = 318,                  /* history  */
  YYSYMBOL_prop_def_section = 319,         /* prop_def_section  */
  YYSYMBOL_320_6 = 320,                    /* $@6  */
  YYSYMBOL_property_defs = 321,            /* property_defs  */
  YYSYMBOL_property_def = 322,             /* property_def  */
  YYSYMBOL_323_7 = 323,                    /* $@7  */
  YYSYMBOL_324_8 = 324,                    /* $@8  */
  YYSYMBOL_325_9 = 325,                    /* $@9  */
  YYSYMBOL_326_10 = 326,                   /* $@10  */
  YYSYMBOL_327_11 = 327,                   /* $@11  */
  YYSYMBOL_328_12 = 328,                   /* $@12  */
  YYSYMBOL_329_13 = 329,                   /* $@13  */
  YYSYMBOL_330_14 = 330,                   /* $@14  */
  YYSYMBOL_331_15 = 331,                   /* $@15  */
  YYSYMBOL_property_type_and_val = 332,    /* property_type_and_val  */
  YYSYMBOL_333_16 = 333,                   /* $@16  */
  YYSYMBOL_334_17 = 334,                   /* $@17  */
  YYSYMBOL_opt_num_val = 335,              /* opt_num_val  */
  YYSYMBOL_units = 336,                    /* units  */
  YYSYMBOL_divider_char = 337,             /* divider_char  */
  YYSYMBOL_bus_bit_chars = 338,            /* bus_bit_chars  */
  YYSYMBOL_canplace = 339,                 /* canplace  */
  YYSYMBOL_340_18 = 340,                   /* $@18  */
  YYSYMBOL_cannotoccupy = 341,             /* cannotoccupy  */
  YYSYMBOL_342_19 = 342,                   /* $@19  */
  YYSYMBOL_orient = 343,                   /* orient  */
  YYSYMBOL_die_area = 344,                 /* die_area  */
  YYSYMBOL_345_20 = 345,                   /* $@20  */
  YYSYMBOL_pin_cap_rule = 346,             /* pin_cap_rule  */
  YYSYMBOL_start_def_cap = 347,            /* start_def_cap  */
  YYSYMBOL_pin_caps = 348,                 /* pin_caps  */
  YYSYMBOL_pin_cap = 349,                  /* pin_cap  */
  YYSYMBOL_end_def_cap = 350,              /* end_def_cap  */
  YYSYMBOL_pin_rule = 351,                 /* pin_rule  */
  YYSYMBOL_start_pins = 352,               /* start_pins  */
  YYSYMBOL_pins = 353,                     /* pins  */
  YYSYMBOL_pin = 354,                      /* pin  */
  YYSYMBOL_355_21 = 355,                   /* $@21  */
  YYSYMBOL_356_22 = 356,                   /* $@22  */
  YYSYMBOL_357_23 = 357,                   /* $@23  */
  YYSYMBOL_pin_options = 358,              /* pin_options  */
  YYSYMBOL_pin_option = 359,               /* pin_option  */
  YYSYMBOL_360_24 = 360,                   /* $@24  */
  YYSYMBOL_361_25 = 361,                   /* $@25  */
  YYSYMBOL_362_26 = 362,                   /* $@26  */
  YYSYMBOL_363_27 = 363,                   /* $@27  */
  YYSYMBOL_364_28 = 364,                   /* $@28  */
  YYSYMBOL_365_29 = 365,                   /* $@29  */
  YYSYMBOL_366_30 = 366,                   /* $@30  */
  YYSYMBOL_367_31 = 367,                   /* $@31  */
  YYSYMBOL_368_32 = 368,                   /* $@32  */
  YYSYMBOL_369_33 = 369,                   /* $@33  */
  YYSYMBOL_pin_layer_mask_opt = 370,       /* pin_layer_mask_opt  */
  YYSYMBOL_pin_via_mask_opt = 371,         /* pin_via_mask_opt  */
  YYSYMBOL_pin_poly_mask_opt = 372,        /* pin_poly_mask_opt  */
  YYSYMBOL_pin_layer_spacing_opt = 373,    /* pin_layer_spacing_opt  */
  YYSYMBOL_pin_poly_spacing_opt = 374,     /* pin_poly_spacing_opt  */
  YYSYMBOL_pin_oxide = 375,                /* pin_oxide  */
  YYSYMBOL_use_type = 376,                 /* use_type  */
  YYSYMBOL_pin_layer_opt = 377,            /* pin_layer_opt  */
  YYSYMBOL_378_34 = 378,                   /* $@34  */
  YYSYMBOL_end_pins = 379,                 /* end_pins  */
  YYSYMBOL_row_rule = 380,                 /* row_rule  */
  YYSYMBOL_381_35 = 381,                   /* $@35  */
  YYSYMBOL_382_36 = 382,                   /* $@36  */
  YYSYMBOL_row_do_option = 383,            /* row_do_option  */
  YYSYMBOL_row_step_option = 384,          /* row_step_option  */
  YYSYMBOL_row_options = 385,              /* row_options  */
  YYSYMBOL_row_option = 386,               /* row_option  */
  YYSYMBOL_387_37 = 387,                   /* $@37  */
  YYSYMBOL_row_prop_list = 388,            /* row_prop_list  */
  YYSYMBOL_row_prop = 389,                 /* row_prop  */
  YYSYMBOL_tracks_rule = 390,              /* tracks_rule  */
  YYSYMBOL_391_38 = 391,                   /* $@38  */
  YYSYMBOL_track_start = 392,              /* track_start  */
  YYSYMBOL_track_type = 393,               /* track_type  */
  YYSYMBOL_track_opts = 394,               /* track_opts  */
  YYSYMBOL_track_mask_statement = 395,     /* track_mask_statement  */
  YYSYMBOL_same_mask = 396,                /* same_mask  */
  YYSYMBOL_track_layer_statement = 397,    /* track_layer_statement  */
  YYSYMBOL_398_39 = 398,                   /* $@39  */
  YYSYMBOL_track_layers = 399,             /* track_layers  */
  YYSYMBOL_track_layer = 400,              /* track_layer  */
  YYSYMBOL_gcellgrid = 401,                /* gcellgrid  */
  YYSYMBOL_extension_section = 402,        /* extension_section  */
  YYSYMBOL_extension_stmt = 403,           /* extension_stmt  */
  YYSYMBOL_via_section = 404,              /* via_section  */
  YYSYMBOL_via = 405,                      /* via  */
  YYSYMBOL_via_declarations = 406,         /* via_declarations  */
  YYSYMBOL_via_declaration = 407,          /* via_declaration  */
  YYSYMBOL_408_40 = 408,                   /* $@40  */
  YYSYMBOL_409_41 = 409,                   /* $@41  */
  YYSYMBOL_layer_stmts = 410,              /* layer_stmts  */
  YYSYMBOL_layer_stmt = 411,               /* layer_stmt  */
  YYSYMBOL_412_42 = 412,                   /* $@42  */
  YYSYMBOL_413_43 = 413,                   /* $@43  */
  YYSYMBOL_414_44 = 414,                   /* $@44  */
  YYSYMBOL_415_45 = 415,                   /* $@45  */
  YYSYMBOL_416_46 = 416,                   /* $@46  */
  YYSYMBOL_417_47 = 417,                   /* $@47  */
  YYSYMBOL_layer_viarule_opts = 418,       /* layer_viarule_opts  */
  YYSYMBOL_419_48 = 419,                   /* $@48  */
  YYSYMBOL_firstPt = 420,                  /* firstPt  */
  YYSYMBOL_nextPt = 421,                   /* nextPt  */
  YYSYMBOL_otherPts = 422,                 /* otherPts  */
  YYSYMBOL_pt = 423,                       /* pt  */
  YYSYMBOL_mask = 424,                     /* mask  */
  YYSYMBOL_via_end = 425,                  /* via_end  */
  YYSYMBOL_regions_section = 426,          /* regions_section  */
  YYSYMBOL_regions_start = 427,            /* regions_start  */
  YYSYMBOL_regions_stmts = 428,            /* regions_stmts  */
  YYSYMBOL_regions_stmt = 429,             /* regions_stmt  */
  YYSYMBOL_430_49 = 430,                   /* $@49  */
  YYSYMBOL_431_50 = 431,                   /* $@50  */
  YYSYMBOL_rect_list = 432,                /* rect_list  */
  YYSYMBOL_region_options = 433,           /* region_options  */
  YYSYMBOL_region_option = 434,            /* region_option  */
  YYSYMBOL_435_51 = 435,                   /* $@51  */
  YYSYMBOL_region_prop_list = 436,         /* region_prop_list  */
  YYSYMBOL_region_prop = 437,              /* region_prop  */
  YYSYMBOL_region_type = 438,              /* region_type  */
  YYSYMBOL_comps_maskShift_section = 439,  /* comps_maskShift_section  */
  YYSYMBOL_comps_section = 440,            /* comps_section  */
  YYSYMBOL_start_comps = 441,              /* start_comps  */
  YYSYMBOL_layer_statement = 442,          /* layer_statement  */
  YYSYMBOL_maskLayer = 443,                /* maskLayer  */
  YYSYMBOL_comps_rule = 444,               /* comps_rule  */
  YYSYMBOL_comp = 445,                     /* comp  */
  YYSYMBOL_comp_start = 446,               /* comp_start  */
  YYSYMBOL_comp_id_and_name = 447,         /* comp_id_and_name  */
  YYSYMBOL_448_52 = 448,                   /* $@52  */
  YYSYMBOL_comp_net_list = 449,            /* comp_net_list  */
  YYSYMBOL_comp_options = 450,             /* comp_options  */
  YYSYMBOL_comp_option = 451,              /* comp_option  */
  YYSYMBOL_comp_extension_stmt = 452,      /* comp_extension_stmt  */
  YYSYMBOL_comp_eeq = 453,                 /* comp_eeq  */
  YYSYMBOL_454_53 = 454,                   /* $@53  */
  YYSYMBOL_comp_generate = 455,            /* comp_generate  */
  YYSYMBOL_456_54 = 456,                   /* $@54  */
  YYSYMBOL_opt_pattern = 457,              /* opt_pattern  */
  YYSYMBOL_comp_source = 458,              /* comp_source  */
  YYSYMBOL_source_type = 459,              /* source_type  */
  YYSYMBOL_comp_region = 460,              /* comp_region  */
  YYSYMBOL_comp_pnt_list = 461,            /* comp_pnt_list  */
  YYSYMBOL_comp_halo = 462,                /* comp_halo  */
  YYSYMBOL_463_55 = 463,                   /* $@55  */
  YYSYMBOL_halo_soft = 464,                /* halo_soft  */
  YYSYMBOL_comp_routehalo = 465,           /* comp_routehalo  */
  YYSYMBOL_466_56 = 466,                   /* $@56  */
  YYSYMBOL_comp_property = 467,            /* comp_property  */
  YYSYMBOL_468_57 = 468,                   /* $@57  */
  YYSYMBOL_comp_prop_list = 469,           /* comp_prop_list  */
  YYSYMBOL_comp_prop = 470,                /* comp_prop  */
  YYSYMBOL_comp_region_start = 471,        /* comp_region_start  */
  YYSYMBOL_comp_foreign = 472,             /* comp_foreign  */
  YYSYMBOL_473_58 = 473,                   /* $@58  */
  YYSYMBOL_opt_paren = 474,                /* opt_paren  */
  YYSYMBOL_comp_type = 475,                /* comp_type  */
  YYSYMBOL_maskShift = 476,                /* maskShift  */
  YYSYMBOL_477_59 = 477,                   /* $@59  */
  YYSYMBOL_placement_status = 478,         /* placement_status  */
  YYSYMBOL_weight = 479,                   /* weight  */
  YYSYMBOL_end_comps = 480,                /* end_comps  */
  YYSYMBOL_nets_section = 481,             /* nets_section  */
  YYSYMBOL_start_nets = 482,               /* start_nets  */
  YYSYMBOL_net_rules = 483,                /* net_rules  */
  YYSYMBOL_one_net = 484,                  /* one_net  */
  YYSYMBOL_net_and_connections = 485,      /* net_and_connections  */
  YYSYMBOL_net_start = 486,                /* net_start  */
  YYSYMBOL_487_60 = 487,                   /* $@60  */
  YYSYMBOL_net_name = 488,                 /* net_name  */
  YYSYMBOL_489_61 = 489,                   /* $@61  */
  YYSYMBOL_490_62 = 490,                   /* $@62  */
  YYSYMBOL_net_connections = 491,          /* net_connections  */
  YYSYMBOL_net_connection = 492,           /* net_connection  */
  YYSYMBOL_493_63 = 493,                   /* $@63  */
  YYSYMBOL_494_64 = 494,                   /* $@64  */
  YYSYMBOL_495_65 = 495,                   /* $@65  */
  YYSYMBOL_conn_opt = 496,                 /* conn_opt  */
  YYSYMBOL_net_options = 497,              /* net_options  */
  YYSYMBOL_net_option = 498,               /* net_option  */
  YYSYMBOL_499_66 = 499,                   /* $@66  */
  YYSYMBOL_500_67 = 500,                   /* $@67  */
  YYSYMBOL_501_68 = 501,                   /* $@68  */
  YYSYMBOL_502_69 = 502,                   /* $@69  */
  YYSYMBOL_503_70 = 503,                   /* $@70  */
  YYSYMBOL_504_71 = 504,                   /* $@71  */
  YYSYMBOL_505_72 = 505,                   /* $@72  */
  YYSYMBOL_506_73 = 506,                   /* $@73  */
  YYSYMBOL_507_74 = 507,                   /* $@74  */
  YYSYMBOL_508_75 = 508,                   /* $@75  */
  YYSYMBOL_509_76 = 509,                   /* $@76  */
  YYSYMBOL_net_prop_list = 510,            /* net_prop_list  */
  YYSYMBOL_net_prop = 511,                 /* net_prop  */
  YYSYMBOL_netsource_type = 512,           /* netsource_type  */
  YYSYMBOL_vpin_stmt = 513,                /* vpin_stmt  */
  YYSYMBOL_514_77 = 514,                   /* $@77  */
  YYSYMBOL_vpin_begin = 515,               /* vpin_begin  */
  YYSYMBOL_516_78 = 516,                   /* $@78  */
  YYSYMBOL_vpin_layer_opt = 517,           /* vpin_layer_opt  */
  YYSYMBOL_518_79 = 518,                   /* $@79  */
  YYSYMBOL_vpin_options = 519,             /* vpin_options  */
  YYSYMBOL_vpin_status = 520,              /* vpin_status  */
  YYSYMBOL_net_type = 521,                 /* net_type  */
  YYSYMBOL_paths = 522,                    /* paths  */
  YYSYMBOL_new_path = 523,                 /* new_path  */
  YYSYMBOL_524_80 = 524,                   /* $@80  */
  YYSYMBOL_path = 525,                     /* path  */
  YYSYMBOL_526_81 = 526,                   /* $@81  */
  YYSYMBOL_527_82 = 527,                   /* $@82  */
  YYSYMBOL_virtual_statement = 528,        /* virtual_statement  */
  YYSYMBOL_rect_statement = 529,           /* rect_statement  */
  YYSYMBOL_path_item_list = 530,           /* path_item_list  */
  YYSYMBOL_path_item = 531,                /* path_item  */
  YYSYMBOL_532_83 = 532,                   /* $@83  */
  YYSYMBOL_533_84 = 533,                   /* $@84  */
  YYSYMBOL_path_pt = 534,                  /* path_pt  */
  YYSYMBOL_virtual_pt = 535,               /* virtual_pt  */
  YYSYMBOL_rect_pts = 536,                 /* rect_pts  */
  YYSYMBOL_opt_taper_style_s = 537,        /* opt_taper_style_s  */
  YYSYMBOL_opt_taper_style = 538,          /* opt_taper_style  */
  YYSYMBOL_opt_taper = 539,                /* opt_taper  */
  YYSYMBOL_540_85 = 540,                   /* $@85  */
  YYSYMBOL_opt_style = 541,                /* opt_style  */
  YYSYMBOL_opt_spaths = 542,               /* opt_spaths  */
  YYSYMBOL_opt_shape_style = 543,          /* opt_shape_style  */
  YYSYMBOL_end_nets = 544,                 /* end_nets  */
  YYSYMBOL_shape_type = 545,               /* shape_type  */
  YYSYMBOL_snets_section = 546,            /* snets_section  */
  YYSYMBOL_snet_rules = 547,               /* snet_rules  */
  YYSYMBOL_snet_rule = 548,                /* snet_rule  */
  YYSYMBOL_snet_options = 549,             /* snet_options  */
  YYSYMBOL_snet_option = 550,              /* snet_option  */
  YYSYMBOL_snet_other_option = 551,        /* snet_other_option  */
  YYSYMBOL_552_86 = 552,                   /* $@86  */
  YYSYMBOL_553_87 = 553,                   /* $@87  */
  YYSYMBOL_554_88 = 554,                   /* $@88  */
  YYSYMBOL_555_89 = 555,                   /* $@89  */
  YYSYMBOL_556_90 = 556,                   /* $@90  */
  YYSYMBOL_557_91 = 557,                   /* $@91  */
  YYSYMBOL_558_92 = 558,                   /* $@92  */
  YYSYMBOL_559_93 = 559,                   /* $@93  */
  YYSYMBOL_560_94 = 560,                   /* $@94  */
  YYSYMBOL_561_95 = 561,                   /* $@95  */
  YYSYMBOL_orient_pt = 562,                /* orient_pt  */
  YYSYMBOL_shield_layer = 563,             /* shield_layer  */
  YYSYMBOL_564_96 = 564,                   /* $@96  */
  YYSYMBOL_snet_width = 565,               /* snet_width  */
  YYSYMBOL_566_97 = 566,                   /* $@97  */
  YYSYMBOL_snet_voltage = 567,             /* snet_voltage  */
  YYSYMBOL_568_98 = 568,                   /* $@98  */
  YYSYMBOL_snet_spacing = 569,             /* snet_spacing  */
  YYSYMBOL_570_99 = 570,                   /* $@99  */
  YYSYMBOL_571_100 = 571,                  /* $@100  */
  YYSYMBOL_snet_prop_list = 572,           /* snet_prop_list  */
  YYSYMBOL_snet_prop = 573,                /* snet_prop  */
  YYSYMBOL_opt_snet_range = 574,           /* opt_snet_range  */
  YYSYMBOL_opt_range = 575,                /* opt_range  */
  YYSYMBOL_pattern_type = 576,             /* pattern_type  */
  YYSYMBOL_spaths = 577,                   /* spaths  */
  YYSYMBOL_snew_path = 578,                /* snew_path  */
  YYSYMBOL_579_101 = 579,                  /* $@101  */
  YYSYMBOL_spath = 580,                    /* spath  */
  YYSYMBOL_581_102 = 581,                  /* $@102  */
  YYSYMBOL_582_103 = 582,                  /* $@103  */
  YYSYMBOL_width = 583,                    /* width  */
  YYSYMBOL_start_snets = 584,              /* start_snets  */
  YYSYMBOL_end_snets = 585,                /* end_snets  */
  YYSYMBOL_groups_section = 586,           /* groups_section  */
  YYSYMBOL_groups_start = 587,             /* groups_start  */
  YYSYMBOL_group_rules = 588,              /* group_rules  */
  YYSYMBOL_group_rule = 589,               /* group_rule  */
  YYSYMBOL_start_group = 590,              /* start_group  */
  YYSYMBOL_591_104 = 591,                  /* $@104  */
  YYSYMBOL_group_members = 592,            /* group_members  */
  YYSYMBOL_group_member = 593,             /* group_member  */
  YYSYMBOL_group_options = 594,            /* group_options  */
  YYSYMBOL_group_option = 595,             /* group_option  */
  YYSYMBOL_596_105 = 596,                  /* $@105  */
  YYSYMBOL_597_106 = 597,                  /* $@106  */
  YYSYMBOL_group_region = 598,             /* group_region  */
  YYSYMBOL_group_prop_list = 599,          /* group_prop_list  */
  YYSYMBOL_group_prop = 600,               /* group_prop  */
  YYSYMBOL_group_soft_options = 601,       /* group_soft_options  */
  YYSYMBOL_group_soft_option = 602,        /* group_soft_option  */
  YYSYMBOL_groups_end = 603,               /* groups_end  */
  YYSYMBOL_assertions_section = 604,       /* assertions_section  */
  YYSYMBOL_constraint_section = 605,       /* constraint_section  */
  YYSYMBOL_assertions_start = 606,         /* assertions_start  */
  YYSYMBOL_constraints_start = 607,        /* constraints_start  */
  YYSYMBOL_constraint_rules = 608,         /* constraint_rules  */
  YYSYMBOL_constraint_rule = 609,          /* constraint_rule  */
  YYSYMBOL_operand_rule = 610,             /* operand_rule  */
  YYSYMBOL_operand = 611,                  /* operand  */
  YYSYMBOL_612_107 = 612,                  /* $@107  */
  YYSYMBOL_613_108 = 613,                  /* $@108  */
  YYSYMBOL_operand_list = 614,             /* operand_list  */
  YYSYMBOL_wiredlogic_rule = 615,          /* wiredlogic_rule  */
  YYSYMBOL_616_109 = 616,                  /* $@109  */
  YYSYMBOL_opt_plus = 617,                 /* opt_plus  */
  YYSYMBOL_delay_specs = 618,              /* delay_specs  */
  YYSYMBOL_delay_spec = 619,               /* delay_spec  */
  YYSYMBOL_constraints_end = 620,          /* constraints_end  */
  YYSYMBOL_assertions_end = 621,           /* assertions_end  */
  YYSYMBOL_scanchains_section = 622,       /* scanchains_section  */
  YYSYMBOL_scanchain_start = 623,          /* scanchain_start  */
  YYSYMBOL_scanchain_rules = 624,          /* scanchain_rules  */
  YYSYMBOL_scan_rule = 625,                /* scan_rule  */
  YYSYMBOL_start_scan = 626,               /* start_scan  */
  YYSYMBOL_627_110 = 627,                  /* $@110  */
  YYSYMBOL_scan_members = 628,             /* scan_members  */
  YYSYMBOL_opt_pin = 629,                  /* opt_pin  */
  YYSYMBOL_scan_member = 630,              /* scan_member  */
  YYSYMBOL_631_111 = 631,                  /* $@111  */
  YYSYMBOL_632_112 = 632,                  /* $@112  */
  YYSYMBOL_633_113 = 633,                  /* $@113  */
  YYSYMBOL_634_114 = 634,                  /* $@114  */
  YYSYMBOL_635_115 = 635,                  /* $@115  */
  YYSYMBOL_636_116 = 636,                  /* $@116  */
  YYSYMBOL_opt_common_pins = 637,          /* opt_common_pins  */
  YYSYMBOL_floating_inst_list = 638,       /* floating_inst_list  */
  YYSYMBOL_one_floating_inst = 639,        /* one_floating_inst  */
  YYSYMBOL_640_117 = 640,                  /* $@117  */
  YYSYMBOL_floating_pins = 641,            /* floating_pins  */
  YYSYMBOL_ordered_inst_list = 642,        /* ordered_inst_list  */
  YYSYMBOL_one_ordered_inst = 643,         /* one_ordered_inst  */
  YYSYMBOL_644_118 = 644,                  /* $@118  */
  YYSYMBOL_ordered_pins = 645,             /* ordered_pins  */
  YYSYMBOL_partition_maxbits = 646,        /* partition_maxbits  */
  YYSYMBOL_scanchain_end = 647,            /* scanchain_end  */
  YYSYMBOL_iotiming_section = 648,         /* iotiming_section  */
  YYSYMBOL_iotiming_start = 649,           /* iotiming_start  */
  YYSYMBOL_iotiming_rules = 650,           /* iotiming_rules  */
  YYSYMBOL_iotiming_rule = 651,            /* iotiming_rule  */
  YYSYMBOL_start_iotiming = 652,           /* start_iotiming  */
  YYSYMBOL_653_119 = 653,                  /* $@119  */
  YYSYMBOL_iotiming_members = 654,         /* iotiming_members  */
  YYSYMBOL_iotiming_member = 655,          /* iotiming_member  */
  YYSYMBOL_656_120 = 656,                  /* $@120  */
  YYSYMBOL_657_121 = 657,                  /* $@121  */
  YYSYMBOL_iotiming_drivecell_opt = 658,   /* iotiming_drivecell_opt  */
  YYSYMBOL_659_122 = 659,                  /* $@122  */
  YYSYMBOL_660_123 = 660,                  /* $@123  */
  YYSYMBOL_iotiming_frompin = 661,         /* iotiming_frompin  */
  YYSYMBOL_662_124 = 662,                  /* $@124  */
  YYSYMBOL_iotiming_parallel = 663,        /* iotiming_parallel  */
  YYSYMBOL_risefall = 664,                 /* risefall  */
  YYSYMBOL_iotiming_end = 665,             /* iotiming_end  */
  YYSYMBOL_floorplan_contraints_section = 666, /* floorplan_contraints_section  */
  YYSYMBOL_fp_start = 667,                 /* fp_start  */
  YYSYMBOL_fp_stmts = 668,                 /* fp_stmts  */
  YYSYMBOL_fp_stmt = 669,                  /* fp_stmt  */
  YYSYMBOL_670_125 = 670,                  /* $@125  */
  YYSYMBOL_671_126 = 671,                  /* $@126  */
  YYSYMBOL_h_or_v = 672,                   /* h_or_v  */
  YYSYMBOL_constraint_type = 673,          /* constraint_type  */
  YYSYMBOL_constrain_what_list = 674,      /* constrain_what_list  */
  YYSYMBOL_constrain_what = 675,           /* constrain_what  */
  YYSYMBOL_676_127 = 676,                  /* $@127  */
  YYSYMBOL_677_128 = 677,                  /* $@128  */
  YYSYMBOL_row_or_comp_list = 678,         /* row_or_comp_list  */
  YYSYMBOL_row_or_comp = 679,              /* row_or_comp  */
  YYSYMBOL_680_129 = 680,                  /* $@129  */
  YYSYMBOL_681_130 = 681,                  /* $@130  */
  YYSYMBOL_timingdisables_section = 682,   /* timingdisables_section  */
  YYSYMBOL_timingdisables_start = 683,     /* timingdisables_start  */
  YYSYMBOL_timingdisables_rules = 684,     /* timingdisables_rules  */
  YYSYMBOL_timingdisables_rule = 685,      /* timingdisables_rule  */
  YYSYMBOL_686_131 = 686,                  /* $@131  */
  YYSYMBOL_687_132 = 687,                  /* $@132  */
  YYSYMBOL_688_133 = 688,                  /* $@133  */
  YYSYMBOL_689_134 = 689,                  /* $@134  */
  YYSYMBOL_td_macro_option = 690,          /* td_macro_option  */
  YYSYMBOL_691_135 = 691,                  /* $@135  */
  YYSYMBOL_692_136 = 692,                  /* $@136  */
  YYSYMBOL_693_137 = 693,                  /* $@137  */
  YYSYMBOL_timingdisables_end = 694,       /* timingdisables_end  */
  YYSYMBOL_partitions_section = 695,       /* partitions_section  */
  YYSYMBOL_partitions_start = 696,         /* partitions_start  */
  YYSYMBOL_partition_rules = 697,          /* partition_rules  */
  YYSYMBOL_partition_rule = 698,           /* partition_rule  */
  YYSYMBOL_start_partition = 699,          /* start_partition  */
  YYSYMBOL_700_138 = 700,                  /* $@138  */
  YYSYMBOL_turnoff = 701,                  /* turnoff  */
  YYSYMBOL_turnoff_setup = 702,            /* turnoff_setup  */
  YYSYMBOL_turnoff_hold = 703,             /* turnoff_hold  */
  YYSYMBOL_partition_members = 704,        /* partition_members  */
  YYSYMBOL_partition_member = 705,         /* partition_member  */
  YYSYMBOL_706_139 = 706,                  /* $@139  */
  YYSYMBOL_707_140 = 707,                  /* $@140  */
  YYSYMBOL_708_141 = 708,                  /* $@141  */
  YYSYMBOL_709_142 = 709,                  /* $@142  */
  YYSYMBOL_710_143 = 710,                  /* $@143  */
  YYSYMBOL_711_144 = 711,                  /* $@144  */
  YYSYMBOL_minmaxpins = 712,               /* minmaxpins  */
  YYSYMBOL_713_145 = 713,                  /* $@145  */
  YYSYMBOL_min_or_max_list = 714,          /* min_or_max_list  */
  YYSYMBOL_min_or_max_member = 715,        /* min_or_max_member  */
  YYSYMBOL_pin_list = 716,                 /* pin_list  */
  YYSYMBOL_risefallminmax1_list = 717,     /* risefallminmax1_list  */
  YYSYMBOL_risefallminmax1 = 718,          /* risefallminmax1  */
  YYSYMBOL_risefallminmax2_list = 719,     /* risefallminmax2_list  */
  YYSYMBOL_risefallminmax2 = 720,          /* risefallminmax2  */
  YYSYMBOL_partitions_end = 721,           /* partitions_end  */
  YYSYMBOL_comp_names = 722,               /* comp_names  */
  YYSYMBOL_comp_name = 723,                /* comp_name  */
  YYSYMBOL_724_146 = 724,                  /* $@146  */
  YYSYMBOL_subnet_opt_syn = 725,           /* subnet_opt_syn  */
  YYSYMBOL_subnet_options = 726,           /* subnet_options  */
  YYSYMBOL_subnet_option = 727,            /* subnet_option  */
  YYSYMBOL_728_147 = 728,                  /* $@147  */
  YYSYMBOL_729_148 = 729,                  /* $@148  */
  YYSYMBOL_subnet_type = 730,              /* subnet_type  */
  YYSYMBOL_pin_props_section = 731,        /* pin_props_section  */
  YYSYMBOL_begin_pin_props = 732,          /* begin_pin_props  */
  YYSYMBOL_opt_semi = 733,                 /* opt_semi  */
  YYSYMBOL_end_pin_props = 734,            /* end_pin_props  */
  YYSYMBOL_pin_prop_list = 735,            /* pin_prop_list  */
  YYSYMBOL_pin_prop_terminal = 736,        /* pin_prop_terminal  */
  YYSYMBOL_737_149 = 737,                  /* $@149  */
  YYSYMBOL_738_150 = 738,                  /* $@150  */
  YYSYMBOL_pin_prop_options = 739,         /* pin_prop_options  */
  YYSYMBOL_pin_prop = 740,                 /* pin_prop  */
  YYSYMBOL_741_151 = 741,                  /* $@151  */
  YYSYMBOL_pin_prop_name_value_list = 742, /* pin_prop_name_value_list  */
  YYSYMBOL_pin_prop_name_value = 743,      /* pin_prop_name_value  */
  YYSYMBOL_blockage_section = 744,         /* blockage_section  */
  YYSYMBOL_blockage_start = 745,           /* blockage_start  */
  YYSYMBOL_blockage_end = 746,             /* blockage_end  */
  YYSYMBOL_blockage_defs = 747,            /* blockage_defs  */
  YYSYMBOL_blockage_def = 748,             /* blockage_def  */
  YYSYMBOL_blockage_rule = 749,            /* blockage_rule  */
  YYSYMBOL_750_152 = 750,                  /* $@152  */
  YYSYMBOL_751_153 = 751,                  /* $@153  */
  YYSYMBOL_752_154 = 752,                  /* $@154  */
  YYSYMBOL_layer_blockage_rules = 753,     /* layer_blockage_rules  */
  YYSYMBOL_layer_blockage_rule = 754,      /* layer_blockage_rule  */
  YYSYMBOL_mask_blockage_rule = 755,       /* mask_blockage_rule  */
  YYSYMBOL_comp_blockage_rule = 756,       /* comp_blockage_rule  */
  YYSYMBOL_757_155 = 757,                  /* $@155  */
  YYSYMBOL_placement_comp_rules = 758,     /* placement_comp_rules  */
  YYSYMBOL_placement_comp_rule = 759,      /* placement_comp_rule  */
  YYSYMBOL_760_156 = 760,                  /* $@156  */
  YYSYMBOL_rectPoly_blockage_rules = 761,  /* rectPoly_blockage_rules  */
  YYSYMBOL_rectPoly_blockage = 762,        /* rectPoly_blockage  */
  YYSYMBOL_763_157 = 763,                  /* $@157  */
  YYSYMBOL_slot_section = 764,             /* slot_section  */
  YYSYMBOL_slot_start = 765,               /* slot_start  */
  YYSYMBOL_slot_end = 766,                 /* slot_end  */
  YYSYMBOL_slot_defs = 767,                /* slot_defs  */
  YYSYMBOL_slot_def = 768,                 /* slot_def  */
  YYSYMBOL_slot_rule = 769,                /* slot_rule  */
  YYSYMBOL_770_158 = 770,                  /* $@158  */
  YYSYMBOL_771_159 = 771,                  /* $@159  */
  YYSYMBOL_geom_slot_rules = 772,          /* geom_slot_rules  */
  YYSYMBOL_geom_slot = 773,                /* geom_slot  */
  YYSYMBOL_774_160 = 774,                  /* $@160  */
  YYSYMBOL_fill_section = 775,             /* fill_section  */
  YYSYMBOL_fill_start = 776,               /* fill_start  */
  YYSYMBOL_fill_end = 777,                 /* fill_end  */
  YYSYMBOL_fill_defs = 778,                /* fill_defs  */
  YYSYMBOL_fill_def = 779,                 /* fill_def  */
  YYSYMBOL_780_161 = 780,                  /* $@161  */
  YYSYMBOL_781_162 = 781,                  /* $@162  */
  YYSYMBOL_fill_rule = 782,                /* fill_rule  */
  YYSYMBOL_783_163 = 783,                  /* $@163  */
  YYSYMBOL_784_164 = 784,                  /* $@164  */
  YYSYMBOL_geom_fill_rules = 785,          /* geom_fill_rules  */
  YYSYMBOL_geom_fill = 786,                /* geom_fill  */
  YYSYMBOL_787_165 = 787,                  /* $@165  */
  YYSYMBOL_fill_layer_mask_opc_opt = 788,  /* fill_layer_mask_opc_opt  */
  YYSYMBOL_opt_mask_opc_l = 789,           /* opt_mask_opc_l  */
  YYSYMBOL_fill_layer_opc = 790,           /* fill_layer_opc  */
  YYSYMBOL_fill_via_pt = 791,              /* fill_via_pt  */
  YYSYMBOL_fill_via_mask_opc_opt = 792,    /* fill_via_mask_opc_opt  */
  YYSYMBOL_opt_mask_opc = 793,             /* opt_mask_opc  */
  YYSYMBOL_fill_via_opc = 794,             /* fill_via_opc  */
  YYSYMBOL_fill_mask = 795,                /* fill_mask  */
  YYSYMBOL_fill_viaMask = 796,             /* fill_viaMask  */
  YYSYMBOL_nondefaultrule_section = 797,   /* nondefaultrule_section  */
  YYSYMBOL_nondefault_start = 798,         /* nondefault_start  */
  YYSYMBOL_nondefault_end = 799,           /* nondefault_end  */
  YYSYMBOL_nondefault_defs = 800,          /* nondefault_defs  */
  YYSYMBOL_nondefault_def = 801,           /* nondefault_def  */
  YYSYMBOL_802_166 = 802,                  /* $@166  */
  YYSYMBOL_803_167 = 803,                  /* $@167  */
  YYSYMBOL_nondefault_options = 804,       /* nondefault_options  */
  YYSYMBOL_nondefault_option = 805,        /* nondefault_option  */
  YYSYMBOL_806_168 = 806,                  /* $@168  */
  YYSYMBOL_807_169 = 807,                  /* $@169  */
  YYSYMBOL_808_170 = 808,                  /* $@170  */
  YYSYMBOL_809_171 = 809,                  /* $@171  */
  YYSYMBOL_810_172 = 810,                  /* $@172  */
  YYSYMBOL_nondefault_layer_options = 811, /* nondefault_layer_options  */
  YYSYMBOL_nondefault_layer_option = 812,  /* nondefault_layer_option  */
  YYSYMBOL_nondefault_prop_opt = 813,      /* nondefault_prop_opt  */
  YYSYMBOL_814_173 = 814,                  /* $@173  */
  YYSYMBOL_nondefault_prop_list = 815,     /* nondefault_prop_list  */
  YYSYMBOL_nondefault_prop = 816,          /* nondefault_prop  */
  YYSYMBOL_styles_section = 817,           /* styles_section  */
  YYSYMBOL_styles_start = 818,             /* styles_start  */
  YYSYMBOL_styles_end = 819,               /* styles_end  */
  YYSYMBOL_styles_rules = 820,             /* styles_rules  */
  YYSYMBOL_styles_rule = 821,              /* styles_rule  */
  YYSYMBOL_822_174 = 822                   /* $@174  */
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
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   1514

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  301
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  522
/* YYNRULES -- Number of rules.  */
#define YYNRULES  985
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1696

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   548


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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     297,   298,   299,   296,   300,   295,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   294,
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
     285,   286,   287,   288,   289,   290,   291,   292,   293
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   226,   226,   229,   230,   230,   256,   257,   270,   288,
     289,   290,   293,   293,   293,   293,   294,   294,   294,   294,
     295,   295,   295,   296,   296,   296,   297,   297,   297,   298,
     298,   298,   298,   299,   302,   302,   302,   302,   303,   303,
     303,   304,   304,   304,   305,   305,   305,   306,   306,   306,
     306,   311,   311,   318,   343,   343,   349,   349,   355,   355,
     361,   368,   367,   379,   380,   383,   383,   392,   392,   401,
     401,   410,   410,   419,   419,   428,   428,   437,   437,   448,
     447,   458,   457,   478,   480,   480,   485,   485,   491,   496,
     501,   507,   508,   511,   519,   526,   533,   533,   545,   545,
     558,   559,   560,   561,   562,   563,   564,   565,   568,   567,
     580,   583,   595,   596,   599,   610,   613,   616,   622,   623,
     626,   627,   628,   626,   641,   642,   644,   650,   656,   662,
     683,   683,   703,   703,   723,   727,   751,   752,   751,   777,
     778,   777,   820,   820,   853,   873,   891,   909,   927,   945,
     945,   963,   963,   982,  1000,  1000,  1018,  1035,  1036,  1050,
    1051,  1058,  1059,  1072,  1073,  1096,  1120,  1121,  1144,  1168,
    1173,  1178,  1183,  1188,  1193,  1198,  1203,  1208,  1213,  1218,
    1223,  1228,  1233,  1238,  1243,  1248,  1253,  1258,  1263,  1268,
    1273,  1278,  1283,  1288,  1293,  1298,  1303,  1308,  1313,  1318,
    1323,  1330,  1332,  1334,  1336,  1338,  1340,  1342,  1344,  1349,
    1350,  1350,  1353,  1359,  1361,  1359,  1375,  1385,  1423,  1426,
    1434,  1435,  1438,  1438,  1442,  1443,  1446,  1458,  1467,  1478,
    1477,  1511,  1516,  1518,  1521,  1523,  1524,  1535,  1536,  1539,
    1540,  1540,  1543,  1544,  1547,  1553,  1582,  1588,  1591,  1594,
    1600,  1601,  1604,  1605,  1604,  1616,  1617,  1620,  1620,  1627,
    1628,  1627,  1654,  1654,  1664,  1666,  1664,  1689,  1690,  1696,
    1708,  1720,  1732,  1732,  1745,  1748,  1751,  1752,  1755,  1762,
    1768,  1774,  1781,  1782,  1785,  1791,  1797,  1803,  1804,  1807,
    1808,  1807,  1817,  1820,  1825,  1826,  1829,  1829,  1832,  1847,
    1848,  1851,  1866,  1875,  1885,  1887,  1890,  1907,  1910,  1917,
    1918,  1921,  1928,  1929,  1932,  1938,  1944,  1944,  1952,  1953,
    1958,  1964,  1965,  1968,  1968,  1968,  1968,  1968,  1969,  1969,
    1969,  1969,  1970,  1970,  1970,  1973,  1980,  1980,  1986,  1986,
    1994,  1995,  1998,  2004,  2006,  2008,  2010,  2015,  2017,  2023,
    2034,  2047,  2046,  2068,  2069,  2089,  2089,  2109,  2109,  2113,
    2114,  2117,  2128,  2137,  2147,  2150,  2150,  2165,  2167,  2170,
    2177,  2185,  2200,  2200,  2209,  2211,  2213,  2216,  2222,  2228,
    2231,  2238,  2239,  2242,  2253,  2257,  2257,  2260,  2259,  2268,
    2268,  2276,  2277,  2280,  2280,  2294,  2294,  2301,  2301,  2310,
    2311,  2318,  2323,  2324,  2328,  2327,  2346,  2349,  2366,  2366,
    2384,  2384,  2387,  2390,  2393,  2396,  2399,  2402,  2405,  2405,
    2416,  2418,  2418,  2421,  2422,  2421,  2468,  2473,  2483,  2467,
    2497,  2497,  2501,  2507,  2508,  2511,  2522,  2531,  2541,  2543,
    2545,  2547,  2549,  2553,  2552,  2563,  2563,  2566,  2567,  2567,
    2570,  2571,  2574,  2576,  2578,  2581,  2583,  2585,  2589,  2593,
    2596,  2596,  2602,  2621,  2601,  2635,  2652,  2669,  2670,  2675,
    2686,  2700,  2707,  2718,  2742,  2776,  2810,  2835,  2836,  2837,
    2837,  2848,  2847,  2857,  2868,  2876,  2883,  2890,  2896,  2904,
    2912,  2920,  2929,  2937,  2944,  2951,  2959,  2968,  2969,  2971,
    2972,  2975,  2979,  2979,  2984,  3004,  3005,  3009,  3013,  3033,
    3039,  3041,  3043,  3045,  3047,  3049,  3051,  3053,  3069,  3071,
    3073,  3075,  3078,  3081,  3082,  3085,  3088,  3089,  3092,  3092,
    3093,  3093,  3096,  3110,  3109,  3131,  3132,  3131,  3138,  3142,
    3148,  3149,  3148,  3189,  3189,  3222,  3223,  3222,  3255,  3258,
    3261,  3264,  3264,  3267,  3270,  3273,  3283,  3286,  3289,  3289,
    3293,  3297,  3298,  3299,  3300,  3301,  3302,  3303,  3304,  3305,
    3308,  3322,  3322,  3377,  3377,  3387,  3387,  3405,  3406,  3405,
    3413,  3414,  3417,  3429,  3438,  3448,  3449,  3454,  3455,  3458,
    3460,  3462,  3464,  3468,  3485,  3488,  3488,  3507,  3515,  3506,
    3523,  3528,  3535,  3542,  3545,  3551,  3552,  3555,  3561,  3561,
    3572,  3573,  3576,  3583,  3584,  3587,  3589,  3589,  3592,  3592,
    3594,  3600,  3610,  3615,  3616,  3619,  3630,  3639,  3649,  3650,
    3653,  3662,  3671,  3681,  3688,  3692,  3695,  3709,  3723,  3724,
    3727,  3728,  3738,  3751,  3751,  3756,  3756,  3761,  3766,  3772,
    3773,  3775,  3777,  3777,  3786,  3787,  3790,  3791,  3794,  3799,
    3804,  3809,  3815,  3826,  3837,  3840,  3846,  3847,  3850,  3856,
    3856,  3863,  3864,  3869,  3870,  3873,  3873,  3877,  3877,  3880,
    3879,  3888,  3888,  3892,  3892,  3894,  3894,  3912,  3919,  3920,
    3929,  3943,  3944,  3948,  3947,  3958,  3959,  3972,  3993,  4024,
    4025,  4029,  4028,  4037,  4038,  4051,  4072,  4104,  4105,  4108,
    4117,  4120,  4131,  4132,  4135,  4141,  4141,  4147,  4148,  4152,
    4157,  4162,  4167,  4168,  4167,  4176,  4183,  4184,  4182,  4190,
    4191,  4191,  4197,  4198,  4204,  4204,  4206,  4212,  4218,  4224,
    4225,  4228,  4229,  4228,  4233,  4235,  4238,  4240,  4242,  4244,
    4247,  4248,  4252,  4251,  4255,  4254,  4259,  4260,  4262,  4262,
    4264,  4264,  4267,  4271,  4278,  4279,  4282,  4283,  4282,  4291,
    4291,  4299,  4299,  4307,  4313,  4314,  4313,  4319,  4319,  4325,
    4332,  4335,  4342,  4343,  4346,  4352,  4352,  4358,  4359,  4366,
    4367,  4369,  4373,  4374,  4376,  4379,  4380,  4383,  4383,  4389,
    4389,  4395,  4395,  4401,  4401,  4407,  4407,  4413,  4413,  4418,
    4426,  4425,  4429,  4430,  4433,  4438,  4444,  4445,  4448,  4449,
    4451,  4453,  4455,  4457,  4461,  4462,  4465,  4468,  4471,  4474,
    4478,  4482,  4483,  4486,  4486,  4495,  4496,  4499,  4500,  4503,
    4502,  4515,  4515,  4518,  4520,  4522,  4524,  4527,  4529,  4535,
    4536,  4539,  4543,  4544,  4547,  4548,  4547,  4557,  4558,  4560,
    4560,  4564,  4565,  4568,  4579,  4588,  4598,  4600,  4604,  4608,
    4609,  4612,  4621,  4622,  4621,  4641,  4640,  4657,  4658,  4661,
    4687,  4709,  4710,  4713,  4722,  4722,  4741,  4762,  4783,  4801,
    4833,  4834,  4839,  4839,  4857,  4875,  4909,  4945,  4946,  4949,
    4955,  4954,  4980,  4982,  4986,  4990,  4991,  4994,  5002,  5003,
    5002,  5010,  5011,  5014,  5020,  5019,  5032,  5034,  5038,  5042,
    5043,  5046,  5053,  5054,  5053,  5063,  5064,  5063,  5072,  5073,
    5076,  5082,  5081,  5100,  5101,  5103,  5104,  5109,  5128,  5138,
    5139,  5141,  5142,  5147,  5167,  5177,  5187,  5190,  5208,  5212,
    5213,  5216,  5217,  5216,  5227,  5228,  5231,  5236,  5238,  5236,
    5245,  5245,  5251,  5251,  5257,  5257,  5263,  5266,  5267,  5270,
    5276,  5282,  5290,  5290,  5294,  5295,  5298,  5309,  5318,  5329,
    5331,  5348,  5352,  5353,  5357,  5356
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
  "\"end of file\"", "error", "\"invalid token\"", "QSTRING", "T_STRING",
  "SITE_PATTERN", "NUMBER", "K_HISTORY", "K_NAMESCASESENSITIVE",
  "K_DESIGN", "K_VIAS", "K_TECH", "K_UNITS", "K_ARRAY", "K_FLOORPLAN",
  "K_SITE", "K_CANPLACE", "K_CANNOTOCCUPY", "K_DIEAREA", "K_PINS",
  "K_DEFAULTCAP", "K_MINPINS", "K_WIRECAP", "K_TRACKS", "K_GCELLGRID",
  "K_DO", "K_BY", "K_STEP", "K_LAYER", "K_ROW", "K_RECT", "K_COMPS",
  "K_COMP_GEN", "K_SOURCE", "K_WEIGHT", "K_EEQMASTER", "K_FIXED",
  "K_COVER", "K_UNPLACED", "K_PLACED", "K_FOREIGN", "K_REGION",
  "K_REGIONS", "K_NETS", "K_START_NET", "K_MUSTJOIN", "K_ORIGINAL",
  "K_USE", "K_STYLE", "K_PATTERN", "K_PATTERNNAME", "K_ESTCAP", "K_ROUTED",
  "K_NEW", "K_SNETS", "K_SHAPE", "K_WIDTH", "K_VOLTAGE", "K_SPACING",
  "K_NONDEFAULTRULE", "K_NONDEFAULTRULES", "K_N", "K_S", "K_E", "K_W",
  "K_FN", "K_FE", "K_FS", "K_FW", "K_GROUPS", "K_GROUP", "K_SOFT",
  "K_MAXX", "K_MAXY", "K_MAXHALFPERIMETER", "K_CONSTRAINTS", "K_NET",
  "K_PATH", "K_SUM", "K_DIFF", "K_SCANCHAINS", "K_START", "K_FLOATING",
  "K_ORDERED", "K_STOP", "K_IN", "K_OUT", "K_RISEMIN", "K_RISEMAX",
  "K_FALLMIN", "K_FALLMAX", "K_WIREDLOGIC", "K_MAXDIST", "K_ASSERTIONS",
  "K_DISTANCE", "K_MICRONS", "K_END", "K_IOTIMINGS", "K_RISE", "K_FALL",
  "K_VARIABLE", "K_SLEWRATE", "K_CAPACITANCE", "K_DRIVECELL", "K_FROMPIN",
  "K_TOPIN", "K_PARALLEL", "K_TIMINGDISABLES", "K_THRUPIN", "K_MACRO",
  "K_PARTITIONS", "K_TURNOFF", "K_FROMCLOCKPIN", "K_FROMCOMPPIN",
  "K_FROMIOPIN", "K_TOCLOCKPIN", "K_TOCOMPPIN", "K_TOIOPIN", "K_SETUPRISE",
  "K_SETUPFALL", "K_HOLDRISE", "K_HOLDFALL", "K_VPIN", "K_SUBNET",
  "K_XTALK", "K_PIN", "K_SYNTHESIZED", "K_DEFINE", "K_DEFINES",
  "K_DEFINEB", "K_IF", "K_THEN", "K_ELSE", "K_FALSE", "K_TRUE", "K_EQ",
  "K_NE", "K_LE", "K_LT", "K_GE", "K_GT", "K_OR", "K_AND", "K_NOT",
  "K_SPECIAL", "K_DIRECTION", "K_RANGE", "K_FPC", "K_HORIZONTAL",
  "K_VERTICAL", "K_ALIGN", "K_MIN", "K_MAX", "K_EQUAL", "K_BOTTOMLEFT",
  "K_TOPRIGHT", "K_ROWS", "K_TAPER", "K_TAPERRULE", "K_VERSION",
  "K_DIVIDERCHAR", "K_BUSBITCHARS", "K_PROPERTYDEFINITIONS", "K_STRING",
  "K_REAL", "K_INTEGER", "K_PROPERTY", "K_BEGINEXT", "K_ENDEXT",
  "K_NAMEMAPSTRING", "K_ON", "K_OFF", "K_X", "K_Y", "K_COMPONENT",
  "K_MASK", "K_MASKSHIFT", "K_COMPSMASKSHIFT", "K_SAMEMASK",
  "K_PINPROPERTIES", "K_TEST", "K_COMMONSCANPINS", "K_SNET",
  "K_COMPONENTPIN", "K_REENTRANTPATHS", "K_SHIELD", "K_SHIELDNET",
  "K_NOSHIELD", "K_VIRTUAL", "K_ANTENNAPINPARTIALMETALAREA",
  "K_ANTENNAPINPARTIALMETALSIDEAREA", "K_ANTENNAPINGATEAREA",
  "K_ANTENNAPINDIFFAREA", "K_ANTENNAPINMAXAREACAR",
  "K_ANTENNAPINMAXSIDEAREACAR", "K_ANTENNAPINPARTIALCUTAREA",
  "K_ANTENNAPINMAXCUTCAR", "K_SIGNAL", "K_POWER", "K_GROUND", "K_CLOCK",
  "K_TIEOFF", "K_ANALOG", "K_SCAN", "K_RESET", "K_RING", "K_STRIPE",
  "K_FOLLOWPIN", "K_IOWIRE", "K_COREWIRE", "K_BLOCKWIRE", "K_FILLWIRE",
  "K_BLOCKAGEWIRE", "K_PADRING", "K_BLOCKRING", "K_BLOCKAGES",
  "K_PLACEMENT", "K_SLOTS", "K_FILLS", "K_PUSHDOWN", "K_NETLIST", "K_DIST",
  "K_USER", "K_TIMING", "K_BALANCED", "K_STEINER", "K_TRUNK",
  "K_FIXEDBUMP", "K_FENCE", "K_FREQUENCY", "K_GUIDE", "K_MAXBITS",
  "K_PARTITION", "K_TYPE", "K_ANTENNAMODEL", "K_DRCFILL", "K_OXIDE1",
  "K_OXIDE2", "K_OXIDE3", "K_OXIDE4", "K_OXIDE5", "K_OXIDE6", "K_OXIDE7",
  "K_OXIDE8", "K_OXIDE9", "K_OXIDE10", "K_OXIDE11", "K_OXIDE12",
  "K_OXIDE13", "K_OXIDE14", "K_OXIDE15", "K_OXIDE16", "K_OXIDE17",
  "K_OXIDE18", "K_OXIDE19", "K_OXIDE20", "K_OXIDE21", "K_OXIDE22",
  "K_OXIDE23", "K_OXIDE24", "K_OXIDE25", "K_OXIDE26", "K_OXIDE27",
  "K_OXIDE28", "K_OXIDE29", "K_OXIDE30", "K_OXIDE31", "K_OXIDE32",
  "K_CUTSIZE", "K_CUTSPACING", "K_DESIGNRULEWIDTH", "K_DIAGWIDTH",
  "K_ENCLOSURE", "K_HALO", "K_GROUNDSENSITIVITY", "K_HARDSPACING",
  "K_LAYERS", "K_MINCUTS", "K_NETEXPR", "K_OFFSET", "K_ORIGIN", "K_ROWCOL",
  "K_STYLES", "K_POLYGON", "K_PORT", "K_SUPPLYSENSITIVITY", "K_VIA",
  "K_VIARULE", "K_WIREEXT", "K_EXCEPTPGNET", "K_FILLWIREOPC", "K_OPC",
  "K_PARTIAL", "K_ROUTEHALO", "';'", "'-'", "'+'", "'('", "')'", "'*'",
  "','", "$accept", "def_file", "version_stmt", "$@1", "case_sens_stmt",
  "rules", "rule", "design_section", "design_name", "$@2", "end_design",
  "tech_name", "$@3", "array_name", "$@4", "floorplan_name", "$@5",
  "history", "prop_def_section", "$@6", "property_defs", "property_def",
  "$@7", "$@8", "$@9", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15",
  "property_type_and_val", "$@16", "$@17", "opt_num_val", "units",
  "divider_char", "bus_bit_chars", "canplace", "$@18", "cannotoccupy",
  "$@19", "orient", "die_area", "$@20", "pin_cap_rule", "start_def_cap",
  "pin_caps", "pin_cap", "end_def_cap", "pin_rule", "start_pins", "pins",
  "pin", "$@21", "$@22", "$@23", "pin_options", "pin_option", "$@24",
  "$@25", "$@26", "$@27", "$@28", "$@29", "$@30", "$@31", "$@32", "$@33",
  "pin_layer_mask_opt", "pin_via_mask_opt", "pin_poly_mask_opt",
  "pin_layer_spacing_opt", "pin_poly_spacing_opt", "pin_oxide", "use_type",
  "pin_layer_opt", "$@34", "end_pins", "row_rule", "$@35", "$@36",
  "row_do_option", "row_step_option", "row_options", "row_option", "$@37",
  "row_prop_list", "row_prop", "tracks_rule", "$@38", "track_start",
  "track_type", "track_opts", "track_mask_statement", "same_mask",
  "track_layer_statement", "$@39", "track_layers", "track_layer",
  "gcellgrid", "extension_section", "extension_stmt", "via_section", "via",
  "via_declarations", "via_declaration", "$@40", "$@41", "layer_stmts",
  "layer_stmt", "$@42", "$@43", "$@44", "$@45", "$@46", "$@47",
  "layer_viarule_opts", "$@48", "firstPt", "nextPt", "otherPts", "pt",
  "mask", "via_end", "regions_section", "regions_start", "regions_stmts",
  "regions_stmt", "$@49", "$@50", "rect_list", "region_options",
  "region_option", "$@51", "region_prop_list", "region_prop",
  "region_type", "comps_maskShift_section", "comps_section", "start_comps",
  "layer_statement", "maskLayer", "comps_rule", "comp", "comp_start",
  "comp_id_and_name", "$@52", "comp_net_list", "comp_options",
  "comp_option", "comp_extension_stmt", "comp_eeq", "$@53",
  "comp_generate", "$@54", "opt_pattern", "comp_source", "source_type",
  "comp_region", "comp_pnt_list", "comp_halo", "$@55", "halo_soft",
  "comp_routehalo", "$@56", "comp_property", "$@57", "comp_prop_list",
  "comp_prop", "comp_region_start", "comp_foreign", "$@58", "opt_paren",
  "comp_type", "maskShift", "$@59", "placement_status", "weight",
  "end_comps", "nets_section", "start_nets", "net_rules", "one_net",
  "net_and_connections", "net_start", "$@60", "net_name", "$@61", "$@62",
  "net_connections", "net_connection", "$@63", "$@64", "$@65", "conn_opt",
  "net_options", "net_option", "$@66", "$@67", "$@68", "$@69", "$@70",
  "$@71", "$@72", "$@73", "$@74", "$@75", "$@76", "net_prop_list",
  "net_prop", "netsource_type", "vpin_stmt", "$@77", "vpin_begin", "$@78",
  "vpin_layer_opt", "$@79", "vpin_options", "vpin_status", "net_type",
  "paths", "new_path", "$@80", "path", "$@81", "$@82", "virtual_statement",
  "rect_statement", "path_item_list", "path_item", "$@83", "$@84",
  "path_pt", "virtual_pt", "rect_pts", "opt_taper_style_s",
  "opt_taper_style", "opt_taper", "$@85", "opt_style", "opt_spaths",
  "opt_shape_style", "end_nets", "shape_type", "snets_section",
  "snet_rules", "snet_rule", "snet_options", "snet_option",
  "snet_other_option", "$@86", "$@87", "$@88", "$@89", "$@90", "$@91",
  "$@92", "$@93", "$@94", "$@95", "orient_pt", "shield_layer", "$@96",
  "snet_width", "$@97", "snet_voltage", "$@98", "snet_spacing", "$@99",
  "$@100", "snet_prop_list", "snet_prop", "opt_snet_range", "opt_range",
  "pattern_type", "spaths", "snew_path", "$@101", "spath", "$@102",
  "$@103", "width", "start_snets", "end_snets", "groups_section",
  "groups_start", "group_rules", "group_rule", "start_group", "$@104",
  "group_members", "group_member", "group_options", "group_option",
  "$@105", "$@106", "group_region", "group_prop_list", "group_prop",
  "group_soft_options", "group_soft_option", "groups_end",
  "assertions_section", "constraint_section", "assertions_start",
  "constraints_start", "constraint_rules", "constraint_rule",
  "operand_rule", "operand", "$@107", "$@108", "operand_list",
  "wiredlogic_rule", "$@109", "opt_plus", "delay_specs", "delay_spec",
  "constraints_end", "assertions_end", "scanchains_section",
  "scanchain_start", "scanchain_rules", "scan_rule", "start_scan", "$@110",
  "scan_members", "opt_pin", "scan_member", "$@111", "$@112", "$@113",
  "$@114", "$@115", "$@116", "opt_common_pins", "floating_inst_list",
  "one_floating_inst", "$@117", "floating_pins", "ordered_inst_list",
  "one_ordered_inst", "$@118", "ordered_pins", "partition_maxbits",
  "scanchain_end", "iotiming_section", "iotiming_start", "iotiming_rules",
  "iotiming_rule", "start_iotiming", "$@119", "iotiming_members",
  "iotiming_member", "$@120", "$@121", "iotiming_drivecell_opt", "$@122",
  "$@123", "iotiming_frompin", "$@124", "iotiming_parallel", "risefall",
  "iotiming_end", "floorplan_contraints_section", "fp_start", "fp_stmts",
  "fp_stmt", "$@125", "$@126", "h_or_v", "constraint_type",
  "constrain_what_list", "constrain_what", "$@127", "$@128",
  "row_or_comp_list", "row_or_comp", "$@129", "$@130",
  "timingdisables_section", "timingdisables_start", "timingdisables_rules",
  "timingdisables_rule", "$@131", "$@132", "$@133", "$@134",
  "td_macro_option", "$@135", "$@136", "$@137", "timingdisables_end",
  "partitions_section", "partitions_start", "partition_rules",
  "partition_rule", "start_partition", "$@138", "turnoff", "turnoff_setup",
  "turnoff_hold", "partition_members", "partition_member", "$@139",
  "$@140", "$@141", "$@142", "$@143", "$@144", "minmaxpins", "$@145",
  "min_or_max_list", "min_or_max_member", "pin_list",
  "risefallminmax1_list", "risefallminmax1", "risefallminmax2_list",
  "risefallminmax2", "partitions_end", "comp_names", "comp_name", "$@146",
  "subnet_opt_syn", "subnet_options", "subnet_option", "$@147", "$@148",
  "subnet_type", "pin_props_section", "begin_pin_props", "opt_semi",
  "end_pin_props", "pin_prop_list", "pin_prop_terminal", "$@149", "$@150",
  "pin_prop_options", "pin_prop", "$@151", "pin_prop_name_value_list",
  "pin_prop_name_value", "blockage_section", "blockage_start",
  "blockage_end", "blockage_defs", "blockage_def", "blockage_rule",
  "$@152", "$@153", "$@154", "layer_blockage_rules", "layer_blockage_rule",
  "mask_blockage_rule", "comp_blockage_rule", "$@155",
  "placement_comp_rules", "placement_comp_rule", "$@156",
  "rectPoly_blockage_rules", "rectPoly_blockage", "$@157", "slot_section",
  "slot_start", "slot_end", "slot_defs", "slot_def", "slot_rule", "$@158",
  "$@159", "geom_slot_rules", "geom_slot", "$@160", "fill_section",
  "fill_start", "fill_end", "fill_defs", "fill_def", "$@161", "$@162",
  "fill_rule", "$@163", "$@164", "geom_fill_rules", "geom_fill", "$@165",
  "fill_layer_mask_opc_opt", "opt_mask_opc_l", "fill_layer_opc",
  "fill_via_pt", "fill_via_mask_opc_opt", "opt_mask_opc", "fill_via_opc",
  "fill_mask", "fill_viaMask", "nondefaultrule_section",
  "nondefault_start", "nondefault_end", "nondefault_defs",
  "nondefault_def", "$@166", "$@167", "nondefault_options",
  "nondefault_option", "$@168", "$@169", "$@170", "$@171", "$@172",
  "nondefault_layer_options", "nondefault_layer_option",
  "nondefault_prop_opt", "$@173", "nondefault_prop_list",
  "nondefault_prop", "styles_section", "styles_start", "styles_end",
  "styles_rules", "styles_rule", "$@174", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-1192)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-572)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -98, -1192,    81,   102,   110, -1192,   201,    82,  -150,  -142,
    -111, -1192,   394, -1192, -1192, -1192, -1192, -1192,   180, -1192,
     108, -1192, -1192, -1192, -1192, -1192,   244,   252,   213,   213,
   -1192,   257,   260,   272,   275,   300,   326,   342,   349,   353,
     216,   396,   403,   421,   425,   431,   435, -1192, -1192, -1192,
     440,   445,   450,   461,   469, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192,   483, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192,   -40, -1192, -1192,   493,    -8,
     508,   427,   528,   535,   555,   565,   278,   306, -1192, -1192,
   -1192, -1192,   582,   590,   307,   310,   311,   312,   313,   314,
     320,   321,   322, -1192,   324,   325,   327,   328,   329,   330,
   -1192,    35,   332,   339,   340,   344,   346,    47,   -49, -1192,
     -48,   -47,   -33,   -31,   -30,   -26,   -25,   -22,   -21,   -19,
     -11,   -10,    -6,     1,    36,    44,    50, -1192, -1192,    51,
     347, -1192,   352,   614,   363,   364,   625,   630,    11,   278,
   -1192, -1192,   623,   655, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   601,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   656,
     641, -1192, -1192,   644, -1192, -1192, -1192,   639,   659, -1192,
   -1192, -1192,   628, -1192, -1192,   634, -1192, -1192, -1192, -1192,
   -1192,   629, -1192, -1192, -1192, -1192, -1192,   619, -1192, -1192,
   -1192,   605, -1192, -1192, -1192, -1192,   585,   284, -1192, -1192,
   -1192, -1192,   600, -1192,   607, -1192, -1192, -1192, -1192,   591,
     393, -1192, -1192, -1192,   545, -1192, -1192,   586,   249, -1192,
   -1192,   584, -1192, -1192, -1192, -1192,   516, -1192, -1192, -1192,
     481,    41, -1192, -1192,    10,   489,   679, -1192, -1192, -1192,
     490,    17, -1192, -1192, -1192,   705,    52,   439,   672, -1192,
   -1192, -1192, -1192,   428, -1192, -1192,   717,   719,    13,    14,
   -1192, -1192,   720,   721,   434, -1192, -1192, -1192, -1192, -1192,
   -1192,   567, -1192, -1192, -1192, -1192,   710, -1192, -1192,   730,
     729, -1192,   732, -1192,   733, -1192,   735,  -128,     9, -1192,
      84,  -114, -1192,   -95, -1192,   736,   737, -1192, -1192, -1192,
     446,   447, -1192, -1192, -1192, -1192,   738,    60, -1192, -1192,
      88, -1192,   742, -1192, -1192, -1192, -1192,   453, -1192,   753,
     151, -1192,   754, -1192, -1192, -1192,   278, -1192, -1192, -1192,
   -1192,   -15, -1192, -1192, -1192,   -12, -1192,   699, -1192, -1192,
   -1192,   756, -1192,   637,   637,   465,   466,   470,   472,  -182,
     740,   766, -1192,   770,   772,   773,   774,   776,   777, -1192,
     778,   821,   822,   824,   464,   800, -1192, -1192,   827, -1192,
      18, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,    19, -1192, -1192, -1192,   278, -1192, -1192, -1192, -1192,
     531, -1192, -1192,   544, -1192, -1192, -1192,   804, -1192,   462,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   170,
     829,   830,   162,   162,   831,   227, -1192, -1192,   268, -1192,
   -1192,   832, -1192,   121, -1192, -1192,   267,   833,   835,   836,
   -1192,   731, -1192,   283, -1192, -1192,   837,   839, -1192,   278,
     278,     4,   840,   278, -1192, -1192, -1192,   841,   842,   278,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192,   823,   825, -1192, -1192, -1192, -1192,
   -1192, -1192,   843,   637,    53,    53,    53,    53,    53,    53,
      53,    53,    53,   553,   775,   846, -1192,   278, -1192, -1192,
     158,   847, -1192, -1192, -1192,   278, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192,   848, -1192,   278,   278,   637, -1192,   851,
    -100,   850, -1192, -1192, -1192,   515,   852,   -13,   853, -1192,
   -1192, -1192, -1192,   854, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,   278, -1192,   158,   855, -1192,   515,   856,   -13,   857,
     337, -1192, -1192, -1192, -1192,   858, -1192, -1192,   859, -1192,
   -1192,   862, -1192,     5, -1192, -1192, -1192,   864, -1192,    40,
     130,   561, -1192,   354, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,   865, -1192, -1192,   866, -1192,    58, -1192, -1192, -1192,
     867,   869,    33,   375, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192,   574, -1192,   278, -1192, -1192, -1192,   278,
     278, -1192, -1192,   278,   278,   235,   278,   868,   870,   581,
   -1192,   874, -1192, -1192,   875,   587,   589,   592,   594,   595,
     596,   597,   598,   599, -1192, -1192,   707,   270,   278,   278,
     876, -1192, -1192, -1192, -1192, -1192, -1192,   880,   637,   881,
     890,   892,   826, -1192, -1192,   278, -1192,   602, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192,   894, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192,   896,   897,   898, -1192,   899,   900, -1192,
     901,   902,   904,   278,   906, -1192, -1192,   907, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192,   908,   909,   910,   911, -1192,
     912, -1192,   913,   914,   915, -1192, -1192, -1192,   916, -1192,
     162, -1192, -1192, -1192,   813,   917,   918,   919,   920,   923,
   -1192, -1192,   924,   624,   925,   632, -1192,   927,   926,   928,
     309,   817,   642, -1192, -1192,   643, -1192, -1192,   386,   929,
     931,   934,   935,   936,   937, -1192, -1192,    -9, -1192,   278,
      37, -1192,   278, -1192, -1192, -1192,   278, -1192,    15, -1192,
   -1192,   278,   921,   922, -1192,   930, -1192,   796,   796, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   939,
     938,   651,   932, -1192,    54, -1192, -1192, -1192, -1192,   278,
     271,   942, -1192, -1192,    20,   187,   890, -1192, -1192, -1192,
     943,   946, -1192,     8, -1192,   947, -1192, -1192, -1192, -1192,
     224,   899, -1192, -1192,   902, -1192, -1192,   903, -1192, -1192,
   -1192,   278, -1192,   951, -1192,   952,   248,   911, -1192, -1192,
   -1192,   688, -1192,   933, -1192,    21,   294, -1192,   948, -1192,
     953, -1192, -1192, -1192, -1192,   949,   957,   958,   949,   959,
   -1192,   734, -1192, -1192,   960,   962, -1192,   963,   964,   965,
   -1192, -1192, -1192,   968,   969, -1192, -1192, -1192, -1192,   970,
     971, -1192,   972,   973, -1192,   274,   668, -1192, -1192, -1192,
     974, -1192, -1192,   278,   -16,   219,   278, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192,   975,   976,   977, -1192,   978,   979,
     979, -1192,   801, -1192, -1192, -1192, -1192, -1192, -1192,   981,
     982,   983, -1192, -1192, -1192, -1192,   -59, -1192, -1192, -1192,
     984, -1192,   637, -1192, -1192, -1192, -1192,   985,   988, -1192,
   -1192, -1192,   680, -1192, -1192, -1192, -1192, -1192,   903, -1192,
   -1192, -1192,   383,   278, -1192, -1192, -1192, -1192, -1192, -1192,
     989,   278, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,   990, -1192, -1192, -1192,   278, -1192,   991,   992,   993,
   -1192,   996, -1192,   700, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,   997,   998, -1192,   891, -1192, -1192, -1192, -1192, -1192,
     280,   999,   940, -1192,   437,   390,   438,   437,   390,   438,
   -1192,   845, -1192,   -14, -1192, -1192, -1192,  1001, -1192,   278,
   -1192,  -103, -1192, -1192, -1192, -1192,   -96, -1192,   708, -1192,
   -1192, -1192, -1192,  1002, -1192,  1003,  1004,  1005,  -141,   986,
     987,   994,   288,  1006, -1192, -1192, -1192, -1192, -1192, -1192,
    1011,  1013,  1014,  1015,  1016,  1017,  1018,  1021,  1022, -1192,
     196, -1192, -1192,  1023, -1192,  1024,  1026,  1027, -1192,   724,
      39,   902, -1192, -1192, -1192, -1192,   278, -1192,   886, -1192,
     915,   278,   278, -1192, -1192,   915, -1192, -1192, -1192, -1192,
     449, -1192, -1192,   739,   741,   743, -1192, -1192, -1192,   941,
   -1192,   398, -1192,  1029, -1192, -1192,  1028,  1031,  1033,  1034,
     390, -1192,  1036,  1037,  1038,  1041, -1192, -1192,   390, -1192,
    1042, -1192,  1043, -1192, -1192, -1192,  1044, -1192, -1192,   278,
    1045, -1192,  1046, -1192,   278, -1192,   278,  1000,  1049,  1048,
   -1192, -1192, -1192,  1051,  1053,  1054, -1192,   861, -1192, -1192,
     289, -1192,  1011,   759, -1192, -1192,  1055, -1192, -1192,   759,
     767, -1192, -1192, -1192, -1192,  1056,   768,   768,   768, -1192,
   -1192, -1192,  1059, -1192, -1192,    22, -1192, -1192, -1192, -1192,
   -1192,   637,  1060, -1192,   933,   278, -1192,   290, -1192, -1192,
   -1192, -1192,  1063, -1192,  1064, -1192,   779,  1065, -1192, -1192,
   -1192,   780,  1066, -1192,    63,  1067,  1069,  1071,  1072, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192,  1068, -1192, -1192,
   -1192, -1192,  1073,   467, -1192, -1192,  1074,  1075,  1008, -1192,
   -1192,   198, -1192, -1192,   278, -1192,  1011,   944,   278,  1076,
   -1192,   803,  1080, -1192,   -18, -1192,   787,   788,   789,  1084,
     118, -1192,  1085,    23,    24, -1192, -1192,  1086, -1192,   278,
      79, -1192, -1192,  1087,  1089,  1090, -1192,  1091, -1192, -1192,
   -1192, -1192, -1192,  1092,  1093, -1192, -1192, -1192, -1192, -1192,
    1096, -1192, -1192, -1192, -1192, -1192,   802,   807,  1097, -1192,
   -1192, -1192,   515, -1192,  1098,  1099,  1100,  1101,  1102,  1103,
    1104,  1105,  1106,   549, -1192,  1094, -1192, -1192, -1192, -1192,
     637, -1192,  1107,   278, -1192,   278,  1108,   482, -1192, -1192,
   -1192, -1192, -1192,  1111, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192,    16,    29,    30,    32,    12, -1192,   278,  1110,
     337, -1192,   792,   806,  1113, -1192,   828,   828, -1192,  1112,
    1114,   499, -1192, -1192, -1192, -1192,  1115,  1118,  1119, -1192,
   -1192,  1109,  1109,  1109,  1109,  1116,  1117,  1109,  1120, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192,  1122, -1192,  1123,  1124,  1125, -1192, -1192, -1192,
     278,  1126, -1192, -1192, -1192,   834,  1127,   902,   838, -1192,
     844, -1192,   849, -1192,   860, -1192,   588,   863,  1128,   872,
   -1192, -1192, -1192, -1192, -1192, -1192,    12,   873,   877,   878,
    1032,    42, -1192,  1129, -1192, -1192, -1192, -1192, -1192,     6,
   -1192,   554, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192,   966,   278,   871,  1009,
     879, -1192,   903, -1192, -1192, -1192, -1192,  1133,  1121,  1134,
   -1192,   105,    25, -1192,  1139,  1145, -1192,  1144, -1192, -1192,
   -1192, -1192,  1146,  1147,  1148, -1192, -1192, -1192, -1192,   980,
    1152,  1153,  1155,  1157,  1007,  1156,   882, -1192,   887, -1192,
   -1192,  1138,  1159,  1160,   618, -1192,   883,    26,    27,  1169,
    1171, -1192,  1174,  1177, -1192, -1192, -1192,  1178,   -17, -1192,
   -1192, -1192, -1192,  1179,     2, -1192,  1180,   278, -1192,  1181,
    1162,  1183,  1184,  1158,   895, -1192,   893,   905,   945,   950,
     954,   955,   956,   961, -1192,  1187,  1188,   278, -1192,  1189,
    1190,   278,  1191,  1194,  1172,  1195,  1196,  1182,  1198,  1199,
   -1192, -1192, -1192, -1192,   967,   995, -1192, -1192, -1192, -1192,
     278, -1192, -1192,   278,  1010,  1202,  1201,  1173,  1012,  1203,
    1185,  1204,  1208,  1209, -1192,   278, -1192,  1210,  1211,  1212,
   -1192,  1192,  1214,  1215,  1218,  1219, -1192,  1019, -1192,  1220,
    1221,  1197,  1222,  1020,  1025,   278,  1030, -1192,  1223,  1224,
    1035, -1192, -1192,  1225, -1192,  1226, -1192,  1227, -1192,  1039,
    1040,  1228,  1229,  1230,  1231, -1192
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       3,     4,     0,     6,     0,     1,     0,     0,     0,     0,
       0,    11,     0,     5,     7,     8,    60,    51,     0,    54,
       0,    56,    58,    96,    98,   108,     0,     0,     0,     0,
     213,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    61,   246,   309,
       0,     0,     0,     0,     0,    10,    12,    38,     2,    48,
      34,    41,    43,    46,    50,    40,    35,    36,    37,    39,
      44,   112,    45,   118,    47,    49,     0,    42,    17,    33,
     250,    27,   287,    19,    15,   312,    23,   381,    30,   523,
      21,   605,    13,    16,   638,   638,    28,   666,    22,   712,
      20,   739,    32,   764,    25,   782,    26,   852,    14,   869,
      29,   905,    18,   919,    24,     0,    31,   982,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   111,   232,
     233,   231,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    53,     0,     0,     0,     0,     0,     0,
      63,     0,   849,     0,     0,     0,     0,     0,     0,   229,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   951,   949,     0,
       0,   249,     0,     0,     0,     0,     0,     0,     0,     0,
     274,   117,     0,     0,   308,   286,   380,   601,   947,   604,
     637,   665,   636,   711,   763,   781,   738,    94,    95,     0,
     311,   306,   310,   850,   848,   867,   903,   917,   980,     0,
       0,   113,   110,     0,   120,   119,   116,     0,     0,   252,
     251,   248,     0,   289,   288,     0,   316,   313,   321,   318,
     307,     0,   385,   382,   402,   384,   379,     0,   526,   524,
     522,     0,   608,   606,   610,   603,     0,     0,   639,   640,
     641,   634,     0,   635,     0,   669,   667,   671,   664,     0,
       0,   713,   717,   710,     0,   741,   740,     0,     0,   765,
     762,     0,   785,   783,   795,   780,     0,   854,   847,   853,
       0,     0,   866,   870,     0,     0,     0,   902,   906,   911,
       0,     0,   916,   920,   928,     0,     0,     0,     0,   979,
     983,    52,    55,     0,    57,    59,     0,     0,     0,     0,
     276,   275,     0,     0,     0,    65,    77,    71,    81,    73,
      67,     0,    75,    69,    79,    64,     0,   115,   212,     0,
       0,   284,     0,   285,     0,   378,     0,     0,   315,   509,
       0,     0,   602,     0,   633,     0,   613,   663,   643,   645,
       0,     0,   652,   656,   662,   709,     0,     0,   736,   715,
       0,   737,     0,   779,   766,   769,   771,     0,   830,     0,
       0,   851,     0,   868,   872,   875,     0,   900,   897,   904,
     908,     0,   918,   925,   922,     0,   952,     0,   946,   950,
     981,     0,    93,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    83,     0,     0,     0,     0,     0,     0,    62,
       0,     0,     0,     0,     0,     0,   253,   290,     0,   314,
       0,   335,   322,   334,   330,   323,   324,   329,   331,   332,
     333,     0,   328,   325,   327,     0,   326,   320,   319,   387,
       0,   386,   383,     0,   432,   403,   420,   447,   525,     0,
     560,   527,   531,   528,   529,   530,   609,   612,   611,     0,
       0,     0,     0,     0,     0,     0,   670,   668,     0,   687,
     672,     0,   714,     0,   725,   718,     0,     0,     0,     0,
     773,   787,   784,     0,   809,   796,     0,     0,   890,     0,
       0,     0,     0,     0,   914,   907,   912,     0,     0,     0,
     931,   921,   929,   954,   948,   984,   100,   102,   103,   101,
     104,   107,   106,   105,     0,     0,   278,   280,   279,   281,
     109,   277,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   255,     0,   317,   338,
       0,     0,   336,   374,   375,   370,   376,   365,   364,   357,
     247,   372,   351,     0,   348,     0,   347,     0,   391,     0,
       0,     0,   455,   456,   410,     0,     0,     0,     0,   457,
     418,   445,   426,     0,   430,   421,   423,   407,   408,   404,
     448,     0,   543,     0,     0,   551,     0,     0,     0,     0,
       0,   573,   575,   577,   558,     0,   535,   549,     0,   540,
     545,   532,   607,     0,   620,   614,   644,     0,   649,     0,
       0,   654,   642,     0,   657,   675,   677,   679,   681,   683,
     685,     0,   734,   735,     0,   722,     0,   744,   745,   742,
       0,     0,     0,   789,   786,   797,   799,   801,   803,   805,
     807,   855,   873,   876,   899,     0,   871,   898,   909,     0,
       0,   926,   923,     0,     0,     0,     0,     0,     0,     0,
     214,    88,    86,    84,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   114,   121,   235,     0,     0,   294,
       0,   343,   344,   345,   346,   342,   377,     0,     0,     0,
       0,     0,   353,   355,   349,     0,   369,   388,   389,   442,
     438,   439,   440,   441,   406,   413,     0,   201,   202,   203,
     204,   205,   206,   207,   208,   416,   417,   592,   589,   590,
     591,   412,   415,     0,     0,     0,   414,     0,     0,   424,
       0,     0,     0,     0,     0,   548,   554,     0,   556,   557,
     553,   555,   510,   511,   512,   513,   514,   515,   516,   519,
     520,   521,   518,   517,   538,     0,     0,     0,     0,   539,
       0,   550,     0,     0,     0,   618,   628,   616,     0,   647,
       0,   650,   648,   655,     0,     0,     0,     0,     0,     0,
     691,   699,     0,   688,     0,     0,   721,     0,     0,     0,
       0,     0,     0,   774,   777,     0,   790,   791,   792,     0,
       0,     0,     0,     0,     0,   857,   877,     0,   891,     0,
       0,   913,     0,   933,   939,   930,     0,   953,     0,   955,
     966,     0,     0,     0,   245,   216,    89,   587,   587,    90,
      66,    78,    72,    82,    74,    68,    76,    70,    80,     0,
       0,     0,   239,   254,     0,   268,   256,   267,   292,     0,
       0,   340,   337,   371,     0,     0,   358,   359,   373,   354,
       0,     0,   350,     0,   392,     0,   411,   419,   446,   427,
       0,   431,   433,   422,     0,   409,   462,   405,   458,   449,
     443,     0,   552,     0,   576,     0,     0,   559,   580,   536,
     541,   561,   597,   534,   593,     0,   615,   623,     0,   651,
       0,   658,   659,   660,   661,   673,   678,   680,   673,     0,
     684,   707,   716,   723,     0,     0,   746,     0,     0,     0,
     750,   767,   770,     0,     0,   772,   793,   794,   788,     0,
       0,   818,     0,     0,   818,     0,   874,   895,   892,   894,
       0,   276,   910,     0,     0,     0,     0,   957,   972,   956,
     964,   960,   962,   276,     0,     0,     0,   220,     0,    91,
      91,   122,   237,   230,   240,   234,   257,   272,   262,     0,
       0,     0,   259,   264,   293,   291,     0,   295,   341,   339,
       0,   367,     0,   362,   363,   361,   360,     0,     0,   393,
     397,   395,     0,   831,   436,   437,   435,   434,   425,   497,
     460,   459,   450,     0,   574,   578,   583,   584,   582,   581,
     570,     0,   562,   564,   565,   563,   566,   569,   568,   567,
     546,     0,   595,   594,   622,     0,   619,     0,     0,     0,
     629,   617,   646,     0,   674,   676,   693,   692,   701,   700,
     682,     0,     0,   686,   729,   719,   720,   748,   747,   749,
       0,     0,     0,   778,     0,     0,   802,     0,     0,   808,
     856,     0,   858,     0,   878,   881,   882,     0,   896,   901,
     276,     0,   927,   934,   935,   936,     0,   276,     0,   940,
     941,   942,   276,     0,   974,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    92,    87,    85,   124,   238,   236,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   296,
       0,   368,   366,     0,   356,     0,     0,     0,   390,   428,
       0,     0,   453,   454,   452,   444,     0,   544,   585,   537,
       0,     0,     0,   600,   505,     0,   621,   630,   631,   632,
       0,   624,   653,   695,   703,     0,   708,   730,   724,     0,
     743,     0,   751,     0,   775,   812,     0,     0,     0,     0,
     800,   824,     0,     0,     0,     0,   819,   812,   806,   859,
       0,   884,     0,   886,   887,   888,     0,   889,   893,   915,
       0,   937,     0,   943,   938,   924,   932,     0,   973,     0,
     961,   963,   985,     0,     0,     0,   215,     0,   221,   588,
       0,   244,   242,   282,   273,   263,     0,   270,   269,   282,
       0,   299,   304,   305,   298,     0,   399,   399,   399,   833,
     837,   832,     0,   501,   502,     0,   463,   498,   500,   499,
     461,     0,     0,   579,   572,     0,   276,     0,   596,   626,
     627,   625,     0,   694,     0,   702,   689,     0,   726,   752,
     754,     0,     0,   798,     0,     0,     0,     0,     0,   825,
     820,   822,   821,   823,   804,   861,   879,     0,   883,   880,
     944,   945,     0,     0,   975,   965,     0,     0,   218,   222,
     123,     0,   125,   127,     0,   241,   242,     0,     0,     0,
     260,     0,   297,   352,     0,   400,     0,     0,     0,     0,
     429,   504,     0,     0,     0,   467,   451,     0,   276,   547,
       0,   598,   506,     0,     0,     0,   731,     0,   756,   756,
     768,   776,   810,     0,     0,   813,   826,   828,   827,   829,
     860,   885,   958,   977,   978,   976,     0,     0,     0,   217,
     224,   136,     0,   126,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   132,     0,   139,   135,   130,   142,
       0,   243,     0,     0,   271,     0,     0,     0,   300,   401,
     394,   398,   396,     0,   843,   844,   845,   841,   846,   838,
     839,   503,     0,     0,     0,     0,   464,   586,   542,     0,
       0,   467,     0,     0,     0,   727,   753,   755,   816,     0,
       0,     0,   862,   967,    97,    99,     0,   223,     0,   134,
     128,   209,   209,   209,   209,     0,     0,   209,     0,   169,
     170,   171,   172,   173,   174,   175,   176,   177,   178,   179,
     180,   181,   182,   183,   184,   185,   186,   187,   188,   189,
     190,   191,   192,   193,   194,   195,   196,   197,   198,   199,
     200,   156,     0,   129,     0,     0,     0,   144,   283,   258,
       0,     0,   302,   303,   301,   835,     0,     0,     0,   484,
       0,   486,     0,   485,     0,   487,   469,     0,     0,     0,
     477,   478,   468,   483,   508,   507,   599,   696,   704,     0,
     732,     0,   757,   811,   814,   815,   864,   865,   863,   959,
     219,     0,   225,   137,   210,   145,   146,   147,   148,   149,
     151,   153,   154,   133,   140,   131,   159,     0,     0,     0,
       0,   842,   840,   488,   490,   489,   491,     0,   471,     0,
     466,   481,     0,   465,     0,     0,   690,     0,   728,   760,
     758,   817,     0,     0,     0,   968,   227,   228,   226,   157,
       0,     0,     0,     0,   161,     0,     0,   276,     0,   836,
     834,     0,     0,     0,   470,   479,     0,     0,     0,     0,
       0,   733,     0,     0,   970,   969,   971,     0,   163,   211,
     150,   152,   155,     0,   166,   160,     0,   261,   265,     0,
       0,     0,     0,   472,     0,   482,     0,     0,     0,     0,
       0,     0,     0,     0,   158,     0,     0,     0,   162,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     492,   494,   493,   495,   697,   705,   761,   759,   164,   165,
       0,   167,   168,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   138,     0,   143,     0,     0,     0,
     496,     0,     0,     0,     0,     0,   276,     0,   474,     0,
       0,     0,     0,     0,     0,   141,     0,   475,     0,     0,
       0,   698,   706,     0,   473,     0,   480,     0,   476,     0,
       0,     0,     0,     0,     0,   266
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,   282, -1192, -1192,   181, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192,  -403, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192,  -591, -1191, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192,  1186, -1192, -1192, -1192, -1192, -1192,   -80,   115,
   -1192, -1192,  -343, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,  -497,
    -187,  -942,  -126,    31, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   645, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   373,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192,    34, -1192, -1192,
   -1192, -1192, -1192, -1192,  1077, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192,  -630, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
     359, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
     783,  -878, -1192, -1192,   114, -1192, -1192, -1192, -1192,  -155,
   -1192, -1192, -1192, -1119, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192,  -149, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192,   358, -1192,   418,   660,   117, -1192, -1192,   116,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192,  1165, -1192, -1192,
    -250, -1192, -1192,   790, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,   348, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192,  -891, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192,   -67, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,    90, -1192, -1192, -1192, -1192,   331, -1192,   197, -1039,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192,   769, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192,   448, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192,   315, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
    1047, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192, -1192,
   -1192, -1192
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,     3,     4,     7,    12,    55,    56,    57,   118,
      58,    59,   120,    60,   122,    61,   123,    62,    63,   150,
     209,   335,   413,   418,   421,   415,   417,   420,   414,   422,
     416,   675,   838,   837,  1105,    64,    65,    66,    67,   124,
      68,   125,   524,    69,   126,    70,    71,   157,   221,   222,
      72,    73,   158,   225,   339,   849,  1107,  1210,  1292,  1465,
    1462,  1418,  1559,  1464,  1564,  1466,  1561,  1562,  1563,  1588,
    1566,  1594,  1617,  1621,  1461,   725,  1515,  1560,   226,    74,
     133,   835,   967,  1349,  1102,  1208,  1350,  1417,  1512,    75,
     227,    76,   131,   851,   852,  1109,   975,  1110,  1295,  1296,
      77,    78,  1305,    79,    80,   160,   230,   342,   546,   687,
     856,  1111,  1117,  1375,  1113,  1118,  1623,   857,  1112,   189,
     531,   409,   321,  1298,   231,    81,    82,   161,   234,   344,
     547,   689,   860,   987,  1221,  1302,  1378,  1224,    83,    84,
      85,   151,   212,   162,   237,   238,   239,   346,   348,   347,
     432,   433,   434,   697,   435,   690,   989,   436,   695,   437,
     566,   438,   702,   870,   439,   871,   440,   700,   866,   867,
     441,   442,   699,   992,   443,   444,   701,   445,   446,   240,
      86,    87,   163,   243,   244,   245,   350,   451,   568,   875,
     707,   874,  1125,  1127,  1126,  1306,   351,   455,   741,   740,
     716,   733,   738,   739,   884,   735,  1003,  1230,   737,   881,
     882,   714,   456,  1012,   457,   734,   591,   742,  1135,  1136,
     589,   887,  1011,  1131,   888,  1009,  1315,  1490,  1491,  1396,
    1492,  1604,  1576,  1493,  1543,  1540,  1130,  1237,  1238,  1312,
    1239,  1247,  1322,   246,   764,    88,   164,   249,   353,   461,
     462,   774,   770,  1020,   772,  1021,   744,   773,  1142,   747,
     768,  1030,  1139,  1140,   463,   765,   464,   766,   465,   767,
    1138,   897,   898,  1243,   969,   731,   903,  1033,  1145,   904,
    1031,  1401,  1144,    89,   250,    90,    91,   165,   253,   254,
     355,   356,   468,   469,   615,   907,   905,  1036,  1041,  1151,
     906,  1040,   255,    92,    93,    94,    95,   166,   258,   259,
     618,   470,   471,   619,   260,   474,   784,   475,   624,   263,
     261,    96,    97,   168,   266,   267,   366,   367,  1045,   480,
     789,   790,   791,   792,   793,   794,   920,   916,  1047,  1153,
    1253,   917,  1049,  1154,  1255,  1053,   268,    98,    99,   169,
     271,   272,   481,   370,   485,   797,  1054,  1158,  1327,  1500,
    1159,  1257,  1548,   636,   273,   100,   101,   170,   276,   372,
     800,   639,   930,  1060,  1162,  1328,  1329,  1406,  1502,  1583,
    1582,   102,   103,   171,   279,   487,  1061,   488,   489,   805,
     933,  1262,   934,   280,   104,   105,   172,   283,   284,   379,
     644,   808,   938,   380,   495,   809,   810,   811,   812,   813,
     814,  1263,  1408,  1264,  1335,  1503,  1066,  1176,  1170,  1171,
     285,  1129,  1231,  1309,  1530,  1310,  1389,  1477,  1476,  1390,
     106,   107,   214,   288,   173,   289,   382,   815,   945,  1072,
    1275,  1340,  1412,   108,   109,   292,   174,   293,   294,   497,
     816,   498,   946,  1074,  1075,  1076,  1277,   653,   818,  1077,
     501,   388,   500,   110,   111,   297,   175,   298,   299,   502,
     820,   391,   506,   660,   112,   113,   302,   176,   303,   508,
     824,   304,   507,   823,   395,   512,   664,   954,  1083,  1084,
    1088,   955,  1089,  1090,  1085,  1091,   114,   115,   398,   306,
     178,   305,   513,   665,   829,  1093,  1413,  1096,  1097,  1095,
    1509,  1555,   830,  1094,  1198,  1284,   116,   117,   309,   179,
     310,   666
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     190,   525,   320,   655,   431,   748,  1008,   363,   454,  1079,
     460,  1236,   999,   447,   509,   503,  1486,   318,   509,   405,
     407,  1098,  1478,   564,   479,  1034,   990,   484,  1313,  1392,
    1394,  1577,  1606,  1608,   386,  1480,  1482,   494,  1484,   210,
     386,  1615,  1487,   957,  1180,   393,   775,   223,   228,   232,
     549,   550,   551,   552,   553,   554,   555,   556,   557,   558,
    1619,     1,   947,   235,  1552,   241,   247,   503,   219,   384,
     251,   256,  1190,  1549,   262,   264,   776,   269,   727,  1192,
     709,     5,  1332,    11,   976,   274,   277,  1232,   449,    -9,
     281,    -9,    -9,    -9,    -9,    -9,    -9,   286,    -9,    -9,
      -9,    -9,    -9,   977,   978,    -9,    -9,  1119,  1379,  1574,
       6,    -9,   530,    -9,     8,   188,   358,   359,   360,   361,
     710,   711,   712,   713,    -9,    -9,   614,  1399,  1321,   450,
     670,  1269,   290,  1000,  1400,  1575,    -9,   803,  1189,  1269,
     295,   804,    -9,   220,    13,  1194,   300,   307,   397,   560,
    1196,    -9,    14,  1202,  1384,  1385,   188,    -9,   798,   799,
    1181,  1182,    -9,   822,   706,   948,   429,   826,   430,   831,
    1386,   777,   560,  1165,  1120,    -9,  1177,  1387,    -9,    -9,
     452,   958,   453,    15,   559,   560,   119,  1488,  1191,    -9,
     993,   994,    -9,   995,   561,  1193,  1233,  1234,  1550,   458,
    1489,   459,   121,  1183,  1184,  1185,   358,   359,   360,   361,
     949,   728,   729,   730,  1333,  1334,   671,   672,   673,   632,
     633,   560,   674,   634,   635,   143,  1351,  1004,  1005,    -9,
    1006,  1516,  1517,  1518,   553,   554,  1521,   556,   358,   359,
     360,   361,    -9,    -9,    -9,  1352,   224,   229,   233,    -9,
     127,  1016,  1017,  1616,  1018,   177,  1186,   385,   128,    -9,
     499,    -9,   236,   134,   242,   242,   135,   510,   504,   252,
     257,   510,  1620,   257,   265,  1187,   270,  1553,   136,   505,
    1081,   137,   511,   950,   275,   278,   181,   387,   560,   282,
     959,   562,   960,   387,  1554,   863,   287,    -9,   656,    -9,
      -9,   961,   962,   394,  1319,  1388,   138,  1001,   448,  1235,
     319,   563,   406,   408,  1479,   565,   188,   188,   188,   567,
     504,  1314,  1393,  1395,  1578,  1607,  1609,  1481,  1483,   211,
    1485,   291,   139,   979,   980,   981,  1235,   982,   779,   296,
     780,   983,  1353,  1354,   855,   301,   308,   177,   140,   625,
     626,   627,   628,   374,   477,   141,   478,   375,   376,   142,
     358,   359,   360,   361,    -9,   560,  1037,  1038,  1039,   781,
     781,     9,    10,   654,   190,   362,  1398,   659,   691,   692,
     693,   694,   482,   663,   483,   129,   130,  1355,  1356,  1357,
    1358,  1359,  1360,  1361,  1362,   645,   646,   647,   648,   649,
     650,    16,   144,    17,    18,    19,    20,    21,    22,   145,
      23,    24,    25,    26,    27,   637,   638,    28,    29,  1132,
    1133,   688,  1134,    30,  1222,    31,  1223,   146,   782,   698,
     780,   147,  1363,   377,   148,   560,    32,    33,   149,   704,
     705,   785,   786,   787,   788,   492,   152,   493,    34,   629,
     560,   153,  1249,  1250,    35,  1251,   154,  1605,  1087,   926,
     927,   928,   929,    36,   612,   743,   613,   155,   819,    37,
    1343,  1344,  1364,  1345,    38,   156,  1365,  1166,  1167,  1168,
    1169,  1366,  1367,  1368,  1369,  1472,  1473,    39,  1474,   159,
      40,    41,   592,   806,   807,   593,   594,   180,   572,   573,
     630,    42,  1506,  1507,    43,  1508,   936,   937,   595,   596,
     597,   598,   182,   599,   579,  1086,   188,   600,   601,   602,
     603,   622,   183,   623,  1141,  1172,  1173,  1174,  1175,   827,
     909,   828,   184,   821,   190,   632,   633,   825,   190,   185,
     190,    44,   752,   753,   754,   755,   756,   757,   758,   759,
     760,   761,  1259,  1260,    45,    46,    47,  1556,  1557,   186,
    1558,    48,   858,   859,   853,   985,   854,   986,  1070,   187,
    1071,    49,   762,    50,  1160,   188,  1161,   570,   571,   872,
     572,   573,  1206,  1290,  1207,  1291,  1320,  1235,   192,  1122,
     574,   575,   576,   577,   193,   578,   579,  1307,  1308,  1532,
     191,   194,   324,   580,   195,   196,   197,   198,   199,    51,
     325,    52,    53,  1537,   200,   201,   202,   890,   203,   204,
     313,   205,   206,   207,   208,  1597,   213,   763,   604,   560,
     326,   316,   951,   215,   216,   953,   317,   605,   217,   956,
     218,   311,   327,  1602,   963,  1246,   312,   606,   322,   516,
     517,   518,   519,   520,   521,   522,   523,   314,   315,   323,
     328,   337,   336,   338,   340,   345,   581,   582,   583,   341,
     343,   329,   349,   352,   354,   364,    54,   330,   357,   516,
     517,   518,   519,   520,   521,   522,   523,   365,   368,   607,
     369,   608,   371,   373,   378,   381,   383,   331,   516,   517,
     518,   519,   520,   521,   522,   523,   389,   390,   392,   396,
     584,   560,   717,   718,   719,   720,   721,   722,   723,   724,
     401,   400,   402,   403,  1675,   404,   410,   411,   412,   419,
     585,   586,   423,   984,   424,   425,   426,   427,   991,   428,
     466,   467,   476,   472,   473,   609,   486,   490,   610,  1022,
    1023,  1024,  1025,  1026,  1027,  1028,  1029,   491,   496,   514,
     544,  1419,   515,   526,   527,  1013,  1080,   532,   528,  1092,
     529,   587,   533,   588,   534,   332,   535,   536,   537,  1035,
     538,   539,   540,   333,   334,  1429,  1430,  1431,  1432,  1433,
    1434,  1435,  1436,  1437,  1438,  1439,  1440,  1441,  1442,  1443,
    1444,  1445,  1446,  1447,  1448,  1449,  1450,  1451,  1452,  1453,
    1454,  1455,  1456,  1457,  1458,  1459,  1460,   676,   677,   678,
     679,   680,   681,   682,   683,   541,   542,   545,   569,   190,
     543,   548,   590,   616,   617,   621,   631,   640,  1316,   641,
     642,   651,   643,   652,   658,   661,   662,   684,   667,   669,
     668,   685,   686,   696,   703,   708,   715,   783,   726,   732,
     736,   746,   749,   751,   769,   771,  -533,  1293,   778,   795,
     817,   801,   796,   802,   832,   834,   833,   836,  1470,   839,
     861,   840,   850,   841,   862,   864,   842,  1137,   843,   844,
     845,   846,   847,   848,   865,   190,   868,   869,   876,   873,
     877,   878,   879,   880,   883,   910,   886,   885,   889,  1146,
     891,   892,   893,   894,   895,   896,   899,   900,   901,   902,
     908,   919,   931,   911,   912,   913,   914,   915,   918,   921,
     922,   923,   924,   939,   925,   940,   932,   935,   941,   942,
     943,   944,   968,   971,   972,   973,   988,   964,   965,   997,
     998,  1002,  1042,  1044,  1245,   966,  1010,  1014,  1015,  1043,
     974,  1046,  1048,  1051,  1073,  1052,  1055,  1467,  1056,  1057,
    1058,  1059,  1062,  1063,  1064,  1065,  1067,  1068,  1128,  1108,
    1078,  1099,  1100,  1101,  1103,  1104,  1032,  1114,  1115,  1116,
    1121,  1123,  1124,  -571,  1152,  1157,  1143,  1147,  1148,  1149,
    1150,  1155,  1195,  1163,  1156,  1188,  1197,  1199,  1200,  1201,
    1241,  1179,  1209,  1203,  1204,  1211,   190,  1213,  1214,  1215,
    1205,  1229,  1216,  1217,  1218,  1219,  1220,  1289,  1226,  1225,
    1227,  1228,  1242,  1261,  1265,  1348,  1252,  1266,  1254,  1267,
    1268,  1256,  1270,  1271,  1272,  1164,  1258,  1273,  1276,  1278,
    1279,  1280,  1281,  1283,  1285,  1297,  1282,  1286,  1318,  1287,
    1288,  1299,  1303,  1301,  1304,  1311,  1317,  1323,  1324,  1326,
    1331,  1376,  1341,  1336,  1330,  1337,  1325,  1338,  1339,  1342,
    1346,  1347,  1374,  1538,  1377,  1380,  1381,  1382,  1383,  1391,
    1497,  1402,  1397,  1403,  1404,  1405,  1414,  1463,  1409,  1410,
    1411,  1415,  1420,  1416,  1498,  1421,  1422,  1423,  1424,  1425,
    1426,  1427,  1428,  1468,  1471,  1475,  1494,  1499,  1504,  1372,
    1505,  1510,  1511,  1513,  1643,  1501,  1523,  1524,  1525,  1526,
    1529,  1531,  1528,  1551,  1541,  1569,  1533,  1514,  1547,  1571,
    1573,  1565,  1534,  1579,  1519,  1520,  1572,  1535,  1522,  1580,
    1581,  1106,  1584,  1585,  1586,  1587,  1589,  1590,  1536,  1591,
    1539,  1592,  1595,  1598,  1599,  1600,  1601,  1568,  1370,  1542,
    1544,  1603,  1373,  1610,  1545,  1611,  1546,  1570,  1612,  1596,
    1235,  1613,  1593,  1628,  1614,  1618,  1622,  1624,  1625,  1626,
    1627,  1630,  1629,  1638,  1639,  1641,  1642,  1644,  1645,  1646,
    1659,  1647,  1648,  1631,  1650,  1651,  1657,  1658,  1649,  1661,
    1663,  1662,  1664,  1665,  1667,   132,  1371,  1668,  1669,  1670,
    1671,  1672,  1673,  1674,  1679,  1212,  1677,  1678,  1680,  1684,
    1685,  1687,  1688,  1689,  1692,  1693,  1694,  1695,   745,   996,
    1007,   248,   611,  1632,  1294,  1240,  1496,  1469,  1633,   190,
    1300,  1495,  1634,  1635,  1636,  1019,   970,  1244,   750,  1637,
     167,  1248,  1407,   620,  1652,  1178,  1050,  1274,   952,  1082,
     657,     0,     0,     0,     0,  1069,     0,     0,     0,     0,
       0,     0,     0,  1527,     0,     0,     0,     0,     0,     0,
       0,     0,  1653,     0,     0,     0,     0,     0,     0,  1683,
       0,     0,     0,     0,     0,     0,     0,     0,  1656,     0,
    1660,     0,  1691,     0,     0,  1676,     0,     0,  1681,     0,
       0,     0,     0,  1682,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1686,     0,  1690,     0,     0,     0,     0,
    1567,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   399,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1655,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1666,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1640,     0,     0,     0,   190,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1654
};

static const yytype_int16 yycheck[] =
{
     126,   404,   189,   500,   347,   596,   884,   257,   351,   951,
     353,  1130,     4,     4,    30,    30,     4,     6,    30,     6,
       6,   963,     6,     4,   367,     4,     6,   370,     6,     6,
       6,     6,     6,     6,    30,     6,     6,   380,     6,     4,
      30,    58,    30,    28,    58,    28,    41,    96,    96,    96,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      58,   159,    71,    96,    58,    96,    96,    30,    21,    28,
      96,    96,   175,    31,    96,    96,    71,    96,    91,   175,
     180,     0,    19,     1,    30,    96,    96,    48,     4,     7,
      96,     9,    10,    11,    12,    13,    14,    96,    16,    17,
      18,    19,    20,    49,    50,    23,    24,   166,   126,     4,
       8,    29,   294,    31,     4,   297,    76,    77,    78,    79,
     220,   221,   222,   223,    42,    43,   469,    48,  1247,    45,
     533,  1170,    96,   125,    55,    30,    54,   104,  1080,  1178,
      96,   108,    60,    96,   294,  1087,    96,    96,    96,   167,
    1092,    69,   294,   294,    36,    37,   297,    75,   100,   101,
     174,   175,    80,   660,   567,   174,   294,   664,   296,   666,
      52,   166,   167,  1064,   233,    93,  1067,    59,    96,    97,
     294,   166,   296,   294,   166,   167,     6,   175,   291,   107,
       3,     4,   110,     6,   176,   291,   157,   158,   156,   294,
     188,   296,    94,   217,   218,   219,    76,    77,    78,    79,
     219,   224,   225,   226,   151,   152,   163,   164,   165,    98,
      99,   167,   169,   102,   103,     9,    28,     3,     4,   147,
       6,  1422,  1423,  1424,    36,    37,  1427,    39,    76,    77,
      78,    79,   160,   161,   162,    47,   295,   295,   295,   167,
       6,     3,     4,   270,     6,   295,   270,   216,     6,   177,
     386,   179,   295,     6,   295,   295,     6,   283,   283,   295,
     295,   283,   270,   295,   295,   289,   295,   271,     6,   294,
     296,     6,   294,   292,   295,   295,   294,   283,   167,   295,
     275,   273,   277,   283,   288,   698,   295,   215,   294,   217,
     218,   286,   287,   286,  1246,   187,     6,   299,   299,   297,
     299,   293,   299,   299,   298,   441,   297,   297,   297,   445,
     283,   299,   299,   299,   299,   299,   299,   298,   298,   294,
     298,   295,     6,   279,   280,   281,   297,   283,   298,   295,
     300,   287,   144,   145,   687,   295,   295,   295,     6,    81,
      82,    83,    84,   104,   294,     6,   296,   108,   109,     6,
      76,    77,    78,    79,   282,   167,    72,    73,    74,   619,
     620,   170,   171,   499,   500,    91,  1318,   503,   220,   221,
     222,   223,   294,   509,   296,   172,   173,   189,   190,   191,
     192,   193,   194,   195,   196,   112,   113,   114,   115,   116,
     117,     7,     6,     9,    10,    11,    12,    13,    14,     6,
      16,    17,    18,    19,    20,   148,   149,    23,    24,    36,
      37,   547,    39,    29,   228,    31,   230,     6,   298,   555,
     300,     6,   234,   184,     3,   167,    42,    43,     3,   565,
     566,    87,    88,    89,    90,   294,     6,   296,    54,   181,
     167,     6,     3,     4,    60,     6,     6,  1576,   955,   150,
     151,   152,   153,    69,   294,   591,   296,     6,   655,    75,
       3,     4,   274,     6,    80,     6,   278,    87,    88,    89,
      90,   283,   284,   285,   286,     3,     4,    93,     6,     6,
      96,    97,    30,   118,   119,    33,    34,     4,    36,    37,
     232,   107,     3,     4,   110,     6,   120,   121,    46,    47,
      48,    49,     4,    51,    52,   296,   297,    55,    56,    57,
      58,   294,    95,   296,  1021,    87,    88,    89,    90,   294,
     780,   296,     4,   659,   660,    98,    99,   663,   664,     4,
     666,   147,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   154,   155,   160,   161,   162,     3,     4,     4,
       6,   167,   688,   689,   294,   294,   296,   296,   294,     4,
     296,   177,   235,   179,   294,   297,   296,    33,    34,   705,
      36,    37,   294,   294,   296,   296,   296,   297,     6,   992,
      46,    47,    48,    49,     4,    51,    52,  1227,  1228,  1477,
     294,   294,     1,    59,   294,   294,   294,   294,   294,   215,
       9,   217,   218,    25,   294,   294,   294,   743,   294,   294,
       6,   294,   294,   294,   294,  1567,   294,   290,   166,   167,
      29,     6,   819,   294,   294,   822,     6,   175,   294,   826,
     294,   294,    41,    25,   831,  1142,   294,   185,    25,    61,
      62,    63,    64,    65,    66,    67,    68,   294,   294,     4,
      59,    20,     6,    19,    25,    31,   122,   123,   124,    10,
      42,    70,    43,    54,    69,    75,   282,    76,    93,    61,
      62,    63,    64,    65,    66,    67,    68,    80,    97,   227,
     297,   229,   147,   107,   110,   179,   215,    96,    61,    62,
      63,    64,    65,    66,    67,    68,   217,    28,   218,     4,
     166,   167,   197,   198,   199,   200,   201,   202,   203,   204,
      48,   282,   294,     6,  1666,     6,     6,     6,   294,   162,
     186,   187,    22,   859,     4,     6,     4,     4,   864,     4,
       4,     4,     4,   297,   297,   283,     4,   294,   286,    61,
      62,    63,    64,    65,    66,    67,    68,     4,     4,    60,
     296,  1352,     6,   298,   298,   891,   953,    27,   298,   956,
     298,   227,     6,   229,     4,   174,     4,     4,     4,   905,
       4,     4,     4,   182,   183,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   247,   248,   249,   250,
     251,   252,   253,   254,   255,   256,   257,   258,   259,   260,
     261,   262,   263,   264,   265,   266,   267,   535,   536,   537,
     538,   539,   540,   541,   542,     4,     4,    27,   297,   955,
       6,     4,    28,     4,     4,     4,     4,     4,  1241,     4,
       4,     4,   111,     4,     4,     4,     4,   294,    25,     6,
      25,    76,     6,     6,     6,     4,     6,   296,     6,     6,
       6,     6,     6,     6,     6,     6,     4,  1210,     4,     4,
     296,     4,     6,     4,     6,   294,     6,     3,  1375,     4,
       4,   294,   175,   294,     4,     4,   294,  1013,   294,   294,
     294,   294,   294,   294,     4,  1021,     4,    71,     4,   297,
       4,     4,     4,     4,     4,    92,     4,     6,     4,  1035,
       4,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,   297,   105,     6,     6,     6,     6,     4,     4,     4,
     298,     4,     6,     4,     6,     4,   294,   294,     4,     4,
       4,     4,   146,     4,     6,   294,     4,    26,    26,     6,
       4,     4,     4,     4,  1141,    25,    53,     6,     6,     6,
      28,     4,     4,     4,   296,   231,     6,  1370,     6,     6,
       6,     6,     4,     4,     4,     4,     4,     4,   298,   178,
       6,     6,     6,     6,     6,     6,    53,     6,     6,     6,
       6,     6,     4,     4,   294,   104,     6,     6,     6,     6,
       4,     4,   294,     4,     6,     4,     4,     4,     4,     4,
    1136,   166,     6,    27,    27,     4,  1142,     4,     4,     4,
      26,   297,     6,     6,     6,     4,     4,   166,     4,     6,
       4,     4,   146,     4,     6,    27,   297,     6,   297,     6,
       6,   298,     6,     6,     6,   105,   105,     6,     6,     6,
       6,     6,     6,     4,     6,   296,    56,     6,  1245,     6,
       6,     6,     6,   296,   296,     6,     6,     4,     4,     4,
       4,   268,     4,     6,   294,     6,   297,     6,     6,     6,
       6,     6,     6,  1486,     4,   298,   298,   298,     4,     4,
     298,     4,     6,     4,     4,     4,   294,     3,     6,     6,
       4,   294,     4,     6,   298,     6,     6,     6,     6,     6,
       6,     6,     6,     6,     6,     4,     6,     4,     6,   175,
       6,     6,     4,     4,  1621,   297,     4,     4,     4,     4,
     296,     4,     6,     4,     6,   126,   298,    28,   106,     6,
       6,   175,   298,     4,    28,    28,    25,   298,    28,     4,
       6,   970,     6,     6,     6,   175,     4,     4,   298,     4,
     297,     4,     6,   276,    26,     6,     6,   296,  1294,   297,
     297,  1574,  1298,     4,   297,     4,   298,   298,     4,   297,
     297,     4,   175,    25,     6,     6,     6,     6,    26,     6,
       6,   298,   297,     6,     6,     6,     6,     6,     4,    27,
      27,     6,     6,   298,     6,     6,     4,     6,    26,     6,
       6,    26,     4,     4,     4,    29,  1296,     6,     6,    27,
       6,     6,     4,     4,    27,  1110,     6,     6,     6,     6,
       6,     6,     6,     6,     6,     6,     6,     6,   593,   866,
     881,   164,   459,   298,  1210,  1131,  1401,  1373,   298,  1375,
    1219,  1400,   298,   298,   298,   897,   838,  1140,   598,   298,
      95,  1145,  1329,   473,   297,  1068,   918,  1177,   820,   954,
     501,    -1,    -1,    -1,    -1,   944,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1470,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   297,    -1,    -1,    -1,    -1,    -1,    -1,   269,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   298,    -1,
     298,    -1,   272,    -1,    -1,   296,    -1,    -1,   298,    -1,
      -1,    -1,    -1,   298,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   298,    -1,   296,    -1,    -1,    -1,    -1,
    1527,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   306,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1643,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1655,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1617,    -1,    -1,    -1,  1621,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1640
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   159,   302,   303,   304,     0,     8,   305,     4,   170,
     171,     1,   306,   294,   294,   294,     7,     9,    10,    11,
      12,    13,    14,    16,    17,    18,    19,    20,    23,    24,
      29,    31,    42,    43,    54,    60,    69,    75,    80,    93,
      96,    97,   107,   110,   147,   160,   161,   162,   167,   177,
     179,   215,   217,   218,   282,   307,   308,   309,   311,   312,
     314,   316,   318,   319,   336,   337,   338,   339,   341,   344,
     346,   347,   351,   352,   380,   390,   392,   401,   402,   404,
     405,   426,   427,   439,   440,   441,   481,   482,   546,   584,
     586,   587,   604,   605,   606,   607,   622,   623,   648,   649,
     666,   667,   682,   683,   695,   696,   731,   732,   744,   745,
     764,   765,   775,   776,   797,   798,   817,   818,   310,     6,
     313,    94,   315,   317,   340,   342,   345,     6,     6,   172,
     173,   393,   393,   381,     6,     6,     6,     6,     6,     6,
       6,     6,     6,     9,     6,     6,     6,     6,     3,     3,
     320,   442,     6,     6,     6,     6,     6,   348,   353,     6,
     406,   428,   444,   483,   547,   588,   608,   608,   624,   650,
     668,   684,   697,   735,   747,   767,   778,   295,   801,   820,
       4,   294,     4,    95,     4,     4,     4,     4,   297,   420,
     423,   294,     6,     4,   294,   294,   294,   294,   294,   294,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   321,
       4,   294,   443,   294,   733,   294,   294,   294,   294,    21,
      96,   349,   350,    96,   295,   354,   379,   391,    96,   295,
     407,   425,    96,   295,   429,    96,   295,   445,   446,   447,
     480,    96,   295,   484,   485,   486,   544,    96,   485,   548,
     585,    96,   295,   589,   590,   603,    96,   295,   609,   610,
     615,   621,    96,   620,    96,   295,   625,   626,   647,    96,
     295,   651,   652,   665,    96,   295,   669,    96,   295,   685,
     694,    96,   295,   698,   699,   721,    96,   295,   734,   736,
      96,   295,   746,   748,   749,    96,   295,   766,   768,   769,
      96,   295,   777,   779,   782,   802,   800,    96,   295,   819,
     821,   294,   294,     6,   294,   294,     6,     6,     6,   299,
     421,   423,    25,     4,     1,     9,    29,    41,    59,    70,
      76,    96,   174,   182,   183,   322,     6,    20,    19,   355,
      25,    10,   408,    42,   430,    31,   448,   450,   449,    43,
     487,   497,    54,   549,    69,   591,   592,    93,    76,    77,
      78,    79,    91,   611,    75,    80,   627,   628,    97,   297,
     654,   147,   670,   107,   104,   108,   109,   184,   110,   700,
     704,   179,   737,   215,    28,   216,    30,   283,   762,   217,
      28,   772,   218,    28,   286,   785,     4,    96,   799,   801,
     282,    48,   294,     6,     6,     6,   299,     6,   299,   422,
       6,     6,   294,   323,   329,   326,   331,   327,   324,   162,
     328,   325,   330,    22,     4,     6,     4,     4,     4,   294,
     296,   403,   451,   452,   453,   455,   458,   460,   462,   465,
     467,   471,   472,   475,   476,   478,   479,     4,   299,     4,
      45,   488,   294,   296,   403,   498,   513,   515,   294,   296,
     403,   550,   551,   565,   567,   569,     4,     4,   593,   594,
     612,   613,   297,   297,   616,   618,     4,   294,   296,   403,
     630,   653,   294,   296,   403,   655,     4,   686,   688,   689,
     294,     4,   294,   296,   403,   705,     4,   750,   752,   423,
     763,   761,   770,    30,   283,   294,   773,   783,   780,    30,
     283,   294,   786,   803,    60,     6,    61,    62,    63,    64,
      65,    66,    67,    68,   343,   343,   298,   298,   298,   298,
     294,   421,    27,     6,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     6,   296,    27,   409,   431,     4,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   166,
     167,   176,   273,   293,     4,   423,   461,   423,   489,   297,
      33,    34,    36,    37,    46,    47,    48,    49,    51,    52,
      59,   122,   123,   124,   166,   186,   187,   227,   229,   521,
      28,   517,    30,    33,    34,    46,    47,    48,    49,    51,
      55,    56,    57,    58,   166,   175,   185,   227,   229,   283,
     286,   521,   294,   296,   403,   595,     4,     4,   611,   614,
     614,     4,   294,   296,   619,    81,    82,    83,    84,   181,
     232,     4,    98,    99,   102,   103,   664,   148,   149,   672,
       4,     4,     4,   111,   701,   112,   113,   114,   115,   116,
     117,     4,     4,   758,   423,   420,   294,   762,     4,   423,
     774,     4,     4,   423,   787,   804,   822,    25,    25,     6,
     343,   163,   164,   165,   169,   332,   332,   332,   332,   332,
     332,   332,   332,   332,   294,    76,     6,   410,   423,   432,
     456,   220,   221,   222,   223,   459,     6,   454,   423,   473,
     468,   477,   463,     6,   423,   423,   343,   491,     4,   180,
     220,   221,   222,   223,   512,     6,   501,   197,   198,   199,
     200,   201,   202,   203,   204,   376,     6,    91,   224,   225,
     226,   576,     6,   502,   516,   506,     6,   509,   503,   504,
     500,   499,   518,   423,   557,   459,     6,   560,   376,     6,
     576,     6,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   235,   290,   545,   566,   568,   570,   561,     6,
     553,     6,   555,   558,   552,    41,    71,   166,     4,   298,
     300,   611,   298,   296,   617,    87,    88,    89,    90,   631,
     632,   633,   634,   635,   636,     4,     6,   656,   100,   101,
     671,     4,     4,   104,   108,   690,   118,   119,   702,   706,
     707,   708,   709,   710,   711,   738,   751,   296,   759,   421,
     771,   423,   420,   784,   781,   423,   420,   294,   296,   805,
     813,   420,     6,     6,   294,   382,     3,   334,   333,     4,
     294,   294,   294,   294,   294,   294,   294,   294,   294,   356,
     175,   394,   395,   294,   296,   403,   411,   418,   423,   423,
     433,     4,     4,   343,     4,     4,   469,   470,     4,    71,
     464,   466,   423,   297,   492,   490,     4,     4,     4,     4,
       4,   510,   511,     4,   505,     6,     4,   522,   525,     4,
     423,     4,     4,     4,     4,     4,     4,   572,   573,     4,
       4,     4,     4,   577,   580,   597,   601,   596,     4,   611,
      92,     6,     6,     6,     6,     4,   638,   642,     4,   297,
     637,     4,   298,     4,     6,     6,   150,   151,   152,   153,
     673,   105,   294,   691,   693,   294,   120,   121,   703,     4,
       4,     4,     4,     4,     4,   739,   753,    71,   174,   219,
     292,   421,   773,   421,   788,   792,   421,    28,   166,   275,
     277,   286,   287,   421,    26,    26,    25,   383,   146,   575,
     575,     4,     6,   294,    28,   397,    30,    49,    50,   279,
     280,   281,   283,   287,   423,   294,   296,   434,     4,   457,
       6,   423,   474,     3,     4,     6,   470,     6,     4,     4,
     125,   299,     4,   507,     3,     4,     6,   511,   522,   526,
      53,   523,   514,   423,     6,     6,     3,     4,     6,   573,
     554,   556,    61,    62,    63,    64,    65,    66,    67,    68,
     562,   581,    53,   578,     4,   423,   598,    72,    73,    74,
     602,   599,     4,     6,     4,   629,     4,   639,     4,   643,
     629,     4,   231,   646,   657,     6,     6,     6,     6,     6,
     674,   687,     4,     4,     4,     4,   717,     4,     4,   717,
     294,   296,   740,   296,   754,   755,   756,   760,     6,   422,
     421,   296,   786,   789,   790,   795,   296,   420,   791,   793,
     794,   796,   421,   806,   814,   810,   808,   809,   422,     6,
       6,     6,   385,     6,     6,   335,   335,   357,   178,   396,
     398,   412,   419,   415,     6,     6,     6,   413,   416,   166,
     233,     6,   343,     6,     4,   493,   495,   494,   298,   722,
     537,   524,    36,    37,    39,   519,   520,   423,   571,   563,
     564,   420,   559,     6,   583,   579,   423,     6,     6,     6,
       4,   600,   294,   640,   644,     4,     6,   104,   658,   661,
     294,   296,   675,     4,   105,   664,    87,    88,    89,    90,
     719,   720,    87,    88,    89,    90,   718,   664,   719,   166,
      58,   174,   175,   217,   218,   219,   270,   289,     4,   422,
     175,   291,   175,   291,   422,   294,   422,     4,   815,     4,
       4,     4,   294,    27,    27,    26,   294,   296,   386,     6,
     358,     4,   400,     4,     4,     4,     6,     6,     6,     4,
       4,   435,   228,   230,   438,     6,     4,     4,     4,   297,
     508,   723,    48,   157,   158,   297,   534,   538,   539,   541,
     525,   423,   146,   574,   577,   421,   420,   542,   580,     3,
       4,     6,   297,   641,   297,   645,   298,   662,   105,   154,
     155,     4,   692,   712,   714,     6,     6,     6,     6,   720,
       6,     6,     6,     6,   712,   741,     6,   757,     6,     6,
       6,     6,    56,     4,   816,     6,     6,     6,     6,   166,
     294,   296,   359,   403,   478,   399,   400,   296,   424,     6,
     424,   296,   436,     6,   296,   403,   496,   496,   496,   724,
     726,     6,   540,     6,   299,   527,   343,     6,   421,   422,
     296,   534,   543,     4,     4,   297,     4,   659,   676,   677,
     294,     4,    19,   151,   152,   715,     6,     6,     6,     6,
     742,     4,     6,     3,     4,     6,     6,     6,    27,   384,
     387,    28,    47,   144,   145,   189,   190,   191,   192,   193,
     194,   195,   196,   234,   274,   278,   283,   284,   285,   286,
     423,   399,   175,   423,     6,   414,   268,     4,   437,   126,
     298,   298,   298,     4,    36,    37,    52,    59,   187,   727,
     730,     4,     6,   299,     6,   299,   530,     6,   422,    48,
      55,   582,     4,     4,     4,     4,   678,   678,   713,     6,
       6,     4,   743,   807,   294,   294,     6,   388,   362,   376,
       4,     6,     6,     6,     6,     6,     6,     6,     6,   236,
     237,   238,   239,   240,   241,   242,   243,   244,   245,   246,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   375,   361,     3,   364,   360,   366,   343,     6,   423,
     420,     6,     3,     4,     6,     4,   729,   728,     6,   298,
       6,   298,     6,   298,     6,   298,     4,    30,   175,   188,
     528,   529,   531,   534,     6,   545,   530,   298,   298,     4,
     660,   297,   679,   716,     6,     6,     3,     4,     6,   811,
       6,     4,   389,     4,    28,   377,   377,   377,   377,    28,
      28,   377,    28,     4,     4,     4,     4,   421,     6,   296,
     725,     4,   522,   298,   298,   298,   298,    25,   343,   297,
     536,     6,   297,   535,   297,   297,   298,   106,   663,    31,
     156,     4,    58,   271,   288,   812,     3,     4,     6,   363,
     378,   367,   368,   369,   365,   175,   371,   421,   296,   126,
     298,     6,    25,     6,     4,    30,   533,     6,   299,     4,
       4,     6,   681,   680,     6,     6,     6,   175,   370,     4,
       4,     4,     4,   175,   372,     6,   297,   422,   276,    26,
       6,     6,    25,   343,   532,   534,     6,   299,     6,   299,
       4,     4,     4,     4,     6,    58,   270,   373,     6,    58,
     270,   374,     6,   417,     6,    26,     6,     6,    25,   297,
     298,   298,   298,   298,   298,   298,   298,   298,     6,     6,
     423,     6,     6,   420,     6,     4,    27,     6,     6,    26,
       6,     6,   297,   297,   423,   421,   298,     4,     6,    27,
     298,     6,    26,     6,     4,     4,   421,     4,     6,     6,
      27,     6,     6,     4,     4,   422,   296,     6,     6,    27,
       6,   298,   298,   269,     6,     6,   298,     6,     6,     6,
     296,   272,     6,     6,     6,     6
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   301,   302,   303,   304,   303,   305,   305,   305,   306,
     306,   306,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   307,   307,   307,   307,   307,   307,
     307,   307,   307,   307,   308,   308,   308,   308,   308,   308,
     308,   308,   308,   308,   308,   308,   308,   308,   308,   308,
     308,   310,   309,   311,   313,   312,   315,   314,   317,   316,
     318,   320,   319,   321,   321,   323,   322,   324,   322,   325,
     322,   326,   322,   327,   322,   328,   322,   329,   322,   330,
     322,   331,   322,   322,   333,   332,   334,   332,   332,   332,
     332,   335,   335,   336,   337,   338,   340,   339,   342,   341,
     343,   343,   343,   343,   343,   343,   343,   343,   345,   344,
     346,   347,   348,   348,   349,   350,   351,   352,   353,   353,
     355,   356,   357,   354,   358,   358,   359,   359,   359,   359,
     360,   359,   361,   359,   359,   359,   362,   363,   359,   364,
     365,   359,   366,   359,   359,   359,   359,   359,   359,   367,
     359,   368,   359,   359,   369,   359,   359,   370,   370,   371,
     371,   372,   372,   373,   373,   373,   374,   374,   374,   375,
     375,   375,   375,   375,   375,   375,   375,   375,   375,   375,
     375,   375,   375,   375,   375,   375,   375,   375,   375,   375,
     375,   375,   375,   375,   375,   375,   375,   375,   375,   375,
     375,   376,   376,   376,   376,   376,   376,   376,   376,   377,
     378,   377,   379,   381,   382,   380,   383,   383,   384,   384,
     385,   385,   387,   386,   388,   388,   389,   389,   389,   391,
     390,   392,   393,   393,   394,   395,   395,   396,   396,   397,
     398,   397,   399,   399,   400,   401,   402,   403,   404,   405,
     406,   406,   408,   409,   407,   410,   410,   412,   411,   413,
     414,   411,   415,   411,   416,   417,   411,   411,   411,   418,
     418,   418,   419,   418,   420,   421,   422,   422,   423,   423,
     423,   423,   424,   424,   425,   426,   427,   428,   428,   430,
     431,   429,   432,   432,   433,   433,   435,   434,   434,   436,
     436,   437,   437,   437,   438,   438,   439,   440,   441,   442,
     442,   443,   444,   444,   445,   446,   448,   447,   449,   449,
     449,   450,   450,   451,   451,   451,   451,   451,   451,   451,
     451,   451,   451,   451,   451,   452,   454,   453,   456,   455,
     457,   457,   458,   459,   459,   459,   459,   460,   460,   461,
     461,   463,   462,   464,   464,   466,   465,   468,   467,   469,
     469,   470,   470,   470,   471,   473,   472,   474,   474,   475,
     475,   475,   477,   476,   478,   478,   478,   479,   480,   481,
     482,   483,   483,   484,   485,   487,   486,   489,   488,   490,
     488,   491,   491,   493,   492,   494,   492,   495,   492,   496,
     496,   496,   497,   497,   499,   498,   498,   498,   500,   498,
     501,   498,   498,   498,   498,   498,   498,   498,   502,   498,
     498,   503,   498,   504,   505,   498,   506,   507,   508,   498,
     509,   498,   498,   510,   510,   511,   511,   511,   512,   512,
     512,   512,   512,   514,   513,   516,   515,   517,   518,   517,
     519,   519,   520,   520,   520,   521,   521,   521,   522,   522,
     524,   523,   526,   527,   525,   528,   529,   530,   530,   531,
     531,   531,   531,   531,   531,   531,   531,   531,   531,   532,
     531,   533,   531,   531,   534,   534,   534,   534,   534,   534,
     534,   534,   535,   535,   535,   535,   536,   537,   537,   538,
     538,   539,   540,   539,   541,   542,   542,   543,   543,   544,
     545,   545,   545,   545,   545,   545,   545,   545,   545,   545,
     545,   545,   546,   547,   547,   548,   549,   549,   550,   550,
     550,   550,   551,   552,   551,   553,   554,   551,   551,   551,
     555,   556,   551,   557,   551,   558,   559,   551,   551,   551,
     551,   560,   551,   551,   551,   551,   551,   551,   561,   551,
     551,   562,   562,   562,   562,   562,   562,   562,   562,   562,
     563,   564,   563,   566,   565,   568,   567,   570,   571,   569,
     572,   572,   573,   573,   573,   574,   574,   575,   575,   576,
     576,   576,   576,   577,   577,   579,   578,   581,   582,   580,
     583,   584,   585,   586,   587,   588,   588,   589,   591,   590,
     592,   592,   593,   594,   594,   595,   596,   595,   597,   595,
     595,   598,   598,   599,   599,   600,   600,   600,   601,   601,
     602,   602,   602,   603,   604,   605,   606,   607,   608,   608,
     609,   609,   610,   612,   611,   613,   611,   611,   611,   614,
     614,   614,   616,   615,   617,   617,   618,   618,   619,   619,
     619,   619,   620,   621,   622,   623,   624,   624,   625,   627,
     626,   628,   628,   629,   629,   631,   630,   632,   630,   633,
     630,   634,   630,   635,   630,   636,   630,   630,   637,   637,
     637,   638,   638,   640,   639,   641,   641,   641,   641,   642,
     642,   644,   643,   645,   645,   645,   645,   646,   646,   647,
     648,   649,   650,   650,   651,   653,   652,   654,   654,   655,
     655,   655,   656,   657,   655,   655,   659,   660,   658,   661,
     662,   661,   663,   663,   664,   664,   665,   666,   667,   668,
     668,   670,   671,   669,   672,   672,   673,   673,   673,   673,
     674,   674,   676,   675,   677,   675,   678,   678,   680,   679,
     681,   679,   682,   683,   684,   684,   686,   687,   685,   688,
     685,   689,   685,   685,   691,   692,   690,   693,   690,   694,
     695,   696,   697,   697,   698,   700,   699,   701,   701,   702,
     702,   702,   703,   703,   703,   704,   704,   706,   705,   707,
     705,   708,   705,   709,   705,   710,   705,   711,   705,   705,
     713,   712,   714,   714,   715,   715,   716,   716,   717,   717,
     718,   718,   718,   718,   719,   719,   720,   720,   720,   720,
     721,   722,   722,   724,   723,   725,   725,   726,   726,   728,
     727,   729,   727,   730,   730,   730,   730,   731,   732,   733,
     733,   734,   735,   735,   737,   738,   736,   739,   739,   741,
     740,   742,   742,   743,   743,   743,   744,   745,   746,   747,
     747,   748,   750,   751,   749,   752,   749,   753,   753,   754,
     754,   754,   754,   755,   757,   756,   756,   756,   756,   756,
     758,   758,   760,   759,   759,   759,   759,   761,   761,   762,
     763,   762,   764,   765,   766,   767,   767,   768,   770,   771,
     769,   772,   772,   773,   774,   773,   775,   776,   777,   778,
     778,   779,   780,   781,   779,   783,   784,   782,   785,   785,
     786,   787,   786,   788,   788,   789,   789,   790,   791,   792,
     792,   793,   793,   794,   795,   796,   797,   798,   799,   800,
     800,   802,   803,   801,   804,   804,   805,   806,   807,   805,
     808,   805,   809,   805,   810,   805,   805,   811,   811,   812,
     812,   812,   814,   813,   815,   815,   816,   816,   816,   817,
     818,   819,   820,   820,   822,   821
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     4,     0,     0,     4,     0,     3,     3,     0,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     0,     4,     2,     0,     4,     0,     4,     0,     4,
       1,     0,     5,     0,     2,     0,     5,     0,     5,     0,
       5,     0,     5,     0,     5,     0,     5,     0,     5,     0,
       5,     0,     5,     2,     0,     4,     0,     4,     1,     2,
       2,     0,     1,     5,     3,     3,     0,    14,     0,    14,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     6,
       3,     2,     0,     2,     5,     2,     3,     3,     0,     2,
       0,     0,     0,    10,     0,     2,     2,     1,     3,     3,
       0,     4,     0,     4,     3,     2,     0,     0,     9,     0,
       0,    11,     0,     9,     3,     4,     4,     4,     4,     0,
       6,     0,     6,     4,     0,     6,     3,     0,     2,     0,
       2,     0,     2,     0,     2,     2,     0,     2,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     0,
       0,     3,     2,     0,     0,    11,     0,     5,     0,     3,
       0,     2,     0,     4,     0,     2,     2,     2,     2,     0,
       9,     2,     1,     1,     2,     0,     3,     0,     1,     0,
       0,     4,     0,     2,     1,     8,     1,     2,     3,     3,
       0,     2,     0,     0,     6,     0,     2,     0,     7,     0,
       0,    10,     0,     4,     0,     0,    24,     1,     1,     4,
       4,     6,     0,     4,     1,     1,     0,     2,     4,     4,
       4,     4,     0,     3,     2,     4,     3,     0,     2,     0,
       0,     7,     2,     3,     0,     2,     0,     4,     3,     0,
       2,     2,     2,     2,     1,     1,     3,     3,     3,     0,
       2,     1,     0,     2,     3,     2,     0,     4,     0,     2,
       2,     0,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     4,     0,     5,
       0,     1,     3,     1,     1,     1,     1,     2,     2,     2,
       3,     0,     8,     0,     1,     0,     6,     0,     4,     1,
       2,     2,     2,     2,     2,     0,     6,     1,     2,     3,
       2,     4,     0,     4,     2,     2,     2,     3,     2,     3,
       3,     0,     2,     3,     1,     0,     3,     0,     3,     0,
       6,     0,     2,     0,     6,     0,     6,     0,     6,     0,
       1,     2,     0,     2,     0,     4,     3,     2,     0,     4,
       0,     4,     3,     3,     3,     3,     3,     3,     0,     4,
       1,     0,     4,     0,     0,     5,     0,     0,     0,     8,
       0,     4,     1,     1,     2,     2,     2,     2,     1,     1,
       1,     1,     1,     0,     6,     0,     4,     0,     0,     3,
       0,     3,     1,     1,     1,     1,     1,     1,     1,     2,
       0,     3,     0,     0,     6,     2,     2,     0,     2,     1,
       3,     2,     4,    10,     8,     9,    11,     1,     1,     0,
      10,     0,     4,     1,     4,     4,     4,     4,     5,     5,
       5,     5,     4,     4,     4,     4,     6,     0,     2,     1,
       1,     1,     0,     3,     2,     0,     2,     3,     3,     2,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     3,     0,     2,     3,     0,     2,     1,     1,
       1,     1,     2,     0,     4,     0,     0,     6,     3,     3,
       0,     0,     9,     0,     6,     0,     0,     8,     3,     2,
       3,     0,     4,     3,     3,     3,     3,     3,     0,     4,
       1,     0,     1,     1,     1,     1,     1,     1,     1,     1,
       0,     0,     2,     0,     5,     0,     4,     0,     0,     7,
       1,     2,     2,     2,     2,     0,     3,     0,     3,     1,
       1,     1,     1,     1,     2,     0,     3,     0,     0,     7,
       1,     3,     2,     3,     3,     0,     2,     4,     0,     3,
       0,     2,     1,     0,     2,     3,     0,     4,     0,     4,
       1,     2,     1,     0,     2,     2,     2,     2,     0,     2,
       2,     2,     2,     2,     3,     3,     3,     3,     0,     2,
       1,     1,     4,     0,     3,     0,     6,     4,     4,     1,
       2,     3,     0,     8,     0,     1,     0,     2,     3,     3,
       3,     3,     2,     2,     3,     3,     0,     2,     3,     0,
       3,     0,     2,     0,     1,     0,     5,     0,     4,     0,
       4,     0,     5,     0,     4,     0,     5,     1,     0,     4,
       8,     0,     2,     0,     3,     0,     4,     8,    12,     0,
       2,     0,     3,     0,     4,     8,    12,     0,     2,     2,
       3,     3,     0,     2,     3,     0,     6,     0,     2,     5,
       5,     3,     0,     0,     6,     1,     0,     0,     6,     0,
       0,     3,     0,     2,     1,     1,     2,     4,     3,     0,
       2,     0,     0,     8,     1,     1,     1,     2,     2,     2,
       0,     2,     0,     4,     0,     4,     0,     2,     0,     5,
       0,     5,     3,     3,     0,     2,     0,     0,    10,     0,
       6,     0,     6,     3,     0,     0,     6,     0,     3,     2,
       3,     3,     0,     2,     3,     0,     4,     0,     3,     0,
       1,     1,     0,     1,     1,     0,     2,     0,     7,     0,
       6,     0,     5,     0,     7,     0,     6,     0,     5,     1,
       0,     4,     0,     2,     3,     3,     0,     2,     0,     2,
       2,     2,     2,     2,     1,     2,     3,     3,     3,     3,
       2,     0,     2,     0,     6,     0,     2,     0,     2,     0,
       3,     0,     3,     1,     1,     1,     1,     3,     3,     0,
       1,     2,     0,     2,     0,     0,     7,     0,     2,     0,
       4,     0,     2,     2,     2,     2,     3,     3,     2,     0,
       2,     4,     0,     0,     6,     0,     4,     0,     2,     3,
       3,     1,     1,     3,     0,     4,     2,     2,     2,     2,
       0,     2,     0,     4,     2,     2,     3,     0,     2,     3,
       0,     6,     3,     3,     2,     0,     2,     3,     0,     0,
       6,     0,     2,     3,     0,     6,     3,     3,     2,     0,
       2,     3,     0,     0,     8,     0,     0,     7,     0,     2,
       3,     0,     6,     0,     2,     1,     1,     2,     2,     0,
       2,     1,     1,     2,     3,     3,     4,     3,     2,     0,
       2,     0,     0,     6,     0,     2,     2,     0,     0,     8,
       0,     4,     0,     4,     0,     5,     1,     0,     2,     2,
       2,     2,     0,     4,     0,     2,     2,     2,     2,     3,
       3,     2,     0,     2,     0,     8
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
        yyerror (defData, YY_("syntax error: cannot back up")); \
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
                  Kind, Value, defData); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, DefParser::defrData *defData)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (defData);
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
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, DefParser::defrData *defData)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep, defData);
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
                 int yyrule, DefParser::defrData *defData)
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
                       &yyvsp[(yyi + 1) - (yynrhs)], defData);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule, defData); \
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
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, DefParser::defrData *defData)
{
  YY_USE (yyvaluep);
  YY_USE (defData);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (DefParser::defrData *defData)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

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
      yychar = yylex (&yylval, defData);
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
  case 4: /* $@1: %empty  */
#line 230 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                { defData->dumb_mode = 1; defData->no_num = 1; }
#line 3355 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 5: /* version_stmt: K_VERSION $@1 T_STRING ';'  */
#line 231 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        defData->VersionNum = defrData::convert_defname2num((yyvsp[-1].string));
        if (defData->VersionNum > CURRENT_VERSION) {
          char temp[300];
          sprintf(temp,
          "The execution has been stopped because the DEF parser %.1f does not support DEF file with version %s.\nUpdate your DEF file to version 5.8 or earlier.",
                  CURRENT_VERSION, (yyvsp[-1].string));
          defData->defError(6503, temp);
          return 1;
        }
        if (defData->callbacks->VersionStrCbk) {
          CALLBACK(defData->callbacks->VersionStrCbk, defrVersionStrCbkType, (yyvsp[-1].string));
        } else if (defData->callbacks->VersionCbk) {
            CALLBACK(defData->callbacks->VersionCbk, defrVersionCbkType, defData->VersionNum);
        }
        if (defData->VersionNum > 5.3 && defData->VersionNum < 5.4)
          defData->defIgnoreVersion = 1;
        if (defData->VersionNum < 5.6)     // default to false before 5.6
          defData->names_case_sensitive = defData->session->reader_case_sensitive;
        else
          defData->names_case_sensitive = 1;
        defData->hasVer = 1;
        defData->doneDesign = 0;
    }
#line 3384 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 7: /* case_sens_stmt: K_NAMESCASESENSITIVE K_ON ';'  */
#line 258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.6) {
          defData->names_case_sensitive = 1;
          if (defData->callbacks->CaseSensitiveCbk)
            CALLBACK(defData->callbacks->CaseSensitiveCbk, defrCaseSensitiveCbkType,
                     defData->names_case_sensitive); 
          defData->hasNameCase = 1;
        } else
          if (defData->callbacks->CaseSensitiveCbk) // write error only if cbk is set 
             if (defData->caseSensitiveWarnings++ < defData->settings->CaseSensitiveWarnings)
               defData->defWarning(7011, "The NAMESCASESENSITIVE statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement.");
      }
#line 3401 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 8: /* case_sens_stmt: K_NAMESCASESENSITIVE K_OFF ';'  */
#line 271 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.6) {
          defData->names_case_sensitive = 0;
          if (defData->callbacks->CaseSensitiveCbk)
            CALLBACK(defData->callbacks->CaseSensitiveCbk, defrCaseSensitiveCbkType,
                     defData->names_case_sensitive);
          defData->hasNameCase = 1;
        } else {
          if (defData->callbacks->CaseSensitiveCbk) { // write error only if cbk is set 
            if (defData->caseSensitiveWarnings++ < defData->settings->CaseSensitiveWarnings) {
              defData->defError(6504, "Def parser version 5.7 and later does not support NAMESCASESENSITIVE OFF.\nEither remove this optional construct or set it to ON.");
              CHKERR();
            }
          }
        }
      }
#line 3422 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 51: /* $@2: %empty  */
#line 311 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      {defData->dumb_mode = 1; defData->no_num = 1; }
#line 3428 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 52: /* design_name: K_DESIGN $@2 T_STRING ';'  */
#line 312 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
            if (defData->callbacks->DesignCbk)
              CALLBACK(defData->callbacks->DesignCbk, defrDesignStartCbkType, (yyvsp[-1].string));
            defData->hasDes = 1;
          }
#line 3438 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 53: /* end_design: K_END K_DESIGN  */
#line 319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            defData->doneDesign = 1;
            if (defData->callbacks->DesignEndCbk)
              CALLBACK(defData->callbacks->DesignEndCbk, defrDesignEndCbkType, 0);
            // 11/16/2001 - pcr 408334
            // Return 1 if there is any defData->errors during parsing
            if (defData->errors)
                return 1;

            if (!defData->hasVer) {
              char temp[300];
              sprintf(temp, "No VERSION statement found, using the default value %2g.", defData->VersionNum);
              defData->defWarning(7012, temp);            
            }
            if (!defData->hasNameCase && defData->VersionNum < 5.6)
              defData->defWarning(7013, "The DEF file is invalid if NAMESCASESENSITIVE is undefined.\nNAMESCASESENSITIVE is a mandatory statement in the DEF file with version 5.6 and earlier.\nTo define the NAMESCASESENSITIVE statement, refer to the LEF/DEF 5.5 and earlier Language Reference manual.");
            if (!defData->hasBusBit && defData->VersionNum < 5.6)
              defData->defWarning(7014, "The DEF file is invalid if BUSBITCHARS is undefined.\nBUSBITCHARS is a mandatory statement in the DEF file with version 5.6 and earlier.\nTo define the BUSBITCHARS statement, refer to the LEF/DEF 5.5 and earlier Language Reference manual.");
            if (!defData->hasDivChar && defData->VersionNum < 5.6)
              defData->defWarning(7015, "The DEF file is invalid if DIVIDERCHAR is undefined.\nDIVIDERCHAR is a mandatory statement in the DEF file with version 5.6 and earlier.\nTo define the DIVIDERCHAR statement, refer to the LEF/DEF 5.5 and earlier Language Reference manual.");
            if (!defData->hasDes)
              defData->defWarning(7016, "DESIGN is a mandatory statement in the DEF file. Ensure that it exists in the file.");
          }
#line 3466 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 54: /* $@3: %empty  */
#line 343 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 3472 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 55: /* tech_name: K_TECH $@3 T_STRING ';'  */
#line 344 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->TechnologyCbk)
              CALLBACK(defData->callbacks->TechnologyCbk, defrTechNameCbkType, (yyvsp[-1].string));
          }
#line 3481 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 56: /* $@4: %empty  */
#line 349 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    {defData->dumb_mode = 1; defData->no_num = 1;}
#line 3487 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 57: /* array_name: K_ARRAY $@4 T_STRING ';'  */
#line 350 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->ArrayNameCbk)
              CALLBACK(defData->callbacks->ArrayNameCbk, defrArrayNameCbkType, (yyvsp[-1].string));
          }
#line 3496 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 58: /* $@5: %empty  */
#line 355 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                            { defData->dumb_mode = 1; defData->no_num = 1; }
#line 3502 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 59: /* floorplan_name: K_FLOORPLAN $@5 T_STRING ';'  */
#line 356 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->FloorPlanNameCbk)
              CALLBACK(defData->callbacks->FloorPlanNameCbk, defrFloorPlanNameCbkType, (yyvsp[-1].string));
          }
#line 3511 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 60: /* history: K_HISTORY  */
#line 362 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->HistoryCbk)
              CALLBACK(defData->callbacks->HistoryCbk, defrHistoryCbkType, &defData->History_text[0]);
          }
#line 3520 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 61: /* $@6: %empty  */
#line 368 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PropDefStartCbk)
              CALLBACK(defData->callbacks->PropDefStartCbk, defrPropDefStartCbkType, 0);
          }
#line 3529 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 62: /* prop_def_section: K_PROPERTYDEFINITIONS $@6 property_defs K_END K_PROPERTYDEFINITIONS  */
#line 373 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->PropDefEndCbk)
              CALLBACK(defData->callbacks->PropDefEndCbk, defrPropDefEndCbkType, 0);
            defData->real_num = 0;     // just want to make sure it is reset 
          }
#line 3539 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 64: /* property_defs: property_defs property_def  */
#line 381 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { }
#line 3545 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 65: /* $@7: %empty  */
#line 383 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       {defData->dumb_mode = 1; defData->no_num = 1; defData->Prop.clear(); }
#line 3551 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 66: /* property_def: K_DESIGN $@7 T_STRING property_type_and_val ';'  */
#line 385 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("design", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->DesignProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3563 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 67: /* $@8: %empty  */
#line 392 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3569 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 68: /* property_def: K_NET $@8 T_STRING property_type_and_val ';'  */
#line 394 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("net", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->NetProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3581 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 69: /* $@9: %empty  */
#line 401 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                 { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3587 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 70: /* property_def: K_SNET $@9 T_STRING property_type_and_val ';'  */
#line 403 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("specialnet", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->SNetProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3599 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 71: /* $@10: %empty  */
#line 410 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                   { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3605 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 72: /* property_def: K_REGION $@10 T_STRING property_type_and_val ';'  */
#line 412 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("region", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->RegionProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3617 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 73: /* $@11: %empty  */
#line 419 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3623 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 74: /* property_def: K_GROUP $@11 T_STRING property_type_and_val ';'  */
#line 421 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("group", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->GroupProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3635 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 75: /* $@12: %empty  */
#line 428 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3641 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 76: /* property_def: K_COMPONENT $@12 T_STRING property_type_and_val ';'  */
#line 430 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("component", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->CompProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3653 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 77: /* $@13: %empty  */
#line 437 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3659 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 78: /* property_def: K_ROW $@13 T_STRING property_type_and_val ';'  */
#line 439 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("row", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->RowProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3671 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 79: /* $@14: %empty  */
#line 448 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3677 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 80: /* property_def: K_COMPONENTPIN $@14 T_STRING property_type_and_val ';'  */
#line 450 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("componentpin", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->CompPinProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3689 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 81: /* $@15: %empty  */
#line 458 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3695 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 82: /* property_def: K_NONDEFAULTRULE $@15 T_STRING property_type_and_val ';'  */
#line 460 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->VersionNum < 5.6) {
                if (defData->nonDefaultWarnings++ < defData->settings->NonDefaultWarnings) {
                  defData->defMsg = (char*)malloc(1000); 
                  sprintf (defData->defMsg,
                     "The NONDEFAULTRULE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6505, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              } else {
                if (defData->callbacks->PropCbk) {
                  defData->Prop.setPropType("nondefaultrule", (yyvsp[-2].string));
                  CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
                }
                defData->session->NDefProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
              }
            }
#line 3718 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 83: /* property_def: error ';'  */
#line 478 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    { yyerrok; yyclearin;}
#line 3724 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 84: /* $@16: %empty  */
#line 480 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                 { defData->real_num = 0; }
#line 3730 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 85: /* property_type_and_val: K_INTEGER $@16 opt_range opt_num_val  */
#line 481 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropInteger();
              defData->defPropDefType = 'I';
            }
#line 3739 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 86: /* $@17: %empty  */
#line 485 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                 { defData->real_num = 1; }
#line 3745 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 87: /* property_type_and_val: K_REAL $@17 opt_range opt_num_val  */
#line 486 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropReal();
              defData->defPropDefType = 'R';
              defData->real_num = 0;
            }
#line 3755 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 88: /* property_type_and_val: K_STRING  */
#line 492 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropString();
              defData->defPropDefType = 'S';
            }
#line 3764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 89: /* property_type_and_val: K_STRING QSTRING  */
#line 497 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropQString((yyvsp[0].string));
              defData->defPropDefType = 'Q';
            }
#line 3773 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 90: /* property_type_and_val: K_NAMEMAPSTRING T_STRING  */
#line 502 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropNameMapString((yyvsp[0].string));
              defData->defPropDefType = 'S';
            }
#line 3782 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 92: /* opt_num_val: NUMBER  */
#line 509 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->PropCbk) defData->Prop.setNumber((yyvsp[0].dval)); }
#line 3788 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 93: /* units: K_UNITS K_DISTANCE K_MICRONS NUMBER ';'  */
#line 512 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->UnitsCbk) {
              if (defData->defValidNum((int)(yyvsp[-1].dval)))
                CALLBACK(defData->callbacks->UnitsCbk,  defrUnitsCbkType, (yyvsp[-1].dval));
            }
          }
#line 3799 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 94: /* divider_char: K_DIVIDERCHAR QSTRING ';'  */
#line 520 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->DividerCbk)
              CALLBACK(defData->callbacks->DividerCbk, defrDividerCbkType, (yyvsp[-1].string));
            defData->hasDivChar = 1;
          }
#line 3809 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 95: /* bus_bit_chars: K_BUSBITCHARS QSTRING ';'  */
#line 527 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->BusBitCbk)
              CALLBACK(defData->callbacks->BusBitCbk, defrBusBitCbkType, (yyvsp[-1].string));
            defData->hasBusBit = 1;
          }
#line 3819 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 96: /* $@18: %empty  */
#line 533 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                     {defData->dumb_mode = 1;defData->no_num = 1; }
#line 3825 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 97: /* canplace: K_CANPLACE $@18 T_STRING NUMBER NUMBER orient K_DO NUMBER K_BY NUMBER K_STEP NUMBER NUMBER ';'  */
#line 535 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->CanplaceCbk) {
                defData->Canplace.setName((yyvsp[-11].string));
                defData->Canplace.setLocation((yyvsp[-10].dval),(yyvsp[-9].dval));
                defData->Canplace.setOrient((yyvsp[-8].integer));
                defData->Canplace.setDo((yyvsp[-6].dval),(yyvsp[-4].dval),(yyvsp[-2].dval),(yyvsp[-1].dval));
                CALLBACK(defData->callbacks->CanplaceCbk, defrCanplaceCbkType,
                &(defData->Canplace));
              }
            }
#line 3840 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 98: /* $@19: %empty  */
#line 545 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                             {defData->dumb_mode = 1;defData->no_num = 1; }
#line 3846 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 99: /* cannotoccupy: K_CANNOTOCCUPY $@19 T_STRING NUMBER NUMBER orient K_DO NUMBER K_BY NUMBER K_STEP NUMBER NUMBER ';'  */
#line 547 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->CannotOccupyCbk) {
                defData->CannotOccupy.setName((yyvsp[-11].string));
                defData->CannotOccupy.setLocation((yyvsp[-10].dval),(yyvsp[-9].dval));
                defData->CannotOccupy.setOrient((yyvsp[-8].integer));
                defData->CannotOccupy.setDo((yyvsp[-6].dval),(yyvsp[-4].dval),(yyvsp[-2].dval),(yyvsp[-1].dval));
                CALLBACK(defData->callbacks->CannotOccupyCbk, defrCannotOccupyCbkType,
                        &(defData->CannotOccupy));
              }
            }
#line 3861 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 100: /* orient: K_N  */
#line 558 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 0;}
#line 3867 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 101: /* orient: K_W  */
#line 559 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 1;}
#line 3873 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 102: /* orient: K_S  */
#line 560 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 2;}
#line 3879 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 103: /* orient: K_E  */
#line 561 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 3;}
#line 3885 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 104: /* orient: K_FN  */
#line 562 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 4;}
#line 3891 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 105: /* orient: K_FW  */
#line 563 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 5;}
#line 3897 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 106: /* orient: K_FS  */
#line 564 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 6;}
#line 3903 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 107: /* orient: K_FE  */
#line 565 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 7;}
#line 3909 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 108: /* $@20: %empty  */
#line 568 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            defData->Geometries.Reset();
          }
#line 3917 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 109: /* die_area: K_DIEAREA $@20 firstPt nextPt otherPts ';'  */
#line 572 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->DieAreaCbk) {
               defData->DieArea.addPoint(&defData->Geometries);
               CALLBACK(defData->callbacks->DieAreaCbk, defrDieAreaCbkType, &(defData->DieArea));
            }
          }
#line 3928 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 110: /* pin_cap_rule: start_def_cap pin_caps end_def_cap  */
#line 581 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { }
#line 3934 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 111: /* start_def_cap: K_DEFAULTCAP NUMBER  */
#line 584 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.4) {
             if (defData->callbacks->DefaultCapCbk)
                CALLBACK(defData->callbacks->DefaultCapCbk, defrDefaultCapCbkType, round((yyvsp[0].dval)));
          } else {
             if (defData->callbacks->DefaultCapCbk) // write error only if cbk is set 
                if (defData->defaultCapWarnings++ < defData->settings->DefaultCapWarnings)
                   defData->defWarning(7017, "The DEFAULTCAP statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
#line 3949 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 114: /* pin_cap: K_MINPINS NUMBER K_WIRECAP NUMBER ';'  */
#line 600 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.4) {
              if (defData->callbacks->PinCapCbk) {
                defData->PinCap.setPin(round((yyvsp[-3].dval)));
                defData->PinCap.setCap((yyvsp[-1].dval));
                CALLBACK(defData->callbacks->PinCapCbk, defrPinCapCbkType, &(defData->PinCap));
              }
            }
          }
#line 3963 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 115: /* end_def_cap: K_END K_DEFAULTCAP  */
#line 611 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { }
#line 3969 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 116: /* pin_rule: start_pins pins end_pins  */
#line 614 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { }
#line 3975 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 117: /* start_pins: K_PINS NUMBER ';'  */
#line 617 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->StartPinsCbk)
              CALLBACK(defData->callbacks->StartPinsCbk, defrStartPinsCbkType, round((yyvsp[-1].dval)));
          }
#line 3984 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 120: /* $@21: %empty  */
#line 626 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {defData->dumb_mode = 1; defData->no_num = 1; }
#line 3990 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 121: /* $@22: %empty  */
#line 627 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {defData->dumb_mode = 1; defData->no_num = 1; }
#line 3996 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 122: /* $@23: %empty  */
#line 628 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              defData->Pin.Setup((yyvsp[-4].string), (yyvsp[0].string));
            }
            defData->hasPort = 0;
            defData->hadPortOnce = 0;
          }
#line 4008 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 123: /* pin: '-' $@21 T_STRING '+' K_NET $@22 T_STRING $@23 pin_options ';'  */
#line 636 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->PinCbk)
              CALLBACK(defData->callbacks->PinCbk, defrPinCbkType, &defData->Pin);
          }
#line 4017 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 126: /* pin_option: '+' K_SPECIAL  */
#line 645 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk)
              defData->Pin.setSpecial();
          }
#line 4026 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 127: /* pin_option: extension_stmt  */
#line 651 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->PinExtCbk)
              CALLBACK(defData->callbacks->PinExtCbk, defrPinExtCbkType, &defData->History_text[0]);
          }
#line 4035 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 128: /* pin_option: '+' K_DIRECTION T_STRING  */
#line 657 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.setDirection((yyvsp[0].string));
          }
#line 4044 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 129: /* pin_option: '+' K_NETEXPR QSTRING  */
#line 663 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The NETEXPR statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6506, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
                defData->Pin.setNetExpr((yyvsp[0].string));

            }
          }
#line 4068 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 130: /* $@24: %empty  */
#line 683 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                  { defData->dumb_mode = 1; }
#line 4074 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 131: /* pin_option: '+' K_SUPPLYSENSITIVITY $@24 T_STRING  */
#line 684 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The SUPPLYSENSITIVITY statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6507, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
                defData->Pin.setSupplySens((yyvsp[0].string));
            }
          }
#line 4097 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 132: /* $@25: %empty  */
#line 703 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                  { defData->dumb_mode = 1; }
#line 4103 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 133: /* pin_option: '+' K_GROUNDSENSITIVITY $@25 T_STRING  */
#line 704 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The GROUNDSENSITIVITY statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6508, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
                defData->Pin.setGroundSens((yyvsp[0].string));
            }
          }
#line 4126 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 134: /* pin_option: '+' K_USE use_type  */
#line 724 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) defData->Pin.setUse((yyvsp[0].string));
          }
#line 4134 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 135: /* pin_option: '+' K_PORT  */
#line 728 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.7) {
               if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                 if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                     (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                   defData->defMsg = (char*)malloc(10000);
                   sprintf (defData->defMsg,
                     "The PORT in PINS is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                   defData->defError(6555, defData->defMsg);
                   free(defData->defMsg);
                   CHKERR();
                 }
               }
            } else {
               if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                   defData->Pin.addPort();
               }

               defData->hasPort = 1;
               defData->hadPortOnce = 1;
            }
          }
#line 4161 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 136: /* $@26: %empty  */
#line 751 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      { defData->dumb_mode = 1; }
#line 4167 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 137: /* $@27: %empty  */
#line 752 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              if (defData->hasPort) {
                 defData->Pin.addPortLayer((yyvsp[0].string));
              } else if (defData->hadPortOnce) {
                 if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                   (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                   defData->defError(7418, "syntax error");
                   CHKERR();
                 }
              } else {
                 defData->Pin.addLayer((yyvsp[0].string));
              }
            }
          }
#line 4187 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 138: /* pin_option: '+' K_LAYER $@26 T_STRING $@27 pin_layer_mask_opt pin_layer_spacing_opt pt pt  */
#line 768 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              if (defData->hasPort)
                 defData->Pin.addPortLayerPts((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
              else if (!defData->hadPortOnce)
                 defData->Pin.addLayerPts((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
            }
          }
#line 4200 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 139: /* $@28: %empty  */
#line 777 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 4206 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 140: /* $@29: %empty  */
#line 778 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The POLYGON statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6509, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort) {
                   defData->Pin.addPortPolygon((yyvsp[0].string));
                } else if (defData->hadPortOnce) {
                   if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                     (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                     defData->defError(7418, "syntax error");
                     CHKERR();
                   }
                } else {
                   defData->Pin.addPolygon((yyvsp[0].string));
                }
              }
            }
            
            defData->Geometries.Reset();            
          }
#line 4242 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 141: /* pin_option: '+' K_POLYGON $@28 T_STRING $@29 pin_poly_mask_opt pin_poly_spacing_opt firstPt nextPt nextPt otherPts  */
#line 810 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum >= 5.6) {  // only add if 5.6 or beyond
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortPolygonPts(&defData->Geometries);
                else if (!defData->hadPortOnce)
                   defData->Pin.addPolygonPts(&defData->Geometries);
              }
            }
          }
#line 4257 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 142: /* $@30: %empty  */
#line 820 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    { defData->dumb_mode = 1; }
#line 4263 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 143: /* pin_option: '+' K_VIA $@30 T_STRING pin_via_mask_opt '(' NUMBER NUMBER ')'  */
#line 821 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.7) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The PIN VIA statement is available in version 5.7 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6556, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort) {
                   defData->Pin.addPortVia((yyvsp[-5].string), (int)(yyvsp[-2].dval),
                                               (int)(yyvsp[-1].dval), (yyvsp[-4].integer));
                } else if (defData->hadPortOnce) {
                   if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                     (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                     defData->defError(7418, "syntax error");
                     CHKERR();
                   }
                } else {
                   defData->Pin.addVia((yyvsp[-5].string), (int)(yyvsp[-2].dval),
                                               (int)(yyvsp[-1].dval), (yyvsp[-4].integer));
                }
              }
            }
          }
#line 4299 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 144: /* pin_option: placement_status pt orient  */
#line 854 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              if (defData->hasPort) {
                 defData->Pin.setPortPlacement((yyvsp[-2].integer), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
                 defData->hasPort = 0;
                 defData->hadPortOnce = 1;
              } else if (defData->hadPortOnce) {
                 if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                   (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                   defData->defError(7418, "syntax error");
                   CHKERR();
                 }
              } else {
                 defData->Pin.setPlacement((yyvsp[-2].integer), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
              }
            }
          }
#line 4321 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 145: /* pin_option: '+' K_ANTENNAPINPARTIALMETALAREA NUMBER pin_layer_opt  */
#line 874 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINPARTIALMETALAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6510, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinPartialMetalArea((int)(yyvsp[-1].dval), (yyvsp[0].string)); 
          }
#line 4343 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 146: /* pin_option: '+' K_ANTENNAPINPARTIALMETALSIDEAREA NUMBER pin_layer_opt  */
#line 892 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINPARTIALMETALSIDEAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6511, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinPartialMetalSideArea((int)(yyvsp[-1].dval), (yyvsp[0].string)); 
          }
#line 4365 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 147: /* pin_option: '+' K_ANTENNAPINGATEAREA NUMBER pin_layer_opt  */
#line 910 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINGATEAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6512, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
                defData->Pin.addAPinGateArea((int)(yyvsp[-1].dval), (yyvsp[0].string)); 
            }
#line 4387 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 148: /* pin_option: '+' K_ANTENNAPINDIFFAREA NUMBER pin_layer_opt  */
#line 928 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINDIFFAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6513, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinDiffArea((int)(yyvsp[-1].dval), (yyvsp[0].string)); 
          }
#line 4409 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 149: /* $@31: %empty  */
#line 945 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                                    {defData->dumb_mode=1;}
#line 4415 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 150: /* pin_option: '+' K_ANTENNAPINMAXAREACAR NUMBER K_LAYER $@31 T_STRING  */
#line 946 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINMAXAREACAR statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6514, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinMaxAreaCar((int)(yyvsp[-3].dval), (yyvsp[0].string)); 
          }
#line 4437 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 151: /* $@32: %empty  */
#line 963 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                                        {defData->dumb_mode=1;}
#line 4443 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 152: /* pin_option: '+' K_ANTENNAPINMAXSIDEAREACAR NUMBER K_LAYER $@32 T_STRING  */
#line 965 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINMAXSIDEAREACAR statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6515, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinMaxSideAreaCar((int)(yyvsp[-3].dval), (yyvsp[0].string)); 
          }
#line 4465 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 153: /* pin_option: '+' K_ANTENNAPINPARTIALCUTAREA NUMBER pin_layer_opt  */
#line 983 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINPARTIALCUTAREA statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6516, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinPartialCutArea((int)(yyvsp[-1].dval), (yyvsp[0].string)); 
          }
#line 4487 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 154: /* $@33: %empty  */
#line 1000 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                                   {defData->dumb_mode=1;}
#line 4493 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 155: /* pin_option: '+' K_ANTENNAPINMAXCUTCAR NUMBER K_LAYER $@33 T_STRING  */
#line 1001 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum <= 5.3) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAPINMAXCUTCAR statement is available in version 5.4 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6517, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAPinMaxCutCar((int)(yyvsp[-3].dval), (yyvsp[0].string)); 
          }
#line 4515 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 156: /* pin_option: '+' K_ANTENNAMODEL pin_oxide  */
#line 1019 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {  // 5.5 syntax 
            if (defData->VersionNum < 5.5) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The ANTENNAMODEL statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6518, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
          }
#line 4535 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 158: /* pin_layer_mask_opt: K_MASK NUMBER  */
#line 1037 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { 
           if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->pinWarnings, defData->settings->PinWarnings)) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortLayerMask((int)(yyvsp[0].dval));
                else
                   defData->Pin.addLayerMask((int)(yyvsp[0].dval));
              }
           }
         }
#line 4550 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 159: /* pin_via_mask_opt: %empty  */
#line 1050 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = 0; }
#line 4556 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 160: /* pin_via_mask_opt: K_MASK NUMBER  */
#line 1052 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { 
           if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->pinWarnings, defData->settings->PinWarnings)) {
             (yyval.integer) = (yyvsp[0].dval);
           }
         }
#line 4566 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 162: /* pin_poly_mask_opt: K_MASK NUMBER  */
#line 1060 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { 
           if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->pinWarnings, defData->settings->PinWarnings)) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortPolyMask((int)(yyvsp[0].dval));
                else
                   defData->Pin.addPolyMask((int)(yyvsp[0].dval));
              }
           }
         }
#line 4581 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 164: /* pin_layer_spacing_opt: K_SPACING NUMBER  */
#line 1074 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The SPACING statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6519, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortLayerSpacing((int)(yyvsp[0].dval));
                else
                   defData->Pin.addLayerSpacing((int)(yyvsp[0].dval));
              }
            }
          }
#line 4608 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 165: /* pin_layer_spacing_opt: K_DESIGNRULEWIDTH NUMBER  */
#line 1097 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "DESIGNRULEWIDTH statement is a version 5.6 and later syntax.\nYour def file is defined with version %g", defData->VersionNum);
                  defData->defError(6520, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortLayerDesignRuleWidth((int)(yyvsp[0].dval));
                else
                   defData->Pin.addLayerDesignRuleWidth((int)(yyvsp[0].dval));
              }
            }
          }
#line 4635 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 167: /* pin_poly_spacing_opt: K_SPACING NUMBER  */
#line 1122 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "SPACING statement is a version 5.6 and later syntax.\nYour def file is defined with version %g", defData->VersionNum);
                  defData->defError(6521, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortPolySpacing((int)(yyvsp[0].dval));
                else
                   defData->Pin.addPolySpacing((int)(yyvsp[0].dval));
              }
            }
          }
#line 4662 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 168: /* pin_poly_spacing_opt: K_DESIGNRULEWIDTH NUMBER  */
#line 1145 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if ((defData->pinWarnings++ < defData->settings->PinWarnings) &&
                    (defData->pinWarnings++ < defData->settings->PinExtWarnings)) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The DESIGNRULEWIDTH statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                  defData->defError(6520, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            } else {
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortPolyDesignRuleWidth((int)(yyvsp[0].dval));
                else
                   defData->Pin.addPolyDesignRuleWidth((int)(yyvsp[0].dval));
              }
            }
          }
#line 4689 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 169: /* pin_oxide: K_OXIDE1  */
#line 1169 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 1;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4698 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 170: /* pin_oxide: K_OXIDE2  */
#line 1174 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 2;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4707 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 171: /* pin_oxide: K_OXIDE3  */
#line 1179 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 3;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4716 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 172: /* pin_oxide: K_OXIDE4  */
#line 1184 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 4;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4725 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 173: /* pin_oxide: K_OXIDE5  */
#line 1189 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 5;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4734 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 174: /* pin_oxide: K_OXIDE6  */
#line 1194 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 6;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4743 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 175: /* pin_oxide: K_OXIDE7  */
#line 1199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 7;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4752 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 176: /* pin_oxide: K_OXIDE8  */
#line 1204 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 8;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4761 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 177: /* pin_oxide: K_OXIDE9  */
#line 1209 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 9;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4770 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 178: /* pin_oxide: K_OXIDE10  */
#line 1214 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 10;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4779 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 179: /* pin_oxide: K_OXIDE11  */
#line 1219 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 11;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4788 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 180: /* pin_oxide: K_OXIDE12  */
#line 1224 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 12;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4797 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 181: /* pin_oxide: K_OXIDE13  */
#line 1229 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 13;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4806 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 182: /* pin_oxide: K_OXIDE14  */
#line 1234 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 14;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4815 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 183: /* pin_oxide: K_OXIDE15  */
#line 1239 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 15;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4824 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 184: /* pin_oxide: K_OXIDE16  */
#line 1244 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 16;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4833 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 185: /* pin_oxide: K_OXIDE17  */
#line 1249 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 17;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4842 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 186: /* pin_oxide: K_OXIDE18  */
#line 1254 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 18;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4851 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 187: /* pin_oxide: K_OXIDE19  */
#line 1259 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 19;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4860 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 188: /* pin_oxide: K_OXIDE20  */
#line 1264 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 20;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4869 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 189: /* pin_oxide: K_OXIDE21  */
#line 1269 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 21;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4878 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 190: /* pin_oxide: K_OXIDE22  */
#line 1274 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 22;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4887 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 191: /* pin_oxide: K_OXIDE23  */
#line 1279 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 23;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4896 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 192: /* pin_oxide: K_OXIDE24  */
#line 1284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 24;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4905 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 193: /* pin_oxide: K_OXIDE25  */
#line 1289 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 25;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4914 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 194: /* pin_oxide: K_OXIDE26  */
#line 1294 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 26;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4923 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 195: /* pin_oxide: K_OXIDE27  */
#line 1299 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 27;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4932 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 196: /* pin_oxide: K_OXIDE28  */
#line 1304 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 28;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4941 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 197: /* pin_oxide: K_OXIDE29  */
#line 1309 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 29;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4950 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 198: /* pin_oxide: K_OXIDE30  */
#line 1314 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 30;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4959 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 199: /* pin_oxide: K_OXIDE31  */
#line 1319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 31;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4968 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 200: /* pin_oxide: K_OXIDE32  */
#line 1324 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->aOxide = 32;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4977 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 201: /* use_type: K_SIGNAL  */
#line 1331 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"SIGNAL"; }
#line 4983 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 202: /* use_type: K_POWER  */
#line 1333 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"POWER"; }
#line 4989 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 203: /* use_type: K_GROUND  */
#line 1335 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"GROUND"; }
#line 4995 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 204: /* use_type: K_CLOCK  */
#line 1337 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"CLOCK"; }
#line 5001 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 205: /* use_type: K_TIEOFF  */
#line 1339 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"TIEOFF"; }
#line 5007 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 206: /* use_type: K_ANALOG  */
#line 1341 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"ANALOG"; }
#line 5013 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 207: /* use_type: K_SCAN  */
#line 1343 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"SCAN"; }
#line 5019 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 208: /* use_type: K_RESET  */
#line 1345 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)"RESET"; }
#line 5025 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 209: /* pin_layer_opt: %empty  */
#line 1349 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (char*)""; }
#line 5031 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 210: /* $@34: %empty  */
#line 1350 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  {defData->dumb_mode=1;}
#line 5037 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 211: /* pin_layer_opt: K_LAYER $@34 T_STRING  */
#line 1351 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.string) = (yyvsp[0].string); }
#line 5043 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 212: /* end_pins: K_END K_PINS  */
#line 1354 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->PinEndCbk)
            CALLBACK(defData->callbacks->PinEndCbk, defrPinEndCbkType, 0);
        }
#line 5052 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 213: /* $@35: %empty  */
#line 1359 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                {defData->dumb_mode = 2; defData->no_num = 2; }
#line 5058 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 214: /* $@36: %empty  */
#line 1361 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
            defData->rowName = (yyvsp[-4].string);
            defData->Row.setup((yyvsp[-4].string), (yyvsp[-3].string), (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].integer));
          }
        }
#line 5069 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 215: /* row_rule: K_ROW $@35 T_STRING T_STRING NUMBER NUMBER orient $@36 row_do_option row_options ';'  */
#line 1369 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) 
            CALLBACK(defData->callbacks->RowCbk, defrRowCbkType, &defData->Row);
        }
#line 5078 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 216: /* row_do_option: %empty  */
#line 1375 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.6) {
            if (defData->callbacks->RowCbk) {
              if (defData->rowWarnings++ < defData->settings->RowWarnings) {
                defData->defError(6523, "Invalid ROW statement defined in the DEF file. The DO statement which is required in the ROW statement is not defined.\nUpdate your DEF file with a DO statement.");
                CHKERR();
              }
            }
          }
        }
#line 5093 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 217: /* row_do_option: K_DO NUMBER K_BY NUMBER row_step_option  */
#line 1386 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          // 06/05/2002 - pcr 448455 
          // Check for 1 and 0 in the correct position 
          // 07/26/2002 - Commented out due to pcr 459218 
          if (defData->hasDoStep) {
            // 04/29/2004 - pcr 695535 
            // changed the testing 
            if ((((yyvsp[-1].dval) == 1) && (defData->yStep == 0)) ||
                (((yyvsp[-3].dval) == 1) && (defData->xStep == 0))) {
              // do nothing 
            } else 
              if (defData->VersionNum < 5.6) {
                if (defData->callbacks->RowCbk) {
                  if (defData->rowWarnings++ < defData->settings->RowWarnings) {
                    defData->defMsg = (char*)malloc(1000);
                    sprintf(defData->defMsg,
                            "The DO statement in the ROW statement with the name %s has invalid syntax.\nThe valid syntax is \"DO numX BY 1 STEP spaceX 0 | DO 1 BY numY STEP 0 spaceY\".\nSpecify the valid syntax and try again.", defData->rowName);
                    defData->defWarning(7018, defData->defMsg);
                    free(defData->defMsg);
                    }
                  }
              }
          }
          // pcr 459218 - Error if at least numX or numY does not equal 1 
          if (((yyvsp[-3].dval) != 1) && ((yyvsp[-1].dval) != 1)) {
            if (defData->callbacks->RowCbk) {
              if (defData->rowWarnings++ < defData->settings->RowWarnings) {
                defData->defError(6524, "Invalid syntax specified. The valid syntax is either \"DO 1 BY num or DO num BY 1\". Specify the valid syntax and try again.");
                CHKERR();
              }
            }
          }
          if (defData->callbacks->RowCbk)
            defData->Row.setDo(round((yyvsp[-3].dval)), round((yyvsp[-1].dval)), defData->xStep, defData->yStep);
        }
#line 5133 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 218: /* row_step_option: %empty  */
#line 1423 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          defData->hasDoStep = 0;
        }
#line 5141 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 219: /* row_step_option: K_STEP NUMBER NUMBER  */
#line 1427 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          defData->hasDoStep = 1;
          defData->Row.setHasDoStep();
          defData->xStep = (yyvsp[-1].dval);
          defData->yStep = (yyvsp[0].dval);
        }
#line 5152 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 222: /* $@37: %empty  */
#line 1438 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                            {defData->dumb_mode = DEF_MAX_INT; }
#line 5158 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 223: /* row_option: '+' K_PROPERTY $@37 row_prop_list  */
#line 1440 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { defData->dumb_mode = 0; }
#line 5164 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 226: /* row_prop: T_STRING NUMBER  */
#line 1447 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
             char propTp;
             char* str = defData->ringCopy("                       ");
             propTp =  defData->session->RowProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "ROW");
             // For backword compatibility, also set the string value 
             sprintf(str, "%g", (yyvsp[0].dval));
             defData->Row.addNumProperty((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
          }
        }
#line 5180 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 227: /* row_prop: T_STRING QSTRING  */
#line 1459 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
             char propTp;
             propTp =  defData->session->RowProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "ROW");
             defData->Row.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5193 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 228: /* row_prop: T_STRING T_STRING  */
#line 1468 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
             char propTp;
             propTp =  defData->session->RowProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "ROW");
             defData->Row.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5206 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 229: /* $@38: %empty  */
#line 1478 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->TrackCbk) {
            defData->Track.setup((yyvsp[-1].string));
          }
        }
#line 5216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 230: /* tracks_rule: track_start NUMBER $@38 K_DO NUMBER K_STEP NUMBER track_opts ';'  */
#line 1484 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (((yyvsp[-4].dval) <= 0) && (defData->VersionNum >= 5.4)) {
            if (defData->callbacks->TrackCbk)
              if (defData->trackWarnings++ < defData->settings->TrackWarnings) {
                defData->defMsg = (char*)malloc(1000);
                sprintf (defData->defMsg,
                   "The DO number %g in TRACK is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again.", (yyvsp[-4].dval));
                defData->defError(6525, defData->defMsg);
                free(defData->defMsg);
              }
          }
          if ((yyvsp[-2].dval) < 0) {
            if (defData->callbacks->TrackCbk)
              if (defData->trackWarnings++ < defData->settings->TrackWarnings) {
                defData->defMsg = (char*)malloc(1000);
                sprintf (defData->defMsg,
                   "The STEP number %g in TRACK is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again.", (yyvsp[-2].dval));
                defData->defError(6526, defData->defMsg);
                free(defData->defMsg);
              }
          }
          if (defData->callbacks->TrackCbk) {
            defData->Track.setDo(round((yyvsp[-7].dval)), round((yyvsp[-4].dval)), (yyvsp[-2].dval));
            CALLBACK(defData->callbacks->TrackCbk, defrTrackCbkType, &defData->Track);
          }
        }
#line 5247 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 231: /* track_start: K_TRACKS track_type  */
#line 1512 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          (yyval.string) = (yyvsp[0].string);
        }
#line 5255 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 232: /* track_type: K_X  */
#line 1517 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"X";}
#line 5261 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 233: /* track_type: K_Y  */
#line 1519 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"Y";}
#line 5267 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 236: /* track_mask_statement: K_MASK NUMBER same_mask  */
#line 1525 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
           { 
              if (defData->validateMaskInput((int)(yyvsp[-1].dval), defData->trackWarnings, defData->settings->TrackWarnings)) {
                  if (defData->callbacks->TrackCbk) {
                    defData->Track.addMask((yyvsp[-1].dval), (yyvsp[0].integer));
                  }
               }
            }
#line 5279 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 237: /* same_mask: %empty  */
#line 1535 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = 0; }
#line 5285 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 238: /* same_mask: K_SAMEMASK  */
#line 1537 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = 1; }
#line 5291 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 240: /* $@39: %empty  */
#line 1540 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1000; }
#line 5297 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 241: /* track_layer_statement: K_LAYER $@39 track_layer track_layers  */
#line 1541 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { defData->dumb_mode = 0; }
#line 5303 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 244: /* track_layer: T_STRING  */
#line 1548 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->TrackCbk)
            defData->Track.addLayer((yyvsp[0].string));
        }
#line 5312 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 245: /* gcellgrid: K_GCELLGRID track_type NUMBER K_DO NUMBER K_STEP NUMBER ';'  */
#line 1555 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if ((yyvsp[-3].dval) <= 0) {
            if (defData->callbacks->GcellGridCbk)
              if (defData->gcellGridWarnings++ < defData->settings->GcellGridWarnings) {
                defData->defMsg = (char*)malloc(1000);
                sprintf (defData->defMsg,
                   "The DO number %g in GCELLGRID is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again.", (yyvsp[-3].dval));
                defData->defError(6527, defData->defMsg);
                free(defData->defMsg);
              }
          }
          if ((yyvsp[-1].dval) < 0) {
            if (defData->callbacks->GcellGridCbk)
              if (defData->gcellGridWarnings++ < defData->settings->GcellGridWarnings) {
                defData->defMsg = (char*)malloc(1000);
                sprintf (defData->defMsg,
                   "The STEP number %g in GCELLGRID is invalid.\nThe number value has to be greater than 0. Specify the valid syntax and try again.", (yyvsp[-1].dval));
                defData->defError(6528, defData->defMsg);
                free(defData->defMsg);
              }
          }
          if (defData->callbacks->GcellGridCbk) {
            defData->GcellGrid.setup((yyvsp[-6].string), round((yyvsp[-5].dval)), round((yyvsp[-3].dval)), (yyvsp[-1].dval));
            CALLBACK(defData->callbacks->GcellGridCbk, defrGcellGridCbkType, &defData->GcellGrid);
          }
        }
#line 5343 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 246: /* extension_section: K_BEGINEXT  */
#line 1583 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ExtensionCbk)
             CALLBACK(defData->callbacks->ExtensionCbk, defrExtensionCbkType, &defData->History_text[0]);
        }
#line 5352 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 247: /* extension_stmt: '+' K_BEGINEXT  */
#line 1589 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { }
#line 5358 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 249: /* via: K_VIAS NUMBER ';'  */
#line 1595 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ViaStartCbk)
            CALLBACK(defData->callbacks->ViaStartCbk, defrViaStartCbkType, round((yyvsp[-1].dval)));
        }
#line 5367 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 252: /* $@40: %empty  */
#line 1604 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                     {defData->dumb_mode = 1;defData->no_num = 1; }
#line 5373 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 253: /* $@41: %empty  */
#line 1605 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->ViaCbk) defData->Via.setup((yyvsp[0].string));
              defData->viaRule = 0;
            }
#line 5382 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 254: /* via_declaration: '-' $@40 T_STRING $@41 layer_stmts ';'  */
#line 1610 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->ViaCbk)
                CALLBACK(defData->callbacks->ViaCbk, defrViaCbkType, &defData->Via);
              defData->Via.clear();
            }
#line 5392 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 257: /* $@42: %empty  */
#line 1620 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       {defData->dumb_mode = 1;defData->no_num = 1; }
#line 5398 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 258: /* layer_stmt: '+' K_RECT $@42 T_STRING mask pt pt  */
#line 1621 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { 
              if (defData->callbacks->ViaCbk)
                if (defData->validateMaskInput((yyvsp[-2].integer), defData->viaWarnings, defData->settings->ViaWarnings)) {
                    defData->Via.addLayer((yyvsp[-3].string), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y, (yyvsp[-2].integer));
                }
            }
#line 5409 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 259: /* $@43: %empty  */
#line 1627 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 5415 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 260: /* $@44: %empty  */
#line 1628 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->VersionNum < 5.6) {
                if (defData->callbacks->ViaCbk) {
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings) {
                    defData->defMsg = (char*)malloc(1000);
                    sprintf (defData->defMsg,
                       "The POLYGON statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                    defData->defError(6509, defData->defMsg);
                    free(defData->defMsg);
                    CHKERR();
                  }
                }
              }
              
              defData->Geometries.Reset();
              
            }
#line 5437 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 261: /* layer_stmt: '+' K_POLYGON $@43 T_STRING mask $@44 firstPt nextPt nextPt otherPts  */
#line 1646 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->VersionNum >= 5.6) {  // only add if 5.6 or beyond
                if (defData->callbacks->ViaCbk)
                  if (defData->validateMaskInput((yyvsp[-5].integer), defData->viaWarnings, defData->settings->ViaWarnings)) {
                    defData->Via.addPolygon((yyvsp[-6].string), &defData->Geometries, (yyvsp[-5].integer));
                  }
              }
            }
#line 5450 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 262: /* $@45: %empty  */
#line 1654 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                            {defData->dumb_mode = 1;defData->no_num = 1; }
#line 5456 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 263: /* layer_stmt: '+' K_PATTERNNAME $@45 T_STRING  */
#line 1655 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->VersionNum < 5.6) {
                if (defData->callbacks->ViaCbk)
                  defData->Via.addPattern((yyvsp[0].string));
              } else
                if (defData->callbacks->ViaCbk)
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings)
                    defData->defWarning(7019, "The PATTERNNAME statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement."); 
            }
#line 5470 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 264: /* $@46: %empty  */
#line 1664 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        {defData->dumb_mode = 1;defData->no_num = 1; }
#line 5476 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 265: /* $@47: %empty  */
#line 1666 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       {defData->dumb_mode = 3;defData->no_num = 1; }
#line 5482 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 266: /* layer_stmt: '+' K_VIARULE $@46 T_STRING '+' K_CUTSIZE NUMBER NUMBER '+' K_LAYERS $@47 T_STRING T_STRING T_STRING '+' K_CUTSPACING NUMBER NUMBER '+' K_ENCLOSURE NUMBER NUMBER NUMBER NUMBER  */
#line 1669 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
               defData->viaRule = 1;
               if (defData->VersionNum < 5.6) {
                if (defData->callbacks->ViaCbk) {
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings) {
                    defData->defMsg = (char*)malloc(1000);
                    sprintf (defData->defMsg,
                       "The VIARULE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                    defData->defError(6557, defData->defMsg);
                    free(defData->defMsg);
                    CHKERR();
                  }
                }
              } else {
                if (defData->callbacks->ViaCbk)
                   defData->Via.addViaRule((yyvsp[-20].string), (int)(yyvsp[-17].dval), (int)(yyvsp[-16].dval), (yyvsp[-12].string), (yyvsp[-11].string),
                                             (yyvsp[-10].string), (int)(yyvsp[-7].dval), (int)(yyvsp[-6].dval), (int)(yyvsp[-3].dval),
                                             (int)(yyvsp[-2].dval), (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval)); 
              }
            }
#line 5507 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 268: /* layer_stmt: extension_stmt  */
#line 1691 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            if (defData->callbacks->ViaExtCbk)
              CALLBACK(defData->callbacks->ViaExtCbk, defrViaExtCbkType, &defData->History_text[0]);
          }
#line 5516 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 269: /* layer_viarule_opts: '+' K_ROWCOL NUMBER NUMBER  */
#line 1697 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (!defData->viaRule) {
                if (defData->callbacks->ViaCbk) {
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings) {
                    defData->defError(6559, "The ROWCOL statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement.");
                    CHKERR();
                  }
                }
              } else if (defData->callbacks->ViaCbk)
                 defData->Via.addRowCol((int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
            }
#line 5532 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 270: /* layer_viarule_opts: '+' K_ORIGIN NUMBER NUMBER  */
#line 1709 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (!defData->viaRule) {
                if (defData->callbacks->ViaCbk) {
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings) {
                    defData->defError(6560, "The ORIGIN statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement.");
                    CHKERR();
                  }
                }
              } else if (defData->callbacks->ViaCbk)
                 defData->Via.addOrigin((int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
            }
#line 5548 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 271: /* layer_viarule_opts: '+' K_OFFSET NUMBER NUMBER NUMBER NUMBER  */
#line 1721 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (!defData->viaRule) {
                if (defData->callbacks->ViaCbk) {
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings) {
                    defData->defError(6561, "The OFFSET statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement.");
                    CHKERR();
                  }
                }
              } else if (defData->callbacks->ViaCbk)
                 defData->Via.addOffset((int)(yyvsp[-3].dval), (int)(yyvsp[-2].dval), (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
            }
#line 5564 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 272: /* $@48: %empty  */
#line 1732 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        {defData->dumb_mode = 1;defData->no_num = 1; }
#line 5570 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 273: /* layer_viarule_opts: '+' K_PATTERN $@48 T_STRING  */
#line 1733 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (!defData->viaRule) {
                if (defData->callbacks->ViaCbk) {
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings) {
                    defData->defError(6562, "The PATTERN statement is missing from the VIARULE statement. Ensure that it exists in the VIARULE statement.");
                    CHKERR();
                  }
                }
              } else if (defData->callbacks->ViaCbk)
                 defData->Via.addCutPattern((yyvsp[0].string));
            }
#line 5586 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 274: /* firstPt: pt  */
#line 1746 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->Geometries.startList((yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 5592 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 275: /* nextPt: pt  */
#line 1749 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { defData->Geometries.addToList((yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 5598 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 278: /* pt: '(' NUMBER NUMBER ')'  */
#line 1756 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            defData->save_x = (yyvsp[-2].dval);
            defData->save_y = (yyvsp[-1].dval);
            (yyval.pt).x = round((yyvsp[-2].dval));
            (yyval.pt).y = round((yyvsp[-1].dval));
          }
#line 5609 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 279: /* pt: '(' '*' NUMBER ')'  */
#line 1763 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            defData->save_y = (yyvsp[-1].dval);
            (yyval.pt).x = round(defData->save_x);
            (yyval.pt).y = round((yyvsp[-1].dval));
          }
#line 5619 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 280: /* pt: '(' NUMBER '*' ')'  */
#line 1769 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            defData->save_x = (yyvsp[-2].dval);
            (yyval.pt).x = round((yyvsp[-2].dval));
            (yyval.pt).y = round(defData->save_y);
          }
#line 5629 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 281: /* pt: '(' '*' '*' ')'  */
#line 1775 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            (yyval.pt).x = round(defData->save_x);
            (yyval.pt).y = round(defData->save_y);
          }
#line 5638 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 282: /* mask: %empty  */
#line 1781 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.integer) = 0; }
#line 5644 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 283: /* mask: '+' K_MASK NUMBER  */
#line 1783 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.integer) = (yyvsp[0].dval); }
#line 5650 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 284: /* via_end: K_END K_VIAS  */
#line 1786 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->ViaEndCbk)
            CALLBACK(defData->callbacks->ViaEndCbk, defrViaEndCbkType, 0);
        }
#line 5659 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 285: /* regions_section: regions_start regions_stmts K_END K_REGIONS  */
#line 1792 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RegionEndCbk)
            CALLBACK(defData->callbacks->RegionEndCbk, defrRegionEndCbkType, 0);
        }
#line 5668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 286: /* regions_start: K_REGIONS NUMBER ';'  */
#line 1798 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RegionStartCbk)
            CALLBACK(defData->callbacks->RegionStartCbk, defrRegionStartCbkType, round((yyvsp[-1].dval)));
        }
#line 5677 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 288: /* regions_stmts: regions_stmts regions_stmt  */
#line 1805 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {}
#line 5683 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 289: /* $@49: %empty  */
#line 1807 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5689 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 290: /* $@50: %empty  */
#line 1808 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk)
             defData->Region.setup((yyvsp[0].string));
          defData->regTypeDef = 0;
        }
#line 5699 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 291: /* regions_stmt: '-' $@49 T_STRING $@50 rect_list region_options ';'  */
#line 1814 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { CALLBACK(defData->callbacks->RegionCbk, defrRegionCbkType, &defData->Region); }
#line 5705 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 292: /* rect_list: pt pt  */
#line 1818 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->RegionCbk)
          defData->Region.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 5712 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 293: /* rect_list: rect_list pt pt  */
#line 1821 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->RegionCbk)
          defData->Region.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 5719 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 296: /* $@51: %empty  */
#line 1829 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                               {defData->dumb_mode = DEF_MAX_INT; }
#line 5725 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 297: /* region_option: '+' K_PROPERTY $@51 region_prop_list  */
#line 1831 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { defData->dumb_mode = 0; }
#line 5731 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 298: /* region_option: '+' K_TYPE region_type  */
#line 1833 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {
           if (defData->regTypeDef) {
              if (defData->callbacks->RegionCbk) {
                if (defData->regionWarnings++ < defData->settings->RegionWarnings) {
                  defData->defError(6563, "The TYPE statement already exists. It has been defined in the REGION statement.");
                  CHKERR();
                }
              }
           }
           if (defData->callbacks->RegionCbk) defData->Region.setType((yyvsp[0].string));
           defData->regTypeDef = 1;
         }
#line 5748 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 301: /* region_prop: T_STRING NUMBER  */
#line 1852 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk) {
             char propTp;
             char* str = defData->ringCopy("                       ");
             propTp = defData->session->RegionProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "REGION");
             // For backword compatibility, also set the string value 
             // We will use a temporary string to store the number.
             // The string space is borrowed from the ring buffer
             // in the lexer.
             sprintf(str, "%g", (yyvsp[0].dval));
             defData->Region.addNumProperty((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
          }
        }
#line 5767 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 302: /* region_prop: T_STRING QSTRING  */
#line 1867 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk) {
             char propTp;
             propTp = defData->session->RegionProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "REGION");
             defData->Region.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5780 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 303: /* region_prop: T_STRING T_STRING  */
#line 1876 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk) {
             char propTp;
             propTp = defData->session->RegionProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "REGION");
             defData->Region.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5793 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 304: /* region_type: K_FENCE  */
#line 1886 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"FENCE"; }
#line 5799 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 305: /* region_type: K_GUIDE  */
#line 1888 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"GUIDE"; }
#line 5805 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 306: /* comps_maskShift_section: K_COMPSMASKSHIFT layer_statement ';'  */
#line 1891 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {
           if (defData->VersionNum < 5.8) {
                if (defData->componentWarnings++ < defData->settings->ComponentWarnings) {
                   defData->defMsg = (char*)malloc(10000);
                   sprintf (defData->defMsg,
                     "The MASKSHIFT statement is available in version 5.8 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                   defData->defError(7415, defData->defMsg);
                   free(defData->defMsg);
                   CHKERR();
                }
            }
            if (defData->callbacks->ComponentMaskShiftLayerCbk) {
                CALLBACK(defData->callbacks->ComponentMaskShiftLayerCbk, defrComponentMaskShiftLayerCbkType, &defData->ComponentMaskShiftLayer);
            }
         }
#line 5825 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 308: /* start_comps: K_COMPS NUMBER ';'  */
#line 1911 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { 
            if (defData->callbacks->ComponentStartCbk)
              CALLBACK(defData->callbacks->ComponentStartCbk, defrComponentStartCbkType,
                       round((yyvsp[-1].dval)));
         }
#line 5835 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 311: /* maskLayer: T_STRING  */
#line 1922 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
            if (defData->callbacks->ComponentMaskShiftLayerCbk) {
              defData->ComponentMaskShiftLayer.addMaskShiftLayer((yyvsp[0].string));
            }
        }
#line 5845 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 314: /* comp: comp_start comp_options ';'  */
#line 1933 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {
            if (defData->callbacks->ComponentCbk)
              CALLBACK(defData->callbacks->ComponentCbk, defrComponentCbkType, &defData->Component);
         }
#line 5854 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 315: /* comp_start: comp_id_and_name comp_net_list  */
#line 1939 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {
            defData->dumb_mode = 0;
            defData->no_num = 0;
         }
#line 5863 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 316: /* $@52: %empty  */
#line 1944 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      {defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT; }
#line 5869 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 317: /* comp_id_and_name: '-' $@52 T_STRING T_STRING  */
#line 1946 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         {
            if (defData->callbacks->ComponentCbk)
              defData->Component.IdAndName((yyvsp[-1].string), (yyvsp[0].string));
         }
#line 5878 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 318: /* comp_net_list: %empty  */
#line 1952 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { }
#line 5884 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 319: /* comp_net_list: comp_net_list '*'  */
#line 1954 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->ComponentCbk)
                defData->Component.addNet("*");
            }
#line 5893 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 320: /* comp_net_list: comp_net_list T_STRING  */
#line 1959 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->ComponentCbk)
                defData->Component.addNet((yyvsp[0].string));
            }
#line 5902 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 335: /* comp_extension_stmt: extension_stmt  */
#line 1974 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            CALLBACK(defData->callbacks->ComponentExtCbk, defrComponentExtCbkType,
                     &defData->History_text[0]);
        }
#line 5912 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 336: /* $@53: %empty  */
#line 1980 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                          {defData->dumb_mode=1; defData->no_num = 1; }
#line 5918 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 337: /* comp_eeq: '+' K_EEQMASTER $@53 T_STRING  */
#line 1981 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setEEQ((yyvsp[0].string));
        }
#line 5927 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 338: /* $@54: %empty  */
#line 1986 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                              { defData->dumb_mode = 2;  defData->no_num = 2; }
#line 5933 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 339: /* comp_generate: '+' K_COMP_GEN $@54 T_STRING opt_pattern  */
#line 1988 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
             defData->Component.setGenerate((yyvsp[-1].string), (yyvsp[0].string));
        }
#line 5942 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 340: /* opt_pattern: %empty  */
#line 1994 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)""; }
#line 5948 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 341: /* opt_pattern: T_STRING  */
#line 1996 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (yyvsp[0].string); }
#line 5954 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 342: /* comp_source: '+' K_SOURCE source_type  */
#line 1999 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setSource((yyvsp[0].string));
        }
#line 5963 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 343: /* source_type: K_NETLIST  */
#line 2005 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"NETLIST"; }
#line 5969 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 344: /* source_type: K_DIST  */
#line 2007 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"DIST"; }
#line 5975 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 345: /* source_type: K_USER  */
#line 2009 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"USER"; }
#line 5981 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 346: /* source_type: K_TIMING  */
#line 2011 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"TIMING"; }
#line 5987 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 347: /* comp_region: comp_region_start comp_pnt_list  */
#line 2016 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { }
#line 5993 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 348: /* comp_region: comp_region_start T_STRING  */
#line 2018 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setRegionName((yyvsp[0].string));
        }
#line 6002 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 349: /* comp_pnt_list: pt pt  */
#line 2024 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
          if (defData->VersionNum < 5.5) {
            if (defData->callbacks->ComponentCbk)
               defData->Component.setRegionBounds((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, 
                                                            (yyvsp[0].pt).x, (yyvsp[0].pt).y);
          }
          else
            defData->defWarning(7020, "The REGION pt pt statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
#line 6017 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 350: /* comp_pnt_list: comp_pnt_list pt pt  */
#line 2035 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
          if (defData->VersionNum < 5.5) {
            if (defData->callbacks->ComponentCbk)
               defData->Component.setRegionBounds((yyvsp[-1].pt).x, (yyvsp[-1].pt).y,
                                                            (yyvsp[0].pt).x, (yyvsp[0].pt).y);
          }
          else
            defData->defWarning(7020, "The REGION pt pt statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
#line 6032 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 351: /* $@55: %empty  */
#line 2047 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.6) {
             if (defData->callbacks->ComponentCbk) {
               if (defData->componentWarnings++ < defData->settings->ComponentWarnings) {
                 defData->defMsg = (char*)malloc(1000);
                 sprintf (defData->defMsg,
                    "The HALO statement is a version 5.6 and later syntax.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                 defData->defError(6529, defData->defMsg);
                 free(defData->defMsg);
                 CHKERR();
               }
             }
          }
        }
#line 6051 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 352: /* comp_halo: '+' K_HALO $@55 halo_soft NUMBER NUMBER NUMBER NUMBER  */
#line 2062 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setHalo((int)(yyvsp[-3].dval), (int)(yyvsp[-2].dval),
                                                 (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
        }
#line 6061 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 354: /* halo_soft: K_SOFT  */
#line 2070 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->ComponentCbk) {
             if (defData->componentWarnings++ < defData->settings->ComponentWarnings) {
                defData->defMsg = (char*)malloc(10000);
                sprintf (defData->defMsg,
                  "The HALO SOFT is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                defData->defError(6550, defData->defMsg);
                free(defData->defMsg);
                CHKERR();
             }
           }
        } else {
           if (defData->callbacks->ComponentCbk)
             defData->Component.setHaloSoft();
        }
      }
#line 6083 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 355: /* $@56: %empty  */
#line 2089 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                       { defData->dumb_mode = 2; defData->no_num = 2; }
#line 6089 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 356: /* comp_routehalo: '+' K_ROUTEHALO NUMBER $@56 T_STRING T_STRING  */
#line 2090 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->ComponentCbk) {
             if (defData->componentWarnings++ < defData->settings->ComponentWarnings) {
                defData->defMsg = (char*)malloc(10000);
                sprintf (defData->defMsg,
                  "The ROUTEHALO is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                defData->defError(6551, defData->defMsg);
                free(defData->defMsg);
                CHKERR();
             }
           }
        } else {
           if (defData->callbacks->ComponentCbk)
             defData->Component.setRouteHalo(
                            (int)(yyvsp[-3].dval), (yyvsp[-1].string), (yyvsp[0].string));
        }
      }
#line 6112 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 357: /* $@57: %empty  */
#line 2109 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                              { defData->dumb_mode = DEF_MAX_INT; }
#line 6118 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 358: /* comp_property: '+' K_PROPERTY $@57 comp_prop_list  */
#line 2111 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 6124 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 361: /* comp_prop: T_STRING NUMBER  */
#line 2118 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            char propTp;
            char* str = defData->ringCopy("                       ");
            propTp = defData->session->CompProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "COMPONENT");
            sprintf(str, "%g", (yyvsp[0].dval));
            defData->Component.addNumProperty((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
          }
        }
#line 6139 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 362: /* comp_prop: T_STRING QSTRING  */
#line 2129 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            char propTp;
            propTp = defData->session->CompProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "COMPONENT");
            defData->Component.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 6152 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 363: /* comp_prop: T_STRING T_STRING  */
#line 2138 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            char propTp;
            propTp = defData->session->CompProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "COMPONENT");
            defData->Component.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 6165 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 364: /* comp_region_start: '+' K_REGION  */
#line 2148 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6171 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 365: /* $@58: %empty  */
#line 2150 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                            { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6177 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 366: /* comp_foreign: '+' K_FOREIGN $@58 T_STRING opt_paren orient  */
#line 2152 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->VersionNum < 5.6) {
            if (defData->callbacks->ComponentCbk) {
              defData->Component.setForeignName((yyvsp[-2].string));
              defData->Component.setForeignLocation((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
            }
         } else
            if (defData->callbacks->ComponentCbk)
              if (defData->componentWarnings++ < defData->settings->ComponentWarnings)
                defData->defWarning(7021, "The FOREIGN statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement.");
         }
#line 6193 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 367: /* opt_paren: pt  */
#line 2166 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { (yyval.pt) = (yyvsp[0].pt); }
#line 6199 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 368: /* opt_paren: NUMBER NUMBER  */
#line 2168 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
         { (yyval.pt).x = round((yyvsp[-1].dval)); (yyval.pt).y = round((yyvsp[0].dval)); }
#line 6205 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 369: /* comp_type: placement_status pt orient  */
#line 2171 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            defData->Component.setPlacementStatus((yyvsp[-2].integer));
            defData->Component.setPlacementLocation((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
          }
        }
#line 6216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 370: /* comp_type: '+' K_UNPLACED  */
#line 2178 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            defData->Component.setPlacementStatus(
                                         DEFI_COMPONENT_UNPLACED);
            defData->Component.setPlacementLocation(-1, -1, -1);
	  }
        }
#line 6228 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 371: /* comp_type: '+' K_UNPLACED pt orient  */
#line 2186 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.4) {   // PCR 495463 
            if (defData->callbacks->ComponentCbk) {
              defData->Component.setPlacementStatus(
                                          DEFI_COMPONENT_UNPLACED);
              defData->Component.setPlacementLocation((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
            }
          } else {
            if (defData->componentWarnings++ < defData->settings->ComponentWarnings)
               defData->defWarning(7022, "In the COMPONENT UNPLACED statement, point and orient are invalid in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
#line 6245 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 372: /* $@59: %empty  */
#line 2200 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                           { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6251 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 373: /* maskShift: '+' K_MASKSHIFT $@59 T_STRING  */
#line 2201 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {  
          if (defData->callbacks->ComponentCbk) {
            if (defData->validateMaskShiftInput((yyvsp[0].string), defData->componentWarnings, defData->settings->ComponentWarnings)) {
                defData->Component.setMaskShift((yyvsp[0].string));
            }
          }
        }
#line 6263 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 374: /* placement_status: '+' K_FIXED  */
#line 2210 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = DEFI_COMPONENT_FIXED; }
#line 6269 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 375: /* placement_status: '+' K_COVER  */
#line 2212 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = DEFI_COMPONENT_COVER; }
#line 6275 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 376: /* placement_status: '+' K_PLACED  */
#line 2214 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = DEFI_COMPONENT_PLACED; }
#line 6281 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 377: /* weight: '+' K_WEIGHT NUMBER  */
#line 2217 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setWeight(round((yyvsp[0].dval)));
        }
#line 6290 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 378: /* end_comps: K_END K_COMPS  */
#line 2223 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->ComponentCbk)
            CALLBACK(defData->callbacks->ComponentEndCbk, defrComponentEndCbkType, 0);
        }
#line 6299 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 380: /* start_nets: K_NETS NUMBER ';'  */
#line 2232 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->NetStartCbk)
            CALLBACK(defData->callbacks->NetStartCbk, defrNetStartCbkType, round((yyvsp[-1].dval)));
          defData->netOsnet = 1;
        }
#line 6309 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 383: /* one_net: net_and_connections net_options ';'  */
#line 2243 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->NetCbk)
            CALLBACK(defData->callbacks->NetCbk, defrNetCbkType, &defData->Net);
        }
#line 6318 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 384: /* net_and_connections: net_start  */
#line 2254 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {defData->dumb_mode = 0; defData->no_num = 0; }
#line 6324 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 385: /* $@60: %empty  */
#line 2257 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT; defData->nondef_is_keyword = true; defData->mustjoin_is_keyword = true;}
#line 6330 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 387: /* $@61: %empty  */
#line 2260 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          // 9/22/1999 
          // this is shared by both net and special net 
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.setName((yyvsp[0].string));
          if (defData->callbacks->NetNameCbk)
            CALLBACK(defData->callbacks->NetNameCbk, defrNetNameCbkType, (yyvsp[0].string));
        }
#line 6343 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 389: /* $@62: %empty  */
#line 2268 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                  {defData->dumb_mode = 1; defData->no_num = 1;}
#line 6349 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 390: /* net_name: K_MUSTJOIN '(' T_STRING $@62 T_STRING ')'  */
#line 2269 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addMustPin((yyvsp[-3].string), (yyvsp[-1].string), 0);
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 6360 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 393: /* $@63: %empty  */
#line 2280 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                             {defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT;}
#line 6366 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 394: /* net_connection: '(' T_STRING $@63 T_STRING conn_opt ')'  */
#line 2282 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          // 9/22/1999 
          // since the code is shared by both net & special net, 
          // need to check on both flags 
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addPin((yyvsp[-4].string), (yyvsp[-2].string), (yyvsp[-1].integer));
          // 1/14/2000 - pcr 289156 
          // reset defData->dumb_mode & defData->no_num to 3 , just in case 
          // the next statement is another net_connection 
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 6383 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 395: /* $@64: %empty  */
#line 2294 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  {defData->dumb_mode = 1; defData->no_num = 1;}
#line 6389 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 396: /* net_connection: '(' '*' $@64 T_STRING conn_opt ')'  */
#line 2295 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addPin("*", (yyvsp[-2].string), (yyvsp[-1].integer));
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 6400 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 397: /* $@65: %empty  */
#line 2301 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    {defData->dumb_mode = 1; defData->no_num = 1;}
#line 6406 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 398: /* net_connection: '(' K_PIN $@65 T_STRING conn_opt ')'  */
#line 2302 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addPin("PIN", (yyvsp[-2].string), (yyvsp[-1].integer));
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 6417 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 399: /* conn_opt: %empty  */
#line 2310 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { (yyval.integer) = 0; }
#line 6423 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 400: /* conn_opt: extension_stmt  */
#line 2312 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->NetConnectionExtCbk)
            CALLBACK(defData->callbacks->NetConnectionExtCbk, defrNetConnectionExtCbkType,
              &defData->History_text[0]);
          (yyval.integer) = 0;
        }
#line 6434 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 401: /* conn_opt: '+' K_SYNTHESIZED  */
#line 2319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = 1; }
#line 6440 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 404: /* $@66: %empty  */
#line 2328 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {  
          if (defData->callbacks->NetCbk) defData->Net.addWire((yyvsp[0].string), NULL);
        }
#line 6448 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 405: /* net_option: '+' net_type $@66 paths  */
#line 2332 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          defData->by_is_keyword = false;
          defData->do_is_keyword = false;
          defData->new_is_keyword = false;
          defData->nondef_is_keyword = false;
          defData->mustjoin_is_keyword = false;
          defData->step_is_keyword = false;
          defData->orient_is_keyword = false;
          defData->virtual_is_keyword = false;
          defData->rect_is_keyword = false;
          defData->mask_is_keyword = false;
          defData->needNPCbk = 0;
        }
#line 6466 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 406: /* net_option: '+' K_SOURCE netsource_type  */
#line 2347 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setSource((yyvsp[0].string)); }
#line 6472 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 407: /* net_option: '+' K_FIXEDBUMP  */
#line 2350 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.5) {
            if (defData->callbacks->NetCbk) {
              if (defData->netWarnings++ < defData->settings->NetWarnings) {
                 defData->defMsg = (char*)malloc(1000);
                 sprintf (defData->defMsg,
                    "The FIXEDBUMP statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                 defData->defError(6530, defData->defMsg);
                 free(defData->defMsg);
                 CHKERR();
              }
            }
          }
          if (defData->callbacks->NetCbk) defData->Net.setFixedbump();
        }
#line 6492 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 408: /* $@67: %empty  */
#line 2366 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                          { defData->real_num = 1; }
#line 6498 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 409: /* net_option: '+' K_FREQUENCY $@67 NUMBER  */
#line 2367 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.5) {
            if (defData->callbacks->NetCbk) {
              if (defData->netWarnings++ < defData->settings->NetWarnings) {
                 defData->defMsg = (char*)malloc(1000);
                 sprintf (defData->defMsg,
                    "The FREQUENCY statement is a version 5.5 and later syntax.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                 defData->defError(6558, defData->defMsg);
                 free(defData->defMsg);
                 CHKERR();
              }
            }
          }
          if (defData->callbacks->NetCbk) defData->Net.setFrequency((yyvsp[0].dval));
          defData->real_num = 0;
        }
#line 6519 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 410: /* $@68: %empty  */
#line 2384 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode = 1; defData->no_num = 1;}
#line 6525 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 411: /* net_option: '+' K_ORIGINAL $@68 T_STRING  */
#line 2385 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setOriginal((yyvsp[0].string)); }
#line 6531 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 412: /* net_option: '+' K_PATTERN pattern_type  */
#line 2388 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setPattern((yyvsp[0].string)); }
#line 6537 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 413: /* net_option: '+' K_WEIGHT NUMBER  */
#line 2391 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setWeight(round((yyvsp[0].dval))); }
#line 6543 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 414: /* net_option: '+' K_XTALK NUMBER  */
#line 2394 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setXTalk(round((yyvsp[0].dval))); }
#line 6549 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 415: /* net_option: '+' K_ESTCAP NUMBER  */
#line 2397 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setCap((yyvsp[0].dval)); }
#line 6555 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 416: /* net_option: '+' K_USE use_type  */
#line 2400 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setUse((yyvsp[0].string)); }
#line 6561 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 417: /* net_option: '+' K_STYLE NUMBER  */
#line 2403 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setStyle((int)(yyvsp[0].dval)); }
#line 6567 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 418: /* $@69: %empty  */
#line 2405 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                               { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6573 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 419: /* net_option: '+' K_NONDEFAULTRULE $@69 T_STRING  */
#line 2406 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->NetCbk && defData->callbacks->NetNonDefaultRuleCbk) {
             // User wants a callback on nondefaultrule 
             CALLBACK(defData->callbacks->NetNonDefaultRuleCbk,
                      defrNetNonDefaultRuleCbkType, (yyvsp[0].string));
          }
          // Still save data in the class 
          if (defData->callbacks->NetCbk) defData->Net.setNonDefaultRule((yyvsp[0].string));
        }
#line 6587 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 421: /* $@70: %empty  */
#line 2418 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                          { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6593 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 422: /* net_option: '+' K_SHIELDNET $@70 T_STRING  */
#line 2419 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addShieldNet((yyvsp[0].string)); }
#line 6599 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 423: /* $@71: %empty  */
#line 2421 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6605 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 424: /* $@72: %empty  */
#line 2422 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { // since the parser still support 5.3 and earlier, can't 
          // move NOSHIELD in net_type 
          if (defData->VersionNum < 5.4) {   // PCR 445209 
            if (defData->callbacks->NetCbk) defData->Net.addNoShield("");
            defData->by_is_keyword = false;
            defData->do_is_keyword = false;
            defData->new_is_keyword = false;
            defData->step_is_keyword = false;
            defData->orient_is_keyword = false;
            defData->virtual_is_keyword = false;
            defData->mask_is_keyword = false;
            defData->rect_is_keyword = false;
            defData->shield = true;    // save the path info in the defData->shield paths 
          } else
            if (defData->callbacks->NetCbk) defData->Net.addWire("NOSHIELD", NULL);
        }
#line 6626 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 425: /* net_option: '+' K_NOSHIELD $@71 $@72 paths  */
#line 2439 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.4) {   // PCR 445209 
            defData->shield = false;
            defData->by_is_keyword = false;
            defData->do_is_keyword = false;
            defData->new_is_keyword = false;
            defData->step_is_keyword = false;
            defData->nondef_is_keyword = false;
            defData->mustjoin_is_keyword = false;
            defData->orient_is_keyword = false;
            defData->virtual_is_keyword = false;
            defData->rect_is_keyword = false;
            defData->mask_is_keyword = false;
          } else {
            defData->by_is_keyword = false;
            defData->do_is_keyword = false;
            defData->new_is_keyword = false;
            defData->step_is_keyword = false;
            defData->nondef_is_keyword = false;
            defData->mustjoin_is_keyword = false;
            defData->orient_is_keyword = false;
            defData->virtual_is_keyword = false;
            defData->rect_is_keyword = false;
            defData->mask_is_keyword = false;
          }
          defData->needNPCbk = 0;
        }
#line 6658 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 426: /* $@73: %empty  */
#line 2468 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { defData->dumb_mode = 1; defData->no_num = 1;
          if (defData->callbacks->NetCbk) {
            defData->Subnet = new defiSubnet(defData);
          }
        }
#line 6668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 427: /* $@74: %empty  */
#line 2473 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                 {
          if (defData->callbacks->NetCbk && defData->callbacks->NetSubnetNameCbk) {
            // User wants a callback on Net subnetName 
            CALLBACK(defData->callbacks->NetSubnetNameCbk, defrNetSubnetNameCbkType, (yyvsp[0].string));
          }
          // Still save the subnet name in the class 
          if (defData->callbacks->NetCbk) {
            defData->Subnet->setName((yyvsp[0].string));
          }
        }
#line 6683 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 428: /* $@75: %empty  */
#line 2483 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                   {
          defData->routed_is_keyword = true;
          defData->fixed_is_keyword = true;
          defData->cover_is_keyword = true;
        }
#line 6693 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 429: /* net_option: '+' K_SUBNET $@73 T_STRING $@74 comp_names $@75 subnet_options  */
#line 2487 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {
          if (defData->callbacks->NetCbk) {
            defData->Net.addSubnet(defData->Subnet);
            defData->Subnet = NULL;
            defData->routed_is_keyword = false;
            defData->fixed_is_keyword = false;
            defData->cover_is_keyword = false;
          }
        }
#line 6707 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 430: /* $@76: %empty  */
#line 2497 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode = DEF_MAX_INT; }
#line 6713 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 431: /* net_option: '+' K_PROPERTY $@76 net_prop_list  */
#line 2499 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { defData->dumb_mode = 0; }
#line 6719 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 432: /* net_option: extension_stmt  */
#line 2502 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { 
          if (defData->callbacks->NetExtCbk)
            CALLBACK(defData->callbacks->NetExtCbk, defrNetExtCbkType, &defData->History_text[0]);
        }
#line 6728 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 435: /* net_prop: T_STRING NUMBER  */
#line 2512 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->NetCbk) {
            char propTp;
            char* str = defData->ringCopy("                       ");
            propTp = defData->session->NetProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "NET");
            sprintf(str, "%g", (yyvsp[0].dval));
            defData->Net.addNumProp((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
          }
        }
#line 6743 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 436: /* net_prop: T_STRING QSTRING  */
#line 2523 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->NetCbk) {
            char propTp;
            propTp = defData->session->NetProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "NET");
            defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 6756 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 437: /* net_prop: T_STRING T_STRING  */
#line 2532 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->callbacks->NetCbk) {
            char propTp;
            propTp = defData->session->NetProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "NET");
            defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 6769 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 438: /* netsource_type: K_NETLIST  */
#line 2542 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"NETLIST"; }
#line 6775 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 439: /* netsource_type: K_DIST  */
#line 2544 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"DIST"; }
#line 6781 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 440: /* netsource_type: K_USER  */
#line 2546 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"USER"; }
#line 6787 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 441: /* netsource_type: K_TIMING  */
#line 2548 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"TIMING"; }
#line 6793 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 442: /* netsource_type: K_TEST  */
#line 2550 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"TEST"; }
#line 6799 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 443: /* $@77: %empty  */
#line 2553 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          // vpin_options may have to deal with orient 
          defData->orient_is_keyword = true;
        }
#line 6808 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 444: /* vpin_stmt: vpin_begin vpin_layer_opt pt pt $@77 vpin_options  */
#line 2558 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk)
            defData->Net.addVpinBounds((yyvsp[-3].pt).x, (yyvsp[-3].pt).y, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y);
          defData->orient_is_keyword = false;
        }
#line 6817 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 445: /* $@78: %empty  */
#line 2563 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       {defData->dumb_mode = 1; defData->no_num = 1;}
#line 6823 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 446: /* vpin_begin: '+' K_VPIN $@78 T_STRING  */
#line 2564 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addVpin((yyvsp[0].string)); }
#line 6829 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 448: /* $@79: %empty  */
#line 2567 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  {defData->dumb_mode=1;}
#line 6835 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 449: /* vpin_layer_opt: K_LAYER $@79 T_STRING  */
#line 2568 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addVpinLayer((yyvsp[0].string)); }
#line 6841 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 451: /* vpin_options: vpin_status pt orient  */
#line 2572 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addVpinLoc((yyvsp[-2].string), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer)); }
#line 6847 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 452: /* vpin_status: K_PLACED  */
#line 2575 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"PLACED"; }
#line 6853 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 453: /* vpin_status: K_FIXED  */
#line 2577 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"FIXED"; }
#line 6859 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 454: /* vpin_status: K_COVER  */
#line 2579 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"COVER"; }
#line 6865 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 455: /* net_type: K_FIXED  */
#line 2582 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"FIXED"; defData->dumb_mode = 1; }
#line 6871 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 456: /* net_type: K_COVER  */
#line 2584 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"COVER"; defData->dumb_mode = 1; }
#line 6877 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 457: /* net_type: K_ROUTED  */
#line 2586 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.string) = (char*)"ROUTED"; defData->dumb_mode = 1; }
#line 6883 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 458: /* paths: path  */
#line 2590 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->NetCbk)
          defData->pathIsDone(defData->shield, 0, defData->netOsnet, &defData->needNPCbk);
      }
#line 6891 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 459: /* paths: paths new_path  */
#line 2594 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 6897 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 460: /* $@80: %empty  */
#line 2596 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                { defData->dumb_mode = 1; }
#line 6903 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 461: /* new_path: K_NEW $@80 path  */
#line 2597 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->NetCbk)
          defData->pathIsDone(defData->shield, 0, defData->netOsnet, &defData->needNPCbk);
      }
#line 6911 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 462: /* $@81: %empty  */
#line 2602 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if ((strcmp((yyvsp[0].string), "TAPER") == 0) || (strcmp((yyvsp[0].string), "TAPERRULE") == 0)) {
          if (defData->NeedPathData && defData->callbacks->NetCbk) {
            if (defData->netWarnings++ < defData->settings->NetWarnings) {
              defData->defError(6531, "The layerName which is required in path is missing. Include the layerName in the path and then try again.");
              CHKERR();
            }
          }
          // Since there is already error, the next token is insignificant 
          defData->dumb_mode = 1; defData->no_num = 1;
        } else {
          // CCR 766289 - Do not accummulate the layer information if there 
          // is not a callback set 
          if (defData->NeedPathData && defData->callbacks->NetCbk)
              defData->PathObj.addLayer((yyvsp[0].string));
          defData->dumb_mode = 0; defData->no_num = 0;
        }
      }
#line 6934 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 463: /* $@82: %empty  */
#line 2621 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = DEF_MAX_INT; defData->by_is_keyword = true; defData->do_is_keyword = true;
/*
       dumb_mode = 1; by_is_keyword = true; do_is_keyword = true;
*/
        defData->new_is_keyword = true; defData->step_is_keyword = true; 
        defData->orient_is_keyword = true; defData->virtual_is_keyword = true;
        defData->mask_is_keyword = true, defData->rect_is_keyword = true;  }
#line 6946 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 464: /* path: T_STRING $@81 opt_taper_style_s path_pt $@82 path_item_list  */
#line 2631 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0;   defData->virtual_is_keyword = false; defData->mask_is_keyword = false,
       defData->rect_is_keyword = false; }
#line 6953 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 465: /* virtual_statement: K_VIRTUAL virtual_pt  */
#line 2636 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
      if (defData->VersionNum < 5.8) {
              if (defData->callbacks->SNetCbk) {
                if (defData->sNetWarnings++ < defData->settings->SNetWarnings) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The VIRTUAL statement is available in version 5.8 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                  defData->defError(6536, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
          }
    }
#line 6972 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 466: /* rect_statement: K_RECT rect_pts  */
#line 2653 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
      if (defData->VersionNum < 5.8) {
              if (defData->callbacks->SNetCbk) {
                if (defData->sNetWarnings++ < defData->settings->SNetWarnings) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The RECT statement is available in version 5.8 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                  defData->defError(6536, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
      }
    }
#line 6991 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 469: /* path_item: T_STRING  */
#line 2676 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          if (strcmp((yyvsp[0].string), "TAPER") == 0)
            defData->PathObj.setTaper();
          else {
            defData->PathObj.addVia((yyvsp[0].string));
            }
        }
      }
#line 7006 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 470: /* path_item: K_MASK NUMBER T_STRING  */
#line 2687 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->validateMaskInput((int)(yyvsp[-1].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
            if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
                (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
              if (strcmp((yyvsp[0].string), "TAPER") == 0)
                defData->PathObj.setTaper();
              else {
                defData->PathObj.addViaMask((yyvsp[-1].dval));
                defData->PathObj.addVia((yyvsp[0].string));
                }
            }
        }
      }
#line 7024 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 471: /* path_item: T_STRING orient  */
#line 2701 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
            defData->PathObj.addVia((yyvsp[-1].string));
            defData->PathObj.addViaRotation((yyvsp[0].integer));
        }
      }
#line 7035 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 472: /* path_item: K_MASK NUMBER T_STRING orient  */
#line 2708 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->validateMaskInput((int)(yyvsp[-2].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
            if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
                (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
                defData->PathObj.addViaMask((yyvsp[-2].dval));
                defData->PathObj.addVia((yyvsp[-1].string));
                defData->PathObj.addViaRotation((yyvsp[0].integer));
            }
        }
      }
#line 7050 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 473: /* path_item: K_MASK NUMBER T_STRING K_DO NUMBER K_BY NUMBER K_STEP NUMBER NUMBER  */
#line 2719 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->validateMaskInput((int)(yyvsp[-8].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {      
            if (((yyvsp[-5].dval) == 0) || ((yyvsp[-3].dval) == 0)) {
              if (defData->NeedPathData &&
                  defData->callbacks->SNetCbk) {
                if (defData->netWarnings++ < defData->settings->NetWarnings) {
                  defData->defError(6533, "Either the numX or numY in the VIA DO statement has the value. The value specified is 0.\nUpdate your DEF file with the correct value and then try again.\n");
                  CHKERR();
                }
              }
            }
            if (defData->NeedPathData && (defData->callbacks->SNetCbk && (defData->netOsnet==2))) {
                defData->PathObj.addViaMask((yyvsp[-8].dval));
                defData->PathObj.addVia((yyvsp[-7].string));
                defData->PathObj.addViaData((int)(yyvsp[-5].dval), (int)(yyvsp[-3].dval), (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
            }  else if (defData->NeedPathData && (defData->callbacks->NetCbk && (defData->netOsnet==1))) {
              if (defData->netWarnings++ < defData->settings->NetWarnings) {
                defData->defError(6567, "The VIA DO statement is defined in the NET statement and is invalid.\nRemove this statement from your DEF file and try again.");
                CHKERR();
              }
            }
        }
      }
#line 7078 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 474: /* path_item: T_STRING K_DO NUMBER K_BY NUMBER K_STEP NUMBER NUMBER  */
#line 2743 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.5) {
          if (defData->NeedPathData && 
              defData->callbacks->SNetCbk) {
            if (defData->netWarnings++ < defData->settings->NetWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The VIA DO statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
              defData->defError(6532, defData->defMsg);
              free(defData->defMsg);
              CHKERR();
            }
          }
        }
        if (((yyvsp[-5].dval) == 0) || ((yyvsp[-3].dval) == 0)) {
          if (defData->NeedPathData &&
              defData->callbacks->SNetCbk) {
            if (defData->netWarnings++ < defData->settings->NetWarnings) {
              defData->defError(6533, "Either the numX or numY in the VIA DO statement has the value. The value specified is 0.\nUpdate your DEF file with the correct value and then try again.\n");
              CHKERR();
            }
          }
        }
        if (defData->NeedPathData && (defData->callbacks->SNetCbk && (defData->netOsnet==2))) {
            defData->PathObj.addVia((yyvsp[-7].string));
            defData->PathObj.addViaData((int)(yyvsp[-5].dval), (int)(yyvsp[-3].dval), (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
        }  else if (defData->NeedPathData && (defData->callbacks->NetCbk && (defData->netOsnet==1))) {
          if (defData->netWarnings++ < defData->settings->NetWarnings) {
            defData->defError(6567, "The VIA DO statement is defined in the NET statement and is invalid.\nRemove this statement from your DEF file and try again.");
            CHKERR();
          }
        }
      }
#line 7116 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 475: /* path_item: T_STRING orient K_DO NUMBER K_BY NUMBER K_STEP NUMBER NUMBER  */
#line 2777 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.5) {
          if (defData->NeedPathData &&
              defData->callbacks->SNetCbk) {
            if (defData->netWarnings++ < defData->settings->NetWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The VIA DO statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
              defData->defError(6532, defData->defMsg);
              CHKERR();
            }
          }
        }
        if (((yyvsp[-5].dval) == 0) || ((yyvsp[-3].dval) == 0)) {
          if (defData->NeedPathData &&
              defData->callbacks->SNetCbk) {
            if (defData->netWarnings++ < defData->settings->NetWarnings) {
              defData->defError(6533, "Either the numX or numY in the VIA DO statement has the value. The value specified is 0.\nUpdate your DEF file with the correct value and then try again.\n");
              CHKERR();
            }
          }
        }
        if (defData->NeedPathData && (defData->callbacks->SNetCbk && (defData->netOsnet==2))) {
            defData->PathObj.addVia((yyvsp[-8].string));
            defData->PathObj.addViaRotation((yyvsp[-7].integer));
            defData->PathObj.addViaData((int)(yyvsp[-5].dval), (int)(yyvsp[-3].dval), (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
        } else if (defData->NeedPathData && (defData->callbacks->NetCbk && (defData->netOsnet==1))) {
          if (defData->netWarnings++ < defData->settings->NetWarnings) {
            defData->defError(6567, "The VIA DO statement is defined in the NET statement and is invalid.\nRemove this statement from your DEF file and try again.");
            CHKERR();
          }
        }
      }
#line 7154 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 476: /* path_item: K_MASK NUMBER T_STRING orient K_DO NUMBER K_BY NUMBER K_STEP NUMBER NUMBER  */
#line 2811 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->validateMaskInput((int)(yyvsp[-9].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
            if (((yyvsp[-5].dval) == 0) || ((yyvsp[-3].dval) == 0)) {
              if (defData->NeedPathData &&
                  defData->callbacks->SNetCbk) {
                if (defData->netWarnings++ < defData->settings->NetWarnings) {
                  defData->defError(6533, "Either the numX or numY in the VIA DO statement has the value. The value specified is 0.\nUpdate your DEF file with the correct value and then try again.\n");
                  CHKERR();
                }
              }
            }
            if (defData->NeedPathData && (defData->callbacks->SNetCbk && (defData->netOsnet==2))) {
                defData->PathObj.addViaMask((yyvsp[-9].dval)); 
                defData->PathObj.addVia((yyvsp[-8].string));
                defData->PathObj.addViaRotation((yyvsp[-7].integer));;
                defData->PathObj.addViaData((int)(yyvsp[-5].dval), (int)(yyvsp[-3].dval), (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
            } else if (defData->NeedPathData && (defData->callbacks->NetCbk && (defData->netOsnet==1))) {
              if (defData->netWarnings++ < defData->settings->NetWarnings) {
                defData->defError(6567, "The VIA DO statement is defined in the NET statement and is invalid.\nRemove this statement from your DEF file and try again.");
                CHKERR();
              }
            }
        }
      }
#line 7183 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 479: /* $@83: %empty  */
#line 2837 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                          { defData->dumb_mode = 6; }
#line 7189 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 480: /* path_item: K_MASK NUMBER K_RECT $@83 '(' NUMBER NUMBER NUMBER NUMBER ')'  */
#line 2838 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
      if (defData->validateMaskInput((int)(yyvsp[-8].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          defData->PathObj.addMask((yyvsp[-8].dval));
          defData->PathObj.addViaRect((yyvsp[-4].dval), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval));
        }
      }
    }
#line 7203 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 481: /* $@84: %empty  */
#line 2848 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
       if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          defData->PathObj.addMask((yyvsp[0].dval)); 
        }
       }  
    }
#line 7216 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 483: /* path_item: path_pt  */
#line 2858 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
       // reset defData->dumb_mode to 1 just incase the next token is a via of the path
        // 2/5/2004 - pcr 686781
        defData->dumb_mode = DEF_MAX_INT; defData->by_is_keyword = true; defData->do_is_keyword = true;
        defData->new_is_keyword = true; defData->step_is_keyword = true;
        defData->orient_is_keyword = true;
    }
#line 7228 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 484: /* path_pt: '(' NUMBER NUMBER ')'  */
#line 2869 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(round((yyvsp[-2].dval)), round((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-2].dval);
        defData->save_y = (yyvsp[-1].dval); 
      }
#line 7240 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 485: /* path_pt: '(' '*' NUMBER ')'  */
#line 2877 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(round(defData->save_x), round((yyvsp[-1].dval))); 
        defData->save_y = (yyvsp[-1].dval);
      }
#line 7251 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 486: /* path_pt: '(' NUMBER '*' ')'  */
#line 2884 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(round((yyvsp[-2].dval)), round(defData->save_y)); 
        defData->save_x = (yyvsp[-2].dval);
      }
#line 7262 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 487: /* path_pt: '(' '*' '*' ')'  */
#line 2891 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(round(defData->save_x), round(defData->save_y)); 
      }
#line 7272 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 488: /* path_pt: '(' NUMBER NUMBER NUMBER ')'  */
#line 2897 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(round((yyvsp[-3].dval)), round((yyvsp[-2].dval)), round((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-3].dval);
        defData->save_y = (yyvsp[-2].dval);
      }
#line 7284 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 489: /* path_pt: '(' '*' NUMBER NUMBER ')'  */
#line 2905 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(round(defData->save_x), round((yyvsp[-2].dval)),
          round((yyvsp[-1].dval))); 
        defData->save_y = (yyvsp[-2].dval);
      }
#line 7296 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 490: /* path_pt: '(' NUMBER '*' NUMBER ')'  */
#line 2913 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(round((yyvsp[-3].dval)), round(defData->save_y),
          round((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-3].dval);
      }
#line 7308 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 491: /* path_pt: '(' '*' '*' NUMBER ')'  */
#line 2921 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(round(defData->save_x), round(defData->save_y),
          round((yyvsp[-1].dval))); 
      }
#line 7319 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 492: /* virtual_pt: '(' NUMBER NUMBER ')'  */
#line 2930 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(round((yyvsp[-2].dval)), round((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-2].dval);
        defData->save_y = (yyvsp[-1].dval);
      }
#line 7331 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 493: /* virtual_pt: '(' '*' NUMBER ')'  */
#line 2938 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(round(defData->save_x), round((yyvsp[-1].dval))); 
        defData->save_y = (yyvsp[-1].dval);
      }
#line 7342 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 494: /* virtual_pt: '(' NUMBER '*' ')'  */
#line 2945 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(round((yyvsp[-2].dval)), round(defData->save_y)); 
        defData->save_x = (yyvsp[-2].dval);
      }
#line 7353 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 495: /* virtual_pt: '(' '*' '*' ')'  */
#line 2952 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(round(defData->save_x), round(defData->save_y));
      }
#line 7363 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 496: /* rect_pts: '(' NUMBER NUMBER NUMBER NUMBER ')'  */
#line 2960 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          defData->PathObj.addViaRect((yyvsp[-4].dval), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval)); 
        }    
    }
#line 7374 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 501: /* opt_taper: K_TAPER  */
#line 2976 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.setTaper(); }
#line 7382 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 502: /* $@85: %empty  */
#line 2979 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1; }
#line 7388 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 503: /* opt_taper: K_TAPERRULE $@85 T_STRING  */
#line 2980 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addTaperRule((yyvsp[0].string)); }
#line 7396 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 504: /* opt_style: K_STYLE NUMBER  */
#line 2985 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->VersionNum < 5.6) {
           if (defData->NeedPathData && (defData->callbacks->NetCbk ||
               defData->callbacks->SNetCbk)) {
             if (defData->netWarnings++ < defData->settings->NetWarnings) {
               defData->defMsg = (char*)malloc(1000);
               sprintf (defData->defMsg,
                  "The STYLE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
               defData->defError(6534, defData->defMsg);
               free(defData->defMsg);
               CHKERR();
             }
           }
        } else
           if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
             (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
             defData->PathObj.addStyle((int)(yyvsp[0].dval));
      }
#line 7419 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 507: /* opt_shape_style: '+' K_SHAPE shape_type  */
#line 3010 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addShape((yyvsp[0].string)); }
#line 7427 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 508: /* opt_shape_style: '+' K_STYLE NUMBER  */
#line 3014 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->VersionNum < 5.6) {
          if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
            if (defData->netWarnings++ < defData->settings->NetWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The STYLE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
              defData->defError(6534, defData->defMsg);
              free(defData->defMsg);
              CHKERR();
            }
          }
        } else {
          if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
            defData->PathObj.addStyle((int)(yyvsp[0].dval));
        }
      }
#line 7450 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 509: /* end_nets: K_END K_NETS  */
#line 3034 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { 
            CALLBACK(defData->callbacks->NetEndCbk, defrNetEndCbkType, 0);
            defData->netOsnet = 0;
          }
#line 7459 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 510: /* shape_type: K_RING  */
#line 3040 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"RING"; }
#line 7465 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 511: /* shape_type: K_STRIPE  */
#line 3042 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"STRIPE"; }
#line 7471 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 512: /* shape_type: K_FOLLOWPIN  */
#line 3044 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"FOLLOWPIN"; }
#line 7477 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 513: /* shape_type: K_IOWIRE  */
#line 3046 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"IOWIRE"; }
#line 7483 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 514: /* shape_type: K_COREWIRE  */
#line 3048 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"COREWIRE"; }
#line 7489 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 515: /* shape_type: K_BLOCKWIRE  */
#line 3050 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"BLOCKWIRE"; }
#line 7495 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 516: /* shape_type: K_FILLWIRE  */
#line 3052 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"FILLWIRE"; }
#line 7501 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 517: /* shape_type: K_FILLWIREOPC  */
#line 3054 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->VersionNum < 5.7) {
                 if (defData->NeedPathData) {
                   if (defData->fillWarnings++ < defData->settings->FillWarnings) {
                     defData->defMsg = (char*)malloc(10000);
                     sprintf (defData->defMsg,
                       "The FILLWIREOPC is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                     defData->defError(6552, defData->defMsg);
                     free(defData->defMsg);
                     CHKERR();
                  }
                }
              }
              (yyval.string) = (char*)"FILLWIREOPC";
            }
#line 7521 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 518: /* shape_type: K_DRCFILL  */
#line 3070 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"DRCFILL"; }
#line 7527 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 519: /* shape_type: K_BLOCKAGEWIRE  */
#line 3072 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"BLOCKAGEWIRE"; }
#line 7533 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 520: /* shape_type: K_PADRING  */
#line 3074 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"PADRING"; }
#line 7539 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 521: /* shape_type: K_BLOCKRING  */
#line 3076 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"BLOCKRING"; }
#line 7545 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 525: /* snet_rule: net_and_connections snet_options ';'  */
#line 3086 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { CALLBACK(defData->callbacks->SNetCbk, defrSNetCbkType, &defData->Net); }
#line 7551 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 532: /* snet_other_option: '+' net_type  */
#line 3097 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
             if (defData->VersionNum >= 5.8) {
                defData->specialWire_routeStatus = (yyvsp[0].string);
             } else {
                 if (defData->callbacks->SNetCbk) {   // PCR 902306 
                   defData->defMsg = (char*)malloc(1024);
                   sprintf(defData->defMsg, "The SPECIAL NET statement, with type %s, does not have any net statement defined.\nThe DEF parser will ignore this statemnet.", (yyvsp[0].string));
                   defData->defWarning(7023, defData->defMsg);
                   free(defData->defMsg);
                 }
             }
            }
#line 7568 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 533: /* $@86: %empty  */
#line 3110 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
            if (defData->callbacks->SNetCbk) defData->Net.addWire((yyvsp[0].string), NULL);
            }
#line 7576 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 534: /* snet_other_option: '+' net_type $@86 spaths  */
#line 3114 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
            // 7/17/2003 - Fix for pcr 604848, add a callback for each wire
            if (defData->callbacks->SNetWireCbk) {
               CALLBACK(defData->callbacks->SNetWireCbk, defrSNetWireCbkType, &defData->Net);
               defData->Net.freeWire();
            }
            defData->by_is_keyword = false;
            defData->do_is_keyword = false;
            defData->new_is_keyword = false;
            defData->step_is_keyword = false;
            defData->orient_is_keyword = false;
            defData->virtual_is_keyword = false;
            defData->mask_is_keyword = false;
            defData->rect_is_keyword = false;
            defData->needSNPCbk = 0;
            }
#line 7597 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 535: /* $@87: %empty  */
#line 3131 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 7603 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 536: /* $@88: %empty  */
#line 3132 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { defData->shieldName = (yyvsp[0].string); 
              defData->specialWire_routeStatus = (char*)"SHIELD";
              defData->specialWire_routeStatusName = (yyvsp[0].string); 
            }
#line 7612 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 538: /* snet_other_option: '+' K_SHAPE shape_type  */
#line 3139 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {  
            defData->specialWire_shapeType = (yyvsp[0].string);
          }
#line 7620 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 539: /* snet_other_option: '+' K_MASK NUMBER  */
#line 3143 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
                defData->specialWire_mask = (yyvsp[0].dval);
            }     
          }
#line 7630 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 540: /* $@89: %empty  */
#line 3148 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 7636 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 541: /* $@90: %empty  */
#line 3149 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->SNetCbk) {
                if (defData->sNetWarnings++ < defData->settings->SNetWarnings) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The POLYGON statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                  defData->defError(6535, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            
            defData->Geometries.Reset();
          }
#line 7657 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 542: /* snet_other_option: '+' K_POLYGON $@89 T_STRING $@90 firstPt nextPt nextPt otherPts  */
#line 3166 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum >= 5.6) {  // only add if 5.6 or beyond
              if (defData->callbacks->SNetCbk) {
                // defData->needSNPCbk will indicate that it has reach the max
                // memory and if user has set partialPathCBk, def parser
                // will make the callback.
                // This will improve performance
                // This construct is only in specialnet
                defData->Net.addPolygon((yyvsp[-5].string), &defData->Geometries, &defData->needSNPCbk, defData->specialWire_mask, defData->specialWire_routeStatus, defData->specialWire_shapeType,
                                                                defData->specialWire_routeStatusName);
                defData->specialWire_mask = 0;
                defData->specialWire_routeStatus = (char*)"ROUTED";
                defData->specialWire_shapeType = (char*)"";
                if (defData->needSNPCbk && defData->callbacks->SNetPartialPathCbk) {
                   CALLBACK(defData->callbacks->SNetPartialPathCbk, defrSNetPartialPathCbkType,
                            &defData->Net);
                   defData->Net.clearRectPolyNPath();
                   defData->Net.clearVia();
                }
              }
            }
          }
#line 7684 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 543: /* $@91: %empty  */
#line 3189 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                     { defData->dumb_mode = 1; }
#line 7690 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 544: /* snet_other_option: '+' K_RECT $@91 T_STRING pt pt  */
#line 3190 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.6) {
              if (defData->callbacks->SNetCbk) {
                if (defData->sNetWarnings++ < defData->settings->SNetWarnings) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The RECT statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                  defData->defError(6536, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
            }
            if (defData->callbacks->SNetCbk) {
              // defData->needSNPCbk will indicate that it has reach the max
              // memory and if user has set partialPathCBk, def parser
              // will make the callback.
              // This will improve performance
              // This construct is only in specialnet
              defData->Net.addRect((yyvsp[-2].string), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y, &defData->needSNPCbk, defData->specialWire_mask, defData->specialWire_routeStatus, defData->specialWire_shapeType, defData->specialWire_routeStatusName);
              defData->specialWire_mask = 0;
              defData->specialWire_routeStatus = (char*)"ROUTED";
              defData->specialWire_shapeType = (char*)"";
              defData->specialWire_routeStatusName = (char*)"";
              if (defData->needSNPCbk && defData->callbacks->SNetPartialPathCbk) {
                 CALLBACK(defData->callbacks->SNetPartialPathCbk, defrSNetPartialPathCbkType,
                          &defData->Net);
                 defData->Net.clearRectPolyNPath();
                 defData->Net.clearVia();
              }
            }
          }
#line 7727 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 545: /* $@92: %empty  */
#line 3222 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    { defData->dumb_mode = 1; }
#line 7733 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 546: /* $@93: %empty  */
#line 3223 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.8) {
              if (defData->callbacks->SNetCbk) {
                if (defData->sNetWarnings++ < defData->settings->SNetWarnings) {
                  defData->defMsg = (char*)malloc(1000);
                  sprintf (defData->defMsg,
                     "The VIA statement is available in version 5.8 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
                  defData->defError(6536, defData->defMsg);
                  free(defData->defMsg);
                  CHKERR();
                }
              }
          }
        }
#line 7752 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 547: /* snet_other_option: '+' K_VIA $@92 T_STRING orient_pt $@93 firstPt otherPts  */
#line 3238 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        {
          if (defData->VersionNum >= 5.8 && defData->callbacks->SNetCbk) {
              defData->Net.addPts((yyvsp[-4].string), (yyvsp[-3].integer), &defData->Geometries, &defData->needSNPCbk, defData->specialWire_mask, defData->specialWire_routeStatus, defData->specialWire_shapeType,
                                                          defData->specialWire_routeStatusName);
              defData->specialWire_mask = 0;
              defData->specialWire_routeStatus = (char*)"ROUTED";
              defData->specialWire_shapeType = (char*)"";
              defData->specialWire_routeStatusName = (char*)"";
              if (defData->needSNPCbk && defData->callbacks->SNetPartialPathCbk) {
                 CALLBACK(defData->callbacks->SNetPartialPathCbk, defrSNetPartialPathCbkType,
                          &defData->Net);
                 defData->Net.clearRectPolyNPath();
                 defData->Net.clearVia();
              }
            }
        }
#line 7773 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 548: /* snet_other_option: '+' K_SOURCE source_type  */
#line 3256 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setSource((yyvsp[0].string)); }
#line 7779 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 549: /* snet_other_option: '+' K_FIXEDBUMP  */
#line 3259 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setFixedbump(); }
#line 7785 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 550: /* snet_other_option: '+' K_FREQUENCY NUMBER  */
#line 3262 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setFrequency((yyvsp[0].dval)); }
#line 7791 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 551: /* $@94: %empty  */
#line 3264 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode = 1; defData->no_num = 1;}
#line 7797 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 552: /* snet_other_option: '+' K_ORIGINAL $@94 T_STRING  */
#line 3265 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setOriginal((yyvsp[0].string)); }
#line 7803 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 553: /* snet_other_option: '+' K_PATTERN pattern_type  */
#line 3268 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setPattern((yyvsp[0].string)); }
#line 7809 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 554: /* snet_other_option: '+' K_WEIGHT NUMBER  */
#line 3271 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setWeight(round((yyvsp[0].dval))); }
#line 7815 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 555: /* snet_other_option: '+' K_ESTCAP NUMBER  */
#line 3274 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { 
              // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
              if (defData->VersionNum < 5.5) {
                 if (defData->callbacks->SNetCbk) defData->Net.setCap((yyvsp[0].dval));
              } else {
                 defData->defWarning(7024, "The ESTCAP statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
              }
            }
#line 7828 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 556: /* snet_other_option: '+' K_USE use_type  */
#line 3284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setUse((yyvsp[0].string)); }
#line 7834 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 557: /* snet_other_option: '+' K_STYLE NUMBER  */
#line 3287 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setStyle((int)(yyvsp[0].dval)); }
#line 7840 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 558: /* $@95: %empty  */
#line 3289 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode = DEF_MAX_INT; }
#line 7846 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 559: /* snet_other_option: '+' K_PROPERTY $@95 snet_prop_list  */
#line 3291 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { defData->dumb_mode = 0; }
#line 7852 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 560: /* snet_other_option: extension_stmt  */
#line 3294 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
          { CALLBACK(defData->callbacks->NetExtCbk, defrNetExtCbkType, &defData->History_text[0]); }
#line 7858 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 561: /* orient_pt: %empty  */
#line 3297 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
        { (yyval.integer) = 0; }
#line 7864 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 562: /* orient_pt: K_N  */
#line 3298 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 0;}
#line 7870 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 563: /* orient_pt: K_W  */
#line 3299 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 1;}
#line 7876 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 564: /* orient_pt: K_S  */
#line 3300 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 2;}
#line 7882 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 565: /* orient_pt: K_E  */
#line 3301 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 3;}
#line 7888 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 566: /* orient_pt: K_FN  */
#line 3302 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 4;}
#line 7894 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 567: /* orient_pt: K_FW  */
#line 3303 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 5;}
#line 7900 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 568: /* orient_pt: K_FS  */
#line 3304 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 6;}
#line 7906 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 569: /* orient_pt: K_FE  */
#line 3305 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {(yyval.integer) = 7;}
#line 7912 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 570: /* shield_layer: %empty  */
#line 3308 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
                if (defData->callbacks->SNetCbk) {
                    if (defData->VersionNum < 5.8) { 
                        defData->defMsg = (char*)malloc(1024);
                        sprintf(defData->defMsg, "The SPECIAL NET SHIELD statement doesn't have routing points definition.\nWill be ignored.");
                        defData->defWarning(7025, defData->defMsg);
                        free(defData->defMsg);
                    } else {  // CCR 1244433
                      defData->specialWire_routeStatus = (char*)"SHIELD";
                      defData->specialWire_routeStatusName = defData->shieldName;
                    }
                }
            }
#line 7930 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 571: /* $@96: %empty  */
#line 3322 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { // since the parser still supports 5.3 and earlier, 
              // can't just move SHIELD in net_type 
              if (defData->VersionNum < 5.4) { // PCR 445209 
                if (defData->callbacks->SNetCbk) defData->Net.addShield(defData->shieldName);
                defData->by_is_keyword = false;
                defData->do_is_keyword = false;
                defData->new_is_keyword = false;
                defData->step_is_keyword = false;
                defData->orient_is_keyword = false;
                defData->virtual_is_keyword = false;
                defData->mask_is_keyword = false;
                defData->rect_is_keyword = false;
                defData->specialWire_routeStatus = (char*)"ROUTED";
                defData->specialWire_routeStatusName = (char*)"";
                defData->shield = true;   // save the path info in the defData->shield paths 
              } else
                if (defData->callbacks->SNetCbk) defData->Net.addWire("SHIELD", defData->shieldName);
                defData->specialWire_routeStatus = (char*)"ROUTED";
                defData->specialWire_routeStatusName = (char*)"";
            }
#line 7955 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 572: /* shield_layer: $@96 spaths  */
#line 3343 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              // 7/17/2003 - Fix for pcr 604848, add a callback for each wire
              if (defData->callbacks->SNetWireCbk) {
                 CALLBACK(defData->callbacks->SNetWireCbk, defrSNetWireCbkType, &defData->Net);
                 if (defData->VersionNum < 5.4)
                   defData->Net.freeShield();
                 else
                   defData->Net.freeWire();
              }
              if (defData->VersionNum < 5.4) {  // PCR 445209 
                defData->shield = false;
                defData->by_is_keyword = false;
                defData->do_is_keyword = false;
                defData->new_is_keyword = false;
                defData->step_is_keyword = false;
                defData->nondef_is_keyword = false;
                defData->mustjoin_is_keyword = false;
                defData->orient_is_keyword = false;
                defData->virtual_is_keyword = false;
                defData->mask_is_keyword = false;
                defData->rect_is_keyword = false;
              } else {
                defData->by_is_keyword = false;
                defData->do_is_keyword = false;
                defData->new_is_keyword = false;
                defData->step_is_keyword = false;
                defData->orient_is_keyword = false;
                defData->virtual_is_keyword = false;
                defData->mask_is_keyword = false;
                defData->rect_is_keyword = false;
              }
              defData->needSNPCbk = 0;
            }
#line 7993 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 573: /* $@97: %empty  */
#line 3377 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 7999 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 574: /* snet_width: '+' K_WIDTH $@97 T_STRING NUMBER  */
#line 3378 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
              if (defData->VersionNum < 5.5) {
                 if (defData->callbacks->SNetCbk) defData->Net.setWidth((yyvsp[-1].string), (yyvsp[0].dval));
              } else {
                 defData->defWarning(7026, "The WIDTH statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
              }
            }
#line 8012 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 575: /* $@98: %empty  */
#line 3387 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                             { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8018 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 576: /* snet_voltage: '+' K_VOLTAGE $@98 T_STRING  */
#line 3388 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defrData::numIsInt((yyvsp[0].string))) {
                 if (defData->callbacks->SNetCbk) defData->Net.setVoltage(atoi((yyvsp[0].string)));
              } else {
                 if (defData->callbacks->SNetCbk) {
                   if (defData->sNetWarnings++ < defData->settings->SNetWarnings) {
                     defData->defMsg = (char*)malloc(1000);
                     sprintf (defData->defMsg,
                        "The value %s for statement VOLTAGE is invalid. The value can only be integer.\nSpecify a valid value in units of millivolts", (yyvsp[0].string));
                     defData->defError(6537, defData->defMsg);
                     free(defData->defMsg);
                     CHKERR();
                   }
                 }
              }
            }
#line 8039 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 577: /* $@99: %empty  */
#line 3405 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                            { defData->dumb_mode = 1; }
#line 8045 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 578: /* $@100: %empty  */
#line 3406 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) defData->Net.setSpacing((yyvsp[-1].string),(yyvsp[0].dval));
            }
#line 8053 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 579: /* snet_spacing: '+' K_SPACING $@99 T_STRING NUMBER $@100 opt_snet_range  */
#line 3410 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
            }
#line 8060 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 582: /* snet_prop: T_STRING NUMBER  */
#line 3418 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) {
                char propTp;
                char* str = defData->ringCopy("                       ");
                propTp = defData->session->SNetProp.propType((yyvsp[-1].string));
                CHKPROPTYPE(propTp, (yyvsp[-1].string), "SPECIAL NET");
                // For backword compatibility, also set the string value 
                sprintf(str, "%g", (yyvsp[0].dval));
                defData->Net.addNumProp((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
              }
            }
#line 8076 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 583: /* snet_prop: T_STRING QSTRING  */
#line 3430 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) {
                char propTp;
                propTp = defData->session->SNetProp.propType((yyvsp[-1].string));
                CHKPROPTYPE(propTp, (yyvsp[-1].string), "SPECIAL NET");
                defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
              }
            }
#line 8089 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 584: /* snet_prop: T_STRING T_STRING  */
#line 3439 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) {
                char propTp;
                propTp = defData->session->SNetProp.propType((yyvsp[-1].string));
                CHKPROPTYPE(propTp, (yyvsp[-1].string), "SPECIAL NET");
                defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
              }
            }
#line 8102 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 586: /* opt_snet_range: K_RANGE NUMBER NUMBER  */
#line 3450 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) defData->Net.setRange((yyvsp[-1].dval),(yyvsp[0].dval));
            }
#line 8110 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 588: /* opt_range: K_RANGE NUMBER NUMBER  */
#line 3456 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { defData->Prop.setRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 8116 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 589: /* pattern_type: K_BALANCED  */
#line 3459 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"BALANCED"; }
#line 8122 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 590: /* pattern_type: K_STEINER  */
#line 3461 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"STEINER"; }
#line 8128 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 591: /* pattern_type: K_TRUNK  */
#line 3463 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"TRUNK"; }
#line 8134 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 592: /* pattern_type: K_WIREDLOGIC  */
#line 3465 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
            { (yyval.string) = (char*)"WIREDLOGIC"; }
#line 8140 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 593: /* spaths: spath  */
#line 3469 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->NeedPathData && defData->callbacks->SNetCbk) {
           if (defData->needSNPCbk && defData->callbacks->SNetPartialPathCbk) { 
              // require a callback before proceed because defData->needSNPCbk must be
              // set to 1 from the previous pathIsDone and user has registered
              // a callback routine.
              CALLBACK(defData->callbacks->SNetPartialPathCbk, defrSNetPartialPathCbkType,
                       &defData->Net);
              defData->needSNPCbk = 0;   // reset the flag
              defData->pathIsDone(defData->shield, 1, defData->netOsnet, &defData->needSNPCbk);
              defData->Net.clearRectPolyNPath();
              defData->Net.clearVia();
           } else
              defData->pathIsDone(defData->shield, 0, defData->netOsnet, &defData->needSNPCbk);
        }
      }
#line 8161 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 594: /* spaths: spaths snew_path  */
#line 3486 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8167 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 595: /* $@101: %empty  */
#line 3488 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                 { defData->dumb_mode = 1; }
#line 8173 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 596: /* snew_path: K_NEW $@101 spath  */
#line 3489 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->SNetCbk) {
           if (defData->needSNPCbk && defData->callbacks->SNetPartialPathCbk) {
              // require a callback before proceed because defData->needSNPCbk must be
              // set to 1 from the previous pathIsDone and user has registered
              // a callback routine.
              CALLBACK(defData->callbacks->SNetPartialPathCbk, defrSNetPartialPathCbkType,
                       &defData->Net);
              defData->needSNPCbk = 0;   // reset the flag
              defData->pathIsDone(defData->shield, 1, defData->netOsnet, &defData->needSNPCbk);
              // reset any poly or rect in special wiring statement
              defData->Net.clearRectPolyNPath();
              defData->Net.clearVia();
           } else
              defData->pathIsDone(defData->shield, 0, defData->netOsnet, &defData->needSNPCbk);
        }
      }
#line 8194 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 597: /* $@102: %empty  */
#line 3507 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->SNetCbk)
           defData->PathObj.addLayer((yyvsp[0].string));
        defData->dumb_mode = 0; defData->no_num = 0;
      }
#line 8203 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 598: /* $@103: %empty  */
#line 3515 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = DEF_MAX_INT; defData->by_is_keyword = true; defData->do_is_keyword = true;
        defData->new_is_keyword = true; defData->step_is_keyword = true;
         defData->orient_is_keyword = true; defData->rect_is_keyword = true, defData->mask_is_keyword = true; 
         defData->virtual_is_keyword = true;  }
#line 8212 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 599: /* spath: T_STRING $@102 width opt_spaths path_pt $@103 path_item_list  */
#line 3521 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->rect_is_keyword = false, defData->mask_is_keyword = false, defData->virtual_is_keyword = false; }
#line 8218 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 600: /* width: NUMBER  */
#line 3524 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->SNetCbk)
          defData->PathObj.addWidth(round((yyvsp[0].dval)));
      }
#line 8226 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 601: /* start_snets: K_SNETS NUMBER ';'  */
#line 3529 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->SNetStartCbk)
          CALLBACK(defData->callbacks->SNetStartCbk, defrSNetStartCbkType, round((yyvsp[-1].dval)));
        defData->netOsnet = 2;
      }
#line 8236 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 602: /* end_snets: K_END K_SNETS  */
#line 3536 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->SNetEndCbk)
          CALLBACK(defData->callbacks->SNetEndCbk, defrSNetEndCbkType, 0);
        defData->netOsnet = 0;
      }
#line 8246 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 604: /* groups_start: K_GROUPS NUMBER ';'  */
#line 3546 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->GroupsStartCbk)
           CALLBACK(defData->callbacks->GroupsStartCbk, defrGroupsStartCbkType, round((yyvsp[-1].dval)));
      }
#line 8255 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 607: /* group_rule: start_group group_members group_options ';'  */
#line 3556 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk)
           CALLBACK(defData->callbacks->GroupCbk, defrGroupCbkType, &defData->Group);
      }
#line 8264 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 608: /* $@104: %empty  */
#line 3561 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                 { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8270 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 609: /* start_group: '-' $@104 T_STRING  */
#line 3562 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        defData->dumb_mode = DEF_MAX_INT;
        defData->no_num = DEF_MAX_INT;
        /* dumb_mode is automatically turned off at the first
         * + in the options or at the ; at the end of the group */
        if (defData->callbacks->GroupCbk) defData->Group.setup((yyvsp[0].string));
        if (defData->callbacks->GroupNameCbk)
           CALLBACK(defData->callbacks->GroupNameCbk, defrGroupNameCbkType, (yyvsp[0].string));
      }
#line 8284 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 611: /* group_members: group_members group_member  */
#line 3574 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {  }
#line 8290 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 612: /* group_member: T_STRING  */
#line 3577 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        // if (defData->callbacks->GroupCbk) defData->Group.addMember($1); 
        if (defData->callbacks->GroupMemberCbk)
          CALLBACK(defData->callbacks->GroupMemberCbk, defrGroupMemberCbkType, (yyvsp[0].string));
      }
#line 8300 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 615: /* group_option: '+' K_SOFT group_soft_options  */
#line 3588 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8306 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 616: /* $@105: %empty  */
#line 3589 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                           { defData->dumb_mode = DEF_MAX_INT; }
#line 8312 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 617: /* group_option: '+' K_PROPERTY $@105 group_prop_list  */
#line 3591 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 8318 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 618: /* $@106: %empty  */
#line 3592 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         { defData->dumb_mode = 1;  defData->no_num = 1; }
#line 8324 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 619: /* group_option: '+' K_REGION $@106 group_region  */
#line 3593 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8330 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 620: /* group_option: extension_stmt  */
#line 3595 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->GroupMemberCbk)
          CALLBACK(defData->callbacks->GroupExtCbk, defrGroupExtCbkType, &defData->History_text[0]);
      }
#line 8339 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 621: /* group_region: pt pt  */
#line 3601 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
          if (defData->callbacks->GroupCbk)
            defData->Group.addRegionRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
        }
        else
          defData->defWarning(7027, "The GROUP REGION pt pt statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
      }
#line 8353 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 622: /* group_region: T_STRING  */
#line 3611 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->GroupCbk)
          defData->Group.setRegionName((yyvsp[0].string));
      }
#line 8361 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 625: /* group_prop: T_STRING NUMBER  */
#line 3620 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk) {
          char propTp;
          char* str = defData->ringCopy("                       ");
          propTp = defData->session->GroupProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "GROUP");
          sprintf(str, "%g", (yyvsp[0].dval));
          defData->Group.addNumProperty((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
        }
      }
#line 8376 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 626: /* group_prop: T_STRING QSTRING  */
#line 3631 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk) {
          char propTp;
          propTp = defData->session->GroupProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "GROUP");
          defData->Group.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 8389 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 627: /* group_prop: T_STRING T_STRING  */
#line 3640 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk) {
          char propTp;
          propTp = defData->session->GroupProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "GROUP");
          defData->Group.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 8402 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 629: /* group_soft_options: group_soft_options group_soft_option  */
#line 3651 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8408 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 630: /* group_soft_option: K_MAXX NUMBER  */
#line 3654 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
          if (defData->callbacks->GroupCbk) defData->Group.setMaxX(round((yyvsp[0].dval)));
        } else {
          defData->defWarning(7028, "The GROUP SOFT MAXX statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 8421 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 631: /* group_soft_option: K_MAXY NUMBER  */
#line 3663 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
          if (defData->callbacks->GroupCbk) defData->Group.setMaxY(round((yyvsp[0].dval)));
        } else {
          defData->defWarning(7029, "The GROUP SOFT MAXY statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 8434 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 632: /* group_soft_option: K_MAXHALFPERIMETER NUMBER  */
#line 3672 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
          if (defData->callbacks->GroupCbk) defData->Group.setPerim(round((yyvsp[0].dval)));
        } else { 
          defData->defWarning(7030, "The GROUP SOFT MAXHALFPERIMETER statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 8447 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 633: /* groups_end: K_END K_GROUPS  */
#line 3682 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->GroupsEndCbk)
          CALLBACK(defData->callbacks->GroupsEndCbk, defrGroupsEndCbkType, 0);
      }
#line 8456 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 636: /* assertions_start: K_ASSERTIONS NUMBER ';'  */
#line 3696 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if ((defData->VersionNum < 5.4) && (defData->callbacks->AssertionsStartCbk)) {
          CALLBACK(defData->callbacks->AssertionsStartCbk, defrAssertionsStartCbkType,
                   round((yyvsp[-1].dval)));
        } else {
          if (defData->callbacks->AssertionCbk)
            if (defData->assertionWarnings++ < defData->settings->AssertionWarnings)
              defData->defWarning(7031, "The ASSERTIONS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
        }
        if (defData->callbacks->AssertionCbk)
          defData->Assertion.setAssertionMode();
      }
#line 8473 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 637: /* constraints_start: K_CONSTRAINTS NUMBER ';'  */
#line 3710 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if ((defData->VersionNum < 5.4) && (defData->callbacks->ConstraintsStartCbk)) {
          CALLBACK(defData->callbacks->ConstraintsStartCbk, defrConstraintsStartCbkType,
                   round((yyvsp[-1].dval)));
        } else {
          if (defData->callbacks->ConstraintCbk)
            if (defData->constraintWarnings++ < defData->settings->ConstraintWarnings)
              defData->defWarning(7032, "The CONSTRAINTS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
        }
        if (defData->callbacks->ConstraintCbk)
          defData->Assertion.setConstraintMode();
      }
#line 8490 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 641: /* constraint_rule: wiredlogic_rule  */
#line 3729 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if ((defData->VersionNum < 5.4) && (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)) {
          if (defData->Assertion.isConstraint()) 
            CALLBACK(defData->callbacks->ConstraintCbk, defrConstraintCbkType, &defData->Assertion);
          if (defData->Assertion.isAssertion()) 
            CALLBACK(defData->callbacks->AssertionCbk, defrAssertionCbkType, &defData->Assertion);
        }
      }
#line 8503 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 642: /* operand_rule: '-' operand delay_specs ';'  */
#line 3739 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if ((defData->VersionNum < 5.4) && (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)) {
          if (defData->Assertion.isConstraint()) 
            CALLBACK(defData->callbacks->ConstraintCbk, defrConstraintCbkType, &defData->Assertion);
          if (defData->Assertion.isAssertion()) 
            CALLBACK(defData->callbacks->AssertionCbk, defrAssertionCbkType, &defData->Assertion);
        }
   
        // reset all the flags and everything
        defData->Assertion.clear();
      }
#line 8519 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 643: /* $@107: %empty  */
#line 3751 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8525 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 644: /* operand: K_NET $@107 T_STRING  */
#line 3752 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
         if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.addNet((yyvsp[0].string));
      }
#line 8534 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 645: /* $@108: %empty  */
#line 3756 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {defData->dumb_mode = 4; defData->no_num = 4;}
#line 8540 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 646: /* operand: K_PATH $@108 T_STRING T_STRING T_STRING T_STRING  */
#line 3757 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
         if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.addPath((yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].string), (yyvsp[0].string));
      }
#line 8549 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 647: /* operand: K_SUM '(' operand_list ')'  */
#line 3762 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.setSum();
      }
#line 8558 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 648: /* operand: K_DIFF '(' operand_list ')'  */
#line 3767 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.setDiff();
      }
#line 8567 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 650: /* operand_list: operand_list operand  */
#line 3774 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8573 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 652: /* $@109: %empty  */
#line 3777 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8579 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 653: /* wiredlogic_rule: '-' K_WIREDLOGIC $@109 T_STRING opt_plus K_MAXDIST NUMBER ';'  */
#line 3779 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setWiredlogic((yyvsp[-4].string), (yyvsp[-1].dval));
      }
#line 8588 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 654: /* opt_plus: %empty  */
#line 3786 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)""; }
#line 8594 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 655: /* opt_plus: '+'  */
#line 3788 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"+"; }
#line 8600 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 658: /* delay_spec: '+' K_RISEMIN NUMBER  */
#line 3795 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setRiseMin((yyvsp[0].dval));
      }
#line 8609 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 659: /* delay_spec: '+' K_RISEMAX NUMBER  */
#line 3800 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setRiseMax((yyvsp[0].dval));
      }
#line 8618 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 660: /* delay_spec: '+' K_FALLMIN NUMBER  */
#line 3805 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setFallMin((yyvsp[0].dval));
      }
#line 8627 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 661: /* delay_spec: '+' K_FALLMAX NUMBER  */
#line 3810 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setFallMax((yyvsp[0].dval));
      }
#line 8636 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 662: /* constraints_end: K_END K_CONSTRAINTS  */
#line 3816 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if ((defData->VersionNum < 5.4) && defData->callbacks->ConstraintsEndCbk) {
          CALLBACK(defData->callbacks->ConstraintsEndCbk, defrConstraintsEndCbkType, 0);
        } else {
          if (defData->callbacks->ConstraintsEndCbk) {
            if (defData->constraintWarnings++ < defData->settings->ConstraintWarnings)
              defData->defWarning(7032, "The CONSTRAINTS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
      }
#line 8650 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 663: /* assertions_end: K_END K_ASSERTIONS  */
#line 3827 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if ((defData->VersionNum < 5.4) && defData->callbacks->AssertionsEndCbk) {
          CALLBACK(defData->callbacks->AssertionsEndCbk, defrAssertionsEndCbkType, 0);
        } else {
          if (defData->callbacks->AssertionsEndCbk) {
            if (defData->assertionWarnings++ < defData->settings->AssertionWarnings)
              defData->defWarning(7031, "The ASSERTIONS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
      }
#line 8664 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 665: /* scanchain_start: K_SCANCHAINS NUMBER ';'  */
#line 3841 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->ScanchainsStartCbk)
          CALLBACK(defData->callbacks->ScanchainsStartCbk, defrScanchainsStartCbkType,
                   round((yyvsp[-1].dval)));
      }
#line 8673 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 667: /* scanchain_rules: scanchain_rules scan_rule  */
#line 3848 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {}
#line 8679 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 668: /* scan_rule: start_scan scan_members ';'  */
#line 3851 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->ScanchainCbk)
          CALLBACK(defData->callbacks->ScanchainCbk, defrScanchainCbkType, &defData->Scanchain);
      }
#line 8688 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 669: /* $@110: %empty  */
#line 3856 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                {defData->dumb_mode = 1; defData->no_num = 1;}
#line 8694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 670: /* start_scan: '-' $@110 T_STRING  */
#line 3857 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setName((yyvsp[0].string));
        defData->bit_is_keyword = true;
      }
#line 8704 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 673: /* opt_pin: %empty  */
#line 3869 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)""; }
#line 8710 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 674: /* opt_pin: T_STRING  */
#line 3871 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (yyvsp[0].string); }
#line 8716 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 675: /* $@111: %empty  */
#line 3873 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode = 2; defData->no_num = 2;}
#line 8722 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 676: /* scan_member: '+' K_START $@111 T_STRING opt_pin  */
#line 3874 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setStart((yyvsp[-1].string), (yyvsp[0].string));
      }
#line 8730 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 677: /* $@112: %empty  */
#line 3877 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8736 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 678: /* scan_member: '+' K_FLOATING $@112 floating_inst_list  */
#line 3878 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->no_num = 0; }
#line 8742 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 679: /* $@113: %empty  */
#line 3880 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
         defData->dumb_mode = 1;
         defData->no_num = 1;
         if (defData->callbacks->ScanchainCbk)
           defData->Scanchain.addOrderedList();
      }
#line 8753 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 680: /* scan_member: '+' K_ORDERED $@113 ordered_inst_list  */
#line 3887 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->no_num = 0; }
#line 8759 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 681: /* $@114: %empty  */
#line 3888 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                   {defData->dumb_mode = 2; defData->no_num = 2; }
#line 8765 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 682: /* scan_member: '+' K_STOP $@114 T_STRING opt_pin  */
#line 3889 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setStop((yyvsp[-1].string), (yyvsp[0].string));
      }
#line 8773 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 683: /* $@115: %empty  */
#line 3892 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                             { defData->dumb_mode = 10; defData->no_num = 10; }
#line 8779 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 684: /* scan_member: '+' K_COMMONSCANPINS $@115 opt_common_pins  */
#line 3893 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0;  defData->no_num = 0; }
#line 8785 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 685: /* $@116: %empty  */
#line 3894 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8791 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 686: /* scan_member: '+' K_PARTITION $@116 T_STRING partition_maxbits  */
#line 3896 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.5) {
          if (defData->callbacks->ScanchainCbk) {
            if (defData->scanchainWarnings++ < defData->settings->ScanchainWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The PARTITION statement is available in version 5.5 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
              defData->defError(6538, defData->defMsg);
              free(defData->defMsg);
              CHKERR();
            }
          }
        }
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setPartition((yyvsp[-1].string), (yyvsp[0].integer));
      }
#line 8812 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 687: /* scan_member: extension_stmt  */
#line 3913 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanChainExtCbk)
          CALLBACK(defData->callbacks->ScanChainExtCbk, defrScanChainExtCbkType, &defData->History_text[0]);
      }
#line 8821 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 688: /* opt_common_pins: %empty  */
#line 3919 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8827 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 689: /* opt_common_pins: '(' T_STRING T_STRING ')'  */
#line 3921 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.setCommonIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.setCommonOut((yyvsp[-1].string));
        }
      }
#line 8840 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 690: /* opt_common_pins: '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')'  */
#line 3930 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-6].string), "IN") == 0 || strcmp((yyvsp[-6].string), "in") == 0)
            defData->Scanchain.setCommonIn((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "OUT") == 0 || strcmp((yyvsp[-6].string), "out") == 0)
            defData->Scanchain.setCommonOut((yyvsp[-5].string));
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.setCommonIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.setCommonOut((yyvsp[-1].string));
        }
      }
#line 8857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 693: /* $@117: %empty  */
#line 3948 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        defData->dumb_mode = 1000;
        defData->no_num = 1000;
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.addFloatingInst((yyvsp[0].string));
      }
#line 8868 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 694: /* one_floating_inst: T_STRING $@117 floating_pins  */
#line 3955 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8874 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 695: /* floating_pins: %empty  */
#line 3958 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8880 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 696: /* floating_pins: '(' T_STRING T_STRING ')'  */
#line 3960 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.addFloatingIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.addFloatingOut((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "BITS") == 0 || strcmp((yyvsp[-2].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-1].string));
            defData->Scanchain.setFloatingBits(defData->bitsNum);
          }
        }
      }
#line 8897 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 697: /* floating_pins: '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')'  */
#line 3973 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-6].string), "IN") == 0 || strcmp((yyvsp[-6].string), "in") == 0)
            defData->Scanchain.addFloatingIn((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "OUT") == 0 || strcmp((yyvsp[-6].string), "out") == 0)
            defData->Scanchain.addFloatingOut((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "BITS") == 0 || strcmp((yyvsp[-6].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-5].string));
            defData->Scanchain.setFloatingBits(defData->bitsNum);
          }
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.addFloatingIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.addFloatingOut((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "BITS") == 0 || strcmp((yyvsp[-2].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-1].string));
            defData->Scanchain.setFloatingBits(defData->bitsNum);
          }
        }
      }
#line 8922 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 698: /* floating_pins: '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')'  */
#line 3995 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-10].string), "IN") == 0 || strcmp((yyvsp[-10].string), "in") == 0)
            defData->Scanchain.addFloatingIn((yyvsp[-9].string));
          else if (strcmp((yyvsp[-10].string), "OUT") == 0 || strcmp((yyvsp[-10].string), "out") == 0)
            defData->Scanchain.addFloatingOut((yyvsp[-9].string));
          else if (strcmp((yyvsp[-10].string), "BITS") == 0 || strcmp((yyvsp[-10].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-9].string));
            defData->Scanchain.setFloatingBits(defData->bitsNum);
          }
          if (strcmp((yyvsp[-6].string), "IN") == 0 || strcmp((yyvsp[-6].string), "in") == 0)
            defData->Scanchain.addFloatingIn((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "OUT") == 0 || strcmp((yyvsp[-6].string), "out") == 0)
            defData->Scanchain.addFloatingOut((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "BITS") == 0 || strcmp((yyvsp[-6].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-5].string));
            defData->Scanchain.setFloatingBits(defData->bitsNum);
          }
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.addFloatingIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.addFloatingOut((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "BITS") == 0 || strcmp((yyvsp[-2].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-1].string));
            defData->Scanchain.setFloatingBits(defData->bitsNum);
          }
        }
      }
#line 8955 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 701: /* $@118: %empty  */
#line 4029 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 1000; defData->no_num = 1000; 
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.addOrderedInst((yyvsp[0].string));
      }
#line 8964 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 702: /* one_ordered_inst: T_STRING $@118 ordered_pins  */
#line 4034 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8970 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 703: /* ordered_pins: %empty  */
#line 4037 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 8976 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 704: /* ordered_pins: '(' T_STRING T_STRING ')'  */
#line 4039 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.addOrderedIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.addOrderedOut((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "BITS") == 0 || strcmp((yyvsp[-2].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-1].string));
            defData->Scanchain.setOrderedBits(defData->bitsNum);
         }
        }
      }
#line 8993 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 705: /* ordered_pins: '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')'  */
#line 4052 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-6].string), "IN") == 0 || strcmp((yyvsp[-6].string), "in") == 0)
            defData->Scanchain.addOrderedIn((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "OUT") == 0 || strcmp((yyvsp[-6].string), "out") == 0)
            defData->Scanchain.addOrderedOut((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "BITS") == 0 || strcmp((yyvsp[-6].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-5].string));
            defData->Scanchain.setOrderedBits(defData->bitsNum);
          }
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.addOrderedIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.addOrderedOut((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "BITS") == 0 || strcmp((yyvsp[-2].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-1].string));
            defData->Scanchain.setOrderedBits(defData->bitsNum);
          }
        }
      }
#line 9018 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 706: /* ordered_pins: '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')' '(' T_STRING T_STRING ')'  */
#line 4074 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-10].string), "IN") == 0 || strcmp((yyvsp[-10].string), "in") == 0)
            defData->Scanchain.addOrderedIn((yyvsp[-9].string));
          else if (strcmp((yyvsp[-10].string), "OUT") == 0 || strcmp((yyvsp[-10].string), "out") == 0)
            defData->Scanchain.addOrderedOut((yyvsp[-9].string));
          else if (strcmp((yyvsp[-10].string), "BITS") == 0 || strcmp((yyvsp[-10].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-9].string));
            defData->Scanchain.setOrderedBits(defData->bitsNum);
          }
          if (strcmp((yyvsp[-6].string), "IN") == 0 || strcmp((yyvsp[-6].string), "in") == 0)
            defData->Scanchain.addOrderedIn((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "OUT") == 0 || strcmp((yyvsp[-6].string), "out") == 0)
            defData->Scanchain.addOrderedOut((yyvsp[-5].string));
          else if (strcmp((yyvsp[-6].string), "BITS") == 0 || strcmp((yyvsp[-6].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-5].string));
            defData->Scanchain.setOrderedBits(defData->bitsNum);
          }
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.addOrderedIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.addOrderedOut((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "BITS") == 0 || strcmp((yyvsp[-2].string), "bits") == 0) {
            defData->bitsNum = atoi((yyvsp[-1].string));
            defData->Scanchain.setOrderedBits(defData->bitsNum);
          }
        }
      }
#line 9051 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 707: /* partition_maxbits: %empty  */
#line 4104 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.integer) = -1; }
#line 9057 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 708: /* partition_maxbits: K_MAXBITS NUMBER  */
#line 4106 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.integer) = round((yyvsp[0].dval)); }
#line 9063 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 709: /* scanchain_end: K_END K_SCANCHAINS  */
#line 4109 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->ScanchainsEndCbk)
          CALLBACK(defData->callbacks->ScanchainsEndCbk, defrScanchainsEndCbkType, 0);
        defData->bit_is_keyword = false;
        defData->dumb_mode = 0; defData->no_num = 0;
      }
#line 9074 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 711: /* iotiming_start: K_IOTIMINGS NUMBER ';'  */
#line 4121 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.4 && defData->callbacks->IOTimingsStartCbk) {
          CALLBACK(defData->callbacks->IOTimingsStartCbk, defrIOTimingsStartCbkType, round((yyvsp[-1].dval)));
        } else {
          if (defData->callbacks->IOTimingsStartCbk)
            if (defData->iOTimingWarnings++ < defData->settings->IOTimingWarnings)
              defData->defWarning(7035, "The IOTIMINGS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 9088 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 713: /* iotiming_rules: iotiming_rules iotiming_rule  */
#line 4133 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 9094 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 714: /* iotiming_rule: start_iotiming iotiming_members ';'  */
#line 4136 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->VersionNum < 5.4 && defData->callbacks->IOTimingCbk)
          CALLBACK(defData->callbacks->IOTimingCbk, defrIOTimingCbkType, &defData->IOTiming);
      }
#line 9103 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 715: /* $@119: %empty  */
#line 4141 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        {defData->dumb_mode = 2; defData->no_num = 2; }
#line 9109 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 716: /* start_iotiming: '-' '(' $@119 T_STRING T_STRING ')'  */
#line 4142 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk)
          defData->IOTiming.setName((yyvsp[-2].string), (yyvsp[-1].string));
      }
#line 9118 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 719: /* iotiming_member: '+' risefall K_VARIABLE NUMBER NUMBER  */
#line 4153 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setVariable((yyvsp[-3].string), (yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 9127 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 720: /* iotiming_member: '+' risefall K_SLEWRATE NUMBER NUMBER  */
#line 4158 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setSlewRate((yyvsp[-3].string), (yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 9136 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 721: /* iotiming_member: '+' K_CAPACITANCE NUMBER  */
#line 4163 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setCapacitance((yyvsp[0].dval));
      }
#line 9145 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 722: /* $@120: %empty  */
#line 4167 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        {defData->dumb_mode = 1; defData->no_num = 1; }
#line 9151 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 723: /* $@121: %empty  */
#line 4168 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setDriveCell((yyvsp[0].string));
      }
#line 9160 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 725: /* iotiming_member: extension_stmt  */
#line 4177 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.4 && defData->callbacks->IoTimingsExtCbk)
          CALLBACK(defData->callbacks->IoTimingsExtCbk, defrIoTimingsExtCbkType, &defData->History_text[0]);
      }
#line 9169 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 726: /* $@122: %empty  */
#line 4183 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
              {defData->dumb_mode = 1; defData->no_num = 1; }
#line 9175 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 727: /* $@123: %empty  */
#line 4184 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setTo((yyvsp[0].string));
      }
#line 9184 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 730: /* $@124: %empty  */
#line 4191 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  {defData->dumb_mode = 1; defData->no_num = 1; }
#line 9190 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 731: /* iotiming_frompin: K_FROMPIN $@124 T_STRING  */
#line 4192 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk)
          defData->IOTiming.setFrom((yyvsp[0].string));
      }
#line 9199 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 733: /* iotiming_parallel: K_PARALLEL NUMBER  */
#line 4199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk)
          defData->IOTiming.setParallel((yyvsp[0].dval));
      }
#line 9208 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 734: /* risefall: K_RISE  */
#line 4204 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                 { (yyval.string) = (char*)"RISE"; }
#line 9214 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 735: /* risefall: K_FALL  */
#line 4204 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                                  { (yyval.string) = (char*)"FALL"; }
#line 9220 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 736: /* iotiming_end: K_END K_IOTIMINGS  */
#line 4207 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.4 && defData->callbacks->IOTimingsEndCbk)
          CALLBACK(defData->callbacks->IOTimingsEndCbk, defrIOTimingsEndCbkType, 0);
      }
#line 9229 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 737: /* floorplan_contraints_section: fp_start fp_stmts K_END K_FPC  */
#line 4213 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->FPCEndCbk)
          CALLBACK(defData->callbacks->FPCEndCbk, defrFPCEndCbkType, 0);
      }
#line 9238 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 738: /* fp_start: K_FPC NUMBER ';'  */
#line 4219 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->FPCStartCbk)
          CALLBACK(defData->callbacks->FPCStartCbk, defrFPCStartCbkType, round((yyvsp[-1].dval)));
      }
#line 9247 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 740: /* fp_stmts: fp_stmts fp_stmt  */
#line 4226 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {}
#line 9253 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 741: /* $@125: %empty  */
#line 4228 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
             { defData->dumb_mode = 1; defData->no_num = 1;  }
#line 9259 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 742: /* $@126: %empty  */
#line 4229 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setName((yyvsp[-1].string), (yyvsp[0].string)); }
#line 9265 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 743: /* fp_stmt: '-' $@125 T_STRING h_or_v $@126 constraint_type constrain_what_list ';'  */
#line 4231 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) CALLBACK(defData->callbacks->FPCCbk, defrFPCCbkType, &defData->FPC); }
#line 9271 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 744: /* h_or_v: K_HORIZONTAL  */
#line 4234 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"HORIZONTAL"; }
#line 9277 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 745: /* h_or_v: K_VERTICAL  */
#line 4236 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"VERTICAL"; }
#line 9283 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 746: /* constraint_type: K_ALIGN  */
#line 4239 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setAlign(); }
#line 9289 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 747: /* constraint_type: K_MAX NUMBER  */
#line 4241 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setMax((yyvsp[0].dval)); }
#line 9295 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 748: /* constraint_type: K_MIN NUMBER  */
#line 4243 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setMin((yyvsp[0].dval)); }
#line 9301 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 749: /* constraint_type: K_EQUAL NUMBER  */
#line 4245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setEqual((yyvsp[0].dval)); }
#line 9307 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 752: /* $@127: %empty  */
#line 4252 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setDoingBottomLeft(); }
#line 9313 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 754: /* $@128: %empty  */
#line 4255 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setDoingTopRight(); }
#line 9319 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 758: /* $@129: %empty  */
#line 4262 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode = 1; defData->no_num = 1; }
#line 9325 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 759: /* row_or_comp: '(' K_ROWS $@129 T_STRING ')'  */
#line 4263 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.addRow((yyvsp[-1].string)); }
#line 9331 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 760: /* $@130: %empty  */
#line 4264 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       {defData->dumb_mode = 1; defData->no_num = 1; }
#line 9337 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 761: /* row_or_comp: '(' K_COMPS $@130 T_STRING ')'  */
#line 4265 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.addComps((yyvsp[-1].string)); }
#line 9343 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 763: /* timingdisables_start: K_TIMINGDISABLES NUMBER ';'  */
#line 4272 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->TimingDisablesStartCbk)
          CALLBACK(defData->callbacks->TimingDisablesStartCbk, defrTimingDisablesStartCbkType,
                   round((yyvsp[-1].dval)));
      }
#line 9353 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 765: /* timingdisables_rules: timingdisables_rules timingdisables_rule  */
#line 4280 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {}
#line 9359 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 766: /* $@131: %empty  */
#line 4282 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                   { defData->dumb_mode = 2; defData->no_num = 2;  }
#line 9365 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 767: /* $@132: %empty  */
#line 4283 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       { defData->dumb_mode = 2; defData->no_num = 2;  }
#line 9371 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 768: /* timingdisables_rule: '-' K_FROMPIN $@131 T_STRING T_STRING K_TOPIN $@132 T_STRING T_STRING ';'  */
#line 4284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk) {
          defData->TimingDisable.setFromTo((yyvsp[-6].string), (yyvsp[-5].string), (yyvsp[-2].string), (yyvsp[-1].string));
          CALLBACK(defData->callbacks->TimingDisableCbk, defrTimingDisableCbkType,
                &defData->TimingDisable);
        }
      }
#line 9383 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 769: /* $@133: %empty  */
#line 4291 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      {defData->dumb_mode = 2; defData->no_num = 2; }
#line 9389 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 770: /* timingdisables_rule: '-' K_THRUPIN $@133 T_STRING T_STRING ';'  */
#line 4292 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk) {
          defData->TimingDisable.setThru((yyvsp[-2].string), (yyvsp[-1].string));
          CALLBACK(defData->callbacks->TimingDisableCbk, defrTimingDisableCbkType,
                   &defData->TimingDisable);
        }
      }
#line 9401 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 771: /* $@134: %empty  */
#line 4299 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    {defData->dumb_mode = 1; defData->no_num = 1;}
#line 9407 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 772: /* timingdisables_rule: '-' K_MACRO $@134 T_STRING td_macro_option ';'  */
#line 4300 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk) {
          defData->TimingDisable.setMacro((yyvsp[-2].string));
          CALLBACK(defData->callbacks->TimingDisableCbk, defrTimingDisableCbkType,
                &defData->TimingDisable);
        }
      }
#line 9419 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 773: /* timingdisables_rule: '-' K_REENTRANTPATHS ';'  */
#line 4308 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->TimingDisableCbk)
          defData->TimingDisable.setReentrantPathsFlag();
      }
#line 9427 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 774: /* $@135: %empty  */
#line 4313 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                           {defData->dumb_mode = 1; defData->no_num = 1;}
#line 9433 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 775: /* $@136: %empty  */
#line 4314 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {defData->dumb_mode=1; defData->no_num = 1;}
#line 9439 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 776: /* td_macro_option: K_FROMPIN $@135 T_STRING K_TOPIN $@136 T_STRING  */
#line 4315 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk)
          defData->TimingDisable.setMacroFromTo((yyvsp[-3].string),(yyvsp[0].string));
      }
#line 9448 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 777: /* $@137: %empty  */
#line 4319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode=1; defData->no_num = 1;}
#line 9454 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 778: /* td_macro_option: K_THRUPIN $@137 T_STRING  */
#line 4320 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk)
          defData->TimingDisable.setMacroThru((yyvsp[0].string));
      }
#line 9463 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 779: /* timingdisables_end: K_END K_TIMINGDISABLES  */
#line 4326 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->TimingDisablesEndCbk)
          CALLBACK(defData->callbacks->TimingDisablesEndCbk, defrTimingDisablesEndCbkType, 0);
      }
#line 9472 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 781: /* partitions_start: K_PARTITIONS NUMBER ';'  */
#line 4336 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionsStartCbk)
          CALLBACK(defData->callbacks->PartitionsStartCbk, defrPartitionsStartCbkType,
                   round((yyvsp[-1].dval)));
      }
#line 9482 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 783: /* partition_rules: partition_rules partition_rule  */
#line 4344 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 9488 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 784: /* partition_rule: start_partition partition_members ';'  */
#line 4347 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->PartitionCbk)
          CALLBACK(defData->callbacks->PartitionCbk, defrPartitionCbkType, &defData->Partition);
      }
#line 9497 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 785: /* $@138: %empty  */
#line 4352 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                     { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9503 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 786: /* start_partition: '-' $@138 T_STRING turnoff  */
#line 4353 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setName((yyvsp[-1].string));
      }
#line 9512 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 788: /* turnoff: K_TURNOFF turnoff_setup turnoff_hold  */
#line 4360 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.addTurnOff((yyvsp[-1].string), (yyvsp[0].string));
      }
#line 9521 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 789: /* turnoff_setup: %empty  */
#line 4366 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)" "; }
#line 9527 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 790: /* turnoff_setup: K_SETUPRISE  */
#line 4368 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"R"; }
#line 9533 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 791: /* turnoff_setup: K_SETUPFALL  */
#line 4370 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"F"; }
#line 9539 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 792: /* turnoff_hold: %empty  */
#line 4373 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)" "; }
#line 9545 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 793: /* turnoff_hold: K_HOLDRISE  */
#line 4375 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"R"; }
#line 9551 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 794: /* turnoff_hold: K_HOLDFALL  */
#line 4377 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"F"; }
#line 9557 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 797: /* $@139: %empty  */
#line 4383 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                     {defData->dumb_mode=2; defData->no_num = 2;}
#line 9563 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 798: /* partition_member: '+' K_FROMCLOCKPIN $@139 T_STRING T_STRING risefall minmaxpins  */
#line 4385 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setFromClockPin((yyvsp[-3].string), (yyvsp[-2].string));
      }
#line 9572 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 799: /* $@140: %empty  */
#line 4389 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                          {defData->dumb_mode=2; defData->no_num = 2; }
#line 9578 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 800: /* partition_member: '+' K_FROMCOMPPIN $@140 T_STRING T_STRING risefallminmax2_list  */
#line 4391 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setFromCompPin((yyvsp[-2].string), (yyvsp[-1].string));
      }
#line 9587 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 801: /* $@141: %empty  */
#line 4395 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        {defData->dumb_mode=1; defData->no_num = 1; }
#line 9593 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 802: /* partition_member: '+' K_FROMIOPIN $@141 T_STRING risefallminmax1_list  */
#line 4397 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setFromIOPin((yyvsp[-1].string));
      }
#line 9602 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 803: /* $@142: %empty  */
#line 4401 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         {defData->dumb_mode=2; defData->no_num = 2; }
#line 9608 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 804: /* partition_member: '+' K_TOCLOCKPIN $@142 T_STRING T_STRING risefall minmaxpins  */
#line 4403 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setToClockPin((yyvsp[-3].string), (yyvsp[-2].string));
      }
#line 9617 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 805: /* $@143: %empty  */
#line 4407 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                        {defData->dumb_mode=2; defData->no_num = 2; }
#line 9623 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 806: /* partition_member: '+' K_TOCOMPPIN $@143 T_STRING T_STRING risefallminmax2_list  */
#line 4409 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setToCompPin((yyvsp[-2].string), (yyvsp[-1].string));
      }
#line 9632 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 807: /* $@144: %empty  */
#line 4413 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      {defData->dumb_mode=1; defData->no_num = 2; }
#line 9638 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 808: /* partition_member: '+' K_TOIOPIN $@144 T_STRING risefallminmax1_list  */
#line 4414 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setToIOPin((yyvsp[-1].string));
      }
#line 9647 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 809: /* partition_member: extension_stmt  */
#line 4419 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->callbacks->PartitionsExtCbk)
          CALLBACK(defData->callbacks->PartitionsExtCbk, defrPartitionsExtCbkType,
                   &defData->History_text[0]);
      }
#line 9657 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 810: /* $@145: %empty  */
#line 4426 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT; }
#line 9663 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 811: /* minmaxpins: min_or_max_list K_PINS $@145 pin_list  */
#line 4427 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->no_num = 0; }
#line 9669 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 813: /* min_or_max_list: min_or_max_list min_or_max_member  */
#line 4431 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 9675 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 814: /* min_or_max_member: K_MIN NUMBER NUMBER  */
#line 4434 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setMin((yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 9684 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 815: /* min_or_max_member: K_MAX NUMBER NUMBER  */
#line 4439 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setMax((yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 9693 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 817: /* pin_list: pin_list T_STRING  */
#line 4446 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addPin((yyvsp[0].string)); }
#line 9699 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 820: /* risefallminmax1: K_RISEMIN NUMBER  */
#line 4452 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addRiseMin((yyvsp[0].dval)); }
#line 9705 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 821: /* risefallminmax1: K_FALLMIN NUMBER  */
#line 4454 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addFallMin((yyvsp[0].dval)); }
#line 9711 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 822: /* risefallminmax1: K_RISEMAX NUMBER  */
#line 4456 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addRiseMax((yyvsp[0].dval)); }
#line 9717 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 823: /* risefallminmax1: K_FALLMAX NUMBER  */
#line 4458 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addFallMax((yyvsp[0].dval)); }
#line 9723 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 826: /* risefallminmax2: K_RISEMIN NUMBER NUMBER  */
#line 4466 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addRiseMinRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9730 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 827: /* risefallminmax2: K_FALLMIN NUMBER NUMBER  */
#line 4469 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addFallMinRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9737 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 828: /* risefallminmax2: K_RISEMAX NUMBER NUMBER  */
#line 4472 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addRiseMaxRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9744 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 829: /* risefallminmax2: K_FALLMAX NUMBER NUMBER  */
#line 4475 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addFallMaxRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9751 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 830: /* partitions_end: K_END K_PARTITIONS  */
#line 4479 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PartitionsEndCbk)
          CALLBACK(defData->callbacks->PartitionsEndCbk, defrPartitionsEndCbkType, 0); }
#line 9758 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 832: /* comp_names: comp_names comp_name  */
#line 4484 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 9764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 833: /* $@146: %empty  */
#line 4486 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
               {defData->dumb_mode=2; defData->no_num = 2; }
#line 9770 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 834: /* comp_name: '(' $@146 T_STRING T_STRING subnet_opt_syn ')'  */
#line 4488 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        // note that the defData->first T_STRING could be the keyword VPIN 
        if (defData->callbacks->NetCbk)
          defData->Subnet->addPin((yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].integer));
      }
#line 9780 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 835: /* subnet_opt_syn: %empty  */
#line 4495 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.integer) = 0; }
#line 9786 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 836: /* subnet_opt_syn: '+' K_SYNTHESIZED  */
#line 4497 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.integer) = 1; }
#line 9792 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 839: /* $@147: %empty  */
#line 4503 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {  
        if (defData->callbacks->NetCbk) defData->Subnet->addWire((yyvsp[0].string));
      }
#line 9800 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 840: /* subnet_option: subnet_type $@147 paths  */
#line 4507 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {  
        defData->by_is_keyword = false;
        defData->do_is_keyword = false;
        defData->new_is_keyword = false;
        defData->step_is_keyword = false;
        defData->orient_is_keyword = false;
        defData->needNPCbk = 0;
      }
#line 9813 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 841: /* $@148: %empty  */
#line 4515 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9819 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 842: /* subnet_option: K_NONDEFAULTRULE $@148 T_STRING  */
#line 4516 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->NetCbk) defData->Subnet->setNonDefault((yyvsp[0].string)); }
#line 9825 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 843: /* subnet_type: K_FIXED  */
#line 4519 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"FIXED"; defData->dumb_mode = 1; }
#line 9831 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 844: /* subnet_type: K_COVER  */
#line 4521 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"COVER"; defData->dumb_mode = 1; }
#line 9837 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 845: /* subnet_type: K_ROUTED  */
#line 4523 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"ROUTED"; defData->dumb_mode = 1; }
#line 9843 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 846: /* subnet_type: K_NOSHIELD  */
#line 4525 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { (yyval.string) = (char*)"NOSHIELD"; defData->dumb_mode = 1; }
#line 9849 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 848: /* begin_pin_props: K_PINPROPERTIES NUMBER opt_semi  */
#line 4530 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PinPropStartCbk)
          CALLBACK(defData->callbacks->PinPropStartCbk, defrPinPropStartCbkType, round((yyvsp[-1].dval))); }
#line 9856 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 849: /* opt_semi: %empty  */
#line 4535 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 9862 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 850: /* opt_semi: ';'  */
#line 4537 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { }
#line 9868 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 851: /* end_pin_props: K_END K_PINPROPERTIES  */
#line 4540 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PinPropEndCbk)
          CALLBACK(defData->callbacks->PinPropEndCbk, defrPinPropEndCbkType, 0); }
#line 9875 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 854: /* $@149: %empty  */
#line 4547 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       { defData->dumb_mode = 2; defData->no_num = 2; }
#line 9881 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 855: /* $@150: %empty  */
#line 4548 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PinPropCbk) defData->PinProp.setName((yyvsp[-1].string), (yyvsp[0].string)); }
#line 9887 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 856: /* pin_prop_terminal: '-' $@149 T_STRING T_STRING $@150 pin_prop_options ';'  */
#line 4550 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->PinPropCbk) {
          CALLBACK(defData->callbacks->PinPropCbk, defrPinPropCbkType, &defData->PinProp);
         // reset the property number
         defData->PinProp.clear();
        }
      }
#line 9898 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 859: /* $@151: %empty  */
#line 4560 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                         { defData->dumb_mode = DEF_MAX_INT; }
#line 9904 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 860: /* pin_prop: '+' K_PROPERTY $@151 pin_prop_name_value_list  */
#line 4562 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 9910 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 863: /* pin_prop_name_value: T_STRING NUMBER  */
#line 4569 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PinPropCbk) {
          char propTp;
          char* str = defData->ringCopy("                       ");
          propTp = defData->session->CompPinProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "PINPROPERTIES");
          sprintf(str, "%g", (yyvsp[0].dval));
          defData->PinProp.addNumProperty((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
        }
      }
#line 9925 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 864: /* pin_prop_name_value: T_STRING QSTRING  */
#line 4580 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PinPropCbk) {
          char propTp;
          propTp = defData->session->CompPinProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "PINPROPERTIES");
          defData->PinProp.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 9938 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 865: /* pin_prop_name_value: T_STRING T_STRING  */
#line 4589 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->PinPropCbk) {
          char propTp;
          propTp = defData->session->CompPinProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "PINPROPERTIES");
          defData->PinProp.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 9951 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 867: /* blockage_start: K_BLOCKAGES NUMBER ';'  */
#line 4601 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->BlockageStartCbk)
          CALLBACK(defData->callbacks->BlockageStartCbk, defrBlockageStartCbkType, round((yyvsp[-1].dval))); }
#line 9958 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 868: /* blockage_end: K_END K_BLOCKAGES  */
#line 4605 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->BlockageEndCbk)
          CALLBACK(defData->callbacks->BlockageEndCbk, defrBlockageEndCbkType, 0); }
#line 9965 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 871: /* blockage_def: blockage_rule rectPoly_blockage rectPoly_blockage_rules ';'  */
#line 4614 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
          CALLBACK(defData->callbacks->BlockageCbk, defrBlockageCbkType, &defData->Blockage);
          defData->Blockage.clear();
        }
      }
#line 9976 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 872: /* $@152: %empty  */
#line 4621 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                           { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9982 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 873: /* $@153: %empty  */
#line 4622 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
          if (defData->Blockage.hasPlacement() != 0) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6539, "Invalid BLOCKAGE statement defined in the DEF file. The BLOCKAGE statment has both the LAYER and the PLACEMENT statements defined.\nUpdate your DEF file to have either BLOCKAGE or PLACEMENT statement only.");
              CHKERR();
            }
          }
          defData->Blockage.setLayer((yyvsp[0].string));
          defData->Blockage.clearPoly(); // free poly, if any
        }
        defData->hasBlkLayerComp = 0;
        defData->hasBlkLayerSpac = 0;
        defData->hasBlkLayerTypeComp = 0;
      }
#line 10002 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 875: /* $@154: %empty  */
#line 4641 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
          if (defData->Blockage.hasLayer() != 0) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6539, "Invalid BLOCKAGE statement defined in the DEF file. The BLOCKAGE statment has both the LAYER and the PLACEMENT statements defined.\nUpdate your DEF file to have either BLOCKAGE or PLACEMENT statement only.");
              CHKERR();
            }
          }
          defData->Blockage.setPlacement();
          defData->Blockage.clearPoly(); // free poly, if any
        }
        defData->hasBlkPlaceComp = 0;
        defData->hasBlkPlaceTypeComp = 0;
      }
#line 10021 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 879: /* layer_blockage_rule: '+' K_SPACING NUMBER  */
#line 4662 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.6) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The SPACING statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
              defData->defError(6540, defData->defMsg);
              free(defData->defMsg);
              CHKERR();
            }
          }
        } else if (defData->hasBlkLayerSpac) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6541, "The SPACING statement is defined in the LAYER statement,\nbut there is already either a SPACING statement or DESIGNRULEWIDTH  statement has defined in the LAYER statement.\nUpdate your DEF file to have either SPACING statement or a DESIGNRULEWIDTH statement.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk)
            defData->Blockage.setSpacing((int)(yyvsp[0].dval));
          defData->hasBlkLayerSpac = 1;
        }
      }
#line 10051 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 880: /* layer_blockage_rule: '+' K_DESIGNRULEWIDTH NUMBER  */
#line 4688 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.6) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6541, "The SPACING statement is defined in the LAYER statement,\nbut there is already either a SPACING statement or DESIGNRULEWIDTH  statement has defined in the LAYER statement.\nUpdate your DEF file to have either SPACING statement or a DESIGNRULEWIDTH statement.");
              CHKERR();
            }
          }
        } else if (defData->hasBlkLayerSpac) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6541, "The SPACING statement is defined in the LAYER statement,\nbut there is already either a SPACING statement or DESIGNRULEWIDTH  statement has defined in the LAYER statement.\nUpdate your DEF file to have either SPACING statement or a DESIGNRULEWIDTH statement.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk)
            defData->Blockage.setDesignRuleWidth((int)(yyvsp[0].dval));
          defData->hasBlkLayerSpac = 1;
        }
      }
#line 10077 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 883: /* mask_blockage_rule: '+' K_MASK NUMBER  */
#line 4714 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {      
        if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->blockageWarnings, defData->settings->BlockageWarnings)) {
          defData->Blockage.setMask((int)(yyvsp[0].dval));
        }
      }
#line 10087 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 884: /* $@155: %empty  */
#line 4722 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10093 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 885: /* comp_blockage_rule: '+' K_COMPONENT $@155 T_STRING  */
#line 4723 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->hasBlkLayerComp) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6542, "The defined BLOCKAGES COMPONENT statement has either COMPONENT, SLOTS, FILLS, PUSHDOWN or EXCEPTPGNET defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES COMPONENT statement per layer.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk) {
            defData->Blockage.setComponent((yyvsp[0].string));
          }
          if (defData->VersionNum < 5.8) {
            defData->hasBlkLayerComp = 1;
          }
        }
      }
#line 10115 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 886: /* comp_blockage_rule: '+' K_SLOTS  */
#line 4742 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->hasBlkLayerComp || defData->hasBlkLayerTypeComp) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6542, "The defined BLOCKAGES COMPONENT statement has either COMPONENT, SLOTS, FILLS, PUSHDOWN or EXCEPTPGNET defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES COMPONENT statement per layer.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk) {
            defData->Blockage.setSlots();
          }
          if (defData->VersionNum < 5.8) {
            defData->hasBlkLayerComp = 1;
          }
          if (defData->VersionNum == 5.8) {
            defData->hasBlkLayerTypeComp = 1;
          }
        }
      }
#line 10140 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 887: /* comp_blockage_rule: '+' K_FILLS  */
#line 4763 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->hasBlkLayerComp || defData->hasBlkLayerTypeComp) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6542, "The defined BLOCKAGES COMPONENT statement has either COMPONENT, SLOTS, FILLS, PUSHDOWN or EXCEPTPGNET defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES COMPONENT statement per layer.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk){
            defData->Blockage.setFills();
          }
          if (defData->VersionNum < 5.8) {
            defData->hasBlkLayerComp = 1;
          }
          if (defData->VersionNum == 5.8) {
            defData->hasBlkLayerTypeComp = 1;
          }
        }
      }
#line 10165 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 888: /* comp_blockage_rule: '+' K_PUSHDOWN  */
#line 4784 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->hasBlkLayerComp) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6542, "The defined BLOCKAGES COMPONENT statement has either COMPONENT, SLOTS, FILLS, PUSHDOWN or EXCEPTPGNET defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES COMPONENT statement per layer.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk){
            defData->Blockage.setPushdown();
          }
          if (defData->VersionNum < 5.8) {
            defData->hasBlkLayerComp = 1;
          }
        }
      }
#line 10187 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 889: /* comp_blockage_rule: '+' K_EXCEPTPGNET  */
#line 4802 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->BlockageCbk) {
             if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
               defData->defMsg = (char*)malloc(10000);
               sprintf (defData->defMsg,
                 "The EXCEPTPGNET is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
               defData->defError(6549, defData->defMsg);
               free(defData->defMsg);
               CHKERR();
              }
           }
        } else {
           if (defData->hasBlkLayerComp) {
             if (defData->callbacks->BlockageCbk) {
               if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
                 defData->defError(6542, "The defined BLOCKAGES COMPONENT statement has either COMPONENT, SLOTS, FILLS, PUSHDOWN or EXCEPTPGNET defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES COMPONENT statement per layer.");
                 CHKERR();
               }
             }
           } else {
             if (defData->callbacks->BlockageCbk){
               defData->Blockage.setExceptpgnet();
             }
             if (defData->VersionNum < 5.8){
               defData->hasBlkLayerComp = 1;
             }
           }
        }
      }
#line 10222 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 892: /* $@156: %empty  */
#line 4839 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10228 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 893: /* placement_comp_rule: '+' K_COMPONENT $@156 T_STRING  */
#line 4840 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->hasBlkPlaceComp) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6543, "The defined BLOCKAGES PLACEMENT statement has either COMPONENT, PUSHDOWN, SOFT or PARTIAL defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES PLACEMENT statement.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk){
            defData->Blockage.setComponent((yyvsp[0].string));
          }
          if (defData->VersionNum < 5.8) {
            defData->hasBlkPlaceComp = 1;
          }
        }
      }
#line 10250 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 894: /* placement_comp_rule: '+' K_PUSHDOWN  */
#line 4858 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->hasBlkPlaceComp) {
          if (defData->callbacks->BlockageCbk) {
            if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
              defData->defError(6543, "The defined BLOCKAGES PLACEMENT statement has either COMPONENT, PUSHDOWN, SOFT or PARTIAL defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES PLACEMENT statement.");
              CHKERR();
            }
          }
        } else {
          if (defData->callbacks->BlockageCbk){
            defData->Blockage.setPushdown();
          }
          if (defData->VersionNum < 5.8) {
            defData->hasBlkPlaceComp = 1;
          }
        }
      }
#line 10272 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 895: /* placement_comp_rule: '+' K_SOFT  */
#line 4876 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->BlockageCbk) {
             if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
               defData->defMsg = (char*)malloc(10000);
               sprintf (defData->defMsg,
                 "The PLACEMENT SOFT is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
               defData->defError(6547, defData->defMsg);
               free(defData->defMsg);
               CHKERR();
             }
           }
        } else {
           if (defData->hasBlkPlaceComp || defData->hasBlkPlaceTypeComp) {
             if (defData->callbacks->BlockageCbk) {
               if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
                 defData->defError(6543, "The defined BLOCKAGES PLACEMENT statement has either COMPONENT, PUSHDOWN, SOFT or PARTIAL defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES PLACEMENT statement.");
                 CHKERR();
               }
             }
           } else {
             if (defData->callbacks->BlockageCbk){
               defData->Blockage.setSoft();
             }
             if (defData->VersionNum < 5.8) {
               defData->hasBlkPlaceComp = 1;
             }
             if (defData->VersionNum == 5.8) {
               defData->hasBlkPlaceTypeComp = 1;
             }
           }
        }
      }
#line 10310 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 896: /* placement_comp_rule: '+' K_PARTIAL NUMBER  */
#line 4910 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->BlockageCbk) {
             if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
                defData->defMsg = (char*)malloc(10000);
                sprintf (defData->defMsg,
                  "The PARTIAL is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
                defData->defError(6548, defData->defMsg);
                free(defData->defMsg);
                CHKERR();
             }
           }
        } else {
           if (defData->hasBlkPlaceComp || defData->hasBlkPlaceTypeComp) {
             if (defData->callbacks->BlockageCbk) {
               if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
                 defData->defError(6543, "The defined BLOCKAGES PLACEMENT statement has either COMPONENT, PUSHDOWN, SOFT or PARTIAL defined.\nOnly one of these statements is allowed per LAYER. Updated the DEF file to define a valid BLOCKAGES PLACEMENT statement.");
                 CHKERR();
               }
             }
           } else {
             if (defData->callbacks->BlockageCbk){
               defData->Blockage.setPartial((yyvsp[0].dval));
             } 
             if (defData->VersionNum < 5.8) {
               defData->hasBlkPlaceComp = 1;
             }
             if (defData->VersionNum == 5.8) {
               defData->hasBlkPlaceTypeComp = 1;
             }
           }
         }
      }
#line 10348 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 899: /* rectPoly_blockage: K_RECT pt pt  */
#line 4950 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk)
          defData->Blockage.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
#line 10357 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 900: /* $@157: %empty  */
#line 4955 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
            defData->Geometries.Reset();
        }
      }
#line 10367 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 901: /* rectPoly_blockage: K_POLYGON $@157 firstPt nextPt nextPt otherPts  */
#line 4961 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
          if (defData->VersionNum >= 5.6) {  // only 5.6 and beyond
            if (defData->Blockage.hasLayer()) {  // only in layer
              if (defData->callbacks->BlockageCbk)
                defData->Blockage.addPolygon(&defData->Geometries);
            } else {
              if (defData->callbacks->BlockageCbk) {
                if (defData->blockageWarnings++ < defData->settings->BlockageWarnings) {
                  defData->defError(6544, "A POLYGON statement is defined in the BLOCKAGE statement,\nbut it is not defined in the BLOCKAGE LAYER statement.\nUpdate your DEF file to either remove the POLYGON statement from the BLOCKAGE statement or\ndefine the POLYGON statement in a BLOCKAGE LAYER statement.");
                  CHKERR();
                }
              }
            }
          }
        }
      }
#line 10389 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 903: /* slot_start: K_SLOTS NUMBER ';'  */
#line 4983 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->SlotStartCbk)
          CALLBACK(defData->callbacks->SlotStartCbk, defrSlotStartCbkType, round((yyvsp[-1].dval))); }
#line 10396 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 904: /* slot_end: K_END K_SLOTS  */
#line 4987 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->SlotEndCbk)
          CALLBACK(defData->callbacks->SlotEndCbk, defrSlotEndCbkType, 0); }
#line 10403 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 907: /* slot_def: slot_rule geom_slot_rules ';'  */
#line 4995 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->SlotCbk) {
          CALLBACK(defData->callbacks->SlotCbk, defrSlotCbkType, &defData->Slot);
          defData->Slot.clear();
        }
      }
#line 10414 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 908: /* $@158: %empty  */
#line 5002 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10420 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 909: /* $@159: %empty  */
#line 5003 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->SlotCbk) {
          defData->Slot.setLayer((yyvsp[0].string));
          defData->Slot.clearPoly();     // free poly, if any
        }
      }
#line 10431 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 913: /* geom_slot: K_RECT pt pt  */
#line 5015 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->SlotCbk)
          defData->Slot.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
#line 10440 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 914: /* $@160: %empty  */
#line 5020 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
          defData->Geometries.Reset();
      }
#line 10448 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 915: /* geom_slot: K_POLYGON $@160 firstPt nextPt nextPt otherPts  */
#line 5024 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum >= 5.6) {  // only 5.6 and beyond
          if (defData->callbacks->SlotCbk)
            defData->Slot.addPolygon(&defData->Geometries);
        }
      }
#line 10459 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 917: /* fill_start: K_FILLS NUMBER ';'  */
#line 5035 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FillStartCbk)
          CALLBACK(defData->callbacks->FillStartCbk, defrFillStartCbkType, round((yyvsp[-1].dval))); }
#line 10466 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 918: /* fill_end: K_END K_FILLS  */
#line 5039 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->FillEndCbk)
          CALLBACK(defData->callbacks->FillEndCbk, defrFillEndCbkType, 0); }
#line 10473 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 921: /* fill_def: fill_rule geom_fill_rules ';'  */
#line 5047 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk) {
          CALLBACK(defData->callbacks->FillCbk, defrFillCbkType, &defData->Fill);
          defData->Fill.clear();
        }
      }
#line 10484 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 922: /* $@161: %empty  */
#line 5053 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10490 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 923: /* $@162: %empty  */
#line 5054 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk) {
          defData->Fill.setVia((yyvsp[0].string));
          defData->Fill.clearPts();
          defData->Geometries.Reset();
        }
      }
#line 10502 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 925: /* $@163: %empty  */
#line 5063 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10508 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 926: /* $@164: %empty  */
#line 5064 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk) {
          defData->Fill.setLayer((yyvsp[0].string));
          defData->Fill.clearPoly();    // free poly, if any
        }
      }
#line 10519 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 930: /* geom_fill: K_RECT pt pt  */
#line 5077 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk)
          defData->Fill.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
#line 10528 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 931: /* $@165: %empty  */
#line 5082 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        defData->Geometries.Reset();
      }
#line 10536 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 932: /* geom_fill: K_POLYGON $@165 firstPt nextPt nextPt otherPts  */
#line 5086 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum >= 5.6) {  // only 5.6 and beyond
          if (defData->callbacks->FillCbk)
            defData->Fill.addPolygon(&defData->Geometries);
        } else {
            defData->defMsg = (char*)malloc(10000);
            sprintf (defData->defMsg,
              "POLYGON statement in FILLS LAYER is a version 5.6 and later syntax.\nYour def file is defined with version %g.", defData->VersionNum);
            defData->defError(6564, defData->defMsg);
            free(defData->defMsg);
            CHKERR();
        }
      }
#line 10554 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 937: /* fill_layer_opc: '+' K_OPC  */
#line 5110 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->FillCbk) {
             if (defData->fillWarnings++ < defData->settings->FillWarnings) {
               defData->defMsg = (char*)malloc(10000);
               sprintf (defData->defMsg,
                 "The LAYER OPC is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
               defData->defError(6553, defData->defMsg);
               free(defData->defMsg);
               CHKERR();
             }
           }
        } else {
           if (defData->callbacks->FillCbk)
             defData->Fill.setLayerOpc();
        }
      }
#line 10576 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 938: /* fill_via_pt: firstPt otherPts  */
#line 5129 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
    {
        if (defData->callbacks->FillCbk) {
          defData->Fill.addPts(&defData->Geometries);
          CALLBACK(defData->callbacks->FillCbk, defrFillCbkType, &defData->Fill);
          defData->Fill.clear();
        }
    }
#line 10588 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 943: /* fill_via_opc: '+' K_OPC  */
#line 5148 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.7) {
           if (defData->callbacks->FillCbk) {
             if (defData->fillWarnings++ < defData->settings->FillWarnings) {
               defData->defMsg = (char*)malloc(10000);
               sprintf (defData->defMsg,
                 "The VIA OPC is available in version 5.7 or later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
               defData->defError(6554, defData->defMsg);
               free(defData->defMsg);
               CHKERR();
             }
           }
        } else {
           if (defData->callbacks->FillCbk)
             defData->Fill.setViaOpc();
        }
      }
#line 10610 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 944: /* fill_mask: '+' K_MASK NUMBER  */
#line 5168 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->fillWarnings, defData->settings->FillWarnings)) {
             if (defData->callbacks->FillCbk) {
                defData->Fill.setMask((int)(yyvsp[0].dval));
             }
        }
      }
#line 10622 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 945: /* fill_viaMask: '+' K_MASK NUMBER  */
#line 5178 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->fillWarnings, defData->settings->FillWarnings)) {
             if (defData->callbacks->FillCbk) {
                defData->Fill.setMask((int)(yyvsp[0].dval));
             }
        }
      }
#line 10634 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 947: /* nondefault_start: K_NONDEFAULTRULES NUMBER ';'  */
#line 5191 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { 
        if (defData->VersionNum < 5.6) {
          if (defData->callbacks->NonDefaultStartCbk) {
            if (defData->nonDefaultWarnings++ < defData->settings->NonDefaultWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The NONDEFAULTRULE statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g.", defData->VersionNum);
              defData->defError(6545, defData->defMsg);
              free(defData->defMsg);
              CHKERR();
            }
          }
        } else if (defData->callbacks->NonDefaultStartCbk)
          CALLBACK(defData->callbacks->NonDefaultStartCbk, defrNonDefaultStartCbkType,
                   round((yyvsp[-1].dval)));
      }
#line 10655 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 948: /* nondefault_end: K_END K_NONDEFAULTRULES  */
#line 5209 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->NonDefaultEndCbk)
          CALLBACK(defData->callbacks->NonDefaultEndCbk, defrNonDefaultEndCbkType, 0); }
#line 10662 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 951: /* $@166: %empty  */
#line 5216 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 952: /* $@167: %empty  */
#line 5217 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.clear(); 
          defData->NonDefault.setName((yyvsp[0].string));
        }
      }
#line 10679 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 953: /* nondefault_def: '-' $@166 T_STRING $@167 nondefault_options ';'  */
#line 5224 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->NonDefaultCbk)
          CALLBACK(defData->callbacks->NonDefaultCbk, defrNonDefaultCbkType, &defData->NonDefault); }
#line 10686 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 956: /* nondefault_option: '+' K_HARDSPACING  */
#line 5232 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk)
          defData->NonDefault.setHardspacing();
      }
#line 10695 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 957: /* $@168: %empty  */
#line 5236 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                    { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10701 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 958: /* $@169: %empty  */
#line 5238 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addLayer((yyvsp[-2].string));
          defData->NonDefault.addWidth((yyvsp[0].dval));
        }
      }
#line 10712 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 960: /* $@170: %empty  */
#line 5245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10718 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 961: /* nondefault_option: '+' K_VIA $@170 T_STRING  */
#line 5246 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addVia((yyvsp[0].string));
        }
      }
#line 10728 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 962: /* $@171: %empty  */
#line 5251 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10734 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 963: /* nondefault_option: '+' K_VIARULE $@171 T_STRING  */
#line 5252 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addViaRule((yyvsp[0].string));
        }
      }
#line 10744 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 964: /* $@172: %empty  */
#line 5257 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10750 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 965: /* nondefault_option: '+' K_MINCUTS $@172 T_STRING NUMBER  */
#line 5258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addMinCuts((yyvsp[-1].string), (int)(yyvsp[0].dval));
        }
      }
#line 10760 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 969: /* nondefault_layer_option: K_DIAGWIDTH NUMBER  */
#line 5271 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addDiagWidth((yyvsp[0].dval));
        }
      }
#line 10770 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 970: /* nondefault_layer_option: K_SPACING NUMBER  */
#line 5277 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addSpacing((yyvsp[0].dval));
        }
      }
#line 10780 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 971: /* nondefault_layer_option: K_WIREEXT NUMBER  */
#line 5283 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addWireExt((yyvsp[0].dval));
        }
      }
#line 10790 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 972: /* $@173: %empty  */
#line 5290 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
                                    { defData->dumb_mode = DEF_MAX_INT;  }
#line 10796 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 973: /* nondefault_prop_opt: '+' K_PROPERTY $@173 nondefault_prop_list  */
#line 5292 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 10802 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 976: /* nondefault_prop: T_STRING NUMBER  */
#line 5299 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          char propTp;
          char* str = defData->ringCopy("                       ");
          propTp = defData->session->NDefProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "NONDEFAULTRULE");
          sprintf(str, "%g", (yyvsp[0].dval));
          defData->NonDefault.addNumProperty((yyvsp[-1].string), (yyvsp[0].dval), str, propTp);
        }
      }
#line 10817 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 977: /* nondefault_prop: T_STRING QSTRING  */
#line 5310 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          char propTp;
          propTp = defData->session->NDefProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "NONDEFAULTRULE");
          defData->NonDefault.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 10830 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 978: /* nondefault_prop: T_STRING T_STRING  */
#line 5319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          char propTp;
          propTp = defData->session->NDefProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "NONDEFAULTRULE");
          defData->NonDefault.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 10843 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 980: /* styles_start: K_STYLES NUMBER ';'  */
#line 5332 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.6) {
          if (defData->callbacks->StylesStartCbk) {
            if (defData->stylesWarnings++ < defData->settings->StylesWarnings) {
              defData->defMsg = (char*)malloc(1000);
              sprintf (defData->defMsg,
                 "The STYLES statement is available in version 5.6 and later.\nHowever, your DEF file is defined with version %g", defData->VersionNum);
              defData->defError(6546, defData->defMsg);
              free(defData->defMsg);
              CHKERR();
            }
          }
        } else if (defData->callbacks->StylesStartCbk)
          CALLBACK(defData->callbacks->StylesStartCbk, defrStylesStartCbkType, round((yyvsp[-1].dval)));
      }
#line 10863 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 981: /* styles_end: K_END K_STYLES  */
#line 5349 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      { if (defData->callbacks->StylesEndCbk)
          CALLBACK(defData->callbacks->StylesEndCbk, defrStylesEndCbkType, 0); }
#line 10870 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 984: /* $@174: %empty  */
#line 5357 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->callbacks->StylesCbk) defData->Styles.setStyle((int)(yyvsp[0].dval));
        defData->Geometries.Reset();
      }
#line 10879 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;

  case 985: /* styles_rule: '-' K_STYLE NUMBER $@174 firstPt nextPt otherPts ';'  */
#line 5362 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"
      {
        if (defData->VersionNum >= 5.6) {  // only 5.6 and beyond will call the callback
          if (defData->callbacks->StylesCbk) {
            defData->Styles.setPolygon(&defData->Geometries);
            CALLBACK(defData->callbacks->StylesCbk, defrStylesCbkType, &defData->Styles);
          }
        }
      }
#line 10892 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"
    break;


#line 10896 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/def/def_parser.cpp"

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
      yyerror (defData, YY_("syntax error"));
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
                      yytoken, &yylval, defData);
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, defData);
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
  yyerror (defData, YY_("memory exhausted"));
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
                  yytoken, &yylval, defData);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, defData);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 5372 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/def/def/def.y"


END_DEF_PARSER_NAMESPACE
