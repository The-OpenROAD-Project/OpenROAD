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

#include "dbInstHdr.h"

#include "db.h"
#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"

namespace odb {

template class dbTable<_dbInstHdr>;

bool _dbInstHdr::operator==(const _dbInstHdr& rhs) const
{
  if (_mterm_cnt != rhs._mterm_cnt)
    return false;

  if (_id != rhs._id)
    return false;

  if (_next_entry != rhs._next_entry)
    return false;

  if (_lib != rhs._lib)
    return false;

  if (_master != rhs._master)
    return false;

  if (_mterms != rhs._mterms)
    return false;

  if (_inst_cnt != rhs._inst_cnt)
    return false;

  return true;
}

void _dbInstHdr::differences(dbDiff&           diff,
                             const char*       field,
                             const _dbInstHdr& rhs) const
{
  DIFF_BEGIN
  DIFF_FIELD(_mterm_cnt);
  DIFF_FIELD(_id);
  DIFF_FIELD_NO_DEEP(_next_entry);
  DIFF_FIELD(_lib);
  DIFF_FIELD(_master);
  DIFF_VECTOR(_mterms);
  DIFF_FIELD(_inst_cnt);
  DIFF_END
}

void _dbInstHdr::out(dbDiff& diff, char side, const char* field) const
{
  DIFF_OUT_BEGIN
  DIFF_OUT_FIELD(_mterm_cnt);
  DIFF_OUT_FIELD(_id);
  DIFF_OUT_FIELD_NO_DEEP(_next_entry);
  DIFF_OUT_FIELD(_lib);
  DIFF_OUT_FIELD(_master);
  DIFF_OUT_VECTOR(_mterms);
  DIFF_OUT_FIELD(_inst_cnt);
  DIFF_END
}

////////////////////////////////////////////////////////////////////
//
// _dbInstHdr - Methods
//
////////////////////////////////////////////////////////////////////

_dbInstHdr::_dbInstHdr(_dbDatabase*)
{
  _id        = 0;
  _mterm_cnt = 0;
  _inst_cnt  = 0;
}

_dbInstHdr::_dbInstHdr(_dbDatabase*, const _dbInstHdr& i)
    : _mterm_cnt(i._mterm_cnt),
      _id(i._id),
      _next_entry(i._next_entry),
      _lib(i._lib),
      _master(i._master),
      _mterms(i._mterms),
      _inst_cnt(i._inst_cnt)
{
}

_dbInstHdr::~_dbInstHdr()
{
}

dbOStream& operator<<(dbOStream& stream, const _dbInstHdr& inst_hdr)
{
  stream << inst_hdr._mterm_cnt;
  stream << inst_hdr._id;
  stream << inst_hdr._next_entry;
  stream << inst_hdr._lib;
  stream << inst_hdr._master;
  stream << inst_hdr._mterms;
  stream << inst_hdr._inst_cnt;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbInstHdr& inst_hdr)
{
  stream >> inst_hdr._mterm_cnt;
  stream >> inst_hdr._id;
  stream >> inst_hdr._next_entry;
  stream >> inst_hdr._lib;
  stream >> inst_hdr._master;
  stream >> inst_hdr._mterms;
  stream >> inst_hdr._inst_cnt;
  return stream;
}

////////////////////////////////////////////////////////////////////
//
// dbInstHdr - Methods
//
////////////////////////////////////////////////////////////////////

dbBlock* dbInstHdr::getBlock()
{
  return (dbBlock*) getOwner();
}

dbLib* dbInstHdr::getLib()
{
  _dbInstHdr*  inst_hdr = (_dbInstHdr*) this;
  _dbDatabase* db       = getDatabase();
  return (dbLib*) db->_lib_tbl->getPtr(inst_hdr->_lib);
}

dbMaster* dbInstHdr::getMaster()
{
  _dbInstHdr*  inst_hdr = (_dbInstHdr*) this;
  _dbDatabase* db       = getDatabase();
  _dbLib*      lib      = db->_lib_tbl->getPtr(inst_hdr->_lib);
  return (dbMaster*) lib->_master_tbl->getPtr(inst_hdr->_master);
}

dbInstHdr* dbInstHdr::create(dbBlock* block_, dbMaster* master_)
{
  _dbBlock*  block  = (_dbBlock*) block_;
  _dbMaster* master = (_dbMaster*) master_;
  _dbLib*    lib    = (_dbLib*) master->getOwner();

  if (!master->_flags._frozen)
    return NULL;

  if (block->_inst_hdr_hash.hasMember(master->_id))
    return NULL;

  _dbInstHdr* inst_hdr;
  // initialize the inst_hdr structure
  inst_hdr             = block->_inst_hdr_tbl->create();
  inst_hdr->_mterm_cnt = master->_mterm_cnt;
  inst_hdr->_id        = master->_id;
  inst_hdr->_lib       = lib->getOID();
  inst_hdr->_master    = master->getOID();

  // insert the inst_hdr into the block inst_hdr hash table.
  block->_inst_hdr_hash.insert(inst_hdr);

  //
  // Each ITerm of and instances points back the MTerm the ITerm
  // represents. To save space in the ITerm and to make the order
  // of the ITerm of an instance appear as {output, inout, input},
  // a mapping structure is used. This structure maps offset of an ITerm
  // back to MTerm. The mapping vector is ordered as {output, inout, input}.
  // The alternative to this strategy would be to enforce a create order
  // on the creation of MTerms on a Master. This stragegy complicates the
  // creation of MTerms with streamed input formats such as LEF, because
  // one does not know the order of MTerms as they would be created.
  // Consequently, you would need to buffer the data of a master, i.e., a LEF
  // MACRO until the complete MACRO is parsed...
  //
  inst_hdr->_mterms.resize(master->_mterm_cnt);

  // mterms, this set is ordered: {output, inout, input}
  dbSet<dbMTerm>           mterms = master_->getMTerms();
  dbSet<dbMTerm>::iterator itr;
  int                      i = 0;

  for (itr = mterms.begin(); itr != mterms.end(); ++itr) {
    dbMTerm* mterm         = *itr;
    inst_hdr->_mterms[i++] = mterm->getImpl()->getOID();
  }

  return (dbInstHdr*) inst_hdr;
}

void dbInstHdr::destroy(dbInstHdr* inst_hdr_)
{
  _dbInstHdr* inst_hdr = (_dbInstHdr*) inst_hdr_;
  _dbBlock*   block    = (_dbBlock*) inst_hdr->getOwner();

  assert(inst_hdr->_inst_cnt == 0);
  block->_inst_hdr_hash.remove(inst_hdr);
  block->_inst_hdr_tbl->destroy(inst_hdr);
}

}  // namespace odb
