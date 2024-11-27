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
    int       nitems;
    int       i;
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
    int       nitems;
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
%typemap(argout) std::vector<int> &OUTPUT {
  for(auto it = $1->begin(); it != $1->end(); it++) {
    Tcl_Obj *obj = Tcl_NewIntObj(*it);
    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), obj);
  }
  delete $1;
}

%apply std::vector<odb::dbShape> &OUTPUT { std::vector<odb::dbShape> & shapes };

%include containers.i
