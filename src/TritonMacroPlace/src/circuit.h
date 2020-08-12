#ifndef __MACRO_PLACER_CIRCUIT__
#define __MACRO_PLACER_CIRCUIT__ 

#include <unordered_map>
#include <vector>
#include <memory>

#include <Eigen/Core>
#include <Eigen/SparseCore>

#include "graph.h"
#include "partition.h"
#include "macro.h"
#include "hashUtil.h"

namespace sta {
class dbSta;
}

namespace odb {
class dbDatabase;
}

namespace MacroPlace {

class Layout;
class Logger;

class MacroCircuit {
  public:
    MacroCircuit();
    MacroCircuit(odb::dbDatabase* db, sta::dbSta* sta);

    std::vector<MacroPlace::Vertex> vertexStor;
    std::vector<MacroPlace::Edge> edgeStor;

    // macro Information
    std::vector<MacroPlace::Macro> macroStor;

    // pin Group Information
    std::vector<MacroPlace::PinGroup> pinGroupStor;

    // pin Group Map;
    // Pin* --> pinGroupStor's index.
    std::unordered_map<sta::Pin*, int> staToPinGroup;

    // macro name -> macroStor's index.
    std::unordered_map<std::string, int> macroNameMap;

    // macro idx/idx pair -> give each
    std::vector< std::vector<int> > macroWeight;

    std::string GetEdgeName(MacroPlace::Edge* edge);

    std::string GetVertexName(MacroPlace::Vertex* vertex);


    // sta::Instance* --> macroStor's index stor
    std::unordered_map<sta::Instance*, int> macroInstMap;

    // Update Macro Location from Partition info
    void UpdateMacroCoordi(MacroPlace::Partition& part);

    // parsing function
    void ParseGlobalConfig(std::string fileName);
    void ParseLocalConfig(std::string fileName);

    // save LocalCfg into this structure
    std::unordered_map< std::string, MacroLocalInfo > macroLocalMap;

    // plotting
    void Plot(std::string outputFile, std::vector<MacroPlace::Partition>& set);

    // netlist
    void UpdateNetlist(MacroPlace::Partition& layout);

    // return weighted wire-length to get best solution
    double GetWeightedWL();


    void StubPlacer(double snapGrid);

    // changing.....
    void setDb(odb::dbDatabase* db);
    void setSta(sta::dbSta* sta);

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

    std::shared_ptr<Logger> log_;

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
    std::unordered_map<MacroPlace::Vertex*, int> vertexPtrMap;


    // adjacency matrix for whole(macro/pins/FFs) graph
    Eigen::SparseMatrix<int, Eigen::RowMajor> adjMatrix;

    // vertex idx --> macroPinAdjMatrix idx.
    std::vector< int > macroPinAdjMatrixMap;

    // adjacency matrix for macro/pins graph
    Eigen::SparseMatrix<int, Eigen::RowMajor> macroPinAdjMatrix;

    // pair of <StartVertex*, EndVertex*> --> edgeStor's index
    std::unordered_map< std::pair<MacroPlace::Vertex*, MacroPlace::Vertex*>,
      int, PointerPairHash, PointerPairEqual > vertexPairEdgeMap;

    int GetPathWeight( MacroPlace::Vertex* from,
        MacroPlace::Vertex* to, int limit );

    // Matrix version
    int GetPathWeightMatrix ( Eigen::SparseMatrix<int, Eigen::RowMajor> & mat,
        MacroPlace::Vertex* from,
        MacroPlace::Vertex* to );

    // Matrix version
    int GetPathWeightMatrix ( Eigen::SparseMatrix<int, Eigen::RowMajor> & mat,
        MacroPlace::Vertex* from,
        int toIdx );

    // Matrix version
    int GetPathWeightMatrix ( Eigen::SparseMatrix<int, Eigen::RowMajor> & mat,
        int fromIdx, int toIdx );

    MacroPlace::Vertex*
      GetVertex( sta::Pin *pin );

    std::pair<void*, VertexType>
      GetPtrClassPair( sta::Pin* pin );

    void init();
    void reset();
};

class Layout {
  public:
    Layout();
    Layout( double lx, double ly, double ux, double uy );
    Layout( Layout& orig, MacroPlace::Partition& part );

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

inline double
Layout::lx() const {
  return lx_;
}

inline double
Layout::ly() const {
  return ly_;
}

inline double
Layout::ux() const {
  return ux_;
}

inline double
Layout::uy() const {
  return uy_;
}

}

#endif

