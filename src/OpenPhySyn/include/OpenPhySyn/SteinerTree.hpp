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

#ifndef __PSN_STEINER_TREE__
#define __PSN_STEINER_TREE__
#include <OpenPhySyn/Types.hpp>
#include <flute.h>
#include <memory>
#include <unordered_map>

#define FLUTE_DTYPE int
namespace psn
{
class Psn;
typedef int SteinerPoint;
const int   SteinerNull = -1;
class SteinerBranch;

class SteinerTree
{
public:
    static std::unique_ptr<SteinerTree> create(Net* net, Psn* psn_inst,
                                               int flute_accuracy = 3);

    DefDbu distance(SteinerPoint& from, SteinerPoint& to) const;

    int           branchCount() const;
    SteinerBranch branch(int index) const;

    SteinerPoint driverPoint() const;

    bool isPlaced() const;

    InstanceTerm* pin(SteinerPoint pt) const;

    Point location(SteinerPoint pt) const;

    SteinerPoint left(SteinerPoint pt) const;

    SteinerPoint right(SteinerPoint pt) const;

    SteinerPoint top() const; // First point after the driver

    float totalLoad(float cap_per_micron) const;
    float subtreeLoad(float cap_per_micron, SteinerPoint pt) const;

    ~SteinerTree();

private:
    void validatePoint(SteinerPoint pt) const;
    void populateSides();
    void populateSides(SteinerPoint from, SteinerPoint to,
                       std::vector<SteinerPoint>& adj1,
                       std::vector<SteinerPoint>& adj2,
                       std::vector<SteinerPoint>& adj3);
    void populateSides(SteinerPoint from, SteinerPoint to, SteinerPoint adj,
                       std::vector<SteinerPoint>& adj1,
                       std::vector<SteinerPoint>& adj2,
                       std::vector<SteinerPoint>& adj3);
    SteinerTree(Flute::Tree tree, std::vector<InstanceTerm*> pins,
                Psn* psn_inst);
    Flute::Tree                tree_;
    std::vector<InstanceTerm*> pins_;
    std::vector<SteinerPoint>  left_;
    std::vector<SteinerPoint>  right_;
    std::vector<InstanceTerm*> point_pin_map_;
    std::unordered_map<Point, InstanceTerm*, PointHash, PointEqual> pin_loc_;
    Psn*                                                            psn_;
};
class SteinerBranch
{
public:
    SteinerBranch(Point pt1 = Point(0, 0), InstanceTerm* pin1 = nullptr,
                  SteinerPoint steiner_pt1 = SteinerNull,
                  Point pt2 = Point(0, 0), InstanceTerm* pin2 = nullptr,
                  SteinerPoint steiner_pt2 = SteinerNull, int wire_length = 0)
        : pt1_(pt1),
          pin1_(pin1),
          steiner_pt1_(steiner_pt1),
          pt2_(pt2),
          pin2_(pin2),
          steiner_pt2_(steiner_pt2),
          wire_length_(wire_length)
    {
    }

    Point
    firstPoint() const
    {
        return pt1_;
    }
    SteinerBranch*
    setFirstPoint(Point pt)
    {
        pt1_ = pt;
        return this;
    }

    Point
    secondPoint() const
    {
        return pt2_;
    }
    SteinerBranch*
    setSecondPoint(Point pt)
    {
        pt2_ = pt;
        return this;
    }

    SteinerPoint
    firstSteinerPoint() const
    {
        return steiner_pt1_;
    }
    SteinerBranch*
    setFirstSteinerPoint(SteinerPoint pt)
    {
        steiner_pt1_ = pt;
        return this;
    }

    SteinerPoint
    secondSteinerPoint() const
    {
        return steiner_pt2_;
    }
    SteinerBranch*
    setSecondSteinerPoint(SteinerPoint pt)
    {
        steiner_pt2_ = pt;
        return this;
    }

    InstanceTerm*
    firstPin() const
    {
        return pin1_;
    }
    SteinerBranch*
    setFirstPin(InstanceTerm* pin)
    {
        pin1_ = pin;
        return this;
    }

    InstanceTerm*
    secondPin() const
    {
        return pin2_;
    }
    SteinerBranch*
    setSecondPin(InstanceTerm* pin)
    {
        pin2_ = pin;
        return this;
    }

    int
    wireLength() const
    {
        return wire_length_;
    }
    SteinerBranch*
    setWireLength(int length)
    {
        wire_length_ = length;
        return this;
    }

public:
    Point         pt1_;
    InstanceTerm* pin1_;
    SteinerPoint  steiner_pt1_;
    Point         pt2_;
    InstanceTerm* pin2_;
    SteinerPoint  steiner_pt2_;
    int           wire_length_;
};
} // namespace psn
#endif