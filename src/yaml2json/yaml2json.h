// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <string>

#include "yaml-cpp/yaml.h"

namespace yaml2json {

// Writes node as JSON, keys in document order, indented by two spaces.
//
// Plain scalars resolve as PyYAML's yaml.safe_load resolves them (YAML 1.1):
// null, ~ and the empty scalar are null; true/false/yes/no/on/off are
// booleans; decimal integers and floats with a '.' are numbers; anything else
// is a string. Quoted scalars are strings.
//
// Throws std::runtime_error on input whose meaning PyYAML and this converter
// could disagree on, or that JSON cannot hold: a duplicate or non-scalar key,
// an octal, hexadecimal, sexagesimal or underscored integer, .inf/.nan, or a
// tag other than the core schema's.
std::string toJson(const YAML::Node& node);

// Parses text, which must hold exactly one YAML document, and converts it.
std::string yamlToJson(const std::string& text);

}  // namespace yaml2json
