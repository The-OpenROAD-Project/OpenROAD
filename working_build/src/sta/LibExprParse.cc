// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.



// First part of user prologue.
#line 27 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"

#include "FuncExpr.hh"
#include "liberty/LibExprReader.hh"
#include "liberty/LibExprReaderPvt.hh"
#include "liberty/LibExprScanner.hh"

#undef yylex
#define yylex scanner->lex

// warning: variable 'yynerrs_' set but not used
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

void
sta::LibExprParse::error(const std::string &msg)
{
  reader->parseError(msg.c_str());
}


#line 61 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"


#include "LibExprParse.hh"




#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif



// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 50 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
namespace sta {
#line 140 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"

  /// Build a parser object.
  LibExprParse::LibExprParse (LibExprScanner *scanner_yyarg, LibExprReader *reader_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      reader (reader_yyarg)
  {}

  LibExprParse::~LibExprParse ()
  {}

  LibExprParse::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/

  // basic_symbol.
  template <typename Base>
  LibExprParse::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  LibExprParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t)
    : Base (t)
    , value ()
  {}

  template <typename Base>
  LibExprParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (value_type) v)
    : Base (t)
    , value (YY_MOVE (v))
  {}


  template <typename Base>
  LibExprParse::symbol_kind_type
  LibExprParse::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  LibExprParse::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  LibExprParse::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
  }

  // by_kind.
  LibExprParse::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  LibExprParse::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  LibExprParse::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  LibExprParse::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  void
  LibExprParse::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  LibExprParse::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  LibExprParse::symbol_kind_type
  LibExprParse::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  LibExprParse::symbol_kind_type
  LibExprParse::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }



  // by_state.
  LibExprParse::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  LibExprParse::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  LibExprParse::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  LibExprParse::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  LibExprParse::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  LibExprParse::symbol_kind_type
  LibExprParse::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  LibExprParse::stack_symbol_type::stack_symbol_type ()
  {}

  LibExprParse::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  LibExprParse::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  LibExprParse::stack_symbol_type&
  LibExprParse::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    return *this;
  }

  LibExprParse::stack_symbol_type&
  LibExprParse::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  LibExprParse::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YY_USE (yysym.kind ());
  }

#if YYDEBUG
  template <typename Base>
  void
  LibExprParse::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " (";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  LibExprParse::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  LibExprParse::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  LibExprParse::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  LibExprParse::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  LibExprParse::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  LibExprParse::debug_level_type
  LibExprParse::debug_level () const
  {
    return yydebug_;
  }

  void
  LibExprParse::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  LibExprParse::state_type
  LibExprParse::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  LibExprParse::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  LibExprParse::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  LibExprParse::operator() ()
  {
    return parse ();
  }

  int
  LibExprParse::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            yyla.kind_ = yytranslate_ (yylex (&yyla.value));
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* If YYLEN is nonzero, implement the default value of the
         action: '$$ = $1'.  Otherwise, use the top of the stack.

         Otherwise, the following line sets YYLHS.VALUE to garbage.
         This behavior is undocumented and Bison users should not rely
         upon it.  */
      if (yylen)
        yylhs.value = yystack_[yylen - 1].value;
      else
        yylhs.value = yystack_[0].value;


      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // result_expr: expr
#line 74 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                { reader->setResult((yystack_[0].value.expr)); }
#line 598 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 3: // result_expr: expr ';'
#line 75 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                { reader->setResult((yystack_[1].value.expr)); }
#line 604 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 4: // terminal: PORT
#line 79 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprPort((yystack_[0].value.string)); }
#line 610 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 5: // terminal: '0'
#line 80 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = sta::FuncExpr::makeZero(); }
#line 616 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 6: // terminal: '1'
#line 81 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = sta::FuncExpr::makeOne(); }
#line 622 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 7: // terminal: '(' expr ')'
#line 82 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = (yystack_[1].value.expr); }
#line 628 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 8: // terminal_expr: terminal
#line 86 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
        { (yylhs.value.expr) = (yystack_[0].value.expr); }
#line 634 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 9: // terminal_expr: '!' terminal
#line 87 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprNot((yystack_[0].value.expr)); }
#line 640 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 10: // terminal_expr: terminal '\''
#line 88 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprNot((yystack_[1].value.expr)); }
#line 646 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 11: // implicit_and: terminal_expr terminal_expr
#line 93 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
        { (yylhs.value.expr) = reader->makeFuncExprAnd((yystack_[1].value.expr), (yystack_[0].value.expr)); }
#line 652 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 12: // implicit_and: implicit_and terminal_expr
#line 95 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
        { (yylhs.value.expr) = reader->makeFuncExprAnd((yystack_[1].value.expr), (yystack_[0].value.expr)); }
#line 658 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 13: // expr: terminal_expr
#line 99 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
        { (yylhs.value.expr) = (yystack_[0].value.expr); }
#line 664 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 14: // expr: implicit_and
#line 100 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
        { (yylhs.value.expr) = (yystack_[0].value.expr); }
#line 670 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 15: // expr: expr '+' expr
#line 101 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprOr((yystack_[2].value.expr), (yystack_[0].value.expr)); }
#line 676 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 16: // expr: expr '|' expr
#line 102 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprOr((yystack_[2].value.expr), (yystack_[0].value.expr)); }
#line 682 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 17: // expr: expr '*' expr
#line 103 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprAnd((yystack_[2].value.expr), (yystack_[0].value.expr)); }
#line 688 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 18: // expr: expr '&' expr
#line 104 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprAnd((yystack_[2].value.expr), (yystack_[0].value.expr)); }
#line 694 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;

  case 19: // expr: expr '^' expr
#line 105 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
                        { (yylhs.value.expr) = reader->makeFuncExprXor((yystack_[2].value.expr), (yystack_[0].value.expr)); }
#line 700 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"
    break;


#line 704 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        std::string msg = YY_("syntax error");
        error (YY_MOVE (msg));
      }


    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;


      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  LibExprParse::error (const syntax_error& yyexc)
  {
    error (yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  LibExprParse::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const signed char LibExprParse::yypact_ninf_ = -6;

  const signed char LibExprParse::yytable_ninf_ = -1;

  const signed char
  LibExprParse::yypact_[] =
  {
      15,    -6,     8,    -6,    -6,    15,     1,     9,    15,    15,
      26,    -6,     2,    -6,    -6,    -6,    -6,    15,    15,    15,
      15,    15,    -6,    -6,    -4,    -4,    -3,    -3,    -6
  };

  const signed char
  LibExprParse::yydefact_[] =
  {
       0,     4,     0,     5,     6,     0,     0,     8,    13,    14,
       2,     9,     0,     1,    10,    11,    12,     0,     0,     0,
       0,     0,     3,     7,    15,    16,    17,    18,    19
  };

  const signed char
  LibExprParse::yypgoto_[] =
  {
      -6,    -6,    21,    17,    -6,    -5
  };

  const signed char
  LibExprParse::yydefgoto_[] =
  {
       0,     6,     7,     8,     9,    10
  };

  const signed char
  LibExprParse::yytable_[] =
  {
      12,    13,    19,    20,    21,    21,    17,    18,    19,    20,
      21,     1,    24,    25,    26,    27,    28,    23,     1,    14,
       3,     4,     5,    11,     2,    15,    16,     3,     4,     5,
      17,    18,    19,    20,    21,     0,     0,    22
  };

  const signed char
  LibExprParse::yycheck_[] =
  {
       5,     0,     6,     7,     8,     8,     4,     5,     6,     7,
       8,     3,    17,    18,    19,    20,    21,    15,     3,    10,
      12,    13,    14,     2,     9,     8,     9,    12,    13,    14,
       4,     5,     6,     7,     8,    -1,    -1,    11
  };

  const signed char
  LibExprParse::yystos_[] =
  {
       0,     3,     9,    12,    13,    14,    17,    18,    19,    20,
      21,    18,    21,     0,    10,    19,    19,     4,     5,     6,
       7,     8,    11,    15,    21,    21,    21,    21,    21
  };

  const signed char
  LibExprParse::yyr1_[] =
  {
       0,    16,    17,    17,    18,    18,    18,    18,    19,    19,
      19,    20,    20,    21,    21,    21,    21,    21,    21,    21
  };

  const signed char
  LibExprParse::yyr2_[] =
  {
       0,     2,     1,     2,     1,     1,     1,     3,     1,     2,
       2,     2,     2,     1,     1,     3,     3,     3,     3,     3
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const LibExprParse::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "PORT", "'+'", "'|'",
  "'*'", "'&'", "'^'", "'!'", "'\\''", "';'", "'0'", "'1'", "'('", "')'",
  "$accept", "result_expr", "terminal", "terminal_expr", "implicit_and",
  "expr", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const signed char
  LibExprParse::yyrline_[] =
  {
       0,    74,    74,    75,    79,    80,    81,    82,    86,    87,
      88,    92,    94,    99,   100,   101,   102,   103,   104,   105
  };

  void
  LibExprParse::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  LibExprParse::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  LibExprParse::symbol_kind_type
  LibExprParse::yytranslate_ (int t) YY_NOEXCEPT
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const signed char
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     9,     2,     2,     2,     2,     7,    10,
      14,    15,     6,     4,     2,     2,     2,     2,    12,    13,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    11,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     8,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     5,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3
    };
    // Last valid token kind.
    const int code_max = 258;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 50 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"
} // sta
#line 1064 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibExprParse.cc"

#line 108 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibExprParse.yy"

