// BSD 3-Clause License

// Copyright (c) 2019, SCALE Lab, Brown University
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.

// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.

// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.

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

#include <OpenPhySyn/Psn.hpp>
#include <OpenPhySyn/SteinerTree.hpp>
#include <algorithm>
#include "PsnException.hpp"
namespace psn
{
std::unique_ptr<SteinerTree>
SteinerTree::create(Net* net, Psn* psn_inst, int flute_accuracy)
{
    DatabaseHandler& handler = *(psn_inst->handler());
    auto             pins    = handler.connectedPins(net);

    std::unique_ptr<SteinerTree> tree(nullptr);
    unsigned int                 pin_count = pins.size();
    if (pin_count >= 2)
    {
        FLUTE_DTYPE* x = new FLUTE_DTYPE[pin_count];
        FLUTE_DTYPE* y = new FLUTE_DTYPE[pin_count];
        for (unsigned int i = 0; i < pin_count; i++)
        {
            auto  pin = pins[i];
            Point loc = handler.location(pin);
            x[i]      = loc.x();
            y[i]      = loc.y();
        }
        Flute::Tree flute_tree = Flute::flute(pin_count, x, y, flute_accuracy);

        tree.reset(new SteinerTree(flute_tree, pins, psn_inst));

        delete[] x;
        delete[] y;
    }
    return tree;
}
bool
SteinerTree::isPlaced() const
{
    for (auto pin : pins_)
    {
        if (psn_->handler()->isPlaced(pin))
        {
            return true;
        }
    }
    return false;
}
SteinerBranch
SteinerTree::branch(int index) const
{
    Flute::Branch& branch_pt1 = tree_.branch[index];
    int            index2     = branch_pt1.n;
    Flute::Branch& branch_pt2 = tree_.branch[index2];
    Point          pt1        = Point(branch_pt1.x, branch_pt1.y);
    Point          pt2        = Point(branch_pt2.x, branch_pt2.y);
    int            pin_count  = pins_.size();
    InstanceTerm*  pin1;
    InstanceTerm*  pin2;
    SteinerPoint   st_pt1 = SteinerNull;
    SteinerPoint   st_pt2 = SteinerNull;
    if (index < pin_count)
    {
        pin1   = pin(index);
        st_pt1 = 0;
    }
    else
    {
        pin1   = nullptr;
        st_pt1 = index;
    }
    if (index2 < pin_count)
    {
        pin2   = pin(index2);
        st_pt2 = 0;
    }
    else
    {
        pin2   = nullptr;
        st_pt2 = index2;
    }

    int wire_length =
        abs(branch_pt1.x - branch_pt2.x) + abs(branch_pt1.y - branch_pt2.y);
    return SteinerBranch(pt1, pin1, st_pt1, pt2, pin2, st_pt2, wire_length);
}
int
SteinerTree::branchCount() const
{
    return tree_.deg * 2 - 2;
}
InstanceTerm*
SteinerTree::pin(SteinerPoint pt) const
{
    validatePoint(pt);
    if (pt < (int)pins_.size())
        return point_pin_map_[pt];
    else
        return nullptr;
}

SteinerPoint
SteinerTree::driverPoint() const
{
    for (unsigned int i = 0; i < pins_.size(); i++)
    {
        auto pin = this->pin(i);
        if (psn_->handler()->isDriver(pin))
            return i;
    }
    return SteinerNull;
}
SteinerTree::SteinerTree(Flute::Tree tree, std::vector<InstanceTerm*> pins,
                         Psn* psn_inst)
    : tree_(tree), pins_(pins), psn_(psn_inst)
{
    unsigned int pin_count = pins.size();
    point_pin_map_.resize(pin_count);
    std::unordered_map<Point, std::vector<InstanceTerm*>, PointHash, PointEqual>
        pins_map;
    for (unsigned int i = 0; i < pin_count; i++)
    {
        auto  pin     = pins_[i];
        Point loc     = psn_inst->handler()->location(pin);
        pin_loc_[loc] = pin;
        pins_map[loc].push_back(pin);
    }
    for (unsigned int i = 0; i < pin_count; i++)
    {
        Flute::Branch&              branch_pt = tree_.branch[i];
        std::vector<InstanceTerm*>& pin_locations =
            pins_map[Point(branch_pt.x, branch_pt.y)];
        auto pin = pin_locations.back();
        pin_locations.pop_back();
        point_pin_map_[i] = pin;
    }
    populateSides();
}

void
SteinerTree::populateSides()
{
    int branch_count = branchCount();
    left_.resize(branch_count, SteinerNull);
    right_.resize(branch_count, SteinerNull);
    std::vector<SteinerPoint> adj1(branch_count, SteinerNull);
    std::vector<SteinerPoint> adj2(branch_count, SteinerNull);
    std::vector<SteinerPoint> adj3(branch_count, SteinerNull);
    for (int i = 0; i < branch_count; i++)
    {
        Flute::Branch& branch_pt = tree_.branch[i];
        SteinerPoint   j         = branch_pt.n;
        if (j != i)
        {
            if (adj1[i] == SteinerNull)
                adj1[i] = j;
            else if (adj2[i] == SteinerNull)
                adj2[i] = j;
            else
                adj3[i] = j;

            if (adj1[j] == SteinerNull)
                adj1[j] = i;
            else if (adj2[j] == SteinerNull)
                adj2[j] = i;
            else
                adj3[j] = i;
        }
    }

    SteinerPoint root     = driverPoint();
    SteinerPoint root_adj = adj1[root];
    left_[root]           = root_adj;
    populateSides(root, root_adj, adj1, adj2, adj3);
}

void
SteinerTree::populateSides(SteinerPoint from, SteinerPoint to,
                           std::vector<SteinerPoint>& adj1,
                           std::vector<SteinerPoint>& adj2,
                           std::vector<SteinerPoint>& adj3)
{
    if (to >= (int)pins_.size())
    {
        SteinerPoint adj;
        adj = adj1[to];
        populateSides(from, to, adj, adj1, adj2, adj3);
        adj = adj2[to];
        populateSides(from, to, adj, adj1, adj2, adj3);
        adj = adj3[to];
        populateSides(from, to, adj, adj1, adj2, adj3);
    }
}

void
SteinerTree::populateSides(SteinerPoint from, SteinerPoint to, SteinerPoint adj,
                           std::vector<SteinerPoint>& adj1,
                           std::vector<SteinerPoint>& adj2,
                           std::vector<SteinerPoint>& adj3)
{
    if (adj != from && adj != SteinerNull)
    {
        if (adj == to)
            throw SteinerException();
        if (left_[to] == SteinerNull)
        {
            left_[to] = adj;
            populateSides(to, adj, adj1, adj2, adj3);
        }
        else if (right_[to] == SteinerNull)
        {
            right_[to] = adj;
            populateSides(to, adj, adj1, adj2, adj3);
        }
    }
}

DefDbu
SteinerTree::distance(SteinerPoint& from, SteinerPoint& to) const
{
    DefDbu find_left, find_right;
    if (from == SteinerNull || to == SteinerNull)
    {
        return -1;
    }
    if (from == to)
    {
        return 0;
    }
    Point        from_pt    = location(from);
    Point        to_pt      = location(to);
    SteinerPoint left_from  = left(from);
    SteinerPoint right_from = right(from);
    if (left_from == to || right_from == to)
    {
        return abs(from_pt.x() - to_pt.x()) + abs(from_pt.y() - to_pt.y());
    }
    if (left_from == SteinerNull && right_from == SteinerNull)
    {
        return -1;
    }

    find_left = distance(left_from, to);
    if (find_left >= 0)
    {
        return find_left + abs(from_pt.x() - to_pt.x()) +
               abs(from_pt.y() - to_pt.y());
    }

    find_right = distance(right_from, to);
    if (find_right >= 0)
    {
        return find_left + abs(from_pt.x() - to_pt.x()) +
               abs(from_pt.y() - to_pt.y());
    }

    return -1;
}

float
SteinerTree::totalLoad(float cap_per_micron) const
{
    SteinerPoint     top_pt  = top();
    SteinerPoint     drvr_pt = driverPoint();
    DatabaseHandler& handler = *(psn_->handler());
    if (top_pt == SteinerNull)
    {
        return 0;
    }
    float top_length   = handler.dbuToMeters(distance(drvr_pt, top_pt));
    float subtree_load = subtreeLoad(cap_per_micron, top_pt);
    return (top_length * cap_per_micron) + subtree_load;
}

float
SteinerTree::subtreeLoad(float cap_per_micron, SteinerPoint pt) const
{
    DatabaseHandler& handler = *(psn_->handler());

    if (pt == SteinerNull)
    {
        return 0;
    }
    SteinerPoint left_pt  = left(pt);
    SteinerPoint right_pt = right(pt);
    bool         isLeaf   = left_pt == SteinerNull && right_pt == SteinerNull;

    if (isLeaf)
    {
        InstanceTerm* pt_pin = pin(pt);
        return handler.pinCapacitance(pt_pin);
    }
    else
    {
        float left_cap  = 0;
        float right_cap = 0;
        if (left_pt != SteinerNull)
        {
            float left_length = handler.dbuToMeters(distance(pt, left_pt));
            left_cap          = subtreeLoad(cap_per_micron, left_pt) +
                       (left_length * cap_per_micron);
        }
        if (right_pt != SteinerNull)
        {
            float right_length = handler.dbuToMeters(distance(pt, right_pt));
            right_cap          = subtreeLoad(cap_per_micron, right_pt) +
                        (right_length * cap_per_micron);
        }

        return left_cap + right_cap;
    }
}

SteinerTree::~SteinerTree()
{
    Flute::free_tree(tree_);
}

void
SteinerTree::validatePoint(SteinerPoint pt) const
{
    if (pt < 0 || pt >= branchCount())
        throw SteinerException();
}
Point
SteinerTree::location(SteinerPoint pt) const
{
    validatePoint(pt);
    Flute::Branch& branch_pt = tree_.branch[pt];
    return Point(branch_pt.x, branch_pt.y);
}

SteinerPoint
SteinerTree::left(SteinerPoint pt) const
{
    if (pt >= (int)left_.size())
        return SteinerNull;
    return left_[pt];
}

SteinerPoint
SteinerTree::right(SteinerPoint pt) const
{
    if (pt >= (int)right_.size())
        return SteinerNull;
    return right_[pt];
}

SteinerPoint
SteinerTree::top() const
{
    SteinerPoint driver = driverPoint();
    SteinerPoint top    = left(driver);
    if (top == SteinerNull)
    {
        top = right(driver);
    }
    return top;
}

} // namespace psn
