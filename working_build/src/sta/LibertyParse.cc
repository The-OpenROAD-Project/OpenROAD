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
#line 25 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"

#include <cstdlib>

#include "Report.hh"
#include "liberty/LibertyParser.hh"
#include "liberty/LibertyScanner.hh"

#undef yylex
#define yylex scanner->lex

// warning: variable 'yynerrs_' set but not used
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

#define loc_line(loc) loc.begin.line

void
sta::LibertyParse::error(const location_type &loc,
                         const std::string &msg)
{
  reader->report()->fileError(164, reader->filename().c_str(),
                              loc.begin.line, "%s", msg.c_str());
}


#line 66 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"


#include "LibertyParse.hh"




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

#line 53 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
namespace sta {
#line 164 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"

  /// Build a parser object.
  LibertyParse::LibertyParse (LibertyScanner *scanner_yyarg, LibertyParser *reader_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      scanner (scanner_yyarg),
      reader (reader_yyarg)
  {}

  LibertyParse::~LibertyParse ()
  {}

  LibertyParse::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/

  // basic_symbol.
  template <typename Base>
  LibertyParse::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  LibertyParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  LibertyParse::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (value_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}


  template <typename Base>
  LibertyParse::symbol_kind_type
  LibertyParse::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  LibertyParse::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  LibertyParse::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_kind.
  LibertyParse::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  LibertyParse::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  LibertyParse::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  LibertyParse::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  void
  LibertyParse::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  void
  LibertyParse::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  LibertyParse::symbol_kind_type
  LibertyParse::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  LibertyParse::symbol_kind_type
  LibertyParse::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }



  // by_state.
  LibertyParse::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  LibertyParse::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  LibertyParse::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  LibertyParse::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  LibertyParse::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  LibertyParse::symbol_kind_type
  LibertyParse::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  LibertyParse::stack_symbol_type::stack_symbol_type ()
  {}

  LibertyParse::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  LibertyParse::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  LibertyParse::stack_symbol_type&
  LibertyParse::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  LibertyParse::stack_symbol_type&
  LibertyParse::stack_symbol_type::operator= (stack_symbol_type& that)
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
  LibertyParse::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YY_USE (yysym.kind ());
  }

#if YYDEBUG
  template <typename Base>
  void
  LibertyParse::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
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
  LibertyParse::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  LibertyParse::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  LibertyParse::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  LibertyParse::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  LibertyParse::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  LibertyParse::debug_level_type
  LibertyParse::debug_level () const
  {
    return yydebug_;
  }

  void
  LibertyParse::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  LibertyParse::state_type
  LibertyParse::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  LibertyParse::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  LibertyParse::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  LibertyParse::operator() ()
  {
    return parse ();
  }

  int
  LibertyParse::parse ()
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
  case 2: // file: group
#line 92 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 638 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 3: // $@1: %empty
#line 97 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { reader->groupBegin((yystack_[3].value.string), nullptr, loc_line(yystack_[3].location)); }
#line 644 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 4: // group: KEYWORD '(' ')' '{' $@1 '}' semi_opt
#line 99 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->groupEnd(); }
#line 650 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 5: // $@2: %empty
#line 101 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { reader->groupBegin((yystack_[3].value.string), nullptr, loc_line(yystack_[3].location)); }
#line 656 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 6: // group: KEYWORD '(' ')' '{' $@2 statements '}' semi_opt
#line 103 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->groupEnd(); }
#line 662 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 7: // $@3: %empty
#line 105 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { reader->groupBegin((yystack_[4].value.string), (yystack_[2].value.attr_values), loc_line(yystack_[4].location)); }
#line 668 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 8: // group: KEYWORD '(' attr_values ')' '{' $@3 '}' semi_opt
#line 107 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->groupEnd(); }
#line 674 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 9: // $@4: %empty
#line 109 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { reader->groupBegin((yystack_[4].value.string), (yystack_[2].value.attr_values), loc_line(yystack_[4].location)); }
#line 680 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 10: // group: KEYWORD '(' attr_values ')' '{' $@4 statements '}' semi_opt
#line 111 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->groupEnd(); }
#line 686 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 13: // statement: simple_attr
#line 120 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 692 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 14: // statement: complex_attr
#line 121 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 698 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 15: // statement: group
#line 122 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 704 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 16: // statement: variable
#line 123 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = (yystack_[0].value.stmt); }
#line 710 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 17: // simple_attr: KEYWORD ':' attr_value semi_opt
#line 128 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->makeSimpleAttr((yystack_[3].value.string), (yystack_[1].value.attr_value), loc_line(yystack_[3].location)); }
#line 716 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 18: // complex_attr: KEYWORD '(' ')' semi_opt
#line 133 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->makeComplexAttr((yystack_[3].value.string), nullptr, loc_line(yystack_[3].location)); }
#line 722 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 19: // complex_attr: KEYWORD '(' attr_values ')' semi_opt
#line 135 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->makeComplexAttr((yystack_[4].value.string), (yystack_[2].value.attr_values), loc_line(yystack_[4].location)); }
#line 728 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 20: // attr_values: attr_value
#line 140 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.attr_values) = new sta::LibertyAttrValueSeq;
	  (yylhs.value.attr_values)->push_back((yystack_[0].value.attr_value));
	}
#line 736 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 21: // attr_values: attr_values ',' attr_value
#line 144 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yystack_[2].value.attr_values)->push_back((yystack_[0].value.attr_value));
	  (yylhs.value.attr_values) = (yystack_[2].value.attr_values);
	}
#line 744 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 22: // attr_values: attr_values attr_value
#line 148 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yystack_[1].value.attr_values)->push_back((yystack_[0].value.attr_value));
	  (yylhs.value.attr_values) = (yystack_[1].value.attr_values);
	}
#line 752 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 23: // variable: string '=' FLOAT semi_opt
#line 155 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.stmt) = reader->makeVariable((yystack_[3].value.string), (yystack_[1].value.number), loc_line(yystack_[3].location)); }
#line 758 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 24: // string: STRING
#line 160 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 764 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 25: // string: KEYWORD
#line 162 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 770 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 26: // attr_value: FLOAT
#line 167 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.attr_value) = reader->makeFloatAttrValue((yystack_[0].value.number)); }
#line 776 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 27: // attr_value: expr
#line 169 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.attr_value) = reader->makeStringAttrValue((yystack_[0].value.string)); }
#line 782 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 28: // attr_value: volt_expr
#line 171 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.attr_value) = reader->makeStringAttrValue((yystack_[0].value.string)); }
#line 788 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 29: // volt_expr: FLOAT volt_op FLOAT
#line 178 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("%e%c%e", (yystack_[2].value.number), (yystack_[1].value.ch), (yystack_[0].value.number)); }
#line 794 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 30: // volt_expr: string volt_op FLOAT
#line 180 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("%s%c%e", (yystack_[2].value.string), (yystack_[1].value.ch), (yystack_[0].value.number));
          sta::stringDelete((yystack_[2].value.string));
        }
#line 802 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 31: // volt_expr: FLOAT volt_op string
#line 184 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("%e%c%s", (yystack_[2].value.number), (yystack_[1].value.ch), (yystack_[0].value.string));
          sta::stringDelete((yystack_[0].value.string));
        }
#line 810 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 32: // volt_expr: volt_expr volt_op FLOAT
#line 188 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("%s%c%e", (yystack_[2].value.string), (yystack_[1].value.ch), (yystack_[0].value.number));
          sta::stringDelete((yystack_[2].value.string));
        }
#line 818 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 33: // volt_op: '+'
#line 195 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '+'; }
#line 824 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 34: // volt_op: '-'
#line 197 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '-'; }
#line 830 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 35: // volt_op: '*'
#line 199 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '*'; }
#line 836 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 36: // volt_op: '/'
#line 201 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '/'; }
#line 842 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 37: // expr: expr_term1
#line 205 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 848 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 38: // expr: expr_term1 expr_op expr
#line 207 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("%s%c%s", (yystack_[2].value.string), (yystack_[1].value.ch), (yystack_[0].value.string));
          sta::stringDelete((yystack_[2].value.string));
          sta::stringDelete((yystack_[0].value.string));
        }
#line 857 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 39: // expr_term: string
#line 214 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 863 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 40: // expr_term: '0'
#line 216 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("0"); }
#line 869 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 41: // expr_term: '1'
#line 218 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("1"); }
#line 875 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 42: // expr_term: '(' expr ')'
#line 220 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("(%s)", (yystack_[1].value.string));
          sta::stringDelete((yystack_[1].value.string));
        }
#line 883 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 43: // expr_term1: expr_term
#line 226 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = (yystack_[0].value.string); }
#line 889 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 44: // expr_term1: '!' expr_term
#line 228 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("!%s", (yystack_[0].value.string));
          sta::stringDelete((yystack_[0].value.string));
        }
#line 897 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 45: // expr_term1: expr_term '\''
#line 232 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.string) = sta::stringPrint("%s'", (yystack_[1].value.string));
          sta::stringDelete((yystack_[1].value.string));
        }
#line 905 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 46: // expr_op: '+'
#line 239 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '+'; }
#line 911 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 47: // expr_op: '|'
#line 241 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '|'; }
#line 917 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 48: // expr_op: '*'
#line 243 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '*'; }
#line 923 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 49: // expr_op: '&'
#line 245 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '&'; }
#line 929 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;

  case 50: // expr_op: '^'
#line 247 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
        { (yylhs.value.ch) = '^'; }
#line 935 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"
    break;


#line 939 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"

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
  LibertyParse::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  LibertyParse::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const signed char LibertyParse::yypact_ninf_ = -64;

  const signed char LibertyParse::yytable_ninf_ = -8;

  const signed char
  LibertyParse::yypact_[] =
  {
       3,     5,    38,   -64,    55,   -64,    10,    99,   -64,   -64,
      87,    34,   -64,   -64,    15,    99,   -64,    99,   -64,    25,
     107,   -64,   -64,   -64,   -64,   -64,   -64,    60,    40,    42,
      47,    74,   -64,    50,    80,   -64,   -64,   -64,   -64,   -64,
     -64,    87,   -64,   -64,   -64,    75,    62,    76,   -64,   -64,
     -64,   -64,   -64,    -4,   -64,    -5,   -64,   -64,   -64,   -64,
      78,    77,    62,    83,    68,    74,   -64,   -64,    93,   -64,
      45,   -64,    34,    32,   -64,    83,   -64,    83,   -64,    83,
      47,    83,    83,    83,    83
  };

  const signed char
  LibertyParse::yydefact_[] =
  {
       0,     0,     0,     2,     0,     1,     0,    26,    24,    25,
       0,     0,    40,    41,     0,    39,    20,    28,    27,    43,
      37,    39,    44,    33,    34,    35,    36,     0,     0,     5,
       0,     0,    22,     0,     0,    45,    46,    47,    48,    49,
      50,     0,    29,    31,    42,     0,     0,     9,    21,    30,
      32,    38,    51,    25,    15,     0,    11,    13,    14,    16,
       0,     0,     0,     4,     0,     0,    51,    12,     0,    51,
       0,    52,    51,     0,    51,     6,    51,     8,    51,    18,
      51,    17,    23,    10,    19
  };

  const signed char
  LibertyParse::yypgoto_[] =
  {
     -64,   -64,   111,   -64,   -64,   -64,   -64,    52,   -50,   -64,
     -64,    53,   -64,    -6,   -13,   -64,    24,    -8,   112,   -64,
     -64,   -63
  };

  const signed char
  LibertyParse::yydefgoto_[] =
  {
       0,     2,    54,    45,    46,    61,    62,    55,    56,    57,
      58,    14,    59,    15,    16,    17,    27,    18,    19,    20,
      41,    63
  };

  const signed char
  LibertyParse::yytable_[] =
  {
      21,    32,    28,    75,    21,    67,    77,     8,    53,    79,
      64,    81,    66,    82,    65,    83,     1,    84,    48,     4,
      67,    43,     8,     9,    10,     6,     7,     8,     9,    10,
      30,    12,    13,    51,    31,    21,    12,    13,     5,    33,
      60,    34,     6,     7,     8,     9,    10,    80,    35,    60,
      29,    31,    74,    12,    13,    44,    60,     8,    53,    -3,
      32,    49,    78,    47,    60,     6,     7,     8,     9,    10,
      11,    42,     8,     9,     8,    53,    12,    13,     6,     7,
       8,     9,    10,    72,     6,     7,     8,     9,    10,    12,
      13,    50,    52,    -7,    69,    12,    13,     6,    68,     8,
       9,    10,    23,    24,    76,    25,    26,    71,    12,    13,
      36,     3,    37,    38,    70,    39,    40,    73,    22
  };

  const signed char
  LibertyParse::yycheck_[] =
  {
       6,    14,    10,    66,    10,    55,    69,    12,    13,    72,
      14,    74,    17,    76,    18,    78,    13,    80,    31,    14,
      70,    27,    12,    13,    14,    10,    11,    12,    13,    14,
      15,    21,    22,    41,    19,    41,    21,    22,     0,    15,
      46,    17,    10,    11,    12,    13,    14,    15,    23,    55,
      16,    19,    65,    21,    22,    15,    62,    12,    13,    17,
      73,    11,    17,    16,    70,    10,    11,    12,    13,    14,
      15,    11,    12,    13,    12,    13,    21,    22,    10,    11,
      12,    13,    14,    15,    10,    11,    12,    13,    14,    21,
      22,    11,    17,    17,    17,    21,    22,    10,    20,    12,
      13,    14,     3,     4,    11,     6,     7,    24,    21,    22,
       3,     0,     5,     6,    62,     8,     9,    64,     6
  };

  const signed char
  LibertyParse::yystos_[] =
  {
       0,    13,    26,    27,    14,     0,    10,    11,    12,    13,
      14,    15,    21,    22,    36,    38,    39,    40,    42,    43,
      44,    38,    43,     3,     4,     6,     7,    41,    42,    16,
      15,    19,    39,    41,    41,    23,     3,     5,     6,     8,
       9,    45,    11,    38,    15,    28,    29,    16,    39,    11,
      11,    42,    17,    13,    27,    32,    33,    34,    35,    37,
      38,    30,    31,    46,    14,    18,    17,    33,    20,    17,
      32,    24,    15,    36,    39,    46,    11,    46,    17,    46,
      15,    46,    46,    46,    46
  };

  const signed char
  LibertyParse::yyr1_[] =
  {
       0,    25,    26,    28,    27,    29,    27,    30,    27,    31,
      27,    32,    32,    33,    33,    33,    33,    34,    35,    35,
      36,    36,    36,    37,    38,    38,    39,    39,    39,    40,
      40,    40,    40,    41,    41,    41,    41,    42,    42,    43,
      43,    43,    43,    44,    44,    44,    45,    45,    45,    45,
      45,    46,    46
  };

  const signed char
  LibertyParse::yyr2_[] =
  {
       0,     2,     1,     0,     7,     0,     8,     0,     8,     0,
       9,     1,     2,     1,     1,     1,     1,     4,     4,     5,
       1,     3,     2,     4,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     1,     1,     1,     1,     1,     3,     1,
       1,     1,     3,     1,     2,     2,     1,     1,     1,     1,
       1,     0,     2
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const LibertyParse::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "'+'", "'-'", "'|'",
  "'*'", "'/'", "'&'", "'^'", "'!'", "FLOAT", "STRING", "KEYWORD", "'('",
  "')'", "'{'", "'}'", "':'", "','", "'='", "'0'", "'1'", "'\\''", "';'",
  "$accept", "file", "group", "$@1", "$@2", "$@3", "$@4", "statements",
  "statement", "simple_attr", "complex_attr", "attr_values", "variable",
  "string", "attr_value", "volt_expr", "volt_op", "expr", "expr_term",
  "expr_term1", "expr_op", "semi_opt", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const unsigned char
  LibertyParse::yyrline_[] =
  {
       0,    92,    92,    97,    96,   101,   100,   105,   104,   109,
     108,   115,   116,   120,   121,   122,   123,   127,   132,   134,
     139,   143,   147,   154,   159,   161,   166,   168,   170,   177,
     179,   183,   187,   194,   196,   198,   200,   205,   206,   214,
     215,   217,   219,   226,   227,   231,   238,   240,   242,   244,
     246,   250,   252
  };

  void
  LibertyParse::yy_stack_print_ () const
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
  LibertyParse::yy_reduce_print_ (int yyrule) const
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

  LibertyParse::symbol_kind_type
  LibertyParse::yytranslate_ (int t) YY_NOEXCEPT
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
       2,     2,     2,    10,     2,     2,     2,     2,     8,    23,
      14,    15,     6,     3,    19,     4,     2,     7,    21,    22,
       2,     2,     2,     2,     2,     2,     2,     2,    18,    24,
       2,    20,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     9,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    16,     5,    17,     2,     2,     2,     2,
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
       2,     2,     2,     2,     2,     2,     1,     2,    11,    12,
      13
    };
    // Last valid token kind.
    const int code_max = 260;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

#line 53 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"
} // sta
#line 1357 "/home/memzfs_projects/MLBuf_extension/OR_latest/build/src/sta/LibertyParse.cc"

#line 255 "/home/memzfs_projects/MLBuf_extension/OR_latest/src/sta/liberty/LibertyParse.yy"

