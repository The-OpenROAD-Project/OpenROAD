/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, University of California, San Diego.
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

#include "SteinerTree.h"

#include <vector>

namespace FastRoute {

void SteinerTree::printSegments()
{
  for (Segment seg : _segments) {
    Node node1 = seg.getFirstNode();
    Node node2 = seg.getLastNode();
    std::cout << "(" << nodeTypeString(node1.getType()) << " "
              << node1.getPosition().getX() << ", "
              << node1.getPosition().getY() << ", " << node1.getLayer()
              << "); (" << nodeTypeString(node2.getType()) << " "
              << node2.getPosition().getX() << ", "
              << node2.getPosition().getY() << ", " << node2.getLayer()
              << ")\n";
  }
}

void SteinerTree::addNode(Node node)
{
  if (!nodeExists(node)) {
    _nodes.push_back(node);
  } else if (node.getType() == NodeType::SOURCE) {
    for (Node& n : _nodes) {
      if (n == node) {
        n.setType(NodeType::SOURCE);
      }
    }
  }
}

void SteinerTree::addSegment(Segment segment)
{
  for (Segment seg : _segments) {
    if ((seg.getFirstNode() == segment.getFirstNode()
         && seg.getLastNode() == segment.getLastNode())
        || (seg.getFirstNode() == segment.getLastNode()
            && seg.getLastNode() == segment.getFirstNode())) {
      return;
    }
  }

  _segments.push_back(segment);
}

bool SteinerTree::nodeExists(Node node)
{
  for (Node n : _nodes) {
    if (n == node) {
      return true;
    }
  }
  return false;
}

bool SteinerTree::getNodeIfExists(Node node, Node& requestedNode)
{
  for (Node n : _nodes) {
    if (n == node) {
      requestedNode = n;
      return true;
    }
  }

  return false;
}

std::vector<Segment> SteinerTree::getNodeSegments(Node node)
{
  std::vector<Segment> nodeSegments;
  for (Segment seg : _segments) {
    if (seg.getFirstNode() == node || seg.getLastNode() == node) {
      nodeSegments.push_back(seg);
    }
  }

  return nodeSegments;
}

Node SteinerTree::getSource()
{
  Node source;
  bool found = false;
  for (Node node : _nodes) {
    if (node.getType() == NodeType::SOURCE) {
      source = node;
      found = true;
    }
  }

  if (!found) {
    std::cout << "[ERROR] Source not found\n";
    std::exit(0);
  }

  return source;
}

std::vector<Node> SteinerTree::getSinks()
{
  std::vector<Node> sinks;

  for (Node node : _nodes) {
    if (node.getType() == NodeType::SINK) {
      sinks.push_back(node);
    }
  }

  return sinks;
}

Segment SteinerTree::getSegmentByIndex(int index)
{
  Segment idxSeg;
  for (Segment seg : _segments) {
    if (seg.getIndex() == index) {
      idxSeg = seg;
      break;
    }
  }

  return idxSeg;
}

}  // namespace FastRoute
