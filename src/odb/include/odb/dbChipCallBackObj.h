// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#pragma once

namespace odb {

class dbChip;
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
