/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
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
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

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
#line 58 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"

#include <stdlib.h>
#include <string.h>
#include "defrReader.hpp"
#include "defiUser.hpp"
#include "defrCallBacks.hpp"
#include "lex.h"

#define DEF_MAX_INT 2147483647
#define YYDEBUG 1     // this is temp fix for pcr 755132 
// TX_DIR:TRANSLATION ON


#include "defrData.hpp"
#include "defrSettings.hpp"
#include "defrCallBacks.hpp"

BEGIN_LEFDEF_PARSER_NAMESPACE

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

#line 146 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"

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

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_DEFYY_HOME_STEPHANO_OPENROAD_SRC_OPENDB_SRC_DEF_DEF_PARSER_HPP_INCLUDED
# define YY_DEFYY_HOME_STEPHANO_OPENROAD_SRC_OPENDB_SRC_DEF_DEF_PARSER_HPP_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int defyydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    QSTRING = 258,
    T_STRING = 259,
    SITE_PATTERN = 260,
    NUMBER = 261,
    K_HISTORY = 262,
    K_NAMESCASESENSITIVE = 263,
    K_DESIGN = 264,
    K_VIAS = 265,
    K_TECH = 266,
    K_UNITS = 267,
    K_ARRAY = 268,
    K_FLOORPLAN = 269,
    K_SITE = 270,
    K_CANPLACE = 271,
    K_CANNOTOCCUPY = 272,
    K_DIEAREA = 273,
    K_PINS = 274,
    K_DEFAULTCAP = 275,
    K_MINPINS = 276,
    K_WIRECAP = 277,
    K_TRACKS = 278,
    K_GCELLGRID = 279,
    K_DO = 280,
    K_BY = 281,
    K_STEP = 282,
    K_LAYER = 283,
    K_ROW = 284,
    K_RECT = 285,
    K_COMPS = 286,
    K_COMP_GEN = 287,
    K_SOURCE = 288,
    K_WEIGHT = 289,
    K_EEQMASTER = 290,
    K_FIXED = 291,
    K_COVER = 292,
    K_UNPLACED = 293,
    K_PLACED = 294,
    K_FOREIGN = 295,
    K_REGION = 296,
    K_REGIONS = 297,
    K_NETS = 298,
    K_START_NET = 299,
    K_MUSTJOIN = 300,
    K_ORIGINAL = 301,
    K_USE = 302,
    K_STYLE = 303,
    K_PATTERN = 304,
    K_PATTERNNAME = 305,
    K_ESTCAP = 306,
    K_ROUTED = 307,
    K_NEW = 308,
    K_SNETS = 309,
    K_SHAPE = 310,
    K_WIDTH = 311,
    K_VOLTAGE = 312,
    K_SPACING = 313,
    K_NONDEFAULTRULE = 314,
    K_NONDEFAULTRULES = 315,
    K_N = 316,
    K_S = 317,
    K_E = 318,
    K_W = 319,
    K_FN = 320,
    K_FE = 321,
    K_FS = 322,
    K_FW = 323,
    K_GROUPS = 324,
    K_GROUP = 325,
    K_SOFT = 326,
    K_MAXX = 327,
    K_MAXY = 328,
    K_MAXHALFPERIMETER = 329,
    K_CONSTRAINTS = 330,
    K_NET = 331,
    K_PATH = 332,
    K_SUM = 333,
    K_DIFF = 334,
    K_SCANCHAINS = 335,
    K_START = 336,
    K_FLOATING = 337,
    K_ORDERED = 338,
    K_STOP = 339,
    K_IN = 340,
    K_OUT = 341,
    K_RISEMIN = 342,
    K_RISEMAX = 343,
    K_FALLMIN = 344,
    K_FALLMAX = 345,
    K_WIREDLOGIC = 346,
    K_MAXDIST = 347,
    K_ASSERTIONS = 348,
    K_DISTANCE = 349,
    K_MICRONS = 350,
    K_END = 351,
    K_IOTIMINGS = 352,
    K_RISE = 353,
    K_FALL = 354,
    K_VARIABLE = 355,
    K_SLEWRATE = 356,
    K_CAPACITANCE = 357,
    K_DRIVECELL = 358,
    K_FROMPIN = 359,
    K_TOPIN = 360,
    K_PARALLEL = 361,
    K_TIMINGDISABLES = 362,
    K_THRUPIN = 363,
    K_MACRO = 364,
    K_PARTITIONS = 365,
    K_TURNOFF = 366,
    K_FROMCLOCKPIN = 367,
    K_FROMCOMPPIN = 368,
    K_FROMIOPIN = 369,
    K_TOCLOCKPIN = 370,
    K_TOCOMPPIN = 371,
    K_TOIOPIN = 372,
    K_SETUPRISE = 373,
    K_SETUPFALL = 374,
    K_HOLDRISE = 375,
    K_HOLDFALL = 376,
    K_VPIN = 377,
    K_SUBNET = 378,
    K_XTALK = 379,
    K_PIN = 380,
    K_SYNTHESIZED = 381,
    K_DEFINE = 382,
    K_DEFINES = 383,
    K_DEFINEB = 384,
    K_IF = 385,
    K_THEN = 386,
    K_ELSE = 387,
    K_FALSE = 388,
    K_TRUE = 389,
    K_EQ = 390,
    K_NE = 391,
    K_LE = 392,
    K_LT = 393,
    K_GE = 394,
    K_GT = 395,
    K_OR = 396,
    K_AND = 397,
    K_NOT = 398,
    K_SPECIAL = 399,
    K_DIRECTION = 400,
    K_RANGE = 401,
    K_FPC = 402,
    K_HORIZONTAL = 403,
    K_VERTICAL = 404,
    K_ALIGN = 405,
    K_MIN = 406,
    K_MAX = 407,
    K_EQUAL = 408,
    K_BOTTOMLEFT = 409,
    K_TOPRIGHT = 410,
    K_ROWS = 411,
    K_TAPER = 412,
    K_TAPERRULE = 413,
    K_VERSION = 414,
    K_DIVIDERCHAR = 415,
    K_BUSBITCHARS = 416,
    K_PROPERTYDEFINITIONS = 417,
    K_STRING = 418,
    K_REAL = 419,
    K_INTEGER = 420,
    K_PROPERTY = 421,
    K_BEGINEXT = 422,
    K_ENDEXT = 423,
    K_NAMEMAPSTRING = 424,
    K_ON = 425,
    K_OFF = 426,
    K_X = 427,
    K_Y = 428,
    K_COMPONENT = 429,
    K_MASK = 430,
    K_MASKSHIFT = 431,
    K_COMPSMASKSHIFT = 432,
    K_SAMEMASK = 433,
    K_PINPROPERTIES = 434,
    K_TEST = 435,
    K_COMMONSCANPINS = 436,
    K_SNET = 437,
    K_COMPONENTPIN = 438,
    K_REENTRANTPATHS = 439,
    K_SHIELD = 440,
    K_SHIELDNET = 441,
    K_NOSHIELD = 442,
    K_VIRTUAL = 443,
    K_ANTENNAPINPARTIALMETALAREA = 444,
    K_ANTENNAPINPARTIALMETALSIDEAREA = 445,
    K_ANTENNAPINGATEAREA = 446,
    K_ANTENNAPINDIFFAREA = 447,
    K_ANTENNAPINMAXAREACAR = 448,
    K_ANTENNAPINMAXSIDEAREACAR = 449,
    K_ANTENNAPINPARTIALCUTAREA = 450,
    K_ANTENNAPINMAXCUTCAR = 451,
    K_SIGNAL = 452,
    K_POWER = 453,
    K_GROUND = 454,
    K_CLOCK = 455,
    K_TIEOFF = 456,
    K_ANALOG = 457,
    K_SCAN = 458,
    K_RESET = 459,
    K_RING = 460,
    K_STRIPE = 461,
    K_FOLLOWPIN = 462,
    K_IOWIRE = 463,
    K_COREWIRE = 464,
    K_BLOCKWIRE = 465,
    K_FILLWIRE = 466,
    K_BLOCKAGEWIRE = 467,
    K_PADRING = 468,
    K_BLOCKRING = 469,
    K_BLOCKAGES = 470,
    K_PLACEMENT = 471,
    K_SLOTS = 472,
    K_FILLS = 473,
    K_PUSHDOWN = 474,
    K_NETLIST = 475,
    K_DIST = 476,
    K_USER = 477,
    K_TIMING = 478,
    K_BALANCED = 479,
    K_STEINER = 480,
    K_TRUNK = 481,
    K_FIXEDBUMP = 482,
    K_FENCE = 483,
    K_FREQUENCY = 484,
    K_GUIDE = 485,
    K_MAXBITS = 486,
    K_PARTITION = 487,
    K_TYPE = 488,
    K_ANTENNAMODEL = 489,
    K_DRCFILL = 490,
    K_OXIDE1 = 491,
    K_OXIDE2 = 492,
    K_OXIDE3 = 493,
    K_OXIDE4 = 494,
    K_CUTSIZE = 495,
    K_CUTSPACING = 496,
    K_DESIGNRULEWIDTH = 497,
    K_DIAGWIDTH = 498,
    K_ENCLOSURE = 499,
    K_HALO = 500,
    K_GROUNDSENSITIVITY = 501,
    K_HARDSPACING = 502,
    K_LAYERS = 503,
    K_MINCUTS = 504,
    K_NETEXPR = 505,
    K_OFFSET = 506,
    K_ORIGIN = 507,
    K_ROWCOL = 508,
    K_STYLES = 509,
    K_POLYGON = 510,
    K_PORT = 511,
    K_SUPPLYSENSITIVITY = 512,
    K_VIA = 513,
    K_VIARULE = 514,
    K_WIREEXT = 515,
    K_EXCEPTPGNET = 516,
    K_FILLWIREOPC = 517,
    K_OPC = 518,
    K_PARTIAL = 519,
    K_ROUTEHALO = 520
  };
#endif

/* Value type.  */



int defyyparse (defrData *defData);

#endif /* !YY_DEFYY_HOME_STEPHANO_OPENROAD_SRC_OPENDB_SRC_DEF_DEF_PARSER_HPP_INCLUDED  */



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
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
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

#if ! defined yyoverflow || YYERROR_VERBOSE

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
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


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
#define YYLAST   1486

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  273
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  522
/* YYNRULES -- Number of rules.  */
#define YYNRULES  957
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  1668

#define YYUNDEFTOK  2
#define YYMAXUTOK   520


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int16 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     269,   270,   271,   268,   272,   267,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,   266,
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
     265
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   216,   216,   219,   220,   220,   246,   247,   260,   278,
     279,   280,   283,   283,   283,   283,   284,   284,   284,   284,
     285,   285,   285,   286,   286,   286,   287,   287,   287,   288,
     288,   288,   288,   289,   292,   292,   292,   292,   293,   293,
     293,   294,   294,   294,   295,   295,   295,   296,   296,   296,
     296,   301,   301,   308,   333,   333,   339,   339,   345,   345,
     351,   358,   357,   369,   370,   373,   373,   382,   382,   391,
     391,   400,   400,   409,   409,   418,   418,   427,   427,   438,
     437,   448,   447,   468,   470,   470,   475,   475,   481,   486,
     491,   497,   498,   501,   509,   516,   523,   523,   535,   535,
     548,   549,   550,   551,   552,   553,   554,   555,   558,   557,
     570,   573,   585,   586,   589,   600,   603,   606,   612,   613,
     616,   617,   618,   616,   630,   631,   633,   639,   645,   651,
     672,   672,   692,   692,   712,   716,   737,   738,   737,   756,
     757,   756,   792,   792,   818,   830,   848,   866,   884,   902,
     902,   920,   920,   939,   957,   957,   975,   992,   993,  1007,
    1008,  1015,  1016,  1029,  1030,  1053,  1077,  1078,  1101,  1125,
    1130,  1135,  1140,  1146,  1148,  1150,  1152,  1154,  1156,  1158,
    1160,  1165,  1166,  1166,  1169,  1175,  1177,  1175,  1191,  1201,
    1239,  1242,  1250,  1251,  1254,  1254,  1258,  1259,  1262,  1274,
    1283,  1294,  1293,  1327,  1332,  1334,  1337,  1339,  1340,  1351,
    1352,  1355,  1356,  1356,  1359,  1360,  1363,  1369,  1398,  1404,
    1407,  1410,  1416,  1417,  1420,  1421,  1420,  1432,  1433,  1436,
    1436,  1443,  1444,  1443,  1470,  1470,  1480,  1482,  1480,  1505,
    1506,  1512,  1524,  1536,  1548,  1548,  1561,  1564,  1567,  1568,
    1571,  1578,  1584,  1590,  1597,  1598,  1601,  1607,  1613,  1619,
    1620,  1623,  1624,  1623,  1633,  1636,  1641,  1642,  1645,  1645,
    1648,  1663,  1664,  1667,  1682,  1691,  1701,  1703,  1706,  1723,
    1726,  1733,  1734,  1737,  1744,  1745,  1748,  1754,  1760,  1760,
    1768,  1769,  1774,  1780,  1781,  1784,  1784,  1784,  1784,  1784,
    1785,  1785,  1785,  1785,  1786,  1786,  1786,  1789,  1796,  1796,
    1802,  1802,  1810,  1811,  1814,  1820,  1822,  1824,  1826,  1831,
    1833,  1839,  1850,  1863,  1862,  1884,  1885,  1905,  1905,  1925,
    1925,  1929,  1930,  1933,  1944,  1953,  1963,  1966,  1966,  1981,
    1983,  1986,  1993,  2000,  2015,  2015,  2024,  2026,  2028,  2031,
    2037,  2043,  2046,  2053,  2054,  2057,  2068,  2072,  2072,  2075,
    2074,  2083,  2083,  2091,  2092,  2095,  2095,  2109,  2109,  2116,
    2116,  2125,  2126,  2133,  2138,  2139,  2143,  2142,  2161,  2164,
    2181,  2181,  2199,  2199,  2202,  2205,  2208,  2211,  2214,  2217,
    2220,  2220,  2231,  2233,  2233,  2236,  2237,  2236,  2283,  2288,
    2298,  2282,  2312,  2312,  2316,  2322,  2323,  2326,  2337,  2346,
    2356,  2358,  2360,  2362,  2364,  2368,  2367,  2378,  2378,  2381,
    2382,  2382,  2385,  2386,  2389,  2391,  2393,  2396,  2398,  2400,
    2404,  2408,  2411,  2411,  2417,  2436,  2416,  2450,  2467,  2484,
    2485,  2490,  2501,  2515,  2522,  2533,  2557,  2591,  2625,  2650,
    2651,  2652,  2652,  2663,  2662,  2672,  2683,  2691,  2698,  2705,
    2711,  2719,  2727,  2735,  2744,  2752,  2759,  2766,  2774,  2783,
    2784,  2786,  2787,  2790,  2794,  2794,  2799,  2819,  2820,  2824,
    2828,  2848,  2854,  2856,  2858,  2860,  2862,  2864,  2866,  2868,
    2884,  2886,  2888,  2890,  2893,  2896,  2897,  2900,  2903,  2904,
    2907,  2907,  2908,  2908,  2911,  2925,  2924,  2946,  2947,  2946,
    2953,  2957,  2963,  2964,  2963,  3004,  3004,  3037,  3038,  3037,
    3070,  3073,  3076,  3079,  3079,  3082,  3085,  3088,  3100,  3103,
    3106,  3106,  3110,  3114,  3115,  3116,  3117,  3118,  3119,  3120,
    3121,  3122,  3125,  3139,  3139,  3194,  3194,  3206,  3206,  3224,
    3225,  3224,  3232,  3233,  3236,  3248,  3257,  3267,  3268,  3273,
    3274,  3277,  3279,  3281,  3283,  3287,  3304,  3307,  3307,  3326,
    3334,  3325,  3342,  3347,  3354,  3361,  3364,  3370,  3371,  3374,
    3380,  3380,  3391,  3392,  3395,  3402,  3403,  3406,  3408,  3408,
    3411,  3411,  3413,  3419,  3429,  3434,  3435,  3438,  3449,  3458,
    3468,  3469,  3472,  3483,  3494,  3506,  3513,  3517,  3520,  3534,
    3548,  3549,  3552,  3553,  3563,  3576,  3576,  3581,  3581,  3586,
    3591,  3597,  3598,  3600,  3602,  3602,  3611,  3612,  3615,  3616,
    3619,  3624,  3629,  3634,  3640,  3651,  3662,  3665,  3671,  3672,
    3675,  3681,  3681,  3688,  3689,  3694,  3695,  3698,  3698,  3702,
    3702,  3705,  3704,  3713,  3713,  3717,  3717,  3719,  3719,  3737,
    3744,  3745,  3754,  3768,  3769,  3773,  3772,  3783,  3784,  3797,
    3818,  3849,  3850,  3854,  3853,  3862,  3863,  3876,  3897,  3929,
    3930,  3933,  3942,  3945,  3956,  3957,  3960,  3966,  3966,  3972,
    3973,  3977,  3982,  3987,  3992,  3993,  3992,  4001,  4008,  4009,
    4007,  4015,  4016,  4016,  4022,  4023,  4029,  4029,  4031,  4037,
    4043,  4049,  4050,  4053,  4054,  4053,  4058,  4060,  4063,  4065,
    4067,  4069,  4072,  4073,  4077,  4076,  4080,  4079,  4084,  4085,
    4087,  4087,  4089,  4089,  4092,  4096,  4103,  4104,  4107,  4108,
    4107,  4116,  4116,  4124,  4124,  4132,  4138,  4139,  4138,  4144,
    4144,  4150,  4157,  4160,  4167,  4168,  4171,  4177,  4177,  4183,
    4184,  4191,  4192,  4194,  4198,  4199,  4201,  4204,  4205,  4208,
    4208,  4214,  4214,  4220,  4220,  4226,  4226,  4232,  4232,  4238,
    4238,  4243,  4251,  4250,  4254,  4255,  4258,  4263,  4269,  4270,
    4273,  4274,  4276,  4278,  4280,  4282,  4286,  4287,  4290,  4293,
    4296,  4299,  4303,  4307,  4308,  4311,  4311,  4320,  4321,  4324,
    4325,  4328,  4327,  4340,  4340,  4343,  4345,  4347,  4349,  4352,
    4354,  4360,  4361,  4364,  4368,  4369,  4372,  4373,  4372,  4382,
    4383,  4385,  4385,  4389,  4390,  4393,  4404,  4413,  4423,  4425,
    4429,  4433,  4434,  4437,  4446,  4447,  4446,  4466,  4465,  4482,
    4483,  4486,  4512,  4534,  4535,  4538,  4547,  4547,  4566,  4587,
    4608,  4626,  4658,  4659,  4664,  4664,  4682,  4700,  4734,  4770,
    4771,  4774,  4780,  4779,  4805,  4807,  4811,  4815,  4816,  4819,
    4827,  4828,  4827,  4835,  4836,  4839,  4845,  4844,  4857,  4859,
    4863,  4867,  4868,  4871,  4878,  4879,  4878,  4888,  4889,  4888,
    4897,  4898,  4901,  4907,  4906,  4925,  4926,  4928,  4929,  4934,
    4953,  4963,  4964,  4966,  4967,  4972,  4992,  5002,  5012,  5015,
    5033,  5037,  5038,  5041,  5042,  5041,  5052,  5053,  5056,  5061,
    5063,  5061,  5070,  5070,  5076,  5076,  5082,  5082,  5088,  5091,
    5092,  5095,  5101,  5107,  5115,  5115,  5119,  5120,  5123,  5134,
    5143,  5154,  5156,  5173,  5177,  5178,  5182,  5181
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "QSTRING", "T_STRING", "SITE_PATTERN",
  "NUMBER", "K_HISTORY", "K_NAMESCASESENSITIVE", "K_DESIGN", "K_VIAS",
  "K_TECH", "K_UNITS", "K_ARRAY", "K_FLOORPLAN", "K_SITE", "K_CANPLACE",
  "K_CANNOTOCCUPY", "K_DIEAREA", "K_PINS", "K_DEFAULTCAP", "K_MINPINS",
  "K_WIRECAP", "K_TRACKS", "K_GCELLGRID", "K_DO", "K_BY", "K_STEP",
  "K_LAYER", "K_ROW", "K_RECT", "K_COMPS", "K_COMP_GEN", "K_SOURCE",
  "K_WEIGHT", "K_EEQMASTER", "K_FIXED", "K_COVER", "K_UNPLACED",
  "K_PLACED", "K_FOREIGN", "K_REGION", "K_REGIONS", "K_NETS",
  "K_START_NET", "K_MUSTJOIN", "K_ORIGINAL", "K_USE", "K_STYLE",
  "K_PATTERN", "K_PATTERNNAME", "K_ESTCAP", "K_ROUTED", "K_NEW", "K_SNETS",
  "K_SHAPE", "K_WIDTH", "K_VOLTAGE", "K_SPACING", "K_NONDEFAULTRULE",
  "K_NONDEFAULTRULES", "K_N", "K_S", "K_E", "K_W", "K_FN", "K_FE", "K_FS",
  "K_FW", "K_GROUPS", "K_GROUP", "K_SOFT", "K_MAXX", "K_MAXY",
  "K_MAXHALFPERIMETER", "K_CONSTRAINTS", "K_NET", "K_PATH", "K_SUM",
  "K_DIFF", "K_SCANCHAINS", "K_START", "K_FLOATING", "K_ORDERED", "K_STOP",
  "K_IN", "K_OUT", "K_RISEMIN", "K_RISEMAX", "K_FALLMIN", "K_FALLMAX",
  "K_WIREDLOGIC", "K_MAXDIST", "K_ASSERTIONS", "K_DISTANCE", "K_MICRONS",
  "K_END", "K_IOTIMINGS", "K_RISE", "K_FALL", "K_VARIABLE", "K_SLEWRATE",
  "K_CAPACITANCE", "K_DRIVECELL", "K_FROMPIN", "K_TOPIN", "K_PARALLEL",
  "K_TIMINGDISABLES", "K_THRUPIN", "K_MACRO", "K_PARTITIONS", "K_TURNOFF",
  "K_FROMCLOCKPIN", "K_FROMCOMPPIN", "K_FROMIOPIN", "K_TOCLOCKPIN",
  "K_TOCOMPPIN", "K_TOIOPIN", "K_SETUPRISE", "K_SETUPFALL", "K_HOLDRISE",
  "K_HOLDFALL", "K_VPIN", "K_SUBNET", "K_XTALK", "K_PIN", "K_SYNTHESIZED",
  "K_DEFINE", "K_DEFINES", "K_DEFINEB", "K_IF", "K_THEN", "K_ELSE",
  "K_FALSE", "K_TRUE", "K_EQ", "K_NE", "K_LE", "K_LT", "K_GE", "K_GT",
  "K_OR", "K_AND", "K_NOT", "K_SPECIAL", "K_DIRECTION", "K_RANGE", "K_FPC",
  "K_HORIZONTAL", "K_VERTICAL", "K_ALIGN", "K_MIN", "K_MAX", "K_EQUAL",
  "K_BOTTOMLEFT", "K_TOPRIGHT", "K_ROWS", "K_TAPER", "K_TAPERRULE",
  "K_VERSION", "K_DIVIDERCHAR", "K_BUSBITCHARS", "K_PROPERTYDEFINITIONS",
  "K_STRING", "K_REAL", "K_INTEGER", "K_PROPERTY", "K_BEGINEXT",
  "K_ENDEXT", "K_NAMEMAPSTRING", "K_ON", "K_OFF", "K_X", "K_Y",
  "K_COMPONENT", "K_MASK", "K_MASKSHIFT", "K_COMPSMASKSHIFT", "K_SAMEMASK",
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
  "K_OXIDE2", "K_OXIDE3", "K_OXIDE4", "K_CUTSIZE", "K_CUTSPACING",
  "K_DESIGNRULEWIDTH", "K_DIAGWIDTH", "K_ENCLOSURE", "K_HALO",
  "K_GROUNDSENSITIVITY", "K_HARDSPACING", "K_LAYERS", "K_MINCUTS",
  "K_NETEXPR", "K_OFFSET", "K_ORIGIN", "K_ROWCOL", "K_STYLES", "K_POLYGON",
  "K_PORT", "K_SUPPLYSENSITIVITY", "K_VIA", "K_VIARULE", "K_WIREEXT",
  "K_EXCEPTPGNET", "K_FILLWIREOPC", "K_OPC", "K_PARTIAL", "K_ROUTEHALO",
  "';'", "'-'", "'+'", "'('", "')'", "'*'", "','", "$accept", "def_file",
  "version_stmt", "$@1", "case_sens_stmt", "rules", "rule",
  "design_section", "design_name", "$@2", "end_design", "tech_name", "$@3",
  "array_name", "$@4", "floorplan_name", "$@5", "history",
  "prop_def_section", "$@6", "property_defs", "property_def", "$@7", "$@8",
  "$@9", "$@10", "$@11", "$@12", "$@13", "$@14", "$@15",
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
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
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
     455,   456,   457,   458,   459,   460,   461,   462,   463,   464,
     465,   466,   467,   468,   469,   470,   471,   472,   473,   474,
     475,   476,   477,   478,   479,   480,   481,   482,   483,   484,
     485,   486,   487,   488,   489,   490,   491,   492,   493,   494,
     495,   496,   497,   498,   499,   500,   501,   502,   503,   504,
     505,   506,   507,   508,   509,   510,   511,   512,   513,   514,
     515,   516,   517,   518,   519,   520,    59,    45,    43,    40,
      41,    42,    44
};
# endif

#define YYPACT_NINF (-1119)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-544)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -94, -1119,   102,   119,   207, -1119,    77,   489,  -104,   -49,
     -37, -1119,   666, -1119, -1119, -1119, -1119, -1119,   227, -1119,
     161, -1119, -1119, -1119, -1119, -1119,   256,   310,   148,   148,
   -1119,   327,   337,   359,   379,   401,   451,   461,   498,   510,
     366,   529,   530,   545,   547,   551,   552, -1119, -1119, -1119,
     550,   554,   555,   559,   560, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119,   564, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119,   290, -1119, -1119,   568,   307,
     570,   480,   572,   573,   574,   576,   312,   317, -1119, -1119,
   -1119, -1119,   578,   583,   322,   324,   325,   326,   328,   329,
     332,   344,   345, -1119,   346,   347,   348,   349,   350,   360,
   -1119,    26,   361,   362,   363,   365,   367,    74,   -29, -1119,
     -13,    -9,    -8,    -2,     4,    41,    43,    47,    50,    51,
      55,    58,    59,    60,    61,    62,    63, -1119, -1119,    64,
     368, -1119,   371,   587,   372,   374,   635,   636,    11,   312,
   -1119, -1119,   618,   642, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   385,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   641,
     628, -1119, -1119,   633, -1119, -1119, -1119,   629,   643, -1119,
   -1119, -1119,   613, -1119, -1119,   638, -1119, -1119, -1119, -1119,
   -1119,   624, -1119, -1119, -1119, -1119, -1119,   616, -1119, -1119,
   -1119,   602, -1119, -1119, -1119, -1119,   579,    57, -1119, -1119,
   -1119, -1119,   599, -1119,   601, -1119, -1119, -1119, -1119,   590,
     419, -1119, -1119, -1119,   544, -1119, -1119,   585,   -18, -1119,
   -1119,   584, -1119, -1119, -1119, -1119,   514, -1119, -1119, -1119,
     485,    35, -1119, -1119,    34,   486,   677, -1119, -1119, -1119,
     492,    44, -1119, -1119, -1119,   711,    65,   464,   671, -1119,
   -1119, -1119, -1119,   455, -1119, -1119,   716,   717,    12,    13,
   -1119, -1119,   719,   721,   462, -1119, -1119, -1119, -1119, -1119,
   -1119,   567, -1119, -1119, -1119, -1119,   708, -1119, -1119,   727,
     726, -1119,   730, -1119,   732, -1119,   733,  -186,     9, -1119,
      97,   -95, -1119,   -76, -1119,   735,   736, -1119, -1119, -1119,
     473,   475, -1119, -1119, -1119, -1119,   741,   -58, -1119, -1119,
     -44, -1119,   743, -1119, -1119, -1119, -1119,   482, -1119,   745,
     -23, -1119,   746, -1119, -1119, -1119,   312, -1119, -1119, -1119,
   -1119,   -14, -1119, -1119, -1119,     2, -1119,   691, -1119, -1119,
   -1119,   747, -1119,   274,   274,   484,   487,   488,   490,  -141,
     725,   749, -1119,   752,   760,   763,   764,   766,   767, -1119,
     768,   770,   771,   772,   509,   753, -1119, -1119,   777, -1119,
      21, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,    16, -1119, -1119, -1119,   312, -1119, -1119, -1119, -1119,
     513, -1119, -1119,   665, -1119, -1119, -1119,   755, -1119,   179,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   116,
     780,   781,   198,   198,   782,   229, -1119, -1119,    24, -1119,
   -1119,   786, -1119,    79, -1119, -1119,   223,   787,   788,   789,
   -1119,   683, -1119,     1, -1119, -1119,   791,   792, -1119,   312,
     312,    15,   793,   312, -1119, -1119, -1119,   794,   795,   312,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119,   775,   776, -1119, -1119, -1119, -1119,
   -1119, -1119,   796,   274,   184,   184,   184,   184,   184,   184,
     184,   184,   184,   537,   728,   799, -1119,   312, -1119, -1119,
     158,   800, -1119, -1119, -1119,   312, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119,   801, -1119,   312,   312,   274, -1119,   804,
    -101,   803, -1119, -1119, -1119,   285,   805,   -50,   806, -1119,
   -1119, -1119, -1119,   808, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,   312, -1119,   158,   809, -1119,   285,   810,   -50,   811,
     395, -1119, -1119, -1119, -1119,   812, -1119, -1119,   813, -1119,
   -1119,   816, -1119,    52, -1119, -1119, -1119,   817, -1119,   -28,
      -3,   542, -1119,   300, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,   818, -1119, -1119,   819, -1119,    83, -1119, -1119, -1119,
     820,   826,    85,   281, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119,   566, -1119,   312, -1119, -1119, -1119,   312,
     312, -1119, -1119,   312,   312,   253,   312,   829,   830,   571,
   -1119,   836, -1119, -1119,   837,   580,   581,   582,   588,   589,
     591,   592,   593,   594, -1119, -1119,   648,   260,   312,   312,
     838, -1119, -1119, -1119, -1119, -1119, -1119,   840,   274,   845,
     846,   849,   769, -1119, -1119,   312, -1119,   595, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119,   852, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119,   857,   858,   859, -1119,   861,   862, -1119,
     863,   864,   866,   312,   867, -1119, -1119,   868, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119,   869,   870,   871,   872, -1119,
     873, -1119,   875,   876,   878, -1119, -1119, -1119,   881, -1119,
     198, -1119, -1119, -1119,   797,   880,   882,   884,   885,   889,
   -1119, -1119,   892,   630,   893,   631, -1119,   894,   896,   897,
     245,   802,   634, -1119, -1119,   639, -1119, -1119,   272,   900,
     902,   904,   906,   907,   908, -1119, -1119,   -25, -1119,   312,
      48, -1119,   312, -1119, -1119, -1119,   312, -1119,    14, -1119,
   -1119,   312,   887,   888, -1119,   890, -1119,   773,   773, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   912,
     911,   652,   895, -1119,    54, -1119, -1119, -1119, -1119,   312,
     261,   917, -1119, -1119,    32,    93,   846, -1119, -1119, -1119,
     916,   920, -1119,     7, -1119,   921, -1119, -1119, -1119, -1119,
     106,   861, -1119, -1119,   864, -1119, -1119,   874, -1119, -1119,
   -1119,   312, -1119,   922, -1119,   923,   412,   872, -1119, -1119,
   -1119,   557, -1119,   877, -1119,    18,   369, -1119,   927, -1119,
     926, -1119, -1119, -1119, -1119,   929,   930,   931,   929,   932,
   -1119,   695, -1119, -1119,   933,   934, -1119,   935,   936,   937,
   -1119, -1119, -1119,   940,   941, -1119, -1119, -1119, -1119,   942,
     943, -1119,   944,   945, -1119,   273,   669, -1119, -1119, -1119,
     946, -1119, -1119,   312,   -15,   143,   312, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119,   947,   949,   950, -1119,   951,   952,
     952, -1119,   783, -1119, -1119, -1119, -1119, -1119, -1119,   953,
     954,   956, -1119, -1119, -1119, -1119,  -122, -1119, -1119, -1119,
     957, -1119,   274, -1119, -1119, -1119, -1119,   958,   961, -1119,
   -1119, -1119,   668, -1119, -1119, -1119, -1119, -1119,   874, -1119,
   -1119, -1119,   399,   312, -1119, -1119, -1119, -1119, -1119, -1119,
     962,   312, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,   963, -1119, -1119, -1119,   312, -1119,   964,   965,   966,
   -1119,   969, -1119,   684, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,   970,   971, -1119,   847, -1119, -1119, -1119, -1119, -1119,
     276,   972,   879, -1119,   321,   315,   390,   321,   315,   390,
   -1119,   814, -1119,   149, -1119, -1119, -1119,   974, -1119,   312,
   -1119,  -123, -1119, -1119, -1119, -1119,   -98, -1119,   702, -1119,
   -1119, -1119, -1119,   975, -1119,   977,   978,   979,  -125,   948,
     959,   967,   279,   981, -1119, -1119, -1119, -1119, -1119, -1119,
     984,   985,   986,   987,   988,   989,   990,   993,   994, -1119,
     318, -1119, -1119,   995, -1119,   996,   998,   999, -1119,   723,
      -5,   864, -1119, -1119, -1119, -1119,   312, -1119,   839, -1119,
     878,   312,   312, -1119, -1119,   878, -1119, -1119, -1119, -1119,
     450, -1119, -1119,   737,   738,   729, -1119, -1119, -1119,   899,
   -1119,   277, -1119,  1001, -1119, -1119,  1002,  1003,  1005,  1006,
     315, -1119,  1007,  1008,  1009,  1011, -1119, -1119,   315, -1119,
    1012, -1119,  1013, -1119, -1119, -1119,  1014, -1119, -1119,   312,
    1015, -1119,  1016, -1119,   312, -1119,   312,   968,  1019,  1020,
   -1119, -1119, -1119,  1021,  1022,  1023, -1119,   865, -1119, -1119,
     284, -1119,   984,   757, -1119, -1119,  1024, -1119, -1119,   757,
     765, -1119, -1119, -1119, -1119,  1026,   774,   774,   774, -1119,
   -1119, -1119,  1028, -1119, -1119,    17, -1119, -1119, -1119, -1119,
   -1119,   274,  1029, -1119,   877,   312, -1119,   159, -1119, -1119,
   -1119, -1119,  1032, -1119,  1033, -1119,   778,  1034, -1119, -1119,
   -1119,   779,  1035, -1119,    49,  1037,  1038,  1040,  1042, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119,  1036, -1119, -1119,
   -1119, -1119,  1043,   456, -1119, -1119,  1044,  1045,  1025, -1119,
   -1119,   167, -1119, -1119,   312, -1119,   984,   886,   312,  1047,
   -1119,   822,  1050, -1119,     5, -1119,   790,   798,   807,  1052,
      33, -1119,  1053,    19,    20, -1119, -1119,  1057, -1119,   312,
     131, -1119, -1119,  1055,  1060,  1061, -1119,  1062, -1119, -1119,
   -1119, -1119, -1119,  1063,  1064, -1119, -1119, -1119, -1119, -1119,
    1067, -1119, -1119, -1119, -1119, -1119,   815,   821,  1066, -1119,
   -1119, -1119,   285, -1119,  1069,  1068,  1070,  1072,  1073,  1074,
    1076,  1077,  1078,   255, -1119,  1082, -1119, -1119, -1119, -1119,
     274, -1119,  1080,   312, -1119,   312,  1083,   460, -1119, -1119,
   -1119, -1119, -1119,  1071, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119,    27,    28,    29,    30,    10, -1119,   312,  1084,
     395, -1119,   823,   824,  1087, -1119,   828,   828, -1119,  1086,
    1089,   511, -1119, -1119, -1119, -1119,  1092,  1095,  1096, -1119,
   -1119,  1039,  1039,  1039,  1039,  1075,  1079,  1039,  1081, -1119,
   -1119, -1119, -1119, -1119,  1097, -1119,  1098,  1100,  1101, -1119,
   -1119, -1119,   312,  1102, -1119, -1119, -1119,   842,  1107,   864,
     843, -1119,   844, -1119,   848, -1119,   850, -1119,   384,   853,
    1106,   854, -1119, -1119, -1119, -1119, -1119, -1119,    10,   855,
     856,   851,   982,    40, -1119,  1111, -1119, -1119, -1119, -1119,
   -1119,   -11, -1119,   519, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   955,   312,
     860,   915,   883, -1119,   874, -1119, -1119, -1119, -1119,  1110,
    1094,  1120, -1119,    94,    22, -1119,  1113,  1123, -1119,  1125,
   -1119, -1119, -1119, -1119,  1126,  1127,  1128, -1119, -1119, -1119,
   -1119,   960,  1132,  1133,  1134,  1135,   973,  1136,   891, -1119,
     898, -1119, -1119,  1103,  1138,  1139,   408, -1119,   901,    23,
      25,  1137,  1143, -1119,  1145,  1146, -1119, -1119, -1119,  1148,
     -19, -1119, -1119, -1119, -1119,  1149,     8, -1119,  1150,   312,
   -1119,  1151,  1114,  1152,  1153,  1140,   905, -1119,   903,   909,
     910,   913,   914,   918,   919,   924, -1119,  1155,  1156,   312,
   -1119,  1157,  1158,   312,  1160,  1147,  1142,  1161,  1165,  1159,
    1169,  1170, -1119, -1119, -1119, -1119,   928,   938, -1119, -1119,
   -1119, -1119,   312, -1119, -1119,   312,   925,  1173,  1172,  1154,
     939,  1176,  1164,  1180,  1183,  1187, -1119,   312, -1119,  1188,
    1190,  1192, -1119,  1166,  1193,  1194,  1197,  1198, -1119,   976,
   -1119,  1199,  1200,  1177,  1202,   980,   983,   312,   991, -1119,
    1204,  1205,   992, -1119, -1119,  1206, -1119,  1207, -1119,  1208,
   -1119,   997,  1004,  1209,  1210,  1211,  1212, -1119
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       3,     4,     0,     6,     0,     1,     0,     0,     0,     0,
       0,    11,     0,     5,     7,     8,    60,    51,     0,    54,
       0,    56,    58,    96,    98,   108,     0,     0,     0,     0,
     185,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    61,   218,   281,
       0,     0,     0,     0,     0,    10,    12,    38,     2,    48,
      34,    41,    43,    46,    50,    40,    35,    36,    37,    39,
      44,   112,    45,   118,    47,    49,     0,    42,    17,    33,
     222,    27,   259,    19,    15,   284,    23,   353,    30,   495,
      21,   577,    13,    16,   610,   610,    28,   638,    22,   684,
      20,   711,    32,   736,    25,   754,    26,   824,    14,   841,
      29,   877,    18,   891,    24,     0,    31,   954,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   111,   204,
     205,   203,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    53,     0,     0,     0,     0,     0,     0,
      63,     0,   821,     0,     0,     0,     0,     0,     0,   201,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   923,   921,     0,
       0,   221,     0,     0,     0,     0,     0,     0,     0,     0,
     246,   117,     0,     0,   280,   258,   352,   573,   919,   576,
     609,   637,   608,   683,   735,   753,   710,    94,    95,     0,
     283,   278,   282,   822,   820,   839,   875,   889,   952,     0,
       0,   113,   110,     0,   120,   119,   116,     0,     0,   224,
     223,   220,     0,   261,   260,     0,   288,   285,   293,   290,
     279,     0,   357,   354,   374,   356,   351,     0,   498,   496,
     494,     0,   580,   578,   582,   575,     0,     0,   611,   612,
     613,   606,     0,   607,     0,   641,   639,   643,   636,     0,
       0,   685,   689,   682,     0,   713,   712,     0,     0,   737,
     734,     0,   757,   755,   767,   752,     0,   826,   819,   825,
       0,     0,   838,   842,     0,     0,     0,   874,   878,   883,
       0,     0,   888,   892,   900,     0,     0,     0,     0,   951,
     955,    52,    55,     0,    57,    59,     0,     0,     0,     0,
     248,   247,     0,     0,     0,    65,    77,    71,    81,    73,
      67,     0,    75,    69,    79,    64,     0,   115,   184,     0,
       0,   256,     0,   257,     0,   350,     0,     0,   287,   481,
       0,     0,   574,     0,   605,     0,   585,   635,   615,   617,
       0,     0,   624,   628,   634,   681,     0,     0,   708,   687,
       0,   709,     0,   751,   738,   741,   743,     0,   802,     0,
       0,   823,     0,   840,   844,   847,     0,   872,   869,   876,
     880,     0,   890,   897,   894,     0,   924,     0,   918,   922,
     953,     0,    93,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    83,     0,     0,     0,     0,     0,     0,    62,
       0,     0,     0,     0,     0,     0,   225,   262,     0,   286,
       0,   307,   294,   306,   302,   295,   296,   301,   303,   304,
     305,     0,   300,   297,   299,     0,   298,   292,   291,   359,
       0,   358,   355,     0,   404,   375,   392,   419,   497,     0,
     532,   499,   503,   500,   501,   502,   581,   584,   583,     0,
       0,     0,     0,     0,     0,     0,   642,   640,     0,   659,
     644,     0,   686,     0,   697,   690,     0,     0,     0,     0,
     745,   759,   756,     0,   781,   768,     0,     0,   862,     0,
       0,     0,     0,     0,   886,   879,   884,     0,     0,     0,
     903,   893,   901,   926,   920,   956,   100,   102,   103,   101,
     104,   107,   106,   105,     0,     0,   250,   252,   251,   253,
     109,   249,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   227,     0,   289,   310,
       0,     0,   308,   346,   347,   342,   348,   337,   336,   329,
     219,   344,   323,     0,   320,     0,   319,     0,   363,     0,
       0,     0,   427,   428,   382,     0,     0,     0,     0,   429,
     390,   417,   398,     0,   402,   393,   395,   379,   380,   376,
     420,     0,   515,     0,     0,   523,     0,     0,     0,     0,
       0,   545,   547,   549,   530,     0,   507,   521,     0,   512,
     517,   504,   579,     0,   592,   586,   616,     0,   621,     0,
       0,   626,   614,     0,   629,   647,   649,   651,   653,   655,
     657,     0,   706,   707,     0,   694,     0,   716,   717,   714,
       0,     0,     0,   761,   758,   769,   771,   773,   775,   777,
     779,   827,   845,   848,   871,     0,   843,   870,   881,     0,
       0,   898,   895,     0,     0,     0,     0,     0,     0,     0,
     186,    88,    86,    84,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   114,   121,   207,     0,     0,   266,
       0,   315,   316,   317,   318,   314,   349,     0,     0,     0,
       0,     0,   325,   327,   321,     0,   341,   360,   361,   414,
     410,   411,   412,   413,   378,   385,     0,   173,   174,   175,
     176,   177,   178,   179,   180,   388,   389,   564,   561,   562,
     563,   384,   387,     0,     0,     0,   386,     0,     0,   396,
       0,     0,     0,     0,     0,   520,   526,     0,   528,   529,
     525,   527,   482,   483,   484,   485,   486,   487,   488,   491,
     492,   493,   490,   489,   510,     0,     0,     0,     0,   511,
       0,   522,     0,     0,     0,   590,   600,   588,     0,   619,
       0,   622,   620,   627,     0,     0,     0,     0,     0,     0,
     663,   671,     0,   660,     0,     0,   693,     0,     0,     0,
       0,     0,     0,   746,   749,     0,   762,   763,   764,     0,
       0,     0,     0,     0,     0,   829,   849,     0,   863,     0,
       0,   885,     0,   905,   911,   902,     0,   925,     0,   927,
     938,     0,     0,     0,   217,   188,    89,   559,   559,    90,
      66,    78,    72,    82,    74,    68,    76,    70,    80,     0,
       0,     0,   211,   226,     0,   240,   228,   239,   264,     0,
       0,   312,   309,   343,     0,     0,   330,   331,   345,   326,
       0,     0,   322,     0,   364,     0,   383,   391,   418,   399,
       0,   403,   405,   394,     0,   381,   434,   377,   430,   421,
     415,     0,   524,     0,   548,     0,     0,   531,   552,   508,
     513,   533,   569,   506,   565,     0,   587,   595,     0,   623,
       0,   630,   631,   632,   633,   645,   650,   652,   645,     0,
     656,   679,   688,   695,     0,     0,   718,     0,     0,     0,
     722,   739,   742,     0,     0,   744,   765,   766,   760,     0,
       0,   790,     0,     0,   790,     0,   846,   867,   864,   866,
       0,   248,   882,     0,     0,     0,     0,   929,   944,   928,
     936,   932,   934,   248,     0,     0,     0,   192,     0,    91,
      91,   122,   209,   202,   212,   206,   229,   244,   234,     0,
       0,     0,   231,   236,   265,   263,     0,   267,   313,   311,
       0,   339,     0,   334,   335,   333,   332,     0,     0,   365,
     369,   367,     0,   803,   408,   409,   407,   406,   397,   469,
     432,   431,   422,     0,   546,   550,   555,   556,   554,   553,
     542,     0,   534,   536,   537,   535,   538,   541,   540,   539,
     518,     0,   567,   566,   594,     0,   591,     0,     0,     0,
     601,   589,   618,     0,   646,   648,   665,   664,   673,   672,
     654,     0,     0,   658,   701,   691,   692,   720,   719,   721,
       0,     0,     0,   750,     0,     0,   774,     0,     0,   780,
     828,     0,   830,     0,   850,   853,   854,     0,   868,   873,
     248,     0,   899,   906,   907,   908,     0,   248,     0,   912,
     913,   914,   248,     0,   946,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    92,    87,    85,   124,   210,   208,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   268,
       0,   340,   338,     0,   328,     0,     0,     0,   362,   400,
       0,     0,   425,   426,   424,   416,     0,   516,   557,   509,
       0,     0,     0,   572,   477,     0,   593,   602,   603,   604,
       0,   596,   625,   667,   675,     0,   680,   702,   696,     0,
     715,     0,   723,     0,   747,   784,     0,     0,     0,     0,
     772,   796,     0,     0,     0,     0,   791,   784,   778,   831,
       0,   856,     0,   858,   859,   860,     0,   861,   865,   887,
       0,   909,     0,   915,   910,   896,   904,     0,   945,     0,
     933,   935,   957,     0,     0,     0,   187,     0,   193,   560,
       0,   216,   214,   254,   245,   235,     0,   242,   241,   254,
       0,   271,   276,   277,   270,     0,   371,   371,   371,   805,
     809,   804,     0,   473,   474,     0,   435,   470,   472,   471,
     433,     0,     0,   551,   544,     0,   248,     0,   568,   598,
     599,   597,     0,   666,     0,   674,   661,     0,   698,   724,
     726,     0,     0,   770,     0,     0,     0,     0,     0,   797,
     792,   794,   793,   795,   776,   833,   851,     0,   855,   852,
     916,   917,     0,     0,   947,   937,     0,     0,   190,   194,
     123,     0,   125,   127,     0,   213,   214,     0,     0,     0,
     232,     0,   269,   324,     0,   372,     0,     0,     0,     0,
     401,   476,     0,     0,     0,   439,   423,     0,   248,   519,
       0,   570,   478,     0,     0,     0,   703,     0,   728,   728,
     740,   748,   782,     0,     0,   785,   798,   800,   799,   801,
     832,   857,   930,   949,   950,   948,     0,     0,     0,   189,
     196,   136,     0,   126,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   132,     0,   139,   135,   130,   142,
       0,   215,     0,     0,   243,     0,     0,     0,   272,   373,
     366,   370,   368,     0,   815,   816,   817,   813,   818,   810,
     811,   475,     0,     0,     0,     0,   436,   558,   514,     0,
       0,   439,     0,     0,     0,   699,   725,   727,   788,     0,
       0,     0,   834,   939,    97,    99,     0,   195,     0,   134,
     128,   181,   181,   181,   181,     0,     0,   181,     0,   169,
     170,   171,   172,   156,     0,   129,     0,     0,     0,   144,
     255,   230,     0,     0,   274,   275,   273,   807,     0,     0,
       0,   456,     0,   458,     0,   457,     0,   459,   441,     0,
       0,     0,   449,   450,   440,   455,   480,   479,   571,   668,
     676,     0,   704,     0,   729,   783,   786,   787,   836,   837,
     835,   931,   191,     0,   197,   137,   182,   145,   146,   147,
     148,   149,   151,   153,   154,   133,   140,   131,   159,     0,
       0,     0,     0,   814,   812,   460,   462,   461,   463,     0,
     443,     0,   438,   453,     0,   437,     0,     0,   662,     0,
     700,   732,   730,   789,     0,     0,     0,   940,   199,   200,
     198,   157,     0,     0,     0,     0,   161,     0,     0,   248,
       0,   808,   806,     0,     0,     0,   442,   451,     0,     0,
       0,     0,     0,   705,     0,     0,   942,   941,   943,     0,
     163,   183,   150,   152,   155,     0,   166,   160,     0,   233,
     237,     0,     0,     0,     0,   444,     0,   454,     0,     0,
       0,     0,     0,     0,     0,     0,   158,     0,     0,     0,
     162,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   464,   466,   465,   467,   669,   677,   733,   731,
     164,   165,     0,   167,   168,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   138,     0,   143,     0,
       0,     0,   468,     0,     0,     0,     0,     0,   248,     0,
     446,     0,     0,     0,     0,     0,     0,   141,     0,   447,
       0,     0,     0,   670,   678,     0,   445,     0,   452,     0,
     448,     0,     0,     0,     0,     0,     0,   238
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,   123, -1119, -1119,   136, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119,  -403, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119,  -591, -1072, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119,  1174, -1119, -1119, -1119, -1119, -1119,  -144,   109,
   -1119, -1119,  -343, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,  -497,
    -187,  -942,  -126,     3, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   627, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   355,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119,    31, -1119, -1119,
   -1119, -1119, -1119, -1119,  1059, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119,  -717, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
     343, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
     784,  -878, -1119, -1119,    95, -1119, -1119, -1119, -1119,  -176,
   -1119, -1119, -1119, -1118, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119,  -173, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119,   331, -1119,   391,   632,    91, -1119, -1119,    88,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119,  1141, -1119, -1119,
    -250, -1119, -1119,   761, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,   319, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119,  -865, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119,   -91, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,    68, -1119, -1119, -1119, -1119,   291, -1119,   171, -1089,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119,   739, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119,   422, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119,   292, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
    1000, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119, -1119,
   -1119, -1119
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     2,     3,     4,     7,    12,    55,    56,    57,   118,
      58,    59,   120,    60,   122,    61,   123,    62,    63,   150,
     209,   335,   413,   418,   421,   415,   417,   420,   414,   422,
     416,   675,   838,   837,  1105,    64,    65,    66,    67,   124,
      68,   125,   524,    69,   126,    70,    71,   157,   221,   222,
      72,    73,   158,   225,   339,   849,  1107,  1210,  1292,  1437,
    1434,  1418,  1531,  1436,  1536,  1438,  1533,  1534,  1535,  1560,
    1538,  1566,  1589,  1593,  1433,   725,  1487,  1532,   226,    74,
     133,   835,   967,  1349,  1102,  1208,  1350,  1417,  1484,    75,
     227,    76,   131,   851,   852,  1109,   975,  1110,  1295,  1296,
      77,    78,  1305,    79,    80,   160,   230,   342,   546,   687,
     856,  1111,  1117,  1375,  1113,  1118,  1595,   857,  1112,   189,
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
     589,   887,  1011,  1131,   888,  1009,  1315,  1462,  1463,  1396,
    1464,  1576,  1548,  1465,  1515,  1512,  1130,  1237,  1238,  1312,
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
     271,   272,   481,   370,   485,   797,  1054,  1158,  1327,  1472,
    1159,  1257,  1520,   636,   273,   100,   101,   170,   276,   372,
     800,   639,   930,  1060,  1162,  1328,  1329,  1406,  1474,  1555,
    1554,   102,   103,   171,   279,   487,  1061,   488,   489,   805,
     933,  1262,   934,   280,   104,   105,   172,   283,   284,   379,
     644,   808,   938,   380,   495,   809,   810,   811,   812,   813,
     814,  1263,  1408,  1264,  1335,  1475,  1066,  1176,  1170,  1171,
     285,  1129,  1231,  1309,  1502,  1310,  1389,  1449,  1448,  1390,
     106,   107,   214,   288,   173,   289,   382,   815,   945,  1072,
    1275,  1340,  1412,   108,   109,   292,   174,   293,   294,   497,
     816,   498,   946,  1074,  1075,  1076,  1277,   653,   818,  1077,
     501,   388,   500,   110,   111,   297,   175,   298,   299,   502,
     820,   391,   506,   660,   112,   113,   302,   176,   303,   508,
     824,   304,   507,   823,   395,   512,   664,   954,  1083,  1084,
    1088,   955,  1089,  1090,  1085,  1091,   114,   115,   398,   306,
     178,   305,   513,   665,   829,  1093,  1413,  1096,  1097,  1095,
    1481,  1527,   830,  1094,  1198,  1284,   116,   117,   309,   179,
     310,   666
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
     190,   525,   320,   655,   431,   748,  1008,   363,   454,  1079,
     460,   999,  1236,   447,  1458,   509,   503,   318,   405,   407,
     564,  1098,  1034,  1313,   479,  1392,  1394,   484,  1549,  1578,
     210,  1580,   509,  1450,  1452,  1454,  1456,   494,   990,  1587,
    1459,   727,   957,  1232,  1119,   386,   947,  1524,   358,   359,
     360,   361,  1190,   549,   550,   551,   552,   553,   554,   555,
     556,   557,   558,   384,   386,     1,  1591,   223,  1332,  1384,
    1385,  1521,   393,   358,   359,   360,   361,  1192,   503,   709,
     429,  1269,   430,   228,   976,  1386,   374,   232,   235,  1269,
     375,   376,  1387,   775,   241,   219,   993,   994,  1546,   995,
     247,   449,     5,   977,   978,   625,   626,   627,   628,  1004,
    1005,  1120,  1006,   645,   646,   647,   648,   649,   650,   710,
     711,   712,   713,   776,  1547,   530,   614,     6,   188,  1321,
     670,  1379,  1000,   358,   359,   360,   361,   251,  1189,   256,
    1191,  1202,   450,   262,   188,  1194,   264,   269,   362,   948,
    1196,   274,  1233,  1234,   277,   281,   286,   290,   295,   300,
     307,   397,    13,   822,   706,  1193,   377,   826,   560,   831,
     220,   452,   560,   453,   728,   729,   730,   632,   633,  1399,
     958,   634,   635,   798,   799,  1460,  1400,   559,   560,   803,
     458,   560,   459,   804,   949,  1351,  1522,   561,  1461,  1165,
    1333,  1334,  1177,   553,   554,   629,   556,  1180,   477,   592,
     478,     8,   593,   594,  1352,   572,   573,    14,   777,   560,
    1388,   560,   482,  1588,   483,   595,   596,   597,   598,    15,
     599,   579,  1525,   119,   600,   601,   602,   603,   224,   950,
     510,   504,   779,   492,   780,   493,   560,     9,    10,  1526,
    1592,   385,   505,  1081,   229,   121,   630,   510,   233,   236,
     499,   959,   127,   960,  1235,   242,   562,   782,   511,   780,
     387,   242,   961,   962,   358,   359,   360,   361,  1001,  1235,
     448,   656,   319,   406,   408,   188,   563,   188,  1314,   387,
    1393,  1395,   211,  1550,  1579,   863,  1581,  1451,  1453,  1455,
    1457,   188,   394,   504,  1319,   979,   980,   981,   252,   982,
     257,  1353,  1354,   983,   257,   565,   128,   265,   270,   567,
     129,   130,   275,  1181,  1182,   278,   282,   287,   291,   296,
     301,   308,   177,   134,   560,   516,   517,   518,   519,   520,
     521,   522,   523,   135,   855,   604,   560,   671,   672,   673,
    1488,  1489,  1490,   674,   605,  1493,  1355,  1356,  1357,  1358,
    1359,  1360,  1361,  1362,   606,   136,  1183,  1184,  1185,   781,
     781,   637,   638,   654,   190,   143,  1398,   659,   691,   692,
     693,   694,   612,   663,   613,   137,   324,   785,   786,   787,
     788,  1186,   936,   937,   325,   926,   927,   928,   929,   806,
     807,  1363,  1166,  1167,  1168,  1169,   607,   138,   608,  1509,
    1187,  1086,   188,  1364,   326,  1016,  1017,  1365,  1018,   632,
     633,   688,  1366,  1367,  1368,  1369,   327,  1320,  1235,   698,
    1577,  1259,  1260,  1574,   609,  1132,  1133,   610,  1134,   704,
     705,  1037,  1038,  1039,   328,   516,   517,   518,   519,   520,
     521,   522,   523,  1249,  1250,   329,  1251,   139,  1087,  1343,
    1344,   330,  1345,  1444,  1445,   743,  1446,   140,   819,   516,
     517,   518,   519,   520,   521,   522,   523,  1172,  1173,  1174,
    1175,   331,   717,   718,   719,   720,   721,   722,   723,   724,
      11,  1429,  1430,  1431,  1432,   622,    -9,   623,    -9,    -9,
      -9,    -9,    -9,    -9,   141,    -9,    -9,    -9,    -9,    -9,
    1307,  1308,    -9,    -9,  1478,  1479,   142,  1480,    -9,   827,
      -9,   828,  1528,  1529,  1141,  1530,   853,   985,   854,   986,
     909,    -9,    -9,   821,   190,   144,   145,   825,   190,  1070,
     190,  1071,  1160,    -9,  1161,  1206,  1222,  1207,  1223,    -9,
    1290,   146,  1291,   147,   148,   149,   152,   177,    -9,   332,
     153,   154,   858,   859,    -9,   155,   156,   333,   334,    -9,
     159,  1504,   180,   181,   182,   183,   184,   185,   186,   872,
     187,   188,    -9,   191,   192,    -9,    -9,   193,   194,  1122,
     195,   196,   197,   313,   198,   199,    -9,  1569,   200,    -9,
     752,   753,   754,   755,   756,   757,   758,   759,   760,   761,
     201,   202,   203,   204,   205,   206,   207,   890,  1022,  1023,
    1024,  1025,  1026,  1027,  1028,  1029,   208,   213,   215,   216,
     762,   217,   951,   218,   311,   953,    -9,   312,   314,   956,
     315,   316,   317,   322,   963,  1246,   323,   336,   337,    -9,
      -9,    -9,   338,   341,   340,   343,    -9,   763,   676,   677,
     678,   679,   680,   681,   682,   683,    -9,   349,    -9,   345,
     352,   354,   357,    16,   364,    17,    18,    19,    20,    21,
      22,   365,    23,    24,    25,    26,    27,   368,   369,    28,
      29,   371,   373,   381,   378,    30,  1647,    31,   570,   571,
     383,   572,   573,   389,    -9,   390,    -9,    -9,    32,    33,
     392,   574,   575,   576,   577,   396,   578,   579,   400,   401,
      34,   402,   403,   404,   580,   410,    35,   411,   412,   419,
     423,   424,   425,   984,   426,    36,   427,   428,   991,   466,
     467,    37,   472,    -9,   473,   476,    38,   486,   490,   491,
     496,   514,   532,   515,   526,   533,   534,   527,   528,    39,
     529,  1419,    40,    41,   535,  1013,  1080,   536,   537,  1092,
     538,   539,   540,    42,   541,   542,    43,   544,   543,  1035,
     545,   548,   569,   590,   616,   617,   621,   581,   582,   583,
     631,   640,   641,   642,   643,   651,   652,   658,   661,   662,
     667,   668,   669,   684,   685,   686,   696,   703,   708,   715,
     783,   726,   732,    44,   736,   746,   749,   751,   769,   771,
    -505,   778,   795,   850,   801,   796,    45,    46,    47,   190,
     802,   584,   560,    48,   817,   832,   833,   834,  1316,   836,
     869,   839,   861,    49,   862,    50,   840,   841,   842,   864,
     865,   585,   586,   868,   843,   844,   876,   845,   846,   847,
     848,   877,   878,   879,   873,   880,   883,  1293,   886,   885,
     889,   891,   892,   893,   894,   895,   896,   899,  1442,   900,
     901,    51,   902,    52,    53,   908,   911,  1137,   912,   910,
     913,   914,   587,   915,   588,   190,   918,   921,   923,   919,
     932,   922,   924,   925,   939,   935,   940,   931,   941,  1146,
     942,   943,   944,   964,   965,   966,   971,   972,   973,   968,
      54,   988,   997,   974,   998,  1002,  1052,  1010,  1014,  1015,
    1032,  1042,  1043,  1044,  1046,  1048,  1051,  1073,  1128,  1055,
    1056,  1057,  1058,  1059,  1062,  1063,  1064,  1065,  1067,  1068,
    1152,  1157,  1078,  1099,  1245,  1100,  1101,  1103,  1104,  1114,
    1115,  1108,  1116,  1121,  1123,  1124,  -543,  1439,  1195,  1143,
    1147,  1148,  1149,  1150,  1155,  1203,  1163,  1156,  1188,  1197,
    1179,  1199,  1200,  1201,  1164,  1242,  1204,  1209,  1211,  1213,
    1214,  1215,  1229,  1205,  1216,  1217,  1218,  1219,  1220,  1256,
    1226,  1225,  1227,  1228,  1258,  1261,  1252,  1254,  1265,  1266,
    1241,  1267,  1268,  1270,  1271,  1272,   190,  1273,  1276,  1278,
    1279,  1280,  1281,  1283,  1282,  1297,  1285,  1286,  1287,  1288,
    1299,  1289,  1303,  1301,  1311,  1317,  1323,  1324,  1326,  1331,
    1341,  1541,  1304,  1336,  1337,  1330,  1338,  1325,  1339,  1342,
    1346,  1347,  1348,  1374,  1377,  1510,  1383,  1391,  1318,  1402,
    1380,  1372,  1376,  1397,  1403,  1404,  1405,  1486,  1381,  1409,
    1410,  1411,  1416,  1420,  1421,  1447,  1422,  1382,  1423,  1424,
    1425,  1414,  1426,  1427,  1428,  1435,  1440,  1415,  1519,  1443,
    1466,  1471,  1476,  1469,  1470,  1477,  1615,  1473,  1482,  1483,
    1485,  1495,  1496,  1491,  1497,  1498,  1106,  1492,  1500,  1494,
    1501,  1503,  1513,  1505,  1506,  1523,  1543,  1551,  1507,  1544,
    1508,  1518,  1511,  1514,  1516,  1517,  1545,  1552,  1540,  1571,
    1537,  1553,  1556,  1557,  1558,  1559,  1561,  1562,  1563,  1564,
    1597,  1582,  1567,  1575,  1572,  1573,  1570,  1583,  1565,  1584,
    1585,  1617,  1371,  1542,  1586,  1590,  1594,  1596,  1598,  1599,
    1568,  1610,  1611,  1613,  1614,  1600,  1616,  1619,  1370,  1618,
    1235,  1620,  1373,  1602,  1601,  1622,  1623,  1629,  1630,  1603,
    1604,  1631,  1633,  1605,  1606,  1621,  1635,  1636,  1607,  1608,
    1634,  1637,  1639,  1642,  1609,  1628,  1640,  1624,  1641,  1643,
    1644,  1645,  1646,   132,  1651,  1649,  1650,  1625,  1652,  1632,
    1656,  1657,  1659,  1660,  1661,  1664,  1665,  1666,  1667,  1212,
     745,   996,  1300,   248,  1007,  1468,  1240,  1467,  1019,   970,
     750,  1244,  1655,  1248,   620,  1069,   167,  1050,  1407,  1178,
     657,  1294,   952,   611,  1648,  1274,  1082,  1441,  1663,   190,
    1653,     0,     0,  1654,     0,  1499,     0,     0,     0,     0,
       0,     0,  1658,     0,     0,  1662,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   399,     0,     0,     0,
       0,     0,  1539,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1627,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1638,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  1612,     0,     0,     0,   190,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1626
};

static const yytype_int16 yycheck[] =
{
     126,   404,   189,   500,   347,   596,   884,   257,   351,   951,
     353,     4,  1130,     4,     4,    30,    30,     6,     6,     6,
       4,   963,     4,     6,   367,     6,     6,   370,     6,     6,
       4,     6,    30,     6,     6,     6,     6,   380,     6,    58,
      30,    91,    28,    48,   166,    30,    71,    58,    76,    77,
      78,    79,   175,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    28,    30,   159,    58,    96,    19,    36,
      37,    31,    28,    76,    77,    78,    79,   175,    30,   180,
     266,  1170,   268,    96,    30,    52,   104,    96,    96,  1178,
     108,   109,    59,    41,    96,    21,     3,     4,     4,     6,
      96,     4,     0,    49,    50,    81,    82,    83,    84,     3,
       4,   233,     6,   112,   113,   114,   115,   116,   117,   220,
     221,   222,   223,    71,    30,   266,   469,     8,   269,  1247,
     533,   126,   125,    76,    77,    78,    79,    96,  1080,    96,
     263,   266,    45,    96,   269,  1087,    96,    96,    91,   174,
    1092,    96,   157,   158,    96,    96,    96,    96,    96,    96,
      96,    96,   266,   660,   567,   263,   184,   664,   167,   666,
      96,   266,   167,   268,   224,   225,   226,    98,    99,    48,
     166,   102,   103,   100,   101,   175,    55,   166,   167,   104,
     266,   167,   268,   108,   219,    28,   156,   176,   188,  1064,
     151,   152,  1067,    36,    37,   181,    39,    58,   266,    30,
     268,     4,    33,    34,    47,    36,    37,   266,   166,   167,
     187,   167,   266,   242,   268,    46,    47,    48,    49,   266,
      51,    52,   243,     6,    55,    56,    57,    58,   267,   264,
     255,   255,   270,   266,   272,   268,   167,   170,   171,   260,
     242,   216,   266,   268,   267,    94,   232,   255,   267,   267,
     386,   247,     6,   249,   269,   267,   245,   270,   266,   272,
     255,   267,   258,   259,    76,    77,    78,    79,   271,   269,
     271,   266,   271,   271,   271,   269,   265,   269,   271,   255,
     271,   271,   266,   271,   271,   698,   271,   270,   270,   270,
     270,   269,   258,   255,  1246,   251,   252,   253,   267,   255,
     267,   144,   145,   259,   267,   441,     6,   267,   267,   445,
     172,   173,   267,   174,   175,   267,   267,   267,   267,   267,
     267,   267,   267,     6,   167,    61,    62,    63,    64,    65,
      66,    67,    68,     6,   687,   166,   167,   163,   164,   165,
    1422,  1423,  1424,   169,   175,  1427,   189,   190,   191,   192,
     193,   194,   195,   196,   185,     6,   217,   218,   219,   619,
     620,   148,   149,   499,   500,     9,  1318,   503,   220,   221,
     222,   223,   266,   509,   268,     6,     1,    87,    88,    89,
      90,   242,   120,   121,     9,   150,   151,   152,   153,   118,
     119,   234,    87,    88,    89,    90,   227,     6,   229,    25,
     261,   268,   269,   246,    29,     3,     4,   250,     6,    98,
      99,   547,   255,   256,   257,   258,    41,   268,   269,   555,
    1548,   154,   155,    25,   255,    36,    37,   258,    39,   565,
     566,    72,    73,    74,    59,    61,    62,    63,    64,    65,
      66,    67,    68,     3,     4,    70,     6,     6,   955,     3,
       4,    76,     6,     3,     4,   591,     6,     6,   655,    61,
      62,    63,    64,    65,    66,    67,    68,    87,    88,    89,
      90,    96,   197,   198,   199,   200,   201,   202,   203,   204,
       1,   236,   237,   238,   239,   266,     7,   268,     9,    10,
      11,    12,    13,    14,     6,    16,    17,    18,    19,    20,
    1227,  1228,    23,    24,     3,     4,     6,     6,    29,   266,
      31,   268,     3,     4,  1021,     6,   266,   266,   268,   268,
     780,    42,    43,   659,   660,     6,     6,   663,   664,   266,
     666,   268,   266,    54,   268,   266,   228,   268,   230,    60,
     266,     6,   268,     6,     3,     3,     6,   267,    69,   174,
       6,     6,   688,   689,    75,     6,     6,   182,   183,    80,
       6,  1449,     4,   266,     4,    95,     4,     4,     4,   705,
       4,   269,    93,   266,     6,    96,    97,     4,   266,   992,
     266,   266,   266,     6,   266,   266,   107,  1539,   266,   110,
     205,   206,   207,   208,   209,   210,   211,   212,   213,   214,
     266,   266,   266,   266,   266,   266,   266,   743,    61,    62,
      63,    64,    65,    66,    67,    68,   266,   266,   266,   266,
     235,   266,   819,   266,   266,   822,   147,   266,   266,   826,
     266,     6,     6,    25,   831,  1142,     4,     6,    20,   160,
     161,   162,    19,    10,    25,    42,   167,   262,   535,   536,
     537,   538,   539,   540,   541,   542,   177,    43,   179,    31,
      54,    69,    93,     7,    75,     9,    10,    11,    12,    13,
      14,    80,    16,    17,    18,    19,    20,    97,   269,    23,
      24,   147,   107,   179,   110,    29,  1638,    31,    33,    34,
     215,    36,    37,   217,   215,    28,   217,   218,    42,    43,
     218,    46,    47,    48,    49,     4,    51,    52,   254,    48,
      54,   266,     6,     6,    59,     6,    60,     6,   266,   162,
      22,     4,     6,   859,     4,    69,     4,     4,   864,     4,
       4,    75,   269,   254,   269,     4,    80,     4,   266,     4,
       4,    60,    27,     6,   270,     6,     4,   270,   270,    93,
     270,  1352,    96,    97,     4,   891,   953,     4,     4,   956,
       4,     4,     4,   107,     4,     4,   110,   268,     6,   905,
      27,     4,   269,    28,     4,     4,     4,   122,   123,   124,
       4,     4,     4,     4,   111,     4,     4,     4,     4,     4,
      25,    25,     6,   266,    76,     6,     6,     6,     4,     6,
     268,     6,     6,   147,     6,     6,     6,     6,     6,     6,
       4,     4,     4,   175,     4,     6,   160,   161,   162,   955,
       4,   166,   167,   167,   268,     6,     6,   266,  1241,     3,
      71,     4,     4,   177,     4,   179,   266,   266,   266,     4,
       4,   186,   187,     4,   266,   266,     4,   266,   266,   266,
     266,     4,     4,     4,   269,     4,     4,  1210,     4,     6,
       4,     4,     4,     4,     4,     4,     4,     4,  1375,     4,
       4,   215,     4,   217,   218,     4,     6,  1013,     6,    92,
       6,     6,   227,     4,   229,  1021,     4,     4,     4,   269,
     266,   270,     6,     6,     4,   266,     4,   105,     4,  1035,
       4,     4,     4,    26,    26,    25,     4,     6,   266,   146,
     254,     4,     6,    28,     4,     4,   231,    53,     6,     6,
      53,     4,     6,     4,     4,     4,     4,   268,   270,     6,
       6,     6,     6,     6,     4,     4,     4,     4,     4,     4,
     266,   104,     6,     6,  1141,     6,     6,     6,     6,     6,
       6,   178,     6,     6,     6,     4,     4,  1370,   266,     6,
       6,     6,     6,     4,     4,    27,     4,     6,     4,     4,
     166,     4,     4,     4,   105,   146,    27,     6,     4,     4,
       4,     4,   269,    26,     6,     6,     6,     4,     4,   270,
       4,     6,     4,     4,   105,     4,   269,   269,     6,     6,
    1136,     6,     6,     6,     6,     6,  1142,     6,     6,     6,
       6,     6,     6,     4,    56,   268,     6,     6,     6,     6,
       6,   166,     6,   268,     6,     6,     4,     4,     4,     4,
       4,   126,   268,     6,     6,   266,     6,   269,     6,     6,
       6,     6,    27,     6,     4,  1458,     4,     4,  1245,     4,
     270,   175,   240,     6,     4,     4,     4,    28,   270,     6,
       6,     4,     6,     4,     6,     4,     6,   270,     6,     6,
       6,   266,     6,     6,     6,     3,     6,   266,   106,     6,
       6,     4,     6,   270,   270,     6,  1593,   269,     6,     4,
       4,     4,     4,    28,     4,     4,   970,    28,     6,    28,
     268,     4,     6,   270,   270,     4,     6,     4,   270,    25,
     270,   270,   269,   269,   269,   269,     6,     4,   268,    26,
     175,     6,     6,     6,     6,   175,     4,     4,     4,     4,
      26,     4,     6,  1546,     6,     6,   248,     4,   175,     4,
       4,     4,  1296,   270,     6,     6,     6,     6,     6,     6,
     269,     6,     6,     6,     6,    25,     6,     6,  1294,    27,
     269,     6,  1298,   270,   269,     6,     6,     4,     6,   270,
     270,    27,     6,   270,   270,    26,     6,     4,   270,   270,
      26,     4,     4,    27,   270,   270,     6,   269,     6,     6,
       6,     4,     4,    29,    27,     6,     6,   269,     6,   270,
       6,     6,     6,     6,     6,     6,     6,     6,     6,  1110,
     593,   866,  1219,   164,   881,  1401,  1131,  1400,   897,   838,
     598,  1140,   241,  1145,   473,   944,    95,   918,  1329,  1068,
     501,  1210,   820,   459,   268,  1177,   954,  1373,   244,  1375,
     270,    -1,    -1,   270,    -1,  1442,    -1,    -1,    -1,    -1,
      -1,    -1,   270,    -1,    -1,   268,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   306,    -1,    -1,    -1,
      -1,    -1,  1499,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1615,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1627,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1589,    -1,    -1,    -1,  1593,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1612
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,   159,   274,   275,   276,     0,     8,   277,     4,   170,
     171,     1,   278,   266,   266,   266,     7,     9,    10,    11,
      12,    13,    14,    16,    17,    18,    19,    20,    23,    24,
      29,    31,    42,    43,    54,    60,    69,    75,    80,    93,
      96,    97,   107,   110,   147,   160,   161,   162,   167,   177,
     179,   215,   217,   218,   254,   279,   280,   281,   283,   284,
     286,   288,   290,   291,   308,   309,   310,   311,   313,   316,
     318,   319,   323,   324,   352,   362,   364,   373,   374,   376,
     377,   398,   399,   411,   412,   413,   453,   454,   518,   556,
     558,   559,   576,   577,   578,   579,   594,   595,   620,   621,
     638,   639,   654,   655,   667,   668,   703,   704,   716,   717,
     736,   737,   747,   748,   769,   770,   789,   790,   282,     6,
     285,    94,   287,   289,   312,   314,   317,     6,     6,   172,
     173,   365,   365,   353,     6,     6,     6,     6,     6,     6,
       6,     6,     6,     9,     6,     6,     6,     6,     3,     3,
     292,   414,     6,     6,     6,     6,     6,   320,   325,     6,
     378,   400,   416,   455,   519,   560,   580,   580,   596,   622,
     640,   656,   669,   707,   719,   739,   750,   267,   773,   792,
       4,   266,     4,    95,     4,     4,     4,     4,   269,   392,
     395,   266,     6,     4,   266,   266,   266,   266,   266,   266,
     266,   266,   266,   266,   266,   266,   266,   266,   266,   293,
       4,   266,   415,   266,   705,   266,   266,   266,   266,    21,
      96,   321,   322,    96,   267,   326,   351,   363,    96,   267,
     379,   397,    96,   267,   401,    96,   267,   417,   418,   419,
     452,    96,   267,   456,   457,   458,   516,    96,   457,   520,
     557,    96,   267,   561,   562,   575,    96,   267,   581,   582,
     587,   593,    96,   592,    96,   267,   597,   598,   619,    96,
     267,   623,   624,   637,    96,   267,   641,    96,   267,   657,
     666,    96,   267,   670,   671,   693,    96,   267,   706,   708,
      96,   267,   718,   720,   721,    96,   267,   738,   740,   741,
      96,   267,   749,   751,   754,   774,   772,    96,   267,   791,
     793,   266,   266,     6,   266,   266,     6,     6,     6,   271,
     393,   395,    25,     4,     1,     9,    29,    41,    59,    70,
      76,    96,   174,   182,   183,   294,     6,    20,    19,   327,
      25,    10,   380,    42,   402,    31,   420,   422,   421,    43,
     459,   469,    54,   521,    69,   563,   564,    93,    76,    77,
      78,    79,    91,   583,    75,    80,   599,   600,    97,   269,
     626,   147,   642,   107,   104,   108,   109,   184,   110,   672,
     676,   179,   709,   215,    28,   216,    30,   255,   734,   217,
      28,   744,   218,    28,   258,   757,     4,    96,   771,   773,
     254,    48,   266,     6,     6,     6,   271,     6,   271,   394,
       6,     6,   266,   295,   301,   298,   303,   299,   296,   162,
     300,   297,   302,    22,     4,     6,     4,     4,     4,   266,
     268,   375,   423,   424,   425,   427,   430,   432,   434,   437,
     439,   443,   444,   447,   448,   450,   451,     4,   271,     4,
      45,   460,   266,   268,   375,   470,   485,   487,   266,   268,
     375,   522,   523,   537,   539,   541,     4,     4,   565,   566,
     584,   585,   269,   269,   588,   590,     4,   266,   268,   375,
     602,   625,   266,   268,   375,   627,     4,   658,   660,   661,
     266,     4,   266,   268,   375,   677,     4,   722,   724,   395,
     735,   733,   742,    30,   255,   266,   745,   755,   752,    30,
     255,   266,   758,   775,    60,     6,    61,    62,    63,    64,
      65,    66,    67,    68,   315,   315,   270,   270,   270,   270,
     266,   393,    27,     6,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     6,   268,    27,   381,   403,     4,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,   166,
     167,   176,   245,   265,     4,   395,   433,   395,   461,   269,
      33,    34,    36,    37,    46,    47,    48,    49,    51,    52,
      59,   122,   123,   124,   166,   186,   187,   227,   229,   493,
      28,   489,    30,    33,    34,    46,    47,    48,    49,    51,
      55,    56,    57,    58,   166,   175,   185,   227,   229,   255,
     258,   493,   266,   268,   375,   567,     4,     4,   583,   586,
     586,     4,   266,   268,   591,    81,    82,    83,    84,   181,
     232,     4,    98,    99,   102,   103,   636,   148,   149,   644,
       4,     4,     4,   111,   673,   112,   113,   114,   115,   116,
     117,     4,     4,   730,   395,   392,   266,   734,     4,   395,
     746,     4,     4,   395,   759,   776,   794,    25,    25,     6,
     315,   163,   164,   165,   169,   304,   304,   304,   304,   304,
     304,   304,   304,   304,   266,    76,     6,   382,   395,   404,
     428,   220,   221,   222,   223,   431,     6,   426,   395,   445,
     440,   449,   435,     6,   395,   395,   315,   463,     4,   180,
     220,   221,   222,   223,   484,     6,   473,   197,   198,   199,
     200,   201,   202,   203,   204,   348,     6,    91,   224,   225,
     226,   548,     6,   474,   488,   478,     6,   481,   475,   476,
     472,   471,   490,   395,   529,   431,     6,   532,   348,     6,
     548,     6,   205,   206,   207,   208,   209,   210,   211,   212,
     213,   214,   235,   262,   517,   538,   540,   542,   533,     6,
     525,     6,   527,   530,   524,    41,    71,   166,     4,   270,
     272,   583,   270,   268,   589,    87,    88,    89,    90,   603,
     604,   605,   606,   607,   608,     4,     6,   628,   100,   101,
     643,     4,     4,   104,   108,   662,   118,   119,   674,   678,
     679,   680,   681,   682,   683,   710,   723,   268,   731,   393,
     743,   395,   392,   756,   753,   395,   392,   266,   268,   777,
     785,   392,     6,     6,   266,   354,     3,   306,   305,     4,
     266,   266,   266,   266,   266,   266,   266,   266,   266,   328,
     175,   366,   367,   266,   268,   375,   383,   390,   395,   395,
     405,     4,     4,   315,     4,     4,   441,   442,     4,    71,
     436,   438,   395,   269,   464,   462,     4,     4,     4,     4,
       4,   482,   483,     4,   477,     6,     4,   494,   497,     4,
     395,     4,     4,     4,     4,     4,     4,   544,   545,     4,
       4,     4,     4,   549,   552,   569,   573,   568,     4,   583,
      92,     6,     6,     6,     6,     4,   610,   614,     4,   269,
     609,     4,   270,     4,     6,     6,   150,   151,   152,   153,
     645,   105,   266,   663,   665,   266,   120,   121,   675,     4,
       4,     4,     4,     4,     4,   711,   725,    71,   174,   219,
     264,   393,   745,   393,   760,   764,   393,    28,   166,   247,
     249,   258,   259,   393,    26,    26,    25,   355,   146,   547,
     547,     4,     6,   266,    28,   369,    30,    49,    50,   251,
     252,   253,   255,   259,   395,   266,   268,   406,     4,   429,
       6,   395,   446,     3,     4,     6,   442,     6,     4,     4,
     125,   271,     4,   479,     3,     4,     6,   483,   494,   498,
      53,   495,   486,   395,     6,     6,     3,     4,     6,   545,
     526,   528,    61,    62,    63,    64,    65,    66,    67,    68,
     534,   553,    53,   550,     4,   395,   570,    72,    73,    74,
     574,   571,     4,     6,     4,   601,     4,   611,     4,   615,
     601,     4,   231,   618,   629,     6,     6,     6,     6,     6,
     646,   659,     4,     4,     4,     4,   689,     4,     4,   689,
     266,   268,   712,   268,   726,   727,   728,   732,     6,   394,
     393,   268,   758,   761,   762,   767,   268,   392,   763,   765,
     766,   768,   393,   778,   786,   782,   780,   781,   394,     6,
       6,     6,   357,     6,     6,   307,   307,   329,   178,   368,
     370,   384,   391,   387,     6,     6,     6,   385,   388,   166,
     233,     6,   315,     6,     4,   465,   467,   466,   270,   694,
     509,   496,    36,    37,    39,   491,   492,   395,   543,   535,
     536,   392,   531,     6,   555,   551,   395,     6,     6,     6,
       4,   572,   266,   612,   616,     4,     6,   104,   630,   633,
     266,   268,   647,     4,   105,   636,    87,    88,    89,    90,
     691,   692,    87,    88,    89,    90,   690,   636,   691,   166,
      58,   174,   175,   217,   218,   219,   242,   261,     4,   394,
     175,   263,   175,   263,   394,   266,   394,     4,   787,     4,
       4,     4,   266,    27,    27,    26,   266,   268,   358,     6,
     330,     4,   372,     4,     4,     4,     6,     6,     6,     4,
       4,   407,   228,   230,   410,     6,     4,     4,     4,   269,
     480,   695,    48,   157,   158,   269,   506,   510,   511,   513,
     497,   395,   146,   546,   549,   393,   392,   514,   552,     3,
       4,     6,   269,   613,   269,   617,   270,   634,   105,   154,
     155,     4,   664,   684,   686,     6,     6,     6,     6,   692,
       6,     6,     6,     6,   684,   713,     6,   729,     6,     6,
       6,     6,    56,     4,   788,     6,     6,     6,     6,   166,
     266,   268,   331,   375,   450,   371,   372,   268,   396,     6,
     396,   268,   408,     6,   268,   375,   468,   468,   468,   696,
     698,     6,   512,     6,   271,   499,   315,     6,   393,   394,
     268,   506,   515,     4,     4,   269,     4,   631,   648,   649,
     266,     4,    19,   151,   152,   687,     6,     6,     6,     6,
     714,     4,     6,     3,     4,     6,     6,     6,    27,   356,
     359,    28,    47,   144,   145,   189,   190,   191,   192,   193,
     194,   195,   196,   234,   246,   250,   255,   256,   257,   258,
     395,   371,   175,   395,     6,   386,   240,     4,   409,   126,
     270,   270,   270,     4,    36,    37,    52,    59,   187,   699,
     702,     4,     6,   271,     6,   271,   502,     6,   394,    48,
      55,   554,     4,     4,     4,     4,   650,   650,   685,     6,
       6,     4,   715,   779,   266,   266,     6,   360,   334,   348,
       4,     6,     6,     6,     6,     6,     6,     6,     6,   236,
     237,   238,   239,   347,   333,     3,   336,   332,   338,   315,
       6,   395,   392,     6,     3,     4,     6,     4,   701,   700,
       6,   270,     6,   270,     6,   270,     6,   270,     4,    30,
     175,   188,   500,   501,   503,   506,     6,   517,   502,   270,
     270,     4,   632,   269,   651,   688,     6,     6,     3,     4,
       6,   783,     6,     4,   361,     4,    28,   349,   349,   349,
     349,    28,    28,   349,    28,     4,     4,     4,     4,   393,
       6,   268,   697,     4,   494,   270,   270,   270,   270,    25,
     315,   269,   508,     6,   269,   507,   269,   269,   270,   106,
     635,    31,   156,     4,    58,   243,   260,   784,     3,     4,
       6,   335,   350,   339,   340,   341,   337,   175,   343,   393,
     268,   126,   270,     6,    25,     6,     4,    30,   505,     6,
     271,     4,     4,     6,   653,   652,     6,     6,     6,   175,
     342,     4,     4,     4,     4,   175,   344,     6,   269,   394,
     248,    26,     6,     6,    25,   315,   504,   506,     6,   271,
       6,   271,     4,     4,     4,     4,     6,    58,   242,   345,
       6,    58,   242,   346,     6,   389,     6,    26,     6,     6,
      25,   269,   270,   270,   270,   270,   270,   270,   270,   270,
       6,     6,   395,     6,     6,   392,     6,     4,    27,     6,
       6,    26,     6,     6,   269,   269,   395,   393,   270,     4,
       6,    27,   270,     6,    26,     6,     4,     4,   393,     4,
       6,     6,    27,     6,     6,     4,     4,   394,   268,     6,
       6,    27,     6,   270,   270,   241,     6,     6,   270,     6,
       6,     6,   268,   244,     6,     6,     6,     6
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int16 yyr1[] =
{
       0,   273,   274,   275,   276,   275,   277,   277,   277,   278,
     278,   278,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   279,   279,   279,   279,   279,   279,
     279,   279,   279,   279,   280,   280,   280,   280,   280,   280,
     280,   280,   280,   280,   280,   280,   280,   280,   280,   280,
     280,   282,   281,   283,   285,   284,   287,   286,   289,   288,
     290,   292,   291,   293,   293,   295,   294,   296,   294,   297,
     294,   298,   294,   299,   294,   300,   294,   301,   294,   302,
     294,   303,   294,   294,   305,   304,   306,   304,   304,   304,
     304,   307,   307,   308,   309,   310,   312,   311,   314,   313,
     315,   315,   315,   315,   315,   315,   315,   315,   317,   316,
     318,   319,   320,   320,   321,   322,   323,   324,   325,   325,
     327,   328,   329,   326,   330,   330,   331,   331,   331,   331,
     332,   331,   333,   331,   331,   331,   334,   335,   331,   336,
     337,   331,   338,   331,   331,   331,   331,   331,   331,   339,
     331,   340,   331,   331,   341,   331,   331,   342,   342,   343,
     343,   344,   344,   345,   345,   345,   346,   346,   346,   347,
     347,   347,   347,   348,   348,   348,   348,   348,   348,   348,
     348,   349,   350,   349,   351,   353,   354,   352,   355,   355,
     356,   356,   357,   357,   359,   358,   360,   360,   361,   361,
     361,   363,   362,   364,   365,   365,   366,   367,   367,   368,
     368,   369,   370,   369,   371,   371,   372,   373,   374,   375,
     376,   377,   378,   378,   380,   381,   379,   382,   382,   384,
     383,   385,   386,   383,   387,   383,   388,   389,   383,   383,
     383,   390,   390,   390,   391,   390,   392,   393,   394,   394,
     395,   395,   395,   395,   396,   396,   397,   398,   399,   400,
     400,   402,   403,   401,   404,   404,   405,   405,   407,   406,
     406,   408,   408,   409,   409,   409,   410,   410,   411,   412,
     413,   414,   414,   415,   416,   416,   417,   418,   420,   419,
     421,   421,   421,   422,   422,   423,   423,   423,   423,   423,
     423,   423,   423,   423,   423,   423,   423,   424,   426,   425,
     428,   427,   429,   429,   430,   431,   431,   431,   431,   432,
     432,   433,   433,   435,   434,   436,   436,   438,   437,   440,
     439,   441,   441,   442,   442,   442,   443,   445,   444,   446,
     446,   447,   447,   447,   449,   448,   450,   450,   450,   451,
     452,   453,   454,   455,   455,   456,   457,   459,   458,   461,
     460,   462,   460,   463,   463,   465,   464,   466,   464,   467,
     464,   468,   468,   468,   469,   469,   471,   470,   470,   470,
     472,   470,   473,   470,   470,   470,   470,   470,   470,   470,
     474,   470,   470,   475,   470,   476,   477,   470,   478,   479,
     480,   470,   481,   470,   470,   482,   482,   483,   483,   483,
     484,   484,   484,   484,   484,   486,   485,   488,   487,   489,
     490,   489,   491,   491,   492,   492,   492,   493,   493,   493,
     494,   494,   496,   495,   498,   499,   497,   500,   501,   502,
     502,   503,   503,   503,   503,   503,   503,   503,   503,   503,
     503,   504,   503,   505,   503,   503,   506,   506,   506,   506,
     506,   506,   506,   506,   507,   507,   507,   507,   508,   509,
     509,   510,   510,   511,   512,   511,   513,   514,   514,   515,
     515,   516,   517,   517,   517,   517,   517,   517,   517,   517,
     517,   517,   517,   517,   518,   519,   519,   520,   521,   521,
     522,   522,   522,   522,   523,   524,   523,   525,   526,   523,
     523,   523,   527,   528,   523,   529,   523,   530,   531,   523,
     523,   523,   523,   532,   523,   523,   523,   523,   523,   523,
     533,   523,   523,   534,   534,   534,   534,   534,   534,   534,
     534,   534,   535,   536,   535,   538,   537,   540,   539,   542,
     543,   541,   544,   544,   545,   545,   545,   546,   546,   547,
     547,   548,   548,   548,   548,   549,   549,   551,   550,   553,
     554,   552,   555,   556,   557,   558,   559,   560,   560,   561,
     563,   562,   564,   564,   565,   566,   566,   567,   568,   567,
     569,   567,   567,   570,   570,   571,   571,   572,   572,   572,
     573,   573,   574,   574,   574,   575,   576,   577,   578,   579,
     580,   580,   581,   581,   582,   584,   583,   585,   583,   583,
     583,   586,   586,   586,   588,   587,   589,   589,   590,   590,
     591,   591,   591,   591,   592,   593,   594,   595,   596,   596,
     597,   599,   598,   600,   600,   601,   601,   603,   602,   604,
     602,   605,   602,   606,   602,   607,   602,   608,   602,   602,
     609,   609,   609,   610,   610,   612,   611,   613,   613,   613,
     613,   614,   614,   616,   615,   617,   617,   617,   617,   618,
     618,   619,   620,   621,   622,   622,   623,   625,   624,   626,
     626,   627,   627,   627,   628,   629,   627,   627,   631,   632,
     630,   633,   634,   633,   635,   635,   636,   636,   637,   638,
     639,   640,   640,   642,   643,   641,   644,   644,   645,   645,
     645,   645,   646,   646,   648,   647,   649,   647,   650,   650,
     652,   651,   653,   651,   654,   655,   656,   656,   658,   659,
     657,   660,   657,   661,   657,   657,   663,   664,   662,   665,
     662,   666,   667,   668,   669,   669,   670,   672,   671,   673,
     673,   674,   674,   674,   675,   675,   675,   676,   676,   678,
     677,   679,   677,   680,   677,   681,   677,   682,   677,   683,
     677,   677,   685,   684,   686,   686,   687,   687,   688,   688,
     689,   689,   690,   690,   690,   690,   691,   691,   692,   692,
     692,   692,   693,   694,   694,   696,   695,   697,   697,   698,
     698,   700,   699,   701,   699,   702,   702,   702,   702,   703,
     704,   705,   705,   706,   707,   707,   709,   710,   708,   711,
     711,   713,   712,   714,   714,   715,   715,   715,   716,   717,
     718,   719,   719,   720,   722,   723,   721,   724,   721,   725,
     725,   726,   726,   726,   726,   727,   729,   728,   728,   728,
     728,   728,   730,   730,   732,   731,   731,   731,   731,   733,
     733,   734,   735,   734,   736,   737,   738,   739,   739,   740,
     742,   743,   741,   744,   744,   745,   746,   745,   747,   748,
     749,   750,   750,   751,   752,   753,   751,   755,   756,   754,
     757,   757,   758,   759,   758,   760,   760,   761,   761,   762,
     763,   764,   764,   765,   765,   766,   767,   768,   769,   770,
     771,   772,   772,   774,   775,   773,   776,   776,   777,   778,
     779,   777,   780,   777,   781,   777,   782,   777,   777,   783,
     783,   784,   784,   784,   786,   785,   787,   787,   788,   788,
     788,   789,   790,   791,   792,   792,   794,   793
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
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
       1,     0,     0,     3,     2,     0,     0,    11,     0,     5,
       0,     3,     0,     2,     0,     4,     0,     2,     2,     2,
       2,     0,     9,     2,     1,     1,     2,     0,     3,     0,
       1,     0,     0,     4,     0,     2,     1,     8,     1,     2,
       3,     3,     0,     2,     0,     0,     6,     0,     2,     0,
       7,     0,     0,    10,     0,     4,     0,     0,    24,     1,
       1,     4,     4,     6,     0,     4,     1,     1,     0,     2,
       4,     4,     4,     4,     0,     3,     2,     4,     3,     0,
       2,     0,     0,     7,     2,     3,     0,     2,     0,     4,
       3,     0,     2,     2,     2,     2,     1,     1,     3,     3,
       3,     0,     2,     1,     0,     2,     3,     2,     0,     4,
       0,     2,     2,     0,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     4,
       0,     5,     0,     1,     3,     1,     1,     1,     1,     2,
       2,     2,     3,     0,     8,     0,     1,     0,     6,     0,
       4,     1,     2,     2,     2,     2,     2,     0,     6,     1,
       2,     3,     2,     4,     0,     4,     2,     2,     2,     3,
       2,     3,     3,     0,     2,     3,     1,     0,     3,     0,
       3,     0,     6,     0,     2,     0,     6,     0,     6,     0,
       6,     0,     1,     2,     0,     2,     0,     4,     3,     2,
       0,     4,     0,     4,     3,     3,     3,     3,     3,     3,
       0,     4,     1,     0,     4,     0,     0,     5,     0,     0,
       0,     8,     0,     4,     1,     1,     2,     2,     2,     2,
       1,     1,     1,     1,     1,     0,     6,     0,     4,     0,
       0,     3,     0,     3,     1,     1,     1,     1,     1,     1,
       1,     2,     0,     3,     0,     0,     6,     2,     2,     0,
       2,     1,     3,     2,     4,    10,     8,     9,    11,     1,
       1,     0,    10,     0,     4,     1,     4,     4,     4,     4,
       5,     5,     5,     5,     4,     4,     4,     4,     6,     0,
       2,     1,     1,     1,     0,     3,     2,     0,     2,     3,
       3,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     0,     2,     3,     0,     2,
       1,     1,     1,     1,     2,     0,     4,     0,     0,     6,
       3,     3,     0,     0,     9,     0,     6,     0,     0,     8,
       3,     2,     3,     0,     4,     3,     3,     3,     3,     3,
       0,     4,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     0,     0,     2,     0,     5,     0,     4,     0,
       0,     7,     1,     2,     2,     2,     2,     0,     3,     0,
       3,     1,     1,     1,     1,     1,     2,     0,     3,     0,
       0,     7,     1,     3,     2,     3,     3,     0,     2,     4,
       0,     3,     0,     2,     1,     0,     2,     3,     0,     4,
       0,     4,     1,     2,     1,     0,     2,     2,     2,     2,
       0,     2,     2,     2,     2,     2,     3,     3,     3,     3,
       0,     2,     1,     1,     4,     0,     3,     0,     6,     4,
       4,     1,     2,     3,     0,     8,     0,     1,     0,     2,
       3,     3,     3,     3,     2,     2,     3,     3,     0,     2,
       3,     0,     3,     0,     2,     0,     1,     0,     5,     0,
       4,     0,     4,     0,     5,     0,     4,     0,     5,     1,
       0,     4,     8,     0,     2,     0,     3,     0,     4,     8,
      12,     0,     2,     0,     3,     0,     4,     8,    12,     0,
       2,     2,     3,     3,     0,     2,     3,     0,     6,     0,
       2,     5,     5,     3,     0,     0,     6,     1,     0,     0,
       6,     0,     0,     3,     0,     2,     1,     1,     2,     4,
       3,     0,     2,     0,     0,     8,     1,     1,     1,     2,
       2,     2,     0,     2,     0,     4,     0,     4,     0,     2,
       0,     5,     0,     5,     3,     3,     0,     2,     0,     0,
      10,     0,     6,     0,     6,     3,     0,     0,     6,     0,
       3,     2,     3,     3,     0,     2,     3,     0,     4,     0,
       3,     0,     1,     1,     0,     1,     1,     0,     2,     0,
       7,     0,     6,     0,     5,     0,     7,     0,     6,     0,
       5,     1,     0,     4,     0,     2,     3,     3,     0,     2,
       0,     2,     2,     2,     2,     2,     1,     2,     3,     3,
       3,     3,     2,     0,     2,     0,     6,     0,     2,     0,
       2,     0,     3,     0,     3,     1,     1,     1,     1,     3,
       3,     0,     1,     2,     0,     2,     0,     0,     7,     0,
       2,     0,     4,     0,     2,     2,     2,     2,     3,     3,
       2,     0,     2,     4,     0,     0,     6,     0,     4,     0,
       2,     3,     3,     1,     1,     3,     0,     4,     2,     2,
       2,     2,     0,     2,     0,     4,     2,     2,     3,     0,
       2,     3,     0,     6,     3,     3,     2,     0,     2,     3,
       0,     0,     6,     0,     2,     3,     0,     6,     3,     3,
       2,     0,     2,     3,     0,     0,     8,     0,     0,     7,
       0,     2,     3,     0,     6,     0,     2,     1,     1,     2,
       2,     0,     2,     1,     1,     2,     3,     3,     4,     3,
       2,     0,     2,     0,     0,     6,     0,     2,     2,     0,
       0,     8,     0,     4,     0,     4,     0,     5,     1,     0,
       2,     2,     2,     2,     0,     4,     0,     2,     2,     2,
       2,     3,     3,     2,     0,     2,     0,     8
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


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

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



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

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value, defData); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, defrData *defData)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  YYUSE (defData);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep, defrData *defData)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep, defData);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule, defrData *defData)
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
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              , defData);
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
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, defrData *defData)
{
  YYUSE (yyvaluep);
  YYUSE (defData);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/*----------.
| yyparse.  |
`----------*/

int
yyparse (defrData *defData)
{
/* The lookahead symbol.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

    /* Number of syntax errors so far.  */
    int yynerrs;

    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
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

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
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
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
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

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex (&yylval, defData);
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
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
  case 4:
#line 220 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                { defData->dumb_mode = 1; defData->no_num = 1; }
#line 3018 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 5:
#line 221 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3047 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 7:
#line 248 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3064 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 8:
#line 261 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3085 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 51:
#line 301 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      {defData->dumb_mode = 1; defData->no_num = 1; }
#line 3091 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 52:
#line 302 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
            if (defData->callbacks->DesignCbk)
              CALLBACK(defData->callbacks->DesignCbk, defrDesignStartCbkType, (yyvsp[-1].string));
            defData->hasDes = 1;
          }
#line 3101 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 53:
#line 309 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3129 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 54:
#line 333 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 3135 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 55:
#line 334 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->TechnologyCbk)
              CALLBACK(defData->callbacks->TechnologyCbk, defrTechNameCbkType, (yyvsp[-1].string));
          }
#line 3144 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 56:
#line 339 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    {defData->dumb_mode = 1; defData->no_num = 1;}
#line 3150 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 57:
#line 340 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->ArrayNameCbk)
              CALLBACK(defData->callbacks->ArrayNameCbk, defrArrayNameCbkType, (yyvsp[-1].string));
          }
#line 3159 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 58:
#line 345 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                            { defData->dumb_mode = 1; defData->no_num = 1; }
#line 3165 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 59:
#line 346 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->FloorPlanNameCbk)
              CALLBACK(defData->callbacks->FloorPlanNameCbk, defrFloorPlanNameCbkType, (yyvsp[-1].string));
          }
#line 3174 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 60:
#line 352 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->HistoryCbk)
              CALLBACK(defData->callbacks->HistoryCbk, defrHistoryCbkType, &defData->History_text[0]);
          }
#line 3183 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 61:
#line 358 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PropDefStartCbk)
              CALLBACK(defData->callbacks->PropDefStartCbk, defrPropDefStartCbkType, 0);
          }
#line 3192 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 62:
#line 363 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->PropDefEndCbk)
              CALLBACK(defData->callbacks->PropDefEndCbk, defrPropDefEndCbkType, 0);
            defData->real_num = 0;     // just want to make sure it is reset 
          }
#line 3202 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 64:
#line 371 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { }
#line 3208 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 65:
#line 373 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       {defData->dumb_mode = 1; defData->no_num = 1; defData->Prop.clear(); }
#line 3214 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 66:
#line 375 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("design", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->DesignProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3226 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 67:
#line 382 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3232 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 68:
#line 384 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("net", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->NetProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3244 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 69:
#line 391 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                 { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3250 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 70:
#line 393 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("specialnet", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->SNetProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3262 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 71:
#line 400 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                   { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3268 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 72:
#line 402 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("region", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->RegionProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3280 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 73:
#line 409 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3286 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 74:
#line 411 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("group", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->GroupProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3298 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 75:
#line 418 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3304 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 76:
#line 420 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("component", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->CompProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3316 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 77:
#line 427 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3322 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 78:
#line 429 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("row", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->RowProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3334 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 79:
#line 438 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3340 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 80:
#line 440 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) {
                defData->Prop.setPropType("componentpin", (yyvsp[-2].string));
                CALLBACK(defData->callbacks->PropCbk, defrPropCbkType, &defData->Prop);
              }
              defData->session->CompPinProp.setPropType(defData->DEFCASE((yyvsp[-2].string)), defData->defPropDefType);
            }
#line 3352 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 81:
#line 448 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->dumb_mode = 1 ; defData->no_num = 1; defData->Prop.clear(); }
#line 3358 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 82:
#line 450 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3381 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 83:
#line 468 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    { yyerrok; yyclearin;}
#line 3387 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 84:
#line 470 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                 { defData->real_num = 0; }
#line 3393 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 85:
#line 471 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropInteger();
              defData->defPropDefType = 'I';
            }
#line 3402 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 86:
#line 475 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                 { defData->real_num = 1; }
#line 3408 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 87:
#line 476 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropReal();
              defData->defPropDefType = 'R';
              defData->real_num = 0;
            }
#line 3418 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 88:
#line 482 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropString();
              defData->defPropDefType = 'S';
            }
#line 3427 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 89:
#line 487 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropQString((yyvsp[0].string));
              defData->defPropDefType = 'Q';
            }
#line 3436 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 90:
#line 492 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->PropCbk) defData->Prop.setPropNameMapString((yyvsp[0].string));
              defData->defPropDefType = 'S';
            }
#line 3445 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 92:
#line 499 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->PropCbk) defData->Prop.setNumber((yyvsp[0].dval)); }
#line 3451 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 93:
#line 502 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->UnitsCbk) {
              if (defData->defValidNum((int)(yyvsp[-1].dval)))
                CALLBACK(defData->callbacks->UnitsCbk,  defrUnitsCbkType, (yyvsp[-1].dval));
            }
          }
#line 3462 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 94:
#line 510 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->DividerCbk)
              CALLBACK(defData->callbacks->DividerCbk, defrDividerCbkType, (yyvsp[-1].string));
            defData->hasDivChar = 1;
          }
#line 3472 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 95:
#line 517 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->BusBitCbk)
              CALLBACK(defData->callbacks->BusBitCbk, defrBusBitCbkType, (yyvsp[-1].string));
            defData->hasBusBit = 1;
          }
#line 3482 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 96:
#line 523 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                     {defData->dumb_mode = 1;defData->no_num = 1; }
#line 3488 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 97:
#line 525 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3503 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 98:
#line 535 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                             {defData->dumb_mode = 1;defData->no_num = 1; }
#line 3509 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 99:
#line 537 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3524 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 100:
#line 548 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 0;}
#line 3530 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 101:
#line 549 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 1;}
#line 3536 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 102:
#line 550 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 2;}
#line 3542 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 103:
#line 551 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 3;}
#line 3548 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 104:
#line 552 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 4;}
#line 3554 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 105:
#line 553 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 5;}
#line 3560 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 106:
#line 554 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 6;}
#line 3566 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 107:
#line 555 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 7;}
#line 3572 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 108:
#line 558 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            defData->Geometries.Reset();
          }
#line 3580 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 109:
#line 562 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->DieAreaCbk) {
               defData->DieArea.addPoint(&defData->Geometries);
               CALLBACK(defData->callbacks->DieAreaCbk, defrDieAreaCbkType, &(defData->DieArea));
            }
          }
#line 3591 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 110:
#line 571 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { }
#line 3597 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 111:
#line 574 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.4) {
             if (defData->callbacks->DefaultCapCbk)
                CALLBACK(defData->callbacks->DefaultCapCbk, defrDefaultCapCbkType, ROUND((yyvsp[0].dval)));
          } else {
             if (defData->callbacks->DefaultCapCbk) // write error only if cbk is set 
                if (defData->defaultCapWarnings++ < defData->settings->DefaultCapWarnings)
                   defData->defWarning(7017, "The DEFAULTCAP statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
#line 3612 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 114:
#line 590 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->VersionNum < 5.4) {
              if (defData->callbacks->PinCapCbk) {
                defData->PinCap.setPin(ROUND((yyvsp[-3].dval)));
                defData->PinCap.setCap((yyvsp[-1].dval));
                CALLBACK(defData->callbacks->PinCapCbk, defrPinCapCbkType, &(defData->PinCap));
              }
            }
          }
#line 3626 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 115:
#line 601 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { }
#line 3632 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 116:
#line 604 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { }
#line 3638 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 117:
#line 607 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->StartPinsCbk)
              CALLBACK(defData->callbacks->StartPinsCbk, defrStartPinsCbkType, ROUND((yyvsp[-1].dval)));
          }
#line 3647 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 120:
#line 616 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         {defData->dumb_mode = 1; defData->no_num = 1; }
#line 3653 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 121:
#line 617 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         {defData->dumb_mode = 1; defData->no_num = 1; }
#line 3659 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 122:
#line 618 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              defData->Pin.Setup((yyvsp[-4].string), (yyvsp[0].string));
            }
            defData->hasPort = 0;
          }
#line 3670 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 123:
#line 625 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->PinCbk)
              CALLBACK(defData->callbacks->PinCbk, defrPinCbkType, &defData->Pin);
          }
#line 3679 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 126:
#line 634 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk)
              defData->Pin.setSpecial();
          }
#line 3688 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 127:
#line 640 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->PinExtCbk)
              CALLBACK(defData->callbacks->PinExtCbk, defrPinExtCbkType, &defData->History_text[0]);
          }
#line 3697 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 128:
#line 646 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.setDirection((yyvsp[0].string));
          }
#line 3706 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 129:
#line 652 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3730 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 130:
#line 672 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                  { defData->dumb_mode = 1; }
#line 3736 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 131:
#line 673 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3759 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 132:
#line 692 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                  { defData->dumb_mode = 1; }
#line 3765 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 133:
#line 693 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3788 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 134:
#line 713 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) defData->Pin.setUse((yyvsp[0].string));
          }
#line 3796 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 135:
#line 717 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
               if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
                 defData->Pin.addPort();
               defData->hasPort = 1;
            }
          }
#line 3820 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 136:
#line 737 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      { defData->dumb_mode = 1; }
#line 3826 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 137:
#line 738 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              if (defData->hasPort)
                 defData->Pin.addPortLayer((yyvsp[0].string));
              else
                 defData->Pin.addLayer((yyvsp[0].string));
            }
          }
#line 3839 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 138:
#line 747 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              if (defData->hasPort)
                 defData->Pin.addPortLayerPts((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
              else
                 defData->Pin.addLayerPts((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
            }
          }
#line 3852 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 139:
#line 756 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 3858 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 140:
#line 757 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
                if (defData->hasPort)
                   defData->Pin.addPortPolygon((yyvsp[0].string));
                else
                   defData->Pin.addPolygon((yyvsp[0].string));
              }
            }
            
            defData->Geometries.Reset();            
          }
#line 3887 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 141:
#line 782 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->VersionNum >= 5.6) {  // only add if 5.6 or beyond
              if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
                if (defData->hasPort)
                   defData->Pin.addPortPolygonPts(&defData->Geometries);
                else
                   defData->Pin.addPolygonPts(&defData->Geometries);
              }
            }
          }
#line 3902 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 142:
#line 792 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    { defData->dumb_mode = 1; }
#line 3908 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 143:
#line 793 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
                if (defData->hasPort)
                   defData->Pin.addPortVia((yyvsp[-5].string), (int)(yyvsp[-2].dval),
                                               (int)(yyvsp[-1].dval), (yyvsp[-4].integer));
                else
                   defData->Pin.addVia((yyvsp[-5].string), (int)(yyvsp[-2].dval),
                                               (int)(yyvsp[-1].dval), (yyvsp[-4].integer));
              }
            }
          }
#line 3937 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 144:
#line 819 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk) {
              if (defData->hasPort) {
                 defData->Pin.setPortPlacement((yyvsp[-2].integer), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
                 defData->hasPort = 0;
              } else
                 defData->Pin.setPlacement((yyvsp[-2].integer), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
            }
          }
#line 3951 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 145:
#line 831 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3973 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 146:
#line 849 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 3995 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 147:
#line 867 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4017 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 148:
#line 885 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4039 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 149:
#line 902 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                                    {defData->dumb_mode=1;}
#line 4045 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 150:
#line 903 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4067 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 151:
#line 920 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                                        {defData->dumb_mode=1;}
#line 4073 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 152:
#line 922 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4095 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 153:
#line 940 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 154:
#line 957 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                                   {defData->dumb_mode=1;}
#line 4123 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 155:
#line 958 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4145 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 156:
#line 976 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4165 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 158:
#line 994 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4180 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 159:
#line 1007 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = 0; }
#line 4186 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 160:
#line 1009 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         { 
           if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->pinWarnings, defData->settings->PinWarnings)) {
             (yyval.integer) = (yyvsp[0].dval);
           }
         }
#line 4196 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 162:
#line 1017 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4211 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 164:
#line 1031 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4238 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 165:
#line 1054 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4265 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 167:
#line 1079 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4292 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 168:
#line 1102 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4319 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 169:
#line 1126 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->aOxide = 1;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4328 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 170:
#line 1131 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->aOxide = 2;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4337 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 171:
#line 1136 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->aOxide = 3;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4346 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 172:
#line 1141 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->aOxide = 4;
            if (defData->callbacks->PinCbk || defData->callbacks->PinExtCbk)
              defData->Pin.addAntennaModel(defData->aOxide);
          }
#line 4355 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 173:
#line 1147 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"SIGNAL"; }
#line 4361 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 174:
#line 1149 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"POWER"; }
#line 4367 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 175:
#line 1151 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"GROUND"; }
#line 4373 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 176:
#line 1153 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"CLOCK"; }
#line 4379 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 177:
#line 1155 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"TIEOFF"; }
#line 4385 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 178:
#line 1157 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"ANALOG"; }
#line 4391 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 179:
#line 1159 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"SCAN"; }
#line 4397 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 180:
#line 1161 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)"RESET"; }
#line 4403 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 181:
#line 1165 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (char*)""; }
#line 4409 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 182:
#line 1166 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  {defData->dumb_mode=1;}
#line 4415 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 183:
#line 1167 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.string) = (yyvsp[0].string); }
#line 4421 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 184:
#line 1170 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->PinEndCbk)
            CALLBACK(defData->callbacks->PinEndCbk, defrPinEndCbkType, 0);
        }
#line 4430 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 185:
#line 1175 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                {defData->dumb_mode = 2; defData->no_num = 2; }
#line 4436 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 186:
#line 1177 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
            defData->rowName = (yyvsp[-4].string);
            defData->Row.setup((yyvsp[-4].string), (yyvsp[-3].string), (yyvsp[-2].dval), (yyvsp[-1].dval), (yyvsp[0].integer));
          }
        }
#line 4447 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 187:
#line 1185 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) 
            CALLBACK(defData->callbacks->RowCbk, defrRowCbkType, &defData->Row);
        }
#line 4456 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 188:
#line 1191 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4471 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 189:
#line 1202 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
            defData->Row.setDo(ROUND((yyvsp[-3].dval)), ROUND((yyvsp[-1].dval)), defData->xStep, defData->yStep);
        }
#line 4511 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 190:
#line 1239 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          defData->hasDoStep = 0;
        }
#line 4519 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 191:
#line 1243 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          defData->hasDoStep = 1;
          defData->Row.setHasDoStep();
          defData->xStep = (yyvsp[-1].dval);
          defData->yStep = (yyvsp[0].dval);
        }
#line 4530 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 194:
#line 1254 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                            {defData->dumb_mode = DEF_MAX_INT; }
#line 4536 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 195:
#line 1256 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         { defData->dumb_mode = 0; }
#line 4542 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 198:
#line 1263 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4558 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 199:
#line 1275 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
             char propTp;
             propTp =  defData->session->RowProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "ROW");
             defData->Row.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 4571 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 200:
#line 1284 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RowCbk) {
             char propTp;
             propTp =  defData->session->RowProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "ROW");
             defData->Row.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 4584 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 201:
#line 1294 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->TrackCbk) {
            defData->Track.setup((yyvsp[-1].string));
          }
        }
#line 4594 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 202:
#line 1300 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
            defData->Track.setDo(ROUND((yyvsp[-7].dval)), ROUND((yyvsp[-4].dval)), (yyvsp[-2].dval));
            CALLBACK(defData->callbacks->TrackCbk, defrTrackCbkType, &defData->Track);
          }
        }
#line 4625 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 203:
#line 1328 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          (yyval.string) = (yyvsp[0].string);
        }
#line 4633 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 204:
#line 1333 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"X";}
#line 4639 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 205:
#line 1335 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"Y";}
#line 4645 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 208:
#line 1341 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
           { 
              if (defData->validateMaskInput((int)(yyvsp[-1].dval), defData->trackWarnings, defData->settings->TrackWarnings)) {
                  if (defData->callbacks->TrackCbk) {
                    defData->Track.addMask((yyvsp[-1].dval), (yyvsp[0].integer));
                  }
               }
            }
#line 4657 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 209:
#line 1351 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = 0; }
#line 4663 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 210:
#line 1353 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = 1; }
#line 4669 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 212:
#line 1356 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1000; }
#line 4675 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 213:
#line 1357 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { defData->dumb_mode = 0; }
#line 4681 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 216:
#line 1364 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->TrackCbk)
            defData->Track.addLayer((yyvsp[0].string));
        }
#line 4690 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 217:
#line 1371 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
            defData->GcellGrid.setup((yyvsp[-6].string), ROUND((yyvsp[-5].dval)), ROUND((yyvsp[-3].dval)), (yyvsp[-1].dval));
            CALLBACK(defData->callbacks->GcellGridCbk, defrGcellGridCbkType, &defData->GcellGrid);
          }
        }
#line 4721 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 218:
#line 1399 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ExtensionCbk)
             CALLBACK(defData->callbacks->ExtensionCbk, defrExtensionCbkType, &defData->History_text[0]);
        }
#line 4730 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 219:
#line 1405 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { }
#line 4736 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 221:
#line 1411 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ViaStartCbk)
            CALLBACK(defData->callbacks->ViaStartCbk, defrViaStartCbkType, ROUND((yyvsp[-1].dval)));
        }
#line 4745 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 224:
#line 1420 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                     {defData->dumb_mode = 1;defData->no_num = 1; }
#line 4751 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 225:
#line 1421 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->ViaCbk) defData->Via.setup((yyvsp[0].string));
              defData->viaRule = 0;
            }
#line 4760 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 226:
#line 1426 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->ViaCbk)
                CALLBACK(defData->callbacks->ViaCbk, defrViaCbkType, &defData->Via);
              defData->Via.clear();
            }
#line 4770 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 229:
#line 1436 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       {defData->dumb_mode = 1;defData->no_num = 1; }
#line 4776 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 230:
#line 1437 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { 
              if (defData->callbacks->ViaCbk)
                if (defData->validateMaskInput((yyvsp[-2].integer), defData->viaWarnings, defData->settings->ViaWarnings)) {
                    defData->Via.addLayer((yyvsp[-3].string), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y, (yyvsp[-2].integer));
                }
            }
#line 4787 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 231:
#line 1443 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 4793 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 232:
#line 1444 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4815 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 233:
#line 1462 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->VersionNum >= 5.6) {  // only add if 5.6 or beyond
                if (defData->callbacks->ViaCbk)
                  if (defData->validateMaskInput((yyvsp[-5].integer), defData->viaWarnings, defData->settings->ViaWarnings)) {
                    defData->Via.addPolygon((yyvsp[-6].string), &defData->Geometries, (yyvsp[-5].integer));
                  }
              }
            }
#line 4828 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 234:
#line 1470 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                            {defData->dumb_mode = 1;defData->no_num = 1; }
#line 4834 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 235:
#line 1471 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->VersionNum < 5.6) {
                if (defData->callbacks->ViaCbk)
                  defData->Via.addPattern((yyvsp[0].string));
              } else
                if (defData->callbacks->ViaCbk)
                  if (defData->viaWarnings++ < defData->settings->ViaWarnings)
                    defData->defWarning(7019, "The PATTERNNAME statement is obsolete in version 5.6 and later.\nThe DEF parser will ignore this statement."); 
            }
#line 4848 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 236:
#line 1480 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        {defData->dumb_mode = 1;defData->no_num = 1; }
#line 4854 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 237:
#line 1482 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       {defData->dumb_mode = 3;defData->no_num = 1; }
#line 4860 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 238:
#line 1485 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4885 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 240:
#line 1507 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            if (defData->callbacks->ViaExtCbk)
              CALLBACK(defData->callbacks->ViaExtCbk, defrViaExtCbkType, &defData->History_text[0]);
          }
#line 4894 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 241:
#line 1513 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4910 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 242:
#line 1525 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4926 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 243:
#line 1537 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4942 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 244:
#line 1548 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        {defData->dumb_mode = 1;defData->no_num = 1; }
#line 4948 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 245:
#line 1549 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 4964 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 246:
#line 1562 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->Geometries.startList((yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 4970 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 247:
#line 1565 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { defData->Geometries.addToList((yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 4976 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 250:
#line 1572 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            defData->save_x = (yyvsp[-2].dval);
            defData->save_y = (yyvsp[-1].dval);
            (yyval.pt).x = ROUND((yyvsp[-2].dval));
            (yyval.pt).y = ROUND((yyvsp[-1].dval));
          }
#line 4987 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 251:
#line 1579 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            defData->save_y = (yyvsp[-1].dval);
            (yyval.pt).x = ROUND(defData->save_x);
            (yyval.pt).y = ROUND((yyvsp[-1].dval));
          }
#line 4997 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 252:
#line 1585 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            defData->save_x = (yyvsp[-2].dval);
            (yyval.pt).x = ROUND((yyvsp[-2].dval));
            (yyval.pt).y = ROUND(defData->save_y);
          }
#line 5007 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 253:
#line 1591 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            (yyval.pt).x = ROUND(defData->save_x);
            (yyval.pt).y = ROUND(defData->save_y);
          }
#line 5016 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 254:
#line 1597 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.integer) = 0; }
#line 5022 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 255:
#line 1599 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.integer) = (yyvsp[0].dval); }
#line 5028 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 256:
#line 1602 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->ViaEndCbk)
            CALLBACK(defData->callbacks->ViaEndCbk, defrViaEndCbkType, 0);
        }
#line 5037 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 257:
#line 1608 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RegionEndCbk)
            CALLBACK(defData->callbacks->RegionEndCbk, defrRegionEndCbkType, 0);
        }
#line 5046 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 258:
#line 1614 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RegionStartCbk)
            CALLBACK(defData->callbacks->RegionStartCbk, defrRegionStartCbkType, ROUND((yyvsp[-1].dval)));
        }
#line 5055 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 260:
#line 1621 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {}
#line 5061 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 261:
#line 1623 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5067 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 262:
#line 1624 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk)
             defData->Region.setup((yyvsp[0].string));
          defData->regTypeDef = 0;
        }
#line 5077 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 263:
#line 1630 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { CALLBACK(defData->callbacks->RegionCbk, defrRegionCbkType, &defData->Region); }
#line 5083 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 264:
#line 1634 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->RegionCbk)
          defData->Region.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 5090 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 265:
#line 1637 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->RegionCbk)
          defData->Region.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y); }
#line 5097 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 268:
#line 1645 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                               {defData->dumb_mode = DEF_MAX_INT; }
#line 5103 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 269:
#line 1647 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         { defData->dumb_mode = 0; }
#line 5109 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 270:
#line 1649 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5126 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 273:
#line 1668 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5145 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 274:
#line 1683 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk) {
             char propTp;
             propTp = defData->session->RegionProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "REGION");
             defData->Region.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5158 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 275:
#line 1692 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->RegionCbk) {
             char propTp;
             propTp = defData->session->RegionProp.propType((yyvsp[-1].string));
             CHKPROPTYPE(propTp, (yyvsp[-1].string), "REGION");
             defData->Region.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5171 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 276:
#line 1702 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"FENCE"; }
#line 5177 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 277:
#line 1704 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"GUIDE"; }
#line 5183 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 278:
#line 1707 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5203 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 280:
#line 1727 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         { 
            if (defData->callbacks->ComponentStartCbk)
              CALLBACK(defData->callbacks->ComponentStartCbk, defrComponentStartCbkType,
                       ROUND((yyvsp[-1].dval)));
         }
#line 5213 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 283:
#line 1738 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
            if (defData->callbacks->ComponentMaskShiftLayerCbk) {
              defData->ComponentMaskShiftLayer.addMaskShiftLayer((yyvsp[0].string));
            }
        }
#line 5223 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 286:
#line 1749 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         {
            if (defData->callbacks->ComponentCbk)
              CALLBACK(defData->callbacks->ComponentCbk, defrComponentCbkType, &defData->Component);
         }
#line 5232 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 287:
#line 1755 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         {
            defData->dumb_mode = 0;
            defData->no_num = 0;
         }
#line 5241 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 288:
#line 1760 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      {defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT; }
#line 5247 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 289:
#line 1762 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         {
            if (defData->callbacks->ComponentCbk)
              defData->Component.IdAndName((yyvsp[-1].string), (yyvsp[0].string));
         }
#line 5256 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 290:
#line 1768 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { }
#line 5262 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 291:
#line 1770 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->ComponentCbk)
                defData->Component.addNet("*");
            }
#line 5271 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 292:
#line 1775 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->ComponentCbk)
                defData->Component.addNet((yyvsp[0].string));
            }
#line 5280 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 307:
#line 1790 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            CALLBACK(defData->callbacks->ComponentExtCbk, defrComponentExtCbkType,
                     &defData->History_text[0]);
        }
#line 5290 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 308:
#line 1796 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                          {defData->dumb_mode=1; defData->no_num = 1; }
#line 5296 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 309:
#line 1797 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setEEQ((yyvsp[0].string));
        }
#line 5305 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 310:
#line 1802 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                              { defData->dumb_mode = 2;  defData->no_num = 2; }
#line 5311 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 311:
#line 1804 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
             defData->Component.setGenerate((yyvsp[-1].string), (yyvsp[0].string));
        }
#line 5320 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 312:
#line 1810 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)""; }
#line 5326 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 313:
#line 1812 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (yyvsp[0].string); }
#line 5332 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 314:
#line 1815 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setSource((yyvsp[0].string));
        }
#line 5341 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 315:
#line 1821 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"NETLIST"; }
#line 5347 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 316:
#line 1823 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"DIST"; }
#line 5353 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 317:
#line 1825 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"USER"; }
#line 5359 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 318:
#line 1827 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"TIMING"; }
#line 5365 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 319:
#line 1832 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { }
#line 5371 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 320:
#line 1834 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setRegionName((yyvsp[0].string));
        }
#line 5380 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 321:
#line 1840 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5395 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 322:
#line 1851 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5410 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 323:
#line 1863 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5429 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 324:
#line 1878 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setHalo((int)(yyvsp[-3].dval), (int)(yyvsp[-2].dval),
                                                 (int)(yyvsp[-1].dval), (int)(yyvsp[0].dval));
        }
#line 5439 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 326:
#line 1886 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5461 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 327:
#line 1905 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                       { defData->dumb_mode = 2; defData->no_num = 2; }
#line 5467 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 328:
#line 1906 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5490 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 329:
#line 1925 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                              { defData->dumb_mode = DEF_MAX_INT; }
#line 5496 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 330:
#line 1927 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 5502 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 333:
#line 1934 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5517 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 334:
#line 1945 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            char propTp;
            propTp = defData->session->CompProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "COMPONENT");
            defData->Component.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5530 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 335:
#line 1954 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            char propTp;
            propTp = defData->session->CompProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "COMPONENT");
            defData->Component.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 5543 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 336:
#line 1964 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5549 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 337:
#line 1966 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                            { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5555 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 338:
#line 1968 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5571 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 339:
#line 1982 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         { (yyval.pt) = (yyvsp[0].pt); }
#line 5577 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 340:
#line 1984 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
         { (yyval.pt).x = ROUND((yyvsp[-1].dval)); (yyval.pt).y = ROUND((yyvsp[0].dval)); }
#line 5583 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 341:
#line 1987 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk) {
            defData->Component.setPlacementStatus((yyvsp[-2].integer));
            defData->Component.setPlacementLocation((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer));
          }
        }
#line 5594 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 342:
#line 1994 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setPlacementStatus(
                                         DEFI_COMPONENT_UNPLACED);
            defData->Component.setPlacementLocation(-1, -1, -1);
        }
#line 5605 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 343:
#line 2001 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5622 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 344:
#line 2015 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                           { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5628 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 345:
#line 2016 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {  
          if (defData->callbacks->ComponentCbk) {
            if (defData->validateMaskShiftInput((yyvsp[0].string), defData->componentWarnings, defData->settings->ComponentWarnings)) {
                defData->Component.setMaskShift((yyvsp[0].string));
            }
          }
        }
#line 5640 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 346:
#line 2025 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = DEFI_COMPONENT_FIXED; }
#line 5646 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 347:
#line 2027 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = DEFI_COMPONENT_COVER; }
#line 5652 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 348:
#line 2029 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = DEFI_COMPONENT_PLACED; }
#line 5658 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 349:
#line 2032 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->ComponentCbk)
            defData->Component.setWeight(ROUND((yyvsp[0].dval)));
        }
#line 5667 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 350:
#line 2038 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->ComponentCbk)
            CALLBACK(defData->callbacks->ComponentEndCbk, defrComponentEndCbkType, 0);
        }
#line 5676 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 352:
#line 2047 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->NetStartCbk)
            CALLBACK(defData->callbacks->NetStartCbk, defrNetStartCbkType, ROUND((yyvsp[-1].dval)));
          defData->netOsnet = 1;
        }
#line 5686 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 355:
#line 2058 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->NetCbk)
            CALLBACK(defData->callbacks->NetCbk, defrNetCbkType, &defData->Net);
        }
#line 5695 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 356:
#line 2069 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {defData->dumb_mode = 0; defData->no_num = 0; }
#line 5701 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 357:
#line 2072 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT; defData->nondef_is_keyword = TRUE; defData->mustjoin_is_keyword = TRUE;}
#line 5707 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 359:
#line 2075 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          // 9/22/1999 
          // this is shared by both net and special net 
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.setName((yyvsp[0].string));
          if (defData->callbacks->NetNameCbk)
            CALLBACK(defData->callbacks->NetNameCbk, defrNetNameCbkType, (yyvsp[0].string));
        }
#line 5720 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 361:
#line 2083 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                  {defData->dumb_mode = 1; defData->no_num = 1;}
#line 5726 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 362:
#line 2084 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addMustPin((yyvsp[-3].string), (yyvsp[-1].string), 0);
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 5737 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 365:
#line 2095 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                             {defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT;}
#line 5743 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 366:
#line 2097 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5760 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 367:
#line 2109 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  {defData->dumb_mode = 1; defData->no_num = 1;}
#line 5766 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 368:
#line 2110 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addPin("*", (yyvsp[-2].string), (yyvsp[-1].integer));
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 5777 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 369:
#line 2116 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    {defData->dumb_mode = 1; defData->no_num = 1;}
#line 5783 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 370:
#line 2117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if ((defData->callbacks->NetCbk && (defData->netOsnet==1)) || (defData->callbacks->SNetCbk && (defData->netOsnet==2)))
            defData->Net.addPin("PIN", (yyvsp[-2].string), (yyvsp[-1].integer));
          defData->dumb_mode = 3;
          defData->no_num = 3;
        }
#line 5794 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 371:
#line 2125 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { (yyval.integer) = 0; }
#line 5800 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 372:
#line 2127 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->NetConnectionExtCbk)
            CALLBACK(defData->callbacks->NetConnectionExtCbk, defrNetConnectionExtCbkType,
              &defData->History_text[0]);
          (yyval.integer) = 0;
        }
#line 5811 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 373:
#line 2134 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = 1; }
#line 5817 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 376:
#line 2143 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {  
          if (defData->callbacks->NetCbk) defData->Net.addWire((yyvsp[0].string), NULL);
        }
#line 5825 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 377:
#line 2147 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          defData->by_is_keyword = FALSE;
          defData->do_is_keyword = FALSE;
          defData->new_is_keyword = FALSE;
          defData->nondef_is_keyword = FALSE;
          defData->mustjoin_is_keyword = FALSE;
          defData->step_is_keyword = FALSE;
          defData->orient_is_keyword = FALSE;
          defData->virtual_is_keyword = FALSE;
          defData->rect_is_keyword = FALSE;
          defData->mask_is_keyword = FALSE;
          defData->needNPCbk = 0;
        }
#line 5843 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 378:
#line 2162 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setSource((yyvsp[0].string)); }
#line 5849 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 379:
#line 2165 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5869 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 380:
#line 2181 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                          { defData->real_num = 1; }
#line 5875 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 381:
#line 2182 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 5896 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 382:
#line 2199 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode = 1; defData->no_num = 1;}
#line 5902 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 383:
#line 2200 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setOriginal((yyvsp[0].string)); }
#line 5908 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 384:
#line 2203 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setPattern((yyvsp[0].string)); }
#line 5914 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 385:
#line 2206 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setWeight(ROUND((yyvsp[0].dval))); }
#line 5920 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 386:
#line 2209 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setXTalk(ROUND((yyvsp[0].dval))); }
#line 5926 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 387:
#line 2212 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setCap((yyvsp[0].dval)); }
#line 5932 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 388:
#line 2215 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setUse((yyvsp[0].string)); }
#line 5938 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 389:
#line 2218 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.setStyle((int)(yyvsp[0].dval)); }
#line 5944 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 390:
#line 2220 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                               { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5950 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 391:
#line 2221 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->NetCbk && defData->callbacks->NetNonDefaultRuleCbk) {
             // User wants a callback on nondefaultrule 
             CALLBACK(defData->callbacks->NetNonDefaultRuleCbk,
                      defrNetNonDefaultRuleCbkType, (yyvsp[0].string));
          }
          // Still save data in the class 
          if (defData->callbacks->NetCbk) defData->Net.setNonDefaultRule((yyvsp[0].string));
        }
#line 5964 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 393:
#line 2233 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                          { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5970 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 394:
#line 2234 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addShieldNet((yyvsp[0].string)); }
#line 5976 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 395:
#line 2236 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         { defData->dumb_mode = 1; defData->no_num = 1; }
#line 5982 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 396:
#line 2237 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { // since the parser still support 5.3 and earlier, can't 
          // move NOSHIELD in net_type 
          if (defData->VersionNum < 5.4) {   // PCR 445209 
            if (defData->callbacks->NetCbk) defData->Net.addNoShield("");
            defData->by_is_keyword = FALSE;
            defData->do_is_keyword = FALSE;
            defData->new_is_keyword = FALSE;
            defData->step_is_keyword = FALSE;
            defData->orient_is_keyword = FALSE;
            defData->virtual_is_keyword = FALSE;
            defData->mask_is_keyword = FALSE;
            defData->rect_is_keyword = FALSE;
            defData->shield = TRUE;    // save the path info in the defData->shield paths 
          } else
            if (defData->callbacks->NetCbk) defData->Net.addWire("NOSHIELD", NULL);
        }
#line 6003 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 397:
#line 2254 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->VersionNum < 5.4) {   // PCR 445209 
            defData->shield = FALSE;
            defData->by_is_keyword = FALSE;
            defData->do_is_keyword = FALSE;
            defData->new_is_keyword = FALSE;
            defData->step_is_keyword = FALSE;
            defData->nondef_is_keyword = FALSE;
            defData->mustjoin_is_keyword = FALSE;
            defData->orient_is_keyword = FALSE;
            defData->virtual_is_keyword = FALSE;
            defData->rect_is_keyword = FALSE;
            defData->mask_is_keyword = FALSE;
          } else {
            defData->by_is_keyword = FALSE;
            defData->do_is_keyword = FALSE;
            defData->new_is_keyword = FALSE;
            defData->step_is_keyword = FALSE;
            defData->nondef_is_keyword = FALSE;
            defData->mustjoin_is_keyword = FALSE;
            defData->orient_is_keyword = FALSE;
            defData->virtual_is_keyword = FALSE;
            defData->rect_is_keyword = FALSE;
            defData->mask_is_keyword = FALSE;
          }
          defData->needNPCbk = 0;
        }
#line 6035 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 398:
#line 2283 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { defData->dumb_mode = 1; defData->no_num = 1;
          if (defData->callbacks->NetCbk) {
            defData->Subnet = new defiSubnet(defData);
          }
        }
#line 6045 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 399:
#line 2288 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6060 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 400:
#line 2298 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                   {
          defData->routed_is_keyword = TRUE;
          defData->fixed_is_keyword = TRUE;
          defData->cover_is_keyword = TRUE;
        }
#line 6070 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 401:
#line 2302 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {
          if (defData->callbacks->NetCbk) {
            defData->Net.addSubnet(defData->Subnet);
            defData->Subnet = NULL;
            defData->routed_is_keyword = FALSE;
            defData->fixed_is_keyword = FALSE;
            defData->cover_is_keyword = FALSE;
          }
        }
#line 6084 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 402:
#line 2312 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode = DEF_MAX_INT; }
#line 6090 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 403:
#line 2314 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { defData->dumb_mode = 0; }
#line 6096 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 404:
#line 2317 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { 
          if (defData->callbacks->NetExtCbk)
            CALLBACK(defData->callbacks->NetExtCbk, defrNetExtCbkType, &defData->History_text[0]);
        }
#line 6105 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 407:
#line 2327 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6120 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 408:
#line 2338 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->NetCbk) {
            char propTp;
            propTp = defData->session->NetProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "NET");
            defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 6133 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 409:
#line 2347 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          if (defData->callbacks->NetCbk) {
            char propTp;
            propTp = defData->session->NetProp.propType((yyvsp[-1].string));
            CHKPROPTYPE(propTp, (yyvsp[-1].string), "NET");
            defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
          }
        }
#line 6146 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 410:
#line 2357 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"NETLIST"; }
#line 6152 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 411:
#line 2359 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"DIST"; }
#line 6158 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 412:
#line 2361 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"USER"; }
#line 6164 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 413:
#line 2363 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"TIMING"; }
#line 6170 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 414:
#line 2365 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"TEST"; }
#line 6176 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 415:
#line 2368 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        {
          // vpin_options may have to deal with orient 
          defData->orient_is_keyword = TRUE;
        }
#line 6185 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 416:
#line 2373 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk)
            defData->Net.addVpinBounds((yyvsp[-3].pt).x, (yyvsp[-3].pt).y, (yyvsp[-2].pt).x, (yyvsp[-2].pt).y);
          defData->orient_is_keyword = FALSE;
        }
#line 6194 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 417:
#line 2378 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       {defData->dumb_mode = 1; defData->no_num = 1;}
#line 6200 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 418:
#line 2379 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addVpin((yyvsp[0].string)); }
#line 6206 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 420:
#line 2382 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  {defData->dumb_mode=1;}
#line 6212 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 421:
#line 2383 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addVpinLayer((yyvsp[0].string)); }
#line 6218 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 423:
#line 2387 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { if (defData->callbacks->NetCbk) defData->Net.addVpinLoc((yyvsp[-2].string), (yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].integer)); }
#line 6224 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 424:
#line 2390 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"PLACED"; }
#line 6230 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 425:
#line 2392 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"FIXED"; }
#line 6236 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 426:
#line 2394 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"COVER"; }
#line 6242 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 427:
#line 2397 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"FIXED"; defData->dumb_mode = 1; }
#line 6248 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 428:
#line 2399 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"COVER"; defData->dumb_mode = 1; }
#line 6254 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 429:
#line 2401 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.string) = (char*)"ROUTED"; defData->dumb_mode = 1; }
#line 6260 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 430:
#line 2405 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->NetCbk)
          defData->pathIsDone(defData->shield, 0, defData->netOsnet, &defData->needNPCbk);
      }
#line 6268 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 431:
#line 2409 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 6274 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 432:
#line 2411 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                { defData->dumb_mode = 1; }
#line 6280 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 433:
#line 2412 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->NetCbk)
          defData->pathIsDone(defData->shield, 0, defData->netOsnet, &defData->needNPCbk);
      }
#line 6288 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 434:
#line 2417 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6311 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 435:
#line 2436 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = DEF_MAX_INT; defData->by_is_keyword = TRUE; defData->do_is_keyword = TRUE;
/*
       dumb_mode = 1; by_is_keyword = TRUE; do_is_keyword = TRUE;
*/
        defData->new_is_keyword = TRUE; defData->step_is_keyword = TRUE; 
        defData->orient_is_keyword = TRUE; defData->virtual_is_keyword = TRUE;
        defData->mask_is_keyword = TRUE, defData->rect_is_keyword = TRUE;  }
#line 6323 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 436:
#line 2446 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0;   defData->virtual_is_keyword = FALSE; defData->mask_is_keyword = FALSE,
       defData->rect_is_keyword = FALSE; }
#line 6330 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 437:
#line 2451 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6349 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 438:
#line 2468 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6368 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 441:
#line 2491 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6383 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 442:
#line 2502 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6401 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 443:
#line 2516 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
            defData->PathObj.addVia((yyvsp[-1].string));
            defData->PathObj.addViaRotation((yyvsp[0].integer));
        }
      }
#line 6412 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 444:
#line 2523 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6427 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 445:
#line 2534 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6455 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 446:
#line 2558 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6493 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 447:
#line 2592 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6531 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 448:
#line 2626 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6560 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 451:
#line 2652 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                          { defData->dumb_mode = 6; }
#line 6566 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 452:
#line 2653 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
    {
      if (defData->validateMaskInput((int)(yyvsp[-8].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          defData->PathObj.addMask((yyvsp[-8].dval));
          defData->PathObj.addViaRect((yyvsp[-4].dval), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval));
        }
      }
    }
#line 6580 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 453:
#line 2663 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
    {
       if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          defData->PathObj.addMask((yyvsp[0].dval)); 
        }
       }  
    }
#line 6593 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 455:
#line 2673 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
    {
       // reset defData->dumb_mode to 1 just incase the next token is a via of the path
        // 2/5/2004 - pcr 686781
        defData->dumb_mode = DEF_MAX_INT; defData->by_is_keyword = TRUE; defData->do_is_keyword = TRUE;
        defData->new_is_keyword = TRUE; defData->step_is_keyword = TRUE;
        defData->orient_is_keyword = TRUE;
    }
#line 6605 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 456:
#line 2684 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(ROUND((yyvsp[-2].dval)), ROUND((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-2].dval);
        defData->save_y = (yyvsp[-1].dval); 
      }
#line 6617 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 457:
#line 2692 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(ROUND(defData->save_x), ROUND((yyvsp[-1].dval))); 
        defData->save_y = (yyvsp[-1].dval);
      }
#line 6628 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 458:
#line 2699 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(ROUND((yyvsp[-2].dval)), ROUND(defData->save_y)); 
        defData->save_x = (yyvsp[-2].dval);
      }
#line 6639 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 459:
#line 2706 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addPoint(ROUND(defData->save_x), ROUND(defData->save_y)); 
      }
#line 6649 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 460:
#line 2712 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
            (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(ROUND((yyvsp[-3].dval)), ROUND((yyvsp[-2].dval)), ROUND((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-3].dval);
        defData->save_y = (yyvsp[-2].dval);
      }
#line 6661 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 461:
#line 2720 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(ROUND(defData->save_x), ROUND((yyvsp[-2].dval)),
          ROUND((yyvsp[-1].dval))); 
        defData->save_y = (yyvsp[-2].dval);
      }
#line 6673 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 462:
#line 2728 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(ROUND((yyvsp[-3].dval)), ROUND(defData->save_y),
          ROUND((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-3].dval);
      }
#line 6685 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 463:
#line 2736 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addFlushPoint(ROUND(defData->save_x), ROUND(defData->save_y),
          ROUND((yyvsp[-1].dval))); 
      }
#line 6696 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 464:
#line 2745 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(ROUND((yyvsp[-2].dval)), ROUND((yyvsp[-1].dval))); 
        defData->save_x = (yyvsp[-2].dval);
        defData->save_y = (yyvsp[-1].dval);
      }
#line 6708 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 465:
#line 2753 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(ROUND(defData->save_x), ROUND((yyvsp[-1].dval))); 
        defData->save_y = (yyvsp[-1].dval);
      }
#line 6719 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 466:
#line 2760 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(ROUND((yyvsp[-2].dval)), ROUND(defData->save_y)); 
        defData->save_x = (yyvsp[-2].dval);
      }
#line 6730 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 467:
#line 2767 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addVirtualPoint(ROUND(defData->save_x), ROUND(defData->save_y));
      }
#line 6740 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 468:
#line 2775 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
    {
        if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2)))) {
          defData->PathObj.addViaRect((yyvsp[-4].dval), (yyvsp[-3].dval), (yyvsp[-2].dval), (yyvsp[-1].dval)); 
        }    
    }
#line 6751 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 473:
#line 2791 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.setTaper(); }
#line 6759 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 474:
#line 2794 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1; }
#line 6765 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 475:
#line 2795 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addTaperRule((yyvsp[0].string)); }
#line 6773 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 476:
#line 2800 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6796 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 479:
#line 2825 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && ((defData->callbacks->NetCbk && (defData->netOsnet==1)) ||
          (defData->callbacks->SNetCbk && (defData->netOsnet==2))))
          defData->PathObj.addShape((yyvsp[0].string)); }
#line 6804 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 480:
#line 2829 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6827 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 481:
#line 2849 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { 
            CALLBACK(defData->callbacks->NetEndCbk, defrNetEndCbkType, 0);
            defData->netOsnet = 0;
          }
#line 6836 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 482:
#line 2855 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"RING"; }
#line 6842 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 483:
#line 2857 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"STRIPE"; }
#line 6848 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 484:
#line 2859 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"FOLLOWPIN"; }
#line 6854 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 485:
#line 2861 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"IOWIRE"; }
#line 6860 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 486:
#line 2863 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"COREWIRE"; }
#line 6866 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 487:
#line 2865 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"BLOCKWIRE"; }
#line 6872 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 488:
#line 2867 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"FILLWIRE"; }
#line 6878 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 489:
#line 2869 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6898 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 490:
#line 2885 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"DRCFILL"; }
#line 6904 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 491:
#line 2887 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"BLOCKAGEWIRE"; }
#line 6910 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 492:
#line 2889 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"PADRING"; }
#line 6916 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 493:
#line 2891 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"BLOCKRING"; }
#line 6922 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 497:
#line 2901 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { CALLBACK(defData->callbacks->SNetCbk, defrSNetCbkType, &defData->Net); }
#line 6928 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 504:
#line 2912 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 6945 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 505:
#line 2925 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
            if (defData->callbacks->SNetCbk) defData->Net.addWire((yyvsp[0].string), NULL);
            }
#line 6953 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 506:
#line 2929 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
            // 7/17/2003 - Fix for pcr 604848, add a callback for each wire
            if (defData->callbacks->SNetWireCbk) {
               CALLBACK(defData->callbacks->SNetWireCbk, defrSNetWireCbkType, &defData->Net);
               defData->Net.freeWire();
            }
            defData->by_is_keyword = FALSE;
            defData->do_is_keyword = FALSE;
            defData->new_is_keyword = FALSE;
            defData->step_is_keyword = FALSE;
            defData->orient_is_keyword = FALSE;
            defData->virtual_is_keyword = FALSE;
            defData->mask_is_keyword = FALSE;
            defData->rect_is_keyword = FALSE;
            defData->needSNPCbk = 0;
            }
#line 6974 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 507:
#line 2946 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 6980 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 508:
#line 2947 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { defData->shieldName = (yyvsp[0].string); 
              defData->specialWire_routeStatus = (char*)"SHIELD";
              defData->specialWire_routeStatusName = (yyvsp[0].string); 
            }
#line 6989 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 510:
#line 2954 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {  
            defData->specialWire_shapeType = (yyvsp[0].string);
          }
#line 6997 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 511:
#line 2958 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          {
            if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->sNetWarnings, defData->settings->SNetWarnings)) {
                defData->specialWire_mask = (yyvsp[0].dval);
            }     
          }
#line 7007 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 512:
#line 2963 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 7013 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 513:
#line 2964 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7034 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 514:
#line 2981 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7061 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 515:
#line 3004 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                     { defData->dumb_mode = 1; }
#line 7067 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 516:
#line 3005 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7104 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 517:
#line 3037 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    { defData->dumb_mode = 1; }
#line 7110 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 518:
#line 3038 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7129 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 519:
#line 3053 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7150 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 520:
#line 3071 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setSource((yyvsp[0].string)); }
#line 7156 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 521:
#line 3074 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setFixedbump(); }
#line 7162 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 522:
#line 3077 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setFrequency((yyvsp[0].dval)); }
#line 7168 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 523:
#line 3079 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode = 1; defData->no_num = 1;}
#line 7174 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 524:
#line 3080 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setOriginal((yyvsp[0].string)); }
#line 7180 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 525:
#line 3083 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setPattern((yyvsp[0].string)); }
#line 7186 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 526:
#line 3086 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setWeight(ROUND((yyvsp[0].dval))); }
#line 7192 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 527:
#line 3089 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { 
              // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
              if (defData->VersionNum < 5.5) {
                 if (defData->callbacks->SNetCbk) {
                   defData->Net.setCap((yyvsp[0].dval));
                 }
              } else  {
                 defData->defWarning(7024, "The ESTCAP statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
              }
            }
#line 7207 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 528:
#line 3101 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setUse((yyvsp[0].string)); }
#line 7213 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 529:
#line 3104 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { if (defData->callbacks->SNetCbk) defData->Net.setStyle((int)(yyvsp[0].dval)); }
#line 7219 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 530:
#line 3106 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode = DEF_MAX_INT; }
#line 7225 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 531:
#line 3108 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { defData->dumb_mode = 0; }
#line 7231 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 532:
#line 3111 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
          { CALLBACK(defData->callbacks->NetExtCbk, defrNetExtCbkType, &defData->History_text[0]); }
#line 7237 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 533:
#line 3114 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
        { (yyval.integer) = 0; }
#line 7243 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 534:
#line 3115 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 0;}
#line 7249 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 535:
#line 3116 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 1;}
#line 7255 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 536:
#line 3117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 2;}
#line 7261 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 537:
#line 3118 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 3;}
#line 7267 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 538:
#line 3119 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 4;}
#line 7273 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 539:
#line 3120 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 5;}
#line 7279 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 540:
#line 3121 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 6;}
#line 7285 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 541:
#line 3122 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {(yyval.integer) = 7;}
#line 7291 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 542:
#line 3125 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7309 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 543:
#line 3139 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { // since the parser still supports 5.3 and earlier, 
              // can't just move SHIELD in net_type 
              if (defData->VersionNum < 5.4) { // PCR 445209 
                if (defData->callbacks->SNetCbk) defData->Net.addShield(defData->shieldName);
                defData->by_is_keyword = FALSE;
                defData->do_is_keyword = FALSE;
                defData->new_is_keyword = FALSE;
                defData->step_is_keyword = FALSE;
                defData->orient_is_keyword = FALSE;
                defData->virtual_is_keyword = FALSE;
                defData->mask_is_keyword = FALSE;
                defData->rect_is_keyword = FALSE;
                defData->specialWire_routeStatus = (char*)"ROUTED";
                defData->specialWire_routeStatusName = (char*)"";
                defData->shield = TRUE;   // save the path info in the defData->shield paths 
              } else
                if (defData->callbacks->SNetCbk) defData->Net.addWire("SHIELD", defData->shieldName);
                defData->specialWire_routeStatus = (char*)"ROUTED";
                defData->specialWire_routeStatusName = (char*)"";
            }
#line 7334 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 544:
#line 3160 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
                defData->shield = FALSE;
                defData->by_is_keyword = FALSE;
                defData->do_is_keyword = FALSE;
                defData->new_is_keyword = FALSE;
                defData->step_is_keyword = FALSE;
                defData->nondef_is_keyword = FALSE;
                defData->mustjoin_is_keyword = FALSE;
                defData->orient_is_keyword = FALSE;
                defData->virtual_is_keyword = FALSE;
                defData->mask_is_keyword = FALSE;
                defData->rect_is_keyword = FALSE;
              } else {
                defData->by_is_keyword = FALSE;
                defData->do_is_keyword = FALSE;
                defData->new_is_keyword = FALSE;
                defData->step_is_keyword = FALSE;
                defData->orient_is_keyword = FALSE;
                defData->virtual_is_keyword = FALSE;
                defData->mask_is_keyword = FALSE;
                defData->rect_is_keyword = FALSE;
              }
              defData->needSNPCbk = 0;
            }
#line 7372 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 545:
#line 3194 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        { defData->dumb_mode = 1; }
#line 7378 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 546:
#line 3195 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
              if (defData->VersionNum < 5.5) {
                  if (defData->callbacks->SNetCbk) {
                      defData->Net.setWidth((yyvsp[-1].string), (yyvsp[0].dval));
                  }
              } else {
                 defData->defWarning(7026, "The WIDTH statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
              }
            }
#line 7393 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 547:
#line 3206 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                             { defData->dumb_mode = 1; defData->no_num = 1; }
#line 7399 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 548:
#line 3207 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7420 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 549:
#line 3224 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                            { defData->dumb_mode = 1; }
#line 7426 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 550:
#line 3225 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) defData->Net.setSpacing((yyvsp[-1].string),(yyvsp[0].dval));
            }
#line 7434 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 551:
#line 3229 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
            }
#line 7441 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 554:
#line 3237 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7457 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 555:
#line 3249 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) {
                char propTp;
                propTp = defData->session->SNetProp.propType((yyvsp[-1].string));
                CHKPROPTYPE(propTp, (yyvsp[-1].string), "SPECIAL NET");
                defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
              }
            }
#line 7470 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 556:
#line 3258 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) {
                char propTp;
                propTp = defData->session->SNetProp.propType((yyvsp[-1].string));
                CHKPROPTYPE(propTp, (yyvsp[-1].string), "SPECIAL NET");
                defData->Net.addProp((yyvsp[-1].string), (yyvsp[0].string), propTp);
              }
            }
#line 7483 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 558:
#line 3269 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            {
              if (defData->callbacks->SNetCbk) defData->Net.setRange((yyvsp[-1].dval),(yyvsp[0].dval));
            }
#line 7491 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 560:
#line 3275 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { defData->Prop.setRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 7497 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 561:
#line 3278 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"BALANCED"; }
#line 7503 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 562:
#line 3280 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"STEINER"; }
#line 7509 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 563:
#line 3282 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"TRUNK"; }
#line 7515 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 564:
#line 3284 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
            { (yyval.string) = (char*)"WIREDLOGIC"; }
#line 7521 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 565:
#line 3288 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7542 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 566:
#line 3305 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 7548 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 567:
#line 3307 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                 { defData->dumb_mode = 1; }
#line 7554 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 568:
#line 3308 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7575 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 569:
#line 3326 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->SNetCbk)
           defData->PathObj.addLayer((yyvsp[0].string));
        defData->dumb_mode = 0; defData->no_num = 0;
      }
#line 7584 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 570:
#line 3334 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = DEF_MAX_INT; defData->by_is_keyword = TRUE; defData->do_is_keyword = TRUE;
        defData->new_is_keyword = TRUE; defData->step_is_keyword = TRUE;
         defData->orient_is_keyword = TRUE; defData->rect_is_keyword = TRUE, defData->mask_is_keyword = TRUE; 
         defData->virtual_is_keyword = TRUE;  }
#line 7593 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 571:
#line 3340 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->rect_is_keyword = FALSE, defData->mask_is_keyword = FALSE, defData->virtual_is_keyword = FALSE; }
#line 7599 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 572:
#line 3343 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->NeedPathData && defData->callbacks->SNetCbk)
          defData->PathObj.addWidth(ROUND((yyvsp[0].dval)));
      }
#line 7607 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 573:
#line 3348 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->SNetStartCbk)
          CALLBACK(defData->callbacks->SNetStartCbk, defrSNetStartCbkType, ROUND((yyvsp[-1].dval)));
        defData->netOsnet = 2;
      }
#line 7617 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 574:
#line 3355 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->SNetEndCbk)
          CALLBACK(defData->callbacks->SNetEndCbk, defrSNetEndCbkType, 0);
        defData->netOsnet = 0;
      }
#line 7627 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 576:
#line 3365 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->GroupsStartCbk)
           CALLBACK(defData->callbacks->GroupsStartCbk, defrGroupsStartCbkType, ROUND((yyvsp[-1].dval)));
      }
#line 7636 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 579:
#line 3375 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk)
           CALLBACK(defData->callbacks->GroupCbk, defrGroupCbkType, &defData->Group);
      }
#line 7645 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 580:
#line 3380 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                 { defData->dumb_mode = 1; defData->no_num = 1; }
#line 7651 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 581:
#line 3381 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        defData->dumb_mode = DEF_MAX_INT;
        defData->no_num = DEF_MAX_INT;
        /* dumb_mode is automatically turned off at the first
         * + in the options or at the ; at the end of the group */
        if (defData->callbacks->GroupCbk) defData->Group.setup((yyvsp[0].string));
        if (defData->callbacks->GroupNameCbk)
           CALLBACK(defData->callbacks->GroupNameCbk, defrGroupNameCbkType, (yyvsp[0].string));
      }
#line 7665 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 583:
#line 3393 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {  }
#line 7671 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 584:
#line 3396 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        // if (defData->callbacks->GroupCbk) defData->Group.addMember($1); 
        if (defData->callbacks->GroupMemberCbk)
          CALLBACK(defData->callbacks->GroupMemberCbk, defrGroupMemberCbkType, (yyvsp[0].string));
      }
#line 7681 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 587:
#line 3407 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 7687 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 588:
#line 3408 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                           { defData->dumb_mode = DEF_MAX_INT; }
#line 7693 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 589:
#line 3410 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 7699 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 590:
#line 3411 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         { defData->dumb_mode = 1;  defData->no_num = 1; }
#line 7705 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 591:
#line 3412 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 7711 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 592:
#line 3414 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->GroupMemberCbk)
          CALLBACK(defData->callbacks->GroupExtCbk, defrGroupExtCbkType, &defData->History_text[0]);
      }
#line 7720 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 593:
#line 3420 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
          if (defData->callbacks->GroupCbk)
            defData->Group.addRegionRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
        }
        else
          defData->defWarning(7027, "The GROUP REGION pt pt statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
      }
#line 7734 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 594:
#line 3430 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->GroupCbk)
          defData->Group.setRegionName((yyvsp[0].string));
      }
#line 7742 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 597:
#line 3439 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7757 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 598:
#line 3450 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk) {
          char propTp;
          propTp = defData->session->GroupProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "GROUP");
          defData->Group.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 7770 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 599:
#line 3459 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->GroupCbk) {
          char propTp;
          propTp = defData->session->GroupProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "GROUP");
          defData->Group.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 7783 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 601:
#line 3470 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 7789 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 602:
#line 3473 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
            if (defData->callbacks->GroupCbk) {
              defData->Group.setMaxX(ROUND((yyvsp[0].dval)));
            }
        } else {
            defData->defWarning(7028, "The GROUP SOFT MAXX statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 7804 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 603:
#line 3484 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
            if (defData->callbacks->GroupCbk) {
              defData->Group.setMaxY(ROUND((yyvsp[0].dval)));
            }
        } else {
            defData->defWarning(7029, "The GROUP SOFT MAXY statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 7819 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 604:
#line 3495 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        // 11/12/2002 - this is obsolete in 5.5, & will be ignored 
        if (defData->VersionNum < 5.5) {
            if (defData->callbacks->GroupCbk) {
                defData->Group.setPerim(ROUND((yyvsp[0].dval)));
            }
        } else {
          defData->defWarning(7030, "The GROUP SOFT MAXHALFPERIMETER statement is obsolete in version 5.5 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 7834 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 605:
#line 3507 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->GroupsEndCbk)
          CALLBACK(defData->callbacks->GroupsEndCbk, defrGroupsEndCbkType, 0);
      }
#line 7843 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 608:
#line 3521 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if ((defData->VersionNum < 5.4) && (defData->callbacks->AssertionsStartCbk)) {
          CALLBACK(defData->callbacks->AssertionsStartCbk, defrAssertionsStartCbkType,
                   ROUND((yyvsp[-1].dval)));
        } else {
          if (defData->callbacks->AssertionCbk)
            if (defData->assertionWarnings++ < defData->settings->AssertionWarnings)
              defData->defWarning(7031, "The ASSERTIONS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
        }
        if (defData->callbacks->AssertionCbk)
          defData->Assertion.setAssertionMode();
      }
#line 7860 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 609:
#line 3535 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if ((defData->VersionNum < 5.4) && (defData->callbacks->ConstraintsStartCbk)) {
          CALLBACK(defData->callbacks->ConstraintsStartCbk, defrConstraintsStartCbkType,
                   ROUND((yyvsp[-1].dval)));
        } else {
          if (defData->callbacks->ConstraintCbk)
            if (defData->constraintWarnings++ < defData->settings->ConstraintWarnings)
              defData->defWarning(7032, "The CONSTRAINTS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
        }
        if (defData->callbacks->ConstraintCbk)
          defData->Assertion.setConstraintMode();
      }
#line 7877 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 613:
#line 3554 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if ((defData->VersionNum < 5.4) && (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)) {
          if (defData->Assertion.isConstraint()) 
            CALLBACK(defData->callbacks->ConstraintCbk, defrConstraintCbkType, &defData->Assertion);
          if (defData->Assertion.isAssertion()) 
            CALLBACK(defData->callbacks->AssertionCbk, defrAssertionCbkType, &defData->Assertion);
        }
      }
#line 7890 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 614:
#line 3564 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 7906 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 615:
#line 3576 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               { defData->dumb_mode = 1; defData->no_num = 1; }
#line 7912 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 616:
#line 3577 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
         if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.addNet((yyvsp[0].string));
      }
#line 7921 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 617:
#line 3581 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {defData->dumb_mode = 4; defData->no_num = 4;}
#line 7927 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 618:
#line 3582 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
         if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.addPath((yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].string), (yyvsp[0].string));
      }
#line 7936 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 619:
#line 3587 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.setSum();
      }
#line 7945 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 620:
#line 3592 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
           defData->Assertion.setDiff();
      }
#line 7954 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 622:
#line 3599 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 7960 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 624:
#line 3602 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 7966 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 625:
#line 3604 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setWiredlogic((yyvsp[-4].string), (yyvsp[-1].dval));
      }
#line 7975 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 626:
#line 3611 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)""; }
#line 7981 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 627:
#line 3613 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"+"; }
#line 7987 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 630:
#line 3620 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setRiseMin((yyvsp[0].dval));
      }
#line 7996 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 631:
#line 3625 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setRiseMax((yyvsp[0].dval));
      }
#line 8005 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 632:
#line 3630 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setFallMin((yyvsp[0].dval));
      }
#line 8014 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 633:
#line 3635 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ConstraintCbk || defData->callbacks->AssertionCbk)
          defData->Assertion.setFallMax((yyvsp[0].dval));
      }
#line 8023 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 634:
#line 3641 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if ((defData->VersionNum < 5.4) && defData->callbacks->ConstraintsEndCbk) {
          CALLBACK(defData->callbacks->ConstraintsEndCbk, defrConstraintsEndCbkType, 0);
        } else {
          if (defData->callbacks->ConstraintsEndCbk) {
            if (defData->constraintWarnings++ < defData->settings->ConstraintWarnings)
              defData->defWarning(7032, "The CONSTRAINTS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
      }
#line 8037 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 635:
#line 3652 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if ((defData->VersionNum < 5.4) && defData->callbacks->AssertionsEndCbk) {
          CALLBACK(defData->callbacks->AssertionsEndCbk, defrAssertionsEndCbkType, 0);
        } else {
          if (defData->callbacks->AssertionsEndCbk) {
            if (defData->assertionWarnings++ < defData->settings->AssertionWarnings)
              defData->defWarning(7031, "The ASSERTIONS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
          }
        }
      }
#line 8051 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 637:
#line 3666 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->ScanchainsStartCbk)
          CALLBACK(defData->callbacks->ScanchainsStartCbk, defrScanchainsStartCbkType,
                   ROUND((yyvsp[-1].dval)));
      }
#line 8060 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 639:
#line 3673 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {}
#line 8066 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 640:
#line 3676 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->ScanchainCbk)
          CALLBACK(defData->callbacks->ScanchainCbk, defrScanchainCbkType, &defData->Scanchain);
      }
#line 8075 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 641:
#line 3681 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                {defData->dumb_mode = 1; defData->no_num = 1;}
#line 8081 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 642:
#line 3682 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setName((yyvsp[0].string));
        defData->bit_is_keyword = TRUE;
      }
#line 8091 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 645:
#line 3694 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)""; }
#line 8097 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 646:
#line 3696 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (yyvsp[0].string); }
#line 8103 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 647:
#line 3698 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode = 2; defData->no_num = 2;}
#line 8109 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 648:
#line 3699 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setStart((yyvsp[-1].string), (yyvsp[0].string));
      }
#line 8117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 649:
#line 3702 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8123 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 650:
#line 3703 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->no_num = 0; }
#line 8129 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 651:
#line 3705 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
         defData->dumb_mode = 1;
         defData->no_num = 1;
         if (defData->callbacks->ScanchainCbk)
           defData->Scanchain.addOrderedList();
      }
#line 8140 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 652:
#line 3712 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->no_num = 0; }
#line 8146 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 653:
#line 3713 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                   {defData->dumb_mode = 2; defData->no_num = 2; }
#line 8152 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 654:
#line 3714 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.setStop((yyvsp[-1].string), (yyvsp[0].string));
      }
#line 8160 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 655:
#line 3717 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                             { defData->dumb_mode = 10; defData->no_num = 10; }
#line 8166 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 656:
#line 3718 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0;  defData->no_num = 0; }
#line 8172 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 657:
#line 3719 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8178 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 658:
#line 3721 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8199 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 659:
#line 3738 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ScanChainExtCbk)
          CALLBACK(defData->callbacks->ScanChainExtCbk, defrScanChainExtCbkType, &defData->History_text[0]);
      }
#line 8208 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 660:
#line 3744 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 8214 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 661:
#line 3746 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->ScanchainCbk) {
          if (strcmp((yyvsp[-2].string), "IN") == 0 || strcmp((yyvsp[-2].string), "in") == 0)
            defData->Scanchain.setCommonIn((yyvsp[-1].string));
          else if (strcmp((yyvsp[-2].string), "OUT") == 0 || strcmp((yyvsp[-2].string), "out") == 0)
            defData->Scanchain.setCommonOut((yyvsp[-1].string));
        }
      }
#line 8227 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 662:
#line 3755 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8244 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 665:
#line 3773 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        defData->dumb_mode = 1000;
        defData->no_num = 1000;
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.addFloatingInst((yyvsp[0].string));
      }
#line 8255 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 666:
#line 3780 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8261 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 667:
#line 3783 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 8267 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 668:
#line 3785 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8284 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 669:
#line 3798 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8309 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 670:
#line 3820 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8342 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 673:
#line 3854 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 1000; defData->no_num = 1000; 
        if (defData->callbacks->ScanchainCbk)
          defData->Scanchain.addOrderedInst((yyvsp[0].string));
      }
#line 8351 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 674:
#line 3859 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8357 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 675:
#line 3862 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 8363 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 676:
#line 3864 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8380 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 677:
#line 3877 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8405 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 678:
#line 3899 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 8438 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 679:
#line 3929 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.integer) = -1; }
#line 8444 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 680:
#line 3931 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.integer) = ROUND((yyvsp[0].dval)); }
#line 8450 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 681:
#line 3934 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->ScanchainsEndCbk)
          CALLBACK(defData->callbacks->ScanchainsEndCbk, defrScanchainsEndCbkType, 0);
        defData->bit_is_keyword = FALSE;
        defData->dumb_mode = 0; defData->no_num = 0;
      }
#line 8461 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 683:
#line 3946 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.4 && defData->callbacks->IOTimingsStartCbk) {
          CALLBACK(defData->callbacks->IOTimingsStartCbk, defrIOTimingsStartCbkType, ROUND((yyvsp[-1].dval)));
        } else {
          if (defData->callbacks->IOTimingsStartCbk)
            if (defData->iOTimingWarnings++ < defData->settings->IOTimingWarnings)
              defData->defWarning(7035, "The IOTIMINGS statement is obsolete in version 5.4 and later.\nThe DEF parser will ignore this statement.");
        }
      }
#line 8475 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 685:
#line 3958 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 8481 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 686:
#line 3961 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->VersionNum < 5.4 && defData->callbacks->IOTimingCbk)
          CALLBACK(defData->callbacks->IOTimingCbk, defrIOTimingCbkType, &defData->IOTiming);
      }
#line 8490 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 687:
#line 3966 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        {defData->dumb_mode = 2; defData->no_num = 2; }
#line 8496 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 688:
#line 3967 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk)
          defData->IOTiming.setName((yyvsp[-2].string), (yyvsp[-1].string));
      }
#line 8505 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 691:
#line 3978 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setVariable((yyvsp[-3].string), (yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 8514 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 692:
#line 3983 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setSlewRate((yyvsp[-3].string), (yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 8523 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 693:
#line 3988 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setCapacitance((yyvsp[0].dval));
      }
#line 8532 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 694:
#line 3992 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        {defData->dumb_mode = 1; defData->no_num = 1; }
#line 8538 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 695:
#line 3993 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setDriveCell((yyvsp[0].string));
      }
#line 8547 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 697:
#line 4002 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.4 && defData->callbacks->IoTimingsExtCbk)
          CALLBACK(defData->callbacks->IoTimingsExtCbk, defrIoTimingsExtCbkType, &defData->History_text[0]);
      }
#line 8556 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 698:
#line 4008 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
              {defData->dumb_mode = 1; defData->no_num = 1; }
#line 8562 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 699:
#line 4009 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk) 
          defData->IOTiming.setTo((yyvsp[0].string));
      }
#line 8571 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 702:
#line 4016 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  {defData->dumb_mode = 1; defData->no_num = 1; }
#line 8577 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 703:
#line 4017 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk)
          defData->IOTiming.setFrom((yyvsp[0].string));
      }
#line 8586 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 705:
#line 4024 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->IOTimingCbk)
          defData->IOTiming.setParallel((yyvsp[0].dval));
      }
#line 8595 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 706:
#line 4029 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                 { (yyval.string) = (char*)"RISE"; }
#line 8601 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 707:
#line 4029 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                                  { (yyval.string) = (char*)"FALL"; }
#line 8607 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 708:
#line 4032 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->VersionNum < 5.4 && defData->callbacks->IOTimingsEndCbk)
          CALLBACK(defData->callbacks->IOTimingsEndCbk, defrIOTimingsEndCbkType, 0);
      }
#line 8616 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 709:
#line 4038 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->FPCEndCbk)
          CALLBACK(defData->callbacks->FPCEndCbk, defrFPCEndCbkType, 0);
      }
#line 8625 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 710:
#line 4044 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->FPCStartCbk)
          CALLBACK(defData->callbacks->FPCStartCbk, defrFPCStartCbkType, ROUND((yyvsp[-1].dval)));
      }
#line 8634 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 712:
#line 4051 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {}
#line 8640 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 713:
#line 4053 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
             { defData->dumb_mode = 1; defData->no_num = 1;  }
#line 8646 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 714:
#line 4054 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setName((yyvsp[-1].string), (yyvsp[0].string)); }
#line 8652 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 715:
#line 4056 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) CALLBACK(defData->callbacks->FPCCbk, defrFPCCbkType, &defData->FPC); }
#line 8658 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 716:
#line 4059 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"HORIZONTAL"; }
#line 8664 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 717:
#line 4061 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"VERTICAL"; }
#line 8670 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 718:
#line 4064 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setAlign(); }
#line 8676 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 719:
#line 4066 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setMax((yyvsp[0].dval)); }
#line 8682 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 720:
#line 4068 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setMin((yyvsp[0].dval)); }
#line 8688 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 721:
#line 4070 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setEqual((yyvsp[0].dval)); }
#line 8694 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 724:
#line 4077 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setDoingBottomLeft(); }
#line 8700 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 726:
#line 4080 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.setDoingTopRight(); }
#line 8706 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 730:
#line 4087 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode = 1; defData->no_num = 1; }
#line 8712 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 731:
#line 4088 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.addRow((yyvsp[-1].string)); }
#line 8718 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 732:
#line 4089 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       {defData->dumb_mode = 1; defData->no_num = 1; }
#line 8724 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 733:
#line 4090 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FPCCbk) defData->FPC.addComps((yyvsp[-1].string)); }
#line 8730 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 735:
#line 4097 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->TimingDisablesStartCbk)
          CALLBACK(defData->callbacks->TimingDisablesStartCbk, defrTimingDisablesStartCbkType,
                   ROUND((yyvsp[-1].dval)));
      }
#line 8740 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 737:
#line 4105 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {}
#line 8746 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 738:
#line 4107 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                   { defData->dumb_mode = 2; defData->no_num = 2;  }
#line 8752 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 739:
#line 4108 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       { defData->dumb_mode = 2; defData->no_num = 2;  }
#line 8758 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 740:
#line 4109 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk) {
          defData->TimingDisable.setFromTo((yyvsp[-6].string), (yyvsp[-5].string), (yyvsp[-2].string), (yyvsp[-1].string));
          CALLBACK(defData->callbacks->TimingDisableCbk, defrTimingDisableCbkType,
                &defData->TimingDisable);
        }
      }
#line 8770 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 741:
#line 4116 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      {defData->dumb_mode = 2; defData->no_num = 2; }
#line 8776 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 742:
#line 4117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk) {
          defData->TimingDisable.setThru((yyvsp[-2].string), (yyvsp[-1].string));
          CALLBACK(defData->callbacks->TimingDisableCbk, defrTimingDisableCbkType,
                   &defData->TimingDisable);
        }
      }
#line 8788 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 743:
#line 4124 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    {defData->dumb_mode = 1; defData->no_num = 1;}
#line 8794 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 744:
#line 4125 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk) {
          defData->TimingDisable.setMacro((yyvsp[-2].string));
          CALLBACK(defData->callbacks->TimingDisableCbk, defrTimingDisableCbkType,
                &defData->TimingDisable);
        }
      }
#line 8806 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 745:
#line 4133 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->TimingDisableCbk)
          defData->TimingDisable.setReentrantPathsFlag();
      }
#line 8814 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 746:
#line 4138 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                           {defData->dumb_mode = 1; defData->no_num = 1;}
#line 8820 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 747:
#line 4139 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {defData->dumb_mode=1; defData->no_num = 1;}
#line 8826 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 748:
#line 4140 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk)
          defData->TimingDisable.setMacroFromTo((yyvsp[-3].string),(yyvsp[0].string));
      }
#line 8835 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 749:
#line 4144 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode=1; defData->no_num = 1;}
#line 8841 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 750:
#line 4145 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->TimingDisableCbk)
          defData->TimingDisable.setMacroThru((yyvsp[0].string));
      }
#line 8850 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 751:
#line 4151 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->TimingDisablesEndCbk)
          CALLBACK(defData->callbacks->TimingDisablesEndCbk, defrTimingDisablesEndCbkType, 0);
      }
#line 8859 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 753:
#line 4161 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionsStartCbk)
          CALLBACK(defData->callbacks->PartitionsStartCbk, defrPartitionsStartCbkType,
                   ROUND((yyvsp[-1].dval)));
      }
#line 8869 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 755:
#line 4169 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 8875 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 756:
#line 4172 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->PartitionCbk)
          CALLBACK(defData->callbacks->PartitionCbk, defrPartitionCbkType, &defData->Partition);
      }
#line 8884 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 757:
#line 4177 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                     { defData->dumb_mode = 1; defData->no_num = 1; }
#line 8890 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 758:
#line 4178 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setName((yyvsp[-1].string));
      }
#line 8899 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 760:
#line 4185 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.addTurnOff((yyvsp[-1].string), (yyvsp[0].string));
      }
#line 8908 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 761:
#line 4191 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)" "; }
#line 8914 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 762:
#line 4193 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"R"; }
#line 8920 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 763:
#line 4195 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"F"; }
#line 8926 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 764:
#line 4198 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)" "; }
#line 8932 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 765:
#line 4200 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"R"; }
#line 8938 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 766:
#line 4202 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"F"; }
#line 8944 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 769:
#line 4208 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                     {defData->dumb_mode=2; defData->no_num = 2;}
#line 8950 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 770:
#line 4210 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setFromClockPin((yyvsp[-3].string), (yyvsp[-2].string));
      }
#line 8959 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 771:
#line 4214 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                          {defData->dumb_mode=2; defData->no_num = 2; }
#line 8965 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 772:
#line 4216 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setFromCompPin((yyvsp[-2].string), (yyvsp[-1].string));
      }
#line 8974 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 773:
#line 4220 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        {defData->dumb_mode=1; defData->no_num = 1; }
#line 8980 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 774:
#line 4222 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setFromIOPin((yyvsp[-1].string));
      }
#line 8989 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 775:
#line 4226 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         {defData->dumb_mode=2; defData->no_num = 2; }
#line 8995 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 776:
#line 4228 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setToClockPin((yyvsp[-3].string), (yyvsp[-2].string));
      }
#line 9004 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 777:
#line 4232 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                        {defData->dumb_mode=2; defData->no_num = 2; }
#line 9010 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 778:
#line 4234 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setToCompPin((yyvsp[-2].string), (yyvsp[-1].string));
      }
#line 9019 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 779:
#line 4238 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      {defData->dumb_mode=1; defData->no_num = 2; }
#line 9025 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 780:
#line 4239 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setToIOPin((yyvsp[-1].string));
      }
#line 9034 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 781:
#line 4244 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->callbacks->PartitionsExtCbk)
          CALLBACK(defData->callbacks->PartitionsExtCbk, defrPartitionsExtCbkType,
                   &defData->History_text[0]);
      }
#line 9044 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 782:
#line 4251 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = DEF_MAX_INT; defData->no_num = DEF_MAX_INT; }
#line 9050 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 783:
#line 4252 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; defData->no_num = 0; }
#line 9056 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 785:
#line 4256 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 9062 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 786:
#line 4259 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setMin((yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 9071 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 787:
#line 4264 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PartitionCbk)
          defData->Partition.setMax((yyvsp[-1].dval), (yyvsp[0].dval));
      }
#line 9080 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 789:
#line 4271 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addPin((yyvsp[0].string)); }
#line 9086 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 792:
#line 4277 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addRiseMin((yyvsp[0].dval)); }
#line 9092 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 793:
#line 4279 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addFallMin((yyvsp[0].dval)); }
#line 9098 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 794:
#line 4281 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addRiseMax((yyvsp[0].dval)); }
#line 9104 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 795:
#line 4283 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk) defData->Partition.addFallMax((yyvsp[0].dval)); }
#line 9110 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 798:
#line 4291 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addRiseMinRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 799:
#line 4294 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addFallMinRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9124 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 800:
#line 4297 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addRiseMaxRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9131 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 801:
#line 4300 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionCbk)
          defData->Partition.addFallMaxRange((yyvsp[-1].dval), (yyvsp[0].dval)); }
#line 9138 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 802:
#line 4304 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PartitionsEndCbk)
          CALLBACK(defData->callbacks->PartitionsEndCbk, defrPartitionsEndCbkType, 0); }
#line 9145 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 804:
#line 4309 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 9151 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 805:
#line 4311 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
               {defData->dumb_mode=2; defData->no_num = 2; }
#line 9157 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 806:
#line 4313 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        // note that the defData->first T_STRING could be the keyword VPIN 
        if (defData->callbacks->NetCbk)
          defData->Subnet->addPin((yyvsp[-3].string), (yyvsp[-2].string), (yyvsp[-1].integer));
      }
#line 9167 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 807:
#line 4320 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.integer) = 0; }
#line 9173 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 808:
#line 4322 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.integer) = 1; }
#line 9179 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 811:
#line 4328 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {  
        if (defData->callbacks->NetCbk) defData->Subnet->addWire((yyvsp[0].string));
      }
#line 9187 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 812:
#line 4332 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {  
        defData->by_is_keyword = FALSE;
        defData->do_is_keyword = FALSE;
        defData->new_is_keyword = FALSE;
        defData->step_is_keyword = FALSE;
        defData->orient_is_keyword = FALSE;
        defData->needNPCbk = 0;
      }
#line 9200 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 813:
#line 4340 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9206 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 814:
#line 4341 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->NetCbk) defData->Subnet->setNonDefault((yyvsp[0].string)); }
#line 9212 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 815:
#line 4344 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"FIXED"; defData->dumb_mode = 1; }
#line 9218 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 816:
#line 4346 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"COVER"; defData->dumb_mode = 1; }
#line 9224 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 817:
#line 4348 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"ROUTED"; defData->dumb_mode = 1; }
#line 9230 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 818:
#line 4350 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { (yyval.string) = (char*)"NOSHIELD"; defData->dumb_mode = 1; }
#line 9236 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 820:
#line 4355 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PinPropStartCbk)
          CALLBACK(defData->callbacks->PinPropStartCbk, defrPinPropStartCbkType, ROUND((yyvsp[-1].dval))); }
#line 9243 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 821:
#line 4360 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 9249 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 822:
#line 4362 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { }
#line 9255 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 823:
#line 4365 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PinPropEndCbk)
          CALLBACK(defData->callbacks->PinPropEndCbk, defrPinPropEndCbkType, 0); }
#line 9262 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 826:
#line 4372 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       { defData->dumb_mode = 2; defData->no_num = 2; }
#line 9268 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 827:
#line 4373 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PinPropCbk) defData->PinProp.setName((yyvsp[-1].string), (yyvsp[0].string)); }
#line 9274 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 828:
#line 4375 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->PinPropCbk) {
          CALLBACK(defData->callbacks->PinPropCbk, defrPinPropCbkType, &defData->PinProp);
         // reset the property number
         defData->PinProp.clear();
        }
      }
#line 9285 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 831:
#line 4385 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                         { defData->dumb_mode = DEF_MAX_INT; }
#line 9291 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 832:
#line 4387 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 9297 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 835:
#line 4394 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9312 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 836:
#line 4405 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PinPropCbk) {
          char propTp;
          propTp = defData->session->CompPinProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "PINPROPERTIES");
          defData->PinProp.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 9325 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 837:
#line 4414 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->PinPropCbk) {
          char propTp;
          propTp = defData->session->CompPinProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "PINPROPERTIES");
          defData->PinProp.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 9338 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 839:
#line 4426 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->BlockageStartCbk)
          CALLBACK(defData->callbacks->BlockageStartCbk, defrBlockageStartCbkType, ROUND((yyvsp[-1].dval))); }
#line 9345 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 840:
#line 4430 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->BlockageEndCbk)
          CALLBACK(defData->callbacks->BlockageEndCbk, defrBlockageEndCbkType, 0); }
#line 9352 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 843:
#line 4439 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
          CALLBACK(defData->callbacks->BlockageCbk, defrBlockageCbkType, &defData->Blockage);
          defData->Blockage.clear();
        }
      }
#line 9363 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 844:
#line 4446 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                           { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9369 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 845:
#line 4447 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9389 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 847:
#line 4466 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9408 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 851:
#line 4487 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9438 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 852:
#line 4513 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9464 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 855:
#line 4539 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {      
        if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->blockageWarnings, defData->settings->BlockageWarnings)) {
          defData->Blockage.setMask((int)(yyvsp[0].dval));
        }
      }
#line 9474 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 856:
#line 4547 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9480 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 857:
#line 4548 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9502 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 858:
#line 4567 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9527 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 859:
#line 4588 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9552 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 860:
#line 4609 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9574 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 861:
#line 4627 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9609 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 864:
#line 4664 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9615 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 865:
#line 4665 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9637 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 866:
#line 4683 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9659 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 867:
#line 4701 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9697 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 868:
#line 4735 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9735 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 871:
#line 4775 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk)
          defData->Blockage.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
#line 9744 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 872:
#line 4780 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->BlockageCbk) {
            defData->Geometries.Reset();
        }
      }
#line 9754 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 873:
#line 4786 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9776 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 875:
#line 4808 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->SlotStartCbk)
          CALLBACK(defData->callbacks->SlotStartCbk, defrSlotStartCbkType, ROUND((yyvsp[-1].dval))); }
#line 9783 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 876:
#line 4812 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->SlotEndCbk)
          CALLBACK(defData->callbacks->SlotEndCbk, defrSlotEndCbkType, 0); }
#line 9790 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 879:
#line 4820 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->SlotCbk) {
          CALLBACK(defData->callbacks->SlotCbk, defrSlotCbkType, &defData->Slot);
          defData->Slot.clear();
        }
      }
#line 9801 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 880:
#line 4827 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9807 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 881:
#line 4828 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->SlotCbk) {
          defData->Slot.setLayer((yyvsp[0].string));
          defData->Slot.clearPoly();     // free poly, if any
        }
      }
#line 9818 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 885:
#line 4840 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->SlotCbk)
          defData->Slot.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
#line 9827 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 886:
#line 4845 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
          defData->Geometries.Reset();
      }
#line 9835 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 887:
#line 4849 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->VersionNum >= 5.6) {  // only 5.6 and beyond
          if (defData->callbacks->SlotCbk)
            defData->Slot.addPolygon(&defData->Geometries);
        }
      }
#line 9846 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 889:
#line 4860 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FillStartCbk)
          CALLBACK(defData->callbacks->FillStartCbk, defrFillStartCbkType, ROUND((yyvsp[-1].dval))); }
#line 9853 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 890:
#line 4864 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->FillEndCbk)
          CALLBACK(defData->callbacks->FillEndCbk, defrFillEndCbkType, 0); }
#line 9860 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 893:
#line 4872 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk) {
          CALLBACK(defData->callbacks->FillCbk, defrFillCbkType, &defData->Fill);
          defData->Fill.clear();
        }
      }
#line 9871 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 894:
#line 4878 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9877 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 895:
#line 4879 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk) {
          defData->Fill.setVia((yyvsp[0].string));
          defData->Fill.clearPts();
          defData->Geometries.Reset();
        }
      }
#line 9889 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 897:
#line 4888 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                       { defData->dumb_mode = 1; defData->no_num = 1; }
#line 9895 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 898:
#line 4889 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk) {
          defData->Fill.setLayer((yyvsp[0].string));
          defData->Fill.clearPoly();    // free poly, if any
        }
      }
#line 9906 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 902:
#line 4902 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->FillCbk)
          defData->Fill.addRect((yyvsp[-1].pt).x, (yyvsp[-1].pt).y, (yyvsp[0].pt).x, (yyvsp[0].pt).y);
      }
#line 9915 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 903:
#line 4907 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        defData->Geometries.Reset();
      }
#line 9923 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 904:
#line 4911 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9941 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 909:
#line 4935 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9963 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 910:
#line 4954 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
    {
        if (defData->callbacks->FillCbk) {
          defData->Fill.addPts(&defData->Geometries);
          CALLBACK(defData->callbacks->FillCbk, defrFillCbkType, &defData->Fill);
          defData->Fill.clear();
        }
    }
#line 9975 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 915:
#line 4973 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 9997 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 916:
#line 4993 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->fillWarnings, defData->settings->FillWarnings)) {
             if (defData->callbacks->FillCbk) {
                defData->Fill.setMask((int)(yyvsp[0].dval));
             }
        }
      }
#line 10009 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 917:
#line 5003 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { 
        if (defData->validateMaskInput((int)(yyvsp[0].dval), defData->fillWarnings, defData->settings->FillWarnings)) {
             if (defData->callbacks->FillCbk) {
                defData->Fill.setMask((int)(yyvsp[0].dval));
             }
        }
      }
#line 10021 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 919:
#line 5016 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
                   ROUND((yyvsp[-1].dval)));
      }
#line 10042 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 920:
#line 5034 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->NonDefaultEndCbk)
          CALLBACK(defData->callbacks->NonDefaultEndCbk, defrNonDefaultEndCbkType, 0); }
#line 10049 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 923:
#line 5041 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10055 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 924:
#line 5042 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.clear(); 
          defData->NonDefault.setName((yyvsp[0].string));
        }
      }
#line 10066 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 925:
#line 5049 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->NonDefaultCbk)
          CALLBACK(defData->callbacks->NonDefaultCbk, defrNonDefaultCbkType, &defData->NonDefault); }
#line 10073 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 928:
#line 5057 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk)
          defData->NonDefault.setHardspacing();
      }
#line 10082 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 929:
#line 5061 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                    { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10088 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 930:
#line 5063 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addLayer((yyvsp[-2].string));
          defData->NonDefault.addWidth((yyvsp[0].dval));
        }
      }
#line 10099 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 932:
#line 5070 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                  { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10105 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 933:
#line 5071 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addVia((yyvsp[0].string));
        }
      }
#line 10115 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 934:
#line 5076 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10121 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 935:
#line 5077 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addViaRule((yyvsp[0].string));
        }
      }
#line 10131 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 936:
#line 5082 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                      { defData->dumb_mode = 1; defData->no_num = 1; }
#line 10137 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 937:
#line 5083 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addMinCuts((yyvsp[-1].string), (int)(yyvsp[0].dval));
        }
      }
#line 10147 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 941:
#line 5096 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addDiagWidth((yyvsp[0].dval));
        }
      }
#line 10157 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 942:
#line 5102 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addSpacing((yyvsp[0].dval));
        }
      }
#line 10167 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 943:
#line 5108 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          defData->NonDefault.addWireExt((yyvsp[0].dval));
        }
      }
#line 10177 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 944:
#line 5115 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
                                    { defData->dumb_mode = DEF_MAX_INT;  }
#line 10183 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 945:
#line 5117 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { defData->dumb_mode = 0; }
#line 10189 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 948:
#line 5124 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
#line 10204 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 949:
#line 5135 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          char propTp;
          propTp = defData->session->NDefProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "NONDEFAULTRULE");
          defData->NonDefault.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 10217 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 950:
#line 5144 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->NonDefaultCbk) {
          char propTp;
          propTp = defData->session->NDefProp.propType((yyvsp[-1].string));
          CHKPROPTYPE(propTp, (yyvsp[-1].string), "NONDEFAULTRULE");
          defData->NonDefault.addProperty((yyvsp[-1].string), (yyvsp[0].string), propTp);
        }
      }
#line 10230 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 952:
#line 5157 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
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
          CALLBACK(defData->callbacks->StylesStartCbk, defrStylesStartCbkType, ROUND((yyvsp[-1].dval)));
      }
#line 10250 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 953:
#line 5174 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      { if (defData->callbacks->StylesEndCbk)
          CALLBACK(defData->callbacks->StylesEndCbk, defrStylesEndCbkType, 0); }
#line 10257 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 956:
#line 5182 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->callbacks->StylesCbk) defData->Styles.setStyle((int)(yyvsp[0].dval));
        defData->Geometries.Reset();
      }
#line 10266 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;

  case 957:
#line 5187 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"
      {
        if (defData->VersionNum >= 5.6) {  // only 5.6 and beyond will call the callback
          if (defData->callbacks->StylesCbk) {
            defData->Styles.setPolygon(&defData->Geometries);
            CALLBACK(defData->callbacks->StylesCbk, defrStylesCbkType, &defData->Styles);
          }
        }
      }
#line 10279 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"
    break;


#line 10283 "/home/stephano/OpenROAD/src/OpenDB/src/def/def_parser.cpp"

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
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

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
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (defData, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (defData, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
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

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
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
                  yystos[yystate], yyvsp, defData);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (defData, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
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
                  yystos[+*yyssp], yyvsp, defData);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 5197 "/home/stephano/OpenROAD/src/OpenDB/src/def/def/def.y"


END_LEFDEF_PARSER_NAMESPACE
