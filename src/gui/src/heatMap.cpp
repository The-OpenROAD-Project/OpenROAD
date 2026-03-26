// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2026, The OpenROAD Authors

#include "gui/heatMap.h"

#include <QDialog>
#include <QObject>
#include <QString>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "heatMapGui.h"
#include "heatMapSetup.h"

namespace gui {

namespace {

class HeatMapRenderer : public Renderer
{
 public:
  explicit HeatMapRenderer(HeatMapDataSource& datasource)
      : datasource_(datasource), first_paint_(true)
  {
    addDisplayControl(datasource_.getName(),
                      false,
                      [this]() { datasource_.showSetup(); },
                      {""});
  }

  const char* getDisplayControlGroupName() override { return "Heat Maps"; }

  void drawObjects(Painter& painter) override
  {
    if (!checkDisplayControl(datasource_.getName())) {
      if (!first_paint_) {
        first_paint_ = true;
        datasource_.onHide();
      }
      return;
    }

    if (first_paint_) {
      first_paint_ = false;
      datasource_.onShow();
    }

    for (const auto& map_point : datasource_.getVisibleMap(
             painter.getBounds(), painter.getPixelsPerDBU())) {
      painter.setPen(map_point.color, true);
      painter.setBrush(map_point.color);
      painter.drawRect(map_point.rect);

      if (datasource_.getShowNumbers()) {
        const int x = map_point.rect.xCenter();
        const int y = map_point.rect.yCenter();
        const Painter::Anchor text_anchor = Painter::Anchor::kCenter;
        const double text_rect_margin = 0.8;

        const std::string text
            = datasource_.formatValue(map_point.value, false);
        const odb::Rect text_bound
            = painter.stringBoundaries(x, y, text_anchor, text);
        if (text_bound.dx() < text_rect_margin * map_point.rect.dx()
            && text_bound.dy() < text_rect_margin * map_point.rect.dy()) {
          painter.setPen(Painter::kWhite, true);
          painter.drawString(x, y, text_anchor, text);
        }
      }
    }

    if (datasource_.getShowLegend()) {
      std::vector<std::pair<int, std::string>> legend;
      for (const auto& [color_index, color_value] :
           datasource_.getLegendValues()) {
        legend.emplace_back(color_index,
                            datasource_.formatValue(color_value, true));
      }
      datasource_.getColorGenerator().drawLegend(painter, legend);
    }
  }

  std::string getSettingsGroupName() override
  {
    return kGroupnamePrefix + datasource_.getSettingsGroupName();
  }

  Settings getSettings() override
  {
    Settings settings = Renderer::getSettings();
    for (const auto& [name, value] : datasource_.getSettings()) {
      settings[kDatasourcePrefix + name] = value;
    }
    return settings;
  }

  void setSettings(const Settings& settings) override
  {
    Renderer::setSettings(settings);
    Settings data_settings;
    for (const auto& [name, value] : settings) {
      if (name.starts_with(kDatasourcePrefix)) {
        data_settings[name.substr(strlen(kDatasourcePrefix))] = value;
      }
    }
    datasource_.setSettings(data_settings);
  }

 private:
  HeatMapDataSource& datasource_;
  bool first_paint_;

  static constexpr char kDatasourcePrefix[] = "data#";
  static constexpr char kGroupnamePrefix[] = "HeatMap#";
};

using SetupMap = std::map<HeatMapDataSource*, HeatMapSetup*>;

SetupMap& activeSetups()
{
  static SetupMap setups;
  return setups;
}

}  // namespace

std::unique_ptr<Renderer> makeHeatMapRenderer(HeatMapDataSource& datasource)
{
  return std::make_unique<HeatMapRenderer>(datasource);
}

void showHeatMapSetupDialog(HeatMapDataSource* source)
{
  if (source == nullptr || source->getBlock() == nullptr) {
    return;
  }

  auto& setups = activeSetups();
  if (auto found = setups.find(source);
      found != setups.end() && found->second != nullptr) {
    found->second->raise();
    return;
  }

  auto* setup = new HeatMapSetup(*source,
                                 QString::fromStdString(source->getName()),
                                 source->getUseDBU(),
                                 source->getBlock()->getDbUnitsPerMicron());
  setups[source] = setup;

  QObject::connect(setup, &QDialog::finished, setup, &QObject::deleteLater);
  QObject::connect(
      setup, &QObject::destroyed, [source]() { activeSetups().erase(source); });
  setup->show();
}

void HeatMapDataSource::registerHeatMap()
{
  Gui::get()->registerHeatMap(this);
}

}  // namespace gui
