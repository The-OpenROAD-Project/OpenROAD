// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2020-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <QFrame>
#include <QLine>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMutex>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QShortcut>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
#include <QWidget>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "gui/gui.h"
#include "label.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "options.h"
#include "renderThread.h"
#include "search.h"

namespace utl {
class Logger;
}

namespace gui {

class GuiPainter;
class LayoutScroll;
class Ruler;
class ScriptWidget;
class LayoutViewer;

// This class draws the layout.  It supports:
//   * zoom in/out with ctrl-mousewheel
//   * rubber band zoom with right mouse button
//   * fit with 'F' key
// The display follows the display options for visibility.
//
// This object resizes with zooming but only the visible
// portion of this widget is ever drawn.
class LayoutViewer : public QWidget
{
  Q_OBJECT

 public:
  enum ContextMenuActions
  {
    kSelectConnectedInstAct,
    kSelectOutputNetsAct,
    kSelectInputNetsAct,
    kSelectAllNetsAct,
    kSelectAllBufferTreesAct,

    kHighlightConnectedInstAct,
    kHighlightOutputNetsAct,
    kHighlightInputNetsAct,
    kHighlightAllNetsAct,
    kHighlightAllBufferTreesAct0,
    kHighlightAllBufferTreesAct1,
    kHighlightAllBufferTreesAct2,
    kHighlightAllBufferTreesAct3,
    kHighlightAllBufferTreesAct4,
    kHighlightAllBufferTreesAct5,
    kHighlightAllBufferTreesAct6,
    kHighlightAllBufferTreesAct7,

    kViewZoominAct,
    kViewZoomoutAct,
    kViewZoomfitAct,

    kSaveWholeImageAct,
    kSaveVisibleImageAct,

    kClearSelectionsAct,
    kClearHighlightsAct,
    kClearRulersAct,
    kClearLabelsAct,
    kClearFocusAct,
    kClearGuidesAct,
    kClearNetTracksAct,
    kClearAllAct
  };

  struct ModuleSettings
  {
    QColor color;
    QColor user_color;
    QColor orig_color;
    bool visible;
  };

  // makeSelected is so that we don't have to pass in the whole
  // MainWindow just to get access to one method.  Communication
  // should happen through signals & slots in all other cases.
  LayoutViewer(Options* options,
               ScriptWidget* output_widget,
               const SelectionSet& selected,
               const HighlightSet& highlighted,
               const Rulers& rulers,
               const Labels& labels,
               const std::map<odb::dbModule*, ModuleSettings>& module_settings,
               const std::set<odb::dbNet*>& focus_nets,
               const std::set<odb::dbNet*>& route_guides,
               const std::set<odb::dbNet*>& net_tracks,
               Gui* gui,
               const std::function<bool()>& using_dbu,
               const std::function<bool()>& show_ruler_as_euclidian,
               const std::function<bool()>& show_db_view,
               QWidget* parent = nullptr);

  odb::dbBlock* getBlock() const { return chip_->getBlock(); }
  odb::dbChip* getChip() const { return chip_; }
  std::map<odb::dbChipInst*, odb::dbChip*> getChips() const;
  void setLogger(utl::Logger* logger);
  qreal getPixelsPerDBU() { return pixels_per_dbu_; }
  void setScroller(LayoutScroll* scroller);

  void restoreTclCommands(std::vector<std::string>& cmds);

  // conversion functions
  odb::Rect screenToDBU(const QRectF& rect) const;
  odb::Point screenToDBU(const QPointF& point) const;
  QRectF dbuToScreen(const odb::Rect& dbu_rect) const;
  QPointF dbuToScreen(const odb::Point& dbu_point) const;

  // save image of the layout
  void saveImage(const QString& filepath,
                 const odb::Rect& region = odb::Rect(),
                 int width_px = 0,
                 double dbu_per_pixel = 0);
  QImage createImage(const odb::Rect& region = odb::Rect(),
                     int width_px = 0,
                     double dbu_per_pixel = 0);

  // From QWidget
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;

  odb::Rect getVisibleBounds()
  {
    return screenToDBU(visibleRegion().boundingRect());
  }

  bool isCursorInsideViewport();
  void updateCursorCoordinates();

  // gets the size of the diameter of the biggest circle that fits the view
  int getVisibleDiameter();

 signals:
  // indicates the current location of the mouse
  void location(int x, int y);

  // indicates a new object has been selected
  void selected(const Selected& selected, bool show_connectivity = false);

  // add additional object to selected set
  void addSelected(const Selected& selected);
  void addSelected(const SelectionSet& selected);

  // add new ruler
  void addRuler(int x0, int y0, int x1, int y1);

  void focusNetsChanged();

  void viewUpdated();

 public slots:
  // zoom in the layout, keeping the current center_
  void zoomIn();

  // zoom in and change center to the focus point
  // do_delta_focus indicates (if true) that the center of the layout should
  // zoom in around the focus instead of making it the new center. This is used
  // when scrolling with the mouse to keep the mouse point steady in the layout
  void zoomIn(const odb::Point& focus, bool do_delta_focus = false);

  // zoom out the layout, keeping the current center_
  void zoomOut();

  // zoom out and change center to the focus point
  // do_delta_focus indicates (if true) that the center of the layout should
  // zoom in around the focus instead of making it the new center. This is used
  // when scrolling with the mouse to keep the mouse point steady in the layout
  void zoomOut(const odb::Point& focus, bool do_delta_focus = false);

  // zoom to the specified rect
  void zoomTo(const odb::Rect& rect_dbu);

  // zoom to the specified point
  void zoomTo(const odb::Point& focus, int diameter);

  // indicates a chip has been loaded
  void chipLoaded(odb::dbChip* chip);

  // fit the whole design in the window
  void fit();

  // center layout at the focus point
  void centerAt(const odb::Point& focus);

  // indicate that the center has changed, due to the scrollarea changing
  void updateCenter(int dx, int dy);

  // set the layout resolution
  void setResolution(qreal pixels_per_dbu);

  // update the fit resolution (the maximum pixels_per_dbu without scroll bars)
  void viewportUpdated();

  // signals that the cache should be flushed and a full repaint should occur.
  void fullRepaint();

  odb::Point getVisibleCenter();

  void selectHighlightConnectedInst(bool select_flag);
  void selectHighlightConnectedNets(bool select_flag, bool output, bool input);
  void selectHighlightConnectedBufferTrees(bool select_flag,
                                           int highlight_group = 0);

  void updateContextMenuItems();
  void showLayoutCustomMenu(QPoint pos);

  void startRulerBuild();
  void cancelRulerBuild();

  int selectArea(const odb::Rect& area, bool append);

  void selection(const Selected& selection);
  void selectionFocus(const Selected& focus);
  void selectionAnimation(const Selected& selection,
                          int repeats = kAnimationRepeats,
                          int update_interval = kAnimationInterval);
  void selectionAnimation(int repeats = kAnimationRepeats,
                          int update_interval = kAnimationInterval)
  {
    selectionAnimation(inspector_selection_, repeats, update_interval);
  }

  void exit();

  void resetCache();

  void commandAboutToExecute();
  void commandFinishedExecuting();
  void executionPaused();

 private slots:
  void setChip(odb::dbChip* chip);
  void updatePixmap(const QImage& image, const QRect& bounds);
  void handleLoadingIndication();

 private:
  struct Boxes
  {
    std::vector<QPolygon> obs;
    std::map<odb::dbMTerm*, std::vector<QPolygon>> mterms;
  };

  using LayerBoxes = std::map<odb::dbTechLayer*, Boxes>;
  using CellBoxes = std::map<odb::dbMaster*, LayerBoxes>;

  void boxesByLayer(odb::dbMaster* master, LayerBoxes& boxes);
  const Boxes* boxesByLayer(odb::dbMaster* master, odb::dbTechLayer* layer);
  void setPixelsPerDBU(qreal pixels_per_dbu);
  void selectAt(odb::Rect region_dbu, std::vector<Selected>& selection);
  SelectionSet selectAt(odb::Rect region_dbu);
  void selectViaShapesAt(odb::dbBlock* block,
                         odb::dbTechLayer* cut_layer,
                         odb::dbTechLayer* select_layer,
                         const odb::Rect& region,
                         int shape_limit,
                         std::vector<Selected>& selections);
  Selected selectAtPoint(const odb::Point& pt_dbu);

  void zoom(const odb::Point& focus, qreal factor, bool do_delta_focus);

  qreal computePixelsPerDBU(const QSize& size, const odb::Rect& dbu_rect);
  odb::Rect getBounds() const;
  odb::Rect getPaddedRect(const odb::Rect& rect,
                          double factor = defaultZoomMargin);

  bool hasDesign() const;
  int getDbuPerMicron() const;

  int fineViewableResolution() const;
  int nominalViewableResolution() const;
  int coarseViewableResolution() const;
  int instanceSizeLimit() const;
  int shapeSizeLimit() const;
  int highlightSizeLimit() const;

  std::vector<std::tuple<odb::dbObject*, odb::Rect, int>> getRowRects(
      odb::dbBlock* block,
      const odb::Rect& bounds);

  void generateCutLayerMaximumSizes();

  void addMenuAndActions();

  using Edge = std::pair<odb::Point, odb::Point>;
  // search for nearest edge to point
  std::pair<Edge, bool> searchNearestEdge(odb::Point pt,
                                          bool horizontal,
                                          bool vertical);
  void searchNearestViaEdge(
      odb::dbBlock* block,
      odb::dbTechLayer* cut_layer,
      odb::dbTechLayer* search_layer,
      const odb::Rect& search_line,
      int shape_limit,
      const std::function<void(const odb::Rect& rect)>& check_rect);
  int edgeToPointDistance(const odb::Point& pt, const Edge& edge) const;
  bool compareEdges(const Edge& lhs, const Edge& rhs) const;

  odb::Point findNextSnapPoint(const odb::Point& end_pt, bool snap = true);
  odb::Point findNextSnapPoint(const odb::Point& end_pt,
                               const odb::Point& start_pt,
                               bool snap = true);

  odb::Point findNextRulerPoint(const odb::Point& mouse);

  void updateScaleAndCentering(const QSize& new_size);

  bool isNetVisible(odb::dbNet* net);

  void drawScaleBar(QPainter* painter, const QRect& rect);
  void drawLoadingIndicator(QPainter* painter, const QRect& bounds);
  QRect computeIndicatorBackground(QPainter* painter,
                                   const QRect& bounds) const;
  void setLoadingState();

  void populateModuleColors();

  odb::dbChip* chip_;
  Options* options_;
  ScriptWidget* output_widget_;
  const SelectionSet& selected_;
  const HighlightSet& highlighted_;
  const Rulers& rulers_;
  const Labels& labels_;
  LayoutScroll* scroller_;

  // Use to avoid painting while a command is executing unless paused.
  bool command_executing_ = false;
  bool paused_ = false;

  // holds the current resolution for drawing the layout (units are pixels /
  // dbu)
  qreal pixels_per_dbu_;

  // holds the resolution for drawing the layout where the whole layout fits in
  // the window (units are pixels / dbu)
  qreal fit_pixels_per_dbu_;
  int min_depth_;
  int max_depth_;
  Search search_;
  CellBoxes cell_boxes_;
  QRect rubber_band_;  // screen coordinates
  QPoint mouse_press_pos_;
  QPoint mouse_move_pos_;
  bool rubber_band_showing_;
  bool is_view_dragging_;
  Gui* gui_;

  std::function<bool()> using_dbu_;
  std::function<bool()> show_ruler_as_euclidian_;
  std::function<bool()> show_db_view_;

  const std::map<odb::dbModule*, ModuleSettings>& modules_;

  bool building_ruler_;
  std::unique_ptr<odb::Point> ruler_start_;

  bool snap_edge_showing_;
  Edge snap_edge_;

  // keeps track of inspector selection and focus items
  Selected inspector_selection_;
  Selected focus_;
  // Timer used to handle blinking objects in the layout
  struct AnimatedSelected
  {
    const Selected selection;
    int state_count;
    const int max_state_count;
    const int state_modulo;
    std::unique_ptr<QTimer> timer;
  };
  std::unique_ptr<AnimatedSelected> animate_selection_;

  bool repaint_requested_;

  utl::Logger* logger_;

  QMenu* layout_context_menu_;
  QMenu* highlight_color_menu_;
  QMap<ContextMenuActions, QAction*> menu_actions_;

  // shift required when drawing the layout to center the layout in the window
  // (units: pixels)
  QPoint centering_shift_;

  // The center point of the layout visible in the window (in dbu).
  // this is needed to keep track of the center without a
  // dependence on the resolution
  odb::Point center_;

  // Cache of the maximum cut size per layer (units: dbu).
  // Used to determine when cuts are too small to be seen and should not be
  // drawn.
  std::map<odb::dbTechLayer*, int> cut_maximum_size_;

  const std::set<odb::dbNet*>& focus_nets_;
  const std::set<odb::dbNet*>& route_guides_;
  const std::set<odb::dbNet*>& net_tracks_;

  RenderThread viewer_thread_;
  QPixmap draw_pixmap_;
  QRect draw_pixmap_bounds_;
  QTimer loading_timer_;
  QTimer repaint_timer_;
  std::string loading_indicator_;

  static constexpr qreal kZoomScaleFactor = 1.2;
  static constexpr double defaultZoomMargin = 0.05;

  // parameters used to animate the selection of objects
  static constexpr int kAnimationRepeats = 6;
  static constexpr int kAnimationInterval = 300;

  friend class RenderThread;
};

// The LayoutViewer widget can become quite large as you zoom
// in so it is stored in a scroll area.
class LayoutScroll : public QScrollArea
{
  Q_OBJECT
 public:
  LayoutScroll(LayoutViewer* viewer,
               const std::function<bool()>& default_mouse_wheel_zoom,
               const std::function<int()>& arrow_keys_scroll_step,
               QWidget* parent = nullptr);
  bool isScrollingWithCursor();
 signals:
  // indicates that the viewport (visible area of the layout) has changed
  void viewportChanged();

  // indicates how far the viewport of the layout has shifted due to a resize
  // event or by the user manipulating the scrollbars
  void centerChanged(int dx, int dy);

 protected:
  void resizeEvent(QResizeEvent* event) override;
  void scrollContentsBy(int dx, int dy) override;
  void wheelEvent(QWheelEvent* event) override;
  bool eventFilter(QObject* object, QEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  std::function<bool()> default_mouse_wheel_zoom_;
  std::function<int()> arrow_keys_scroll_step_;
  LayoutViewer* viewer_;

  bool scrolling_with_cursor_;
};

}  // namespace gui
