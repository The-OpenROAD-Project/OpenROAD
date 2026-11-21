// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Builds a Module by appending objects, for importers and passes that rebuild
// a whole module; unlike the Editor, it keeps no fanout lists. Forward
// references go through placeholders. finish() resolves them, cuts
// combinational cycles with loop pairs and reports the first error (later
// calls are ignored). Not thread-safe.

#pragma once

#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "syn/lir/ConstView.h"
#include "syn/lir/Design.h"
#include "syn/lir/Ids.h"
#include "syn/lir/Module.h"
#include "syn/lir/ObjAttributes.h"
#include "syn/lir/Op.h"
#include "syn/lir/RegClassPool.h"
#include "syn/lir/SourceLoc.h"

namespace syn::lir {

class ModuleBuilder
{
 public:
  // Half of the id space: ids with the top bit set are placeholders.
  // A larger module fails with ResourceExhausted.
  static constexpr ObjIndex kMaxObjects
      = (ObjIndex{1} << (kObjIdBits - 1u)) - 1;

  // Construction and reserve.
  ModuleBuilder() = default;
  explicit ModuleBuilder(NameId name);
  // Rebuild of `src`, which must stay unchanged until install(): starts with
  // its name, flags and inputs (input()). Add or copy objects, add the
  // outputs in port order, then install().
  explicit ModuleBuilder(const Module& src);
  ModuleBuilder(ModuleBuilder&&) noexcept = default;
  ModuleBuilder& operator=(ModuleBuilder&&) noexcept = default;
  ModuleBuilder(const ModuleBuilder&) = delete;
  ModuleBuilder& operator=(const ModuleBuilder&) = delete;
  void reserve(int64_t objects, int64_t fanin_slots);

  // Ports.
  // Adds a module input at the next port index. Port names must be valid and
  // unique among all ports; finish() fails otherwise.
  ObjId addPi(NameId name,
              int width,
              Signedness signedness = Signedness::kUnsigned);
  // Adds a module output driven by `driver`, at the next port index. In a
  // rebuild, an invalid `name` keeps the source port's name.
  ObjId addPo(NameId name, ObjId driver);

  // Objects: gates, constants, registers and loop pairs.
  // Adds any object except ports, constants, register and loop halves, and
  // memory ops, which have their own methods (or none yet).
  ObjId add(Op op,
            int width,
            std::span<const ObjId> fanins,
            uint32_t data = 0,
            Signedness signedness = Signedness::kUnsigned);
  ObjId add(Op op,
            int width,
            std::initializer_list<ObjId> fanins,
            uint32_t data = 0,
            Signedness signedness = Signedness::kUnsigned);
  // Inline only: `width` must be at most kMaxInlineConstWidth. Bits above the
  // width are ignored.
  ObjId addConst(int width, uint32_t bits, uint32_t x_mask = 0);
  // Any width. Wider values are interned in `pool`; a new value in a
  // read-only pool fails (FailedPrecondition).
  ObjId addConst(ConstView value, ConstPool& pool);
  // Copies kConst `obj` of `from`, a module of the same Design. Needs no pool
  // lookup, so it works while the design is read-only.
  ObjId copyConst(const Module& from, ObjId obj);
  // Register classes for addRegisterOut(). A rebuild starts with a copy of
  // the source's classes, so source class ids stay valid.
  RegClassPool& regClasses() { return module_.regClasses(); }
  // A register is added in two steps: the output (Q) first, usable as a fanin
  // right away, then the input once its pin drivers exist. `pins` are the
  // present pin types, in RegPinType order. An invalid `reg_class` fails
  // (FailedPrecondition).
  ObjId addRegisterOut(int width,
                       RegClassId reg_class,
                       Signedness signedness = Signedness::kUnsigned);
  void setRegisterClass(ObjId reg_out, RegClassId reg_class);
  ObjId addRegisterIn(ObjId reg_out, std::span<const ObjId> pins);
  ObjId addRegisterIn(ObjId reg_out, std::initializer_list<ObjId> pins);
  // A loop pair (a wire cut to break a combinational cycle) is added the same
  // way. `driver` must have the loop output's width.
  ObjId addLoopOut(int width, Signedness signedness = Signedness::kUnsigned);
  ObjId addLoopIn(ObjId loop_out, ObjId driver);

  // Placeholders.
  // Forward references. A placeholder is driven as a whole, or piecewise by
  // drivers of bits [lsb, lsb + width(driver)). Each bit is driven at most
  // once; finish() concatenates the pieces.
  ObjId addPlaceholder(int width);
  void drivePlaceholder(ObjId placeholder, ObjId driver);
  void drivePlaceholder(ObjId placeholder, int lsb, ObjId driver);
  static bool isPlaceholder(ObjId id);
  // When true, finish() drives placeholder bits that nothing drives with
  // kUndriven objects instead of failing. Default: false.
  void setAllowUndriven(bool allow) { allow_undriven_ = allow; }

  // Metadata: module and object names, attributes and source locations.
  void setModuleName(NameId name);
  // Module attributes; isOrdered() is computed by finish().
  void setModuleAttributes(const ModuleAttributes& attributes);
  // Source location of the module declaration.
  void setModuleSourceLoc(SourceLoc loc);
  // Names an object or a placeholder; on a port this renames it. A
  // placeholder's name goes to its driver unless the driver already has one.
  void setName(ObjId id, NameId name);
  // Adds `attrs` to an object or a placeholder. A placeholder's attributes
  // are added to its driver's.
  void addObjAttributes(ObjId id, ObjAttributes attrs);
  // Sets the source location of an object or a placeholder. A placeholder's
  // location goes to its driver unless the driver already has one.
  void setObjSourceLoc(ObjId id, SourceLoc loc);

  // Reading back added objects.
  // Width of an object or placeholder added to this builder.
  int width(ObjId id) const;
  // Operation, constant value, name and source location of a non-placeholder
  // object.
  Op op(ObjId id) const { return module_.op(id); }
  // Fanins of a non-placeholder object (they may be placeholders).
  std::span<const ObjId> fanins(ObjId id) const ABSL_ATTRIBUTE_LIFETIME_BOUND;
  ConstView constValue(ObjId id, const ConstPool& pool) const;
  NameId objName(ObjId id) const { return module_.objName(id); }
  SourceLoc objSourceLoc(ObjId id) const { return module_.objSourceLoc(id); }
  // Module::takesNetName() of a non-placeholder object.
  bool takesNetName(ObjId id) const;
  // Objects added so far (placeholders excluded).
  ObjIndex size() const { return module_.size(); }

  // Copying from a source module (rebuild only: ModuleBuilder(const Module&)).
  // The new Pi for input port `port` of the source module.
  ObjId input(int port) const;
  // Copies `src_obj` (with its name, attributes and location) with new
  // `fanins` from this builder. A Pi returns input(port); outputs must be
  // copied in port order; state outputs take no fanins (copying their input
  // sets the link). Memory ops are not supported yet.
  ObjId copy(ObjId src_obj, std::span<const ObjId> fanins);
  ObjId copy(ObjId src_obj, std::initializer_list<ObjId> fanins);
  // The copy of `src_obj` so far, or an invalid id.
  ObjId copyOf(ObjId src_obj) const;

  // Finish and install.
  // Resolves placeholders, cuts every combinational cycle with a loop pair,
  // and returns the module, or the first error.
  absl::StatusOr<Module> finish() &&;
  // Replaces module `id` of `design` with the rebuild and returns the
  // source-to-new id map (invalid for objects not copied). On failure (build,
  // port count or widths, well-formedness) the design is unchanged. The
  // caller keeps the hierarchy acyclic.
  absl::StatusOr<IdRemap> install(Design& design, ModId id) &&;

  // Status.
  bool ok() const { return error_.empty(); }

 private:
  static constexpr ObjIndex kPlaceholderBit = ObjIndex{1} << (kObjIdBits - 1u);

  // A driver of bits [lsb, lsb + width(driver)) of a placeholder.
  struct Piece
  {
    ObjIndex placeholder = 0;  // placeholder index
    int32_t lsb = 0;
    ObjId driver;
  };
  static_assert(sizeof(Piece) == 3 * sizeof(ObjIndex),
                "Piece must stay 12 bytes (24 with 64-bit object ids)");

  ObjId append(Op op,
               int width,
               std::span<const ObjId> fanins,
               ObjIndex data,
               bool is_signed);
  bool checkFanins(std::span<const ObjId> fanins);
  bool isKnown(ObjId id) const;
  ObjIndex placeholderIndex(ObjId id) const;
  // Turns pieces and undriven bits into drivers; false on error.
  bool assemblePlaceholders();
  // Cuts each back edge `u -> w` of a DFS over fanins with a loop pair: `u`
  // reads the LoopOut and the LoopIn reads `w`.
  void cutCombinationalLoops();
  // Every port has a valid name, unique among the ports; false on error.
  bool checkPortNames();
  // The real object each placeholder resolves to, following chains.
  absl::StatusOr<std::vector<ObjId>> resolvePlaceholderChains() const;
  // Gives placeholder names, attributes and locations to their drivers.
  void applyPlaceholderMetadata(std::span<const ObjId> final_driver);
  void rewritePlaceholderFanins(std::span<const ObjId> final_driver);
  // No fanin points forward; true without placeholder fanins.
  bool faninsAreOrdered() const;
  bool copyTakesNoFanins(const ObjRecord& r,
                         ObjId src_obj,
                         std::span<const ObjId> fanins);
  // The op-specific part of copy().
  ObjId copyByOp(const ObjRecord& r,
                 ObjId src_obj,
                 std::span<const ObjId> fanins);
  void fail(std::string message,
            absl::StatusCode code = absl::StatusCode::kInvalidArgument);
  void failTooLarge();  // the module is full (ResourceExhausted)
  // An invalid RegClassId (FailedPrecondition).
  void failInvalidRegClass(const char* what);
  absl::Status error() const { return absl::Status(error_code_, error_); }

  Module module_;
  std::vector<int> placeholder_width_;
  std::vector<ObjId> placeholder_driver_;       // whole-width driver
  std::vector<uint8_t> placeholder_has_piece_;  // driven piecewise
  std::vector<Piece> pieces_;
  std::vector<std::pair<ObjIndex, NameId>> placeholder_names_;
  std::vector<std::pair<ObjIndex, ObjAttributes>> placeholder_attrs_;
  std::vector<std::pair<ObjIndex, SourceLoc>> placeholder_locs_;
  bool has_placeholder_fanins_ = false;
  bool allow_undriven_ = false;
  int64_t open_pairs_ = 0;  // RegOuts/LoopOuts still waiting for their input
  std::string error_;
  absl::StatusCode error_code_ = absl::StatusCode::kOk;
  // Rebuild only.
  const Module* src_ = nullptr;
  std::vector<ObjId> src_map_;  // by source object: its copy, or invalid
};

// Inline definitions.

inline ObjId ModuleBuilder::add(Op op,
                                int width,
                                std::initializer_list<ObjId> fanins,
                                uint32_t data,
                                Signedness signedness)
{
  return add(
      op, width, std::span(fanins.begin(), fanins.size()), data, signedness);
}

inline ObjId ModuleBuilder::addRegisterIn(ObjId reg_out,
                                          std::initializer_list<ObjId> pins)
{
  return addRegisterIn(reg_out, std::span(pins.begin(), pins.size()));
}

inline bool ModuleBuilder::isPlaceholder(ObjId id)
{
  return id.isValid() && (id.value() & kPlaceholderBit) != 0;
}

inline std::span<const ObjId> ModuleBuilder::fanins(ObjId id) const
{
  return module_.fanins(id);
}

inline ConstView ModuleBuilder::constValue(ObjId id,
                                           const ConstPool& pool) const
{
  return module_.constValue(id, pool);
}

inline bool ModuleBuilder::takesNetName(ObjId id) const
{
  return module_.takesNetName(id);
}

inline ObjId ModuleBuilder::copy(ObjId src_obj,
                                 std::initializer_list<ObjId> fanins)
{
  return copy(src_obj, std::span(fanins.begin(), fanins.size()));
}

inline ObjId ModuleBuilder::copyOf(ObjId src_obj) const
{
  return src_obj.value() < src_map_.size() ? src_map_[src_obj.value()]
                                           : ObjId();
}

}  // namespace syn::lir
