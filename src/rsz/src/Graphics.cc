// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#include "Graphics.hh"

#include "BufferedNet.hh"
#include "odb/db.h"
#include "odb/geom.h"
#include "web/core.h"

namespace rsz {

Graphics::Graphics()
{
  web::Gui::get()->registerRenderer(this);
}

void Graphics::setNet(odb::dbNet* net)
{
  net_ = net;
  if (!net) {
    subdivide_ignore_ = false;
  }
}

void Graphics::stopOnSubdivideStep(const bool stop)
{
  stop_on_subdivide_step_ = stop;
}

void Graphics::subdivideStart(odb::dbNet* net)
{
  lines_.clear();
  if (net_) {
    subdivide_ignore_ = (net != net_);
  }
}

void Graphics::subdivide(const odb::Line& line)
{
  if (subdivide_ignore_) {
    return;
  }
  lines_.emplace_back(line);
  if (stop_on_subdivide_step_) {
    web::Gui::get()->redraw();
    web::Gui::get()->pause();
  }
}

void Graphics::subdivideDone()
{
  if (!subdivide_ignore_) {
    web::Gui::get()->redraw();
    web::Gui::get()->pause();
  }
}

void Graphics::repairNetStart(const BufferedNetPtr& bnet, odb::dbNet* net)
{
  lines_.clear();
  if (net_) {
    repair_net_ignore_ = (net != net_);
  }
  if (!repair_net_ignore_) {
    bnet_ = bnet;
    web::Gui::get()->redraw();
    web::Gui::get()->pause();
  }
}

void Graphics::makeBuffer(odb::dbInst* inst)
{
  if (!repair_net_ignore_) {
    buffers_.push_back(inst);
    web::Gui::get()->redraw();
    web::Gui::get()->pause();
  }
}

void Graphics::repairNetDone()
{
  if (!repair_net_ignore_) {
    web::Gui::get()->redraw();
    web::Gui::get()->pause();
  }
  bnet_.reset();
  buffers_.clear();
}

void Graphics::drawBNet(const BufferedNetPtr& bnet, web::Painter& painter)
{
  switch (bnet->type()) {
    case BufferedNetType::via: {
      drawBNet(bnet->ref(), painter);
      break;
    }
    case BufferedNetType::wire: {
      painter.drawLine(bnet->location(), bnet->ref()->location());
      drawBNet(bnet->ref(), painter);
      break;
    }
    case BufferedNetType::junction:
      painter.drawLine(bnet->location(), bnet->ref()->location());
      painter.drawLine(bnet->location(), bnet->ref2()->location());
      drawBNet(bnet->ref(), painter);
      drawBNet(bnet->ref2(), painter);
      break;
    case BufferedNetType::load: {
      odb::Point loc = bnet->location();
      painter.drawCircle(loc.x(), loc.y(), 1000);
      break;
    }
    case BufferedNetType::buffer:
      painter.drawLine(bnet->location(), bnet->ref()->location());
      drawBNet(bnet->ref(), painter);
      break;
  }
}

void Graphics::drawObjects(web::Painter& painter)
{
  painter.setPen(web::Painter::kRed, true);
  for (const odb::Line& line : lines_) {
    painter.drawLine(line.pt0(), line.pt1());
  }

  if (bnet_) {
    painter.setPenAndBrush(web::Painter::kPink, true);
    drawBNet(bnet_, painter);
    painter.setPenAndBrush(web::Painter::kGreen, true);
    for (odb::dbInst* buffer : buffers_) {
      painter.drawRect(buffer->getBBox()->getBox());
    }
  }
}

}  // namespace rsz
