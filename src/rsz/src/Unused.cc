/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2019, OpenROAD
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

// Resizer code on injured reserve.

// K means clustering algorithm.
void
Resizer::groupLoadsCluster(Pin *drvr_pin,
                           int group_count,
                           int group_size,
                           // Return value.
                           GroupedPins &grouped_loads)
{
  Vector<Point> centers;
  grouped_loads.resize(group_count);
  centers.resize(group_count);

  PinSeq loads;
  findLoads(drvr_pin, loads);
  // Find the bbox of the loads.
  Rect bbox;
  bbox.mergeInit();
  for (Pin *load : loads) {
    Point loc = pinLocation(load, db_network_);
    Rect r(loc.x(), loc.y(), loc.x(), loc.y());
    bbox.merge(r);
  }
  // Choose initial centers on bbox diagonal.
  for(int i = 0; i < group_count; i++) {
    Point center(bbox.xMin() + i * bbox.dx() / group_count,
                    bbox.yMin() + i * bbox.dy() / group_count);
    centers[i] = center;
  }

  for (int j = 0; j < 10; j++) {
    for (int i = 0; i < group_count; i++)
      grouped_loads[i].clear();

    // Assign each load to the group with the nearest center.
    for (Pin *load : loads) {
      Point loc = pinLocation(load, db_network_);
      int64_t min_dist2 = std::numeric_limits<int64_t>::max();
      int min_index = 0;
      for(int i = 0; i < group_count; i++) {
        Point center = centers[i];
        int64_t dist2 = Point::squaredDistance(loc, center);
        if (dist2 < min_dist2) {
          min_dist2 = dist2;
          min_index = i;
        }
      }
      grouped_loads[min_index].push_back(load);
    }

    // Find the center of each group.
    for (int i = 0; i < group_count; i++) {
      Vector<Pin*> &loads = grouped_loads[i];
      Point center = findCenter(loads);
      centers[i] = center;
    }
  }
}

////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////

void
repair_max_fanout_cmd(LibertyCell *buffer_cell)
{
  ensureLinked();
  Resizer *resizer = getResizer();
  resizer->repairMaxFanout(buffer_cell);
}

void
Resizer::repairMaxFanout(LibertyCell *buffer_cell)
{
  int max_fanout_violation_count = 0;
  inserted_buffer_count_ = 0;

  init();
  sta_->checkFanoutLimitPreamble();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    Pin *drvr_pin = vertex->pin();
    Net *net = network_->net(drvr_pin);
    // Hands off the clock tree.
    if (!network_->isTopLevelPort(drvr_pin)
        && !isClock(net)
        // Exclude tie hi/low cells.
        && !isFuncOneZero(drvr_pin)
        && net
        && !isSpecial(net)) {
      float fanout, max_fanout, slack;
      sta_->checkFanout(drvr_pin, MinMax::max(),
                        fanout, max_fanout, slack);
      if (slack < 0.0) {
        max_fanout_violation_count++;
        int buffer_count = ceil(fanout / max_fanout);
        int buffer_fanout = ceil(fanout / static_cast<double>(buffer_count));
        bufferLoads(drvr_pin, buffer_count, buffer_fanout,
                    buffer_cell, "max_fanout");
        if (overMaxArea()) {
          warn("max utilization reached.");
          break;
        }
      }
    }
  }

  if (max_fanout_violation_count > 0)
    printf("Found %d max fanout violations.\n", max_fanout_violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers.\n", inserted_buffer_count_);
    level_drvr_verticies_valid_ = false;
  }
}

////////////////////////////////////////////////////////////////

// This does not work well because downsizing the gate can lower the
// path delay by reducing the load capacitance on an earlier stage.
void
Resizer::repairHoldResize(Pin *drvr_pin,
                          Slack hold_slack)
{
  Instance *inst = network_->instance(drvr_pin);
  LibertyCell *inst_cell = network_->libertyCell(inst);
  LibertyCellSeq *equiv_cells = sta_->equivCells(inst_cell);
  bool found_cell = false;
  int i;
  for (i = 0; i < equiv_cells->size(); i++) {
    LibertyCell *equiv = (*equiv_cells)[i];
    if (equiv == inst_cell) {
      found_cell = true;
      break;
    }
  }
  // Equiv cells are sorted by drive strength.
  // If inst_cell is the first equiv it is already the lowest drive option.
  if (found_cell && i > 0) {
    double load_cap = graph_delay_calc_->loadCap(drvr_pin, dcalc_ap_);
    double drive_res = cellDriveResistance(inst_cell);
    double rc_delay0 = drive_res * load_cap;
    // Downsize until RC delay > hold violation.
    for (int k = i - 1; k >= 0; k--) {
      LibertyCell *equiv = (*equiv_cells)[k];
      double drive_res = cellDriveResistance(equiv);
      double rc_delay = drive_res * load_cap;
      if (rc_delay - rc_delay0 > -hold_slack
          // Last chance baby.
          || k == 0) {
        debugPrint4(debug_, "resizer", 3, "%s %s -> %s +%s\n",
                    sdc_network_->pathName(inst),
                    inst_cell->name(),
                    equiv->name(),
                    delayAsString(rc_delay - rc_delay0, this));
        sta_->replaceCell(inst, equiv);
        break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////

// Assumes resizePreamble has been called.
void
Resizer::repairMaxCap(LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  int repaired_net_count = 0;
  int violation_count = 0;

  sta_->checkCapacitanceLimitPreamble();
  sta_->findDelays();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    Pin *drvr_pin = vertex->pin();
    // Hands off the clock tree.
    Net *net = network_->net(drvr_pin);
    if (net
        && !isClock(net)
        // Exclude tie hi/low cells.
        && !isFuncOneZero(drvr_pin)
        // Hands off special nets.
        && !isSpecial(net)) {
      const Corner *ignore_corner;
      const RiseFall *ignore_rf;
      float cap, limit, slack;
      sta_->checkCapacitance(drvr_pin, corner_, MinMax::max(), 
                             ignore_corner, ignore_rf, cap, limit, slack);
      if (slack < 0.0 && limit > 0.0) {
        violation_count++;
        repaired_net_count++;
        Pin *drvr_pin = vertex->pin();
        float limit_ratio = cap / limit;
        int buffer_count = ceil(limit_ratio);
        int buffer_fanout = ceil(fanout(drvr_pin) / static_cast<double>(buffer_count));
        bufferLoads(drvr_pin, buffer_count, buffer_fanout,
                    buffer_cell, "max_cap");
        if (overMaxArea()) {
          warn("max utilization reached.");
          break;
        }
      }
    }
  }
  
  if (violation_count > 0)
    printf("Found %d max capacitance violations.\n", violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
           inserted_buffer_count_,
           repaired_net_count);
    level_drvr_verticies_valid_ = false;
  }
}

void
Resizer::repairMaxSlew(LibertyCell *buffer_cell)
{
  inserted_buffer_count_ = 0;
  int repaired_net_count = 0;
  int violation_count = 0;

  sta_->findDelays();
  sta_->checkSlewLimitPreamble();
  // Rebuffer in reverse level order.
  for (int i = level_drvr_verticies_.size() - 1; i >= 0; i--) {
    Vertex *vertex = level_drvr_verticies_[i];
    const Pin *drvr_pin = vertex->pin();
    Net *net = network_->net(vertex->pin());
    if (net
        // Hands off the clock tree.
        && !isClock(net)
        // Exclude tie hi/low cells.
        && !isFuncOneZero(drvr_pin)
        // Hands off special nets.
        && !isSpecial(net)) {
      // Max slew can be specified for input and output pin in liberty
      // so all the pins have to be checked.
      bool violation = false;
      float limit_ratio;
      NetPinIterator *pin_iter = network_->pinIterator(net);
      while (pin_iter->hasNext()) {
        Pin *pin = pin_iter->next();
        const Corner *ignore_corner;
        const RiseFall *ignore_rf;
        Slew slew;
        float limit, slack;
        sta_->checkSlew(pin, corner_, MinMax::max(), false,
                        ignore_corner, ignore_rf, slew, limit, slack);
        if (slack < 0.0 && limit > 0.0) {
          violation = true;
          limit_ratio = slew / limit;
          violation_count++;
          break;
        }
      }
      delete pin_iter;
      if (violation) {
        Pin *drvr_pin = vertex->pin();
        int buffer_count = ceil(limit_ratio);
        int buffer_fanout = ceil(fanout(drvr_pin) / static_cast<double>(buffer_count));
        bufferLoads(drvr_pin, buffer_count, buffer_fanout, buffer_cell, "max_slew");
        repaired_net_count++;
        if (overMaxArea()) {
          warn("max utilization reached.");
          break;
        }
      }
    }
  }
  
  if (violation_count > 0)
    printf("Found %d max slew violations.\n", violation_count);
  if (inserted_buffer_count_ > 0) {
    printf("Inserted %d buffers in %d nets.\n",
           inserted_buffer_count_,
           repaired_net_count);
    level_drvr_verticies_valid_ = false;
  }
}

void
Resizer::bufferLoads(Pin *drvr_pin,
                     int buffer_count,
                     int max_fanout,
                     LibertyCell *buffer_cell,
                     const char *reason)
{
  debugPrint1(debug_, "buffer_loads", 2, "driver %s\n",
              sdc_network_->pathName(drvr_pin));
  GroupedPins grouped_loads;
  groupLoadsSteiner(drvr_pin, buffer_count, max_fanout, grouped_loads);
  //reportGroupedLoads(grouped_loads);
  Vector<Instance*> buffers(buffer_count);
  Net *net = network_->net(drvr_pin);
  Instance *top_inst = db_network_->topInstance();
  LibertyPort *buffer_in, *buffer_out;
  buffer_cell->bufferPorts(buffer_in, buffer_out);
  Point drvr_loc = db_network_->location(drvr_pin);
  for (int i = 0; i < buffer_count; i++) {
    PinSeq &loads = grouped_loads[i];
    if (loads.size()) {
      Point center = findCenter(loads);
      string load_net_name = makeUniqueNetName();
      Net *load_net = db_network_->makeNet(load_net_name.c_str(), top_inst);
      string inst_name = makeUniqueInstName(reason);
      Instance *buffer = db_network_->makeInstance(buffer_cell,
                                                   inst_name.c_str(),
                                                   top_inst);
      setLocation(buffer, Point((drvr_loc.getX() + center.getX()) / 2,
                                (drvr_loc.getX() + center.getY()) / 2));
      inserted_buffer_count_++;
      design_area_ += area(db_network_->cell(buffer_cell));

      sta_->connectPin(buffer, buffer_in, net);
      sta_->connectPin(buffer, buffer_out, load_net);

      for (Pin *load : loads) {
        Instance *load_inst = network_->instance(load);
        Port *load_port = network_->port(load);
        sta_->disconnectPin(load);
        sta_->connectPin(load_inst, load_port, load_net);
      }
      if (have_estimated_parasitics_)
        estimateWireParasitic(load_net);
    }
  }
  if (have_estimated_parasitics_)
    estimateWireParasitic(net);
}

void
Resizer::groupLoadsSteiner(Pin *drvr_pin,
                           int group_count,
                           int group_size,
                           // Return value.
                           GroupedPins &grouped_loads)
{
  Net* net = sdc_network_->net(drvr_pin);
  SteinerTree *tree = makeSteinerTree(net, true, db_network_);
  grouped_loads.resize(group_count);
  if (tree) {
    SteinerPt drvr_pt = tree->drvrPt(db_network_);
    int group_index = 0;
    groupLoadsSteiner(tree, drvr_pt, group_size, group_index, grouped_loads);
  }
  else {
    PinSeq loads;
    findLoads(drvr_pin, loads);
    int group_index = 0;
    for (Pin *load : loads) {
      Vector<Pin*> &loads = grouped_loads[group_index];
      loads.push_back(load);
      if (loads.size() == group_size)
        group_index++;
    }
  }
}

// DFS of steiner tree collecting leaf pins into groups.
void
Resizer::groupLoadsSteiner(SteinerTree *tree,
                           SteinerPt pt,
                           int group_size,
                           int &group_index,
                           GroupedPins &grouped_loads)
{
  Pin *pin = tree->pin(pt);
  if (pin && db_network_->isLoad(pin)) {
    Vector<Pin*> &loads = grouped_loads[group_index];
    loads.push_back(pin);
    if (loads.size() == group_size)
      group_index++;
  }
  else {
    SteinerPt left = tree->left(pt);
    if (left != SteinerTree::null_pt)
      groupLoadsSteiner(tree, left, group_size, group_index, grouped_loads);
    SteinerPt right = tree->right(pt);
    if (right != SteinerTree::null_pt)
      groupLoadsSteiner(tree, right, group_size, group_index, grouped_loads);
  }
}

void
Resizer::reportGroupedLoads(GroupedPins &grouped_loads)
{
  int i = 0;
  for (Vector<Pin*> &loads : grouped_loads) {
    if (!loads.empty()) {
      printf("Group %d %lu members\n", i, loads.size());
      Point center = findCenter(loads);
      double max_dist = 0.0;
      double sum_dist = 0.0;
      for (Pin *load : loads) {
        uint64_t dist2 = Point::squaredDistance(db_network_->location(load),
                                                center);
        double dist = std::sqrt(dist2);
        printf(" %.2e %s\n", dbuToMeters(dist), db_network_->pathName(load));
        sum_dist += dist;
        if (dist > max_dist)
          max_dist = dist;
      }
      double avg_dist = std::sqrt(sum_dist / loads.size());
      printf(" avg dist %.2e\n", dbuToMeters(avg_dist));
      printf(" max dist %.2e\n", dbuToMeters(max_dist));
      i++;
    }
  }
}

Point
Resizer::findCenter(PinSeq &pins)
{
  Point sum(0, 0);
  for (Pin *pin : pins) {
    Point loc = db_network_->location(pin);
    sum.x() += loc.x();
    sum.y() += loc.y();
  }
  return Point(sum.x() / pins.size(), sum.y() / pins.size());
}

void
Resizer::findLoads(Pin *drvr_pin,
                   PinSeq &loads)
{
  PinSeq drvrs;
  PinSet visited_drvrs;
  FindNetDrvrLoads visitor(drvr_pin, visited_drvrs, loads, drvrs, network_);
  network_->visitConnectedPins(drvr_pin, visitor);
}

Point
Resizer::closestPt(Pin *pin,
                   PinSeq &pins)
{
  Point pin_loc = db_network_->location(pin);
  int closest_dist = std::numeric_limits<int>::max();
  Point closest_loc;
  for (Pin *pin1 : pins) {
    Point loc1 = db_network_->location(pin1);
    int dist = Point::manhattanDistance(loc, loc1);
    if (dist < closest_dist) {
      closest_dist = dist;
      closest_loc = load_loc;
    }
  }
  return closest_loc;
}

////////////////////////////////////////////////////////////////

void
Resizer::repairTiming()
{
  init();
  Slack worst_slack;
  Vertex *worst_vertex;
  sta_->worstSlack(MinMax::max(), worst_slack, worst_vertex);
  PathRef worst_path;
  sta_->vertexWorstSlackPath(worst_vertex, MinMax::max(), worst_path);
  PathExpanded worst_expanded(&worst_path, sta_);
  if (worst_expanded.size() > 1) {
    // slack_room says how much slack can be taken out at a vertex before
    // another tributary downstream becomes the worst path.
    int worst_length = worst_expanded.size();
    vector<Slack> slack_room(worst_length);
    vector<Delay> arc_delays(worst_length);
    int end_index = worst_length - 1;
    PathRef *path = worst_expanded.path(end_index);
    Slack downstream_room = INF;
    slack_room[end_index] = downstream_room;
    for (int i = worst_length - 2; i >= worst_expanded.startIndex(); i--) {
      PathRef *prev_path = worst_expanded.path(i);
      Vertex *path_vertex = path->vertex(sta_);
      const RiseFall *path_rf = path->transition(sta_);
      Vertex *prev_vertex = prev_path ? prev_path->vertex(sta_) : nullptr;
      slack_room[i] = downstream_room;
      arc_delays[i + 1] = path->arrival(sta_) - prev_path->arrival(sta_);
      if (path_vertex->isDriver(network_)) {
        printf("%s\n", path_vertex->name(network_));
        VertexInEdgeIterator edge_iter(path_vertex, graph_);
        while (edge_iter.hasNext()) {
          Edge *edge = edge_iter.next();
          Vertex *fanin_vertex = edge->from(graph_);
          if (fanin_vertex != prev_vertex
              && !search_->isClock(fanin_vertex)) {
            TimingArcSet *arc_set = edge->timingArcSet();
            for (TimingArc *arc : arc_set->arcs()) {
              if (arc->toTrans()->asRiseFall() == path_rf) {
                const RiseFall *fanin_rf = arc->fromTrans()->asRiseFall();
                Slack fanin_slack = sta_->vertexSlack(fanin_vertex, fanin_rf,
                                                      MinMax::max());
                Slack room = fanin_slack - worst_slack;
#if 1
                printf(" %s room = %s\n",
                       fanin_vertex->name(network_),
                       units_->timeUnit()->asString(room, 3));
#endif
                downstream_room = min(downstream_room, room);
              }
            }
          }
        }
      }
      path = prev_path;
    }
#if 1
    for (int i = worst_expanded.startIndex(); i < worst_length; i++) {
      PathRef *path = worst_expanded.path(i);
      Vertex *path_vertex = path->vertex(sta_);
      printf("%s delay = %s room = %s\n",
             path_vertex->name(network_),
             units_->timeUnit()->asString(arc_delays[i], 3),
             units_->timeUnit()->asString(slack_room[i], 3));
    }
#endif
    vector<pair<int, Delay>> sorted_arc_delays;
    for (int i = worst_expanded.startIndex(); i < worst_length; i++)
      sorted_arc_delays.push_back(pair(i, arc_delays[i]));
    sort(sorted_arc_delays.begin(), sorted_arc_delays.end(),
         [](pair<int, Delay> pair1,
            pair<int, Delay> pair2) {
           return pair1.second > pair2.second;
         });
    printf("sorted arcs\n");
    for (auto index_delay : sorted_arc_delays) {
      int i = index_delay.first;
      PathRef *path = worst_expanded.path(i);
      Vertex *path_vertex = path->vertex(sta_);
      printf("%3d %s delay = %s room = %s\n",
             i,
             path_vertex->name(network_),
             units_->timeUnit()->asString(arc_delays[i], 3),
             units_->timeUnit()->asString(slack_room[i], 3));
    }
  }
}

////////////////////////////////////////////////////////////////

  float findBufferMinCap(LibertyLibrarySeq *resize_libs);
  LibertyCell *findMinDelayBuffer(LibertyLibrarySeq *resize_libs,
                                  float cap);

void
Resizer::findBuffers(LibertyLibrarySeq *resize_libs)
{
  LibertyCellSet keepers;
  float cap = findBufferMinCap(resize_libs);
  for (int i = 0; i < 15; i++) {
    LibertyCell *buffer = findMinDelayBuffer(resize_libs, cap);
    keepers.insert(buffer);
    cap *= 1.5;
  }
  printf("saving\n");
  for (LibertyCell *buffer : keepers)
    printf("%s\n", buffer->name());
}

LibertyCell *
Resizer::findMinDelayBuffer(LibertyLibrarySeq *resize_libs,
                            float cap)
{
  float min_delay = INF;
  LibertyCell *min_buffer = nullptr;
  for (LibertyLibrary *lib : *resize_libs) {
    for (LibertyCell *buffer : *lib->buffers()) {
      if (!dontUse(buffer)) {
        LibertyPort *input, *output;
        buffer->bufferPorts(input, output);
        float delay = bufferDelay(buffer, cap) - input->intrinsicDelay();
        printf("%s %s %s\n",
               buffer->name(),
               units_->capacitanceUnit()->asString(cap),
               delayAsString(delay, sta_, 5));
        if (delay < min_delay) {
          min_buffer = buffer;
          min_delay = delay;
        }
      }
    }
  }
  return min_buffer;
}

float
Resizer::findBufferMinCap(LibertyLibrarySeq *resize_libs)
{
  float min_cap = INF;
  for (LibertyLibrary *lib : *resize_libs) {
    for (LibertyCell *buffer : *lib->buffers()) {
      LibertyPort *input, *output;
      buffer->bufferPorts(input, output);
      float cap = input->capacitance();
      min_cap = min(cap, min_cap);
    }
  }
  return min_cap;
}

////////////////////////////////////////////////////////////////

static void
highlightPdRevGraph(PD::PdRev *pdrev,
                    gui::Gui *gui);

void
highlightPdrevTree(const Pin *drvr_pin,
                   float alpha,
                   bool use_pd)
{
  gui::Gui *gui = gui::Gui::get();
  if (drvr_pin) {
    ord::OpenRoad *openroad = ord::OpenRoad::openRoad();
    sta::dbNetwork *network = openroad->getDbNetwork();
    utl::Logger *logger = openroad->getLogger();
    Net *net = network->isTopLevelPort(drvr_pin)
      ? network->net(network->term(drvr_pin))
      : network->net(drvr_pin);
    PinSeq pins;
    connectedPins(net, network, pins);
    int pin_count = pins.size();
    bool is_placed = true;
    if (pin_count >= 2) {
      int *x = new int[pin_count];
      int *y = new int[pin_count];
      int drvr_idx = 0;
      bool found_drvr = false;
      for (int i = 0; i < pin_count; i++) {
        Pin *pin = pins[i];
        if (pin == drvr_pin)
          drvr_idx = i;
        Point loc = network->location(pin);
        x[i] = loc.x();
        y[i] = loc.y();
        is_placed &= network->isPlaced(pin);
      }
      if (is_placed) {
        // Move the driver to the pole position.
        std::swap(pins[0], pins[drvr_idx]);
        std::swap(x[0], x[drvr_idx]);
        std::swap(y[0], y[drvr_idx]);

        PD::PdRev* pd = new PD::PdRev(logger);
        std::vector<unsigned> vec_x(x, x + pin_count);
        std::vector<unsigned> vec_y(y, y + pin_count);
        // Must be called before addNet.
        pd->setAlphaPDII(alpha);
        pd->addNet(vec_x, vec_y);
        if (use_pd)
          pd->runPD(alpha);
        else
          pd->runPDII();
        highlightPdRevGraph(pd, gui::Gui::get());
        delete pd;
      }
    }
  }
  else
    highlightPdRevGraph(nullptr, gui);
}

static void
highlightPdRevGraph(PD::PdRev *pdrev,
                    gui::Gui *gui)
{
  if (gui) {
    if (lines_renderer == nullptr) {
      lines_renderer = new LinesRenderer();
      gui->registerRenderer(lines_renderer);
    }
    std::vector<std::pair<std::pair<int, int>, std::pair<int, int>>> xy_lines;
    if (pdrev)
      pdrev->graphLines(xy_lines);
    std::vector<std::pair<odb::Point, odb::Point>> lines;
    for (int i = 0; i < xy_lines.size(); i++) {
      std::pair<int, int> xy1 = xy_lines[i].first;
      std::pair<int, int> xy2 = xy_lines[i].second;
      lines.push_back(std::pair(odb::Point(xy1.first, xy1.second),
                                odb::Point(xy2.first, xy2.second)));
    }
    lines_renderer->highlight(lines, gui::Painter::red);
  }
}

void
highlight_pd_tree(const Pin *drvr_pin,
                  float alpha)
{
  highlightPdrevTree(drvr_pin, alpha, true);
}

void
highlight_pdII_tree(const Pin *drvr_pin,
                    float alpha)
{
  highlightPdrevTree(drvr_pin, alpha, false);
}
