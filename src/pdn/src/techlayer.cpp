//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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

#include "techlayer.h"

namespace pdn {

TechLayer::TechLayer(odb::dbTechLayer* layer) : layer_(layer)
{
}

int TechLayer::getSpacing(int width, int length) const
{
  // get the spacing the DB would use
  const int db_spacing = layer_->getSpacing(width, length);

  // Check the two widths table for spacing assuming same width metal
  const int two_widths_spacing = layer_->findTwSpacing(width, width, length);

  return std::max(db_spacing, two_widths_spacing);
}

int TechLayer::micronToDbu(const std::string& value) const
{
  return micronToDbu(std::stof(value));
}

int TechLayer::micronToDbu(double value) const
{
  return value * getLefUnits();
}

std::vector<std::string> TechLayer::tokenizeStringProperty(
    const std::string& property_name) const
{
  auto* property = odb::dbStringProperty::find(layer_, property_name.c_str());
  if (property == nullptr) {
    return {};
  }
  std::vector<std::string> tokenized;
  std::string token;
  for (const char& c : property->getValue()) {
    if (std::isspace(c)) {
      if (!token.empty()) {
        tokenized.push_back(token);
        token.clear();
      }
      continue;
    }

    if (c == ';') {
      // end
      break;
    }

    token += c;
  }

  if (!token.empty()) {
    tokenized.push_back(token);
  }

  return tokenized;
}

}  // namespace pdn
