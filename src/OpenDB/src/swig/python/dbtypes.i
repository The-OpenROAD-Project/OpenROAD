%rename(assign) *::operator=;
%rename(_print) *::print;
%rename(pre_inc) *::operator++();
%rename(post_inc) *::operator++(int);

%template(vector_str) std::vector<std::string>;


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

