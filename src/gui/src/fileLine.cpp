// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "fileLine.h"

namespace gui {

std::string FileLineDescriptor::getName(const std::any& object) const
{
  FileLine fl = std::any_cast<FileLine>(object);
  return fmt::format("{}:{}", fl.file_name, fl.line_number);
}

std::string FileLineDescriptor::getTypeName() const
{
  return "FileLine";
}

bool FileLineDescriptor::getBBox(const std::any& object, odb::Rect& bbox) const
{
  return false;
}

void FileLineDescriptor::visitAllObjects(
    const std::function<void(const Selected&)>& func) const
{
}

Descriptor::Properties FileLineDescriptor::getProperties(
    const std::any& object) const
{
  return {};
}

Selected FileLineDescriptor::makeSelected(const std::any& object) const
{
  FileLine fl = std::any_cast<FileLine>(object);
  return Selected(fl, this);
}

bool FileLineDescriptor::lessThan(const std::any& l, const std::any& r) const
{
  auto l_fl = std::any_cast<FileLine>(l);
  auto r_fl = std::any_cast<FileLine>(r);
  return std::tie(l_fl.file_name, l_fl.line_number)
         < std::tie(r_fl.file_name, r_fl.line_number);
}

void FileLineDescriptor::highlight(const std::any& object,
                                   Painter& painter) const
{
}

}  // namespace gui
