// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// SPEF from a block's ODB parasitics: cap nodes with their own
// capacitance (foreign), resistor segments and coupling segments, as
// Extractor writes them.

#pragma once

#include <ostream>
#include <string>
#include <vector>

namespace odb {
class dbBlock;
class dbNet;
}  // namespace odb

namespace pex {

struct SpefOptions
{
  // The nets written; empty for every net with an RC network.
  std::vector<odb::dbNet*> nets;
  // The *DATE and *VERSION lines; the current time when date is empty.
  std::string date;
  std::string version;
  int corner = 0;
};

// Writes one corner in IEEE 1481-1999 form, capacitance in fF and
// resistance in ohms. Nets and instances are named through the name map,
// pins as instance:pin, ports by name, and internal nodes as net:node with
// the node's junction id. Each coupling appears under both of its nets.
// Throws std::invalid_argument for a net in `nets` without an RC network.
void writeSpef(odb::dbBlock* block,
               std::ostream& out,
               const SpefOptions& options = {});

}  // namespace pex
