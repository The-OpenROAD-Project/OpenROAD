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

#include "tclCmdHighlighter.h"

#include <QTextDocument>

namespace gui {

TclCmdHighlighter::TclCmdHighlighter(QTextDocument* parent,
                                     const std::vector<CommandArguments>& or_cmds,
                                     const std::string& command_start,
                                     const std::string& command_end) :
    QSyntaxHighlighter(parent)
{
  initFormats();
  init(or_cmds, command_start, command_end);
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

void TclCmdHighlighter::init(const std::vector<CommandArguments>& or_cmds,
                             const std::string& start_of_command,
                             const std::string& end_of_command)
{
  initTclKeywords(start_of_command, end_of_command);
  initOpenRoad(or_cmds, start_of_command, end_of_command);
  initOther();
}

void TclCmdHighlighter::initOpenRoad(const std::vector<CommandArguments>& or_cmds,
                                     const std::string& start_of_command,
                                     const std::string& end_of_command)
{
  std::vector<CommandRulePtr> cmd_regexes;

  // create highlighting for commands and associated arguments
  int idx = 0;
  for (const auto& [cmd, or_cmd, args] : or_cmds) {
    std::vector<std::string> args_regex;
    for (const std::string& arg : args) {
      args_regex.push_back(escape(arg) + end_of_command);
    }

    cmd_regexes.push_back(buildKeywordRule(idx, cmd, start_of_command, end_of_command));
    if (args_regex.size() > 0) {
      argument_rules_.emplace(idx, buildArgumentRule(args_regex, &openroad_arg_format_));
    }

    idx++;
  }

  addRuleGroup(cmd_rules_, cmd_regexes, &openroad_cmd_format_);
}

void TclCmdHighlighter::initTclKeywords(const std::string& start_of_command,
                                        const std::string& end_of_command)
{
  std::vector<CommandRulePtr> rules;

  // tcl keywords
  const std::string tcl_keywords[] = {"after", "append", "apply", "array", "auto_execok",
      "auto_import", "auto_load", "auto_load_index", "auto_qualify", "binary", "break",
      "catch", "cd", "chan", "clock", "close", "concat", "continue", "coroutine", "dict",
      "echo", "else", "elseif", "encoding", "eof", "error", "eval", "exec", "exit", "expr",
      "fblocked", "fconfigure", "fcopy", "file", "fileevent", "flush", "for", "foreach",
      "fork", "format", "gets", "glob", "global", "history", "if", "incr", "info", "interp",
      "join", "lappend", "lassign", "lindex", "linsert", "list", "llength", "load",
      "lrange", "lrepeat", "lreplace", "lreverse", "lsearch", "lset", "lsort", "namespace",
      "open", "oo::class", "oo::copy", "oo::define", "oo::objdefine", "oo::object",
      "package", "parray", "pid", "proc", "puts", "pwd", "read", "regexp", "regsub", "rename",
      "return", "scan", "seek", "set", "sleep", "socket", "source", "split", "string",
      "subst", "switch", "system", "tailcall", "tclLog", "tell", "throw", "time", "trace",
      "try", "unload", "unset", "update", "uplevel", "upvar", "variable", "vwait", "wait",
      "while", "zlib"};
  for (const std::string& word : tcl_keywords) {
    rules.push_back(buildKeywordRule(-1, word, start_of_command, end_of_command));
  }

  addRuleGroup(cmd_rules_, rules, &tcl_cmd_format_);
}

void TclCmdHighlighter::initOther()
{
  std::string not_escaped = "(?<!\\\\)";

  // string
  string_rule.rules.push_back(buildRule("\".*?"+not_escaped+"\"")); // complete string
  string_rule.rules.push_back(buildRule("(\".*?$)")); // string at end of line
  string_rule.rules.push_back(buildRule("^.*?"+not_escaped+"\"")); //start of line
  string_rule.rules.push_back(buildRule(".*")); //whole line
  string_rule.format = &string_format_;

  std::vector<CommandRulePtr> rules;

  // tcl {} []
  rules.push_back(buildRule(not_escaped+"(\\{|\\})"));
  rules.push_back(buildRule(not_escaped+"(\\[|\\])"));
  addRuleGroup(syntax_rules_, rules, &brackets_format_);

  // variable
  std::string variable_form = "(\\w*::)*\\w+(\\(\\w+\\))?";
  rules.push_back(buildRule("(\\$"+variable_form+")"));
  rules.push_back(buildRule("(\\$\\{"+variable_form+"\\})"));
  addRuleGroup(syntax_rules_, rules, &variable_format_);

  // comment
  rules.push_back(buildRule(not_escaped+"#.*"));
  addRuleGroup(syntax_rules_, rules, &comment_format_);
}

const std::string TclCmdHighlighter::escape(const std::string& preregex)
{
  QString escaped = QRegularExpression::escape(preregex.c_str());

  return escaped.toLatin1().data();
}

CommandRulePtr TclCmdHighlighter::buildKeywordRule(const int command_id,
                                                   const std::string& command,
                                                   const std::string& start_of_command,
                                                   const std::string& end_of_command)
{
  return buildRule(command_id, start_of_command + "((::)?" + escape(command) + ")" + end_of_command);
}

CommandRulePtr TclCmdHighlighter::buildRule(const std::string& pattern)
{
  return buildRule(-1, pattern);
}

CommandRulePtr TclCmdHighlighter::buildRule(const int command_id,
                                            const std::string& pattern)
{
  CommandRulePtr rule = std::make_unique<CommandRule>();

  rule->pattern = std::make_unique<QRegularExpression>(pattern.c_str());
  rule->command = command_id;

  return rule;
}

ArgumentRulePtr TclCmdHighlighter::buildArgumentRule(const std::vector<std::string>& args,
                                                     const QTextCharFormat* format)
{
  ArgumentRulePtr rule = std::make_unique<ArgumentRule>();

  rule->format = format;
  for (const std::string& arg : args) {
    rule->rules.push_back(std::make_unique<QRegularExpression>(arg.c_str()));
  }

  return rule;
}

void TclCmdHighlighter::addRuleGroup(std::vector<CommandRuleGroup>& rule_group,
                                     std::vector<CommandRulePtr>& rules,
                                     const QTextCharFormat* format)
{
  CommandRuleGroup group;
  for (CommandRulePtr& rule : rules) {
    group.rules.push_back(std::move(rule));
  }
  rules.clear();
  group.format = format;

  rule_group.push_back(std::move(group));
}

void TclCmdHighlighter::highlightBlock(const QString& text)
{
  std::set<int> matched_commands;

  int current_block_number = currentBlock().blockNumber();
  if (current_block_number > 0) {
    TclCmdUserData* previous_block_data = static_cast<TclCmdUserData*>(document()->findBlockByNumber(current_block_number-1).userData());
    if (previous_block_data != nullptr && previous_block_data->line_continued) {
      matched_commands = previous_block_data->commands;
    }
  }

  // highlight commands
  highlightBlockWithRules(text, 0, cmd_rules_, matched_commands);

  // highlight command arguments
  for (const int cmd : matched_commands) {
    ArgumentRulePtr& args = argument_rules_[cmd];
    if (args != nullptr) {
      for (const QRegularExpressionPtr& rule : args->rules) {
        highlightBlockWithRule(text, 0, rule, args->format);
      }
    }
  }

  // highlight strings
  highlightBlockWithString(text);

  // highlight remaining syntax
  highlightBlockWithRules(text, 0, syntax_rules_, matched_commands);

  // not a multi line statement, clear cmd_args list
  TclCmdUserData* command_data = static_cast<TclCmdUserData*>(currentBlockUserData());
  if (command_data == nullptr) {
    // QFragmentMap handles free
    command_data = new TclCmdUserData();
  }
  command_data->line_continued = true;
  command_data->commands = matched_commands;
  if (text.size() > 0 && text.at(text.size() - 1).toLatin1() != '\\') {
    command_data->line_continued = false;
  }
  setCurrentBlockUserData(command_data);
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
                                                const std::vector<CommandRuleGroup>& group,
                                                std::set<int>& matched_commands)
{
  for (const CommandRuleGroup& rules : group) {
    for (const CommandRulePtr& rule : rules.rules) {
      if (highlightBlockWithRule(text, start_idx, rule->pattern, rules.format) != -1) {
        if (rule->command >= 0) {
          matched_commands.insert(rule->command);
        }
      }
    }
  }
}

}  // namespace gui
