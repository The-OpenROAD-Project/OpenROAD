// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "gotoDialog.h"

#include <QDialog>
#include <QString>
#include <QWidget>
#include <cmath>

#include "gui/gui.h"
#include "layoutViewer.h"

namespace gui {
GotoLocationDialog::GotoLocationDialog(QWidget* parent, LayoutTabs* viewers)
    : QDialog(parent), viewers_(viewers)
{
  setupUi(this);
  connect(gotoBtn, &QPushButton::clicked, this, &GotoLocationDialog::goTo);
}

void GotoLocationDialog::updateUnits(int dbu_per_micron, bool use_dbu)
{
  if (use_dbu) {
    xEdit->setText(QString::number(xEdit->text().toDouble() * dbu_per_micron));
    yEdit->setText(QString::number(yEdit->text().toDouble() * dbu_per_micron));
    sEdit->setText(QString::number(sEdit->text().toDouble() * dbu_per_micron));
  } else {
    xEdit->setText(QString::number(xEdit->text().toDouble() / dbu_per_micron));
    yEdit->setText(QString::number(yEdit->text().toDouble() / dbu_per_micron));
    sEdit->setText(QString::number(sEdit->text().toDouble() / dbu_per_micron));
  }
}

// NOLINTNEXTLINE(readability-non-const-parameter)
void GotoLocationDialog::updateLocation()
{
  auto viewer = viewers_->getCurrent();
  if (!viewer) {
    return;
  }
  xEdit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().x(), false)));
  yEdit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().y(), false)));
  int box_size = viewer->getVisibleDiameter();
  sEdit->setText(QString::fromStdString(
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
      xEdit->text().toStdString(), &convert_x_ok);
  int y_coord = Descriptor::Property::convert_string(
      yEdit->text().toStdString(), &convert_y_ok);
  int diameter = Descriptor::Property::convert_string(
      sEdit->text().toStdString(), &convert_s_ok);
  if (convert_x_ok && convert_y_ok && convert_s_ok) {
    gui->zoomTo(odb::Point(x_coord, y_coord), diameter);
  }
  updateLocation();
}

void GotoLocationDialog::accept()
{
}
}  // namespace gui
