// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "gotoDialog.h"

#include <QDialog>
#include <QString>
#include <QWidget>
#include <cmath>

#include "gui/gui.h"
#include "layoutTabs.h"
#include "layoutViewer.h"

namespace gui {
GotoLocationDialog::GotoLocationDialog(QWidget* parent, LayoutTabs* viewers)
    : QDialog(parent), viewers_(viewers)
{
  setupUi(this);
  connect(gotoBtn, &QPushButton::clicked, this, &GotoLocationDialog::goTo);
}

// NOLINTNEXTLINE(readability-non-const-parameter)
void GotoLocationDialog::updateLocation()
{
  auto viewer = viewers_->getCurrent();
  if (!viewer) {
    return;
  }
  x_edit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().x(), false)));
  y_edit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().y(), false)));
  int box_size = viewer->getVisibleDiameter();
  s_edit->setText(QString::fromStdString(
      Descriptor::Property::convert_dbu(box_size, false)));
}

void GotoLocationDialog::showInit()
{
  updateLocation();
  show();
}

void GotoLocationDialog::goTo()
{
  auto gui = gui::Gui::get();
  bool convert_x_ok;
  bool convert_y_ok;
  bool convert_s_ok;
  int x_coord = Descriptor::Property::convert_string(
      x_edit->text().toStdString(), &convert_x_ok);
  int y_coord = Descriptor::Property::convert_string(
      y_edit->text().toStdString(), &convert_y_ok);
  int diameter = Descriptor::Property::convert_string(
      s_edit->text().toStdString(), &convert_s_ok);
  if (convert_x_ok && convert_y_ok && convert_s_ok) {
    gui->zoomTo(odb::Point(x_coord, y_coord), diameter);
  }
  updateLocation();
}

void GotoLocationDialog::accept()
{
}
}  // namespace gui
