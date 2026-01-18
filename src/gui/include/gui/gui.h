// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <any>
#include <array>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"

struct Tcl_Interp;
struct GifWriter;

namespace sta {
class dbSta;
}

namespace utl {
class Logger;
}  // namespace utl

namespace gui {
class HeatMapDataSource;
class PinDensityDataSource;
class PlacementDensityDataSource;
class PowerDensityDataSource;
class Painter;
class Selected;
class Options;

struct GIF
{
  std::string filename;
  std::unique_ptr<GifWriter> writer;
  int height = -1;
  int width = -1;
};

// A collection of selected objects

// Only a finite set of highlight color is supported for now
constexpr int kNumHighlightSet = 16;
using SelectionSet = std::set<Selected>;
using HighlightSet = std::array<SelectionSet, kNumHighlightSet>;

using DBUToString = std::function<std::string(int, bool)>;
using StringToDBU = std::function<int(const std::string&, bool*)>;

// This is an API that the Renderer instances will use to do their
// rendering.  This is subclassed in the gui module and hides Qt from
// the clients.  Clients will only deal with this API and not Qt itself,
// which contains the Qt dependencies to the gui module.
class Painter
{
 public:
  struct Font
  {
    Font(const std::string& name, int size) : name(name), size(size) {}

    std::string name;
    int size;

    bool operator==(const Font& other) const
    {
      return (name == other.name) && (size == other.size);
    }
  };

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

  static inline const Color kBlack{0x00, 0x00, 0x00, 0xff};
  static inline const Color kWhite{0xff, 0xff, 0xff, 0xff};
  static inline const Color kDarkGray{0x80, 0x80, 0x80, 0xff};
  static inline const Color kGray{0xa0, 0xa0, 0xa4, 0xff};
  static inline const Color kLightGray{0xc0, 0xc0, 0xc0, 0xff};
  static inline const Color kRed{0xff, 0x00, 0x00, 0xff};
  static inline const Color kGreen{0x00, 0xff, 0x00, 0xff};
  static inline const Color kBlue{0x00, 0x00, 0xff, 0xff};
  static inline const Color kCyan{0x00, 0xff, 0xff, 0xff};
  static inline const Color kMagenta{0xff, 0x00, 0xff, 0xff};
  static inline const Color kYellow{0xff, 0xff, 0x00, 0xff};
  static inline const Color kDarkRed{0x80, 0x00, 0x00, 0xff};
  static inline const Color kDarkGreen{0x00, 0x80, 0x00, 0xff};
  static inline const Color kDarkBlue{0x00, 0x00, 0x80, 0xff};
  static inline const Color kDarkCyan{0x00, 0x80, 0x80, 0xff};
  static inline const Color kDarkMagenta{0x80, 0x00, 0x80, 0xff};
  static inline const Color kDarkYellow{0x80, 0x80, 0x00, 0xff};
  static inline const Color kOrange{0xff, 0xa5, 0x00, 0xff};
  static inline const Color kPurple{0x80, 0x00, 0x80, 0xff};
  static inline const Color kLime{0xbf, 0xff, 0x00, 0xff};
  static inline const Color kTeal{0x00, 0x80, 0x80, 0xff};
  static inline const Color kPink{0xff, 0xc0, 0xcb, 0xff};
  static inline const Color kBrown{0x8b, 0x45, 0x13, 0xff};
  static inline const Color kIndigo{0x4b, 0x00, 0x82, 0xff};
  static inline const Color kTurquoise{0x40, 0xe0, 0xd0, 0xff};
  static inline const Color kTransparent{0x00, 0x00, 0x00, 0x00};

  static std::map<std::string, Color> colors();
  static Color stringToColor(const std::string& color, utl::Logger* logger);
  static std::string colorToString(const Color& color);

  static inline const std::array<Painter::Color, kNumHighlightSet>
      kHighlightColors{Color(Painter::kGreen, 100),
                       Color(Painter::kYellow, 100),
                       Color(Painter::kCyan, 100),
                       Color(Painter::kMagenta, 100),
                       Color(Painter::kRed, 100),
                       Color(Painter::kDarkGreen, 100),
                       Color(Painter::kDarkMagenta, 100),
                       Color(Painter::kBlue, 100),
                       Color(Painter::kOrange, 100),
                       Color(Painter::kPurple, 100),
                       Color(Painter::kLime, 100),
                       Color(Painter::kTeal, 100),
                       Color(Painter::kPink, 100),
                       Color(Painter::kBrown, 100),
                       Color(Painter::kIndigo, 100),
                       Color(Painter::kTurquoise, 100)};

  // The color to highlight in
  static inline const Color kHighlight = kYellow;
  static inline const Color kPersistHighlight = kYellow;

  virtual ~Painter() = default;

  // Get the current pen color
  virtual Color getPenColor() = 0;

  // Set the pen to whatever the user has chosen for this layer
  virtual void setPen(odb::dbTechLayer* layer, bool cosmetic) = 0;
  void setPen(odb::dbTechLayer* layer) { setPen(layer, false); }

  // Set the pen to an RGBA value
  virtual void setPen(const Color& color, bool cosmetic, int width) = 0;
  void setPen(const Color& color, bool cosmetic) { setPen(color, cosmetic, 1); }
  void setPen(const Color& color) { setPen(color, false); }

  virtual void setPenWidth(int width) = 0;
  // Set the brush to whatever the user has chosen for this layer
  // The alpha value may be overridden
  virtual void setBrush(odb::dbTechLayer* layer, int alpha) = 0;
  void setBrush(odb::dbTechLayer* layer) { setBrush(layer, -1); }

  // Set the brush to whatever the user has chosen for this layer
  enum Brush
  {
    kNone,
    kSolid,
    kDiagonal,
    kCross,
    kDots
  };
  virtual void setBrush(const Color& color, const Brush& style) = 0;
  void setBrush(const Color& color) { setBrush(color, Brush::kSolid); }

  virtual void setFont(const Font& font) = 0;

  // Set the pen to an RGBA value and the brush
  void setPenAndBrush(const Color& color,
                      bool cosmetic = false,
                      const Brush& style = kSolid,
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
  virtual void drawRect(const odb::Rect& rect, int round_x, int round_y) = 0;

  // Draw a rect with coordinates in DBU with the current pen/brush
  void drawRect(const odb::Rect& rect) { drawRect(rect, 0, 0); }

  // Draw a line with coordinates in DBU with the current pen
  virtual void drawLine(const odb::Point& p1, const odb::Point& p2) = 0;
  void drawLine(const odb::Line& line) { drawLine(line.pt0(), line.pt1()); }

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
    kBottomLeft,
    kBottomRight,
    kTopLeft,
    kTopRight,

    // centers
    kCenter,
    kBottomCenter,
    kTopCenter,
    kLeftCenter,
    kRightCenter
  };
  static std::map<std::string, Anchor> anchors();
  static Anchor stringToAnchor(const std::string& anchor, utl::Logger* logger);
  static std::string anchorToString(const Anchor& anchor);
  virtual void drawString(int x,
                          int y,
                          Anchor anchor,
                          const std::string& s,
                          bool rotate_90)
      = 0;
  void drawString(int x, int y, Anchor anchor, const std::string& s)
  {
    drawString(x, y, anchor, s, false);
  }
  virtual odb::Rect stringBoundaries(int x,
                                     int y,
                                     Anchor anchor,
                                     const std::string& s)
      = 0;

  virtual void drawRuler(int x0,
                         int y0,
                         int x1,
                         int y1,
                         bool euclidian,
                         const std::string& label)
      = 0;
  void drawRuler(int x0, int y0, int x1, int y1, bool euclidian)
  {
    drawRuler(x0, y0, x1, y1, euclidian, "");
  }
  void drawRuler(int x0, int y0, int x1, int y1)
  {
    drawRuler(x0, y0, x1, y1, true);
  }

  // Draw a line with coordinates in DBU with the current pen
  void drawLine(int xl, int yl, int xh, int yh)
  {
    drawLine(odb::Point(xl, yl), odb::Point(xh, yh));
  }

  double getPixelsPerDBU() { return pixels_per_dbu_; }
  Options* getOptions() { return options_; }
  const odb::Rect& getBounds() { return bounds_; }

 protected:
  Painter(Options* options, const odb::Rect& bounds, double pixels_per_dbu)
      : options_(options), bounds_(bounds), pixels_per_dbu_(pixels_per_dbu)
  {
  }

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
  virtual std::string getName(const std::any& object) const = 0;
  virtual std::string getShortName(const std::any& object) const
  {
    return getName(object);
  }
  virtual std::string getTypeName() const = 0;
  virtual std::string getTypeName(const std::any& /* object */) const
  {
    return getTypeName();
  }
  virtual bool getBBox(const std::any& object, odb::Rect& bbox) const = 0;

  virtual bool isInst(const std::any& /* object */) const { return false; }
  virtual bool isNet(const std::any& /* object */) const { return false; }

  virtual void visitAllObjects(
      const std::function<void(const Selected&)>& func) const
      = 0;

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
  using ActionCallback = std::function<Selected()>;
  struct Action
  {
    std::string name;
    ActionCallback callback;
  };
  using Actions = std::vector<Action>;
  static constexpr std::string_view kDeselectAction = "deselect";

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

  virtual Properties getProperties(const std::any& object) const = 0;
  virtual Actions getActions(const std::any& /* object */) const
  {
    return Actions();
  }
  virtual Editors getEditors(const std::any& /* object */) const
  {
    return Editors();
  }

  virtual Selected makeSelected(const std::any& object) const = 0;

  virtual bool lessThan(const std::any& l, const std::any& r) const = 0;

  static Editor makeEditor(const EditorCallback& func,
                           const std::vector<EditorOption>& options)
  {
    return {func, options};
  }
  static Editor makeEditor(const EditorCallback& func)
  {
    return makeEditor(func, {});
  }

  // The caller (Selected and Renderers) will pre-configure the Painter's pen
  // and brush before calling.
  virtual void highlight(const std::any& object, Painter& painter) const = 0;
  virtual bool isSlowHighlight(const std::any& /* object */) const
  {
    return false;
  }

  static std::string convertUnits(double value,
                                  bool area = false,
                                  int digits = 3);
};

class PropertyTable
{
 public:
  PropertyTable(int rows, int columns)
  {
    row_header_.resize(rows);
    column_header_.resize(columns);
    data_.resize(rows);
    for (auto& row : data_) {
      row.resize(columns);
    }
  };

  void setRowHeader(int row, const std::string& header)
  {
    row_header_.at(row) = header;
  }
  const std::vector<std::string>& getRowHeaders() const { return row_header_; }

  void setColumnHeader(int column, const std::string& header)
  {
    column_header_.at(column) = header;
  }
  const std::vector<std::string>& getColumnHeaders() const
  {
    return column_header_;
  }

  void setData(int row, int column, const std::string& value)
  {
    data_.at(row).at(column) = value;
  }
  const std::vector<std::vector<std::string>>& getData() const { return data_; }

  bool empty() const
  {
    for (const auto& row : data_) {
      for (const auto& item : row) {
        if (!item.empty()) {
          return false;
        }
      }
    }
    return true;
  }

 private:
  std::vector<std::string> column_header_;
  std::vector<std::string> row_header_;
  std::vector<std::vector<std::string>> data_;
};

// An object selected in the gui.  The object is stored as a
// std::any to allow any client objects to be stored.  The descriptor
// is the API for the object as described above.  This doesn't
// require the client object to use inheritance from an interface.
class Selected
{
 public:
  // Null case
  Selected() = default;

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
  const std::any& getObject() const { return object_; }

  // If the select_flag is false, the drawing will happen in highlight mode.
  // Highlight shapes are persistent which will not get removed from
  // highlightSet, if the user clicks on layout view as in case of selectionSet
  void highlight(Painter& painter,
                 const Painter::Color& pen = Painter::kPersistHighlight,
                 int pen_width = 0,
                 const Painter::Color& brush = Painter::kTransparent,
                 const Painter::Brush& brush_style
                 = Painter::Brush::kSolid) const;
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
  const Descriptor* descriptor_{nullptr};
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
  using DisplayControlCallback = std::function<void()>;
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
      try {
        value = std::get<T>(settings.at(key));
        // NOLINTNEXTLINE(bugprone-empty-catch)
      } catch (const std::bad_variant_access&) {
        // Stay with current value
      }
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
  static const unsigned char kSpectrum[256][3];
  double scale_;
  static constexpr int kLegendColorIncrement = 2;
};

class Legend
{
 public:
  virtual ~Legend() = default;

  virtual void draw(Painter& painter) const = 0;

 protected:
  Legend() = default;
};

// A legend for a linear spectrum of colors
// The colors are specified as continuous from low to high
// For example, a heat map legend
class LinearLegend : public Legend
{
 public:
  LinearLegend(const std::vector<Painter::Color>& colors);

  // Set the legend key as a vector of (color, text) pairs
  void setLegendKey(
      const std::vector<std::pair<Painter::Color, std::string>>& legend_key);

  void draw(Painter& painter) const override;

 private:
  std::vector<Painter::Color> colors_;
  std::vector<std::pair<Painter::Color, std::string>> legend_key_;
};

// A legend for discrete colors
// Each color is associated with a text label
// For example, a timing path legend
class DiscreteLegend : public Legend
{
 public:
  DiscreteLegend() = default;

  // Add a (color, text) entry to the legend
  void addLegendKey(const Painter::Color& color, const std::string& text);

  void draw(Painter& painter) const override;

 private:
  std::vector<std::pair<Painter::Color, std::string>> color_key_;
};

// A chart with a single X axis and potentially multiple Y axes
class Chart
{
 public:
  virtual ~Chart() = default;

  // printf-style format string
  virtual void setXAxisFormat(const std::string& format) = 0;
  // printf-style format.  An empty string is a no-op placeholder
  virtual void setYAxisFormats(const std::vector<std::string>& formats) = 0;
  virtual void setYAxisMin(const std::vector<std::optional<double>>& mins) = 0;
  // One y per series.  The order matches y_labels in addChart
  virtual void addPoint(double x, const std::vector<double>& ys) = 0;
  virtual void clearPoints() = 0;

  virtual void addVerticalMarker(double x, const Painter::Color& color) = 0;

 protected:
  Chart() = default;
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

  // Return the selected set
  const SelectionSet& selection();

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

  std::string addLabel(int x,
                       int y,
                       const std::string& text,
                       std::optional<Painter::Color> color = {},
                       std::optional<int> size = {},
                       std::optional<Painter::Anchor> anchor = {},
                       const std::optional<std::string>& name = {});
  void deleteLabel(const std::string& name);
  void clearLabels();

  std::string addRuler(int x0,
                       int y0,
                       int x1,
                       int y1,
                       const std::string& label = "",
                       const std::string& name = "",
                       bool euclidian = true);
  void deleteRuler(const std::string& name);
  void clearRulers();

  void clearSelections();
  void clearHighlights(int highlight_group = 0);

  int select(const std::string& type,
             const std::string& name_filter = "",
             const std::string& attribute = "",
             const std::any& value = "",
             bool filter_case_sensitive = true,
             int highlight_group = -1);

  // Zoom to the given rectangle
  void zoomTo(const odb::Rect& rect_dbu);
  // zoom to the specified point
  void zoomTo(const odb::Point& focus, int diameter);
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
  void selectClockviewerClock(const std::string& clock_name,
                              std::optional<int> depth);

  // Save histogram view
  void saveHistogramImage(const std::string& filename,
                          const std::string& mode,
                          int width_px = 0,
                          int height_px = 0);

  void showWorstTimingPath(bool setup);
  void clearTimingPath();

  // modify display controls
  void setDisplayControlsVisible(const std::string& name, bool value);
  void setDisplayControlsSelectable(const std::string& name, bool value);
  void setDisplayControlsColor(const std::string& name,
                               const Painter::Color& color);
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

  Chart* addChart(const std::string& name,
                  const std::string& x_label,
                  const std::vector<std::string>& y_labels);

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

  using Term = std::variant<odb::dbITerm*, odb::dbBTerm*>;
  void timingCone(Term term, bool fanin, bool fanout);
  void timingPathsThrough(const std::set<Term>& terms);

  // open markers
  void selectMarkers(odb::dbMarkerCategory* markers);

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
  void showGui(const std::string& cmds = "",
               bool interactive = true,
               bool load_settings = true);
  void minimize();
  void unminimize();

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

  // GIF API
  // Start returns the key for use by add and end.  This allows multiple
  // gifs to be open at once.
  int gifStart(const std::string& filename);
  void gifAddFrame(std::optional<int> key,
                   const odb::Rect& region = odb::Rect(),
                   int width_px = 0,
                   double dbu_per_pixel = 0,
                   std::optional<int> delay = {});
  void gifEnd(std::optional<int> key);

  void setHeatMapSetting(const std::string& name,
                         const std::string& option,
                         const Renderer::Setting& value);
  Renderer::Setting getHeatMapSetting(const std::string& name,
                                      const std::string& option);
  void dumpHeatMap(const std::string& name, const std::string& file);

  void setMainWindowTitle(const std::string& title);
  std::string getMainWindowTitle();

  void selectHelp(const std::string& item);
  void selectChart(const std::string& name);
  void updateTimingReport();

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
  void init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger);

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

  // There are RTTI implementation differences between libstdc++ and libc++,
  // where the latter seems to generate multiple typeids for classes including
  // but not limited to sta::Instance* in different compile units. We have been
  // unable to remedy this.
  //
  // These classes are a workaround such that unless __GLIBCXX__ is set, hashing
  // and comparing are done on the type's name instead, which adds a negligible
  // performance penalty but has the distinct advantage of not crashing when an
  // Instance is clicked in the GUI.
  //
  // In the event the RTTI issue is ever resolved, the following two structs may
  // be removed.
  struct TypeInfoHasher
  {
    std::size_t operator()(const std::type_index& x) const;
  };
  struct TypeInfoComparator
  {
    bool operator()(const std::type_index& a, const std::type_index& b) const;
  };

  // Maps types to descriptors
  std::unordered_map<std::type_index,
                     std::unique_ptr<const Descriptor>,
                     TypeInfoHasher,
                     TypeInfoComparator>
      descriptors_;
  // Heatmaps
  std::set<HeatMapDataSource*> heat_maps_;

  // tcl commands needed to restore state
  std::vector<std::string> tcl_state_commands_;

  std::set<Renderer*> renderers_;

  std::unique_ptr<PinDensityDataSource> pin_density_heat_map_;
  std::unique_ptr<PlacementDensityDataSource> placement_density_heat_map_;
  std::unique_ptr<PowerDensityDataSource> power_density_heat_map_;

  std::vector<std::unique_ptr<GIF>> gifs_;
  static constexpr int kDefaultGifDelay = 250;

  std::string main_window_title_ = "OpenROAD";
};

// The main entry point
int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script = "",
             bool interactive = true,
             bool load_settings = true,
             bool minimize = false);

}  // namespace gui
