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
#line 25 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"

#include <cctype>

#include "Report.hh"
#include "StringUtil.hh"
#include "power/SaifReaderPvt.hh"
#include "power/SaifScanner.hh"

#undef yylex
#define yylex scanner->lex

// warning: variable 'yynerrs_' set but not used
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define loc_line(loc) loc.begin.line

void
sta::SaifParse::error(const location_type &loc,
                      const std::string &msg)
{
  reader->report()->fileError(169,reader->filename(),loc.begin.line,"%s",msg.c_str());
}

#line 65 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"


#include "SaifParse.hh"

// Second part of user prologue.
#line 85 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"


#line 74 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"



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

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


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

#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
namespace sta {
#line 168 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"

  /// Build a parser object.
  SaifParse::SaifParse (SaifScanner *scanner_yyarg, SaifReader *reader_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      reader (reader_yyarg)
  {}

  SaifParse::~SaifParse ()
  {}

  SaifParse::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/

  // basic_symbol.
  template <typename Base>
  SaifParse::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  SaifParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  SaifParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (value_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}


  template <typename Base>
  SaifParse::symbol_kind_type
  SaifParse::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  SaifParse::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  SaifParse::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_kind.
  SaifParse::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  SaifParse::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  SaifParse::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  SaifParse::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  void
  SaifParse::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  SaifParse::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  SaifParse::symbol_kind_type
  SaifParse::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  SaifParse::symbol_kind_type
  SaifParse::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }



  // by_state.
  SaifParse::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  SaifParse::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  SaifParse::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  SaifParse::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  SaifParse::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  SaifParse::symbol_kind_type
  SaifParse::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  SaifParse::stack_symbol_type::stack_symbol_type ()
  {}

  SaifParse::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  SaifParse::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  SaifParse::stack_symbol_type&
  SaifParse::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  SaifParse::stack_symbol_type&
  SaifParse::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  SaifParse::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YY_USE (yysym.kind ());
  }

#if YYDEBUG
  template <typename Base>
  void
  SaifParse::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  SaifParse::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  SaifParse::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  SaifParse::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  SaifParse::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  SaifParse::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  SaifParse::debug_level_type
  SaifParse::debug_level () const
  {
    return yydebug_;
  }

  void
  SaifParse::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  SaifParse::state_type
  SaifParse::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  SaifParse::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  SaifParse::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  SaifParse::operator() ()
  {
    return parse ();
  }

  int
  SaifParse::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

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
            yyla.kind_ = yytranslate_ (yylex (&yyla.value, &yyla.location));
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

      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // file: '(' SAIFILE header instance ')'
#line 91 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                                         {}
#line 642 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 5: // header_stmt: '(' SAIFVERSION QSTRING ')'
#line 100 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                                    { sta::stringDelete((yystack_[1].value.string)); }
#line 648 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 6: // header_stmt: '(' DIRECTION QSTRING ')'
#line 101 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                                  { sta::stringDelete((yystack_[1].value.string)); }
#line 654 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 7: // header_stmt: '(' DESIGN QSTRING ')'
#line 102 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                               { sta::stringDelete((yystack_[1].value.string)); }
#line 660 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 8: // header_stmt: '(' DESIGN ')'
#line 103 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                       { }
#line 666 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 9: // header_stmt: '(' DATE QSTRING ')'
#line 104 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                             { sta::stringDelete((yystack_[1].value.string)); }
#line 672 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 10: // header_stmt: '(' VENDOR QSTRING ')'
#line 105 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                               { sta::stringDelete((yystack_[1].value.string)); }
#line 678 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 11: // header_stmt: '(' PROGRAM_NAME QSTRING ')'
#line 106 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                                     { sta::stringDelete((yystack_[1].value.string)); }
#line 684 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 12: // header_stmt: '(' VERSION QSTRING ')'
#line 107 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                                { sta::stringDelete((yystack_[1].value.string)); }
#line 690 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 13: // header_stmt: '(' DIVIDER hchar ')'
#line 108 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                              { reader->setDivider((yystack_[1].value.character)); }
#line 696 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 14: // header_stmt: '(' TIMESCALE UINT ID ')'
#line 109 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                                  { reader->setTimescale((yystack_[2].value.uint), (yystack_[1].value.string)); }
#line 702 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 15: // header_stmt: '(' DURATION UINT ')'
#line 110 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
                              { reader->setDuration((yystack_[1].value.uint)); }
#line 708 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 16: // hchar: '/'
#line 115 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.character) = '/'; }
#line 714 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 17: // hchar: '.'
#line 117 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.character) = '.'; }
#line 720 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 18: // $@1: %empty
#line 122 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { reader->instancePush((yystack_[0].value.string)); }
#line 726 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 19: // instance: '(' INSTANCE ID $@1 instance_contents ')'
#line 124 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { reader->instancePop(); }
#line 732 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 20: // $@2: %empty
#line 126 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { reader->instancePush((yystack_[1].value.string)); }
#line 738 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 21: // instance: '(' INSTANCE QSTRING ID $@2 instance_contents ')'
#line 128 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { reader->instancePop(); }
#line 744 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 30: // net: '(' ID state_durations ')'
#line 150 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { reader->setNetDurations((yystack_[2].value.string), (yystack_[1].value.state_durations)); }
#line 750 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 34: // state_durations: '(' state UINT ')'
#line 164 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state_durations)[static_cast<int>((yystack_[2].value.state))] = (yystack_[1].value.uint); }
#line 756 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 35: // state_durations: state_durations '(' state UINT ')'
#line 166 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state_durations)[static_cast<int>((yystack_[2].value.state))] = (yystack_[1].value.uint); }
#line 762 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 36: // state: T0
#line 171 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::T0; }
#line 768 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 37: // state: T1
#line 173 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::T1; }
#line 774 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 38: // state: TX
#line 175 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::TX; }
#line 780 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 39: // state: TZ
#line 177 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::TZ; }
#line 786 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 40: // state: TB
#line 179 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::TB; }
#line 792 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 41: // state: TC
#line 181 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::TC; }
#line 798 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;

  case 42: // state: IG
#line 183 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
        { (yylhs.value.state) = sta::SaifState::IG; }
#line 804 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"
    break;


#line 808 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"

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
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
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

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

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
  SaifParse::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  SaifParse::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const signed char SaifParse::yypact_ninf_ = -21;

  const signed char SaifParse::yytable_ninf_ = -1;

  const signed char
  SaifParse::yypact_[] =
  {
      -1,    30,    45,    25,   -21,     7,    26,   -21,    32,    33,
       5,    34,    35,    36,    37,     6,    38,    39,    -4,   -21,
      40,    41,    42,    43,   -21,    44,    46,    47,    48,   -21,
     -21,    49,    50,    51,    15,   -21,   -21,   -21,   -21,   -21,
     -21,   -21,   -21,   -21,    52,   -21,    55,   -21,   -21,   -21,
      54,    54,    16,   -21,    12,   -21,    14,    56,    57,   -21,
     -21,   -21,    59,    17,   -21,    62,    19,   -21,    60,   -21,
     -21,    60,   -21,   -21,     4,    21,    23,   -21,   -21,   -21,
     -21,   -21,   -21,   -21,    63,     4,   -21,   -21,    58,    64,
     -21,    65,   -21
  };

  const signed char
  SaifParse::yydefact_[] =
  {
       0,     0,     0,     0,     1,     0,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     4,
       0,     0,     0,     0,     8,     0,     0,     0,     0,    16,
      17,     0,     0,     0,     0,     2,     5,     6,     7,     9,
      10,    11,    12,    13,     0,    15,     0,    18,    14,    20,
      22,    22,     0,    27,     0,    23,     0,     0,     0,    19,
      24,    21,     0,     0,    28,     0,     0,    31,     0,    26,
      29,     0,    25,    32,     0,     0,     0,    36,    37,    38,
      39,    40,    41,    42,     0,     0,    30,    33,     0,     0,
      34,     0,    35
  };

  const signed char
  SaifParse::yypgoto_[] =
  {
     -21,   -21,   -21,    84,   -21,    87,   -21,   -21,    11,   -20,
     -21,     0,   -21,    -2,    -6,   -17
  };

  const signed char
  SaifParse::yydefgoto_[] =
  {
       0,     2,     6,     7,    31,    53,    50,    51,    54,    55,
      63,    64,    66,    67,    75,    84
  };

  const signed char
  SaifParse::yytable_[] =
  {
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      34,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    77,    78,    79,    80,    81,    82,    83,     1,    23,
      34,    57,    58,     3,    60,    24,    60,    29,    30,    46,
      47,    52,    59,    52,    61,     4,    62,    69,    65,    72,
      85,    86,    85,    87,     5,    18,    21,    22,    25,    26,
      27,    28,    56,    70,    73,    76,    32,    33,    89,     0,
      35,    36,    37,    38,    39,    44,    40,    41,    42,    43,
      49,    45,    48,    52,    68,    62,    65,    71,    90,    74,
      19,    88,    91,    20,     0,    92
  };

  const signed char
  SaifParse::yycheck_[] =
  {
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    17,    18,    19,    20,    21,    22,    23,    29,    24,
      14,    15,    16,     3,    54,    30,    56,    31,    32,    24,
      25,    29,    30,    29,    30,     0,    29,    30,    29,    30,
      29,    30,    29,    30,    29,    29,    24,    24,    24,    24,
      24,    24,    51,    63,    66,    71,    28,    28,    85,    -1,
      30,    30,    30,    30,    30,    25,    30,    30,    30,    30,
      25,    30,    30,    29,    25,    29,    29,    25,    30,    29,
       6,    28,    28,     6,    -1,    30
  };

  const signed char
  SaifParse::yystos_[] =
  {
       0,    29,    34,     3,     0,    29,    35,    36,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    29,    36,
      38,    24,    24,    24,    30,    24,    24,    24,    24,    31,
      32,    37,    28,    28,    14,    30,    30,    30,    30,    30,
      30,    30,    30,    30,    25,    30,    24,    25,    30,    25,
      39,    40,    29,    38,    41,    42,    41,    15,    16,    30,
      42,    30,    29,    43,    44,    29,    45,    46,    25,    30,
      44,    25,    30,    46,    29,    47,    47,    17,    18,    19,
      20,    21,    22,    23,    48,    29,    30,    30,    28,    48,
      30,    28,    30
  };

  const signed char
  SaifParse::yyr1_[] =
  {
       0,    33,    34,    35,    35,    36,    36,    36,    36,    36,
      36,    36,    36,    36,    36,    36,    37,    37,    39,    38,
      40,    38,    41,    41,    41,    42,    42,    42,    43,    43,
      44,    45,    45,    46,    47,    47,    48,    48,    48,    48,
      48,    48,    48
  };

  const signed char
  SaifParse::yyr2_[] =
  {
       0,     2,     5,     1,     2,     4,     4,     4,     3,     4,
       4,     4,     4,     4,     5,     4,     1,     1,     0,     6,
       0,     7,     0,     1,     2,     4,     4,     1,     1,     2,
       4,     1,     2,     4,     4,     5,     1,     1,     1,     1,
       1,     1,     1
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const SaifParse::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "SAIFILE",
  "SAIFVERSION", "DIRECTION", "DESIGN", "DATE", "VENDOR", "PROGRAM_NAME",
  "VERSION", "DIVIDER", "TIMESCALE", "DURATION", "INSTANCE", "NET", "PORT",
  "T0", "T1", "TX", "TZ", "TB", "TC", "IG", "QSTRING", "ID", "FNUMBER",
  "DNUMBER", "UINT", "'('", "')'", "'/'", "'.'", "$accept", "file",
  "header", "header_stmt", "hchar", "instance", "$@1", "$@2",
  "instance_contents", "instance_content", "nets", "net", "ports", "port",
  "state_durations", "state", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const unsigned char
  SaifParse::yyrline_[] =
  {
       0,    91,    91,    95,    96,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   114,   116,   122,   121,
     126,   125,   131,   133,   134,   138,   139,   140,   144,   145,
     149,   154,   155,   159,   163,   165,   170,   172,   174,   176,
     178,   180,   182
  };

  void
  SaifParse::yy_stack_print_ () const
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
  SaifParse::yy_reduce_print_ (int yyrule) const
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

  SaifParse::symbol_kind_type
  SaifParse::yytranslate_ (int t) YY_NOEXCEPT
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      29,    30,     2,     2,     2,     2,    32,    31,     2,     2,
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
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28
    };
    // Last valid token kind.
    const int code_max = 283;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"
} // sta
#line 1223 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SaifParse.cc"

#line 186 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/power/SaifParse.yy"

