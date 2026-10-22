// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <mutex>
#include <set>
#include <string>

namespace web {

// Each Renderer display control's visibility, by its "Group/Name" path.
// Read from the render threads, so every access is locked.
class RendererControls
{
 public:
  using Values = std::map<std::string, bool>;

  // A path nobody set or seeded reads as `default_val`.
  bool check(const std::string& path, bool default_val = true) const;
  void set(const std::string& path, bool value);
  // Sets `path` unless it already has a value, so a renderer registering
  // again keeps what the user chose.
  void seed(const std::string& path, bool value);
  // True the first time `renderer` is offered for seeding.
  bool markSeeded(const void* renderer);

  Values values() const;
  // Puts back the paths in `values` that still have a value here.
  void restore(const Values& values);

 private:
  mutable std::mutex mutex_;
  Values visible_;
  std::set<const void*> seeded_;
};

// Installs each registered renderer's own control defaults, once per
// renderer.  Defined in web.cpp, which may reach web::Gui.
void seedRendererControls(RendererControls& controls);

// Turning on the control at `path` turns off the siblings it is mutually
// exclusive with, as DisplayControls::itemChanged does.  Defined in web.cpp.
void applyRendererControlExclusivity(RendererControls& controls,
                                     const std::string& path);
// Sets the control at `path`, with the exclusivity above when turned on.
void setRendererControl(RendererControls& controls,
                        const std::string& path,
                        bool value);

}  // namespace web
