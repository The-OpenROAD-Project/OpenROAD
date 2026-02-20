// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "heatMapSetup.h"

#include <QComboBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <variant>

#include "gui/heatMap.h"

namespace gui {

HeatMapSetup::HeatMapSetup(HeatMapDataSource& source,
                           const QString& title,
                           bool use_dbu,
                           int dbu,
                           QWidget* parent)
    : QDialog(parent),
      source_(source),
      use_dbu_(use_dbu),
      dbu_(dbu),
      log_scale_(new QCheckBox(this)),
      reverse_log_scale_(new QCheckBox(this)),
      show_numbers_(new QCheckBox(this)),
      show_legend_(new QCheckBox(this)),
      grid_x_size_(nullptr),
      grid_y_size_(nullptr),
      grid_x_size_dbu_(nullptr),
      grid_y_size_dbu_(nullptr),
      min_range_selector_(new QDoubleSpinBox(this)),
      show_mins_(new QCheckBox(this)),
      max_range_selector_(new QDoubleSpinBox(this)),
      show_maxs_(new QCheckBox(this)),
      alpha_selector_(new QSpinBox(this)),
      rebuild_(new QPushButton("Rebuild data", this)),
      close_(new QPushButton("Close", this))
{
  setWindowTitle(title);

  QVBoxLayout* overall_layout = new QVBoxLayout;
  QFormLayout* form = new QFormLayout;

  for (const auto& option : source_.getMapSettings()) {
    if (std::holds_alternative<HeatMapDataSource::MapSettingBoolean>(option)) {
      addBooleanOption(form,
                       std::get<HeatMapDataSource::MapSettingBoolean>(option));
    } else if (std::holds_alternative<HeatMapDataSource::MapSettingMultiChoice>(
                   option)) {
      addMultiChoiceOption(
          form, std::get<HeatMapDataSource::MapSettingMultiChoice>(option));
    }
  }

  form->addRow(tr("Log scale"), log_scale_);
  form->addRow(tr("Reverse Log scale"), reverse_log_scale_);

  form->addRow(tr("Show numbers"), show_numbers_);

  form->addRow(tr("Show legend"), show_legend_);

  QHBoxLayout* grid_layout = new QHBoxLayout;
  if (!use_dbu_) {
    grid_x_size_ = new QDoubleSpinBox(this);
    grid_y_size_ = new QDoubleSpinBox(this);

    const QString grid_suffix(" μm");
    grid_x_size_->setRange(source_.getGridSizeMinimumValue(),
                           source_.getGridSizeMaximumValue());
    grid_x_size_->setSuffix(grid_suffix);
    grid_y_size_->setRange(source_.getGridSizeMinimumValue(),
                           source_.getGridSizeMaximumValue());
    grid_y_size_->setSuffix(grid_suffix);
    grid_layout->addWidget(new QLabel("X", this));
    grid_layout->addWidget(grid_x_size_);
    grid_layout->addWidget(new QLabel("Y", this));
    grid_layout->addWidget(grid_y_size_);
    if (!source_.canAdjustGrid()) {
      grid_x_size_->setEnabled(false);
      grid_y_size_->setEnabled(false);
    }
  } else {
    grid_x_size_dbu_ = new QSpinBox(this);
    grid_y_size_dbu_ = new QSpinBox(this);

    grid_x_size_dbu_->setRange(source_.getGridSizeMinimumValue() * dbu_,
                               source_.getGridSizeMaximumValue() * dbu_);
    grid_y_size_dbu_->setRange(source_.getGridSizeMinimumValue() * dbu_,
                               source_.getGridSizeMaximumValue() * dbu_);
    grid_layout->addWidget(new QLabel("X", this));
    grid_layout->addWidget(grid_x_size_dbu_);
    grid_layout->addWidget(new QLabel("Y", this));
    grid_layout->addWidget(grid_y_size_dbu_);
    if (!source_.canAdjustGrid()) {
      grid_x_size_dbu_->setEnabled(false);
      grid_y_size_dbu_->setEnabled(false);
    }
  }
  form->addRow(tr("Grid"), grid_layout);

  min_range_selector_->setDecimals(3);
  QHBoxLayout* min_grid = new QHBoxLayout;
  min_grid->addWidget(min_range_selector_);
  min_grid->addWidget(new QLabel("Show values below", this));
  min_grid->addWidget(show_mins_);
  form->addRow(tr("Minimum"), min_grid);

  max_range_selector_->setDecimals(3);
  QHBoxLayout* max_grid = new QHBoxLayout;
  max_grid->addWidget(max_range_selector_);
  max_grid->addWidget(new QLabel("Show values above", this));
  max_grid->addWidget(show_maxs_);
  form->addRow(tr("Maximum"), max_grid);

  alpha_selector_->setRange(source_.getColorAlphaMinimum(),
                            source_.getColorAlphaMaximum());
  form->addRow(tr("Color alpha"), alpha_selector_);

  overall_layout->addLayout(form);

  QHBoxLayout* buttons = new QHBoxLayout;
  buttons->addWidget(rebuild_);
  buttons->addWidget(close_);

  overall_layout->addLayout(buttons);

  setLayout(overall_layout);

  updateWidgets();

  connect(
      log_scale_, &QCheckBox::stateChanged, this, &HeatMapSetup::updateScale);

  connect(reverse_log_scale_,
          &QCheckBox::stateChanged,
          this,
          &HeatMapSetup::updateReverseScale);

  if (!use_dbu_) {
    connect(grid_x_size_,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &HeatMapSetup::updateGridSize);
    connect(grid_y_size_,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &HeatMapSetup::updateGridSize);
  } else {
    connect(grid_x_size_dbu_,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &HeatMapSetup::updateGridSize);
    connect(grid_y_size_dbu_,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &HeatMapSetup::updateGridSize);
  }

  connect(show_numbers_,
          &QCheckBox::stateChanged,
          this,
          &HeatMapSetup::updateShowNumbers);

  connect(show_legend_,
          &QCheckBox::stateChanged,
          this,
          &HeatMapSetup::updateShowLegend);

  connect(min_range_selector_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this,
          &HeatMapSetup::updateRange);
  connect(max_range_selector_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this,
          &HeatMapSetup::updateRange);
  connect(show_mins_,
          &QCheckBox::stateChanged,
          this,
          &HeatMapSetup::updateShowMinRange);
  connect(show_maxs_,
          &QCheckBox::stateChanged,
          this,
          &HeatMapSetup::updateShowMaxRange);

  connect(alpha_selector_,
          qOverload<int>(&QSpinBox::valueChanged),
          this,
          &HeatMapSetup::updateAlpha);

  connect(this, &HeatMapSetup::changed, this, &HeatMapSetup::updateWidgets);

  connect(rebuild_, &QPushButton::pressed, this, &HeatMapSetup::destroyMap);

  connect(close_, &QPushButton::pressed, this, &HeatMapSetup::accept);
}

void HeatMapSetup::updateWidgets()
{
  show_mins_->setCheckState(source_.getDrawBelowRangeMin() ? Qt::Checked
                                                           : Qt::Unchecked);

  min_range_selector_->blockSignals(true);
  min_range_selector_->setRange(
      source_.convertPercentToValue(source_.getDisplayRangeMinimumValue()),
      source_.convertPercentToValue(source_.getDisplayRangeMaximumValue()));
  min_range_selector_->setValue(
      source_.convertPercentToValue(source_.getDisplayRangeMin()));
  min_range_selector_->blockSignals(false);
  min_range_selector_->setSuffix(
      " " + QString::fromStdString(source_.getValueUnits()));
  min_range_selector_->setSingleStep(source_.getDisplayRangeIncrement());

  show_maxs_->setCheckState(source_.getDrawAboveRangeMax() ? Qt::Checked
                                                           : Qt::Unchecked);

  max_range_selector_->blockSignals(true);
  max_range_selector_->setRange(
      source_.convertPercentToValue(source_.getDisplayRangeMinimumValue()),
      source_.convertPercentToValue(source_.getDisplayRangeMaximumValue()));
  max_range_selector_->setValue(
      source_.convertPercentToValue(source_.getDisplayRangeMax()));
  max_range_selector_->blockSignals(false);
  max_range_selector_->setSuffix(
      " " + QString::fromStdString(source_.getValueUnits()));
  max_range_selector_->setSingleStep(source_.getDisplayRangeIncrement());

  if (!use_dbu_) {
    grid_x_size_->setValue(source_.getGridXSize());
    grid_y_size_->setValue(source_.getGridYSize());
  } else {
    grid_x_size_dbu_->setValue(source_.getGridXSize() * dbu_);
    grid_y_size_dbu_->setValue(source_.getGridYSize() * dbu_);
  }

  alpha_selector_->setValue(source_.getColorAlpha());

  log_scale_->setCheckState(source_.getLogScale() ? Qt::Checked
                                                  : Qt::Unchecked);
  reverse_log_scale_->setCheckState(
      source_.getReverseLogScale() ? Qt::Checked : Qt::Unchecked);
  reverse_log_scale_->setEnabled(source_.getLogScale());
  show_numbers_->setCheckState(source_.getShowNumbers() ? Qt::Checked
                                                        : Qt::Unchecked);
  show_legend_->setCheckState(source_.getShowLegend() ? Qt::Checked
                                                      : Qt::Unchecked);
}

void HeatMapSetup::destroyMap()
{
  source_.destroyMap();
}

void HeatMapSetup::updateScale(int option)
{
  source_.setLogScale(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateReverseScale(int option)
{
  source_.setReverseLogScale(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowNumbers(int option)
{
  source_.setShowNumbers(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowLegend(int option)
{
  source_.setShowLegend(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowMinRange(int option)
{
  source_.setDrawBelowRangeMin(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateShowMaxRange(int option)
{
  source_.setDrawAboveRangeMax(option == Qt::Checked);
  emit changed();
}

void HeatMapSetup::updateRange()
{
  source_.setDisplayRange(
      source_.convertValueToPercent(min_range_selector_->value()),
      source_.convertValueToPercent(max_range_selector_->value()));
  emit changed();
}

void HeatMapSetup::updateGridSize()
{
  if (!use_dbu_) {
    source_.setGridSizes(grid_x_size_->value(), grid_y_size_->value());
  } else {
    const double dbu = dbu_;
    source_.setGridSizes(grid_x_size_dbu_->value() / dbu,
                         grid_x_size_dbu_->value() / dbu);
  }
  emit changed();
}

void HeatMapSetup::updateAlpha(int alpha)
{
  source_.setColorAlpha(alpha);
  emit changed();
}

void HeatMapSetup::addBooleanOption(
    QFormLayout* layout,
    const HeatMapDataSource::MapSettingBoolean& option)
{
  QCheckBox* check_box = new QCheckBox(this);
  check_box->setCheckState(option.getter() ? Qt::Checked : Qt::Unchecked);

  layout->addRow(QString::fromStdString(option.label), check_box);

  QObject::connect(
      check_box, &QCheckBox::stateChanged, [this, option](int value) {
        option.setter(value == Qt::Checked);
        destroyMap();
        source_.redraw();
      });
}

void HeatMapSetup::addMultiChoiceOption(
    QFormLayout* layout,
    const HeatMapDataSource::MapSettingMultiChoice& option)
{
  QComboBox* combo_box = new QComboBox(this);
  for (const auto& value : option.choices()) {
    combo_box->addItem(QString::fromStdString(value));
  }
  combo_box->setCurrentText(QString::fromStdString(option.getter()));

  layout->addRow(QString::fromStdString(option.label), combo_box);

  QObject::connect(combo_box,
                   &QComboBox::currentTextChanged,
                   [this, &option](const QString& value) {
                     option.setter(value.toStdString());
                     destroyMap();
                     source_.redraw();
                   });
}

}  // namespace gui
