%rename(assign) *::operator=;
%rename(_print) *::print;
%rename(pre_inc) *::operator++();
%rename(post_inc) *::operator++(int);

%template(vector_str) std::vector<std::string>;

%typemap(typecheck,precedence=SWIG_TYPECHECK_INTEGER) uint {
   $1 = PyInt_Check($input) ? 1 : 0;
}

// DB specital types
%typemap(out) odb::dbStringProperty {
    PyObject *obj = PyString_FromString($1.getValue().c_str());
    $result = obj;
}

%typemap(out) odb::Point, Point {
    PyObject *list = PyList_New(0);
    PyObject *x = PyInt_FromLong($1.getX());
    PyObject *y = PyInt_FromLong($1.getY());
    PyList_Append(list, x);
    PyList_Append(list, y);
    $result = list;
}

// Wrapper for dbSet, dbVector...etc
%define WRAP_DB_CONTAINER(T)
%typemap(out) dbSet< T >, dbVector< T > {
    PyObject *list = PyList_New(0);
    swig_type_info *tf = SWIG_TypeQuery("T" "*");
    for (dbSet< T >::iterator itr = $1.begin(); itr != $1.end(); ++itr)
    {
        PyObject *obj = SWIG_NewInstanceObj(*itr, tf, 0);
        PyList_Append(list, obj);
    }
    $result = list;
}

%typemap(out) std::vector< T > {
    PyObject *list = PyList_New(0);
    for (unsigned int i=0; i<$1.size(); i++) {
        T* ptr = new T((($1_type &)$1)[i]);
        PyList_Append(list,  SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
    $result = list;
}
%typemap(out) std::vector< T* > {
    PyObject *list = PyList_New(0);
    for (unsigned int i = 0; i < $1.size(); i++) {
        T* ptr = ((($1_type &)$1)[i]);
        PyList_Append(list,  SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
    $result = list;
}

%typemap(out) std::pair< int, int > {
    PyObject *list = PyList_New(0);
    PyList_Append(list, PyInt_FromLong((long)$1.first));
    PyList_Append(list, PyInt_FromLong((long)$1.second));
    $result = list;
}

%typemap(out) std::vector< std::pair< T*, int > > {
    PyObject *list = PyList_New(0);
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(0);
        std::pair< T*, int > p = ((($1_type &)$1)[i]);
        T* ptr1 = p.first;
        int num = p.second;
        PyObject *obj = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyList_Append(sub_list, obj);
        PyList_Append(sub_list, PyInt_FromLong((long)num));
        PyList_Append(list, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::tuple< T*, T*, int > > {
    PyObject *list = PyList_New(0);
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(0);
        std::tuple< T*, T*, int > p = ((($1_type &)$1)[i]);
        T* ptr1 = std::get<0>(p);
        T* ptr2 = std::get<1>(p);
        int num = std::get<2>(p);
        PyObject *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyObject *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(T *), 0);
        PyList_Append(sub_list, obj1);
        PyList_Append(sub_list, obj2);
        PyList_Append(sub_list, PyInt_FromLong((long)num));
        PyList_Append(list, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::tuple< T*, int, int, int > > {
    PyObject *list = PyList_New(0);
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(0);
        std::tuple< T*, int, int, int > p = ((($1_type &)$1)[i]);
        T* ptr = std::get<0>(p);
        int num1 = std::get<1>(p);
        int num2 = std::get<2>(p);
        int num3 = std::get<3>(p);
        PyObject *obj = SWIG_NewInstanceObj(ptr, $descriptor(T *), 0);
        PyList_Append(sub_list, obj);
        PyList_Append(sub_list, PyInt_FromLong((long)num1));
        PyList_Append(sub_list, PyInt_FromLong((long)num2));
        PyList_Append(sub_list, PyInt_FromLong((long)num3));
        PyList_Append(list, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::pair< T*, T* > > {
    PyObject *list = PyList_New(0);
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(0);
        std::pair< T*, T* > p = ((($1_type &)$1)[i]);
        T* ptr1 = p.first;
        T* ptr2 = p.second;
        PyObject *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyObject *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(T *), 0);
        PyList_Append(sub_list, obj1);
        PyList_Append(sub_list, obj2);
        PyList_Append(list, sub_list);
    }
    $result = list;
}

%typemap(in) std::vector< T* >* (std::vector< T* > *v, std::vector< T* > w),
             std::vector< T* >& (std::vector< T* > *v, std::vector< T* > w) {

        int nitems;
        int i;
        T*        temp;

        if(SWIG_ConvertPtr($input, (void **) &v, \
                            $&1_descriptor, 0) == 0) {
            $1 = v;
        } else {
            // It isn't a vector< T > so it should be a list of T's
            if(PyList_Check($input)) {
                nitems = PyList_Size($input);
                w = std::vector< T *>();
                for (i = 0; i < nitems; i++) {
                    PyObject *o = PyList_GetItem($input,i);
                    if ((SWIG_ConvertPtr(o,(void **) &temp,
                                        $descriptor(T *),0)) != 0) {
                        PyErr_SetString(PyExc_TypeError,"list of " #T "expected");
                        return NULL;
                    }
                    w.push_back(temp);
                }
                $1 = &w;
            }
            else {
                PyErr_SetString(PyExc_TypeError,"not a list");
                return NULL;
            }
        }
}
%typemap(typecheck) vector< T * >, std::vector< T * >, vector< T * > &, std::vector< T * > & {
    int       nitems;
    T         *temp;
    std::vector< T > *v;
    swig_type_info *tf = SWIG_TypeQuery("T" "*");
    if(SWIG_ConvertPtr($input, (void **) &v, $&1_descriptor, 0) == 0){
        $1 = 1;
    } else {
        if(!PyList_Check($input))
            $1 = 0;
        else
            if (PyList_Size($input) == 0)
                $1 = 1;
       if (SWIG_ConvertPtr(PyList_GetItem($input,0),(void **) &temp, tf, 0) != 0) {
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
    PyObject *o, *o2, *o3;
    o = SWIG_NewInstanceObj($1, tf, 0);
    if ((!$result) || ($result == Py_None)) {
        $result = o;
    } else {
        if (!PyTuple_Check($result)) {
            PyObject *o2 = $result;
            $result = PyTuple_New(1);
            PyTuple_SetItem($result,0,o2);
        }
        o3 = PyTuple_New(1);
        PyTuple_SetItem(o3,0,o);
        o2 = $result;
        $result = PySequence_Concat(o2,o3);
        Py_DECREF(o2);
        Py_DECREF(o3);
    }
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
  for(std::vector<odb::dbShape>::iterator it = $1->begin(); it != $1->end(); it++) {
    PyObject *o = SWIG_NewInstanceObj(&(*it), tf, 0);
    $result = SWIG_Python_AppendOutput($result, o);
  }
}

%typemap(argout) std::vector<int> &OUTPUT {
  for(auto it = $1->begin(); it != $1->end(); it++) {
    PyObject *obj = PyInt_FromLong((long)*it);
    $result = SWIG_Python_AppendOutput($result, obj);
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
