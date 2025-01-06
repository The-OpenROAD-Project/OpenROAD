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
    PyObject *list = PyList_New(2);
    PyObject *x = PyInt_FromLong($1.getX());
    PyObject *y = PyInt_FromLong($1.getY());
    PyList_SetItem(list, 0, x);
    PyList_SetItem(list, 1, y);
    $result = list;
}

// Wrapper for dbSet, dbVector...etc
%define WRAP_DB_CONTAINER(T)
%typemap(out) dbSet< T >, dbVector< T > {
    PyObject *list = PyList_New($1.size());
    swig_type_info *tf = SWIG_TypeQuery("T" "*");
    int pos = 0;
    for (dbSet< T >::iterator itr = $1.begin(); itr != $1.end(); ++itr, ++pos)
    {
        PyObject *obj = SWIG_NewInstanceObj(*itr, tf, 0);
        PyList_SetItem(list, pos, obj);
    }
    $result = list;
}

%typemap(out) std::vector< T > {
    PyObject *list = PyList_New($1.size());
    std::vector<T>& v = *&($1);
    for (unsigned int i=0; i<$1.size(); i++) {
        T* ptr = new T(v[i]);
        PyList_SetItem(list, i, SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
    $result = list;
}
%typemap(out) std::vector< T* > {
    PyObject *list = PyList_New($1.size());
    std::vector<T*>& v = *&($1);
    for (unsigned int i = 0; i < $1.size(); i++) {
        T* ptr = v[i];
        PyList_SetItem(list, i, SWIG_NewInstanceObj(ptr, $descriptor(T *), 0));
    }
    $result = list;
}

%typemap(out) std::pair< int, int > {
    PyObject *list = PyList_New(2);
    PyList_SetItem(list, 0, PyInt_FromLong((long)$1.first));
    PyList_SetItem(list, 1, PyInt_FromLong((long)$1.second));
    $result = list;
}

%typemap(out) uint64_t {
    $result = PyLong_FromUnsignedLongLong($1);
}

%typemap(out) std::optional<uint8_t> {
    if ($1.has_value()) {
        $result = PyInt_FromLong((long)$1.value());
    } else {
        Py_INCREF(Py_None);
        $result = Py_None;
    }
}

%typemap(out) std::vector< std::pair< T*, int > > {
    PyObject *list = PyList_New($1.size());
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(2);
        std::pair< T*, int > p = ((($1_type &)$1)[i]);
        T* ptr1 = p.first;
        int num = p.second;
        PyObject *obj = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyList_SetItem(sub_list, 0, obj);
        PyList_SetItem(sub_list, 1, PyInt_FromLong((long)num));
        PyList_SetItem(list, i, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::tuple< T*, T*, int > > {
    PyObject *list = PyList_New($1.size());
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(3);
        std::tuple< T*, T*, int > p = ((($1_type &)$1)[i]);
        T* ptr1 = std::get<0>(p);
        T* ptr2 = std::get<1>(p);
        int num = std::get<2>(p);
        PyObject *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyObject *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(T *), 0);
        PyList_SetItem(sub_list, 0, obj1);
        PyList_SetItem(sub_list, 1, obj2);
        PyList_SetItem(sub_list, 2, PyInt_FromLong((long)num));
        PyList_SetItem(list, i, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::tuple< T*, int, int, int > > {
    PyObject *list = PyList_New($1.size());
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(4);
        std::tuple< T*, int, int, int > p = ((($1_type &)$1)[i]);
        T* ptr = std::get<0>(p);
        int num1 = std::get<1>(p);
        int num2 = std::get<2>(p);
        int num3 = std::get<3>(p);
        PyObject *obj = SWIG_NewInstanceObj(ptr, $descriptor(T *), 0);
        PyList_SetItem(sub_list, 0, obj);
        PyList_SetItem(sub_list, 1, PyInt_FromLong((long)num1));
        PyList_SetItem(sub_list, 2, PyInt_FromLong((long)num2));
        PyList_SetItem(sub_list, 3, PyInt_FromLong((long)num3));
        PyList_SetItem(list, i, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::pair< T*, T* > > {
    PyObject *list = PyList_New($1.size());
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(2);
        std::pair< T*, T* > p = ((($1_type &)$1)[i]);
        T* ptr1 = p.first;
        T* ptr2 = p.second;
        PyObject *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyObject *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(T *), 0);
        PyList_SetItem(sub_list, 0, obj1);
        PyList_SetItem(sub_list, 1, obj2);
        PyList_SetItem(list, i, sub_list);
    }
    $result = list;
}

%typemap(out) std::vector< std::pair< T*, odb::Rect > > {
    PyObject *list = PyList_New($1.size());
    for (unsigned int i = 0; i < $1.size(); i++) {
        PyObject *sub_list = PyList_New(2);
        std::pair< T*, odb::Rect > p = $1.at(i);
        T* ptr1 = p.first;
        odb::Rect* ptr2 = new odb::Rect(p.second);
        PyObject *obj1 = SWIG_NewInstanceObj(ptr1, $descriptor(T *), 0);
        PyObject *obj2 = SWIG_NewInstanceObj(ptr2, $descriptor(odb::Rect *), 0);
        PyList_SetItem(sub_list, 0, obj1);
        PyList_SetItem(sub_list, 1, obj2);
        PyList_SetItem(list, i, sub_list);
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

%typemap(in, numinputs=0) std::vector<std::pair<double, odb::dbTechLayer*>> &OUTPUT (std::vector<std::pair<double, odb::dbTechLayer*>> temp) {
   $1 = new std::vector<std::pair<double, dbTechLayer*>>(temp);
}

%typemap(argout) std::vector<odb::dbShape> &OUTPUT {
  swig_type_info *tf = SWIG_TypeQuery("odb::dbShape" "*");
  for(std::vector<odb::dbShape>::iterator it = $1->begin(); it != $1->end(); it++) {
    PyObject *o = SWIG_NewInstanceObj(&(*it), tf, 0);
    $result = SWIG_Python_AppendOutput($result, o);
  }
}

%typemap(argout) std::vector<std::pair<double, odb::dbTechLayer*>> &OUTPUT {
  $result = PyList_New(0);
  swig_type_info *tf = SWIG_TypeQuery("odb::dbTechLayer" "*");
  for(auto it = $1->begin(); it != $1->end(); it++) {
    auto value = it->first;
    auto layer = it->second;
    PyObject *layer_swig = SWIG_NewInstanceObj(layer, tf, 0);
    PyObject *tuple = PyTuple_Pack(2, PyFloat_FromDouble(value), layer_swig);
    $result = SWIG_Python_AppendOutput($result, tuple);
  }
}

%typemap(argout) std::vector<int> &OUTPUT {
  for(auto it = $1->begin(); it != $1->end(); it++) {
    PyObject *obj = PyInt_FromLong((long)*it);
    $result = SWIG_Python_AppendOutput($result, obj);
  }
}

%apply std::vector<odb::dbShape> &OUTPUT { std::vector<odb::dbShape> & shapes };
%apply std::vector<std::pair<double, odb::dbTechLayer*>> &OUTPUT { std::vector<std::pair<double, odb::dbTechLayer*>> & data };

%include containers.i
