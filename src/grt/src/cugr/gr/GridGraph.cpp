#include "GridGraph.h"
#include "GRNet.h"

GridGraph::GridGraph(const Design& design, const Parameters& params): libDBU(design.getLibDBU()), parameters(params) {
    gridlines = design.getGridlines();
    nLayers = design.getNumLayers();
    xSize = gridlines[0].size() - 1;
    ySize = gridlines[1].size() - 1;
    
    gridCenters.resize(2);
    for (unsigned dimension = 0; dimension <= 1; dimension++) {
        gridCenters[dimension].resize(gridlines[dimension].size() - 1);
        for (int gridIndex = 0; gridIndex < gridlines[dimension].size() - 1; gridIndex++) {
            gridCenters[dimension][gridIndex] = 
                (gridlines[dimension][gridIndex] + gridlines[dimension][gridIndex + 1]) / 2;
        }
    }
    
    m2_pitch = design.getLayer(1).getPitch();

    layerNames.resize(nLayers);
    layerDirections.resize(nLayers);
    layerMinLengths.resize(nLayers);
    for (int layerIndex = 0; layerIndex < nLayers; layerIndex++) {
        const auto& layer = design.getLayer(layerIndex);
        layerNames[layerIndex] = layer.getName();
        layerDirections[layerIndex] = layer.getDirection();
        layerMinLengths[layerIndex] = layer.getMinLength();
    }
    
    unit_length_wire_cost = design.getUnitLengthWireCost();
    unit_via_cost = design.getUnitViaCost();
    unit_length_short_costs.resize(nLayers);
    for (int layerIndex = 0; layerIndex < nLayers; layerIndex++) {
        unit_length_short_costs[layerIndex] = design.getUnitLengthShortCost(layerIndex);
    }
    
    // Init grid graph edges
    vector<vector<int>> gridTracks(nLayers);
    graphEdges.assign(nLayers, vector<vector<GraphEdge>>(xSize, vector<GraphEdge>(ySize)));
    for (int layerIndex = 0; layerIndex < nLayers; layerIndex++) {
        const MetalLayer& layer = design.getLayer(layerIndex);
        const unsigned direction = layer.getDirection();
        
        const int nGrids = gridlines[1 - direction].size() - 1;
        gridTracks[layerIndex].resize(nGrids);
        for (size_t gridIndex = 0; gridIndex < nGrids; gridIndex++) {
            utils::IntervalT<DBU> locRange(gridlines[1 - direction][gridIndex], gridlines[1 - direction][gridIndex + 1]);
            auto trackRange = layer.rangeSearchTracks(locRange);
            if (trackRange.IsValid()) {
                gridTracks[layerIndex][gridIndex] = trackRange.range() + 1;
                // exclude the track on the higher gridline
                if (gridIndex != nGrids - 1 && layer.getTrackLocation(trackRange.high) == locRange.high) {
                    gridTracks[layerIndex][gridIndex]--;
                }
            } else {
                gridTracks[layerIndex][gridIndex] = 0;
            }
        }
        
        // Initialize edges' capacity to the number of tracks
        if (direction == MetalLayer::V) {
            for (size_t x = 0; x < xSize; x++) {
                CapacityT nTracks = gridTracks[layerIndex][x];
                for (size_t y = 0; y + 1 < ySize; y++) {
                    graphEdges[layerIndex][x][y].capacity = nTracks;
                }
            }
        } else {
            for (size_t y = 0; y < ySize; y++) {
                CapacityT nTracks = gridTracks[layerIndex][y];
                for (size_t x = 0; x + 1 < xSize; x++) {
                    graphEdges[layerIndex][x][y].capacity = nTracks;
                }
            }
        }
    }

    // Deduct obstacles usage for layers EXCEPT Metal 1
    vector<vector<utils::BoxT<DBU>>> obstacles(nLayers);
    design.getAllObstacles(obstacles, true);
    for (int layerIndex = 1; layerIndex < nLayers; layerIndex++) {
        const MetalLayer& layer = design.getLayer(layerIndex);
        unsigned direction = layer.getDirection();
        const int nGrids = gridlines[1 - direction].size() - 1;
        const int nEdges = gridlines[direction].size() - 2;
        DBU minEdgeLength = std::numeric_limits<DBU>::max();
        for (int edgeIndex = 0; edgeIndex < nEdges; edgeIndex++) {
            minEdgeLength = min(minEdgeLength, gridCenters[direction][edgeIndex + 1] - gridCenters[direction][edgeIndex]);
        }
        vector<vector<std::shared_ptr<std::pair<utils::BoxT<DBU>, utils::IntervalT<int>>>>> obstaclesInGrid(nGrids); // obstacle indices sorted in track grids
        // Sort obstacles in track grids
        for (auto& obs : obstacles[layerIndex]) {
            DBU width = min(obs.x.range(), obs.y.range());
            DBU spacing = layer.getParallelSpacing(width, min(minEdgeLength, obs[direction].range())) + layer.getWidth() / 2 - 1;
            utils::PointT<DBU> margin(0, 0);
            margin[1 - direction] = spacing;
            utils::BoxT<DBU> obsBox(
                obs.x.low  - margin.x, obs.y.low  - margin.y, 
                obs.x.high + margin.x, obs.y.high + margin.y
            ); // enlarged obstacle box
            utils::IntervalT<int> trackRange = layer.rangeSearchTracks(obsBox[1 - direction]);
            std::shared_ptr<std::pair<utils::BoxT<DBU>, utils::IntervalT<int>>> obstacle = 
                std::make_shared<std::pair<utils::BoxT<DBU>, utils::IntervalT<int>>>(obsBox, trackRange);
            // Get grid range
            utils::IntervalT<int> gridRange = rangeSearchRows(1 - direction, obsBox[1 - direction]);
            for (int gridIndex = gridRange.low; gridIndex <= gridRange.high; gridIndex++) {
                obstaclesInGrid[gridIndex].push_back(obstacle);
            }
        }
        // Handle each track grid
        utils::IntervalT<int> gridTrackRange;
        for (int gridIndex = 0; gridIndex < nGrids; gridIndex++) {
            if (gridIndex == 0) {
                gridTrackRange.low = 0;
                gridTrackRange.high = gridTracks[layerIndex][gridIndex] - 1;
            } else {
                gridTrackRange.low = gridTrackRange.high + 1;
                gridTrackRange.high += gridTracks[layerIndex][gridIndex];
            }
            if (!gridTrackRange.IsValid()) continue;
            if (obstaclesInGrid[gridIndex].size() == 0) continue;
            vector<vector<std::shared_ptr<std::pair<utils::BoxT<DBU>, utils::IntervalT<int>>>>> obstaclesAtEdge(nEdges);
            for (auto& obstacle : obstaclesInGrid[gridIndex]) {
                utils::IntervalT<int> gridlineRange = rangeSearchGridlines(direction, obstacle->first[direction]);
                utils::IntervalT<int> edgeRange(max(gridlineRange.low - 2, 0), min(gridlineRange.high, nEdges - 1));
                for (int edgeIndex = edgeRange.low; edgeIndex <= edgeRange.high; edgeIndex++) {
                    obstaclesAtEdge[edgeIndex].emplace_back(obstacle);
                }
            }
            for (int edgeIndex = 0; edgeIndex < nEdges; edgeIndex++) {
                if (obstaclesAtEdge[edgeIndex].size() == 0) continue;
                DBU gridline = gridlines[direction][edgeIndex + 1];
                utils::IntervalT<DBU> edgeInterval(gridCenters[direction][edgeIndex], gridCenters[direction][edgeIndex + 1]);
                // Update cpacity
                vector<utils::IntervalT<DBU>> usableIntervals(
                    gridTrackRange.range() + 1, edgeInterval
                );
                for (auto& obstacle : obstaclesAtEdge[edgeIndex]) {
                    utils::IntervalT<int> affectedTrackRange = gridTrackRange.IntersectWith(obstacle->second);
                    if (!affectedTrackRange.IsValid()) continue;
                    for (int trackIndex = affectedTrackRange.low; trackIndex <= affectedTrackRange.high; trackIndex++) {
                        int tIdx = trackIndex - gridTrackRange.low;
                        if (obstacle->first[direction].low <= gridline && obstacle->first[direction].high >= gridline) {
                            // Completely blocked 
                            usableIntervals[tIdx] = {gridline, gridline};
                        } else if (obstacle->first[direction].high < gridline) {
                            usableIntervals[tIdx].low = max(usableIntervals[tIdx].low, obstacle->first[direction].high);
                        } else if (obstacle->first[direction].low > gridline) {
                            usableIntervals[tIdx].high = min(usableIntervals[tIdx].high, obstacle->first[direction].low);
                        }
                        // if (obstacle->first[direction].HasStrictIntersectWith(usableIntervals[tIdx])) {
                        //     usableIntervals[tIdx] = {gridline, gridline};
                        // }
                    }
                }
                CapacityT capacity = 0;
                for (int tIdx = 0; tIdx < usableIntervals.size(); tIdx++) {
                    capacity += (CapacityT)usableIntervals[tIdx].range() / edgeInterval.range();
                }
                if (direction == MetalLayer::V) {
                    graphEdges[layerIndex][gridIndex][edgeIndex].capacity = capacity;
                } else {
                    graphEdges[layerIndex][edgeIndex][gridIndex].capacity = capacity;
                }
            }
        }
    }
}

utils::IntervalT<int> GridGraph::rangeSearchGridlines(const unsigned dimension, const utils::IntervalT<DBU>& locInterval) const {
    utils::IntervalT<int> range;
    range.low = lower_bound(gridlines[dimension].begin(), gridlines[dimension].end(), locInterval.low) - gridlines[dimension].begin();
    range.high = lower_bound(gridlines[dimension].begin(), gridlines[dimension].end(), locInterval.high) - gridlines[dimension].begin();
    if (range.high >= gridlines[dimension].size()) {
        range.high = gridlines[dimension].size() - 1;
    } else if (gridlines[dimension][range.high] > locInterval.high) {
        range.high -= 1;
    }
    return range;
}
  
utils::IntervalT<int> GridGraph::rangeSearchRows(const unsigned dimension, const utils::IntervalT<DBU>& locInterval) const {
    const auto& lineRange = rangeSearchGridlines(dimension, locInterval);
    return {
        gridlines[dimension][lineRange.low] == locInterval.low ? lineRange.low : max(lineRange.low - 1, 0),
        gridlines[dimension][lineRange.high] == locInterval.high ? lineRange.high - 1 : min(lineRange.high, static_cast<int>(getSize(dimension)) - 1)
    };
}

utils::BoxT<DBU> GridGraph::getCellBox(utils::PointT<int> point) const {
    return {
        getGridline(0, point.x), getGridline(1, point.y),
        getGridline(0, point.x + 1), getGridline(1, point.y + 1)
    };
}

utils::BoxT<int> GridGraph::rangeSearchCells(const utils::BoxT<DBU>& box) const {
    return {rangeSearchRows(0, box[0]), rangeSearchRows(1, box[1])};
}

DBU GridGraph::getEdgeLength(unsigned direction, unsigned edgeIndex) const {
    return gridCenters[direction][edgeIndex + 1] - gridCenters[direction][edgeIndex];
}

inline double GridGraph::logistic(const CapacityT& input, const double slope) const {
    return 1.0 / (1.0 + exp(input * slope));
}

CostT GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> lower, const CapacityT demand) const {
    unsigned direction = layerDirections[layerIndex];
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    DBU demandLength = demand * edgeLength;
    const auto& edge = graphEdges[layerIndex][lower.x][lower.y];
    CostT cost = demandLength * unit_length_wire_cost;
    cost += demandLength * unit_length_short_costs[layerIndex] * (
        edge.capacity < 1.0 ? 1.0 : logistic(edge.capacity - edge.demand, parameters.cost_logistic_slope)
    );
    return cost;
}

CostT GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const {
    unsigned direction = layerDirections[layerIndex];
    assert(u[1 - direction] == v[1 - direction]);
    CostT cost = 0;
    if (direction == MetalLayer::H) {
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++) cost += getWireCost(layerIndex, {x, u.y});
    } else {
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++) cost += getWireCost(layerIndex, {u.x, y});
    }
    return cost;
}

CostT GridGraph::getViaCost(const int layerIndex, const utils::PointT<int> loc) const {
    assert(layerIndex + 1 < nLayers);
    CostT cost = unit_via_cost;
    // Estimated wire cost to satisfy min-area
    for (int l = layerIndex; l <= layerIndex + 1; l++) {
        unsigned direction = layerDirections[l];
        utils::PointT<int> lowerLoc = loc;
        lowerLoc[direction] -= 1;
        DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction]) : 0;
        DBU higherEdgeLength = loc[direction] < getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
        CapacityT demand = (CapacityT) layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
        if (lowerEdgeLength > 0) cost += getWireCost(l, lowerLoc, demand);
        if (higherEdgeLength > 0) cost += getWireCost(l, loc, demand);
    }
    return cost;
}

void GridGraph::selectAccessPoints(GRNet& net, robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>>& selectedAccessPoints) const {
    selectedAccessPoints.clear();
    // cell hash (2d) -> access point, fixed layer interval
    selectedAccessPoints.reserve(net.getNumPins());
    const auto& boundingBox = net.getBoundingBox();
    utils::PointT<int> netCenter(boundingBox.cx(), boundingBox.cy());
    for (const auto& accessPoints : net.getPinAccessPoints()) {
        std::pair<int, int> bestAccessDist = {0, std::numeric_limits<int>::max()};
        int bestIndex = -1;
        for (int index = 0; index < accessPoints.size(); index++) {
            const auto& point = accessPoints[index];
            int accessibility = 0;
            if (point.layerIdx >= parameters.min_routing_layer) {
                unsigned direction = getLayerDirection(point.layerIdx);
                accessibility += getEdge(point.layerIdx, point.x, point.y).capacity >= 1;
                if (point[direction] > 0) {
                    auto lower = point;
                    lower[direction] -= 1;
                    accessibility += getEdge(lower.layerIdx, lower.x, lower.y).capacity >= 1;
                }
            } else {
                accessibility = 1;
            }
            int distance = abs(netCenter.x - point.x) + abs(netCenter.y - point.y);
            if (accessibility > bestAccessDist.first || (accessibility == bestAccessDist.first && distance < bestAccessDist.second)) {
                bestIndex = index;
                bestAccessDist = {accessibility, distance};
            }
        }
        if (bestAccessDist.first == 0) {
            log() << "Warning: the pin is hard to access." << std::endl;
        }
        const utils::PointT<int> selectedPoint = accessPoints[bestIndex];
        const uint64_t hash = hashCell(selectedPoint.x, selectedPoint.y);
        if (selectedAccessPoints.find(hash) == selectedAccessPoints.end()) {
            selectedAccessPoints.emplace(hash, std::make_pair(selectedPoint, utils::IntervalT<int>()));
        }
        utils::IntervalT<int>& fixedLayerInterval = selectedAccessPoints[hash].second;
        for (const auto& point : accessPoints) {
            if (point.x == selectedPoint.x && point.y == selectedPoint.y) {
                fixedLayerInterval.Update(point.layerIdx);
            }
        }
    }
    // Extend the fixed layers to 2 layers higher to facilitate track switching
    for (auto& accessPoint : selectedAccessPoints) {
        utils::IntervalT<int>& fixedLayers = accessPoint.second.second;
        fixedLayers.high = min(fixedLayers.high + 2, (int)getNumLayers() - 1);
    }
}

void GridGraph::commit(const int layerIndex, const utils::PointT<int> lower, const CapacityT demand) {
    graphEdges[layerIndex][lower.x][lower.y].demand += demand;
}

void GridGraph::commitWire(const int layerIndex, const utils::PointT<int> lower, const bool reverse) {
    unsigned direction = layerDirections[layerIndex];
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    if (reverse) {
        commit(layerIndex, lower, -1);
        totalLength -= edgeLength;
    } else {
        commit(layerIndex, lower, 1);
        totalLength += edgeLength;
    }
}

void GridGraph::commitVia(const int layerIndex, const utils::PointT<int> loc, const bool reverse) {
    assert(layerIndex + 1 < nLayers);
    for (int l = layerIndex; l <= layerIndex + 1; l++) {
        unsigned direction = layerDirections[l];
        utils::PointT<int> lowerLoc = loc;
        lowerLoc[direction] -= 1;
        DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction]) : 0;
        DBU higherEdgeLength = loc[direction] < getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
        CapacityT demand = (CapacityT) layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
        if (lowerEdgeLength > 0) commit(l, lowerLoc, (reverse ? -demand : demand));
        if (higherEdgeLength > 0) commit(l, loc, (reverse ? -demand : demand));
    }
    if (reverse) totalNumVias -= 1;
    else totalNumVias += 1;
}

void GridGraph::commitTree(const std::shared_ptr<GRTreeNode>& tree, const bool reverse) {
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = layerDirections[node->layerIdx];
                if (direction == MetalLayer::H) {
                    assert(node->y == child->y);
                    int l = min(node->x, child->x), h = max(node->x, child->x);
                    for (int x = l; x < h; x++) {
                        commitWire(node->layerIdx, {x, node->y}, reverse);
                    }
                } else {
                    assert(node->x == child->x);
                    int l = min(node->y, child->y), h = max(node->y, child->y);
                    for (int y = l; y < h; y++) {
                        commitWire(node->layerIdx, {node->x, y}, reverse);
                    }
                }
            } else {
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx < maxLayerIndex; layerIdx++) {
                    commitVia(layerIdx, {node->x, node->y}, reverse);
                }
            }
        }
    });
}

int GridGraph::checkOverflow(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const {
    int num = 0;
    unsigned direction = layerDirections[layerIndex];
    if (direction == MetalLayer::H) {
        assert(u.y == v.y);
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++) {
            if (checkOverflow(layerIndex, x, u.y)) num++;
        }
    } else {
        assert(u.x == v.x);
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++) {
            if (checkOverflow(layerIndex, u.x, y)) num++;
        }
    }
    return num;
}

int GridGraph::checkOverflow(const std::shared_ptr<GRTreeNode>& tree) const {
    if (!tree) return 0;
    int num = 0;
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (auto& child : node->children) {
            // Only check wires
            if (node->layerIdx == child->layerIdx) {
                num += checkOverflow(node->layerIdx, (utils::PointT<int>)*node, (utils::PointT<int>)*child);
            }
        }
    });
    return num;
}


std::string GridGraph::getPythonString(const std::shared_ptr<GRTreeNode>& routingTree) const {
    vector<std::tuple<utils::PointT<int>, utils::PointT<int>, bool>> edges;
    GRTreeNode::preorder(routingTree, [&] (std::shared_ptr<GRTreeNode> node) {
        for (auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = getLayerDirection(node->layerIdx);
                int r = (*node)[1 - direction];
                const int l = min((*node)[direction], (*child)[direction]);
                const int h = max((*node)[direction], (*child)[direction]);
                if (l == h) continue;
                utils::PointT<int> lpoint = (direction == MetalLayer::H ? utils::PointT<int>(l, r) : utils::PointT<int>(r, l));
                utils::PointT<int> hpoint = (direction == MetalLayer::H ? utils::PointT<int>(h, r) : utils::PointT<int>(r, h));
                bool congested = false;
                for (int c = l; c < h; c++) {
                    utils::PointT<int> cpoint = (direction == MetalLayer::H ? utils::PointT<int>(c, r) : utils::PointT<int>(r, c));
                    if (checkOverflow(node->layerIdx, cpoint.x, cpoint.y) != congested) {
                        if (lpoint != cpoint) {
                            edges.emplace_back(lpoint, cpoint, congested);
                            lpoint = cpoint;
                        }
                        congested = !congested;
                    } 
                }
                if (lpoint != hpoint) edges.emplace_back(lpoint, hpoint, congested);
            }
        }
    });
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < edges.size(); i++) {
        auto& edge = edges[i];
        ss << "[" << std::get<0>(edge) << ", " << std::get<1>(edge) << ", " << (std::get<2>(edge) ? 1 : 0) << "]";
        ss << (i < edges.size() - 1 ? ", " : "]");
    }
    return ss.str();
}

void GridGraph::extractBlockageView(GridGraphView<bool>& view) const {
    view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, true)));
    for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++) {
        unsigned direction = getLayerDirection(layerIndex);
        for (int x = 0; x < xSize; x++) {
            for (int y = 0; y < ySize; y++) {
                if (getEdge(layerIndex, x, y).capacity >= 1.0) {
                    view[direction][x][y] = false;
                }
            }
        }
    }
}

void GridGraph::extractCongestionView(GridGraphView<bool>& view) const {
    view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, false)));
    for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++) {
        unsigned direction = getLayerDirection(layerIndex);
        for (int x = 0; x < xSize; x++) {
            for (int y = 0; y < ySize; y++) {
                if (checkOverflow(layerIndex, x, y)) {
                    view[direction][x][y] = true;
                }
            }
        }
    }
}

void GridGraph::extractWireCostView(GridGraphView<CostT>& view) const {
    view.assign(2, vector<vector<CostT>>(xSize, vector<CostT>(ySize, std::numeric_limits<CostT>::max())));
    for (unsigned direction = 0; direction < 2; direction++) {
        vector<int> layerIndices;
        CostT unitLengthShortCost = std::numeric_limits<CostT>::max();
        for (int layerIndex = parameters.min_routing_layer; layerIndex < getNumLayers(); layerIndex++) {
            if (getLayerDirection(layerIndex) == direction) {
                layerIndices.emplace_back(layerIndex);
                unitLengthShortCost = min(unitLengthShortCost, getUnitLengthShortCost(layerIndex));
            }
        }
        for (int x = 0; x < xSize; x++) {
            for (int y = 0; y < ySize; y++) {
                int edgeIndex = direction == MetalLayer::H ? x : y;
                if (edgeIndex >= getSize(direction) - 1) continue;
                CapacityT capacity = 0;
                CapacityT demand = 0;
                for (int layerIndex : layerIndices) {
                    const auto& edge = getEdge(layerIndex, x, y);
                    capacity += edge.capacity;
                    demand += edge.demand;
                }
                DBU length = getEdgeLength(direction, edgeIndex);
                view[direction][x][y] = length * (
                    unit_length_wire_cost + unitLengthShortCost * (
                        capacity < 1.0 ? 1.0 : logistic(capacity - demand, parameters.maze_logistic_slope)
                    )
                );
            }
        }
    }
}

void GridGraph::updateWireCostView(GridGraphView<CostT>& view, std::shared_ptr<GRTreeNode> routingTree) const {
    vector<vector<int>> sameDirectionLayers(2);
    vector<CostT> unitLengthShortCost(2, std::numeric_limits<CostT>::max());
    for (int layerIndex = parameters.min_routing_layer; layerIndex < getNumLayers(); layerIndex++) {
        unsigned direction = getLayerDirection(layerIndex);
        sameDirectionLayers[direction].emplace_back(layerIndex);
        unitLengthShortCost[direction] = min(unitLengthShortCost[direction], getUnitLengthShortCost(layerIndex));
    }
    auto update = [&] (unsigned direction, int x, int y) {
        int edgeIndex = direction == MetalLayer::H ? x : y;
        if (edgeIndex >= getSize(direction) - 1) return;
        CapacityT capacity = 0;
        CapacityT demand = 0;
        for (int layerIndex : sameDirectionLayers[direction]) {
            if (getLayerDirection(layerIndex) != direction) continue;
            const auto& edge = getEdge(layerIndex, x, y);
            capacity += edge.capacity;
            demand += edge.demand;
        }
        DBU length = getEdgeLength(direction, edgeIndex);
        view[direction][x][y] = length * (
            unit_length_wire_cost + unitLengthShortCost[direction] * (
                capacity < 1.0 ? 1.0 : logistic(capacity - demand, parameters.maze_logistic_slope)
            )
        );
    };
    GRTreeNode::preorder(routingTree, [&] (std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = getLayerDirection(node->layerIdx);
                if (direction == MetalLayer::H) {
                    assert(node->y == child->y);
                    int l = min(node->x, child->x), h = max(node->x, child->x);
                    for (int x = l; x < h; x++) {
                        update(direction, x, node->y);
                    }
                } else {
                    assert(node->x == child->x);
                    int l = min(node->y, child->y), h = max(node->y, child->y);
                    for (int y = l; y < h; y++) {
                        update(direction, node->x, y);
                    }
                }
            } else {
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx < maxLayerIndex; layerIdx++) {
                    unsigned direction = getLayerDirection(layerIdx);
                    update(direction, node->x, node->y);
                    if ((*node)[direction] > 0) update(direction, node->x - 1 + direction, node->y - direction);
                }
            }
        }
    });
}

void GridGraph::write(const std::string heatmap_file) const {
    log() << "writing heatmap to file..." << std::endl;
    std::stringstream ss;
    
    ss << nLayers << " " << xSize << " " << ySize << " " << std::endl;
    for (int layerIndex = 0; layerIndex < nLayers; layerIndex++) {
        ss << layerNames[layerIndex] << std::endl;
        for (int y = 0; y < ySize; y++) {
            for (int x = 0; x < xSize; x++) {
                ss << (graphEdges[layerIndex][x][y].capacity - graphEdges[layerIndex][x][y].demand)
                     << (x == xSize - 1 ? "" : " ");
            }
            ss << std::endl;
        }
    }
    std::ofstream fout(heatmap_file);
    fout << ss.str();
    fout.close();
}
