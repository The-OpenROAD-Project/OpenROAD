#pragma once
#ifdef ENABLE_ABC

#include "detail/foreach.hpp"
#include <kitty/dynamic_truth_table.hpp>

namespace abc {
  typedef struct Gia_Man_t_ Gia_Man_t;
  typedef struct Gia_Obj_t_ Gia_Obj_t;
  typedef struct Abc_Frame_t_ Abc_Frame_t;
  
  inline int      Abc_LitNot( int Lit )                  { assert(Lit >= 0); return Lit ^ 1;                           }
  inline int      Abc_Lit2Var( int Lit )                 { assert(Lit >= 0); return Lit >> 1;                          }
  inline int      Abc_LitRegular( int Lit )              { assert(Lit >= 0); return Lit & ~01;                         }

  extern Gia_Obj_t * Gia_Regular( Gia_Obj_t * p );
  extern Gia_Obj_t * Gia_Not( Gia_Obj_t * p );
  extern Gia_Man_t * Gia_ManStart(int nObjsMax);
  extern void        Gia_ManStop(Gia_Man_t * p);
  extern int         Gia_ManAppendCi(Gia_Man_t * p);
  extern int         Gia_ManAppendCo(Gia_Man_t * p, int iLit0);
  extern int         Gia_ManAppendAnd2(Gia_Man_t * p, int iLit0, int iLit1);
  extern Gia_Obj_t * Gia_ManObj(Gia_Man_t * p, int v);
  extern int         Gia_ObjIsPi(Gia_Man_t * p, Gia_Obj_t * pObj);
  extern int         Gia_ObjIsAnd(Gia_Obj_t * pObj);
  extern int         Gia_IsComplement(Gia_Obj_t * p);
  extern int         Gia_ManPiNum(Gia_Man_t * p);
  extern int         Gia_ManPoNum(Gia_Man_t * p);
  extern int         Gia_ManAndNum(Gia_Man_t * p);
  extern int         Gia_ManObjNum(Gia_Man_t * p);
  extern Gia_Obj_t * Gia_ManPi(Gia_Man_t * p, int v);
  extern int         Gia_Obj2Lit(Gia_Man_t * p, Gia_Obj_t * pObj);
  extern Gia_Obj_t * Gia_ManCi(Gia_Man_t * p, int v);
  extern Gia_Obj_t * Gia_ManCo(Gia_Man_t * p, int v);
  extern Gia_Obj_t * Gia_ObjFanin0(Gia_Obj_t * pObj);
  extern Gia_Obj_t * Gia_ObjFanin1(Gia_Obj_t * pObj);
  extern int Gia_ObjFaninC0(Gia_Obj_t * pObj);
  extern int Gia_ObjFaninC1(Gia_Obj_t * pObj);
  extern int         Gia_ManLevelNum( Gia_Man_t * p );
  extern Gia_Obj_t * Gia_ManConst0( Gia_Man_t * p );
  extern Gia_Obj_t * Gia_ManConst1( Gia_Man_t * p );
  extern int Gia_ObjId( Gia_Man_t * p, Gia_Obj_t * pObj );
  extern Gia_Man_t * Gia_ManDup( Gia_Man_t * p );

  extern Abc_Frame_t * Abc_FrameGetGlobalFrame();
  extern void Abc_FrameUpdateGia( Abc_Frame_t * p, Gia_Man_t * pNew );
  extern Gia_Man_t * Abc_FrameGetGia( Abc_Frame_t * p );
  extern int Cmd_CommandExecute( Abc_Frame_t * pAbc, const char * sCommand );
}

#include <iostream>

namespace mockturtle {

class gia_network;
  
class gia_signal {
  friend class gia_network;

public:
  explicit gia_signal() = default;
  explicit gia_signal(abc::Gia_Obj_t * obj) : obj_(obj) {}
 
  gia_signal operator!() const { return gia_signal(abc::Gia_Not(obj_)); }
  gia_signal operator+() const { return gia_signal(abc::Gia_Regular(obj_)); }
  gia_signal operator-() const { return gia_signal(abc::Gia_Not(abc::Gia_Regular(obj_))); }
 
  bool operator==(const gia_signal& other) const { return obj_ == other.obj_; }
  bool operator!=(const gia_signal& other) const { return !operator==(other); }
  bool operator<(const gia_signal& other) const { return obj_ < other.obj_; }

  abc::Gia_Obj_t * obj() const { return obj_; }
  
private:
  abc::Gia_Obj_t * obj_;
};
  
class gia_network {
public:
  static constexpr auto min_fanin_size = 2u;
  static constexpr auto max_fanin_size = 2u;

  using base_type = gia_network;
  using node = int;
  using signal = gia_signal;
  using storage = abc::Gia_Man_t*;
  
  gia_network(int size)
    : gia_(abc::Gia_ManStart(size)) /* doesn't automatically resize? */
  {}

  /* network does not implement constant value */
  bool constant_value(node n) const { (void)n; return false; }

  /* each node implements AND function */
  kitty::dynamic_truth_table node_function(node n) const { (void)n; kitty::dynamic_truth_table tt(2); tt._bits[0] = 0x8; return tt; }

  
  signal get_constant(bool value) const {
    return value ? signal(abc::Gia_ManConst1(gia_)) : signal(abc::Gia_ManConst0(gia_));
  }
  
  signal create_pi() {
    return signal(abc::Gia_ManObj(gia_, abc::Abc_Lit2Var(abc::Gia_ManAppendCi(gia_))));
  }

  void create_po(const signal& f) {
    /* po_node = */abc::Gia_ManAppendCo(gia_, abc::Gia_Obj2Lit(gia_, f.obj()));
  }

  signal create_not(const signal& f) {
    return !f;
  }
  
  signal create_and(const signal& f, const signal& g) {
    return signal(abc::Gia_ManObj(gia_, abc::Abc_Lit2Var(abc::Gia_ManAppendAnd2(gia_, abc::Gia_Obj2Lit(gia_, f.obj()), abc::Gia_Obj2Lit(gia_, g.obj())))));
  }

  bool is_constant(node n) const {
    return n == 0;
  }

  node get_node(const signal& f) const {
    return Gia_ObjId(gia_, f.obj());
  }

  bool is_pi(node const& n) const {
    return abc::Gia_ObjIsPi(gia_, abc::Gia_ManObj(gia_, n));
  }

  bool is_complemented(const signal& f) const {
    return Gia_IsComplement(f.obj());
  }

  template<typename Fn>
  void foreach_pi(Fn&& fn) const {
    abc::Gia_Obj_t * pObj;
    for (int i = 0; (i < abc::Gia_ManPiNum(gia_)) && ((pObj) = abc::Gia_ManCi(gia_, i)); ++i) {
      fn(Gia_ObjId(gia_, pObj));
    }
  }

  template<typename Fn>
  void foreach_po(Fn&& fn) const {
    abc::Gia_Obj_t * pObj, * pFiObj;
    for (int i = 0; (i < abc::Gia_ManPoNum(gia_)) && ((pObj) = abc::Gia_ManCo(gia_, i)); ++i) {
      pFiObj = abc::Gia_ObjFanin0(pObj);
      fn(abc::Gia_ObjFaninC0(pObj) ? !signal(pFiObj) : signal(pFiObj));
    }
  }

  template<typename Fn>
  void foreach_gate(Fn&& fn) const {
    abc::Gia_Obj_t * pObj;
    for (int i = 0; i < abc::Gia_ManObjNum(gia_) && ((pObj) = abc::Gia_ManObj(gia_, i)); ++i) {
      if (abc::Gia_ObjIsAnd(pObj)) {
        fn(Gia_ObjId(gia_, pObj));
      }
    }
  }  

  template<typename Fn>
  void foreach_node(Fn&& fn) const {
    abc::Gia_Obj_t * pObj;
    for (int i = 0; i < abc::Gia_ManObjNum(gia_) && ((pObj) = abc::Gia_ManObj(gia_, i)); ++i) {
      fn(signal(pObj));
    }
  }  

  template<typename Fn>
  void foreach_fanin(node n, Fn&& fn) const {
    static_assert( detail::is_callable_without_index_v<Fn, signal, bool> ||
                   detail::is_callable_with_index_v<Fn, signal, bool> ||
                   detail::is_callable_without_index_v<Fn, signal, void> ||
                   detail::is_callable_with_index_v<Fn, signal, void> );

    if (n == 0 || is_pi(n)) { return; }    

    abc::Gia_Obj_t * pObj = abc::Gia_ManObj(gia_, n);
    if constexpr ( detail::is_callable_without_index_v<Fn, signal, bool> )
    {
      if (!fn(signal(abc::Gia_ObjFaninC0(pObj) ? Gia_Not(abc::Gia_ObjFanin0(pObj)) : abc::Gia_ObjFanin0(pObj)))) {
        return;
      }
      fn(signal(abc::Gia_ObjFaninC1(pObj) ? Gia_Not(abc::Gia_ObjFanin1(pObj)) : abc::Gia_ObjFanin1(pObj)));
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, bool> )
    {
      if (!fn(signal(abc::Gia_ObjFaninC0(pObj) ? Gia_Not(abc::Gia_ObjFanin0(pObj)) : abc::Gia_ObjFanin0(pObj))), 0) {
        return;
      }
      fn(signal(abc::Gia_ObjFaninC1(pObj) ? Gia_Not(abc::Gia_ObjFanin1(pObj)) : abc::Gia_ObjFanin1(pObj)), 1);
    }
    else if constexpr ( detail::is_callable_without_index_v<Fn, signal, void> )
    {
      fn(signal(abc::Gia_ObjFaninC0(pObj) ? Gia_Not(abc::Gia_ObjFanin0(pObj)) : abc::Gia_ObjFanin0(pObj)));
      fn(signal(abc::Gia_ObjFaninC1(pObj) ? Gia_Not(abc::Gia_ObjFanin1(pObj)) : abc::Gia_ObjFanin1(pObj)));
    }
    else if constexpr ( detail::is_callable_with_index_v<Fn, signal, void> )
    {
      fn(signal(abc::Gia_ObjFaninC0(pObj) ? Gia_Not(abc::Gia_ObjFanin0(pObj)) : abc::Gia_ObjFanin0(pObj)), 0);
      fn(signal(abc::Gia_ObjFaninC1(pObj) ? Gia_Not(abc::Gia_ObjFanin1(pObj)) : abc::Gia_ObjFanin1(pObj)), 1);
    }
  }  

  int literal(const signal& f) const
  {
    return (abc::Gia_ObjId(gia_, f.obj()) << 1) + abc::Gia_IsComplement(f.obj());
  }
  
  int node_to_index(node n) const
  {
    return n;
  }

  auto num_pis() const { return abc::Gia_ManPiNum(gia_); }
  auto num_pos() const { return abc::Gia_ManPoNum(gia_); }
  auto num_gates() const { return abc::Gia_ManAndNum(gia_); }
  auto num_levels() const { return abc::Gia_ManLevelNum(gia_); }
  auto size() const { return abc::Gia_ManObjNum(gia_); }

  bool load_rc() {
    abc::Abc_Frame_t * abc = abc::Abc_FrameGetGlobalFrame();
    const int success = abc::Cmd_CommandExecute(abc, default_rc);
    if (success != 0) {
      printf("syntax error in script\n");
    }
    return success == 0;
  }

  bool run_opt_script(const std::string &script) {
    abc::Gia_Man_t * gia = abc::Gia_ManDup(gia_);
    abc::Abc_Frame_t * abc = abc::Abc_FrameGetGlobalFrame();
    abc::Abc_FrameUpdateGia(abc, gia);

    const int success = abc::Cmd_CommandExecute(abc, script.c_str());
    if (success != 0) {
      printf("syntax error in script\n");
    }

    abc::Gia_Man_t * new_gia = abc::Abc_FrameGetGia(abc);
    abc::Gia_ManStop(gia_);
    gia_ = new_gia;
    
    return success == 0;
  }

private:
  const char * default_rc =
    "alias b balance;\n"
    "alias rw rewrite;\n"
    "alias rwz rewrite -z;\n"
    "alias rf refactor;\n"
    "alias rfz refactor -z;\n"
    "alias rs resub;\n"
    "alias rsz resub -z;\n"
    "alias &r2rs '&put; b; rs -K 6; rw; rs -K 6 -N 2; rf; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rfz; rs -K 12 -N 2; rwz; b; &get';\n"
    "alias &c2rs '&put; b -l; rs -K 6 -l; rw -l; rs -K 6 -N 2 -l; rf -l; rs -K 8 -l; b -l; rs -K 8 -N 2 -l; rw -l; rs -K 10 -l; rwz -l; rs -K 10 -N 2 -l; b -l; rs -K 12 -l; rfz -l; rs -K 12 -N 2 -l; rwz -l; b -l; &get';\n"
    "alias compress2rs 'b -l; rs -K 6 -l; rw -l; rs -K 6 -N 2 -l; rf -l; rs -K 8 -l; b -l; rs -K 8 -N 2 -l; rw -l; rs -K 10 -l; rwz -l; rs -K 10 -N 2 -l; b -l; rs -K 12 -l; rfz -l; rs -K 12 -N 2 -l; rwz -l; b -l';\n"
    "alias resyn2rs 'b; rs -K 6; rw; rs -K 6 -N 2; rf; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rfz; rs -K 12 -N 2; rwz; b;'\n"
    "alias resyn2rs2 'b; rs -K 6; rw; rs -K 6 -N 2; rs -K 8; b; rs -K 8 -N 2; rw; rs -K 10; rwz; rs -K 10 -N 2; b; rs -K 12; rs -K 12 -N 2; rwz; b;'\n";

private:
  abc::Gia_Man_t *gia_;
}; // gia_network

} // mockturtle

#endif
