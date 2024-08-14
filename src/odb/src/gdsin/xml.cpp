///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, Nefelus Inc
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

#include "odb/xml.h"

#include <sstream>
#include <stack>

namespace odb {

XML::XML() = default;

XML::~XML() = default;

void XML::parseXML(const std::string& filename)
{
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Unable to open file: " + filename);
  }

  _name = filename.substr(filename.find_last_of('/') + 1);
  _value = "";

  std::stack<XML*> elementStack;
  std::string line;

  elementStack.push(this);

  while (getline(file, line)) {
    std::stringstream ss(line);
    std::string indent;
    std::string token;
    std::string value;

    getline(ss, indent, '<');
    getline(ss, token, '>');

    if (token.empty()) {
      continue;
    }

    if (token[0] == '/') {  // End tag
      if (!elementStack.empty()) {
        elementStack.pop();
      }
      continue;
    }

    if (token[0] == '?') {  // XML declaration
      continue;
    }

    if (token[token.length() - 1] == '/') {  // Self-closing tag
      XML newElement;
      newElement._name = token.substr(0, token.length() - 1);
      if (!elementStack.empty()) {
        elementStack.top()->_children.push_back(newElement);
      }
    } else {
      XML newElement;
      newElement._name = token;
      if (!elementStack.empty()) {
        elementStack.top()->_children.push_back(newElement);
      }
      elementStack.push(&elementStack.top()->_children.back());
    }

    if (getline(ss, value, '<') && getline(ss, token, '>')) {
      elementStack.top()->_value = value;
      elementStack.pop();
    }
  }

  if (elementStack.size() != 1) {
    // printf("Element stack size: %lu\n", elementStack.size());
    // while(!elementStack.empty()){
    //   printf("Element: %s\n", elementStack.top()->_name.c_str());
    //   elementStack.pop();
    // }
    throw std::runtime_error("Invalid XML file");
  }

  file.close();
}

std::string XML::to_string(int depth) const
{
  std::string indent(depth * 2UL, ' ');
  indent += _name + ": " + _value + "\n";
  for (const auto& child : _children) {
    indent += child.to_string(depth + 1);
  }
  return indent;
}

std::vector<XML>& XML::getChildren()
{
  return _children;
}

std::string XML::getName()
{
  return _name;
}

std::string XML::getValue()
{
  return _value;
}

XML* XML::findChild(const std::string& name)
{
  for (auto& child : _children) {
    if (child.getName() == name) {
      return &child;
    }
  }
  return nullptr;
}

}  // namespace odb
