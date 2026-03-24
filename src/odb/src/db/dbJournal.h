// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <string>

#include "dbJournalLog.h"
#include "odb/db.h"
#include "odb/dbObject.h"

namespace utl {
class Logger;
}

namespace odb {

class dbIStream;
class dbOStream;
class dbBlock;
class dbMaster;
class dbNet;
class dbInst;
class dbITerm;

class dbJournal
{
 public:
  enum Action
  {
    kCreateObject,
    kDeleteObject,
    kConnectObject,
    kDisconnectObject,
    kSwapObject,
    kUpdateField,
    kEndAction
  };

  dbJournal(dbBlock* block);

  void clear();
  int size() const { return log_.size(); }

  //
  // Methods to push entries into the transaction log.
  //
  // The entries in the log take the form:
  //
  //    <ACTION>
  //    <PARAM-1>
  //     ...
  //    <PARAM-N>
  //    <ACTION-OFFSET>
  //
  void beginAction(Action type);
  void pushParam(bool value);
  void pushParam(char value);
  void pushParam(unsigned char value);
  void pushParam(int value);
  void pushParam(unsigned int value);
  void pushParam(float value);
  void pushParam(double value);
  void pushParam(const char* value);
  void pushParam(const std::string& value);
  void endAction();

  //
  // updateField : helper methods to update fields in objects.
  //
  // The update field entries in the log take the form:
  //
  //    <ACTION>
  //    <OBJECT-TYPE>
  //    <OBJECT-ID>
  //    <FIELD-ID>
  //    <PREV_VALUE>
  //    <NEW_VALUE>
  //    <ACTION-OFFSET>
  //
  void updateField(dbObject* obj,
                   int field_id,
                   bool prev_value,
                   bool new_value);
  void updateField(dbObject* obj,
                   int field_id,
                   char prev_value,
                   char new_value);
  void updateField(dbObject* obj,
                   int field_id,
                   unsigned char prev_value,
                   unsigned char new_value);
  void updateField(dbObject* obj, int field_id, int prev_value, int new_value);
  void updateField(dbObject* obj,
                   int field_id,
                   unsigned int prev_value,
                   unsigned int new_value);
  void updateField(dbObject* obj,
                   int field_id,
                   float prev_value,
                   float new_value);
  void updateField(dbObject* obj,
                   int field_id,
                   double prev_value,
                   double new_value);
  void updateField(dbObject* obj,
                   int field_id,
                   const char* prev_value,
                   const char* new_value);

  // redo the transaction log
  void redo();

  // undo the transaction log
  void undo();

  bool empty() const { return log_.empty(); }

  void append(dbJournal* other);

 private:
  friend class dbDatabase;

  void redo_createObject();
  void redo_deleteObject();
  void redo_connectObject();
  void redo_disconnectObject();
  void redo_swapObject();
  void redo_updateField();
  void redo_updateBlockField();
  void redo_updateNetField();
  void redo_updateModNetField();
  void redo_updateInstField();
  void redo_updateITermField();
  void redo_updateRSegField();
  void redo_updateCapNodeField();
  void redo_updateCCSegField();
  void redo_updateBTermField();

  void undo_createObject();
  void undo_deleteObject();
  void undo_connectObject();
  void undo_disconnectObject();
  void undo_swapObject();
  void undo_updateField();
  void undo_updateNetField();
  void undo_updateModNetField();
  void undo_updateInstField();
  void undo_updateITermField();
  void undo_updateRSegField();
  void undo_updateCapNodeField();
  void undo_updateCCSegField();

  dbObjectType popObjectType();

  friend dbIStream& operator>>(dbIStream& stream, dbJournal& jrnl);
  friend dbOStream& operator<<(dbOStream& stream, const dbJournal& jrnl);

  dbBlock* block_;
  utl::Logger* logger_;
  dbJournalLog log_;
  bool start_action_{false};
  uint32_t action_idx_{0};
  Action cur_action_{kCreateObject};
};

dbIStream& operator>>(dbIStream& stream, dbJournal& jrnl);
dbOStream& operator<<(dbOStream& stream, const dbJournal& jrnl);

}  // namespace odb
