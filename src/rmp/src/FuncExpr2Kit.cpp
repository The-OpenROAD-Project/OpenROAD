/*
Convert a functional expression in Timing Analyzer
to a Kit data structure suitable for abc transformation
*/

#include <base/abc/abc.h>
#include <bool/kit/kit.h>
#include <misc/util/abc_global.h>
#include <rmp/FuncExpr2Kit.h>

#include <sta/FuncExpr.hh>
#include <sta/Liberty.hh>
#include <sta/Network.hh>
#include <sta/StringUtil.hh>

namespace sta {

FuncExpr2Kit::FuncExpr2Kit(FuncExpr* fe, int& var_count, unsigned*& kit_table)
{
  var_count_ = 0;

  FuncExprPortIterator iter(fe);
  while (iter.hasNext()) {
    LibertyPort* lp = iter.next();
    lib_ports_[var_count_] = lp;
    var_count_++;
  }
  var_count = var_count_;

  for (int i = 0; i < var_count_; i++) {
    int nWords = abc::Kit_TruthWordNum(var_count_);
    unsigned* pTruth = ABC_ALLOC(unsigned, nWords);
    abc::Kit_TruthIthVar(pTruth, var_count_, i);
    kit_vars_[lib_ports_[i]] = pTruth;
  }
  int nWords = abc::Kit_TruthWordNum(var_count_);
  kit_table = ABC_ALLOC(unsigned, nWords);
  RecursivelyEvaluate(fe, kit_table);
}

void FuncExpr2Kit::RecursivelyEvaluate(FuncExpr* fe, unsigned* result)
{
  if (fe) {
    switch (fe->op()) {
      case FuncExpr::op_port: {
        LibertyPort* p = fe->port();
        abc::Kit_TruthCopy(result, kit_vars_[p], var_count_);
      } break;

      case FuncExpr::op_not: {
        int nWords = abc::Kit_TruthWordNum(var_count_);
        unsigned* result_left = ABC_ALLOC(unsigned, nWords);
        RecursivelyEvaluate(fe->left(), result_left);
        abc::Kit_TruthNot(result, result_left, var_count_);
        ABC_FREE(result_left);
      } break;

      case FuncExpr::op_or: {
        int nWords = abc::Kit_TruthWordNum(var_count_);
        unsigned* result_left = ABC_ALLOC(unsigned, nWords);
        unsigned* result_right = ABC_ALLOC(unsigned, nWords);
        RecursivelyEvaluate(fe->left(), result_left);
        RecursivelyEvaluate(fe->right(), result_right);
        abc::Kit_TruthOr(result, result_left, result_right, var_count_);
        ABC_FREE(result_left);
        ABC_FREE(result_right);
      } break;
      case FuncExpr::op_and: {
        int nWords = abc::Kit_TruthWordNum(var_count_);
        unsigned* result_left = ABC_ALLOC(unsigned, nWords);
        unsigned* result_right = ABC_ALLOC(unsigned, nWords);
        RecursivelyEvaluate(fe->left(), result_left);
        RecursivelyEvaluate(fe->right(), result_right);
        abc::Kit_TruthAnd(result, result_left, result_right, var_count_);
        ABC_FREE(result_left);
        ABC_FREE(result_right);
      } break;
      case FuncExpr::op_xor: {
        int nWords = abc::Kit_TruthWordNum(var_count_);
        unsigned* result_left = ABC_ALLOC(unsigned, nWords);
        unsigned* result_right = ABC_ALLOC(unsigned, nWords);
        RecursivelyEvaluate(fe->left(), result_left);
        RecursivelyEvaluate(fe->right(), result_right);
        abc::Kit_TruthXor(result, result_left, result_right, var_count_);
        ABC_FREE(result_left);
        ABC_FREE(result_right);
      } break;
      case FuncExpr::op_one: {
        int nWords = abc::Kit_TruthWordNum(var_count_);
        unsigned* const_zero = ABC_ALLOC(unsigned, nWords);
        abc::Kit_TruthFill(const_zero, var_count_);
        abc::Kit_TruthNot(result, const_zero, var_count_);
        ABC_FREE(const_zero);
      } break;

      case FuncExpr::op_zero: {
        abc::Kit_TruthFill(result, var_count_);
      } break;
    }
  }
}
}  // namespace sta
