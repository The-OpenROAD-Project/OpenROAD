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
#include "dbShape.h"
#include "defin.h"
#include "displayControls.h"
#include "geom.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "scriptWidget.h"
#include "lefin.h"
#include "mainWindow.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"
#include "utl/Logger.h"

#include "drcWidget.h"
#include "ruler.h"

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
  if (singleton_ == nullptr) {
    singleton_ = new Gui();
  }

  return singleton_;
}

Gui::Gui() : continue_after_close_(false),
             logger_(nullptr),
             db_(nullptr)
{
}

bool Gui::enabled()
{
  return main_window != nullptr;
}

void Gui::registerRenderer(Renderer* renderer)
{
  main_window->getControls()->registerRenderer(renderer);

  renderers_.insert(renderer);
  redraw();
}

void Gui::unregisterRenderer(Renderer* renderer)
{
  main_window->getControls()->unregisterRenderer(renderer);

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

void Gui::pause(int timeout)
{
  main_window->pause(timeout);
}

Selected Gui::makeSelected(std::any object, void* additional_data)
{
  if (!object.has_value()) {
    return Selected();
  }

  auto it = descriptors_.find(object.type());
  if (it != descriptors_.end()) {
    return it->second->makeSelected(object, additional_data);
  } else {
    logger_->warn(utl::GUI, 33, "No descriptor is registered for {}.", object.type().name());
    return Selected();  // FIXME: null descriptor
  }
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

void Gui::selectAt(const odb::Rect& area, bool append)
{
  main_window->getLayoutViewer()->selectArea(area, append);
}

int Gui::selectNext()
{
  return main_window->getInspector()->selectNext();
}

int Gui::selectPrevious()
{
  return main_window->getInspector()->selectPrevious();
}

std::string Gui::addRuler(int x0, int y0, int x1, int y1, const std::string& label, const std::string& name)
{
  return main_window->addRuler(x0, y0, x1, y1, label, name);
}

void Gui::deleteRuler(const std::string& name)
{
  main_window->deleteRuler(name);
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

void Gui::loadDRC(const std::string& filename)
{
  if (!filename.empty()) {
    main_window->getDRCViewer()->loadReport(QString::fromStdString(filename));
  }
}

void Gui::setDisplayControlsVisible(const std::string& name, bool value)
{
  main_window->getControls()->setControlByPath(name, true, value ? Qt::Checked : Qt::Unchecked);
}

bool Gui::checkDisplayControlsVisible(const std::string& name)
{
  return main_window->getControls()->checkControlByPath(name, true);
}

void Gui::setDisplayControlsSelectable(const std::string& name, bool value)
{
  main_window->getControls()->setControlByPath(name, false, value ? Qt::Checked : Qt::Unchecked);
}

bool Gui::checkDisplayControlsSelectable(const std::string& name)
{
  return main_window->getControls()->checkControlByPath(name, false);
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

void Gui::saveImage(const std::string& filename, const odb::Rect& region, double dbu_per_pixel, const std::map<std::string, bool>& display_settings)
{
  if (!enabled()) {
    if (db_ == nullptr) {
      logger_->error(utl::GUI, 15, "No design loaded.");
    }
    auto* tech = db_->getTech();
    if (tech == nullptr) {
      logger_->error(utl::GUI, 16, "No design loaded.");
    }
    const double dbu_per_micron = tech->getLefUnits();

    std::string save_cmds;
    // build display control commands
    save_cmds = "set ::gui::display_settings [gui::DisplayControlMap]\n";
    for (const auto& [control, value] : display_settings) {
      // first save current setting
      save_cmds += fmt::format("$::gui::display_settings set \"{}\" {}", control, value) + "\n";
    }
    // save command
    save_cmds += "gui::save_image ";
    save_cmds += "\"" + filename + "\" ";
    save_cmds += std::to_string(region.xMin() / dbu_per_micron) + " ";
    save_cmds += std::to_string(region.yMin() / dbu_per_micron) + " ";
    save_cmds += std::to_string(region.xMax() / dbu_per_micron) + " ";
    save_cmds += std::to_string(region.yMax() / dbu_per_micron) + " ";
    save_cmds += std::to_string(dbu_per_pixel) + " ";
    save_cmds += "$::gui::display_settings\n";
    // delete display settings map
    save_cmds += "rename $::gui::display_settings \"\"\n";
    save_cmds += "unset ::gui::display_settings\n";
    // end with hide to return
    save_cmds += "gui::hide";
    showGui(save_cmds, false);
  } else {
    // save current display settings and apply new
    main_window->getControls()->save();
    for (const auto& [control, value] : display_settings) {
      setDisplayControlsVisible(control, value);
    }
    main_window->getLayoutViewer()->saveImage(filename.c_str(), region, dbu_per_pixel);
    // restore settings
    main_window->getControls()->restore();
  }
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

void Renderer::redraw()
{
  Gui::get()->redraw();
}

bool Renderer::checkDisplayControl(const std::string& name)
{
  const std::string& group_name = getDisplayControlGroupName();

  if (group_name.empty()) {
    return Gui::get()->checkDisplayControlsVisible(name);
  } else {
    return Gui::get()->checkDisplayControlsVisible(group_name + "/" + name);
  }
}

void Renderer::addDisplayControl(const std::string& name, bool initial_state)
{
  controls_[name] = initial_state;
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
  descriptors_[type] = std::unique_ptr<const Descriptor>(descriptor);
}

void Gui::unregisterDescriptor(const std::type_info& type)
{
  descriptors_.erase(type);
}

const Selected& Gui::getInspectorSelection()
{
  return main_window->getInspector()->getSelection();
}

void Gui::setLogger(utl::Logger* logger)
{
  if (logger == nullptr) {
    return;
  }

  logger_ = logger;

  if (enabled()) {
    // gui already requested, so go ahead and set the logger
    main_window->setLogger(logger);
  }
}

void Gui::setDatabase(odb::dbDatabase* db)
{
  db_ = db;
}

void Gui::hideGui()
{
  // ensure continue after close is true, since we want to return to tcl
  continue_after_close_ = true;
  main_window->exit();
}

void Gui::showGui(const std::string& cmds, bool interactive)
{
  if (enabled()) {
    logger_->warn(utl::GUI, 8, "GUI already active.");
    return;
  }

  // OR already running, so GUI should not set anything up
  // passing in 0, nullptr, nullptr to indicate such
  // pass cmds and interactive along
  startGui(0, nullptr, nullptr, cmds, interactive);
}

//////////////////////////////////////////////////

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
int startGui(int argc, char* argv[], Tcl_Interp* interp, const std::string& script, bool interactive)
{
  auto gui = gui::Gui::get();
  // ensure continue after close is false
  gui->clearContinueAfterClose();

  QApplication app(argc, argv);

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  auto* open_road = ord::OpenRoad::openRoad();

  // create new MainWindow
  main_window = new gui::MainWindow;

  open_road->addObserver(main_window);
  if (!interactive) {
    main_window->setAttribute(Qt::WA_DontShowOnScreen);
  }
  main_window->show();

  gui->setLogger(open_road->getLogger());

  main_window->setDatabase(open_road->getDb());

  bool init_openroad = interp != nullptr;
  if (!init_openroad) {
    interp = open_road->tclInterp();
  }

  // pass in tcl interp to script widget and ensure OpenRoad gets initialized
  main_window->getScriptWidget()->setupTcl(interp, init_openroad);

  // openroad is guaranteed to be initialized here
  main_window->init(open_road->getSta());

  // Exit the app if someone chooses exit from the menu in the window
  QObject::connect(main_window, SIGNAL(exit()), &app, SLOT(quit()));

  // Hide the Gui if someone chooses hide from the menu in the window
  QObject::connect(main_window, &gui::MainWindow::hide, [gui]() {
    gui->hideGui();
  });

  // Save the window's status into the settings when quitting.
  QObject::connect(&app, SIGNAL(aboutToQuit()), main_window, SLOT(saveSettings()));

  // execute commands to restore state of gui
  std::string restore_commands;
  for (const auto& cmd : gui->getRestoreStateCommands()) {
    restore_commands += cmd + "\n";
  }
  if (!restore_commands.empty()) {
    // Temporarily connect to script widget to get ending tcl state
    int tcl_return_code = TCL_OK;
    auto tcl_return_code_connect = QObject::connect(main_window->getScriptWidget(), &ScriptWidget::commandExecuted, [&tcl_return_code](int code) {
      tcl_return_code = code;
    });

    main_window->getScriptWidget()->executeSilentCommand(QString::fromStdString(restore_commands));

    // disconnect tcl return lister
    QObject::disconnect(tcl_return_code_connect);

    if (tcl_return_code != TCL_OK) {
      auto& cmds = gui->getRestoreStateCommands();
      if (cmds[cmds.size() - 1] == "exit") { // exit, will be the last command if it is present
        // if there was a failure and exit was requested, exit with failure
        // this will mirror the behavior of tclAppInit
        exit(EXIT_FAILURE);
      }
    }
  }
  gui->clearRestoreStateCommands();

  // Execute script
  if (!script.empty()) {
    main_window->getScriptWidget()->executeCommand(QString::fromStdString(script));
  }

  bool do_exec = interactive;
  // check if hide was called by script
  if (gui->isContinueAfterClose()) {
    do_exec = false;
  }

  int ret = 0;
  if (do_exec) {
    ret = app.exec();
  }

  // cleanup
  open_road->removeObserver(main_window);

  // save restore state commands
  for (const auto& cmd : main_window->getRestoreTclCommands()) {
    gui->addRestoreStateCommand(cmd);
  }

  // delete main window and set to nullptr
  delete main_window;
  main_window = nullptr;

  if (!gui->isContinueAfterClose()) {
    // if exiting, go ahead and exit with gui return code.
    exit(ret);
  }

  return ret;
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

std::string Descriptor::Property::toString(const std::any& value)
{
  if (auto v = std::any_cast<Selected>(&value)) {
    if (*v) {
      return v->getName();
    }
  } else if (auto v = std::any_cast<const char*>(&value)) {
    return *v;
  } else if (auto v = std::any_cast<const std::string>(&value)) {
    return *v;
  } else if (auto v = std::any_cast<int>(&value)) {
    return std::to_string(*v);
  } else if (auto v = std::any_cast<unsigned int>(&value)) {
    return std::to_string(*v);
  } else if (auto v = std::any_cast<double>(&value)) {
    return QString::number(*v).toStdString();
  } else if (auto v = std::any_cast<float>(&value)) {
    return QString::number(*v).toStdString();
  } else if (auto v = std::any_cast<bool>(&value)) {
    return *v ? "True" : "False";
  }

  return "<unknown>";
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

  // ensure gui is made
  auto* gui = gui::Gui::get();
  gui->setDatabase(openroad->getDb());
  gui->setLogger(openroad->getLogger());
}

}  // namespace ord
