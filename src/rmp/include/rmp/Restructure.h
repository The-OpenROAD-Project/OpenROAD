/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#pragma once

#include <functional>
#include <string>

#include "db_sta/dbSta.hh"
#include "rsz/Resizer.hh"

namespace abc {
  struct Abc_Obj_t_;
  struct Abc_Ntk_t_;
  struct Amap_Lib_t_;
  struct Amap_Par_t_;
  extern Abc_Ntk_t_* Abc_NtkDarAmap(Abc_Ntk_t_*, Amap_Par_t_*);  
}  // namespace abc

namespace utl {
class Logger;
}

namespace odb {
class dbDatabase;
class dbBlock;
class dbInst;
class dbNet;
class dbITerm;
}  // namespace odb

namespace sta {
  class dbSta;
  class Network;
  class Vertex;
  class LibertyBuilder;  
}  // namespace sta

namespace rmp {


  //Cut resynthesis stuff +++
  

  //Cut: Region of network to optimize. File Cut.cpp
  struct Cut {
    std::vector<sta::Pin*> roots_;
    std::vector<sta::Pin*> leaves_;
    std::vector<sta::Instance*> volume_;
    std::map<const sta::Pin*,sta::LibertyPort*> internal_pins_;
    int id_;
    sta::LibertyCell* cut_cell_;
    void Print(sta::Network* nwk);
    bool Check(sta::Network* nwk);

    
    //Build logic expression for cut.Each root has a FuncExpr, ordered in root order.
    void BuildFuncExpr(sta::Network* nwk,
		       sta::LibertyLibrary* cut_library,
		       std::vector<sta::FuncExpr*>& root_fns);
    
    bool LeafPin(const sta::Pin* cur_pin);
    
    sta::FuncExpr* ExpandFunctionExprR(sta::Network* nwk,
				       const sta::Pin* op_pin,
				       sta::FuncExpr* op_pin_fe,
				       std::map<const sta::Pin*,
				       sta::LibertyPort*>& leaf_pin_to_port,
				       std::map<const sta::Pin*, sta::FuncExpr*>& visited,
				       sta::LibertyBuilder& lib_builder
				       );
    sta::FuncExpr*  BuildFuncExprR(sta::Network* nwk,
				   sta::Instance* cur_inst,
				   const sta::Pin* op_pin,
				   sta::FuncExpr* op_pin_fe,
				   std::map<const sta::Pin*,sta::LibertyPort*>& leaf_pin2port,
				   std::map<const sta::Pin*, sta::FuncExpr*>& visited_pins,
				   sta::LibertyBuilder &lib_builder);
  };


  
  class CutGen { //File CutGen.cpp
    public:
    using expansionSet=std::vector<std::pair<std::vector<const sta::Pin*>, std::vector<const sta::Pin*> > >;
    CutGen(sta::Network* nwk, sta::LibertyLibrary* liberty_lib):
      network_(nwk),liberty_lib_(liberty_lib){}
    
    void GenerateInstanceWaveFrontCutSet(sta::Instance*,std::vector<Cut*>&);
    bool EnumerateExpansions(std::vector<const sta::Pin*>& candidate_wavefront,
			     expansionSet& expansion_set);
    Cut* BuildCutFromExpansionElement(sta::Instance* cur_inst,
				      const sta::Pin* root_pin,
				      std::vector<const sta::Pin*>& keep, //set of pins to keep
				      std::vector<const sta::Pin*>& expand);
    bool Boundary(const sta::Pin* cur_pin);
    const sta::Pin* WalkThroughBuffersAndInverters(Cut*,
						   const sta::Pin* op_pin,
						   std::set<const sta::Pin*>& unique_set);
  private:
    sta::Network* network_;
    sta::LibertyLibrary* liberty_lib_;
  };


    
  //Library reader. File LibRead.cpp
  class LibRead{
  public:
    LibRead(utl::Logger* logger, char* lib_file_name,bool verbose);
    
  private:
    utl::Logger* logger_;        
    char* lib_file_name_;
    bool verbose_;
    abc::Amap_Lib_t_ *lib_;
    friend class PhysRemap;
  };


  //STA -> ABC
  //
  //file Cut2Ntk.cpp
  //
  class Cut2Ntk {
  public:
    Cut2Ntk(utl::Logger* logger,
	    sta::Network*  nwk,
	    Cut* cut,
	    std::string cut_name):
      logger_(logger),
      sta_nwk_(nwk),
      cut_(cut),
      cut_name_(cut_name){}
    abc::Abc_Ntk_t_* BuildSopLogicNetlist();
    abc::Abc_Obj_t_* CreateGate(const sta::Instance* cur_inst);
    void CleanUpNetNets();
  private:
    utl::Logger* logger_;    
    sta::Network* sta_nwk_;    
    Cut* cut_;
    std::string cut_name_;
    abc::Abc_Ntk_t_* abc_nwk_;
    //cross reference dictionaries for ease of constraining.
    std::map<const sta::Pin*, abc::Abc_Obj_t_*> pi_table_;
    std::map<const sta::Pin*, abc::Abc_Obj_t_*> po_table_;    
    std::map<const sta::Pin*, abc::Abc_Obj_t_*>  net_table_sta2abc_; //sta -> abc
    std::map<abc::Abc_Obj_t_*, const sta::Pin*>  net_table_abc2sta_; //abc -> sta.
    friend class Restructure;    
  };


  //ABC -> STA
  //file Ntk2Cut.cpp
  class Ntk2Cut { 
  public:
    Ntk2Cut(abc::Abc_Ntk_t_*, Cut*, sta::Network*);
    void BuildNwkElements();
  private:
    abc::Abc_Ntk_t_* mapped_netlist_;
    Cut* cut_;
    sta::Network* target_nwk_;
  };

  //Timing record for constraining abc.
  struct TimingRecord {
    float required_rise;
    float required_fall;
    float arrival_rise;
    float arrival_fall;
    float load;
  } ;
  

  //Remap a cut
  //File: PhysRemap.cpp
  class PhysRemap{
  public:
    PhysRemap(utl::Logger* logger,
	      sta::Network* nwk,
	      Cut* cut_to_remap,
	      std::vector<std::pair<const sta::Pin*, TimingRecord*> > & timing_requirements,
	      char* lib_name,
	      LibRead& lib_file,
	      bool script,
	      std::string &script_to_apply,
	      bool accept
	      ):logger_(logger),
		nwk_(nwk),
		cut_(cut_to_remap),
		timing_requirements_(timing_requirements),
		lib_name_(lib_name),
		liberty_library_(lib_file.lib_),
		script_(script),
		script_to_apply_(script_to_apply),
		accept_(accept)
    {
      cut_name_ = "cut_" + std::to_string(cut_to_remap -> id_);
      global_optimize_ = false;
      verbose_=true;
    }
    void Remap();
    void AnnotateTimingRequirementsOntoABCNwk(abc::Abc_Ntk_t_* sop_logic_nwk);
    bool getAbcResult(abc::Abc_Ntk_t_*&);
    void BuildCutRealization(abc::Abc_Ntk_t_*);
    float TimingGain();
    
  private:
    utl::Logger* logger_;
    sta::Network* nwk_;
    
    Cut* cut_;
    std::vector<std::pair<const sta::Pin*, TimingRecord*> > & timing_requirements_;
    char* lib_name_;
    std::string cut_name_;
    abc::Amap_Lib_t_* liberty_library_;
    bool global_optimize_;
    bool verbose_;
    bool script_;
    std::string script_to_apply_;
    bool accept_;
    abc::Abc_Ntk_t_* intermediate_result_;
    friend class Restructure;    
  };
  //Cut resynthesis ----
  

  
using utl::Logger;

enum class Mode
{
  AREA_1 = 0,
  AREA_2,
  AREA_3,
  DELAY_1,
  DELAY_2,
  DELAY_3,
  DELAY_4
};

class Restructure
{
 public:
  Restructure() = default;
  ~Restructure();

  void init(utl::Logger* logger,
            sta::dbSta* open_sta,
            odb::dbDatabase* db,
            rsz::Resizer* resizer);
  void reset();
  void run(char* liberty_file_name,
           float slack_threshold,
           unsigned max_depth,
           char* workdir_name,
           char* abc_logfile);

  void setMode(const char* mode_name);
  void setTieLoPort(sta::LibertyPort* loport);
  void setTieHiPort(sta::LibertyPort* hiport);

  //Cut based resynthesis methods
  
  void cutresynthrun(char* liberty_file_name,
		     char* script,
		     sta::Pin* head_pin, //if null we do the big blob, if set we do single op
		     bool unconstrained,
		     bool verbose);

  void annotateCutTiming(Cut*, std::vector<std::pair<const sta::Pin*, TimingRecord*> >& timing_requirements);
  
  //Various cut generation methods
  //The default: big cut like a blob
  void generateUnconstrainedCuts(std::vector<Cut*>& cut_set);
  //Focussed: use wavefront cut
  void generateWaveFrontSingleOpCutSet(sta::Network* nwk,
				       sta::Pin* root,
				       std::vector<Cut*>& cut_set);
  void generateWaveFrontSingleOpCutSet(sta::Network* nwk,
				       sta::Instance* root,
				       std::vector<Cut*>& cut_set);
  

  static     bool isRegInput(sta::Network* nwk, sta::Vertex* vertex);
  static     bool isRegOutput(sta::Network* nwk, sta::Vertex* vertex);
  static     bool isPrimary(sta::Network* nwk, sta::Vertex* vertex);
  void ResetCutTemporaries(){ leaves_.clear(); roots_.clear(); cut_volume_.clear();ordered_leaves_.clear();ordered_roots_.clear(); pin_visited_.clear();}
  void AmendCutForEscapeLeaves(sta::Network* nwk);
  
  //privates for cut based resynthesis
private:
  //These are used for the blob/cut generation
  std::set<sta::Vertex*> end_points_; //full set of end points  
  std::map<sta::Pin*,int> leaves_;
  std::map<sta::Pin*,int> roots_;
  std::set<sta::Instance*> cut_volume_;
  std::set<sta::Pin*> pin_visited_;
  std::vector<sta::Pin*> ordered_leaves_;
  std::vector<sta::Pin*> ordered_roots_;
  Cut* extractCut(int cut_id);
  void walkBackwardsToTimingEndPointsR(sta::Graph* graph, sta::Network* nwk, sta::Pin* start_pin, int depth);
  void walkForwardsToTimingEndPointsR(sta::Graph* graph, sta::Network* nwk,  sta::Pin* start_pin, int depth);

  
 private:
  void deleteComponents();
  void getBlob(unsigned max_depth);
  void runABC();
  void postABC(float worst_slack);
  bool writeAbcScript(std::string file_name);
  void writeOptCommands(std::ofstream& script);
  void initDB();
  void getEndPoints(sta::PinSet& ends, bool area_mode, unsigned max_depth);
  int countConsts(odb::dbBlock* top_block);
  void removeConstCells();
  void removeConstCell(odb::dbInst* inst);
  bool readAbcLog(std::string abc_file_name, int& level_gain, float& delay_val);

  Logger* logger_;
  std::string logfile_;
  std::string locell_;
  std::string loport_;
  std::string hicell_;
  std::string hiport_;
  std::string work_dir_name_;

  // db vars
  sta::dbSta* open_sta_;
  odb::dbDatabase* db_;
  rsz::Resizer* resizer_;
  odb::dbBlock* block_ = nullptr;

  std::string input_blif_file_name_;
  std::string output_blif_file_name_;
  std::vector<std::string> lib_file_names_;
  std::set<odb::dbInst*> path_insts_;

  Mode opt_mode_;
  bool is_area_mode_;
};

}  // namespace rmp
