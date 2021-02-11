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

#include <Eigen/Core>
#include <Eigen/SparseCore>
#include <memory>
#include <unordered_map>
#include <vector>
#include <map>
#include <set>

#include "graph.h"
#include "hashUtil.h"
#include "macro.h"
#include "partition.h"

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

typedef std::set<Macro*> MacroSet;
// vertex -> fanin macro set
typedef std::map<sta::Vertex*, MacroSet> VertexFaninMap;

enum class BoundaryEdge
{
  West,
  East,
  North,
  South,
  Unknown
};

class MacroCircuit
{
public:
  MacroCircuit();
  MacroCircuit(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log);

  std::vector<Vertex> vertexStor;

  // macro Information
  std::vector<Macro> macroStor;

  // pin Group Information
  std::vector<PinGroup> pinGroupStor;

  // pin Group Map;
  // Pin* --> pinGroupStor's index.
  std::unordered_map<sta::Pin*, int> staToPinGroup;

  // macro name -> macroStor's index.
  std::unordered_map<std::string, int> macroNameMap;

  // macro idx/idx pair -> give each
  std::vector<std::vector<int>> macroWeight;

  std::string GetVertexName(Vertex* vertex);

  // sta::Instance* --> macroStor's index stor
  std::unordered_map<sta::Instance*, int> macroInstMap;

  // Update Macro Location from Partition info
  void UpdateMacroCoordi(Partition& part);

  // parsing function
  void ParseGlobalConfig(std::string fileName);
  void ParseLocalConfig(std::string fileName);

  // save LocalCfg into this structure
  std::unordered_map<std::string, MacroLocalInfo> macroLocalMap;

  // plotting
  void Plot(std::string outputFile, std::vector<Partition>& set);

  // netlist
  void UpdateNetlist(Partition& layout);

  // return weighted wire-length to get best solution
  double GetWeightedWL();

  void StubPlacer(double snapGrid);

  void init(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log);

  void setGlobalConfig(const char* globalConfig);
  void setLocalConfig(const char* localConfig);
  void setPlotEnable(bool mode);

  void setVerboseLevel(int verbose);
  void setFenceRegion(double lx, double ly, double ux, double uy);

  void PlaceMacros(int& solCount);

  const bool isTiming() const { return isTiming_; }

private:
  void FillMacroStor();
  void FillPinGroup();
  void FillVertexEdge();
  void CheckGraphInfo();
  void FillMacroPinAdjMatrix();
  void FillMacroConnection();

  void UpdateVertexToMacroStor();
  void UpdateInstanceToMacroStor();

  // either Pin*, Inst* -> vertexStor's index.
  std::unordered_map<void*, int> pinInstVertexMap;

  // adjacency matrix for whole(macro/pins/FFs) graph
  Eigen::SparseMatrix<int, Eigen::RowMajor> adjMatrix;

  // vertex idx --> macroPinAdjMatrix idx.
  std::vector<int> macroPinAdjMatrixMap;

  // adjacency matrix for macro/pins graph
  Eigen::SparseMatrix<int, Eigen::RowMajor> macroPinAdjMatrix;

  int index(Vertex* vertex);

  // Matrix version
  int GetPathWeightMatrix(Eigen::SparseMatrix<int, Eigen::RowMajor>& mat,
                          Vertex* from,
                          Vertex* to);

  // Matrix version
  int GetPathWeightMatrix(Eigen::SparseMatrix<int, Eigen::RowMajor>& mat,
                          Vertex* from,
                          int toIdx);

  // Matrix version
  int GetPathWeightMatrix(Eigen::SparseMatrix<int, Eigen::RowMajor>& mat,
                          int fromIdx,
                          int toIdx);

  Vertex* GetVertex(sta::Pin* pin);

  std::pair<void*, VertexType> GetPtrClassPair(sta::Pin* pin);

  void init();
  void reset();

  // graph based adjacencies
  void findAdjacencies();
  void findFanins(sta::BfsFwdIterator &bfs,
                  VertexFaninMap &vertex_fanins,
                  sta::dbNetwork *network,
                  sta::Graph *graph);
  void copyFaninsAcrossRegisters(sta::BfsFwdIterator &bfs,
                                 VertexFaninMap &vertex_fanins,
                                 sta::dbNetwork *network,
                                 sta::Graph *graph);
  sta::Pin *findSeqOutPin(sta::Instance *inst,
                          sta::LibertyPort *out_port,
                          sta::Network *network);
  PinGroupLocation findNearestEdge(odb::dbBTerm* bTerm);
  std::string faninName(Macro *macro);
  int macroIndex(Macro *macro);
  bool macroIndexIsEdge(Macro *macro);

  ////////////////////////////////////////////////////////////////
    
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* log_;

  // config filenames
  std::string globalConfig_;
  std::string localConfig_;

  bool isTiming_;
  bool isPlot_;

  // layout
  double lx_, ly_, ux_, uy_;
  double fenceLx_, fenceLy_, fenceUx_, fenceUy_;
  double siteSizeX_, siteSizeY_;
  double haloX_, haloY_;
  double channelX_, channelY_;
  double* netTable_;
  int verbose_;
  bool fenceRegionMode_;
};

class Layout
{
 public:
  Layout();
  Layout(double lx, double ly, double ux, double uy);
  Layout(Layout& orig, Partition& part);

  double lx() const;
  double ly() const;
  double ux() const;
  double uy() const;

  void setLx(double lx);
  void setLy(double ly);
  void setUx(double ux);
  void setUy(double uy);

 private:
  double lx_, ly_, ux_, uy_;
};

inline double Layout::lx() const
{
  return lx_;
}

inline double Layout::ly() const
{
  return ly_;
}

inline double Layout::ux() const
{
  return ux_;
}

inline double Layout::uy() const
{
  return uy_;
}

}  // namespace mpl

