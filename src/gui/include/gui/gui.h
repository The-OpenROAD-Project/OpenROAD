///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, The Regents of the University of California
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

#include <any>
#include <array>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <variant>

#include "odb/db.h"

struct Tcl_Interp;

namespace utl {
class Logger;
}  // namespace utl

namespace gui {
class HeatMapDataSource;
class PlacementDensityDataSource;
class Painter;
class Selected;
class Options;

// A collection of selected objects
using SelectionSet = std::set<Selected>;
using HighlightSet = std::array<SelectionSet, 8>;  // Only 8 Discrete Highlight
                                                   // Color is supported for now

using DBUToString = std::function<std::string(int, bool)>;
using StringToDBU = std::function<int(const std::string&, bool*)>;

// This is an API that the Renderer instances will use to do their
// rendering.  This is subclassed in the gui module and hides Qt from
// the clients.  Clients will only deal with this API and not Qt itself,
// which contains the Qt dependencies to the gui module.
class Painter
{
 public:
  struct Color
  {
    constexpr Color() : r(0), g(0), b(0), a(255) {}
    constexpr Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a)
    {
    }
    constexpr Color(const Color& color, int a)
        : r(color.r), g(color.g), b(color.b), a(a)
    {
    }

    int r;
    int g;
    int b;
    int a;

    bool operator==(const Color& other) const
    {
      return (r == other.r) && (g == other.g) && (b == other.b)
             && (a == other.a);
    }
  };

  static inline const Color black{0x00, 0x00, 0x00, 0xff};
  static inline const Color white{0xff, 0xff, 0xff, 0xff};
  static inline const Color dark_gray{0x80, 0x80, 0x80, 0xff};
  static inline const Color gray{0xa0, 0xa0, 0xa4, 0xff};
  static inline const Color light_gray{0xc0, 0xc0, 0xc0, 0xff};
  static inline const Color red{0xff, 0x00, 0x00, 0xff};
  static inline const Color green{0x00, 0xff, 0x00, 0xff};
  static inline const Color blue{0x00, 0x00, 0xff, 0xff};
  static inline const Color cyan{0x00, 0xff, 0xff, 0xff};
  static inline const Color magenta{0xff, 0x00, 0xff, 0xff};
  static inline const Color yellow{0xff, 0xff, 0x00, 0xff};
  static inline const Color dark_red{0x80, 0x00, 0x00, 0xff};
  static inline const Color dark_green{0x00, 0x80, 0x00, 0xff};
  static inline const Color dark_blue{0x00, 0x00, 0x80, 0xff};
  static inline const Color dark_cyan{0x00, 0x80, 0x80, 0xff};
  static inline const Color dark_magenta{0x80, 0x00, 0x80, 0xff};
  static inline const Color dark_yellow{0x80, 0x80, 0x00, 0xff};
  static inline const Color transparent{0x00, 0x00, 0x00, 0x00};

  static inline const std::array<Painter::Color, 8> highlightColors{
      Color(Painter::green, 100),
      Color(Painter::yellow, 100),
      Color(Painter::cyan, 100),
      Color(Painter::magenta, 100),
      Color(Painter::red, 100),
      Color(Painter::dark_green, 100),
      Color(Painter::dark_magenta, 100),
      Color(Painter::blue, 100)};

  // The color to highlight in
  static inline const Color highlight = yellow;
  static inline const Color persistHighlight = yellow;

  Painter(Options* options, const odb::Rect& bounds, double pixels_per_dbu)
      : options_(options), bounds_(bounds), pixels_per_dbu_(pixels_per_dbu)
  {
  }
  virtual ~Painter() = default;

  // Get the current pen color
  virtual Color getPenColor() = 0;

  // Set the pen to whatever the user has chosen for this layer
  virtual void setPen(odb::dbTechLayer* layer, bool cosmetic = false) = 0;

  // Set the pen to an RGBA value
  virtual void setPen(const Color& color, bool cosmetic = false, int width = 1)
      = 0;

  virtual void setPenWidth(int width) = 0;
  // Set the brush to whatever the user has chosen for this layer
  // The alpha value may be overridden
  virtual void setBrush(odb::dbTechLayer* layer, int alpha = -1) = 0;

  // Set the brush to whatever the user has chosen for this layer
  enum Brush
  {
    NONE,
    SOLID,
    DIAGONAL,
    CROSS,
    DOTS
  };
  virtual void setBrush(const Color& color, const Brush& style = SOLID) = 0;

  // Set the pen to an RGBA value and the brush
  void setPenAndBrush(const Color& color,
                      bool cosmetic = false,
                      const Brush& style = SOLID,
                      int width = 1)
  {
    setPen(color, cosmetic, width);
    setBrush(color, style);
  }

  // save the state of the painter so it can be restored later
  virtual void saveState() = 0;
  // restore the saved state of the painter
  virtual void restoreState() = 0;

  // Draw an octagon shape as a polygon with coordinates in DBU with the
  // current pen/brush
  virtual void drawOctagon(const odb::Oct& oct) = 0;

  // Draw a rect with coordinates in DBU with the current pen/brush; draws a
  // round rect if roundX > 0 or roundY > 0
  virtual void drawRect(const odb::Rect& rect, int roundX = 0, int roundY = 0)
      = 0;

  // Draw a line with coordinates in DBU with the current pen
  virtual void drawLine(const odb::Point& p1, const odb::Point& p2) = 0;

  // Draw a circle with coordinates in DBU with the current pen
  virtual void drawCircle(int x, int y, int r) = 0;

  // Draw an 'X' with coordinates in DBU with the current pen.  The
  // crossing point of the X is at (x,y). The size is the width and
  // height of the X.
  virtual void drawX(int x, int y, int size) = 0;

  virtual void drawPolygon(const odb::Polygon& polygon) = 0;
  virtual void drawPolygon(const std::vector<odb::Point>& points) = 0;

  enum Anchor
  {
    // four corners
    BOTTOM_LEFT,
    BOTTOM_RIGHT,
    TOP_LEFT,
    TOP_RIGHT,

    // centers
    CENTER,
    BOTTOM_CENTER,
    TOP_CENTER,
    LEFT_CENTER,
    RIGHT_CENTER
  };
  virtual void drawString(int x,
                          int y,
                          Anchor anchor,
                          const std::string& s,
                          bool rotate_90 = false)
      = 0;
  virtual odb::Rect stringBoundaries(int x,
                                     int y,
                                     Anchor anchor,
                                     const std::string& s)
      = 0;

  virtual void drawRuler(int x0,
                         int y0,
                         int x1,
                         int y1,
                         bool euclidian = true,
                         const std::string& label = "")
      = 0;

  // Draw a line with coordinates in DBU with the current pen
  void drawLine(int xl, int yl, int xh, int yh)
  {
    drawLine(odb::Point(xl, yl), odb::Point(xh, yh));
  }

  inline double getPixelsPerDBU() { return pixels_per_dbu_; }
  inline Options* getOptions() { return options_; }
  inline const odb::Rect& getBounds() { return bounds_; }

 private:
  Options* options_;
  const odb::Rect bounds_;
  double pixels_per_dbu_;
};

// This interface allows the GUI to interact with selected objects of
// types it knows nothing about.  It can just ask the descriptor to
// give it information about the foreign object (eg attributes like
// name).
class Descriptor
{
 public:
  virtual ~Descriptor() = default;
  virtual std::string getName(std::any object) const = 0;
  virtual std::string getShortName(std::any object) const
  {
    return getName(std::move(object));
  }
  virtual std::string getTypeName() const = 0;
  virtual std::string getTypeName(std::any /* object */) const
  {
    return getTypeName();
  }
  virtual bool getBBox(std::any object, odb::Rect& bbox) const = 0;

  virtual bool isInst(std::any /* object */) const { return false; }
  virtual bool isNet(std::any /* object */) const { return false; }

  virtual bool getAllObjects(SelectionSet& /* objects */) const = 0;

  // A property is a name and a value.
  struct Property
  {
    std::string name;
    std::any value;

    static DBUToString convert_dbu;
    static StringToDBU convert_string;

    static std::string toString(const std::any& /* value */);
    std::string toString() const { return toString(value); };
  };
  using Properties = std::vector<Property>;
  using PropertyList = std::vector<std::pair<std::any, std::any>>;

  // An action is a name and a callback function, the function should return
  // the next object to select (when deleting the object just return Selected())
  using ActionCallback = std::function<Selected(void)>;
  struct Action
  {
    std::string name;
    ActionCallback callback;
  };
  using Actions = std::vector<Action>;
  static constexpr std::string_view deselect_action_ = "deselect";

  // An editor is a callback function and a list of possible values (this can be
  // empty), the name of the editor should match the property it modifies the
  // callback should return true if the edit was successful, otherwise false
  struct EditorOption
  {
    std::string name;
    std::any value;
  };
  using EditorCallback = std::function<bool(const std::any&)>;
  struct Editor
  {
    EditorCallback callback;
    std::vector<EditorOption> options;
  };
  using Editors = std::map<std::string, Editor>;

  virtual Properties getProperties(std::any object) const = 0;
  virtual Actions getActions(std::any /* object */) const { return Actions(); }
  virtual Editors getEditors(std::any /* object */) const { return Editors(); }

  virtual Selected makeSelected(std::any object) const = 0;

  virtual bool lessThan(std::any l, std::any r) const = 0;

  static const Editor makeEditor(const EditorCallback& func,
                                 const std::vector<EditorOption>& options)
  {
    return {func, options};
  }
  static const Editor makeEditor(const EditorCallback& func)
  {
    return makeEditor(func, {});
  }

  // The caller (Selected and Renderers) will pre-configure the Painter's pen
  // and brush before calling.
  virtual void highlight(std::any object, Painter& painter) const = 0;
  virtual bool isSlowHighlight(std::any /* object */) const { return false; }

  static std::string convertUnits(double value,
                                  bool area = false,
                                  int digits = 3);
};

// An object selected in the gui.  The object is stored as a
// std::any to allow any client objects to be stored.  The descriptor
// is the API for the object as described above.  This doesn't
// require the client object to use inheritance from an interface.
class Selected
{
 public:
  // Null case
  Selected() : object_({}), descriptor_(nullptr) {}

  Selected(std::any object, const Descriptor* descriptor)
      : object_(std::move(object)), descriptor_(descriptor)
  {
  }

  std::string getName() const { return descriptor_->getName(object_); }
  std::string getShortName() const
  {
    return descriptor_->getShortName(object_);
  }
  std::string getTypeName() const { return descriptor_->getTypeName(object_); }
  bool getBBox(odb::Rect& bbox) const
  {
    return descriptor_->getBBox(object_, bbox);
  }

  bool isInst() const { return descriptor_->isInst(object_); }
  bool isNet() const { return descriptor_->isNet(object_); }
  std::any getObject() const { return object_; }

  // If the select_flag is false, the drawing will happen in highlight mode.
  // Highlight shapes are persistent which will not get removed from
  // highlightSet, if the user clicks on layout view as in case of selectionSet
  void highlight(Painter& painter,
                 const Painter::Color& pen = Painter::persistHighlight,
                 int pen_width = 0,
                 const Painter::Color& brush = Painter::transparent,
                 const Painter::Brush& brush_style
                 = Painter::Brush::SOLID) const;
  bool isSlowHighlight() const { return descriptor_->isSlowHighlight(object_); }

  Descriptor::Properties getProperties() const;

  std::any getProperty(const std::string& name) const
  {
    for (auto& [prop, value] : getProperties()) {
      if (prop == name) {
        return value;
      }
    }
    return std::any();
  }

  Descriptor::Actions getActions() const;

  Descriptor::Editors getEditors() const
  {
    return descriptor_->getEditors(object_);
  }

  operator bool() const { return object_.has_value(); }

  // For SelectionSet
  friend bool operator<(const Selected& l, const Selected& r)
  {
    auto& l_type_info = l.object_.type();
    auto& r_type_info = r.object_.type();
    if (l_type_info == r_type_info) {
      return l.descriptor_->lessThan(l.object_, r.object_);
    }
    return l_type_info.before(r_type_info);
  }

  friend bool operator==(const Selected& l, const Selected& r)
  {
    return !(l < r) && !(r < l);
  }

 private:
  std::any object_;
  const Descriptor* descriptor_;
};

// This is an interface for classes that wish to be called to render
// on the layout.  Clients will subclass and register their instances with
// the Gui singleton.
class Renderer
{
 public:
  // Automatically unregisters this renderable
  virtual ~Renderer();

  // Draw on top of the particular layer.
  virtual void drawLayer(odb::dbTechLayer* /* layer */, Painter& /* painter */)
  {
  }

  // Draw on top of the layout in general after the layers
  // have been drawn.
  virtual void drawObjects(Painter& /* painter */) {}

  // Handle user clicks.  Layer is a nullptr for the
  // object not associated with a layer.
  // Return true if an object was found; false otherwise.
  virtual SelectionSet select(odb::dbTechLayer* /* layer */,
                              const odb::Rect& /* region */)
  {
    return SelectionSet();
  }

  // Used to trigger a draw
  void redraw();

  // If group_name is empty, no display group is created and all items will be
  // added at the top level of the display control list else, a group is created
  // and items added under that list
  virtual const char* getDisplayControlGroupName() { return ""; }

  // Used to register display controls for this renderer.
  // DisplayControls is a map with the name of the control and the initial
  // setting for the control
  using DisplayControlCallback = std::function<void(void)>;
  struct DisplayControl
  {
    bool visibility;
    DisplayControlCallback interactive_setup;
    std::set<std::string> mutual_exclusivity;
  };
  using DisplayControls = std::map<std::string, DisplayControl>;
  const DisplayControls& getDisplayControls() { return controls_; }

  // Used to check the value of the display control
  bool checkDisplayControl(const std::string& name);
  // Used to set the value of the display control
  void setDisplayControl(const std::string& name, bool value);

  virtual std::string getSettingsGroupName() { return ""; }
  using Setting = std::variant<bool, int, double, std::string>;
  using Settings = std::map<std::string, Setting>;
  virtual Settings getSettings();
  virtual void setSettings(const Settings& settings);

  template <typename T>
  static void setSetting(const Settings& settings,
                         const std::string& key,
                         T& value)
  {
    if (settings.count(key) == 1) {
      value = std::get<T>(settings.at(key));
    }
  }

 protected:
  // Adds a display control
  void addDisplayControl(const std::string& name,
                         bool initial_visible = false,
                         const DisplayControlCallback& setup
                         = DisplayControlCallback(),
                         const std::vector<std::string>& mutual_exclusivity
                         = {});

 private:
  // Holds map of display controls and callback function
  DisplayControls controls_;
};

class SpectrumGenerator
{
 public:
  SpectrumGenerator(double max_value);

  int getColorCount() const;
  Painter::Color getColor(double value, int alpha = 150) const;

  void drawLegend(
      Painter& painter,
      const std::vector<std::pair<int, std::string>>& legend_key) const;

 private:
  static const unsigned char spectrum_[256][3];
  double scale_;
};

// This is the API for the rest of the program to interact with the
// GUI.  This class is accessed by the GUI implementation to interact
// with the rest of the system.  This class itself doesn't hold the
// GUI implementation.  This class is a singleton.
class Gui
{
 public:
  // Registered renderers must remain valid until they are
  // unregistered.
  void registerRenderer(Renderer* renderer);
  void unregisterRenderer(Renderer* renderer);

  // Make a Selected any object in the gui.  It should have a descriptor
  // registered for its exact type to be useful.
  Selected makeSelected(const std::any& object);

  // Set the current selected object in the gui.
  void setSelected(const Selected& selection);

  void removeSelectedByType(const std::string& type);

  template <class T>
  void removeSelected()
  {
    const auto* descriptor = getDescriptor<T>();
    removeSelectedByType(descriptor->getTypeName());
  }

  // Add a net to the selection set
  void addSelectedNet(const char* name);

  // Add an instance to the selection set
  void addSelectedInst(const char* name);

  // check if any object(inst/net) is present in sect/highlight set
  bool anyObjectInSet(bool selection_set, odb::dbObjectType obj_type) const;

  void selectHighlightConnectedInsts(bool select_flag, int highlight_group = 0);
  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group = 0);
  void selectHighlightConnectedBufferTrees(bool select_flag,
                                           int highlight_group = 0);
  void addInstToHighlightSet(const char* name, int highlight_group = 0);
  void addNetToHighlightSet(const char* name, int highlight_group = 0);

  int selectAt(const odb::Rect& area, bool append = true);
  int selectNext();
  int selectPrevious();
  void animateSelection(int repeat = 0);

  std::string addRuler(int x0,
                       int y0,
                       int x1,
                       int y1,
                       const std::string& label = "",
                       const std::string& name = "",
                       bool euclidian = true);
  void deleteRuler(const std::string& name);

  void clearSelections();
  void clearHighlights(int highlight_group = 0);
  void clearRulers();

  int select(const std::string& type,
             const std::string& name_filter = "",
             const std::string& attribute = "",
             const std::any& value = "",
             bool filter_case_sensitive = true,
             int highlight_group = -1);

  // Zoom to the given rectangle
  void zoomTo(const odb::Rect& rect_dbu);
  void zoomIn();
  void zoomIn(const odb::Point& focus_dbu);
  void zoomOut();
  void zoomOut(const odb::Point& focus_dbu);
  void centerAt(const odb::Point& focus_dbu);
  void setResolution(double pixels_per_dbu);

  // Save layout to an image file
  void saveImage(const std::string& filename,
                 const odb::Rect& region = odb::Rect(),
                 int width_px = 0,
                 double dbu_per_pixel = 0,
                 const std::map<std::string, bool>& display_settings = {});

  // Save clock tree view
  void saveClockTreeImage(const std::string& clock_name,
                          const std::string& filename,
                          const std::string& corner = "",
                          int width_px = 0,
                          int height_px = 0);

  // modify display controls
  void setDisplayControlsVisible(const std::string& name, bool value);
  void setDisplayControlsSelectable(const std::string& name, bool value);
  // Get the visibility/selectability for a control in the 'Display Control'
  // panel.
  bool checkDisplayControlsVisible(const std::string& name);
  bool checkDisplayControlsSelectable(const std::string& name);

  // Used to save and restore the display controls, useful for batch operations
  void saveDisplayControls();
  void restoreDisplayControls();

  // Used to add and remove focus nets from layout
  void addFocusNet(odb::dbNet* net);
  void removeFocusNet(odb::dbNet* net);
  void clearFocusNets();
  // Used to add, remove and clear route guides
  void addRouteGuides(odb::dbNet* net);
  void removeRouteGuides(odb::dbNet* net);
  void clearRouteGuides();
  // Used to add, remove and clear assigned tracks
  void addNetTracks(odb::dbNet* net);
  void removeNetTracks(odb::dbNet* net);
  void clearNetTracks();

  // show/hide widgets
  void showWidget(const std::string& name, bool show);

  // trigger actions in the GUI
  void triggerAction(const std::string& name);

  // adding custom buttons to toolbar
  std::string addToolbarButton(const std::string& name,
                               const std::string& text,
                               const std::string& script,
                               bool echo);
  void removeToolbarButton(const std::string& name);

  // adding custom menu items to menu bar
  std::string addMenuItem(const std::string& name,
                          const std::string& path,
                          const std::string& text,
                          const std::string& script,
                          const std::string& shortcut,
                          bool echo);
  void removeMenuItem(const std::string& name);

  // request for user input
  std::string requestUserInput(const std::string& title,
                               const std::string& question);

  using odbTerm = std::variant<odb::dbITerm*, odb::dbBTerm*>;
  void timingCone(odbTerm term, bool fanin, bool fanout);
  void timingPathsThrough(const std::set<odbTerm>& terms);

  // open DRC
  void loadDRC(const std::string& filename);

  // Force an immediate redraw.
  void redraw();

  // Waits for the user to click continue before returning
  // Draw events are processed while paused.
  // timeout is in milliseconds (0 is no timeout)
  void pause(int timeout = 0);

  // Show a message in the status bar
  void status(const std::string& message);

  const std::set<Renderer*>& renderers() { return renderers_; }

  void fit();

  // Called to hide the gui and return to tcl command line
  void hideGui();

  // Called to show the gui and return to tcl command line
  void showGui(const std::string& cmds = "", bool interactive = true);

  // set the system logger
  void setLogger(utl::Logger* logger);

  // check if tcl should take over after closing gui
  bool isContinueAfterClose() { return continue_after_close_; }
  // set continue after close, needs to be set when running in non-interactive
  // mode
  void setContinueAfterClose() { continue_after_close_ = true; }
  // clear continue after close, needed to reset before GUI starts
  void clearContinueAfterClose() { continue_after_close_ = false; }

  const Selected& getInspectorSelection();

  void setHeatMapSetting(const std::string& name,
                         const std::string& option,
                         const Renderer::Setting& value);
  Renderer::Setting getHeatMapSetting(const std::string& name,
                                      const std::string& option);
  void dumpHeatMap(const std::string& name, const std::string& file);

  // accessors for to add and remove commands needed to restore the state of the
  // gui
  const std::vector<std::string>& getRestoreStateCommands()
  {
    return tcl_state_commands_;
  }
  void addRestoreStateCommand(const std::string& cmd)
  {
    tcl_state_commands_.push_back(cmd);
  }
  void clearRestoreStateCommands() { tcl_state_commands_.clear(); }

  template <class T>
  void registerDescriptor(const Descriptor* descriptor)
  {
    registerDescriptor(typeid(T), descriptor);
  }

  template <class T>
  const Descriptor* getDescriptor() const
  {
    return getDescriptor(typeid(T));
  }

  template <class T>
  void unregisterDescriptor()
  {
    unregisterDescriptor(typeid(T));
  }

  void registerHeatMap(HeatMapDataSource* heatmap);
  void unregisterHeatMap(HeatMapDataSource* heatmap);
  const std::set<HeatMapDataSource*>& getHeatMaps() { return heat_maps_; }
  HeatMapDataSource* getHeatMap(const std::string& name);

  // returns the Gui singleton
  static Gui* get();

  // Will return true if the GUI is active, false otherwise
  static bool enabled();

  // initialize the GUI
  void init(odb::dbDatabase* db, utl::Logger* logger);

 private:
  Gui();

  void registerDescriptor(const std::type_info& type,
                          const Descriptor* descriptor);
  const Descriptor* getDescriptor(const std::type_info& type) const;
  void unregisterDescriptor(const std::type_info& type);

  bool filterSelectionProperties(const Descriptor::Properties& properties,
                                 const std::string& attribute,
                                 const std::any& value,
                                 bool& is_valid_attribute);

  // flag to indicate if tcl should take over after gui closes
  bool continue_after_close_;

  utl::Logger* logger_;
  odb::dbDatabase* db_;

  // Maps types to descriptors
  std::unordered_map<std::type_index, std::unique_ptr<const Descriptor>>
      descriptors_;
  // Heatmaps
  std::set<HeatMapDataSource*> heat_maps_;

  // tcl commands needed to restore state
  std::vector<std::string> tcl_state_commands_;

  std::set<Renderer*> renderers_;

  std::unique_ptr<PlacementDensityDataSource> placement_density_heat_map_;

  static Gui* singleton_;
};

// The main entry point
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script = "",
             bool interactive = true);

}  // namespace gui
