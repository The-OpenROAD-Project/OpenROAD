// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "renderer_controls.h"

#include <mutex>
#include <string>

namespace web {

bool RendererControls::check(const std::string& path,
                             const bool default_val) const
{
  const std::scoped_lock lock(mutex_);
  const auto it = visible_.find(path);
  return it == visible_.end() ? default_val : it->second;
}

void RendererControls::set(const std::string& path, const bool value)
{
  const std::scoped_lock lock(mutex_);
  visible_[path] = value;
}

void RendererControls::seed(const std::string& path, const bool value)
{
  const std::scoped_lock lock(mutex_);
  visible_.insert({path, value});
}

bool RendererControls::markSeeded(const void* renderer)
{
  const std::scoped_lock lock(mutex_);
  return seeded_.insert(renderer).second;
}

RendererControls::Values RendererControls::values() const
{
  const std::scoped_lock lock(mutex_);
  return visible_;
}

void RendererControls::restore(const Values& values)
{
  const std::scoped_lock lock(mutex_);
  for (const auto& [path, value] : values) {
    if (auto it = visible_.find(path); it != visible_.end()) {
      it->second = value;
    }
  }
}

}  // namespace web
