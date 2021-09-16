///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include "tclCmdInputWidget.h"

#include <regex>

#include <QAbstractItemView>
#include <QMimeData>
#include <QScrollBar>
#include <QTextStream>

namespace gui {

TclCmdInputWidget::TclCmdInputWidget(QWidget* parent) :
    QPlainTextEdit(parent), line_height_(0), document_margins_(0),
    max_height_(QWIDGETSIZE_MAX), interp_(nullptr),
    context_menu_(nullptr), enable_highlighting_(nullptr),
    enable_completion_(nullptr), highlighter_(nullptr),
    completer_(nullptr), completer_options_(nullptr),
    completer_commands_(nullptr), completer_start_of_command_(nullptr),
    completer_end_of_command_(nullptr)
{
  setObjectName("tcl_scripting");  // for settings
  setPlaceholderText("TCL commands");
  setAcceptDrops(true);

  determineLineHeight();

  // add option to default context menu to enable or disable syntax highlighting
  context_menu_.reset(createStandardContextMenu());
  context_menu_->addSeparator();
  enable_highlighting_ = std::make_unique<QAction>("Syntax highlighting", this);
  enable_highlighting_->setCheckable(true);
  enable_highlighting_->setChecked(true);
  context_menu_->addAction(enable_highlighting_.get());
  connect(enable_highlighting_.get(), SIGNAL(triggered()), this, SLOT(updateHighlighting()));
  enable_completion_ = std::make_unique<QAction>("Command completion", this);
  enable_completion_->setCheckable(true);
  enable_completion_->setChecked(true);
  context_menu_->addAction(enable_completion_.get());
  connect(enable_completion_.get(), SIGNAL(triggered()), this, SLOT(updateCompletion()));

  // precompute size for updating text box size
  document_margins_ = 2 * (document()->documentMargin() + 3);

  connect(this, SIGNAL(textChanged()), this, SLOT(updateSize()));
  updateSize();
}

TclCmdInputWidget::~TclCmdInputWidget()
{
}

void TclCmdInputWidget::setFont(const QFont& font)
{
  QPlainTextEdit::setFont(font);

  determineLineHeight();
}

void TclCmdInputWidget::determineLineHeight()
{
  QFontMetrics font_metrics = fontMetrics();
  line_height_ = font_metrics.lineSpacing();

  double tab_indent_width = 2 * font_metrics.averageCharWidth();
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
  // setTabStopWidth deprecated in 5.10
  setTabStopDistance(tab_indent_width);
#else
  setTabStopWidth(tab_indent_width);
#endif
}

void TclCmdInputWidget::keyPressEvent(QKeyEvent* e)
{
  const int key = e->key();

  // handle completer if it is visible
  if (completer_ != nullptr && completer_->popup()->isVisible()) {
      // The following keys are forwarded by the completer to the widget
    if (key == Qt::Key_Enter ||
        key == Qt::Key_Return ||
        key == Qt::Key_Escape ||
        key == Qt::Key_Tab ||
        key == Qt::Key_Backtab) {
      // let the completer do default behavior
      e->ignore();
      return;
    }
  }

  // handle regular command typing
  bool has_control = e->modifiers().testFlag(Qt::ControlModifier);
  if (key == Qt::Key_Enter || key == Qt::Key_Return) {
    // Handle enter
    if (has_control) {
      // don't execute just insert the newline
      // does not get inserted by Qt, so manually inserting
      insertPlainText("\n");
      return;
    } else {
      // Check if command complete and attempt to execute, otherwise do nothing
      if (isCommandComplete(toPlainText().simplified().toStdString())) {
        // execute command
        emit completeCommand(text());
        return;
      }
    }
  } else if (key == Qt::Key_Down) {
    // Handle down through history
    // control+down immediate
    if ((!textCursor().hasSelection() && !textCursor().movePosition(QTextCursor::Down))
        || has_control) {
      emit historyGoForward();
      return;
    }
  } else if (key == Qt::Key_Up) {
    // Handle up through history
    // control+up immediate
    if ((!textCursor().hasSelection() && !textCursor().movePosition(QTextCursor::Up))
        || has_control) {
      emit historyGoBack();
      return;
    }
  }

  // handle completer
  if (completer_ == nullptr) {
    // no completer
    QPlainTextEdit::keyPressEvent(e);
  }
  else {
    bool is_completer_shortcut = has_control && key == Qt::Key_E; // CTRL+E
    if (!is_completer_shortcut) {
      // forward keypress if it is not the completer shortcut
      QPlainTextEdit::keyPressEvent(e);
    }

    if (e->text().isEmpty()) {
      // key press doesn't contain anything
      return;
    }

    QString completion_prefix = wordUnderCursor();
    const swig_class* swig_type = swigBeforeCursor();
    bool is_variable = completion_prefix.startsWith("$");
    bool is_argument = completion_prefix.startsWith("-");
    bool is_swig     = swig_type != nullptr;

    bool show_popup = is_completer_shortcut; // shortcut enabled it
    show_popup |= completion_prefix.length() >= completer_mimimum_length_; // minimum length
    show_popup |= is_argument; // is argument
    show_popup |= is_variable; // is variable
    show_popup |= is_swig;     // is swig argument
    if (!show_popup) {
      completer_->popup()->hide();
    }
    else {
      if (completion_prefix != completer_->completionPrefix()) {
        // prefix changed
        completer_->setCompletionPrefix(completion_prefix);
        completer_->popup()->setCurrentIndex(completer_->completionModel()->index(0, 0));

        if (is_variable) {
          // complete with variables
          setCompleterVariables();
        }
        else {
          TclCmdUserData* block_data = static_cast<TclCmdUserData*>(textCursor().block().userData());
          if (is_argument && block_data != nullptr) {
            // get command arguments
            setCompleterArguments(block_data->commands);
          }
          else {
            if (is_swig) {
              // previous item was of swig type, so complete with arguments for that type
              setCompleterSWIG(swig_type);
            }
            else {
              // default to just commands
              setCompleterCommands();
            }
          }
        }
      }

      // check if only one thing matches and its a complete match, then no need to show
      if (completer_->completionCount() == 1 &&
          completer_->currentCompletion() == completer_->completionPrefix()) {
        completer_->popup()->hide();
      }
      else {
        QRect cr = cursorRect();
        cr.setWidth(completer_->popup()->sizeHintForColumn(0)
            + completer_->popup()->verticalScrollBar()->sizeHint().width());
        completer_->complete(cr);
      }
    }
  }
}

void TclCmdInputWidget::keyReleaseEvent(QKeyEvent* e)
{
  int key = e->key();
  if (key == Qt::Key_Enter || key == Qt::Key_Return) {
    // announce that text might have changed due to enter key
    // to ensure resizing is processed
    emit textChanged();
  }

  QPlainTextEdit::keyReleaseEvent(e);
}

bool TclCmdInputWidget::isCommandComplete(const std::string& cmd)
{
  if (cmd.empty()) {
    return false;
  }

  // check if last character is \, then assume it is multiline
  if (cmd.at(cmd.size() - 1) == '\\') {
    return false;
  }

  return Tcl_CommandComplete(cmd.c_str());
}

// Slot to announce command executed
void TclCmdInputWidget::commandExecuted(int return_code)
{
  if (return_code == TCL_OK) {
    clear();
  }
}

// Update the size of the widget to match text
void TclCmdInputWidget::updateSize()
{
  int height = document()->size().toSize().height();
  if (height < 1) {
    height = 1; // ensure minimum is 1 line
  }

  // in px
  int desired_height = height * line_height_ + document_margins_;

  if (desired_height > max_height_) {
    desired_height = max_height_; // ensure maximum from Qt suggestion
  }

  setFixedHeight(desired_height);

  ensureCursorVisible();
}

// Handle dragged and drop script files
void TclCmdInputWidget::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->mimeData()->text().startsWith("file://")) {
    event->accept();
  }
}

void TclCmdInputWidget::dropEvent(QDropEvent* event)
{
  if (event->mimeData()->text().startsWith("file://")) {
    event->accept();

    // replace the content in the text area with the file
    QFile drop_file(event->mimeData()->text().remove(0, 7).simplified());
    if (drop_file.open(QIODevice::ReadOnly)) {
      QTextStream file_data(&drop_file);
      setText(file_data.readAll());
      drop_file.close();
    }
  }
}

// setup syntax highlighter
void TclCmdInputWidget::init(Tcl_Interp* interp)
{
  interp_ = interp;

  initOpenRoadCommands();

  const char* start_of_command = "(?:^|(?<=(?:\\s|\\[|\\{)))";
  const char* end_of_command = "(?:$|(?=(?:\\s|\\]|\\})))";

  // setup highlighter
  highlighter_ = std::make_unique<TclCmdHighlighter>(document(),
                                                     commands_,
                                                     start_of_command,
                                                     end_of_command);
  updateHighlighting();

  // setup command completer
  completer_start_of_command_ = std::make_unique<QRegularExpression>(start_of_command);
  completer_end_of_command_ = std::make_unique<QRegularExpression>(end_of_command);

  // initialize the commands completion
  completer_commands_ = std::make_unique<QStringList>();
  // fill with commands
  QStringList namespaces;
  for (const auto& [cmd, or_cmd, args] : commands_) {
    if (or_cmd) {
      completer_commands_->append(cmd.c_str());
    }
    else {
      namespaces.append(cmd.c_str());
    }
  }
  completer_commands_->sort();
  namespaces.sort();
  // add namespaces second
  *completer_commands_ << namespaces;

  updateCompletion();
}

// replicate QLineEdit function
QString TclCmdInputWidget::text()
{
  return toPlainText();
}

// replicate QLineEdit function
void TclCmdInputWidget::setText(const QString& text)
{
  setPlainText(text);
  emit textChanged();
}

void TclCmdInputWidget::setMaximumHeight(int height)
{
  int min_height = line_height_ + document_margins_; // atleast one line
  if (height < min_height) {
    height = min_height;
  }

  // save max height, since it's overwritten by setFixedHeight
  max_height_ = height;
  QPlainTextEdit::setMaximumHeight(height);

  updateSize();
}

void TclCmdInputWidget::contextMenuEvent(QContextMenuEvent *event)
{
    context_menu_->exec(event->globalPos());
}

void TclCmdInputWidget::updateHighlighting()
{
  if (highlighter_ != nullptr) {
    if (enable_highlighting_->isChecked()) {
      highlighter_->setDocument(document());
    }
    else {
      highlighter_->setDocument(nullptr);
    }
  }
}

void TclCmdInputWidget::updateCompletion()
{
  if (enable_completion_->isChecked()) {
    completer_ = std::make_unique<QCompleter>(this);
    completer_options_ = std::make_unique<QStringListModel>(completer_.get());
    completer_->setModel(completer_options_.get());
    completer_->setModelSorting(QCompleter::UnsortedModel);
    completer_->setCaseSensitivity(Qt::CaseSensitive);
    completer_->setWrapAround(false);

    completer_->setWidget(this);
    completer_->setCompletionMode(QCompleter::PopupCompletion);

    setCompleterCommands();

    connect(completer_.get(), QOverload<const QString&>::of(&QCompleter::activated),
            this, &TclCmdInputWidget::insertCompletion);
  }
  else {
    if (completer_ != nullptr) {
      completer_->disconnect(this);

      completer_options_ = nullptr;
      completer_ = nullptr;
    }
  }
}

void TclCmdInputWidget::readSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  enable_highlighting_->setChecked(settings->value(enable_highlighting_keyword_, true).toBool());
  enable_completion_->setChecked(settings->value(enable_completion_keyword_, true).toBool());
  settings->endGroup();
}

void TclCmdInputWidget::writeSettings(QSettings* settings)
{
  settings->beginGroup(objectName());
  settings->setValue(enable_highlighting_keyword_, enable_highlighting_->isChecked());
  settings->setValue(enable_completion_keyword_, enable_completion_->isChecked());
  settings->endGroup();
}

void TclCmdInputWidget::parseOpenRoadArguments(const char* or_args,
                                               std::set<std::string>& args)
{
  // look for -????
  std::regex arg_matcher("\\-[a-zA-Z0-9_]+");
  std::string local_or_args = or_args;

  std::regex_iterator<std::string::iterator> args_it(local_or_args.begin(),
                                                     local_or_args.end(),
                                                     arg_matcher);
  std::regex_iterator<std::string::iterator> args_end;
  while (args_it != args_end) {
    args.insert(args_it->str());
    ++args_it;
  }
}


void TclCmdInputWidget::initOpenRoadCommands()
{
  commands_.clear();

  // get registered commands
  if (Tcl_Eval(interp_, "array names sta::cmd_args") == TCL_OK) {
    Tcl_Obj* cmd_names = Tcl_GetObjResult(interp_);
    int cmd_size;
    Tcl_Obj** cmds_objs;
    if (Tcl_ListObjGetElements(interp_, cmd_names, &cmd_size, &cmds_objs) == TCL_OK) {
      for (int i = 0; i < cmd_size; i++) {
        commands_.push_back({Tcl_GetString(cmds_objs[i]), true, {}});
      }
    }
  }

  // create highlighting for commands and associated arguments
  for (auto& [cmd, ns, args] : commands_) {
    parseOpenRoadArguments(Tcl_GetVar2(interp_,
                                       "sta::cmd_args",
                                       cmd.c_str(),
                                       TCL_LEAVE_ERR_MSG), args);
  }

  // get namespaces
  std::set<std::string> namespaces;
  collectNamespaces(namespaces);

  // get commands from namespaces
  for (const std::string& ns : namespaces) {
    std::string info = "info commands " + ns + "::*";
    if (Tcl_Eval(interp_, info.c_str()) == TCL_OK) {
      Tcl_Obj* cmd_names = Tcl_GetObjResult(interp_);
      int cmd_size;
      Tcl_Obj** cmds_objs;
      if (Tcl_ListObjGetElements(interp_, cmd_names, &cmd_size, &cmds_objs) == TCL_OK) {
        for (int i = 0; i < cmd_size; i++) {
          std::string cmd = Tcl_GetString(cmds_objs[i]);
          // remove leading ::
          commands_.push_back({cmd.substr(2), false, {}});
        }
      }
    }
  }

  // get swig arguments
  collectSWIGArguments();
}

void TclCmdInputWidget::collectNamespaces(std::set<std::string>& namespaces)
{
  if (Tcl_Eval(interp_, "namespace children") == TCL_OK) {
    Tcl_Obj* ns = Tcl_GetObjResult(interp_);
    int namespace_size;
    Tcl_Obj** namespace_objs;
    if (Tcl_ListObjGetElements(interp_, ns, &namespace_size, &namespace_objs) == TCL_OK) {
      for (int i = 0; i < namespace_size; i++) {
        namespaces.insert(Tcl_GetString(namespace_objs[i]));
      }
    }
  }
}

void TclCmdInputWidget::collectSWIGArguments()
{
  swig_arguments_.clear();

  swig_module_info* module = SWIG_Tcl_GetModule(interp_);
  if (module == nullptr) {
    return;
  }

  // loop through circularly linked list of modules
  swig_module_info* first_module = module;
  do {
    // loop through types in modules to find classes
    for (int i = 0; i < module->size; i++) {
      swig_class* cls = static_cast<swig_class*>(module->types[i]->clientdata);

      if (cls != nullptr) {
        std::unique_ptr<QStringList> methods = std::make_unique<QStringList>();

        // loop through methods for each class to collect method names
        for (swig_method* meth = cls->methods; meth != nullptr && meth->name; ++meth) {
          methods->append(meth->name);
        }

        if (!methods->isEmpty()) {
          methods->sort();
          swig_arguments_[cls] = std::move(methods);
        }
      }
    }

    module = module->next;
  } while (first_module != module);
}

void TclCmdInputWidget::setCompleterCommands()
{
  bool add_colons = completer_->completionPrefix().startsWith(":");

  QStringList options;
  // fill with commands
  for (const QString& cmd : *completer_commands_) {
    if (add_colons) {
      options.append("::" + cmd);
    }
    else {
      options.append(cmd);
    }
  }

  completer_options_->setStringList(options);
}

void TclCmdInputWidget::setCompleterSWIG(const swig_class* type)
{
  completer_options_->setStringList(*swig_arguments_[type]);
}

void TclCmdInputWidget::setCompleterArguments(const std::set<int>& cmds)
{
  // fill with arguments
  QStringList options;
  for (const int cmd_id : cmds) {
    for (const auto& args : commands_[cmd_id].arguments) {
      options.append(args.c_str());
    }
  }
  options.sort();

  completer_options_->setStringList(options);
}

void TclCmdInputWidget::setCompleterVariables()
{
  const std::string prefix = completer_->completionPrefix().mid(1).toStdString();

  // fill with arguments
  QStringList variables;

  bool starts_with_colon = !prefix.empty() && prefix[0] == ':';

  std::string tcl_cmd = "info vars " + prefix;
  // check if prefix ends with ":" and append ":" to complete namespace
  if (!prefix.empty() && prefix.back() == ':' && // check if ends with :
      (prefix.length() == 1 || prefix[prefix.length()-2] != ':')  ) { // check if is does not end with ::
    tcl_cmd += ":";
  }
  tcl_cmd += "*";

  if (Tcl_Eval(interp_, tcl_cmd.c_str()) == TCL_OK) {
    Tcl_Obj* var_names = Tcl_GetObjResult(interp_);
    int var_size;
    Tcl_Obj** vars_objs;
    if (Tcl_ListObjGetElements(interp_, var_names, &var_size, &vars_objs) == TCL_OK) {
      for (int i = 0; i < var_size; i++) {
        std::string var = Tcl_GetString(vars_objs[i]);

        // remove leading colons if they are not used.
        if (!starts_with_colon && var[0] == ':') {
          var = var.substr(2);
        }

        variables.append(("$" + var).c_str());
      }
    }
  }

  // get namespaces to add at the end of list
  std::set<std::string> namespaces;
  collectNamespaces(namespaces);
  for (const std::string& ns : namespaces) {
    std::string name = ns;
    if (!starts_with_colon) {
      name = name.substr(2);
    }
    variables.append(("$" + name).c_str());
  }

  completer_options_->setStringList(variables);
}

void TclCmdInputWidget::insertCompletion(const QString& text)
{
  if (completer_ == nullptr) {
    return;
  }

  QTextCursor cursor = textCursor();
  int extra_chars = text.length() - completer_->completionPrefix().length();
  cursor.movePosition(QTextCursor::Left);
  cursor.movePosition(QTextCursor::EndOfWord);

  QString insert = text.right(extra_chars);
  if (completer_->completionPrefix().isEmpty() && !cursor.atStart()) {
    // prefix was empty and not at start of line, need space
    insert = " " + insert;
  }
  cursor.insertText(insert);
  setTextCursor(cursor);
}

const QString TclCmdInputWidget::wordUnderCursor()
{
  // get line
  QTextCursor cursor = textCursor();
  int cursor_position = cursor.positionInBlock();

  cursor.select(QTextCursor::LineUnderCursor);
  const QString line = cursor.selectedText();

  int start_of_word = line.lastIndexOf(*completer_start_of_command_.get(), cursor_position);
  if (start_of_word == -1) {
    start_of_word = 0;
  }
  int end_of_word   = line.indexOf(*completer_end_of_command_.get(), cursor_position);
  if (end_of_word == -1) {
    end_of_word = line.length();
  }

  return line.mid(start_of_word, end_of_word-start_of_word);
}

const swig_class* TclCmdInputWidget::swigBeforeCursor()
{
  QTextCursor cursor = textCursor();
  int cursor_position = cursor.positionInBlock();

  cursor.select(QTextCursor::LineUnderCursor);
  const QString line = cursor.selectedText();

  int end_of_word  = line.lastIndexOf(*completer_end_of_command_.get(), cursor_position - 1);
  if (end_of_word == -1) {
    end_of_word = 0;
  }
  int start_of_word = line.lastIndexOf(*completer_start_of_command_.get(), end_of_word);
  if (start_of_word == -1) {
    start_of_word = 0;
  }

  const QString word = line.mid(start_of_word, end_of_word-start_of_word).trimmed();
  const QString remainder = line.right(line.length() - end_of_word).trimmed();

  if (!remainder.isEmpty()) {
    const std::regex word_regex("^\\w+");
    // check if remainder contains a non-word character, if yes, then it's not swig
    if (!std::regex_search(remainder.toStdString(), word_regex)) {
      return nullptr;
    }
  }

  std::string variable_content = word.toStdString();
  if (word.startsWith("$")) {
    // variable
    const char* var_content = Tcl_GetVar(interp_, variable_content.substr(1).c_str(), TCL_GLOBAL_ONLY);

    if (var_content == nullptr) {
      // invalid variable
      return nullptr;
    }
    variable_content = var_content;
  }

  Tcl_CmdInfo infoPtr;
  // find command information
  if (Tcl_GetCommandInfo(interp_, variable_content.c_str(), &infoPtr) != 0) {
    if (infoPtr.isNativeObjectProc == 1) {
      // set to one if created by Tcl_CreateObjCommand()
      swig_instance* inst = static_cast<swig_instance*>(infoPtr.objClientData);
      if (inst != nullptr && inst->classptr != nullptr) {
        // make sure cls is in the arguments
        if (swig_arguments_.count(inst->classptr) != 0) {
          return inst->classptr;
        }
      }
    }
  }

  return nullptr;
}

}  // namespace gui
