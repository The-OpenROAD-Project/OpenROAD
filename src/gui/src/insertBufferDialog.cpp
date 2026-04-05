// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "insertBufferDialog.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QStringListModel>
#include <algorithm>
#include <set>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "sta/Liberty.hh"
#include "utl/algorithms.h"

Q_DECLARE_METATYPE(odb::dbObject*);

namespace gui {

InsertBufferDialog::InsertBufferDialog(odb::dbNet* net,
                                       sta::dbSta* sta,
                                       QWidget* parent)
    : QDialog(parent),
      net_(net),
      sta_(sta),
      pin_list_(new QListWidget(this)),
      master_combo_(new QComboBox(this)),
      buffer_name_edit_(new QLineEdit(this)),
      net_name_edit_(new QLineEdit(this)),
      ok_button_(new QPushButton("OK", this))
{
  setWindowTitle("Insert Buffer on Net: "
                 + QString::fromStdString(net->getName()));

  QVBoxLayout* layout = new QVBoxLayout(this);

  QGroupBox* pin_group = new QGroupBox("Select Pin(s) to Buffer", this);
  QVBoxLayout* pin_layout = new QVBoxLayout(pin_group);
  pin_list_->setSelectionMode(QAbstractItemView::MultiSelection);
  connect(pin_list_,
          &QListWidget::itemClicked,
          this,
          &InsertBufferDialog::handlePinClicked);
  populatePins();
  pin_layout->addWidget(pin_list_);
  layout->addWidget(pin_group);

  QFormLayout* form_layout = new QFormLayout();
  populateMasters();
  form_layout->addRow("Buffer Master:", master_combo_);

  buffer_name_edit_->setPlaceholderText("Optional: new buffer name");
  form_layout->addRow("Buffer Name:", buffer_name_edit_);

  net_name_edit_->setPlaceholderText("Optional: new net name");
  form_layout->addRow("Net Name:", net_name_edit_);

  layout->addLayout(form_layout);

  QHBoxLayout* button_layout = new QHBoxLayout();
  QPushButton* cancel_button = new QPushButton("Cancel", this);
  button_layout->addStretch();
  button_layout->addWidget(cancel_button);
  button_layout->addWidget(ok_button_);
  layout->addLayout(button_layout);

  connect(ok_button_, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);
  connect(pin_list_,
          &QListWidget::itemSelectionChanged,
          this,
          &InsertBufferDialog::toggleOkButton);
  connect(master_combo_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &InsertBufferDialog::toggleOkButton);

  toggleOkButton();
}

void InsertBufferDialog::populatePins()
{
  auto is_source_iterm = [](odb::dbITerm* iterm) -> bool {
    const auto iotype = iterm->getIoType();
    return iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT;
  };
  auto is_sink_iterm = [](odb::dbITerm* iterm) -> bool {
    const auto iotype = iterm->getIoType();
    return iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT;
  };
  auto is_source_bterm = [](odb::dbBTerm* bterm) -> bool {
    const auto iotype = bterm->getIoType();
    return iotype == odb::dbIoType::INPUT || iotype == odb::dbIoType::INOUT
           || iotype == odb::dbIoType::FEEDTHRU;
  };
  auto is_sink_bterm = [](odb::dbBTerm* bterm) -> bool {
    const auto iotype = bterm->getIoType();
    return iotype == odb::dbIoType::OUTPUT || iotype == odb::dbIoType::INOUT
           || iotype == odb::dbIoType::FEEDTHRU;
  };

  struct PinInfo
  {
    QString label;
    odb::dbObject* object;
    bool is_driver;
  };
  std::vector<PinInfo> pins;

  for (auto* iterm : net_->getITerms()) {
    QString label = QString::fromStdString(iterm->getInst()->getName() + "/"
                                           + iterm->getMTerm()->getName());
    bool source = is_source_iterm(iterm);
    bool sink = is_sink_iterm(iterm);
    if (source && sink) {
      label += " (Driver/Load)";
    } else if (source) {
      label += " (Driver)";
    } else if (sink) {
      label += " (Load)";
    }
    pins.push_back({label, iterm, source});
  }

  for (auto* bterm : net_->getBTerms()) {
    QString label = QString::fromStdString(bterm->getName());
    bool source = is_source_bterm(bterm);
    bool sink = is_sink_bterm(bterm);
    if (source && sink) {
      label += " (Driver/Load Port)";
    } else if (source) {
      label += " (Driver Port)";
    } else if (sink) {
      label += " (Load Port)";
    }
    pins.push_back({label, bterm, source});
  }

  std::ranges::sort(pins, [](const PinInfo& a, const PinInfo& b) {
    if (a.is_driver != b.is_driver) {
      return a.is_driver;
    }
    return utl::natural_compare(a.label.toStdString(), b.label.toStdString());
  });

  for (const auto& pin : pins) {
    QListWidgetItem* item = new QListWidgetItem(pin.label, pin_list_);
    item->setData(Qt::UserRole, QVariant::fromValue(pin.object));
    item->setData(Qt::UserRole + 1, pin.is_driver);
    if (pin.is_driver) {
      item->setSelected(true);
    }
  }
}

void InsertBufferDialog::handlePinClicked(QListWidgetItem* item)
{
  if (!item->isSelected()) {
    toggleOkButton();
    return;
  }
  bool is_driver = item->data(Qt::UserRole + 1).toBool();
  if (is_driver) {
    for (int i = 0; i < pin_list_->count(); ++i) {
      QListWidgetItem* it = pin_list_->item(i);
      if (it != item && !it->data(Qt::UserRole + 1).toBool()) {
        it->setSelected(false);
      }
    }
  } else {
    for (int i = 0; i < pin_list_->count(); ++i) {
      QListWidgetItem* it = pin_list_->item(i);
      if (it->data(Qt::UserRole + 1).toBool()) {
        it->setSelected(false);
      }
    }
  }
  toggleOkButton();
}

void InsertBufferDialog::populateMasters()
{
  sta::dbNetwork* network = sta_->getDbNetwork();
  odb::dbDatabase* db = net_->getDb();

  std::vector<odb::dbMaster*> masters;
  for (auto* lib : db->getLibs()) {
    for (auto* master : lib->getMasters()) {
      sta::LibertyCell* lib_cell
          = network->libertyCell(network->dbToSta(master));
      if (lib_cell && lib_cell->isBuffer()) {
        masters.push_back(master);
      }
    }
  }

  std::ranges::sort(masters, [](odb::dbMaster* a, odb::dbMaster* b) {
    return utl::natural_compare(a->getName(), b->getName());
  });

  for (auto* master : masters) {
    master_combo_->addItem(QString::fromStdString(master->getName()),
                           QVariant::fromValue(static_cast<void*>(master)));
  }
}

void InsertBufferDialog::toggleOkButton()
{
  bool pin_selected = !pin_list_->selectedItems().isEmpty();
  bool master_selected = master_combo_->currentIndex() != -1;
  ok_button_->setEnabled(pin_selected && master_selected);
}

odb::dbMaster* InsertBufferDialog::getSelectedMaster() const
{
  if (master_combo_->currentIndex() == -1) {
    return nullptr;
  }
  return static_cast<odb::dbMaster*>(
      master_combo_->currentData().value<void*>());
}

void InsertBufferDialog::getSelection(odb::dbObject*& driver,
                                      std::set<odb::dbObject*>& loads) const
{
  driver = nullptr;
  loads.clear();
  for (auto* item : pin_list_->selectedItems()) {
    odb::dbObject* pin = item->data(Qt::UserRole).value<odb::dbObject*>();
    bool is_drvr = item->data(Qt::UserRole + 1).toBool();

    if (is_drvr) {
      driver = pin;
    } else {
      loads.insert(pin);
    }
  }
}

QString InsertBufferDialog::getBufferName() const
{
  return buffer_name_edit_->text();
}

QString InsertBufferDialog::getNetName() const
{
  return net_name_edit_->text();
}

}  // namespace gui
