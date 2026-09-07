// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <string>

namespace odb {
class dbBlock;
}

namespace sta {

class dbSta;

// Timing constraints stored in the .odb.
//
// .odb already subsumes the netlist, tech, placement, routing and even
// dont_touch. Timing constraints are the conspicuous omission, so a flow
// must carry a matching .sdc next to every .odb and then work out which
// .sdc goes with which .odb -- a guess that can be wrong. Storing the
// constraints in the block makes an .odb self-describing.
//
// The opt-in is on the producer: `write_db -sdc` stores the current
// constraints in the block. A reader has nothing to remember: `read_db`
// restores whatever the block carries whenever the design is linked with
// liberty, which is the only situation in which constraints mean
// anything, and leaves the record alone otherwise. So an unmodified flow
// writes .odb files without constraints and behaves exactly as before,
// and an odb-only tool (no liberty, no STA) never pays for a restore and
// carries the record through unchanged. `write_db` without -sdc on a
// linked design removes any record the block carried, because the caller
// chose not to store, and a record written before an edit could be stale.
//
// Two encodings live under block properties, and a block carries at most
// one of them:
//
//   sta.sdc.native  A line-oriented record of the Sdc where every pin,
//                   instance and net is an odb object id. Restoring it is
//                   a linear walk that calls the Sta constraint makers
//                   directly: no Tcl, no name lookup, no pattern match.
//                   This is what a flow pays at every stage boundary, so
//                   it is the form that has to be fast. The record names
//                   the objects it refers to and carries a digest of their
//                   names, so a record that no longer matches the design
//                   is rejected instead of being applied to the wrong
//                   objects.
//
//   sta.sdc         The write_sdc text, replayed through the Tcl
//                   interpreter. Used only when the Sdc holds a construct
//                   the native encoder does not cover, so nothing is ever
//                   silently dropped: whatever write_sdc can say, the
//                   .odb can carry.
//
// Both payloads are opaque to odb (plain string properties), so no
// odb -> sta dependency is created, and an OpenROAD that predates this
// reads the .odb unchanged -- odb serializes properties it does not
// interpret and ignores them.
class SdcInDb
{
 public:
  enum class Kind
  {
    kNone,
    kNative,
    kText
  };

  // Store the current constraints in the block. A block without a linked,
  // constrained design is left untouched.
  static void save(dbSta* sta, odb::dbBlock* block);
  // Remove any stored constraints from the block.
  static void clear(odb::dbBlock* block);
  // Restore the constraints stored in the block, if the design is linked
  // with liberty. Returns the kind that was restored; kNone means nothing
  // was done, because the block carries no constraints or because there
  // is no timing network to restore them into (the record is left as is).
  static Kind restore(dbSta* sta, odb::dbBlock* block);
  // Which encoding the block carries, without restoring it.
  static Kind kind(odb::dbBlock* block);
  static const char* kindName(Kind kind);

  static constexpr const char* kNativeProperty = "sta.sdc.native";
  static constexpr const char* kTextProperty = "sta.sdc";
};

}  // namespace sta
