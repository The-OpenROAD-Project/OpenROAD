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

#include "dbCore.h"
#include "dbId.h"
#include "dbTypes.h"
#include "dbVector.h"
#include "odb.h"

namespace odb {

class _dbMPin;
class _dbTarget;
class _dbDatabase;
class _dbTechAntennaAreaElement;
class _dbTechAntennaPinModel;
class dbIStream;
class dbOStream;
class dbDiff;

struct _dbMTermFlags
{
  dbIoType::Value _io_type : 4;
  dbSigType::Value _sig_type : 4;
  uint _mark : 1;
  uint _spare_bits : 23;
};

class _dbMTerm : public _dbObject
{
 public:
  // PERSISTANT-MEMBERS
  _dbMTermFlags _flags;
  uint _order_id;
  char* _name;
  dbId<_dbMTerm> _next_entry;
  dbId<_dbMTerm> _next_mterm;
  dbId<_dbMPin> _pins;
  dbId<_dbTarget> _targets;
  dbId<_dbTechAntennaPinModel> _oxide1;
  dbId<_dbTechAntennaPinModel> _oxide2;

  dbVector<_dbTechAntennaAreaElement*> _par_met_area;
  dbVector<_dbTechAntennaAreaElement*> _par_met_sidearea;
  dbVector<_dbTechAntennaAreaElement*> _par_cut_area;
  dbVector<_dbTechAntennaAreaElement*> _diffarea;

  void* _sta_port;  // not saved

  friend dbOStream& operator<<(dbOStream& stream, const _dbMTerm& mterm);
  friend dbIStream& operator>>(dbIStream& stream, _dbMTerm& mterm);

  _dbMTerm(_dbDatabase* db);
  _dbMTerm(_dbDatabase* db, const _dbMTerm& m);
  ~_dbMTerm();

  bool operator==(const _dbMTerm& rhs) const;
  bool operator!=(const _dbMTerm& rhs) const { return !operator==(rhs); }
  void differences(dbDiff& diff, const char* field, const _dbMTerm& rhs) const;
  void out(dbDiff& diff, char side, const char* field) const;
};

inline _dbMTerm::_dbMTerm(_dbDatabase*)
{
  _flags._io_type = dbIoType::INPUT;
  _flags._sig_type = dbSigType::SIGNAL;
  _flags._mark = 0;
  _flags._spare_bits = 0;
  _order_id = 0;
  _name = 0;
  _par_met_area.clear();
  _par_met_sidearea.clear();
  _par_cut_area.clear();
  _diffarea.clear();
  _sta_port = nullptr;
}
inline _dbMTerm::~_dbMTerm()
{
  if (_name)
    free((void*) _name);

  /************************************ dimitri_note
  *********************************** The following 4 vfields should change to
  look like     dbId<_dbTechAntennaPinModel> _oxide1;

          dbVector<_dbTechAntennaAreaElement *>  _par_met_area;
          dbVector<_dbTechAntennaAreaElement *>  _par_met_sidearea;
          dbVector<_dbTechAntennaAreaElement *>  _par_cut_area;
          dbVector<_dbTechAntennaAreaElement *>  _diffarea;
  ************************************************************************************************/

  /* dimitri_fix : cooment out delete loops because of the copiler warning
  *************************** dbMTerm.h:97:15: warning: possible problem
  detected in invocation of delete operator: [-Wdelete-incomplete] delete
  *antitr;
  ****************************************************************************************************/

  /**********************************************************************************
  dimitri_fix ********

      dbVector<_dbTechAntennaAreaElement *>::iterator  antitr;
      for (antitr = _par_met_area.begin(); antitr != _par_met_area.end();
  antitr++) delete *antitr; _par_met_area.clear();

      for (antitr = _par_met_sidearea.begin(); antitr !=
  _par_met_sidearea.end(); antitr++) delete *antitr; _par_met_sidearea.clear();

      for (antitr = _par_cut_area.begin(); antitr != _par_cut_area.end();
  antitr++) delete *antitr; _par_cut_area.clear();

      for (antitr = _diffarea.begin(); antitr != _diffarea.end(); antitr++)
        delete *antitr;
      _diffarea.clear();
  ***********************************************************************************************************/
}
}  // namespace odb
