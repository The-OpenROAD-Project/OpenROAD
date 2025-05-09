#pragma once
#include <sta/FuncExpr.hh>
#include <sta/StringUtil.hh>
#include <sta/Liberty.hh>
#include <sta/Network.hh>


namespace sta {

  class FuncExpr2Kit {
  public:    
    FuncExpr2Kit(FuncExpr* fe, int& var_count, unsigned *&kit_tables);
    void RecursivelyEvaluate(FuncExpr* fe, unsigned*);
  private:
    std::map<LibertyPort*, unsigned*> kit_vars_; //pointer (not name) addressed.
    std::map<int,LibertyPort*>  lib_ports_;
    int var_count_;
  };
  
}
