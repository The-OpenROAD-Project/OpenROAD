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

#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "OpenPhySyn/Types.hpp"
namespace psn
{

class LibraryCellMappingNode;

// LibraryCellMapping represents different possible coverages for the same
// liberty cell
class LibraryCellMapping
{
public:
    LibraryCellMapping(std::string& cell_group_id);
    std::string id() const;
    std::unordered_map<std::string, std::shared_ptr<LibraryCellMappingNode>>*
                                                         mappings() const;
    std::vector<std::shared_ptr<LibraryCellMappingNode>> terminals() const;
    ~LibraryCellMapping();
    void logDebug() const;
    void logInfo() const;

private:
    std::string id_;
    std::unordered_map<std::string, std::shared_ptr<LibraryCellMappingNode>>*
        mappings_;
    friend class DatabaseHandler;
};

// LibraryCellMappingNode represents a node in LibraryCellMapping coverage tree
class LibraryCellMappingNode
{
public:
    LibraryCellMappingNode(std::string name = "", std::string cell_id = "none",
                           LibraryCellMappingNode* parent_node = nullptr,
                           bool is_terminal = true, bool is_recurring = false,
                           bool is_buffer = false, bool is_inverter = false,
                           int node_level = 0);
    std::string id() const;
    std::string name() const;
    std::unordered_map<std::string, std::shared_ptr<LibraryCellMappingNode>>&
                                                         children();
    LibraryCellMappingNode*                              parent() const;
    int                                                  level() const;
    bool                                                 recurring() const;
    bool                                                 terminal() const;
    bool                                                 isBuffer() const;
    bool                                                 isInverter() const;
    std::vector<std::shared_ptr<LibraryCellMappingNode>> terminals() const;
    void                                                 terminals(
                                                        std::vector<std::shared_ptr<LibraryCellMappingNode>>& terms) const;
    std::shared_ptr<LibraryCellMappingNode> self() const;

    void setId(std::string& cell_group_id);
    void setName(std::string& node_name);
    void setParent(LibraryCellMappingNode* parent_node);
    void setLevel(int node_level);
    void setRecurring(bool is_recurring);
    void setTerminal(bool is_terminal);
    void setIsBuffer(bool is_buffer);
    void setIsInverter(bool is_inverter);
    void setSelf(std::shared_ptr<LibraryCellMappingNode> self);
    void logDebug() const;
    void logInfo() const;

private:
    std::string                             name_;
    std::string                             id_;
    LibraryCellMappingNode*                 parent_;
    bool                                    terminal_;
    bool                                    recurring_;
    bool                                    is_buffer_;
    bool                                    is_inverter_;
    int                                     level_;
    std::shared_ptr<LibraryCellMappingNode> self_;
    std::unordered_map<std::string, std::shared_ptr<LibraryCellMappingNode>>
        children_;
    friend class DatabaseHandler;
};
} // namespace psn
