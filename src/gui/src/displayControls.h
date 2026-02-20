// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QColor>
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
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "gui/gui.h"
#include "odb/db.h"
#include "odb/dbBlockCallBackObj.h"
#include "options.h"

namespace odb {
class dbDatabase;
class dbBlock;
class dbNet;
class dbInst;
class dbSite;
}  // namespace odb

namespace sta {
class dbSta;
class LibertyCell;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace gui {
class DbInstDescriptor;

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

  void paintEvent(QPaintEvent* event) override;
  Qt::BrushStyle pattern() const { return pattern_; }

 private:
  Qt::BrushStyle pattern_;
};

class DisplayColorDialog : public QDialog
{
  Q_OBJECT
 public:
  DisplayColorDialog(const QColor& color,
                     Qt::BrushStyle pattern,
                     QWidget* parent = nullptr);
  DisplayColorDialog(const QColor& color, QWidget* parent = nullptr);
  ~DisplayColorDialog() override;

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

  static inline const std::vector<std::vector<Qt::BrushStyle>> kBrushPatterns{
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

  QVariant data(const QModelIndex& index, int role) const override;
  QVariant headerData(int section,
                      Qt::Orientation orientation,
                      int role) const override;

 private:
  const int user_data_item_idx_;
};

// This class shows the user the set of layers & objects that
// they can control the visibility and selectablity of.  The
// controls are shown in a tree view to provide grouping of
// related options.
//
// It also implements the Options interface so that other clients can
// access the data.
class DisplayControls : public QDockWidget,
                        public Options,
                        public sta::dbNetworkObserver,
                        public odb::dbBlockCallBackObj
{
  Q_OBJECT

 public:
  // One leaf (non-group) row in the model
  struct ModelRow
  {
    QStandardItem* name = nullptr;
    QStandardItem* swatch = nullptr;
    QStandardItem* visible = nullptr;
    QStandardItem* selectable = nullptr;  // may be null
  };

  DisplayControls(QWidget* parent = nullptr);
  ~DisplayControls() override;

  bool eventFilter(QObject* obj, QEvent* event) override;

  void addTech(odb::dbTech* tech);
  void setLogger(utl::Logger* logger);
  void setSTA(sta::dbSta* sta);
  void setDBInstDescriptor(DbInstDescriptor* desciptor);

  void readSettings(QSettings* settings);
  void writeSettings(QSettings* settings);

  void setControlByPath(const std::string& path,
                        bool is_visible,
                        Qt::CheckState value);
  void setControlByPath(const std::string& path, const QColor& color);
  bool checkControlByPath(const std::string& path, bool is_visible);

  void registerRenderer(Renderer* renderer);
  void unregisterRenderer(Renderer* renderer);

  void save();
  void restore();

  void restoreTclCommands(std::vector<std::string>& cmds);

  // From the Options API
  QColor background() override;
  QColor color(const odb::dbTechLayer* layer) override;
  Qt::BrushStyle pattern(const odb::dbTechLayer* layer) override;
  QColor placementBlockageColor() override;
  Qt::BrushStyle placementBlockagePattern() override;
  QColor regionColor() override;
  Qt::BrushStyle regionPattern() override;
  QColor instanceNameColor() override;
  QFont instanceNameFont() override;
  QColor itermLabelColor() override;
  QFont itermLabelFont() override;
  QColor siteColor(odb::dbSite* site) override;
  bool isVisible(const odb::dbTechLayer* layer) override;
  bool isSelectable(const odb::dbTechLayer* layer) override;
  bool isNetVisible(odb::dbNet* net) override;
  bool isNetSelectable(odb::dbNet* net) override;
  bool isInstanceVisible(odb::dbInst* inst) override;
  bool isInstanceSelectable(odb::dbInst* inst) override;
  bool areInstanceNamesVisible() override;
  bool areInstancePinsVisible() override;
  bool areInstancePinsSelectable() override;
  bool areInstancePinNamesVisible() override;
  bool areInstanceBlockagesVisible() override;
  bool areBlockagesVisible() override;
  bool areBlockagesSelectable() override;
  bool areObstructionsVisible() override;
  bool areObstructionsSelectable() override;
  bool areSitesVisible() override;
  bool areSitesSelectable() override;
  bool isSiteVisible(odb::dbSite* site) override;
  bool isSiteSelectable(odb::dbSite* site) override;
  bool arePrefTracksVisible() override;
  bool areNonPrefTracksVisible() override;

  bool areIOPinsVisible() const override;
  bool areIOPinsSelectable() const override;
  bool areIOPinNamesVisible() const override;
  QFont ioPinMarkersFont() const override;

  bool areRoutingSegmentsVisible() const override;
  bool areRoutingViasVisible() const override;
  bool areSpecialRoutingSegmentsVisible() const override;
  bool areSpecialRoutingViasVisible() const override;
  bool areFillsVisible() const override;

  QColor rulerColor() override;
  QFont rulerFont() override;
  bool areRulersVisible() override;
  bool areRulersSelectable() override;

  QFont labelFont() override;
  bool areLabelsVisible() override;
  bool areLabelsSelectable() override;

  bool isDetailedVisibility() override;

  bool areSelectedVisible() override;

  bool isScaleBarVisible() const override;
  bool areAccessPointsVisible() const override;
  bool areRegionsVisible() const override;
  bool areRegionsSelectable() const override;
  bool isManufacturingGridVisible() const override;

  bool isModuleView() const override;

  bool isGCellGridVisible() const override;
  bool isFlywireHighlightOnly() const override;
  bool areFocusedNetsGuidesVisible() const override;

  // API from dbNetworkObserver
  void postReadLiberty() override;
  void postReadDb() override;

  // API from dbBlockCallBackObj
  void inDbRowCreate(odb::dbRow*) override;

 signals:
  // The display options have changed and clients need to update
  void changed();
  void colorChanged();

  // Emit a selected tech layer
  void selected(const Selected& selected);

 public slots:
  // Tells this widget that a new design is loaded and the
  // options displayed need to match
  void blockLoaded(odb::dbBlock* block);

  void setCurrentChip(odb::dbChip* chip);

  // This is called by the check boxes to update the state
  void itemChanged(QStandardItem* item);
  void displayItemSelected(const QItemSelection& selection);
  void displayItemDblClicked(const QModelIndex& index);

 private slots:
  void itemContextMenu(const QPoint& point);

 private:
  // The columns in the tree view
  enum Column
  {
    kName,
    kSwatch,
    kVisible,
    kSelectable
  };

  // The *Models are groups in the tree
  struct NetModels
  {
    ModelRow signal;
    ModelRow power;
    ModelRow ground;
    ModelRow clock;
    ModelRow reset;
    ModelRow tieoff;
    ModelRow scan;
    ModelRow analog;
  };

  struct LayerModels
  {
    ModelRow implant;
    ModelRow other;
  };

  struct InstanceModels
  {
    ModelRow stdcells;
    ModelRow blocks;
    ModelRow pads;
    ModelRow physical;
  };

  struct StdCellModels
  {
    ModelRow bufinv;
    ModelRow combinational;
    ModelRow sequential;
    ModelRow clock_tree;
    ModelRow level_shiters;
  };

  struct BufferInverterModels
  {
    ModelRow timing;
    ModelRow other;
  };

  struct ClockTreeModels
  {
    ModelRow bufinv;
    ModelRow clock_gates;
  };

  struct PadModels
  {
    ModelRow input;
    ModelRow output;
    ModelRow inout;
    ModelRow power;
    ModelRow spacer;
    ModelRow areaio;
    ModelRow other;
  };

  struct PhysicalModels
  {
    ModelRow fill;
    ModelRow endcap;
    ModelRow tap;
    ModelRow antenna;
    ModelRow tie;
    ModelRow cover;
    ModelRow bump;
    ModelRow other;
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
    ModelRow instances;
    ModelRow scale_bar;
    ModelRow access_points;
    ModelRow regions;
    ModelRow detailed;
    ModelRow selected;
    ModelRow module;
    ModelRow manufacturing_grid;
    ModelRow gcell_grid;
    ModelRow flywires_only;
    ModelRow labels;
    ModelRow background;
    ModelRow focused_nets_guides;
  };

  struct InstanceShapeModels
  {
    ModelRow names;
    ModelRow pins;
    ModelRow iterm_labels;
    ModelRow blockages;
  };

  struct RoutingModels
  {
    ModelRow segments;
    ModelRow vias;
  };

  struct IOPinModels
  {
    ModelRow names;
  };

  struct ShapeTypeModels
  {
    ModelRow routing_group;
    RoutingModels routing;
    ModelRow special_routing_group;
    RoutingModels special_routing;
    ModelRow pins;
    ModelRow pin_names;
    ModelRow fill;
  };

  void techInit(odb::dbTech* tech);
  void libInit(odb::dbDatabase* db);

  void collectControls(const QStandardItem* parent,
                       Column column,
                       std::map<std::string, QStandardItem*>& items,
                       const std::string& prefix = "");
  void findControlsInItems(const std::string& path,
                           Column column,
                           std::vector<QStandardItem*>& items);

  QStandardItem* makeParentItem(ModelRow& row,
                                const QString& text,
                                QStandardItem* parent,
                                Qt::CheckState checked,
                                bool add_selectable = false,
                                const QColor& color = Qt::transparent);

  void makeLeafItem(ModelRow& row,
                    const QString& text,
                    QStandardItem* parent,
                    std::optional<Qt::CheckState> checked,
                    bool add_selectable = false,
                    const QColor& color = Qt::transparent,
                    const QVariant& user_data = QVariant());

  QIcon makeSwatchIcon(const QColor& color);

  void toggleAllChildren(bool checked, QStandardItem* parent, Column column);
  void toggleParent(const QStandardItem* parent,
                    QStandardItem* parent_flag,
                    int column);
  void toggleParent(ModelRow& row);

  void readSettingsForRow(QSettings* settings,
                          const ModelRow& row,
                          bool include_children = true);
  void readSettingsForRow(QSettings* settings,
                          const QStandardItem* name,
                          QStandardItem* visible = nullptr,
                          QStandardItem* selectable = nullptr,
                          bool include_children = true);
  void writeSettingsForRow(QSettings* settings,
                           const ModelRow& row,
                           bool include_children = true);
  void writeSettingsForRow(QSettings* settings,
                           const QStandardItem* name,
                           const QStandardItem* visible = nullptr,
                           const QStandardItem* selectable = nullptr,
                           bool include_children = true);

  void buildRestoreTclCommands(std::vector<std::string>& cmds,
                               const QStandardItem* parent,
                               const std::string& prefix = "");

  void saveRendererState(Renderer* renderer);

  void setNameItemDoubleClickAction(ModelRow& row,
                                    const std::function<void()>& callback);
  void setItemExclusivity(ModelRow& row,
                          const std::set<std::string>& exclusivity);

  void createLayerMenu();
  void layerShowOnlySelectedNeighbors(int lower, int upper);
  void collectNeighboringLayers(odb::dbTechLayer* layer,
                                int lower,
                                int upper,
                                std::set<const odb::dbTechLayer*>& layers);
  void setOnlyVisibleLayers(const std::set<const odb::dbTechLayer*>& layers);

  const ModelRow* getLayerRow(const odb::dbTechLayer* layer) const;
  const ModelRow* getSiteRow(odb::dbSite* site) const;
  const ModelRow* getInstRow(odb::dbInst* inst) const;
  const ModelRow* getNetRow(odb::dbNet* net) const;

  bool isModelRowVisible(const ModelRow* row) const;
  bool isModelRowSelectable(const ModelRow* row) const;

  void checkLiberty(bool assume_loaded = false);

  std::pair<QColor*, Qt::BrushStyle*> lookupColor(QStandardItem* item,
                                                  const QModelIndex* index
                                                  = nullptr);

  QTreeView* view_;
  DisplayControlModel* model_;
  QMenu* routing_layers_menu_;
  QMenu* layers_menu_;
  odb::dbTechLayer* layers_menu_layer_;

  bool ignore_callback_;
  bool ignore_selection_;

  QColor default_site_color_;

  // Categories in the model
  ModelRow layers_group_;
  ModelRow tracks_group_;
  ModelRow nets_group_;
  ModelRow instance_group_;
  ModelRow blockage_group_;
  ModelRow misc_group_;
  ModelRow site_group_;
  ModelRow shape_type_group_;

  // instances
  LayerModels layers_;
  InstanceModels instances_;
  StdCellModels stdcell_instances_;
  BufferInverterModels bufinv_instances_;
  ClockTreeModels clock_tree_instances_;
  PadModels pad_instances_;
  PhysicalModels physical_instances_;

  InstanceShapeModels instance_shapes_;
  ShapeTypeModels shape_types_;

  // Object controls
  NetModels nets_;
  ModelRow rulers_;
  BlockageModels blockages_;
  TrackModels tracks_;
  MiscModels misc_;

  std::map<const odb::dbTechLayer*, ModelRow> layer_controls_;
  std::map<const odb::dbSite*, ModelRow> site_controls_;
  int custom_controls_start_;
  std::map<Renderer*, std::vector<ModelRow>> custom_controls_;
  std::map<std::string, Renderer::Settings> custom_controls_settings_;
  std::map<QStandardItem*, Qt::CheckState> saved_state_;

  std::set<odb::dbTech*> techs_;
  utl::Logger* logger_;
  sta::dbSta* sta_;
  DbInstDescriptor* inst_descriptor_;

  std::map<const odb::dbTechLayer*, QColor> layer_color_;
  std::map<const odb::dbTechLayer*, Qt::BrushStyle> layer_pattern_;

  std::map<const odb::dbSite*, QColor> site_color_;

  QColor background_color_;

  QColor placement_blockage_color_;
  Qt::BrushStyle placement_blockage_pattern_;

  QColor instance_name_color_;
  QFont instance_name_font_;
  QColor iterm_label_color_;
  QFont iterm_label_font_;

  QColor ruler_color_;
  QFont ruler_font_;

  QFont label_font_;

  QColor region_color_;
  Qt::BrushStyle region_pattern_;

  QFont pin_markers_font_;

  static constexpr int kUserDataItemIdx = Qt::UserRole;
  static constexpr int kCallbackItemIdx = Qt::UserRole + 1;
  static constexpr int kDoubleclickItemIdx = Qt::UserRole + 2;
  static constexpr int kExclusivityItemIdx = Qt::UserRole + 3;
  static constexpr int kDisableRowItemIdx = Qt::UserRole + 4;
};

}  // namespace gui

Q_DECLARE_METATYPE(gui::Callback);
