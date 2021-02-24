///////////////////////////////////////////////////////////////////////////////
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

#pragma once

#include <tcl.h>

#include <QColorDialog>
#include <QDialog>
#include <QDockWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QModelIndex>
#include <QRadioButton>
#include <QStandardItemModel>
#include <QStringList>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>
#include <vector>

#include "congestionSetupDialog.h"
#include "options.h"

namespace odb {
class dbDatabase;
class dbBlock;
class dbNet;
}  // namespace odb

namespace gui {

struct Callback
{
  std::function<void(bool)> action;
};

class PatternButton : public QRadioButton
{
  Q_OBJECT
 public:
  PatternButton(Qt::BrushStyle pattern, QWidget* parent = nullptr);
  ~PatternButton() {}

  void paintEvent(QPaintEvent* event);
  Qt::BrushStyle pattern() const { return pattern_; }

 private:
  Qt::BrushStyle pattern_;
};

class DisplayColorDialog : public QDialog
{
  Q_OBJECT
 public:
  DisplayColorDialog(QColor color,
                     Qt::BrushStyle pattern = Qt::SolidPattern,
                     QWidget* parent = nullptr);
  ~DisplayColorDialog();

  QColor getSelectedColor() const { return color_; }
  Qt::BrushStyle getSelectedPattern() const;

 public slots:
  void acceptDialog();
  void rejectDialog();

 private:
  QColor color_;
  Qt::BrushStyle pattern_;

  QGroupBox* pattern_group_box_;
  QGridLayout* grid_layout_;
  QVBoxLayout* main_layout_;

  std::vector<PatternButton*> pattern_buttons_;
  QColorDialog* color_dialog_;

  void buildUI();

  static inline std::vector<std::vector<Qt::BrushStyle>> brush_patterns_{
      {Qt::NoBrush, Qt::SolidPattern},
      {Qt::HorPattern, Qt::VerPattern},
      {Qt::CrossPattern, Qt::DiagCrossPattern},
      {Qt::FDiagPattern, Qt::BDiagPattern}};
};

// This class shows the user the set of layers & objects that
// they can control the visibility and selectablity of.  The
// controls are show in a tree view to provide grouping of
// related options.
//
// It also implements the Options interface so that other clients can
// access the data.
class DisplayControls : public QDockWidget, public Options
{
  Q_OBJECT

 public:
  DisplayControls(QWidget* parent = nullptr);

  void setDb(odb::dbDatabase* db);

  // From the Options API
  QColor color(const odb::dbTechLayer* layer) override;
  Qt::BrushStyle pattern(const odb::dbTechLayer* layer) override;
  bool isVisible(const odb::dbTechLayer* layer) override;
  bool isSelectable(const odb::dbTechLayer* layer) override;
  bool isNetVisible(odb::dbNet* net) override;
  bool areFillsVisible() override;
  bool areRowsVisible() override;
  bool arePrefTracksVisible() override;
  bool areNonPrefTracksVisible() override;

  bool isCongestionVisible() const override;
  bool showHorizontalCongestion() const override;
  bool showVerticalCongestion() const override;
  float getMinCongestionToShow() const override;
  float getMaxCongestionToShow() const override;
  QColor getCongestionColor(float congestion) const override;

 signals:
  // The display options have changed and clients need to update
  void changed();

 public slots:
  // Tells this widget that a new design is loaded and the
  // options displayed need to match
  void designLoaded(odb::dbBlock* block);

  // This is called by the check boxes to update the state
  void itemChanged(QStandardItem* item);
  void displayItemDblClicked(const QModelIndex& index);

  void showCongestionSetup();

 private:
  // The columns in the tree view
  enum Column
  {
    Name,
    Swatch,
    Visible,
    Selectable
  };

  void techInit();

  template <typename T>
  QStandardItem* makeItem(const QString& text,
                          T* parent,
                          Qt::CheckState checked,
                          const std::function<void(bool)>& visibility_action,
                          const std::function<void(bool)>& select_action
                          = std::function<void(bool)>(),
                          const QColor& color = Qt::transparent,
                          odb::dbTechLayer* tech_layer = nullptr);

  void toggleAllChildren(bool checked, QStandardItem* parent, Column column);

  QTreeView* view_;
  QStandardItemModel* model_;

  // Categories in the model
  QStandardItem* layers_;
  QStandardItem* routing_;
  QStandardItem* tracks_;
  QStandardItem* nets_;
  QStandardItem* misc_;

  // Object controls
  QStandardItem* fills_;
  QStandardItem* rows_;
  QStandardItem* congestion_map_;
  QStandardItem* tracks_pref_;
  QStandardItem* tracks_non_pref_;
  QStandardItem* nets_signal_;
  QStandardItem* nets_special_;
  QStandardItem* nets_power_;
  QStandardItem* nets_ground_;
  QStandardItem* nets_clock_;

  odb::dbDatabase* db_;
  bool tech_inited_;
  bool fills_visible_;
  bool tracks_visible_pref_;
  bool tracks_visible_non_pref_;
  bool rows_visible_;
  bool nets_signal_visible_;
  bool nets_special_visible_;
  bool nets_power_visible_;
  bool nets_ground_visible_;
  bool nets_clock_visible_;
  bool congestion_visible_;

  std::map<const odb::dbTechLayer*, QColor> layer_color_;
  std::map<const odb::dbTechLayer*, Qt::BrushStyle> layer_pattern_;
  std::map<const odb::dbTechLayer*, bool> layer_visible_;
  std::map<const odb::dbTechLayer*, bool> layer_selectable_;

  CongestionSetupDialog* congestion_dialog_;
};

}  // namespace gui

Q_DECLARE_METATYPE(gui::Callback);
