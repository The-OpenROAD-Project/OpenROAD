// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

%rename(_print) print;

%define WRAP_OBJECT_OPERATOR(T)
%rename(incr) T::operator++();
%rename(incr_int) T::operator++(int);
%enddef

WRAP_OBJECT_OPERATOR(odb::dbSetIterator<dbBlock>)
WRAP_OBJECT_OPERATOR(odb::dbSetIterator<dbCCSeg>)

%define WRAP_OBJECT_STREAM(T)
%rename(IStream) operator>>(dbIStream &, T &);
%rename(OStream) operator<<(dbOStream &, const T &);
%rename(equal)  odb::T::operator=(const T &);

%enddef
WRAP_OBJECT_STREAM(Point)
WRAP_OBJECT_STREAM(Rect)
WRAP_OBJECT_STREAM(dbTransform)
WRAP_OBJECT_STREAM(_dbViaParams)

// Python equality and hashing for all odb database objects.
//
// We use %pythoncode rather than %extend to avoid SWIG Warning 303 (fired in
// SWIG >= 4.3 when %extend targets a class not directly %include-d).
// After %include "odb/dbObject.h", all concrete db* classes inherit
// getId()/getObjectType() which provide a stable, portable hash key.
%pythoncode %{
def _odb_install_equality():
    import sys

    def _eq(self, other):
        if type(self) is not type(other):
            return NotImplemented
        return self.this == other.this

    def _ne(self, other):
        result = _eq(self, other)
        if result is NotImplemented:
            return result
        return not result

    def _hash(self):
        # (objectType, id) uniquely identifies each odb object and is
        # consistent with pointer-based equality.
        return hash((self.getObjectType(), self.getId()))

    module = sys.modules[__name__]
    for cls in vars(module).values():
        # Only patch concrete dbObject-derived classes; dbObject itself is
        # excluded as it cannot be instantiated directly.
        if (isinstance(cls, type)
                and cls is not module.dbObject
                and issubclass(cls, module.dbObject)):
            cls.__eq__ = _eq
            cls.__ne__ = _ne
            cls.__hash__ = _hash

_odb_install_equality()
del _odb_install_equality
%}

