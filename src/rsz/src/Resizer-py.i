// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

%{
#include <memory>

#include "rsz/Resizer.hh"

using namespace rsz;
%}

%include "../../Exception-py.i"
%include <typemaps.i>
%include <std_string.i>
%include <std_vector.i>

%ignore rsz::Resizer::Resizer;
%ignore rsz::Resizer::init;
%ignore rsz::Resizer::postReadLiberty;

%ignore rsz::Resizer::setDebugGraphics;

%ignore rsz::Resizer::orderedLoadPinVertices;
%ignore rsz::Resizer::resizeWorstSlackNets;
%ignore rsz::Resizer::resizeNetSlack;
%ignore rsz::Resizer::findFaninFanouts;
%ignore rsz::Resizer::findFanins;

%ignore rsz::Resizer::findFloatingNets;
%ignore rsz::Resizer::findFloatingPins;
%ignore rsz::Resizer::findOverdrivenNets;

%ignore rsz::Resizer::parseMove;
%ignore rsz::Resizer::parseMoveSequence;

%ignore rsz::Resizer::repairSetup(double, double, int, int, int, bool, bool,
                                  const std::vector<rsz::MoveType>&,
                                  const char*,
                                  bool, bool, bool, bool, bool, bool, bool, bool);

%ignore rsz::Resizer::computeNewDelaysSlews;
%ignore rsz::Resizer::estimateSlewsAfterBufferRemoval;
%ignore rsz::Resizer::estimateSlewsInTree;
%ignore rsz::Resizer::annotateInputSlews;
%ignore rsz::Resizer::findDriverSlewForLoad;

%ignore rsz::Resizer::lib_data_;

// removeBuffers(InstanceSeq) requires sta::InstanceSeq which has no Python
// typemap; %extend below exposes the "remove all" case via an empty sequence.
%ignore rsz::Resizer::removeBuffers;

%apply float& OUTPUT { sta::Delay& delay, sta::Slew& slew };

%include "rsz/Resizer.hh"

%extend rsz::Resizer {
  void removeBuffers() { $self->removeBuffers({}); }

  int findFloatingNetsCount()
  {
    std::unique_ptr<sta::NetSeq> nets($self->findFloatingNets());
    return nets ? static_cast<int>(nets->size()) : 0;
  }
  int findFloatingPinsCount()
  {
    std::unique_ptr<sta::PinSet> pins($self->findFloatingPins());
    return pins ? static_cast<int>(pins->size()) : 0;
  }
  int findOverdrivenNetsCount(bool include_parallel_driven)
  {
    std::unique_ptr<sta::NetSeq> nets(
        $self->findOverdrivenNets(include_parallel_driven));
    return nets ? static_cast<int>(nets->size()) : 0;
  }

  bool repairSetup(double setup_margin,
                   double repair_tns_end_percent,
                   int max_passes,
                   int max_iterations,
                   int max_repairs_per_pass,
                   bool match_cell_footprint,
                   bool verbose,
                   const char* sequence,
                   const char* phases,
                   bool skip_pin_swap,
                   bool skip_gate_cloning,
                   bool skip_size_down,
                   bool skip_buffering,
                   bool skip_buffer_removal,
                   bool skip_last_gasp,
                   bool skip_vt_swap,
                   bool skip_crit_vt_swap)
  {
    auto move_seq = rsz::Resizer::parseMoveSequence(
        std::string(sequence ? sequence : ""));
    return $self->repairSetup(setup_margin, repair_tns_end_percent,
                              max_passes, max_iterations, max_repairs_per_pass,
                              match_cell_footprint, verbose, move_seq,
                              phases ? phases : "",
                              skip_pin_swap, skip_gate_cloning, skip_size_down,
                              skip_buffering, skip_buffer_removal,
                              skip_last_gasp, skip_vt_swap, skip_crit_vt_swap);
  }
}
