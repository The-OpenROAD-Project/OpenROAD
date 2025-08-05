#include <queue>
#include "MazeRoute.h"

void SparseGraph::init(GridGraphView<CostT>& wireCostView, SparseGrid& grid) {
    // 0. Create pseudo pins
    robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>> selectedAccessPoints;
    gridGraph.selectAccessPoints(net, selectedAccessPoints);
    pseudoPins.reserve(selectedAccessPoints.size());
    for (auto& selectedPoint : selectedAccessPoints) pseudoPins.push_back(selectedPoint.second);

    // 1. Collect additional routing grid lines
    vector<int> pxs;
    vector<int> pys;
    pxs.reserve(net.getNumPins());
    pys.reserve(net.getNumPins());
    for (const auto& pin : pseudoPins) {
        pxs.emplace_back(pin.first.x);
        pys.emplace_back(pin.first.y);
    }
    std::sort(pxs.begin(), pxs.end());
    std::sort(pys.begin(), pys.end());
    
    const int xSize = gridGraph.getSize(0);
    const int ySize = gridGraph.getSize(1);
    xs.reserve(xSize / grid.interval.x + pxs.size());
    ys.reserve(ySize / grid.interval.y + pys.size());
    int j = 0;
    for (int i = 0; true; i++) {
        int x = i * grid.interval.x + grid.offset.x;
        for ( ; j < pxs.size() && pxs[j] <= x; j++) {
            if ((xs.size() > 0 && pxs[j] == xs.back()) || pxs[j] == x) continue;
            xs.emplace_back(pxs[j]);
        }
        if (x < xSize) {
            xs.emplace_back(x);
        } else {
            break;
        }
    }
    j = 0;
    for (int i = 0; true; i++) {
        int y = i * grid.interval.y + grid.offset.y;
        for ( ; j < pys.size() && pys[j] <= y; j++) {
            if ((ys.size() > 0 && pys[j] == ys.back()) || pys[j] == y) continue;
            ys.emplace_back(pys[j]);
        }
        if (y < ySize) {
            ys.emplace_back(y);
        } else {
            break;
        }
    }
    
    // 2. Add vertices
    vertices.reserve(2 * xs.size() * ys.size());
    for (unsigned direction = 0; direction < 2; direction++) {
        for (auto& y : ys) {
            for (auto& x : xs) {
                vertices.emplace_back(direction, x, y);
            }
        }
    }
    
    // 3. Add same-layer connections
    edges.resize(vertices.size(), {-1, -1, -1});
    costs.resize(vertices.size(), {-1, -1, -1});
    auto addSameLayerEdge = [&] (const unsigned direction, const int xi, const int yi) {
        const int u = getVertexIndex(direction, xi, yi);
        const int v = direction == MetalLayer::H ? u + 1 : u + xs.size();
        utils::PointT<int> U(xs[xi], ys[yi]);
        utils::PointT<int> V(xs[xi + 1 - direction], ys[yi + direction]);
        
        edges[u][0] = v;
        edges[v][1] = u;
        costs[u][0] = costs[v][1] = wireCostView.sum(U, V);
    };
    
    for (unsigned direction = 0; direction < 2; direction++) {
        if (direction == MetalLayer::H) {
            for (int yi = 0; yi < ys.size(); yi++) {
                for (int xi = 0; xi + 1 < xs.size(); xi++) {
                    addSameLayerEdge(direction, xi, yi);
                }
            }
        } else {
            for (int xi = 0; xi < xs.size(); xi++) {
                for (int yi = 0; yi + 1 < ys.size(); yi++) {
                    addSameLayerEdge(direction, xi, yi);
                }
            }
        }
    }
    
    // 4. Add diff-layer connections
    auto addDiffLayerEdge = [&] (const int xi, const int yi) {
        const int u = getVertexIndex(0, xi, yi);
        const int v = u + xs.size() * ys.size();
        
        edges[u][2] = v;
        edges[v][2] = u;
        costs[u][2] = costs[v][2] = gridGraph.getUnitViaCost();
    };
    
    for (int xi = 0; xi < xs.size(); xi++) {
        for (int yi = 0; yi < ys.size(); yi++) {
            addDiffLayerEdge(xi, yi);
        }
    }
    
    // 5. Add pseudo pin locations
    robin_hood::unordered_map<int, int> xtoxi;
    robin_hood::unordered_map<int, int> ytoyi;
    for (int xi = 0; xi < xs.size(); xi++) xtoxi.emplace(xs[xi], xi);
    for (int yi = 0; yi < ys.size(); yi++) ytoyi.emplace(ys[yi], yi);
    
    pinVertex.resize(pseudoPins.size(), -1);
    for (int pinIndex = 0; pinIndex < pseudoPins.size(); pinIndex++) {
        const auto& pin = pseudoPins[pinIndex];
        const int xi = xtoxi[pin.first.x];
        const int yi = ytoyi[pin.first.y];
        const int u = getVertexIndex(0, xi, yi);
        vertexPin.emplace(u, pinIndex);
        pinVertex[pinIndex] = u;
        // Set the cost of the diff-layer connection at u to be 0
        costs[u][2] = 0;
        costs[u + xs.size() * ys.size()][2] = 0;
    }
}

void MazeRoute::run() {
    vector<CostT> minCosts(graph.getNumVertices(), std::numeric_limits<CostT>::max());
    solutions.reserve(net.getNumPins());
    auto compareSolution = [&] (const std::shared_ptr<Solution>& lhs, const std::shared_ptr<Solution>& rhs) {
        return lhs->cost > rhs->cost;
    };
    std::priority_queue<
        std::shared_ptr<Solution>, vector<std::shared_ptr<Solution>>, decltype(compareSolution)
    > queue(compareSolution);
    auto updateSolution = [&] (const std::shared_ptr<Solution>& solution) {
        queue.push(solution);
        if (solution->cost < minCosts[solution->vertex]) minCosts[solution->vertex] = solution->cost;
    };
    
    vector<bool> visited(net.getNumPins(), false);
    const int startPinIndex = 0;
    visited[startPinIndex] = true;
    int numDetached = graph.getNumPseudoPins() - 1;
    updateSolution(std::make_shared<Solution>(0, graph.getPinVertex(startPinIndex), nullptr));
    
    while (numDetached > 0) {
        std::shared_ptr<Solution> foundSolution;
        int foundPinIndex;
        while(!queue.empty()) {
            auto solution = queue.top();
            queue.pop();
            foundPinIndex = graph.getVertexPin(solution->vertex);
            if (foundPinIndex != -1 && !visited[foundPinIndex]) {
                foundSolution = solution;
                break;
            }
            // Pruning
            if (solution->cost > minCosts[solution->vertex]) continue;
            for (int edgeIndex = 0; edgeIndex < 3; edgeIndex++) {
                int nextVertex = graph.getNextVertex(solution->vertex, edgeIndex);
                if (nextVertex == -1 || (solution->prev && nextVertex == solution->prev->vertex)) continue;
                CostT nextCost = solution->cost + graph.getEdgeCost(solution->vertex, edgeIndex);
                if (nextCost < minCosts[nextVertex]) 
                    updateSolution(std::make_shared<Solution>(nextCost, nextVertex, solution));
            }
        }
        
        solutions.emplace_back(foundSolution);
        visited[foundPinIndex] = true;
        numDetached -= 1; 
        
        // Update the cost of the vertices on the path
        std::shared_ptr<Solution> temp = foundSolution;
        while (temp && temp->cost != 0) {
            updateSolution(std::make_shared<Solution>(0, temp->vertex, temp->prev));
            temp = temp->prev;
        }
    }
    
    if (numDetached != 0) {
        log() << "Error: failed to connect all pins." << std::endl;
    }
}


std::shared_ptr<SteinerTreeNode> MazeRoute::getSteinerTree() const {
    std::shared_ptr<SteinerTreeNode> tree = nullptr;
    if (graph.getNumPseudoPins() == 1) {
        const auto& pseudoPin = graph.getPseudoPin(0);
        tree = std::make_shared<SteinerTreeNode>(pseudoPin.first, pseudoPin.second);
        return tree;
    }
    
    vector<bool> visited(net.getNumPins(), false);
    robin_hood::unordered_map<int, std::shared_ptr<SteinerTreeNode>> created;
    for (auto& solution : solutions) {
        std::shared_ptr<Solution> temp = solution;
        std::shared_ptr<SteinerTreeNode> lastNode = nullptr;
        while (temp) {
            auto it = created.find(temp->vertex);
            if (it == created.end()) {
                utils::PointT<int> point = graph.getPoint(temp->vertex);
                auto node = std::make_shared<SteinerTreeNode>(point);
                created.emplace(temp->vertex, node);
                if (lastNode) node->children.emplace_back(lastNode);
                if (!temp->prev) tree = node;
                if (!lastNode || !temp->prev) {
                    // Both the start and the end of the path should contain pins
                    int pinIndex = graph.getVertexPin(temp->vertex);
                    assert(pinIndex != -1);
                    node->fixedLayers = graph.getPseudoPin(pinIndex).second;
                }
                lastNode = node;
                temp = temp->prev;
            } else {
                if (lastNode) it->second->children.emplace_back(lastNode);
                break;
            }
        }
    }
    
    // Remove redundant tree nodes
    SteinerTreeNode::preorder(tree, [&] (std::shared_ptr<SteinerTreeNode> node) {
        for (int childIndex = 0; childIndex < node->children.size(); childIndex++) {
            std::shared_ptr<SteinerTreeNode> child = node->children[childIndex];
            if (node->x == child->x && node->y == child->y) {
                for (auto& gradchild : child->children) node->children.emplace_back(gradchild);
                if (child->fixedLayers.IsValid()) {
                    if (node->fixedLayers.IsValid()) node->fixedLayers.UnionWith(child->fixedLayers);
                    else node->fixedLayers = child->fixedLayers;
                }
                node->children.erase(node->children.begin() + childIndex);
                childIndex -= 1;
            }
        }
    });
    
    // Remove intermediate tree nodes
    SteinerTreeNode::preorder(tree, [&] (std::shared_ptr<SteinerTreeNode> node) {
        for (std::shared_ptr<SteinerTreeNode>& child : node->children) {
            unsigned direction = (node->y == child->y ? MetalLayer::H : MetalLayer::V);
            std::shared_ptr<SteinerTreeNode> temp = child;
            while(
                !temp->fixedLayers.IsValid() && temp->children.size() == 1 &&
                (*temp)[1 - direction] == (*(temp->children[0]))[1 - direction]
            ) temp = temp->children[0];
            child = temp;
        }
    });
    
    // Check duplicate tree nodes
    SteinerTreeNode::preorder(tree, [&] (std::shared_ptr<SteinerTreeNode> node) {
        for (auto child : node->children) {
            if (node->x == child->x && node->y == child->y) {
                log() << "Error: duplicate tree nodes encountered." << std::endl;
            }
        }
    });
    return tree;
}