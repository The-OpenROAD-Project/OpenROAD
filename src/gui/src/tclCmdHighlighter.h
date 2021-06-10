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

#pragma once

#include <tcl.h>

#include <set>
#include <vector>
#include <memory>

#include <QSyntaxHighlighter>
#include <QRegularExpression>
#include <QTextCharFormat>

namespace gui {

using QRegularExpressionPtr = std::shared_ptr<QRegularExpression>;
struct SyntaxRule
{
  QRegularExpressionPtr pattern;
  std::shared_ptr<QVector<QRegularExpressionPtr>> args;
};

using SyntaxRulePtr = std::shared_ptr<SyntaxRule>;

struct SyntaxRuleGroup
{
  QVector<SyntaxRulePtr> rules;
  const QTextCharFormat* format;
  const QTextCharFormat* args_format;
};

class TclCmdHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

  public:
    TclCmdHighlighter(QTextDocument* parent, Tcl_Interp* interp);
    ~TclCmdHighlighter();

  protected:
    void highlightBlock(const QString& text) override;

  private:
    void initFormats();

    void init(Tcl_Interp* interp);
    void initOpenRoad(Tcl_Interp* interp);
    void initTclKeywords();
    void initOther();

    void parseOpenRoadArguments(const char* or_args, std::set<std::string>& args);

    const std::string escape(const std::string& preregex);
    SyntaxRulePtr buildKeywordRule(const std::string& pattern);
    SyntaxRulePtr buildKeywordRule(const std::string& pattern,
                                   const std::vector<std::string>& args);
    SyntaxRulePtr buildRule(const std::string& pattern);
    SyntaxRulePtr buildRule(const std::string& pattern,
                            const std::vector<std::string>& args);

    void addRuleGroup(QVector<SyntaxRuleGroup>& rule_group,
                      const QVector<SyntaxRulePtr>& rules,
                      const QTextCharFormat* format,
                      const QTextCharFormat* args_format);

    void highlightBlockWithRules(const QString& text,
                                 int start_idx,
                                 const QVector<SyntaxRuleGroup>& rules);
    int highlightBlockWithRule(const QString& text,
                               int start_idx,
                               const QRegularExpressionPtr& rule,
                               const QTextCharFormat* format);

    void highlightBlockWithString(const QString& text);

    QVector<SyntaxRuleGroup> cmd_rules_;
    // general syntax rules
    QVector<SyntaxRuleGroup> syntax_rules_;
    // string formatting, needs to be handled separately since it can span multiple lines
    SyntaxRuleGroup string_rule;

    // holds the rules for commands detected
    QVector<std::pair<const std::shared_ptr<QVector<QRegularExpressionPtr>>, const QTextCharFormat*>> argument_rules_;

    // formatting
    QTextCharFormat openroad_cmd_format_;
    QTextCharFormat openroad_arg_format_;
    QTextCharFormat tcl_cmd_format_;
    QTextCharFormat brackets_format_;
    QTextCharFormat string_format_;
    QTextCharFormat variable_format_;
    QTextCharFormat comment_format_;

    // common regex
    const std::string start_of_command_ = "(?:^|(?<=\\s)|(?<=\\[))";
    const std::string end_of_command_   = "(?:$|(?=\\s)|(?=\\]))";
};

}  // namespace gui
