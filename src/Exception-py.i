// Copyright (c) 2020, Parallax Software, Inc.
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

%{
%}

%exception {
  try { $function }
  catch (std::bad_alloc &) {
    fprintf(stderr, "Error: out of memory.");
    exit(0);
  }
  // This catches std::runtime_error (utl::error) and sta::Exception.
  catch (std::exception &excp) {
    PyErr_SetString(PyExc_RuntimeError, excp.what());
    SWIG_fail;
  }
  catch (...) {
    PyErr_SetString(PyExc_Exception, "Unknown exception");
    SWIG_fail;
  }      
}
