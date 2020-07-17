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

#include "OpenPhySyn/LibraryMapping.hpp"
#include <algorithm>
#include "OpenPhySyn/PsnGlobal.hpp"
#include "OpenPhySyn/PsnLogger.hpp"

namespace psn
{
LibraryCellMapping::LibraryCellMapping(std::string& cell_group_id)
    : id_(cell_group_id),
      mappings_(new std::unordered_map<std::string,
                                       std::shared_ptr<LibraryCellMappingNode>>)
{
}

std::string
LibraryCellMapping::id() const
{
    return id_;
}
std::unordered_map<std::string, std::shared_ptr<LibraryCellMappingNode>>*
LibraryCellMapping::mappings() const
{
    return mappings_;
}

std::vector<std::shared_ptr<LibraryCellMappingNode>>
LibraryCellMapping::terminals() const
{
    std::vector<std::shared_ptr<LibraryCellMappingNode>> terms;
    for (auto& pr : *mappings_)
    {
        auto mapping_terms = pr.second->terminals();
        terms.insert(terms.end(), mapping_terms.begin(), mapping_terms.end());
    }
    return terms;
}

void
LibraryCellMapping::logDebug() const
{
    for (auto& pr : *mappings_)
    {
        pr.second->logDebug();
    }
}
void
LibraryCellMapping::logInfo() const
{
    for (auto& pr : *mappings_)
    {
        pr.second->logInfo();
    }
}

LibraryCellMapping::~LibraryCellMapping()
{
    delete mappings_;
}

LibraryCellMappingNode::LibraryCellMappingNode(
    std::string node_name, std::string cell_id,
    LibraryCellMappingNode* parent_node, bool is_terminal, bool is_recurring,
    bool is_buffer, bool is_inverter, int node_level)
    : name_(node_name),
      id_(cell_id),
      parent_(parent_node),
      terminal_(is_terminal),
      recurring_(is_recurring),
      is_buffer_(is_buffer_),
      is_inverter_(is_inverter),
      level_(node_level)
{
}
std::string
LibraryCellMappingNode::id() const
{
    return id_;
}
std::unordered_map<std::string, std::shared_ptr<LibraryCellMappingNode>>&
LibraryCellMappingNode::children()
{
    return children_;
};
LibraryCellMappingNode*
LibraryCellMappingNode::parent() const
{
    return parent_;
}
int
LibraryCellMappingNode::level() const
{
    return level_;
}
bool
LibraryCellMappingNode::recurring() const
{
    return recurring_;
}
bool
LibraryCellMappingNode::terminal() const
{
    return terminal_;
}
std::vector<std::shared_ptr<LibraryCellMappingNode>>
LibraryCellMappingNode::terminals() const
{
    std::vector<std::shared_ptr<LibraryCellMappingNode>> terms;
    terminals(terms);
    auto last = std::unique(terms.begin(), terms.end());
    terms.erase(last, terms.end());
    return terms;
}

void
LibraryCellMappingNode::terminals(
    std::vector<std::shared_ptr<LibraryCellMappingNode>>& terms) const
{
    if (terminal())
    {
        terms.push_back(self());
    }
    for (auto& child_pr : children_)
    {
        child_pr.second->terminals(terms);
    }
}

void
LibraryCellMappingNode::setSelf(std::shared_ptr<LibraryCellMappingNode> self)
{
    self_ = self;
}
std::shared_ptr<LibraryCellMappingNode>
LibraryCellMappingNode::self() const
{
    return self_;
}

void
LibraryCellMappingNode::setId(std::string& id)
{
    id_ = id;
}

void
LibraryCellMappingNode::setParent(LibraryCellMappingNode* parent_node)
{
    parent_ = parent_node;
}
void
LibraryCellMappingNode::setLevel(int node_level)
{
    level_ = node_level;
}
void
LibraryCellMappingNode::setRecurring(bool is_recurring)
{
    recurring_ = is_recurring;
}

void
LibraryCellMappingNode::setTerminal(bool is_terminal)
{
    terminal_ = is_terminal;
}

bool
LibraryCellMappingNode::isBuffer() const
{
    return is_buffer_;
}

bool
LibraryCellMappingNode::isInverter() const
{
    return is_inverter_;
}

void
LibraryCellMappingNode::setIsBuffer(bool is_buffer)
{
    is_buffer_ = is_buffer;
}
void
LibraryCellMappingNode::setIsInverter(bool is_inverter)
{
    is_inverter_ = is_inverter;
}

std::string
LibraryCellMappingNode::name() const
{
    return name_;
}

void
LibraryCellMappingNode::setName(std::string& node_name)
{
    name_ = node_name;
}

void
LibraryCellMappingNode::logDebug() const
{
    PSN_LOG_DEBUG(std::string(level() * 2, '_'), "[", level(), "]",
                  name().size() ? name() : id(), terminal() ? "x" : "");

    for (auto& pr : children_)
    {
        pr.second->logDebug();
    }
}
void
LibraryCellMappingNode::logInfo() const
{
    PSN_LOG_INFO(std::string(level() * 2, '_'), "[", level(), "]",
                 name().size() ? name() : id(), terminal() ? "x" : "");

    for (auto& pr : children_)
    {
        pr.second->logInfo();
    }
}
} // namespace psn
