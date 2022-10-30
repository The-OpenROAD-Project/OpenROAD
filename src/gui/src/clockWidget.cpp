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

#include <QGraphicsRectItem>
#include <QVBoxLayout>

#include "clockWidget.h"
#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "sta/ClkNetwork.hh"
#include "sta/Liberty.hh"
#include "sta/Sdc.hh"
#include "utl/Logger.h"

namespace gui {

ClockWidget::ClockWidget(QWidget* parent)
    : QDockWidget("Clock Tree Viewer", parent),
      logger_(nullptr),
      block_(nullptr),
      sta_(nullptr),
      scene_(new QGraphicsScene(this)),
      view_(new QGraphicsView(scene_, this)),
      update_button_(new QPushButton("Update", this))
{
  setObjectName("clock_viewer");  // for settings

  QWidget* container = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout;
  layout->addWidget(update_button_);
  layout->addWidget(view_);

  container->setLayout(layout);
  setWidget(container);

  connect(update_button_, SIGNAL(clicked()), this, SLOT(populate()));

  // FIXME: temp scene
  auto item = new QGraphicsRectItem(5, 5, 50, 50);
  scene_->addItem(item);

  auto item2 = new QGraphicsRectItem(500, 5, 50, 50);
  scene_->addItem(item2);
}

void ClockWidget::setSTA(sta::dbSta* sta)
{
  sta_ = sta;
}

void ClockWidget::setLogger(utl::Logger* logger)
{
  logger_ = logger;
}

void ClockWidget::setBlock(odb::dbBlock* block)
{
  block_ = block;
}

void ClockWidget::updateSelection(const Selected& selection)
{
  // FIXME
}

struct Tree
{
  sta::PinSet drivers;
  sta::Net* net;
  std::map<sta::Net*, Tree> fanout;
  sta::PinSet leaves;
};

// Assumes net & drivers of 'tree' are handled by the caller and fills
// out the fanout.
void buildTreeFanout(const sta::PinSet* clk_pins,
                     sta::dbNetwork* network,
                     Tree& tree,
                     utl::Logger* logger)
{
  std::unique_ptr<sta::NetPinIterator> net_iter(network->pinIterator(tree.net));
  while (net_iter->hasNext()) {
    sta::Pin* pin = net_iter->next();
    if (tree.drivers.hasKey(pin)) {
      continue;
    }
    sta::Instance* inst = network->instance(pin);
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network->pinIterator(inst));
    while (pin_iter->hasNext()) {
      sta::Pin* inst_pin = pin_iter->next();
      if (!clk_pins->hasKey(inst_pin)) {
        continue;
      }
      auto inst_pin_net = network->net(inst_pin);
      if (inst_pin_net == tree.net) {
        continue;
      }
      if (tree.fanout.find(inst_pin_net) == tree.fanout.end()) {
        auto& tree2 = tree.fanout[inst_pin_net];
        tree2.drivers.insert(inst_pin);
        tree2.net = inst_pin_net;
        buildTreeFanout(clk_pins, network, tree2, logger);
      }
      tree.fanout[inst_pin_net].drivers.insert(inst_pin);
      logger->report("  pin = {} net={}",
                     network->name(inst_pin),
                     inst_pin_net ? network->name(inst_pin_net) : "NULL");
    }
  }
}

void ClockWidget::populate()
{
  Pins roots;
  sta::Sdc* sdc = sta_->sdc();
  sta::dbNetwork* network = sta_->getDbNetwork();
  sta_->ensureClkNetwork();
  for (auto clk : *sdc->clocks()) {
    std::string clkName = clk->name();
    logger_->report("clk = {}", clkName);

    Tree root;
    root.drivers = clk->leafPins();
    auto clk_pins = sta_->clkNetwork()->pins(clk);
    for (sta::Pin* pin : root.drivers) {
      auto net = network->net(pin);
      if (!net) {
        sta::Term* term = network->term(pin);
        net = term ? network->net(term) : nullptr;
      }
      if (net) {
        root.net = net;
        break;
      }
    }
    if (!root.net) {
      logger_->error(utl::GUI, 74, "no net for clk {}", clkName);
    }
    buildTreeFanout(clk_pins, network, root, logger_);
  }
}

}  // namespace gui
