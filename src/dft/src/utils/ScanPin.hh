// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#pragma once

#include <string_view>
#include <variant>

#include "odb/db.h"

namespace dft {

// Helper struct to match over variants
template <class... Ts>
struct overloaded : Ts...
{
  using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// A Scan Pin can be either iterm or bterm because sometimes we can want to
// connect a pin of a cell to a port or to the pin of another cell
class ScanPin
{
 public:
  explicit ScanPin(std::variant<odb::dbBTerm*, odb::dbITerm*> term);

  odb::dbNet* getNet() const;
  std::string_view getName() const;
  const std::variant<odb::dbBTerm*, odb::dbITerm*>& getValue() const;

 protected:
  std::variant<odb::dbBTerm*, odb::dbITerm*> value_;
};

// Typesafe wrapper for load pins
class ScanLoad : public ScanPin
{
 public:
  explicit ScanLoad(std::variant<odb::dbBTerm*, odb::dbITerm*> term);
};

// Typesafe wrapper for driver pins
class ScanDriver : public ScanPin
{
 public:
  explicit ScanDriver(std::variant<odb::dbBTerm*, odb::dbITerm*> term);
};

}  // namespace dft
