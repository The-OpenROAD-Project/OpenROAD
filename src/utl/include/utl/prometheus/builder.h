// MIT License

// Copyright (c) 2021 biaks (ianiskr@gmail.com)

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <map>
#include <string>

#include "utl/prometheus/family.h"
#include "utl/prometheus/registry.h"

namespace utl {

template <typename CustomMetric>
class Builder
{
  Family::Labels labels_;
  std::string name_;
  std::string help_;

 public:
  Builder& Labels(const std::map<const std::string, const std::string>& labels)
  {
    labels_ = labels;
    return *this;
  }
  Builder& Name(const std::string& name)
  {
    name_ = name;
    return *this;
  }
  Builder& Help(const std::string& help)
  {
    help_ = help;
    return *this;
  }
  CustomFamily<CustomMetric>& Register(PrometheusRegistry& registry)
  {
    return registry.Add<CustomFamily<CustomMetric>>(name_, help_, labels_);
  }
};

}  // namespace utl