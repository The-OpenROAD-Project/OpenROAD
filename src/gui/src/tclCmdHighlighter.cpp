///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2019, OpenROAD
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

#include "tclCmdHighlighter.h"

#include <regex>

namespace gui {

TclCmdHighlighter::TclCmdHighlighter(QTextDocument* parent, Tcl_Interp* interp) :
    QSyntaxHighlighter(parent)
{
  initFormats();

  init(interp);
}

TclCmdHighlighter::~TclCmdHighlighter()
{
}

void TclCmdHighlighter::initFormats()
{
  // color of green ring from OpenRoad logo
  QColor openroad_logo = QColor::fromRgb(0x00, 0x4A, 0x17);
  // openroad command
  openroad_cmd_format_.setForeground(openroad_logo);
  openroad_cmd_format_.setFontWeight(QFont::Bold);

  // openroad command argument
  openroad_arg_format_.setForeground(openroad_logo.lighter());
  openroad_arg_format_.setFontItalic(true);

  // tcl commands
  tcl_cmd_format_.setForeground(Qt::darkRed);
  tcl_cmd_format_.setFontWeight(QFont::Bold);

  // tcl brackets [] {}
  brackets_format_.setForeground(Qt::black);
  brackets_format_.setFontWeight(QFont::Bold);

  // strings
  string_format_.setForeground(Qt::darkMagenta);
  string_format_.setFontWeight(QFont::Bold);

  // tcl variables
  variable_format_.setForeground(Qt::darkGreen);
  variable_format_.setFontWeight(QFont::Bold);

  // tcl comments
  comment_format_.setForeground(Qt::darkBlue);
  comment_format_.setFontWeight(QFont::Normal);
}

void TclCmdHighlighter::init(Tcl_Interp* interp)
{
  initTclKeywords();
  initOpenRoad(interp);
  initOther();
}

void TclCmdHighlighter::initOpenRoad(Tcl_Interp* interp)
{
  std::set<std::string> cmds;

  QVector<SyntaxRulePtr> cmd_regexes;

  // get registered commands
  if (Tcl_Eval(interp, "array names sta::cmd_args") == TCL_OK) {
    Tcl_Obj* cmd_names = Tcl_GetObjResult(interp);

    int cmd_size;
    Tcl_Obj** cmds_objs;
    Tcl_ListObjGetElements(interp, cmd_names, &cmd_size, &cmds_objs);
    for (int i = 0; i < cmd_size; i++) {
      cmds.insert(Tcl_GetString(cmds_objs[i]));
    }
  }

  // create highlighting for commands and associated arguments
  for (const std::string& cmd : cmds) {
    std::set<std::string> args;
    parseOpenRoadArguments(Tcl_GetVar2(interp,
                                       "sta::cmd_args",
                                       cmd.c_str(),
                                       TCL_LEAVE_ERR_MSG), args);

    std::vector<std::string> args_regex;
    for (const std::string& arg : args) {
      args_regex.push_back(escape(arg) + end_of_command_);
    }

    cmd_regexes.push_back(buildKeywordRule(cmd, args_regex));
  }

  // get commands from common OpenRoad namespaces
  const std::string namespaces[] = {"ord", "sta", "odb", "utl"};
  for (const std::string& ns : namespaces) {
    std::string info = "info commands ::" + ns + "::*";
    if (Tcl_Eval(interp, info.c_str()) == TCL_OK) {
      Tcl_Obj* cmd_names = Tcl_GetObjResult(interp);
      int cmd_size;
      Tcl_Obj** cmds_objs;
      Tcl_ListObjGetElements(interp, cmd_names, &cmd_size, &cmds_objs);
      for (int i = 0; i < cmd_size; i++) {
        std::string cmd = Tcl_GetString(cmds_objs[i]);
        cmd_regexes.push_back(buildRule(start_of_command_ + "((::)?" + escape(cmd.substr(2)) + ")" + end_of_command_));
      }
    }
  }

  addRuleGroup(cmd_rules_, cmd_regexes, &openroad_cmd_format_, &openroad_arg_format_);
}

void TclCmdHighlighter::parseOpenRoadArguments(const char* or_args,
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

void TclCmdHighlighter::initTclKeywords()
{
  QVector<SyntaxRulePtr> rules;

  // tcl keywords
  const std::string tcl_keywords[] = {"after", "append", "apply", "array", "auto_execok",
      "auto_import", "auto_load", "auto_load_index", "auto_qualify", "binary", "break",
      "catch", "cd", "chan", "clock", "close", "concat", "continue", "coroutine", "dict",
      "echo", "encoding", "eof", "error", "eval", "exec", "exit", "expr", "fblocked",
      "fconfigure", "fcopy", "file", "fileevent", "flush", "for", "foreach", "fork",
      "format", "gets", "glob", "global", "history", "if", "incr", "info", "interp",
      "join", "lappend", "lassign", "lindex", "linsert", "list", "llength", "load",
      "lrange", "lrepeat", "lreplace", "lreverse", "lsearch", "lset", "lsort", "namespace",
      "open", "oo::class", "oo::copy", "oo::define", "oo::objdefine", "oo::object",
      "package", "pid", "proc", "puts", "pwd", "read", "regexp", "regsub", "rename",
      "return", "scan", "seek", "set", "sleep", "socket", "source", "split", "string",
      "subst", "switch", "system", "tailcall", "tclLog", "tell", "throw", "time", "trace",
      "try", "unload", "unset", "update", "uplevel", "upvar", "variable", "vwait", "wait",
      "while", "zlib"};
  for (const std::string& word : tcl_keywords) {
    rules.push_back(buildKeywordRule(word));
  }

  addRuleGroup(cmd_rules_, rules, &tcl_cmd_format_, nullptr);
}

void TclCmdHighlighter::initOther()
{
  std::string not_escaped = "(?<!\\\\)";

  QVector<SyntaxRulePtr> rules;

  // string
  rules.clear();
  rules.push_back(buildRule("\".*?"+not_escaped+"\"")); // complete string
  rules.push_back(buildRule("(\".*?$)")); // string at end of line
  rules.push_back(buildRule("^.*?"+not_escaped+"\"")); //start of line
  rules.push_back(buildRule(".*")); //whole line
  string_rule.rules = rules;
  string_rule.format = &string_format_;
  string_rule.args_format = nullptr;

  // tcl {} []
  rules.clear();
  rules.push_back(buildRule(not_escaped+"(\\{|\\})"));
  rules.push_back(buildRule(not_escaped+"(\\[|\\])"));
  addRuleGroup(syntax_rules_, rules, &brackets_format_, nullptr);

  // variable
  rules.clear();
  std::string variable_form = "(\\w*::)*\\w+(\\(\\w+\\))?";
  rules.push_back(buildRule("(\\$"+variable_form+")"));
  rules.push_back(buildRule("(\\$\\{"+variable_form+"\\})"));
  addRuleGroup(syntax_rules_, rules, &variable_format_, nullptr);

  // comment
  rules.clear();
  rules.push_back(buildRule(not_escaped+"#.*"));
  addRuleGroup(syntax_rules_, rules, &comment_format_, nullptr);
}

const std::string TclCmdHighlighter::escape(const std::string& preregex)
{
  QString escaped = QRegularExpression::escape(preregex.c_str());

  return escaped.toLatin1().data();
}

SyntaxRulePtr TclCmdHighlighter::buildKeywordRule(const std::string& pattern)
{
  return buildKeywordRule(pattern, {});
}

SyntaxRulePtr TclCmdHighlighter::buildKeywordRule(const std::string& pattern,
                                                  const std::vector<std::string>& args)
{
  return buildRule(start_of_command_ + "(" + escape(pattern) + ")" + end_of_command_, args);
}

SyntaxRulePtr TclCmdHighlighter::buildRule(const std::string& pattern)
{
  return buildRule(pattern, {});
}

SyntaxRulePtr TclCmdHighlighter::buildRule(const std::string& pattern,
                                           const std::vector<std::string>& args)
{
  SyntaxRulePtr rule = std::make_shared<SyntaxRule>();

  rule->pattern = std::make_shared<QRegularExpression>(pattern.c_str());
  if (args.empty()) {
    rule->args = nullptr;
  }
  else {
    rule->args = std::make_shared<QVector<QRegularExpressionPtr>>();
    for (const std::string& arg : args) {
      rule->args->push_back(std::make_shared<QRegularExpression>(arg.c_str()));
    }
  }

  return rule;
}

void TclCmdHighlighter::addRuleGroup(QVector<SyntaxRuleGroup>& rule_group,
                                     const QVector<SyntaxRulePtr>& rules,
                                     const QTextCharFormat* format,
                                     const QTextCharFormat* args_format)
{
  SyntaxRuleGroup group;
  group.rules = rules;
  group.format = format;
  group.args_format = args_format;

  rule_group.push_back(group);
}

void TclCmdHighlighter::highlightBlock(const QString& text)
{
  // highlight commands
  highlightBlockWithRules(text, 0, cmd_rules_);

  // highlight command arguments
  for (auto& [args_rule, format] : qAsConst(argument_rules_)) {
    for (const QRegularExpressionPtr& rule : *args_rule) {
      highlightBlockWithRule(text, 0, rule, format);
    }
  }

  // highlight strings
  highlightBlockWithString(text);

  // highlight remaining syntax
  highlightBlockWithRules(text, 0, syntax_rules_);

  // not a multi line statement, clear cmd_args list
  if (text.size() > 0 && text.at(text.size() - 1).toLatin1() != '\\') {
    argument_rules_.clear();
  }
}

void TclCmdHighlighter::highlightBlockWithString(const QString& text)
{
  // highlight strings
  int offset = 0;
  if (previousBlockState() > 0) {
    // check if string ended
    offset = highlightBlockWithRule(text, 0, string_rule.rules[2]->pattern, string_rule.format);
    if (offset != -1) {
      setCurrentBlockState(-1);
    }
    else {
      highlightBlockWithRule(text, 0, string_rule.rules[3]->pattern, string_rule.format);
      setCurrentBlockState(2);
    }
  }

  if (currentBlockState() != 2) {
    int complete_offset = highlightBlockWithRule(text, offset, string_rule.rules[0]->pattern, string_rule.format);
    if (complete_offset != -1) {
      offset = complete_offset;
    }

    if (highlightBlockWithRule(text, offset, string_rule.rules[1]->pattern, string_rule.format) != -1) {
      setCurrentBlockState(1);
    }
    else {
      setCurrentBlockState(-1);
    }
  }
  else {
    setCurrentBlockState(1);
  }
}

int TclCmdHighlighter::highlightBlockWithRule(const QString& text,
                                              int start_idx,
                                              const QRegularExpressionPtr& rule,
                                              const QTextCharFormat* format)
{
  int last_match = -1;
  QRegularExpressionMatchIterator matchIterator = rule->globalMatch(text, start_idx);

  while (matchIterator.hasNext()) {
    QRegularExpressionMatch match = matchIterator.next();

    int groups_start = 0;
    if (match.lastCapturedIndex() > 0) {
      // sub groups were used, so ignore whole match
      groups_start = 1;
    }

    for (int i = groups_start; i <= match.lastCapturedIndex(); i++) {
      setFormat(match.capturedStart(i), match.capturedLength(i), *format);
      last_match = match.capturedEnd(i);
    }
  }

  return last_match;
}

void TclCmdHighlighter::highlightBlockWithRules(const QString& text,
                                                int start_idx,
                                                const QVector<SyntaxRuleGroup>& group)
{
  for (const SyntaxRuleGroup& rules : qAsConst(group)) {
    for (const SyntaxRulePtr& rule : rules.rules) {
      if (highlightBlockWithRule(text, start_idx, rule->pattern, rules.format) != -1) {
        if (rule->args != nullptr) {
          argument_rules_.push_back(std::make_pair(rule->args, rules.args_format));
        }
      }
    }
  }
}

}  // namespace gui
