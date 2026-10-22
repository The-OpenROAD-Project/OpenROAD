// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "display_control_tree.h"

#include <fnmatch.h>

#include <optional>
#include <string>

#include "odb/db.h"
#include "odb/dbTypes.h"
#include "tile_generator.h"

namespace web {

namespace {

constexpr const char* kLayers = "Layers/";
constexpr const char* kImplantLayers = "Layers/Implant";
constexpr const char* kOtherLayers = "Layers/Other";
constexpr const char* kRows = "Rows";

using Flag = bool TileVisibility::*;

// A leaf of DisplayControls' constructor, in its order, and the tile fields it
// drives.  A `selectable` row's box starts as `visible`, as in makeLeafItem.
struct LeafSpec
{
  const char* path;
  bool visible;
  bool selectable;
  Flag vis_field;
  Flag sel_field;
};

using TV = TileVisibility;

// clang-format off
// NOLINTBEGIN(modernize-use-designated-initializers,readability-trailing-comma)
const LeafSpec kLeafSpecs[] = {
  {"Layers/Implant", true,  true,  nullptr, nullptr},
  {"Layers/Other",   false, true,  nullptr, nullptr},

  {"Nets/Signal",  true, true, &TV::net_signal,  &TV::net_signal_selectable},
  {"Nets/Power",   true, true, &TV::net_power,   &TV::net_power_selectable},
  {"Nets/Ground",  true, true, &TV::net_ground,  &TV::net_ground_selectable},
  {"Nets/Clock",   true, true, &TV::net_clock,   &TV::net_clock_selectable},
  {"Nets/Reset",   true, true, &TV::net_reset,   &TV::net_reset_selectable},
  {"Nets/Tie off", true, true, &TV::net_tieoff,  &TV::net_tieoff_selectable},
  {"Nets/Scan",    true, true, &TV::net_scan,    &TV::net_scan_selectable},
  {"Nets/Analog",  true, true, &TV::net_analog,  &TV::net_analog_selectable},

  {"Instances/StdCells/Buffers/Inverters/Timing opt.", true, true,
   &TV::std_bufinv_timing, &TV::std_bufinv_timing_selectable},
  {"Instances/StdCells/Buffers/Inverters/Netlist", true, true,
   &TV::std_bufinv, &TV::std_bufinv_selectable},
  {"Instances/StdCells/Combinational", true, true,
   &TV::std_combinational, &TV::std_combinational_selectable},
  {"Instances/StdCells/Sequential", true, true,
   &TV::std_sequential, &TV::std_sequential_selectable},
  {"Instances/StdCells/Clock tree/Buffer/Inverter", true, true,
   &TV::std_clock_bufinv, &TV::std_clock_bufinv_selectable},
  {"Instances/StdCells/Clock tree/Clock gate", true, true,
   &TV::std_clock_gate, &TV::std_clock_gate_selectable},
  {"Instances/StdCells/Level shifter", true, true,
   &TV::std_level_shift, &TV::std_level_shift_selectable},
  {"Instances/Macro", true, true, &TV::macros, &TV::macros_selectable},
  {"Instances/Pads/Input",   true, true, &TV::pad_input,  &TV::pad_input_selectable},
  {"Instances/Pads/Output",  true, true, &TV::pad_output, &TV::pad_output_selectable},
  {"Instances/Pads/Inout",   true, true, &TV::pad_inout,  &TV::pad_inout_selectable},
  {"Instances/Pads/Power",   true, true, &TV::pad_power,  &TV::pad_power_selectable},
  {"Instances/Pads/Spacer",  true, true, &TV::pad_spacer, &TV::pad_spacer_selectable},
  {"Instances/Pads/Area IO", true, true, &TV::pad_areaio, &TV::pad_areaio_selectable},
  {"Instances/Pads/Other",   true, true, &TV::pad_other,  &TV::pad_other_selectable},
  {"Instances/Physical/Fill cell",    true, true, &TV::phys_fill,    &TV::phys_fill_selectable},
  {"Instances/Physical/Endcap",       true, true, &TV::phys_endcap,  &TV::phys_endcap_selectable},
  {"Instances/Physical/Welltap",      true, true, &TV::phys_welltap, &TV::phys_welltap_selectable},
  {"Instances/Physical/Tie high/low", true, true, &TV::phys_tie,     &TV::phys_tie_selectable},
  {"Instances/Physical/Antenna",      true, true, &TV::phys_antenna, &TV::phys_antenna_selectable},
  {"Instances/Physical/Cover",        true, true, &TV::phys_cover,   &TV::phys_cover_selectable},
  {"Instances/Physical/Bump",         true, true, &TV::phys_bump,    &TV::phys_bump_selectable},
  {"Instances/Physical/Other",        true, true, &TV::phys_other,   &TV::phys_other_selectable},

  {"Blockages/Placement", true, true,
   &TV::placement_blockages, &TV::placement_blockages_selectable},
  {"Blockages/Routing", true, true,
   &TV::routing_obstructions, &TV::routing_obstructions_selectable},

  {"Rulers", true,  true, nullptr, nullptr},
  {kRows,    false, true, nullptr, nullptr},

  {"Tracks/Pref",     false, false, &TV::tracks_pref,     nullptr},
  {"Tracks/Non Pref", false, false, &TV::tracks_non_pref, nullptr},

  {"Shape Types/Routing/Segments",         true,  false, &TV::routing_segments,  nullptr},
  {"Shape Types/Routing/Vias",             true,  false, &TV::routing_vias,      nullptr},
  {"Shape Types/Special Routing/Segments", true,  false, &TV::srouting_segments, nullptr},
  {"Shape Types/Special Routing/Vias",     true,  false, &TV::srouting_vias,     nullptr},
  {"Shape Types/Pins",                     true,  true,  &TV::pins,  &TV::pins_selectable},
  {"Shape Types/Pin Names",                true,  false, &TV::pin_names,         nullptr},
  {"Shape Types/Fills",                    false, false, &TV::fills,             nullptr},

  {"Misc/Instances/Names",     true,  false, &TV::inst_names,     nullptr},
  {"Misc/Instances/Pins",      true,  true,  &TV::inst_pins,      &TV::inst_pins_selectable},
  {"Misc/Instances/Pin Names", false, false, &TV::inst_pin_names, nullptr},
  {"Misc/Instances/Blockages", true,  false, &TV::blockages,      nullptr},
  {"Misc/Scale bar",           true,  false, nullptr,             nullptr},
  {"Misc/Access points",       false, false, &TV::access_points,  nullptr},
  {"Misc/Regions",             true,  true,  &TV::regions,        nullptr},
  {"Misc/Detailed view",       false, false, &TV::detailed,       nullptr},
  {"Misc/Highlight selected",  true,  false, nullptr,             nullptr},
  {"Misc/Module view",         false, false, nullptr,             nullptr},
  {"Misc/Manufacturing grid",  false, false, &TV::mfg_grid,       nullptr},
  {"Misc/GCell grid",          false, false, &TV::gcell_grid,     nullptr},
  {"Misc/Flywires only",       false, false, nullptr,             nullptr},
  {"Misc/Focused nets guides", false, false, nullptr,             nullptr},
  {"Misc/Labels",              true,  true,  &TV::labels,         nullptr},
};
// NOLINTEND(modernize-use-designated-initializers,readability-trailing-comma)
// clang-format on

}  // namespace

DisplayControlTree::DisplayControlTree()
{
  for (const LeafSpec& spec : kLeafSpecs) {
    Leaf leaf{.visible = spec.visible, .selectable = std::nullopt};
    if (spec.selectable) {
      leaf.selectable = spec.visible;
    }
    leaves_[spec.path] = leaf;
  }
  // No checkbox: the panel only sets its colour.
  leaves_["Misc/Background"] = Leaf{};
}

void DisplayControlTree::addLeaf(const std::string& path, const Leaf& leaf)
{
  leaves_.insert({path, leaf});
}

DisplayControlTree::Leaf* DisplayControlTree::find(const std::string& path)
{
  const auto it = leaves_.find(path);
  return it == leaves_.end() ? nullptr : &it->second;
}

void DisplayControlTree::addDesignRows(odb::dbDatabase* db)
{
  if (db == nullptr) {
    return;
  }

  // DisplayControls::addTech: routing and cut layers directly under Layers,
  // implant under Implant, and the rest under Other, which starts off.
  for (odb::dbTech* tech : db->getTechs()) {
    for (odb::dbTechLayer* layer : tech->getLayers()) {
      std::string parent;
      bool on = true;
      switch (layer->getType()) {
        case odb::dbTechLayerType::ROUTING:
        case odb::dbTechLayerType::CUT:
          parent = kLayers;
          break;
        case odb::dbTechLayerType::IMPLANT:
          parent = std::string(kImplantLayers) + "/";
          leaves_.erase(kImplantLayers);
          break;
        default:
          parent = std::string(kOtherLayers) + "/";
          leaves_.erase(kOtherLayers);
          on = false;
          break;
      }
      addLeaf(parent + layer->getName(), {.visible = on, .selectable = on});
    }
  }

  // DisplayControls::libInit: a site starts as Rows is when it is added.
  for (odb::dbLib* lib : db->getLibs()) {
    for (odb::dbSite* site : lib->getSites()) {
      const std::string path = std::string(kRows) + "/" + site->getName();
      if (leaves_.contains(path)) {
        continue;
      }
      bool on = true;
      if (const Leaf* rows = find(kRows)) {
        on = rows->visible.value_or(false);
      } else {
        for (const auto& [other, leaf] : leaves_) {
          if (other.starts_with(std::string(kRows) + "/")) {
            on = on && leaf.visible.value_or(false);
          }
        }
      }
      leaves_.erase(kRows);
      addLeaf(path, {.visible = on, .selectable = on});
    }
  }
}

bool DisplayControlTree::matches(const std::string& pattern,
                                 const std::string& path)
{
  // No FNM_PATHNAME, so '*' crosses '/' as in Qt.
  return fnmatch(pattern.c_str(), path.c_str(), FNM_CASEFOLD) == 0;
}

bool DisplayControlTree::visible(const std::string& path) const
{
  const auto it = leaves_.find(path);
  return it != leaves_.end() && it->second.visible.value_or(false);
}

bool DisplayControlTree::selectable(const std::string& path) const
{
  const auto it = leaves_.find(path);
  return it != leaves_.end() && it->second.selectable.value_or(false);
}

void DisplayControlTree::apply(TileVisibility& vis) const
{
  for (const LeafSpec& spec : kLeafSpecs) {
    if (spec.vis_field != nullptr) {
      vis.*spec.vis_field = visible(spec.path);
    }
    if (spec.sel_field != nullptr) {
      vis.*spec.sel_field = selectable(spec.path);
    }
  }

  // Qt draws by a parent row's state, which is on while any leaf under it is.
  // dbSta's STD_PHYSICAL and STD_OTHER go by Instances/Physical and Instances.
  vis.stdcells = false;
  vis.stdcells_selectable = false;
  vis.std_physical = false;
  vis.std_physical_selectable = false;
  vis.std_other = false;
  vis.std_other_selectable = false;
  for (const auto& [path, leaf] : leaves_) {
    if (!path.starts_with("Instances/")) {
      continue;
    }
    const bool on = leaf.visible.value_or(false);
    const bool selectable_on = leaf.selectable.value_or(false);
    if (path.starts_with("Instances/StdCells/")) {
      vis.stdcells = vis.stdcells || on;
      vis.stdcells_selectable = vis.stdcells_selectable || selectable_on;
    }
    if (path.starts_with("Instances/Physical/")) {
      vis.std_physical = vis.std_physical || on;
      vis.std_physical_selectable
          = vis.std_physical_selectable || selectable_on;
    }
    vis.std_other = vis.std_other || on;
    vis.std_other_selectable = vis.std_other_selectable || selectable_on;
  }
  vis.routing = vis.routing_segments || vis.routing_vias;
  vis.special_nets = vis.srouting_segments || vis.srouting_vias;
  vis.pin_markers = vis.pins;

  vis.rows = visible(kRows);
  vis.sites.clear();
  vis.site_selectable.clear();
  vis.visible_layers.clear();
  vis.selectable_layers.clear();
  const std::string rows_prefix = std::string(kRows) + "/";
  for (const auto& [path, leaf] : leaves_) {
    if (path.starts_with(rows_prefix)) {
      const std::string site = path.substr(rows_prefix.size());
      vis.sites[site] = leaf.visible.value_or(false);
      vis.site_selectable[site] = leaf.selectable.value_or(false);
      vis.rows = vis.rows || leaf.visible.value_or(false);
    } else if (path.starts_with(kLayers) && path != kImplantLayers
               && path != kOtherLayers) {
      const std::string layer = path.substr(path.rfind('/') + 1);
      if (leaf.visible.value_or(false)) {
        vis.visible_layers.insert(layer);
      }
      if (leaf.selectable.value_or(false)) {
        vis.selectable_layers.insert(layer);
      }
    }
  }
  vis.has_visible_layers = true;
  vis.has_selectable_layers = true;
}

}  // namespace web
