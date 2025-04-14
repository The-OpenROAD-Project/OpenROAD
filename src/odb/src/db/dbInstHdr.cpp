// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "dbInstHdr.h"

#include "dbBlock.h"
#include "dbDatabase.h"
#include "dbLib.h"
#include "dbMTerm.h"
#include "dbMaster.h"
#include "dbTable.h"
#include "dbTable.hpp"
#include "odb/db.h"

namespace odb {

template class dbTable<_dbInstHdr>;

bool _dbInstHdr::operator==(const _dbInstHdr& rhs) const
{
  if (_mterm_cnt != rhs._mterm_cnt) {
    return false;
  }

  if (_id != rhs._id) {
    return false;
  }

  if (_next_entry != rhs._next_entry) {
    return false;
  }

  if (_lib != rhs._lib) {
    return false;
  }

  if (_master != rhs._master) {
    return false;
  }

  if (_mterms != rhs._mterms) {
    return false;
  }

  if (_inst_cnt != rhs._inst_cnt) {
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
  _id = 0;
  _mterm_cnt = 0;
  _inst_cnt = 0;
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
  _dbInstHdr* inst_hdr = (_dbInstHdr*) this;
  _dbDatabase* db = getDatabase();
  return (dbLib*) db->_lib_tbl->getPtr(inst_hdr->_lib);
}

dbMaster* dbInstHdr::getMaster()
{
  _dbInstHdr* inst_hdr = (_dbInstHdr*) this;
  _dbDatabase* db = getDatabase();
  _dbLib* lib = db->_lib_tbl->getPtr(inst_hdr->_lib);
  return (dbMaster*) lib->_master_tbl->getPtr(inst_hdr->_master);
}

dbInstHdr* dbInstHdr::create(dbBlock* block_, dbMaster* master_)
{
  _dbBlock* block = (_dbBlock*) block_;
  _dbMaster* master = (_dbMaster*) master_;
  _dbLib* lib = (_dbLib*) master->getOwner();

  if (!master->_flags._frozen) {
    return nullptr;
  }

  if (block->_inst_hdr_hash.hasMember(master->_id)) {
    return nullptr;
  }

  _dbInstHdr* inst_hdr;
  // initialize the inst_hdr structure
  inst_hdr = block->_inst_hdr_tbl->create();
  inst_hdr->_mterm_cnt = master->_mterm_cnt;
  inst_hdr->_id = master->_id;
  inst_hdr->_lib = lib->getOID();
  inst_hdr->_master = master->getOID();

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
  int i = 0;
  for (dbMTerm* mterm : master_->getMTerms()) {
    inst_hdr->_mterms[i++] = mterm->getImpl()->getOID();
  }

  return (dbInstHdr*) inst_hdr;
}

void dbInstHdr::destroy(dbInstHdr* inst_hdr_)
{
  _dbInstHdr* inst_hdr = (_dbInstHdr*) inst_hdr_;
  _dbBlock* block = (_dbBlock*) inst_hdr->getOwner();

  assert(inst_hdr->_inst_cnt == 0);
  block->_inst_hdr_hash.remove(inst_hdr);
  block->_inst_hdr_tbl->destroy(inst_hdr);
}

void _dbInstHdr::collectMemInfo(MemInfo& info)
{
  info.cnt++;
  info.size += sizeof(*this);

  info.children_["mterms"].add(_mterms);
}

}  // namespace odb
