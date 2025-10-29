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
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

namespace gui {

using stdout_write_type = std::function<void(std::string)>;

struct StreamCatcher
{
  PyObject_HEAD;
  stdout_write_type write;
  std::string buffer;
  bool is_stderr;  // distinguish stdout vs stderr
};

// method implementation
static PyObject* StreamCatcher_write(PyObject* self, PyObject* args)
{
  const char* text = nullptr;
  if (!PyArg_ParseTuple(args, "s", &text)) {
    Py_RETURN_NONE;
  }

  if (text[0] == '\0') {
    Py_RETURN_NONE;
  }

  auto* catcher = reinterpret_cast<StreamCatcher*>(self);
  catcher->buffer += text;

  if (catcher->buffer.back() == '\n') {
    const size_t end = catcher->buffer.find_last_not_of("\r\n");
    catcher->buffer.erase(end + 1);
  } else {
    Py_RETURN_NONE;
  }

  catcher->write(catcher->buffer);
  catcher->buffer.clear();

  Py_RETURN_NONE;
}

static PyMethodDef StreamCatcher_methods[]
    = {{"write", (PyCFunction) StreamCatcher_write, METH_VARARGS, "Write text"},
       {nullptr, nullptr, 0, nullptr}};

static PyType_Slot StreamCatcher_slots[]
    = {{Py_tp_methods, StreamCatcher_methods},
       {Py_tp_doc, (void*) "C++ StreamCatcher for stdout/stderr"},
       {0, nullptr}};

static PyType_Spec StreamCatcher_spec = {"embed.StreamCatcher",
                                         sizeof(StreamCatcher),
                                         0,
                                         Py_TPFLAGS_DEFAULT,
                                         StreamCatcher_slots};

// Keep global references so they can be restored
static PyObject* original_stdout = nullptr;
static PyObject* original_stderr = nullptr;
static StreamCatcher* stdout_catcher = nullptr;
static StreamCatcher* stderr_catcher = nullptr;

static StreamCatcher* create_catcher(const bool is_stderr,
                                     stdout_write_type write)
{
  PyObject* type = PyType_FromSpec(&StreamCatcher_spec);
  if (!type) {
    return nullptr;
  }

  PyObject* obj = PyObject_CallObject(type, nullptr);
  Py_DECREF(type);
  if (!obj) {
    return nullptr;
  }

  auto* catcher = reinterpret_cast<StreamCatcher*>(obj);
  catcher->is_stderr = is_stderr;
  catcher->write = std::move(write);
  return catcher;
}

// Redirect sys.stdout and sys.stderr
bool redirect_python_output(stdout_write_type stdout_write,
                            stdout_write_type stderr_write)
{
  PyObject* sysmod = PyImport_ImportModule("sys");
  if (!sysmod) {
    return false;
  }

  // Save original streams
  original_stdout = PyObject_GetAttrString(sysmod, "stdout");
  original_stderr = PyObject_GetAttrString(sysmod, "stderr");

  // Create new catchers
  stdout_catcher = create_catcher(false, stdout_write);
  stderr_catcher = create_catcher(true, stderr_write);

  if (!stdout_catcher || !stderr_catcher) {
    Py_DECREF(sysmod);
    PyErr_Print();
    return false;
  }

  // Replace Python's sys.stdout/stderr
  PyObject_SetAttrString(
      sysmod, "stdout", reinterpret_cast<PyObject*>(stdout_catcher));
  PyObject_SetAttrString(
      sysmod, "stderr", reinterpret_cast<PyObject*>(stderr_catcher));

  Py_DECREF(sysmod);
  return true;
}

// Restore original sys.stdout/stderr
void restore_python_output()
{
  PyObject* sysmod = PyImport_ImportModule("sys");
  if (!sysmod) {
    return;
  }

  if (original_stdout) {
    PyObject_SetAttrString(sysmod, "stdout", original_stdout);
    Py_DECREF(original_stdout);
    original_stdout = nullptr;
  }
  if (original_stderr) {
    PyObject_SetAttrString(sysmod, "stderr", original_stderr);
    Py_DECREF(original_stderr);
    original_stderr = nullptr;
  }

  Py_XDECREF(stdout_catcher);
  Py_XDECREF(stderr_catcher);
  stdout_catcher = nullptr;
  stderr_catcher = nullptr;

  Py_DECREF(sysmod);
}

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
  ord::pyAppInit(false);

  auto* openroad = ord::OpenRoad::openRoad();
  if (openroad == nullptr) {
    return;
  }
  auto* logger = openroad->getLogger();
  if (logger == nullptr) {
    return;
  }

  stdout_write_type stdout_write
      = [logger](std::string s) { logger->report("{}", s); };
  stdout_write_type stderr_write = [this](std::string s) {
    emit addResultToOutput(QString::fromStdString(s), false);
  };

  if (!redirect_python_output(stdout_write, stderr_write)) {
    std::cerr << "Failed to redirect Python output" << std::endl;
    Py_Finalize();
    return;
  }

  Py_AtExit(&exitHandler);
}

}  // namespace gui

// NOLINTEND(misc-include-cleaner)
