// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// generate_regfile: the views a parent's flow consumes before the block is
// routed -- an abstract LEF from the placed block and a model liberty --
// and the check that a spec's ports are the RTL module's.
//
// The lib is a model, the way FakeRAM's is, so it lands in seconds inside
// the parent's synthesis: pin loads from fanout, arcs from the known gate
// depth of the read and write paths, a constant leakage per cell. It is
// replaced by the routed block's own abstract for a measured run; the
// numbers here are written so that swap changes as little as possible.
#pragma once

#include <string>
#include <vector>

#include "regfile.h"

namespace odb {
class dbBlock;
}

namespace utl {
class Logger;
}

namespace ram::regfile {

// Abstract LEF of the placed block: outline, pins, obstruction on every
// layer the cells occupy. Bloats the occupied layers into one obstruction,
// as write_abstract_lef -bloat_occupied_layers does.
void WriteLef(odb::dbBlock* block,
              utl::Logger* logger,
              const std::string& path);

// Model liberty for the generated block, see LibModel. `pre_layout` only
// changes the comment: the model has no clock tree to be ideal about.
void WriteLiberty(odb::dbBlock* block,
                  const Spec& spec,
                  const LibModel& model,
                  const std::string& path,
                  bool pre_layout);

// One RTL port as declared: name, width in bits (1 for a scalar), input?
struct RtlPort
{
  std::string name;
  int width = 1;
  bool input = true;
};

// The ports of `module` in a Verilog file, firtool's declaration style:
// ANSI header, and consecutive same-direction ports packed on bare
// continuation lines that inherit direction and width. Throws
// std::runtime_error when the module is not in the file.
std::vector<RtlPort> ReadModulePorts(const std::string& verilog,
                                     const std::string& module);

// Refuses, with every difference named, a spec whose ports are not the
// module's: a port the spec names that the module lacks, a width that
// differs, a direction that differs, or a module port the spec never
// mentions. Returns the list of problems; empty means they agree.
std::vector<std::string> CheckPorts(const Spec& spec,
                                    const std::vector<RtlPort>& ports);

}  // namespace ram::regfile
