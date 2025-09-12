// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include "gui/gui.h"

namespace gui {

// Hold reference to a source file + line number (eg Verilog source)
struct FileLine
{
  const std::string file_name;
  const int line_number;
};

// Something of a placeholder as we don't intend to show these in
// the inspector but just trigger a file open.  However Selected
// expects a descriptor and it is safer to just provide one.
class FileLineDescriptor : public Descriptor
{
 public:
  std::string getName(const std::any& object) const override;
  std::string getTypeName() const override;
  bool getBBox(const std::any& object, odb::Rect& bbox) const override;

  void visitAllObjects(
      const std::function<void(const Selected&)>& func) const override;

  Properties getProperties(const std::any& object) const override;

  Selected makeSelected(const std::any& object) const override;

  bool lessThan(const std::any& l, const std::any& r) const override;

  void highlight(const std::any& object, Painter& painter) const override;
};

}  // namespace gui
