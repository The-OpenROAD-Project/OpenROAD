#ifndef __MACRO_PLACER_PARTITION__
#define __MACRO_PLACER_PARTITION__


#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace MacroPlace {

class MacroCircuit;
class Macro;
class Logger;

enum PartClass {
  S, N, W, E, NW, NE, SW, SE, ALL, None
};

struct PartClassHash;
struct PartClassEqual;

class Partition {
  public:
    PartClass partClass;
    double lx, ly;
    double width, height;
    std::vector<Macro> macroStor;
    double* netTable;
    int tableCnt;
    std::unordered_map<std::string, int> macroMap;


    Partition();
    Partition(PartClass _partClass, double _lx, double _ly,
        double _width, double _height );

    // destructor
    ~Partition();

    // copy constructor
    Partition(const Partition& prev);

    // assign operator overloading
    Partition& operator= (const Partition& prev);

    void FillNetlistTable(MacroCircuit& mckt,
        std::unordered_map<PartClass, std::vector<int>,
        PartClassHash, PartClassEqual>& macroPartMap);

    void Dump();

    // Call Parquet to have annealing solution
    bool DoAnneal(std::shared_ptr<Logger>);

    // Update Macro location from MacroCircuit
    void UpdateMacroCoordi(MacroCircuit& mckt);

    // Writing functions
    void PrintSetFormat(FILE* fp);

    // Parquet print functions
    void PrintParquetFormat(std::string origName);
    void WriteBlkFile(std::string blkName);
    void WriteNetFile(std::vector< std::pair<int, int>> & netStor, std::string netName);
    void WriteWtsFile(std::vector<int>& costStor, std::string wtsName );
    void WritePlFile(std::string plName);

    std::string GetName(int macroIdx);

  private:
    void FillNetlistTableIncr();
    void FillNetlistTableDesc();
};

struct PartClassHash {
  std::size_t operator()(const MacroPlace::PartClass &k) const {
    return k;
  }
};

struct PartClassEqual {
  bool operator()(const MacroPlace::PartClass &p1,
      const MacroPlace::PartClass &p2) const {
    return p1 == p2;
  }
};

}



#endif
