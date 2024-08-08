///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2023, Precision Innovations Inc.
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

#pragma once

#include <QTabWidget>
#include <functional>
#include <memory>

#include "gui/gui.h"
#include "layoutViewer.h"

namespace gui {

class LayoutScroll;
class LayoutViewer;
class Options;
class Ruler;
class ScriptWidget;

class LayoutTabs : public QTabWidget
{
  Q_OBJECT

 public:
  LayoutTabs(Options* options,
             ScriptWidget* output_widget,
             const SelectionSet& selected,
             const HighlightSet& highlighted,
             const std::vector<std::unique_ptr<Ruler>>& rulers,
             Gui* gui,
             std::function<bool(void)> usingDBU,
             std::function<bool(void)> usingPolyDecompView,
             std::function<bool(void)> showRulerAsEuclidian,
             std::function<bool(void)> default_mouse_wheel_zoom,
             std::function<int(void)> arrow_keys_scroll_step,
             QWidget* parent = nullptr);

  LayoutViewer* getCurrent() const { return current_viewer_; }

  void setLogger(utl::Logger* logger);

  const std::map<odb::dbModule*, LayoutViewer::ModuleSettings>&
  getModuleSettings()
  {
    return modules_;
  }

  const std::set<odb::dbNet*>& getFocusNets() { return focus_nets_; }
  const std::set<odb::dbNet*>& getRouteGuides() { return route_guides_; }
  const std::set<odb::dbNet*>& getNetTracks() { return net_tracks_; }

  void addFocusNet(odb::dbNet* net);
  void removeFocusNet(odb::dbNet* net);
  void addRouteGuides(odb::dbNet* net);
  void removeRouteGuides(odb::dbNet* net);
  void addNetTracks(odb::dbNet* net);
  void removeNetTracks(odb::dbNet* net);
  void clearFocusNets();
  void clearRouteGuides();
  void clearNetTracks();

 signals:
  void setCurrentBlock(odb::dbBlock* block);
  void newViewer(LayoutViewer* viewer);

  // These are just forwarding from the LayoutViewer(s).  Only the
  // active viewer should be emitting signals, but all are connected
  // as signal-to-signal connections.
  void location(int x, int y);
  void selected(const Selected& selected, bool showConnectivity = false);
  void addSelected(const Selected& selected);
  void addSelected(const SelectionSet& selected);
  void addRuler(int x0, int y0, int x1, int y1);
  void focusNetsChanged();

 public slots:
  void tabChange(int index);

  // These are just forwarding to the current LayoutViewer
  void zoomIn();
  void zoomOut();
  void zoomTo(const odb::Rect& rect_dbu);
  void blockLoaded(odb::dbBlock* block);
  void fit();
  void fullRepaint();
  void startRulerBuild();
  void cancelRulerBuild();
  void selection(const Selected& selection);
  void selectionFocus(const Selected& focus);
  void updateModuleVisibility(odb::dbModule* module, bool visible);
  void updateModuleColor(odb::dbModule* module,
                         const QColor& color,
                         bool user_selected);
  void populateModuleColors(odb::dbBlock* block);
  void exit();
  void commandAboutToExecute();
  void commandFinishedExecuting();
  void resetCache();

  // Method forwarding
  void restoreTclCommands(std::vector<std::string>& cmds);
  void executionPaused();

 private:
  LayoutViewer* current_viewer_ = nullptr;
  std::vector<LayoutViewer*> viewers_;

  Options* options_;
  ScriptWidget* output_widget_;
  const SelectionSet& selected_;
  const HighlightSet& highlighted_;
  const std::vector<std::unique_ptr<Ruler>>& rulers_;
  std::map<odb::dbModule*, LayoutViewer::ModuleSettings> modules_;
  Gui* gui_;
  std::function<bool(void)> usingDBU_;
  std::function<bool(void)> usingPolyDecompView_;
  std::function<bool(void)> showRulerAsEuclidian_;
  std::function<bool(void)> default_mouse_wheel_zoom_;
  std::function<int(void)> arrow_keys_scroll_step_;
  utl::Logger* logger_;
  bool command_executing_ = false;

  // Set of nets to focus drawing on, if empty draw everything
  std::set<odb::dbNet*> focus_nets_;
  // Set of nets to draw route guides for, if empty draw nothing
  std::set<odb::dbNet*> route_guides_;
  // Set of nets to draw assigned tracks for, if empty draw nothing
  std::set<odb::dbNet*> net_tracks_;
};

}  // namespace gui
