// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <ostream>
#include <string_view>

namespace odb {

// Reports where each dbTable slot lands in a .odb as it is written, so a
// tool outside OpenROAD can reformat the file (for instance, to make it
// compress better) and restore it byte for byte. It describes the bytes
// of the one file it was written with and nothing else: it is not part of
// the .odb format, OpenROAD never reads it, and it carries no
// compatibility promise from one OpenROAD revision to the next.
//
// Text, one line per run of consecutive slots of one table that are
// contiguous in the file and of equal length:
//
//   odb-layout 1
//   <table> <offset> <length> <count>
//   ...
//   size <bytes>
//
// A slot is its allocated flag and what follows it: the record written
// by the type's operator<<, or a free slot's list links. A slot whose
// record holds tables of its own (a chip's, a block's) is not listed; the
// slots inside it are. Bytes not covered by a run (headers, hash tables,
// vectors) are the file's own.
class dbStreamLayout
{
 public:
  explicit dbStreamLayout(std::ostream& out);

  void addSlot(const char* table, uint64_t offset, uint64_t length);

  // Slots added so far: a slot that saw this change while it was written
  // holds tables of its own.
  uint64_t slotCount() const { return slots_; }

  // Ends the report; size is the length of the file it describes.
  void finish(uint64_t size);

 private:
  void flushRun();

  std::ostream& out_;
  std::string_view table_;
  uint64_t offset_ = 0;
  uint64_t length_ = 0;
  uint64_t count_ = 0;
  uint64_t slots_ = 0;
};

}  // namespace odb
