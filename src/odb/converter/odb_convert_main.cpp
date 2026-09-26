// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

// One hop of an .odb schema upgrade.
//
// This file is compiled once per snapshot in //src/odb/converter/BUILD, each
// time against a frozen OpenROAD source archive rather than against the
// checked-out tree. A build of snapshot N therefore reads every schema
// revision that OpenROAD N could read and writes schema N, which is the
// whole trick: the conversion code is not maintained, it is simply the code
// that already shipped, kept buildable.
//
// Reading takes a path rather than stdin because dbIStream rewinds its
// unconsumed buffer tail when it is destroyed, which a pipe cannot do.
// Writing goes to stdout, which is safe: dbOStream only ever appends.
//
// Keep this file to the public dbDatabase API. It is the one piece of the
// converter that master owns and that must compile against every snapshot
// in the support window, so every symbol it names is a compatibility
// commitment.

#include <unistd.h>

#include <cstdio>
#include <exception>
#include <fstream>
#include <ostream>

#include "boost/iostreams/device/file_descriptor.hpp"
#include "boost/iostreams/stream.hpp"
#include "odb/db.h"
#include "utl/Logger.h"

namespace {

// utl::Logger unconditionally installs a stdout sink, and odb reports
// through it while reading, so anything the read logs would land in the
// middle of the database we are writing. Move the real stdout somewhere
// private and point fd 1 at stderr, so every existing stdout writer in the
// snapshot -- the logger, a stray printf -- becomes a diagnostic and only
// this file can reach the output stream.
int claimStdout()
{
  const int saved = dup(STDOUT_FILENO);
  if (saved == -1) {
    return -1;
  }
  if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1) {
    close(saved);
    return -1;
  }
  return saved;
}

}  // namespace

int main(int argc, char* argv[])
{
  if (argc != 2) {
    std::fprintf(stderr,
                 "usage: %s <input.odb>\n"
                 "Writes the converted database to stdout.\n",
                 argv[0]);
    return 2;
  }

  const int out_fd = claimStdout();
  if (out_fd == -1) {
    std::fprintf(stderr, "error: cannot take over stdout\n");
    return 1;
  }

  try {
    utl::Logger logger;

    std::ifstream in(argv[1], std::ios::binary);
    if (!in) {
      std::fprintf(stderr, "error: cannot open %s\n", argv[1]);
      return 1;
    }

    odb::dbDatabase* db = odb::dbDatabase::create();
    db->setLogger(&logger);
    db->read(in);

    boost::iostreams::stream<boost::iostreams::file_descriptor_sink> out(
        boost::iostreams::file_descriptor_sink(out_fd,
                                               boost::iostreams::close_handle));
    db->write(out);
    out.flush();
    if (!out) {
      std::fprintf(stderr, "error: writing converted database failed\n");
      return 1;
    }
  } catch (const std::exception& e) {
    std::fprintf(stderr, "error: conversion failed: %s\n", e.what());
    return 1;
  }

  return 0;
}
