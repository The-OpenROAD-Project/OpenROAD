///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019-2020, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <unordered_map>
#include <vector>
#include <map>
#include <set>

#include "mpl/Partition.h"

#include "opendb/db.h"

#include "sta/NetworkClass.hh"
#include "sta/GraphClass.hh"

namespace sta {
class dbSta;
class BfsFwdIterator;
class dbNetwork;
class LibertyPort;
}

namespace odb {
class dbDatabase;
class dbBTerm;
}

namespace utl {
class Logger;
}

namespace mpl {

class Layout;

using std::string;
using std::pair;
using std::set;
using std::map;
using std::vector;
using std::unordered_map;

typedef set<Macro*> MacroSet;
// vertex -> fanin macro set
typedef map<sta::Vertex*, MacroSet> VertexFaninMap;
typedef pair<Macro*, Macro*> MacroPair;
// from/to -> weight
// weight = from/pin -> to/pin count
typedef map<MacroPair, int> AdjWeightMap;

enum class CoreEdge
{
  West,
  East,
  North,
  South,
};

constexpr int core_edge_count = 4;
const char *coreEdgeString(CoreEdge edge);
CoreEdge coreEdgeFromIndex(int edge_index);
int coreEdgeIndex(CoreEdge edge);

class Macro
{
 public:
  Macro(double _lx,
        double _ly,
        double _w,
        double _h,
        odb::dbInst* _dbInstPtr);
  Macro(double _lx,
        double _ly,
        const Macro &copy_from);
  string name();

  double lx, ly;
  double w, h;
  odb::dbInst* dbInstPtr;
};

class MacroSpacings
{
 public:
  MacroSpacings();
  MacroSpacings(double halo_x,
                double halo_y,
                double channel_x,
                double channel_y);
  void setHalo(double halo_x,
               double halo_y);
  void setChannel(double channel_x,
                  double channel_y);
  void setChannelX(double channel_x);
  void setChannelY(double channel_y);
  double getHaloX() const { return halo_x_; }
  double getHaloY() const { return halo_y_; }
  double getChannelX() const { return channel_x_; }
  double getChannelY() const { return channel_y_; }
  double getSpacingX() const;
  double getSpacingY() const;

 private:
  double halo_x_, halo_y_, channel_x_, channel_y_;
};

class MacroPlacer
{
public:
  MacroPlacer();
  void init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log);

  void setHalo(double halo_x, double halo_y);
  void setChannel(double channel_x, double channel_y);
  void setVerboseLevel(int verbose);
  void setFenceRegion(double lx, double ly, double ux, double uy);
  void setSnapLayer(odb::dbTechLayer *snap_layer);

  void placeMacrosCenterSpread();
  void placeMacrosCornerMaxWl();
  int getSolutionCount();

  // return weighted wire-length to get best solution
  double getWeightedWL();
  int weight(int idx11, int idx12);
  int macroIndex(odb::dbInst *inst);
  MacroSpacings &getSpacings(Macro &macro);
  Macro& macro(int idx) { return macros_[idx]; }
  size_t macroCount() { return macros_.size(); }

private:
  void findMacros();
  bool isMissingLiberty();

  void init();
  // Update Macro Location from Partition info
  void updateMacroLocations(Partition& part);
  void updateDbInstLocations();
  void updateMacroPartMap(Partition& part, MacroPartMap &macroPartMap);
  vector<pair<Partition, Partition>> getPartitions(const Layout& layout,
                                                   const Partition& partition,
                                                   bool isHorizontal);
  void cutRoundUp(const Layout& layout,
                  double& cutLine,
                  bool isHorizontal);
    void setDbInstLocations(Partition &partition);

  // graph based adjacencies
  void findAdjacencies();
  void seedFaninBfs(sta::BfsFwdIterator &bfs,
                    VertexFaninMap &vertex_fanins);
  void findFanins(sta::BfsFwdIterator &bfs,
                  VertexFaninMap &vertex_fanins);
  void copyFaninsAcrossRegisters(sta::BfsFwdIterator &bfs,
                                 VertexFaninMap &vertex_fanins);
  void findAdjWeights(VertexFaninMap &vertex_fanins,
                      AdjWeightMap &adj_map);
  sta::Pin *findSeqOutPin(sta::Instance *inst,
                          sta::LibertyPort *out_port);
  void fillMacroWeights(AdjWeightMap &adj_map);
  CoreEdge findNearestEdge(odb::dbBTerm* bTerm);
  string faninName(Macro *macro);
  int macroIndex(Macro *macro);
  bool macroIndexIsEdge(Macro *macro);
  string macroIndexName(int index);

  void reportEdgePinCounts();

  ////////////////////////////////////////////////////////////////
    
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* logger_;
  odb::dbTechLayer *snap_layer_;

  bool connection_driven_;

  // macro idx/idx pair -> give each
  vector<vector<int>> macro_weights_;
  // macro Information
  vector<Macro> macros_;
  // dbInst* --> macros_'s index
  unordered_map<odb::dbInst*, int> macro_inst_map_;

  MacroSpacings default_macro_spacings_;
  unordered_map<odb::dbInst*, MacroSpacings> macro_spacings_;

  double lx_, ly_, ux_, uy_;
  int verbose_;
  int solution_count_;
  // Number of register levels to look through for macro adjacency.
  static constexpr int reg_adjacency_depth_ = 3;
};

class Layout
{
 public:
  Layout();
  Layout(double lx, double ly, double ux, double uy);
  Layout(Layout& orig, Partition& part);

  double lx() const { return lx_; }
  double ly() const { return ly_; }
  double ux() const { return ux_; }
  double uy() const { return uy_; }

  void setLx(double lx);
  void setLy(double ly);
  void setUx(double ux);
  void setUy(double uy);

 private:
  double lx_, ly_, ux_, uy_;
};

}  // namespace mpl
