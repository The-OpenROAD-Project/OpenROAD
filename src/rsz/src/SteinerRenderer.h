#pragma once

#include "AbstractSteinerRenderer.h"
#include "gui/gui.h"

namespace rsz {

class SteinerRenderer : public gui::Renderer, public AbstractSteinerRenderer
{
 public:
  SteinerRenderer();
  ~SteinerRenderer() override = default;

  void highlight(SteinerTree* tree) override;
  virtual void drawObjects(gui::Painter& /* painter */) override;

 private:
  SteinerTree* tree_;
};

}  // namespace rsz
