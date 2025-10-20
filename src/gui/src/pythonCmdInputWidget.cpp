///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// Clang-tidy doesn't understand that Python.h is a super-header and you should
// not include sub-headers.
// NOLINTBEGIN(misc-include-cleaner)

// clang-format off
// Python.h must come first to avoid conflict with Qt
#define PY_SSIZE_T_CLEAN
#include "Python.h"
// clang-format on

#include "pythonCmdInputWidget.h"

#include <QCoreApplication>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

namespace gui {

// https://stackoverflow.com/questions/4307187/how-to-catch-python-stdout-in-c-code/8335297#8335297
namespace emb {

using stdout_write_type = std::function<void(std::string)>;

struct Stdout
{
  PyObject_HEAD stdout_write_type write;
};

static PyObject* Stdout_write(PyObject* self, PyObject* args)
{
  std::size_t written(0);
  Stdout* selfimpl = reinterpret_cast<Stdout*>(self);
  if (selfimpl->write) {
    char* data;
    if (!PyArg_ParseTuple(args, "s", &data)) {
      return nullptr;
    }

    std::string str(data);
    selfimpl->write(str);
    written = str.size();
  }
  return PyLong_FromSize_t(written);
}

static PyObject* Stdout_flush(PyObject* self, PyObject* args)
{
  // no-op
  return Py_BuildValue("");
}

static PyMethodDef Stdout_methods[] = {
    {"write", Stdout_write, METH_VARARGS, "sys.stdout.write"},
    {"flush", Stdout_flush, METH_VARARGS, "sys.stdout.flush"},
    {nullptr, nullptr, 0, nullptr}  // sentinel
};

static PyTypeObject StdoutType = {
    PyVarObject_HEAD_INIT(0, 0) "emb.StdoutType", /* tp_name */
    sizeof(Stdout),                               /* tp_basicsize */
    0,                                            /* tp_itemsize */
    nullptr,                                      /* tp_dealloc */
    0,                                            /* tp_print */
    nullptr,                                      /* tp_getattr */
    nullptr,                                      /* tp_setattr */
    nullptr,                                      /* tp_reserved */
    nullptr,                                      /* tp_repr */
    nullptr,                                      /* tp_as_number */
    nullptr,                                      /* tp_as_sequence */
    nullptr,                                      /* tp_as_mapping */
    nullptr,                                      /* tp_hash  */
    nullptr,                                      /* tp_call */
    nullptr,                                      /* tp_str */
    nullptr,                                      /* tp_getattro */
    nullptr,                                      /* tp_setattro */
    nullptr,                                      /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                           /* tp_flags */
    "emb.Stdout objects",                         /* tp_doc */
    nullptr,                                      /* tp_traverse */
    nullptr,                                      /* tp_clear */
    nullptr,                                      /* tp_richcompare */
    0,                                            /* tp_weaklistoffset */
    nullptr,                                      /* tp_iter */
    nullptr,                                      /* tp_iternext */
    Stdout_methods,                               /* tp_methods */
    nullptr,                                      /* tp_members */
    nullptr,                                      /* tp_getset */
    nullptr,                                      /* tp_base */
    nullptr,                                      /* tp_dict */
    nullptr,                                      /* tp_descr_get */
    nullptr,                                      /* tp_descr_set */
    0,                                            /* tp_dictoffset */
    nullptr,                                      /* tp_init */
    nullptr,                                      /* tp_alloc */
    nullptr,                                      /* tp_new */
};

static PyModuleDef embmodule = {
    PyModuleDef_HEAD_INIT,
    "emb",
    nullptr,
    -1,
    nullptr,
};

// Internal state
struct StreamPair
{
  PyObject* saved = nullptr;
  PyObject* current = nullptr;
};
static std::map<std::string, StreamPair> streams;

PyMODINIT_FUNC PyInit_emb(void)
{
  StdoutType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&StdoutType) < 0) {
    return nullptr;
  }

  PyObject* m = PyModule_Create(&embmodule);
  if (m) {
    Py_INCREF(&StdoutType);
    PyModule_AddObject(m, "Stdout", reinterpret_cast<PyObject*>(&StdoutType));
  }
  return m;
}

static void set_stream(const std::string& stream, stdout_write_type write)
{
  auto& stream_pair = streams[stream];
  if (stream_pair.current == nullptr) {
    stream_pair.saved = PySys_GetObject(stream.c_str());
    stream_pair.current = StdoutType.tp_new(&StdoutType, nullptr, nullptr);
  }

  Stdout* impl = reinterpret_cast<Stdout*>(stream_pair.current);
  impl->write = std::move(write);
  PySys_SetObject(stream.c_str(), stream_pair.current);
}

static void reset_streams()
{
  for (auto& [stream, stream_pair] : streams) {
    if (stream_pair.saved != nullptr) {
      PySys_SetObject(stream.c_str(), stream_pair.saved);
    }
    Py_XDECREF(stream_pair.current);
    stream_pair.current = nullptr;
  }
}

}  // namespace emb

static PythonCmdInputWidget* widget = nullptr;

class PythonRef : public std::unique_ptr<PyObject>
{
 public:
  PythonRef() : std::unique_ptr<PyObject>(nullptr) {}
  PythonRef(PyObject* object) : std::unique_ptr<PyObject>(object)
  {
    Py_XINCREF(get());
  }
  ~PythonRef()
  {
    PyObject* object = release();
    if (object != nullptr) {
      Py_XDECREF(object);
    }
  }
};

PythonCmdInputWidget::PythonCmdInputWidget(QWidget* parent)
    : CmdInputWidget("Python", parent)
{
  setObjectName("python_scripting");  // for settings

  widget = this;
}

PythonCmdInputWidget::~PythonCmdInputWidget()
{
  emb::reset_streams();
  widget = nullptr;
}

void PythonCmdInputWidget::executeCommand(const QString& cmd,
                                          bool echo,
                                          bool silent)
{
  if (cmd.isEmpty()) {
    return;
  }

  if (echo) {
    // Show the command that we executed
    emit addCommandToOutput(cmd);
  }

  emit commandAboutToExecute();
  QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

  const std::string command = cmd.toStdString();

  PythonRef code(Py_CompileString(command.c_str(), "(eval)", Py_single_input));
  if (code == nullptr) {
    emit commandFinishedExecuting(false);
    return;
  }

  PythonRef main_module(PyImport_AddModule("__main__"));
  PythonRef dict(PyModule_GetDict(main_module.get()));

  PythonRef ret(PyEval_EvalCode(code.get(), dict.get(), dict.get()));

  bool success = false;
  if (ret != nullptr) {
    success = true;
    if (ret.get() != Py_None) {
      PythonRef ret_str(PyObject_Str(ret.get()));
      emit addResultToOutput(PyUnicode_AsUTF8(ret_str.get()), true);
    }
  } else {
    if (PyErr_Occurred() != nullptr) {
      PyErr_Print();
    } else {
      success = true;
    }
  }

  if (success) {
    clear();
  }
  emit commandFinishedExecuting(success);

  PyErr_Clear();
}

void PythonCmdInputWidget::keyPressEvent(QKeyEvent* e)
{
  if (handleHistoryKeyPress(e)) {
    return;
  }

  if (handleEnterKeyPress(e)) {
    return;
  }

  CmdInputWidget::keyPressEvent(e);
}

bool PythonCmdInputWidget::isCommandComplete(const std::string& cmd) const
{
  if (cmd.empty()) {
    return false;
  }

  PythonRef code(Py_CompileString(cmd.c_str(), "(eval)", Py_single_input));

  return code != nullptr;
}

void PythonCmdInputWidget::exitHandler()
{
  Gui::get()->clearContinueAfterClose();
  if (widget != nullptr) {
    emit widget->exiting();
  }
}

void PythonCmdInputWidget::init()
{
  PyImport_AppendInittab("emb", emb::PyInit_emb);
  ord::pyAppInit(false);

  PyImport_ImportModule("emb");

  Py_AtExit(&exitHandler);

  emb::stdout_write_type stdout_stream = [](std::string s) {
    auto* openroad = ord::OpenRoad::openRoad();
    if (openroad == nullptr) {
      return;
    }
    auto* logger = openroad->getLogger();
    if (logger == nullptr) {
      return;
    }
    s.erase(s.find_last_not_of("\r\n") + 1);
    s.erase(s.find_last_not_of('\n') + 1);
    if (s.empty()) {
      return;
    }
    logger->report("{}", s);
  };
  emb::set_stream("stdout", stdout_stream);
  emb::stdout_write_type stderr_stream = [this](std::string s) {
    auto* openroad = ord::OpenRoad::openRoad();
    if (openroad == nullptr) {
      return;
    }
    auto* logger = openroad->getLogger();
    if (logger == nullptr) {
      return;
    }
    s.erase(s.find_last_not_of("\r\n") + 1);
    s.erase(s.find_last_not_of('\n') + 1);
    if (s.empty()) {
      return;
    }
    emit addResultToOutput(QString::fromStdString(s), false);
  };
  emb::set_stream("stderr", stderr_stream);
}

}  // namespace gui

// NOLINTEND(misc-include-cleaner)
