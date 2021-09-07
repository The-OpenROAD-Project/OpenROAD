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

#include <string.h>

#include <any>
#include <array>
#include <functional>
#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <typeinfo>
#include <variant>

#include "odb/db.h"

namespace gui {
class Painter;
class Selected;
class Options;

// This interface allows the GUI to interact with selected objects of
// types it knows nothing about.  It can just ask the descriptor to
// give it information about the foreign object (eg attributes like
// name).
class Descriptor
{
 public:
  virtual std::string getName(std::any object) const = 0;
  virtual std::string getTypeName(std::any object) const = 0;
  virtual bool getBBox(std::any object, odb::Rect& bbox) const = 0;

  virtual bool isInst(std::any /* object */) const { return false; }
  virtual bool isNet(std::any /* object */) const { return false; }

  // A property is a name and a value.
  struct Property {
    std::string name;
    std::any value;
  };
  using Properties = std::vector<Property>;

  // An action is a name and a callback function, the function should return
  // the next object to select (when deleting the object just return Selected())
  using ActionCallback = std::function<Selected(void)>;
  struct Action {
    std::string name;
    ActionCallback callback;
  };
  using Actions = std::vector<Action>;

  // An editor is a callback function and a list of possible values (this can be empty),
  // the name of the editor should match the property it modifies
  // the callback should return true if the edit was successful, otherwise false
  struct EditorOption {
    std::string name;
    std::any value;
  };
  using EditorCallback = std::function<bool(std::any)>;
  struct Editor {
    EditorCallback callback;
    std::vector<EditorOption> options;
  };
  using Editors = std::map<std::string, Editor>;

  virtual Properties getProperties(std::any object) const = 0;
  virtual Actions getActions(std::any /* object */) const { return Actions(); }
  virtual Editors getEditors(std::any /* object */) const { return Editors(); }

  virtual Selected makeSelected(std::any object,
                                void* additional_data) const = 0;

  virtual bool lessThan(std::any l, std::any r) const = 0;

  std::any getProperty(std::any object, const std::string& name) const
  {
    for (auto& [prop, value] : getProperties(object)) {
      if (prop == name) {
        return value;
      }
    }
    return std::any();
  }

  static const Editor makeEditor(const EditorCallback& func, const std::vector<EditorOption>& options)
  {
    return {func, options};
  }
  static const Editor makeEditor(const EditorCallback& func) { return makeEditor(func, {}); }

protected:
  // The caller (Selected) will pre-configure the Painter's pen
  // and brush before calling.
  virtual void highlight(std::any object,
                         Painter& painter,
                         void* additional_data = nullptr) const = 0;

  friend class Selected;
};

// An object selected in the gui.  The object is stored as a
// std::any to allow any client objects to be stored.  The descriptor
// is the API for the object as described above.  This doesn't
// require the client object to use inheritance from an interface.
class Selected
{
 public:
  // Null case
  Selected() : additional_data_(nullptr), descriptor_(nullptr) {}

  Selected(std::any object,
           const Descriptor* descriptor,
           void* additional_data = nullptr)
      : object_(object),
        additional_data_(additional_data),
        descriptor_(descriptor)
  {
  }

  std::string getName() const { return descriptor_->getName(object_); }
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
                 bool select_flag = true,
                 int highlight_group = 0) const;

  Descriptor::Properties getProperties() const
  {
    return descriptor_->getProperties(object_);
  }

  std::any getProperty(const std::string& name) const
  {
    return descriptor_->getProperty(object_, name);
  }

  Descriptor::Actions getActions() const
  {
    return descriptor_->getActions(object_);
  }

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

 private:
  std::any object_;
  void* additional_data_;  // Will only be required for highlighting input nets,
                           // in which case it will store the input instTerm
  const Descriptor* descriptor_;
};

// A collection of selected objects
using SelectionSet = std::set<Selected>;
using HighlightSet = std::array<SelectionSet, 8>;  // Only 8 Discrete Highlight
                                                   // Color is supported for now

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

    int r;
    int g;
    int b;
    int a;
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
      Painter::green,
      Painter::yellow,
      Painter::cyan,
      Painter::magenta,
      Painter::red,
      Painter::dark_green,
      Painter::dark_magenta,
      Painter::blue};

  // The color to highlight in
  static inline const Color highlight = yellow;
  static inline const Color persistHighlight = yellow;

  Painter(Options* options, double pixels_per_dbu) : options_(options), pixels_per_dbu_(pixels_per_dbu) {}
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
  virtual void setBrush(const Color& color) = 0;

  // Draw a geom shape as a polygon with coordinates in DBU with the current
  // pen/brush
  virtual void drawGeomShape(const odb::GeomShape* shape) = 0;

  // Draw a rect with coordinates in DBU with the current pen/brush; draws a
  // round rect if roundX > 0 or roundY > 0
  virtual void drawRect(const odb::Rect& rect, int roundX = 0, int roundY = 0)
      = 0;

  // Draw a line with coordinates in DBU with the current pen
  virtual void drawLine(const odb::Point& p1, const odb::Point& p2) = 0;

  virtual void drawCircle(int x, int y, int r) = 0;

  virtual void drawPolygon(const std::vector<odb::Point>& points) = 0;

  enum Anchor {
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
  virtual void drawString(int x, int y, Anchor anchor, const std::string& s) = 0;

  virtual void drawRuler(int x0, int y0, int x1, int y1, const std::string& label = "") = 0;

  // Draw a line with coordinates in DBU with the current pen
  void drawLine(int xl, int yl, int xh, int yh)
  {
    drawLine(odb::Point(xl, yl), odb::Point(xh, yh));
  }

  virtual void setTransparentBrush() = 0;
  virtual void setHashedBrush(const Color& color) = 0;

  inline double getPixelsPerDBU() { return pixels_per_dbu_; }
  inline Options* getOptions() { return options_; }

 private:
  Options* options_;
  double pixels_per_dbu_;
};

// This is an interface for classes that wish to be called to render
// on the layout.  Clients will subclass and register their instances with
// the Gui singleton.
class Renderer
{
 public:
  Renderer() : repaint_required_(true)
  {
  }

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
                              const odb::Point& /* point */)
  {
    return SelectionSet();
  }

  // If this return true, the renderer needs the layout to
  // be redrawn because it's output has changed. Otherwise, it will
  // return false.
  virtual bool isRepaintRequired()
  {
    if (repaint_required_) {
      // reset the flag
      repaint_required_ = false;
      return true;
    }

    return false;
  }

  // Sets the flag it indicate that a repaint is needed by this renderer
  void setRepaintRequired()
  {
    repaint_required_ = true;
  }

  // Clears the flag it indicate that a repaint is needed by this renderer
  void clearRepaintRequired()
  {
    repaint_required_ = false;
  }

 private:
  bool repaint_required_;
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
  Selected makeSelected(std::any object, void* additional_data = nullptr);

  // Set the current selected object in the gui.
  void setSelected(Selected selection);

  // Add a net to the selection set
  void addSelectedNet(const char* name);

  // Add nets matching the pattern to the selection set
  void addSelectedNets(const char* pattern,
                       bool match_case = true,
                       bool match_reg_ex = false,
                       bool add_to_highlight_set = false,
                       int highlight_group = 0);

  // Add an instance to the selection set
  void addSelectedInst(const char* name);

  // Add instances matching the pattern to the selection set
  void addSelectedInsts(const char* pattern,
                        bool match_case = true,
                        bool match_regE_ex = true,
                        bool add_to_highlight_set = false,
                        int highlight_group = 0);
  // check if any object(inst/net) is present in sect/highlight set
  bool anyObjectInSet(bool selection_set, odb::dbObjectType obj_type) const;

  void selectHighlightConnectedInsts(bool select_flag, int highlight_group = 0);
  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group = 0);

  void addInstToHighlightSet(const char* name, int highlight_group = 0);
  void addNetToHighlightSet(const char* name, int highlight_group = 0);

  std::string addRuler(int x0, int y0, int x1, int y1, const std::string& label = "", const std::string& name = "");
  void deleteRuler(const std::string& name);

  void clearSelections();
  void clearHighlights(int highlight_group = 0);
  void clearRulers();

  // Zoom to the given rectangle
  void zoomTo(const odb::Rect& rect_dbu);
  void zoomIn();
  void zoomIn(const odb::Point& focus_dbu);
  void zoomOut();
  void zoomOut(const odb::Point& focus_dbu);
  void centerAt(const odb::Point& focus_dbu);
  void setResolution(double pixels_per_dbu);

  // Save layout to an image file
  void saveImage(const std::string& filename, const odb::Rect& region = odb::Rect());

  // modify display controls
  void setDisplayControlsVisible(const std::string& name, bool value);
  void setDisplayControlsSelectable(const std::string& name, bool value);

  // show/hide widgets
  void showWidget(const std::string& name, bool show);

  // adding custom buttons to toolbar
  const std::string addToolbarButton(const std::string& name,
                                     const std::string& text,
                                     const std::string& script,
                                     bool echo);
  void removeToolbarButton(const std::string& name);

  // request for user input
  const std::string requestUserInput(const std::string& title, const std::string& question);

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

  // Add a custom visibilty control to the 'Display Control' panel.
  // Useful for debug renderers to control their display.
  void addCustomVisibilityControl(const std::string& name,
                                  bool initially_visible = false);

  // Get the visibility for a custom control in the 'Display Control' panel.
  bool checkCustomVisibilityControl(const std::string& name);

  const std::set<Renderer*>& renderers() { return renderers_; }

  // The GUI listening for callbacks on read_def or read_db but
  // if the design is created by direct opendb calls then this
  // will trigger the GUI to load it.
  void load_design();

  void fit();

  template <class T>
  void registerDescriptor(const Descriptor* descriptor)
  {
    registerDescriptor(typeid(T), descriptor);
  }

  // Will return nullptr if openroad was invoked without -gui
  static Gui* get();

 private:
  Gui() = default;
  void registerDescriptor(const std::type_info& type,
                          const Descriptor* descriptor);

  std::set<Renderer*> renderers_;
  static Gui* singleton_;
};

// The main entry point
int startGui(int argc, char* argv[]);

}  // namespace gui
