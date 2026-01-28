// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbInstHdr.h"

#include <cassert>

#include "dbBlock.h"
#include "dbCore.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbInstHdr>;

bool _dbInstHdr::operator==(const _dbInstHdr& rhs) const
{
  if (mterm_cnt_ != rhs.mterm_cnt_) {
    return false;
  }

  if (id_ != rhs.id_) {
    return false;
  }

  if (next_entry_ != rhs.next_entry_) {
    return false;
  }

  if (lib_ != rhs.lib_) {
    return false;
  }

  if (master_ != rhs.master_) {
    return false;
  }

  if (mterms_ != rhs.mterms_) {
    return false;
  }

  if (inst_cnt_ != rhs.inst_cnt_) {
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////
//
// _dbInstHdr - Methods
//
////////////////////////////////////////////////////////////////////

_dbInstHdr::_dbInstHdr(_dbDatabase*)
{
  id_ = 0;
  mterm_cnt_ = 0;
  inst_cnt_ = 0;
}

_dbInstHdr::_dbInstHdr(_dbDatabase*, const _dbInstHdr& i)
    : mterm_cnt_(i.mterm_cnt_),
      id_(i.id_),
      next_entry_(i.next_entry_),
      lib_(i.lib_),
      master_(i.master_),
      mterms_(i.mterms_),
      inst_cnt_(i.inst_cnt_)
{
}

dbOStream& operator<<(dbOStream& stream, const _dbInstHdr& inst_hdr)
{
  stream << inst_hdr.mterm_cnt_;
  stream << inst_hdr.id_;
  stream << inst_hdr.next_entry_;
  stream << inst_hdr.lib_;
  stream << inst_hdr.master_;
  stream << inst_hdr.mterms_;
  stream << inst_hdr.inst_cnt_;
  return stream;
}

dbIStream& operator>>(dbIStream& stream, _dbInstHdr& inst_hdr)
{
  stream >> inst_hdr.mterm_cnt_;
  stream >> inst_hdr.id_;
  stream >> inst_hdr.next_entry_;
  stream >> inst_hdr.lib_;
  stream >> inst_hdr.master_;
  stream >> inst_hdr.mterms_;
  stream >> inst_hdr.inst_cnt_;
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
  _dbInstHdr* inst_hdr = (_dbInstHdr*) this;
  _dbDatabase* db = getDatabase();
  return (dbLib*) db->lib_tbl_->getPtr(inst_hdr->lib_);
}

dbMaster* dbInstHdr::getMaster()
{
  _dbInstHdr* inst_hdr = (_dbInstHdr*) this;
  _dbDatabase* db = getDatabase();
  _dbLib* lib = db->lib_tbl_->getPtr(inst_hdr->lib_);
  return (dbMaster*) lib->master_tbl_->getPtr(inst_hdr->master_);
}

dbInstHdr* dbInstHdr::create(dbBlock* block_, dbMaster* master_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbMaster* master = (_dbMaster*) master_;
  _dbLib* lib = (_dbLib*) master->getOwner();

  if (!master->flags_.frozen) {
    return nullptr;
  }

  if (block->inst_hdr_hash_.hasMember(master->id_)) {
    return nullptr;
  }

  _dbInstHdr* inst_hdr;
  // initialize the inst_hdr structure
  inst_hdr = block->inst_hdr_tbl_->create();
  inst_hdr->mterm_cnt_ = master->mterm_cnt_;
  inst_hdr->id_ = master->id_;
  inst_hdr->lib_ = lib->getOID();
  inst_hdr->master_ = master->getOID();

  // insert the inst_hdr into the block inst_hdr hash table.
  block->inst_hdr_hash_.insert(inst_hdr);

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
  inst_hdr->mterms_.resize(master->mterm_cnt_);

  // mterms, this set is ordered: {output, inout, input}
  int i = 0;
  for (dbMTerm* mterm : master_->getMTerms()) {
    inst_hdr->mterms_[i++] = mterm->getImpl()->getOID();
  }

  return (dbInstHdr*) inst_hdr;
}

void dbInstHdr::destroy(dbInstHdr* inst_hdr_)
{
  _dbInstHdr* inst_hdr = (_dbInstHdr*) inst_hdr_;
  _dbBlock* block = (_dbBlock*) inst_hdr->getOwner();

  assert(inst_hdr->inst_cnt_ == 0);
  block->inst_hdr_hash_.remove(inst_hdr);
  block->inst_hdr_tbl_->destroy(inst_hdr);
}

void _dbInstHdr::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children["mterms"].add(mterms_);
}

}  // namespace odb
