// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#include "ClaimFile.h"

#include <stdlib.h>  // NOLINT(modernize-deprecated-headers): for mkdtemp()

#include <cerrno>
#include <exception>
#include <filesystem>
#include <functional>
#include <ios>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace wmk {
namespace fs = std::filesystem;

ClaimFile::ClaimFile(const std::string& path)
{
  try {
    if (path.empty()) {
      throw std::runtime_error("empty path");
    }
    destination_ = fs::absolute(path);
    checkDestination();
    // mkdtemp creates the directory exclusively with mode 0700. Keeping the
    // stream inside it avoids reopening an unprotected temporary filename.
    std::string pattern
        = (destination_.parent_path() / ".wmk-claims-XXXXXX").string();
    if (mkdtemp(pattern.data()) == nullptr) {
      throw std::system_error(
          errno, std::generic_category(), "cannot create temporary directory");
    }
    directory_ = pattern;
    temporary_ = directory_ / "claims";
    stream_.exceptions(std::ios::failbit | std::ios::badbit);
    stream_.open(temporary_);
  } catch (const std::exception& error) {
    discard();
    throw std::runtime_error("Cannot prepare claims '" + path
                             + "': " + error.what());
  }
}

ClaimFile::~ClaimFile()
{
  discard();
}

void ClaimFile::checkDestination() const
{
  const fs::file_status status = fs::symlink_status(destination_);
  if (fs::exists(status) && !fs::is_regular_file(status)) {
    // In particular, never replace a device or follow an output symlink.
    throw std::runtime_error(
        "destination must be a regular file or a new path");
  }
}

void ClaimFile::publish(const std::function<void(std::ostream&)>& write)
{
  try {
    write(stream_);
    // Closing flushes the last buffered bytes. Check its failure before any
    // rename; the ofstream destructor alone cannot report this to the caller.
    stream_.close();
    checkDestination();
    if (fs::exists(destination_)) {
      fs::permissions(temporary_, fs::status(destination_).permissions());
    }
    // Same-filesystem rename leaves the old file intact on publication error.
    fs::rename(temporary_, destination_);
  } catch (const std::exception& error) {
    throw std::runtime_error("Cannot publish claims '" + destination_.string()
                             + "': " + error.what());
  }
}

void ClaimFile::discard() noexcept
{
  stream_.exceptions(std::ios::goodbit);
  if (stream_.is_open()) {
    stream_.close();
  }
  if (!directory_.empty()) {
    std::error_code error;
    fs::remove_all(directory_, error);
  }
}

}  // namespace wmk
