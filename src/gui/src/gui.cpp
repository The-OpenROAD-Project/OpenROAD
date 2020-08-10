#include <QApplication>
#include <stdexcept>

#include "db.h"
#include "defin.h"
#include "gui/gui.h"
#include "lefin.h"
#include "mainWindow.h"
#include "openroad/OpenRoad.hh"

namespace gui {

// This is the main entry point to start the GUI.  It only
// returns when the GUI is done.
int start_gui(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Default to 12 point for easier reading
  QFont font = QApplication::font();
  font.setPointSize(12);
  QApplication::setFont(font);

  gui::MainWindow w;
  auto*           open_road = ord::OpenRoad::openRoad();
  w.setDb(open_road->getDb());
  open_road->addObserver(&w);
  w.show();

  // Exit the app if someone chooses exit from the menu in the window
  QObject::connect(&w, SIGNAL(exit()), &app, SLOT(quit()));

  // Save the window's status into the settings when quitting.
  QObject::connect(&app, SIGNAL(aboutToQuit()), &w, SLOT(saveSettings()));
 
  return app.exec();
}

}  // namespace gui
