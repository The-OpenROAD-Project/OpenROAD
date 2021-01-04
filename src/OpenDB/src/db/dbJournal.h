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

#include "dbJournalLog.h"
#include "odb.h"

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
  dbJournalLog  _log;
  dbBlock*      _block;
  bool          _start_action;
  uint          _action_idx;
  unsigned char _cur_action;

  void redo_createObject();
  void redo_deleteObject();
  void redo_connectObject();
  void redo_disconnectObject();
  void redo_swapObject();
  void redo_updateField();
  void redo_updateBlockField();
  void redo_updateNetField();
  void redo_updateInstField();
  void redo_updateITermField();
  void redo_updateRSegField();
  void redo_updateCapNodeField();
  void redo_updateCCSegField();

  void undo_createObject();
  void undo_deleteObject();
  void undo_connectObject();
  void undo_disconnectObject();
  void undo_swapObject();
  void undo_updateField();
  void undo_updateNetField();
  void undo_updateInstField();
  void undo_updateITermField();
  void undo_updateRSegField();
  void undo_updateCapNodeField();
  void undo_updateCCSegField();

 public:
  enum Action
  {
    CREATE_OBJECT,
    DELETE_OBJECT,
    CONNECT_OBJECT,
    DISCONNECT_OBJECT,
    SWAP_OBJECT,
    UPDATE_FIELD,
    END_ACTION
  };

  dbJournal(dbBlock* block);
  ~dbJournal();
  void clear();

  int size() { return _log.size(); }

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
                   int       field_id,
                   bool      prev_value,
                   bool      new_value);
  void updateField(dbObject* obj,
                   int       field_id,
                   char      prev_value,
                   char      new_value);
  void updateField(dbObject*     obj,
                   int           field_id,
                   unsigned char prev_value,
                   unsigned char new_value);
  void updateField(dbObject* obj, int field_id, int prev_value, int new_value);
  void updateField(dbObject*    obj,
                   int          field_id,
                   unsigned int prev_value,
                   unsigned int new_value);
  void updateField(dbObject* obj,
                   int       field_id,
                   float     prev_value,
                   float     new_value);
  void updateField(dbObject* obj,
                   int       field_id,
                   double    prev_value,
                   double    new_value);
  void updateField(dbObject*   obj,
                   int         field_id,
                   const char* prev_value,
                   const char* new_value);

  // redo the transaction log
  void redo();

  // undo the transaction log
  void undo();

  bool empty() { return _log.empty(); }

  friend dbIStream& operator>>(dbIStream& stream, dbJournal& jrnl);
  friend dbOStream& operator<<(dbOStream& stream, const dbJournal& jrnl);
  friend class dbDatabase;
};

dbIStream& operator>>(dbIStream& stream, dbJournal& jrnl);
dbOStream& operator<<(dbOStream& stream, const dbJournal& jrnl);

}  // namespace odb
