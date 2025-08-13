// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

#include "FastRoute.h"

namespace grt {

void Graph2D::init(const int x_grid,
                   const int y_grid,
                   const int h_capacity,
                   const int v_capacity,
                   const int num_layers,
                   utl::Logger* logger)
{
  x_grid_ = x_grid;
  y_grid_ = y_grid;
  num_layers_ = num_layers;
  logger_ = logger;

  h_edges_.resize(boost::extents[x_grid - 1][y_grid]);
  v_edges_.resize(boost::extents[x_grid][y_grid - 1]);

  for (int x = 0; x < x_grid - 1; x++) {
    for (int y = 0; y < y_grid; y++) {
      // Edge initialization
      h_edges_[x][y].cap = h_capacity;
      h_edges_[x][y].usage = 0;
      h_edges_[x][y].est_usage = 0;
      h_edges_[x][y].red = 0;
      h_edges_[x][y].last_usage = 0;
      h_edges_[x][y].num_ndrs = 0;
    }
  }
  for (int x = 0; x < x_grid; x++) {
    for (int y = 0; y < y_grid - 1; y++) {
      // Edge initialization
      v_edges_[x][y].cap = v_capacity;
      v_edges_[x][y].usage = 0;
      v_edges_[x][y].est_usage = 0;
      v_edges_[x][y].red = 0;
      v_edges_[x][y].last_usage = 0;
      v_edges_[x][y].num_ndrs = 0;
    }
  }
}

void Graph2D::InitEstUsage()
{
  foreachEdge([](Edge& edge) { edge.est_usage = 0; });
}

void Graph2D::InitLastUsage(const int upType)
{
  foreachEdge([](Edge& edge) { edge.last_usage = 0; });

  if (upType == 1) {
    foreachEdge([](Edge& edge) { edge.congCNT = 0; });
  } else if (upType == 2) {
    foreachEdge([](Edge& edge) { edge.last_usage = edge.last_usage * 0.2; });
  }
}

void Graph2D::clear()
{
  h_edges_.resize(boost::extents[0][0]);
  v_edges_.resize(boost::extents[0][0]);
}

void Graph2D::clearUsed()
{
  v_used_ggrid_.clear();
  h_used_ggrid_.clear();
}

bool Graph2D::hasEdges() const
{
  return !h_edges_.empty() && !v_edges_.empty();
}

uint16_t Graph2D::getUsageH(const int x, const int y) const
{
  return h_edges_[x][y].usage;
}

uint16_t Graph2D::getUsageV(const int x, const int y) const
{
  return v_edges_[x][y].usage;
}

int16_t Graph2D::getLastUsageH(int x, int y) const
{
  return h_edges_[x][y].last_usage;
}

int16_t Graph2D::getLastUsageV(int x, int y) const
{
  return v_edges_[x][y].last_usage;
}

double Graph2D::getEstUsageH(const int x, const int y) const
{
  return h_edges_[x][y].est_usage;
}

double Graph2D::getEstUsageV(const int x, const int y) const
{
  return v_edges_[x][y].est_usage;
}

uint16_t Graph2D::getUsageRedH(const int x, const int y) const
{
  return h_edges_[x][y].usage_red();
}

uint16_t Graph2D::getUsageRedV(const int x, const int y) const
{
  return v_edges_[x][y].usage_red();
}

double Graph2D::getEstUsageRedH(const int x, const int y) const
{
  return h_edges_[x][y].est_usage_red();
}

double Graph2D::getEstUsageRedV(const int x, const int y) const
{
  return v_edges_[x][y].est_usage_red();
}

int Graph2D::getOverflowH(const int x, const int y) const
{
  const auto& edge = h_edges_[x][y];
  return edge.usage - edge.cap;
}

int Graph2D::getOverflowV(const int x, const int y) const
{
  const auto& edge = v_edges_[x][y];
  return edge.usage - edge.cap;
}

uint16_t Graph2D::getCapH(int x, int y) const
{
  return h_edges_[x][y].cap;
}

uint16_t Graph2D::getCapV(int x, int y) const
{
  return v_edges_[x][y].cap;
}

const std::set<std::pair<int, int>>& Graph2D::getUsedGridsH() const
{
  return h_used_ggrid_;
}

const std::set<std::pair<int, int>>& Graph2D::getUsedGridsV() const
{
  return v_used_ggrid_;
}

void Graph2D::addCapH(const int x, const int y, const int cap)
{
  h_edges_[x][y].cap += cap;
}

void Graph2D::addCapV(const int x, const int y, const int cap)
{
  v_edges_[x][y].cap += cap;
}

void Graph2D::addEstUsageH(const Interval& xi, const int y, const double usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    h_edges_[x][y].est_usage += usage;
    if (usage > 0) {
      h_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::updateEstUsageH(const Interval& xi, const int y, FrNet* net, const double usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    updateEstUsageH(x, y, net, usage);
  }
}

void Graph2D::updateEstUsageH(const int x, const int y, FrNet* net, const double usage)
{
  if(x==39 && y==61){
    logger_->report("Before Underflow {} {} est {} num {}",net->getName(), usage, h_edges_[x][y].est_usage, h_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  h_edges_[x][y].est_usage += getCostNDRAware(net, x, y, usage, EdgeDirection::Horizontal);
  updateNDRCapLayer(x, y, net, EdgeDirection::Horizontal, usage);
  if(x==39 && y==61){
    logger_->report("Underflow {} {} est {} num {}",net->getName(), usage, h_edges_[x][y].est_usage, h_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageH(const int x, const int y, const double usage)
{
  h_edges_[x][y].est_usage += usage;
  if (usage > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageToUsage()
{
  foreachEdge([](Edge& edge) { edge.usage += edge.est_usage; });
}

void Graph2D::addEstUsageV(const int x, const Interval& yi, const double usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    v_edges_[x][y].est_usage += usage;
    if (usage > 0) {
      v_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::updateEstUsageV(const int x, const Interval& yi, FrNet* net, const double usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    updateEstUsageV(x, y, net, usage);
  }
}

void Graph2D::updateEstUsageV(const int x, const int y, FrNet* net, const double usage)
{
  if(x==39 && y==61){
    logger_->report("Before Underflow {} {} {} {}",net->getName(), usage, v_edges_[x][y].est_usage, v_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  v_edges_[x][y].est_usage += getCostNDRAware(net, x, y, usage, EdgeDirection::Vertical);
  updateNDRCapLayer(x, y, net, EdgeDirection::Vertical, usage);
  if(x==39 && y==61){
    logger_->report("Underflow {} {} {} {}",net->getName(), usage, v_edges_[x][y].est_usage, v_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }

  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addEstUsageV(const int x, const int y, const double usage)
{
  v_edges_[x][y].est_usage += usage;
  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::addRedH(const int x, const int y, const int red)
{
  auto& val = h_edges_[x][y].red;
  val = std::max(val + red, 0);
}

void Graph2D::addRedV(const int x, const int y, const int red)
{
  auto& val = v_edges_[x][y].red;
  val = std::max(val + red, 0);
}

void Graph2D::addUsageH(const Interval& xi, const int y, const int usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    h_edges_[x][y].usage += usage;
    if (usage > 0) {
      h_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addUsageV(const int x, const Interval& yi, const int usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    v_edges_[x][y].usage += usage;
    if (usage > 0) {
      v_used_ggrid_.insert({x, y});
    }
  }
}

void Graph2D::addUsageH(const int x, const int y, const int usage)
{
  h_edges_[x][y].usage += usage;
  if (usage > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageH(const int x, const int y, FrNet* net, const int usage)
{
  if(x==39 && y==61){
    logger_->report("Before U Underflow {} {} {} {}",net->getName(), usage, h_edges_[x][y].usage, h_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  h_edges_[x][y].usage += getCostNDRAware(net, x, y, usage, EdgeDirection::Horizontal);
  updateNDRCapLayer(x, y, net, EdgeDirection::Horizontal, usage);
  if(x==39 && y==61){
    logger_->report("U Underflow {} {} {} {}",net->getName(), usage, h_edges_[x][y].usage, h_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  if (usage > 0) {
    h_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageH(const Interval& xi, const int y, FrNet* net, const int usage)
{
  for (int x = xi.lo; x < xi.hi; x++) {
    updateUsageH(x, y, net, usage);
  }
}

void Graph2D::addUsageV(const int x, const int y, const int usage)
{
  v_edges_[x][y].usage += usage;
  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageV(const int x, const int y, FrNet* net, const int usage)
{
  if(x==39 && y==61){
    logger_->report("U Before Underflow {} {} {} {}",net->getName(), usage, v_edges_[x][y].usage, v_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  v_edges_[x][y].usage += getCostNDRAware(net, x, y, usage, EdgeDirection::Vertical);
  updateNDRCapLayer(x, y, net, EdgeDirection::Vertical, usage);
  if(x==39 && y==61){
    logger_->report("U Underflow {} {} {} {}",net->getName(), usage, v_edges_[x][y].usage, v_edges_[x][y].num_ndrs);
    printNDRCap(x,y);
  }
  if (usage > 0) {
    v_used_ggrid_.insert({x, y});
  }
}

void Graph2D::updateUsageV(const int x, const Interval& yi, FrNet* net, const int usage)
{
  for (int y = yi.lo; y < yi.hi; y++) {
    updateUsageV(x, y, net, usage);
  }
}

/*
 * num_iteration : the total number of iterations for maze route to run
 * round : the number of maze route stages runned
 */

void Graph2D::updateCongestionHistory(const int up_type,
                                      const int ahth,
                                      bool stop_decreasing,
                                      int& max_adj)
{
  int maxlimit = 0;

  if (up_type == 2) {
    stop_decreasing = max_adj < ahth;
  }

  auto updateEdges = [&](const auto& grid, auto& edges) {
    for (const auto& [x, y] : grid) {
      const int overflow = edges[x][y].usage - edges[x][y].cap;
      if (overflow > 0) {
        edges[x][y].congCNT++;
        edges[x][y].last_usage += overflow;
      } else if (!stop_decreasing) {
        if (up_type != 1) {
          edges[x][y].congCNT = std::max<int>(0, edges[x][y].congCNT - 1);
        }
        if (up_type != 3) {
          edges[x][y].last_usage *= 0.9;
        } else {
          edges[x][y].last_usage
              = std::max<int>(edges[x][y].last_usage + overflow, 0);
        }
      }
      maxlimit = std::max<int>(maxlimit, edges[x][y].last_usage);
    }
  };

  updateEdges(h_used_ggrid_, h_edges_);
  updateEdges(v_used_ggrid_, v_edges_);

  max_adj = maxlimit;
}

void Graph2D::str_accu(const int rnd)
{
  foreachEdge([rnd](Edge& edge) {
    const int overflow = edge.usage - edge.cap;
    if (overflow > 0 || edge.congCNT > rnd) {
      edge.last_usage += edge.congCNT * overflow / 2;
    }
  });
}

void Graph2D::foreachEdge(const std::function<void(Edge&)>& func)
{
  auto inner = [&](auto& edges) {
    Edge* edges_data = edges.data();

    const size_t num_edges = edges.num_elements();

    for (size_t i = 0; i < num_edges; ++i) {
      func(edges_data[i]);
    }
  };
  inner(h_edges_);
  inner(v_edges_);
}

void Graph2D::initCap3D()
{
  _v_cap_3D_.resize(boost::extents[num_layers_][x_grid_][y_grid_]);
  _h_cap_3D_.resize(boost::extents[num_layers_][x_grid_][y_grid_]);
}

void Graph2D::updateCap3D(int x, int y, int layer, EdgeDirection direction, const double cap)
{
  if (direction == EdgeDirection::Horizontal){
    _h_cap_3D_[layer][x][y].cap = cap;
    _h_cap_3D_[layer][x][y].cap_ndr = cap;
  }else {
    _v_cap_3D_[layer][x][y].cap = cap;
    _v_cap_3D_[layer][x][y].cap_ndr = cap;
  }
}

void Graph2D::printEdgeCapPerLayer()
{
  logger_->report("=== printEdgeCapPerLayer ===");
  for (int y = 0; y < y_grid_; y++) {
    for (int x = 0; x < x_grid_; x++) {
      for (int l = 0; l < num_layers_; l++) {
        if (x < x_grid_ - 1) {
          logger_->report("\tH x{} y{} l{}: {}",x,y,l,_h_cap_3D_[l][x][y].cap);
        }
        if (y < y_grid_ - 1) {
          logger_->report("\tV x{} y{} l{}: {}",x,y,l,_v_cap_3D_[l][x][y].cap);
        }
      }
    }
  }
}

bool Graph2D::hasNDRCapacity(FrNet* net, int x, int y, EdgeDirection direction, double edge_cost)
{
  // Check if this is an NDR net
  bool is_ndr = (net->getDbNet()->getNonDefaultRule() != nullptr);
  // const int edgeCost = net->getEdgeCost();
  double max_single_layer_cap = 0;
  
  // if (is_ndr) {
  if (edge_cost > 1) {
      // For NDR nets, we need at least one layer with sufficient capacity
      for (int l = net->getMinLayer(); l <= net->getMaxLayer(); l++) {
        double layer_cap = 0;
        if (direction == EdgeDirection::Horizontal) {
            layer_cap = _h_cap_3D_[l][x][y].cap_ndr;
        } else {
            layer_cap = _v_cap_3D_[l][x][y].cap_ndr;
        }
        max_single_layer_cap = std::max(max_single_layer_cap,layer_cap);
        if (layer_cap >= edge_cost) {
          return true; 
        }
      }
      if(x==39 && y==61)
        logger_->report("=== Max Layer Cap: {} V{} x{} y{} max {}",net->getName(), direction==EdgeDirection::Vertical, x, y, max_single_layer_cap);

      // No single layer can accommodate this NDR net
      return false;
  }
  
  return true;
}

double Graph2D::getCostNDRAware(FrNet* net, int x, int y, const double edge_cost, EdgeDirection direction)
{ 
  if(x==39 && y==61){
    logger_->report("getCostNDR {} V{} H{} edge {}",net->getName(), v_edges_[x][y].est_usage, h_edges_[x][y].est_usage, edge_cost);
  }
  // No single layer can accommodate this NDR net
  if (std::abs(edge_cost) > 1 && !hasNDRCapacity(net, x, y, direction, std::abs(edge_cost))) {
    if(x==39 && y==61){
      logger_->report("NoSingleLayer {} Vndrs{} Hndrs{} edge {} V{}",
        net->getName(), v_edges_[x][y].num_ndrs, h_edges_[x][y].num_ndrs, edge_cost, direction==EdgeDirection::Vertical);
    }
    if(edge_cost < 0){ // Rip-up
      if(direction == EdgeDirection::Horizontal){
        if(h_edges_[x][y].num_ndrs == 0)
          return edge_cost;
        h_edges_[x][y].num_ndrs-=1;          
      }else{
        if(v_edges_[x][y].num_ndrs == 0)
          return edge_cost;
        v_edges_[x][y].num_ndrs-=1;          
      }

    } else {
      // Increment number of NDR nets in congestion situation
      if(direction == EdgeDirection::Horizontal)
        h_edges_[x][y].num_ndrs++;
      else
        v_edges_[x][y].num_ndrs++;
    }
    return 100*edge_cost; 
  }
  return edge_cost;
}

// Get cost using integer instead of double starting after convertToMazeroute()
int Graph2D::getCostNDRAware(FrNet* net, int x, int y, const int edge_cost, EdgeDirection direction)
{  
  if(x==39 && y==61){
    logger_->report("getCostNDR {} V{} H{} edge {}",net->getName(), v_edges_[x][y].usage, h_edges_[x][y].usage, edge_cost);
  }
  // No single layer can accommodate this NDR net
  if (std::abs(edge_cost) > 1 && !hasNDRCapacity(net, x, y, direction, std::abs(edge_cost))) {
    if(x==39 && y==61){
      logger_->report("NoSingleLayer {} Vndrs{} Hndrs{} edge {} V{}",
        net->getName(), v_edges_[x][y].num_ndrs, h_edges_[x][y].num_ndrs, edge_cost, direction==EdgeDirection::Vertical);
    }
    if(edge_cost < 0){ // Rip-up
      if(direction == EdgeDirection::Horizontal){
        if(h_edges_[x][y].num_ndrs == 0)
          return edge_cost;
        h_edges_[x][y].num_ndrs-=1;          
      }else{
        if(v_edges_[x][y].num_ndrs == 0)
          return edge_cost;
        v_edges_[x][y].num_ndrs-=1;          
      }

    } else {
      // Increment number of NDR nets in congestion situation
      if(direction == EdgeDirection::Horizontal)
        h_edges_[x][y].num_ndrs++;
      else
        v_edges_[x][y].num_ndrs++;
    }
    return 100*edge_cost; 
  }
  return edge_cost;
}

void Graph2D::printNDRCap(const int x, const int y)
{
  logger_->report("=== PrintNDRCap (x{} y{}) ===", x, y);
  for(int l=0; l < num_layers_; l++)
  {
    logger_->report("\tL{} - H Cap: {} NDR Cap: {} - V Cap: {} NDR Cap: {}", 
        l, _h_cap_3D_[l][x][y].cap, _h_cap_3D_[l][x][y].cap_ndr,  _v_cap_3D_[l][x][y].cap, _v_cap_3D_[l][x][y].cap_ndr);
  }
}

void Graph2D::fixFractionEdgeUsage(const int min_layer, const int max_layer, const int x, const int y, const double edge_cost, EdgeDirection dir)
{
  double tmp = std::abs(edge_cost);
  double cap, cap_ndr;
  for(int l=min_layer; l <= max_layer; l++){ 
    if(dir == EdgeDirection::Horizontal){
      cap_ndr = _h_cap_3D_[l][x][y].cap_ndr;
      cap = _h_cap_3D_[l][x][y].cap;
      if(cap - cap_ndr > 0 && cap - cap_ndr <= tmp){
        tmp -= (cap - cap_ndr);
        _h_cap_3D_[l][x][y].cap_ndr += (cap - cap_ndr);
      } else if(cap - cap_ndr > 0 && cap - cap_ndr >= tmp){
        _h_cap_3D_[l][x][y].cap_ndr += tmp;
        break;
      }
    } else {
      cap_ndr = _v_cap_3D_[l][x][y].cap_ndr;
      cap = _v_cap_3D_[l][x][y].cap;
      if(cap - cap_ndr > 0 && cap - cap_ndr <= tmp){
        tmp -= (cap - cap_ndr);
        _v_cap_3D_[l][x][y].cap_ndr += (cap - cap_ndr);
      } else if(cap - cap_ndr > 0 && cap - cap_ndr >= tmp){
        _v_cap_3D_[l][x][y].cap_ndr += tmp;
        break;
      }
    }
  }
}

void Graph2D::updateNDRCapLayer(const int x, const int y, FrNet* net, EdgeDirection dir, const double edge_cost)
{
  bool is_ndr = net->getDbNet()->getNonDefaultRule() != nullptr;
  double cap_ndr = 0;
  double cap = 0;

  // if(is_ndr && std::abs(edge_cost) > 1){
  if(std::abs(edge_cost) > 1){
    for(int l=net->getMinLayer(); l <= net->getMaxLayer(); l++){  
      if(dir == EdgeDirection::Horizontal){
        cap_ndr =_h_cap_3D_[l][x][y].cap_ndr;
        cap = _h_cap_3D_[l][x][y].cap;
        if(edge_cost < 0){ // Reducing edge usage
          // check available capacity to reduce usage
          if(cap - cap_ndr >= std::abs(edge_cost)){ 
            _h_cap_3D_[l][x][y].cap_ndr -= edge_cost;
            break;
          }
          // Fix edge usage when riping up L route
          if(l == net->getMaxLayer()) 
            fixFractionEdgeUsage(net->getMinLayer(), net->getMaxLayer(), x, y, edge_cost, dir);
        } else {
          // check available capacity to increase usage
          if(cap_ndr >= std::abs(edge_cost)){
            _h_cap_3D_[l][x][y].cap_ndr -= edge_cost;
            break;
          }
          // Overflow (need to update cap_ndr even with congestion)
          if(l == net->getMaxLayer()) 
            _h_cap_3D_[net->getMinLayer()][x][y].cap_ndr -= edge_cost;
        }
      } else {
        cap_ndr =_v_cap_3D_[l][x][y].cap_ndr;
        cap = _v_cap_3D_[l][x][y].cap;
        if(edge_cost < 0){ 
          if(cap - cap_ndr >= std::abs(edge_cost)){ 
            _v_cap_3D_[l][x][y].cap_ndr -= edge_cost;
            break;
          }
          // Fix edge usage when riping up L route
          if(l == net->getMaxLayer()) 
            fixFractionEdgeUsage(net->getMinLayer(), net->getMaxLayer(), x, y, edge_cost, dir);
        } else{
          if(cap_ndr >= std::abs(edge_cost)){ 
            _v_cap_3D_[l][x][y].cap_ndr -= edge_cost;
            break;
          }
          // Overflow (need to update cap_ndr even with congestion)
          if(l == net->getMaxLayer()) 
            _v_cap_3D_[net->getMinLayer()][x][y].cap_ndr -= edge_cost;
        }
      }
    }
  }
  // if(net->getDbNet()->getName()=="gen_tiles\\[0\\].i_tile.i_tile/gen_caches\\[0\\].i_snitch_icache/i_lookup.g_sets\\[0\\].gen_scm.i_tag.CG_CELL_WORD_ITER\\[20\\].CG_Inst.clk_o")
  //   logger_->report("\tAfter cap: {} capNDR: {} {}",cap,cap_ndr,edge_cost);
}

}  // namespace grt
