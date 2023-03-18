//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, The Regents of the University of California
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

#include "gotoDialog.h"

#include "gui/gui.h"

namespace gui {
GotoLocationDialog::GotoLocationDialog(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
}

void GotoLocationDialog::show_init(LayoutViewer* viewer_)
{
  xEdit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer_->getVisibleCenter().x(), false)));
  yEdit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer_->getVisibleCenter().y(), false)));
  show();
}

void GotoLocationDialog::accept()
{
  auto gui = gui::Gui::get();
  bool convert_x_ok;
  bool convert_y_ok;
  const int x_coord = Descriptor::Property::convert_string(
      xEdit->text().toStdString(), &convert_x_ok);
  const int y_coord = Descriptor::Property::convert_string(
      yEdit->text().toStdString(), &convert_y_ok);
  if (convert_x_ok && convert_y_ok) {
    gui->centerAt({x_coord, y_coord});
  }
}
}  // namespace gui
