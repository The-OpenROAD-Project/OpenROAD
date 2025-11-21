/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_DEFYY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_ODB_SRC_DEF_DEF_PARSER_HPP_INCLUDED
# define YY_DEFYY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_ODB_SRC_DEF_DEF_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int defyydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    QSTRING = 258,                 /* QSTRING  */
    T_STRING = 259,                /* T_STRING  */
    SITE_PATTERN = 260,            /* SITE_PATTERN  */
    NUMBER = 261,                  /* NUMBER  */
    K_HISTORY = 262,               /* K_HISTORY  */
    K_NAMESCASESENSITIVE = 263,    /* K_NAMESCASESENSITIVE  */
    K_DESIGN = 264,                /* K_DESIGN  */
    K_VIAS = 265,                  /* K_VIAS  */
    K_TECH = 266,                  /* K_TECH  */
    K_UNITS = 267,                 /* K_UNITS  */
    K_ARRAY = 268,                 /* K_ARRAY  */
    K_FLOORPLAN = 269,             /* K_FLOORPLAN  */
    K_SITE = 270,                  /* K_SITE  */
    K_CANPLACE = 271,              /* K_CANPLACE  */
    K_CANNOTOCCUPY = 272,          /* K_CANNOTOCCUPY  */
    K_DIEAREA = 273,               /* K_DIEAREA  */
    K_PINS = 274,                  /* K_PINS  */
    K_DEFAULTCAP = 275,            /* K_DEFAULTCAP  */
    K_MINPINS = 276,               /* K_MINPINS  */
    K_WIRECAP = 277,               /* K_WIRECAP  */
    K_TRACKS = 278,                /* K_TRACKS  */
    K_GCELLGRID = 279,             /* K_GCELLGRID  */
    K_DO = 280,                    /* K_DO  */
    K_BY = 281,                    /* K_BY  */
    K_STEP = 282,                  /* K_STEP  */
    K_LAYER = 283,                 /* K_LAYER  */
    K_ROW = 284,                   /* K_ROW  */
    K_RECT = 285,                  /* K_RECT  */
    K_COMPS = 286,                 /* K_COMPS  */
    K_COMP_GEN = 287,              /* K_COMP_GEN  */
    K_SOURCE = 288,                /* K_SOURCE  */
    K_WEIGHT = 289,                /* K_WEIGHT  */
    K_EEQMASTER = 290,             /* K_EEQMASTER  */
    K_FIXED = 291,                 /* K_FIXED  */
    K_COVER = 292,                 /* K_COVER  */
    K_UNPLACED = 293,              /* K_UNPLACED  */
    K_PLACED = 294,                /* K_PLACED  */
    K_FOREIGN = 295,               /* K_FOREIGN  */
    K_REGION = 296,                /* K_REGION  */
    K_REGIONS = 297,               /* K_REGIONS  */
    K_NETS = 298,                  /* K_NETS  */
    K_START_NET = 299,             /* K_START_NET  */
    K_MUSTJOIN = 300,              /* K_MUSTJOIN  */
    K_ORIGINAL = 301,              /* K_ORIGINAL  */
    K_USE = 302,                   /* K_USE  */
    K_STYLE = 303,                 /* K_STYLE  */
    K_PATTERN = 304,               /* K_PATTERN  */
    K_PATTERNNAME = 305,           /* K_PATTERNNAME  */
    K_ESTCAP = 306,                /* K_ESTCAP  */
    K_ROUTED = 307,                /* K_ROUTED  */
    K_NEW = 308,                   /* K_NEW  */
    K_SNETS = 309,                 /* K_SNETS  */
    K_SHAPE = 310,                 /* K_SHAPE  */
    K_WIDTH = 311,                 /* K_WIDTH  */
    K_VOLTAGE = 312,               /* K_VOLTAGE  */
    K_SPACING = 313,               /* K_SPACING  */
    K_NONDEFAULTRULE = 314,        /* K_NONDEFAULTRULE  */
    K_NONDEFAULTRULES = 315,       /* K_NONDEFAULTRULES  */
    K_N = 316,                     /* K_N  */
    K_S = 317,                     /* K_S  */
    K_E = 318,                     /* K_E  */
    K_W = 319,                     /* K_W  */
    K_FN = 320,                    /* K_FN  */
    K_FE = 321,                    /* K_FE  */
    K_FS = 322,                    /* K_FS  */
    K_FW = 323,                    /* K_FW  */
    K_GROUPS = 324,                /* K_GROUPS  */
    K_GROUP = 325,                 /* K_GROUP  */
    K_SOFT = 326,                  /* K_SOFT  */
    K_MAXX = 327,                  /* K_MAXX  */
    K_MAXY = 328,                  /* K_MAXY  */
    K_MAXHALFPERIMETER = 329,      /* K_MAXHALFPERIMETER  */
    K_CONSTRAINTS = 330,           /* K_CONSTRAINTS  */
    K_NET = 331,                   /* K_NET  */
    K_PATH = 332,                  /* K_PATH  */
    K_SUM = 333,                   /* K_SUM  */
    K_DIFF = 334,                  /* K_DIFF  */
    K_SCANCHAINS = 335,            /* K_SCANCHAINS  */
    K_START = 336,                 /* K_START  */
    K_FLOATING = 337,              /* K_FLOATING  */
    K_ORDERED = 338,               /* K_ORDERED  */
    K_STOP = 339,                  /* K_STOP  */
    K_IN = 340,                    /* K_IN  */
    K_OUT = 341,                   /* K_OUT  */
    K_RISEMIN = 342,               /* K_RISEMIN  */
    K_RISEMAX = 343,               /* K_RISEMAX  */
    K_FALLMIN = 344,               /* K_FALLMIN  */
    K_FALLMAX = 345,               /* K_FALLMAX  */
    K_WIREDLOGIC = 346,            /* K_WIREDLOGIC  */
    K_MAXDIST = 347,               /* K_MAXDIST  */
    K_ASSERTIONS = 348,            /* K_ASSERTIONS  */
    K_DISTANCE = 349,              /* K_DISTANCE  */
    K_MICRONS = 350,               /* K_MICRONS  */
    K_END = 351,                   /* K_END  */
    K_IOTIMINGS = 352,             /* K_IOTIMINGS  */
    K_RISE = 353,                  /* K_RISE  */
    K_FALL = 354,                  /* K_FALL  */
    K_VARIABLE = 355,              /* K_VARIABLE  */
    K_SLEWRATE = 356,              /* K_SLEWRATE  */
    K_CAPACITANCE = 357,           /* K_CAPACITANCE  */
    K_DRIVECELL = 358,             /* K_DRIVECELL  */
    K_FROMPIN = 359,               /* K_FROMPIN  */
    K_TOPIN = 360,                 /* K_TOPIN  */
    K_PARALLEL = 361,              /* K_PARALLEL  */
    K_TIMINGDISABLES = 362,        /* K_TIMINGDISABLES  */
    K_THRUPIN = 363,               /* K_THRUPIN  */
    K_MACRO = 364,                 /* K_MACRO  */
    K_PARTITIONS = 365,            /* K_PARTITIONS  */
    K_TURNOFF = 366,               /* K_TURNOFF  */
    K_FROMCLOCKPIN = 367,          /* K_FROMCLOCKPIN  */
    K_FROMCOMPPIN = 368,           /* K_FROMCOMPPIN  */
    K_FROMIOPIN = 369,             /* K_FROMIOPIN  */
    K_TOCLOCKPIN = 370,            /* K_TOCLOCKPIN  */
    K_TOCOMPPIN = 371,             /* K_TOCOMPPIN  */
    K_TOIOPIN = 372,               /* K_TOIOPIN  */
    K_SETUPRISE = 373,             /* K_SETUPRISE  */
    K_SETUPFALL = 374,             /* K_SETUPFALL  */
    K_HOLDRISE = 375,              /* K_HOLDRISE  */
    K_HOLDFALL = 376,              /* K_HOLDFALL  */
    K_VPIN = 377,                  /* K_VPIN  */
    K_SUBNET = 378,                /* K_SUBNET  */
    K_XTALK = 379,                 /* K_XTALK  */
    K_PIN = 380,                   /* K_PIN  */
    K_SYNTHESIZED = 381,           /* K_SYNTHESIZED  */
    K_DEFINE = 382,                /* K_DEFINE  */
    K_DEFINES = 383,               /* K_DEFINES  */
    K_DEFINEB = 384,               /* K_DEFINEB  */
    K_IF = 385,                    /* K_IF  */
    K_THEN = 386,                  /* K_THEN  */
    K_ELSE = 387,                  /* K_ELSE  */
    K_FALSE = 388,                 /* K_FALSE  */
    K_TRUE = 389,                  /* K_TRUE  */
    K_EQ = 390,                    /* K_EQ  */
    K_NE = 391,                    /* K_NE  */
    K_LE = 392,                    /* K_LE  */
    K_LT = 393,                    /* K_LT  */
    K_GE = 394,                    /* K_GE  */
    K_GT = 395,                    /* K_GT  */
    K_OR = 396,                    /* K_OR  */
    K_AND = 397,                   /* K_AND  */
    K_NOT = 398,                   /* K_NOT  */
    K_SPECIAL = 399,               /* K_SPECIAL  */
    K_DIRECTION = 400,             /* K_DIRECTION  */
    K_RANGE = 401,                 /* K_RANGE  */
    K_FPC = 402,                   /* K_FPC  */
    K_HORIZONTAL = 403,            /* K_HORIZONTAL  */
    K_VERTICAL = 404,              /* K_VERTICAL  */
    K_ALIGN = 405,                 /* K_ALIGN  */
    K_MIN = 406,                   /* K_MIN  */
    K_MAX = 407,                   /* K_MAX  */
    K_EQUAL = 408,                 /* K_EQUAL  */
    K_BOTTOMLEFT = 409,            /* K_BOTTOMLEFT  */
    K_TOPRIGHT = 410,              /* K_TOPRIGHT  */
    K_ROWS = 411,                  /* K_ROWS  */
    K_TAPER = 412,                 /* K_TAPER  */
    K_TAPERRULE = 413,             /* K_TAPERRULE  */
    K_VERSION = 414,               /* K_VERSION  */
    K_DIVIDERCHAR = 415,           /* K_DIVIDERCHAR  */
    K_BUSBITCHARS = 416,           /* K_BUSBITCHARS  */
    K_PROPERTYDEFINITIONS = 417,   /* K_PROPERTYDEFINITIONS  */
    K_STRING = 418,                /* K_STRING  */
    K_REAL = 419,                  /* K_REAL  */
    K_INTEGER = 420,               /* K_INTEGER  */
    K_PROPERTY = 421,              /* K_PROPERTY  */
    K_BEGINEXT = 422,              /* K_BEGINEXT  */
    K_ENDEXT = 423,                /* K_ENDEXT  */
    K_NAMEMAPSTRING = 424,         /* K_NAMEMAPSTRING  */
    K_ON = 425,                    /* K_ON  */
    K_OFF = 426,                   /* K_OFF  */
    K_X = 427,                     /* K_X  */
    K_Y = 428,                     /* K_Y  */
    K_COMPONENT = 429,             /* K_COMPONENT  */
    K_MASK = 430,                  /* K_MASK  */
    K_MASKSHIFT = 431,             /* K_MASKSHIFT  */
    K_COMPSMASKSHIFT = 432,        /* K_COMPSMASKSHIFT  */
    K_SAMEMASK = 433,              /* K_SAMEMASK  */
    K_PINPROPERTIES = 434,         /* K_PINPROPERTIES  */
    K_TEST = 435,                  /* K_TEST  */
    K_COMMONSCANPINS = 436,        /* K_COMMONSCANPINS  */
    K_SNET = 437,                  /* K_SNET  */
    K_COMPONENTPIN = 438,          /* K_COMPONENTPIN  */
    K_REENTRANTPATHS = 439,        /* K_REENTRANTPATHS  */
    K_SHIELD = 440,                /* K_SHIELD  */
    K_SHIELDNET = 441,             /* K_SHIELDNET  */
    K_NOSHIELD = 442,              /* K_NOSHIELD  */
    K_VIRTUAL = 443,               /* K_VIRTUAL  */
    K_ANTENNAPINPARTIALMETALAREA = 444, /* K_ANTENNAPINPARTIALMETALAREA  */
    K_ANTENNAPINPARTIALMETALSIDEAREA = 445, /* K_ANTENNAPINPARTIALMETALSIDEAREA  */
    K_ANTENNAPINGATEAREA = 446,    /* K_ANTENNAPINGATEAREA  */
    K_ANTENNAPINDIFFAREA = 447,    /* K_ANTENNAPINDIFFAREA  */
    K_ANTENNAPINMAXAREACAR = 448,  /* K_ANTENNAPINMAXAREACAR  */
    K_ANTENNAPINMAXSIDEAREACAR = 449, /* K_ANTENNAPINMAXSIDEAREACAR  */
    K_ANTENNAPINPARTIALCUTAREA = 450, /* K_ANTENNAPINPARTIALCUTAREA  */
    K_ANTENNAPINMAXCUTCAR = 451,   /* K_ANTENNAPINMAXCUTCAR  */
    K_SIGNAL = 452,                /* K_SIGNAL  */
    K_POWER = 453,                 /* K_POWER  */
    K_GROUND = 454,                /* K_GROUND  */
    K_CLOCK = 455,                 /* K_CLOCK  */
    K_TIEOFF = 456,                /* K_TIEOFF  */
    K_ANALOG = 457,                /* K_ANALOG  */
    K_SCAN = 458,                  /* K_SCAN  */
    K_RESET = 459,                 /* K_RESET  */
    K_RING = 460,                  /* K_RING  */
    K_STRIPE = 461,                /* K_STRIPE  */
    K_FOLLOWPIN = 462,             /* K_FOLLOWPIN  */
    K_IOWIRE = 463,                /* K_IOWIRE  */
    K_COREWIRE = 464,              /* K_COREWIRE  */
    K_BLOCKWIRE = 465,             /* K_BLOCKWIRE  */
    K_FILLWIRE = 466,              /* K_FILLWIRE  */
    K_BLOCKAGEWIRE = 467,          /* K_BLOCKAGEWIRE  */
    K_PADRING = 468,               /* K_PADRING  */
    K_BLOCKRING = 469,             /* K_BLOCKRING  */
    K_BLOCKAGES = 470,             /* K_BLOCKAGES  */
    K_PLACEMENT = 471,             /* K_PLACEMENT  */
    K_SLOTS = 472,                 /* K_SLOTS  */
    K_FILLS = 473,                 /* K_FILLS  */
    K_PUSHDOWN = 474,              /* K_PUSHDOWN  */
    K_NETLIST = 475,               /* K_NETLIST  */
    K_DIST = 476,                  /* K_DIST  */
    K_USER = 477,                  /* K_USER  */
    K_TIMING = 478,                /* K_TIMING  */
    K_BALANCED = 479,              /* K_BALANCED  */
    K_STEINER = 480,               /* K_STEINER  */
    K_TRUNK = 481,                 /* K_TRUNK  */
    K_FIXEDBUMP = 482,             /* K_FIXEDBUMP  */
    K_FENCE = 483,                 /* K_FENCE  */
    K_FREQUENCY = 484,             /* K_FREQUENCY  */
    K_GUIDE = 485,                 /* K_GUIDE  */
    K_MAXBITS = 486,               /* K_MAXBITS  */
    K_PARTITION = 487,             /* K_PARTITION  */
    K_TYPE = 488,                  /* K_TYPE  */
    K_ANTENNAMODEL = 489,          /* K_ANTENNAMODEL  */
    K_DRCFILL = 490,               /* K_DRCFILL  */
    K_OXIDE1 = 491,                /* K_OXIDE1  */
    K_OXIDE2 = 492,                /* K_OXIDE2  */
    K_OXIDE3 = 493,                /* K_OXIDE3  */
    K_OXIDE4 = 494,                /* K_OXIDE4  */
    K_OXIDE5 = 495,                /* K_OXIDE5  */
    K_OXIDE6 = 496,                /* K_OXIDE6  */
    K_OXIDE7 = 497,                /* K_OXIDE7  */
    K_OXIDE8 = 498,                /* K_OXIDE8  */
    K_OXIDE9 = 499,                /* K_OXIDE9  */
    K_OXIDE10 = 500,               /* K_OXIDE10  */
    K_OXIDE11 = 501,               /* K_OXIDE11  */
    K_OXIDE12 = 502,               /* K_OXIDE12  */
    K_OXIDE13 = 503,               /* K_OXIDE13  */
    K_OXIDE14 = 504,               /* K_OXIDE14  */
    K_OXIDE15 = 505,               /* K_OXIDE15  */
    K_OXIDE16 = 506,               /* K_OXIDE16  */
    K_OXIDE17 = 507,               /* K_OXIDE17  */
    K_OXIDE18 = 508,               /* K_OXIDE18  */
    K_OXIDE19 = 509,               /* K_OXIDE19  */
    K_OXIDE20 = 510,               /* K_OXIDE20  */
    K_OXIDE21 = 511,               /* K_OXIDE21  */
    K_OXIDE22 = 512,               /* K_OXIDE22  */
    K_OXIDE23 = 513,               /* K_OXIDE23  */
    K_OXIDE24 = 514,               /* K_OXIDE24  */
    K_OXIDE25 = 515,               /* K_OXIDE25  */
    K_OXIDE26 = 516,               /* K_OXIDE26  */
    K_OXIDE27 = 517,               /* K_OXIDE27  */
    K_OXIDE28 = 518,               /* K_OXIDE28  */
    K_OXIDE29 = 519,               /* K_OXIDE29  */
    K_OXIDE30 = 520,               /* K_OXIDE30  */
    K_OXIDE31 = 521,               /* K_OXIDE31  */
    K_OXIDE32 = 522,               /* K_OXIDE32  */
    K_CUTSIZE = 523,               /* K_CUTSIZE  */
    K_CUTSPACING = 524,            /* K_CUTSPACING  */
    K_DESIGNRULEWIDTH = 525,       /* K_DESIGNRULEWIDTH  */
    K_DIAGWIDTH = 526,             /* K_DIAGWIDTH  */
    K_ENCLOSURE = 527,             /* K_ENCLOSURE  */
    K_HALO = 528,                  /* K_HALO  */
    K_GROUNDSENSITIVITY = 529,     /* K_GROUNDSENSITIVITY  */
    K_HARDSPACING = 530,           /* K_HARDSPACING  */
    K_LAYERS = 531,                /* K_LAYERS  */
    K_MINCUTS = 532,               /* K_MINCUTS  */
    K_NETEXPR = 533,               /* K_NETEXPR  */
    K_OFFSET = 534,                /* K_OFFSET  */
    K_ORIGIN = 535,                /* K_ORIGIN  */
    K_ROWCOL = 536,                /* K_ROWCOL  */
    K_STYLES = 537,                /* K_STYLES  */
    K_POLYGON = 538,               /* K_POLYGON  */
    K_PORT = 539,                  /* K_PORT  */
    K_SUPPLYSENSITIVITY = 540,     /* K_SUPPLYSENSITIVITY  */
    K_VIA = 541,                   /* K_VIA  */
    K_VIARULE = 542,               /* K_VIARULE  */
    K_WIREEXT = 543,               /* K_WIREEXT  */
    K_EXCEPTPGNET = 544,           /* K_EXCEPTPGNET  */
    K_FILLWIREOPC = 545,           /* K_FILLWIREOPC  */
    K_OPC = 546,                   /* K_OPC  */
    K_PARTIAL = 547,               /* K_PARTIAL  */
    K_ROUTEHALO = 548              /* K_ROUTEHALO  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */




int defyyparse (DefParser::defrData *defData);


#endif /* !YY_DEFYY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_ODB_SRC_DEF_DEF_PARSER_HPP_INCLUDED  */
