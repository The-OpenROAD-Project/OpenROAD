// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The options structs of the commands, by command name, so that the Tcl
// layer can parse a command's switches and the generic commands
// (describe_options, report_options) can find them.

#pragma once

#include <string>
#include <vector>

#include "utl/OptionsArgs.h"

namespace utl {

class SchemaBase;

struct OptionsEntry
{
  const SchemaBase* schema;
  ArgAliases aliases;
};

class OptionsRegistry
{
 public:
  static OptionsRegistry& get();

  // Registers the options a command takes, and the older switch names it
  // keeps accepting.  Called once per command when its module is
  // initialized, before its Tcl is evaluated.
  void add(const std::string& command,
           const SchemaBase& schema,
           ArgAliases aliases = {});

  // The entry registered for the command, or nullptr.
  const OptionsEntry* find(const std::string& command) const;

  // The registered command names, sorted.
  std::vector<std::string> commands() const;

 private:
  std::vector<std::pair<std::string, OptionsEntry>> entries_;
};

}  // namespace utl
