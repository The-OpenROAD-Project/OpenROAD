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

#include <cmath>

#include "gui/gui.h"
#include "layoutViewer.h"

namespace gui {
GotoLocationDialog::GotoLocationDialog(QWidget* parent, LayoutTabs* viewers)
    : QDialog(parent), viewers_(viewers)
{
  setupUi(this);
}

void GotoLocationDialog::updateUnits(int dbu_per_micron, bool useDBU)
{
  if (useDBU) {
    xEdit->setText(QString::number(xEdit->text().toDouble() * dbu_per_micron));
    yEdit->setText(QString::number(yEdit->text().toDouble() * dbu_per_micron));
    sEdit->setText(QString::number(sEdit->text().toDouble() * dbu_per_micron));
  } else {
    xEdit->setText(QString::number(xEdit->text().toDouble() / dbu_per_micron));
    yEdit->setText(QString::number(yEdit->text().toDouble() / dbu_per_micron));
    sEdit->setText(QString::number(sEdit->text().toDouble() / dbu_per_micron));
  }
}

void GotoLocationDialog::updateLocation(QLineEdit* xEdit, QLineEdit* yEdit)
{
  auto viewer = viewers_->getCurrent();
  xEdit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().x(), false)));
  yEdit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().y(), false)));
}

void GotoLocationDialog::show_init()
{
  auto viewer = viewers_->getCurrent();
  GotoLocationDialog::updateLocation(xEdit, yEdit);
  int box_size = sqrt(pow((viewer->getVisibleBounds().lr().x()
                           - viewer->getVisibleBounds().ll().x()),
                          2)
                      + pow((viewer->getVisibleBounds().ul().y()
                             - viewer->getVisibleBounds().ll().y()),
                            2))
                 / 2;
  sEdit->setText(QString::fromStdString(
      Descriptor::Property::convert_dbu(box_size, false)));
  show();
}

void GotoLocationDialog::accept()
{
  auto gui = gui::Gui::get();
  bool convert_x_ok;
  bool convert_y_ok;
  bool convert_s_ok;
  int x_coord = Descriptor::Property::convert_string(
      xEdit->text().toStdString(), &convert_x_ok);
  int y_coord = Descriptor::Property::convert_string(
      yEdit->text().toStdString(), &convert_y_ok);
  int box_size = Descriptor::Property::convert_string(
      sEdit->text().toStdString(), &convert_s_ok);
  if (convert_x_ok && convert_y_ok && convert_s_ok) {
    gui->zoomTo(odb::Rect(x_coord - box_size,
                          y_coord - box_size,
                          x_coord + box_size,
                          y_coord + box_size));
  }
  GotoLocationDialog::updateLocation(xEdit, yEdit);
}
}  // namespace gui
