#include <algorithm>
#include <cstdio>
#include <limits>

#include "graphics.h"
#include "nesterovBase.h"
#include "placerBase.h"

namespace replace {

Graphics::Graphics(std::shared_ptr<PlacerBase> pb,
                   std::shared_ptr<NesterovBase> nb,
                   bool draw_bins)
    : pb_(pb), nb_(nb), draw_bins_(draw_bins)
{
  gui::Gui::get()->register_renderer(this);
}

void Graphics::drawObjects(gui::Painter& painter)
{
  // draw core bounds
  auto& die = pb_->die();
  painter.setPen(gui::Painter::yellow, /* cosmetic */ true);
  painter.drawLine(die.coreLx(), die.coreLy(), die.coreUx(), die.coreLy());
  painter.drawLine(die.coreUx(), die.coreLy(), die.coreUx(), die.coreUy());
  painter.drawLine(die.coreUx(), die.coreUy(), die.coreLx(), die.coreUy());
  painter.drawLine(die.coreLx(), die.coreUy(), die.coreLx(), die.coreLy());

  if (draw_bins_) {
    // Draw the bins
    painter.setPen(gui::Painter::white, /* cosmetic */ true);
    for (auto& bin : nb_->bins()) {
      int color = bin->density() * 50 + 20;

      color = (color > 255) ? 255 : (color < 20) ? 20 : color;
      color = 255 - color;

      painter.setBrush({color, color, color, 180});
      painter.drawRect({bin->lx(), bin->ly(), bin->ux(), bin->uy()});
    }
  }

  // Draw the placeable objects
  painter.setPen(gui::Painter::white);
  for (auto& gCell : nb_->gCells()) {
    const int gcx = gCell->dCx();
    const int gcy = gCell->dCy();

    int xl = gcx - gCell->dx() / 2;
    int yl = gcy - gCell->dy() / 2;
    int xh = gcx + gCell->dx() / 2;
    int yh = gcy + gCell->dy() / 2;

    gui::Painter::Color color;
    if (gCell->isInstance()) {
      color = gui::Painter::dark_blue;
    } else if (gCell->isFiller()) {
      color = gui::Painter::dark_magenta;
    }
    color.a = 180;
    painter.setBrush(color);
    painter.drawRect({xl, yl, xh, yh});
  }

  // Draw force direction lines
  if (true || draw_bins_) {
    float efMax = 0;
    int max_len = std::numeric_limits<int>::max();
    for (auto& bin : nb_->bins()) {
      efMax
          = std::max(efMax, hypot(bin->electroForceX(), bin->electroForceY()));
      max_len = std::min({max_len, bin->dx(), bin->dy()});
    }

    for (auto& bin : nb_->bins()) {
      float fx = bin->electroForceX();
      float fy = bin->electroForceY();
      float angle = atan(fy / fx);
      float ratio = hypot(fx, fy) / efMax;
      float dx = cos(angle) * max_len * ratio;
      float dy = sin(angle) * max_len * ratio;

      int cx = bin->cx();
      int cy = bin->cy();

      painter.setPen(gui::Painter::red, true);
      painter.drawLine(cx, cy, cx + dx, cy + dy);
    }
  }
}  // namespace replace

void Graphics::cellPlot(bool pause)
{
  gui::Gui::get()->redraw();
  if (pause) {
    gui::Gui::get()->pause();
  }
}

}  // namespace replace
