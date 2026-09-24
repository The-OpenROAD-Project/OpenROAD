// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2026, The OpenROAD Authors

#include "web/heatMap.h"

#include <QDialog>
#include <QObject>
#include <QString>
#include <map>

#include "gui/gui.h"
#include "heatMapGui.h"
#include "heatMapSetup.h"

namespace gui {

namespace {

using SetupMap = std::map<web::HeatMapDataSource*, HeatMapSetup*>;

SetupMap& activeSetups()
{
  static SetupMap setups;
  return setups;
}

}  // namespace

void showHeatMapSetupDialog(web::HeatMapDataSource* source)
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

}  // namespace gui
