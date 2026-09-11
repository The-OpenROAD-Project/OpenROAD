// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <utility>
#include <vector>

#include "boost/container/flat_map.hpp"
#include "dbCore.h"
#include "dbDatabase.h"
#include "odb/db.h"
#include "odb/dbId.h"

namespace odb {

class _dbNet;
class _dbModNet;
class _dbMTerm;
class _dbInst;
class _dbITerm;
class _dbDatabase;
class dbIStream;
class dbOStream;
class _dbAccessPoint;
class _dbMPin;

using ApMap = boost::container::flat_map<dbId<_dbMPin>, dbId<_dbAccessPoint>>;

struct dbITermFlags
{
  // note: number of bits must add up to 32 !!!
  uint32_t mterm_idx : 20;  // index into inst-hdr-mterm-vector
  uint32_t spare_bits : 7;
  uint32_t clocked : 1;
  uint32_t mark : 1;
  uint32_t spef : 1;       // Spef flag
  uint32_t special : 1;    // Special net connection.
  uint32_t connected : 1;  // terminal is physically connected
};

// The columns of iterm_tbl. One array per field, page_size entries each,
// owned by the page -- the structure-of-arrays half of the same table a
// dbTable has always been.
//
// Two of the columns do double duty. While a slot is free its next and prev
// free-list links live in `net` and `mnet`, which mean nothing for a free
// slot: exactly the overlay _dbFreeObject performs on an allocated record's
// bytes today, moved into the columns.
struct _dbITermFields
{
  explicit _dbITermFields(uint32_t slots)
      : flags(slots),
        ext_id(slots),
        net(slots),
        mnet(slots),
        inst(slots),
        next_net_iterm(slots),
        prev_net_iterm(slots),
        next_modnet_iterm(slots),
        prev_modnet_iterm(slots),
        mterm(slots, nullptr),
        sta_vertex_id(slots)
  {
  }

  std::vector<dbITermFlags> flags;
  std::vector<uint32_t> ext_id;
  std::vector<dbId<_dbNet>> net;
  std::vector<dbId<_dbModNet>> mnet;
  std::vector<dbId<_dbInst>> inst;
  std::vector<dbId<_dbITerm>> next_net_iterm;
  std::vector<dbId<_dbITerm>> prev_net_iterm;
  std::vector<dbId<_dbITerm>> next_modnet_iterm;
  std::vector<dbId<_dbITerm>> prev_modnet_iterm;
  std::vector<_dbMTerm*> mterm;         // not saved - cached pointer
  std::vector<uint32_t> sta_vertex_id;  // not saved

  // Access points are the one field most slots do not have: before pin
  // access runs, none of them do, and a dense column would spend 24 bytes
  // per slot on an empty map. This one is keyed by slot instead.
  boost::container::flat_map<uint32_t, ApMap> aps;

  // What a slot without an entry reads as. Const so a read cannot create
  // one by accident.
  static const ApMap kNoAps;
};

// A slot in iterm_tbl. It carries no fields of its own: what a slot still
// needs an address for is that every dbITerm* handed out by the public API
// is that address, and that dbObject::getObjectPage() finds its page from
// the offset word it inherits. The fields live in the page's columns, and
// the accessors below return references into them -- so a call site reads
// `iterm->net() = n` where it used to read `iterm->net_ = n`.
class _dbITerm : public _dbObject
{
 public:
  // iterm_tbl is the largest table in a placed design, and most of a slot's
  // columns are constant or monotone. See dbFieldMajorTable().
  static constexpr bool kFieldMajorTable = true;

  // ... and it stores those columns as arrays. See dbSoaTable().
  static constexpr bool kSoaTable = true;
  using Fields = _dbITermFields;

  enum Field  // dbJournal field name
  {
    kFlags
  };

  _dbITerm(_dbDatabase*) {}

  bool operator==(const _dbITerm& rhs) const;
  bool operator!=(const _dbITerm& rhs) const { return !operator==(rhs); }
  bool operator<(const _dbITerm& rhs) const;
  void collectMemInfo(MemInfo& info);

  _dbMTerm* getMTerm() const;
  _dbInst* getInst() const;
  void resolveMTerm();

  // The slot's own columns.
  _dbITermFields* columns() const
  {
    return (_dbITermFields*) getObjectPage()->fields_;
  }
  uint32_t slot() const { return getOID() - getObjectPage()->page_addr_; }

#define ODB_ITERM_COLUMN(name, type)               \
  type& name() { return columns()->name[slot()]; } \
  const type& name() const { return columns()->name[slot()]; }

  ODB_ITERM_COLUMN(flags, dbITermFlags)
  ODB_ITERM_COLUMN(ext_id, uint32_t)
  ODB_ITERM_COLUMN(net, dbId<_dbNet>)
  ODB_ITERM_COLUMN(mnet, dbId<_dbModNet>)
  ODB_ITERM_COLUMN(inst, dbId<_dbInst>)
  ODB_ITERM_COLUMN(next_net_iterm, dbId<_dbITerm>)
  ODB_ITERM_COLUMN(prev_net_iterm, dbId<_dbITerm>)
  ODB_ITERM_COLUMN(next_modnet_iterm, dbId<_dbITerm>)
  ODB_ITERM_COLUMN(prev_modnet_iterm, dbId<_dbITerm>)
  ODB_ITERM_COLUMN(sta_vertex_id, uint32_t)
#undef ODB_ITERM_COLUMN

  // Reading is sparse-aware; writing creates the slot's entry, so a caller
  // that only wants to look should use the const overload.
  const ApMap& aps() const
  {
    const auto& sparse = columns()->aps;
    const auto entry = sparse.find(slot());
    return entry == sparse.end() ? _dbITermFields::kNoAps : entry->second;
  }
  ApMap& aps() { return columns()->aps[slot()]; }

  _dbMTerm*& mterm() { return columns()->mterm[slot()]; }
  _dbMTerm* mterm() const { return columns()->mterm[slot()]; }

  // While the slot is free these two columns hold the free-list links.
  uint32_t freeNext() const { return columns()->net[slot()]; }
  uint32_t freePrev() const { return columns()->mnet[slot()]; }
  void setFreeNext(uint32_t id) { columns()->net[slot()] = id; }
  void setFreePrev(uint32_t id) { columns()->mnet[slot()] = id; }

  // dbTable calls these where it would otherwise run a constructor or a
  // destructor over the slot's own storage.
  void initFields();
  void clearFields();
};

// A slot's columns are default-constructed with its page and reset when it
// is freed, so allocating one has nothing left to initialize. This is what
// dbTable calls in place of running a constructor over slot storage.
inline void _dbITerm::initFields()
{
  flags() = dbITermFlags{};
  ext_id() = 0;
  net() = dbId<_dbNet>();
  mnet() = dbId<_dbModNet>();
  inst() = dbId<_dbInst>();
  next_net_iterm() = dbId<_dbITerm>();
  prev_net_iterm() = dbId<_dbITerm>();
  next_modnet_iterm() = dbId<_dbITerm>();
  prev_modnet_iterm() = dbId<_dbITerm>();
  mterm() = nullptr;
  sta_vertex_id() = 0;
  columns()->aps.erase(slot());
}

inline void _dbITerm::clearFields()
{
  // The sparse column is the only one that owns anything.
  columns()->aps.erase(slot());
  mterm() = nullptr;
}

inline dbOStream& operator<<(dbOStream& stream, const _dbITerm& iterm)
{
  const dbITermFlags flags = iterm.flags();
  uint32_t bit_field;
  static_assert(sizeof(bit_field) == sizeof(flags));
  std::memcpy(&bit_field, &flags, sizeof(bit_field));
  stream << bit_field;
  stream << iterm.ext_id();
  stream << iterm.net();
  stream << iterm.inst();
  stream << iterm.next_net_iterm();
  stream << iterm.prev_net_iterm();
  stream << iterm.mnet();
  stream << iterm.next_modnet_iterm();
  stream << iterm.prev_modnet_iterm();
  stream << iterm.aps();
  return stream;
}

inline dbIStream& operator>>(dbIStream& stream, _dbITerm& iterm)
{
  dbBlock* block = (dbBlock*) (iterm.getOwner());
  _dbDatabase* db = (_dbDatabase*) (block->getDataBase());
  uint32_t bit_field;
  stream >> bit_field;
  dbITermFlags flags;
  std::memcpy(&flags, &bit_field, sizeof(bit_field));
  iterm.flags() = flags;
  stream >> iterm.ext_id();
  stream >> iterm.net();
  stream >> iterm.inst();
  stream >> iterm.next_net_iterm();
  stream >> iterm.prev_net_iterm();
  if (db->isSchema(kSchemaUpdateHierarchy)) {
    stream >> iterm.mnet();
    stream >> iterm.next_modnet_iterm();
    stream >> iterm.prev_modnet_iterm();
  }
  // Read into a local and only create the slot's entry if there is
  // something to put in it: asking for a mutable reference would give every
  // iterm in the design an entry, which is what the sparse column exists to
  // avoid.
  ApMap aps;
  stream >> aps;
  if (!aps.empty()) {
    iterm.aps() = std::move(aps);
  }
  return stream;
}

}  // namespace odb
