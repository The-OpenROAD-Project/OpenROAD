// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The options structs of the commands, by command name, so that generic
// commands (describe_options, default_options, report_options) can find them.

#pragma once

#include <string>
#include <vector>

namespace utl {

class SchemaBase;

class OptionsRegistry
{
 public:
  static OptionsRegistry& get();

  // Registers the schema of the options a command takes.  Called once per
  // command when its module is initialized.
  void add(const std::string& command, const SchemaBase& schema);

  // The schema registered for the command, or nullptr.
  const SchemaBase* find(const std::string& command) const;

  // The registered command names, sorted.
  std::vector<std::string> commands() const;

 private:
  std::vector<std::pair<std::string, const SchemaBase*>> schemas_;
};

}  // namespace utl
