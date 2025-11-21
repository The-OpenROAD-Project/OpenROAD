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

#ifndef YY_LEFYY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_ODB_SRC_LEF_LEF_PARSER_HPP_INCLUDED
# define YY_LEFYY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_ODB_SRC_LEF_LEF_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int lefyydebug;
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
    K_HISTORY = 258,               /* K_HISTORY  */
    K_ABUT = 259,                  /* K_ABUT  */
    K_ABUTMENT = 260,              /* K_ABUTMENT  */
    K_ACTIVE = 261,                /* K_ACTIVE  */
    K_ANALOG = 262,                /* K_ANALOG  */
    K_ARRAY = 263,                 /* K_ARRAY  */
    K_AREA = 264,                  /* K_AREA  */
    K_BLOCK = 265,                 /* K_BLOCK  */
    K_BOTTOMLEFT = 266,            /* K_BOTTOMLEFT  */
    K_BOTTOMRIGHT = 267,           /* K_BOTTOMRIGHT  */
    K_BY = 268,                    /* K_BY  */
    K_CAPACITANCE = 269,           /* K_CAPACITANCE  */
    K_CAPMULTIPLIER = 270,         /* K_CAPMULTIPLIER  */
    K_CLASS = 271,                 /* K_CLASS  */
    K_CLOCK = 272,                 /* K_CLOCK  */
    K_CLOCKTYPE = 273,             /* K_CLOCKTYPE  */
    K_COLUMNMAJOR = 274,           /* K_COLUMNMAJOR  */
    K_DESIGNRULEWIDTH = 275,       /* K_DESIGNRULEWIDTH  */
    K_INFLUENCE = 276,             /* K_INFLUENCE  */
    K_CORE = 277,                  /* K_CORE  */
    K_CORNER = 278,                /* K_CORNER  */
    K_COVER = 279,                 /* K_COVER  */
    K_CPERSQDIST = 280,            /* K_CPERSQDIST  */
    K_CURRENT = 281,               /* K_CURRENT  */
    K_CURRENTSOURCE = 282,         /* K_CURRENTSOURCE  */
    K_CUT = 283,                   /* K_CUT  */
    K_DEFAULT = 284,               /* K_DEFAULT  */
    K_DATABASE = 285,              /* K_DATABASE  */
    K_DATA = 286,                  /* K_DATA  */
    K_DIELECTRIC = 287,            /* K_DIELECTRIC  */
    K_DIRECTION = 288,             /* K_DIRECTION  */
    K_DO = 289,                    /* K_DO  */
    K_EDGECAPACITANCE = 290,       /* K_EDGECAPACITANCE  */
    K_EEQ = 291,                   /* K_EEQ  */
    K_END = 292,                   /* K_END  */
    K_ENDCAP = 293,                /* K_ENDCAP  */
    K_FALL = 294,                  /* K_FALL  */
    K_FALLCS = 295,                /* K_FALLCS  */
    K_FALLT0 = 296,                /* K_FALLT0  */
    K_FALLSATT1 = 297,             /* K_FALLSATT1  */
    K_FALLRS = 298,                /* K_FALLRS  */
    K_FALLSATCUR = 299,            /* K_FALLSATCUR  */
    K_FALLTHRESH = 300,            /* K_FALLTHRESH  */
    K_FEEDTHRU = 301,              /* K_FEEDTHRU  */
    K_FIXED = 302,                 /* K_FIXED  */
    K_FOREIGN = 303,               /* K_FOREIGN  */
    K_FROMPIN = 304,               /* K_FROMPIN  */
    K_GENERATE = 305,              /* K_GENERATE  */
    K_GENERATOR = 306,             /* K_GENERATOR  */
    K_GROUND = 307,                /* K_GROUND  */
    K_HEIGHT = 308,                /* K_HEIGHT  */
    K_HORIZONTAL = 309,            /* K_HORIZONTAL  */
    K_INOUT = 310,                 /* K_INOUT  */
    K_INPUT = 311,                 /* K_INPUT  */
    K_INPUTNOISEMARGIN = 312,      /* K_INPUTNOISEMARGIN  */
    K_COMPONENTPIN = 313,          /* K_COMPONENTPIN  */
    K_INTRINSIC = 314,             /* K_INTRINSIC  */
    K_INVERT = 315,                /* K_INVERT  */
    K_IRDROP = 316,                /* K_IRDROP  */
    K_ITERATE = 317,               /* K_ITERATE  */
    K_IV_TABLES = 318,             /* K_IV_TABLES  */
    K_LAYER = 319,                 /* K_LAYER  */
    K_LEAKAGE = 320,               /* K_LEAKAGE  */
    K_LEQ = 321,                   /* K_LEQ  */
    K_LIBRARY = 322,               /* K_LIBRARY  */
    K_MACRO = 323,                 /* K_MACRO  */
    K_MATCH = 324,                 /* K_MATCH  */
    K_MAXDELAY = 325,              /* K_MAXDELAY  */
    K_MAXLOAD = 326,               /* K_MAXLOAD  */
    K_METALOVERHANG = 327,         /* K_METALOVERHANG  */
    K_MILLIAMPS = 328,             /* K_MILLIAMPS  */
    K_MILLIWATTS = 329,            /* K_MILLIWATTS  */
    K_MINFEATURE = 330,            /* K_MINFEATURE  */
    K_MUSTJOIN = 331,              /* K_MUSTJOIN  */
    K_NAMESCASESENSITIVE = 332,    /* K_NAMESCASESENSITIVE  */
    K_NANOSECONDS = 333,           /* K_NANOSECONDS  */
    K_NETS = 334,                  /* K_NETS  */
    K_NEW = 335,                   /* K_NEW  */
    K_NONDEFAULTRULE = 336,        /* K_NONDEFAULTRULE  */
    K_NONINVERT = 337,             /* K_NONINVERT  */
    K_NONUNATE = 338,              /* K_NONUNATE  */
    K_OBS = 339,                   /* K_OBS  */
    K_OHMS = 340,                  /* K_OHMS  */
    K_OFFSET = 341,                /* K_OFFSET  */
    K_ORIENTATION = 342,           /* K_ORIENTATION  */
    K_ORIGIN = 343,                /* K_ORIGIN  */
    K_OUTPUT = 344,                /* K_OUTPUT  */
    K_OUTPUTNOISEMARGIN = 345,     /* K_OUTPUTNOISEMARGIN  */
    K_OVERHANG = 346,              /* K_OVERHANG  */
    K_OVERLAP = 347,               /* K_OVERLAP  */
    K_OFF = 348,                   /* K_OFF  */
    K_ON = 349,                    /* K_ON  */
    K_OVERLAPS = 350,              /* K_OVERLAPS  */
    K_PAD = 351,                   /* K_PAD  */
    K_PATH = 352,                  /* K_PATH  */
    K_PATTERN = 353,               /* K_PATTERN  */
    K_PICOFARADS = 354,            /* K_PICOFARADS  */
    K_PIN = 355,                   /* K_PIN  */
    K_PITCH = 356,                 /* K_PITCH  */
    K_PLACED = 357,                /* K_PLACED  */
    K_POLYGON = 358,               /* K_POLYGON  */
    K_PORT = 359,                  /* K_PORT  */
    K_POST = 360,                  /* K_POST  */
    K_POWER = 361,                 /* K_POWER  */
    K_PRE = 362,                   /* K_PRE  */
    K_PULLDOWNRES = 363,           /* K_PULLDOWNRES  */
    K_RECT = 364,                  /* K_RECT  */
    K_RESISTANCE = 365,            /* K_RESISTANCE  */
    K_RESISTIVE = 366,             /* K_RESISTIVE  */
    K_RING = 367,                  /* K_RING  */
    K_RISE = 368,                  /* K_RISE  */
    K_RISECS = 369,                /* K_RISECS  */
    K_RISERS = 370,                /* K_RISERS  */
    K_RISESATCUR = 371,            /* K_RISESATCUR  */
    K_RISETHRESH = 372,            /* K_RISETHRESH  */
    K_RISESATT1 = 373,             /* K_RISESATT1  */
    K_RISET0 = 374,                /* K_RISET0  */
    K_RISEVOLTAGETHRESHOLD = 375,  /* K_RISEVOLTAGETHRESHOLD  */
    K_FALLVOLTAGETHRESHOLD = 376,  /* K_FALLVOLTAGETHRESHOLD  */
    K_ROUTING = 377,               /* K_ROUTING  */
    K_ROWMAJOR = 378,              /* K_ROWMAJOR  */
    K_RPERSQ = 379,                /* K_RPERSQ  */
    K_SAMENET = 380,               /* K_SAMENET  */
    K_SCANUSE = 381,               /* K_SCANUSE  */
    K_SHAPE = 382,                 /* K_SHAPE  */
    K_SHRINKAGE = 383,             /* K_SHRINKAGE  */
    K_SIGNAL = 384,                /* K_SIGNAL  */
    K_SITE = 385,                  /* K_SITE  */
    K_SIZE = 386,                  /* K_SIZE  */
    K_SOURCE = 387,                /* K_SOURCE  */
    K_SPACER = 388,                /* K_SPACER  */
    K_SPACING = 389,               /* K_SPACING  */
    K_SPECIALNETS = 390,           /* K_SPECIALNETS  */
    K_STACK = 391,                 /* K_STACK  */
    K_START = 392,                 /* K_START  */
    K_STEP = 393,                  /* K_STEP  */
    K_STOP = 394,                  /* K_STOP  */
    K_STRUCTURE = 395,             /* K_STRUCTURE  */
    K_SYMMETRY = 396,              /* K_SYMMETRY  */
    K_TABLE = 397,                 /* K_TABLE  */
    K_THICKNESS = 398,             /* K_THICKNESS  */
    K_TIEHIGH = 399,               /* K_TIEHIGH  */
    K_TIELOW = 400,                /* K_TIELOW  */
    K_TIEOFFR = 401,               /* K_TIEOFFR  */
    K_TIME = 402,                  /* K_TIME  */
    K_TIMING = 403,                /* K_TIMING  */
    K_TO = 404,                    /* K_TO  */
    K_TOPIN = 405,                 /* K_TOPIN  */
    K_TOPLEFT = 406,               /* K_TOPLEFT  */
    K_TOPRIGHT = 407,              /* K_TOPRIGHT  */
    K_TOPOFSTACKONLY = 408,        /* K_TOPOFSTACKONLY  */
    K_TRISTATE = 409,              /* K_TRISTATE  */
    K_TYPE = 410,                  /* K_TYPE  */
    K_UNATENESS = 411,             /* K_UNATENESS  */
    K_UNITS = 412,                 /* K_UNITS  */
    K_USE = 413,                   /* K_USE  */
    K_VARIABLE = 414,              /* K_VARIABLE  */
    K_VERTICAL = 415,              /* K_VERTICAL  */
    K_VHI = 416,                   /* K_VHI  */
    K_VIA = 417,                   /* K_VIA  */
    K_VIARULE = 418,               /* K_VIARULE  */
    K_VLO = 419,                   /* K_VLO  */
    K_VOLTAGE = 420,               /* K_VOLTAGE  */
    K_VOLTS = 421,                 /* K_VOLTS  */
    K_WIDTH = 422,                 /* K_WIDTH  */
    K_X = 423,                     /* K_X  */
    K_Y = 424,                     /* K_Y  */
    T_STRING = 425,                /* T_STRING  */
    QSTRING = 426,                 /* QSTRING  */
    NUMBER = 427,                  /* NUMBER  */
    K_N = 428,                     /* K_N  */
    K_S = 429,                     /* K_S  */
    K_E = 430,                     /* K_E  */
    K_W = 431,                     /* K_W  */
    K_FN = 432,                    /* K_FN  */
    K_FS = 433,                    /* K_FS  */
    K_FE = 434,                    /* K_FE  */
    K_FW = 435,                    /* K_FW  */
    K_R0 = 436,                    /* K_R0  */
    K_R90 = 437,                   /* K_R90  */
    K_R180 = 438,                  /* K_R180  */
    K_R270 = 439,                  /* K_R270  */
    K_MX = 440,                    /* K_MX  */
    K_MY = 441,                    /* K_MY  */
    K_MXR90 = 442,                 /* K_MXR90  */
    K_MYR90 = 443,                 /* K_MYR90  */
    K_USER = 444,                  /* K_USER  */
    K_MASTERSLICE = 445,           /* K_MASTERSLICE  */
    K_ENDMACRO = 446,              /* K_ENDMACRO  */
    K_ENDMACROPIN = 447,           /* K_ENDMACROPIN  */
    K_ENDVIARULE = 448,            /* K_ENDVIARULE  */
    K_ENDVIA = 449,                /* K_ENDVIA  */
    K_ENDLAYER = 450,              /* K_ENDLAYER  */
    K_ENDSITE = 451,               /* K_ENDSITE  */
    K_CANPLACE = 452,              /* K_CANPLACE  */
    K_CANNOTOCCUPY = 453,          /* K_CANNOTOCCUPY  */
    K_TRACKS = 454,                /* K_TRACKS  */
    K_FLOORPLAN = 455,             /* K_FLOORPLAN  */
    K_GCELLGRID = 456,             /* K_GCELLGRID  */
    K_DEFAULTCAP = 457,            /* K_DEFAULTCAP  */
    K_MINPINS = 458,               /* K_MINPINS  */
    K_WIRECAP = 459,               /* K_WIRECAP  */
    K_STABLE = 460,                /* K_STABLE  */
    K_SETUP = 461,                 /* K_SETUP  */
    K_HOLD = 462,                  /* K_HOLD  */
    K_DEFINE = 463,                /* K_DEFINE  */
    K_DEFINES = 464,               /* K_DEFINES  */
    K_DEFINEB = 465,               /* K_DEFINEB  */
    K_IF = 466,                    /* K_IF  */
    K_THEN = 467,                  /* K_THEN  */
    K_ELSE = 468,                  /* K_ELSE  */
    K_FALSE = 469,                 /* K_FALSE  */
    K_TRUE = 470,                  /* K_TRUE  */
    K_EQ = 471,                    /* K_EQ  */
    K_NE = 472,                    /* K_NE  */
    K_LE = 473,                    /* K_LE  */
    K_LT = 474,                    /* K_LT  */
    K_GE = 475,                    /* K_GE  */
    K_GT = 476,                    /* K_GT  */
    K_OR = 477,                    /* K_OR  */
    K_AND = 478,                   /* K_AND  */
    K_NOT = 479,                   /* K_NOT  */
    K_DELAY = 480,                 /* K_DELAY  */
    K_TABLEDIMENSION = 481,        /* K_TABLEDIMENSION  */
    K_TABLEAXIS = 482,             /* K_TABLEAXIS  */
    K_TABLEENTRIES = 483,          /* K_TABLEENTRIES  */
    K_TRANSITIONTIME = 484,        /* K_TRANSITIONTIME  */
    K_EXTENSION = 485,             /* K_EXTENSION  */
    K_PROPDEF = 486,               /* K_PROPDEF  */
    K_STRING = 487,                /* K_STRING  */
    K_INTEGER = 488,               /* K_INTEGER  */
    K_REAL = 489,                  /* K_REAL  */
    K_RANGE = 490,                 /* K_RANGE  */
    K_PROPERTY = 491,              /* K_PROPERTY  */
    K_VIRTUAL = 492,               /* K_VIRTUAL  */
    K_BUSBITCHARS = 493,           /* K_BUSBITCHARS  */
    K_VERSION = 494,               /* K_VERSION  */
    K_BEGINEXT = 495,              /* K_BEGINEXT  */
    K_ENDEXT = 496,                /* K_ENDEXT  */
    K_UNIVERSALNOISEMARGIN = 497,  /* K_UNIVERSALNOISEMARGIN  */
    K_EDGERATETHRESHOLD1 = 498,    /* K_EDGERATETHRESHOLD1  */
    K_CORRECTIONTABLE = 499,       /* K_CORRECTIONTABLE  */
    K_EDGERATESCALEFACTOR = 500,   /* K_EDGERATESCALEFACTOR  */
    K_EDGERATETHRESHOLD2 = 501,    /* K_EDGERATETHRESHOLD2  */
    K_VICTIMNOISE = 502,           /* K_VICTIMNOISE  */
    K_NOISETABLE = 503,            /* K_NOISETABLE  */
    K_EDGERATE = 504,              /* K_EDGERATE  */
    K_OUTPUTRESISTANCE = 505,      /* K_OUTPUTRESISTANCE  */
    K_VICTIMLENGTH = 506,          /* K_VICTIMLENGTH  */
    K_CORRECTIONFACTOR = 507,      /* K_CORRECTIONFACTOR  */
    K_OUTPUTPINANTENNASIZE = 508,  /* K_OUTPUTPINANTENNASIZE  */
    K_INPUTPINANTENNASIZE = 509,   /* K_INPUTPINANTENNASIZE  */
    K_INOUTPINANTENNASIZE = 510,   /* K_INOUTPINANTENNASIZE  */
    K_CURRENTDEN = 511,            /* K_CURRENTDEN  */
    K_PWL = 512,                   /* K_PWL  */
    K_ANTENNALENGTHFACTOR = 513,   /* K_ANTENNALENGTHFACTOR  */
    K_TAPERRULE = 514,             /* K_TAPERRULE  */
    K_DIVIDERCHAR = 515,           /* K_DIVIDERCHAR  */
    K_ANTENNASIZE = 516,           /* K_ANTENNASIZE  */
    K_ANTENNAMETALLENGTH = 517,    /* K_ANTENNAMETALLENGTH  */
    K_ANTENNAMETALAREA = 518,      /* K_ANTENNAMETALAREA  */
    K_RISESLEWLIMIT = 519,         /* K_RISESLEWLIMIT  */
    K_FALLSLEWLIMIT = 520,         /* K_FALLSLEWLIMIT  */
    K_FUNCTION = 521,              /* K_FUNCTION  */
    K_BUFFER = 522,                /* K_BUFFER  */
    K_INVERTER = 523,              /* K_INVERTER  */
    K_NAMEMAPSTRING = 524,         /* K_NAMEMAPSTRING  */
    K_NOWIREEXTENSIONATPIN = 525,  /* K_NOWIREEXTENSIONATPIN  */
    K_WIREEXTENSION = 526,         /* K_WIREEXTENSION  */
    K_MESSAGE = 527,               /* K_MESSAGE  */
    K_CREATEFILE = 528,            /* K_CREATEFILE  */
    K_OPENFILE = 529,              /* K_OPENFILE  */
    K_CLOSEFILE = 530,             /* K_CLOSEFILE  */
    K_WARNING = 531,               /* K_WARNING  */
    K_ERROR = 532,                 /* K_ERROR  */
    K_FATALERROR = 533,            /* K_FATALERROR  */
    K_RECOVERY = 534,              /* K_RECOVERY  */
    K_SKEW = 535,                  /* K_SKEW  */
    K_ANYEDGE = 536,               /* K_ANYEDGE  */
    K_POSEDGE = 537,               /* K_POSEDGE  */
    K_NEGEDGE = 538,               /* K_NEGEDGE  */
    K_SDFCONDSTART = 539,          /* K_SDFCONDSTART  */
    K_SDFCONDEND = 540,            /* K_SDFCONDEND  */
    K_SDFCOND = 541,               /* K_SDFCOND  */
    K_MPWH = 542,                  /* K_MPWH  */
    K_MPWL = 543,                  /* K_MPWL  */
    K_PERIOD = 544,                /* K_PERIOD  */
    K_ACCURRENTDENSITY = 545,      /* K_ACCURRENTDENSITY  */
    K_DCCURRENTDENSITY = 546,      /* K_DCCURRENTDENSITY  */
    K_AVERAGE = 547,               /* K_AVERAGE  */
    K_PEAK = 548,                  /* K_PEAK  */
    K_RMS = 549,                   /* K_RMS  */
    K_FREQUENCY = 550,             /* K_FREQUENCY  */
    K_CUTAREA = 551,               /* K_CUTAREA  */
    K_MEGAHERTZ = 552,             /* K_MEGAHERTZ  */
    K_USELENGTHTHRESHOLD = 553,    /* K_USELENGTHTHRESHOLD  */
    K_LENGTHTHRESHOLD = 554,       /* K_LENGTHTHRESHOLD  */
    K_ANTENNAINPUTGATEAREA = 555,  /* K_ANTENNAINPUTGATEAREA  */
    K_ANTENNAINOUTDIFFAREA = 556,  /* K_ANTENNAINOUTDIFFAREA  */
    K_ANTENNAOUTPUTDIFFAREA = 557, /* K_ANTENNAOUTPUTDIFFAREA  */
    K_ANTENNAAREARATIO = 558,      /* K_ANTENNAAREARATIO  */
    K_ANTENNADIFFAREARATIO = 559,  /* K_ANTENNADIFFAREARATIO  */
    K_ANTENNACUMAREARATIO = 560,   /* K_ANTENNACUMAREARATIO  */
    K_ANTENNACUMDIFFAREARATIO = 561, /* K_ANTENNACUMDIFFAREARATIO  */
    K_ANTENNAAREAFACTOR = 562,     /* K_ANTENNAAREAFACTOR  */
    K_ANTENNASIDEAREARATIO = 563,  /* K_ANTENNASIDEAREARATIO  */
    K_ANTENNADIFFSIDEAREARATIO = 564, /* K_ANTENNADIFFSIDEAREARATIO  */
    K_ANTENNACUMSIDEAREARATIO = 565, /* K_ANTENNACUMSIDEAREARATIO  */
    K_ANTENNACUMDIFFSIDEAREARATIO = 566, /* K_ANTENNACUMDIFFSIDEAREARATIO  */
    K_ANTENNASIDEAREAFACTOR = 567, /* K_ANTENNASIDEAREAFACTOR  */
    K_DIFFUSEONLY = 568,           /* K_DIFFUSEONLY  */
    K_MANUFACTURINGGRID = 569,     /* K_MANUFACTURINGGRID  */
    K_FIXEDMASK = 570,             /* K_FIXEDMASK  */
    K_ANTENNACELL = 571,           /* K_ANTENNACELL  */
    K_CLEARANCEMEASURE = 572,      /* K_CLEARANCEMEASURE  */
    K_EUCLIDEAN = 573,             /* K_EUCLIDEAN  */
    K_MAXXY = 574,                 /* K_MAXXY  */
    K_USEMINSPACING = 575,         /* K_USEMINSPACING  */
    K_ROWMINSPACING = 576,         /* K_ROWMINSPACING  */
    K_ROWABUTSPACING = 577,        /* K_ROWABUTSPACING  */
    K_FLIP = 578,                  /* K_FLIP  */
    K_NONE = 579,                  /* K_NONE  */
    K_ANTENNAPARTIALMETALAREA = 580, /* K_ANTENNAPARTIALMETALAREA  */
    K_ANTENNAPARTIALMETALSIDEAREA = 581, /* K_ANTENNAPARTIALMETALSIDEAREA  */
    K_ANTENNAGATEAREA = 582,       /* K_ANTENNAGATEAREA  */
    K_ANTENNADIFFAREA = 583,       /* K_ANTENNADIFFAREA  */
    K_ANTENNAMAXAREACAR = 584,     /* K_ANTENNAMAXAREACAR  */
    K_ANTENNAMAXSIDEAREACAR = 585, /* K_ANTENNAMAXSIDEAREACAR  */
    K_ANTENNAPARTIALCUTAREA = 586, /* K_ANTENNAPARTIALCUTAREA  */
    K_ANTENNAMAXCUTCAR = 587,      /* K_ANTENNAMAXCUTCAR  */
    K_SLOTWIREWIDTH = 588,         /* K_SLOTWIREWIDTH  */
    K_SLOTWIRELENGTH = 589,        /* K_SLOTWIRELENGTH  */
    K_SLOTWIDTH = 590,             /* K_SLOTWIDTH  */
    K_SLOTLENGTH = 591,            /* K_SLOTLENGTH  */
    K_MAXADJACENTSLOTSPACING = 592, /* K_MAXADJACENTSLOTSPACING  */
    K_MAXCOAXIALSLOTSPACING = 593, /* K_MAXCOAXIALSLOTSPACING  */
    K_MAXEDGESLOTSPACING = 594,    /* K_MAXEDGESLOTSPACING  */
    K_SPLITWIREWIDTH = 595,        /* K_SPLITWIREWIDTH  */
    K_MINIMUMDENSITY = 596,        /* K_MINIMUMDENSITY  */
    K_MAXIMUMDENSITY = 597,        /* K_MAXIMUMDENSITY  */
    K_DENSITYCHECKWINDOW = 598,    /* K_DENSITYCHECKWINDOW  */
    K_DENSITYCHECKSTEP = 599,      /* K_DENSITYCHECKSTEP  */
    K_FILLACTIVESPACING = 600,     /* K_FILLACTIVESPACING  */
    K_MINIMUMCUT = 601,            /* K_MINIMUMCUT  */
    K_ADJACENTCUTS = 602,          /* K_ADJACENTCUTS  */
    K_ANTENNAMODEL = 603,          /* K_ANTENNAMODEL  */
    K_BUMP = 604,                  /* K_BUMP  */
    K_ENCLOSURE = 605,             /* K_ENCLOSURE  */
    K_FROMABOVE = 606,             /* K_FROMABOVE  */
    K_FROMBELOW = 607,             /* K_FROMBELOW  */
    K_IMPLANT = 608,               /* K_IMPLANT  */
    K_LENGTH = 609,                /* K_LENGTH  */
    K_MAXVIASTACK = 610,           /* K_MAXVIASTACK  */
    K_AREAIO = 611,                /* K_AREAIO  */
    K_BLACKBOX = 612,              /* K_BLACKBOX  */
    K_MAXWIDTH = 613,              /* K_MAXWIDTH  */
    K_MINENCLOSEDAREA = 614,       /* K_MINENCLOSEDAREA  */
    K_MINSTEP = 615,               /* K_MINSTEP  */
    K_ORIENT = 616,                /* K_ORIENT  */
    K_OXIDE1 = 617,                /* K_OXIDE1  */
    K_OXIDE2 = 618,                /* K_OXIDE2  */
    K_OXIDE3 = 619,                /* K_OXIDE3  */
    K_OXIDE4 = 620,                /* K_OXIDE4  */
    K_OXIDE5 = 621,                /* K_OXIDE5  */
    K_OXIDE6 = 622,                /* K_OXIDE6  */
    K_OXIDE7 = 623,                /* K_OXIDE7  */
    K_OXIDE8 = 624,                /* K_OXIDE8  */
    K_OXIDE9 = 625,                /* K_OXIDE9  */
    K_OXIDE10 = 626,               /* K_OXIDE10  */
    K_OXIDE11 = 627,               /* K_OXIDE11  */
    K_OXIDE12 = 628,               /* K_OXIDE12  */
    K_OXIDE13 = 629,               /* K_OXIDE13  */
    K_OXIDE14 = 630,               /* K_OXIDE14  */
    K_OXIDE15 = 631,               /* K_OXIDE15  */
    K_OXIDE16 = 632,               /* K_OXIDE16  */
    K_OXIDE17 = 633,               /* K_OXIDE17  */
    K_OXIDE18 = 634,               /* K_OXIDE18  */
    K_OXIDE19 = 635,               /* K_OXIDE19  */
    K_OXIDE20 = 636,               /* K_OXIDE20  */
    K_OXIDE21 = 637,               /* K_OXIDE21  */
    K_OXIDE22 = 638,               /* K_OXIDE22  */
    K_OXIDE23 = 639,               /* K_OXIDE23  */
    K_OXIDE24 = 640,               /* K_OXIDE24  */
    K_OXIDE25 = 641,               /* K_OXIDE25  */
    K_OXIDE26 = 642,               /* K_OXIDE26  */
    K_OXIDE27 = 643,               /* K_OXIDE27  */
    K_OXIDE28 = 644,               /* K_OXIDE28  */
    K_OXIDE29 = 645,               /* K_OXIDE29  */
    K_OXIDE30 = 646,               /* K_OXIDE30  */
    K_OXIDE31 = 647,               /* K_OXIDE31  */
    K_OXIDE32 = 648,               /* K_OXIDE32  */
    K_PARALLELRUNLENGTH = 649,     /* K_PARALLELRUNLENGTH  */
    K_MINWIDTH = 650,              /* K_MINWIDTH  */
    K_PROTRUSIONWIDTH = 651,       /* K_PROTRUSIONWIDTH  */
    K_SPACINGTABLE = 652,          /* K_SPACINGTABLE  */
    K_WITHIN = 653,                /* K_WITHIN  */
    K_ABOVE = 654,                 /* K_ABOVE  */
    K_BELOW = 655,                 /* K_BELOW  */
    K_CENTERTOCENTER = 656,        /* K_CENTERTOCENTER  */
    K_CUTSIZE = 657,               /* K_CUTSIZE  */
    K_CUTSPACING = 658,            /* K_CUTSPACING  */
    K_DENSITY = 659,               /* K_DENSITY  */
    K_DIAG45 = 660,                /* K_DIAG45  */
    K_DIAG135 = 661,               /* K_DIAG135  */
    K_MASK = 662,                  /* K_MASK  */
    K_DIAGMINEDGELENGTH = 663,     /* K_DIAGMINEDGELENGTH  */
    K_DIAGSPACING = 664,           /* K_DIAGSPACING  */
    K_DIAGPITCH = 665,             /* K_DIAGPITCH  */
    K_DIAGWIDTH = 666,             /* K_DIAGWIDTH  */
    K_GENERATED = 667,             /* K_GENERATED  */
    K_GROUNDSENSITIVITY = 668,     /* K_GROUNDSENSITIVITY  */
    K_HARDSPACING = 669,           /* K_HARDSPACING  */
    K_INSIDECORNER = 670,          /* K_INSIDECORNER  */
    K_LAYERS = 671,                /* K_LAYERS  */
    K_LENGTHSUM = 672,             /* K_LENGTHSUM  */
    K_MICRONS = 673,               /* K_MICRONS  */
    K_MINCUTS = 674,               /* K_MINCUTS  */
    K_MINSIZE = 675,               /* K_MINSIZE  */
    K_NETEXPR = 676,               /* K_NETEXPR  */
    K_OUTSIDECORNER = 677,         /* K_OUTSIDECORNER  */
    K_PREFERENCLOSURE = 678,       /* K_PREFERENCLOSURE  */
    K_ROWCOL = 679,                /* K_ROWCOL  */
    K_ROWPATTERN = 680,            /* K_ROWPATTERN  */
    K_SOFT = 681,                  /* K_SOFT  */
    K_SUPPLYSENSITIVITY = 682,     /* K_SUPPLYSENSITIVITY  */
    K_USEVIA = 683,                /* K_USEVIA  */
    K_USEVIARULE = 684,            /* K_USEVIARULE  */
    K_WELLTAP = 685,               /* K_WELLTAP  */
    K_ARRAYCUTS = 686,             /* K_ARRAYCUTS  */
    K_ARRAYSPACING = 687,          /* K_ARRAYSPACING  */
    K_ANTENNAAREADIFFREDUCEPWL = 688, /* K_ANTENNAAREADIFFREDUCEPWL  */
    K_ANTENNAAREAMINUSDIFF = 689,  /* K_ANTENNAAREAMINUSDIFF  */
    K_ANTENNACUMROUTINGPLUSCUT = 690, /* K_ANTENNACUMROUTINGPLUSCUT  */
    K_ANTENNAGATEPLUSDIFF = 691,   /* K_ANTENNAGATEPLUSDIFF  */
    K_ENDOFLINE = 692,             /* K_ENDOFLINE  */
    K_ENDOFNOTCHWIDTH = 693,       /* K_ENDOFNOTCHWIDTH  */
    K_EXCEPTEXTRACUT = 694,        /* K_EXCEPTEXTRACUT  */
    K_EXCEPTSAMEPGNET = 695,       /* K_EXCEPTSAMEPGNET  */
    K_EXCEPTPGNET = 696,           /* K_EXCEPTPGNET  */
    K_LONGARRAY = 697,             /* K_LONGARRAY  */
    K_MAXEDGES = 698,              /* K_MAXEDGES  */
    K_NOTCHLENGTH = 699,           /* K_NOTCHLENGTH  */
    K_NOTCHSPACING = 700,          /* K_NOTCHSPACING  */
    K_ORTHOGONAL = 701,            /* K_ORTHOGONAL  */
    K_PARALLELEDGE = 702,          /* K_PARALLELEDGE  */
    K_PARALLELOVERLAP = 703,       /* K_PARALLELOVERLAP  */
    K_PGONLY = 704,                /* K_PGONLY  */
    K_PRL = 705,                   /* K_PRL  */
    K_TWOEDGES = 706,              /* K_TWOEDGES  */
    K_TWOWIDTHS = 707,             /* K_TWOWIDTHS  */
    IF = 708,                      /* IF  */
    LNOT = 709,                    /* LNOT  */
    UMINUS = 710                   /* UMINUS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/odb/src/lef/lef/lef.y"

        double    dval ;
        int       integer ;
        char *    string ;
        LefParser::lefPOINT  pt;

#line 526 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/odb/src/lef/lef_parser.hpp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE lefyylval;


int lefyyparse (void);


#endif /* !YY_LEFYY_HOME_MEMZFS_PROJECTS_MLBUF_EXTENSION_OR_LATEST_BUILD_SRC_ODB_SRC_LEF_LEF_PARSER_HPP_INCLUDED  */
