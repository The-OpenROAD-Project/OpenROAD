// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "qtDialogs.h"

#include <QDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <exception>
#include <optional>
#include <string>
#include <vector>

#include "gui/gui.h"
#include "heatMapGui.h"
#include "insertBufferDialog.h"
#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbObject.h"

namespace gui {

std::optional<int> QtDialogs::chooseItem(const std::string& title,
                                         const std::string& label,
                                         const std::vector<std::string>& items,
                                         int current)
{
  QStringList choices;
  choices.reserve(items.size());
  for (const auto& item : items) {
    choices.append(QString::fromStdString(item));
  }

  bool okay = false;
  const QString selection = QInputDialog::getItem(nullptr,
                                                  QString::fromStdString(title),
                                                  QString::fromStdString(label),
                                                  choices,
                                                  current,
                                                  false,
                                                  &okay);
  if (!okay) {
    return {};
  }
  const int index = choices.indexOf(selection);
  if (index == -1) {
    return {};
  }
  return index;
}

odb::dbInst* QtDialogs::insertBuffer(odb::dbNet* net, sta::dbSta* sta)
{
  InsertBufferDialog dialog(net, sta, nullptr);
  if (dialog.exec() != QDialog::Accepted) {
    return nullptr;
  }

  odb::dbMaster* master = dialog.getSelectedMaster();
  odb::dbObject* driver = nullptr;
  odb::PtrSet<odb::dbObject> loads;
  dialog.getSelection(driver, loads);

  const std::string buf_name = dialog.getBufferName().toStdString();
  const std::string net_name = dialog.getNetName().toStdString();
  const char* buf_p = buf_name.empty() ? kDefaultBufBaseName : buf_name.c_str();
  const char* net_p = net_name.empty() ? kDefaultNetBaseName : net_name.c_str();

  odb::dbInst* buffer_inst = nullptr;
  try {
    if (driver) {
      buffer_inst
          = net->insertBufferAfterDriver(driver,
                                         master,
                                         nullptr,
                                         buf_p,
                                         net_p,
                                         odb::dbNameUniquifyType::IF_NEEDED);
    } else if (!loads.empty()) {
      buffer_inst
          = net->insertBufferBeforeLoads(loads,
                                         master,
                                         nullptr,
                                         buf_p,
                                         net_p,
                                         odb::dbNameUniquifyType::IF_NEEDED);
    }
  } catch (const std::exception& e) {
    QMessageBox::critical(nullptr, "Error", e.what());
    return nullptr;
  }

  web::Gui::get()->redraw();
  return buffer_inst;
}

void QtDialogs::showHeatMapSetup(web::HeatMapDataSource* source)
{
  showHeatMapSetupDialog(source);
}

}  // namespace gui
