// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#pragma once

#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <set>

#include "odb/db.h"
#include "odb/dbObject.h"

namespace sta {
class dbSta;
}

namespace gui {

class InsertBufferDialog : public QDialog
{
  Q_OBJECT

 public:
  InsertBufferDialog(odb::dbNet* net,
                     sta::dbSta* sta,
                     QWidget* parent = nullptr);

  odb::dbMaster* getSelectedMaster() const;
  void getSelection(odb::dbObject*& driver,
                    std::set<odb::dbObject*>& loads) const;
  QString getBufferName() const;
  QString getNetName() const;

 private slots:
  void toggleOkButton();
  void handlePinClicked(QListWidgetItem* item);

 private:
  void populatePins();
  void populateMasters();

  odb::dbNet* net_;
  sta::dbSta* sta_;

  QListWidget* pin_list_;
  QComboBox* master_combo_;
  QLineEdit* buffer_name_edit_;
  QLineEdit* net_name_edit_;
  QPushButton* ok_button_;
};

}  // namespace gui
