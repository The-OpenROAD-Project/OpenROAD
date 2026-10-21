// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#include "gui/gui.h"

#include <QApplication>
#include <QColor>
#include <QGuiApplication>
#include <QImage>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <algorithm>
#include <any>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#include "boost/algorithm/string/predicate.hpp"
#include "chartsWidget.h"
#include "clockWidget.h"
#include "displayControls.h"
#include "drcWidget.h"
#include "gui_utils.h"
#include "heatMapGui.h"
#include "helpWidget.h"
#include "inspector.h"
#include "layoutViewer.h"
#include "mainWindow.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbShape.h"
#include "odb/geom.h"
#include "ord/OpenRoad.hh"
#include "qtDialogs.h"
#include "ruler.h"
#include "scriptWidget.h"
#include "timingWidget.h"
#include "utl/Logger.h"
#include "utl/decode.h"
#include "utl/exception.h"
#include "web/descriptor_registry.h"
#include "web/heatMap.h"

extern int cmd_argc;
extern char** cmd_argv;

namespace web {

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

  // A Bazel-installed binary has no reachable ICU data directory (see
  // bazel/icu-patches), so QCollator legitimately cannot open a collator.
  // Qt already falls back to a plain, non-locale-aware string compare in
  // that case (qcollator_icu.cpp), so this is harmless -- just noisy.
  if (msg.contains("Could not create collator")) {
    suppress = true;
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
      debugPrint(logger, utl::GUI, "qt", 1, print_msg);
      break;
    case QtInfoMsg:
      logger->info(utl::GUI, 75, "{}", print_msg);
      break;
    case QtWarningMsg:
      logger->warn(utl::GUI, 76, "{}", print_msg);
      break;
    case QtCriticalMsg:
    case QtFatalMsg:
      logger->error(utl::GUI, 77, "{}", print_msg);
      break;
  }
}

// This provides the link for Gui::redraw to the widget
static gui::MainWindow* main_window = nullptr;

static QWidget* findWidget(const std::string& name)
{
  if (name == "main_window" || name == "OpenROAD") {
    return main_window;
  }

  if (main_window == nullptr) {
    return nullptr;
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

// Bridges Gui to the Qt main window.  Installed once the window is built and
// uninstalled before it is destroyed, so main_window is non-null and fully
// alive for every call below -- which is why none of them check it.
class QtGuiBackend : public GuiBackend
{
 public:
  bool hasWindow() const override { return true; }

  void redraw() override { main_window->redraw(); }

  void pause(int timeout_ms) override { main_window->pause(timeout_ms); }

  bool isPaused() const override
  {
    return main_window->getScriptWidget()->isPaused();
  }

  void status(const std::string& message) override
  {
    main_window->status(message);
  }

  void registerRenderer(Renderer* renderer) override
  {
    main_window->getControls()->registerRenderer(renderer);
  }

  void unregisterRenderer(Renderer* renderer) override
  {
    main_window->getControls()->unregisterRenderer(renderer);
  }

  void setSelected(const Selected& selection) override
  {
    main_window->setSelected(selection);
  }

  void addSelected(const Selected& selection) override
  {
    main_window->addSelected(selection);
  }

  void addSelected(const SelectionSet& selection, bool find_in_cts) override
  {
    main_window->addSelected(selection, find_in_cts);
  }

  void removeSelectedByType(const std::string& type) override
  {
    main_window->removeSelectedByType(type);
  }

  const SelectionSet& selection() override { return main_window->selection(); }

  const Selected& inspectorSelection() override
  {
    return main_window->getInspector()->getSelection();
  }

  bool anyObjectInSet(bool selection_set,
                      odb::dbObjectType obj_type) const override
  {
    return main_window->anyObjectInSet(selection_set, obj_type);
  }

  void addHighlighted(const SelectionSet& selection,
                      int highlight_group) override
  {
    main_window->addHighlighted(selection, highlight_group);
  }

  void clearHighlighted(int highlight_group) override
  {
    main_window->clearHighlighted(highlight_group);
  }

  void selectHighlightConnectedInsts(bool select_flag,
                                     int highlight_group) override
  {
    main_window->selectHighlightConnectedInsts(select_flag, highlight_group);
  }

  void selectHighlightConnectedNets(bool select_flag,
                                    bool output,
                                    bool input,
                                    int highlight_group) override
  {
    main_window->selectHighlightConnectedNets(
        select_flag, output, input, highlight_group);
  }

  void selectHighlightConnectedBufferTrees(bool select_flag,
                                           int highlight_group) override
  {
    main_window->selectHighlightConnectedBufferTrees(select_flag,
                                                     highlight_group);
  }

  int selectArea(const odb::Rect& area, bool append) override
  {
    return main_window->getLayoutViewer()->selectArea(area, append);
  }

  int selectNext() override
  {
    return main_window->getInspector()->selectNext();
  }

  int selectPrevious() override
  {
    return main_window->getInspector()->selectPrevious();
  }

  void selectionAnimation(int repeat) override
  {
    main_window->getLayoutViewer()->selectionAnimation(repeat);
  }

  void zoomTo(const odb::Rect& rect_dbu) override
  {
    main_window->zoomTo(rect_dbu);
  }

  void zoomTo(const odb::Point& focus, int diameter) override
  {
    main_window->zoomTo(focus, diameter);
  }

  void zoomIn() override { main_window->getLayoutViewer()->zoomIn(); }

  void zoomIn(const odb::Point& focus_dbu) override
  {
    main_window->getLayoutViewer()->zoomIn(focus_dbu);
  }

  void zoomOut() override { main_window->getLayoutViewer()->zoomOut(); }

  void zoomOut(const odb::Point& focus_dbu) override
  {
    main_window->getLayoutViewer()->zoomOut(focus_dbu);
  }

  void centerAt(const odb::Point& focus_dbu) override
  {
    main_window->getLayoutViewer()->centerAt(focus_dbu);
  }

  void setResolution(double pixels_per_dbu) override
  {
    main_window->getLayoutViewer()->setResolution(pixels_per_dbu);
  }

  void fit() override { main_window->fit(); }

  std::string addLabel(int x,
                       int y,
                       const std::string& text,
                       std::optional<Painter::Color> color,
                       std::optional<int> size,
                       std::optional<Painter::Anchor> anchor,
                       const std::optional<std::string>& name) override
  {
    return main_window->addLabel(x, y, text, color, size, anchor, name);
  }

  void deleteLabel(const std::string& name) override
  {
    main_window->deleteLabel(name);
  }

  void clearLabels() override { main_window->clearLabels(); }

  std::string addRuler(int x0,
                       int y0,
                       int x1,
                       int y1,
                       const std::string& label,
                       const std::string& name,
                       bool euclidian) override
  {
    return main_window->addRuler(x0, y0, x1, y1, label, name, euclidian);
  }

  void deleteRuler(const std::string& name) override
  {
    main_window->deleteRuler(name);
  }

  void clearRulers() override { main_window->clearRulers(); }

  bool checkDisplayControlVisible(const std::string& name) override
  {
    return main_window->getControls()->checkControlByPath(name, true);
  }

  bool checkDisplayControlSelectable(const std::string& name) override
  {
    return main_window->getControls()->checkControlByPath(name, false);
  }

  void setDisplayControlVisible(const std::string& name, bool value) override
  {
    main_window->getControls()->setControlByPath(
        name, true, value ? Qt::Checked : Qt::Unchecked);
  }

  void setDisplayControlSelectable(const std::string& name, bool value) override
  {
    main_window->getControls()->setControlByPath(
        name, false, value ? Qt::Checked : Qt::Unchecked);
  }

  void setDisplayControlColor(const std::string& name,
                              const Painter::Color& color) override
  {
    main_window->getControls()->setControlByPath(name, gui::toQColor(color));
  }

  void saveDisplayControls() override { main_window->getControls()->save(); }

  void restoreDisplayControls() override
  {
    main_window->getControls()->restore();
  }

  void addFocusNet(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->addFocusNet(net);
  }

  void removeFocusNet(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->removeFocusNet(net);
  }

  void clearFocusNets() override
  {
    main_window->getLayoutTabs()->clearFocusNets();
  }

  void addRouteGuides(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->addRouteGuides(net);
  }

  void removeRouteGuides(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->removeRouteGuides(net);
  }

  void clearRouteGuides() override
  {
    main_window->getLayoutTabs()->clearRouteGuides();
  }

  void addNetTracks(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->addNetTracks(net);
  }

  void removeNetTracks(odb::dbNet* net) override
  {
    main_window->getLayoutTabs()->removeNetTracks(net);
  }

  void clearNetTracks() override
  {
    main_window->getLayoutTabs()->clearNetTracks();
  }

  void saveClockTreeImage(const std::string& clock_name,
                          const std::string& filename,
                          const std::string& scene,
                          std::optional<int> width_px,
                          std::optional<int> height_px) override
  {
    main_window->getClockViewer()->saveImage(
        clock_name, filename, scene, width_px, height_px);
  }

  void saveImage(const std::string& filename,
                 const odb::Rect& region,
                 int width_px,
                 double dbu_per_pixel) override
  {
    main_window->getLayoutViewer()->saveImage(
        filename.c_str(), region, width_px, dbu_per_pixel);
  }

  void saveHistogramImage(const std::string& filename,
                          const std::string& mode,
                          std::optional<int> width_px,
                          std::optional<int> height_px) override
  {
    auto* charts = main_window->getChartsWidget();
    charts->saveImage(
        filename, charts->modeFromString(mode), width_px, height_px);
  }

  bool isOffscreen() const override
  {
    if (main_window->testAttribute(Qt::WA_DontShowOnScreen)) {
      // Set when the gui was started non-interactively.
      return true;
    }
    // The platform plugin decides this too: "offscreen" and "minimal" never
    // map a window, so the layout viewer is never laid out and its viewport
    // is meaningless even when the window was asked for interactively
    // (openroad -gui under QT_QPA_PLATFORM=offscreen).
    const QString platform = QGuiApplication::platformName();
    return platform == QLatin1String("offscreen")
           || platform == QLatin1String("minimal");
  }

  RenderedImage renderImage(
      const odb::Rect& region,
      int width_px,
      double dbu_per_pixel,
      std::optional<std::pair<int, int>> scale_to) override
  {
    QImage img = main_window->getLayoutViewer()->createImage(
        region, width_px, dbu_per_pixel);
    if (scale_to.has_value()) {
      img = img.scaled(scale_to->first, scale_to->second, Qt::KeepAspectRatio);
    }

    // Format_RGBA8888 is byte-ordered, so its memory layout is already the
    // R,G,B,A this hands back and the rows can be copied whole.  Going
    // through pixel() instead costs a bounds check, a format check and a
    // coordinate mapping for each of the hundreds of thousands of pixels in
    // a frame.  Copying per row rather than in one block avoids assuming
    // bytesPerLine() has no padding.
    const QImage rgba = img.convertToFormat(QImage::Format_RGBA8888);

    RenderedImage out;
    out.width = rgba.width();
    out.height = rgba.height();
    const size_t row_bytes = static_cast<size_t>(out.width) * 4;
    out.rgba.resize(row_bytes * out.height);
    for (int y = 0; y < out.height; y++) {
      std::copy_n(rgba.constScanLine(y), row_bytes, &out.rgba[row_bytes * y]);
    }
    return out;
  }

  Chart* addChart(const std::string& name,
                  const std::string& x_label,
                  const std::vector<std::string>& y_labels) override
  {
    return main_window->getChartsWidget()->addChart(name, x_label, y_labels);
  }

  void timingCone(Term term, bool fanin, bool fanout) override
  {
    main_window->timingCone(term, fanin, fanout);
  }

  void timingPathsThrough(const std::set<Term>& terms) override
  {
    main_window->timingPathsThrough(terms);
  }

  void triggerAction(const std::string& name) override
  {
    // name is widget.action: up to the last dot picks the widget, the rest
    // names the action in it.  With no dot there is no action to look for --
    // the whole string would stand in for both halves, and the widget would
    // be searched for an action named after itself.
    const size_t dot_idx = name.find_last_of('.');
    if (dot_idx == std::string::npos) {
      return;
    }

    auto* widget = findWidget(name.substr(0, dot_idx));
    if (widget == nullptr) {
      return;
    }

    const QString find_name = QString::fromStdString(name.substr(dot_idx + 1));

    // Find QAction
    for (QAction* action : widget->findChildren<QAction*>()) {
      ord::OpenRoad::openRoad()->getLogger()->report(
          "{} {}",
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
};

static QtGuiBackend qt_backend;

// Opens a gui to run a script in, for the callers that need one rendering
// and have none -- see GuiLauncher.
class QtGuiLauncher : public GuiLauncher
{
 public:
  void openAndRun(const std::string& cmds) override
  {
    Gui::get()->showGui(cmds, false);
  }
};

static QtGuiLauncher qt_launcher;

std::string Gui::addToolbarButton(const std::string& name,
                                  const std::string& text,
                                  const std::string& script,
                                  bool echo)
{
  if (!hasUI()) {
    return "";
  }
  return main_window->addToolbarButton(
      name, QString::fromStdString(text), QString::fromStdString(script), echo);
}

void Gui::removeToolbarButton(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  main_window->removeToolbarButton(name);
}

std::string Gui::addMenuItem(const std::string& name,
                             const std::string& path,
                             const std::string& text,
                             const std::string& script,
                             const std::string& shortcut,
                             bool echo)
{
  if (!hasUI()) {
    return "";
  }
  return main_window->addMenuItem(name,
                                  QString::fromStdString(path),
                                  QString::fromStdString(text),
                                  QString::fromStdString(script),
                                  QString::fromStdString(shortcut),
                                  echo);
}

void Gui::removeMenuItem(const std::string& name)
{
  if (!hasUI()) {
    return;
  }
  main_window->removeMenuItem(name);
}

std::string Gui::requestUserInput(const std::string& title,
                                  const std::string& question)
{
  if (!hasUI()) {
    return "";
  }
  return main_window->requestUserInput(QString::fromStdString(title),
                                       QString::fromStdString(question));
}

void Gui::selectMarkers(odb::dbMarkerCategory* markers)
{
  if (!hasUI()) {
    return;
  }
  main_window->getDRCViewer()->selectCategory(markers);
}

void Gui::showWorstTimingPath(bool setup)
{
  if (!hasUI()) {
    return;
  }
  main_window->getTimingWidget()->showWorstTimingPath(setup);
}

void Gui::clearTimingPath()
{
  if (!hasUI()) {
    return;
  }
  main_window->getTimingWidget()->clearSelection();
}

void Gui::selectClockviewerClock(const std::string& clock_name,
                                 std::optional<int> depth)
{
  if (!hasUI()) {
    return;
  }
  main_window->getClockViewer()->selectClock(clock_name, depth);
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

void Gui::setMainWindowTitle(const std::string& title)
{
  main_window_title_ = title;
  if (main_window) {
    main_window->setTitle(title);
  }
}

std::string Gui::getMainWindowTitle()
{
  return main_window_title_;
}

void Gui::setLogger(utl::Logger* logger)
{
  if (logger == nullptr) {
    return;
  }

  logger_ = logger;
  qInstallMessageHandler(message_handler);

  if (hasUI()) {
    // gui already requested, so go ahead and set the logger
    main_window->setLogger(logger);
  }
}

void Gui::hideGui()
{
  if (!hasUI()) {
    return;
  }
  // ensure continue after close is true, since we want to return to tcl
  setContinueAfterClose();
  main_window->exit();
}

void Gui::showGui(const std::string& cmds, bool interactive, bool load_settings)
{
  if (enabled()) {
    logger_->warn(utl::GUI, 8, "GUI already active.");
    return;
  }

  // OR already running, so GUI should not set anything up
  // passing in cmd_argc and cmd_argv to meet Qt application requirement for
  // arguments nullptr for tcl interp to indicate nothing to setup and commands
  // and interactive setting
  gui::startGui(cmd_argc, cmd_argv, nullptr, cmds, interactive, load_settings);
}

void Gui::minimize()
{
  if (!hasUI()) {
    return;
  }
  main_window->showMinimized();
}

void Gui::unminimize()
{
  if (!hasUI()) {
    return;
  }
  main_window->showNormal();
}

void Gui::init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* logger)
{
  initCommon(db, sta, logger);
  setLogger(logger);

  // Lets the descriptors offer the actions that need a modal dialog, and
  // saveImage reopen the gui when no window is up.  Only this file is
  // Qt-only, so a build without Qt leaves both hooks null.
  static gui::QtDialogs dialogs;
  setDialogs(&dialogs);
  setLauncher(&qt_launcher);

  for (const auto& source : getRegisteredHeatMapSources()) {
    const bool already_registered = std::ranges::any_of(
        heat_maps_, [&source](HeatMapDataSource* heatmap) {
          return heatmap->getShortName() == source->getShortName();
        });
    if (already_registered) {
      continue;
    }
    auto instance = source->createInstance();
    instance->setChip(db->getChip());
    registerHeatMap(instance.get());
    owned_heat_maps_.push_back(std::move(instance));
  }
}

void Gui::selectHelp(const std::string& item)
{
  if (!hasUI()) {
    return;
  }

  main_window->getHelpViewer()->selectHelp(item);
}

void Gui::selectChart(const std::string& name)
{
  if (!hasUI()) {
    return;
  }

  const gui::ChartsWidget::Mode mode
      = main_window->getChartsWidget()->modeFromString(name);
  main_window->getChartsWidget()->setMode(mode);
}

void Gui::updateTimingReport()
{
  if (!hasUI()) {
    return;
  }
  main_window->getTimingWidget()->populatePaths();
}

class SafeApplication : public QApplication
{
 public:
  using QApplication::QApplication;

  bool notify(QObject* receiver, QEvent* event) override
  {
    try {
      return QApplication::notify(receiver, event);
    } catch (std::exception& ex) {
      // Ignored here as the message will be logged in the GUI
      qDebug() << "Caught exception:" << ex.what();

      // Returning true indicates the event has been handled. In this case,
      // we've "handled" it by catching the exception, so we prevent
      // further processing that might rely on a corrupt state.
      return true;
    }

    return false;
  }
};

//////////////////////////////////////////////////

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
}  // namespace web

namespace gui {

// The entry points keep their own namespace: gui/gui.h and gui/MakeGui.h
// declare them there, and OpenRoad calls them by that name.  Everything they
// reach for -- the window, the backend, web::Gui itself -- is web's now.
using namespace web;  // NOLINT(build/namespaces)

int startGui(int& argc,
             char* argv[],
             Tcl_Interp* interp,
             const std::string& script,
             bool interactive,
             bool load_settings,
             bool minimize)
{
#ifdef STATIC_QPA_PLUGIN_XCB
  const char* qt_qpa_platform_env = getenv("QT_QPA_PLATFORM");
  std::string qpa_platform
      = qt_qpa_platform_env == nullptr ? "" : qt_qpa_platform_env;
  if (qpa_platform != "") {
    if (qpa_platform.find("xcb") == std::string::npos
        && qpa_platform.find("offscreen") == std::string::npos) {
      // OpenROAD logger is not available yet, using cout.
      std::cout << "Your system has set QT_QPA_PLATFORM='" << qpa_platform
                << "', openroad only supports 'offscreen' and 'xcb', please "
                   "include one of these plugins in your platform env\n";
    }
  }
#endif
  auto gui = web::Gui::get();
  // ensure continue after close is false
  gui->clearContinueAfterClose();

  SafeApplication app(argc, argv);
  application = &app;

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  auto* open_road = ord::OpenRoad::openRoad();

  // create new MainWindow
  main_window = new gui::MainWindow(load_settings);
  gui->setBackend(&qt_backend);
  if (minimize) {
    main_window->showMinimized();
  }
  main_window->setTitle(gui->getMainWindowTitle());

  open_road->getDb()->addObserver(main_window);
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
        main_window->init(open_road->getSta(), open_road->getDocsPath());
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

  // Hide the web::Gui if someone chooses hide from the menu in the window
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
  open_road->getDb()->removeObserver(main_window);

  if (!exception.hasException()) {
    // don't save anything if exception occured
    gui->clearRestoreStateCommands();
    // save restore state commands
    for (const auto& cmd : main_window->getRestoreTclCommands()) {
      gui->addRestoreStateCommand(cmd);
    }
  }

  main_window->exit();

  // Uninstall before destroying the window, not after.  ~MainWindow destroys
  // its children in construction order, so DisplayControls (the first one)
  // is already gone when DRCWidget and the clock viewer destroy the
  // Renderers they own.  Each ~web::Renderer calls
  // web::Gui::unregisterRenderer, and with the backend still installed that
  // would reach main_window->getControls() on a freed DisplayControls.
  web::Gui::get()->setBackend(nullptr);

  // delete main window and set to nullptr
  delete main_window;
  main_window = nullptr;
  application = nullptr;

  web::Gui::resetDbuConversions();

  // rethow exception, if one happened after cleanup of main_window
  exception.rethrow();

  debugPrint(open_road->getLogger(),
             utl::GUI,
             "init",
             1,
             "Exit state: interactive ({}), isContinueAfterClose ({}), "
             "exit_requested ({}), exit_code ({})",
             interactive,
             gui->isContinueAfterClose(),
             exit_requested,
             exit_code);

  const bool do_exit = !gui->isContinueAfterClose() && exit_requested;
  if (interactive && do_exit) {
    // if exiting, go ahead and exit with gui return code.
    exit(exit_code);
  }

  return exit_code;
}

// Tcl files encoded into strings.
extern const char* gui_tcl_inits[];

extern "C" {
extern int Gui_Init(Tcl_Interp* interp);
}

void initGui(Tcl_Interp* interp,
             odb::dbDatabase* db,
             sta::dbSta* sta,
             utl::Logger* logger)
{
  // Define swig TCL commands.
  Gui_Init(interp);
  utl::evalTclInit(interp, gui::gui_tcl_inits);

  // ensure gui is made
  auto* gui = web::Gui::get();
  gui->init(db, sta, logger);
}

}  // namespace gui
