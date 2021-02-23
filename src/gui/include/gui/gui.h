///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2020, Liberty Software LLC
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

#include <tcl.h>

#include <array>
#include <initializer_list>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <string.h>

#include "opendb/db.h"

namespace gui {
class Painter;

// This interface allows the GUI to interact with selected objects of
// types it knows nothing about.  It can just ask the descriptor to
// give it information about the foreign object (eg attributes like
// name).
class Descriptor
{
 public:
  virtual std::string getName(void* object) const = 0;
  virtual std::string getLocation(void* object) const = 0;
  virtual bool getBBox(void* object, odb::Rect& bbox) const = 0;

  // If the select_flag is false, the drawing will happen in highlight mode.
  // Highlight shapes are persistent which will not get removed from
  // highlightSet, if the user clicks on layout view as in case of selectionSet
  virtual void highlight(void* object,
                         Painter& painter,
                         bool select_flag = true,
                         int highlight_group = 0,
                         void* additional_data = nullptr) const = 0;

  virtual bool isInst(void* object) const = 0;
  virtual bool isNet(void* object) const = 0;
};

// An implementation of the Descriptor interface for OpenDB
// objects for client convenience.
class OpenDbDescriptor : public Descriptor
{
 public:
  std::string getName(void* object) const override;
  std::string getLocation(void* object) const override;
  bool getBBox(void* object, odb::Rect& bbox) const override;

  void highlight(void* object,
                 Painter& painter,
                 bool select_flag,
                 int highlight_group,
                 void* additional_data) const override;

  bool isInst(void* object) const override;
  bool isNet(void* object) const override;

  static OpenDbDescriptor* get();

 private:
  OpenDbDescriptor() = default;
  static OpenDbDescriptor* singleton_;
};

// An object selected in the gui.  The objects is stored as a
// void* to allow any client objects to be stored.  The descriptor
// is the API for the object as described above.  This doesn't
// require the client object to use inheritance from an interface.
class Selected
{
 public:
  // Null case
  Selected() : object_(nullptr), additional_data_(nullptr), descriptor_(nullptr)
  {
  }

  Selected(void* object,
           Descriptor* descriptor,
           void* additional_data = nullptr)
      : object_(object),
        additional_data_(additional_data),
        descriptor_(descriptor)
  {
  }

  Selected(odb::dbObject* object, odb::dbObject* sink_object = nullptr)
      : object_(object),
        additional_data_(sink_object),
        descriptor_(OpenDbDescriptor::get())
  {
  }

  std::string getName() const { return descriptor_->getName(object_); }
  std::string getLocation() const { return descriptor_->getLocation(object_); }
  bool getBBox(odb::Rect& bbox) const
  {
    return descriptor_->getBBox(object_, bbox);
  }

  bool isInst() const { return descriptor_->isInst(object_); }
  bool isNet() const { return descriptor_->isNet(object_); }
  void* getObject() const { return object_; }

  void highlight(Painter& painter,
                 bool select_flag = true,
                 int highlight_group = 0) const
  {
    return descriptor_->highlight(
        object_, painter, select_flag, highlight_group, additional_data_);
  }

  operator bool() const { return object_ != nullptr; }

  // For SelectionSet
  friend bool operator<(const Selected& l, const Selected& r)
  {
    return l.object_ < r.object_;
  }

 private:
  void* object_;
  void* additional_data_;  // Will only be required for highlighting input nets,
                           // in which case it will store the input instTerm
  Descriptor* descriptor_;
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

  virtual ~Painter() = default;

  // Set the pen to whatever the user has chosen for this layer
  virtual void setPen(odb::dbTechLayer* layer, bool cosmetic = false) = 0;

  // Set the pen to an RGBA value
  virtual void setPen(const Color& color, bool cosmetic = false, int width=1) = 0;

  virtual void setPenWidth(int width) = 0;
  // Set the brush to whatever the user has chosen for this layer
  // The alpha value may be overridden
  virtual void setBrush(odb::dbTechLayer* layer, int alpha = -1) = 0;

  // Set the brush to whatever the user has chosen for this layer
  virtual void setBrush(const Color& color) = 0;

  // Draw a geom shape as a polygon with coordinates in DBU with the current
  // pen/brush
  virtual void drawGeomShape(const odb::GeomShape* shape) = 0;

  // Draw a rect with coordinates in DBU with the current pen/brush; draws a round rect if roundX > 0 or roundY > 0
  virtual void drawRect(const odb::Rect& rect, int roundX=0, int roundY=0) = 0;
  
  // Draw a line with coordinates in DBU with the current pen
  virtual void drawLine(const odb::Point& p1, const odb::Point& p2) = 0;
 
  virtual void drawCircle(int x, int y, int r) = 0;
  
  virtual void drawString(int x, int y, int of, std::string& s) = 0;
  
  // Draw a line with coordinates in DBU with the current pen
  void drawLine(int xl, int yl, int xh, int yh)
  {
    drawLine(odb::Point(xl, yl), odb::Point(xh, yh));
  }
  
  virtual void setTransparentBrush() = 0;
  
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
  virtual Selected select(odb::dbTechLayer* /* layer */,
                          const odb::Point& /* point */)
  {
    return Selected();
  }
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

  void clearSelections();
  void clearHighlights(int highlight_group = 0);

  // Zoom to the given rectangle
  void zoomTo(const odb::Rect& rect_dbu);

  // Force an immediate redraw.
  void redraw();

  // Waits for the user to click continue before returning
  // Draw events are processed while paused.
  void pause();

  // Show a message in the status bar
  void status(const std::string& message);

  const std::set<Renderer*>& renderers() { return renderers_; }

  // Will return nullptr if openroad was invoked without -gui
  static Gui* get();

  bool isGridGraphVisible();
  bool areRouteGuidesVisible();
  bool areRoutingObjsVisible();
  
 private:
  Gui() = default;

  std::set<Renderer*> renderers_;
  static Gui* singleton_;
};

// The main entry point
int startGui(int argc, char* argv[]);

}  // namespace gui