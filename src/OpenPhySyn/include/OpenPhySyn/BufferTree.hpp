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

#include <unordered_set>
#include "OpenPhySyn/IntervalMap.hpp"
#include "OpenPhySyn/LibraryMapping.hpp"
#include "OpenPhySyn/SteinerTree.hpp"
#include "OpenPhySyn/Types.hpp"
#include "opendb/geom.h"

#include <memory>

namespace psn
{

class Psn;

enum BufferMode
{
    TimingDriven,
    Timerless
};

enum RepairTarget
{
    RepairMaxCapacitance,
    RepairMaxTransition,
    RepairSlack,
};
enum DesignPhase
{
    PostPlace,
    PostDetailedPlace,
    PostRoute
};

// Buffer cluster size thresholds
#define PSN_CLUSTER_SIZE_SINGLE 1.0
#define PSN_CLUSTER_SIZE_SMALL 3.0 / 4.0
#define PSN_CLUSTER_SIZE_MEDIUM 1.0 / 4.0
#define PSN_CLUSTER_SIZE_LARGE 1.0 / 2.0
#define PSN_CLUSTER_SIZE_ALL 0.0

// Represents a single candidate buffer tree.
class BufferTree
{
    float capacitance_;      // Tree total capacitance
    float required_or_slew_; // required time for timing-driven and slew for
                             // timerless
    float                       wire_capacitance_;     // Wire capacitance
    float                       wire_delay_or_slew_;   // Wire delay
    float                       cost_;                 // Tree cost
    Point                       location_;             // Buffer location
    std::shared_ptr<BufferTree> left_, right_;         // Left and right nodes
    LibraryCell*                buffer_cell_;          // Buffer cell type
    InstanceTerm*               pin_;                  // Buffered pin
    LibraryTerm*                library_pin_;          // Buffered pin type
    LibraryCell*                upstream_buffer_cell_; // Minimum upstream cell
    LibraryCell*                driver_cell_;          // Driving cell
    int        polarity_;     // Tree polarity (inverted or not inverted)
    int        buffer_count_; // Number of buffer cells
    BufferMode mode_;         // Timing-driven or timerless
    std::shared_ptr<LibraryCellMappingNode>
          library_mapping_node_; // Resynthesis mapping
    Point driver_location_;      // Driver cell location

public:
    BufferTree(float cap = 0.0, float req = 0.0, float cost = 0.0,
               Point location = Point(0, 0), LibraryTerm* library_pin = nullptr,
               InstanceTerm* pin = nullptr, LibraryCell* buffer_cell = nullptr,
               int        polarity    = 0,
               BufferMode buffer_mode = BufferMode::TimingDriven);
    BufferTree(Psn* psn_inst, std::shared_ptr<BufferTree> left,
               std::shared_ptr<BufferTree> right, Point location);
    float         totalCapacitance() const;
    float         capacitance() const;
    float         requiredOrSlew() const;
    float         totalRequiredOrSlew() const;
    float         wireCapacitance() const;
    float         wireDelayOrSlew() const;
    float         cost() const;
    int           polarity() const;
    InstanceTerm* pin() const;
    bool checkLimits(Psn* psn_inst, LibraryTerm* driver_pin, float slew_limit,
                     float cap_limit);

    void setDriverLocation(Point loc);

    Point driverLocation() const;

    void setLibraryMappingNode(std::shared_ptr<LibraryCellMappingNode> node);

    std::shared_ptr<LibraryCellMappingNode> libraryMappingNode() const;

    void setCapacitance(float cap);
    void setRequiredOrSlew(float req);
    void setWireCapacitance(float cap);
    void setWireDelayOrSlew(float delay);
    void setCost(float cost);
    void setPolarity(int polarity);
    void setPin(InstanceTerm* pin);
    void setLibraryPin(LibraryTerm* library_pin);

    void setMode(BufferMode buffer_mode);

    BufferMode mode() const;

    float bufferSlew(Psn* psn_inst, LibraryCell* buffer_cell,
                     float tr_slew = 0.0);
    float bufferFixedInputSlew(Psn* psn_inst, LibraryCell* buffer_cell);

    float bufferRequired(Psn* psn_inst, LibraryCell* buffer_cell) const;
    float bufferRequired(Psn* psn_inst) const;
    float upstreamBufferRequired(Psn* psn_inst) const;

    bool hasDownstreamSlewViolation(Psn* psn_inst, float slew_limit,
                                    float tr_slew = 0.0);

    LibraryTerm*                 libraryPin() const;
    std::shared_ptr<BufferTree>& left();
    std::shared_ptr<BufferTree>& right();
    void                         setLeft(std::shared_ptr<BufferTree> left);
    void                         setRight(std::shared_ptr<BufferTree> right);
    bool                         hasUpstreamBufferCell() const;
    bool                         hasBufferCell() const;
    bool                         hasDriverCell() const;

    LibraryCell* bufferCell() const;
    LibraryCell* upstreamBufferCell() const;
    LibraryCell* driverCell() const;
    void         setBufferCell(LibraryCell* buffer_cell);
    void         setUpstreamBufferCell(LibraryCell* buffer_cell);
    void         setDriverCell(LibraryCell* driver_cell);

    Point location() const;

    bool isBufferNode() const;
    bool isLoadNode() const;
    bool isBranched() const;
    bool operator<(const BufferTree& other) const;
    bool operator>(const BufferTree& other) const;
    bool operator<=(const BufferTree& other) const;
    bool operator>=(const BufferTree& other) const;
    void setBufferCount(int buffer_count);
    int  bufferCount() const;
    int  count();
    int  branchCount() const;
    void logInfo() const;
    void logDebug() const;
    bool isTimerless() const;
};

class TimerlessBufferTree : public BufferTree
{
public:
    TimerlessBufferTree(float cap = 0.0, float req = 0.0, float cost = 0.0,
                        Point         location    = Point(0, 0),
                        LibraryTerm*  library_pin = nullptr,
                        InstanceTerm* pin         = nullptr,
                        LibraryCell* buffer_cell = nullptr, int polarity = 0);
    TimerlessBufferTree(Psn* psn_inst, std::shared_ptr<BufferTree> left,
                        std::shared_ptr<BufferTree> right, Point location);
};

// Options to customize the optimization
class OptimizationOptions
{
public:
    OptimizationOptions() : buffer_lib_lookup(0), inverter_lib_lookup(0)
    {
        max_iterations                   = 1;
        minimum_gain                     = 0;
        max_negative_slack_paths         = 0;
        max_negative_slack_path_depth    = 0;
        area_penalty                     = 0.0;
        cluster_buffers                  = false;
        cluster_inverters                = false;
        minimize_cluster_buffers         = false;
        cluster_threshold                = 0.0;
        driver_resize                    = false;
        repair_capacitance_violations    = false;
        repair_transition_violations     = false;
        repair_negative_slack            = false;
        timerless                        = false;
        disable_buffering                = false;
        ripup_existing_buffer_max_levels = 0;
        legalization_frequency           = 0;
        repair_by_buffer                 = true;
        repair_by_resize                 = true;
        repair_by_pinswap                = true;
        repair_by_downsize               = false;
        repair_by_resynthesis            = false;
        repair_by_move                   = false;
        resize_for_negative_slack        = true;
        minimum_cost                     = false;
        phase                            = DesignPhase::PostPlace;
        use_best_solution_threshold      = true;
        best_solution_threshold          = 10E-12; // 10ps
        best_solution_threshold_range    = 3;      // Check the top 3 solutions
        minimum_upstream_resistance      = 120;
        legalize_eventually              = false;
        legalize_each_iteration          = false;
        current_iteration                = 0;
        capacitance_pessimism_factor     = 1.0;
        transition_pessimism_factor      = 1.0;
    }
    float initial_area;             // Area before the optimization
    int   max_iterations;           // Maximum number of optimization iterations
    int   current_iteration;        // Current optimization iteration
    int   max_negative_slack_paths; // Maximum number of negative slack paths to
                                    // check (0 for no limit)
    int max_negative_slack_path_depth; // Maximum vertices in the negative slack
                                       // path to check (0 for no limit)
    float minimum_gain;             // Minimum slack gain to accept a solution
    float area_penalty;             // Area penalty for driver sizing
    bool  cluster_buffers;          // Auto-select buffer library
    bool  cluster_inverters;        // Include inverters in the buffer library
    bool  minimize_cluster_buffers; // Pre-prunt the buffer library
    float cluster_threshold; // Specify the auto-selected buffer library size
    bool  driver_resize;     // Enable driver resizing with buffering
    bool repair_capacitance_violations; // Repair maximum capacitance violations
    bool repair_transition_violations;  // Repair maximum transition violations
    bool repair_negative_slack;         // Repair negative slack violations
    bool timerless;                     // Timerless buffer mode
    bool disable_buffering;             // Disable all buffering
    int  ripup_existing_buffer_max_levels; // Remove existing buffers before
                                           // buffering
    bool legalize_eventually;     // Legalize at the end of the optimization
    bool legalize_each_iteration; // Legalize after each iteraion
    int  legalization_frequency;  // Legalization frequency
    std::vector<LibraryCell*>        buffer_lib;   // Buffer library used
    std::vector<LibraryCell*>        inverter_lib; // Inverter library used
    std::unordered_set<LibraryCell*> buffer_lib_set;
    std::unordered_set<LibraryCell*> inverter_lib_set;
    IntervalMap<int, LibraryCell*>   buffer_lib_lookup;
    IntervalMap<int, LibraryCell*>   inverter_lib_lookup;
    bool                             repair_by_buffer; // Use buffer for repair
    bool repair_by_resize;            // Use driver upsizing for optimization
    bool repair_by_downsize;          // Use driver downsizing for optimization
    bool repair_by_resynthesis;       // Use resynthesis for optimization
    bool repair_by_pinswap;           // Repair by commutative pin-swap
    bool repair_by_move;              // Repair by cell movement
    bool resize_for_negative_slack;   // Enable resizing when solving negative
                                      // slack violations
    bool        minimum_cost;         // Use minimum-cost buffering
    DesignPhase phase;                // Design optimizatin phase
    bool use_best_solution_threshold; // Use lower cost solution for minimal
                                      // degradation
    float best_solution_threshold;    // Specify use_best_solution_threshold
                                      // threshold
    size_t
        best_solution_threshold_range; // Number of lower cost solutions to test
    float
          minimum_upstream_resistance; // Minimum upstream resistance for pruning
    float capacitance_pessimism_factor; // Scaling factor for capacitance
                                        // violations
    float transition_pessimism_factor;  // Scaling factor for transition
                                        // violations
};

// Represents a set of non-dominatd candidate buffer trees.
class BufferSolution
{
    std::vector<std::shared_ptr<BufferTree>> buffer_trees_;
    BufferMode                               mode_;

public:
    BufferSolution(BufferMode buffer_mode = BufferMode::TimingDriven);
    BufferSolution(Psn* psn_inst, std::shared_ptr<BufferSolution>& left,
                   std::shared_ptr<BufferSolution>& right, Point location,
                   LibraryCell* upstream_res_cell,
                   float        minimum_upstream_res_or_max_slew,
                   BufferMode   buffer_mode = BufferMode::TimingDriven);

    // van Ginneken buffer algorithm bottom-up
    static std::shared_ptr<BufferSolution>
    bottomUp(Psn* psn_inst, InstanceTerm* driver_pin, SteinerPoint pt,
             SteinerPoint prev, std::shared_ptr<SteinerTree> st_tree,
             std::unique_ptr<OptimizationOptions>& options);

    // van Ginneken buffer algorithm bottom-up with resynthesis support
    static std::shared_ptr<BufferSolution> bottomUpWithResynthesis(
        Psn* psn_inst, InstanceTerm* driver_pin, SteinerPoint pt,
        SteinerPoint prev, std::shared_ptr<SteinerTree> st_tree,
        std::unique_ptr<OptimizationOptions>&                 options,
        std::vector<std::shared_ptr<LibraryCellMappingNode>>& mapping_terminals);

    // van Ginneken buffer algorithm top-down
    static void topDown(Psn* psn_inst, Net* net,
                        std::shared_ptr<BufferTree> tree, float& area,
                        int& net_index, int& buff_index,
                        std::unordered_set<Instance*>& added_buffers,
                        std::unordered_set<Net*>&      affected_nets);

    // van Ginneken buffer algorithm top-down
    static void topDown(Psn* psn_inst, InstanceTerm* pin,
                        std::shared_ptr<BufferTree> tree, float& area,
                        int& net_index, int& buff_index,
                        std::unordered_set<Instance*>& added_buffers,
                        std::unordered_set<Net*>&      affected_nets);

    // Merges two candidate solutions
    void mergeBranches(Psn* psn_inst, std::shared_ptr<BufferSolution>& left,
                       std::shared_ptr<BufferSolution>& right, Point location,
                       LibraryCell* upstream_res_cell,
                       float        minimum_upstream_res_or_max_slew);

    // Add new candidate tree
    void addTree(std::shared_ptr<BufferTree>& tree);

    // Returns included candidate trees
    std::vector<std::shared_ptr<BufferTree>>& bufferTrees();

    // Addd wire parasitics
    void addWireDelayAndCapacitance(float wire_res, float wire_cap);
    void addWireSlewAndCapacitance(float wire_res, float wire_cap);

    // Add leaf nodes
    void addLeafTrees(Psn* psn_inst, InstanceTerm*, Point pt,
                      std::vector<LibraryCell*>& buffer_lib,
                      std::vector<LibraryCell*>& inverter_lib,
                      float                      slew_limit = 0.0);

    // Add leaf nodes with resynthesis support
    void addLeafTreesWithResynthesis(
        Psn* psn_inst, InstanceTerm*, Point pt,
        std::vector<LibraryCell*>& buffer_lib,
        std::vector<LibraryCell*>& inverter_lib,
        std::vector<std::shared_ptr<LibraryCellMappingNode>>&
            mappings_terminals);
    void addUpstreamReferences(Psn*                        psn_inst,
                               std::shared_ptr<BufferTree> base_buffer_tree);

    // Returns the maximum required time tree with driver resizing
    std::shared_ptr<BufferTree>
    optimalDriverTreeWithResize(Psn* psn_inst, InstanceTerm* driver_pin,
                                std::vector<LibraryCell*> driver_types,
                                float                     area_penalty);

    // Returns the maximum required time tree with resynthesis support
    std::shared_ptr<BufferTree>
    optimalDriverTreeWithResynthesis(Psn* psn_inst, InstanceTerm* driver_pin,
                                     float  area_penalty,
                                     float* tree_slack = nullptr);
    // Not used
    std::shared_ptr<BufferTree>
    optimalTimerlessDriverTree(Psn* psn_inst, InstanceTerm* driver_pin);

    // Returns the maximum required time tree
    std::shared_ptr<BufferTree>
    optimalDriverTree(Psn* psn_inst, InstanceTerm* driver_pin,
                      std::shared_ptr<BufferTree>& inverted_sol,
                      float*                       tree_slack = nullptr);

    // Returns minimum cost tree that satisfies cap_limit
    std::shared_ptr<BufferTree>
    optimalCapacitanceTree(Psn* psn_inst, InstanceTerm* driver_pin,
                           std::shared_ptr<BufferTree>& inverted_sol,
                           float                        cap_limit);
    // Returns minimum cost tree that satisfies slew_limit
    std::shared_ptr<BufferTree>
    optimalSlewTree(Psn* psn_inst, InstanceTerm* driver_pin,
                    std::shared_ptr<BufferTree>& inverted_sol,
                    float                        slew_limit);
    // Returns minimum cost tree that satisfies slew_limit and cap_limit
    std::shared_ptr<BufferTree>
    optimalCostTree(Psn* psn_inst, InstanceTerm* driver_pin,
                    std::shared_ptr<BufferTree>& inverted_sol, float slew_limit,
                    float cap_limit);

    // Helper fuzzy comparisons
    static bool isGreater(float first, float second, float threshold = 1E-6F);
    static bool isLess(float first, float second, float threshold = 1E-6F);
    static bool isEqual(float first, float second, float threshold = 1E-6F);
    static bool isLessOrEqual(float first, float second, float threshold);
    static bool isGreaterOrEqual(float first, float second, float threshold);

    // Prune buffer trees
    void prune(Psn* psn_inst, LibraryCell* upstream_res_cell,
               float       minimum_upstream_res_or_max_slew,
               const float cap_prune_threshold  = 1E-6F,
               const float cost_prune_threshold = 1E-6F);

    // Not used
    void       setMode(BufferMode buffer_mode);
    BufferMode mode() const;
    bool       isTimerless() const;
}; // namespace psn

class TimerlessBufferSolution : public BufferSolution
{

public:
    TimerlessBufferSolution();
    TimerlessBufferSolution(Psn*                             psn_inst,
                            std::shared_ptr<BufferSolution>& left,
                            std::shared_ptr<BufferSolution>& right,
                            Point location, LibraryCell* upstream_res_cell,
                            float      minimum_upstream_res_or_max_slew,
                            BufferMode buffer_mode = BufferMode::TimingDriven);
};
} // namespace psn
