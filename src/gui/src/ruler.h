///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#include <memory>

#include "gui/gui.h"

namespace utl {
class Logger;
}  // namespace utl

namespace odb {
class Point;
}  // namespace odb

namespace gui {

class RulerDescriptor : public Descriptor
{
 public:
  std::string getName(std::any object) const override;
  std::string getTypeName(std::any object) const override;
  bool getBBox(std::any object, odb::Rect& bbox) const override;

  void highlight(std::any object,
                 Painter& painter,
                 void* additional_data) const override;

  Properties getProperties(std::any object) const override;
  Editors getEditors(std::any object) const override;
  Actions getActions(std::any object) const override;
  Selected makeSelected(std::any object, void* additional_data) const override;
  bool lessThan(std::any l, std::any r) const override;

 private:
  static bool editPoint(std::any value, int dbu_per_uu, odb::Point& pt, bool is_x);
};

class Ruler
{
 public:
  Ruler(const odb::Point& pt0, const odb::Point& pt1, const std::string& name = "", const std::string& label = "");

  odb::Point& getPt0() { return pt0_; }
  const odb::Point getPt0() const { return pt0_; }
  odb::Point& getPt1() { return pt1_; }
  const odb::Point getPt1() const { return pt1_; }
  const std::string getName() const { return name_; }
  void setName(const std::string& name) { name_ = name; }
  const std::string getLabel() const { return label_; }
  void setLabel(const std::string& label) { label_ = label; }

  bool fuzzyIntersection(const odb::Point& pt, int margin) const;

  bool operator ==(const Ruler& other) const;

 private:
  odb::Point pt0_;
  odb::Point pt1_;

  std::string name_;
  std::string label_;
};

class RulerManager
{
 public:
  RulerManager() : logger_(nullptr) {}

  const std::string add(const odb::Point& p0, const odb::Point& p1, const std::string& label = "", const std::string& name = "");
  void remove(const std::string& name);
  void clear();

  Ruler* find(const std::string& name);
  bool contains(const std::string& name);

  const std::vector<std::unique_ptr<Ruler>>& getRulers() { return rulers_; }

  void setLogger(utl::Logger* logger) { logger_ = logger; }

  static RulerManager* manager();

 private:
  utl::Logger* logger_;
  std::vector<std::unique_ptr<Ruler>> rulers_;

  // singleton of rulers
  static std::unique_ptr<RulerManager> manager_;
};

}  // namespace gui
