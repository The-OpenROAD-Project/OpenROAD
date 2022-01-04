///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <QColorDialog>
#include <QDialog>
#include <QDockWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QMenu>
#include <QModelIndex>
#include <QRadioButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QStringList>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>
#include <functional>
#include <map>
#include <set>
#include <vector>

#include "options.h"

#include "gui/gui.h"

namespace odb {
class dbDatabase;
class dbBlock;
class dbNet;
class dbInst;
}  // namespace odb

namespace utl {
class Logger;
} // namespace utl

namespace gui {

using CallbackFunction = std::function<void(bool)>;

struct Callback
{
  CallbackFunction action;
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
                     Qt::BrushStyle pattern,
                     QWidget* parent = nullptr);
  DisplayColorDialog(QColor color,
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
  bool show_brush_;

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

class DisplayControlModel : public QStandardItemModel
{
  Q_OBJECT

 public:
  DisplayControlModel(int user_data_item_idx, QWidget* parent = nullptr);

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

 private:
  const int user_data_item_idx_;
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
  ~DisplayControls();

  void setDb(odb::dbDatabase* db);
  void setLogger(utl::Logger* logger);

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

  void setControlByPath(const std::string& path, bool is_visible, Qt::CheckState value);
  bool checkControlByPath(const std::string& path, bool is_visible);

  void registerRenderer(Renderer* renderer);
  void unregisterRenderer(Renderer* renderer);

  void save();
  void restore();

  void restoreTclCommands(std::vector<std::string>& cmds);

  // From the Options API
  QColor color(const odb::dbTechLayer* layer) override;
  Qt::BrushStyle pattern(const odb::dbTechLayer* layer) override;
  QColor placementBlockageColor() override;
  Qt::BrushStyle placementBlockagePattern() override;
  QColor instanceNameColor() override;
  QFont instanceNameFont() override;
  QColor rowColor() override;
  bool isVisible(const odb::dbTechLayer* layer) override;
  bool isSelectable(const odb::dbTechLayer* layer) override;
  bool isNetVisible(odb::dbNet* net) override;
  bool isNetSelectable(odb::dbNet* net) override;
  bool isInstanceVisible(odb::dbInst* inst) override;
  bool isInstanceSelectable(odb::dbInst* inst) override;
  bool areInstanceNamesVisible() override;
  bool areFillsVisible() override;
  bool areBlockagesVisible() override;
  bool areBlockagesSelectable() override;
  bool areObstructionsVisible() override;
  bool areObstructionsSelectable() override;
  bool areRowsVisible() override;
  bool arePrefTracksVisible() override;
  bool areNonPrefTracksVisible() override;

  QColor rulerColor() override;
  QFont rulerFont() override;
  bool areRulersVisible() override;
  bool areRulersSelectable() override;

  bool isDetailedVisibility() override;

  bool areSelectedVisible() override;

  bool isScaleBarVisible() const override;
  bool arePinMarkersVisible() const override;
  bool areAccessPointsVisible() const override;
  QFont pinMarkersFont() override;

 signals:
  // The display options have changed and clients need to update
  void changed();

  // Emit a selected tech layer
  void selected(const Selected& selected);

 public slots:
  // Tells this widget that a new design is loaded and the
  // options displayed need to match
  void designLoaded(odb::dbBlock* block);

  // This is called by the check boxes to update the state
  void itemChanged(QStandardItem* item);
  void displayItemSelected(const QItemSelection& selected);
  void displayItemDblClicked(const QModelIndex& index);

 private slots:
  void itemContextMenu(const QPoint &point);

 private:
  // The columns in the tree view
  enum Column
  {
    Name,
    Swatch,
    Visible,
    Selectable
  };

  // One leaf (non-group) row in the model
  struct ModelRow
  {
    QStandardItem* name = nullptr;
    QStandardItem* swatch = nullptr;
    QStandardItem* visible = nullptr;
    QStandardItem* selectable = nullptr;  // may be null
  };

  // The *Models are groups in the tree
  struct NetModels
  {
    ModelRow signal;
    ModelRow power;
    ModelRow ground;
    ModelRow clock;
  };

  struct InstanceModels
  {
    ModelRow core;
    ModelRow blocks;
    ModelRow fill;
    ModelRow endcap;
    ModelRow welltap;
    ModelRow pads;
    ModelRow cover;
  };

  struct BlockageModels
  {
    ModelRow blockages;
    ModelRow obstructions;
  };

  struct TrackModels
  {
    ModelRow pref;
    ModelRow non_pref;
  };

  struct MiscModels
  {
    ModelRow instance_names;
    ModelRow scale_bar;
    ModelRow fills;
    ModelRow access_points;
    ModelRow detailed;
    ModelRow selected;
  };

  void techInit();

  void collectControls(const QStandardItem* parent,
                       Column column,
                       std::map<std::string, QStandardItem*>& items,
                       const std::string& prefix = "");
  void findControlsInItems(const std::string& path,
                           Column column,
                           std::vector<QStandardItem*>& items);

  QStandardItem* makeParentItem(ModelRow& row,
                                const QString& text,
                                QStandardItemModel* parent,
                                Qt::CheckState checked,
                                bool add_selectable = false,
                                const QColor& color = Qt::transparent);

  void makeLeafItem(ModelRow& row,
                    const QString& text,
                    QStandardItem* parent,
                    Qt::CheckState checked,
                    bool add_selectable = false,
                    const QColor& color = Qt::transparent,
                    const QVariant& user_data = QVariant());

  const QIcon makeSwatchIcon(const QColor& color);

  void toggleAllChildren(bool checked, QStandardItem* parent, Column column);
  void toggleParent(const QStandardItem* parent, QStandardItem* parent_flag, int column);
  void toggleParent(ModelRow& row);

  void readSettingsForRow(QSettings* settings, const ModelRow& row);
  void writeSettingsForRow(QSettings* settings, const ModelRow& row);

  void buildRestoreTclCommands(std::vector<std::string>& cmds, const QStandardItem* parent, const std::string& prefix = "");

  void saveRendererState(Renderer* renderer);

  void setNameItemDoubleClickAction(ModelRow& row, const std::function<void(void)>& callback);
  void setItemExclusivity(ModelRow& row, const std::set<std::string>& exclusivity);

  void createLayerMenu();
  void layerShowOnlySelectedNeighbors(int lower, int upper);
  void collectNeighboringLayers(odb::dbTechLayer* layer, int lower, int upper, std::set<const odb::dbTechLayer*>& layers);
  void setOnlyVisibleLayers(const std::set<const odb::dbTechLayer*> layers);

  QTreeView* view_;
  DisplayControlModel* model_;
  QMenu* layers_menu_;
  odb::dbTechLayer* layers_menu_layer_;

  bool ignore_callback_;

  // Categories in the model
  ModelRow layers_group_;
  ModelRow routing_group_;
  ModelRow tracks_group_;
  ModelRow nets_group_;
  ModelRow instance_group_;
  ModelRow blockage_group_;
  ModelRow misc_group_;

  // Object controls
  NetModels nets_;
  InstanceModels instances_;
  BlockageModels blockages_;
  ModelRow rows_;
  ModelRow pin_markers_;
  ModelRow rulers_;
  TrackModels tracks_;
  MiscModels misc_;

  std::map<const odb::dbTechLayer*, ModelRow> layer_controls_;
  std::map<Renderer*, std::vector<ModelRow>> custom_controls_;
  std::map<std::string, Renderer::Settings> custom_controls_settings_;
  std::map<QStandardItem*, Qt::CheckState> saved_state_;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  bool tech_inited_;

  std::map<const odb::dbTechLayer*, QColor> layer_color_;
  std::map<const odb::dbTechLayer*, Qt::BrushStyle> layer_pattern_;

  QColor placement_blockage_color_;
  Qt::BrushStyle placement_blockage_pattern_;

  QColor instance_name_color_;
  QFont instance_name_font_;

  QColor ruler_color_;
  QFont ruler_font_;

  QColor row_color_;

  QFont pin_markers_font_;

  static constexpr int user_data_item_idx_ = Qt::UserRole;
  static constexpr int callback_item_idx_ = Qt::UserRole + 1;
  static constexpr int doubleclick_item_idx_ = Qt::UserRole + 2;
  static constexpr int exclusivity_item_idx_ = Qt::UserRole + 3;
};

}  // namespace gui

Q_DECLARE_METATYPE(gui::Callback);
