#pragma once 

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <endian.h>

namespace odb {

enum RecordType
{
  HEADER = 0,     BGNLIB = 1,     LIBNAME = 2,      UNITS = 3, 
  ENDLIB = 4,     BGNSTR = 5,     STRNAME = 6 ,     ENDSTR = 7,
  BOUNDARY = 8,   PATH = 9,       SREF = 10,        AREF = 11,
  TEXT = 12,      LAYER = 13,     DATATYPE = 14,    WIDTH = 15,
  XY = 16,        ENDEL = 17,     SNAME = 18,       COLROW = 19,
  TEXTNODE = 20,  NODE = 21,      TEXTTYPE = 22,    PRESENTATION = 23,
  SPACING = 24,   STRING = 25,    STRANS = 26,      MAG = 27,
  ANGLE = 28,     UINTEGER = 29,  USTRING = 30,     REFLIBS = 31,
  FONTS = 32,     PATHTYPE = 33,  GENERATIONS = 34, ATTRTABLE = 35,
  STYPTABLE = 36, STRTYPE = 37,   ELFLAGS = 38,     ELKEY = 39,
  LINKTYPE = 40,  LINKKEYS = 41,  NODETYPE = 42,    PROPATTR = 43,
  PROPVALUE = 44, BOX = 45,       BOXTYPE = 46,     PLEX = 47,
  BGNEXTN = 48,   ENDEXTN = 49,   TAPENUM = 50,     TAPECODE = 51,
  STRCLASS = 52,  RESERVED = 53,  FORMAT = 54,      MASK = 55,
  ENDMASKS = 56,  LIBDIRSIZE = 57, SRFNAME = 58,    LIBSECUR = 59,
  INVALID_RT = 60
};

RecordType toRecordType(uint8_t recordType);

uint8_t fromRecordType(RecordType recordType);

static const char* recordNames[RecordType::INVALID_RT] = {
  "HEADER",       "BGNLIB",     "LIBNAME",     "UNITS",  
  "ENDLIB",       "BGNSTR",     "STRNAME",     "ENDSTR",
  "BOUNDARY",     "PATH",       "SREF",        "AREF",
  "TEXT",         "LAYER",      "DATATYPE",    "WIDTH",
  "XY",           "ENDEL",      "SNAME",       "COLROW",
  "TEXTNODE",     "NODE",       "TEXTTYPE",    "PRESENTATION",
  "SPACING",      "STRING",     "STRANS",      "MAG",
  "ANGLE",        "UINTEGER",   "USTRING",     "REFLIBS",
  "FONTS",        "PATHTYPE",   "GENERATIONS", "ATTRTABLE",
  "STYPTABLE",    "STRTYPE",    "ELFLAGS",     "ELKEY",
  "LINKTYPE",     "LINKKEYS",   "NODETYPE",    "PROPATTR",
  "PROPVALUE",    "BOX",        "BOXTYPE",     "PLEX",
  "BGNEXTN",      "ENDEXTN",    "TAPENUM",     "TAPECODE",
  "STRCLASS",     "RESERVED",   "FORMAT",      "MASK",
  "ENDMASKS",     "LIBDIRSIZE", "SRFNAME",     "LIBSECUR"
};

enum DataType {
  NO_DATA = 0, BIT_ARRAY, INT_2, INT_4, REAL_4, REAL_8, ASCII_STRING, INVALID_DT
};

static const size_t dataTypeSize[DataType::INVALID_DT] = { 1, 1, 2, 4, 4, 8, 1 };

double real8_to_double(uint64_t real);

uint64_t double_to_real8(double real);

DataType toDataType(uint8_t dataType);

uint8_t fromDataType(DataType dataType);

typedef struct
{
  RecordType type;
  DataType dataType;
  uint16_t length;
  std::string data8;
  std::vector<int16_t> data16;
  std::vector<int32_t> data32;
  std::vector<double> data64;
} record_t;

std::map<std::pair<int16_t, int16_t>, std::string> getLayerMap(std::string filename);


} // namespace odb