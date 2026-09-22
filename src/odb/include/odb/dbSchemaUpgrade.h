// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace utl {
class Logger;
}

namespace odb {

// The four-word header of an .odb file, read without parsing the database.
struct DbFileHeader
{
  uint32_t schema_major;
  uint32_t schema_minor;
};

// Returns the header of an .odb file, or nullopt if it cannot be opened or
// does not start with the OpenDB magic. Handles .gz like the reader does.
std::optional<DbFileHeader> peekDbFileHeader(const std::string& filename);

// A converter binary found next to openroad. The name encodes the closed
// range of schema revisions it reads and the revision it writes, so the
// binary itself is the only registry -- openroad does not need a list of
// which conversions exist.
struct SchemaConverter
{
  std::string path;
  uint32_t reads_from;
  uint32_t writes;
};

// Converter binaries visible to this process, in discovery order.
std::vector<SchemaConverter> findSchemaConverters();

// Presents an .odb file as a stream the running build can read.
//
// When the file is new enough, this is just the file. When it predates
// kSchemaOldestReadable, a converter runs as a child process and the stream
// is its output, read through a pipe: no temporary file is written and the
// intermediate database never enters this process's heap.
class DbFileStream
{
 public:
  // Throws std::runtime_error if the file is too old and no converter can
  // bring it forward.
  DbFileStream(const std::string& filename, utl::Logger* logger);
  ~DbFileStream();

  DbFileStream(const DbFileStream&) = delete;
  DbFileStream& operator=(const DbFileStream&) = delete;

  std::istream& stream();

  // Reaps the converter, if one ran, and throws if it failed. Call after
  // reading; the destructor cannot report an error.
  void finish();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace odb
