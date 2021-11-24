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
    for (unsigned int i=0; i<$1.size(); i++) {
        T* ptr = new T((($1_type &)$1)[i]);
        Tcl_ListObjAppendElement(interp, $result,  SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
}
%typemap(out) std::vector< T* > {
    for (unsigned int i = 0; i < $1.size(); i++) {
        T* ptr = ((($1_type &)$1)[i]);
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
}

%apply std::vector<odb::dbShape> &OUTPUT { std::vector<odb::dbShape> & boxes };


// Wrap containers
WRAP_DB_CONTAINER(odb::dbProperty)
WRAP_DB_CONTAINER(odb::dbLib)
WRAP_DB_CONTAINER(odb::dbChip)
WRAP_DB_CONTAINER(odb::dbBlock)
WRAP_DB_CONTAINER(odb::dbBTerm)
WRAP_DB_CONTAINER(odb::dbITerm)
WRAP_DB_CONTAINER(odb::dbInst)
WRAP_DB_CONTAINER(odb::dbObstruction)
WRAP_DB_CONTAINER(odb::dbBlockage)
WRAP_DB_CONTAINER(odb::dbWire)
WRAP_DB_CONTAINER(odb::dbNet)
WRAP_DB_CONTAINER(odb::dbCapNode)
WRAP_DB_CONTAINER(odb::dbRSeg)
WRAP_DB_CONTAINER(odb::dbVia)
WRAP_DB_CONTAINER(odb::dbTrackGrid)
WRAP_DB_CONTAINER(odb::dbRow)
WRAP_DB_CONTAINER(odb::dbCCSeg)
WRAP_DB_CONTAINER(odb::dbRegion)
WRAP_DB_CONTAINER(odb::dbTechNonDefaultRule)
WRAP_DB_CONTAINER(odb::dbBPin)
WRAP_DB_CONTAINER(odb::dbSWire)
WRAP_DB_CONTAINER(odb::dbBox)
WRAP_DB_CONTAINER(odb::dbSBox)
WRAP_DB_CONTAINER(odb::dbMaster)
WRAP_DB_CONTAINER(odb::dbSite)
WRAP_DB_CONTAINER(odb::dbMTerm)
WRAP_DB_CONTAINER(odb::dbMPin)
WRAP_DB_CONTAINER(odb::dbTarget)
WRAP_DB_CONTAINER(odb::dbTechLayer)
WRAP_DB_CONTAINER(odb::dbTechVia)
WRAP_DB_CONTAINER(odb::dbTechViaRule)
WRAP_DB_CONTAINER(odb::dbTechViaGenerateRule)
WRAP_DB_CONTAINER(odb::dbTechLayerSpacingRule)
WRAP_DB_CONTAINER(odb::dbTechV55InfluenceEntry)
WRAP_DB_CONTAINER(odb::dbTechMinCutRule)
WRAP_DB_CONTAINER(odb::dbTechMinEncRule)
WRAP_DB_CONTAINER(odb::dbModule)
WRAP_DB_CONTAINER(odb::dbModInst)
WRAP_DB_CONTAINER(odb::dbGroup)
WRAP_DB_CONTAINER(odb::dbTechLayerMinStepRule)
WRAP_DB_CONTAINER(odb::dbTechLayerCornerSpacingRule)
WRAP_DB_CONTAINER(odb::dbTechLayerSpacingTablePrlRule)
WRAP_DB_CONTAINER(odb::dbTechLayerCutClassRule)
WRAP_DB_CONTAINER(odb::dbTechLayerCutSpacingRule)
WRAP_DB_CONTAINER(odb::dbTechLayerCutSpacingTableOrthRule)
WRAP_DB_CONTAINER(odb::dbTechLayerCutSpacingTableDefRule)
