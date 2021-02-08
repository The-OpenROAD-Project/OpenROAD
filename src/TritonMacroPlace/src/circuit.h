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

#include "graph.h"
#include "hashUtil.h"
#include "macro.h"
#include "partition.h"

namespace sta {
class dbSta;
}

namespace odb {
class dbDatabase;
}

namespace utl {
class Logger;
}

namespace mpl {

class Layout;
class Logger;

class MacroCircuit
{
 public:
  MacroCircuit();
  MacroCircuit(odb::dbDatabase* db, sta::dbSta* sta, utl::Logger* log);

  std::vector<mpl::Vertex> vertexStor;
  std::vector<mpl::Edge> edgeStor;

  // macro Information
  std::vector<mpl::Macro> macroStor;

  // pin Group Information
  std::vector<mpl::PinGroup> pinGroupStor;

  // pin Group Map;
  // Pin* --> pinGroupStor's index.
  std::unordered_map<sta::Pin*, int> staToPinGroup;

  // macro name -> macroStor's index.
  std::unordered_map<std::string, int> macroNameMap;

  // macro idx/idx pair -> give each
  std::vector<std::vector<int>> macroWeight;

  std::string GetEdgeName(mpl::Edge* edge);
  std::string GetVertexName(mpl::Vertex* vertex);

  // sta::Instance* --> macroStor's index stor
  std::unordered_map<sta::Instance*, int> macroInstMap;

  // Update Macro Location from Partition info
  void UpdateMacroCoordi(mpl::Partition& part);

  // parsing function
  void ParseGlobalConfig(std::string fileName);
  void ParseLocalConfig(std::string fileName);

  // save LocalCfg into this structure
  std::unordered_map<std::string, MacroLocalInfo> macroLocalMap;

  // plotting
  void Plot(std::string outputFile, std::vector<mpl::Partition>& set);

  // netlist
  void UpdateNetlist(mpl::Partition& layout);

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
  odb::dbDatabase* db_;
  sta::dbSta* sta_;
  utl::Logger* log_;

  std::string globalConfig_;
  std::string localConfig_;

  bool isTiming_;
  bool isPlot_;

  // layout
  double lx_, ly_, ux_, uy_;

  double fenceLx_, fenceLy_, fenceUx_, fenceUy_;

  double siteSizeX_, siteSizeY_;

  // haloX, haloY
  double haloX_, haloY_;

  // channelX, channelY (TODO)
  double channelX_, channelY_;

  // netlistTable
  double* netTable_;

  // verboseLevel
  int verbose_;

  // fenceRegionMode
  bool fenceRegionMode_;

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

  // pair of <StartVertex*, EndVertex*> --> edgeStor's index
  std::unordered_map<std::pair<mpl::Vertex*, mpl::Vertex*>,
                     int,
                     PointerPairHash,
                     PointerPairEqual>
      vertexPairEdgeMap;

  int index(mpl::Vertex* vertex);

  // not used -cherry
  int GetPathWeight(mpl::Vertex* from, mpl::Vertex* to, int limit);

  // Matrix version
  int GetPathWeightMatrix(Eigen::SparseMatrix<int, Eigen::RowMajor>& mat,
                          mpl::Vertex* from,
                          mpl::Vertex* to);

  // Matrix version
  int GetPathWeightMatrix(Eigen::SparseMatrix<int, Eigen::RowMajor>& mat,
                          mpl::Vertex* from,
                          int toIdx);

  // Matrix version
  int GetPathWeightMatrix(Eigen::SparseMatrix<int, Eigen::RowMajor>& mat,
                          int fromIdx,
                          int toIdx);

  mpl::Vertex* GetVertex(sta::Pin* pin);

  std::pair<void*, VertexType> GetPtrClassPair(sta::Pin* pin);

  void init();
  void reset();
};

class Layout
{
 public:
  Layout();
  Layout(double lx, double ly, double ux, double uy);
  Layout(Layout& orig, mpl::Partition& part);

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

