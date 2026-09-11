// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2019-2025, The OpenROAD Authors

%{
#if TCL_MAJOR_VERSION < 9 && !defined(Tcl_Size)
  typedef int Tcl_Size;
#endif

// Borrowed pointers reach Tcl as bare handle strings rather than as registered
// object commands.  Every shadow-creation site in the module funnels through the
// SWIG_NewInstanceObj macro, so redirecting it here covers the generated
// accessors and the container typemaps below alike.  Objects SWIG owns keep a
// real object command, which is what reclaims them.  Method dispatch on a bare
// handle goes through odb_unknown() in the wrapper section.
#undef  SWIG_NewInstanceObj
#define SWIG_NewInstanceObj(thisvalue, type, flags) \
        odbNewHandleObj(interp, thisvalue, type, flags)

static Tcl_Obj* odbNewHandleObj(Tcl_Interp* interp,
                                void* ptr,
                                swig_type_info* type,
                                int flags)
{
  if (flags) {
    return SWIG_Tcl_NewInstanceObj(interp, ptr, type, flags);
  }
  // SWIG encodes the handle by copying the bytes of the pointer
  // (SWIG_PackData reads &ptr), which gcc's escape analysis does not treat as
  // letting the pointee escape.  Inlined into a constructor wrapper, gcc then
  // proves the freshly allocated object is never read and drops the stores that
  // initialize it, so the handle names uninitialized memory.  Escaping the
  // pointer explicitly restores the dependency at no runtime cost.
#if defined(__GNUC__)
  asm volatile("" : : "r"(ptr) : "memory");
#endif
  return SWIG_Tcl_NewPointerObj(ptr, type, 0);
}

// Strips the global-namespace qualifier a caller may have written in front of a
// packed handle.  Returns a pointer into the original string.  A qualified
// object-command name keeps its qualifier, so that SWIG's own command lookup
// still resolves the global command the caller asked for rather than one that
// shadows it in the current namespace.
static const char* odbBareHandle(const char* name)
{
  const bool qualified_handle
      = name[0] == ':' && name[1] == ':' && name[2] == '_';
  return qualified_handle ? name + 2 : name;
}

// A qualified handle is not a resolvable command name, so normalize it before
// SWIG's own conversion sees it.
#undef  SWIG_ConvertPtr
#define SWIG_ConvertPtr(obj, ptr, type, flags) \
        odbConvertPtr(interp, obj, ptr, type, flags)

static int odbConvertPtr(Tcl_Interp* interp,
                         Tcl_Obj* obj,
                         void** ptr,
                         swig_type_info* type,
                         int flags)
{
  const char* name = Tcl_GetString(obj);
  const char* bare = odbBareHandle(name);
  if (bare != name) {
    return SWIG_Tcl_ConvertPtrFromString(interp, bare, ptr, type, flags);
  }
  return SWIG_Tcl_ConvertPtr(interp, obj, ptr, type, flags);
}
%}

%wrapper %{
// Holds the command prefix that odb_unknown displaced, so that commands which
// are not odb handles keep reaching it.
static const char* const odbDisplacedUnknownVar = "::odb_displaced_unknown";

// Dispatches "$handle method args..." for handles that have no object command.
// The handle string carries the pointer and its mangled type, so the swig_class
// method tables SWIG already generated supply the method lookup, including the
// base-class walk for inherited methods.
static int odbUnknownCmd(ClientData,
                         Tcl_Interp* interp,
                         int objc,
                         Tcl_Obj* const objv[])
{
  if (objc >= 3) {
    const char* name = Tcl_GetString(objv[1]);
    // Both "$handle method" and "::$handle method" are accepted.
    const char* bare = odbBareHandle(name);
    void* ptr = nullptr;
    if (bare[0] == '_') {
      const char* mangled = SWIG_UnpackData(bare + 1, &ptr, sizeof(void*));
      swig_type_info* type = mangled ? SWIG_MangledTypeQuery(mangled) : nullptr;
      if (type && type->clientdata) {
        const char* method = Tcl_GetString(objv[2]);
        if (strcmp(method, "-delete") == 0 || strcmp(method, "-acquire") == 0
            || strcmp(method, "-disown") == 0) {
          Tcl_AppendResult(
              interp, "no ownership operations on handle ", name, nullptr);
          return TCL_ERROR;
        }
        // The method wrappers decode the pointer from thisptr, so it has to be
        // the unqualified spelling.
        Tcl_Obj* thisptr
            = (bare == name) ? objv[1] : Tcl_NewStringObj(bare, -1);
        Tcl_IncrRefCount(thisptr);
        // Zero-initialized so that a field SWIG may add stays well defined.
        swig_instance inst = {};
        inst.thisptr = thisptr;
        inst.thisvalue = ptr;
        inst.classptr = (swig_class*) type->clientdata;
        const int code = SWIG_Tcl_MethodCommand(
            (ClientData) &inst, interp, objc - 1, objv + 1);
        Tcl_DecrRefCount(thisptr);
        return code;
      }
    }
  }

  // Not a handle: hand it to the handler odbInstallUnknown displaced, which is
  // a command prefix rather than a bare command name.
  Tcl_Obj* chain = Tcl_GetVar2Ex(
      interp, odbDisplacedUnknownVar, nullptr, TCL_GLOBAL_ONLY);
  Tcl_Size prefixc = 0;
  Tcl_Obj** prefixv = nullptr;
  if (chain == nullptr
      || Tcl_ListObjGetElements(interp, chain, &prefixc, &prefixv) != TCL_OK
      || prefixc == 0) {
    // Nothing was displaced, so Tcl's own default applies.
    chain = Tcl_NewStringObj("::unknown", -1);
    Tcl_IncrRefCount(chain);
    Tcl_ListObjGetElements(interp, chain, &prefixc, &prefixv);
  } else {
    Tcl_IncrRefCount(chain);
  }

  const int argc = prefixc + objc - 1;
  Tcl_Obj** argv = (Tcl_Obj**) ckalloc(sizeof(Tcl_Obj*) * argc);
  for (Tcl_Size i = 0; i < prefixc; i++) {
    argv[i] = prefixv[i];
  }
  for (int i = 1; i < objc; i++) {
    argv[prefixc + i - 1] = objv[i];
  }
  const int code = Tcl_EvalObjv(interp, argc, argv, 0);
  ckfree((char*) argv);
  Tcl_DecrRefCount(chain);
  return code;
}

// Makes odb_unknown the global namespace unknown handler, remembering the
// command prefix it displaces.  Idempotent, so an application can call it again
// after installing a handler of its own.
static int odbInstallUnknown(ClientData,
                             Tcl_Interp* interp,
                             int,
                             Tcl_Obj* const[])
{
  static const char* const query = "namespace eval :: {namespace unknown}";
  if (Tcl_Eval(interp, query) != TCL_OK) {
    return TCL_ERROR;
  }
  Tcl_Obj* displaced = Tcl_GetObjResult(interp);
  if (strcmp(Tcl_GetString(displaced), "odb_unknown") != 0) {
    Tcl_SetVar2Ex(
        interp, odbDisplacedUnknownVar, nullptr, displaced, TCL_GLOBAL_ONLY);
  }
  Tcl_ResetResult(interp);

  static const char* const install
      = "namespace eval :: {namespace unknown odb_unknown}";
  return Tcl_Eval(interp, install);
}
%}

%init %{
  Tcl_CreateObjCommand(interp, "odb_unknown", odbUnknownCmd, nullptr, nullptr);
  Tcl_CreateObjCommand(
      interp, "odb_install_unknown", odbInstallUnknown, nullptr, nullptr);
  // Enough on its own for the standalone odbtcl interpreter.  An application
  // that installs its own handler after this (OpenROAD, via OpenSTA) has to
  // call odb_install_unknown again to put odb_unknown back in front of it.
  odbInstallUnknown(nullptr, interp, 0, nullptr);
%}

%import <std_vector.i>

%template(vector_str) std::vector<std::string>;

// DB specital types
%typemap(out) odb::dbStringProperty {
    Tcl_Obj *obj = Tcl_NewStringObj($1.getValue().c_str(), $1.getValue().length());
    Tcl_SetObjResult(interp, obj);
}
%typemap(out) odb::dbStringProperty {
    Tcl_Obj *obj = Tcl_NewStringObj($1.getValue().c_str(), $1.getValue().length());
    Tcl_SetObjResult(interp, obj);
}

%typemap(out) odb::Point, Point {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    Tcl_Obj *x = Tcl_NewIntObj($1.getX());
    Tcl_Obj *y = Tcl_NewIntObj($1.getY());
    Tcl_ListObjAppendElement(interp, list, x);
    Tcl_ListObjAppendElement(interp, list, y);
    Tcl_SetObjResult(interp, list);
}

%typemap(out) odb::Point3D, Point3D {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    Tcl_Obj *x = Tcl_NewIntObj($1.x());
    Tcl_Obj *y = Tcl_NewIntObj($1.y());
    Tcl_Obj *z = Tcl_NewIntObj($1.z());
    Tcl_ListObjAppendElement(interp, list, x);
    Tcl_ListObjAppendElement(interp, list, y);
    Tcl_ListObjAppendElement(interp, list, z);
    Tcl_SetObjResult(interp, list);
}

%typemap(out) std::optional<uint8_t> {
    if ($1.has_value()) {
        Tcl_SetIntObj($result, (int) $1.value());
    } else {
        Tcl_Obj *obj = Tcl_NewStringObj("NULL", 4);
        Tcl_SetObjResult(interp, obj);
    }
}

// Wrapper for dbSet, dbVector...etc
%define WRAP_DB_CONTAINER(T) 

%typemap(out) dbSet< T > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    swig_type_info *tf = SWIG_TypeQuery("T" "*");
    for (dbSet< T >::iterator itr = $1.begin(); itr != $1.end(); ++itr)
    {
        Tcl_Obj *obj = SWIG_NewInstanceObj(*itr, tf, 0);
        Tcl_ListObjAppendElement(interp, list, obj);
    }
    Tcl_SetObjResult(interp, list);
}
%typemap(out) dbVector< T > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    swig_type_info *tf = SWIG_TypeQuery("T" "*");
    for (dbVector< T >::iterator itr = $1.begin(); itr != $1.end(); ++itr)
    {
        Tcl_Obj *obj = SWIG_NewInstanceObj(*itr, tf, 0);
        Tcl_ListObjAppendElement(interp, list, obj);
    }
    Tcl_SetObjResult(interp, list);
}
%typemap(out) std::vector< T > {
    std::vector<T>& v = *&($1);
    for (size_t i = 0; i< v.size(); i++) {
        T* ptr = new T(v[i]);
        Tcl_ListObjAppendElement(interp, $result,  SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
}
%typemap(out) std::vector< T* > {
    std::vector<T*>& v = *&($1);
    for (size_t i = 0; i < v.size(); i++) {
        T* ptr = v[i];
        Tcl_ListObjAppendElement(interp, $result,  SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
}

%typemap(out) std::pair< int, int > {
    Tcl_ListObjAppendElement(interp, $result, Tcl_NewIntObj($1.first));
    Tcl_ListObjAppendElement(interp, $result, Tcl_NewIntObj($1.second));
}

%typemap(out) std::vector< std::pair< T*, int > > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    for (unsigned int i = 0; i < $1.size(); i++) {
        Tcl_Obj *sub_list = Tcl_NewListObj(0, nullptr);
        std::pair< T*, int > p = ((($1_type &)$1)[i]);
        T* ptr1 = p.first;
        int num = p.second;
        Tcl_Obj *obj = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        Tcl_ListObjAppendElement(interp, sub_list, obj);
        Tcl_ListObjAppendElement(interp, sub_list, Tcl_NewIntObj(num));
        Tcl_ListObjAppendElement(interp, list, sub_list);
    }
    Tcl_SetObjResult(interp, list);
}
%typemap(out) std::vector< std::tuple< T*, T*, int > > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    for (unsigned int i = 0; i < $1.size(); i++) {
        Tcl_Obj *sub_list = Tcl_NewListObj(0, nullptr);
        std::tuple< T*, T*, int > p = ((($1_type &)$1)[i]);
        T* ptr1 = std::get<0>(p);
        T* ptr2 = std::get<1>(p);
        int num = std::get<2>(p);
        Tcl_Obj *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        Tcl_Obj *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(T *), 0);
        Tcl_ListObjAppendElement(interp, sub_list, obj1);
        Tcl_ListObjAppendElement(interp, sub_list, obj2);
        Tcl_ListObjAppendElement(interp, sub_list, Tcl_NewIntObj(num));
        Tcl_ListObjAppendElement(interp, list, sub_list);
    }
    Tcl_SetObjResult(interp, list);
}
%typemap(out) std::vector< std::tuple< T*, int, int, int > > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    for (unsigned int i = 0; i < $1.size(); i++) {
        Tcl_Obj *sub_list = Tcl_NewListObj(0, nullptr);
        std::tuple< T*, int, int, int > p = ((($1_type &)$1)[i]);
        T* ptr = std::get<0>(p);
        int num1 = std::get<1>(p);
        int num2 = std::get<2>(p);
        int num3 = std::get<3>(p);
        Tcl_Obj *obj = SWIG_NewInstanceObj(ptr, $descriptor(T *), 0);
        Tcl_ListObjAppendElement(interp, sub_list, obj);
        Tcl_ListObjAppendElement(interp, sub_list, Tcl_NewIntObj(num1));
        Tcl_ListObjAppendElement(interp, sub_list, Tcl_NewIntObj(num2));
        Tcl_ListObjAppendElement(interp, sub_list, Tcl_NewIntObj(num3));
        Tcl_ListObjAppendElement(interp, list, sub_list);
    }
    Tcl_SetObjResult(interp, list);
}

%typemap(out) std::vector< std::pair< T*, T* > > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    for (unsigned int i = 0; i < $1.size(); i++) {
        Tcl_Obj *sub_list = Tcl_NewListObj(0, nullptr);
        std::pair< T*, T* > p = ((($1_type &)$1)[i]);
        T* ptr1 = p.first;
        T* ptr2 = p.second;
        Tcl_Obj *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        Tcl_Obj *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(T *), 0);
        Tcl_ListObjAppendElement(interp, sub_list, obj1);
        Tcl_ListObjAppendElement(interp, sub_list, obj2);
        Tcl_ListObjAppendElement(interp, list, sub_list);
    }
    Tcl_SetObjResult(interp, list);
}

%typemap(out) std::vector< std::pair< T*, odb::Rect > > {
    Tcl_Obj *list = Tcl_NewListObj(0, nullptr);
    for (unsigned int i = 0; i < $1.size(); i++) {
        Tcl_Obj *sub_list = Tcl_NewListObj(0, nullptr);
        std::pair< T*, odb::Rect > p = $1.at(i);
        T* ptr1 = p.first;
        odb::Rect* ptr2 = new odb::Rect(p.second);
        Tcl_Obj *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        Tcl_Obj *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(odb::Rect *), 0);
        Tcl_ListObjAppendElement(interp, sub_list, obj1);
        Tcl_ListObjAppendElement(interp, sub_list, obj2);
        Tcl_ListObjAppendElement(interp, list, sub_list);
    }
    Tcl_SetObjResult(interp, list);
}

%typemap(in) std::vector< T* >* (std::vector< T* > *v, std::vector< T* > w),
             std::vector< T* >& (std::vector< T* > *v, std::vector< T* > w) {
    Tcl_Obj **listobjv;
    Tcl_Size  nitems;
    Tcl_Size  i;
    T*        temp;
    swig_type_info *tf = SWIG_TypeQuery("T" "*");

    if(SWIG_ConvertPtr($input, (void **) &v, $&1_descriptor, 0) == 0) {
        $1 = v;
    } else {
        if(Tcl_ListObjGetElements(interp, $input, &nitems, &listobjv) == TCL_ERROR)
            return TCL_ERROR;
        w = std::vector< T *>();
        for (i = 0; i < nitems; i++) {
            if ((SWIG_ConvertPtr(listobjv[i],(void **) &temp, tf, 0)) != 0) {
                char message[] = 
                    "lllist of " #T " expected";
                Tcl_SetResult(interp, message, TCL_VOLATILE);
                return TCL_ERROR;
            }
            w.push_back(temp);
        } 
        $1 = &w;
    }
}
%typemap(typecheck) vector< T * >, std::vector< T * >, vector< T * > &, std::vector< T * > & {
    Tcl_Obj **listobjv;
    Tcl_Size   nitems;
    T         *temp;
    std::vector< T > *v;
    swig_type_info *tf = SWIG_TypeQuery("T" "*");
    if(SWIG_ConvertPtr($input, (void **) &v, $&1_descriptor, 0) == 0){
        $1 = 1;
    } else {
        if(Tcl_ListObjGetElements(interp, $input, &nitems, &listobjv) == TCL_ERROR)
            $1 = 0;
        else
            if (nitems == 0)
                $1 = 1;
       if (SWIG_ConvertPtr(listobjv[0],(void **) &temp, tf, 0) != 0) {
            $1 = 0;
        } else {
            $1 = 1;
        }
    }
}      
%enddef

%define WRAP_OBJECT_RETURN_REF(T, A)
%typemap(in, numinputs=0) T &OUTPUT (T temp) {
   $1 = new T(temp);
}

%typemap(argout) T &OUTPUT {
  swig_type_info *tf = SWIG_TypeQuery("T" "*");
  Tcl_Obj *obj = SWIG_NewInstanceObj($1, tf, 0);
  Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), obj);
}

%apply T &OUTPUT { T & A };
%enddef


// Handle return by ref.
%apply int &OUTPUT { int & overhang1, int & overhang2 };
%apply int &OUTPUT { int & x, int & y };
%apply int &OUTPUT { int & x_spacing, int & y_spacing };
WRAP_OBJECT_RETURN_REF(odb::Rect, r)
WRAP_OBJECT_RETURN_REF(odb::Rect, rect)
WRAP_OBJECT_RETURN_REF(odb::Rect, bbox)

WRAP_OBJECT_RETURN_REF(odb::dbViaParams, params_return)



// Some special cases for return by ref
%typemap(in, numinputs=1) std::vector<odb::dbShape> &OUTPUT (std::vector<odb::dbShape> temp) {
   $1 = new std::vector<odb::dbShape>(temp);
}
%typemap(argout) std::vector<odb::dbShape> &OUTPUT {
  swig_type_info *tf = SWIG_TypeQuery("odb::dbShape" "*");
  for(auto it = $1->begin(); it != $1->end(); it++) {
    Tcl_Obj *obj = SWIG_NewInstanceObj(&(*it), tf, 0);
    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), obj);
  }
}
%typemap(in, numinputs=0) odb::dbShape &OUTPUT (odb::dbShape temp) {
    $1 = new odb::dbShape(temp);
}
%typemap(argout) odb::dbShape &OUTPUT {
    swig_type_info *tf = SWIG_TypeQuery("odb::dbShape" "*");
    Tcl_Obj *obj = SWIG_NewInstanceObj($1, tf, SWIG_POINTER_OWN);
    Tcl_SetObjResult(interp, obj);
}
%typemap(argout) std::vector<int> &OUTPUT {
  for(auto it = $1->begin(); it != $1->end(); it++) {
    Tcl_Obj *obj = Tcl_NewIntObj(*it);
    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), obj);
  }
  delete $1;
}

%apply std::vector<odb::dbShape> &OUTPUT { std::vector<odb::dbShape> & shapes };
%apply odb::dbShape &OUTPUT { odb::dbShape & shape };

%include containers.i
