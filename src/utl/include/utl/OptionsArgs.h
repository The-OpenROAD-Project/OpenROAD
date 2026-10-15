// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Command-line switches generated from an options schema.
//
// A command whose settings are an options struct takes `-options dict`, the
// whole struct as a nested dictionary, and one switch per field:
// a key with a value for a number, string or list
// (`-timing.nets_percentage 15`), and a flag for a bool
// (`-timing.enabled`, or `-no_pin_density_adjust` for a bool that defaults
// to true).  The switch name is the field's dotted path, so the groups read
// apart from the words of a field name, and a field added to any struct,
// including an embedded sub-tool's, is a switch of every command that uses
// it, with no per-command code.  A command may also keep older names as
// aliases.

#pragma once

#include <boost/json.hpp>
#include <functional>
#include <string>
#include <vector>

#include "utl/Validate.h"

namespace utl {

class Logger;

// Converts a value in the user's units to the SI unit the C++ API takes.
using UnitScaler = std::function<double(Unit unit, double value)>;

// Parses the text of -options into the dictionary the schema applies (a
// Tcl dict, say; see utl/OptionsTcl.h).
using DictParser = std::function<boost::json::object(const std::string& text)>;

// The switch that takes the whole struct as a dictionary.
inline constexpr const char* kOptionsSwitch = "-options";

// An older switch name a command keeps accepting.
struct ArgAlias
{
  // The switch, e.g. "-timing_driven".
  std::string name;
  // The dotted path(s) the switch sets.
  std::vector<std::string> paths;
  // A flag takes no value on the command line and sets `value`; a key sets
  // the value that follows it.
  bool is_flag = false;
  std::string value;
  // Replaces paths: builds the dictionary entries from the value by hand,
  // for a switch with no one-to-one field.
  std::function<void(boost::json::object& options, const std::string& value)>
      custom;
};
using ArgAliases = std::vector<ArgAlias>;

// One generated switch.
struct ArgSpec
{
  std::string name;  // "-timing.nets_percentage"
  std::string path;  // "timing.nets_percentage"
  const FieldMeta* meta;
  bool is_flag;     // a bool field: the switch takes no value
  bool flag_value;  // the value a flag sets
};

// The switches of every leaf field of the schema, in declaration order.
std::vector<ArgSpec> argSpecs(const SchemaBase& schema);

// The usage synopsis of the generated switches, for define_cmd_args:
// "[-density density] [-timing_enabled] ...".
std::string argSynopsis(const SchemaBase& schema);

// Parses the switches of a command invocation into the dictionary the
// schema applies: -options dicts, generated switches and aliases, exact name
// or unique prefix, later ones overriding earlier ones.  A value with a unit
// is scaled.  Errors report with the schema's generic id.
boost::json::object parseArgs(const std::vector<std::string>& argv,
                              const SchemaBase& schema,
                              const ArgAliases& aliases,
                              const UnitScaler& scaler,
                              const DictParser& dict_parser,
                              Logger* logger);

// Help for the generated switches, as text and as the README table.
std::string describeArgsText(const SchemaBase& schema);
std::string describeArgsMarkdown(const SchemaBase& schema);

}  // namespace utl
