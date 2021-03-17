///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <string>
#include <vector>

#include "dbId.h"
#include "dbObject.h"
#include "dbTypes.h"
#include "geom.h"
#include "odb.h"

namespace odb {

class dbNet;
class dbBTerm;
class dbSWire;
class dbBlock;
class dbInst;

class _dbBlock;

class dbDiff : public dbObject
{
  int _indent_level;
  FILE* _f;
  bool _deep_diff;
  std::vector<std::string> _headers;
  int _indent_per_level;
  bool _has_differences;

  void write_headers();
  void indent();

 public:
  enum Side
  {
    LEFT = '<',
    RIGHT = '>'
  };

  dbDiff(FILE* out_file);
  ~dbDiff();

  bool hasDifferences() const { return _has_differences; }

  // begin comparison of object
  void begin(const char* field, const char* objname, uint oid);
  void begin(const char side, const char* field, const char* objname, uint oid);
  void begin(const char* field,
             const char* objname,
             uint oid,
             const char* name);
  void begin(const char side,
             const char* field,
             const char* objname,
             uint oid,
             const char* name);
  void begin_object(const char* fmt, ...) ADS_FORMAT_PRINTF(2, 3);

  // end comparison of object
  void end_object();

  // report a difference
  void report(const char* fmt, ...) ADS_FORMAT_PRINTF(2, 3);

  // increment the indent level
  void increment() { ++_indent_level; }

  // deccrement the indent level
  void decrement()
  {
    --_indent_level;
    assert(_indent_level >= 0);
  }

  // set the depth of comparison.
  void setDeepDiff(bool value) { _deep_diff = value; }

  // return the depth of comparison
  bool deepDiff() { return _deep_diff; }

  // Set the indent count per level (default is 4)
  void setIndentPerLevel(int n) { _indent_per_level = n; }

  dbDiff& operator<<(bool c);
  dbDiff& operator<<(char c);
  dbDiff& operator<<(unsigned char c);
  dbDiff& operator<<(short c);
  dbDiff& operator<<(unsigned short c);
  dbDiff& operator<<(int c);
  dbDiff& operator<<(unsigned int c);
  dbDiff& operator<<(float c);
  dbDiff& operator<<(double c);
  dbDiff& operator<<(long double c);
  dbDiff& operator<<(const char* c);
  dbDiff& operator<<(const Point& p);
  dbDiff& operator<<(const Rect& r);
  dbDiff& operator<<(const Oct& o);

  void diff(const char* field, bool lhs, bool rhs);
  void diff(const char* field, char lhs, char rhs);
  void diff(const char* field, unsigned char lhs, unsigned char rhs);
  void diff(const char* field, short lhs, short rhs);
  void diff(const char* field, unsigned short lhs, unsigned short rhs);
  void diff(const char* field, int lhs, int rhs);
  void diff(const char* field, unsigned int lhs, unsigned int rhs);
  void diff(const char* field, float lhs, float rhs);
  void diff(const char* field, double lhs, double rhs);
  void diff(const char* field, long double lhs, long double rhs);
  void diff(const char* field, Point lhs, Point rhs);
  void diff(const char* field, Rect lhs, Rect rhs);
  void diff(const char* field, Oct lhs, Oct rhs);

  void diff(const char* field, const char* lhs, const char* rhs);
  void diff(const char* field,
            dbOrientType::Value lhs,
            dbOrientType::Value rhs);
  void diff(const char* field, dbSigType::Value lhs, dbSigType::Value rhs);
  void diff(const char* field, dbIoType::Value lhs, dbIoType::Value rhs);
  void diff(const char* field,
            dbPlacementStatus::Value lhs,
            dbPlacementStatus::Value rhs);
  void diff(const char* field,
            dbMasterType::Value lhs,
            dbMasterType::Value rhs);
  void diff(const char* field,
            dbTechLayerType::Value lhs,
            dbTechLayerType::Value rhs);
  void diff(const char* field,
            dbTechLayerDir::Value lhs,
            dbTechLayerDir::Value rhs);
  void diff(const char* field, dbRowDir::Value lhs, dbRowDir::Value rhs);
  void diff(const char* field, dbBoxOwner::Value lhs, dbBoxOwner::Value rhs);
  void diff(const char* field, dbWireType::Value lhs, dbWireType::Value rhs);
  void diff(const char* field,
            dbWireShapeType::Value lhs,
            dbWireShapeType::Value rhs);
  void diff(const char* field, dbSiteClass::Value lhs, dbSiteClass::Value rhs);
  void diff(const char* field, dbOnOffType::Value lhs, dbOnOffType::Value rhs);
  void diff(const char* field,
            dbClMeasureType::Value lhs,
            dbClMeasureType::Value rhs);
  void diff(const char* field, dbDirection::Value lhs, dbDirection::Value rhs);

  void out(char side, const char* field, bool value);
  void out(char side, const char* field, char value);
  void out(char side, const char* field, unsigned char value);
  void out(char side, const char* field, short value);
  void out(char side, const char* field, unsigned short value);
  void out(char side, const char* field, int value);
  void out(char side, const char* field, unsigned int value);
  void out(char side, const char* field, float value);
  void out(char side, const char* field, double value);
  void out(char side, const char* field, long double value);
  void out(char side, const char* field, Point value);
  void out(char side, const char* field, Rect value);
  void out(char side, const char* field, Oct value);
  void out(char side, const char* field, const char* value);
  void out(char side, const char* field, dbOrientType::Value value);
  void out(char side, const char* field, dbSigType::Value value);
  void out(char side, const char* field, dbIoType::Value value);
  void out(char side, const char* field, dbPlacementStatus::Value value);
  void out(char side, const char* field, dbMasterType::Value value);
  void out(char side, const char* field, dbTechLayerType::Value value);
  void out(char side, const char* field, dbTechLayerDir::Value value);
  void out(char side, const char* field, dbRowDir::Value value);
  void out(char side, const char* field, dbBoxOwner::Value value);
  void out(char side, const char* field, dbWireType::Value value);
  void out(char side, const char* field, dbWireShapeType::Value value);
  void out(char side, const char* field, dbSiteClass::Value value);
  void out(char side, const char* field, dbOnOffType::Value value);
  void out(char side, const char* field, dbClMeasureType::Value value);
  void out(char side, const char* field, dbDirection::Value value);
};

#define DIFF_BEGIN \
  { /* } */        \
    diff.begin(field, getObjName(), getId());

#define DIFF_OUT_BEGIN \
  { /* } */            \
    diff.begin(side, field, getObjName(), getId());

#define DIFF_END             \
  diff.end_object(); /* { */ \
  }

#define DIFF_FIELD(FIELD) diff.diff(#FIELD, FIELD, rhs.FIELD);

#define DIFF_FIELD_NO_DEEP(FIELD) \
  if (!diff.deepDiff())           \
    diff.diff(#FIELD, FIELD, rhs.FIELD);

#define DIFF_STRUCT(FIELD)                      \
  if (FIELD != rhs.FIELD) {                     \
    FIELD.differences(diff, #FIELD, rhs.FIELD); \
  }

#define DIFF_NAME_CACHE(FIELD)                    \
  if (*FIELD != *rhs.FIELD) {                     \
    FIELD->differences(diff, #FIELD, *rhs.FIELD); \
  }

#define DIFF_VECTOR(FIELD)                      \
  if (FIELD != rhs.FIELD) {                     \
    FIELD.differences(diff, #FIELD, rhs.FIELD); \
  }

#define DIFF_VECTOR_DEEP(FIELD)                           \
  if (FIELD != rhs.FIELD) {                               \
    if (!diff.deepDiff())                                 \
      FIELD.differences(diff, #FIELD, rhs.FIELD);         \
    else                                                  \
      set_symmetric_diff(diff, #FIELD, FIELD, rhs.FIELD); \
  }

#define DIFF_MATRIX(FIELD)                      \
  if (FIELD != rhs.FIELD) {                     \
    FIELD.differences(diff, #FIELD, rhs.FIELD); \
  }

#define DIFF_VECTOR_PTR(FIELD)                  \
  if (FIELD != rhs.FIELD) {                     \
    FIELD.differences(diff, #FIELD, rhs.FIELD); \
  }

#define DIFF_HASH_TABLE(FIELD)                  \
  if (FIELD != rhs.FIELD) {                     \
    FIELD.differences(diff, #FIELD, rhs.FIELD); \
  }

#define DIFF_OBJECT(FIELD, LHS_TBL, RHS_TBL) \
  diff_object(diff, #FIELD, FIELD, rhs.FIELD, LHS_TBL, RHS_TBL);

#define DIFF_SET(FIELD, LHS_ITR, RHS_ITR) \
  diff_set(diff,                          \
           #FIELD,                        \
           FIELD,                         \
           rhs.FIELD,                     \
           (dbObject*) this,              \
           (dbObject*) &rhs,              \
           LHS_ITR,                       \
           RHS_ITR);

#define DIFF_TABLE_NO_DEEP(TABLE)         \
  if (!diff.deepDiff()) {                 \
    TABLE->differences(diff, *rhs.TABLE); \
  }

#define DIFF_TABLE(TABLE)                                 \
  if (diff.deepDiff()) {                                  \
    set_symmetric_diff(diff, #TABLE, *TABLE, *rhs.TABLE); \
  } else {                                                \
    TABLE->differences(diff, *rhs.TABLE);                 \
  }

#define DIFF_OUT_FIELD(FIELD) diff.out(side, #FIELD, FIELD);

#define DIFF_OUT_FIELD_NO_DEEP(FIELD) \
  if (!diff.deepDiff())               \
    diff.out(side, #FIELD, FIELD);

#define DIFF_OUT_STRUCT(FIELD) FIELD.out(diff, side, #FIELD);

#define DIFF_OUT_NAME_CACHE(FIELD) FIELD->out(diff, side, #FIELD);

#define DIFF_OUT_VECTOR(FIELD) FIELD.out(diff, side, #FIELD);

#define DIFF_OUT_MATRIX(FIELD) FIELD.out(diff, side, #FIELD);

#define DIFF_OUT_VECTOR_PTR(FIELD) FIELD.out(diff, side, #FIELD);

#define DIFF_OUT_HASH_TABLE(FIELD) FIELD.out(diff, side, #FIELD);

#define DIFF_OUT_OBJECT(FIELD, LHS_TBL) \
  diff_out_object(diff, side, #FIELD, FIELD, LHS_TBL);

#define DIFF_OUT_SET(FIELD, LHS_ITR) \
  diff_out_set(diff, side, #FIELD, FIELD, (dbObject*) this, LHS_ITR);

#define DIFF_OUT_TABLE_NO_DEEP(TABLE) \
  if (!diff.deepDiff())               \
    TABLE->out(diff, side);

#define DIFF_OUT_TABLE(TABLE) TABLE->out(diff, side);

template <class T>
class dbDiffCmp
{
 public:
  int operator()(const T* lhs, const T* rhs) const { return *lhs < *rhs; }
};

template <class T>
class dbDiffDifferences
{
 public:
  void operator()(dbDiff& diff,
                  const char* field,
                  const T* lhs,
                  const T* rhs) const
  {
    lhs->differences(diff, field, rhs);
  }
};

template <class T>
class dbDiffOut
{
 public:
  void operator()(dbDiff& diff, char side, const char* field, const T* o) const
  {
    o->out(diff, side, field);
  }
};

template <class T>
class dbTable;
template <class T>
class dbArrayTable;
class dbIterator;

template <class T>
void diff_object(dbDiff& diff,
                 const char* field,
                 dbId<T> lhs,
                 dbId<T> rhs,
                 dbTable<T>* lhs_tbl,
                 dbTable<T>* rhs_tbl);

template <class T>
void diff_object(dbDiff& diff,
                 const char* field,
                 dbId<T> lhs,
                 dbId<T> rhs,
                 dbArrayTable<T>* lhs_tbl,
                 dbArrayTable<T>* rhs_tbl);

template <class T>
void diff_set(dbDiff& diff,
              const char* field,
              dbId<T> lhs,
              dbId<T> rhs,
              dbObject* lhs_owner,
              dbObject* rhs_owner,
              dbIterator* lhs_itr,
              dbIterator* rhs_itr);

template <class T>
void set_symmetric_diff(dbDiff& diff,
                        const char* field,
                        std::vector<T*>& lhs,
                        std::vector<T*>& rhs);

template <class T>
void set_symmetric_diff(dbDiff& diff,
                        const char* field,
                        const std::vector<T>& lhs,
                        const std::vector<T>& rhs);

template <class T>
void set_symmetric_diff(dbDiff& diff,
                        const char* field,
                        dbTable<T>& lhs,
                        dbTable<T>& rhs);

template <class T>
void set_symmetric_diff(dbDiff& diff,
                        const char* field,
                        dbArrayTable<T>& lhs,
                        dbArrayTable<T>& rhs);

template <class T>
void diff_out_object(dbDiff& diff,
                     char side,
                     const char* field,
                     dbId<T> id,
                     dbTable<T>* tbl);

template <class T>
void diff_out_object(dbDiff& diff,
                     char side,
                     const char* field,
                     dbId<T> id,
                     dbArrayTable<T>* tbl);

template <class T>
void diff_out_set(dbDiff& diff,
                  char side,
                  const char* field,
                  dbId<T> id,
                  dbObject* owner,
                  dbIterator* itr);

}  // namespace odb
