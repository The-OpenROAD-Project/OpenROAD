/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

#include "globalConnectDialog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "utl/Logger.h"

Q_DECLARE_METATYPE(odb::dbNet*);
Q_DECLARE_METATYPE(odb::dbRegion*);

namespace gui {

enum class GlobalConnectField
{
  Instance,
  Pin,
  Net,
  Region,
  Run,
  Remove
};

static int toValue(GlobalConnectField f)
{
  return static_cast<int>(f);
}

/////////

GlobalConnectDialog::GlobalConnectDialog(odb::dbBlock* block, QWidget* parent)
    : QDialog(parent),
      block_(block),
      layout_(new QGridLayout),
      add_(new QPushButton(this)),
      clear_(new QPushButton(this)),
      run_(new QPushButton(this)),
      rules_({}),
      inst_pattern_(new QLineEdit(this)),
      pin_pattern_(new QLineEdit(this)),
      net_(new QComboBox(this)),
      region_(new QComboBox(this)),
      connections_(new QLabel(this))
{
  setWindowTitle("Global Connect Rules");

  QVBoxLayout* layout = new QVBoxLayout;
  layout->addLayout(layout_);
  QHBoxLayout* button_layout = new QHBoxLayout;
  button_layout->addWidget(clear_);
  button_layout->addWidget(run_);
  layout->addLayout(button_layout);
  layout->addWidget(connections_);

  setLayout(layout);

  add_->setIcon(QIcon(":/add.png"));
  add_->setToolTip(tr("Run"));
  add_->setAutoDefault(false);
  add_->setDefault(false);
  add_->setEnabled(false);

  run_->setIcon(QIcon(":/play.png"));
  run_->setToolTip(tr("Run"));
  run_->setAutoDefault(false);
  run_->setDefault(false);

  clear_->setIcon(QIcon(":/delete.png"));
  clear_->setToolTip(tr("Clear"));
  clear_->setAutoDefault(false);
  clear_->setDefault(false);

  connect(add_, &QPushButton::pressed, this, &GlobalConnectDialog::makeRule);
  connect(run_, &QPushButton::pressed, this, &GlobalConnectDialog::runRules);
  connect(
      clear_, &QPushButton::pressed, this, &GlobalConnectDialog::clearRules);

  layout_->addWidget(new QLabel("Instance pattern", this),
                     0,
                     toValue(GlobalConnectField::Instance),
                     Qt::AlignCenter);
  layout_->addWidget(new QLabel("Pin pattern", this),
                     0,
                     toValue(GlobalConnectField::Pin),
                     Qt::AlignCenter);
  layout_->addWidget(new QLabel("Net", this),
                     0,
                     toValue(GlobalConnectField::Net),
                     Qt::AlignCenter);
  layout_->addWidget(new QLabel("Region", this),
                     0,
                     toValue(GlobalConnectField::Region),
                     Qt::AlignCenter);
  for (auto* rule : block_->getGlobalConnects()) {
    addRule(rule);
  }

  region_->addItem("All",
                   QVariant::fromValue(static_cast<odb::dbRegion*>(nullptr)));
  for (auto* region : block_->getRegions()) {
    region_->addItem(QString::fromStdString(region->getName()),
                     QVariant::fromValue(static_cast<odb::dbRegion*>(region)));
  }

  for (auto* net : block_->getNets()) {
    if (!net->isSpecial()) {
      continue;
    }
    net_->addItem(QString::fromStdString(net->getName()),
                  QVariant::fromValue(static_cast<odb::dbNet*>(net)));
  }

  const int row_idx = rules_.size() + 1;
  layout_->addWidget(inst_pattern_,
                     row_idx,
                     toValue(GlobalConnectField::Instance),
                     Qt::AlignCenter);
  layout_->addWidget(
      pin_pattern_, row_idx, toValue(GlobalConnectField::Pin), Qt::AlignCenter);
  layout_->addWidget(
      net_, row_idx, toValue(GlobalConnectField::Net), Qt::AlignCenter);
  layout_->addWidget(
      region_, row_idx, toValue(GlobalConnectField::Region), Qt::AlignCenter);
  layout_->addWidget(
      add_, row_idx, toValue(GlobalConnectField::Run), Qt::AlignCenter);

  connect(this,
          &GlobalConnectDialog::connectionsMade,
          this,
          &GlobalConnectDialog::announceConnections);
  connect(inst_pattern_,
          &QLineEdit::textChanged,
          this,
          &GlobalConnectDialog::addRegexTextChanged);
  connect(pin_pattern_,
          &QLineEdit::textChanged,
          this,
          &GlobalConnectDialog::addRegexTextChanged);
}

void GlobalConnectDialog::runRules()
{
  runRule(nullptr);
}

void GlobalConnectDialog::clearRules()
{
  auto rule_set = block_->getGlobalConnects();
  std::set<odb::dbGlobalConnect*> rules(rule_set.begin(), rule_set.end());
  for (auto* rule : rules) {
    deleteRule(rule);
  }
}

void GlobalConnectDialog::deleteRule(odb::dbGlobalConnect* gc)
{
  auto& widgets = rules_[gc];

  layout_->removeWidget(widgets.inst_pattern);
  layout_->removeWidget(widgets.pin_pattern);
  layout_->removeWidget(widgets.net);
  layout_->removeWidget(widgets.region);
  layout_->removeWidget(widgets.run);
  layout_->removeWidget(widgets.remove);

  delete widgets.inst_pattern;
  delete widgets.pin_pattern;
  delete widgets.net;
  delete widgets.region;
  delete widgets.run;
  delete widgets.remove;

  rules_.erase(gc);

  odb::dbGlobalConnect::destroy(gc);

  adjustSize();
}

void GlobalConnectDialog::addRule(odb::dbGlobalConnect* gc)
{
  const int row_idx = rules_.size() + 1;
  auto& widgets = rules_[gc];

  auto setup_line = [](QLineEdit* line) {
    line->setReadOnly(true);
    line->setStyleSheet("color: black; background-color: lightGray");
  };

  widgets.inst_pattern = new QLineEdit(this);
  setup_line(widgets.inst_pattern);
  widgets.inst_pattern->setText(QString::fromStdString(gc->getInstPattern()));
  layout_->addWidget(widgets.inst_pattern,
                     row_idx,
                     toValue(GlobalConnectField::Instance),
                     Qt::AlignCenter);
  widgets.pin_pattern = new QLineEdit(this);
  setup_line(widgets.pin_pattern);
  widgets.pin_pattern->setText(QString::fromStdString(gc->getPinPattern()));
  layout_->addWidget(widgets.pin_pattern,
                     row_idx,
                     toValue(GlobalConnectField::Pin),
                     Qt::AlignCenter);
  widgets.net = new QLineEdit(this);
  setup_line(widgets.net);
  widgets.net->setText(QString::fromStdString(gc->getNet()->getName()));
  layout_->addWidget(
      widgets.net, row_idx, toValue(GlobalConnectField::Net), Qt::AlignCenter);

  widgets.region = new QLineEdit(this);
  odb::dbRegion* dbregion = gc->getRegion();
  if (dbregion == nullptr) {
    widgets.region->setText("All");
  } else {
    widgets.region->setText(QString::fromStdString(dbregion->getName()));
  }
  setup_line(widgets.region);
  layout_->addWidget(widgets.region,
                     row_idx,
                     toValue(GlobalConnectField::Region),
                     Qt::AlignCenter);

  widgets.run = new QPushButton(this);
  widgets.run->setIcon(QIcon(":/play.png"));
  widgets.run->setToolTip(tr("Run"));
  widgets.run->setAutoDefault(false);
  widgets.run->setDefault(false);
  connect(widgets.run, &QPushButton::pressed, [this, gc]() { runRule(gc); });
  layout_->addWidget(
      widgets.run, row_idx, toValue(GlobalConnectField::Run), Qt::AlignCenter);

  widgets.remove = new QPushButton(this);
  widgets.remove->setIcon(QIcon(":/delete.png"));
  widgets.remove->setToolTip(tr("Delete"));
  widgets.remove->setAutoDefault(false);
  widgets.remove->setDefault(false);
  connect(
      widgets.remove, &QPushButton::pressed, [this, gc]() { deleteRule(gc); });
  layout_->addWidget(widgets.remove,
                     row_idx,
                     toValue(GlobalConnectField::Remove),
                     Qt::AlignCenter);

  adjustSize();
}

void GlobalConnectDialog::makeRule()
{
  const std::string inst_pattern = inst_pattern_->text().toStdString();
  const std::string pin_pattern = pin_pattern_->text().toStdString();
  odb::dbNet* net = net_->currentData().value<odb::dbNet*>();
  odb::dbRegion* region = region_->currentData().value<odb::dbRegion*>();

  try {
    auto* rule
        = odb::dbGlobalConnect::create(net, region, inst_pattern, pin_pattern);
    addRule(rule);
  } catch (const std::runtime_error& error) {
    QMessageBox::critical(this, error.what(), "Failed to add rule.");
  }

  layout_->removeWidget(inst_pattern_);
  layout_->removeWidget(pin_pattern_);
  layout_->removeWidget(net_);
  layout_->removeWidget(region_);
  layout_->removeWidget(add_);

  const int row_idx = rules_.size() + 1;
  layout_->addWidget(inst_pattern_,
                     row_idx,
                     toValue(GlobalConnectField::Instance),
                     Qt::AlignCenter);
  layout_->addWidget(
      pin_pattern_, row_idx, toValue(GlobalConnectField::Pin), Qt::AlignCenter);
  layout_->addWidget(
      net_, row_idx, toValue(GlobalConnectField::Net), Qt::AlignCenter);
  layout_->addWidget(
      region_, row_idx, toValue(GlobalConnectField::Region), Qt::AlignCenter);
  layout_->addWidget(
      add_, row_idx, toValue(GlobalConnectField::Run), Qt::AlignCenter);

  inst_pattern_->clear();
  pin_pattern_->clear();
  add_->setEnabled(false);

  adjustSize();
}

void GlobalConnectDialog::announceConnections(int connections)
{
  connections_->setText(
      QString::fromStdString(fmt::format("Connected {} pin(s)", connections)));
}

void GlobalConnectDialog::runRule(odb::dbGlobalConnect* gc)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  QApplication::processEvents();
  int connections = 0;
  if (gc == nullptr) {
    connections = block_->globalConnect();
  } else {
    connections = block_->globalConnect(gc);
  }
  QApplication::restoreOverrideCursor();
  emit connectionsMade(connections);
}

void GlobalConnectDialog::addRegexTextChanged(const QString& text)
{
  // test if this is a valid regex
  try {
    std::regex(text.toStdString());
  } catch (const std::regex_error&) {
    add_->setEnabled(false);
    return;
  }

  const bool enable
      = !inst_pattern_->text().isEmpty() && !pin_pattern_->text().isEmpty();
  add_->setEnabled(enable);
}

}  // namespace gui
