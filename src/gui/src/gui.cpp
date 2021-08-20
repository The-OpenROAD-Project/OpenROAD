///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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

#include "gui/gui.h"

#include <QApplication>
#include <QDebug>
#include <boost/algorithm/string/predicate.hpp>
#include <stdexcept>
#include <string>

#include "db.h"
#include "dbDescriptors.h"
#include "dbShape.h"
#include "defin.h"
#include "displayControls.h"
#include "geom.h"
#include "layoutViewer.h"
#include "lefin.h"
#include "mainWindow.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace gui {

static odb::dbBlock* getBlock(odb::dbDatabase* db)
{
  if (!db) {
    return nullptr;
  }

  auto chip = db->getChip();
  if (!chip) {
    return nullptr;
  }

  return chip->getBlock();
}

// This provides the link for Gui::redraw to the widget
static gui::MainWindow* main_window = nullptr;

Gui* Gui::singleton_ = nullptr;

Gui* Gui::get()
{
  if (main_window == nullptr) {
    return nullptr;  // batch mode
  }

  if (!singleton_) {
    singleton_ = new Gui();
  }

  return singleton_;
}

void Gui::registerRenderer(Renderer* renderer)
{
  renderers_.insert(renderer);
  redraw();
}

void Gui::unregisterRenderer(Renderer* renderer)
{
  renderers_.erase(renderer);
  redraw();
}

void Gui::redraw()
{
  main_window->redraw();
}

void Gui::status(const std::string& message)
{
  main_window->status(message);
}

void Gui::pause()
{
  main_window->pause();
}

Selected Gui::makeSelected(std::any object, void* additional_data)
{
  return main_window->makeSelected(object, additional_data);
}

void Gui::setSelected(Selected selection)
{
  main_window->setSelected(selection);
}

void Gui::addSelectedNet(const char* name)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  auto net = block->findNet(name);
  if (!net) {
    return;
  }

  main_window->addSelected(makeSelected(net));
}

void Gui::addSelectedNets(const char* pattern,
                          bool match_case,
                          bool match_reg_ex,
                          bool add_to_highlight_set,
                          int highlight_group)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  QRegExp re(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
  SelectionSet nets;
  if (match_reg_ex == true) {
    QRegExp re(pattern,
               match_case == true ? Qt::CaseSensitive : Qt::CaseInsensitive,
               QRegExp::Wildcard);

    for (auto* net : block->getNets()) {
      if (re.exactMatch(net->getConstName())) {
        nets.insert(makeSelected(net));
      }
    }
  } else if (match_case == false) {
    for (auto* net : block->getNets()) {
      if (boost::iequals(pattern, net->getConstName()))
        nets.insert(makeSelected(net));
    }
  } else {
    for (auto* net : block->getNets()) {
      if (pattern == net->getConstName()) {
        nets.insert(makeSelected(net));
      }
    }
  }

  main_window->addSelected(nets);
  if (add_to_highlight_set == true)
    main_window->addHighlighted(nets, highlight_group);
}  // namespace gui

void Gui::addSelectedInst(const char* name)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  auto inst = block->findInst(name);
  if (!inst) {
    return;
  }

  main_window->addSelected(makeSelected(inst));
}

void Gui::addSelectedInsts(const char* pattern,
                           bool match_case,
                           bool match_reg_ex,
                           bool add_to_highlight_set,
                           int highlight_group)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  SelectionSet insts;
  if (match_reg_ex) {
    QRegExp re(pattern,
               match_case == true ? Qt::CaseSensitive : Qt::CaseInsensitive,
               QRegExp::Wildcard);
    for (auto* inst : block->getInsts()) {
      if (re.exactMatch(inst->getConstName())) {
        insts.insert(makeSelected(inst));
      }
    }
  } else if (match_case == false) {
    for (auto* inst : block->getInsts()) {
      if (boost::iequals(inst->getConstName(), pattern))
        insts.insert(makeSelected(inst));
    }
  } else {
    for (auto* inst : block->getInsts()) {
      if (pattern == inst->getConstName()) {
        insts.insert(makeSelected(inst));
        break;  // There can't be two insts with the same name
      }
    }
  }

  main_window->addSelected(insts);
  if (add_to_highlight_set == true)
    main_window->addHighlighted(insts, highlight_group);
}

bool Gui::anyObjectInSet(bool selection_set, odb::dbObjectType obj_type) const
{
  return main_window->anyObjectInSet(selection_set, obj_type);
}

void Gui::selectHighlightConnectedInsts(bool select_flag, int highlight_group)
{
  return main_window->selectHighlightConnectedInsts(select_flag,
                                                    highlight_group);
}
void Gui::selectHighlightConnectedNets(bool select_flag,
                                       bool output,
                                       bool input,
                                       int highlight_group)
{
  return main_window->selectHighlightConnectedNets(
      select_flag, output, input, highlight_group);
}

void Gui::addInstToHighlightSet(const char* name, int highlight_group)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  auto inst = block->findInst(name);
  if (!inst) {
    return;
  }
  SelectionSet sel_inst_set;
  sel_inst_set.insert(makeSelected(inst));
  main_window->addHighlighted(sel_inst_set, highlight_group);
}

void Gui::addNetToHighlightSet(const char* name, int highlight_group)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  auto net = block->findNet(name);
  if (!net) {
    return;
  }
  SelectionSet selection_set;
  selection_set.insert(makeSelected(net));
  main_window->addHighlighted(selection_set, highlight_group);
}

void Gui::addRuler(int x0, int y0, int x1, int y1)
{
  main_window->addRuler(x0, y0, x1, y1);
}

void Gui::clearSelections()
{
  main_window->setSelected(Selected());
}

void Gui::clearHighlights(int highlight_group)
{
  main_window->clearHighlighted(highlight_group);
}

void Gui::clearRulers()
{
  main_window->clearRulers();
}

const std::string Gui::addToolbarButton(const std::string& name,
                                        const std::string& text,
                                        const std::string& script,
                                        bool echo)
{
  return main_window->addToolbarButton(name,
                                       QString::fromStdString(text),
                                       QString::fromStdString(script),
                                       echo);
}

void Gui::removeToolbarButton(const std::string& name)
{
  main_window->removeToolbarButton(name);
}

const std::string Gui::requestUserInput(const std::string& title, const std::string& question)
{
  return main_window->requestUserInput(QString::fromStdString(title), QString::fromStdString(question));
}

void Gui::addCustomVisibilityControl(const std::string& name,
                                     bool initially_visible)
{
  main_window->getControls()->addCustomVisibilityControl(name,
                                                         initially_visible);
}

bool Gui::checkCustomVisibilityControl(const std::string& name)
{
  return main_window->getControls()->checkCustomVisibilityControl(name);
}

void Gui::setDisplayControlsVisible(const std::string& name, bool value)
{
  main_window->getControls()->setControlByPath(name, true, value ? Qt::Checked : Qt::Unchecked);
}

void Gui::setDisplayControlsSelectable(const std::string& name, bool value)
{
  main_window->getControls()->setControlByPath(name, false, value ? Qt::Checked : Qt::Unchecked);
}

void Gui::zoomTo(const odb::Rect& rect_dbu)
{
  main_window->zoomTo(rect_dbu);
}

void Gui::zoomIn()
{
  main_window->getLayoutViewer()->zoomIn();
}

void Gui::zoomIn(const odb::Point& focus_dbu)
{
  main_window->getLayoutViewer()->zoomIn(focus_dbu);
}

void Gui::zoomOut()
{
  main_window->getLayoutViewer()->zoomOut();
}

void Gui::zoomOut(const odb::Point& focus_dbu)
{
  main_window->getLayoutViewer()->zoomOut(focus_dbu);
}

void Gui::centerAt(const odb::Point& focus_dbu)
{
  main_window->getLayoutViewer()->centerAt(focus_dbu);
}

void Gui::setResolution(double pixels_per_dbu)
{
  main_window->getLayoutViewer()->setResolution(pixels_per_dbu);
}

void Gui::saveImage(const std::string& filename, const odb::Rect& region)
{
  main_window->getLayoutViewer()->saveImage(filename.c_str(), region);
}

void Gui::showWidget(const std::string& name, bool show)
{
  const QString find_name = QString::fromStdString(name);
  for (const auto& widget : main_window->findChildren<QDockWidget*>()) {
    if (widget->objectName() == find_name || widget->windowTitle() == find_name) {
      if (show) {
        widget->show();
        widget->raise();
      } else {
        widget->hide();
      }
    }
  }
}

Renderer::~Renderer()
{
  gui::Gui::get()->unregisterRenderer(this);
}

void Gui::load_design()
{
  main_window->postReadDb(main_window->getDb());
}

void Gui::fit()
{
  main_window->fit();
}

void Gui::registerDescriptor(const std::type_info& type,
                             const Descriptor* descriptor)
{
  main_window->registerDescriptor(type, descriptor);
}

//////////////////////////////////////////////////

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
int startGui(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  gui::MainWindow win;
  main_window = &win;
  auto* open_road = ord::OpenRoad::openRoad();
  win.setDb(open_road->getDb());
  open_road->addObserver(&win);
  win.show();

  // Exit the app if someone chooses exit from the menu in the window
  QObject::connect(&win, SIGNAL(exit()), &app, SLOT(quit()));

  // Save the window's status into the settings when quitting.
  QObject::connect(&app, SIGNAL(aboutToQuit()), &win, SLOT(saveSettings()));

  return app.exec();
}

void Selected::highlight(Painter& painter,
                         bool select_flag,
                         int highlight_group) const
{
  if (select_flag) {
    painter.setPen(Painter::highlight, true);
    painter.setBrush(Painter::transparent);
  } else if (highlight_group >= 0 && highlight_group < 7) {
    auto highlight_color = Painter::highlightColors[highlight_group];
    highlight_color.a = 100;
    painter.setPen(highlight_color, true);
    painter.setBrush(highlight_color);
  } else {
    painter.setPen(Painter::persistHighlight);
    painter.setBrush(Painter::transparent);
  }

  return descriptor_->highlight(object_, painter, additional_data_);
}

}  // namespace gui

namespace sta {
// Tcl files encoded into strings.
extern const char* gui_tcl_inits[];
}  // namespace sta

extern "C" {
struct Tcl_Interp;
}

namespace ord {

extern "C" {
extern int Gui_Init(Tcl_Interp* interp);
}

void initGui(OpenRoad* openroad)
{
  // Define swig TCL commands.
  Gui_Init(openroad->tclInterp());
  sta::evalTclInit(openroad->tclInterp(), sta::gui_tcl_inits);
  if (gui::main_window) {
    using namespace gui;
    main_window->setLogger(openroad->getLogger());
    Gui::get()->registerDescriptor<odb::dbInst*>(new DbInstDescriptor);
    Gui::get()->registerDescriptor<odb::dbMaster*>(new DbMasterDescriptor);
    Gui::get()->registerDescriptor<odb::dbNet*>(new DbNetDescriptor);
    Gui::get()->registerDescriptor<odb::dbITerm*>(new DbITermDescriptor);
    Gui::get()->registerDescriptor<odb::dbBTerm*>(new DbBTermDescriptor);
    Gui::get()->registerDescriptor<odb::dbBlockage*>(new DbBlockageDescriptor);
    Gui::get()->registerDescriptor<odb::dbObstruction*>(new DbObstructionDescriptor);
  }
}

}  // namespace ord
