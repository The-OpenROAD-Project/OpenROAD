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
#line 25 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"

#include <cctype>

#include "sdf/SdfReaderPvt.hh"
#include "sdf/SdfScanner.hh"

#undef yylex
#define yylex scanner->lex

// warning: variable 'yynerrs_' set but not used
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

void
sta::SdfParse::error(const location_type &loc,
                     const std::string &msg)
{
  reader->report()->fileError(164,reader->filename().c_str(),
                              loc.begin.line,"%s",msg.c_str());
}

#line 62 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"


#include "SdfParse.hh"




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

#line 49 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
namespace sta {
#line 160 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"

  /// Build a parser object.
  SdfParse::SdfParse (SdfScanner *scanner_yyarg, SdfReader *reader_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      reader (reader_yyarg)
  {}

  SdfParse::~SdfParse ()
  {}

  SdfParse::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/

  // basic_symbol.
  template <typename Base>
  SdfParse::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  SdfParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  SdfParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (value_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}


  template <typename Base>
  SdfParse::symbol_kind_type
  SdfParse::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  SdfParse::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  SdfParse::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_kind.
  SdfParse::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  SdfParse::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  SdfParse::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  SdfParse::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  void
  SdfParse::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  SdfParse::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  SdfParse::symbol_kind_type
  SdfParse::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  SdfParse::symbol_kind_type
  SdfParse::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }



  // by_state.
  SdfParse::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  SdfParse::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  SdfParse::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  SdfParse::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  SdfParse::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  SdfParse::symbol_kind_type
  SdfParse::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  SdfParse::stack_symbol_type::stack_symbol_type ()
  {}

  SdfParse::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  SdfParse::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  SdfParse::stack_symbol_type&
  SdfParse::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  SdfParse::stack_symbol_type&
  SdfParse::stack_symbol_type::operator= (stack_symbol_type& that)
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
  SdfParse::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    switch (yysym.kind ())
    {
      case symbol_kind::S_QSTRING: // QSTRING
#line 93 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                    { delete (yysym.value.string); }
#line 373 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
        break;

      default:
        break;
    }
  }

#if YYDEBUG
  template <typename Base>
  void
  SdfParse::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
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
  SdfParse::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  SdfParse::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  SdfParse::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  SdfParse::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  SdfParse::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  SdfParse::debug_level_type
  SdfParse::debug_level () const
  {
    return yydebug_;
  }

  void
  SdfParse::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  SdfParse::state_type
  SdfParse::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  SdfParse::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  SdfParse::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  SdfParse::operator() ()
  {
    return parse ();
  }

  int
  SdfParse::parse ()
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
  case 2: // file: '(' DELAYFILE header cells ')'
#line 100 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                        {}
#line 644 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 5: // header_stmt: '(' SDFVERSION QSTRING ')'
#line 110 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                   { delete (yystack_[1].value.string); }
#line 650 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 6: // header_stmt: '(' DESIGN QSTRING ')'
#line 111 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                               { delete (yystack_[1].value.string); }
#line 656 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 7: // header_stmt: '(' DATE QSTRING ')'
#line 112 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                             { delete (yystack_[1].value.string); }
#line 662 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 8: // header_stmt: '(' VENDOR QSTRING ')'
#line 113 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                               { delete (yystack_[1].value.string); }
#line 668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 9: // header_stmt: '(' PROGRAM QSTRING ')'
#line 114 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                { delete (yystack_[1].value.string); }
#line 674 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 10: // header_stmt: '(' PVERSION QSTRING ')'
#line 115 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                 { delete (yystack_[1].value.string); }
#line 680 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 11: // header_stmt: '(' DIVIDER hchar ')'
#line 116 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                              { reader->setDivider((yystack_[1].value.character)); }
#line 686 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 12: // header_stmt: '(' VOLTAGE triple ')'
#line 117 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                               { reader->deleteTriple((yystack_[1].value.triple)); }
#line 692 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 15: // header_stmt: '(' PROCESS QSTRING ')'
#line 120 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                { delete (yystack_[1].value.string); }
#line 698 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 18: // header_stmt: '(' TEMPERATURE triple ')'
#line 123 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                   { reader->deleteTriple((yystack_[1].value.triple)); }
#line 704 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 20: // header_stmt: '(' TIMESCALE NUMBER ID ')'
#line 125 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                                    { reader->setTimescale((yystack_[2].value.number), (yystack_[1].value.string)); }
#line 710 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 21: // hchar: '/'
#line 130 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.character) = '/'; }
#line 716 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 22: // hchar: '.'
#line 132 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.character) = '.'; }
#line 722 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 23: // number_opt: %empty
#line 135 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
            { (yylhs.value.number_ptr) = nullptr; }
#line 728 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 24: // number_opt: NUMBER
#line 136 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                { (yylhs.value.number_ptr) = new float((yystack_[0].value.number)); }
#line 734 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 27: // cell: '(' CELL celltype cell_instance timing_specs ')'
#line 146 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->cellFinish(); }
#line 740 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 28: // celltype: '(' CELLTYPE QSTRING ')'
#line 151 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->setCell((yystack_[1].value.string)); }
#line 746 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 29: // cell_instance: '(' INSTANCE ')'
#line 156 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->setInstance(nullptr); }
#line 752 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 30: // cell_instance: '(' INSTANCE '*' ')'
#line 158 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->setInstanceWildcard(); }
#line 758 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 31: // cell_instance: '(' INSTANCE path ')'
#line 160 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->setInstance((yystack_[1].value.string)); }
#line 764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 38: // $@1: %empty
#line 179 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->setInIncremental(false); }
#line 770 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 40: // $@2: %empty
#line 182 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->setInIncremental(true); }
#line 776 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 44: // path: ID
#line 192 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.string) = reader->unescaped((yystack_[0].value.string)); }
#line 782 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 45: // path: path hchar ID
#line 194 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.string) = reader->makePath((yystack_[2].value.string), reader->unescaped((yystack_[0].value.string))); }
#line 788 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 46: // del_def: '(' IOPATH port_spec port_instance retains delval_list ')'
#line 199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->iopath((yystack_[4].value.port_spec), (yystack_[3].value.string), (yystack_[1].value.delval_list), nullptr, false); }
#line 794 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 47: // del_def: '(' CONDELSE '(' IOPATH port_spec port_instance retains delval_list ')' ')'
#line 202 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->iopath((yystack_[5].value.port_spec), (yystack_[4].value.string), (yystack_[2].value.delval_list), nullptr, true); }
#line 800 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 48: // del_def: '(' COND EXPR_OPEN_IOPATH port_spec port_instance retains delval_list ')' ')'
#line 205 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->iopath((yystack_[5].value.port_spec), (yystack_[4].value.string), (yystack_[2].value.delval_list), (yystack_[6].value.string), false); }
#line 806 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 49: // del_def: '(' INTERCONNECT port_instance port_instance delval_list ')'
#line 207 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->interconnect((yystack_[3].value.string), (yystack_[2].value.string), (yystack_[1].value.delval_list)); }
#line 812 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 50: // del_def: '(' PORT port_instance delval_list ')'
#line 209 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->port((yystack_[2].value.string), (yystack_[1].value.delval_list)); }
#line 818 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 51: // del_def: '(' DEVICE delval_list ')'
#line 211 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->device((yystack_[1].value.delval_list)); }
#line 824 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 52: // del_def: '(' DEVICE port_instance delval_list ')'
#line 213 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->device((yystack_[2].value.string), (yystack_[1].value.delval_list)); }
#line 830 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 55: // retain: '(' RETAIN delval_list ')'
#line 223 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->deleteTripleSeq((yystack_[1].value.delval_list)); }
#line 836 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 56: // delval_list: value
#line 228 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.delval_list) = reader->makeTripleSeq(); (yylhs.value.delval_list)->push_back((yystack_[0].value.triple)); }
#line 842 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 57: // delval_list: delval_list value
#line 230 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yystack_[1].value.delval_list)->push_back((yystack_[0].value.triple)); (yylhs.value.delval_list) = (yystack_[1].value.delval_list); }
#line 848 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 60: // $@3: %empty
#line 238 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                  { reader->setInTimingCheck(true); }
#line 854 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 61: // tchk_def: '(' SETUP $@3 port_tchk port_tchk value ')'
#line 240 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheck(sta::TimingRole::setup(), (yystack_[3].value.port_spec), (yystack_[2].value.port_spec), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 862 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 62: // $@4: %empty
#line 243 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                 { reader->setInTimingCheck(true); }
#line 868 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 63: // tchk_def: '(' HOLD $@4 port_tchk port_tchk value ')'
#line 245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheck(sta::TimingRole::hold(), (yystack_[3].value.port_spec), (yystack_[2].value.port_spec), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 876 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 64: // $@5: %empty
#line 248 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                      { reader->setInTimingCheck(true); }
#line 882 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 65: // tchk_def: '(' SETUPHOLD $@5 port_tchk port_tchk value value ')'
#line 250 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheckSetupHold((yystack_[4].value.port_spec), (yystack_[3].value.port_spec), (yystack_[2].value.triple), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 890 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 66: // $@6: %empty
#line 253 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                     { reader->setInTimingCheck(true); }
#line 896 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 67: // tchk_def: '(' RECOVERY $@6 port_tchk port_tchk value ')'
#line 255 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheck(sta::TimingRole::recovery(),(yystack_[3].value.port_spec),(yystack_[2].value.port_spec),(yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 904 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 68: // $@7: %empty
#line 258 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                    { reader->setInTimingCheck(true); }
#line 910 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 69: // tchk_def: '(' REMOVAL $@7 port_tchk port_tchk value ')'
#line 260 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheck(sta::TimingRole::removal(),(yystack_[3].value.port_spec),(yystack_[2].value.port_spec),(yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 918 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 70: // $@8: %empty
#line 263 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                   { reader->setInTimingCheck(true); }
#line 924 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 71: // tchk_def: '(' RECREM $@8 port_tchk port_tchk value value ')'
#line 265 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheckRecRem((yystack_[4].value.port_spec), (yystack_[3].value.port_spec), (yystack_[2].value.triple), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 932 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 72: // $@9: %empty
#line 268 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                 { reader->setInTimingCheck(true); }
#line 938 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 73: // tchk_def: '(' SKEW $@9 port_tchk port_tchk value ')'
#line 271 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheck(sta::TimingRole::skew(),(yystack_[2].value.port_spec),(yystack_[3].value.port_spec),(yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 946 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 74: // $@10: %empty
#line 274 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                  { reader->setInTimingCheck(true); }
#line 952 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 75: // tchk_def: '(' WIDTH $@10 port_tchk value ')'
#line 276 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheckWidth((yystack_[2].value.port_spec), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 960 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 76: // $@11: %empty
#line 279 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                   { reader->setInTimingCheck(true); }
#line 966 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 77: // tchk_def: '(' PERIOD $@11 port_tchk value ')'
#line 281 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheckPeriod((yystack_[2].value.port_spec), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 974 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 78: // $@12: %empty
#line 284 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                     { reader->setInTimingCheck(true); }
#line 980 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 79: // tchk_def: '(' NOCHANGE $@12 port_tchk port_tchk value value ')'
#line 286 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { reader->timingCheckNochange((yystack_[4].value.port_spec), (yystack_[3].value.port_spec), (yystack_[2].value.triple), (yystack_[1].value.triple));
	  reader->setInTimingCheck(false);
	}
#line 988 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 80: // port: ID
#line 293 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.string) = reader->unescaped((yystack_[0].value.string)); }
#line 994 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 81: // port: ID '[' DNUMBER ']'
#line 295 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.string) = reader->makeBusName((yystack_[3].value.string), (yystack_[1].value.integer)); }
#line 1000 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 82: // port_instance: port
#line 299 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 1006 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 83: // port_instance: path hchar port
#line 301 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.string) = reader->makePath((yystack_[2].value.string), (yystack_[0].value.string)); }
#line 1012 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 84: // port_spec: port_instance
#line 306 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.port_spec)=reader->makePortSpec(sta::Transition::riseFall(),(yystack_[0].value.string),nullptr); }
#line 1018 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 85: // port_spec: '(' port_transition port_instance ')'
#line 308 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.port_spec) = reader->makePortSpec((yystack_[2].value.transition), (yystack_[1].value.string), nullptr); }
#line 1024 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 86: // port_transition: POSEDGE
#line 312 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                { (yylhs.value.transition) = sta::Transition::rise(); }
#line 1030 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 87: // port_transition: NEGEDGE
#line 313 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                { (yylhs.value.transition) = sta::Transition::fall(); }
#line 1036 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 88: // port_tchk: port_spec
#line 317 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.port_spec) = (yystack_[0].value.port_spec); }
#line 1042 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 89: // port_tchk: '(' COND EXPR_ID_CLOSE
#line 319 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.port_spec) = reader->makeCondPortSpec((yystack_[0].value.string)); }
#line 1048 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 90: // port_tchk: '(' COND EXPR_OPEN port_transition port_instance ')' ')'
#line 321 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.port_spec) = reader->makePortSpec((yystack_[3].value.transition), (yystack_[2].value.string), (yystack_[4].value.string)); }
#line 1054 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 91: // value: '(' ')'
#line 326 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        {
	  (yylhs.value.triple) = reader->makeTriple();
	}
#line 1062 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 92: // value: '(' NUMBER ')'
#line 330 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        {
	  (yylhs.value.triple) = reader->makeTriple((yystack_[1].value.number));
	}
#line 1070 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 93: // value: '(' triple ')'
#line 333 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
                       { (yylhs.value.triple) = (yystack_[1].value.triple); }
#line 1076 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 94: // triple: NUMBER ':' number_opt ':' number_opt
#line 338 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        {
	  float *fp = new float((yystack_[4].value.number));
	  (yylhs.value.triple) = reader->makeTriple(fp, (yystack_[2].value.number_ptr), (yystack_[0].value.number_ptr));
	}
#line 1085 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 95: // triple: number_opt ':' NUMBER ':' number_opt
#line 343 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        {
	  float *fp = new float((yystack_[2].value.number));
	  (yylhs.value.triple) = reader->makeTriple((yystack_[4].value.number_ptr), fp, (yystack_[0].value.number_ptr));
	}
#line 1094 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 96: // triple: number_opt ':' number_opt ':' NUMBER
#line 348 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        {
	  float *fp = new float((yystack_[0].value.number));
	  (yylhs.value.triple) = reader->makeTriple((yystack_[4].value.number_ptr), (yystack_[2].value.number_ptr), fp);
	}
#line 1103 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 97: // NUMBER: FNUMBER
#line 355 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.number) = (yystack_[0].value.number); }
#line 1109 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 98: // NUMBER: DNUMBER
#line 357 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.number) = static_cast<float>((yystack_[0].value.integer)); }
#line 1115 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;

  case 99: // NUMBER: '-' DNUMBER
#line 359 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
        { (yylhs.value.number) = static_cast<float>(-(yystack_[0].value.integer)); }
#line 1121 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"
    break;


#line 1125 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"

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
  SdfParse::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  SdfParse::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const short SdfParse::yypact_ninf_ = -188;

  const signed char SdfParse::yytable_ninf_ = -46;

  const short
  SdfParse::yypact_[] =
  {
     -11,    44,    61,    16,  -188,   146,    18,  -188,    38,    40,
      64,    66,    77,    82,   -17,   -32,    -9,   -18,    76,   132,
    -188,   133,  -188,    84,    99,   124,   166,   167,   168,  -188,
    -188,   169,  -188,  -188,  -188,    49,    70,   171,   -19,   173,
    -188,  -188,   174,    34,   177,   176,   210,  -188,  -188,  -188,
    -188,  -188,  -188,  -188,  -188,  -188,  -188,    76,  -188,  -188,
      76,  -188,  -188,  -188,   178,   212,   181,   175,   179,   180,
    -188,  -188,   185,   214,  -188,    76,    76,    76,   183,    -1,
     135,  -188,  -188,  -188,  -188,  -188,  -188,   184,   126,    36,
    -188,  -188,  -188,  -188,   194,  -188,  -188,  -188,   137,   139,
     170,  -188,  -188,   134,  -188,  -188,  -188,  -188,  -188,  -188,
    -188,  -188,  -188,  -188,  -188,  -188,  -188,  -188,  -188,  -188,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
     143,   145,    71,   141,   -17,  -188,  -188,  -188,    52,    52,
      52,    52,    52,    52,   189,   189,    52,    52,    51,  -188,
    -188,  -188,   195,  -188,  -188,   149,   196,   198,   189,   189,
     189,   189,   189,   189,    43,   192,   193,   189,   189,   196,
     196,    53,    54,   199,   197,   200,   160,  -188,   201,   121,
    -188,   202,   203,   189,   204,   206,   189,  -188,   207,    48,
    -188,  -188,   208,   189,   196,   189,   151,   189,  -188,   160,
     196,    54,   218,  -188,   196,  -188,  -188,  -188,   209,  -188,
    -188,   211,  -188,  -188,  -188,   213,   189,   153,  -188,  -188,
     155,  -188,   196,    54,   215,  -188,  -188,  -188,   157,  -188,
    -188,   217,  -188,   196,   219,  -188,    26,  -188,   159,   217,
    -188,  -188,   189,  -188,   161,   217,   163,   220,   165,  -188,
    -188,   221,  -188
  };

  const signed char
  SdfParse::yydefact_[] =
  {
       0,     0,     0,     0,     1,     0,     0,     3,     0,     0,
       0,     0,     0,     0,     0,    23,     0,    23,     0,     0,
       4,     0,    25,     0,     0,     0,     0,     0,     0,    21,
      22,     0,    97,    98,    14,     0,     0,     0,     0,     0,
      16,    19,     0,     0,     0,     0,     0,     2,    26,     5,
       6,     7,     8,     9,    10,    11,    99,    23,    12,    13,
      23,    15,    18,    17,     0,     0,     0,     0,     0,     0,
      24,    20,     0,     0,    32,     0,    23,    23,     0,     0,
       0,    96,    95,    94,    28,    44,    29,     0,     0,     0,
      27,    33,    30,    31,     0,    36,    58,    45,     0,     0,
       0,    34,    37,     0,    35,    59,    38,    40,    60,    62,
      64,    66,    68,    70,    74,    76,    72,    78,    42,    42,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    80,     0,     0,    82,    84,    88,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    39,
      43,    41,     0,    86,    87,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    23,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    89,     0,    80,
      83,     0,     0,     0,     0,     0,     0,    91,     0,     0,
      75,    77,     0,     0,     0,     0,     0,     0,    56,     0,
       0,     0,     0,    81,     0,    85,    61,    63,     0,    67,
      69,     0,    93,    92,    73,     0,     0,     0,    51,    57,
       0,    53,     0,     0,     0,    65,    71,    79,     0,    50,
      52,     0,    53,     0,     0,    49,    23,    54,     0,     0,
      53,    90,     0,    46,     0,     0,     0,     0,     0,    55,
      48,     0,    47
  };

  const short
  SdfParse::yypgoto_[] =
  {
    -188,  -188,  -188,   240,   -75,     8,  -188,   226,  -188,  -188,
    -188,  -188,  -188,  -188,  -188,  -188,   129,   182,  -188,  -169,
    -188,  -187,  -188,  -188,  -188,  -188,  -188,  -188,  -188,  -188,
    -188,  -188,  -188,  -188,    92,  -155,  -166,    83,   -12,  -140,
      -8,   -15
  };

  const unsigned char
  SdfParse::yydefgoto_[] =
  {
       0,     2,     6,     7,    31,    36,    21,    22,    66,    74,
      80,    91,    98,   102,   118,   119,   130,   134,   150,   231,
     237,   196,    99,   105,   120,   121,   122,   123,   124,   125,
     128,   126,   127,   129,   135,   136,   137,   156,   138,   198,
     188,    70
  };

  const short
  SdfParse::yytable_[] =
  {
      38,   178,    43,    44,   165,   166,   200,    37,   217,    42,
     220,    32,    33,    94,   194,   195,   197,    34,   181,   182,
     183,   184,   185,   186,    35,    32,    33,   192,   193,   228,
      59,    41,    39,    29,    30,   222,    60,     1,    35,   216,
      40,    85,    68,   208,   238,   221,   211,     3,    86,   224,
     242,    87,   244,   215,    95,   246,   219,   233,   248,   157,
      81,     4,    96,   239,     5,    67,    19,   232,    69,    32,
      33,   245,   169,   170,   171,   187,   172,   219,   240,    23,
     219,    24,    35,    63,    82,    83,    32,    33,   219,    60,
     173,   174,   187,    56,   132,   132,   132,   213,   219,    35,
     133,   164,   199,    60,   219,    25,   219,    26,   219,   139,
     140,   141,   142,   143,   144,   145,   146,   147,    27,    32,
      33,   -44,   -44,    28,   152,    57,   158,   159,   160,   161,
     162,   163,    35,    49,   167,   168,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    45,    50,   189,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,   108,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   -45,   -45,    51,   152,    93,    29,    30,   153,   154,
     155,    46,    47,    89,    90,   100,   101,   103,   104,   106,
     107,   148,   149,   148,   151,   176,   177,   153,   154,   164,
     218,   164,   229,   164,   230,   164,   235,   164,   243,   164,
     247,   164,   249,   164,   251,    52,    53,    54,    55,    64,
      58,   189,    61,    62,    65,    45,    78,    71,    72,    73,
      75,    79,    84,    92,    76,    77,    97,   164,   132,   175,
     179,   190,   191,   223,   201,   202,    20,    48,   131,   180,
     205,   206,   207,   209,   203,   210,   212,   214,   225,   204,
     226,    88,   227,     0,   234,   236,     0,     0,   241,   250,
     252
  };

  const short
  SdfParse::yycheck_[] =
  {
      15,   156,    17,    18,   144,   145,   172,    15,   195,    17,
     197,    43,    44,    88,   169,   170,   171,    49,   158,   159,
     160,   161,   162,   163,    56,    43,    44,   167,   168,   216,
      49,    49,    41,    50,    51,   201,    55,    48,    56,   194,
      49,    42,    57,   183,   231,   200,   186,     3,    49,   204,
      24,    52,   239,   193,    18,   242,   196,   223,   245,   134,
      75,     0,    26,   232,    48,    57,    48,   222,    60,    43,
      44,   240,    21,    22,    23,    49,    25,   217,   233,    41,
     220,    41,    56,    49,    76,    77,    43,    44,   228,    55,
      39,    40,    49,    44,    42,    42,    42,    49,   238,    56,
      48,    48,    48,    55,   244,    41,   246,    41,   248,   121,
     122,   123,   124,   125,   126,   127,   128,   129,    41,    43,
      44,    50,    51,    41,    53,    55,   138,   139,   140,   141,
     142,   143,    56,    49,   146,   147,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    49,   164,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    50,    51,    49,    53,    49,    50,    51,    37,    38,
      39,    48,    49,    48,    49,    48,    49,    48,    49,    19,
      20,    48,    49,    48,    49,    46,    47,    37,    38,    48,
      49,    48,    49,    48,    49,    48,    49,    48,    49,    48,
      49,    48,    49,    48,    49,    49,    49,    49,    49,    42,
      49,   236,    49,    49,    48,    15,    41,    49,    16,    48,
      55,    17,    49,    49,    55,    55,    42,    48,    42,    44,
      42,    49,    49,    25,    45,    48,     6,    21,   119,   157,
      49,    49,    49,    49,    54,    49,    49,    49,    49,   176,
      49,    79,    49,    -1,    49,    48,    -1,    -1,    49,    49,
      49
  };

  const signed char
  SdfParse::yystos_[] =
  {
       0,    48,    58,     3,     0,    48,    59,    60,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    48,
      60,    63,    64,    41,    41,    41,    41,    41,    41,    50,
      51,    61,    43,    44,    49,    56,    62,    97,    98,    41,
      49,    49,    97,    98,    98,    15,    48,    49,    64,    49,
      49,    49,    49,    49,    49,    49,    44,    55,    49,    49,
      55,    49,    49,    49,    42,    48,    65,    62,    98,    62,
      98,    49,    16,    48,    66,    55,    55,    55,    41,    17,
      67,    98,    62,    62,    49,    42,    49,    52,    74,    48,
      49,    68,    49,    49,    61,    18,    26,    42,    69,    79,
      48,    49,    70,    48,    49,    80,    19,    20,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    71,    72,
      81,    82,    83,    84,    85,    86,    88,    89,    87,    90,
      73,    73,    42,    48,    74,    91,    92,    93,    95,    95,
      95,    95,    95,    95,    95,    95,    95,    95,    48,    49,
      75,    49,    53,    37,    38,    39,    94,    61,    95,    95,
      95,    95,    95,    95,    48,    96,    96,    95,    95,    21,
      22,    23,    25,    39,    40,    44,    46,    47,    92,    42,
      91,    96,    96,    96,    96,    96,    96,    49,    97,    98,
      49,    49,    96,    96,    92,    92,    78,    92,    96,    48,
      93,    45,    48,    54,    94,    49,    49,    49,    96,    49,
      49,    96,    49,    49,    49,    96,    92,    78,    49,    96,
      78,    92,    93,    25,    92,    49,    49,    49,    78,    49,
      49,    76,    92,    93,    49,    49,    48,    77,    78,    76,
      92,    49,    24,    49,    78,    76,    78,    49,    78,    49,
      49,    49,    49
  };

  const signed char
  SdfParse::yyr1_[] =
  {
       0,    57,    58,    59,    59,    60,    60,    60,    60,    60,
      60,    60,    60,    60,    60,    60,    60,    60,    60,    60,
      60,    61,    61,    62,    62,    63,    63,    64,    65,    66,
      66,    66,    67,    67,    68,    68,    69,    69,    71,    70,
      72,    70,    73,    73,    74,    74,    75,    75,    75,    75,
      75,    75,    75,    76,    76,    77,    78,    78,    79,    79,
      81,    80,    82,    80,    83,    80,    84,    80,    85,    80,
      86,    80,    87,    80,    88,    80,    89,    80,    90,    80,
      91,    91,    92,    92,    93,    93,    94,    94,    95,    95,
      95,    96,    96,    96,    97,    97,    97,    98,    98,    98
  };

  const signed char
  SdfParse::yyr2_[] =
  {
       0,     2,     5,     1,     2,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     3,     4,     3,     4,     4,     3,
       5,     1,     1,     0,     1,     1,     2,     6,     4,     3,
       4,     4,     0,     2,     4,     4,     0,     2,     0,     5,
       0,     5,     0,     2,     1,     3,     7,    10,     9,     6,
       5,     4,     5,     0,     2,     4,     1,     2,     0,     2,
       0,     7,     0,     7,     0,     8,     0,     7,     0,     7,
       0,     8,     0,     7,     0,     6,     0,     6,     0,     8,
       1,     4,     1,     3,     1,     4,     1,     1,     1,     3,
       7,     2,     3,     3,     5,     5,     5,     1,     1,     2
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const SdfParse::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "DELAYFILE",
  "SDFVERSION", "DESIGN", "DATE", "VENDOR", "PROGRAM", "PVERSION",
  "DIVIDER", "VOLTAGE", "PROCESS", "TEMPERATURE", "TIMESCALE", "CELL",
  "CELLTYPE", "INSTANCE", "DELAY", "ABSOLUTE", "INCREMENTAL",
  "INTERCONNECT", "PORT", "DEVICE", "RETAIN", "IOPATH", "TIMINGCHECK",
  "SETUP", "HOLD", "SETUPHOLD", "RECOVERY", "REMOVAL", "RECREM", "WIDTH",
  "PERIOD", "SKEW", "NOCHANGE", "POSEDGE", "NEGEDGE", "COND", "CONDELSE",
  "QSTRING", "ID", "FNUMBER", "DNUMBER", "EXPR_OPEN_IOPATH", "EXPR_OPEN",
  "EXPR_ID_CLOSE", "'('", "')'", "'/'", "'.'", "'*'", "'['", "']'", "':'",
  "'-'", "$accept", "file", "header", "header_stmt", "hchar", "number_opt",
  "cells", "cell", "celltype", "cell_instance", "timing_specs",
  "timing_spec", "deltypes", "deltype", "$@1", "$@2", "del_defs", "path",
  "del_def", "retains", "retain", "delval_list", "tchk_defs", "tchk_def",
  "$@3", "$@4", "$@5", "$@6", "$@7", "$@8", "$@9", "$@10", "$@11", "$@12",
  "port", "port_instance", "port_spec", "port_transition", "port_tchk",
  "value", "triple", "NUMBER", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  SdfParse::yyrline_[] =
  {
       0,   100,   100,   104,   105,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   129,   131,   135,   136,   140,   141,   145,   150,   155,
     157,   159,   163,   165,   169,   170,   173,   174,   179,   178,
     182,   181,   186,   187,   191,   193,   198,   200,   203,   206,
     208,   210,   212,   216,   218,   222,   227,   229,   233,   234,
     238,   238,   243,   243,   248,   248,   253,   253,   258,   258,
     263,   263,   268,   268,   274,   274,   279,   279,   284,   284,
     292,   294,   299,   300,   305,   307,   312,   313,   317,   318,
     320,   325,   329,   333,   337,   342,   347,   355,   356,   358
  };

  void
  SdfParse::yy_stack_print_ () const
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
  SdfParse::yy_reduce_print_ (int yyrule) const
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

  SdfParse::symbol_kind_type
  SdfParse::yytranslate_ (int t) YY_NOEXCEPT
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
      48,    49,    52,     2,     2,    56,    51,    50,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    55,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    53,     2,    54,     2,     2,     2,     2,     2,     2,
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
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47
    };
    // Last valid token kind.
    const int code_max = 302;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 49 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"
} // sta
#line 1655 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/SdfParse.cc"

#line 362 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/sdf/SdfParse.yy"

