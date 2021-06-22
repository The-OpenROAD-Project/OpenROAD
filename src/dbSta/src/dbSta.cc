/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// BSD 3-Clause License
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
//
///////////////////////////////////////////////////////////////////////////////

// dbSta, OpenSTA on OpenDB

// Network edit function flow
// tcl edit cmd -> dbNetwork -> db -> dbStaCbk -> Sta
// Edits from tcl, Sta api and db edits are all supported.

#include "db_sta/dbSta.hh"

#include <algorithm>            // min
#include <tcl.h>

#include "sta/StaMain.hh"
#include "sta/Graph.hh"
#include "sta/Clock.hh"
#include "sta/Sdc.hh"
#include "sta/Search.hh"
#include "sta/PathRef.hh"
#include "sta/PathExpanded.hh"
#include "sta/Bfs.hh"
#include "sta/EquivCells.hh"
#include "sta/ReportTcl.hh"

#include "opendb/db.h"

#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

#include "gui/gui.h"

#include "db_sta/dbNetwork.hh"
#include "db_sta/MakeDbSta.hh"

#include "dbSdcNetwork.hh"

namespace ord {

using sta::dbSta;
using sta::PathRef;
using sta::PathExpanded;

sta::dbSta *
makeDbSta()
{
  return new sta::dbSta;
}

void
initDbSta(OpenRoad *openroad)
{
  dbSta *sta = openroad->getSta();
  sta->init(openroad->tclInterp(), openroad->getDb(),
            // Broken gui api missing openroad accessor.
            gui::Gui::get(),
            openroad->getLogger());
  openroad->addObserver(sta);
}

void
deleteDbSta(sta::dbSta *sta)
{
  delete sta;
}

} // namespace ord

////////////////////////////////////////////////////////////////

namespace sta {

using std::min;

using utl::Logger;
using utl::STA;

class dbStaReport : public sta::ReportTcl
{
public:
  void setLogger(Logger *logger);
  virtual void warn(int id,
                    const char *fmt,
                    ...)
    __attribute__((format (printf, 3, 4)));
  virtual void fileWarn(int id,
                        const char *filename,
                        int line,
                        const char *fmt, ...)
    __attribute__((format (printf, 5, 6)));
  virtual void vfileWarn(int id,
                         const char *filename,
                         int line,
                         const char *fmt,
                         va_list args);

  virtual void error(int id,
                     const char *fmt,
                     ...)
    __attribute__((format (printf, 3, 4)));
  virtual void fileError(int id,
                         const char *filename,
                         int line,
                         const char *fmt, ...)
    __attribute__((format (printf, 5, 6)));
  virtual void vfileError(int id,
                          const char *filename,
                          int line,
                          const char *fmt,
                          va_list args);

  virtual void critical(int id,
                        const char *fmt,
                        ...)
    __attribute__((format (printf, 3, 4)));
  virtual size_t printString(const char *buffer,
                             size_t length);

protected:
  virtual void printLine(const char *line,
                         size_t length);

  Logger *logger_;
};

class dbStaCbk : public dbBlockCallBackObj
{
public:
  dbStaCbk(dbSta *sta,
           Logger *logger);
  void setNetwork(dbNetwork *network);
  virtual void inDbInstCreate(dbInst *inst) override;
  virtual void inDbInstDestroy(dbInst *inst) override;
  virtual void inDbInstSwapMasterBefore(dbInst *inst,
                                        dbMaster *master) override;
  virtual void inDbInstSwapMasterAfter(dbInst *inst) override;
  virtual void inDbNetDestroy(dbNet *net) override;
  void inDbITermPostConnect(dbITerm *iterm) override;
  void inDbITermPreDisconnect(dbITerm *iterm) override;
  void inDbITermDestroy(dbITerm *iterm) override;
  void inDbBTermPostConnect(dbBTerm *bterm) override;
  void inDbBTermPreDisconnect(dbBTerm *bterm) override;
  void inDbBTermDestroy(dbBTerm *bterm) override;

private:
  dbSta *sta_;
  dbNetwork *network_;
  Logger *logger_;
};

class PathRenderer : public gui::Renderer
{
public:
  PathRenderer(dbSta *sta);
  ~PathRenderer();
  void highlight(PathRef *path);
  virtual void drawObjects(gui::Painter& /* painter */) override;

private:
  void highlightInst(const Pin *pin,
                     gui::Painter &painter);

  dbSta *sta_;
  // Expanded path is owned by PathRenderer.
  PathExpanded *path_;
  static gui::Painter::Color signal_color;
  static gui::Painter::Color clock_color;
};

dbSta *
makeBlockSta(ord::OpenRoad *openroad,
             dbBlock *block)
{
  dbSta *sta = openroad->getSta();
  dbSta *sta2 = new dbSta;
  sta2->makeComponents();
  sta2->getDbNetwork()->setBlock(block);
  sta2->setTclInterp(sta->tclInterp());
  sta2->getDbReport()->setLogger(openroad->getLogger());
  sta2->copyUnits(sta->units());
  return sta2;
}

extern "C" {
extern int Dbsta_Init(Tcl_Interp *interp);
}

extern const char *dbSta_tcl_inits[];

dbSta::dbSta() :
  Sta(),
  db_(nullptr),
  db_cbk_(nullptr),
  path_renderer_(nullptr)
{
}

dbSta::~dbSta()
{
  delete path_renderer_;
}

void
dbSta::init(Tcl_Interp *tcl_interp,
	    dbDatabase *db,
            gui::Gui *gui,
            Logger *logger)
{
  initSta();
  Sta::setSta(this);
  db_ = db;
  gui_ = gui;
  logger_ = logger;
  makeComponents();
  setTclInterp(tcl_interp);
  db_report_->setLogger(logger);
  db_cbk_ = new dbStaCbk(this, logger);
  // Define swig TCL commands.
  Dbsta_Init(tcl_interp);
  // Eval encoded sta TCL sources.
  evalTclInit(tcl_interp, dbSta_tcl_inits);
}

// Wrapper to init network db.
void
dbSta::makeComponents()
{
  Sta::makeComponents();
  db_network_->setDb(db_);
}

////////////////////////////////////////////////////////////////

void
dbSta::makeReport()
{
  db_report_ = new dbStaReport();
  report_ = db_report_;
}

void
dbSta::makeNetwork()
{
  db_network_ = new class dbNetwork();
  network_ = db_network_;
}

void
dbSta::makeSdcNetwork()
{
  sdc_network_ = new dbSdcNetwork(network_);
}

void
dbSta::postReadLef(dbTech* tech,
                   dbLib* library)
{
  if (library) {
    db_network_->readLefAfter(library);
  }
}

void
dbSta::postReadDef(dbBlock* block)
{
  db_network_->readDefAfter(block);
  db_cbk_->addOwner(block);
  db_cbk_->setNetwork(db_network_);
}

void
dbSta::postReadDb(dbDatabase* db)
{
  db_network_->readDbAfter(db);
  odb::dbChip *chip = db_->getChip();
  if (chip) {
    odb::dbBlock *block = chip->getBlock();
    if (block) {
      db_cbk_->addOwner(block);
      db_cbk_->setNetwork(db_network_);
    }
  }
}

Slack
dbSta::netSlack(const dbNet *db_net,
		const MinMax *min_max)
{
  const Net *net = db_network_->dbToSta(db_net);
  return netSlack(net, min_max);
}

std::set<dbNet*>
dbSta::findClkNets()
{
  ensureClkNetwork();
  std::set<dbNet*> clk_nets;
  for (Clock *clk : sdc_->clks()) {
    for (const Pin *pin : *pins(clk)) {
      Net *net = network_->net(pin);      
      if (net)
	clk_nets.insert(db_network_->staToDb(net));
    }
  }
  return clk_nets;
}

std::set<dbNet*>
dbSta::findClkNets(const Clock *clk)
{
  ensureClkNetwork();
  std::set<dbNet*> clk_nets;
  for (const Pin *pin : *pins(clk)) {
    Net *net = network_->net(pin);      
    if (net)
      clk_nets.insert(db_network_->staToDb(net));
  }
  return clk_nets;
}

////////////////////////////////////////////////////////////////

// Network edit functions.
// These override the default sta functions that call sta before/after
// functions because the db calls them via callbacks.

void
dbSta::deleteInstance(Instance *inst)
{
  NetworkEdit *network = networkCmdEdit();
  network->deleteInstance(inst);
}

void
dbSta::replaceCell(Instance *inst,
                   Cell *to_cell,
                   LibertyCell *to_lib_cell)
{
  NetworkEdit *network = networkCmdEdit();
  LibertyCell *from_lib_cell = network->libertyCell(inst);
  if (sta::equivCells(from_lib_cell, to_lib_cell)) {
    replaceEquivCellBefore(inst, to_lib_cell);
    network->replaceCell(inst, to_cell);
    replaceEquivCellAfter(inst);
  }
  else {
    replaceCellBefore(inst, to_lib_cell);
    network->replaceCell(inst, to_cell);
    replaceCellAfter(inst);
  }
}

void
dbSta::deleteNet(Net *net)
{
  NetworkEdit *network = networkCmdEdit();
  network->deleteNet(net);
}

void
dbSta::connectPin(Instance *inst,
                  Port *port,
                  Net *net)
{
  NetworkEdit *network = networkCmdEdit();
  network->connect(inst, port, net);
}

void
dbSta::connectPin(Instance *inst,
                  LibertyPort *port,
                  Net *net)
{
  NetworkEdit *network = networkCmdEdit();
  network->connect(inst, port, net);
}

void
dbSta::disconnectPin(Pin *pin)
{
  NetworkEdit *network = networkCmdEdit();
  network->disconnectPin(pin);
}

////////////////////////////////////////////////////////////////

void
dbStaReport::setLogger(Logger *logger)
{
  logger_ = logger;
}

// Line return \n is implicit.
void
dbStaReport::printLine(const char *buffer,
                       size_t length)
{
  logger_->report(buffer);
}

// Only used by encapsulated Tcl channels, ie puts and command prompt.
size_t
dbStaReport::printString(const char *buffer,
                         size_t length)
{
  string buf(buffer, length);

  size_t last_new_line = 0;
  size_t next_new_line = buf.find_first_of("\n");
  while (next_new_line != string::npos) {
    size_t length = next_new_line - last_new_line;
    logger_->report(buf.substr(last_new_line, length));

    last_new_line = next_new_line + 1;
    next_new_line = buf.find_first_of("\n", last_new_line);
  }
  if (last_new_line < buf.length()) {
    logger_->reportNoNewLine(buf.substr(last_new_line).c_str());
  }
  return length;
}

void
dbStaReport::warn(int id,
                  const char *fmt,
                  ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->warn(STA, id, "{}", buffer_);
  va_end(args);
}

void
dbStaReport::fileWarn(int id,
                      const char *filename,
                      int line,
                      const char *fmt,
                      ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->warn(STA, id, "{}", buffer_);
  va_end(args);
}

void
dbStaReport::vfileWarn(int id,
                       const char *filename,
                       int line,
                       const char *fmt,
                       va_list args)
{
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->warn(STA, id, "{}", buffer_);
}

void
dbStaReport::error(int id,
                   const char *fmt,
                   ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->error(STA, id, buffer_);
  va_end(args);
}

void
dbStaReport::fileError(int id,
                       const char *filename,
                       int line,
                       const char *fmt,
                       ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->error(STA, id, "{}", buffer_);
  va_end(args);
}

void
dbStaReport::vfileError(int id,
                        const char *filename,
                        int line,
                        const char *fmt,
                        va_list args)
{
  printToBuffer("%s line %d, ", filename, line);
  printToBufferAppend(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->error(STA, id, "{}", buffer_);
}

void
dbStaReport::critical(int id,
                      const char *fmt,
                      ...)
{
  va_list args;
  va_start(args, fmt);
  std::unique_lock<std::mutex> lock(buffer_lock_);
  printToBuffer(fmt, args);
  // Don't give std::format a chance to interpret the message.
  logger_->critical(STA, id, "{}", buffer_);
  va_end(args);
}

////////////////////////////////////////////////////////////////
//
// OpenDB callbacks to notify OpenSTA of network edits
//
////////////////////////////////////////////////////////////////

dbStaCbk::dbStaCbk(dbSta *sta,
                   Logger *logger) :
  sta_(sta),
  network_(nullptr),  // not built yet
  logger_(logger)
{
}

void
dbStaCbk::setNetwork(dbNetwork *network)
{
  network_ = network;
}

void
dbStaCbk::inDbInstCreate(dbInst* inst)
{
  sta_->makeInstanceAfter(network_->dbToSta(inst));
}

void
dbStaCbk::inDbInstDestroy(dbInst *inst)
{
  // This is called after the iterms have been destroyed
  // so it side-steps Sta::deleteInstanceAfter.
  sta_->deleteLeafInstanceBefore(network_->dbToSta(inst));
}

void
dbStaCbk::inDbInstSwapMasterBefore(dbInst *inst,
                                   dbMaster *master)
{
  LibertyCell *to_lib_cell = network_->libertyCell(network_->dbToSta(master));
  LibertyCell *from_lib_cell = network_->libertyCell(inst);
  Instance *sta_inst = network_->dbToSta(inst);
  if (sta::equivCells(from_lib_cell, to_lib_cell))
    sta_->replaceEquivCellBefore(sta_inst, to_lib_cell);
  else
    logger_->error(STA, 1000, "instance {} swap master {} is not equivalent",
                   inst->getConstName(),
                   master->getConstName());
}

void
dbStaCbk::inDbInstSwapMasterAfter(dbInst *inst)
{
  sta_->replaceEquivCellAfter(network_->dbToSta(inst));
}

void
dbStaCbk::inDbNetDestroy(dbNet *db_net)
{
  Net *net = network_->dbToSta(db_net);
  sta_->deleteNetBefore(net);
  network_->deleteNetBefore(net);
}

void
dbStaCbk::inDbITermPostConnect(dbITerm *iterm)
{
  Pin *pin = network_->dbToSta(iterm);
  network_->connectPinAfter(pin);
  sta_->connectPinAfter(pin);
}

void
dbStaCbk::inDbITermPreDisconnect(dbITerm *iterm)
{
  Pin *pin = network_->dbToSta(iterm);
  sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void
dbStaCbk::inDbITermDestroy(dbITerm *iterm)
{
  sta_->deletePinBefore(network_->dbToSta(iterm));
}

void
dbStaCbk::inDbBTermPostConnect(dbBTerm *bterm)
{
  Pin *pin = network_->dbToSta(bterm);
  network_->connectPinAfter(pin);
  sta_->connectPinAfter(pin);
}

void
dbStaCbk::inDbBTermPreDisconnect(dbBTerm *bterm)
{
  Pin *pin = network_->dbToSta(bterm);
  sta_->disconnectPinBefore(pin);
  network_->disconnectPinBefore(pin);
}

void
dbStaCbk::inDbBTermDestroy(dbBTerm *bterm)
{
  sta_->deletePinBefore(network_->dbToSta(bterm));
}

////////////////////////////////////////////////////////////////

// Highlight path in the gui.
void
dbSta::highlight(PathRef *path)
{
  if (gui_) {
    if (path_renderer_ == nullptr) {
      path_renderer_ = new PathRenderer(this);
      gui_->registerRenderer(path_renderer_);
    }
    path_renderer_->highlight(path);
  }
}

gui::Painter::Color PathRenderer::signal_color = gui::Painter::red;
gui::Painter::Color PathRenderer::clock_color = gui::Painter::yellow;

PathRenderer::PathRenderer(dbSta *sta) :
  sta_(sta),
  path_(nullptr)
{
}

PathRenderer::~PathRenderer()
{
  delete path_;
}

void
PathRenderer::highlight(PathRef *path)
{
  if (path_)
    delete path_;
  path_ = new PathExpanded(path, sta_);
}

void
PathRenderer::drawObjects(gui::Painter &painter)
{
  if (path_) {
    dbNetwork *network = sta_->getDbNetwork();
    Point prev_pt;
    for (unsigned int i = 0; i < path_->size(); i++) {
      PathRef *path = path_->path(i);
      TimingArc *prev_arc = path_->prevArc(i);
      // Draw lines for wires on the path.
      if (prev_arc && prev_arc->role()->isWire()) {
        PathRef *prev_path = path_->path(i - 1); 
        const Pin *pin = path->pin(sta_);
        const Pin *prev_pin = prev_path->pin(sta_);
        Point pt1 = network->location(pin);
        Point pt2 = network->location(prev_pin);
        gui::Painter::Color wire_color = sta_->isClock(pin) ? clock_color : signal_color;
        painter.setPen(wire_color, true);
        painter.drawLine(pt1, pt2);
        highlightInst(prev_pin, painter);
        if (i == path_->size() - 1)
          highlightInst(pin, painter);
      }
    }
  }
}

// Color in the instances to make them more visible.
void
PathRenderer::highlightInst(const Pin *pin,
                            gui::Painter &painter)
{
  dbNetwork *network = sta_->getDbNetwork();
  const Instance *inst = network->instance(pin);
  if (!network->isTopInstance(inst)) {
    dbInst *db_inst = network->staToDb(inst);
    odb::dbBox *bbox = db_inst->getBBox();
    odb::Rect rect;
    bbox->getBox(rect);
    gui::Painter::Color inst_color = sta_->isClock(pin) ? clock_color : signal_color;
    painter.setBrush(inst_color);
    painter.drawRect(rect);
  }
}

} // namespace sta
