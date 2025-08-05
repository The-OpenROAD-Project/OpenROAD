#include "PatternRoute.h"

void SteinerTreeNode::preorder(
    std::shared_ptr<SteinerTreeNode> node, 
    std::function<void(std::shared_ptr<SteinerTreeNode>)> visit
) {
    visit(node);
    for (auto& child : node->children) preorder(child, visit);
}

std::string SteinerTreeNode::getPythonString(std::shared_ptr<SteinerTreeNode> node) {
    vector<std::pair<utils::PointT<int>, utils::PointT<int>>> edges;
    preorder(node, [&] (std::shared_ptr<SteinerTreeNode> node) {
        for (auto child : node->children) {
            edges.emplace_back(*node, *child);
        }
    });
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < edges.size(); i++) {
        auto& edge = edges[i];
        ss << "[" << edge.first << ", " << edge.second << "]";
        ss << (i < edges.size() - 1 ? ", " : "]");
    }
    return ss.str();
}

std::string PatternRoutingNode::getPythonString(std::shared_ptr<PatternRoutingNode> routingDag) {
    vector<std::pair<utils::PointT<int>, utils::PointT<int>>> edges;
    std::function<void(std::shared_ptr<PatternRoutingNode>)> getEdges = 
        [&] (std::shared_ptr<PatternRoutingNode> node) {
            for (auto& childPaths : node->paths) {
                for (auto& path : childPaths) {
                    edges.emplace_back(*node, *path);
                    getEdges(path);
                }
            }
        };
    getEdges(routingDag);
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < edges.size(); i++) {
        auto& edge = edges[i];
        ss << "[" << edge.first << ", " << edge.second << "]";
        ss << (i < edges.size() - 1 ? ", " : "]");
    }
    return ss.str();
}

void PatternRoute::constructSteinerTree() {
    // 1. Select access points
    robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>> selectedAccessPoints;
    gridGraph.selectAccessPoints(net, selectedAccessPoints);
    
    // 2. Construct Steiner tree
    const int degree = selectedAccessPoints.size();
    if (degree == 1) {
        for (auto& accessPoint : selectedAccessPoints) {
            steinerTree = std::make_shared<SteinerTreeNode>(accessPoint.second.first, accessPoint.second.second);
        }
    } else {
        int xs[degree * 4];
        int ys[degree * 4];
        int i = 0;
        for (auto& accessPoint : selectedAccessPoints) {
            xs[i] = accessPoint.second.first.x;
            ys[i] = accessPoint.second.first.y;
            i++;
        }
        Tree flutetree = flute(degree, xs, ys, ACCURACY);
        const int numBranches = degree + degree - 2;
        vector<utils::PointT<int>> steinerPoints;
        steinerPoints.reserve(numBranches);
        vector<vector<int>> adjacentList(numBranches);
        for (int branchIndex = 0; branchIndex < numBranches; branchIndex++) {
            const Branch& branch = flutetree.branch[branchIndex];
            steinerPoints.emplace_back(branch.x, branch.y);
            if (branchIndex == branch.n) continue;
            adjacentList[branchIndex].push_back(branch.n);
            adjacentList[branch.n].push_back(branchIndex);
        }
        std::function<void(std::shared_ptr<SteinerTreeNode>&, int, int)> constructTree = [&] (
            std::shared_ptr<SteinerTreeNode>& parent, int prevIndex, int curIndex
        ) {
            std::shared_ptr<SteinerTreeNode> current = std::make_shared<SteinerTreeNode>(steinerPoints[curIndex]);
            if (parent != nullptr && parent->x == current->x && parent->y == current->y) {
                for (int nextIndex : adjacentList[curIndex]) {
                    if (nextIndex == prevIndex) continue;
                    constructTree(parent, curIndex, nextIndex);
                }
                return;
            }
            // Build subtree
            for (int nextIndex : adjacentList[curIndex]) {
                if (nextIndex == prevIndex) continue;
                constructTree(current, curIndex, nextIndex);
            }
            // Set fixed layer interval
            uint64_t hash = gridGraph.hashCell(current->x, current->y);
            if (selectedAccessPoints.find(hash) != selectedAccessPoints.end()) {
                current->fixedLayers = selectedAccessPoints[hash].second;
            }
            // Connect current to parent
            if (parent == nullptr) {
                parent = current;
            } else {
                parent->children.emplace_back(current);
            }
        };
        // Pick a root having degree 1
        int root = 0;
        std::function<bool(int)> hasDegree1 = [&] (int index) {
            if (adjacentList[index].size() == 1) {
                int nextIndex = adjacentList[index][0];
                if (steinerPoints[index] == steinerPoints[nextIndex]) {
                    return hasDegree1(nextIndex);
                } else {
                    return true;
                }
            } else {
                return false;
            }
        };
        for (int i = 0; i < steinerPoints.size(); i++) {
            if (hasDegree1(i)) {
                root = i;
                break;
            }
        }
        constructTree(steinerTree, -1, root);
    }
}

void PatternRoute::constructRoutingDAG() {
    std::function<void(std::shared_ptr<PatternRoutingNode>&, std::shared_ptr<SteinerTreeNode>&)> constructDag = [&] (
        std::shared_ptr<PatternRoutingNode>& dstNode, std::shared_ptr<SteinerTreeNode>& steiner
    ) {
        std::shared_ptr<PatternRoutingNode> current = std::make_shared<PatternRoutingNode>(
            *steiner, steiner->fixedLayers, numDagNodes++
        );
        for (auto steinerChild : steiner->children) {
            constructDag(current, steinerChild);
        }
        if (dstNode == nullptr) {
            dstNode = current;
        } else {
            dstNode->children.emplace_back(current);
            constructPaths(dstNode, current);
        }
    };
    constructDag(routingDag, steinerTree);
}

/*
void PatternRoute::constructRoutingDAG() {
    // 1. Select access points
    robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>> selectedAccessPoints;
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
                unsigned direction = gridGraph.getLayerDirection(point.layerIdx);
                accessibility += gridGraph.getEdge(point.layerIdx, point.x, point.y).capacity >= 1;
                if (point[direction] > 0) {
                    auto lower = point;
                    lower[direction] -= 1;
                    accessibility += gridGraph.getEdge(lower.layerIdx, lower.x, lower.y).capacity >= 1;
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
            for (auto pt : accessPoints) {
                // log() << gridGraph.getSize(0) << ", " << gridGraph.getSize(1) << std::endl;
                log() << gridGraph.getCellBox(pt).x.low / 2000.0 << " " << gridGraph.getCellBox(pt).y.low / 2000.0 << " " << gridGraph.getCellBox(pt).x.high / 2000.0 << " " << gridGraph.getCellBox(pt).y.high / 2000.0 << std::endl;
            }
        }
        const utils::PointT<int> selectedPoint = accessPoints[bestIndex];
        const uint64_t hash = gridGraph.hashCell(selectedPoint.x, selectedPoint.y);
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
        fixedLayers.high = min(fixedLayers.high + 2, (int)gridGraph.getNumLayers() - 1);
    }
    // 2. Construct Steiner tree
    const int degree = selectedAccessPoints.size();
    if (degree == 1) {
        for (auto& accessPoint : selectedAccessPoints) {
            routingDag = std::make_shared<PatternRoutingNode>(accessPoint.second.first, accessPoint.second.second, numDagNodes++);
        }
    } else {
        int xs[degree * 4];
        int ys[degree * 4];
        int i = 0;
        for (auto& accessPoint : selectedAccessPoints) {
            xs[i] = accessPoint.second.first.x;
            ys[i] = accessPoint.second.first.y;
            i++;
        }
        Tree flutetree = flute(degree, xs, ys, ACCURACY);
        const int numBranches = degree + degree - 2;
        vector<utils::PointT<int>> steinerPoints;
        steinerPoints.reserve(numBranches);
        vector<vector<int>> adjacentList(numBranches);
        for (int branchIndex = 0; branchIndex < numBranches; branchIndex++) {
            const Branch& branch = flutetree.branch[branchIndex];
            steinerPoints.emplace_back(branch.x, branch.y);
            if (branchIndex == branch.n) continue;
            adjacentList[branchIndex].push_back(branch.n);
            adjacentList[branch.n].push_back(branchIndex);
        }
        std::function<void(std::shared_ptr<PatternRoutingNode>&, int, int)> constructNodes = 
        [&](std::shared_ptr<PatternRoutingNode>& parent, int prevIndex, int curIndex) {
            std::shared_ptr<PatternRoutingNode> current = std::make_shared<PatternRoutingNode>(steinerPoints[curIndex], numDagNodes++);
            if (parent != nullptr && parent->x == current->x && parent->y == current->y) {
                for (int nextIndex : adjacentList[curIndex]) {
                    if (nextIndex == prevIndex) continue;
                    constructNodes(parent, curIndex, nextIndex);
                }
                return;
            }
            // Build subtree
            for (int nextIndex : adjacentList[curIndex]) {
                if (nextIndex == prevIndex) continue;
                constructNodes(current, curIndex, nextIndex);
            }
            // Set fixed layer interval
            uint64_t hash = gridGraph.hashCell(current->x, current->y);
            if (selectedAccessPoints.find(hash) != selectedAccessPoints.end()) {
                current->fixedLayers = selectedAccessPoints[hash].second;
            }
            // Connect current to parent
            if (parent == nullptr) {
                parent = current;
            } else {
                parent->children.emplace_back(current);
                constructPaths(parent, current);
            }
        };
        int root = 0;
        // pick a root has a degree of 1
        std::function<bool(int)> hasDegree1 = [&] (int index) {
            if (adjacentList[index].size() == 1) {
                int nextIndex = adjacentList[index][0];
                if (steinerPoints[index] == steinerPoints[nextIndex]) {
                    return hasDegree1(nextIndex);
                } else {
                    return true;
                }
            } else {
                return false;
            }
        };
        for (int i = 0; i < steinerPoints.size(); i++) {
            if (hasDegree1(i)) {
                root = i;
                break;
            }
        }
        constructNodes(routingDag, -1, root);
    }
}
*/

void PatternRoute::constructPaths(std::shared_ptr<PatternRoutingNode>& start, std::shared_ptr<PatternRoutingNode>& end, int childIndex) {
    if (childIndex == -1) {
        childIndex = start->paths.size();
        start->paths.emplace_back();
    }
    vector<std::shared_ptr<PatternRoutingNode>>& childPaths = start->paths[childIndex];
    if (start->x == end->x || start->y == end->y) {
        childPaths.push_back(end);
    } else {
        for (int pathIndex = 0; pathIndex <= 1; pathIndex++) { // two paths of different L-shape
            utils::PointT<int> midPoint = pathIndex ? utils::PointT<int>(start->x, end->y) : utils::PointT<int>(end->x, start->y);
            std::shared_ptr<PatternRoutingNode> mid = std::make_shared<PatternRoutingNode>(midPoint, numDagNodes++, true);
            mid->paths = {{end}};
            childPaths.push_back(mid);
        }
    }
}

void PatternRoute::constructDetours(GridGraphView<bool>& congestionView) {
    struct ScaffoldNode {
        std::shared_ptr<PatternRoutingNode> node;
        vector<std::shared_ptr<ScaffoldNode>> children;
        ScaffoldNode(std::shared_ptr<PatternRoutingNode> n): node(n) {}
    };
    
    vector<vector<std::shared_ptr<ScaffoldNode>>> scaffolds(2);
    vector<vector<std::shared_ptr<ScaffoldNode>>> scaffoldNodes(
        2, vector<std::shared_ptr<ScaffoldNode>>(numDagNodes, nullptr)
    ); // direction -> numDagNodes -> scaffold node
    vector<bool> visited(numDagNodes, false);
    
    std::function<void(std::shared_ptr<PatternRoutingNode>)> buildScaffolds = 
        [&] (std::shared_ptr<PatternRoutingNode> node) {
            if (visited[node->index]) return;
            visited[node->index] = true;
            
            if (node->optional) {
                assert(node->paths.size() == 1 && node->paths[0].size() == 1 && !node->paths[0][0]->optional);
                auto& path = node->paths[0][0];
                buildScaffolds(path);
                unsigned direction = (node->y == path->y ? MetalLayer::H : MetalLayer::V);
                if (!scaffoldNodes[direction][path->index] && congestionView.check(*node, *path)) {
                    scaffoldNodes[direction][path->index] = std::make_shared<ScaffoldNode>(path);
                }
            } else {
                for (auto& childPaths : node->paths) {
                    for (auto& path : childPaths) {
                        buildScaffolds(path);
                        unsigned direction = (node->y == path->y ? MetalLayer::H : MetalLayer::V);
                        if (path->optional) {
                            if (!scaffoldNodes[direction][node->index] && congestionView.check(*node, *path)) {
                                scaffoldNodes[direction][node->index] = std::make_shared<ScaffoldNode>(node);
                            }
                        } else {
                            if (congestionView.check(*node, *path)) {
                                if (!scaffoldNodes[direction][node->index]) {
                                    scaffoldNodes[direction][node->index] = std::make_shared<ScaffoldNode>(node);
                                }
                                if (!scaffoldNodes[direction][path->index]) {
                                    scaffoldNodes[direction][node->index]->children.emplace_back(std::make_shared<ScaffoldNode>(path));
                                } else {
                                    scaffoldNodes[direction][node->index]->children.emplace_back(scaffoldNodes[direction][path->index]);
                                    scaffoldNodes[direction][path->index] = nullptr;
                                }
                            }
                        }
                    }
                    for (auto& child : node->children) {
                        for (unsigned direction = 0; direction < 2; direction++) {
                            if (scaffoldNodes[direction][child->index]) {
                                scaffolds[direction].emplace_back(std::make_shared<ScaffoldNode>(node));
                                scaffolds[direction].back()->children.emplace_back(scaffoldNodes[direction][child->index]);
                                scaffoldNodes[direction][child->index] = nullptr;
                            }
                        }
                    }
                }
            }
        };
        
    buildScaffolds(routingDag);
    for (unsigned direction = 0; direction < 2; direction++) {
        if (scaffoldNodes[direction][routingDag->index]) {
            scaffolds[direction].emplace_back(std::make_shared<ScaffoldNode>(nullptr));
            scaffolds[direction].back()->children.emplace_back(scaffoldNodes[direction][routingDag->index]);
        }
    }
    
    
    // std::function<void(std::shared_ptr<ScaffoldNode>)> printScaffold = 
    //     [&] (std::shared_ptr<ScaffoldNode> scaffoldNode) {
    // 
    //         for (auto& scaffoldChild : scaffoldNode->children) {
    //             if (scaffoldNode->node) std::cout << (utils::PointT<int>)*scaffoldNode->node << " " << scaffoldNode->children.size();
    //             else std::cout << "null";
    //             std::cout << " -> " << (utils::PointT<int>)*scaffoldChild->node;
    //             if (scaffoldNode->node) std::cout << " " << congested(scaffoldNode->node, scaffoldChild->node);
    //             std::cout << std::endl;
    //             printScaffold(scaffoldChild);
    //         }
    //     };
    // 
    // for (unsigned direction = 0; direction < 1; direction++) {
    //     for (auto scaffold : scaffolds[direction]) {
    //         printScaffold(scaffold);
    //         std::cout << std::endl;
    //     }
    // }
    
    std::function<void(std::shared_ptr<ScaffoldNode>, utils::IntervalT<int>&, vector<int>&, unsigned, bool)> getTrunkAndStems = 
        [&] (std::shared_ptr<ScaffoldNode> scaffoldNode, utils::IntervalT<int>& trunk, vector<int>& stems, unsigned direction, bool starting) {
            if (starting) {
                if (scaffoldNode->node) {
                    stems.emplace_back((*scaffoldNode->node)[1 - direction]);
                    trunk.Update((*scaffoldNode->node)[direction]);
                }
                for (auto& scaffoldChild : scaffoldNode->children) getTrunkAndStems(scaffoldChild, trunk, stems, direction, false);
            } else {
                trunk.Update((*scaffoldNode->node)[direction]);
                if (scaffoldNode->node->fixedLayers.IsValid()) {
                    stems.emplace_back((*scaffoldNode->node)[1 - direction]);
                }
                for (auto& treeChild : scaffoldNode->node->children) {
                    bool scaffolded = false;
                    for (auto& scaffoldChild : scaffoldNode->children) {
                        if (treeChild == scaffoldChild->node) {
                            getTrunkAndStems(scaffoldChild, trunk, stems, direction, false);
                            scaffolded = true;
                            break;
                        }
                    }
                    if (!scaffolded) {
                        stems.emplace_back((*treeChild)[1 - direction]);
                        trunk.Update((*treeChild)[direction]);
                    }
                }
            }
        };
    
    auto getTotalStemLength = [&] (const vector<int>& stems, const int pos) {
        int length = 0;
        for (int stem : stems) length += abs(stem - pos);
        return length;
    };
    
    std::function<std::shared_ptr<PatternRoutingNode>(std::shared_ptr<ScaffoldNode>, unsigned, int)> buildDetour = 
        [&] (std::shared_ptr<ScaffoldNode> scaffoldNode, unsigned direction, int shiftAmount) {
            std::shared_ptr<PatternRoutingNode> treeNode = scaffoldNode->node;
            if (treeNode->fixedLayers.IsValid()) {
                std::shared_ptr<PatternRoutingNode> dupTreeNode = 
                    std::make_shared<PatternRoutingNode>((utils::PointT<int>)*treeNode, treeNode->fixedLayers, numDagNodes++);
                std::shared_ptr<PatternRoutingNode> shiftedTreeNode = 
                    std::make_shared<PatternRoutingNode>((utils::PointT<int>)*treeNode, numDagNodes++);
                (*shiftedTreeNode)[1 - direction] += shiftAmount;
                constructPaths(shiftedTreeNode, dupTreeNode);
                for (auto& treeChild : treeNode->children) {
                    bool built = false;
                    for (auto& scaffoldChild : scaffoldNode->children) {
                        if (treeChild == scaffoldChild->node) {
                            auto shiftedChildTreeNode = buildDetour(scaffoldChild, direction, shiftAmount);
                            constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                            built = true;
                            break;
                        }
                    }
                    if (!built) {
                        constructPaths(shiftedTreeNode, treeChild);
                    }
                }
                return shiftedTreeNode;
            } else {
                std::shared_ptr<PatternRoutingNode> shiftedTreeNode = 
                    std::make_shared<PatternRoutingNode>((utils::PointT<int>)*treeNode, numDagNodes++);
                (*shiftedTreeNode)[1 - direction] += shiftAmount;
                for (auto& treeChild : treeNode->children) {
                    bool built = false;
                    for (auto& scaffoldChild : scaffoldNode->children) {
                        if (treeChild == scaffoldChild->node) {
                            auto shiftedChildTreeNode = buildDetour(scaffoldChild, direction, shiftAmount);
                            constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                            built = true;
                            break;
                        }
                    }
                    if (!built) {
                        constructPaths(shiftedTreeNode, treeChild);
                    }
                }
                return shiftedTreeNode;
            }
        };
        
    for (unsigned direction = 0; direction < 2; direction++) {
        for (std::shared_ptr<ScaffoldNode> scaffold : scaffolds[direction]) {
            assert (scaffold->children.size() == 1);
            
            utils::IntervalT<int> trunk;
            vector<int> stems;
            getTrunkAndStems(scaffold, trunk, stems, direction, true);
            std::sort(stems.begin(), stems.end());
            int trunkPos = (*scaffold->children[0]->node)[1 - direction];
            int originalLength = getTotalStemLength(stems, trunkPos);
            utils::IntervalT<int> shiftInterval(trunkPos);
            int maxLengthIncrease = trunk.range() * parameters.max_detour_ratio;
            while (shiftInterval.low - 1 >= 0 && getTotalStemLength(stems, shiftInterval.low - 1) - originalLength <= maxLengthIncrease) shiftInterval.low--;
            while (shiftInterval.high + 1 < gridGraph.getSize(1 - direction) && getTotalStemLength(stems, shiftInterval.high - 1) - originalLength <= maxLengthIncrease) shiftInterval.high++;
            int step = 1;
            while ((trunkPos - shiftInterval.low) / (step + 1) + (shiftInterval.high - trunkPos) / (step + 1) >= parameters.target_detour_count) step++;
            utils::IntervalT<int> dupShiftInterval = shiftInterval;
            shiftInterval.low = trunkPos - (trunkPos - shiftInterval.low) / step * step;
            shiftInterval.high = trunkPos + (shiftInterval.high - trunkPos) / step * step;
            for (double pos = shiftInterval.low; pos <= shiftInterval.high; pos += step) {
                int shiftAmount = (pos - trunkPos); 
                if (shiftAmount == 0) continue;
                if (scaffold->node) {
                    auto& scaffoldChild = scaffold->children[0];
                    if ((*scaffoldChild->node)[1 - direction] + shiftAmount < 0 || 
                        (*scaffoldChild->node)[1 - direction] + shiftAmount >= gridGraph.getSize(1 - direction)) {
                        continue;
                    }
                    for (int childIndex = 0; childIndex < scaffold->node->children.size(); childIndex++) {
                        auto& treeChild = scaffold->node->children[childIndex];
                        if (treeChild == scaffoldChild->node) {
                            std::shared_ptr<PatternRoutingNode> shiftedChild = buildDetour(scaffoldChild, direction, shiftAmount);
                            constructPaths(scaffold->node, shiftedChild, childIndex);
                        }
                    }
                } else {
                    std::shared_ptr<ScaffoldNode> scaffoldNode = scaffold->children[0];
                    auto treeNode = scaffoldNode->node;
                    if (treeNode->children.size() == 1) {
                        if ((*treeNode)[1 - direction] + shiftAmount < 0 || 
                            (*treeNode)[1 - direction] + shiftAmount >= gridGraph.getSize(1 - direction)) {
                            continue;
                        }
                        std::shared_ptr<PatternRoutingNode> shiftedTreeNode = 
                            std::make_shared<PatternRoutingNode>((utils::PointT<int>)*treeNode, numDagNodes++);
                        (*shiftedTreeNode)[1 - direction] += shiftAmount;
                        constructPaths(treeNode, shiftedTreeNode, 0);
                        for (auto& treeChild : treeNode->children) {
                            bool built = false;
                            for (auto& scaffoldChild : scaffoldNode->children) {
                                if (treeChild == scaffoldChild->node) {
                                    auto shiftedChildTreeNode = buildDetour(scaffoldChild, direction, shiftAmount);
                                    constructPaths(shiftedTreeNode, shiftedChildTreeNode);
                                    built = true;
                                    break;
                                }
                            }
                            if (!built) {
                                constructPaths(shiftedTreeNode, treeChild);
                            }
                        }
                    
                    } else {
                        log() << "Warning: the root has not exactly one child." << std::endl;
                    }
                }
            }
        }
    }
}

void PatternRoute::run() {
    calculateRoutingCosts(routingDag);
    net.setRoutingTree(getRoutingTree(routingDag));
}

void PatternRoute::calculateRoutingCosts(std::shared_ptr<PatternRoutingNode>& node) {
    if (node->costs.size() != 0) return;
    vector<vector<std::pair<CostT, int>>> childCosts; // childIndex -> layerIndex -> (cost, pathIndex)
    // Calculate child costs
    if (node->paths.size() > 0) childCosts.resize(node->paths.size());
    for (int childIndex = 0; childIndex < node->paths.size(); childIndex++) {
        auto& childPaths = node->paths[childIndex];
        auto& costs = childCosts[childIndex];
        costs.assign(gridGraph.getNumLayers(), {std::numeric_limits<CostT>::max(), -1});
        for (int pathIndex = 0; pathIndex < childPaths.size(); pathIndex++) {
            std::shared_ptr<PatternRoutingNode>& path = childPaths[pathIndex];
            calculateRoutingCosts(path);
            unsigned direction = node->x == path->x ? MetalLayer::V : MetalLayer::H;
            assert((*node)[1 - direction] == (*path)[1 - direction]);
            for (int layerIndex = parameters.min_routing_layer; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
                if (gridGraph.getLayerDirection(layerIndex) != direction) continue;
                CostT cost = path->costs[layerIndex] + gridGraph.getWireCost(layerIndex, *node, *path);
                if (cost < costs[layerIndex].first) costs[layerIndex] = std::make_pair(cost, pathIndex);
            }
        }
    }
    
    node->costs.assign(gridGraph.getNumLayers(), std::numeric_limits<CostT>::max());
    node->bestPaths.resize(gridGraph.getNumLayers());
    if (node->paths.size() > 0) {
        for (int layerIndex = 1; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
            node->bestPaths[layerIndex].assign(node->paths.size(), {-1, -1});
        }
    }
    // Calculate the partial sum of the via costs
    vector<CostT> viaCosts(gridGraph.getNumLayers());
    viaCosts[0] = 0;
    for (int layerIndex = 1; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
        viaCosts[layerIndex] = viaCosts[layerIndex - 1] + gridGraph.getViaCost(layerIndex - 1, *node);
    }
    utils::IntervalT<int> fixedLayers = node->fixedLayers;
    fixedLayers.low = min(fixedLayers.low, static_cast<int>(gridGraph.getNumLayers()) - 1);
    fixedLayers.high = max(fixedLayers.high, parameters.min_routing_layer);
    
    for (int lowLayerIndex = 0; lowLayerIndex <= fixedLayers.low; lowLayerIndex++) {
        vector<CostT> minChildCosts; 
        vector<std::pair<int, int>> bestPaths; 
        if (node->paths.size() > 0) {
            minChildCosts.assign(node->paths.size(), std::numeric_limits<CostT>::max());
            bestPaths.assign(node->paths.size(), {-1, -1});
        }
        for (int layerIndex = lowLayerIndex; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
            for (int childIndex = 0; childIndex < node->paths.size(); childIndex++) {
                if (childCosts[childIndex][layerIndex].first < minChildCosts[childIndex]) {
                    minChildCosts[childIndex] = childCosts[childIndex][layerIndex].first;
                    bestPaths[childIndex] = std::make_pair(childCosts[childIndex][layerIndex].second, layerIndex);
                }
            }
            if (layerIndex >= fixedLayers.high) {
                CostT cost = viaCosts[layerIndex] - viaCosts[lowLayerIndex];
                for (CostT childCost : minChildCosts) cost += childCost;
                if (cost < node->costs[layerIndex]) {
                    node->costs[layerIndex] = cost;
                    node->bestPaths[layerIndex] = bestPaths;
                }
            }
        }
        for (int layerIndex = gridGraph.getNumLayers() - 2; layerIndex >= lowLayerIndex; layerIndex--) {
            if (node->costs[layerIndex + 1] < node->costs[layerIndex]) {
                node->costs[layerIndex] = node->costs[layerIndex + 1];
                node->bestPaths[layerIndex] = node->bestPaths[layerIndex + 1];
            }
        }
    }
}

std::shared_ptr<GRTreeNode> PatternRoute::getRoutingTree(std::shared_ptr<PatternRoutingNode>& node, int parentLayerIndex) {
    if (parentLayerIndex == -1) {
        CostT minCost = std::numeric_limits<CostT>::max();
        for (int layerIndex = 0; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
            if (routingDag->costs[layerIndex] < minCost) {
                minCost = routingDag->costs[layerIndex];
                parentLayerIndex = layerIndex;
            }
        }
    }
    std::shared_ptr<GRTreeNode> routingNode = std::make_shared<GRTreeNode>(parentLayerIndex, node->x, node->y);
    std::shared_ptr<GRTreeNode> lowestRoutingNode = routingNode;
    std::shared_ptr<GRTreeNode> highestRoutingNode = routingNode;
    if (node->paths.size() > 0) {
        int pathIndex, layerIndex;
        vector<vector<std::shared_ptr<PatternRoutingNode>>> pathsOnLayer(gridGraph.getNumLayers());
        for (int childIndex = 0; childIndex < node->paths.size(); childIndex++) {
            std::tie(pathIndex, layerIndex) = node->bestPaths[parentLayerIndex][childIndex];
            pathsOnLayer[layerIndex].push_back(node->paths[childIndex][pathIndex]);
        }
        if (pathsOnLayer[parentLayerIndex].size() > 0) {
            for (auto& path : pathsOnLayer[parentLayerIndex]) {
                routingNode->children.push_back(getRoutingTree(path, parentLayerIndex));
            }
        }
        for (int layerIndex = parentLayerIndex - 1; layerIndex >= 0; layerIndex--) {
            if (pathsOnLayer[layerIndex].size() > 0) {
                lowestRoutingNode->children.push_back(std::make_shared<GRTreeNode>(layerIndex, node->x, node->y));
                lowestRoutingNode = lowestRoutingNode->children.back();
                for (auto& path : pathsOnLayer[layerIndex]) {
                    lowestRoutingNode->children.push_back(getRoutingTree(path, layerIndex));
                }
            }
        }
        for (int layerIndex = parentLayerIndex + 1; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
            if (pathsOnLayer[layerIndex].size() > 0) {
                highestRoutingNode->children.push_back(std::make_shared<GRTreeNode>(layerIndex, node->x, node->y));
                highestRoutingNode = highestRoutingNode->children.back();
                for (auto& path : pathsOnLayer[layerIndex]) {
                    highestRoutingNode->children.push_back(getRoutingTree(path, layerIndex));
                }
            }
        }
    }
    if (lowestRoutingNode->layerIdx > node->fixedLayers.low) {
        lowestRoutingNode->children.push_back(std::make_shared<GRTreeNode>(node->fixedLayers.low, node->x, node->y));
    }
    if (highestRoutingNode->layerIdx < node->fixedLayers.high) {
        highestRoutingNode->children.push_back(std::make_shared<GRTreeNode>(node->fixedLayers.high, node->x, node->y));
    }
    return routingNode;
}
