// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace odb {

class dbChip;
class dbChipBumpInst;
class dbChipConn;
class dbChipInst;
class dbChipNet;
class dbMarker;
class dbMarkerCategory;

///////////////////////////////////////////////////////////////////////////////
///
/// dbChipCallBackObj - An object comprising a list of stub routines
/// invoked by dbChip.
/// Derived classes may implement these routines, causing external code
/// to be invoked from inside dbBlock methods.
///
///////////////////////////////////////////////////////////////////////////////

class dbChipCallBackObj
{
 public:
  dbChipCallBackObj() { _owner = nullptr; }
  virtual ~dbChipCallBackObj() { removeOwner(); }

  // dbMarkerCategory Start
  virtual void inDbMarkerCategoryCreate(dbMarkerCategory*) {}
  virtual void inDbMarkerCategoryDestroy(dbMarkerCategory*) {}
  // dbMarkerCategory End

  // dbMarker Start
  virtual void inDbMarkerCreate(dbMarker*) {}
  virtual void inDbMarkerDestroy(dbMarker*) {}
  // dbMarker End

  // dbChipInst Start
  virtual void inDbChipInstCreate(dbChipInst*) {}
  virtual void inDbChipInstDestroy(dbChipInst*) {}
  virtual void inDbPreMoveDbChipInst(dbChipInst*) {}
  virtual void inDbPostMoveDbChipInst(dbChipInst*) {}
  // dbChipInst End

  // dbChipConn Start
  virtual void inDbChipConnCreate(dbChipConn*) {}
  virtual void inDbChipConnDestroy(dbChipConn*) {}
  // dbChipConn End

  // dbChipNet Start
  virtual void inDbChipNetCreate(dbChipNet*) {}
  virtual void inDbChipNetDestroy(dbChipNet*) {}
  virtual void inDbChipNetConnectBumpInst(dbChipNet*, dbChipBumpInst*) {}
  // dbChipNet End

  // allow ECO client initialization - payam
  virtual dbChipCallBackObj& operator()() { return *this; }

  // Manipulate _callback list of owner -- in journal.cpp
  void addOwner(dbChip* new_owner);
  bool hasOwner() const { return (_owner != nullptr); }
  void removeOwner();

 private:
  dbChip* _owner = nullptr;
};

}  // namespace odb
