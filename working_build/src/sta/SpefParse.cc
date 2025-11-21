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
#line 25 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"

#include <cstring>

#include "Report.hh"
#include "StringUtil.hh"
#include "StringSeq.hh"
#include "parasitics/SpefReaderPvt.hh"
#include "parasitics/SpefScanner.hh"

#undef yylex
#define yylex scanner->lex

// warning: variable 'yynerrs_' set but not used
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

void
sta::SpefParse::error(const location_type &loc,
                     const std::string &msg)
{
  reader->report()->fileError(164,reader->filename(),
                              loc.begin.line,"%s",msg.c_str());
}

#line 65 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"


#include "SpefParse.hh"

// Second part of user prologue.
#line 121 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"


#line 74 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"



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

#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
namespace sta {
#line 168 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"

  /// Build a parser object.
  SpefParse::SpefParse (SpefScanner *scanner_yyarg, SpefReader *reader_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      reader (reader_yyarg)
  {}

  SpefParse::~SpefParse ()
  {}

  SpefParse::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/

  // basic_symbol.
  template <typename Base>
  SpefParse::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  SpefParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  SpefParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (value_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}


  template <typename Base>
  SpefParse::symbol_kind_type
  SpefParse::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  SpefParse::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  SpefParse::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_kind.
  SpefParse::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  SpefParse::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  SpefParse::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  SpefParse::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  void
  SpefParse::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  SpefParse::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  SpefParse::symbol_kind_type
  SpefParse::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  SpefParse::symbol_kind_type
  SpefParse::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }



  // by_state.
  SpefParse::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  SpefParse::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  SpefParse::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  SpefParse::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  SpefParse::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  SpefParse::symbol_kind_type
  SpefParse::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  SpefParse::stack_symbol_type::stack_symbol_type ()
  {}

  SpefParse::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  SpefParse::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  SpefParse::stack_symbol_type&
  SpefParse::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  SpefParse::stack_symbol_type&
  SpefParse::stack_symbol_type::operator= (stack_symbol_type& that)
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
  SpefParse::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YY_USE (yysym.kind ());
  }

#if YYDEBUG
  template <typename Base>
  void
  SpefParse::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
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
  SpefParse::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  SpefParse::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  SpefParse::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  SpefParse::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  SpefParse::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  SpefParse::debug_level_type
  SpefParse::debug_level () const
  {
    return yydebug_;
  }

  void
  SpefParse::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  SpefParse::state_type
  SpefParse::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  SpefParse::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  SpefParse::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  SpefParse::operator() ()
  {
    return parse ();
  }

  int
  SpefParse::parse ()
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
  case 3: // prefix_bus_delim: '['
#line 139 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '['; }
#line 642 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 4: // prefix_bus_delim: '{'
#line 141 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '}'; }
#line 648 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 5: // prefix_bus_delim: '('
#line 143 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = ')'; }
#line 654 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 6: // prefix_bus_delim: '<'
#line 145 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '<'; }
#line 660 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 7: // suffix_bus_delim: ']'
#line 150 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = ']'; }
#line 666 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 8: // suffix_bus_delim: '}'
#line 152 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '}'; }
#line 672 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 9: // suffix_bus_delim: ')'
#line 154 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = ')'; }
#line 678 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 10: // suffix_bus_delim: '>'
#line 156 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '>'; }
#line 684 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 11: // hchar: '.'
#line 161 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '.'; }
#line 690 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 12: // hchar: '/'
#line 163 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '/'; }
#line 696 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 13: // hchar: '|'
#line 165 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = '|'; }
#line 702 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 14: // hchar: ':'
#line 167 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.ch) = ':'; }
#line 708 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 16: // spef_version: SPEF QSTRING
#line 188 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 714 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 17: // design_name: DESIGN QSTRING
#line 193 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 720 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 18: // date: DATE QSTRING
#line 198 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 726 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 19: // program_name: PROGRAM QSTRING
#line 203 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 732 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 20: // program_version: PVERSION QSTRING
#line 208 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 738 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 21: // vendor: VENDOR QSTRING
#line 213 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 744 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 22: // design_flow: DESIGN_FLOW qstrings
#line 218 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setDesignFlow((yystack_[0].value.string_seq)); }
#line 750 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 23: // qstrings: QSTRING
#line 223 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string_seq) = new sta::StringSeq;
	  (yylhs.value.string_seq)->push_back((yystack_[0].value.string));
	}
#line 758 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 24: // qstrings: qstrings QSTRING
#line 227 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string_seq)->push_back((yystack_[0].value.string)); }
#line 764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 25: // hierarchy_div_def: DIVIDER hchar
#line 232 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setDivider((yystack_[0].value.ch)); }
#line 770 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 26: // pin_delim_def: DELIMITER hchar
#line 237 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setDelimiter((yystack_[0].value.ch)); }
#line 776 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 27: // bus_delim_def: BUS_DELIMITER prefix_bus_delim
#line 242 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setBusBrackets((yystack_[0].value.ch), '\0'); }
#line 782 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 28: // bus_delim_def: BUS_DELIMITER prefix_bus_delim suffix_bus_delim
#line 244 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setBusBrackets((yystack_[1].value.ch), (yystack_[0].value.ch)); }
#line 788 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 30: // time_scale: T_UNIT pos_number IDENT
#line 258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setTimeScale((yystack_[1].value.number), (yystack_[0].value.string)); }
#line 794 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 31: // cap_scale: C_UNIT pos_number IDENT
#line 263 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setCapScale((yystack_[1].value.number), (yystack_[0].value.string)); }
#line 800 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 32: // res_scale: R_UNIT pos_number IDENT
#line 268 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setResScale((yystack_[1].value.number), (yystack_[0].value.string)); }
#line 806 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 33: // induc_scale: L_UNIT pos_number IDENT
#line 273 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->setInductScale((yystack_[1].value.number), (yystack_[0].value.string)); }
#line 812 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 36: // name_map_entries: name_map_entry
#line 284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 818 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 37: // name_map_entries: name_map_entries name_map_entry
#line 285 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[1].value.string); }
#line 824 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 38: // name_map_entry: INDEX mapped_item
#line 290 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->makeNameMapEntry((yystack_[1].value.string), (yystack_[0].value.string)); }
#line 830 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 39: // mapped_item: IDENT
#line 294 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 836 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 40: // mapped_item: NAME
#line 295 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 842 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 41: // mapped_item: QSTRING
#line 296 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 848 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 48: // net_names: net_name
#line 317 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 854 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 49: // net_names: net_names net_name
#line 318 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[1].value.string); }
#line 860 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 50: // net_name: name_or_index
#line 323 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 866 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 56: // port_entries: port_entry
#line 340 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 872 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 57: // port_entries: port_entries port_entry
#line 341 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[1].value.string); }
#line 878 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 58: // port_entry: port_name direction conn_attrs
#line 346 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[2].value.string)); }
#line 884 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 59: // direction: IDENT
#line 351 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.port_dir) = reader->portDirection((yystack_[0].value.string));
          sta::stringDelete((yystack_[0].value.string));
	}
#line 892 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 60: // port_name: name_or_index
#line 357 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 898 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 61: // inst_name: name_or_index
#line 361 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 904 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 63: // pport_entries: pport_entry
#line 369 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 910 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 64: // pport_entries: pport_entries pport_entry
#line 370 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[1].value.string); }
#line 916 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 65: // pport_entry: pport_name IDENT conn_attrs
#line 374 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[2].value.string); }
#line 922 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 66: // pport_name: name_or_index
#line 379 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 928 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 67: // pport_name: physical_inst ':' pport
#line 381 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[2].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 936 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 68: // pport: name_or_index
#line 387 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 942 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 69: // physical_inst: name_or_index
#line 391 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 948 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 77: // cap_load: KW_L par_value
#line 414 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { delete (yystack_[0].value.triple); }
#line 954 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 78: // par_value: number
#line 419 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.triple) = new sta::SpefTriple((yystack_[0].value.number)); }
#line 960 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 79: // par_value: number ':' number ':' number
#line 421 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.triple) = new sta::SpefTriple((yystack_[4].value.number), (yystack_[2].value.number), (yystack_[0].value.number)); }
#line 966 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 80: // slews: KW_S par_value par_value
#line 426 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { delete (yystack_[1].value.triple);
	  delete (yystack_[0].value.triple);
	}
#line 974 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 81: // slews: KW_S par_value par_value threshold threshold
#line 430 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { delete (yystack_[3].value.triple);
	  delete (yystack_[2].value.triple);
	}
#line 982 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 82: // threshold: pos_number
#line 436 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 988 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 83: // threshold: pos_number ':' pos_number ':' pos_number
#line 437 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[4].value.number); }
#line 994 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 84: // driving_cell: KW_D cell_type
#line 442 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 1000 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 85: // cell_type: IDENT
#line 446 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1006 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 86: // cell_type: INDEX
#line 447 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1012 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 89: // define_entry: DEFINE inst_name entity
#line 459 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[1].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 1020 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 90: // define_entry: DEFINE inst_name inst_name entity
#line 463 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[2].value.string));
	  sta::stringDelete((yystack_[1].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 1029 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 91: // define_entry: PDEFINE physical_inst entity
#line 468 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[1].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 1037 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 92: // entity: QSTRING
#line 474 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1043 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 99: // $@1: %empty
#line 495 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->dspfBegin((yystack_[1].value.net), (yystack_[0].value.triple)); }
#line 1049 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 100: // d_net: D_NET net total_cap $@1 routing_conf conn_sec cap_sec res_sec induc_sec END
#line 497 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->dspfFinish(); }
#line 1055 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 101: // net: name_or_index
#line 502 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.net) = reader->findNet((yystack_[0].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 1063 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 102: // total_cap: par_value
#line 508 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.triple) = (yystack_[0].value.triple); }
#line 1069 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 105: // conf: pos_integer
#line 517 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[0].value.integer); }
#line 1075 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 112: // external_connection: name_or_index
#line 539 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 1081 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 113: // external_connection: physical_inst ':' pport
#line 541 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[2].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 1089 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 114: // internal_connection: pin_name
#line 547 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.pin) = (yystack_[0].value.pin); }
#line 1095 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 115: // pin_name: name_or_index
#line 552 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.pin) = reader->findPin((yystack_[0].value.string));
	  sta::stringDelete((yystack_[0].value.string));
	}
#line 1103 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 119: // internal_parasitic_node: name_or_index
#line 568 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[0].value.string)); }
#line 1109 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 122: // cap_elems: %empty
#line 580 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = 0; }
#line 1115 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 123: // cap_elems: cap_elems cap_elem
#line 581 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[1].value.integer); }
#line 1121 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 124: // cap_elem: cap_id parasitic_node par_value
#line 586 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->makeCapacitor((yystack_[2].value.integer), (yystack_[1].value.string), (yystack_[0].value.triple)); }
#line 1127 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 125: // cap_elem: cap_id parasitic_node parasitic_node par_value
#line 588 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->makeCapacitor((yystack_[3].value.integer), (yystack_[2].value.string), (yystack_[1].value.string), (yystack_[0].value.triple)); }
#line 1133 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 126: // cap_id: pos_integer
#line 592 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[0].value.integer); }
#line 1139 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 127: // parasitic_node: name_or_index
#line 596 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1145 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 130: // res_elems: %empty
#line 608 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = 0; }
#line 1151 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 131: // res_elems: res_elems res_elem
#line 609 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[1].value.integer); }
#line 1157 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 132: // res_elem: res_id parasitic_node parasitic_node par_value
#line 614 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->makeResistor((yystack_[3].value.integer), (yystack_[2].value.string), (yystack_[1].value.string), (yystack_[0].value.triple)); }
#line 1163 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 133: // res_id: pos_integer
#line 618 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[0].value.integer); }
#line 1169 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 136: // induc_elems: %empty
#line 630 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = 0; }
#line 1175 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 137: // induc_elems: induc_elems induc_elem
#line 631 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[1].value.integer); }
#line 1181 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 138: // induc_elem: induc_id parasitic_node parasitic_node par_value
#line 636 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { delete (yystack_[0].value.triple); }
#line 1187 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 139: // induc_id: pos_integer
#line 640 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.integer) = (yystack_[0].value.integer); }
#line 1193 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 140: // $@2: %empty
#line 647 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->rspfBegin((yystack_[1].value.net), (yystack_[0].value.triple)); }
#line 1199 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 141: // r_net: R_NET net total_cap $@2 routing_conf driver_reducs END
#line 649 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->rspfFinish(); }
#line 1205 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 144: // $@3: %empty
#line 659 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->rspfDrvrBegin((yystack_[2].value.pin), (yystack_[0].value.pi));
	  sta::stringDelete((yystack_[1].value.string));
	}
#line 1213 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 145: // driver_reduc: driver_pair driver_cell pi_model $@3 load_desc
#line 663 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->rspfDrvrFinish(); }
#line 1219 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 146: // driver_pair: DRIVER pin_name
#line 668 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.pin) = (yystack_[0].value.pin); }
#line 1225 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 147: // driver_cell: CELL cell_type
#line 673 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1231 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 148: // pi_model: C2_R1_C1 par_value par_value par_value
#line 678 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.pi) = new sta::SpefRspfPi((yystack_[2].value.triple), (yystack_[1].value.triple), (yystack_[0].value.triple)); }
#line 1237 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 152: // rc_desc: RC pin_name par_value
#line 694 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->rspfLoad((yystack_[1].value.pin), (yystack_[0].value.triple)); }
#line 1243 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 153: // rc_desc: RC pin_name par_value pole_residue_desc
#line 696 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { reader->rspfLoad((yystack_[2].value.pin), (yystack_[1].value.triple)); }
#line 1249 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 156: // poles: pole
#line 708 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1255 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 157: // poles: poles pole
#line 709 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[1].value.number); }
#line 1261 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 158: // pole: complex_par_value
#line 713 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1267 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 159: // complex_par_value: cnumber
#line 717 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1273 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 160: // complex_par_value: number
#line 718 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1279 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 161: // complex_par_value: cnumber ':' cnumber ':' cnumber
#line 719 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[4].value.number); }
#line 1285 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 162: // complex_par_value: number ':' number ':' number
#line 720 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[4].value.number); }
#line 1291 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 163: // cnumber: '(' real_component imaginary_component ')'
#line 725 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[2].value.number); }
#line 1297 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 164: // real_component: number
#line 729 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1303 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 165: // imaginary_component: number
#line 733 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1309 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 167: // residues: residue
#line 741 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1315 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 168: // residues: residues residue
#line 742 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[1].value.number); }
#line 1321 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 169: // residue: complex_par_value
#line 746 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1327 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 170: // d_pnet: D_PNET pnet_ref total_cap routing_conf pconn_sec cap_sec res_sec induc_sec END
#line 754 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[7].value.string)); }
#line 1333 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 171: // pnet_ref: name_or_index
#line 758 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1339 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 177: // pexternal_connection: pport_name
#line 776 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1345 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 181: // internal_pdspf_node: name_or_index
#line 790 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        {
	  sta::stringDelete((yystack_[0].value.string));
	  (yylhs.value.string) = 0;
	}
#line 1354 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 182: // name_or_index: IDENT
#line 797 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1360 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 183: // name_or_index: NAME
#line 798 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1366 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 184: // name_or_index: INDEX
#line 799 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1372 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 185: // r_pnet: R_PNET pnet_ref total_cap routing_conf END
#line 806 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[3].value.string)); }
#line 1378 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 186: // r_pnet: R_PNET pnet_ref total_cap routing_conf pdriver_reduc END
#line 808 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { sta::stringDelete((yystack_[4].value.string)); }
#line 1384 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 189: // number: INTEGER
#line 823 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = static_cast<float>((yystack_[0].value.integer)); }
#line 1390 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 190: // number: FLOAT
#line 824 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1396 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 191: // pos_integer: INTEGER
#line 829 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { int value = (yystack_[0].value.integer);
	  if (value < 0)
	    reader->warn(1525, "%d is not positive.", value);
	  (yylhs.value.integer) = value;
	}
#line 1406 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 192: // pos_number: INTEGER
#line 838 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { float value = static_cast<float>((yystack_[0].value.integer));
	  if (value < 0)
	    reader->warn(1526, "%.4f is not positive.", value);
	  (yylhs.value.number) = value;
	}
#line 1416 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;

  case 193: // pos_number: FLOAT
#line 844 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
        { float value = static_cast<float>((yystack_[0].value.number));
	  if (value < 0)
	    reader->warn(1527, "%.4f is not positive.", value);
	  (yylhs.value.number) = value;
	}
#line 1426 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"
    break;


#line 1430 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"

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
  SpefParse::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  SpefParse::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const short SpefParse::yypact_ninf_ = -307;

  const signed char SpefParse::yytable_ninf_ = -113;

  const short
  SpefParse::yypact_[] =
  {
      44,   -12,    66,    56,    80,  -307,  -307,    63,   105,    70,
     124,    65,    63,  -307,    45,    45,    35,   120,  -307,  -307,
      90,   136,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
      45,  -307,  -307,    45,    45,    45,  -307,   117,  -307,  -307,
    -307,    94,   138,  -307,    45,  -307,    96,  -307,    45,  -307,
      98,    82,    84,    49,  -307,  -307,   101,   143,  -307,  -307,
    -307,  -307,  -307,    45,    45,    45,    45,    45,    45,    45,
    -307,    12,  -307,  -307,  -307,  -307,  -307,  -307,   103,   148,
      68,    68,  -307,  -307,    42,  -307,   107,  -307,    77,  -307,
      77,  -307,    77,    77,  -307,  -307,   108,   149,    77,    77,
      77,    76,  -307,  -307,  -307,  -307,  -307,  -307,   107,  -307,
    -307,  -307,  -307,  -307,  -307,   100,   137,  -307,   137,  -307,
     110,    38,   151,    77,  -307,    77,  -307,  -307,  -307,  -307,
     137,    77,   115,   132,   137,   -23,  -307,  -307,  -307,  -307,
    -307,  -307,    38,   155,  -307,    83,   134,   104,  -307,  -307,
    -307,    95,   135,  -307,  -307,    45,   140,   129,  -307,    50,
     161,  -307,  -307,    83,   111,    97,   135,    77,    45,    45,
      95,  -307,  -307,   139,    26,  -307,  -307,  -307,  -307,    76,
     141,  -307,  -307,  -307,  -307,    51,    83,  -307,   165,  -307,
      83,    45,    45,    97,  -307,   139,  -307,  -307,    96,    96,
    -307,   142,   115,  -307,   144,  -307,    45,  -307,   129,  -307,
      77,   150,  -307,  -307,  -307,  -307,  -307,   128,    83,   168,
     121,   122,    96,   133,    96,  -307,   153,   144,    68,    68,
      45,  -307,  -307,    45,  -307,   115,  -307,   162,  -307,   141,
      77,   152,  -307,  -307,   146,    83,   172,    83,    45,  -307,
    -307,    45,  -307,   166,  -307,  -307,   180,  -307,    34,  -307,
    -307,    45,  -307,   115,  -307,  -307,    77,    45,   152,  -307,
    -307,   154,    83,  -307,  -307,  -307,    68,    68,   180,  -307,
    -307,  -307,  -307,    77,    45,  -307,    45,  -307,   150,  -307,
      77,  -307,  -307,   157,  -307,  -307,    77,    45,  -307,   158,
    -307,  -307,    77,   115,  -307,   156,  -307,     9,   115,  -307,
      77,     9,  -307,  -307,   145,   147,     9,    77,  -307,  -307,
     163,    77,  -307,     9,  -307,   164,  -307,   167,   170,  -307,
    -307,   163,    77,  -307,  -307
  };

  const unsigned char
  SpefParse::yydefact_[] =
  {
       0,     0,     0,    34,     0,    16,     1,     0,    42,     0,
       0,     0,    35,    36,     0,     0,    51,    43,    44,    17,
       0,     0,    41,    39,    40,    38,    37,   184,   182,   183,
      46,    48,    50,    47,     0,     0,    87,    52,    53,    45,
      18,     0,     0,    49,    55,    56,     0,    60,    62,    63,
       0,     0,    66,     0,    54,    21,     0,     0,    57,    59,
      70,    64,    70,     0,     0,     0,     0,     0,     0,     0,
      88,     2,    93,    95,    96,    97,    98,    19,     0,     0,
      58,    65,    67,    68,     0,    61,     0,    69,     0,   101,
       0,   171,     0,     0,    94,    20,     0,     0,     0,     0,
       0,     0,    71,    72,    73,    74,    75,    92,     0,    89,
      91,   189,   190,   102,    99,    78,   103,   140,   103,    23,
      22,     0,     0,     0,    77,     0,    86,    85,    84,    90,
     103,     0,     0,     0,   103,     0,    24,    11,    12,    13,
      14,    25,     0,     0,    76,    80,   106,     0,   191,   104,
     105,     0,   120,   142,   185,     0,     0,     0,    26,     0,
       0,   192,   193,     0,    82,     0,   120,     0,     0,     0,
     178,   173,   122,   128,     0,   188,   114,   115,   186,     0,
       0,     3,     4,     5,     6,    27,     0,    15,     0,    81,
       0,     0,     0,   116,   108,   128,    79,   177,     0,     0,
     174,   172,   121,   130,   134,   141,     0,   143,     0,   147,
       0,     0,     7,     8,     9,    10,    28,     0,     0,     0,
       0,     0,     0,    69,     0,   109,   107,   134,     0,     0,
       0,   179,   123,     0,   126,   129,   136,     0,   146,     0,
       0,     0,   187,    30,     0,     0,     0,     0,     0,    70,
      70,     0,   117,     0,   175,   176,     0,   181,     0,   127,
     131,     0,   133,   135,   170,   144,     0,     0,   149,   150,
      31,     0,     0,    29,    83,   113,   110,   111,     0,   119,
     100,   180,   124,     0,     0,   137,     0,   139,     0,   148,
       0,   151,    32,     0,   118,   125,     0,     0,   145,   152,
      33,   132,     0,     0,   153,     0,   138,     0,     0,   154,
       0,   155,   156,   158,   159,   160,     0,     0,   164,   157,
       0,     0,   169,   166,   167,     0,   165,     0,     0,   168,
     163,     0,     0,   161,   162
  };

  const short
  SpefParse::yypgoto_[] =
  {
    -307,  -307,  -307,  -307,    59,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,  -307,  -307,   193,  -307,  -307,  -307,   190,   198,
      -4,  -307,  -307,  -307,   173,  -176,  -307,   159,   181,  -307,
     174,    55,   -22,   -61,   -59,   -91,  -245,  -307,   -94,  -307,
      67,  -307,    52,  -307,  -307,   -71,  -307,   169,  -307,  -307,
     160,    29,  -106,  -307,  -307,  -307,    36,  -307,  -160,  -198,
    -307,  -307,  -307,    72,  -307,  -307,  -307,  -222,    46,  -307,
    -307,  -307,     6,  -307,  -307,  -307,  -307,  -307,  -307,  -307,
    -307,  -307,    31,     3,   -44,  -307,   -21,  -307,  -307,  -307,
     -66,  -260,  -306,  -307,  -307,  -307,  -307,   -77,  -307,   179,
    -307,  -307,    79,  -307,  -307,  -307,  -307,   -14,  -307,  -307,
    -307,   -96,  -195,  -173
  };

  const short
  SpefParse::yydefgoto_[] =
  {
       0,     2,   185,   216,   141,     3,     4,    10,    21,    57,
      79,    42,    97,   120,   122,   143,   160,   187,   188,   219,
     246,   273,     8,    12,    13,    25,    16,    17,    18,    30,
      31,    36,    37,    44,    45,    60,    46,    84,    38,    48,
      49,    50,    82,    51,    80,   102,   103,   104,   113,   105,
     163,   106,   128,    53,    70,   109,    71,    72,    73,   130,
      88,   114,   133,   149,   166,   193,   194,   222,   175,   176,
     226,   252,   278,   173,   202,   232,   233,   258,   204,   235,
     260,   261,   237,   263,   285,   286,    74,   134,   174,   207,
     288,   208,   180,   211,   242,   268,   269,   304,   305,   311,
     312,   313,   314,   317,   325,   309,   323,   324,    75,    90,
     152,   170,   171,   198,   201,   231,   256,   259,    76,   156,
     157,   115,   150,   164
  };

  const short
  SpefParse::yytable_[] =
  {
      32,    32,   123,    81,    86,   124,   125,   234,   238,   199,
     154,   281,   135,   217,   327,   110,    32,   220,   155,    32,
      47,    52,   228,   229,   146,   333,    43,   144,   153,    43,
      47,   145,   224,   294,    52,   147,   283,   129,     5,   284,
     262,    66,    67,    68,    69,   244,   249,     1,   250,    83,
      85,    87,    89,    91,    89,    91,   322,   111,   112,   205,
      34,    35,   296,   322,   297,   310,     6,   206,   287,   290,
      85,   196,   271,     7,   274,   302,    64,    65,    66,    67,
      68,    69,   111,   112,     9,    27,    28,    29,    98,    99,
     100,   101,   107,    27,    28,    29,    27,    28,    29,   293,
     137,   138,   139,   140,   181,   182,   183,   184,   307,   212,
     213,   214,   215,   316,    11,    22,   240,    23,    24,   116,
      19,   117,   118,    14,    15,   111,   112,   126,   127,    20,
     221,   161,   162,   168,   169,   191,   192,   254,   255,    15,
      40,   177,    41,    35,    55,    56,   266,    63,    59,   -69,
      62,    77,    78,    95,    52,   177,    96,   107,   119,   121,
     136,   132,   142,   148,   282,   131,   151,   159,   165,   167,
     172,   179,   289,   178,   186,   203,   190,   223,   177,   218,
     243,   236,   230,   245,   210,  -112,   247,   248,   272,   295,
     276,   277,   177,   251,   241,   264,   299,   267,   270,   280,
      98,   158,   301,   308,   303,    26,   292,    39,   306,   300,
     320,   315,   321,    33,   318,   315,   257,    58,    54,   310,
     315,   326,    61,   197,   330,   328,   275,   315,    92,   225,
     189,   209,   331,   253,    83,   332,   334,   279,   195,   239,
      94,   227,   265,   108,   298,   319,   329,   291,    93,   200,
       0,     0,     0,   177
  };

  const short
  SpefParse::yycheck_[] =
  {
      14,    15,    98,    62,    65,    99,   100,   202,   206,   169,
      33,   256,   118,   186,   320,    86,    30,   190,    41,    33,
      34,    35,   198,   199,   130,   331,    30,   123,   134,    33,
      44,   125,   192,   278,    48,   131,   258,   108,    50,   261,
     235,    29,    30,    31,    32,   218,   222,     3,   224,    63,
      64,    65,    66,    67,    68,    69,   316,    48,    49,    33,
      25,    26,   284,   323,   286,    56,     0,    41,   263,   267,
      84,   167,   245,    17,   247,   297,    27,    28,    29,    30,
      31,    32,    48,    49,     4,    51,    52,    53,    20,    21,
      22,    23,    50,    51,    52,    53,    51,    52,    53,   272,
      62,    63,    64,    65,    54,    55,    56,    57,   303,    58,
      59,    60,    61,   308,    51,    50,   210,    52,    53,    90,
      50,    92,    93,    18,    19,    48,    49,    51,    52,     5,
     191,    48,    49,    38,    39,    38,    39,   228,   229,    19,
      50,   155,     6,    26,    50,     7,   240,    65,    52,    65,
      52,    50,     9,    50,   168,   169,     8,    50,    50,    10,
      50,    24,    11,    48,   258,    65,    34,    12,    34,    65,
      35,    42,   266,    33,    13,    36,    65,   191,   192,    14,
      52,    37,    40,    15,    43,    52,    65,    65,    16,   283,
     249,   250,   206,    40,    44,    33,   290,    45,    52,    33,
      20,   142,   296,    47,    46,    12,    52,    17,   302,    52,
      65,   307,    65,    15,   310,   311,   230,    44,    37,    56,
     316,   317,    48,   168,    60,   321,   248,   323,    68,   193,
     163,   179,    65,   227,   248,    65,   332,   251,   166,   208,
      71,   195,   239,    84,   288,   311,   323,   268,    69,   170,
      -1,    -1,    -1,   267
  };

  const unsigned char
  SpefParse::yystos_[] =
  {
       0,     3,    67,    71,    72,    50,     0,    17,    88,     4,
      73,    51,    89,    90,    18,    19,    92,    93,    94,    50,
       5,    74,    50,    52,    53,    91,    90,    51,    52,    53,
      95,    96,   183,    95,    25,    26,    97,    98,   104,    94,
      50,     6,    77,    96,    99,   100,   102,   183,   105,   106,
     107,   109,   183,   119,   104,    50,     7,    75,   100,    52,
     101,   106,    52,    65,    27,    28,    29,    30,    31,    32,
     120,   122,   123,   124,   152,   174,   184,    50,     9,    76,
     110,   110,   108,   183,   103,   183,   109,   183,   126,   183,
     175,   183,   126,   175,   123,    50,     8,    78,    20,    21,
      22,    23,   111,   112,   113,   115,   117,    50,   103,   121,
     121,    48,    49,   114,   127,   187,   127,   127,   127,    50,
      79,    10,    80,   187,   114,   114,    51,    52,   118,   121,
     125,    65,    24,   128,   153,   128,    50,    62,    63,    64,
      65,    70,    11,    81,   187,   114,   128,   187,    48,   129,
     188,    34,   176,   128,    33,    41,   185,   186,    70,    12,
      82,    48,    49,   116,   189,    34,   130,    65,    38,    39,
     177,   178,    35,   139,   154,   134,   135,   183,    33,    42,
     158,    54,    55,    56,    57,    68,    13,    83,    84,   116,
      65,    38,    39,   131,   132,   139,   187,   107,   179,   134,
     178,   180,   140,    36,   144,    33,    41,   155,   157,   118,
      43,   159,    58,    59,    60,    61,    69,   189,    14,    85,
     189,   109,   133,   183,   134,   132,   136,   144,   101,   101,
      40,   181,   141,   142,   188,   145,    37,   148,   135,   158,
     114,    44,   160,    52,   189,    15,    86,    65,    65,   101,
     101,    40,   137,   148,   111,   111,   182,   183,   143,   183,
     146,   147,   188,   149,    33,   159,   114,    45,   161,   162,
      52,   189,    16,    87,   189,   108,   110,   110,   138,   183,
      33,   112,   114,   143,   143,   150,   151,   188,   156,   114,
     135,   162,    52,   189,   112,   114,   143,   143,   160,   114,
      52,   114,   143,    46,   163,   164,   114,   188,    47,   171,
      56,   165,   166,   167,   168,   187,   188,   169,   187,   166,
      65,    65,   167,   172,   173,   170,   187,   168,   187,   173,
      60,    65,    65,   168,   187
  };

  const unsigned char
  SpefParse::yyr1_[] =
  {
       0,    66,    67,    68,    68,    68,    68,    69,    69,    69,
      69,    70,    70,    70,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    79,    80,    81,    82,    82,    83,
      84,    85,    86,    87,    88,    88,    89,    89,    90,    91,
      91,    91,    92,    92,    92,    92,    93,    94,    95,    95,
      96,    97,    97,    97,    97,    98,    99,    99,   100,   101,
     102,   103,   104,   105,   105,   106,   107,   107,   108,   109,
     110,   110,   111,   111,   111,   111,   112,   113,   114,   114,
     115,   115,   116,   116,   117,   118,   118,   119,   119,   120,
     120,   120,   121,   122,   122,   123,   123,   123,   123,   125,
     124,   126,   127,   128,   128,   129,   130,   130,   131,   131,
     132,   132,   133,   133,   134,   135,   136,   136,   137,   138,
     139,   139,   140,   140,   141,   141,   142,   143,   144,   144,
     145,   145,   146,   147,   148,   148,   149,   149,   150,   151,
     153,   152,   154,   154,   156,   155,   157,   158,   159,   160,
     161,   161,   162,   162,   163,   164,   165,   165,   166,   167,
     167,   167,   167,   168,   169,   170,   171,   172,   172,   173,
     174,   175,   176,   177,   177,   178,   178,   179,   180,   180,
     181,   182,   183,   183,   183,   184,   184,   185,   186,   187,
     187,   188,   189,   189
  };

  const signed char
  SpefParse::yyr2_[] =
  {
       0,     2,     6,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,    11,     2,     2,     2,     2,
       2,     2,     2,     1,     2,     2,     2,     2,     3,     4,
       3,     3,     3,     3,     0,     2,     1,     2,     2,     1,
       1,     1,     0,     1,     1,     2,     2,     2,     1,     2,
       1,     0,     1,     1,     2,     2,     1,     2,     3,     1,
       1,     1,     2,     1,     2,     3,     1,     3,     1,     1,
       0,     2,     1,     1,     1,     1,     3,     2,     1,     5,
       3,     5,     1,     5,     2,     1,     1,     0,     2,     3,
       4,     3,     1,     1,     2,     1,     1,     1,     1,     0,
      10,     1,     1,     0,     2,     1,     0,     3,     1,     2,
       4,     4,     1,     3,     1,     1,     0,     2,     3,     1,
       0,     2,     0,     2,     3,     4,     1,     1,     0,     2,
       0,     2,     4,     1,     0,     2,     0,     2,     4,     1,
       0,     7,     0,     2,     0,     5,     2,     2,     4,     2,
       1,     2,     3,     4,     2,     3,     1,     2,     1,     1,
       1,     5,     5,     4,     1,     1,     3,     1,     2,     1,
       9,     1,     3,     1,     2,     4,     4,     1,     0,     2,
       3,     1,     1,     1,     1,     5,     6,     4,     2,     1,
       1,     1,     1,     1
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const SpefParse::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "SPEF", "DESIGN",
  "DATE", "VENDOR", "PROGRAM", "DESIGN_FLOW", "PVERSION", "DIVIDER",
  "DELIMITER", "BUS_DELIMITER", "T_UNIT", "C_UNIT", "R_UNIT", "L_UNIT",
  "NAME_MAP", "POWER_NETS", "GROUND_NETS", "KW_C", "KW_L", "KW_S", "KW_D",
  "KW_V", "PORTS", "PHYSICAL_PORTS", "DEFINE", "PDEFINE", "D_NET",
  "D_PNET", "R_NET", "R_PNET", "END", "CONN", "CAP", "RES", "INDUC",
  "KW_P", "KW_I", "KW_N", "DRIVER", "CELL", "C2_R1_C1", "LOADS", "RC",
  "KW_Q", "KW_K", "INTEGER", "FLOAT", "QSTRING", "INDEX", "IDENT", "NAME",
  "'['", "'{'", "'('", "'<'", "']'", "'}'", "')'", "'>'", "'.'", "'/'",
  "'|'", "':'", "$accept", "file", "prefix_bus_delim", "suffix_bus_delim",
  "hchar", "header_def", "spef_version", "design_name", "date",
  "program_name", "program_version", "vendor", "design_flow", "qstrings",
  "hierarchy_div_def", "pin_delim_def", "bus_delim_def", "unit_def",
  "time_scale", "cap_scale", "res_scale", "induc_scale", "name_map",
  "name_map_entries", "name_map_entry", "mapped_item", "power_def",
  "power_net_def", "ground_net_def", "net_names", "net_name",
  "external_def", "port_def", "port_entries", "port_entry", "direction",
  "port_name", "inst_name", "physical_port_def", "pport_entries",
  "pport_entry", "pport_name", "pport", "physical_inst", "conn_attrs",
  "conn_attr", "coordinates", "cap_load", "par_value", "slews",
  "threshold", "driving_cell", "cell_type", "define_def", "define_entry",
  "entity", "internal_def", "nets", "d_net", "$@1", "net", "total_cap",
  "routing_conf", "conf", "conn_sec", "conn_defs", "conn_def",
  "external_connection", "internal_connection", "pin_name",
  "internal_node_coords", "internal_node_coord", "internal_parasitic_node",
  "cap_sec", "cap_elems", "cap_elem", "cap_id", "parasitic_node",
  "res_sec", "res_elems", "res_elem", "res_id", "induc_sec", "induc_elems",
  "induc_elem", "induc_id", "r_net", "$@2", "driver_reducs",
  "driver_reduc", "$@3", "driver_pair", "driver_cell", "pi_model",
  "load_desc", "rc_descs", "rc_desc", "pole_residue_desc", "pole_desc",
  "poles", "pole", "complex_par_value", "cnumber", "real_component",
  "imaginary_component", "residue_desc", "residues", "residue", "d_pnet",
  "pnet_ref", "pconn_sec", "pconn_defs", "pconn_def",
  "pexternal_connection", "internal_pnode_coords", "internal_pnode_coord",
  "internal_pdspf_node", "name_or_index", "r_pnet", "pdriver_reduc",
  "pdriver_pair", "number", "pos_integer", "pos_number", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  SpefParse::yyrline_[] =
  {
       0,   127,   127,   138,   140,   142,   144,   149,   151,   153,
     155,   160,   162,   164,   166,   173,   187,   192,   197,   202,
     207,   212,   217,   222,   226,   231,   236,   241,   243,   250,
     257,   262,   267,   272,   278,   280,   284,   285,   289,   294,
     295,   296,   301,   303,   304,   305,   309,   313,   317,   318,
     322,   328,   330,   331,   332,   336,   340,   341,   345,   350,
     357,   361,   365,   369,   370,   374,   378,   380,   387,   391,
     396,   398,   402,   403,   404,   405,   409,   413,   418,   420,
     425,   429,   436,   437,   441,   446,   447,   452,   454,   458,
     462,   467,   474,   480,   481,   485,   486,   487,   488,   495,
     494,   501,   508,   511,   513,   517,   522,   524,   528,   529,
     533,   534,   538,   540,   547,   551,   557,   559,   563,   567,
     573,   575,   580,   581,   585,   587,   592,   596,   601,   603,
     608,   609,   613,   618,   623,   625,   630,   631,   635,   640,
     647,   646,   652,   654,   659,   658,   667,   672,   677,   684,
     688,   689,   693,   695,   700,   704,   708,   709,   713,   717,
     718,   719,   720,   724,   729,   733,   737,   741,   742,   746,
     752,   758,   762,   766,   767,   771,   772,   776,   779,   781,
     785,   789,   797,   798,   799,   805,   807,   812,   816,   822,
     824,   828,   837,   843
  };

  void
  SpefParse::yy_stack_print_ () const
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
  SpefParse::yy_reduce_print_ (int yyrule) const
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

  SpefParse::symbol_kind_type
  SpefParse::yytranslate_ (int t) YY_NOEXCEPT
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
      56,    60,     2,     2,     2,     2,    62,    63,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    65,     2,
      57,     2,    61,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    54,     2,    58,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    55,    64,    59,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53
    };
    // Last valid token kind.
    const int code_max = 308;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 52 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"
} // sta
#line 2046 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SpefParse.cc"

#line 851 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/parasitics/SpefParse.yy"

