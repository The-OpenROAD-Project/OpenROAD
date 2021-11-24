/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace utl {
class Logger;
}

namespace rmp {

enum class GateType
{
  None,
  Mlatch,
  Gate
};

struct Gate
{
  GateType type_;
  std::string master_;
  std::vector<std::string> connections_;
  Gate(const GateType& type,
       const std::string& master,
       const std::vector<std::string>& connections)
      : type_(type), master_(master), connections_(connections)
  {
  }
};

class BlifParser
{
 public:
  BlifParser();
  bool parse(std::string& file_contents);
  void addInput(const std::string& input);
  void addOutput(const std::string& output);
  void addClock(const std::string& clock);
  void addNewInstanceType(const std::string& type);
  void addNewGate(const std::string& cell_name);
  void addConnection(const std::string& connection);
  void endParser();

  const std::vector<std::string>& getInputs() const;
  const std::vector<std::string>& getOutputs() const;
  const std::vector<std::string>& getClocks() const;
  const std::vector<Gate>& getGates() const;
  int getCombGateCount() const;
  int getFlopCount() const;

 private:
  std::vector<std::string> inputs_;
  std::vector<std::string> outputs_;
  std::vector<std::string> clocks_;
  std::vector<Gate> gates_;
  std::string currentGate_;
  GateType currentInstanceType_;
  std::vector<std::string> currentConnections_;
  int combCount_;
  int flopCount_;
};

}  // namespace rmp
