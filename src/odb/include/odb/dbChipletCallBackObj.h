// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2026, The OpenROAD Authors

#pragma once

#include <vector>

namespace odb {

class dbDatabase;
class dbChip;
class dbChipInst;
class dbChipConn;
class dbChipNet;
class dbChipRegion;
class dbChipBump;
class dbChipBumpInst;

///////////////////////////////////////////////////////////////////////////////
///
/// dbChipletCallBackObj - An object comprising a list of stub routines
/// invoked by chiplet-level ODB mutations. Registered on dbDatabase.
/// Derived classes may implement these routines to react to chiplet
/// hierarchy changes (instance create/destroy/move, connections, nets,
/// region/bump modifications).
///
///////////////////////////////////////////////////////////////////////////////

class dbChipletCallBackObj
{
 public:
  dbChipletCallBackObj() = default;
  virtual ~dbChipletCallBackObj();

  // dbChipInst lifecycle
  virtual void inDbChipInstCreate(dbChipInst*) {}
  virtual void inDbChipInstDestroy(dbChipInst*) {}

  // dbChipInst property changes (pre/post pairs)
  virtual void inDbChipInstPreModify(dbChipInst*) {}
  virtual void inDbChipInstPostModify(dbChipInst*) {}

  // dbChip geometry changes (generated setters fire these via "notify" flag)
  virtual void inDbChipPreModify(dbChip*) {}
  virtual void inDbChipPostModify(dbChip*) {}

  // dbChipConn lifecycle
  virtual void inDbChipConnCreate(dbChipConn*) {}
  virtual void inDbChipConnDestroy(dbChipConn*) {}

  // dbChipConn property changes (generated setters fire these via "notify")
  virtual void inDbChipConnPreModify(dbChipConn*) {}
  virtual void inDbChipConnPostModify(dbChipConn*) {}

  // dbChipNet lifecycle
  virtual void inDbChipNetCreate(dbChipNet*) {}
  virtual void inDbChipNetDestroy(dbChipNet*) {}
  virtual void inDbChipNetAddBumpInst(dbChipNet*,
                                      dbChipBumpInst*,
                                      const std::vector<dbChipInst*>& /*path*/)
  {
  }

  // dbChipRegion lifecycle and property changes
  virtual void inDbChipRegionCreate(dbChipRegion*) {}
  virtual void inDbChipRegionDestroy(dbChipRegion*) {}
  virtual void inDbChipRegionPreModify(dbChipRegion*) {}
  virtual void inDbChipRegionPostModify(dbChipRegion*) {}

  // dbChipBump lifecycle and property changes
  virtual void inDbChipBumpCreate(dbChipBump*) {}
  virtual void inDbChipBumpDestroy(dbChipBump*) {}
  virtual void inDbChipBumpPreModify(dbChipBump*) {}
  virtual void inDbChipBumpPostModify(dbChipBump*) {}

  // Registration on dbDatabase
  void addOwner(dbDatabase* db);
  void removeOwner();
  bool hasOwner() const { return owner_ != nullptr; }

 private:
  dbDatabase* owner_ = nullptr;
};

}  // namespace odb
