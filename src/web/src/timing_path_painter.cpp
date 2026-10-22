// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "timing_path_painter.h"

#include <cstddef>
#include <vector>

#include "dbDescriptors.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "timing_report.h"
#include "web/core.h"
#include "web/timing_path_style.h"

namespace web {

namespace {

odb::dbNet* netOf(const TimingNode& node)
{
  if (node.iterm != nullptr) {
    return node.iterm->getNet();
  }
  if (node.bterm != nullptr) {
    return node.bterm->getNet();
  }
  return nullptr;
}

odb::dbObject* pinOf(const TimingNode& node)
{
  if (node.iterm != nullptr) {
    return node.iterm;
  }
  return node.bterm;
}

// TimingPathRenderer::drawNodesList, last node first.  A driver's net is drawn
// to the node after it, which is its sink along the path.
void paintNodes(const std::vector<TimingNode>& nodes,
                const Painter::Color& clock_color,
                const bool draw_clock,
                const bool draw_signal,
                Painter& painter)
{
  Gui* gui = Gui::get();
  const Descriptor* net_descriptor = gui->getDescriptor<odb::dbNet*>();
  const Descriptor* inst_descriptor = gui->getDescriptor<odb::dbInst*>();
  const Descriptor* bterm_descriptor = gui->getDescriptor<odb::dbBTerm*>();

  for (size_t i = nodes.size(); i-- > 0;) {
    const TimingNode& node = nodes[i];
    if (node.is_clock ? !draw_clock : !draw_signal) {
      continue;
    }

    if (node.iterm != nullptr) {
      painter.setPenAndBrush(kTimingPathInstColor, true);
      inst_descriptor->highlight(node.iterm->getInst(), painter);
    }
    if (node.bterm != nullptr) {
      painter.setPenAndBrush(kTimingPathTermColor, true);
      bterm_descriptor->highlight(node.bterm, painter);
    }

    if (node.is_driver && i + 1 < nodes.size()) {
      odb::dbNet* net = netOf(node);
      const TimingNode& sink = nodes[i + 1];
      if (net != nullptr && netOf(sink) == net) {
        painter.setPenAndBrush(
            node.is_clock ? clock_color : kTimingPathSignalColor, true);
        net_descriptor->highlight(
            DbNetDescriptor::NetWithSink{.net = net, .sink = pinOf(sink)},
            painter);
      }
    }
  }
}

}  // namespace

void paintTimingPath(const TimingPathSummary& path,
                     const TimingPathParts& parts,
                     Painter& painter)
{
  paintNodes(path.capture_nodes,
             kTimingPathCaptureClockColor,
             parts.capture_clock,
             parts.capture_clock,
             painter);
  paintNodes(path.data_nodes,
             kTimingPathClockColor,
             parts.launch_clock,
             parts.data,
             painter);
}

}  // namespace web
