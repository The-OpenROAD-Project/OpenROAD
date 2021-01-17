///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "db.h"
#include "dbShape.h"
#include "defin.h"
#include "geom.h"
#include "lefin.h"
#include "mainWindow.h"
#include "openroad/OpenRoad.hh"

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
static gui::MainWindow* mainWindow;

Gui* Gui::singleton_ = nullptr;

Gui* Gui::get()
{
  if (mainWindow == nullptr) {
    return nullptr;  // batch mode
  }

  if (!singleton_) {
    singleton_ = new Gui();
  }

  return singleton_;
}

void Gui::register_renderer(Renderer* renderer)
{
  renderers_.insert(renderer);
  redraw();
}

void Gui::unregister_renderer(Renderer* renderer)
{
  renderers_.erase(renderer);
  redraw();
}

void Gui::redraw()
{
  mainWindow->redraw();
}

void Gui::status(const std::string& message)
{
  mainWindow->status(message);
}

void Gui::pause()
{
  mainWindow->pause();
}

void Gui::addSelectedNet(const char* name)
{
  auto block = getBlock(mainWindow->getDb());
  if (!block) {
    return;
  }

  auto net = block->findNet(name);
  if (!net) {
    return;
  }

  mainWindow->addSelected(Selected(net, OpenDbDescriptor::get()));
}

void Gui::addSelectedNets(const char* pattern,
                          bool matchCase,
                          bool matchRegEx,
                          bool addToHighlightSet,
                          unsigned highlightGroup)
{
  auto block = getBlock(mainWindow->getDb());
  if (!block) {
    return;
  }

  QRegExp re(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
  SelectionSet nets;
  if (matchRegEx == true) {
    QRegExp re(pattern,
               matchCase == true ? Qt::CaseSensitive : Qt::CaseInsensitive,
               QRegExp::Wildcard);

    for (auto* net : block->getNets()) {
      if (re.exactMatch(net->getConstName())) {
        nets.emplace(net, OpenDbDescriptor::get());
      }
    }
  } else if (matchCase == false) {
    for (auto* net : block->getNets()) {
      if (boost::iequals(pattern, net->getConstName()))
        nets.emplace(net, OpenDbDescriptor::get());
    }
  } else {
    for (auto* net : block->getNets()) {
      if (pattern == net->getConstName()) {
        nets.emplace(net, OpenDbDescriptor::get());
      }
    }
  }

  mainWindow->addSelected(nets);
  if (addToHighlightSet == true)
    mainWindow->addHighlighted(nets, highlightGroup);
}  // namespace gui

void Gui::addSelectedInst(const char* name)
{
  auto block = getBlock(mainWindow->getDb());
  if (!block) {
    return;
  }

  auto inst = block->findInst(name);
  if (!inst) {
    return;
  }

  mainWindow->addSelected(Selected(inst, OpenDbDescriptor::get()));
}

void Gui::addSelectedInsts(const char* pattern,
                           bool matchCase,
                           bool matchRegEx,
                           bool addToHighlightSet,
                           unsigned highlightGroup)
{
  auto block = getBlock(mainWindow->getDb());
  if (!block) {
    return;
  }

  SelectionSet insts;
  if (matchRegEx) {
    QRegExp re(pattern,
               matchCase == true ? Qt::CaseSensitive : Qt::CaseInsensitive,
               QRegExp::Wildcard);
    for (auto* inst : block->getInsts()) {
      if (re.exactMatch(inst->getConstName())) {
        insts.emplace(inst, OpenDbDescriptor::get());
      }
    }
  } else if (matchCase == false) {
    for (auto* inst : block->getInsts()) {
      if (boost::iequals(inst->getConstName(), pattern))
        insts.emplace(inst, OpenDbDescriptor::get());
    }
  } else {
    for (auto* inst : block->getInsts()) {
      if (pattern == inst->getConstName()) {
        insts.emplace(inst, OpenDbDescriptor::get());
        break;  // There can't be two insts with the same name
      }
    }
  }

  mainWindow->addSelected(insts);
  if (addToHighlightSet == true)
    mainWindow->addHighlighted(insts, highlightGroup);
}

bool Gui::anyObjectInSet(bool selectionSet, bool instType) const
{
  return mainWindow->anyObjectInSet(selectionSet, instType);
}

void Gui::selectHighlightConnectedInsts(bool selectFlag, int highlightGroup)
{
  return mainWindow->selectHighlightConnectedInsts(selectFlag, highlightGroup);
}
void Gui::selectHighlightConnectedNets(bool selectFlag,
                                       bool output,
                                       bool input,
                                       int highlightGroup)
{
  return mainWindow->selectHighlightConnectedNets(
      selectFlag, output, input, highlightGroup);
}

void Gui::addInstToHighlightSet(const char* name, unsigned highlightGroup)
{
  auto block = getBlock(mainWindow->getDb());
  if (!block) {
    return;
  }

  auto inst = block->findInst(name);
  if (!inst) {
    return;
  }
  SelectionSet selInstSet;
  selInstSet.insert(Selected(inst, OpenDbDescriptor::get()));
  mainWindow->addHighlighted(selInstSet, highlightGroup);
}

void Gui::addNetToHighlightSet(const char* name, unsigned highlightGroup)
{
  auto block = getBlock(mainWindow->getDb());
  if (!block) {
    return;
  }

  auto net = block->findNet(name);
  if (!net) {
    return;
  }
  SelectionSet selNetSet;
  selNetSet.insert(Selected(net, OpenDbDescriptor::get()));
  mainWindow->addHighlighted(selNetSet, highlightGroup);
}

void Gui::clearSelections()
{
  mainWindow->setSelected(Selected());
}

void Gui::clearHighlights(int highlightGroup)
{
  mainWindow->clearHighlighted(highlightGroup);
}

void Gui::zoomTo(const odb::Rect& rect_dbu)
{
  mainWindow->zoomTo(rect_dbu);
}

Renderer::~Renderer()
{
  gui::Gui::get()->unregister_renderer(this);
}

OpenDbDescriptor* OpenDbDescriptor::singleton_ = nullptr;
OpenDbDescriptor* OpenDbDescriptor::get()
{
  if (!singleton_) {
    singleton_ = new OpenDbDescriptor();
  }

  return singleton_;
}

std::string OpenDbDescriptor::getName(void* object) const
{
  odb::dbObject* db_obj = static_cast<odb::dbObject*>(object);
  switch (db_obj->getObjectType()) {
    case odb::dbNetObj:
      return "Net: " + static_cast<odb::dbNet*>(db_obj)->getName();
    case odb::dbInstObj:
      return "Inst: " + static_cast<odb::dbInst*>(db_obj)->getName();
    default:
      return db_obj->getObjName();
  }
}

std::string OpenDbDescriptor::getLocation(void* object) const
{
  odb::dbObject* db_obj = static_cast<odb::dbObject*>(object);
  auto block = getBlock(mainWindow->getDb());
  auto toMicrons = block->getDbUnitsPerMicron();
  switch (db_obj->getObjectType()) {
    case odb::dbNetObj: {
      auto net = static_cast<odb::dbNet*>(db_obj);
      auto wire = net->getWire();
      odb::Rect wireBBox;
      if (wire && wire->getBBox(wireBBox)) {
        std::stringstream ss;
        ss << "[(" << wireBBox.xMin() / toMicrons << ","
           << wireBBox.yMin() / toMicrons << "), ("
           << wireBBox.xMax() / toMicrons << "," << wireBBox.yMax() / toMicrons
           << ")]";
        return ss.str();
      }
      return std::string("NA");
    } break;
    case odb::dbInstObj: {
      auto instObj = static_cast<odb::dbInst*>(db_obj);
      auto instBBox = instObj->getBBox();
      auto placementStatus = instObj->getPlacementStatus().getString();
      auto instOrient = instObj->getOrient().getString();
      std::stringstream ss;
      ss << "[(" << instBBox->xMin() / toMicrons << ","
         << instBBox->yMin() / toMicrons << "), ("
         << instBBox->xMax() / toMicrons << "," << instBBox->yMax() / toMicrons
         << ")], " << instOrient << ": " << placementStatus;
      return ss.str();
    } break;
    default:
      return std::string("NA");
  }
}

bool OpenDbDescriptor::getBBox(void* object, odb::Rect& bbox) const
{
  odb::dbObject* db_obj = static_cast<odb::dbObject*>(object);
  switch (db_obj->getObjectType()) {
    case odb::dbNetObj: {
      auto net = static_cast<odb::dbNet*>(db_obj);
      auto wire = net->getWire();
      if (wire && wire->getBBox(bbox)) {
        return true;
      }
      return false;
    }
    case odb::dbInstObj: {
      auto instObj = static_cast<odb::dbInst*>(db_obj);
      auto instBBox = instObj->getBBox();
      bbox.set_xlo(instBBox->xMin());
      bbox.set_ylo(instBBox->yMin());
      bbox.set_xhi(instBBox->xMax());
      bbox.set_yhi(instBBox->yMax());
      return true;
    }
  }
  return false;
}

void OpenDbDescriptor::highlight(void* object,
                                 Painter& painter,
                                 bool selectFlag,
                                 int highlightGroup) const
{
  auto highlightColor = Painter::persistHighlight;
  if (selectFlag == true) {
    painter.setPen(Painter::highlight, true);
    painter.setBrush(Painter::transparent);
  } else {
    if (highlightGroup >= 0 && highlightGroup < 7) {
      highlightColor = Painter::highlightColors[highlightGroup];
      highlightColor.a = 100;
      painter.setPen(highlightColor, true);
      painter.setBrush(highlightColor);
    } else
      painter.setPen(Painter::persistHighlight);
  }
  odb::dbObject* db_obj = static_cast<odb::dbObject*>(object);
  switch (db_obj->getObjectType()) {
    case odb::dbNetObj: {
      auto net = static_cast<odb::dbNet*>(db_obj);

      // Draw regular routing
      odb::Rect rect;
      odb::dbWire* wire = net->getWire();
      if (wire) {
        odb::dbWireShapeItr it;
        it.begin(wire);
        odb::dbShape shape;
        while (it.next(shape)) {
          shape.getBox(rect);
          painter.drawRect(rect);
        }
      } else {
        std::set<odb::Point> driverLocs;
        std::set<odb::Point> sinkLocs;

        auto iTerms = net->getITerms();
        // auto bTerms = net->getBTerms();

        auto iiter = iTerms.begin();
        auto iiterE = iTerms.end();
        for (; iiter != iiterE; ++iiter) {
          auto itermInst = (*iiter)->getInst();
          odb::dbBox* bbox = itermInst->getBBox();
          odb::Rect rect;
          bbox->getBox(rect);
          odb::Point rectCenter((rect.xMax() + rect.xMin()) / 2.0,
                                (rect.yMax() + rect.yMin()) / 2.0);
          if ((*iiter)->getIoType() == INPUT || (*iiter)->getIoType() == INOUT)
            sinkLocs.insert(rectCenter);
          else
            driverLocs.insert(rectCenter);
        }
        if (driverLocs.empty() || sinkLocs.empty())
          return;
        highlightColor.a = 255;
        painter.setPen(highlightColor, true);
        painter.setBrush(highlightColor);
        for (auto& driver : driverLocs) {
          for (auto& sink : sinkLocs) {
            painter.drawLine(driver, sink);
          }
        }
      }

      // Draw special (i.e. geometric) routing
      for (auto swire : net->getSWires()) {
        for (auto sbox : swire->getWires()) {
          sbox->getBox(rect);
          painter.drawGeomShape(sbox->getGeomShape());
          // painter.drawRect(rect);
        }
      }
      break;
    }
    case odb::dbInstObj: {
      auto inst = static_cast<odb::dbInst*>(db_obj);
      odb::dbPlacementStatus status = inst->getPlacementStatus();
      if (status == odb::dbPlacementStatus::NONE
          || status == odb::dbPlacementStatus::UNPLACED) {
        return;
      }
      odb::dbBox* bbox = inst->getBBox();
      odb::Rect rect;
      bbox->getBox(rect);
      painter.drawRect(rect);
      break;
    }
    default:
      throw std::runtime_error("Unsupported type for highlighting");
  }
}

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
int start_gui(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  gui::MainWindow win;
  mainWindow = &win;
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

}  // namespace gui

namespace ord {

extern "C" {
extern int Gui_Init(Tcl_Interp* interp);
}

void initGui(OpenRoad* openroad)
{
  // Define swig TCL commands.
  Gui_Init(openroad->tclInterp());
  if (gui::mainWindow) {
    gui::mainWindow->setLogger(openroad->getLogger());
  }
}

}  // namespace ord
