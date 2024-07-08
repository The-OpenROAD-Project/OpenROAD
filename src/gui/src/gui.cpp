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
#include <boost/algorithm/string/predicate.hpp>
#include <stdexcept>
#include <string>

#include "clockWidget.h"
#include "displayControls.h"
#include "drcWidget.h"
#include "heatMapPlacementDensity.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "mainWindow.h"
#include "odb/db.h"
#include "odb/dbShape.h"
#include "odb/defin.h"
#include "odb/geom.h"
#include "odb/lefin.h"
#include "ord/OpenRoad.hh"
#include "ruler.h"
#include "scriptWidget.h"
#include "sta/StaMain.hh"
#include "utl/Logger.h"
#include "utl/exception.h"

extern int cmd_argc;
extern char** cmd_argv;

namespace gui {

static QApplication* application = nullptr;
static void message_handler(QtMsgType type,
                            const QMessageLogContext& context,
                            const QString& msg)
{
  auto* logger = ord::OpenRoad::openRoad()->getLogger();

  bool suppress = false;
#if NDEBUG
  // suppress messages when built as a release, but preserve them in debug
  // builds
  if (application != nullptr) {
    if (QApplication::platformName() == "offscreen"
        && msg.contains("This plugin does not support")) {
      suppress = true;
    }
  }
#endif

  if (suppress) {
    return;
  }

  std::string print_msg;
  if (context.file != nullptr && context.function != nullptr) {
    print_msg = fmt::format("{}:{}:{}: {}",
                            context.file,
                            context.function,
                            context.line,
                            msg.toStdString());
  } else {
    print_msg = msg.toStdString();
  }
  switch (type) {
    case QtDebugMsg:
      logger->debug(utl::GUI, "qt", print_msg);
      break;
    case QtInfoMsg:
      logger->info(utl::GUI, 75, print_msg);
      break;
    case QtWarningMsg:
      logger->warn(utl::GUI, 76, print_msg);
      break;
    case QtCriticalMsg:
    case QtFatalMsg:
      logger->error(utl::GUI, 77, print_msg);
      break;
  }
}

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

// Used by toString to convert dbu to microns (and back), will be set in
// main_window
DBUToString Descriptor::Property::convert_dbu;
StringToDBU Descriptor::Property::convert_string;

// Heatmap / Spectrum colors
// https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
// https://gist.github.com/mikhailov-work/6a308c20e494d9e0ccc29036b28faa7a
const unsigned char SpectrumGenerator::spectrum_[256][3]
    = {{48, 18, 59},   {50, 21, 67},   {51, 24, 74},    {52, 27, 81},
       {53, 30, 88},   {54, 33, 95},   {55, 36, 102},   {56, 39, 109},
       {57, 42, 115},  {58, 45, 121},  {59, 47, 128},   {60, 50, 134},
       {61, 53, 139},  {62, 56, 145},  {63, 59, 151},   {63, 62, 156},
       {64, 64, 162},  {65, 67, 167},  {65, 70, 172},   {66, 73, 177},
       {66, 75, 181},  {67, 78, 186},  {68, 81, 191},   {68, 84, 195},
       {68, 86, 199},  {69, 89, 203},  {69, 92, 207},   {69, 94, 211},
       {70, 97, 214},  {70, 100, 218}, {70, 102, 221},  {70, 105, 224},
       {70, 107, 227}, {71, 110, 230}, {71, 113, 233},  {71, 115, 235},
       {71, 118, 238}, {71, 120, 240}, {71, 123, 242},  {70, 125, 244},
       {70, 128, 246}, {70, 130, 248}, {70, 133, 250},  {70, 135, 251},
       {69, 138, 252}, {69, 140, 253}, {68, 143, 254},  {67, 145, 254},
       {66, 148, 255}, {65, 150, 255}, {64, 153, 255},  {62, 155, 254},
       {61, 158, 254}, {59, 160, 253}, {58, 163, 252},  {56, 165, 251},
       {55, 168, 250}, {53, 171, 248}, {51, 173, 247},  {49, 175, 245},
       {47, 178, 244}, {46, 180, 242}, {44, 183, 240},  {42, 185, 238},
       {40, 188, 235}, {39, 190, 233}, {37, 192, 231},  {35, 195, 228},
       {34, 197, 226}, {32, 199, 223}, {31, 201, 221},  {30, 203, 218},
       {28, 205, 216}, {27, 208, 213}, {26, 210, 210},  {26, 212, 208},
       {25, 213, 205}, {24, 215, 202}, {24, 217, 200},  {24, 219, 197},
       {24, 221, 194}, {24, 222, 192}, {24, 224, 189},  {25, 226, 187},
       {25, 227, 185}, {26, 228, 182}, {28, 230, 180},  {29, 231, 178},
       {31, 233, 175}, {32, 234, 172}, {34, 235, 170},  {37, 236, 167},
       {39, 238, 164}, {42, 239, 161}, {44, 240, 158},  {47, 241, 155},
       {50, 242, 152}, {53, 243, 148}, {56, 244, 145},  {60, 245, 142},
       {63, 246, 138}, {67, 247, 135}, {70, 248, 132},  {74, 248, 128},
       {78, 249, 125}, {82, 250, 122}, {85, 250, 118},  {89, 251, 115},
       {93, 252, 111}, {97, 252, 108}, {101, 253, 105}, {105, 253, 102},
       {109, 254, 98}, {113, 254, 95}, {117, 254, 92},  {121, 254, 89},
       {125, 255, 86}, {128, 255, 83}, {132, 255, 81},  {136, 255, 78},
       {139, 255, 75}, {143, 255, 73}, {146, 255, 71},  {150, 254, 68},
       {153, 254, 66}, {156, 254, 64}, {159, 253, 63},  {161, 253, 61},
       {164, 252, 60}, {167, 252, 58}, {169, 251, 57},  {172, 251, 56},
       {175, 250, 55}, {177, 249, 54}, {180, 248, 54},  {183, 247, 53},
       {185, 246, 53}, {188, 245, 52}, {190, 244, 52},  {193, 243, 52},
       {195, 241, 52}, {198, 240, 52}, {200, 239, 52},  {203, 237, 52},
       {205, 236, 52}, {208, 234, 52}, {210, 233, 53},  {212, 231, 53},
       {215, 229, 53}, {217, 228, 54}, {219, 226, 54},  {221, 224, 55},
       {223, 223, 55}, {225, 221, 55}, {227, 219, 56},  {229, 217, 56},
       {231, 215, 57}, {233, 213, 57}, {235, 211, 57},  {236, 209, 58},
       {238, 207, 58}, {239, 205, 58}, {241, 203, 58},  {242, 201, 58},
       {244, 199, 58}, {245, 197, 58}, {246, 195, 58},  {247, 193, 58},
       {248, 190, 57}, {249, 188, 57}, {250, 186, 57},  {251, 184, 56},
       {251, 182, 55}, {252, 179, 54}, {252, 177, 54},  {253, 174, 53},
       {253, 172, 52}, {254, 169, 51}, {254, 167, 50},  {254, 164, 49},
       {254, 161, 48}, {254, 158, 47}, {254, 155, 45},  {254, 153, 44},
       {254, 150, 43}, {254, 147, 42}, {254, 144, 41},  {253, 141, 39},
       {253, 138, 38}, {252, 135, 37}, {252, 132, 35},  {251, 129, 34},
       {251, 126, 33}, {250, 123, 31}, {249, 120, 30},  {249, 117, 29},
       {248, 114, 28}, {247, 111, 26}, {246, 108, 25},  {245, 105, 24},
       {244, 102, 23}, {243, 99, 21},  {242, 96, 20},   {241, 93, 19},
       {240, 91, 18},  {239, 88, 17},  {237, 85, 16},   {236, 83, 15},
       {235, 80, 14},  {234, 78, 13},  {232, 75, 12},   {231, 73, 12},
       {229, 71, 11},  {228, 69, 10},  {226, 67, 10},   {225, 65, 9},
       {223, 63, 8},   {221, 61, 8},   {220, 59, 7},    {218, 57, 7},
       {216, 55, 6},   {214, 53, 6},   {212, 51, 5},    {210, 49, 5},
       {208, 47, 5},   {206, 45, 4},   {204, 43, 4},    {202, 42, 4},
       {200, 40, 3},   {197, 38, 3},   {195, 37, 3},    {193, 35, 2},
       {190, 33, 2},   {188, 32, 2},   {185, 30, 2},    {183, 29, 2},
       {180, 27, 1},   {178, 26, 1},   {175, 24, 1},    {172, 23, 1},
       {169, 22, 1},   {167, 20, 1},   {164, 19, 1},    {161, 18, 1},
       {158, 16, 1},   {155, 15, 1},   {152, 14, 1},    {149, 13, 1},
       {146, 11, 1},   {142, 10, 1},   {139, 9, 2},     {136, 8, 2},
       {133, 7, 2},    {129, 6, 2},    {126, 5, 2},     {122, 4, 3}};

static void resetConversions()
{
  Descriptor::Property::convert_dbu
      = [](int value, bool) { return std::to_string(value); };
  Descriptor::Property::convert_string
      = [](const std::string& value, bool*) { return 0; };
}

Gui* Gui::singleton_ = nullptr;

Gui* Gui::get()
{
  if (singleton_ == nullptr) {
    singleton_ = new Gui();
  }

  return singleton_;
}

Gui::Gui()
    : continue_after_close_(false),
      logger_(nullptr),
      db_(nullptr),
      placement_density_heat_map_(nullptr)
{
  resetConversions();
}

bool Gui::enabled()
{
  return main_window != nullptr;
}

void Gui::registerRenderer(Renderer* renderer)
{
  if (Gui::enabled()) {
    main_window->getControls()->registerRenderer(renderer);
  }

  renderers_.insert(renderer);
  redraw();
}

void Gui::unregisterRenderer(Renderer* renderer)
{
  if (renderers_.count(renderer) == 0) {
    return;
  }

  if (Gui::enabled()) {
    main_window->getControls()->unregisterRenderer(renderer);
  }

  renderers_.erase(renderer);
  redraw();
}

void Gui::redraw()
{
  if (!Gui::enabled()) {
    return;
  }
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

Selected Gui::makeSelected(const std::any& object)
{
  if (!object.has_value()) {
    return Selected();
  }

  auto it = descriptors_.find(object.type());
  if (it != descriptors_.end()) {
    return it->second->makeSelected(object);
  }
  logger_->warn(utl::GUI,
                33,
                "No descriptor is registered for {}.",
                object.type().name());
  return Selected();  // FIXME: null descriptor
}

void Gui::setSelected(const Selected& selection)
{
  main_window->setSelected(selection);
}

void Gui::removeSelectedByType(const std::string& type)
{
  main_window->removeSelectedByType(type);
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

void Gui::selectHighlightConnectedBufferTrees(bool select_flag,
                                              int highlight_group)
{
  return main_window->selectHighlightConnectedBufferTrees(select_flag,
                                                          highlight_group);
}

void Gui::addInstToHighlightSet(const char* name, int highlight_group)
{
  auto block = getBlock(main_window->getDb());
  if (!block) {
    return;
  }

  auto inst = block->findInst(name);
  if (!inst) {
    logger_->error(utl::GUI, 100, "No instance named {} found.", name);
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
    logger_->error(utl::GUI, 101, "No net named {} found.", name);
    return;
  }
  SelectionSet selection_set;
  selection_set.insert(makeSelected(net));
  main_window->addHighlighted(selection_set, highlight_group);
}

int Gui::selectAt(const odb::Rect& area, bool append)
{
  return main_window->getLayoutViewer()->selectArea(area, append);
}

int Gui::selectNext()
{
  return main_window->getInspector()->selectNext();
}

int Gui::selectPrevious()
{
  return main_window->getInspector()->selectPrevious();
}

void Gui::animateSelection(int repeat)
{
  main_window->getLayoutViewer()->selectionAnimation(repeat);
}

std::string Gui::addRuler(int x0,
                          int y0,
                          int x1,
                          int y1,
                          const std::string& label,
                          const std::string& name,
                          bool euclidian)
{
  return main_window->addRuler(x0, y0, x1, y1, label, name, euclidian);
}

void Gui::deleteRuler(const std::string& name)
{
  main_window->deleteRuler(name);
}

int Gui::select(const std::string& type,
                const std::string& name_filter,
                const std::string& attribute,
                const std::any& value,
                bool filter_case_sensitive,
                int highlight_group)
{
  for (auto& [object_type, descriptor] : descriptors_) {
    if (descriptor->getTypeName() == type) {
      SelectionSet selected_set;
      if (descriptor->getAllObjects(selected_set)) {
        if (!name_filter.empty()) {
          // convert to vector
          std::vector<Selected> selected_vector(selected_set.begin(),
                                                selected_set.end());
          // remove elements
          QRegExp reg_filter(
              QString::fromStdString(name_filter),
              filter_case_sensitive ? Qt::CaseSensitive : Qt::CaseInsensitive,
              QRegExp::WildcardUnix);
          auto remove_if = std::remove_if(
              selected_vector.begin(),
              selected_vector.end(),
              [&name_filter, &reg_filter](auto sel) -> bool {
                const std::string sel_name = sel.getName();
                if (sel_name == name_filter) {
                  // direct match, so don't remove
                  return false;
                }
                return !reg_filter.exactMatch(QString::fromStdString(sel_name));
              });
          selected_vector.erase(remove_if, selected_vector.end());
          // rebuild selectionset
          selected_set.clear();
          selected_set.insert(selected_vector.begin(), selected_vector.end());
        }

        if (!attribute.empty()) {
          bool is_valid_attribute = false;
          for (SelectionSet::iterator selected_iter = selected_set.begin();
               selected_iter != selected_set.end();) {
            Descriptor::Properties properties
                = descriptor->getProperties(selected_iter->getObject());
            if (filterSelectionProperties(
                    properties, attribute, value, is_valid_attribute)) {
              ++selected_iter;
            } else {
              selected_iter = selected_set.erase(selected_iter);
            }
          }

          if (!is_valid_attribute) {
            logger_->error(
                utl::GUI, 59, "Entered attribute {} is not valid.", attribute);
          } else if (selected_set.empty()) {
            logger_->error(utl::GUI,
                           75,
                           "Couldn't find any object for the specified value.");
          }
        }

        main_window->addSelected(selected_set);
        if (highlight_group != -1) {
          main_window->addHighlighted(selected_set, highlight_group);
        }
      }

      // already found the descriptor, so return to exit loop
      return selected_set.size();
    }
  }

  logger_->error(utl::GUI, 35, "Unable to find descriptor for: {}", type);
}

bool Gui::filterSelectionProperties(const Descriptor::Properties& properties,
                                    const std::string& attribute,
                                    const std::any& value,
                                    bool& is_valid_attribute)
{
  for (const Descriptor::Property& property : properties) {
    if (attribute == property.name) {
      is_valid_attribute = true;
      if (auto props_selected_set
          = std::any_cast<SelectionSet>(&property.value)) {
        if (Descriptor::Property::toString(value) == "CONNECTED"
            && (*props_selected_set).size() != 0) {
          return true;
        }
        for (const auto& selected : *props_selected_set) {
          if (Descriptor::Property::toString(value) == selected.getName()) {
            return true;
          }
        }
      } else if (auto props_list
                 = std::any_cast<Descriptor::PropertyList>(&property.value)) {
        for (const auto& prop : *props_list) {
          if (Descriptor::Property::toString(prop.first)
                  == Descriptor::Property::toString(value)
              || Descriptor::Property::toString(prop.second)
                     == Descriptor::Property::toString(value)) {
            return true;
          }
        }
      } else if (Descriptor::Property::toString(value)
                 == Descriptor::Property::toString(property.value)) {
        return true;
      }
    }
  }

  return false;
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

std::string Gui::addToolbarButton(const std::string& name,
                                  const std::string& text,
                                  const std::string& script,
                                  bool echo)
{
  return main_window->addToolbarButton(
      name, QString::fromStdString(text), QString::fromStdString(script), echo);
}

void Gui::removeToolbarButton(const std::string& name)
{
  main_window->removeToolbarButton(name);
}

std::string Gui::addMenuItem(const std::string& name,
                             const std::string& path,
                             const std::string& text,
                             const std::string& script,
                             const std::string& shortcut,
                             bool echo)
{
  return main_window->addMenuItem(name,
                                  QString::fromStdString(path),
                                  QString::fromStdString(text),
                                  QString::fromStdString(script),
                                  QString::fromStdString(shortcut),
                                  echo);
}

void Gui::removeMenuItem(const std::string& name)
{
  main_window->removeMenuItem(name);
}

std::string Gui::requestUserInput(const std::string& title,
                                  const std::string& question)
{
  return main_window->requestUserInput(QString::fromStdString(title),
                                       QString::fromStdString(question));
}

void Gui::loadDRC(const std::string& filename)
{
  if (!filename.empty()) {
    main_window->getDRCViewer()->loadReport(QString::fromStdString(filename));
  }
}

void Gui::setDisplayControlsVisible(const std::string& name, bool value)
{
  main_window->getControls()->setControlByPath(
      name, true, value ? Qt::Checked : Qt::Unchecked);
}

bool Gui::checkDisplayControlsVisible(const std::string& name)
{
  return main_window->getControls()->checkControlByPath(name, true);
}

void Gui::setDisplayControlsSelectable(const std::string& name, bool value)
{
  main_window->getControls()->setControlByPath(
      name, false, value ? Qt::Checked : Qt::Unchecked);
}

bool Gui::checkDisplayControlsSelectable(const std::string& name)
{
  return main_window->getControls()->checkControlByPath(name, false);
}

void Gui::saveDisplayControls()
{
  main_window->getControls()->save();
}

void Gui::restoreDisplayControls()
{
  main_window->getControls()->restore();
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

void Gui::saveImage(const std::string& filename,
                    const odb::Rect& region,
                    int width_px,
                    double dbu_per_pixel,
                    const std::map<std::string, bool>& display_settings)
{
  if (db_ == nullptr) {
    logger_->error(utl::GUI, 15, "No design loaded.");
  }
  odb::Rect save_region = region;
  const bool use_die_area = region.dx() == 0 || region.dy() == 0;
  const bool is_offscreen
      = main_window == nullptr
        || main_window->testAttribute(
            Qt::WA_DontShowOnScreen); /* if not interactive this will be set */
  if (is_offscreen
      && use_die_area) {  // if gui is active and interactive the visible are of
                          // the layout viewer will be used.
    auto* chip = db_->getChip();
    if (chip == nullptr) {
      logger_->error(utl::GUI, 64, "No design loaded.");
    }

    auto* block = chip->getBlock();
    if (block == nullptr) {
      logger_->error(utl::GUI, 65, "No design loaded.");
    }

    save_region
        = block->getBBox()
              ->getBox();  // get die area since screen area is not reliable
    const double bloat_by = 0.05;  // 5%
    const int bloat = std::min(save_region.dx(), save_region.dy()) * bloat_by;

    save_region.bloat(bloat, save_region);
  }

  if (!enabled()) {
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
      save_cmds += fmt::format(
                       "$::gui::display_settings set \"{}\" {}", control, value)
                   + "\n";
    }
    // save command
    save_cmds += "gui::save_image ";
    save_cmds += "\"" + filename + "\" ";
    save_cmds += std::to_string(save_region.xMin() / dbu_per_micron) + " ";
    save_cmds += std::to_string(save_region.yMin() / dbu_per_micron) + " ";
    save_cmds += std::to_string(save_region.xMax() / dbu_per_micron) + " ";
    save_cmds += std::to_string(save_region.yMax() / dbu_per_micron) + " ";
    save_cmds += std::to_string(width_px) + " ";
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

    main_window->getLayoutViewer()->saveImage(
        filename.c_str(), save_region, width_px, dbu_per_pixel);
    // restore settings
    main_window->getControls()->restore();
  }
}

void Gui::saveClockTreeImage(const std::string& clock_name,
                             const std::string& filename,
                             const std::string& corner,
                             int width_px,
                             int height_px)
{
  if (!enabled()) {
    return;
  }
  std::optional<int> width;
  std::optional<int> height;
  if (width_px > 0) {
    width = width_px;
  }
  if (height_px > 0) {
    height = height_px;
  }
  main_window->getClockViewer()->saveImage(
      clock_name, filename, corner, width, height);
}

static QWidget* findWidget(const std::string& name)
{
  if (name == "main_window" || name == "OpenROAD") {
    return main_window;
  }

  const QString find_name = QString::fromStdString(name);
  for (const auto& widget : main_window->findChildren<QDockWidget*>()) {
    if (widget->objectName() == find_name
        || widget->windowTitle() == find_name) {
      return widget;
    }
  }
  return nullptr;
}

void Gui::showWidget(const std::string& name, bool show)
{
  auto* widget = findWidget(name);
  if (widget == nullptr) {
    return;
  }

  if (show) {
    widget->show();
    widget->raise();
  } else {
    widget->hide();
  }
}

void Gui::triggerAction(const std::string& name)
{
  const size_t dot_idx = name.find_last_of('.');
  auto* widget = findWidget(name.substr(0, dot_idx));
  if (widget == nullptr) {
    return;
  }

  const QString find_name = QString::fromStdString(name.substr(dot_idx + 1));

  // Find QAction
  for (QAction* action : widget->findChildren<QAction*>()) {
    logger_->report("{} {}",
                    action->objectName().toStdString(),
                    action->text().toStdString());
    if (action->objectName() == find_name || action->text() == find_name) {
      action->trigger();
      return;
    }
  }

  // Find QPushButton
  for (QPushButton* button : widget->findChildren<QPushButton*>()) {
    if (button->objectName() == find_name || button->text() == find_name) {
      button->click();
      return;
    }
  }
}

void Gui::registerHeatMap(HeatMapDataSource* heatmap)
{
  heat_maps_.insert(heatmap);
  registerRenderer(heatmap->getRenderer());
  if (Gui::enabled()) {
    main_window->registerHeatMap(heatmap);
  }
}

void Gui::unregisterHeatMap(HeatMapDataSource* heatmap)
{
  if (heat_maps_.count(heatmap) == 0) {
    return;
  }

  unregisterRenderer(heatmap->getRenderer());
  if (Gui::enabled()) {
    main_window->unregisterHeatMap(heatmap);
  }
  heat_maps_.erase(heatmap);
}

HeatMapDataSource* Gui::getHeatMap(const std::string& name)
{
  HeatMapDataSource* source = nullptr;

  for (auto* heat_map : heat_maps_) {
    if (heat_map->getShortName() == name) {
      source = heat_map;
      break;
    }
  }

  if (source == nullptr) {
    QStringList options;
    for (auto* heat_map : heat_maps_) {
      options.append(QString::fromStdString(heat_map->getShortName()));
    }
    logger_->error(utl::GUI,
                   28,
                   "{} is not a known map. Valid options are: {}",
                   name,
                   options.join(", ").toStdString());
  }

  return source;
}

void Gui::setHeatMapSetting(const std::string& name,
                            const std::string& option,
                            const Renderer::Setting& value)
{
  HeatMapDataSource* source = getHeatMap(name);

  const std::string rebuild_map_option = "rebuild";
  if (option == rebuild_map_option) {
    source->destroyMap();
    source->ensureMap();
  } else {
    auto settings = source->getSettings();

    if (settings.count(option) == 0) {
      QStringList options;
      options.append(QString::fromStdString(rebuild_map_option));
      for (const auto& [key, kv] : settings) {
        options.append(QString::fromStdString(key));
      }
      logger_->error(utl::GUI,
                     29,
                     "{} is not a valid option. Valid options are: {}",
                     option,
                     options.join(", ").toStdString());
    }

    auto& current_value = settings[option];
    if (std::holds_alternative<bool>(current_value)) {
      // is bool
      if (auto* s = std::get_if<bool>(&value)) {
        settings[option] = *s;
      }
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = *s != 0;
      }
      if (auto* s = std::get_if<double>(&value)) {
        settings[option] = *s != 0.0;
      } else {
        logger_->error(utl::GUI, 60, "{} must be a boolean", option);
      }
    } else if (std::holds_alternative<int>(current_value)) {
      // is int
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = *s;
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = static_cast<int>(*s);
      } else {
        logger_->error(utl::GUI, 61, "{} must be an integer or double", option);
      }
    } else if (std::holds_alternative<double>(current_value)) {
      // is double
      if (auto* s = std::get_if<int>(&value)) {
        settings[option] = static_cast<double>(*s);
      } else if (auto* s = std::get_if<double>(&value)) {
        settings[option] = *s;
      } else {
        logger_->error(utl::GUI, 62, "{} must be an integer or double", option);
      }
    } else {
      // is string
      if (auto* s = std::get_if<std::string>(&value)) {
        settings[option] = *s;
      } else {
        logger_->error(utl::GUI, 63, "{} must be a string", option);
      }
    }
    source->setSettings(settings);
  }

  source->getRenderer()->redraw();
}

Renderer::Setting Gui::getHeatMapSetting(const std::string& name,
                                         const std::string& option)
{
  HeatMapDataSource* source = getHeatMap(name);

  const std::string map_has_option = "has_data";
  if (option == map_has_option) {
    return source->hasData();
  }

  auto settings = source->getSettings();

  if (settings.count(option) == 0) {
    QStringList options;
    for (const auto& [key, kv] : settings) {
      options.append(QString::fromStdString(key));
    }
    logger_->error(utl::GUI,
                   95,
                   "{} is not a valid option. Valid options are: {}",
                   option,
                   options.join(", ").toStdString());
  }

  return settings[option];
}

void Gui::dumpHeatMap(const std::string& name, const std::string& file)
{
  HeatMapDataSource* source = getHeatMap(name);
  source->dumpToFile(file);
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
  }
  return Gui::get()->checkDisplayControlsVisible(group_name + "/" + name);
}

void Renderer::setDisplayControl(const std::string& name, bool value)
{
  const std::string& group_name = getDisplayControlGroupName();

  if (group_name.empty()) {
    return Gui::get()->setDisplayControlsVisible(name, value);
  }
  return Gui::get()->setDisplayControlsVisible(group_name + "/" + name, value);
}

void Renderer::addDisplayControl(
    const std::string& name,
    bool initial_visible,
    const DisplayControlCallback& setup,
    const std::vector<std::string>& mutual_exclusivity)
{
  auto& control = controls_[name];

  control.visibility = initial_visible;
  control.interactive_setup = setup;
  control.mutual_exclusivity.insert(mutual_exclusivity.begin(),
                                    mutual_exclusivity.end());
}

Renderer::Settings Renderer::getSettings()
{
  Settings settings;
  for (const auto& [key, init_value] : controls_) {
    settings[key] = checkDisplayControl(key);
  }
  return settings;
}

void Renderer::setSettings(const Renderer::Settings& settings)
{
  for (auto& [key, control] : controls_) {
    setSetting<bool>(settings, key, control.visibility);
    setDisplayControl(key, control.visibility);
  }
}

SpectrumGenerator::SpectrumGenerator(double max_value) : scale_(1.0 / max_value)
{
}

int SpectrumGenerator::getColorCount() const
{
  return 256;
}

Painter::Color SpectrumGenerator::getColor(double value, int alpha) const
{
  const int max_index = getColorCount() - 1;
  int index = std::round(scale_ * value * max_index);
  if (index < 0) {
    index = 0;
  } else if (index > max_index) {
    index = max_index;
  }

  return Painter::Color(
      spectrum_[index][0], spectrum_[index][1], spectrum_[index][2], alpha);
}

void SpectrumGenerator::drawLegend(
    Painter& painter,
    const std::vector<std::pair<int, std::string>>& legend_key) const
{
  const odb::Rect& bounds = painter.getBounds();
  const double pixel_per_dbu = painter.getPixelsPerDBU();
  const int legend_offset = 20 / pixel_per_dbu;  // 20 pixels
  const double box_height = 1 / pixel_per_dbu;   // 1 pixels
  const int legend_width = 20 / pixel_per_dbu;   // 20 pixels
  const int text_offset = 2 / pixel_per_dbu;
  const int legend_top = bounds.yMax() - legend_offset;
  const int legend_right = bounds.xMax() - legend_offset;
  const int legend_left = legend_right - legend_width;
  const Painter::Anchor key_anchor = Painter::Anchor::RIGHT_CENTER;

  odb::Rect legend_bounds(
      legend_left, legend_top, legend_right + text_offset, legend_top);

  const int color_count = getColorCount();
  const int color_incr = 2;

  std::vector<std::pair<odb::Point, std::string>> legend_key_points;
  for (const auto& [legend_value, legend_text] : legend_key) {
    const int text_right = legend_left - text_offset;
    const int box_top
        = legend_top - ((color_count - legend_value) * box_height) / color_incr;

    legend_key_points.push_back({{text_right, box_top}, legend_text});
    const odb::Rect text_bounds = painter.stringBoundaries(
        text_right, box_top, key_anchor, legend_text);

    legend_bounds.merge(text_bounds);
  }

  // draw background
  painter.setPen(Painter::dark_gray, true);
  painter.setBrush(Painter::dark_gray);
  painter.drawRect(legend_bounds, 10, 10);

  // draw color map
  double box_top = legend_top;
  for (int i = 0; i < color_count; i += color_incr) {
    const double color_idx = (color_count - 1 - i) / scale_;

    painter.setPen(getColor(color_idx / color_count), true);
    painter.drawLine(odb::Point(legend_left, box_top),
                     odb::Point(legend_right, box_top));
    box_top -= box_height;
  }

  // draw key values
  painter.setPen(Painter::black, true);
  painter.setBrush(Painter::transparent);
  for (const auto& [pt, text] : legend_key_points) {
    painter.drawString(pt.x(), pt.y(), key_anchor, text);
  }
  painter.drawRect(odb::Rect(legend_left, box_top, legend_right, legend_top));
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

const Descriptor* Gui::getDescriptor(const std::type_info& type) const
{
  auto find_descriptor = descriptors_.find(type);
  if (find_descriptor == descriptors_.end()) {
    logger_->error(
        utl::GUI, 53, "Unable to find descriptor for: {}", type.name());
  }

  return find_descriptor->second.get();
}

void Gui::unregisterDescriptor(const std::type_info& type)
{
  descriptors_.erase(type);
}

const Selected& Gui::getInspectorSelection()
{
  return main_window->getInspector()->getSelection();
}

void Gui::timingCone(odbTerm term, bool fanin, bool fanout)
{
  main_window->timingCone(term, fanin, fanout);
}

void Gui::timingPathsThrough(const std::set<odbTerm>& terms)
{
  main_window->timingPathsThrough(terms);
}

void Gui::addFocusNet(odb::dbNet* net)
{
  main_window->getLayoutTabs()->addFocusNet(net);
}

void Gui::addRouteGuides(odb::dbNet* net)
{
  main_window->getLayoutTabs()->addRouteGuides(net);
}

void Gui::removeRouteGuides(odb::dbNet* net)
{
  main_window->getLayoutTabs()->removeRouteGuides(net);
}

void Gui::addNetTracks(odb::dbNet* net)
{
  main_window->getLayoutTabs()->addNetTracks(net);
}

void Gui::removeNetTracks(odb::dbNet* net)
{
  main_window->getLayoutTabs()->removeNetTracks(net);
}

void Gui::removeFocusNet(odb::dbNet* net)
{
  main_window->getLayoutTabs()->removeFocusNet(net);
}

void Gui::clearFocusNets()
{
  main_window->getLayoutTabs()->clearFocusNets();
}

void Gui::clearRouteGuides()
{
  main_window->getLayoutTabs()->clearRouteGuides();
}

void Gui::clearNetTracks()
{
  main_window->getLayoutTabs()->clearNetTracks();
}

void Gui::setLogger(utl::Logger* logger)
{
  if (logger == nullptr) {
    return;
  }

  logger_ = logger;
  qInstallMessageHandler(message_handler);

  if (enabled()) {
    // gui already requested, so go ahead and set the logger
    main_window->setLogger(logger);
  }
}

void Gui::hideGui()
{
  // ensure continue after close is true, since we want to return to tcl
  setContinueAfterClose();
  main_window->exit();
}

void Gui::showGui(const std::string& cmds, bool interactive)
{
  if (enabled()) {
    logger_->warn(utl::GUI, 8, "GUI already active.");
    return;
  }

  // OR already running, so GUI should not set anything up
  // passing in cmd_argc and cmd_argv to meet Qt application requirement for
  // arguments nullptr for tcl interp to indicate nothing to setup and commands
  // and interactive setting
  startGui(cmd_argc, cmd_argv, nullptr, cmds, interactive);
}

void Gui::init(odb::dbDatabase* db, utl::Logger* logger)
{
  db_ = db;
  setLogger(logger);

  // placement density heatmap
  placement_density_heat_map_
      = std::make_unique<PlacementDensityDataSource>(logger);
  placement_density_heat_map_->registerHeatMap();
}

//////////////////////////////////////////////////

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script,
             bool interactive)
{
  auto gui = gui::Gui::get();
  // ensure continue after close is false
  gui->clearContinueAfterClose();

  QApplication app(argc, argv);
  application = &app;

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  auto* open_road = ord::OpenRoad::openRoad();

  // create new MainWindow
  main_window = new gui::MainWindow;

  open_road->addObserver(main_window);
  if (!interactive) {
    gui->setContinueAfterClose();
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
  main_window->getScriptWidget()->setupTcl(
      interp, interactive, init_openroad, [&]() {
        // init remainder of GUI, to be called immediately after OpenRoad is
        // guaranteed to be initialized.
        main_window->init(open_road->getSta());
        // announce design created to ensure GUI gets setup
        main_window->postReadDb(main_window->getDb());
      });

  // Exit the app if someone chooses exit from the menu in the window
  QObject::connect(main_window, &MainWindow::exit, &app, &QApplication::quit);
  // Track the exit in case it originated during a script
  bool exit_requested = false;
  int exit_code = EXIT_SUCCESS;
  QObject::connect(
      main_window, &MainWindow::exit, [&]() { exit_requested = true; });

  // Hide the Gui if someone chooses hide from the menu in the window
  QObject::connect(main_window, &MainWindow::hide, [gui]() { gui->hideGui(); });

  // Save the window's status into the settings when quitting.
  QObject::connect(
      &app, &QApplication::aboutToQuit, main_window, &MainWindow::saveSettings);

  // execute commands to restore state of gui
  std::string restore_commands;
  for (const auto& cmd : gui->getRestoreStateCommands()) {
    restore_commands += cmd + "\n";
  }
  if (!restore_commands.empty()) {
    // Temporarily connect to script widget to get ending tcl state
    bool tcl_ok = true;
    auto tcl_return_code_connect
        = QObject::connect(main_window->getScriptWidget(),
                           &ScriptWidget::commandExecuted,
                           [&tcl_ok](bool is_ok) { tcl_ok = is_ok; });

    main_window->getScriptWidget()->executeSilentCommand(
        QString::fromStdString(restore_commands));

    // disconnect tcl return lister
    QObject::disconnect(tcl_return_code_connect);

    if (!exit_requested && !tcl_ok) {
      auto& cmds = gui->getRestoreStateCommands();
      if (cmds[cmds.size() - 1]
          == "exit") {  // exit, will be the last command if it is present
        // if there was a failure and exit was requested, exit with failure
        // this will mirror the behavior of tclAppInit
        gui->clearContinueAfterClose();
        exit_code = EXIT_FAILURE;
        exit_requested = true;
      }
    }
  }

  // temporary storage for any exceptions thrown by scripts
  utl::ThreadException exception;
  // Execute script
  if (!script.empty() && !exit_requested) {
    try {
      main_window->getScriptWidget()->executeCommand(
          QString::fromStdString(script));
    } catch (const std::runtime_error& /* e */) {
      exception.capture();
    }
  }

  bool do_exec = interactive && !exception.hasException() && !exit_requested;
  // check if hide was called by script
  if (gui->isContinueAfterClose()) {
    do_exec = false;
  }

  if (do_exec) {
    exit_code = QApplication::exec();
  }

  // cleanup
  open_road->removeObserver(main_window);

  if (!exception.hasException()) {
    // don't save anything if exception occured
    gui->clearRestoreStateCommands();
    // save restore state commands
    for (const auto& cmd : main_window->getRestoreTclCommands()) {
      gui->addRestoreStateCommand(cmd);
    }
  }

  main_window->exit();

  // delete main window and set to nullptr
  delete main_window;
  main_window = nullptr;
  application = nullptr;

  resetConversions();

  // rethow exception, if one happened after cleanup of main_window
  exception.rethrow();

  if (interactive && (!gui->isContinueAfterClose() || exit_requested)) {
    // if exiting, go ahead and exit with gui return code.
    exit(exit_code);
  }

  return exit_code;
}

void Selected::highlight(Painter& painter,
                         const Painter::Color& pen,
                         int pen_width,
                         const Painter::Color& brush,
                         const Painter::Brush& brush_style) const
{
  painter.setPen(pen, true, pen_width);
  painter.setBrush(brush, brush_style);

  return descriptor_->highlight(object_, painter);
}

Descriptor::Properties Selected::getProperties() const
{
  Descriptor::Properties props = descriptor_->getProperties(object_);
  props.insert(props.begin(), {"Name", getName()});
  props.insert(props.begin(), {"Type", getTypeName()});
  odb::Rect bbox;
  if (getBBox(bbox)) {
    props.push_back({"BBox", bbox});
    // convenience; the user may want to know the dimensions
    props.push_back(
        {"BBox Width, Height",
         std::string("(") + Descriptor::Property::convert_dbu(bbox.dx(), false)
             + ", " + Descriptor::Property::convert_dbu(bbox.dy(), false)
             + ")"});
  }

  return props;
}

Descriptor::Actions Selected::getActions() const
{
  auto actions = descriptor_->getActions(object_);

  odb::Rect bbox;
  if (getBBox(bbox)) {
    actions.push_back({"Zoom to", [this, bbox]() -> Selected {
                         auto gui = Gui::get();
                         gui->zoomTo(bbox);
                         return *this;
                       }});
  }

  return actions;
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
  } else if (auto v = std::any_cast<odb::Rect>(&value)) {
    std::string text = "(";
    text += convert_dbu(v->xMin(), false) + ",";
    text += convert_dbu(v->yMin(), false) + "), (";
    text += convert_dbu(v->xMax(), false) + ",";
    text += convert_dbu(v->yMax(), false) + ")";
    return text;
  } else if (auto v = std::any_cast<odb::Point>(&value)) {
    std::string text = fmt::format(
        "({},{})", convert_dbu(v->x(), false), convert_dbu(v->y(), false));
    return text;
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
  gui->init(openroad->getDb(), openroad->getLogger());
}

}  // namespace ord
