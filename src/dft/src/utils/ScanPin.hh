///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Google LLC
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
#pragma once

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
  explicit ScanPin(odb::dbITerm* iterm);
  explicit ScanPin(odb::dbBTerm* bterm);
  ScanPin(const ScanPin&) = delete;  // no copy

  odb::dbNet* getNet() const;
  std::string_view getName() const;
  const std::variant<odb::dbITerm*, odb::dbBTerm*>& getValue() const;

 protected:
  std::variant<odb::dbITerm*, odb::dbBTerm*> value_;
};

// Typesafe wrapper for load pins
class ScanLoad : public ScanPin
{
 public:
  explicit ScanLoad(odb::dbITerm* iterm);
  explicit ScanLoad(odb::dbBTerm* bterm);
  ScanLoad(const ScanLoad&) = delete;  // no copy
};

// Typesafe wrapper for driver pins
class ScanDriver : public ScanPin
{
 public:
  explicit ScanDriver(odb::dbITerm* iterm);
  explicit ScanDriver(odb::dbBTerm* bterm);
  ScanDriver(const ScanDriver&) = delete;  // no copy
};

}  // namespace dft
