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
void GotoLocationDialog::updateLocation(QLineEdit* x_edit, QLineEdit* y_edit)
{
  auto viewer = viewers_->getCurrent();
  x_edit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
      viewer->getVisibleCenter().x(), false)));
  y_edit->setText(QString::fromStdString(Descriptor::Property::convert_dbu(
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
