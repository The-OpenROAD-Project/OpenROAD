// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The model file: JSON, with a format version.

#pragma once

#include <iosfwd>
#include <string>

#include "pex/Model.h"

namespace pex {

inline constexpr int kModelFormatVersion = 1;

// What a model was built from: the technology name of its stack and the
// file it came from.
struct ModelSource
{
  std::string technology;
  std::string file;
  std::string fingerprint;  // buildFingerprint of the stack and options
};

// Writes the model with a header: the format version and its source.
// Grids and values are written to seven significant digits. The file
// version writes through a temporary file, so a failure leaves an existing
// file as it was.
void writeModel(const Model& model,
                const ModelSource& source,
                std::ostream& out);
void writeModelFile(const Model& model,
                    const ModelSource& source,
                    const std::string& path);

// Reads a model written by writeModel, and its source when asked. Throws
// std::runtime_error for a file that cannot be read or parsed, or of
// another format version.
Model readModel(std::istream& in, ModelSource* source = nullptr);
Model readModelFile(const std::string& path, ModelSource* source = nullptr);

}  // namespace pex
