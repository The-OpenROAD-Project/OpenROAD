// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <map>
#include <optional>
#include <string>

namespace odb {
class dbDatabase;
}

namespace web {

struct TileVisibility;

// The Qt display-control panel's leaves, by gui::set_display_controls paths
// and with Qt's defaults.  As in Qt, a parent is a leaf while it has no rows.
class DisplayControlTree
{
 public:
  struct Leaf
  {
    std::optional<bool> visible;     // unset for a row with no checkbox
    std::optional<bool> selectable;  // unset for a row Qt cannot select by
  };
  using Leaves = std::map<std::string, Leaf>;

  DisplayControlTree();

  // Adds the leaves the design defines, one per tech layer and one per lib
  // site, keeping the state of those already there, as Qt does.
  void addDesignRows(odb::dbDatabase* db);
  // Adds `path` unless it is already a leaf.
  void addLeaf(const std::string& path, const Leaf& leaf);
  void removeLeaf(const std::string& path) { leaves_.erase(path); }

  const Leaves& leaves() const { return leaves_; }
  Leaf* find(const std::string& path);

  // Whether `path` matches `pattern` as Qt matches them: wildcards, case
  // insensitive, with '*' crossing '/'.
  static bool matches(const std::string& pattern, const std::string& path);

  // Sets the fields the tile renderer draws by from the leaves.
  void apply(TileVisibility& vis) const;

 private:
  bool visible(const std::string& path) const;
  bool selectable(const std::string& path) const;

  Leaves leaves_;
};

}  // namespace web
