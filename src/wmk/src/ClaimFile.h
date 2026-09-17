// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors
#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <ostream>
#include <string>

namespace wmk {

// Staged claim output. Construction validates the destination and opens a
// private temporary file in its parent directory, before embedding can edit
// the design. Only publish() replaces the destination, after a checked close.
// Destruction discards unpublished output; it never commits during unwinding.
class ClaimFile
{
 public:
  explicit ClaimFile(const std::string& path);
  ~ClaimFile();
  ClaimFile(const ClaimFile&) = delete;
  ClaimFile& operator=(const ClaimFile&) = delete;

  void publish(const std::function<void(std::ostream&)>& write);

 private:
  void checkDestination() const;
  void discard() noexcept;

  std::filesystem::path destination_;
  std::filesystem::path directory_;
  std::filesystem::path temporary_;
  std::ofstream stream_;
};

}  // namespace wmk
