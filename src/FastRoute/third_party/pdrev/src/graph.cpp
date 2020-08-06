///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
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
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////
/**************************************************************************
 * Copyright(c) 2018 Regents of the University of California
 *              Kwangsoo Han, Andrew B. Kahng and Sriram Venkatesh
 * Contact      kwhan@ucsd.edu, abk@cs.ucsd.edu, srvenkat@ucsd.edu
 * Affiliation: Computer Science and Engineering Department, UC San Diego,
 *              La Jolla, CA 92093-0404, USA
 *
 *************************************************************************/

/**************************************************************************
 * UCSD Prim-Dijkstra Revisited
 * graph.cpp
 *************************************************************************/

#include "argument.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include "graph.h"
#include <math.h>
#include <deque>
#include <vector>
#include <cmath>
#include <map>

using namespace std;

ostream& operator<<(ostream& os, Node &n) {
    cout << n.idx << "(" << n.x << ", " << n.y << ") "
            << " parent: " << n.parent << " children: ";
    for (unsigned i = 0; i < n.children.size(); ++i) {
        cout << n.children[i] << " ";
    }
    cout << " N: ";
    for (unsigned i = 0; i < n.N.size(); ++i) {
        cout << n.N[i] << " ";
    }
    cout << " S: ";
    for (unsigned i = 0; i < n.S.size(); ++i) {
        cout << n.S[i] << " ";
    }
    cout << " E: ";
    for (unsigned i = 0; i < n.E.size(); ++i) {
        cout << n.E[i] << " ";
    }
    cout << " W: ";
    for (unsigned i = 0; i < n.W.size(); ++i) {
        cout << n.W[i] << " ";
    }
    cout << "PL: " << n.src_to_sink_dist
            << " MaxPLToChild: " << n.maxPLToChild;
    return os;
}

ostream& operator<<(ostream& os, Edge &n) {
    cout << n.idx << "(" << n.head << ", " << n.tail << ") edgeShape: " << n.best_shape;
    cout << "  Steiner: ";
    for (unsigned i = 0; i < n.STNodes.size(); ++i) {
        cout << " (" << n.STNodes[i].x
                << " " << n.STNodes[i].y << ") Child: ";
        for (unsigned j = 0; j < n.STNodes[i].sp_chil.size(); ++j) {
            cout << n.STNodes[i].sp_chil[j] << " ";
        }
        cout << "/";
    }
    if (n.best_shape == 0) {
        for (unsigned i = 0; i < n.lower_sps_to_be_added_x.size(); ++i) {
            cout << " (" << n.lower_sps_to_be_added_x[i].x
                    << " " << n.lower_sps_to_be_added_x[i].y << ") Child: ";
            for (unsigned j = 0; j < n.lower_sps_to_be_added_x[i].sp_chil.size(); ++j) {
                cout << n.lower_sps_to_be_added_x[i].sp_chil[j] << " ";
            }
            cout << "/";
        }
    } else {
        for (unsigned i = 0; i < n.upper_sps_to_be_added_x.size(); ++i) {
            cout << " (" << n.upper_sps_to_be_added_x[i].x
                    << " " << n.upper_sps_to_be_added_x[i].y << ") Child: ";
            for (unsigned j = 0; j < n.upper_sps_to_be_added_x[i].sp_chil.size(); ++j) {
                cout << n.upper_sps_to_be_added_x[i].sp_chil[j] << " ";
            }
            cout << "/";
        }
    }
    return os;
}

Graph::Graph(unsigned _num_terminals, unsigned _verbose, float _alpha1, float _alpha2,
        float _alpha3, float _alpha4, unsigned _root, float _beta, float _margin,
        unsigned _seed, unsigned _distance, vector<unsigned>& x, vector<unsigned>& y) {
    verbose = _verbose;
    num_terminals = x.size();
    orig_num_terminals = num_terminals;
    alpha1 = _alpha1;
    root_idx = _root;
    seed = _seed;
    heap_size = 0;
    maxPL = 0;
    PLmargin = _margin;
    aux.resize(_num_terminals);
    alpha2 = _alpha2;
    beta = _beta;
    distance = _distance;

    M = (1 + beta * (1 - alpha2)) / pow((num_terminals - 1), (beta * (1 - alpha2)));

    //real pointsets
    for (unsigned i = 0; i < num_terminals; ++i) {
        bool flag = false;
        for (unsigned j = 0; j < nodes.size(); ++j) {
            if (nodes[j].x == x[i] && nodes[j].y == y[i]) {
                flag = true;
                break;
            }
        }
        if (flag)
            continue;
        int idx = nodes.size();
        nodes.push_back(Node(idx, x[i], y[i]));
        edges.push_back(Edge(idx, 0, 0));
        sheared.push_back(Node(idx, 0, 0));
        sorted.push_back(idx);

        urux.push_back(9999999);
        urlx.push_back(x[i]);
        ulux.push_back(x[i]);
        ullx.push_back(-9999999);
        lrux.push_back(9999999);
        lrlx.push_back(x[i]);
        llux.push_back(x[i]);
        lllx.push_back(-9999999);

        vector<int> newColumn;
        nn.push_back(newColumn);
    }
    num_terminals = nodes.size();
    orig_num_terminals = nodes.size();
    aux.resize(num_terminals);

    if (verbose > 2) {
        cout << "-- Node locations --" << endl;
        for (unsigned i = 0; i < nodes.size(); ++i) {
            cout << "  " << nodes[i] << endl;
        }
    }
}

Graph::~Graph() {
    nodes.clear();
    sheared.clear();
    sorted.clear();
    aux.clear();

    heap_key.clear();
    heap_idx.clear();
    heap_elt.clear();
    for (unsigned i = 0; i < nn.size(); ++i) {
        nn[i].clear();
    }
    nn.clear();
    for (unsigned i = 0; i < tree_struct.size(); ++i) {
        tree_struct[i].clear();
    }
    tree_struct.clear();
    tree_struct_1darr.clear();
    edges.clear();
    urux.clear();
    urlx.clear();
    ulux.clear();
    ullx.clear();
    lrux.clear();
    lrlx.clear();
    llux.clear();
    lllx.clear();
}

bool comp_xn(Node1 i, Node1 j) {
    if (i.x == j.x) {
        return (i.y < j.y);
    } else {
        return (i.x < j.x);
    }
}

//increasing order

bool comp_xi(Node1 i, Node1 j) {
    if (i.x == j.x) {
        return (i.y < j.y);
    } else {
        return (i.x < j.x);
    }
}

//decreasing order

bool comp_xd(Node1 i, Node1 j) {
    if (i.x == j.x) {
        return (i.y < j.y);
    } else {
        return (i.x > j.x);
    }
}

//increasing order

bool comp_yi(Node1 i, Node1 j) {
    if (i.y == j.y) {
        return (i.x < j.x);
    } else {
        return (i.y < j.y);
    }
}

//decreasing order

bool comp_yd(Node1 i, Node1 j) {
    if (i.y == j.y) {
        return (i.x < j.x);
    } else {
        return (i.y > j.y);
    }
}

bool comp_xn2(Node i, Node j) {
    if (i.x == j.x) {
        return (i.y < j.y);
    } else {
        return (i.x < j.x);
    }
}

bool comp_yn(Node1 i, Node1 j) {
    if (i.y == j.y) {
        return (i.x < j.x);
    } else {
        return (i.y < j.y);
    }
}

bool comp_sw(Node i, Node j) {
    if (i.y == j.y) {
        return (i.x > j.x);
    } else {
        return (i.y > j.y);
    }
}

bool comp_se(Node i, Node j) {
    if (i.y == j.y) {
        return (i.x < j.x);
    } else {
        return (i.y > j.y);
    }
}

bool comp_nw(Node i, Node j) {
    if (i.y == j.y) {
        return (i.x > j.x);
    } else {
        return (i.y < j.y);
    }
}

bool comp_ne(Node i, Node j) {
    if (i.y == j.y) {
        return (i.x < j.x);
    } else {
        return (i.y < j.y);
    }
}

bool comp_en(Node i, Node j) {
    if (i.x == j.x) {
        return (i.y < j.y);
    } else {
        return (i.x < j.x);
    }
}

bool comp_es(Node i, Node j) {
    if (i.x == j.x) {
        return (i.y > j.y);
    } else {
        return (i.x < j.x);
    }
}

bool comp_wn(Node i, Node j) {
    if (i.x == j.x) {
        return (i.y < j.y);
    } else {
        return (i.x > j.x);
    }
}

bool comp_ws(Node i, Node j) {
    if (i.x == j.x) {
        return (i.y > j.y);
    } else {
        return (i.x > j.x);
    }
}

/*********************************************************************/

/*
   Return the Manhattan distance between two points
 */
int dist(Node& p, Node& q) {
    int dx, dy;
    dx = (p.x) - (q.x);
    if (dx < 0) dx = -dx;
    dy = (p.y) - (q.y);
    if (dy < 0) dy = -dy;

    //float sum = sqrt( pow(dx,2) + pow(dy,2));
    int sum = dx + dy;
    return sum;
}

int dist_ints(int x1, int y1, int x2, int y2) {
    int dx, dy;
    dx = (x1) - (x2);
    if (dx < 0) dx = -dx;
    dy = (y1) - (y2);
    if (dy < 0) dy = -dy;
    //float sum = sqrt( pow(dx,2) + pow(dy,2));
    int sum = dx + dy;
    return sum;
}

void Graph::UpdateManhDist() {
    for (unsigned i = 0; i < nodes.size(); ++i) {
        vector <int> distV;
        for (unsigned j = 0; j < nodes.size(); ++j) {
            if (i == j) {
                distV.push_back(0);
            } else {
                int cDist = dist(nodes[i], nodes[j]);
                distV.push_back(cDist);
            }
        }
        ManhDist.push_back(distV);
    }
}

bool Graph::IsSubTree(int cIdx, int tIdx) {
    queue<int> myqueue;
    myqueue.push(cIdx);
    while (!myqueue.empty()) {
        int cIdx = myqueue.front();
        myqueue.pop();
        for (unsigned i = 0; i < nodes[cIdx].children.size(); ++i) {
            myqueue.push(nodes[cIdx].children[i]);
            if (tIdx == nodes[cIdx].children[i])
                return true;
        }
    }

    return false;
}

void Graph::UpdateMaxPLToChild(int cIdx) {
    queue<int> myqueue;
    myqueue.push(cIdx);
    int cPL = nodes[cIdx].src_to_sink_dist;
    int maxPL = 0;
    while (!myqueue.empty()) {
        int cIdx = myqueue.front();
        myqueue.pop();
        for (unsigned i = 0; i < nodes[cIdx].children.size(); ++i) {
            int ccIdx = nodes[cIdx].children[i];
            if (nodes[ccIdx].src_to_sink_dist > maxPL) {
                maxPL = nodes[ccIdx].src_to_sink_dist;
            }
            myqueue.push(ccIdx);
        }
    }
    nodes[cIdx].maxPLToChild = max(maxPL - nodes[cIdx].src_to_sink_dist, 0);
}

void Graph::PrintInfo() {
    cout << "  Dag:";
    for (unsigned i = 0; i < dag.size(); ++i) {
        cout << " " << i;
    }
    cout << endl;
    cout << "  NN:" << endl;
    for (unsigned i = 0; i < nn.size(); ++i) {
        cout << "     " << i << " -- ";
        for (unsigned j = 0; j < nn[i].size(); ++j) {
            cout << nn[i][j] << " ";
        }
        cout << endl;
    }
    cout << "  maxPL: " << maxPL << endl;
    cout << "  Manhattan distance:" << endl;
    for (unsigned i = 0; i < ManhDist.size(); ++i) {
        cout << "    ";
        for (unsigned j = 0; j < ManhDist[i].size(); ++j) {
            cout << ManhDist[i][j] << " ";
        }
        cout << endl;
    }
    cout << "  Node Info: " << endl;
    for (unsigned i = 0; i < dag.size(); ++i) {
        cout << "    " << nodes[dag[i]] << endl;
    }
    cout << "  Edge Info: " << endl;
    for (unsigned i = 0; i < dag.size(); ++i) {
        cout << "    " << edges[dag[i]] << endl;
    }
}

Node Graph::GetCornerNode(int cIdx) {
    Edge &e = edges[cIdx];
    int shape = e.best_shape;

    if (shape == 0) {
        return (Node(100000, nodes[e.head].x, nodes[e.tail].y));
    } else {
        return (Node(100000, nodes[e.tail].x, nodes[e.head].y));
    }
}

bool Graph::IsOnEdge(Node &tNode, int idx) {
    Node &cNode = nodes[idx];
    Node &pNode = nodes[nodes[idx].parent];
    int maxX = max(cNode.x, pNode.x);
    int minX = min(cNode.x, pNode.x);
    int maxY = max(cNode.y, pNode.y);
    int minY = min(cNode.y, pNode.y);

    if (verbose > 3) {
        cout << "cNode: " << cNode << endl;
        cout << "pNode: " << pNode << endl;
    }
    if (edges[idx].best_shape == 0) {
        if (((pNode.x == tNode.x && (tNode.y <= maxY && tNode.y >= minY))
                || ((cNode.y == tNode.y) && (tNode.x <= maxX && tNode.x >= minX)))) {
            if (verbose > 3)
                cout << "True" << endl;
            return true;
        }
    } else {
        if (((cNode.x == tNode.x && (tNode.y <= maxY && tNode.y >= minY))
                || ((pNode.y == tNode.y) && (tNode.x <= maxX && tNode.x >= minX)))) {
            if (verbose > 3)
                cout << "True" << endl;
            return true;
        }
    }

    return false;
}

void Graph::GetSteiner(int cIdx, int nIdx, vector<Node>& STNodes) {
    int pIdx = nodes[cIdx].parent;
    Node corner1 = GetCornerNode(cIdx);
    if (verbose > 3)
        cout << cIdx << " corner1: " << corner1 << endl;
    if (IsOnEdge(corner1, nIdx)
            && (corner1.x != nodes[cIdx].x || corner1.y != nodes[cIdx].y)
            && (corner1.x != nodes[pIdx].x || corner1.y != nodes[pIdx].y)) {
        corner1.conn_to_par = true;
        corner1.sp_chil.push_back(cIdx);
        corner1.sp_chil.push_back(nIdx);
        STNodes.push_back(corner1);
    }
    Node corner2 = GetCornerNode(nIdx);
    if (verbose > 3)
        cout << nIdx << " corner2: " << corner2 << endl;
    if (IsOnEdge(corner2, cIdx) && (corner1.x != corner2.x || corner1.y != corner2.y)
            && (corner2.x != nodes[cIdx].x || corner2.y != nodes[cIdx].y)
            && (corner2.x != nodes[pIdx].x || corner2.y != nodes[pIdx].y)) {
        corner2.conn_to_par = true;
        corner2.sp_chil.push_back(cIdx);
        corner2.sp_chil.push_back(nIdx);
        STNodes.push_back(corner2);
    }
}

bool Graph::IsParent(int cIdx, int nIdx) {
    int idx = nIdx;
    if (cIdx == 0)
        return true;

    while (idx != 0) {
        if (idx == cIdx)
            return true;
        else
            idx = nodes[idx].parent;
    }
    return false;
}

void Graph::DupRemoval(vector <Node>& STNodes) {
    vector <Node> optSTNodes;
    for (unsigned i = 0; i < STNodes.size(); ++i) {
        bool IsDup = false;
        int tIdx;
        for (unsigned j = 0; j < optSTNodes.size(); ++j) {
            if (STNodes[i].x == optSTNodes[j].x && STNodes[i].y == optSTNodes[j].y) {
                tIdx = j;
                IsDup = true;
                break;
            }
        }
        if (IsDup) {
            vector <int> sp = optSTNodes[tIdx].sp_chil;
            for (unsigned k1 = 0; k1 < STNodes[i].sp_chil.size(); ++k1) {
                bool IsDupN = false;
                for (unsigned k2 = 0; k2 < sp.size(); ++k2) {
                    if (STNodes[i].sp_chil[k1] == sp[k2]) {
                        IsDupN = true;
                        break;
                    }
                }
                if (!IsDupN) {
                    optSTNodes[tIdx].sp_chil.push_back(STNodes[i].sp_chil[k1]);
                }
            }
        } else {
            optSTNodes.push_back(STNodes[i]);
        }
    }
    STNodes.clear();
    for (unsigned i = 0; i < optSTNodes.size(); ++i) {
        STNodes.push_back(optSTNodes[i]);
    }
}

void Graph::GetSteinerNodes(int idx, vector<Node> &fSTNodes) {
    if (verbose > 3) {
        cout << "cur fSTNode size: " << fSTNodes.size() << endl;
    }
    int cIdx = dag[idx];
    vector<Node> STNodes;
    for (unsigned i = idx + 1; i < dag.size(); ++i) {
        int nIdx = dag[i];
        if (verbose > 3) {
            cout << "dag.size: " << dag.size()
                    << " i = " << i
                    << " nIdx: " << nIdx << endl;
        }
        if (IsParent(nodes[cIdx].parent, nIdx)) {
            GetSteiner(cIdx, nIdx, STNodes);
        }
    }

    for (unsigned i = 0; i < fSTNodes.size(); ++i) {
        STNodes.push_back(fSTNodes[i]);
    }

    if (verbose > 3) {
        for (unsigned i = 0; i < STNodes.size(); ++i) {
            cout << "Before dupRemoval STNodes: " << STNodes[i] << endl;
            cout << " Child: ";
            for (unsigned j = 0; j < STNodes[i].sp_chil.size(); ++j) {
                cout << STNodes[i].sp_chil[j] << " ";
            }
            cout << endl;
        }
    }
    // post-processing
    DupRemoval(STNodes);

    if (verbose > 3) {
        for (unsigned i = 0; i < STNodes.size(); ++i) {
            cout << "After dupRemoval STNodes: " << STNodes[i] << endl;
            cout << " Child: ";
            for (unsigned j = 0; j < STNodes[i].sp_chil.size(); ++j) {
                cout << STNodes[i].sp_chil[j] << " ";
            }
            cout << endl;
        }
    }

    fSTNodes.clear();
    for (unsigned i = 0; i < STNodes.size(); ++i) {
        fSTNodes.push_back(STNodes[i]);
    }
}

void Graph::SortCNodes(vector<Node> &cNodes, int cIdx, int pIdx, int eShape) {
    if (eShape == 0) {
        switch (IdentLoc(pIdx, cIdx)) {
            case 0: sort(cNodes.begin(), cNodes.end(), comp_sw);
                break;
            case 1: sort(cNodes.begin(), cNodes.end(), comp_se);
                break;
            case 2: sort(cNodes.begin(), cNodes.end(), comp_sw);
                break;
            case 3: sort(cNodes.begin(), cNodes.end(), comp_nw);
                break;
            case 4: sort(cNodes.begin(), cNodes.end(), comp_ne);
                break;
            case 5: sort(cNodes.begin(), cNodes.end(), comp_nw);
                break;
            case 6: sort(cNodes.begin(), cNodes.end(), comp_ws);
                break;
            case 7: sort(cNodes.begin(), cNodes.end(), comp_es);
                break;
        }
    } else {
        switch (IdentLoc(pIdx, cIdx)) {
            case 0: sort(cNodes.begin(), cNodes.end(), comp_ws);
                break;
            case 1: sort(cNodes.begin(), cNodes.end(), comp_es);
                break;
            case 2: sort(cNodes.begin(), cNodes.end(), comp_se);
                break;
            case 3: sort(cNodes.begin(), cNodes.end(), comp_wn);
                break;
            case 4: sort(cNodes.begin(), cNodes.end(), comp_en);
                break;
            case 5: sort(cNodes.begin(), cNodes.end(), comp_ne);
                break;
            case 6: sort(cNodes.begin(), cNodes.end(), comp_ws);
                break;
            case 7: sort(cNodes.begin(), cNodes.end(), comp_es);
                break;
        }
    }
}

void Graph::UpdateEdges(vector<Node> &STNodes) {
    vector <int> idxs;
    for (unsigned j = 0; j < STNodes.size(); ++j) {
        Node &cN = STNodes[j];
        for (unsigned i = 0; i < dag.size(); ++i) {
            Node &tN = nodes[dag[i]];
            if (tN.x == cN.x && tN.y == cN.y) {

                idxs.push_back(j);
                break;
            }
        }
    }
    sort(idxs.begin(), idxs.end());
    reverse(idxs.begin(), idxs.end());
    for (unsigned i = 0; i < idxs.size(); ++i) {

        STNodes.erase(STNodes.begin() + idxs[i]);
    }
    for (unsigned i = 0; i < dag.size(); ++i) {
        int cIdx = dag[i];
        if (cIdx != 0) {
            vector<Node> cNodes;
            for (unsigned j = 0; j < STNodes.size(); ++j) {
                bool flag = false;
                for (unsigned k = 0; k < STNodes[j].sp_chil.size(); ++k) {
                    if (cIdx == STNodes[j].sp_chil[k]) {
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    cNodes.push_back(STNodes[j]);
                }
            }

            Edge &e = edges[cIdx];

            SortCNodes(cNodes, cIdx, nodes[cIdx].parent, e.best_shape);
            for (unsigned i = 0; i < cNodes.size(); ++i) {
                e.STNodes.push_back(cNodes[i]);
            }
        }
    }
}

void Graph::UpdateSteinerNodes() {
    BuildDAG();

    vector<Node> fSTNodes;
    for (unsigned i = 0; i < dag.size(); ++i) {
        int cIdx = dag[i];
        Edge &e = edges[cIdx];
        e.STNodes.clear();
        if (cIdx != 0) {
            if (verbose > 3)
                cout << "cIdx = " << cIdx << endl;
            GetSteinerNodes(i, fSTNodes);
        }
    }

    UpdateEdges(fSTNodes);
}

void Graph::addChild(Node &pNode, int idx) {
    vector <int> newC = pNode.children;
    bool flag = true;
    for (unsigned i = 0; i < newC.size(); ++i) {
        if (newC[i] == idx) {
            flag = false;
            break;
        }
    }
    if (flag)
        pNode.children.push_back(idx);
}

void Graph::removeN(Node &pN, int idx) {
    bool flag = false;
    int cIdx = 1000;
    vector <int> &nList = pN.N;
    for (unsigned i = 0; i < nList.size(); ++i) {
        if (nList[i] == idx) {
            flag = true;
            cIdx = i;
            break;
        }
    }
    if (flag)
        nList.erase(nList.begin() + cIdx);
}

void Graph::removeS(Node &pN, int idx) {
    bool flag = false;
    int cIdx = 1000;
    vector <int> &nList = pN.S;
    for (unsigned i = 0; i < nList.size(); ++i) {
        if (nList[i] == idx) {
            flag = true;
            cIdx = i;
            break;
        }
    }
    if (flag)
        nList.erase(nList.begin() + cIdx);
}

void Graph::removeE(Node &pN, int idx) {
    bool flag = false;
    int cIdx = 1000;
    vector <int> &nList = pN.E;
    for (unsigned i = 0; i < nList.size(); ++i) {
        if (nList[i] == idx) {
            flag = true;
            cIdx = i;
            break;
        }
    }
    if (flag)
        nList.erase(nList.begin() + cIdx);
}

void Graph::removeW(Node &pN, int idx) {
    bool flag = false;
    int cIdx = 1000;
    vector <int> &nList = pN.W;
    for (unsigned i = 0; i < nList.size(); ++i) {
        if (nList[i] == idx) {
            flag = true;
            cIdx = i;
            break;
        }
    }
    if (flag)
        nList.erase(nList.begin() + cIdx);
}

void Graph::RemoveSTNodes() {
    vector<int> toBeRemoved;
    for (unsigned i = num_terminals; i < nodes.size(); ++i) {
        if (nodes[i].children.size() < 2 || (nodes[i].parent == i && nodes[i].children.size() == 2)) {
            toBeRemoved.push_back(i);
        }
    }
    for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
        Node &cN = nodes[toBeRemoved[i]];
        removeChild(nodes[cN.parent], cN.idx);
        for (unsigned j = 0; j < cN.children.size(); ++j) {
            replaceParent(nodes[cN.children[j]], cN.idx, cN.parent);
            addChild(nodes[cN.parent], cN.children[j]);
        }
    }
    for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
        nodes.erase(nodes.begin() + toBeRemoved[i]);
    }

    map<int, int> idxMap;
    for (unsigned i = 0; i < nodes.size(); ++i) {
        idxMap[nodes[i].idx] = i;
    }
    for (unsigned i = 0; i < nodes.size(); ++i) {
        Node &cN = nodes[i];
        for (unsigned j = 0; j < toBeRemoved.size(); ++j) {
            removeChild(nodes[i], toBeRemoved[j]);
        }

        sort(cN.children.begin(), cN.children.end());
        for (unsigned j = 0; j < cN.children.size(); ++j) {
            if (cN.children[j] != idxMap[cN.children[j]])
                replaceChild(cN, cN.children[j], idxMap[cN.children[j]]);
        }
        cN.idx = i;
        cN.parent = idxMap[cN.parent];
    }
}

void Graph::RemoveUnneceSTNodes() {
    vector<int> toBeRemoved;
    for (unsigned i = num_terminals; i < nodes.size(); ++i) {
        if (nodes[i].children.size() < 2) {
            toBeRemoved.push_back(i);
        }
    }
    for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
        Node &cN = nodes[toBeRemoved[i]];
        removeChild(nodes[cN.parent], cN.idx);
        for (unsigned j = 0; j < cN.children.size(); ++j) {
            replaceParent(nodes[cN.children[j]], cN.idx, cN.parent);
            addChild(nodes[cN.parent], cN.children[j]);
            edges[cN.children[j]].head = cN.parent;
        }
    }
    for (int i = toBeRemoved.size() - 1; i >= 0; --i) {
        nodes.erase(nodes.begin() + toBeRemoved[i]);
        edges.erase(edges.begin() + toBeRemoved[i]);
    }

    map<int, int> idxMap;
    for (unsigned i = 0; i < nodes.size(); ++i) {
        if (verbose > 3) {
            cout << "idxMap " << i << " " << nodes[i].idx << endl;
        }
        idxMap[nodes[i].idx] = i;
    }
    for (unsigned i = 0; i < nodes.size(); ++i) {
        Node &cN = nodes[i];
        for (unsigned j = 0; j < toBeRemoved.size(); ++j) {
            removeChild(nodes[i], toBeRemoved[j]);
        }

        if (verbose > 3) {
            cout << "before cN: " << cN << endl;
        }
        sort(cN.children.begin(), cN.children.end());
        for (unsigned j = 0; j < cN.children.size(); ++j) {
            if (cN.children[j] != idxMap[cN.children[j]])
                replaceChild(cN, cN.children[j], idxMap[cN.children[j]]);
        }
        cN.idx = i;
        cN.parent = idxMap[cN.parent];
        edges[i].tail = i;
        edges[i].head = idxMap[edges[i].head];
        edges[i].idx = i;
        if (verbose > 3) {
            cout << "after cN: " << cN << endl;
        }
    }
}

void Graph::replaceParent(Node &pNode, int idx, int tIdx) {
    if (pNode.parent == idx)
        pNode.parent = tIdx;
}

void Graph::replaceChild(Node &pNode, int idx, int tIdx) {
    vector <int> newC = pNode.children;
    pNode.children.clear();
    for (unsigned i = 0; i < newC.size(); ++i) {
        if (newC[i] != idx) {
            pNode.children.push_back(newC[i]);
        } else {
            pNode.children.push_back(tIdx);
        }
    }
}

void Graph::removeChild(Node &pNode, int idx) {
    vector <int> newC = pNode.children;
    pNode.children.clear();
    for (unsigned i = 0; i < newC.size(); ++i) {
        if (newC[i] != idx) {
            pNode.children.push_back(newC[i]);
        }
    }
}

int Graph::IdentLoc(int pIdx, int cIdx) {
    Node &pN = nodes[pIdx];
    Node &cN = nodes[cIdx];

    if (pN.y > cN.y) {
        if (pN.x > cN.x) {
            return 0; // lower left of pN
        } else if (pN.x < cN.x) {
            return 1; // lower right of pN
        } else {
            return 2; // lower of pN
        }
    } else if (pN.y < cN.y) {
        if (pN.x > cN.x) {
            return 3; // upper left of pN
        } else if (pN.x < cN.x) {
            return 4; // upper right of pN
        } else {
            return 5; // upper of pN
        }
    } else // pN.y == cN.y
    {
        if (pN.x > cN.x) {
            return 6; // left of pN
        } else if (pN.x < cN.x) {
            return 7; // right of pN
        } else {
            if (verbose > 3)
                cout << "Error!! - pN == cN" << endl;
            return 10;
        }
    }
}

void Graph::UpdateNSEW(Edge& e) {
    Node &pN = nodes[e.head];
    Node &cN = nodes[e.tail];

    // lower L --> update N, S for parent / E, W for child
    if (e.best_shape == 0) {
        switch (IdentLoc(pN.idx, cN.idx)) {
            case 0: pN.S.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                break;
            case 1: pN.S.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                break;
            case 2: pN.S.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 3: pN.N.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                break;
            case 4: pN.N.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                break;
            case 5: pN.N.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 6: pN.W.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                break;
            case 7: pN.E.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                break;
        }
    }        // upper L --> update E, W for parent / N, S for child
    else if (e.best_shape == 1) {
        switch (IdentLoc(pN.idx, cN.idx)) {
            case 0: pN.W.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 1: pN.E.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 2: pN.S.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 3: pN.W.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 4: pN.E.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 5: pN.N.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 6: pN.W.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                break;
            case 7: pN.E.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                break;
        }
    } else {
        switch (IdentLoc(pN.idx, cN.idx)) {
            case 0: pN.S.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                pN.W.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 1: pN.S.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                pN.E.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 2: pN.S.push_back(cN.idx);
                cN.N.push_back(pN.idx);
                break;
            case 3: pN.N.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                pN.W.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 4: pN.N.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                pN.E.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 5: pN.N.push_back(cN.idx);
                cN.S.push_back(pN.idx);
                break;
            case 6: pN.W.push_back(cN.idx);
                cN.E.push_back(pN.idx);
                break;
            case 7: pN.E.push_back(cN.idx);
                cN.W.push_back(pN.idx);
                break;
        }
    }
}

void Graph::SortE(Node &n) {
    vector <Node1> tmpNode1;
    vector <int> &nList = n.E;
    for (unsigned i = 0; i < nList.size(); ++i) {
        Node &cN = nodes[nList[i]];
        tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
    }
    sort(tmpNode1.begin(), tmpNode1.end(), comp_xi);
    nList.clear();
    for (unsigned i = 0; i < tmpNode1.size(); ++i) {
        nList.push_back(tmpNode1[i].idx);
    }
}

void Graph::SortW(Node &n) {
    vector <Node1> tmpNode1;
    vector <int> &nList = n.W;
    for (unsigned i = 0; i < nList.size(); ++i) {
        Node &cN = nodes[nList[i]];
        tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
    }
    sort(tmpNode1.begin(), tmpNode1.end(), comp_xd);
    nList.clear();
    for (unsigned i = 0; i < tmpNode1.size(); ++i) {
        nList.push_back(tmpNode1[i].idx);
    }
}

void Graph::SortS(Node &n) {
    vector <Node1> tmpNode1;
    vector <int> &nList = n.S;
    for (unsigned i = 0; i < nList.size(); ++i) {
        Node &cN = nodes[nList[i]];
        tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
    }
    sort(tmpNode1.begin(), tmpNode1.end(), comp_yd);
    nList.clear();
    for (unsigned i = 0; i < tmpNode1.size(); ++i) {
        nList.push_back(tmpNode1[i].idx);
    }
}

void Graph::SortN(Node &n) {
    vector <Node1> tmpNode1;
    vector <int> &nList = n.N;
    for (unsigned i = 0; i < nList.size(); ++i) {
        Node &cN = nodes[nList[i]];
        tmpNode1.push_back(Node1(cN.idx, cN.x, cN.y));
    }
    sort(tmpNode1.begin(), tmpNode1.end(), comp_yi);
    nList.clear();
    for (unsigned i = 0; i < tmpNode1.size(); ++i) {
        nList.push_back(tmpNode1[i].idx);
    }
}

void Graph::SortAll(Node &n) {
    SortN(n);
    SortS(n);
    SortE(n);
    SortW(n);
}

void Graph::UpdateAllEdgesNSEW() {
    for (int i = 0; i < dag.size(); ++i) {
        int cIdx = dag[i];
        Node &cN = nodes[cIdx];
        cN.N.clear();
        cN.S.clear();
        cN.E.clear();
        cN.W.clear();
    }

    for (int i = 0; i < dag.size(); ++i) {
        int cIdx = dag[i];
        if (cIdx != 0) {
            UpdateNSEW(edges[cIdx]);
        }
    }

    for (int i = 0; i < dag.size(); ++i) {
        SortAll(nodes[dag[i]]);
    }
}

int Graph::DeltaN(int idx, int rIdx, bool isRemove) {
    Node &cN = nodes[idx];
    vector <int> &nList = cN.N;
    //cout << "N nList size: " << nList.size() << endl;
    if (nList.size() != 0) {
        if (nList[nList.size() - 1] == rIdx) {
            Node &rN = nodes[rIdx];
            int ref = cN.y;
            if (nList.size() >= 2)
                ref = nodes[nList[nList.size() - 2]].y;
            int delta = abs(rN.y - ref);
            //cout << " delta = " << delta << endl;
            if (isRemove)
                nList.erase(nList.begin() + nList.size() - 1);
            return delta;
        }
        if (isRemove) {
            int idxN = 100000;
            for (unsigned i = 0; i < nList.size(); ++i) {
                if (nList[i] == rIdx) {
                    idxN = i;
                    break;
                }
            }
            if (idxN != 100000) {
                nList.erase(nList.begin() + idxN);
            }
        }
    }
    return 0;
}

int Graph::DeltaS(int idx, int rIdx, bool isRemove) {
    Node &cN = nodes[idx];
    vector <int> &nList = cN.S;
    //cout << "S nList size: " << nList.size() << endl;
    if (nList.size() != 0) {
        if (nList[nList.size() - 1] == rIdx) {
            Node &rN = nodes[rIdx];
            int ref = cN.y;
            if (nList.size() >= 2)
                ref = nodes[nList[nList.size() - 2]].y;
            int delta = abs(rN.y - ref);
            //cout << " delta = " << delta << endl;
            if (isRemove)
                nList.erase(nList.begin() + nList.size() - 1);
            return delta;
        }
        if (isRemove) {
            int idxN = 100000;
            for (unsigned i = 0; i < nList.size(); ++i) {
                if (nList[i] == rIdx) {
                    idxN = i;
                    break;
                }
            }
            if (idxN != 100000) {
                nList.erase(nList.begin() + idxN);
            }
        }
    }
    return 0;
}

int Graph::DeltaW(int idx, int rIdx, bool isRemove) {
    Node &cN = nodes[idx];
    vector <int> &nList = cN.W;
    //cout << "W nList size: " << nList.size() << endl;
    if (nList.size() != 0) {
        if (nList[nList.size() - 1] == rIdx) {
            Node &rN = nodes[rIdx];
            Node &rrN = cN;
            int ref = cN.x;
            if (nList.size() >= 2)
                ref = nodes[nList[nList.size() - 2]].x;
            int delta = abs(rN.x - ref);
            //cout << " delta = " << delta << endl;
            if (isRemove)
                nList.erase(nList.begin() + nList.size() - 1);
            return delta;
        }
        if (isRemove) {
            int idxN = 100000;
            for (unsigned i = 0; i < nList.size(); ++i) {
                if (nList[i] == rIdx) {
                    idxN = i;
                    break;
                }
            }
            if (idxN != 100000) {
                nList.erase(nList.begin() + idxN);
            }
        }
    }
    return 0;
}

int Graph::DeltaE(int idx, int rIdx, bool isRemove) {
    Node &cN = nodes[idx];
    vector <int> &nList = cN.E;
    //cout << "E nList size: " << nList.size() << endl;
    if (nList.size() != 0) {
        if (nList[nList.size() - 1] == rIdx) {
            Node &rN = nodes[rIdx];
            int ref = cN.x;
            if (nList.size() >= 2)
                ref = nodes[nList[nList.size() - 2]].x;
            int delta = abs(rN.x - ref);
            //cout << " delta = " << delta << endl;
            if (isRemove)
                nList.erase(nList.begin() + nList.size() - 1);
            return delta;
        }
        if (isRemove) {
            int idxN = 100000;
            for (unsigned i = 0; i < nList.size(); ++i) {
                if (nList[i] == rIdx) {
                    idxN = i;
                    break;
                }
            }
            if (idxN != 100000) {
                nList.erase(nList.begin() + idxN);
            }
        }
    }
    return 0;
}

bool Graph::IsSameDir(int cIdx, int nIdx) {
    Node& cN = nodes[nIdx];
    int pIdx = nodes[nIdx].parent;
    bool isSameDir = false;
    int pId = 0;
    int cId = 0;
    for (unsigned i = 0; i < cN.N.size(); ++i) {
        if (cN.N[i] == pIdx)
            pId = 1;
        if (cN.N[i] == cIdx)
            cId = 1;
    }
    for (unsigned i = 0; i < cN.S.size(); ++i) {
        if (cN.S[i] == pIdx)
            pId = 2;
        if (cN.S[i] == cIdx)
            cId = 2;
    }
    for (unsigned i = 0; i < cN.E.size(); ++i) {
        if (cN.E[i] == pIdx)
            pId = 3;
        if (cN.E[i] == cIdx)
            cId = 3;
    }
    for (unsigned i = 0; i < cN.W.size(); ++i) {
        if (cN.W[i] == pIdx)
            pId = 4;
        if (cN.W[i] == cIdx)
            cId = 4;
    }
    if (cId != 0 && pId != 0 && cId == pId)
        return true;
    else
        return false;

}

void Graph::AddNode(int cIdx, int pIdx, int eShape) {
    if (verbose > 3) {
        cout << "loc: " << IdentLoc(pIdx, cIdx)
                << " eShape: " << eShape
                << endl;
    }
    if (!eShape) {
        switch (IdentLoc(pIdx, cIdx)) {
            case 0: nodes[pIdx].S.push_back(cIdx);
                SortS(nodes[pIdx]);
                nodes[cIdx].E.push_back(pIdx);
                SortE(nodes[cIdx]);
                break;
            case 1: nodes[pIdx].S.push_back(cIdx);
                SortS(nodes[pIdx]);
                nodes[cIdx].W.push_back(pIdx);
                SortW(nodes[cIdx]);
                break;
            case 2: nodes[pIdx].S.push_back(cIdx);
                SortS(nodes[pIdx]);
                nodes[cIdx].N.push_back(pIdx);
                SortN(nodes[cIdx]);
                break;
            case 3: nodes[pIdx].N.push_back(cIdx);
                SortN(nodes[pIdx]);
                nodes[cIdx].E.push_back(pIdx);
                SortE(nodes[cIdx]);
                break;
            case 4: nodes[pIdx].N.push_back(cIdx);
                SortN(nodes[pIdx]);
                nodes[cIdx].W.push_back(pIdx);
                SortW(nodes[cIdx]);
                break;
            case 5: nodes[pIdx].N.push_back(cIdx);
                SortN(nodes[pIdx]);
                nodes[cIdx].S.push_back(pIdx);
                SortS(nodes[cIdx]);
                break;
            case 6: nodes[pIdx].W.push_back(cIdx);
                SortW(nodes[pIdx]);
                nodes[cIdx].E.push_back(pIdx);
                SortE(nodes[cIdx]);
                break;
            case 7: nodes[pIdx].E.push_back(cIdx);
                SortE(nodes[pIdx]);
                nodes[cIdx].W.push_back(pIdx);
                SortW(nodes[cIdx]);
                break;
        }
    } else {
        switch (IdentLoc(pIdx, cIdx)) {
            case 0: nodes[pIdx].W.push_back(cIdx);
                SortW(nodes[pIdx]);
                nodes[cIdx].N.push_back(pIdx);
                SortN(nodes[cIdx]);
                break;
            case 1: nodes[pIdx].E.push_back(cIdx);
                SortE(nodes[pIdx]);
                nodes[cIdx].N.push_back(pIdx);
                SortN(nodes[cIdx]);
                break;
            case 2: nodes[pIdx].S.push_back(cIdx);
                SortS(nodes[pIdx]);
                nodes[cIdx].N.push_back(pIdx);
                SortN(nodes[cIdx]);
                break;
            case 3: nodes[pIdx].W.push_back(cIdx);
                SortW(nodes[pIdx]);
                nodes[cIdx].S.push_back(pIdx);
                SortS(nodes[cIdx]);
                break;
            case 4: nodes[pIdx].E.push_back(cIdx);
                SortE(nodes[pIdx]);
                nodes[cIdx].S.push_back(pIdx);
                SortS(nodes[cIdx]);
                break;
            case 5: nodes[pIdx].N.push_back(cIdx);
                SortN(nodes[pIdx]);
                nodes[cIdx].S.push_back(pIdx);
                SortS(nodes[cIdx]);
                break;
            case 6: nodes[pIdx].W.push_back(cIdx);
                SortW(nodes[pIdx]);
                nodes[cIdx].E.push_back(pIdx);
                SortE(nodes[cIdx]);
                break;
            case 7: nodes[pIdx].E.push_back(cIdx);
                SortE(nodes[pIdx]);
                nodes[cIdx].W.push_back(pIdx);
                SortW(nodes[cIdx]);
                break;
        }
    }
}

void Graph::removeNeighbor(int pIdx, int cIdx) {
    Node &pN = nodes[pIdx];
    Node &cN = nodes[cIdx];

    removeN(pN, cIdx);
    removeS(pN, cIdx);
    removeW(pN, cIdx);
    removeE(pN, cIdx);
    removeN(cN, pIdx);
    removeS(cN, pIdx);
    removeW(cN, pIdx);
    removeE(cN, pIdx);
}

int Graph::ComputeWL(int cIdx, int pIdx, bool isRemove, int eShape) {
    int WL = 0;
    int delta = 0;
    if (verbose > 3) {
        cout << "loc: " << IdentLoc(pIdx, cIdx) << endl;
        cout << "pNode: " << nodes[pIdx] << endl;
        cout << "cNode: " << nodes[cIdx] << endl;
    }
    switch (IdentLoc(pIdx, cIdx)) {
        case 0: WL += !eShape ? DeltaS(pIdx, cIdx, isRemove): DeltaW(pIdx, cIdx, isRemove);
            WL += !eShape ? DeltaE(cIdx, pIdx, isRemove) : DeltaN(cIdx, pIdx, isRemove);
            break;
        case 1: WL += !eShape ? DeltaS(pIdx, cIdx, isRemove): DeltaE(pIdx, cIdx, isRemove);
            WL += !eShape ? DeltaW(cIdx, pIdx, isRemove) : DeltaN(cIdx, pIdx, isRemove);
            break;
        case 2: WL += DeltaS(pIdx, cIdx, isRemove);
            DeltaN(cIdx, pIdx, isRemove);
            break;
        case 3: WL += !eShape ? DeltaN(pIdx, cIdx, isRemove): DeltaW(pIdx, cIdx, isRemove);
            WL += !eShape ? DeltaE(cIdx, pIdx, isRemove) : DeltaS(cIdx, pIdx, isRemove);
            break;
        case 4: WL += !eShape ? DeltaN(pIdx, cIdx, isRemove): DeltaE(pIdx, cIdx, isRemove);
            WL += !eShape ? DeltaW(cIdx, pIdx, isRemove) : DeltaS(cIdx, pIdx, isRemove);
            break;
        case 5: WL += DeltaS(cIdx, pIdx, isRemove);
            DeltaN(pIdx, cIdx, isRemove);
            break;
        case 6: WL += DeltaE(cIdx, pIdx, isRemove);
            DeltaW(pIdx, cIdx, isRemove);
            break;
        case 7: WL += DeltaW(cIdx, pIdx, isRemove);
            DeltaE(pIdx, cIdx, isRemove);
            break;
    }
    if (isRemove)
        removeNeighbor(pIdx, cIdx);

    return WL;
}

void Graph::refineSteiner2() {
    if (verbose > 3) {
        cout << "Pre-refineSteiner() graph status: " << endl;
        PrintInfo();
    }

    for (int i = dag.size() - 1; i >= 0; --i) {
        Node &cN = nodes[dag[i]];
        if (verbose > 3)
            cout << "cN: " << cN << endl;
        if (cN.idx == 0)
            continue;

        int cGain = 1000000;
        int bestWLGain = cGain;
        int edgeShape = 1000;
        if (edges[cN.idx].best_shape == 5) {
            cGain = ComputeWL(cN.idx, cN.parent, false, 0);
            if (cGain < bestWLGain) {
                bestWLGain = cGain;
                edgeShape = 0;
            }
            cGain = ComputeWL(cN.idx, cN.parent, true, 1);
            if (cGain < bestWLGain) {
                bestWLGain = cGain;
                edgeShape = 1;
            }
        } else {
            cGain = ComputeWL(cN.idx, cN.parent, true, edges[cN.idx].best_shape);
            if (cGain < bestWLGain) {
                bestWLGain = cGain;
                edgeShape = edges[cN.idx].best_shape;
            }
        }

        if (verbose > 3)
            cout << "cGain: " << cGain << endl;
        int newParent = cN.parent;

        vector <int> neighbors = nn[cN.idx];
        int newMaxPL = 0;
        int newPLToChildForParent = 0;

        if (cN.parent != newParent) {
            removeChild(nodes[cN.parent], cN.idx);
            addChild(nodes[newParent], cN.idx);
            cN.parent = newParent;
            nodes[newParent].maxPLToChild = newPLToChildForParent;
            edges[cN.idx].head = newParent;
            cN.src_to_sink_dist = newMaxPL;
            if (newMaxPL > maxPL)
                maxPL = newMaxPL;
        }
        AddNode(cN.idx, newParent, edgeShape);
        edges[cN.idx].best_shape = edgeShape;
    }
    if (verbose > 3) {
        cout << "Post-refineSteiner() graph status: " << endl;
        PrintInfo();
    }

    RemoveUnneceSTNodes();

    FreeManhDist();
    UpdateManhDist();

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());

    UpdateAllEdgesNSEW();

    if (verbose > 3) {
        cout << "Remove unnecessary Steiner nodes -- done" << endl;
        PrintInfo();
    }
    UpdateSteinerNodes();
    if (verbose > 3) {
        cout << "Post-updateSteinerNodes() graph status: " << endl;
        PrintInfo();
    }
}

void Graph::refineSteiner() {
    if (verbose > 3) {
        cout << "Pre-refineSteiner() graph status: " << endl;
        PrintInfo();
    }

    for (int i = dag.size() - 1; i >= 0; --i) {
        Node &cN = nodes[dag[i]];
        if (verbose > 3)
            cout << "cN: " << cN << endl;
        if (cN.idx == 0)
            continue;

        int cGain = 1000000;
        int bestWLGain = cGain;
        int edgeShape = 1000;
        if (edges[cN.idx].best_shape == 5) {
            cGain = ComputeWL(cN.idx, cN.parent, false, 0);
            if (cGain < bestWLGain) {
                bestWLGain = cGain;
                edgeShape = 0;
            }
            cGain = ComputeWL(cN.idx, cN.parent, true, 1);
            if (cGain < bestWLGain) {
                bestWLGain = cGain;
                edgeShape = 1;
            }
        } else {
            cGain = ComputeWL(cN.idx, cN.parent, true, edges[cN.idx].best_shape);
            if (cGain < bestWLGain) {
                bestWLGain = cGain;
                edgeShape = edges[cN.idx].best_shape;
            }
        }

        if (verbose > 3)
            cout << "cGain: " << cGain << endl;
        int newParent = cN.parent;

        vector <int> neighbors = nn[cN.idx];
        int newMaxPL = 0;
        int newPLToChildForParent = 0;
        for (unsigned j = 0; j < neighbors.size(); ++j) {
            Node &nNode = nodes[neighbors[j]];

            int newPLToChildForParentCandi = 0;
            int newMaxPLCandi = 0;
            //cout << neighbors[j] << " " << newPL << endl;
            if (verbose > 3) {
                cout << "nNode: " << nNode << endl;
                cout << "IsSubTree: " << IsSubTree(cN.idx, nNode.idx) << endl;
            }
            // is this neighbor node in sub-tree rooted by Node cN

            if (IsSubTree(cN.idx, nNode.idx) == false) {
                for (unsigned i = 0; i < 2; ++i) {
                    AddNode(cN.idx, nNode.idx, i);

                    cGain = ComputeWL(cN.idx, nNode.idx, true, i);
                    newMaxPLCandi = nNode.src_to_sink_dist + ManhDist[cN.idx][nNode.idx] + cN.maxPLToChild;
                    if (IsSameDir(cN.idx, nNode.idx)) {
                        newMaxPLCandi = newMaxPLCandi - 2 * (ManhDist[cN.idx][nNode.idx] - cGain);
                    }
                    newPLToChildForParentCandi = max(nNode.maxPLToChild, newMaxPLCandi - nNode.src_to_sink_dist);
                    if (verbose > 3) {
                        cout << "Computed gain: " << cGain << " " << bestWLGain << endl;
                        cout << "newMaxPLCandi: " << newMaxPLCandi << " " << maxPL * PLmargin << endl;
                    }
                    if (cGain < bestWLGain
                            && newMaxPLCandi < maxPL * PLmargin) {

                        if (verbose > 3) {
                            cout << "new best gain: " << cGain
                                    << " edgeShape: " << i
                                    << " newParent: " << nNode.idx
                                    << " newMaxPL: " << newMaxPLCandi
                                    << " newPLToChild: " << newPLToChildForParentCandi
                                    << endl;
                        }
                        bestWLGain = cGain;
                        edgeShape = i;
                        newParent = nNode.idx;
                        newMaxPL = newMaxPLCandi;
                        newPLToChildForParent = newPLToChildForParentCandi;
                    }
                }
            }
        }

        if (cN.parent != newParent) {
            removeChild(nodes[cN.parent], cN.idx);
            addChild(nodes[newParent], cN.idx);
            cN.parent = newParent;
            nodes[newParent].maxPLToChild = newPLToChildForParent;
            edges[cN.idx].head = newParent;
            cN.src_to_sink_dist = newMaxPL;
            if (newMaxPL > maxPL)
                maxPL = newMaxPL;
        }
        AddNode(cN.idx, newParent, edgeShape);
        edges[cN.idx].best_shape = edgeShape;
    }
    if (verbose > 3) {
        cout << "Post-refineSteiner() graph status: " << endl;
        PrintInfo();
    }

    RemoveUnneceSTNodes();

    FreeManhDist();
    UpdateManhDist();

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());

    UpdateAllEdgesNSEW();

    if (verbose > 3) {
        cout << "Remove unnecessary Steiner nodes -- done" << endl;
        PrintInfo();
    }
    UpdateSteinerNodes();
    if (verbose > 3) {
        cout << "Post-updateSteinerNodes() graph status: " << endl;
        PrintInfo();
    }
}

/***************************************************************************/

/* comparison function for use in sort
 */
bool Graph::make_unique(vector<Node>& vec) {
    for (unsigned a = 0; a < vec.size(); a++) {
        for (unsigned b = 0; b < vec.size(); b++) if (a != b) {
                if ((vec[a].x == vec[b].x) && (vec[a].y == vec[b].y)) {
                    swap(vec[b], vec.back());
                    vec.pop_back();
                    b--;
                }
            }
    }
    return true;
}

bool comp_x(Node i, Node j) {
    if (i.x == j.x) return (i.y < j.y);
    else return (i.x < j.x);
}

bool comp_y(Node i, Node j) {
    if (i.y == j.y) return (i.x < j.x);
    else return (i.y < j.y);
}

bool comp_det_cost(Node i, Node j) {
    return (i.det_cost_node > j.det_cost_node);
}

bool Graph::buildNearestNeighbors_single_node(unsigned num_terms, unsigned node_idx) {
    vector <Node> tmp = nodes;
    sorted.resize(num_terms);
    nn.resize(num_terms);
    // sort in y-axis
    sort(tmp.begin(), tmp.end(), comp_y);
    for (unsigned i = 0; i < num_terms; ++i) {
        sorted[i] = tmp[i].idx;
    }
    cout << "Sorted: ";
    for (unsigned abc = 0; abc < sorted.size(); abc++) cout << sorted[abc] << " ";
    cout << endl;

    unsigned idx = 0;
    for (unsigned abc = 0; abc < sorted.size(); abc++) if (sorted[abc] == node_idx) {
            idx = abc;
            break;
        }

    // collect neighbor 
    Node& cNode = nodes[node_idx];
    // update idx to neighbors
    // Note: nNode.y <= cNode.y 
    for (unsigned i = 0; i < idx; ++i) {
        Node& nNode = nodes[sorted[i]];
        //cout << "1 " << nNode << endl;
        if (urlx[nNode.idx] == cNode.x) {
            nn[nNode.idx].push_back(cNode.idx);
            urux[nNode.idx] = cNode.x;
            ullx[nNode.idx] = cNode.x;
        } else if (urux[nNode.idx] > cNode.x && urlx[nNode.idx] < cNode.x) {
            // right
            nn[nNode.idx].push_back(cNode.idx);
            urux[nNode.idx] = cNode.x;
        } else if (ullx[nNode.idx] < cNode.x && ulux[nNode.idx] > cNode.x) {
            // left 
            nn[nNode.idx].push_back(cNode.idx);
            ullx[nNode.idx] = cNode.x;
        }
    }

    // update neighbor to idx
    // Note: nNode.y <= cNode.y 
    for (int i = idx - 1; i >= 0; --i) {
        Node& nNode = nodes[sorted[i]];
        //cout << "2 " << nNode << endl;
        if (lrlx[cNode.idx] == nNode.x) {
            nn[cNode.idx].push_back(nNode.idx);
            lrux[cNode.idx] = nNode.x;
            lllx[cNode.idx] = nNode.x;
        } else if (lrux[cNode.idx] > nNode.x && lrlx[cNode.idx] < nNode.x) {
            // right
            nn[cNode.idx].push_back(nNode.idx);
            lrux[cNode.idx] = nNode.x;
        } else if (lllx[cNode.idx] < nNode.x && llux[cNode.idx] > nNode.x) {
            // left
            nn[cNode.idx].push_back(nNode.idx);
            lllx[cNode.idx] = nNode.x;
        }
    }

    // print neighbors
    if (verbose > 2) {
        cout << "Print neighbors" << endl;
        cout << node_idx << "th node " << nodes[node_idx] << endl;
        for (unsigned j = 0; j < nn[node_idx].size(); ++j) {
            cout << " " << nn[node_idx][j] << " " << nodes[nn[node_idx][j]] << endl;
        }
    }

    tmp.clear();
    sorted.clear();
    return true;
} //End of buildNearestNeighbors_single_node function

bool Graph::buildNearestNeighborsForSPT(unsigned num_terms) {

    for (unsigned i = 0; i < nn.size(); ++i) {
        nn[i].clear();
    }
    nn.clear();
    vector <Node> tmp = nodes;
    sorted.clear();
    urux.clear();
    urlx.clear();
    ulux.clear();
    ullx.clear();
    lrux.clear();
    lrlx.clear();
    llux.clear();
    lllx.clear();
    for (unsigned i = 0; i < num_terms; ++i) {
        sorted.push_back(nodes[i].idx);
        urux.push_back(9999999);
        urlx.push_back(nodes[i].x);
        ulux.push_back(nodes[i].x);
        ullx.push_back(-9999999);
        lrux.push_back(9999999);
        lrlx.push_back(nodes[i].x);
        llux.push_back(nodes[i].x);
        lllx.push_back(-9999999);
        vector<int> tmp2;
        nn.push_back(tmp2);
    }
    // sort in y-axis
    sort(tmp.begin(), tmp.end(), comp_y);
    for (unsigned i = 0; i < num_terms; ++i) {
        sorted[i] = tmp[i].idx;
    }

    // collect neighbor 
    for (unsigned idx = 0; idx < num_terms; ++idx) {
        if (verbose > 2)
            cout << "sorted idx: " << sorted[idx] << endl;
        Node& cNode = nodes[sorted[idx]];
        // update idx to neighbors
        // Note: nNode.y <= cNode.y 
        for (unsigned i = 0; i < idx; ++i) {
            Node& nNode = nodes[sorted[i]];
            if (urlx[nNode.idx] == cNode.x) {
                nn[nNode.idx].push_back(cNode.idx);
                urux[nNode.idx] = cNode.x;
                ullx[nNode.idx] = cNode.x;
            } else if (urux[nNode.idx] > cNode.x && urlx[nNode.idx] < cNode.x) {
                if (verbose > 2)
                    cout << "right for nNode cNode idx: " << sorted[idx] << " nNode idx: " << nNode.idx << endl;
                ;
                // right
                nn[nNode.idx].push_back(cNode.idx);
                urux[nNode.idx] = cNode.x;
                if (verbose > 2)
                    cout << "added to nNode right: " << sorted[idx] << endl;
            } else if (ullx[nNode.idx] < cNode.x && ulux[nNode.idx] > cNode.x) {
                if (verbose > 2)
                    cout << "left for nNode cNode idx: " << sorted[idx]
                        << " nNode idx: " << nNode.idx << endl;
                ;
                if (idx == num_terms - 1) {
                    // left 
                    nn[nNode.idx].push_back(cNode.idx);
                    ullx[nNode.idx] = cNode.x;
                    if (verbose > 2)
                        cout << "added to nNode left: " << sorted[idx] << endl;
                } else {
                    if (nodes[sorted[idx + 1]].y != cNode.y || nodes[sorted[idx + 1]].x > nNode.x) {
                        nn[nNode.idx].push_back(cNode.idx);
                        ullx[nNode.idx] = cNode.x;
                        if (verbose > 2)
                            cout << "added to nNode left: " << sorted[idx] << endl;
                    }
                }
            }
        }

        int tIdx = idx;
        while (nodes[sorted[tIdx]].x == nodes[sorted[idx]].x) {
            tIdx++;
            if (tIdx == num_terms)
                break;
        }
        tIdx = tIdx - 1;
        // update neighbor to idx
        // Note: nNode.y <= cNode.y 
        for (int i = tIdx; i >= 0; --i) {
            if (idx == i)
                continue;
            Node& nNode = nodes[sorted[i]];
            if (i >= 1) {
                if (nodes[sorted[i]].y == nodes[sorted[i - 1]].y)
                    continue;
            }
            if (lrlx[cNode.idx] == nNode.x) {
                nn[cNode.idx].push_back(nNode.idx);
                lrux[cNode.idx] = nNode.x;
                lllx[cNode.idx] = nNode.x;
                if (verbose > 2)
                    cout << "added cNode center: " << nNode.idx << endl;
            } else if (lrux[cNode.idx] > nNode.x && lrlx[cNode.idx] < nNode.x) {
                // right
                nn[cNode.idx].push_back(nNode.idx);
                lrux[cNode.idx] = nNode.x;
                if (verbose > 2)
                    cout << "added cNode right: " << nNode.idx << endl;
            } else if (lllx[cNode.idx] < nNode.x && llux[cNode.idx] > nNode.x) {
                // left
                if (verbose > 2)
                    cout << "added cNode left: " << nNode.idx << endl;
                nn[cNode.idx].push_back(nNode.idx);
                lllx[cNode.idx] = nNode.x;
                if (cNode.y == nNode.y) {
                    ullx[cNode.idx] = nNode.x;
                }
            }
        }
    }
    unsigned total = 0, max = 0, size = 0, max_id = 0;
    for (unsigned i = 0; i < num_terms; ++i) {
        size = nn[i].size();
        total += size;
        if (size > max) {
            max = size;
            max_id = i;
        }
    }
    // print neighbors
    if (verbose > 2) {
        cout << "Print neighbors" << endl;
        for (unsigned i = 0; i < num_terms; ++i) {
            cout << i << "th node " << nodes[i] << endl;
            for (unsigned j = 0; j < nn[i].size(); ++j) {
                cout << "    " << nn[i][j] << " " << nodes[nn[i][j]] << endl;
            }
        }
    }

    int totNN = 0;
    for (unsigned j = 0; j < nodes.size(); ++j) /* For each terminal */ {
        totNN += nn[j].size();
    }
    avgNN = (float) totNN * 1.0 / nodes.size();

    return true;
    //End of buildNearestNeighborsForSPT function
}

void Graph::NESW_Combine(int left, int mid, int right, unsigned oct) {
    int i, j, k, y2;
    int i1;
    int i2;
    int best_i2; /* index of current best nearest-neighbor */
    int best_dist; /* distance to best nearest-neighbor      */
    int d;

    /*
       update north-east nearest neighbors accross the mid-line
     */
    i1 = left;
    i2 = mid;
    y2 = sheared[sorted[i2]].y;

    while ((i1 < mid) && (sheared[sorted[i1]].y >= y2)) {
        i1++;
    }

    if (i1 < mid) {
        best_i2 = i2;
        best_dist = dist(sheared[sorted[i1]], sheared[sorted[best_i2]]);
        i2++;

        while ((i1 < mid) && (i2 < right)) {
            if (sheared[sorted[i1]].y < sheared[sorted[i2]].y) {
                d = dist(sheared[sorted[i1]], sheared[sorted[i2]]);
                if (d < best_dist) {
                    best_i2 = i2;
                    best_dist = d;
                }
                i2++;
            } else {
                if ((nn[ sorted[i1] ][oct] == -1) ||
                        (best_dist < dist(sheared[sorted[i1]], sheared[ nn[ sorted[i1] ][oct]]))
                        ) {
                    nn[ sorted[i1] ][oct] = sorted[best_i2];
                }
                i1++;
                if (i1 < mid) {
                    best_dist = dist(sheared[sorted[i1]], sheared[sorted[best_i2]]);
                }
            }
        }

        while (i1 < mid) {
            if ((nn[ sorted[i1] ][oct] == -1) ||
                    (dist(sheared[sorted[i1]], sheared[sorted[best_i2]]) <
                    dist(sheared[sorted[i1]], sheared[ nn[ sorted[i1] ][oct]]))
                    ) {
                nn[ sorted[i1] ][oct] = sorted[best_i2];
            }
            i1++;
        }
    }

    /*
       repeat for south-west nearest neighbors
     */
    oct = (oct + 4) % 8;

    i1 = right - 1;
    i2 = mid - 1;
    y2 = sheared[sorted[i2]].y;

    while ((i1 >= mid) && (sheared[sorted[i1]].y <= y2)) {
        i1--;
    }

    if (i1 >= mid) {
        best_i2 = i2;
        best_dist = dist(sheared[sorted[i1]], sheared[sorted[best_i2]]);
        i2--;

        while ((i1 >= mid) && (i2 >= left)) {
            if (sheared[sorted[i1]].y > sheared[sorted[i2]].y) {
                d = dist(sheared[sorted[i1]], sheared[sorted[i2]]);
                if (d < best_dist) {
                    best_i2 = i2;
                    best_dist = d;
                }
                i2--;
            } else {
                if ((nn[ sorted[i1] ][oct] == -1) ||
                        (best_dist < dist(sheared[sorted[i1]], sheared[ nn[ sorted[i1] ][oct]]))
                        ) {
                    nn[ sorted[i1] ][oct] = sorted[best_i2];
                }
                i1--;
                if (i1 >= mid) {
                    best_dist = dist(sheared[sorted[i1]], sheared[sorted[best_i2]]);
                }
            }
        }

        while (i1 >= mid) {
            if ((nn[ sorted[i1] ][oct] == -1) ||
                    (dist(sheared[sorted[i1]], sheared[sorted[best_i2]]) <
                    dist(sheared[sorted[i1]], sheared[ nn[ sorted[i1] ][oct]]))
                    ) {
                nn[ sorted[i1] ][oct] = sorted[best_i2];
            }
            i1--;
        }
    }

    /*
       merge sorted[left..mid-1] with sorted[mid..right-1] by y-coordinate
     */

    i = left; /* first unprocessed element in left  list  */
    j = mid; /* first unprocessed element in right list  */
    k = left; /* first free available slot in output list */

    while ((i < mid) && (j < right)) {
        if (sheared[sorted[i]].y >= sheared[sorted[j]].y) {
            aux[k++] = sorted[i++];
        } else {
            aux[k++] = sorted[j++];
        }
    }

    /*
       copy leftovers 
     */
    while (i < mid) {
        aux[k++] = sorted[i++];
    }
    while (j < right) {
        aux[k++] = sorted[j++];
    }

    for (i = left; i < right; i++) {
        sorted[i] = aux[i];
    }
}

void Graph::NESW_NearestNeighbors(int left, int right, unsigned oct) {
    if (right == left + 1) {
        nn[ sorted[left] ][oct] = nn[ sorted[left] ][(oct + 4) % 8] = -1;
    } else {
        int mid = (left + right) / 2;

        NESW_NearestNeighbors(left, mid, oct);
        NESW_NearestNeighbors(mid, right, oct);
        NESW_Combine(left, mid, right, oct);

        if (verbose > 2) {
            cout << oct << " " << left << " " << mid << " " << right << endl;
            for (unsigned i = 0; i < num_terminals; ++i) {
                cout << "  " << nn[i][oct];
            }
            cout << endl;
        }
    }
}

void Graph::heap_insert(int p, unsigned key) {
    int k; /* hole in the heap     */
    int j; /* parent of the hole   */
    int q; /* heap_elt(j)          */

    heap_key[p] = key;

    if (heap_size == 0) {
        heap_size = 1;
        heap_elt[1] = p;
        heap_idx[p] = 1;
        if (verbose > 3) {
            cout << "    heap_elt[1]= " << heap_elt[1]
                    << "  heap_idx[p]= " << heap_idx[p] << endl;
        }
    } else {

        k = ++heap_size;
        j = k >> 1; /* k/2 */

        if (verbose > 3) {
            cout << "    k= " << k << " j= " << j << "  heap_elt[j]= " << heap_elt[j]
                    << "  heap_key[q]= " << heap_key[heap_elt[j]] << endl;
        }

        q = heap_elt[j];
        while ((j > 0) && (heap_key[q] > key)) {

            heap_elt[k] = q;
            heap_idx[q] = k;
            k = j;
            j = k >> 1; /* k/2 */
            q = heap_elt[j];

            if (verbose > 3) {
                cout << "    k= " << k << " j= " << j << "  heap_elt[j]= " << heap_elt[j]
                        << "  heap_key[q]= " << heap_key[heap_elt[j]] << endl;
            }
        }

        /* store p in the position of the hole */
        heap_elt[k] = p;
        heap_idx[p] = k;
    }
}

unsigned Graph::heap_delete_min() {
    unsigned min, last;
    unsigned k; /* hole in the heap     */
    unsigned j; /* child of the hole    */
    unsigned l_key; /* key of last point    */

    //cout << "heap size = " << heap_size << endl;
    if (heap_size == 0) /* heap is empty */
        return ( -1);

    min = heap_elt[1];
    last = heap_elt[heap_size--];
    l_key = heap_key[last];

    k = 1;
    j = 2;
    while (j <= heap_size) {

        if (heap_key[heap_elt[j]] > heap_key[heap_elt[j + 1]])
            j++;

        if (heap_key[heap_elt[j]] >= l_key)
            break; /* found a position to insert 'last' */

        /* else, sift hole down */
        heap_elt[k] = heap_elt[j]; /* Note that j <= _heap_size */
        heap_idx[heap_elt[k]] = k;
        k = j;
        j = k << 1;

        if (verbose > 3) {
            cout << "    k= " << k << " j= " << j << "  heap_size= " << heap_size << endl;
        }
    }

    if (verbose > 3) {
        cout << "    k= " << k << " last= " << last << "  min= " << min << endl;
    }
    heap_elt[k] = last;
    heap_idx[last] = k;

    heap_idx[min] = -1; /* mark the point visited */
    return min;

} /* END heap_delete_min() */

void Graph::heap_decrease_key(int p, float new_key) {
    int k; /* hole in the heap     */
    int j; /* parent of the hole   */
    int q; /* heap_elt(j)          */

    heap_key[p] = new_key;
    k = heap_idx[p];
    j = k >> 1; /* k/2 */

    q = heap_elt[j];
    if ((j > 0) && (heap_key[q] > new_key)) { /* change is needed */
        do {

            heap_elt[k] = q;
            heap_idx[q] = k;
            k = j;
            j = k >> 1; /* k/2 */
            q = heap_elt[j];
        } while ((j > 0) && (heap_key[q] > new_key));

        /* store p in the position of the hole */
        heap_elt[k] = p;
        heap_idx[p] = k;
    }
} /* END heap_decrease_key() */

void Graph::updateMinDist() {
    for (unsigned i = 0; i < num_terminals; ++i) {
        if (i == root_idx) {
            nodes[i].min_dist = 0;
        } else {
            nodes[i].min_dist = dist(nodes[i], nodes[root_idx]);
        }
    }
}

void Graph::get_children_of_node() {
    for (unsigned j = 0; j < num_terminals; ++j) {
        nodes[j].children.clear();
        for (unsigned k = 0; k < num_terminals; ++k) {
            if ((nodes[k].parent == j) && (k != nodes[root_idx].idx) && (k != j)) {
                nodes[j].children.push_back(nodes[k].idx);
            }
        }
    }
}

void Graph::print_tree() {
    for (unsigned j = 0; j < nodes.size(); ++j) /* For each terminal */ {
        unsigned child = j;
        unsigned par = nodes[j].parent;
        cout << "Node " << child << "\t(" << nodes[child].x
                << " , " << nodes[child].y << ")"
                << "\tparent[" << child << "]= " << par
                << " Level=" << nodes[child].level << endl;
    }
}

void Graph::print_tree_v2(ofstream &ofs) {
    //count ==> 1 = PD, 2 = PD-II, 3 = HVW, 4 = DAS
    for (unsigned j = 0; j < nodes.size(); ++j) /* For each terminal */ {
        unsigned child = j;
        unsigned par = nodes[j].parent;
        ofs << child << " " << nodes[child].x << " " << nodes[child].y << " " << par << endl;
    }
}

unsigned Graph::calc_tree_wl_pd() {
    int wl = 0;
    for (unsigned j = 0; j < nodes.size(); ++j) {
        unsigned child = j;
        unsigned par = nodes[j].parent;
        nodes[child].cost_edgeToP = dist(nodes[par], nodes[child]);
        wl += nodes[child].cost_edgeToP;
    }
    return (wl);
}

unsigned Graph::calc_tree_pl() {
    int pl = 0;
    for (unsigned j = 0; j < orig_num_terminals; ++j) {
        unsigned child = j;
        unsigned par = nodes[j].parent;
        while (par != child) {
            if (verbose > 2) {
                cout << "Child = " << child << " ; SV Par = " << par << endl;
            }
            nodes[child].cost_edgeToP = dist(nodes[par], nodes[child]);
            pl += nodes[child].cost_edgeToP;
            child = par;
            par = nodes[par].parent;
        }
    }
    return (pl);
}

float Graph::calc_tree_det_cost() {
    int det_cost = 0;
    for (unsigned j = 0; j < num_terminals; ++j)
        det_cost += nodes[j].det_cost_node;
    return (det_cost);
}

bool Graph::run_PD_brute_force(float alp) {
    heap_key.clear();
    heap_idx.clear();
    heap_elt.clear();
    heap_key.resize(num_terminals);
    heap_idx.resize(num_terminals);
    heap_elt.resize(num_terminals);

    heap_insert(root_idx, 0);
    nodes[root_idx].parent = root_idx;

    // update shortest path
    updateMinDist();
    unsigned count = 0;
    for (unsigned k = 0; k < num_terminals; k++) /* n points to be extracted from heap */ {
        int i = heap_delete_min();
        if (verbose > 2) {
            cout << "\n##############k=" << k << " i=" << i << endl;
            cout << "Heap_idx array: ";
            for (unsigned ar = 0; ar < heap_idx.size(); ar++) cout << heap_idx[ar] << " ";
            cout << endl;
            cout << "Heap_key array: ";
            for (unsigned ar = 0; ar < heap_key.size(); ar++) cout << heap_key[ar] << " ";
            cout << endl;
            cout << "Heap_elt array: ";
            for (unsigned ar = 0; ar < heap_elt.size(); ar++) cout << heap_elt[ar] << " ";
            cout << endl;
            cout << "Heaps min_dist: ";
            for (unsigned ar = 0; ar < heap_elt.size(); ar++) cout << nodes[heap_elt[ar]].min_dist << " ";
            cout << endl;
        }
        if (i >= 0) {
            //pt[i] entered the tree, update heap keys for its neighbors
            //BruteForcefor(unsigned oct = 0;  oct < num_terminals;  oct++ )
            unsigned par = nodes[i].parent;
            nodes[i].path_length = nodes[par].path_length + dist(nodes[i], nodes[par]);
            for (unsigned oct = 0; oct < nn[i].size(); oct++) {
                int nn1 = nn[i][oct];
                //BruteForceint nn1 = nodes[oct].idx;
                if (nn1 >= 0) {
                    if (verbose > 2)
                        cout << "NN=" << nn1 << " i=" << i << " min_dist of node i=" << nodes[i].min_dist;
                    unsigned edge_len = dist(nodes[i], nodes[nn1]);
                    float d = alp * (float) nodes[i].path_length;
                    if (verbose > 2)
                        cout << "intermediate d = alp *(float) nodes[i].path_length = " << alp
                            << "*" << nodes[i].path_length << " = " << d << endl;
                    if (nn1 != root_idx) d += edge_len;
                    if (verbose > 2)
                        cout << " d=" << d << " heap_idx[nn1]=" << heap_idx[nn1]
                            << " heap_key[nn1]=" << heap_key[nn1] << endl;
                    if ((heap_idx[nn1] > 0) && (d <= heap_key[nn1])) //FIXME : Tie-break
                    {
                        heap_decrease_key(nn1, d);
                    }
                    else if (heap_idx[nn1] == 0) {
                        heap_insert(nn1, d);
                    }
                    nodes[nn1].parent = i;
                }
            }
        }
    }

    if (verbose > 1) {
        print_tree();
    }
    pd_wl = calc_tree_wl_pd();
    pd_pl = calc_tree_pl();

    return true;
    //end of run_PD_brute_force function
}

void Graph::PDBU_new_NN() {
    buildNearestNeighborsForSPT(num_terminals);

    // Tree preparation
    for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */ {
        unsigned child = j;
        unsigned par = nodes[j].parent;
        update_edgecosts_to_parent(child, par);
        if (verbose > 2) {
            cout << "  Detour cost of edge to parent = "
                    << nodes[child].detcost_edgePToNode << endl;
            cout << "  Nearest neighbors of node " << j << " are: " << endl;
        }
        nodes[j].nn_edge_detcost.resize(nn[j].size());

        //Calculating detour cost of all edges to nearest neighbours
        update_detourcosts_to_NNs(j);
    }
    //print_tree();
    get_children_of_node();

    //Calculating detour cost of each node and the K_t value
    for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */
        update_node_detcost_Kt(j);
    // End tree preparation

    float initial_tree_cost = calc_tree_cost();
    float final_tree_cost = 0;
    float tree_cost_difference = final_tree_cost - initial_tree_cost;
    unsigned count = 1;

    while ((tree_cost_difference > 0) || (tree_cost_difference < -1)) {
        if (verbose > 2) {
            cout << "\n################# PDBU Swap Iteration "
                    << count << " #####################\n" << endl;
        }
        count++;

        if (count >= num_terminals) {
            break;
        }

        if (verbose > 2) {
            cout << "Tree before PDBU iteration " << count - 1 << endl;
            print_tree();
            cout << "\nInitial tree cost = " << initial_tree_cost << endl;
        }

        //Generating the swap space
        for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */ {
            nodes[j].swap_space.clear();
            vector<int> tmp_children = nodes[j].children;
            unsigned iter = 0;
            vector<int> tmp;
            nodes[j].swap_space.push_back(tmp);
            tmp.clear();
            nodes[j].swap_space[iter].insert(nodes[j].swap_space[iter].end(),
                    tmp_children.begin(),
                    tmp_children.end());
            iter++;
            while (iter <= num_terminals) {
                vector<int> tmp;
                nodes[j].swap_space.push_back(tmp);
                tmp.clear();
                for (unsigned k = 0; k < tmp_children.size(); k++) {
                    unsigned child = tmp_children[k];
                    if (nodes[child].children.size() > 0) {
                        nodes[j].swap_space[iter].insert(nodes[j].swap_space[iter].end(),
                                nodes[child].children.begin(),
                                nodes[child].children.end());
                    }
                }
                tmp_children = nodes[j].swap_space[iter];
                iter++;
            }
            if (verbose > 2) {
                cout << "\nj = " << j << "  Swap space: " << endl;
                for (unsigned p = 0; p < nodes[j].swap_space.size(); p++) {
                    for (unsigned q = 0; q < nodes[j].swap_space[p].size(); q++) {
                        cout << nodes[j].swap_space[p][q] << " ";
                    }
                    cout << endl;
                }
                cout << "\nSwap space size = " << nodes[j].swap_space.size() << endl;
            }
        }

        float overall_min_cost = 10000;
        int overall_min_node = -1;
        int overall_min_nn_idx = -1;
        int overall_swap_dist = -1;
        unsigned overall_initial_i = 0;

        //Min node of the entire tree
        for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */ {
            //For every row in the nodes[j].swap_space
            for (unsigned iter = 0; iter <= distance; iter++) {
                //For every element in the "iter"th row of nodes[j].swap_space
                for (unsigned idx = 0; idx < nodes[j].swap_space[iter].size(); idx++) {
                    float swap_cost = 10000;
                    //Node to get new edge
                    int node_e_new = nodes[j].swap_space[iter][idx];
                    for (unsigned oct = 0; oct < nn[node_e_new].size(); oct++) {
                        int nn_node = nn[node_e_new][oct];
                        bool is_found_in_swap_space = false;
                        if (nn_node != -1) {
                            int child = node_e_new, par = nodes[node_e_new].parent;
                            //for (unsigned i=0;i<=distance+6;i++)
                            for (unsigned i = 0; i <= num_terminals; i++) {
                                if (i < nodes[child].swap_space.size()) {
                                    for (unsigned k = 0; k < nodes[child].swap_space[i].size(); k++) {
                                        if (nn_node == nodes[child].swap_space[i][k]) {
                                            is_found_in_swap_space = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            unsigned count = 0;
                            while (count < iter) {
                                for (unsigned i = 0; i <= num_terminals; i++) {
                                    if (i < nodes[par].swap_space.size())
                                        for (unsigned k = 0; k < nodes[par].swap_space[i].size(); k++) {
                                            if (nn_node == nodes[par].swap_space[i][k]) {
                                                is_found_in_swap_space = true;
                                                break;
                                            }
                                            if (nn_node == par) {
                                                is_found_in_swap_space = true;
                                                break;
                                            }
                                        }
                                }
                                child = par;
                                par = nodes[par].parent;
                                count++;
                            }
                            if (nn_node == j) is_found_in_swap_space = true;
                        }
                        if (verbose > 2) {
                            cout << "j=" << j << " iter=" << iter
                                    << " Node getting new edge=" << node_e_new
                                    << " NN_node=" << nn_node
                                    << " Is found in swap space?" << is_found_in_swap_space
                                    << endl;
                        }
                        if ((nn_node) > -1 && (nn_node != j) && !is_found_in_swap_space) {
                            int child = node_e_new, par = nodes[node_e_new].parent;
                            float sum_q_terms = nodes[node_e_new].nn_edge_detcost[oct];
                            float cumul_det_cost_term = 0;
                            while (par != j) {
                                sum_q_terms += nodes[child].detcost_edgeNodeToP;
                                cumul_det_cost_term += (nodes[par].K_t - nodes[child].K_t) *
                                        (nodes[nn_node].det_cost_node + sum_q_terms -
                                        nodes[par].det_cost_node);
                                child = par;
                                par = nodes[par].parent;
                                if ((child == 0)&&(par == 0)) break;
                            }
                            //node_to_get_edge_removed
                            int node_e_rem = child;
                            int node_e_rem_par = j;
                            float last_len_term = (dist(nodes[nn_node], nodes[node_e_new]) -
                                    (dist(nodes[node_e_rem_par], nodes[node_e_rem])));
                            float last_det_cost_term = nodes[node_e_new].K_t * (nodes[nn_node].det_cost_node +
                                    nodes[node_e_new].nn_edge_detcost[oct] -
                                    nodes[node_e_new].det_cost_node);
                            swap_cost = alpha2 * M * (cumul_det_cost_term + last_det_cost_term) +
                                    (1 - alpha2)*(float) last_len_term;
                            if (verbose > 2) {
                                cout << " j=" << j << " iter=" << iter << " cost=" << swap_cost << endl;
                            }
                        }
                        if (swap_cost < overall_min_cost) {
                            overall_min_cost = swap_cost;
                            overall_min_node = node_e_new;
                            overall_min_nn_idx = oct;
                            overall_swap_dist = iter;
                            overall_initial_i = j;
                        }
                    }
                }
            }
            //End of the for loop which loops through each terminal
        }

        if (overall_min_cost < -0.05) {
            if (verbose > 2) {
                cout << "Overall min = " << overall_min_cost
                        << "  for node " << overall_min_node
                        << "  New parent (Nearest neighbour) is "
                        << nn[overall_min_node][overall_min_nn_idx] << endl;
                cout << "Swapping with a distance of " << overall_swap_dist << endl;
            }
            swap_and_update_tree(overall_min_node, overall_min_nn_idx,
                    distance, overall_initial_i);
        }

        final_tree_cost = calc_tree_cost();
        tree_cost_difference = final_tree_cost - initial_tree_cost;

        initial_tree_cost = final_tree_cost;
        if (verbose > 1) {
            cout << "Final tree cost = " << final_tree_cost
                    << "\ntree_cost_difference = " << tree_cost_difference << endl
                    << "Tree after PDBU" << endl;
            print_tree();
        }
        //End of the while loop
    }

    pdbu_pl = calc_tree_pl();
    pdbu_wl = calc_tree_wl_pd();
    pdbu_dc = calc_tree_det_cost();

    //End of PDBU_new_NN() function
}

int Graph::IsAdded(Node &cN) {
    for (unsigned i = 1; i < nodes.size(); ++i) {
        if (nodes[i].x == cN.x && nodes[i].y == cN.y) {
            return i;
        }
    }
    return 0;
}

void Graph::FreeManhDist() {
    for (unsigned i = 0; i < ManhDist.size(); ++i) {
        ManhDist[i].clear();
    }
    ManhDist.clear();
}

void Graph::constructSteiner() {
    vector <int> newSP;
    for (unsigned i = 1; i < dag.size(); ++i) {
        int cIdx = dag[i];
        Node child = nodes[cIdx];
        Node pN = nodes[child.parent];
        if (verbose > 2) {
            cout << "parent: " << pN << endl;
            cout << "child: " << child << endl;
        }
        Edge e = edges[cIdx];

        vector <int> toBeRemoved;
        for (unsigned j = 0; j < e.STNodes.size(); ++j) {
            Node &cN = e.STNodes[j];
            if (!IsOnEdge(cN, cIdx)) {
                continue;
            }
            if (verbose > 2) {
                cout << "Before cN: " << cN << endl;
                cout << "Before pN: " << pN << endl;
            }
            int idx = IsAdded(cN);
            // check whether cN exists in the current nodes
            if (idx == 0) {
                int pIdx = 10000;
                bool flagIdx = true;
                for (unsigned k = 0; k < cN.sp_chil.size(); ++k) {
                    if (pN.idx == cN.sp_chil[k]) {
                        flagIdx = false;
                        pIdx = nodes[pN.idx].parent;
                    }
                }
                if (flagIdx) {
                    pIdx = pN.idx;
                }

                //update current node cN
                nodes.push_back(Node(nodes.size(), cN.x, cN.y));
                edges.push_back(Edge(edges.size(), pIdx, edges.size()));
                nodes[nodes.size() - 1].parent = pIdx;
                newSP.push_back(nodes.size() - 1);
                //update parent node pN
                for (unsigned k = 0; k < cN.sp_chil.size(); ++k) {
                    nodes[nodes.size() - 1].children.push_back(cN.sp_chil[k]);

                    removeChild(nodes[pN.idx], cN.sp_chil[k]);
                    if (flagIdx == false)
                        removeChild(nodes[pIdx], cN.sp_chil[k]);
                }
                addChild(nodes[pIdx], nodes.size() - 1);
                if (verbose > 2) {
                    cout << "After cN: " << nodes[nodes.size() - 1] << endl;
                    cout << "After pN: " << nodes[pIdx] << endl;
                }
                pN = nodes[nodes.size() - 1];
            } else {
                if (nodes[idx].parent != pN.idx
                        && idx != pN.parent
                        && nodes[idx].parent != pN.parent) {
                    if (verbose > 2) {
                        cout << "Error!! " << endl;
                        cout << "cNode: " << nodes[idx] << endl;
                        cout << "pNode: " << nodes[pN.idx] << endl;
                    }
                    for (unsigned k = 0; k < newSP.size(); k++) {
                        if (newSP[k] == idx) {

                            removeChild(nodes[idx], pN.idx);
                            addChild(nodes[nodes[pN.idx].parent], pN.idx);
                            int dir1 = IdentLoc(pN.idx, idx);
                            int dir2 = IdentLoc(nodes[idx].parent, idx);
                            int dir3 = IdentLoc(pN.parent, idx);
                            if (dir1 == dir2) {
                                if ((nodes[idx].x < nodes[nodes[idx].parent].x
                                        && nodes[pN.idx].x < nodes[nodes[idx].parent].x)
                                        || (nodes[idx].x > nodes[nodes[idx].parent].x
                                        && nodes[pN.idx].x > nodes[nodes[idx].parent].x)
                                        ) {
                                    if (verbose > 2)
                                        cout << "same direction!!" << endl;

                                    removeChild(nodes[idx], pN.idx);
                                    removeChild(nodes[nodes[idx].parent], idx);

                                    bool flag2 = true;
                                    for (unsigned l = 0; l < nodes[pN.idx].children.size(); ++l) {
                                        if (nodes[pN.idx].children[l] == nodes[idx].parent) {
                                            flag2 = false;
                                            break;
                                        }
                                    }
                                    if (flag2)
                                        nodes[pN.idx].parent = nodes[idx].parent;
                                    addChild(nodes[nodes[idx].parent], pN.idx);
                                    nodes[idx].parent = pN.idx;
                                    for (unsigned l = 0; l < nodes[idx].children.size(); ++l) {
                                        removeChild(nodes[pN.idx], nodes[idx].children[l]);
                                    }
                                    addChild(nodes[pN.idx], idx);
                                    if (verbose > 2)
                                        cout << "newSP: " << nodes[idx] << endl;
                                }
                            } else if (dir1 == dir3) {
                                removeChild(nodes[idx], pN.idx);
                                addChild(nodes[pN.parent], pN.idx);
                                for (unsigned k = 0; k < cN.sp_chil.size(); ++k) {
                                    removeChild(nodes[pN.idx], cN.sp_chil[k]);
                                }
                            }
                        }
                    }
                }
                if (verbose > 2) {
                    cout << "After cN: " << nodes[idx] << endl;
                    cout << "After pN: " << nodes[pN.idx] << endl;
                }
                pN = nodes[idx];
            }
            if (j == e.STNodes.size() - 1) {
                nodes[child.idx].parent = pN.idx;
                edges[child.idx].head = pN.idx;
                edges[child.idx].best_shape = 5;
                for (unsigned k = 0; k < cN.sp_chil.size(); ++k) {
                    removeChild(nodes[child.idx], cN.sp_chil[k]);
                }
                addChild(nodes[pN.idx], child.idx);

                for (unsigned k = 0; k < pN.children.size(); ++k) {
                    removeChild(nodes[pN.children[k]], child.idx);
                }
            }
        }
    }

    FreeManhDist();
    UpdateManhDist();
    BuildDAG();
}

bool Graph::doSteiner_HoVW() {
    //unsigned orig_num_terminals = num_terminals;
    // Tree preparation
    updateMinDist();
    for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */ {
        unsigned child = j;
        unsigned par = nodes[j].parent;
        edges[j].head = par;
        edges[j].tail = child;
        update_edgecosts_to_parent(child, par);
    }

    get_children_of_node();
    get_level_in_tree();

    for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */
        update_node_detcost_Kt(j);
    // End tree preparation

    if (verbose > 3) {
        cout << "Wirelength before Steiner = " << calc_tree_wl_pd() << endl;
        cout << "Tree detour cost before Steiner = " << calc_tree_det_cost() << endl;
        cout << calc_tree_wl_pd() << " " << calc_tree_det_cost() << " ";
        print_tree();
    }

    vector<Node> set_of_nodes;
    for (int k = tree_struct.size() - 3; k >= 0; k--) //Starting from the nodes in the second level from bottom
    {
        for (unsigned l = 0; l < tree_struct[k].size(); l++) {
            unsigned child = tree_struct[k][l], par = nodes[child].parent;
            Node tmp_node = nodes[child];

            set_of_nodes.push_back(tmp_node);
            set_of_nodes.push_back(nodes[par]);
            for (unsigned m = 0; m < tmp_node.children.size(); m++)
                set_of_nodes.push_back(nodes[tmp_node.children[m]]);

            if (set_of_nodes.size() > 2) {
                get_overlap_lshape(set_of_nodes, nodes[child].idx);
            }
            set_of_nodes.clear();
        }
    }

    //Assigning best shapes top-down
    for (int k = 0; k <= tree_struct.size() - 3; k++) {
        for (unsigned l = 0; l < tree_struct[k].size(); l++) {
            unsigned curr_node = tree_struct[k][l];
            if (nodes[curr_node].children.size() > 0) {
                if (curr_node == 0) {
                    for (unsigned i = 0; i < edges[curr_node].lower_best_config.size(); i = i + 2)
                        edges[edges[curr_node].lower_best_config[i]].best_shape = edges[curr_node].lower_best_config[i + 1];
                } else if (edges[curr_node].best_shape == 0) {
                    for (unsigned i = 0; i < edges[curr_node].lower_best_config.size(); i = i + 2)
                        edges[edges[curr_node].lower_best_config[i]].best_shape = edges[curr_node].lower_best_config[i + 1];
                } else if (edges[curr_node].best_shape == 1) {
                    for (unsigned i = 0; i < edges[curr_node].upper_best_config.size(); i = i + 2)
                        edges[edges[curr_node].upper_best_config[i]].best_shape = edges[curr_node].upper_best_config[i + 1];
                }
                else if (edges[curr_node].best_shape == 5) {
                    for (unsigned i = 0; i < edges[curr_node].upper_best_config.size(); i = i + 2)
                        edges[edges[curr_node].upper_best_config[i]].best_shape = edges[curr_node].upper_best_config[i + 1];
                }
                //Condition for best_shape == 5? //SV: Not required since if current edge shape is don't care, child config won't be set
            }
        }
    }

    if (verbose > 1) {
        for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */ {
            cout << "Edge " << edges[j] << "\tLower ov=" << edges[j].lower_ov
                    << "\tUpper ov=" << edges[j].upper_ov
                    << "\tBest_ov=" << edges[j].best_ov
                    << "\tBest shape=" << edges[j].best_shape << endl;

            if (nodes[j].children.size() > 0) {
                for (unsigned i = 0; i < edges[j].lower_best_config.size(); i = i + 2)
                    cout << "\t[Lower]Child=Node" << edges[j].lower_best_config[i]
                        << " its best shape=" << edges[j].lower_best_config[i + 1]
                        << " ; ";
                cout << endl;
                for (unsigned i = 0; i < edges[j].upper_best_config.size(); i = i + 2)
                    cout << "\t[Upper]Child=Node" << edges[j].upper_best_config[i]
                        << " its best shape=" << edges[j].upper_best_config[i + 1]
                        << " ; ";
                cout << endl;
            }
        }

        unsigned wl_val = calc_tree_wl_pd(), ov_val = edges[0].best_ov;
        cout << "WL subtracting overlap=" << wl_val - ov_val
                << "  : WL before St=" << wl_val << " Overlap=" << ov_val << endl;
        cout << "Starting refineSteiner" << endl;
    }

    FreeManhDist();
    UpdateManhDist();

    // update and print out HVW Steiner solutions
    st_wl = calc_tree_wl_pd();
    st_pl = calc_tree_pl();
    st_dc = calc_tree_det_cost();

    return (true);
    //End of doSteiner_HoVW
}

bool Graph::fix_max_dc() {

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());

    UpdateAllEdgesNSEW();

    //cout << "Starting refineSteiner" << endl;
    refineSteiner2();
    constructSteiner();
    //print_tree_v2();

    FreeManhDist();
    UpdateManhDist();

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());
    UpdateAllEdgesNSEW();

    //cout << "Starting refineSteiner" << endl;
    refineSteiner();

    if (verbose > 1) {
        cout << "Finished refineSteiner" << endl;
        cout << "Starting Steiner tree construction" << endl;
    }

    constructSteiner();

    if (verbose > 2) {
        cout << "Finished Steiner tree construction" << endl;
        PrintInfo();
        cout << "Start second refineStiner" << endl;
    }

    FreeManhDist();
    UpdateManhDist();

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());

    UpdateAllEdgesNSEW();

    //cout << "Starting refineSteiner" << endl;
    refineSteiner();

    if (verbose > 2) {
        cout << "Finished second refineSteiner" << endl;
        cout << "Starting Steiner tree construction" << endl;
    }

    constructSteiner();
    if (verbose > 2) {
        cout << "Finished Steiner tree construction" << endl;
        PrintInfo();
        cout << "Remove unnecessary Steiner nodes" << endl;
    }

    RemoveUnneceSTNodes();

    FreeManhDist();
    UpdateManhDist();

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());

    UpdateAllEdgesNSEW();

    if (verbose > 2) {
        cout << "Post-Steiner node removal" << endl;
        PrintInfo();
        cout << "Start third refineSteiner" << endl;
    }

    //cout << "Starting refineSteiner" << endl;
    refineSteiner();

    if (verbose > 2) {
        cout << "Finished third refineSteiner" << endl;
        cout << "Starting Steiner tree construction" << endl;
    }

    constructSteiner();

    RemoveUnneceSTNodes();

    FreeManhDist();
    UpdateManhDist();

    //DAG traversal
    BuildDAG();

    buildNearestNeighborsForSPT(nodes.size());

    UpdateAllEdgesNSEW();

    refineSteiner();
    constructSteiner();
    if (verbose > 2) {
        cout << "Post-Steiner construction" << endl;
        PrintInfo();
    }

    st_wl = calc_tree_wl_pd();
    st_pl = calc_tree_pl();
    st_dc = calc_tree_det_cost();

    vector<float> st_mds_dc(2);
    find_max_dc_node(st_mds_dc);
    unsigned use_nn = 0;
    if (verbose > 2) {
        cout << "tree struct 1d array: ";
        for (unsigned k = 0; k < tree_struct_1darr.size(); k++)
            cout << tree_struct_1darr[k] << " ";
        cout << "Size = " << tree_struct_1darr.size() << endl;
    }

    float init_wl = st_wl;
    for (int k = 1; k < tree_struct_1darr.size(); k++) {
        int cnode = tree_struct_1darr[k];
        float cnode_dc = (float) nodes[cnode].det_cost_node;
        if (cnode_dc > 0) {
            if (use_nn == 1) {
                buildNearestNeighbors_single_node(num_terminals, cnode);
                update_detourcosts_to_NNs(cnode);
            }
            unsigned cpar = nodes[cnode].parent;
            int edge_len_to_par = dist(nodes[cnode], nodes[cpar]);
            int new_edge_len, nn2, det_cost_new_edge, size = 0, new_tree_wl = 0, diff_in_wl, diff_in_dc = 0;
            float new_dc, min_dc = cnode_dc;
            int min_dc_nn = -1, min_dc_new_tree_wl = 0, min_dc_edge_len = edge_len_to_par;

            if (use_nn == 1) {
                size = nn[cnode].size();
            } else {
                size = k;
            }

            for (unsigned ag = 0; ag < size; ag++) {
                if (use_nn == 1) {
                    nn2 = nn[cnode][ag];
                } else {
                    nn2 = tree_struct_1darr[ag];
                }
                if (verbose > 2) cout << "NN=" << nn2 << endl;
                if (nn2 != cpar) {
                    new_edge_len = dist(nodes[cnode], nodes[nn2]);
                    diff_in_wl = new_edge_len - edge_len_to_par;
                    new_tree_wl = init_wl + diff_in_wl;
                    if (((float) (new_tree_wl - st_wl) / (float) st_wl) <= alpha3) {
                        if (use_nn == 1) {
                            new_dc = nodes[cnode].nn_edge_detcost[ag] + nodes[nn2].det_cost_node;
                            if (verbose > 2) {
                                cout << "NN " << nn2
                                        << "\t its node DC=" << nodes[nn2].det_cost_node
                                        << "\tDC of edge to nn = "
                                        << nodes[cnode].nn_edge_detcost[ag]
                                        << "\tNew DC of cnode=" << new_dc
                                        << "\tNew edge length = " << new_edge_len << endl;
                            }
                        } else {
                            det_cost_new_edge = nodes[nn2].min_dist + dist(nodes[nn2], nodes[cnode]) -
                                    nodes[cnode].min_dist;
                            new_dc = det_cost_new_edge + nodes[nn2].det_cost_node;
                            if (verbose > 2) {
                                cout << "NN " << nn2
                                        << "\t its node DC=" << nodes[nn2].det_cost_node
                                        << "\tDC of edge to nn = " << det_cost_new_edge
                                        << "\tNew DC of cnode=" << new_dc
                                        << "\tNew edge length = " << new_edge_len << endl;
                            }
                        }
                        if (new_dc < cnode_dc) {
                            if (new_dc < min_dc) {
                                min_dc = new_dc;
                                min_dc_nn = nn2;
                                min_dc_edge_len = new_edge_len;
                                min_dc_new_tree_wl = new_tree_wl;
                            }
                        }
                    }
                }
            }
            if (min_dc_nn != -1) {
                vector<int> chi = nodes[cnode].children, tmp;
                float init_dc = cnode_dc, final_dc = min_dc, change = init_dc - final_dc;
                unsigned id = 0, count = 0;
                while (chi.size() != 0) {
                    tmp = chi;
                    chi.clear();
                    for (id = 0; id < tmp.size(); id++) {
                        init_dc += nodes[tmp[id]].det_cost_node;
                        count++;
                        chi.insert(chi.end(), nodes[tmp[id]].children.begin(), nodes[tmp[id]].children.end());
                    }
                    tmp.clear();
                }
                final_dc = init_dc - count*change;
                chi.clear();
                tmp.clear();
                if ((init_dc - final_dc) / init_dc >= alpha4) {
                    nodes[cnode].parent = min_dc_nn;
                    nodes[cnode].det_cost_node = min_dc;
                    diff_in_dc = cnode_dc - min_dc;
                    init_wl = min_dc_new_tree_wl;
                    //Update the DCs of the nodes in the subtree of k
                    vector<int> chi = nodes[cnode].children, tmp;
                    unsigned id = 0;
                    while (chi.size() != 0) {
                        tmp = chi;
                        chi.clear();
                        for (id = 0; id < tmp.size(); id++) {
                            nodes[tmp[id]].det_cost_node -= diff_in_dc;
                            chi.insert(chi.end(), nodes[tmp[id]].children.begin(), nodes[tmp[id]].children.end());
                        }
                        tmp.clear();
                    }
                    chi.clear();
                    tmp.clear();
                }
            }
        }
    }

    ////Final WL, DC calculation
    daf_wl = calc_tree_wl_pd();
    daf_pl = calc_tree_pl();
    daf_dc = calc_tree_det_cost();
    vector<float> daf_mds_dc(2);
    find_max_dc_node(daf_mds_dc);
    if (daf_mds_dc[0] == 0) daf_mds_dc[0] = st_mds_dc[0];
    unsigned change = 0;
    //Temp vector<double> ed_daf(2);
    if ((daf_wl < st_wl) || (daf_dc < st_dc)) {
        change = 1;
    }

    return true;
} //End of fix_max_dc fn

void Graph::generate_permutations(vector < vector<unsigned> > lists,
        vector < vector<unsigned> > &result,
        unsigned depth, vector<unsigned> current) {
    if (depth == lists.size()) {
        result.push_back(current); //result.add(current);
        return;
    }
    for (unsigned i = 0; i < lists[depth].size(); ++i) {
        vector<unsigned> tmp = current;
        tmp.push_back(lists[depth][i]);
        generate_permutations(lists, result, depth + 1, tmp);
    }
}

bool Graph::get_overlap_lshape(vector<Node>& set_of_nodes, int index) {

    vector < vector<unsigned> > lists, result;
    vector<unsigned> tmp1, tmp2;
    tmp1.push_back(0);
    tmp1.push_back(1);

    //Enumerate all possible options for children
    for (unsigned i = 2; i < set_of_nodes.size(); i++) lists.push_back(tmp1);

    generate_permutations(lists, result, 0, tmp2);
    //Lower of curr_edge
    //For each combination, calc overlap
    unsigned max_ov = 0;
    vector<int> best_config;
    vector<Node> best_sps_x, best_sps_y;
    unsigned best_sps_curr_node_idx_x = 9999999, best_sps_curr_node_idx_y = 9999999;
    //unsigned best_sps_curr_node_idx_x=INT_MAX, best_sps_curr_node_idx_y=INT_MAX;
    vector<unsigned> all_lower_ovs;

    for (unsigned i = 0; i < result.size(); i++) {
        vector < vector<Node> > set_of_points;
        vector<Node> tmp3;
        tmp3.push_back(set_of_nodes[0]);
        tmp3.push_back(Node(0, set_of_nodes[1].x, set_of_nodes[0].y));
        tmp3.push_back(set_of_nodes[1]);
        set_of_points.push_back(tmp3);
        unsigned lower_ov = 0;
        vector<int> config;
        for (unsigned j = 2; j < set_of_nodes.size(); j++) {
            if (result[i][j - 2] == 0) {
                vector<Node> tmp4;
                tmp4.push_back(set_of_nodes[0]);
                tmp4.push_back(Node(0, set_of_nodes[0].x, set_of_nodes[j].y));
                tmp4.push_back(set_of_nodes[j]);
                set_of_points.push_back(tmp4);
                tmp4.clear();
                lower_ov += edges[set_of_nodes[j].idx].lower_ov;
                config.push_back(set_of_nodes[j].idx);
                config.push_back(0);
            } else if (result[i][j - 2] == 1) {
                vector<Node> tmp5;
                tmp5.push_back(set_of_nodes[0]);
                tmp5.push_back(Node(0, set_of_nodes[j].x, set_of_nodes[0].y));
                tmp5.push_back(set_of_nodes[j]);
                set_of_points.push_back(tmp5);
                tmp5.clear();
                lower_ov += edges[set_of_nodes[j].idx].upper_ov;
                config.push_back(set_of_nodes[j].idx);
                config.push_back(1);
            }
        }

        //Calculation of overlaps
        unsigned num_edges = set_of_points.size();
        unsigned curr_level_ov = calc_overlap(set_of_points);
        lower_ov += curr_level_ov;
        result[i].push_back(lower_ov);
        if (lower_ov >= max_ov) {
            max_ov = lower_ov;
            best_config = config;
            best_sps_x.clear();
            best_sps_y.clear();
            best_sps_curr_node_idx_x = 9999999;
            best_sps_curr_node_idx_y = 9999999;
            if (set_of_points.size() != num_edges) {
                best_sps_x = set_of_points[num_edges];
                best_sps_curr_node_idx_x = nodes[index].idx_of_cn_x;
                best_sps_y = set_of_points[num_edges + 1];
                best_sps_curr_node_idx_y = nodes[index].idx_of_cn_y;
            }
        }
        for (unsigned i = 0; i < set_of_points.size(); ++i) set_of_points[i].clear();
        set_of_points.clear();
        config.clear();
    }

    //New part added from here
    //Count of Max_ov value appearing in the results combination
    unsigned max_ap_cnt = 0;
    unsigned res_size = result[0].size(), not_dont_care_flag = 0;
    vector<unsigned> tmp_res, not_dont_care_child;
    for (unsigned p = 0; p < result.size(); p++) {
        if (max_ov == result[p][res_size - 1]) {
            max_ap_cnt++;
            //Set first row which matches as reference row
            if (max_ap_cnt == 1) {
                tmp_res = result[p];
            }
            else if (max_ap_cnt > 1) {
                for (unsigned idk = 0; idk < res_size - 1; idk++) {
                    if (res_size == 2) {
                        not_dont_care_flag++;
                    } else {
                        if (result[p][idk] == tmp_res[idk]) {
                            not_dont_care_flag++;
                            not_dont_care_child.push_back(idk);
                        }
                    }
                }
            }
        }
    }
    int dont_care_flag = res_size - 1 - not_dont_care_flag;
    //Only if dont_care_flag = number of repeating rows - 1, then make the child edge dont_care
    if ((dont_care_flag != 0) && (dont_care_flag == max_ap_cnt - 1)) {
        vector<unsigned> List1;
        for (unsigned mm = 0; mm < result[0].size() - 1; mm++) List1.push_back(mm);
        vector<unsigned> dont_care_child;
        //Get dont care child index by removing the rest of the children indices
        copy_if(List1.begin(), List1.end(), back_inserter(dont_care_child),
                [&not_dont_care_child](const unsigned& arg) {
                    return (find(not_dont_care_child.begin(), not_dont_care_child.end(), arg) == not_dont_care_child.end());
                }
        );

        if (best_config.size() != 0) {
            for (unsigned mm = 0; mm < dont_care_child.size(); mm++) {
                best_config[dont_care_child[mm]*2 + 1] = 5;
            }
        }
        dont_care_child.clear();
        List1.clear();
    }
    tmp_res.clear();
    not_dont_care_child.clear();

    edges[index].lower_ov = max_ov;
    edges[index].lower_best_config = best_config;
    edges[index].lower_sps_to_be_added_x = best_sps_x;
    edges[index].lower_sps_to_be_added_y = best_sps_y;

    edges[index].lower_idx_of_cn_x = best_sps_curr_node_idx_x;
    edges[index].lower_idx_of_cn_y = best_sps_curr_node_idx_y;
    best_config.clear();
    best_sps_x.clear();
    best_sps_y.clear();
    max_ov = 0;
    for (unsigned i = 0; i < result.size(); i++) {
        vector < vector<Node> > set_of_points;
        vector<Node> tmp3;
        tmp3.push_back(set_of_nodes[0]);
        tmp3.push_back(Node(0, set_of_nodes[0].x, set_of_nodes[1].y));
        tmp3.push_back(set_of_nodes[1]);
        set_of_points.push_back(tmp3);
        unsigned upper_ov = 0;
        vector<int> config;
        for (unsigned j = 2; j < set_of_nodes.size(); j++) {
            if (result[i][j - 2] == 0) {
                vector<Node> tmp4;
                tmp4.push_back(set_of_nodes[0]);
                tmp4.push_back(Node(0, set_of_nodes[0].x, set_of_nodes[j].y));
                tmp4.push_back(set_of_nodes[j]);
                set_of_points.push_back(tmp4);
                upper_ov += edges[set_of_nodes[j].idx].lower_ov;
                config.push_back(set_of_nodes[j].idx);
                config.push_back(0);
            } else if (result[i][j - 2] == 1) {
                vector<Node> tmp5;
                tmp5.push_back(set_of_nodes[0]);
                tmp5.push_back(Node(0, set_of_nodes[j].x, set_of_nodes[0].y));
                tmp5.push_back(set_of_nodes[j]);
                set_of_points.push_back(tmp5);
                upper_ov += edges[set_of_nodes[j].idx].upper_ov;
                config.push_back(set_of_nodes[j].idx);
                config.push_back(1);
            }
        }

        //Calculation of overlaps
        unsigned num_edges = set_of_points.size();
        unsigned curr_level_ov = calc_overlap(set_of_points);
        upper_ov += curr_level_ov;
        int last_res_idx = result[i].size() - 1;
        result[i][last_res_idx] = upper_ov;
        if (upper_ov >= max_ov) {
            max_ov = upper_ov;
            best_config = config;
            best_sps_curr_node_idx_x = 9999999;
            best_sps_curr_node_idx_y = 9999999;
            best_sps_x.clear();
            best_sps_y.clear();
            if (set_of_points.size() != num_edges) {
                best_sps_x = set_of_points[num_edges];
                best_sps_curr_node_idx_x = nodes[index].idx_of_cn_x;
                best_sps_y = set_of_points[num_edges + 1];
                best_sps_curr_node_idx_y = nodes[index].idx_of_cn_y;
            }
        }
        for (unsigned i = 0; i < set_of_points.size(); ++i) set_of_points[i].clear();
        set_of_points.clear();
        config.clear();
    }

    //New part added from here
    max_ap_cnt = 0; //Count of Max_ov value appearing in the results combination
    res_size = result[0].size();
    not_dont_care_flag = 0;
    for (unsigned p = 0; p < result.size(); p++) {
        if (max_ov == result[p][res_size - 1]) {
            max_ap_cnt++;
            if (max_ap_cnt == 1) {
                tmp_res = result[p];
            } else if (max_ap_cnt > 1) {
                for (unsigned idk = 0; idk < res_size - 1; idk++) {
                    if (res_size == 2) {
                        not_dont_care_flag++;
                    } else {
                        if (result[p][idk] == tmp_res[idk]) {
                            not_dont_care_flag++;
                            not_dont_care_child.push_back(idk);
                        }
                    }
                }
            }
        }
    }
    dont_care_flag = res_size - 1 - not_dont_care_flag;

    //Only if dont_care_flag = number of repeating rows - 1, then make the child edge dont_care
    if ((dont_care_flag != 0) && (dont_care_flag == max_ap_cnt - 1)) {
        vector<unsigned> List1;
        for (unsigned mm = 0; mm < result[0].size() - 1; mm++) List1.push_back(mm);
        vector<unsigned> dont_care_child;
        //Get dont care child index by removing the rest of the children indices
        copy_if(List1.begin(), List1.end(), back_inserter(dont_care_child),
                [&not_dont_care_child](const unsigned& arg) {
                    return (find(not_dont_care_child.begin(), not_dont_care_child.end(), arg) == not_dont_care_child.end());
                }
        );
        if (best_config.size() != 0) {
            for (unsigned mm = 0; mm < dont_care_child.size(); mm++) {
                best_config[dont_care_child[mm]*2 + 1] = 5;
            }
        }
        dont_care_child.clear();
        List1.clear();
    }
    tmp_res.clear();
    not_dont_care_child.clear();
    //New part added till here
    edges[index].upper_ov = max_ov;
    edges[index].upper_best_config = best_config;
    edges[index].upper_sps_to_be_added_x = best_sps_x;
    edges[index].upper_sps_to_be_added_y = best_sps_y;
    edges[index].upper_idx_of_cn_x = best_sps_curr_node_idx_x;
    edges[index].upper_idx_of_cn_y = best_sps_curr_node_idx_y;
    best_config.clear();
    best_sps_x.clear();
    best_sps_y.clear();

    //Choosing the best
    if (edges[index].lower_ov > edges[index].upper_ov) {
        edges[index].best_ov = edges[index].lower_ov;
        edges[index].best_shape = 0;
    } else if (edges[index].lower_ov < edges[index].upper_ov) {
        edges[index].best_ov = edges[index].upper_ov;
        edges[index].best_shape = 1;
    }
    else {
        edges[index].best_ov = edges[index].lower_ov;
        edges[index].best_shape = 5;
    }

    for (unsigned i = 0; i < lists.size(); i++) lists[i].clear();
    for (unsigned i = 0; i < result.size(); i++) result[i].clear();
    lists.clear();
    result.clear();
    tmp1.clear();
    tmp2.clear();
    set_of_nodes.clear();

    return (true);
    //End of get_overlap_lshape fn
}

bool Graph::segmentIntersection(std::pair<double, double> A, std::pair<double, double> B,
                                std::pair<double, double> C, std::pair<double, double> D,
                                std::pair<double, double> &out) {
    double x, y;
    double a1 = B.second - A.second; 
    double b1 = A.first - B.first; 
    double c1 = a1*(A.first) + b1*(A.second);
    
    double a2 = D.second - C.second; 
    double b2 = C.first - D.first; 
    double c2 = a2*(C.first)+ b2*(C.second);
    
    double determinant = a1*b2 - a2*b1;
    
    if (A == B && C == D && A != C) {
        return false;
    }
  
    if (determinant == 0) {
        if (A == B) {
            x = A.first;
            y = A.second;
            
            if (x <= max(C.first, D.first) && x >= min(C.first, D.first) && 
                y <= max(C.second, D.second) && y >= min(C.second, D.second)) {
                std::pair<double, double> intersect(x, y);
                out = intersect;
                return true; 
            }
        }
        
        if (C == D) {
            x = C.first;
            y = C.second;
            
            if (x <= max(A.first, B.first) && x >= min(A.first, B.first) && 
                y <= max(A.second, B.second) && y >= min(A.second, B.second)) {
                std::pair<double, double> intersect(x, y);
                out = intersect;
                return true; 
            }
        }
        
        if (A == C || A == D) {
            x = A.first;
            y = A.second;
            
            std::pair<double, double> intersect(x, y);
            out = intersect;
            return true;
        }
        
        if (B == C || B == D) {
            x = B.first;
            y = B.second;
            
            std::pair<double, double> intersect(x, y);
            out = intersect;
            return true;
        }
        
        return false;
    } else {
        x = (b2*c1 - b1*c2)/determinant;
        y = (a1*c2 - a2*c1)/determinant;
        
        if (x <= max(A.first, B.first) && x >= min(A.first, B.first) && 
            y <= max(A.second, B.second) && y >= min(A.second, B.second) &&
            x <= max(C.first, D.first) && x >= min(C.first, D.first) && 
            y <= max(C.second, D.second) && y >= min(C.second, D.second)) {
            std::pair<double, double> intersect(x, y);
            out = intersect;
            return true; 
        } else {
            return false;
        }
    }
}

void Graph::intersection(const std::vector<std::pair<double, double>> l1, 
                         const std::vector<std::pair<double, double>> l2, 
                         std::vector<std::pair<double, double>> &out) {
    std::vector<std::pair<double, double>> tmpVec;
    for (int i = 0; i < l1.size() - 1; i++) {
        for (int j = 0; j < l2.size() - 1; j++) {
            std::pair<double, double> intersect;
            if (segmentIntersection(l1[i], l1[i+1], l2[j], l2[j+1], intersect)) {
                tmpVec.push_back(intersect);
            }
        }
    }
    
    out = tmpVec;
}

double Graph::length(std::vector<std::pair<double, double>> l) {
    double totalLen = 0;
    
    if (l.size() <= 1) {
        return 0;
    }
    
    for (int i = 0; i < l.size()-1; i++) {
        double tmpLen = std::sqrt((std::pow((l[i+1].first - l[i].first), 2) + 
                              std::pow((l[i+1].second - l[i].second), 2)));
        totalLen += tmpLen;
    }

    return totalLen;
}

unsigned Graph::calc_overlap(vector < vector<Node> > &set_of_nodes) {
    unsigned max_ov = 0, tmp_ov = 0;
    typedef std::pair<double, double> s_point;
    Node curr_node = set_of_nodes[0][0];
    vector<Node> all_pts, sorted_x, sorted_y;
    for (int i = 0; i < set_of_nodes.size(); i++) {
        for (unsigned j = i + 1; j < set_of_nodes.size(); j++) {
            vector<Node> n = set_of_nodes[i];
            vector<Node> m = set_of_nodes[j];
            
            s_point pn0((double)n[0].x, (double)n[0].y);
            s_point pn1((double)n[1].x, (double)n[1].y);
            s_point pn2((double)n[2].x, (double)n[2].y);
            
            s_point pm0((double)m[0].x, (double)m[0].y);
            s_point pm1((double)m[1].x, (double)m[1].y);
            s_point pm2((double)m[2].x, (double)m[2].y);
            
            std::vector<s_point> line1 {pn0, pn1, pn2};
            std::vector<s_point> line2 {pm0, pm1, pm2};
            std::vector<s_point> output;
            
            intersection(line1, line2, output);

            tmp_ov = (unsigned) length(output);
            
//            Known Problem - output-double, when converting to int for output_nodes, 232 becomes 231 for somereason
            vector<Node> output_nodes;
            for (unsigned g = 0; g < output.size(); g++) output_nodes.push_back(Node(0, output[g].first, output[g].second));

            make_unique(output_nodes);

            if (output_nodes.size() > 1) {
                for (unsigned k = 0; k < output_nodes.size(); ++k) {
                    //set flag if this node == curr_node
                    unsigned curr_node_flag = 0;
                    if ((output_nodes[k].x == curr_node.x)&&(output_nodes[k].y == curr_node.y)) {
                        curr_node_flag = 1;
                    }
                    //If not present in all_pts
                    bool is_present = false;
                    unsigned all_pts_idx = 9999999;
                    for (unsigned s = 0; s < all_pts.size(); s++) {
                        if ((output_nodes[k].x == all_pts[s].x)&&(output_nodes[k].y == all_pts[s].y)) {
                            if (curr_node_flag == 1) {
                                is_present = true;
                                all_pts_idx = s;
                            } //Add anyway if not curr_node
                        }
                    }
                    if (!is_present) {
                        if (i == 0) {
                            output_nodes[k].conn_to_par = true;
                            output_nodes[k].sp_chil.push_back(m[2].idx);
                        } else {
                            output_nodes[k].sp_chil.push_back(n[2].idx);
                            output_nodes[k].sp_chil.push_back(m[2].idx);
                        }
                    } else {
                        if (i == 0) {
                            all_pts[all_pts_idx].conn_to_par = true;
                            all_pts[all_pts_idx].sp_chil.push_back(m[2].idx);
                        } else {
                            all_pts[all_pts_idx].sp_chil.push_back(n[2].idx);
                            all_pts[all_pts_idx].sp_chil.push_back(m[2].idx);
                        }
                    }

                    if (!is_present) {
                        if (output_nodes.size() == 2) {
                            if ((output_nodes[0].x == output_nodes[1].x) ||
                                    (output_nodes[0].y == output_nodes[1].y))
                                all_pts.push_back(output_nodes[k]);
                        } else {
                            all_pts.push_back(output_nodes[k]);
                        }
                    }
                }
            }
            output.clear();
            n.clear();
            m.clear();
            output_nodes.clear();
        }
    }
    if (all_pts.size() > 1) {
        unsigned posn_of_cn = 9999999;
        for (unsigned u = 0; u < all_pts.size(); u++) if ((all_pts[u].x == curr_node.x) && (all_pts[u].y == curr_node.y)) {
                posn_of_cn = u;
                break;
            }
        for (unsigned u = 0; u < all_pts.size(); u++) {
            if (all_pts[u].x == all_pts[posn_of_cn].x) sorted_y.push_back(all_pts[u]);
            if (all_pts[u].y == all_pts[posn_of_cn].y) sorted_x.push_back(all_pts[u]);
        }
        sort(sorted_x.begin(), sorted_x.end(), comp_x);
        sort(sorted_y.begin(), sorted_y.end(), comp_y);
        for (unsigned u = 0; u < sorted_x.size(); u++)
            if ((sorted_x[u].x == curr_node.x) && (sorted_x[u].y == curr_node.y)) {
                nodes[curr_node.idx].idx_of_cn_x = u;
                break;
            }
        for (unsigned u = 0; u < sorted_y.size(); u++)
            if ((sorted_y[u].x == curr_node.x) && (sorted_y[u].y == curr_node.y)) {
                nodes[curr_node.idx].idx_of_cn_y = u;
                break;
            }
        set_of_nodes.push_back(sorted_x);
        set_of_nodes.push_back(sorted_y);
        unsigned ov_x = 0, ov_y = 0;
        if (sorted_x.size() > 1) {
            char x[] = "x";
            ov_x = calc_ov_x_or_y(sorted_x, curr_node, x);
        }
        if (sorted_y.size() > 1) {
            char y[] = "y";
            ov_y = calc_ov_x_or_y(sorted_y, curr_node, y);
        }
        max_ov = ov_x + ov_y;
    }
    all_pts.clear();
    return max_ov;
}

unsigned Graph::calc_ov_x_or_y(vector<Node>& sorted, Node curr_node, char tag[]) {
    unsigned ov1 = 0, ov2 = 0;
    vector<unsigned> tmp_ov, tmp;
    int ind_of_curr_node = 0;
    for (int i = 0; i < sorted.size(); i++) //Getting position of "curr_node" 
        if ((sorted[i].x == curr_node.x) && (sorted[i].y == curr_node.y)) {
            ind_of_curr_node = i;
            break;
        }
    if (ind_of_curr_node > 0) {
        unsigned cnt = 0;
        for (int j = ind_of_curr_node - 1; j >= 0; j--) {
            tmp.push_back(0);
            if (strcmp(tag, "x") == 0) tmp[cnt] = sorted[j + 1].x - sorted[j].x;
            if (strcmp(tag, "y") == 0) tmp[cnt] = sorted[j + 1].y - sorted[j].y;
            cnt++;
        }
        cnt = 0;
        unsigned s = tmp.size(); //cout << "Size of tmp = " << s << endl;
        for (unsigned j = 0; j < s; j++) {
            tmp_ov.push_back(0);
            tmp_ov[j] = tmp[j]*(s - j);
        }
        for (unsigned j = 0; j < s; j++) ov1 += tmp_ov[j];

        if (verbose > 3) cout << "\tov1 = " << ov1 << endl;
        tmp_ov.clear();
        tmp.clear();
    }
    if (ind_of_curr_node < (sorted.size() - 1)) {
        unsigned cnt = 0;
        for (unsigned j = ind_of_curr_node + 1; j <= sorted.size() - 1; j++) {
            tmp.push_back(0);
            if (strcmp(tag, "x") == 0) tmp[cnt] = sorted[j].x - sorted[j - 1].x;
            if (strcmp(tag, "y") == 0) tmp[cnt] = sorted[j].y - sorted[j - 1].y;
            cnt++;
        }
        cnt = 0;
        unsigned s = tmp.size(); //cout << "Size of tmp = " << s << endl;
        for (unsigned j = 0; j < s; j++) {
            tmp_ov.push_back(0);
            tmp_ov[j] = tmp[j]*(s - j);
        }
        for (unsigned j = 0; j < s; j++) ov2 += tmp_ov[j];
        tmp_ov.clear();
        tmp.clear();
    }
    tmp_ov.clear();
    tmp.clear();
    return (ov1 + ov2);
}

void Graph::update_edgecosts_to_parent(unsigned child, unsigned par) {
    nodes[child].cost_edgeToP = dist(nodes[par], nodes[child]);
    nodes[child].detcost_edgePToNode = nodes[par].min_dist + dist(nodes[par], nodes[child]) -
            nodes[child].min_dist;
    nodes[child].detcost_edgeNodeToP = nodes[child].min_dist + dist(nodes[par], nodes[child]) -
            nodes[par].min_dist;
}

void Graph::update_node_detcost_Kt(unsigned j) {
    unsigned par = nodes[j].parent;
    unsigned child = j;
    unsigned count = 1;
    nodes[j].det_cost_node = 0;
    while (par != child) {
        nodes[j].det_cost_node += nodes[child].detcost_edgePToNode;
        nodes[par].K_t++;
        child = par;
        par = nodes[par].parent;
        count++;
        if (count > 1000) break;
    }
    if (verbose > 3) {
        cout << "  parent[" << j << "]= " << nodes[j].parent;
        cout << "  Detour cost of node = " << nodes[j].det_cost_node << endl;
    }
}

void Graph::get_level_in_tree() {
    tree_struct.clear();
    unsigned iter = 0;
    vector<int> tmp1;
    tree_struct.push_back(tmp1);
    tmp1.clear();
    tree_struct[iter].push_back(0);
    iter++;
    unsigned j = 0, level_count = 0;
    vector<int> set_of_chi = nodes[j].children, tmp;
    vector<int> tmp2;
    tree_struct.push_back(tmp2);
    tmp2.clear();
    tree_struct[iter].insert(tree_struct[iter].end(), set_of_chi.begin(), set_of_chi.end());
    unsigned size_of_chi = set_of_chi.size();
    nodes[j].level = level_count;
    while (size_of_chi != 0) {
        level_count++;
        iter++;
        tmp = set_of_chi;
        set_of_chi.clear();
        for (unsigned l = 0; l < tmp.size(); l++) {
            nodes[tmp[l]].level = level_count;
            set_of_chi.insert(set_of_chi.end(), nodes[tmp[l]].children.begin(),
                    nodes[tmp[l]].children.end());
        }

        vector<int> tmp3;
        tree_struct.push_back(tmp3);
        tmp3.clear();
        tree_struct[iter].insert(tree_struct[iter].end(), set_of_chi.begin(),
                set_of_chi.end());
        size_of_chi = set_of_chi.size();
    }
    tmp.clear();
    set_of_chi.clear();
}

void Graph::update_detourcosts_to_NNs(unsigned j) {
    unsigned child = j;
    for (unsigned oct = 0; oct < nn[child].size(); oct++) {
        int nn_node = nn[child][oct];
        int det_cost_nn_edge;
        if ((nn_node > -1) && (nodes[nn_node].parent != j)) {
            det_cost_nn_edge = nodes[nn_node].min_dist
                    + dist(nodes[nn_node], nodes[child])
                    - nodes[child].min_dist;
            if (verbose > 3) {
                cout << "Node " << nn_node
                        << " (" << nodes[nn_node].x << " , " << nodes[nn_node].y
                        << ") with detcost = " << det_cost_nn_edge << endl;
            }
        } else {
            det_cost_nn_edge = 10000;
        }
        nodes[j].nn_edge_detcost[oct] = det_cost_nn_edge;
    }
}

void Graph::swap_and_update_tree(unsigned min_node, int nn_idx, unsigned distance, unsigned i_node) {
    unsigned child = min_node, par = nodes[child].parent;
    while ((par != i_node) && (par != 0)) {
        unsigned tmp_par = nodes[par].parent;
        nodes[par].parent = child;
        update_edgecosts_to_parent(par, nodes[par].parent);
        child = par;
        par = tmp_par;
    }

    nodes[min_node].parent = nn[min_node][nn_idx];

    update_edgecosts_to_parent(min_node, nodes[min_node].parent);
    if (verbose > 3) {
        cout << " Child " << min_node
                << " New parent " << nodes[min_node].parent << endl;
    }

    for (unsigned j = 0; j < num_terminals; ++j)
        nodes[j].K_t = 1; //Resetting the K_t value for each node 

    for (unsigned j = 0; j < num_terminals; ++j) /* For each terminal */ {
        update_detourcosts_to_NNs(j);
        update_node_detcost_Kt(j);
    }
    get_children_of_node();
}

float Graph::calc_tree_cost() {
    float tree_cost = 0;
    int det_cost = 0, wl = 0;
    for (unsigned j = 0; j < num_terminals; ++j) {
        wl += abs(nodes[j].cost_edgeToP);
        det_cost += nodes[j].det_cost_node;
    }
    tree_cost = alpha2 * M * (float) det_cost + (1 - alpha2)*(float) wl;
    return (tree_cost);
}

void Graph::BuildDAG() {
    vector <bool> isVisited(nodes.size(), false);
    dag.clear();
    //Build dag
    queue<int> myqueue;
    myqueue.push(0);
    while (!myqueue.empty()) {
        int cIdx = myqueue.front();
        if (isVisited[cIdx]) {

            if (verbose > 3) {
                cout << "ERROR node " << cIdx << " is already visited"
                        << endl << nodes[cIdx]
                        << endl;
            }
            int cParent = nodes[cIdx].parent;
            for (unsigned i = 0; i < nodes.size(); ++i) {
                if (i != cParent) {
                    removeChild(nodes[i], cIdx);
                }
            }
            addChild(nodes[cParent], cIdx);
            myqueue.pop();
            continue;
            for (unsigned j = 0; j < nodes.size(); ++j) {
                cout << nodes[j] << endl;
            }

            exit(1);
        }
        isVisited[cIdx] = true;
        dag.push_back(cIdx);
        if (cIdx != 0) {
            nodes[cIdx].src_to_sink_dist = nodes[nodes[cIdx].parent].src_to_sink_dist +
                    ManhDist[cIdx][nodes[cIdx].parent];
        }
        myqueue.pop();
        for (unsigned i = 0; i < nodes[cIdx].children.size(); ++i) {
            myqueue.push(nodes[cIdx].children[i]);
        }
    }

    for (unsigned i = 0; i < dag.size(); ++i) {
        if (nodes[dag[i]].src_to_sink_dist > maxPL) {
            maxPL = nodes[dag[i]].src_to_sink_dist;
        }
        UpdateMaxPLToChild(dag[i]);
    }
}

bool Graph::find_max_dc_node(vector<float>& node_and_dc) {
    unsigned max_node = 0, max_node_dc = 0;
    for (unsigned j = 1; j < num_terminals; ++j) /* For each terminal */ {
        if (nodes[j].det_cost_node > max_node_dc) {
            max_node_dc = nodes[j].det_cost_node;
            max_node = j;
        }
    }
    node_and_dc[0] = max_node;
    node_and_dc[1] = max_node_dc;

    return (true);
}

