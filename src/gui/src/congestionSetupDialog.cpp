//////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include "congestionSetupDialog.h"

#include <QColor>
#include <QColorDialog>
#include <QDebug>
#include <QLinearGradient>
#include <QListWidgetItem>
#include <QPalette>
#include <map>
#include <string>

#include "db.h"
#include "gui/gui.h"

namespace gui {

CongestionSetupDialog::ColorRangeMap CongestionSetupDialog::_color_ranges
    = {{2,
        {CongestionBandInfo("blue", "0", 50),
         CongestionBandInfo("red", "1", 100)}},
       {3,
        {CongestionBandInfo("blue", "0", 33),
         CongestionBandInfo("yellow", "0.5", 66),
         CongestionBandInfo("red", "1", 100)}},
       {4,
        {CongestionBandInfo("blue", "0", 25),
         CongestionBandInfo("green", "0.33", 50),
         CongestionBandInfo("yellow", "0.66", 75),
         CongestionBandInfo("red", "1", 100)}},
       {5,
        {CongestionBandInfo("blue", "0", 20),
         CongestionBandInfo("cyan", "0.25", 40),
         CongestionBandInfo("green", "0.5", 60),
         CongestionBandInfo("yellow", "0.75", 80),
         CongestionBandInfo("red", "1", 100)}}};

CongestionSetupDialog::CongestionSetupDialog(QWidget* parent) : QDialog(parent)
{
  setupUi(this);
  std::vector<QString> labels = getColorNames(0);
  QListWidgetItem* lower_bound_item = new QListWidgetItem(labels[0]);
  lower_bound_item->setBackground(QBrush(Qt::blue));
  QListWidgetItem* upper_bound_item = new QListWidgetItem(labels[1]);
  upper_bound_item->setBackground(QBrush(Qt::red));
  colorRangeListWidget->clear();
  colorRangeListWidget->addItem(lower_bound_item);
  colorRangeListWidget->addItem(upper_bound_item);

  congestionButton->setStyleSheet(
      "QPushButton { background-color: qlineargradient(x1:0 y1:0, x2:1 y2:0, "
      "stop:0 blue, stop:1 red); }");

  colorRangeListWidget->setStyleSheet(
      "QListView::item:selected {background : none; border: solid 2px "
      "red;}");

  colorRangeListWidget->setSelectionMode(QAbstractItemView::NoSelection);

  connect(colorRangeListWidget,
          SIGNAL(itemDoubleClicked(QListWidgetItem*)),
          this,
          SLOT(colorIntervalItemDblClicked(QListWidgetItem*)));

  connect(spinBox,
          SIGNAL(valueChanged(int)),
          this,
          SLOT(colorIntervalChanged(int)));
  connect(startCongestionSpinBox,
          SIGNAL(valueChanged(int)),
          this,
          SLOT(congestionStartValueChanged(int)));
  connect(applyButton, SIGNAL(pressed()), this, SLOT(saveState()));
  connect(
      applyButton, SIGNAL(clicked()), this, SIGNAL(applyCongestionRequested()));
  min_congestion_ = 0;
  cong_dir_index_ = BOTH_DIR;

  colors_.push_back(_color_ranges[2][0].band_color_);
  colors_.push_back(_color_ranges[2][1].band_color_);
}

void CongestionSetupDialog::saveState()
{
  // Update the dialog state
  min_congestion_ = minCongestionSlider->value();
  colors_.clear();
  int row = 0;
  while (row < colorRangeListWidget->count()) {
    colors_.push_back(colorRangeListWidget->item(row++)->background().color());
  }
  cong_dir_index_ = BOTH_DIR;
  if (horCongDir->isChecked())
    cong_dir_index_ = HORIZONTAL_DIR;
  else if (verCongDir->isChecked())
    cong_dir_index_ = VERTICAL_DIR;
}

void CongestionSetupDialog::accept()
{
  saveState();
  QDialog::accept();
  emit congestionSetupChanged();
}

void CongestionSetupDialog::reject()
{
  // reset the dialog state
  minCongestionSlider->setValue(min_congestion_);
  int row = 0;
  spinBox->setValue(colors_.size());
  std::string color_sheet_value;
  colorRangeListWidget->clear();
  std::vector<QString> color_labels = getColorNames(spinBox->value());

  for (auto& color_data : _color_ranges[spinBox->value()]) {
    QListWidgetItem* item = new QListWidgetItem(color_labels[row]);
    item->setBackground(QBrush(colors_[row]));
    colorRangeListWidget->addItem(item);
    color_sheet_value = color_sheet_value
                        + "stop:" + color_data.color_style_sheet_ + " "
                        + colors_[row++].name().toStdString() + ", ";
  }

  color_sheet_value = color_sheet_value.substr(0, color_sheet_value.size() - 2);
  updateCongestionButtonStyleSheet(color_sheet_value);

  if (cong_dir_index_ == BOTH_DIR) {
    bothCongDir->setChecked(true);
    horCongDir->setChecked(false);
    verCongDir->setChecked(false);
  } else if (cong_dir_index_ == HORIZONTAL_DIR) {
    bothCongDir->setChecked(false);
    horCongDir->setChecked(true);
    verCongDir->setChecked(false);
  } else {
    bothCongDir->setChecked(false);
    horCongDir->setChecked(false);
    verCongDir->setChecked(true);
  }
  QDialog::reject();
}

void CongestionSetupDialog::updateCongestionButtonStyleSheet(
    const std::string& color_values)
{
  std::string new_style_sheet(
      "QPushButton { background-color: qlineargradient(x1:0 y1:0, x2:1 y2:0, ");
  new_style_sheet += color_values;
  new_style_sheet += "); }";
  congestionButton->setStyleSheet(new_style_sheet.c_str());
}

void CongestionSetupDialog::colorIntervalChanged(int value)
{
  std::string color_sheet_value;
  colorRangeListWidget->clear();
  std::vector<QString> color_labels = getColorNames(value);
  int row = 0;
  for (auto& color_data : CongestionSetupDialog::_color_ranges[value]) {
    QListWidgetItem* item = new QListWidgetItem(color_labels[row++]);
    item->setBackground(QBrush(color_data.band_color_));
    colorRangeListWidget->addItem(item);
    color_sheet_value = color_sheet_value
                        + "stop:" + color_data.color_style_sheet_ + " "
                        + color_data.band_color_.name().toStdString() + ", ";
  }

  color_sheet_value = color_sheet_value.substr(0, color_sheet_value.size() - 2);
  updateCongestionButtonStyleSheet(color_sheet_value);
}

void CongestionSetupDialog::congestionStartValueChanged(int value)
{
  std::vector<QString> color_labels = getColorNames(spinBox->value());
  int row = 0;
  for (auto& color_label : color_labels) {
    QListWidgetItem* item = colorRangeListWidget->item(row++);
    item->setText(color_label);
  }
}

void CongestionSetupDialog::colorIntervalItemDblClicked(QListWidgetItem* item)
{
  auto prev_color = item->background().color();
  QColor new_color = QColorDialog::getColor(prev_color, this);
  if (new_color.isValid()) {
    item->setBackground(QBrush(new_color));
    std::string color_value = getCongestionButtonStyleSheetColors();
    updateCongestionButtonStyleSheet(color_value);
  }
  return;
}

std::string CongestionSetupDialog::getCongestionButtonStyleSheetColors()
{
  std::string color_sheet_value;
  int value = spinBox->value();
  int row = 0;
  for (auto& color_data : CongestionSetupDialog::_color_ranges[value]) {
    QListWidgetItem* item = colorRangeListWidget->item(row++);
    color_sheet_value = color_sheet_value
                        + "stop:" + color_data.band_color_.name().toStdString()
                        + " " + item->background().color().name().toStdString()
                        + ", ";
  }

  color_sheet_value = color_sheet_value.substr(0, color_sheet_value.size() - 2);
  return color_sheet_value;
}

std::vector<QString> CongestionSetupDialog::getColorNames(int band_index)
{
  std::vector<QString> labels;
  QString label;
  auto color_data = _color_ranges[spinBox->value()];
  int index = 0;
  int current_percent = startCongestionSpinBox->value();
  int percent_step = color_data[0].band_ulimit_;
  while (index < color_data.size() - 1) {
    label = QString::number(current_percent) + "% <= Usage < "
            + QString::number(current_percent + percent_step) + "%";
    labels.push_back(label);
    current_percent += percent_step;
    ++index;
  }
  label = QString("Usage >= ") + QString::number(current_percent) + "%";
  labels.push_back(label);
  return labels;
}

void CongestionSetupDialog::designLoaded(odb::dbBlock* block)
{
}

QColor CongestionSetupDialog::getCongestionColorForPercentage(
    float congestion_percent,
    int alpha) const
{
  auto& color_data_vec = _color_ranges[colors_.size()];
  if (congestion_percent > 100)
    return color_data_vec.rbegin()->band_color_;

  int index = 0;
  while (index < color_data_vec.size() - 1) {
    if (congestion_percent
        <= (color_data_vec[index].band_ulimit_ + showStartCongestionValue()))
      break;
    index++;
  }

  QColor min_color(0, 0, 0);
  QColor max_color = colors_[index];
  int max_percent
      = color_data_vec[index].band_ulimit_ + showStartCongestionValue();
  int min_percent = startCongestionSpinBox->value();
  if (index != 0) {
    min_color = colors_[index - 1];
    min_percent
        = color_data_vec[index - 1].band_ulimit_ + showStartCongestionValue();
  }
  float band_percent
      = (congestion_percent - min_percent) / (max_percent - min_percent);

  int red_val = std::min(
      255,
      min_color.red()
          + std::abs(int(band_percent * (max_color.red() - min_color.red()))));
  int green_val = std::min(
      255,
      min_color.green()
          + std::abs(
              int(band_percent * (max_color.green() - min_color.green()))));
  int blue_val
      = std::min(255,
                 min_color.blue()
                     + std::abs(int(band_percent
                                    * (max_color.blue() - min_color.blue()))));
  QColor congestion_color;

  congestion_color.setRed(red_val);
  congestion_color.setGreen(green_val);
  congestion_color.setBlue(blue_val);

  congestion_color.setAlpha(alpha);

  return congestion_color;
}

}  // namespace gui